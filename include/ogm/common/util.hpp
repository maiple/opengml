/*
    This is a messy file containing many helpful functions and macros
    for general-purpose coding. Please add anything you like here if
    you think it will be helpful across the board.

    Future refactoring: how to clean up this header...
*/

#pragma once

#include "ogm/common/types.hpp"
#include "ogm/common/error.hpp"

#include <cctype>
#include <sstream>
#include <fstream>
#include <string>
#include <string_view>
#include <cmath>
#include <cstring>
#include <vector>
#include <functional>
#include <numeric>
#include <algorithm>
#include <memory>

#ifdef PARALLEL_COMPILE
#   include <thread>
#   include <future>
#endif

#if defined(__GNUC__)
#define FORCEINLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline
#endif

namespace ogm
{

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
// examples:
//     4 -> 4;
//     5 -> 8;
//   754 -> 1024
inline uint64_t power_of_two(uint64_t v)
{
    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
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

inline std::string_view common_substring(std::string_view a, std::string_view b)
{
    std::string_view ab { "", 0 };
    while (ab.size() < a.size() && ab.size() < b.size() && a.at(ab.size()) == b.at(ab.size()))
    {
        ab = a.substr(0, ab.size() + 1);
    }
    return ab;
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

  return std::pair<uint32_t, uint32_t> {
    static_cast<uint32_t>(std::min<size_t>(
        a.size(),
        b.size()
    )),
    line
  };
}

inline bool get_string_line_column_position(const char* in, const char* substr, size_t& out_line, size_t& out_col)
{
    if (substr < in)
    {
        return false;
    }
    
    out_line = 0;
    out_col = 0;
    
    while (in < substr)
    {
        if (*in == 0)
        {
            return false;
        }
        if (*in == '\n')
        {
            out_line++;
            out_col = 0;
        }
        else
        {
            out_col++;
        }
        
        in++;
    }
    
    return true;
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

// retrives the start of the line for the given pointer in the given string.
inline const char* get_line_start(const char* source, const char* index, size_t count=0)
{
    while (index > source)
    {
        --index;
        if (*index == 0)
        {
            return index + 1;
        }
        if (*index == '\n')
        {
            if (count-- == 0)
            {
                return index + 1;
            }
        }
    }
    return index;
}

// retrives the first line end for the given string
inline const char* get_line_end(const char* index, size_t count=0)
{
    // for unknown reasons, the logic below requires this?
    // FIXME: this is a workaround.
    if (count > 0) count++;

    const char* start = index;
    if (*index == '\r') ++index;
    while (*index)
    {
        if (*index == 0) return index;
        if (*index == '\n')
        {
            if (count-- == 0)
            {
                --index;
                if (*index == '\r' && index > start)
                {
                    return index - 1;
                }
                return index + 1;
            }
        }

        ++index;
    }
    return index;
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

//https://stackoverflow.com/a/5932408
inline bool is_real(const char* str)
{
    char* endptr = 0;
    strtod(str, &endptr);

    if(*endptr != '\0' || endptr == str)
        return false;
    return true;
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

inline bool contains(const std::string& a, const std::string& b)
{
    return a.find(b) != std::string::npos;
}

template<typename T>
inline T* alloc(size_t size=1)
{
    return static_cast<T*>(malloc(size * sizeof(T)));
}

// https://stackoverflow.com/a/40766163
inline char* _strdup (const char* s)
{
  size_t slen = strlen(s);
  char* result = (alloc<char>(slen + 1));
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

// std::stoi, but for string_view
inline int32_t svtoi(const std::string_view& s)
{
    // TODO: optimize
    return std::stoi(std::string{ s });
}

// std::stod, but for string_view
inline double svtod(const std::string_view& s)
{
    // TODO: optimize
    return std::stod(std::string{ s });
}

// as above, but also accepts as a % of max.
inline double svtod_or_percent(const std::string_view& s, double max=1)
{
    if (ends_with(s, "%"))
    {
        return svtod(s) * max / 100.0;
    }
    else
    {
        return svtod(s);
    }
}

inline int32_t svtoi_or_percent(const std::string_view& s, double max)
{
    return svtod_or_percent(s, max);
}

// https://stackoverflow.com/a/12399290
template <typename T>
std::vector<size_t> sort_indices(const std::vector<T> &v) {

  // initialize original index locations
  std::vector<size_t> idx(v.size());
  std::iota(idx.begin(), idx.end(), 0);

  // sort indexes based on comparing values in v
  std::sort(idx.begin(), idx.end(),
       [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});

  return idx;
}

template <typename T, typename C>
std::vector<size_t> sort_indices(const std::vector<T> &v, const C& c) {

    // initialize original index locations
    std::vector<size_t> idx(v.size());
    std:: iota(idx.begin(), idx.end(), 0);

    // sort indexes based on comparing values in v
    std::sort(idx.begin(), idx.end(),
        [&v, &c](size_t i1, size_t i2)
        {
            return c(v[i1], v[i2]);
        }
    );

    return idx;
}

// in-place
// replaces ><& with &gt;&lt;&amp;
inline void xml_sanitize(std::string& s, bool attribute=false)
{
    // OPTIMIZE: use an in-place replace_all function.
    s = replace_all(s, "&", "&amp;");
    s = replace_all(s, "<", "&lt;");
    s = replace_all(s, ">", "&gt;");
    if (attribute)
    {
        s = replace_all(s, "\n", "&#xA;");
        s = replace_all(s, "\"", "&quot;");
    }
}

inline void xml_desanitize(std::string& s)
{
    if (strchr(s.c_str(), '<') || strchr(s.c_str(), '>') || strstr(s.c_str(), "&&"))
    {
        throw MiscError("unsanitized characters found in xml string.");
    }
    s = replace_all(s, "&gt;", ">");
    s = replace_all(s, "&lt;", "<");
    s = replace_all(s, "&quot;", "\"");
    s = replace_all(s, "&#xA;", "\n");
    s = replace_all(s, "&amp;", "&");
}

std::string join(const std::vector<std::string>& vec, const std::string& sep="");

template<typename T, typename C>
void erase_if(std::vector<T>& v, C& callback)
{
    v.erase(
        std::remove_if(v.begin(), v.end(), callback),
        v.end()
    );
}

inline std::string escape(unsigned char a)
{
    if (a >= 0x20 && a < 127)
    {
        return std::string{1, static_cast<char>(a)};
    }
    if (a >= 0x80)
    {
        return "\\?";
    }
    switch (a)
    {
    case '\t': return "\\t";
    case '\n': return "\\n";
    case '\r': return "\\r";
    case 0x7f: return "\\^D";
    default:
        if (a < 100)
        {
            return "\\" + std::to_string(static_cast<int32_t>(a));
        }
        return std::to_string(static_cast<int32_t>(a));
    }
}

inline std::string pluralize(const std::string& name, int count)
{
    if (count == 1) return name;
    return name + "s";
}

inline std::string pluralize(const std::string& name, int count, const std::string& plural_form)
{
    if (count == 1) return name;
    return plural_form;
}

inline std::string npluralize(const std::string& name, int count)
{
    if (count == 1) return "1 " + name;
    return std::to_string(count) + " " + name + "s";
}

inline std::string npluralize(const std::string& name, int count, const std::string& plural_form)
{
    if (count == 1) return "1 " + name;
    return std::to_string(count) + " " + plural_form;
}

std::string pretty_typeid(const std::string& name);

static const double PI = pi();
static const double TAU = 2 * PI;

static const bool IS_BIG_ENDIAN = machine_is_big_endian();

#define OGM_PASTE_HELPER(a,b) a ## b
#define OGM_PASTE(a,b) OGM_PASTE_HELPER(a,b)

#define ogm_defer(x) std::shared_ptr<void> OGM_PASTE(_defer_, __LINE__) \
    ((void*) 1, [&](...){ x; })

template<typename T>
inline T clamp(T x, T a, T b)
{
    if (x < a) return a;
    if (x > b) return b;
    return x;
}


}
