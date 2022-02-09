
#ifdef OPTIMIZE_PARSE
#ifdef __GNUC__
#pragma GCC optimize ("O3")
#endif
#endif

#include <cctype>
#include <sstream>
#include <iostream>

#include "Lexer.hpp"
#include "ogm/common/util.hpp"

#include <unordered_set>

#include <utf8.h>

using namespace std;
using namespace ogm;

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

Lexer::Lexer(istream* istream, int flags)
    : is(istream)
    , next(END,"")
    , istream_mine(false)
    , no_decorations(flags & 0x1)
    , m_v2(flags & 0x2)
{
  read();
}

Lexer::Lexer(std::string s, int flags)
    : is(new std::stringstream(s))
    , next(END,"")
    , istream_mine(true)
    , no_decorations(flags & 0x1)
    , m_v2(flags & 0x2)
{
  read();
}

Lexer::~Lexer() {
  if (istream_mine)
    delete(is);
}

char Lexer::read_char() {

  m_location[1] = m_location[0];

  char c = is->get();
  if (c=='\n') {
    // OPTIMIZE: skip string copy.
    m_location[0].m_column = 0;
    m_location[0].m_source_column = 0;
    m_location[0].m_line++;
    m_location[0].m_source_line++;
  } else {
    m_location[0].m_column++;
    m_location[0].m_source_column++;
  }
  return c;
}

void Lexer::putback_char(char c) {
  is->putback(c);
  m_location[0] = m_location[1];
}

namespace
{
  inline uint8_t read_hex_digit_from_char(char c)
  {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a';
    if (c >= 'A' && c <= 'F') return c - 'A';
    return 0xFF;
  }
  
  inline bool is_hex_digit(char c)
  {
    return read_hex_digit_from_char(c) != 0xFF;
  }
}

void Lexer::read_string_helper_escaped(std::string& val) {
  unsigned char c;  
  *is >> c;
  
  if (c == 'n') {
    val += "\n"; return;
  }
  if (c == '\\') {
    val += "\\"; return;
  }
  if (c == '\'') {
    val += "\'"; return;
  }
  if (c == '\"') {
    val += "\""; return;
  }
  if (c == 'r') {
    val += "\r"; return;
  }
  if (c == 'b') {
    val += "\b"; return;
  }
  if (c == 'f') {
    val += "\f"; return;
  }
  if (c == 't') {
    val += "\t"; return;
  }
  if (c == 'v') {
    val += "\v"; return;
  }
  if (c == 'a') {
    val += "\a"; return;
  }
  std::stringstream ss;
  ss << "\\";
  uint32_t wchar;
  
  // hex
  if (c == 'x') {
    char c2;
    *is >> c2;
    while (is_hex_digit(c2)) {
      wchar <<= 4;
      wchar |= read_hex_digit_from_char(c2);
      *is >> c2;
    }
    is->putback(c2);
    char buff[2] = {c2, 0};
    val += buff;
    return;
  }
  
  // octal
  else if ((char)c >= '0' && (char)c < '8') {
    char c2 = c;
    while (c2 >= '0' && c2 <= '7') {
      wchar <<= 3;
      wchar |= read_hex_digit_from_char(c2);
      *is >> c2; // octal is subset of hex.
    }
    is->putback(c2);
    goto UNICODE;
  }
  
  // unicode
  else if ((char)c == 'u') {
    char c2;
    *is >> c2;
    while (is_hex_digit(c2)) {
      wchar <<= 4;
      wchar |= c;
      *is >> c2;
    }
    is->putback(c2);
    goto UNICODE;
  }
  
  else
  {
    // default -- ignore escape; insert literal.
    std::cout << "WARNING: unrecognized escape sequence '" << ss.str() << "'\n";
    val += ss.str();
    return;
  }
  
UNICODE: {
    // insert wide character
    char buff[5];
    memset(buff, 0, sizeof(buff));
    u8_wc_toutf8(buff, wchar);
    val += buff;
    return;
  }
}

Token Lexer::read_string() {
  unsigned char c;
  unsigned char terminal;
  bool is_at_literal = false;
  string val;
  *is >> terminal; // determine terminal character
  if (terminal == '@')
  {
    *is >> terminal;
    is_at_literal = true;
  }
  if (is->eof())
    return Token(ERR,"Unterminated string");
  while (true)
  {
    c = read_char();
    if (c == terminal) break;
    if (is->eof())
      return Token(ERR,"Unterminated string");
    if (c == '\\' && (!is_at_literal || m_v2))
    {
      read_string_helper_escaped(val);
    }
    else
    {
      val += c;
    }
  }
  std::string terminal_str = " ";
  terminal_str[0] = terminal;
  return Token(STR,terminal_str + val + terminal_str);
}

Token Lexer::read_number(int hex) {
  unsigned char c;
  string val;
  bool encountered_dot = false;
  int position = 0;
  for (int i = 0; i < hex; ++i)
  {
    val += read_char(); //$ or 0x
  }
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

namespace
{
    unordered_set<std::string> op_multichar = {
      {"++"},
      {"--"},
      {"+="},
      {"!="},
      {">="},
      {"<="},
      {"=="},
      {"||"},
      {"&&"},
      {"^^"},
      {"!="},
      {"+="},
      {"%="},
      {"^="},
      {"-="},
      {"/="},
      {"&="},
      {"|="},
      {"*="},
      {"~="}, // apparently this is exists?
      {"<<"},
      {">>"},
      {"<>"} // same as !=
    };
}

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

    auto iter = op_multichar.find(multi);
    if (iter != op_multichar.end())
    {
        return Token((multi == "++" || multi == "--")?OPR:OP,multi);
    }

    putback_char(c2);
  }

  return Token(OP,string(1, (char)c1));
}

namespace
{
    const unordered_set<std::string> keywords = {
      "var",
      "globalvar",
      "static",
      "if",
      "then",
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
      "function",
      "constructor",
      "new",
      "try",
      "catch",
      "finally"
    };
}

Token Lexer::read_ident() {
  int index = -1;
  unsigned char c;
  std::stringstream vals;
  while (true) {
    index += 1;
    if (is->eof())
      break;
    c = read_char();
    if ((c >= '0' && c <= '9') && index > 0) {
      vals << c;
      continue;
    }
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
      vals << c;
      continue;
    }
    // utf-8 support
    if (c >= 0x80 && c != 0xff)
    {
        vals << c;
        continue;
    }
    putback_char(c);
    break;
  }
  std::string val = vals.str();
  if (val.length() == 0)
    return Token(ERR,"");
  if (keywords.find(val) != keywords.end())
    return Token(KW,val);
  return Token(ID,val);
}

void Lexer::set_line_preprocessor(const char* _pp) {
  ogm_assert(_pp && strstr(_pp, "#line ") == _pp);
  _pp += strlen("#line");

  // copy preprocessor line so we can edit it.
  char * pp = strdup(_pp);
  _pp = pp;
  ogm_defer(free(const_cast<char *>(_pp)));
  
  int token = 0;
  const char* file = nullptr;
  const char* line = nullptr;
  const char* column = nullptr;

  while (true)
  {
    if (token >= 4) throw ParseError(131, location(), "preprocessor \"#line\" takes at most 3 tokens.");

    // skip whitespace
    while (*pp && isspace(*pp)) ++pp;
    if (!*pp) break;

    token++;

    // parse token
    if (*pp == '"')
    {
      if (token != 1) throw ParseError(131, location(), "preprocessor \"#line\" file token must be first token.");
      file = pp + 1;
      ++pp;
    }
    else 
    {
      if (line) column = pp;
      else line = pp;
    }

    // proceed until whitespace or \", then place 0
    while (*pp && !isspace(*pp) && *pp != '"')
    {
      pp++;
    }
    if (!*pp) break;
    *(pp++) = 0;
  }

  if (token == 0) throw ParseError(131, location(), "preprocessor \"#line\" requires exactly 1 or 2 tokens.");
  
  if (!line) throw ParseError(131, location(), "preprocessor \"#line\" requires line number to be set.");

  m_location[0].m_source_line = std::atoi(line);
  m_location[0].m_source_column = 0;

  if (column) m_location[0].m_source_column = std::atoi(column);

  if (file)
  {
    m_location[0] = LineColumn {
      m_location[0].m_line,
      m_location[0].m_column,
      m_location[0].m_source_line,
      m_location[0].m_source_column,
      file
    };
  }
}

Token Lexer::read_preprocessor() {
    std::stringstream ss;
    unsigned char in = read_char();
    ogm_assert(in == '#');

    ss << "#";
    while (true) {
        if (is->eof()) {
            goto ret;
        }

        in = read_char();

        if (in == '\n')
        {
        ret:
            std::string s = ss.str();

            if (starts_with(s, "#line "))
            {
                set_line_preprocessor(s.c_str());
            }
            else
            {
                // except for #line ppstatements, we put back the newline.
                putback_char(in);
            }

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
    set_peek_location();
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
  if (in == '@') {
    unsigned char in2 = read_char();
    putback_char(in2);
    if (in2 == '"' || in2 == '\'') {
      putback_char(in);
      no_preprocessor = false;
      return read_string();
    }
  }
  if (in >= '0' && in <= '9') {
    int hex = false;
    if (in == '0' && m_v2)
    {
      unsigned char in2 = read_char();
      putback_char(in2);
      if (in2 == 'x')
      {
        hex = 2;
      }
    }
    putback_char(in);
    no_preprocessor = false;
    return read_number(hex);
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
      if (no_decorations)
      {
          read_comment();
          return read_next();
      }
      return read_comment();
    }
    if (in2 == '*') {
      putback_char(in);
      if (no_decorations)
      {
          read_comment_multiline();
          return read_next();
      }
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
  set_peek_location();
  Token to_return = next;
  next = read_next();
  return to_return;
}

const char ops[] = "+-*/<>=&|!%^~?";
const char opas[] = "#@";
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

LLKLexer::LLKLexer(istream* is, int flags, uint16_t k): Lexer(is, flags), k(k) {
    while (buffer.size() + 1 < k && !Lexer::eof())
    {
        locs.push_back(Lexer::location());
        buffer.push_back(Lexer::read());
    }
}

LLKLexer::LLKLexer(std::string s, int flags, uint16_t k): Lexer(s, flags), k(k) {
    while (buffer.size() + 1< k && !Lexer::eof())
    {
        locs.push_back(Lexer::location());
        buffer.push_back(Lexer::read());
    }
}

Token LLKLexer::read() {
  if (!excess.empty())
  {
      Token t = excess.back();
      excess.pop_back();
      return t;
  }
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
