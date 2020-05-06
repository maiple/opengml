#include <vector>
#include <string>
#include <typeinfo>
#include <deque>
#include <memory>

#include "Lexer.hpp"

#ifndef PRODUCTION_H
#define PRODUCTION_H

struct CompilerContext;

struct PrInfixWS;

// TODO: switch everything over to std::unique_ptr.

struct Production {
  friend class Parser;
  Production() {}
  Production(LineColumn lc) : m_start(lc) {}
  virtual ~Production();

  virtual std::string to_string();

  LineColumn m_start{ 0, 0 };
  LineColumn m_end{ 0, 0 };

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
  
  virtual ~PrExprParen()
  {
      if (content) delete content;
  }
};

struct PrExpressionFn: PrExpression{
  PrExpressionFn(Token identifier, LineColumn);
  virtual std::string to_string();

  Token identifier;
  std::vector<PrExpression*> args;
  
  virtual ~PrExpressionFn()
  {
      for (auto* p : args)
      {
          if (p) delete p;
      }
  }
};

struct PrExprArithmetic: PrExpression {
  PrExprArithmetic(PrExpression* lhs, Token op, PrExpression* rhs, LineColumn);
  virtual std::string to_string();

  PrExpression* lhs;
  PrExpression* rhs;
  Token op;
  
  virtual ~PrExprArithmetic()
  {
      if (lhs) delete lhs;
      if (rhs) delete rhs;
  }
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

struct PrLiteral: PrExpression {
  virtual std::string to_string();
  PrLiteral(Token t, LineColumn);

  Token final;
};

struct PrArrayLiteral: PrExpression {
  virtual std::string to_string();

  std::vector<PrExpression*> vector;
  
  virtual ~PrArrayLiteral()
  {
      for (auto* p : vector)
      {
          if (p) delete p;
      }
  }
};

struct PrVarDeclaration;

struct PrStructLiteral: PrExpression {
  virtual std::string to_string();

  std::vector<PrVarDeclaration*> declarations;
  
  virtual ~PrStructLiteral();
};

struct PrTernary: PrExpression {
  virtual std::string to_string();

  PrExpression* condition;
  PrExpression* result;
  PrExpression* otherwise;
  
  virtual ~PrTernary()
  {
      if (condition) delete condition;
      if (result) delete result;
      if (otherwise) delete otherwise;
  }
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
  
  virtual ~PrAssignment()
  {
      if (lhs) delete lhs;
      if (rhs) delete rhs;
  }
};

struct PrStatementFn: PrStatement {
  virtual std::string to_string();

  PrExpressionFn* fn;
  
  virtual ~PrStatementFn()
  {
      if (fn) delete fn;
  }
};

struct PrVarDeclaration: Production {
  PrVarDeclaration(Token ident,  PrExpression*, LineColumn);
  virtual std::string to_string();

  Token identifier;
  PrExpression* definition;
  
  virtual ~PrVarDeclaration()
  {
      if (definition) delete definition;
  }
};

struct PrStatementVar: PrStatement {
  virtual std::string to_string();

  std::vector<std::string> types;
  std::vector<PrVarDeclaration*> declarations;
  
  virtual ~PrStatementVar()
  {
      for (auto* p : declarations)
      {
          if (p) delete p;
      }
  }
};

struct PrStatementEnum: PrStatement {
  virtual std::string to_string();

  Token identifier;
  std::vector<PrVarDeclaration*> declarations;
  
  virtual ~PrStatementEnum()
  {
      for (auto* p : declarations)
      {
          if (p) delete p;
      }
  }
};

struct PrBody: PrStatement {
  virtual std::string to_string();
  bool is_root = false;

  std::vector<Production*> productions;
  
  virtual ~PrBody()
  {
      for (auto* p : productions) delete p;
  }
};

// contains a list of bodies with associated names.
// the names are either blank "" or the define preprocessor
// statement.
struct PrBodyContainer : Production {
    std::vector<PrBody*> bodies;
    std::vector<std::string> names;
    
    virtual ~PrBodyContainer()
    {
        for (PrBody* body : bodies) delete body;
    }
};

struct PrStatementIf: PrStatement {
  virtual std::string to_string();

  bool has_then = false; // if (x) then (y)
  PrExpression* condition;
  PrStatement* result;
  PrStatement* otherwise=nullptr;
  
  virtual ~PrStatementIf()
  {
      if (condition) delete condition;
      if (result) delete result;
      if (otherwise) delete otherwise;
  }
};

struct PrFor: PrStatement {
  virtual std::string to_string();

  PrStatement* init;
  PrExpression* condition;
  PrStatement* second;
  PrStatement* first;
  
  virtual ~PrFor()
  {
      if (init) delete init;
      if (condition) delete condition;
      if (second) delete second;
      if (first) delete first;
  }
};

struct PrWhile: PrStatement {
  virtual std::string to_string();

  bool has_do = false; // while (x) do (y)
  PrExpression* condition;
  PrStatement* event;
  
  virtual ~PrWhile()
  {
      if (condition) delete condition;
      if (event) delete event;
  }
};

struct PrRepeat: PrStatement {
  virtual std::string to_string();

  PrExpression* count;
  PrStatement* event;
  
  virtual ~PrRepeat()
  {
      if (count) delete count;
      if (event) delete event;
  }
};

struct PrDo: PrStatement {
  virtual std::string to_string();

  PrExpression* condition;
  PrStatement* event;
  
  virtual ~PrDo()
  {
      if (condition) delete condition;
      if (event) delete event;
  }
};

struct PrWith: PrStatement {
  virtual std::string to_string();

  PrExpression* objid;
  PrStatement* event;
  
  virtual ~PrWith()
  {
      if (objid) delete objid;
      if (event) delete event;
  }
};

struct PrAccessorExpression: PrExpression {
  virtual std::string to_string();

  std::string acc = "";
  PrExpression* ds;
  std::vector<PrExpression*> indices;
  
  virtual ~PrAccessorExpression()
  {
      if (ds) delete ds;
      for (auto* p : indices)
      {
          if (p) delete p;
      }
  }
};

struct PrCase;

struct PrCase: PrStatement {
  virtual std::string to_string();

  PrExpression* value;
  std::vector<Production*> productions;
  
  virtual ~PrCase()
  {
      if (value) delete value;
      for (auto* p : productions)
      {
          if (p) delete p;
      }
  }
};

struct PrSwitch: PrStatement {
  virtual std::string to_string();

  PrExpression* condition;
  std::vector<PrCase*> cases;
  
  virtual ~PrSwitch()
  {
      if (condition) delete condition;
      for (auto* p : cases)
      {
          if (p) delete p;
      }
  }
};

struct PrControl: PrStatement {
  PrControl(Token,PrExpression* val, LineColumn);
  virtual std::string to_string();

  Token kw;
  PrExpression* val;
  
  virtual ~PrControl()
  {
      if (val) delete val;
  }
};

struct PrFunctionLiteral: PrExpression {
  virtual std::string to_string();

  // optional -- can be an empty string
  Token name;
  
  std::vector<Token> args;
  
  bool constructor;
  
  std::unique_ptr<PrBody> body;
};

struct PrStatementFunctionLiteral: PrStatement {
  inline virtual std::string to_string()
  {
    return fn->to_string();
  }

  std::unique_ptr<PrFunctionLiteral> fn;
  
  PrStatementFunctionLiteral(PrFunctionLiteral* f)
    : fn(f)
  {
    m_start = fn->m_start;
    m_end = fn->m_end;
  }
};

#endif /*PRODUCTION_H*/
