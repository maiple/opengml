#include <cctype>
#include <sstream>

#include "Lexer.hpp"
#include "ogm/common/util.hpp"

using namespace std;

const char* TOKEN_NAME[] = {
  "PUNC", // (),. etc.
  "OP", // operator
  "OPR", //++ or --
  "OPA", //accessor operator
  "NUM",
  "STR",
  "KW",
  "ID",
  "PPDEF",
  "COMMENT",
  "WS",
  "ENX",
  "END",
  "ERR"
};

const char* TOKEN_NAME_PLAIN[] = {
  "punctuation", // (),. etc.
  "operator", // operator
  "lr-operator", //++ or --
  "accessor operator", //accessor operator
  "number literal",
  "string literal",
  "keyword",
  "identifier",
  "preprocessor #define",
  "comment",
  "whitespace",
  "end-statement",
  "end-of-string",
  "error"
};

Token::Token(const TokenType type, const std::string value):
  type(type),
  value(value)
{ }

Token::Token(): Token(ERR,"")
{ }

bool Token::operator==(const Token& other) const {
  return type == other.type && value == other.value;
}

bool Token::operator!=(const Token& other) const {
  return !(*this == other);
}

bool Token::is_op_keyword() {
  if (type == KW) {
    if (value == "not")
      return true;
    if (value == "and")
      return true;
    if (value == "or")
      return true;
    if (value == "xor")
      return true;
    if (value == "mod")
      return true;
    if (value == "div")
      return true;
  }
  return false;
}

std::ostream &operator<<(std::ostream &os,const Token &token) {
  if (token.type == WS)
    return os << "--";
  if (token.type == END)
    return os << "EOF";
  return os << TOKEN_NAME[token.type] << ": " << token.value;
}

Lexer::Lexer(istream* istream): is(istream), next(END,""), istream_mine(false) {
  read();
}

LineColumn Lexer::location() const {
  return{ row, col };
}

Lexer::Lexer(std::string s): is(new std::stringstream(s)), next(END,""), istream_mine(true) {
  read();
}

Lexer::~Lexer() {
  if (istream_mine)
    delete(is);
}

char Lexer::read_char() {
  char c = is->get();
  if (c=='\n') {
    prev_line_col = col;
    col = 0;
    row ++;
  } else {
    col ++;
  }
  return c;
}

void Lexer::putback_char(char c) {
  is->putback(c);
  if (col == 0)
  {
      row--;

      // FIXME: go back further with multiple putback_char calls?
      col = prev_line_col;
  }
  else
  {
      col--;
  }
}

Token Lexer::read_string() {
  unsigned char c;
  unsigned char terminal;
  string val;
  *is >> terminal; // determine terminal character
  if (is->eof())
    return Token(ERR,"Unterminated string");
  while (true) {
    c = read_char();
    if (c == terminal) break;
    if (is->eof())
      return Token(ERR,"Unterminated string");
    val += c;
  }
  std::string terminal_str = " ";
  terminal_str[0] = terminal;
  return Token(STR,terminal_str + val + terminal_str);
}

Token Lexer::read_number(bool hex) {
  unsigned char c;
  string val;
  bool encountered_dot = false;
  int position = 0;
  if (hex)
    val += read_char(); //$
  while (true) {
    if (is->eof())
      break;
    c = read_char();
    position += 1;
    if ((c >= '0' && c <= '9') || (hex && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))) {
      val += c;
      continue;
    }
    if (c == '.' &&! encountered_dot) {
      val += c;
      encountered_dot = true;
      continue;
    }
    putback_char(c);
    break;
  }
  return Token(NUM, val);
}

Token Lexer::read_comment() {
  string val;
  char c;
  c = read_char();
  c = read_char();
  val = "//";

  while (true) {
    if (is->eof())
      break;
    c = read_char();
    if (c == '\n' || c == -1) {
      putback_char(c);
      break;
    }
    val += c;
  }

  return Token(COMMENT,val);
}

Token Lexer::read_comment_multiline() {
  string val;
  char c;
  c = read_char();
  c = read_char();
  val = "/*";
  while (true) {
    if (is->eof()) break;
    c = read_char();
    if (c == -1 || is->eof()) {
      rtrim(val);
      break;
    }
    if (c == '*') {
      char c2;
      c2 = read_char();
      if (c2 == '/') {
        val += "*/";
        break;
      } else
        putback_char(c2);
    }
    val += c;
  }

  return Token(COMMENT, val);
}

const char* op_multichar[] = {
  "++",
  "--",
  "+=",
  "!=",
  ">=",
  "<=",
  "==",
  "||",
  "&&",
  "^^",
  "!=",
  "+=",
  "%=",
  "^=",
  "-=",
  "/=",
  "&=",
  "|=",
  "*=",
  "~="
  "<<",
  ">>",
  "<>"
};

Token Lexer::read_operator() {
  char c1;
  c1 = read_char();
  if (is_opa_char(c1)) {
    return Token(OPA,string(1,(char)c1));
  }
  if (!is->eof()) {
    char c2;
    c2 = read_char();

    string multi(1, c1);
    multi += c2;

    for (size_t i=0;i < sizeof(op_multichar)/sizeof(char*);i++)
      if (multi == op_multichar[i])
        return Token((i <= 1)?OPR:OP,multi);

    putback_char(c2);
  }

  return Token(OP,string(1, (char)c1));
}


const char* KEYWORDS[] = {
  "var",
  "globalvar",
  "if",
  "else",
  "while",
  "repeat",
  "do",
  "until",
  "with",
  "for",
  "switch",
  "case",
  "default",
  "break",
  "continue",
  "return",
  "exit",
  "enum",
  "not",
  "and",
  "or",
  "xor",
  "mod",
  "div",
};

Token Lexer::read_ident() {
  int index = -1;
  unsigned char c;
  string val;
  while (true) {
    index += 1;
    if (is->eof())
      break;
    c = read_char();
    if ((c >= '0' && c <= '9') && index > 0) {
      val += c;
      continue;
    }
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
      val += c;
      continue;
    }
    putback_char(c);
    break;
  }
  if (val.length() == 0)
    return Token(ERR,"");
  for (size_t i = 0; i< sizeof(KEYWORDS)/sizeof(char*);i+=1)
    if (val == KEYWORDS[i])
      return Token(KW,val);
  return Token(ID,val);
}

Token Lexer::read_preprocessor() {
    std::stringstream ss;
    unsigned char in = read_char();
    //assert(in == '#');

    ss << "#";
    while (true) {
        if (is->eof()) {
            goto ret;
        }
        in = read_char();
        if (in == '\n') {
        ret:
            putback_char(in);
            std::string s = ss.str();
            return Token{ starts_with(s, "#define")
                ? PPDEF
                : COMMENT, s };
        }
        ss << in;
    }
}

Token Lexer::read_next() {
  unsigned char in;
  if (is->eof())
    return Token(END,"");
  // read whitespace:
  while (true) {
    in = read_char();
    if (!isspace(in) || in == '\n'|| is->eof()) break;
  }
  if (in == '#' && !no_preprocessor) {
    putback_char(in);
    return read_preprocessor();
  }
  if (in == '\n')
    return Token(ENX,string(1, (char)in));
  if (is->eof())
    return Token(END,"");
  // parse token:
  if (in == '"' || in == '\'') {
    putback_char(in);
    no_preprocessor = false;
    return read_string();
  }
  if (in >= '0' && in <= '9') {
    putback_char(in);
    no_preprocessor = false;
    return read_number();
  }
  if ((in == '.' || in == '$') &&! is->eof()) {
    unsigned char in2;
    in2 = read_char();
    putback_char(in2);
    no_preprocessor = false;
    if ((in2 >= '0' && in2 <= '9') || in == '$') {
      putback_char(in);
      return read_number(in == '$');
    }
  }
  if (in == '/' &&! is->eof()) {
    unsigned char in2;
    in2 = read_char();
    putback_char(in2);
    if (in2 == '/') {
      putback_char(in);
      return read_comment();
    }
    if (in2 == '*') {
      putback_char(in);
      return read_comment_multiline();
    }
  }
  if (in == ';')
    return Token(ENX,string(1, (char)in));
  if (is_punc_char(in)) {
    no_preprocessor = false;
    // need to read [# as accessor, not preprocessor.
    if (in == '[') no_preprocessor = true;
    return Token(PUNC,string(1, (char)in));
  }
  if (is_op_char(in) || is_opa_char(in)) {
    putback_char(in);
    no_preprocessor = false;
    return read_operator();
  }
  putback_char(in);
  return read_ident();
}

Token Lexer::read() {
  Token to_return = next;
  next = read_next();
  return to_return;
}

Token Lexer::peek() const {
  return next;
}

bool Lexer::eof() {
  return peek().type == END || peek().type == ERR;
}

const char ops[] = "+-*/<>=&|!%^~";
const char opas[] = "?#@";
const char punc[] = "(){}.,[]:";

bool Lexer::is_op_char(const unsigned char c) {
  for (size_t i=0;i<sizeof(ops);i++) {
    if (ops[i] == c)
      return true;
  }
  return false;
}

bool Lexer::is_opa_char(const unsigned char c) {
  for (size_t i=0;i<sizeof(opas);i++) {
    if (opas[i] == c)
      return true;
  }
  return false;
}

bool Lexer::is_punc_char(const
 unsigned char c) {
  for (size_t i=0;i<sizeof(punc);i++) {
    if (punc[i] == c)
      return true;
  }
  return false;
}

LLKLexer::LLKLexer(istream* is, uint16_t k): Lexer(is), k(k) {
    while (buffer.size() + 1 < k && !Lexer::eof())
    {
        locs.push_back(Lexer::location());
        buffer.push_back(Lexer::read());
    }
}

LLKLexer::LLKLexer(std::string s, uint16_t k): Lexer(s), k(k) {
    while (buffer.size() + 1< k && !Lexer::eof())
    {
        locs.push_back(Lexer::location());
        buffer.push_back(Lexer::read());
    }
}

Token LLKLexer::peek() const {
    if (buffer.size() == 0)
    {
        return Lexer::peek();
    }
    return buffer.front();
}

Token LLKLexer::peek(unsigned int i) const {
  if (i + 1 == k)
    return Lexer::peek();
  else
    return buffer[i];
}

LineColumn LLKLexer::location() const {
  return locs.front();
}

Token LLKLexer::read() {
  if (!Lexer::eof())
  {
    locs.push_back(Lexer::location());
    buffer.push_back(Lexer::read());
  }
  Token to_return = buffer.front();
  locs.pop_front();
  buffer.pop_front();
  return to_return;
}

bool LLKLexer::eof() const {
  return buffer.size() == 0;
}

bool LLKLexer::has(unsigned int k) const {
  return buffer.size() > k;
}
