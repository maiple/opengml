#include "Parser.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"

#include <cassert>

using namespace std;

// order of operations. Higher values are stronger (tighter-binding).
// Some of these need to be checked.
uint32_t operator_priority(Token op)
{
    if (op.value == "||" || op.value == "or")
        return 1;

    if (op.value == "&&" || op.value == "and")
        return 2;

    if (op.value == "^^" || op.value == "xor")
        return 3;

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

    assert(false);
    return 100;
}

Parser::Parser(istream* is): ts(is, 2) { }
Parser::Parser(std::string s): ts(s, 2) { }

void Parser::assert_peek(Token t, std::string message) const {
  if (ts.peek().type != t.type) {
    throw ParseError(replace_all(message, "%unexpected", "unexpected " + std::string(TOKEN_NAME_PLAIN[ts.peek().type])), ts.location().pair());
  }
  if (ts.peek().value != t.value) {
    throw ParseError(replace_all(message, "%unexpected", "unexpected token \"" + ts.peek().value + "\""), ts.location().pair());
  }
}

Production* Parser::read() {
  if (ts.peek().type == END || ts.peek().type == ERR)
    return nullptr;
  return read_production();
}

PrBodyContainer* Parser::parse() {
    PrBodyContainer* p = new PrBodyContainer();

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
        std::string name = ts.read().value;
        PrBody* body = read_block(false, true);
        p->bodies.push_back(body);
        p->names.push_back(name);
    }
    return p;
}

PrExpression* Parser::parse_expression() {
    return read_expression();
}

Production* Parser::read_production() {

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

PrDecor* Parser::read_rawtoken() {
  LineColumn lc = ts.location();
  auto* tr = new PrDecor(ts.read(), lc);
  tr->m_end = ts.location();
  return tr;
}

PrStatement* Parser::read_statement() {
  string value(ts.peek().value);
  switch (ts.peek().type) {
  case WS:
  case COMMENT:
  case ENX:
    return new PrEmptyStatement();
  case KW:
    if (value == "var" || value == "globalvar")
      return read_statement_var();
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
      throw ParseError("keyword " + value + " cannot start a statement.", ts.location().pair());
    }
  case PUNC:
    if (value == "(") {
      return read_assignment();
    }
    if (value == "{") {
      return read_block();
    }
    throw ParseError("unexpected punctuation \"" + value + "\" where a statement was expected.", ts.location().pair());
  case ID:
    {
      Token t = ts.peek(1);
      if (t == Token(PUNC,"("))
        return read_statement_function();
      return read_assignment();
    }
  case OPR:
    return read_assignment();
  default:
    throw ParseError("unexpected token \"" + value + "\" when a statement was expected instead.", ts.location().pair());
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
    PrExpression* lhs = read_term();
    // check op of correct format:
    if (ts.peek().type != OPR && ts.peek().type != OP) {
      throw ParseError("unexpected token \"" + ts.peek().value
          + "\" where an assignment operator was expected.", ts.location().pair());
    }
    // read operator
    LineColumn lc = ts.location();
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
    if (t == Token(OP,"-")   || t == Token(OP,"!") || t == Token(OP,"+")
        || t == Token(KW,"not") || t == Token(OP,"~") || t.type == OPR)
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
            to_return = new PrFinal(ts.read(), lc);
        }
        if (t == Token(PUNC,"("))
        {
          // parentheses expression
          to_return = read_expression_parentheses();
        }
        else if (t.type == ID)
        {
          // function or identifier
          t = ts.peek(1);
          if (t == Token(PUNC,"("))
          {
              to_return = read_expression_function();
          }
          else
          {
              LineColumn lc = ts.location();
              to_return = new PrIdentifier(ts.read(), lc);
          }
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
  while (ts.peek() == Token(PUNC, ".")) {
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
  if (ts.peek() != Token(PUNC,"["))
    return ds;

  PrAccessorExpression* a = new PrAccessorExpression();
  siphonWS(ds, a, false, true);
  a->ds = ds;
  LineColumn lc = ts.location();
  ts.read(); // [
  ignoreWS(a);
  // TODO: assert valid accessor symbol
  if (ts.peek().type == OPA || ts.peek() == Token(OP,"|"))
    a->acc = ts.read().value;
  a->m_start = lc;

READ_INDEX:
  ignoreWS(a);
  a->indices.push_back(read_expression());
  ignoreWS(a);
  if (ts.peek() == Token(PUNC,",")) {
    ts.read();
    goto READ_INDEX;
  }

  assert_peek(Token(PUNC,"]"), "%unexpected while parsing accessor; either \",\" or \"]\" expected");
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
        if (t == Token(OP,"!") || t == Token(KW,"not") || t == Token(OP,"~"))
        {
            throw ParseError("unexpected unary operator after expression", ts.location().pair());
        }
        if (t.type == OPR)
        {
            LineColumn lc = ts.location();
            auto* a = new PrExprArithmetic(lhs, ts.read(), nullptr, lc);
            a->m_end = ts.location();
            return a;
        }

        uint32_t rhs_priority = operator_priority(t);
        if (rhs_priority >= priority)
        {
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
    return lhs;
}

PrExpressionFn* Parser::read_expression_function() {
  LineColumn lc = ts.location();
  PrExpressionFn* pfn = new PrExpressionFn(ts.read(), lc);

  ignoreWS(pfn);

  // (
  assert_peek(Token(PUNC,"("),"%unexpected while expecting open-parenthesis \"(\" for function");
  ts.read();

  while (true) {
    ignoreWS(pfn);
    Token next(ts.peek());
    if (next == Token(PUNC,")"))
      break;
    pfn->args.push_back(read_expression());
    ignoreWS(pfn);
    next = ts.peek();
    if (next == Token(PUNC,",")) {
      ts.read();
      continue;
    }
    else break;
  }

  // )
  assert_peek(Token(PUNC,")"),"%unexpected while parsing function; expected \",\" or \")\"");
  ts.read();

  ignoreWS(pfn, true);

  pfn->m_end = ts.location();

  return pfn;
}

PrStatementFn* Parser::read_statement_function() {
  LineColumn lc = ts.location();
  PrStatementFn* fn = new PrStatementFn();
  fn->m_start = lc;
  fn->fn = read_expression_function();
  siphonWS(fn->fn,fn,true);
  fn->m_end = ts.location();
  return fn;
}

PrExprParen* Parser::read_expression_parentheses() {
  LineColumn lc = ts.location();
  PrExprParen* p = new PrExprParen();
  p->m_start = lc;

  assert_peek(Token(PUNC,"("),"%unexpected while expecting open parenthesis \"(\"");
  ts.read(); //(
  ignoreWS(p);

  p->content = read_expression();

  assert_peek(Token(PUNC,")"),"%unexpected while expecting matching close parenthesis \"(\"");
  ts.read(); //)
  p->m_end = ts.location();
  return p;
}

PrStatementVar* Parser::read_statement_var() {
  LineColumn lc = ts.location();
  PrStatementVar* p = new PrStatementVar();
  p->m_start = lc;
  if (ts.peek() != Token(KW,"globalvar"))
    assert_peek(Token(KW,"var"),"%unexpected while expecting var declaration");
  p->type = ts.read().value; // read "var"
  while (true) {
    ignoreWS(p);
    if (ts.peek().type != ID)
      throw ParseError("Unexpected token \"" + ts.peek().value + "\" while reading var declaration; expected variable name.", ts.location().pair());
    LineColumn lc = ts.location();
    PrVarDeclaration* d = new PrVarDeclaration(ts.read(), nullptr, lc);
    ignoreWS(d);
    if (ts.peek() == Token(OP,"=")) {
      ts.read();
      ignoreWS(d);
      d->definition = read_expression();
    }
    ignoreWS(p);
    p->declarations.push_back(d);
    if (ts.peek() == Token(PUNC,",")) {
      ts.read();
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
  assert(kw_enum == Token(KW, "enum"));
  p->identifier = ts.read(); // read name of enum
  assert(p->identifier.type == ID);
  ignoreWS(p);

  // {
  assert(ts.peek().value == "{");
  ts.read();

  while (true) {
    ignoreWS(p);
    if (ts.peek().type != ID)
      throw ParseError("Unexpected token \"" + ts.peek().value + "\" while reading enum; expected variable name.", ts.location().pair());
    LineColumn lc = ts.location();
    PrVarDeclaration* d = new PrVarDeclaration(ts.read(), nullptr, lc);
    ignoreWS(d);
    if (ts.peek() == Token(OP,"=")) {
      ts.read();
      ignoreWS(d);
      d->definition = read_expression();
    }
    ignoreWS(p);
    p->declarations.push_back(d);
    if (ts.peek() == Token(PUNC,",")) {
      ts.read();
      continue;
    }
    break;
  }

  // }
  assert(ts.peek().value == "}");
  ts.read();

  p->m_end = ts.location();

  return p;
}

PrStatementIf* Parser::read_statement_if() {
  LineColumn lc = ts.location();
  PrStatementIf* p = new PrStatementIf();
  p->m_start = lc;
  assert_peek(Token(KW,"if"), "%unexpected; expected if statement");
  ts.read(); // read IF
  ignoreWS(p);
  p->condition = read_expression();
  siphonWS(p->condition,p,false,true);

  p->result = read_statement();
  read_statement_end();
  ignoreWS(p->result,true);
  siphonWS(p->result,p,true,true);

  if (ts.peek() == Token(KW, "else")) {
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
  assert(!braces || !end_at_define);
  if (braces) {
    assert_peek(Token(PUNC,"{"), "expected open brace, \"{\"");
    ts.read(); // {
  }

  // read productions inside of braces
  while (
      !(braces && ts.peek() == Token(PUNC,"}"))
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
    assert_peek(Token(PUNC,"}"), "expected matching end brace, \"}\"");
    ts.read(); // }
  }
  ignoreWS(p, true);

  p->m_end = ts.location();

  return p;
}

void Parser::ignoreWS(Production* p, bool as_postfix) {
  if (ts.peek() == Token(ENX,"\n") || (ts.peek().type == COMMENT)) {
    LineColumn lc = ts.location();
    PrInfixWS* infix = new PrInfixWS(ts.read(), lc);
    ignoreWS(infix);
    infix->m_end = ts.location();
    if (!ignore_decor)
      p->infixes.push_back(infix);
    else {
      delete(infix);
      p->infixes.push_back(nullptr);
    }
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
  assert_peek(Token(KW,"for"), "%unexpected; expected \"for\" statement.");
  ts.read(); // for
  ignoreWS(pfor);
  assert_peek(Token(PUNC,"("), "%unexpected where open parenthesis \"(\" for for statement arguments expected.");
  ts.read(); //(

  ignoreWS(pfor);
  pfor->init = read_statement();
  read_statement_end();
  ignoreWS(pfor);

  if (ts.peek() != Token(ENX,";"))
    pfor->condition = read_expression();
  else
    pfor->condition = nullptr;
  read_statement_end();

  ignoreWS(pfor);
  if (ts.peek() != Token(PUNC,")")) {
    pfor->second = read_statement();
    read_statement_end();
  } else {
    LineColumn lc = ts.location();
    pfor->second = new PrEmptyStatement();
    pfor->m_start = lc;
  }
  ignoreWS(pfor);

  assert_peek(Token(PUNC,")"), "%unexpected where end parenthesis \")\" for for statement arguments expected.");
  ts.read(); //)

  ignoreWS(pfor);
  pfor->first = read_statement();
  siphonWS(pfor->first, pfor, true);
  pfor->m_end = ts.location();
  return pfor;
}

void Parser::read_statement_end() {
  if (ts.peek() == Token(ENX,";")) {
    ts.read();
  }
}

PrWhile* Parser::read_while() {
  LineColumn lc = ts.location();
  PrWhile* p = new PrWhile();
  p->m_start = lc;
  assert_peek(Token(KW,"while"), "%unexpected; expected \"while\" statement.");
  ts.read(); // while
  ignoreWS(p);
  p->condition = read_expression();
  siphonWS(p->condition, p, false, true);
  p->event = read_statement();
  siphonWS(p->event, p, true);
  p->m_end = ts.location();
  return p;
}

PrRepeat* Parser::read_repeat() {
  LineColumn lc = ts.location();
  PrRepeat* p = new PrRepeat();
  p->m_start = lc;
  assert_peek(Token(KW,"repeat"), "%unexpected; expected \"repeat\" statement.");
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
  assert_peek(Token(KW,"do"), "%unexpected; expected \"do\" statement.");
  ts.read(); // do
  ignoreWS(p);
  p->event = read_statement();
  siphonWS(p->event, p, false, true);
  assert_peek(Token(KW,"until"), "%unexpected where \"until\" needed following \"do\"");
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
  assert_peek(Token(KW,"with"), "%unexpected; expected \"with\" statement.");
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

  assert_peek(Token(KW,"switch"), "%unexpected; expected \"switch\" statement.");
  ts.read(); // switch
  ignoreWS(p);

  p->condition = read_expression();
  siphonWS(p->condition, p, false, true);

  assert_peek(Token(PUNC,"{"), "%unexpected where open brace \"{\" required for switch statement.");
  ts.read(); // {
  ignoreWS(p);

  while (true) {
    if (ts.peek() == Token(PUNC,"}"))
        break;

    LineColumn lc = ts.location();
    PrCase* c = new PrCase();
    c->m_start = lc;
    if (ts.peek() != Token(KW,"default"))
      assert_peek(Token(KW,"case"), "%unexpected; switch statement requires cases.");
    Token t(ts.read()); // case
    ignoreWS(c);

    if (t==Token(KW,"case")) {
      c->value = read_expression();
      ignoreWS(c);
    } else {
      c->value = nullptr;
    }

    assert_peek(Token(PUNC,":"), "%unexpected where colon \":\" required for case.");
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

  assert_peek(Token(PUNC,"}"), "%unexpected; expected matching close brace \"}\" for switch statement.");
  ts.read(); // }
  ignoreWS(p, true);

  p->m_end = ts.location();
  return p;
}
