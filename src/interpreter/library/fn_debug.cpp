#include "library.h"
#include "ogm/interpreter/Variable.hpp"
#include "ogm/common/error.hpp"

#include <string>
#include "ogm/common/error.hpp"
#include <locale>
#include <iostream>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

void get_real(VO out, V prompt, V def)
{
  std::cout<<prompt<<"> ";
  string_t s;
  std::cin>>s;
  Variable v = s;
  real(out, v);
}

void get_real(VO out, V prompt)
{
  get_real(out, prompt, "");
}

void ogm::interpreter::fn::get_integer(VO out, V prompt)
{
  get_integer(out, prompt, 0);
}

void ogm::interpreter::fn::get_integer(VO out, V prompt, V def)
{
    Variable v;
    get_real(v, prompt, def);
    round(out, v);
}

void ogm::interpreter::fn::get_string(VO out, V prompt)
{
  get_string(out, prompt, 0);
}

void ogm::interpreter::fn::get_string(VO out, V prompt, V def)
{
    std::cout<<prompt<<"> ";
    string_t s;
    std::cin>>s;
    out = s;
}

void ogm::interpreter::fn::show_message(VO out, V msg)
{
  std::cout<<msg<<std::endl;
}

void ogm::interpreter::fn::show_question(VO out, V msg)
{
  throw NotImplementedError();

  out = 0;
}

void ogm::interpreter::fn::show_debug_message(VO out, V msg)
{
    Variable v;
    string(v, msg);
    std::cout << v.string_view() << std::endl;
    v.cleanup();
}

void ogm::interpreter::fn::show_debug_overlay(VO out, V enable)
{
  throw NotImplementedError();
}

void ogm::interpreter::fn::code_is_compiled(VO out)
{
  out = false;
}

void ogm::interpreter::fn::fps(VO out)
{
  throw NotImplementedError();
  out = static_cast<real_t>(60);
}

void ogm::interpreter::fn::fps_real(VO out)
{
  throw NotImplementedError();
  out = static_cast<real_t>(60);
}
