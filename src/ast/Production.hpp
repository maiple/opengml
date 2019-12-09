#include <vector>
#include <string>
#include <typeinfo>
#include <deque>

#include "Lexer.hpp"

#ifndef PRODUCTION_H
#define PRODUCTION_H

struct CompilerContext;

struct PrInfixWS;

struct Production {
  friend class Parser;
  Production() {}
  Production(LineColumn lc) : m_start(lc) {}
  virtual ~Production();

  virtual std::string to_string();

  LineColumn m_start;
  LineColumn m_end;

  void flattenPostfixes();

  // OPTIMIZE: allocating these is slow. is there a small_deque?
  std::deque<PrInfixWS*> infixes;
  int postfix_n = 0;
};

struct PrInfixWS: Production {
  PrInfixWS(Token, LineColumn);
  virtual std::string to_string();

  Token val;
};


struct PrDecor: Production {
  virtual std::string to_string();

  PrDecor(Token rawToken, LineColumn);
  Token rawToken;
};

struct PrExpression: Production {
};

// parentheses are important to remember for beautifier
struct PrExprParen: PrExpression {
  virtual std::string to_string();

  PrExpression* content;
};

struct PrExpressionFn: PrExpression{
  PrExpressionFn(Token identifier, LineColumn);
  virtual std::string to_string();

  Token identifier;
  std::vector<PrExpression*> args;
};

struct PrExprArithmetic: PrExpression {
  PrExprArithmetic(PrExpression* lhs, Token op, PrExpression* rhs, LineColumn);
  virtual std::string to_string();

  PrExpression* lhs;
  PrExpression* rhs;
  Token op;
};

struct PrStatement: Production {

};

struct PrEmptyStatement: PrStatement {
  PrEmptyStatement();
  PrEmptyStatement(Token enx);
  virtual std::string to_string();

  bool hastoken;
  Token enx;
};

struct PrFinal: PrExpression {
  virtual std::string to_string();
  PrFinal(Token t, LineColumn);

  Token final;
};

struct PrArrayLiteral: PrExpression {
  virtual std::string to_string();

  std::vector<PrExpression*> vector;
};

struct PrTernary: PrExpression {
  virtual std::string to_string();

  PrExpression* condition;
  PrExpression* result;
  PrExpression* otherwise;
};

struct PrIdentifier: PrExpression {
  virtual std::string to_string();
  PrIdentifier(Token t, LineColumn);

  Token identifier;
};

struct PrAssignment: PrStatement {
  PrAssignment(PrExpression* lhs, Token op, PrExpression* rhs, LineColumn);
  virtual std::string to_string();

  PrExpression* lhs;
  PrExpression* rhs;
  Token op;
};

struct PrStatementFn: PrStatement {
  virtual std::string to_string();

  PrExpressionFn* fn;
};

struct PrVarDeclaration: Production {
  PrVarDeclaration(Token ident,  PrExpression*, LineColumn);
  virtual std::string to_string();

  Token identifier;
  PrExpression* definition;
};

struct PrStatementVar: PrStatement {
  virtual std::string to_string();

  std::vector<std::string> types;
  std::vector<PrVarDeclaration*> declarations;
};

struct PrStatementEnum: PrStatement {
  virtual std::string to_string();

  Token identifier;
  std::vector<PrVarDeclaration*> declarations;
};

struct PrBody: PrStatement {
  virtual std::string to_string();
  bool is_root = false;

  std::vector<Production*> productions;
};

// contains a list of bodies with associated names.
// the names are either blank "" or the define preprocessor
// statement.
struct PrBodyContainer : Production {
    std::vector<PrBody*> bodies;
    std::vector<std::string> names;
};

struct PrStatementIf: PrStatement {
  virtual std::string to_string();

  bool has_then = false; // if (x) then (y)
  PrExpression* condition;
  PrStatement* result;
  PrStatement* otherwise=nullptr;
};

struct PrFor: PrStatement {
  virtual std::string to_string();

  PrStatement* init;
  PrExpression* condition;
  PrStatement* second;
  PrStatement* first;
};

struct PrWhile: PrStatement {
  virtual std::string to_string();

  bool has_do = false; // while (x) do (y)
  PrExpression* condition;
  PrStatement* event;
};

struct PrRepeat: PrStatement {
  virtual std::string to_string();

  PrExpression* count;
  PrStatement* event;
};

struct PrDo: PrStatement {
  virtual std::string to_string();

  PrExpression* condition;
  PrStatement* event;
};

struct PrWith: PrStatement {
  virtual std::string to_string();

  PrExpression* objid;
  PrStatement* event;
};

struct PrAccessorExpression: PrExpression {
  virtual std::string to_string();

  std::string acc = "";
  PrExpression* ds;
  std::vector<PrExpression*> indices;
};

struct PrCase;

struct PrSwitch: PrStatement {
  virtual std::string to_string();

  PrExpression* condition;
  std::vector<PrCase*> cases;
};

struct PrCase: PrStatement {
  virtual std::string to_string();

  PrExpression* value;
  std::vector<Production*> productions;
};

struct PrControl: PrStatement {
  PrControl(Token,PrExpression* val, LineColumn);
  virtual std::string to_string();

  Token kw;
  PrExpression* val;
};

#endif /*PRODUCTION_H*/
