#pragma once

#include "ogm/common/error.hpp"

#include <algorithm>
#include <cctype>
#include <locale>
#include <sstream>
#include <fstream>
#include <string>
#include <string_view>
#include <cmath>
#include <cstring>
#include <vector>
#include "ogm/common/error.hpp"
#include <functional>

#ifdef PARALLEL_COMPILE
#   include <thread>
#   include <future>
#endif

#if defined(__GNUC__)
#define FORCEINLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline
#endif

template <typename...> struct ErrorWithType;

// TODO: encapsulate these all in the namespace ogm.

#define handle_type(x, T, base) if (T x = dynamic_cast<T>(base))

// hashes string to uint64_t.
uint64_t hash64(const char*);

// binary search for first item not matching the given monotonic predicate
template<typename vector, typename predicate>
typename vector::iterator find_first_matching(const vector& v, const predicate& p)
{
    typename vector::iterator it_left = v.begin();
    typename vector::iterator it_right = v.end();
    while (std::distance(it_left, it_right) > 1)
    {
        typename vector::iterator it_mid = it_left;
        std::advance(it_mid, std::distance(it_left, it_right) / 2);
        if (p(static_cast<typename vector::const_iterator>(it_mid)))
        {
            it_right = it_mid;
        }
        else
        {
            it_left = it_mid;
        }
    }

    return it_left;
}

template<class Base, class Any>
inline bool is_a(Any* ptr) {
    return !! dynamic_cast<Base*>(ptr);
}

static inline char* memdup(const char* mem, size_t length)
{
    char* v = new char[length];
    memcpy(v, mem, length);
    return v;
}

// returns next power of two larger-than-or-equal-to a (or zero, if a=0).
inline uint32_t power_of_two(uint32_t v)
{
    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
}

// trim from https://stackoverflow.com/a/217605

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

// trim from both ends (not in place)
static inline std::string ext_trim(std::string s)
{
  trim(s);
  return s;
}

// trim from end (not in place)
static inline std::string ext_rtrim(std::string s)
{
  rtrim(s);
  return s;
}

// OPTIMIZE: this whole function
// combine: filter out empty strings in output
template<typename Out>
static inline void split(std::vector<Out>& out, std::string_view s, const std::string& delimiter=" ", bool combine=true)
{
    int start = 0;
    while (start < s.size())
    {
        auto next = s.find(delimiter);
        if (next == std::string_view::npos)
        {
            if (!combine || s != "")
            {
                out.emplace_back(s);
            }
            return;
        }
        else
        {
            std::string_view r = s.substr(0, next);
            if (!combine || r != "")
            {
                out.emplace_back(r);
            }
            s = s.substr(next + delimiter.size());
        }
    }
}

static inline std::string remove_extension(std::string path)
{
    std::vector<std::string> s;
    split(s, path, ".");
    return s[0];
}

// number of lines in given string
static inline size_t num_lines(const char* str)
{
    size_t n = 0;
    while (*str)
    {
        if (*str == '\n')
        {
            ++n;
        }
        ++str;
    }

    return n;
}

// returns the nth line of the given string
static inline std::string nth_line(const char* source, size_t _line)
{
    std::string acc = "";
    size_t line = 0;
    while (*source)
    {
        if (*source == '\n')
        {
            ++line;
        }
        else if (line == _line)
        {
            acc += *source;
        }

        ++source;
    }

    return acc;
}

// modified from https://stackoverflow.com/a/874160
inline std::string remove_suffix(const std::string& a, const std::string& suffix)
{
    if (a.length() >= suffix.length())
    {
        if (!a.compare(a.length() - suffix.length(), suffix.length(), suffix))
        {
            return a.substr(0, a.length() - suffix.length());
        }
    }
    return a;
}

inline std::string remove_prefix(const std::string& a, const std::string& prefix)
{
    if (a.length() >= prefix.length())
    {
        if (!a.compare(0, prefix.length(), prefix))
        {
            return a.substr(prefix.length(), a.length());
        }
    }
    return a;
}

// turns a path like "/a/" to "/a"
inline std::string trim_terminating_path_separator(std::string a)
{
    if (a.back() == '\\' || a.back() == '/')
    {
        return a.substr(0, a.length() - 1);
    }

    return a;
}

inline std::string path_leaf(std::string path) {
  size_t last_bsl = path.find_last_of("\\");
  size_t last_rsl = path.find_last_of("/");

  size_t sep;

  if (last_bsl == std::string::npos) {
    if (last_rsl == std::string::npos)
      return "";
    sep = last_rsl;
  }
  else if (last_rsl == std::string::npos) {
    sep = last_bsl;
  } else {
    sep = std::max(last_rsl, last_bsl);
  }

  return path.substr(sep+1,path.length() - sep-1);
}

inline std::string_view common_substring(std::string_view a, std::string_view b)
{
    std::string_view ab { "", 0 };
    while (ab.size() < a.size() && ab.size() < b.size() && a.at(ab.size()) == b.at(ab.size()))
    {
        ab = a.substr(0, ab.size() + 1);
    }
    return ab;
}

// pulls out the first element of the path
// path is then set to the remainder.
// intermediate path separator is removed completely.
// if no separator in path, then path becomes "" and previous value is returned.
inline std::string path_split_first(std::string& path) {
  size_t last_bsl = path.find("\\");
  size_t last_rsl = path.find("/");

  size_t sep;

  if (last_bsl == std::string::npos) {
    if (last_rsl == std::string::npos)
    {
        std::string _prevpath = path;
        path = "";
        return _prevpath;
    }
    sep = last_rsl;
  }
  else if (last_rsl == std::string::npos) {
    sep = last_bsl;
  } else {
    sep = std::min(last_rsl, last_bsl);
  }

  std::string head = path.substr(0, sep);
  path = path.substr(sep+1);
  return head;
}

#ifdef _WIN32
const char PATH_SEPARATOR = '\\';
#else
const char PATH_SEPARATOR = '/';
#endif

inline std::string path_directory(std::string path) {
  size_t last_bsl = path.find_last_of("\\");
  size_t last_rsl = path.find_last_of("/");

  size_t sep;

  if (last_bsl == std::string::npos) {
    if (last_rsl == std::string::npos)
      return "." + std::string(1, PATH_SEPARATOR);
    sep = last_rsl;
  }
  else if (last_rsl == std::string::npos) {
    sep = last_bsl;
  } else {
    sep = std::max(last_rsl, last_bsl);
  }

  return path.substr(0,sep+1);
}

// from https://stackoverflow.com/a/24315631
inline std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

inline std::string string_lowercase(std::string in)
{
    for (size_t i = 0; i < in.size(); ++i)
    {
        in[i] = std::tolower(in[i]);
    }
    return in;
}

inline std::string string_uppercase(std::string in)
{
    for (size_t i = 0; i < in.size(); ++i)
    {
        in[i] = std::toupper(in[i]);
    }
    return in;
}

// converts / and \\ to native path separator
inline std::string native_path(std::string path) {
  #ifdef _WIN32
  return replace_all(path,"/","\\");
  #elif defined __unix__
  return replace_all(path,"\\","/");
  #else
  ogm_assert(false);
  #endif
}

bool path_exists(const std::string&);
bool can_read_file(const std::string&);

// lists files matching wildcard
std::vector<std::string> __glob(const std::string&);

// "base" should end with path separator.
void list_paths(const std::string& base, std::vector<std::string>& out);
void list_paths_recursive(const std::string& base, std::vector<std::string>& out);

bool path_is_directory(const std::string&);

// finds a matching path based on this case-insensitive descriptor
// "base" is the case-sensitive start, and "head" is case-insensitive.
// condition: "head" must _not_ start with the path separator character;
// "base" _must_ end with the path separator charater.
// if out_casechange is provided, it will be set to true if a case change had to
// be performed (it will not be set to false otherwise).
std::string case_insensitive_path(const std::string& base, const std::string& head, bool* out_casechange=nullptr);

// as above, but also converts to native path (replaces path separator character)
inline std::string case_insensitive_native_path(const std::string& base, const std::string& head, bool* out_casechange=nullptr)
{
    return case_insensitive_path(native_path(base), native_path(head), out_casechange);
}

inline std::string read_file_contents(const std::string& path_to_file) {
  std::string line;
  std::string out;

  std::ifstream infile(native_path(path_to_file));
  while (getline(infile, line))
  {
      out += line + "\n";
  }

  return out;
}

inline std::string read_file_contents(std::ifstream& infile) {
  std::string out;
  std::string line;
  while (getline(infile, line))
  {
      out += line + "\n";
  }

  return out;
}

inline void write_file_contents(const char* file, const char* data, size_t len)
{
    std::ofstream of;
    of.open(file, std::ios::out | std::ios::binary);
    of.write(data, len);
    of.close();
}

inline std::pair<uint32_t,uint32_t> first_difference(const std::string& a, const std::string& b) {
  if (a == b)
    return std::pair<int, int>(-1,-1);

  uint32_t line = 1;
  for (uint32_t x=0;x<std::min(a.size(),b.size());x++) {
    if (a[x] != b[x])
      return std::pair<uint32_t, uint32_t>(x,line);
    if (a[x] == '\n')
      line ++;
  }

  return std::pair<uint32_t, uint32_t> (
    std::min(a.size(),b.size()),
    line
  );
}

// gets address character at the given line, column (0-indexed)
// returns nullptr if not found.
inline const char* get_string_position_line_column(const char* in, size_t line, size_t col)
{
    const char* c = in;
    size_t _line = 0;
    size_t _col = 0;
    while (*c)
    {
        if (_line == line && _col == col)
        {
            return c;
        }

        if (*c == '\n')
        {
            _col = 0;
            ++_line;
            if (_line > line)
            {
                return nullptr;
            }
        }
        else
        {
            ++_col;
        }
        ++c;
    }

    return nullptr;
}

inline bool ends_with(const std::string_view& full, const std::string_view& suffix) {
  if (suffix.length() > full.length())
    return false;
  return (full.substr(full.length() - suffix.length(), suffix.length()) == suffix);
}

inline bool starts_with(const std::string_view& full, const std::string_view& prefix) {
  if (prefix.length() > full.length())
    return false;
  return (full.substr(0, prefix.length()) == prefix);
}

// https://stackoverflow.com/a/8889045
inline bool is_digits(const std::string& str)
{
    return str.find_first_not_of("0123456789") == std::string::npos;
}

// inefficient implementation of int-to-string.
template<typename T>
inline std::string itos(T i)
{
    std::string s;
    std::string n = "";
    if (i == 0)
    {
        return "0";
    }
    if (i < 0)
    {
        i *= -1;
        n = "-";
    }
    while (i > 0)
    {
        T dig = i % 10;
        char v[2];
        v[0] = static_cast<char>('0' + dig);
        v[1] = 0;
        s = std::string(&v[0]) + s;
        i /= 10;
    }
    return n + s;
}

inline char* new_c_str(const char* original)
{
    char* c = new char[strlen(original) + 1];
    strcpy(c, original);
    return c;
}

inline const bool machine_is_big_endian()
{
    uint16_t x = 0x0100;
    char* b = static_cast<char*>(static_cast<void*>(&x));
    return !!b[0];
}

template<typename E>
inline constexpr uint32_t enum_pair(E a, E b)
{
    return (static_cast<uint32_t>(a) << 16) | static_cast<uint32_t>(b);
}

inline double pi()
{
    return std::atan(1)*4;
}

template<typename T>
inline T positive_modulo(T a, T b)
{
    if (b == 0)
    {
        return 0;
    }
    return a - std::floor(a / b) * b;
}

// checks if a standard container contains the given key.
template<typename Container, typename KeyType>
inline bool std_contains(Container& ds, const KeyType& k)
{
    return ds.find(k) != ds.end();
}

// https://stackoverflow.com/a/40766163
inline char* _strdup (const char* s)
{
  size_t slen = strlen(s);
  char* result = static_cast<char*>(malloc(slen + 1));
  if(result == NULL)
  {
    return NULL;
  }

  memcpy(result, s, slen+1);
  return result;
}

template<typename T>
inline void VALGRIND_CHECK_INITIALIZED(const T& t)
{
#ifndef NDEBUG
    static volatile bool A, B;
    (void)A;
    (void)B;
    if (t)
    {
        A = true;
    }
    else
    {
        B = true;
    }
#endif
}

template<typename mapped_type, typename iterator_type>
inline mapped_type mapped_minimum(iterator_type begin, iterator_type end, std::function<mapped_type(decltype(*begin))> c)
{
    ogm_assert(begin != end);
    mapped_type min = c(*begin);
    ++begin;
    while (begin != end)
    {
        mapped_type cmp = c(*begin);
        if (cmp < min)
        {
            min = cmp;
        }
        ++begin;
    }
    return min;
}

template<typename mapped_type, typename iterator_type>
inline mapped_type mapped_maximum(iterator_type begin, iterator_type end, std::function<mapped_type(decltype(*begin))> c)
{
    ogm_assert(begin != end);
    mapped_type min = c(*begin);
    ++begin;
    while (begin != end)
    {
        mapped_type cmp = c(*begin);
        if (min < cmp)
        {
            min = cmp;
        }
        ++begin;
    }
    return min;
}

inline bool is_whitespace(const std::string_view& s)
{
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (!isspace(s[i]))
        {
            return false;
        }
    }
    return true;
}

std::string pretty_typeid(const std::string& name);

static const double PI = pi();
static const double TAU = 2 * PI;

static const bool IS_BIG_ENDIAN = machine_is_big_endian();
