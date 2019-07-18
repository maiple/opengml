#include "Parser.hpp"
#include <iostream>

using namespace std;

Production::~Production() {
  while (!infixes.empty()) {
    PrInfixWS* p = infixes.front();
    if (p)
      delete(p);
    infixes.pop_front();
  }
}

string Production::to_string() {
  return "?";
}

void Production::flattenPostfixes() {
  //TODO: rewrite with iterators
  for (int i=infixes.size()-postfix_n;i<infixes.size();i++) {
    // flatten postfixes
    auto ws = infixes[i];
    if (ws) {
      ws->postfix_n = 0;
      // pull out nested infixes
      while (!ws->infixes.empty()) {
        auto nested_ws = ws->infixes.back();
        ws->infixes.pop_back();
        infixes.insert(infixes.begin() + i+1,nested_ws);
        postfix_n++;
      }
    }
  } 
}

PrDecor::PrDecor(Token t, LineColumn lc): rawToken(t), Production(lc) {}

string PrDecor::to_string() {
  return rawToken.value;
}

string PrExprParen::to_string() {
  return "(" + content->to_string() + ")";
}

PrExprArithmetic::PrExprArithmetic(PrExpression* lhs, Token op, PrExpression* rhs, LineColumn lc): lhs(lhs), op(op), rhs(rhs) {m_start = lc;}

string PrExprArithmetic::to_string() {
  return ((lhs)?(lhs->to_string() + " "):"") + op.value + ((rhs)?(" " + rhs->to_string()):"");
}

PrAssignment::PrAssignment(PrExpression* lhs, Token op, PrExpression* rhs, LineColumn lc): lhs(lhs), op(op), rhs(rhs) {m_start = lc;}

string PrAssignment::to_string() {
  if (op.type == OPR) {
    return lhs->to_string() + op.value;
  } else {
    return lhs->to_string() + " " + op.value + " " + rhs->to_string();
  }
}

PrExpressionFn::PrExpressionFn(Token id, LineColumn lc): identifier(id) {m_start = lc;}

string PrExpressionFn::to_string() {
  string str = identifier.value + "(";
  for (auto arg: args) {
    str += arg->to_string() + ",";
  }
  if (args.size() > 0)
    str = str.substr(0, str.size() - 1);
  str +=  ")";
  return str;
}

string PrStatementFn::to_string() {
  return "@" + fn->to_string();
}

string PrBody::to_string() {
  string str = "{\n";
  for (auto p: productions) {
    str += p->to_string() + "\n";
  }
  str += "}";
  return str;
}

PrEmptyStatement::PrEmptyStatement(): enx(Token(ENX,"\n")), hastoken(false) {}

PrEmptyStatement::PrEmptyStatement(Token t): enx(t), hastoken(true) {}

string PrEmptyStatement::to_string() {
  if (!hastoken)
    return "";
  if (enx.value == ";")
    return "`";
  return "";
}

PrFinal::PrFinal(Token t, LineColumn lc): final(t) {m_start = lc;}

string PrFinal::to_string() {
  if (final.type == STR)
    return "\"" + final.value + "\"";
  return "%" + final.value;
}

PrIdentifier::PrIdentifier(Token t, LineColumn lc): identifier(t) {m_start = lc;}

string PrIdentifier::to_string() {
  return "$" + identifier.value;
}

PrVarDeclaration::PrVarDeclaration(Token t, PrExpression* expr, LineColumn lc): identifier(t), definition(expr) {m_start = lc;}

string PrVarDeclaration::to_string() {
  if (definition != 0)
    return identifier.value + " = " + definition->to_string();
  return identifier.value;
}

string PrStatementVar::to_string() {
  string s = type + " ";
  for (auto v: declarations) {
    s += v->to_string();
    s += ",";
  }
  s = s.substr(0,s.length() - 1);
  return s;
}

string PrStatementEnum::to_string() {
  string s = "enum " + identifier.value + " {";
  for (auto v: declarations) {
    s += v->to_string();
    s += ",";
  }
  s = s.substr(0,s.length() - 1);
  s += "}";
  return s;
}

string PrStatementIf::to_string() {
  string s = "if " + condition->to_string();
  s += "\n> " + result->to_string();
  if (otherwise)
    s += "\nelse\n> " + otherwise->to_string();
  return s;
}

string PrControl::to_string() {
  string s = kw.value;
  if (val)
    s += val->to_string();
  return s;
}

PrControl::PrControl(Token t, PrExpression* val, LineColumn lc): kw(t), val(val) {m_start = lc;}

string PrFor::to_string() {
  string s = "for (";
  s += init->to_string();
  s += "; ";
  if (condition)
    s += condition->to_string();
  s += "; ";
  s += second->to_string();
  s += ")\n";
  s += "> " + first->to_string();
  return s;
}

string PrWhile::to_string() {
  return "while " + condition->to_string() + "\n" + event->to_string();
}

string PrWith::to_string() {
  return "with " + objid->to_string() + "\n" + event->to_string();
}

string PrRepeat::to_string() {
  return "repeat " + count->to_string() + "\n" + event->to_string();
}

string PrDo::to_string() {
  return "do "+event->to_string() + " until " + condition->to_string();
}

string PrAccessorExpression::to_string() {
  string s = ds->to_string() + "[" + acc;
  for (auto index: indices) {
    s += index->to_string() + ", ";
  }
  return s.substr(0,s.length()-2) + "]";
}

string PrCase::to_string() {
  string s;
  if (value)
    s = "case " + value->to_string() + ":\n";
  else
    s = "default:\n";
  
  for (auto p: productions)
    s += p->to_string() + "\n";
    
  return s;
}

string PrSwitch::to_string() {
  string s = "switch " + condition->to_string() + " {\n";
  
  for (auto c: cases) {
    s += c->to_string();
  }
  
  s += "}";
  return s;
}

PrInfixWS::PrInfixWS(Token t, LineColumn lc): val(t) {m_start = lc;;}

string PrInfixWS::to_string() {
  if (val.value == "\n") {
    return "\\n";
  }
  return val.value;
}
