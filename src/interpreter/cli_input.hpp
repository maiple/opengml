#pragma once

#include <functional>
#include <string>
#include <vector>

// debugger user input.

namespace ogm { namespace interpreter
{
    typedef void (*input_completer_t)(const char* text, std::vector<std::string>& out);

    std::string input(
        const char* prompt="> ",
        input_completer_t=nullptr
    );
}}
