#pragma once

#include <algorithm>
#include <cctype>

#include <sstream>
#include <fstream>
#include <string>

#include <fmt/core.h>
#include "location.h"

/*
Error categories:
  A: assertion failure error
  C: compiler error
  P: parser error (part of compilation)
  F: project error (part of compilation)
  R: runtime error
*/

struct ogm_location;

namespace ogm
{
    typedef uint32_t error_code_t;
    typedef char error_category_t;

    enum DetailCategory
    {
        location_at,
        location_start,
        location_end,
        source_buffer,
        resource_type,
        resource_name,
        resource_event,
        resource_section
    };

    template<DetailCategory C>
    struct DetailTraits {
      typedef std::string Type;
    };

    template<enum DetailCategory C>
    struct Detail
    {
        bool set = false;
        typename DetailTraits<C>::Type value;
    };

    template<>
    struct DetailTraits<location_at> {
        typedef ogm_location_t Type;
    };

    template<>
    struct DetailTraits<location_start> {
        typedef ogm_location_t Type;
    };

    template<>
    struct DetailTraits<location_end> {
        typedef ogm_location_t Type;
    };

    class Error : public std::exception
    {
    protected:
        template<typename... P>
        Error(error_category_t E, error_code_t code, const char* fmt="", const P&... args)
            : m_category(E)
            , m_code(code)
            , m_message{ fmt::format(fmt, args...) }
        { }

    public:
        const char* what() const noexcept override;

        template<DetailCategory C>
        Error& detail(const typename DetailTraits<C>::Type& v);

    private:
        template<bool colour>
        void assemble_message() const;

        template<bool colour>
        void loc_preview(std::stringstream&, const char*, const char*, const char*, size_t) const;

    private:
        error_category_t m_category;
        error_code_t m_code;
        std::string m_message;

    private:
        mutable std::string m_what;
        
        // details (all are optional)
        Detail<location_at> m_detail_location_at;
        Detail<location_start> m_detail_location_start;
        Detail<location_end> m_detail_location_end;
        Detail<source_buffer> m_detail_source_buffer;
        Detail<resource_name> m_detail_resource_name;
        Detail<resource_type> m_detail_resource_type;
        Detail<resource_event> m_detail_resource_event;
        Detail<resource_section> m_detail_resource_section;
    };

    class RuntimeError : public Error
    {
    public:
        template<typename... P>
        RuntimeError(error_code_t error_code, const char* fmt="", const P&... args)
            : Error('R', error_code, fmt, args...)
        { }
    };

    class AssertionError : public Error
    {
    public:
        template<typename... P>
        AssertionError(const char* fmt="", const P&... args)
            : Error('A', 0, fmt, args...)
        { }
    };

    class ProjectError : public Error
    {
      public:
        template<typename... P>
        ProjectError(error_code_t error_code, const char* fmt="", const P&... args)
            : Error('F', error_code, fmt, args...)
        { }
    };

    class LineNumberError : public Error
    {
    protected:
        template<typename... P>
        LineNumberError(error_category_t E, error_code_t error_code, const ogm_location_t& location, const char* fmt="", const P&... args)
            : Error(E, error_code, fmt, args...)
         {
            detail<location_at>(location);
        }
    };

    class CompileError : public LineNumberError
    {
    public:
        template<typename... P>
        CompileError(error_code_t error_code, const ogm_location_t& location, const char* fmt="", const P&... args)
            : LineNumberError('C', error_code, location, fmt, args...)
        { }
    };

    class ParseError : public LineNumberError
    {
    public:
        template<typename... P>
        ParseError(error_code_t error_code, const ogm_location_t& location, const char* fmt="", const P&... args)
            : LineNumberError('P', error_code, location, fmt, args...)
        { }
    };
}

class UnknownIdentifierError : public std::exception {
public:
  UnknownIdentifierError(std::string message) : message(message) { }
  virtual const char* what() const noexcept override {
    return message.c_str();
  }
private:
  std::string message;
};

//! when testing causes an error
class TestError : public std::exception {
public:
  TestError(std::string message) : message(message) { }
  virtual const char* what() const noexcept override {
    return message.c_str();
  }
private:
  std::string message;
};

class DivideByZeroError : public std::exception {
    virtual const char* what() const noexcept override {
      return "Division by zero.";
    }
};

//! when I'm too lazy to fi
class MiscError : public std::exception {
public:
  MiscError() : message("") { }
  MiscError(std::string message) : message(message) { }
  virtual const char* what() const noexcept override {
    return message.c_str();
  }
private:
  std::string message;
};

// out of heap space, can't new/malloc
class HeapSpaceError : public std::exception {
public:
  HeapSpaceError() : message("Out of heap space.") { }
  HeapSpaceError(std::string message) : message(message) { }
  virtual const char* what() const noexcept override {
    return message.c_str();
  }
private:
  std::string message;
};

class LanguageFeatureNotImplementedError : public std::exception {
public:
  LanguageFeatureNotImplementedError(std::string message) : message("Not implemented: " + message) { }
  virtual const char* what() const noexcept override {
    return message.c_str();
  }
private:
  std::string message;
};

class TypeError : public std::exception {
public:
  TypeError(std::string message) : message(message) { }
  virtual const char* what() const noexcept override {
    return message.c_str();
  }
private:
  std::string message;
};

class NotImplementedError : public std::exception {
public:
  NotImplementedError() : message("function not yet implemented in OGM") { }
  NotImplementedError(std::string s) : message(s) { }
  virtual const char* what() const noexcept override {
    return message.c_str();
  }
private:
  std::string message;
};

// when a behaviour case for which the reference implementation's
// behaviour is uncertain (because it hasn't been investigated yet).
class UnknownIntendedBehaviourError : public std::exception
{
public:
    UnknownIntendedBehaviourError() : message("intended behaviour unknown.") { }
    UnknownIntendedBehaviourError(std::string s) : message(s) { }
    virtual const char* what() const noexcept override {
        return message.c_str();
    }
private:
    std::string message;
};

#ifndef NDEBUG
#define ogm_assert(expression){if (!(expression)) \
  throw ogm::AssertionError("OpenGML internal assertion \""#expression"\" failed on line {} in file \"{}\"", __LINE__, __FILE__);}
#else
#define ogm_assert(expression) {}
#endif