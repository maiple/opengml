#include <vector>
#include <string>
#include <typeinfo>
#include <deque>

#include "Lexer.hpp"
#include "Production.hpp"

#ifndef PARSER_H
#define PARSER_H

class Parser {
public:
  Parser(std::istream* is);
  Parser(std::string s);

  //! parses full stream
  PrBodyContainer* parse();

  //! parses an expression
  PrExpression* parse_expression();

  //! parses a single line
  Production* read();

  //! ignore comments and whitespace
  bool ignore_decor = false;
private:
  Production* read_production();
  PrDecor* read_rawtoken();
  PrExpression* read_expression(uint32_t priority=0);
  PrExpression* read_term(bool readAccessor=true, bool read_possessive=true);
  PrExprParen* read_expression_parentheses();
  PrStatement* read_statement();
  PrAssignment* read_assignment();
  PrExpression* read_accessors(PrExpression* ds);
  PrExpression* read_possessive(PrExpression* owner);
  PrExpression* read_arithmetic(uint32_t priority, PrExpression* lhs);
  PrExpressionFn* read_expression_function();
  PrStatementFn* read_statement_function();
  PrStatementVar* read_statement_var();
  PrStatementIf* read_statement_if();
  PrBody* read_block(bool braces=true, bool end_at_define=false);
  PrFor* read_for();
  PrWith* read_with();
  PrWhile* read_while();
  PrRepeat* read_repeat();
  PrDo* read_do();
  PrSwitch* read_switch();
  PrStatementEnum* read_enum();
  void assert_peek(Token t, std::string message) const;

  //! read comments and whitespaces as infixes for p
  void ignoreWS(Production* p, bool as_postfix = false);

  //! take any postfixes of src and apply them as infixes (postfixes, if as_postfix) to dst
  void siphonWS(Production* src, Production* dst, bool as_postfix = false, bool condense = false);

  //! removes final newline from end of postfixes
  void removeExtraNewline(Production* p);

  //! read semicolon and/or line ending
  void read_statement_end();

  LLKLexer ts;
};

#endif /*PARSER_H*/
