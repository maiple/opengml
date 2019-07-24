#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"

#include <string>
#include <cassert>
#include <locale>
#include <cctype>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

namespace
{
    inline unsigned int _char_index(V s, V v)
    {
      if (v.castCoerce<real_t>() < 1)
          return 0;
      if (v.castCoerce<real_t>() >= s.castCoerce<string_t>().length())
          return s.castCoerce<string_t>().length() - 1;
      return (unsigned int)(v.castCoerce<real_t>() - 1);
    }

    inline char_t _char_at(V s, V v)
    {
        return s.castCoerce<string_t>()[_char_index(s, v)];
    }
}

void ogm::interpreter::fn::ansi_char(VO out, V v)
{
  char_t _v{ (char)v.castCoerce<real_t>() };
  string_t s;
  s.push_back(_v);
  out = s;
}

void ogm::interpreter::fn::chr(VO out, V v)
{
  char_t _v(v.castCoerce<real_t>());
  string_t s;
  s.push_back(_v);
  out = s;
}

void ogm::interpreter::fn::ord(VO out, V v)
{
  out = (real_t)(unsigned char)(v.castCoerce<string_t>()[0]);
}

void ogm::interpreter::fn::real(VO out, V v)
{
    out = static_cast<real_t>(std::stod(v.castCoerce<string_t>()));
}

void ogm::interpreter::fn::int64(VO out, V v)
{
    out = static_cast<uint64_t>(std::stol(v.castCoerce<string_t>()));
}

void ogm::interpreter::fn::string(VO out, V v)
{
    if (v.get_type() == VT_UNDEFINED)
    {
        out = "undefined";
    }
    else if (v.get_type() == VT_BOOL)
    {
        out = v.get<bool>() ? "True" : "False";
    }
    else if (v.get_type() == VT_INT)
    {
        out = itos<int32_t>(v.get<int32_t>());
    }
    else if (v.get_type() == VT_UINT64)
    {
        out = itos<uint64_t>(v.get<uint64_t>());
    }
  else if (v.get_type() == VT_REAL)
  {
    std::string s;
    unsigned long _v_dec = std::floor(std::abs(v.castCoerce<real_t>()));
    if (_v_dec == 0)
      s = "0";
    else while (_v_dec != 0)
    {
      char vc = '0' + (_v_dec % 10);
      std::string sp;
      sp.push_back(vc);
      s = sp + s;
      _v_dec /= 10l;
    }
    if (v.castCoerce<real_t>() < 0)
      s = "-" + s;
    Variable vf;
    frac(vf, v);
    real_t d = std::abs(vf.castCoerce<real_t>());
    if (d!=0)
    {
      s += ".";
      for (int i=0;i<2;i++)
      {
        d *= 10;
        char vc = '0' + (((int)(std::floor(d))) % 10);
        s.push_back(vc);
      }
    }
    out = s;
  }
  else if (v.get_type() == VT_STRING)
  {
    out.copy(v);
  }
  else if (v.get_type() == VT_ARRAY)
  {
    std::stringstream ss;
    ss << '{';
    ss << ' ';
    for (size_t i = 0; i < v.array_height(); ++i)
    {
      bool _first = true;
      ss << "{ ";
      for (size_t j = 0; j < v.array_length(i); ++j)
      {
          if (!_first)
          {
              ss << ',';
          }
          _first = false;
          ss << v.array_at(i, j);
      }
      ss << " }, ";
    }
    ss << ' ';
    ss << '}';
    out = ss.str();
  }
  else if (v.get_type() == VT_PTR)
  {
    std::stringstream ss;
    ss << "<pointer " << v.get<void*>() << ">";
    out = ss.str();
  }
  else
  {
      out = "<unknown data type>";
  }
}

void ogm::interpreter::fn::string_byte_at(VO out, V v, V pos)
{
  std::string s = (char*)v.castCoerce<string_t>().data();
  out = (real_t)s[(unsigned int)pos.castCoerce<real_t>() - 1];
}

void ogm::interpreter::fn::string_byte_length(VO out, V v)
{
  std::string s = (char*)v.castCoerce<string_t>().data();
  out = (real_t)s.length();
}

void ogm::interpreter::fn::string_set_byte_at(VO out, V v, V pos, V b)
{
  std::string s = (char*)v.castCoerce<string_t>().data();
  char* cs = (char*)s.data();
  cs[(unsigned int)pos.castCoerce<real_t>()] = (char) b.castCoerce<real_t>();
  out = (char_t*)s.c_str();
}

void ogm::interpreter::fn::string_char_at(VO out, V v, V pos)
{
  string_t s;
  s.push_back(_char_at(v, pos));
  out = s;
}

void ogm::interpreter::fn::string_ord_at(VO out, V v, V pos)
{
  var ch;
  ogm::interpreter::fn::string_char_at(ch, v, pos);
  ogm::interpreter::fn::ord(out, ch);
}

void ogm::interpreter::fn::string_copy(VO out, V str)
{
  out.copy(str);
}

void ogm::interpreter::fn::string_copy(VO out, V str, V pos)
{
    out = str.castCoerce<string_t>().substr(_char_index(str, pos));
}

void ogm::interpreter::fn::string_copy(VO out, V str, V pos, V len)
{
  if (len.castCoerce<real_t>() + pos.castCoerce<real_t>() >= str.castCoerce<string_t>().length())
  {
      ogm::interpreter::fn::string_copy(out, str, pos);
      return;
  }
  if (len <= var(0))
  {
      out = (char_t*)"";
      return;
  }
  out = str.castCoerce<string_t>().substr(_char_index(str, pos), (int)len.castCoerce<real_t>());
}

void ogm::interpreter::fn::string_count(VO out, V substr, V str)
{
    std::string s = str.castCoerce<string_t>();
    std::string sub = substr.castCoerce<string_t>();
    const char* sc = s.c_str();
    const char* subc = sub.c_str();
    size_t n = 0;
    while (true)
    {
        sc = strstr(sc, subc);
        if (sc)
        {
            ++sc;
            ++n;
        }
        else
        {
            break;
        }
    }
    out = n;
}

void ogm::interpreter::fn::string_delete(VO out, V str, V pos, V count)
{
    Variable b;
    ogm::interpreter::fn::string_copy(out, str, 1, pos);
    ogm::interpreter::fn::string_copy(b, str, pos.castCoerce<int32_t>() + count.castCoerce<int32_t>());
    out += b;
    b.cleanup();
}

void ogm::interpreter::fn::string_digits(VO out, V str)
{
  string_t sanitized;
  for (int i = 0;i < str.castCoerce<string_t>().size(); i++)
  {
    char_t c = str.castCoerce<string_t>().at(i);
    if (c >= '0' && c <= '9')
    {
      sanitized.push_back(c);
    }
  }
  out = sanitized;
}

void ogm::interpreter::fn::string_format(VO out, V r, V tot, V dec)
{
  throw NotImplementedError();
}

void ogm::interpreter::fn::string_insert(VO out, V substr, V str, V pos)
{
    Variable a;
    ogm::interpreter::fn::string_copy(a, str, 1, pos);
    Variable b;
    ogm::interpreter::fn::string_copy(b, str, pos);
    out = a.castCoerce<string_t>() + substr.castCoerce<string_t>() + b.castCoerce<string_t>();
}

void ogm::interpreter::fn::string_length(VO out, V str)
{
    out = (int)str.castCoerce<string_t>().length();
}

void ogm::interpreter::fn::string_letters(VO out, V str)
{
      string_t sanitized;
      for (int i = 0;i < str.castCoerce<string_t>().size(); i++)
      {
            char_t c = str.castCoerce<string_t>().at(i);
            if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z')
                sanitized.push_back(c);
      }
      out = sanitized;
}

void ogm::interpreter::fn::string_lettersdigits(VO out, V str)
{
  string_t sanitized;
  for (int i = 0;i < str.castCoerce<string_t>().size(); i++)
  {
    char_t c = str.castCoerce<string_t>().at(i);
    if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9')
      sanitized.push_back(c);
  }
  out = sanitized;
}

void ogm::interpreter::fn::string_pos(VO out, V substr, V str)
{
    std::string s = str.castCoerce<string_t>();
    std::string sub = substr.castCoerce<string_t>();
    const char* sc = s.c_str();
    const char* subc = sub.c_str();
    size_t n = 0;
    const char* sc2 = strstr(sc, subc);
    if (sc2)
    {
        out = static_cast<uint32_t>(sc2 - sc) + 1;
    }
    else
    {
        out = 0;
    }
}

void ogm::interpreter::fn::string_repeat(VO out, V str, V count)
{
  string_t to_return;
  for (int i = 0; i < count.castCoerce<real_t>(); i++)
  {
    to_return += str.castCoerce<string_t>();
  }
  out = to_return;
}

void ogm::interpreter::fn::string_replace(VO out, V str, V old, V _new)
{
    std::string s = str.castCoerce<string_t>();
    std::string sold = old.castCoerce<string_t>();
    std::string snew = _new.castCoerce<string_t>();
    const char* sc = s.c_str();
    const char* scold = sold.c_str();
    const char* scnew = snew.c_str();
    const char* sc2 = strstr(sc, scold);
    if (sc2)
    {
        size_t start = sc2 - sc;
        size_t end = start + (sold.length());
        s.erase(start, end);
        s.insert(start, snew);
        out = s;
    }
    else
    {
        out = 0;
    }
}

void ogm::interpreter::fn::string_replace_all(VO out, V str, V old, V _new)
{
  throw NotImplementedError();
}

void ogm::interpreter::fn::string_lower(VO out, V str)
{
    std::string s = str.castCoerce<string_t>();
    out = string_lowercase(s);
}


void ogm::interpreter::fn::string_upper(VO out, V str)
{
    std::string s = str.castCoerce<string_t>();
    out = string_uppercase(s);
}

void ogm::interpreter::fn::string_height(VO out, V str)
{
  throw NotImplementedError();
}

void ogm::interpreter::fn::string_height_ext(VO out, V str, V sep, V w)
{
  throw NotImplementedError();
}

void ogm::interpreter::fn::string_width(VO out, V str)
{
  throw NotImplementedError();
}

void ogm::interpreter::fn::string_width_ext(VO out, V str, V sep, V w)
{
  throw NotImplementedError();
}

void ogm::interpreter::fn::clipboard_has_text(VO out)
{
  throw NotImplementedError();
}

void ogm::interpreter::fn::clipboard_get_text(VO out)
{
  throw NotImplementedError();
}

void ogm::interpreter::fn::clipboard_set_text(VO out)
{
  throw NotImplementedError();
}
