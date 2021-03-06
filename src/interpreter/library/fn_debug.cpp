#include "libpre.h"
    #include "fn_debug.h"
    #include "ogm/fn_ogmeta.h"
    #include "fn_string.h"
    #include "fn_math.h"
#include "libpost.h"

#include "ogm/interpreter/Variable.hpp"
#include "ogm/interpreter/debug_log.hpp"
#include "ogm/common/error.hpp"

#include <string>
#include "ogm/common/error.hpp"

#include <iostream>

using namespace ogm::interpreter;
using namespace ogm::interpreter::fn;

void get_real(VO out, V prompt, V def)
{
  Variable vp;    
  string(vp, prompt);
  std::cout<<vp.string_view()<<"> ";
  string_t s;
  std::cin>>s;
  Variable v = s;
  real(out, v);
  vp.cleanup();
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
    fn::round(out, v);
}

void ogm::interpreter::fn::get_string(VO out, V prompt)
{
  get_string(out, prompt, 0);
}

void ogm::interpreter::fn::get_string(VO out, V prompt, V def)
{
    Variable v;    
    string(v, prompt);
    std::cout<<v.string_view()<<"> ";
    string_t s;
    std::getline(std::cin, s);
    out = s;
    v.cleanup();
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

namespace
{
    // collect debug out messages.
    bool g_collect_debug = false;
    std::stringstream g_debug_log;
    std::string g_debug_expected;
}

namespace ogm::interpreter
{
    std::string get_debug_log()
    {
        return g_debug_log.str();
    }

    std::string get_debug_expected()
    {
        return g_debug_expected;
    }

    void clear_debug_log()
    {
        // clear log
        g_debug_log.str(std::string{});
        g_debug_expected = std::string{};
    }

    void set_collect_debug_info(bool collect)
    {
        g_collect_debug = collect;
    }
}

void ogm::interpreter::fn::show_debug_message(VO out, V msg)
{
    Variable v;
    string(v, msg);
    if (g_collect_debug)
    {
        g_debug_log << v.string_view() << std::endl;
    }
    else
    {
        std::cout << v.string_view() << std::endl;
    }
    v.cleanup();
}

void ogm::interpreter::fn::ogm_expected(VO out, V str)
{
    g_debug_expected = str.castCoerce<std::string>();
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
