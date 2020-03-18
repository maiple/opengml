#include <algorithm>
#include <cctype>

#include <sstream>
#include <fstream>
#include <string>

#ifndef ERROR_H
#define ERROR_H

//! when parsing causes an error
class ParseError : public std::exception {
public:
    ParseError(std::string message, std::pair<int,int> location = std::pair<int, int>(-1, -1)) :
    message(message + "\n    at row "
            + std::to_string(location.first)
            + ", column " + std::to_string(location.second)),
    location(location) { }
    virtual const char* what() const noexcept override {
        return message.c_str();
    }
private:
    std::string message;
    std::pair<int,int> location;
};

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

class AssertionError : public std::exception
{
public:
    AssertionError() : message("Fatal assertion.") { }
    AssertionError(std::string s) : message(s) { }
    virtual const char* what() const noexcept override {
        return message.c_str();
    }
private:
    std::string message;
};

#ifndef NDEBUG
#define ogm_assert(expression) {if (!(expression)) throw AssertionError("Assertion \""#expression"\" failed on line " + std::to_string(__LINE__) + " in file " __FILE__);}
#else
#define ogm_assert(expression) {}
#endif

#endif /*ERROR_H*/
