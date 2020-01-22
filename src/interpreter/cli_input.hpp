#pragma once

#include <functional>
#include <string>

// debugger user input.

namespace ogm { namespace interpreter
{
    std::string input(
        const char* prompt="> ",
        char** (*completer)(const char* text, int start, int end)=nullptr
    );

    // current contents of entry buffer
    const char* get_rl_buffer();
}}
