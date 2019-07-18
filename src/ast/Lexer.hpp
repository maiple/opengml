#include <iostream>
#include <string>
#include <istream>
#include <queue>

#ifndef LEXER_H
#define LEXER_H

enum TokenType {
  PUNC,
  OP,
  OPR,
  OPA,
  NUM,
  STR,
  KW,
  ID,
  PPDEF,
  COMMENT,
  WS,
  ENX,
  END,
  ERR,
};

static const char* TOKEN_NAME[] = {
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

static const char* TOKEN_NAME_PLAIN[] = {
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

struct LineColumn
{
    // first line is 0.
    size_t m_line;
    
    // first column is 0.
    size_t m_col;
    
    inline std::pair<int32_t, int32_t> pair() const
    {
        return{ m_line, m_col };
    }
};

struct Token {
  TokenType type;
  std::string value;
  Token();
  Token(const TokenType type, const std::string value);
  bool operator==(const Token& other) const;
  bool operator!=(const Token& other) const;
  bool is_op_keyword();
};

std::ostream &operator<<(std::ostream &,const Token &);

class Lexer {
public:
  Lexer(std::istream*);
  Lexer(std::string);
  ~Lexer();

  // gobbles the next token, returning it
  virtual Token read();
  
  // peeks at next token but does not gobble it
  Token peek() const;
  
  // returns (row, column) pair of where the lexer currently is in the input
  LineColumn location() const;
  
  // end of file has been reached; peek() or read() will fail.
  virtual bool eof();
private:
  Token next;
  std::istream* is;
  bool istream_mine; // ownership of is.
  bool no_preprocessor = false;
  unsigned int row=0;
  unsigned int col=0;
  unsigned int prev_line_col=0;
  
  char read_char();
  void putback_char(char c);
  
  Token read_next();
  Token read_string();
  Token read_number(bool hex = false);
  Token read_comment();
  Token read_comment_multiline();
  Token read_operator();
  Token read_ident();
  Token read_preprocessor();
  
  bool is_op_char(const unsigned char);
  bool is_opa_char(const unsigned char);
  bool is_punc_char(const unsigned char);
};

class LLKLexer: Lexer {
public:
  LLKLexer(std::istream*, const int k);
  LLKLexer(std::string, const int k);
  
  Token peek() const;
  Token peek(unsigned int skip) const;
  LineColumn location() const;
  Token read();
  bool eof() const;
  bool has(unsigned int k) const;

private: 
  const int k;
  std::deque<LineColumn> locs;
  std::deque<Token> buffer;
};

#endif /* LEXER_H */
