#include <iostream>
#include <string>
#include <istream>
#include <queue>

#include "ogm/common/util.hpp"
#include "COWString.hpp"

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

extern const char* TOKEN_NAME[];

extern const char* TOKEN_NAME_PLAIN[];

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

struct CmpToken {
    TokenType type;
    const char* value;
    CmpToken(TokenType t, const char* v)
        : type(t)
        , value(v)
    { }
};

struct Token {
  TokenType type;
  COWString value;
  FORCEINLINE Token(): Token(ERR,"")
  { }
  FORCEINLINE Token(const Token& other):
    type(other.type),
    value(other.value)
  { }

  FORCEINLINE Token(const TokenType type, const std::string value):
    type(type),
    value(value)
  { }
  FORCEINLINE bool operator==(const Token& other) const {
      if (type != other.type) return false;
      return value == other.value;
  }
  FORCEINLINE bool operator!=(const Token& other) const {
    return !(*this == other);
  }

  FORCEINLINE bool operator==(const CmpToken& other) const {
      if (type != other.type) return false;
      return value == other.value;
  }
  FORCEINLINE bool operator!=(const CmpToken& other) const {
    return !(*this == other);
  }
  bool is_op_keyword();
};

std::ostream &operator<<(std::ostream &,const Token &);

class Lexer {
public:
  Lexer(std::istream*, int flags=0);
  Lexer(std::string, int flags=0);
  ~Lexer();

  // gobbles the next token, returning it
  virtual Token read();

  FORCEINLINE const Token& peek() const
  {
      return next;
  }

  // returns (row, column) pair of where the lexer currently is in the input
  FORCEINLINE LineColumn location() const
  {
    return{ row, col };
  }

  // end of file has been reached; peek() or read() will fail.
  virtual bool eof()
  {
      return peek().type == END || peek().type == ERR;
  }

private:
  std::istream* is;
  Token next;
  bool istream_mine; // ownership of is.
  bool no_decorations;
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
  LLKLexer(std::istream*, int flags, const uint16_t k);
  LLKLexer(std::string, int flags, const uint16_t k);

  FORCEINLINE const Token& peek() const
  {
      if (!excess.empty())
      {
          return excess.back();
      }
      if (buffer.size() == 0)
      {
          return Lexer::peek();
      }
      return buffer.front();
  }

  FORCEINLINE const Token& peek(unsigned int i) const
  {
      if (!excess.empty())
      {
          if (static_cast<size_t>(i) < excess.size())
          {
              return excess[excess.size() - i - 1];
          }
          i -= static_cast<unsigned int>(excess.size());
      }
      if (i + 1 == k)
        return Lexer::peek();
      else
        return buffer[i];
  }

  Token read();
  FORCEINLINE bool eof() const
  {
      return buffer.size() == 0 && excess.empty();
  }

  FORCEINLINE bool has(unsigned int k) const
  {
      return buffer.size() + excess.size() > k;
  }

  FORCEINLINE void put_back(Token t)
  {
      excess.push_back(t);
  }

  FORCEINLINE LineColumn location() const
  {
      if (locs.empty()) return {0, 0};
      return locs.front();
  }

private:
  const uint16_t k;
  std::vector<Token> excess;
  std::deque<LineColumn> locs;
  std::deque<Token> buffer;
};

#endif /* LEXER_H */
