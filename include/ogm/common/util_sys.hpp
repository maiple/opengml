// This file is lists the api for various
// system and filesystem-related functions whose implementations
// are dependent on platform and compiler.
//
// for implementations, see: src/common/fs/

#pragma once

#include "ogm/common/types.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"

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

namespace ogm {

// turns a path like "/a/" to "/a"
inline std::string trim_terminating_path_separator(std::string a)
{
    if (a.back() == '\\' || a.back() == '/')
    {
        return a.substr(0, a.length() - 1);
    }

    return a;
}

inline std::string path_leaf(const std::string& path) {
  size_t last_bsl = path.find_last_of("\\");
  size_t last_rsl = path.find_last_of("/");

  size_t sep;

  if (last_bsl == std::string::npos) {
    if (last_rsl == std::string::npos)
      return path;
    sep = last_rsl;
  }
  else if (last_rsl == std::string::npos) {
    sep = last_bsl;
  } else {
    sep = std::max(last_rsl, last_bsl);
  }

  return path.substr(sep+1);
}

inline std::string path_basename(const std::string& path) {
    return path_leaf(path);
}

// TODO: ignore extensions in path name.
static inline std::string remove_extension(std::string path)
{
    std::vector<std::string> s;
    split(s, path, ".");
    return s[0];
}

static inline std::string get_extension(std::string path)
{
    path = path_leaf(path);
    auto index = path.find(".");
    if (index == std::string::npos)
    {
        return "";
    }
    else
    {
        return path.substr(index);
    }
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

inline std::string path_join(std::string a, std::string b)
{
    return trim_terminating_path_separator(a) + std::string(1, PATH_SEPARATOR) + b;
}

template<typename... T>
static std::string path_join(std::string a, std::string b, T... c)
{
    return path_join(path_join(a, b), c...);
}

// result ends with PATH_SEPARATOR
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

// converts / and \\ to native path separator
inline std::string native_path(std::string path) {
  #ifdef _WIN32
  return replace_all(path,"/","\\");
  #elif defined(__unix__) || defined (__APPLE__)
  return replace_all(path,"\\","/");
  #else
  ogm_assert(false);
  #endif
}

bool path_exists(const std::string&);
bool can_read_file(const std::string&);

// returns true if successful
bool create_directory(const std::string& path);

// returns true if successful
bool remove_directory(const std::string& path);

// returns path to the directory containing the executable.
std::string get_binary_directory();

// returns the folder where temporary files go.
std::string get_temp_root();

// creates a temporary directory.
std::string create_temp_directory();

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

uint64_t get_file_write_time(const std::string& path_to_file);

inline std::string read_file_contents(const std::string& path_to_file) {
  std::string line;
  std::string out;

  std::ifstream infile{
      native_path(path_to_file)
  };

  if (!infile.good())
  {
      throw MiscError("File not found: " + path_to_file);
  }

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


void sleep(int32_t ms);

constexpr inline bool is_64_bit()
{
    return (sizeof(void*) == 8);
}

constexpr inline bool is_32_bit()
{
    return (sizeof(void*) == 4);
}

#ifdef _MSC_VER
// for debugging purposes
#define STRING2(x) #x
#define STRING(x) STRING2(x)

#pragma message(STRING(_MSC_VER))
#endif


static_assert(
    #ifdef OGM_X32
        is_32_bit() && ! is_64_bit()
    #elif defined(OGM_X64)
        is_64_bit() && ! is_32_bit()
    #else
        false
    #endif
    , "architecture bits detection (32/64) fail"
);

// running in terminal?
bool is_terminal();
bool terminal_supports_colours();

void enable_terminal_colours();
void restore_terminal_colours();

inline std::string ansi_colour(const char* col)
{
    const bool colour = terminal_supports_colours();
    return ((colour) ? ("\033[" + std::string(col) + "m") : "");
}

}