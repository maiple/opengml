#ifdef OPTIMIZE_PARSE
#ifdef __GNUC__
#pragma GCC optimize ("O3")
#endif
#endif

#include "Parser.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"

#include "ogm/common/error.hpp"
#include <regex>

using namespace ogm;

// order of operations. Higher values are stronger (tighter-binding).
// Some of these need to be checked.
uint32_t operator_priority(const Token& op)
{
    // note: ternary operators should all have priority 1.
    if (op.value == "?")
        return 1;

    if (op.value == "||" || op.value == "or")
        return 2;

    if (op.value == "&&" || op.value == "and")
        return 3;

    if (op.value == "^^" || op.value == "xor")
        return 4;

    if (op.value == "="
     || op.value == "=="
     || op.value == "!="
     || op.value == "<>"
     || op.value == "<="
     || op.value == "<"
     || op.value == ">="
     || op.value == ">")
        return 10;

    if (op.value == "|")
        return 20;

    if (op.value == "&")
        return 21;

    if (op.value == "^")
        return 22;

    if (op.value == "<<"
     || op.value == ">>")
        return 30;

    if (op.value == "+")
        return 31;

    if (op.value == "-")
        return 32;

    if (op.value == "mod" || op.value == "%")
        return 40;

    if (op.value == "div")
        return 41;

    if (op.value == "*")
        return 50;

    if (op.value == "/")
        return 51;

    if (op.value == "!"
     || op.value == "~")
        return 80;

    if (op.value == "++" || op.value == "--")
        return 90;

    ogm_assert(false);
    return 100;
}

using namespace std;

Parser::Parser(istream* is, int flags)
    : ts(is, flags, 2)
    , ignore_decor(flags & 0x1)
{ }
Parser::Parser(std::string s, int flags)
    : ts(s, flags, 2)
    , ignore_decor(flags & 0x1)
{ }

void Parser::assert_peek(ErrorCode::P error_code, const CmpToken& t, std::string message) const {
  if (ts.peek().type != t.type) {
    throw ParseError(error_code, ts.location(), "{}", replace_all(message, "%unexpected", "unexpected " + std::string(TOKEN_NAME_PLAIN[ts.peek().type])));
  }
  if (ts.peek().value != t.value) {
    throw ParseError(error_code, ts.location(), "{}", replace_all(message, "%unexpected", "unexpected token \"" + ts.peek().value + "\""));
  }
}

Production* Parser::read() {
  if (ts.peek().type == END || ts.peek().type == ERR)
    return nullptr;
  return read_production();
}

PrBodyContainer* Parser::parse() {
    PrBodyContainer* p = new PrBodyContainer();
    p->m_start = ts.location();

    // parse first body.
    PrBody* body = read_block(false, true);
    if (body->productions.empty())
    {
        delete body;
    }
    else
    {
        p->bodies.push_back(body);
        p->names.push_back("");
    }

    // read additional defines
    while (!ts.eof() && ts.peek().type == PPDEF)
    {
        std::string name = *ts.read().value;
        PrBody* body = read_block(false, true);
        p->bodies.push_back(body);
        p->names.push_back(name);
    }
    
    p->m_end = ts.location();
    
    return p;
}

PrExpression* Parser::parse_expression() {
    return read_expression();
}

Production* Parser::read_production() {
  
  if (ts.peek().type == PPMACRO)
  {
    PrStatement* p = read_macro_definition();
    return p;
  }
  else
  {
    PrStatement* p = read_statement();
    // read comments before final semicolon
    while (ts.peek().type == COMMENT) {
      LineColumn lc = ts.location();
      p->infixes.push_back(new PrInfixWS(ts.read(), lc));
      p->infixes.back()->m_end = ts.location();
    }
    read_statement_end();
    ignoreWS(p, true);
    removeExtraNewline(p);
    return p;
  }
}

PrDecor* Parser::read_rawtoken() {
  LineColumn lc = ts.location();
  auto* tr = new PrDecor(ts.read(), lc);
  tr->m_end = ts.location();
  return tr;
}

#define REGEX_IDENT "[a-zA-Z_][a-zA-Z_0-9]*"

PrMacroDefinition* Parser::read_macro_definition() {
  ogm_assert(ts.peek().type == PPMACRO);
  LineColumn lc = ts.location();
  std::string s = *ts.peek().value;
  trim(s);
  ts.read();
  
  static std::regex re ("#macro\\s+((" REGEX_IDENT "):)?(" REGEX_IDENT ")\\s+(.*)");
  
  std::smatch sm;
  if (std::regex_match(s,sm,re))
  {
    ogm_assert(sm.size() == 5)
    cout << "matched.\n";
    
    std::string config = sm[2].str();
    std::string name = sm[3].str();
    std::string value = sm[4].str();
    trim(config);
    trim(name);
    trim(value);
    
    PrMacroDefinition* p = new PrMacroDefinition(config, name, value);
    p->m_start = lc;
    p->m_end = ts.location();
    return p;
  }
  else
  {
    throw ParseError(ErrorCode::P::ppmacrodef, lc, "invalid macro definition; {}", s);
  }
}

PrStatement* Parser::read_statement() {
  COWString value(ts.peek().value);
  switch (ts.peek().type) {
  case WS:
  case COMMENT:
  case ENX:
    return new PrEmptyStatement();
  case KW:
    if (value == "var" || value == "globalvar" || value == "static")
      return read_statement_var();
    if (value == "function")
    {
      std::unique_ptr<PrStatementFunctionLiteral> fn{
        new PrStatementFunctionLiteral{
          read_function_literal()
        }
      };
      
      if (fn->fn->name.value->length() == 0)
      {
        throw ParseError(ErrorCode::P::fndefname, fn->m_start, "function declaration requires name");
      }
      
      return fn.release();
    }
    else if (value == "if")
      return read_statement_if();
    else if (value == "for")
      return read_for();
    else if (value == "enum")
      return read_enum();
    else if (value == "while")
      return read_while();
    else if (value == "repeat")
      return read_repeat();
    else if (value == "do")
      return read_do();
    else if (value == "with")
      return read_with();
    else if (value == "switch")
      return read_switch();
    else if (value == "new")
      return read_statement_function();
    else if (value == "return") {
      LineColumn lc = ts.location();
      auto* p = new PrControl(ts.read(), nullptr, lc);
      ignoreWS(p);
      p->val = read_expression();
      siphonWS(p->val, p, true);
      p->m_end = ts.location();
      return p;
    } else if (value == "exit" || value == "continue" || value == "break") {
      LineColumn lc = ts.location();
      auto* p = new PrControl(ts.read(), nullptr, lc);
      ignoreWS(p, true);
      p->m_end = ts.location();
      return p;
    }
    else {
      throw ParseError(ErrorCode::P::stunkstartkw, ts.location(), "keyword {} cannot start a statement.", *value);
    }
  case PUNC:
    if (value == "(") {
      return read_assignment();
    }
    if (value == "{") {
      return read_block();
    }
  throw ParseError(ErrorCode::P::stpunc, ts.location(), "unexpected punctuation \"{}\" where a statement was expected.", *value);
  case ID:
    {
      if (peek_function())
        return read_statement_function();
      return read_assignment();
    }
  case OPR:
    return read_assignment();
  default:
    throw ParseError(ErrorCode::P::sttok, ts.location(), "unexpected token \"{}\" when a statement was expected instead.", *value);
  }
}

PrAssignment* Parser::read_assignment() {
  if (ts.peek().type == OPR) {
    LineColumn lc = ts.location();
    Token op = ts.read();
    PrExpression* lhs = read_term();
    auto* a = new PrAssignment(lhs,op,nullptr, lc);
    a->m_end = ts.location();
    return a;
  } else {
    LineColumn lc = ts.location();
    PrExpression* lhs = read_term();
    // check op of correct format:
    if (ts.peek().type != OPR && ts.peek().type != OP) {
      throw ParseError(ErrorCode::P::asstok, ts.location(), "unexpected token \"{}"
          "\" where an assignment operator was expected.", *ts.peek().value);
    }
    // read operator
    Token op = ts.read();
    PrAssignment* p = new PrAssignment(lhs,op,nullptr, lc);
    ignoreWS(p);
    PrExpression* rhs = nullptr;
    if (op.type != OPR) {
      rhs = read_expression();
      p->postfix_n = 0;
      siphonWS(rhs,p,true);
    }
    p->rhs = rhs;
    p->m_end = ts.location();
    return p;
  }
}

PrExpression* Parser::read_term(bool readAccessor, bool readPossessive) {
    PrExpression* to_return = nullptr;
    Token t(ts.peek());
    if (t == CmpToken(OP,"-")   || t == CmpToken(OP,"!") || t == CmpToken(OP,"+")
        || t == CmpToken(KW,"not") || t == CmpToken(OP,"~") || t.type == OPR)
    {
        // unary operator term
        LineColumn lc = ts.location();
        auto op = ts.read();
        to_return = new PrExprArithmetic(nullptr, op, read_term(), lc);
    }
    else
    {
        if (t.type == NUM || t.type == STR)
        {
            // literal
            LineColumn lc = ts.location();
            to_return = new PrLiteral(ts.read(), lc);
        }
        else if (t == CmpToken(PUNC,"("))
        {
          // parentheses expression
          to_return = read_expression_parentheses();
        }
        else if (t == CmpToken(PUNC,"["))
        {
          // parentheses expression
          to_return = read_array_literal();
        }
        else if (t == CmpToken(PUNC, "{"))
        {
            to_return = read_struct_literal();
        }
        else if (t == CmpToken(KW, "new"))
        {
            to_return = read_expression_function();
        }
        else if (t == CmpToken(KW, "function"))
        {
            to_return = read_function_literal();
        }
        else if (t.type == ID)
        {
          // function or identifier
          if (readAccessor && peek_function())
          {
              to_return = read_expression_function();
          }
          else
          {
              LineColumn lc = ts.location();
              to_return = new PrIdentifier(ts.read(), lc);
          }
        }
        else
        {
            throw ParseError(ErrorCode::P::termtok, ts.location(), "Unexpected token \"{}\" encountered when parsing a term.", *t.value);
        }
    }

    // read postfixes
    ignoreWS(to_return, true);

    to_return->m_end = ts.location();

    // optional right-hand modifiers (array, .)
    if (readPossessive)
    {
        to_return = read_possessive(to_return);
    }
    if (readAccessor)
    {
        to_return = read_accessors(to_return);
    }
    return to_return;
}

PrExpression* Parser::read_possessive(PrExpression* owner) {
  while (ts.peek() == CmpToken(PUNC, ".")) {
    LineColumn lc = ts.location();
    auto op = ts.read();
    PrExprArithmetic* p = new PrExprArithmetic(owner, op, read_term(false, false), lc);
    siphonWS(p->rhs,p,true);
    p->m_end = ts.location();
    owner = p;
  }
  return owner;
}

PrExpression* Parser::read_accessors(PrExpression* ds) {
  if (ts.peek() != CmpToken(PUNC,"["))
    return ds;

  PrAccessorExpression* a = new PrAccessorExpression();
  siphonWS(ds, a, false, true);
  a->ds = ds;
  LineColumn lc = ts.location();
  ts.read(); // [
  ignoreWS(a);
  // TODO: ogm_assert valid accessor symbol
  if (ts.peek().type == OPA || ts.peek() == CmpToken(OP,"|") || ts.peek() == CmpToken(OP,"?"))
    a->acc = *ts.read().value;
  a->m_start = lc;

READ_INDEX:
  ignoreWS(a);
  a->indices.push_back(read_expression());
  ignoreWS(a);
  if (ts.peek() == CmpToken(PUNC,",")) {
    ts.read();
    goto READ_INDEX;
  }

  assert_peek(ErrorCode::P::acc, CmpToken(PUNC,"]"), "%unexpected while parsing accessor; either \",\" or \"]\" expected");
  ts.read(); // ]

  ignoreWS(a, true);

  a->m_end = ts.location();

  return read_accessors(read_possessive(a));
}

PrExpression* Parser::read_expression(uint32_t priority)
{
  PrExpression* to_return = read_term();
  ignoreWS(to_return, true);
  to_return->m_end = ts.location();
  return read_arithmetic(priority, to_return);
}

// checks for operator after existing expression. Can return original expression.
PrExpression* Parser::read_arithmetic(uint32_t priority, PrExpression* lhs)
{
    Token t(ts.peek());
    if (t.type == OP || t.type == OPR || t.is_op_keyword())
    {
        if (t == CmpToken(OP,"!") || t == CmpToken(KW,"not") || t == CmpToken(OP,"~"))
        {
            throw ParseError(ErrorCode::P::exopun, ts.location(), "unexpected unary operator \"{}\" after expression", *t.value);
        }
        if (t.type == OPR)
        {
            // not allowed after () expression
            // e.g.,, we forbid (x)--.
            if (dynamic_cast<PrExprParen*>(lhs)) return lhs;
            LineColumn lc = ts.location();
            auto* a = new PrExprArithmetic(lhs, ts.read(), nullptr, lc);
            a->m_end = ts.location();
            return read_arithmetic(priority, a);
        }

        uint32_t rhs_priority = operator_priority(t);
        if (rhs_priority >= priority)
        {
            if (t == CmpToken(OP, "?"))
            {
                // ternary has special priority rules, because it's
                // not an ambiguous parse.
                return read_ternary(lhs);
            }
            else
            {
                // priority rules.
                Token op = ts.read();
                LineColumn lc = ts.location();
                PrExprArithmetic* p = new PrExprArithmetic(lhs, op, nullptr, lc);
                siphonWS(lhs,p,false,true);
                ignoreWS(p);
                p->rhs = read_expression(rhs_priority + 1);
                siphonWS(p->rhs,p,true);
                p->m_end = ts.location();
                return read_arithmetic(priority, p);
            }
        }
    }
    return lhs;
}

// Reads a ternary operator.
PrExpression* Parser::read_ternary(PrExpression* lhs)
{
    Token t(ts.peek());
    // check for ternary-start operator (currently only ?)
    if (t == CmpToken(OP, "?"))
    {
        Token op = ts.read();
        LineColumn lc = ts.location();
        PrTernary* p = new PrTernary();
        p->condition = lhs;
        siphonWS(lhs,p,false,true);
        ignoreWS(p);
        p->result = read_expression(0);
        siphonWS(p->result,p,false,true);
        ignoreWS(p);
        Token divider = ts.peek();
        assert_peek(ErrorCode::P::tern, CmpToken(PUNC,":"), "%unexpected while parsing ternary expression; \":\" expected");
        ts.read();
        p->otherwise = read_expression(0);
        siphonWS(p->otherwise,p,true);
        p->m_end = ts.location();

        // This may be unnecessary?
        read_arithmetic(0, p);

        return p;
    }
    return lhs;
}

PrExpressionFn* Parser::read_expression_function() {
  LineColumn lc = ts.location();
  std::unique_ptr<PrExpressionFn> pfn{
      new PrExpressionFn()
  };
  pfn->m_start = lc;
  
  if (ts.peek() == CmpToken(KW, "new"))
  {
      ts.read();
      pfn->m_new = true;
      ignoreWS(pfn.get());
  }
  
  pfn->callee = std::unique_ptr<PrExpression>(read_term(false));

  ignoreWS(pfn.get());

  // (
  assert_peek(ErrorCode::P::fnparen, CmpToken(PUNC,"("),"%unexpected while expecting open-parenthesis \"(\" for function");
  ts.read();

  while (true) {
    ignoreWS(pfn.get());
    Token next(ts.peek());
    if (next == CmpToken(PUNC,")"))
      break;
    pfn->args.emplace_back(read_expression());
    ignoreWS(pfn.get());
    next = ts.peek();
    if (next == CmpToken(PUNC,",")) {
      ts.read();
      continue;
    }
    else break;
  }

  // )
  assert_peek(ErrorCode::P::fnargs, CmpToken(PUNC,")"),"%unexpected while parsing function; expected \",\" or \")\"");
  ts.read();

  ignoreWS(pfn.get(), true);

  pfn->m_end = ts.location();

  return pfn.release();
}

PrStatementFn* Parser::read_statement_function() {
  LineColumn lc = ts.location();
  std::unique_ptr<PrStatementFn> fn{ new PrStatementFn() };
  fn->m_start = lc;
  fn->fn = std::unique_ptr<PrExpressionFn>{ read_expression_function() };
  siphonWS(fn->fn.get(),fn.get(),true);
  fn->m_end = ts.location();
  return fn.release();
}

PrExprParen* Parser::read_expression_parentheses() {
  LineColumn lc = ts.location();
  PrExprParen* p = new PrExprParen();
  p->m_start = lc;

  assert_peek(ErrorCode::P::paren, CmpToken(PUNC,"("),"%unexpected while expecting open parenthesis \"(\"");
  ts.read(); //(
  ignoreWS(p);

  p->content = read_expression();

  assert_peek(ErrorCode::P::parend, CmpToken(PUNC,")"),"%unexpected while expecting matching close parenthesis \"(\"");
  ts.read(); //)
  p->m_end = ts.location();
  return p;
}

PrFunctionLiteral* Parser::read_function_literal() {
  LineColumn lc = ts.location();
  std::unique_ptr<PrFunctionLiteral> p{ new PrFunctionLiteral() };
  p->m_start = lc;
  
  assert_peek(ErrorCode::P::litfnkw, CmpToken(KW,"function"),"%unexpected while expecting keyword \"function\"");
  
  ts.read(); //function
  
  ignoreWS(p.get());
  
  if (ts.peek().type == ID)
  {
    p->name = ts.read();
  }
  else
  {
    p->name = {ID, ""};
  }
  
  ignoreWS(p.get());
  
  assert_peek(ErrorCode::P::litfnparen, CmpToken(PUNC,"("),"%unexpected while expecting open-parenthesis \"(\" for function literal");
  ts.read(); //(
  
  while (true) {
    ignoreWS(p.get());
    Token next(ts.peek());
    if (next == CmpToken(PUNC,")"))
      break;
    if (next.type != ID)
    {
      throw ParseError(ErrorCode::P::litfnarg, ts.location(), "Expected ID while parsing function literal arguments");
    }
    p->args.push_back(ts.read());
    ignoreWS(p.get());
    next = ts.peek();
    if (next == CmpToken(PUNC,",")) {
      ts.read();
      continue;
    }
    else break;
  }
  
  assert_peek(ErrorCode::P::litfnargs, CmpToken(PUNC,")"),"%unexpected while parsing function literal; expected \",\" or \")\"");
  ts.read(); //)
  
  ignoreWS(p.get());
  
  if (ts.peek() == CmpToken(KW, "constructor"))
  {
    p->constructor = true;
    ts.read(); //constructor
  }
  
  ignoreWS(p.get());
  
  assert_peek(ErrorCode::P::litfnbrace, CmpToken(PUNC, "{"), "%unexpected while parsing function literal; expected body open-brace \"{\"");
  p->body = std::unique_ptr<PrBody>( read_block(true, false) );
  
  p->m_end = ts.location();
  return p.release();
}

PrStructLiteral* Parser::read_struct_literal() {
    
    #ifndef OGM_STRUCT_SUPPORT
    throw ParseError(ErrorCode::P::hasstruct, ts.location(), "Structs support is not enabled. Please recompile ogm with -DOGM_STRUCT_SUPPORT=ON.");
    #endif
    
    LineColumn lc = ts.location();
    std::unique_ptr<PrStructLiteral> p{ new PrStructLiteral() };
    p->m_start = lc;
    
    assert_peek(ErrorCode::P::litstruct, CmpToken(PUNC,"{"),"%unexpected while expecting \"{\"");
    ts.read(); //{
        
    while (true)
    {
        ignoreWS(p.get());

        Token t = ts.peek();
        if (t == CmpToken(PUNC,"}"))
        {
            ts.read();
            break;
        }
        
        if (ts.peek().type != ID)
        {
            throw ParseError(ErrorCode::P::litobtok, ts.location(), "Unexpected token \"{}\" while reading object literal; expected member name.", *ts.peek().value);
        }
        
        LineColumn lc = ts.location();
        PrVarDeclaration* d = new PrVarDeclaration(ts.read(), nullptr, lc);
        
        ignoreWS(d);
        assert_peek(ErrorCode::P::litstructfield, CmpToken(PUNC,":"),"%unexpected while expecting \":\"");
        ts.read(); // :
        ignoreWS(d);
        d->definition = read_expression();
        d->m_end = ts.location();
        p->declarations.push_back(d);
        ignoreWS(p.get());

        t = ts.peek();
        if (t == CmpToken(PUNC,","))
        {
            ts.read();
            continue;
        }
        else if (t == CmpToken(PUNC,"}"))
        {
            ts.read();
            break;
        }
    }

    p->m_end = ts.location();
    return p.release();
}

PrArrayLiteral* Parser::read_array_literal() {
  LineColumn lc = ts.location();
  PrArrayLiteral* p = new PrArrayLiteral();
  p->m_start = lc;

  assert_peek(ErrorCode::P::litarr, CmpToken(PUNC,"["),"%unexpected while expecting \"[\"");
  ts.read(); //[
  while (true)
  {
      ignoreWS(p);

      Token t = ts.peek();
      if (t == CmpToken(PUNC,"]"))
      {
          ts.read();
          break;
      }
      p->vector.push_back(read_expression());

      ignoreWS(p);

      t = ts.peek();
      if (t == CmpToken(PUNC,","))
      {
          ts.read();
          continue;
      }
      else if (t == CmpToken(PUNC,"]"))
      {
          ts.read();
          break;
      }
  }

  p->m_end = ts.location();
  return p;
}

PrStatementVar* Parser::read_statement_var() {
  LineColumn lc = ts.location();
  PrStatementVar* p = new PrStatementVar();
  p->m_start = lc;
  if (ts.peek() != CmpToken(KW,"globalvar") && ts.peek() != CmpToken(KW, "static"))
    assert_peek(ErrorCode::P::varkw, CmpToken(KW,"var"),"%unexpected while expecting var declaration");
  while (true) {
    // it's permissible to have multiple "var" or "globalvar" keywords.
    if (ts.peek() == CmpToken(KW,"var") || ts.peek() == CmpToken(KW,"globalvar") || ts.peek() == CmpToken(KW,"static"))
    {
        p->types.push_back(*ts.read().value);
    }
    else
    {
        p->types.push_back("");
    }
    ignoreWS(p);
    if (ts.peek().type != ID)
    {
      //throw ParseError(ErrorCode::P::vartok, ts.location(), "Unexpected token \"{}\" while reading var declaration; expected variable name.", *ts.peek().value);
      
      PrVarDeclaration* d = new PrVarDeclaration(Token(TokenType::WS, ""), nullptr, ts.location());
      p->declarations.push_back(d);
      return p;
    }
    LineColumn lc = ts.location();
    PrVarDeclaration* d = new PrVarDeclaration(ts.read(), nullptr, lc);
    ignoreWS(d);
    if (ts.peek() == CmpToken(OP,"=")) {
      ts.read();
      ignoreWS(d);
      d->definition = read_expression();
    }
    ignoreWS(p);
    p->declarations.push_back(d);
    if (ts.peek() == CmpToken(PUNC,",")) {
      ts.read();
      ignoreWS(p);
      if (ts.peek() == CmpToken(ENX, ";"))
      {
          break;
      }
      continue;
    }
    break;
  }
  p->m_end = ts.location();
  return p;
}

PrStatementEnum* Parser::read_enum() {
  LineColumn lc = ts.location();
  PrStatementEnum* p = new PrStatementEnum();
  p->m_start = lc;
  // read the keyword "enum"
  const auto kw_enum = ts.read();
  ogm_assert(kw_enum == CmpToken(KW, "enum"));
  p->identifier = ts.read(); // read name of enum
  ogm_assert(p->identifier.type == ID);
  ignoreWS(p);

  // {
  ogm_assert(ts.peek().value == "{");
  ts.read();

  while (true) {
    ignoreWS(p);
    if (ts.peek() == CmpToken(PUNC, "}"))
    {
        // done
        break;
    }
    if (ts.peek().type != ID)
    {
        throw ParseError(ErrorCode::P::enumtok, ts.location(), "Unexpected token \"{}\" while reading enum; expected name.", *ts.peek().value);
    }
    LineColumn lc = ts.location();
    PrVarDeclaration* d = new PrVarDeclaration(ts.read(), nullptr, lc);
    ignoreWS(d);
    if (ts.peek() == CmpToken(OP,"=")) {
      ts.read();
      ignoreWS(d);
      d->definition = read_expression();
    }
    ignoreWS(p);
    p->declarations.push_back(d);
    if (ts.peek() == CmpToken(PUNC,",")) {
      ts.read();
      continue;
    }
    break;
  }

  // }
  ogm_assert(ts.peek().value == "}");
  ts.read();

  p->m_end = ts.location();

  return p;
}

PrStatementIf* Parser::read_statement_if() {
  LineColumn lc = ts.location();
  PrStatementIf* p = new PrStatementIf();
  p->m_start = lc;
  assert_peek(ErrorCode::P::ifkw, CmpToken(KW,"if"), "%unexpected; expected if statement");
  ts.read(); // read IF
  ignoreWS(p);
  p->condition = read_expression();
  siphonWS(p->condition,p,false,true);

  if (ts.peek() == CmpToken(KW, "then"))
  {
      // ignore this
      ts.read();
      p->has_then = true;
  }
  ignoreWS(p);

  p->result = read_statement();
  read_statement_end();
  ignoreWS(p->result,true);
  siphonWS(p->result,p,true,true);

  if (ts.peek() == CmpToken(KW, "else")) {
    // what were previously postfixes now count as infixes, since we're extending
    p->postfix_n = 0;
    ts.read();
    ignoreWS(p);
    p->otherwise = read_statement();
    siphonWS(p->otherwise, p, true);
    read_statement_end();
    ignoreWS(p, true);
  }

  p->m_end = ts.location();

  return p;
}

PrBody* Parser::read_block(bool braces, bool end_at_define) {
  LineColumn lc = ts.location();
  PrBody* p = new PrBody();
  p->m_start = lc;
  p->is_root = !braces;
  ogm_assert(!braces || !end_at_define);
  if (braces) {
    assert_peek(ErrorCode::P::blockbrace, CmpToken(PUNC,"{"), "expected open brace, \"{\"");
    ts.read(); // {
  }

  // read productions inside of braces
  while (
      !(braces && ts.peek() == CmpToken(PUNC,"}"))
    && !ts.eof()
    && !(end_at_define && ts.peek().type == PPDEF)
  )
  {
      p->productions.push_back(read_production());
  }
  if (end_at_define && ts.peek().type == PPDEF)
  {
      // #define statement ending
      p->m_end = ts.location();
      return p;
  }
  if (braces) {
    assert_peek(ErrorCode::P::blockbraced, CmpToken(PUNC,"}"), "expected matching end brace, \"}\"");
    ts.read(); // }
  }
  ignoreWS(p, true);

  p->m_end = ts.location();

  return p;
}

void Parser::ignoreWS(Production* p, bool as_postfix) {
  if (ignore_decor)
  {
    while (ts.peek() == CmpToken(ENX,"\n") || (ts.peek().type == COMMENT))
        ts.read();
    p->infixes.push_back(nullptr);
    return;
  }
  if (ts.peek() == CmpToken(ENX,"\n") || (ts.peek().type == COMMENT)) {

    LineColumn lc = ts.location();
    PrInfixWS* infix = new PrInfixWS(ts.read(), lc);
    ignoreWS(infix);
    infix->m_end = ts.location();
    p->infixes.push_back(infix);
  } else {
    p->infixes.push_back(nullptr);
  }
  p->postfix_n += as_postfix;
}

//! takes all postfixes from src and inserts them into dst
void Parser::siphonWS(Production* src, Production* dst, bool as_postfix, bool condense) {
  int N = src->postfix_n;
  PrInfixWS** infixes = new PrInfixWS*[max(N,1)];

  // remove infixes from src
  while (src->postfix_n > 0) {
    infixes[--src->postfix_n] = src->infixes.back();
    src->infixes.pop_back();
  }

  // condense all siphoned infixes into a single infix (if condense is true)
  if (condense) {
    // find first not-null postfix
    int first_non_null = 0;
    for (int i=0;i<N;i++) {
      if (infixes[i] == nullptr)
        first_non_null++;
      else
        break;
    }

    // attach other postfixes as postfixes to the first non-null postfix:
    for (int i=first_non_null+1;i<N;i++) {
      infixes[first_non_null]->infixes.push_back(infixes[i]);
      infixes[first_non_null]->postfix_n++;
    }

    // edge cases:
    if (N==0) {
      infixes[0] = nullptr;
    } else if (first_non_null < N) {
      infixes[0] = infixes[first_non_null];
    }
    N = 1;
  }

  // append siphoned infixes to dst
  for (int i=0;i<N;i++) {
    dst->infixes.push_back(infixes[i]);
    dst->postfix_n += as_postfix;
  }

  delete[] infixes;
}

// statements generally are followed by a newline.
// This function discards exactly one newline from the end of the
// whitespace postfixes.
void Parser::removeExtraNewline(Production* p) {
  //TODO: rewrite this to use iterators
  auto& infixes = p->infixes;
  auto& postfix_n = p->postfix_n;

  p->flattenPostfixes();

  for (int i=0;i<postfix_n;i++) {
    int iter = infixes.size() - i - 1;
    if (infixes[iter]) {
      if (infixes[iter]->val.value == "\n") {
        delete(infixes[iter]);
        infixes[iter] = nullptr;
        return;
      } else {
        return;
      }
    }
  }
}

PrFor* Parser::read_for() {
  LineColumn lc = ts.location();
  PrFor* pfor = new PrFor();
  pfor->m_start = lc;
  assert_peek(ErrorCode::P::forkw, CmpToken(KW,"for"), "%unexpected; expected \"for\" statement.");
  ts.read(); // for
  ignoreWS(pfor);
  assert_peek(ErrorCode::P::forparen, CmpToken(PUNC,"("), "%unexpected where open parenthesis \"(\" for for statement arguments expected.");
  ts.read(); //(

  ignoreWS(pfor);
  pfor->init = read_statement();
  read_statement_end();
  ignoreWS(pfor);

  if (ts.peek() != CmpToken(ENX,";"))
    pfor->condition = read_expression();
  else
    pfor->condition = nullptr;
  read_statement_end();

  ignoreWS(pfor);
  if (ts.peek() != CmpToken(PUNC,")")) {
    pfor->second = read_statement();
    read_statement_end();
  } else {
    LineColumn lc = ts.location();
    pfor->second = new PrEmptyStatement();
    pfor->m_start = lc;
  }
  ignoreWS(pfor);

  assert_peek(ErrorCode::P::forparend, CmpToken(PUNC,")"), "%unexpected where end parenthesis \")\" for for statement arguments expected.");
  ts.read(); //)

  ignoreWS(pfor);
  pfor->first = read_statement();
  siphonWS(pfor->first, pfor, true);
  pfor->m_end = ts.location();
  return pfor;
}

void Parser::read_statement_end() {
  while (ts.peek() == CmpToken(ENX,";")) {
    ts.read();
  }
}

static bool token_is_decoration(
    const Token& token
)
{
    if (token == CmpToken(ENX, ";")) return false;
    if (token.type == COMMENT || token.type == WS || token.type == ENX) return true;
    return false;
}

bool Parser::peek_function() {
    
    // try some common early-outs.
    if(ts.peek().type != ID) return false;
    if (ts.peek(1) == CmpToken(PUNC,"(")) {
      return true;
    }
    if (!token_is_decoration(ts.peek(1)))
    {
        return false;
    }

    // this is heavy stuff
    std::vector<Token> excess;
    excess.push_back(ts.read());
    excess.push_back(ts.read());
    bool rv;
    while (true)
    {
        if (!token_is_decoration(ts.peek()))
        {
            rv = (ts.peek() == CmpToken(PUNC,"("));
            break;
        }
        excess.push_back(ts.read());
    }

    while (!excess.empty())
    {
        ts.put_back(excess.back());
        excess.pop_back();
    }

    return rv;
}

PrWhile* Parser::read_while() {
  LineColumn lc = ts.location();
  PrWhile* p = new PrWhile();
  p->m_start = lc;
  assert_peek(ErrorCode::P::whilekw, CmpToken(KW,"while"), "%unexpected; expected \"while\" statement.");
  ts.read(); // while
  ignoreWS(p);
  p->condition = read_expression();
  siphonWS(p->condition, p, false, true);

  if (ts.peek() == CmpToken(KW, "do"))
  {
      ts.read();
      p->has_do = true;
  }
  ignoreWS(p);
  p->event = read_statement();
  siphonWS(p->event, p, true);
  p->m_end = ts.location();
  return p;
}

PrRepeat* Parser::read_repeat() {
  LineColumn lc = ts.location();
  PrRepeat* p = new PrRepeat();
  p->m_start = lc;
  assert_peek(ErrorCode::P::repkw, CmpToken(KW,"repeat"), "%unexpected; expected \"repeat\" statement.");
  ts.read(); // repeat
  ignoreWS(p);
  p->count = read_expression();
  siphonWS(p->count, p, false, true);
  p->event = read_statement();
  siphonWS(p->event, p, true);
  p->m_end = ts.location();
  return p;
}

PrDo* Parser::read_do() {
  LineColumn lc = ts.location();
  PrDo* p = new PrDo();
  p->m_start = lc;
  assert_peek(ErrorCode::P::dokw, CmpToken(KW,"do"), "%unexpected token; expected \"do\" statement.");
  ts.read(); // do
  ignoreWS(p);
  p->event = read_statement();
  read_statement_end();
  ignoreWS(p, true);
  siphonWS(p->event, p, false, true);
  assert_peek(ErrorCode::P::dountkw, CmpToken(KW,"until"), "%unexpected where \"until\" needed following \"do\"");
  ts.read(); // until
  ignoreWS(p);
  p->condition = read_expression();
  siphonWS(p->condition, p, true);
  p->m_end = ts.location();
  return p;
}


PrWith* Parser::read_with() {
  LineColumn lc = ts.location();
  PrWith* p = new PrWith();
  p->m_start = lc;
  assert_peek(ErrorCode::P::withkw, CmpToken(KW,"with"), "%unexpected; expected \"with\" statement.");
  ts.read(); // with
  ignoreWS(p);
  p->objid = read_expression();
  siphonWS(p->objid, p, false, true);
  p->event = read_statement();
  siphonWS(p->event, p, true);
  p->m_end = ts.location();
  return p;
}

PrSwitch* Parser::read_switch() {
  LineColumn lc = ts.location();
  PrSwitch* p = new PrSwitch();
  p->m_start = lc;

  assert_peek(ErrorCode::P::switchkw, CmpToken(KW,"switch"), "%unexpected; expected \"switch\" statement.");
  ts.read(); // switch
  ignoreWS(p);

  p->condition = read_expression();
  siphonWS(p->condition, p, false, true);

  assert_peek(ErrorCode::P::switchbrace, CmpToken(PUNC,"{"), "%unexpected where open brace \"{\" required for switch statement.");
  ts.read(); // {
  ignoreWS(p);

  while (true) {
    if (ts.peek() == CmpToken(PUNC,"}"))
        break;

    LineColumn lc = ts.location();
    PrCase* c = new PrCase();
    c->m_start = lc;
    if (ts.peek() != CmpToken(KW,"default"))
      assert_peek(ErrorCode::P::switchcasekw, CmpToken(KW,"case"), "%unexpected; switch statement requires cases.");
    Token t(ts.read()); // case
    ignoreWS(c);

    if (t==Token(KW,"case")) {
      c->value = read_expression();
      ignoreWS(c);
    } else {
      c->value = nullptr;
    }

    assert_peek(ErrorCode::P::switchcasepunc, CmpToken(PUNC,":"), "%unexpected where colon \":\" required for case.");
    ts.read(); //:
    ignoreWS(c);

    while (ts.peek()!=Token(KW,"case") &&
           ts.peek()!=Token(KW,"default") &&
           ts.peek()!=Token(PUNC,"}")) {
      c->productions.push_back(read_production());
    }
    if (!c->productions.empty())
      siphonWS(c->productions.back(), c, false, true);
    c->m_end = ts.location();
    p->cases.push_back(c);
  }

  assert_peek(ErrorCode::P::switchbraced, CmpToken(PUNC,"}"), "%unexpected; expected matching close brace \"}\" for switch statement.");
  ts.read(); // }
  ignoreWS(p, true);

  p->m_end = ts.location();
  return p;
}
