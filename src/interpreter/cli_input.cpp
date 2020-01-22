#include "cli_input.hpp"

#include "ogm/common/util.hpp"

#include <iostream>
#include <string>

#ifdef READLINE_AVAILABLE
#include <readline/readline.h>
#include <readline/history.h>
#endif

namespace ogm { namespace interpreter
{
#ifdef READLINE_AVAILABLE

namespace
{
    std::string prev;
}

std::string input(const char* prompt, char** (*completer)(const char* text, int start, int end))
{
    rl_attempted_completion_function = completer;

    char* _input = readline(prompt);

    if (_input)
    {
        std::string s{ _input };
        trim(s);
        if (s.length() > 0 && s != prev)
        {
            // add line to history
            add_history(s.c_str());
            prev = s;
        }

        free(_input);

        return s;
    }
    else
    {
        return "";
    }
}

const char* get_rl_buffer()
{
    return rl_line_buffer;
}

#else

std::string input(const char* prompt, char** (*completer)(const char* text, int start, int end))
{
    std::cout << prompt << std::flush;
    std::string _input;
    std::getline(std::cin, _input);
    return _input;
}

const char* get_rl_buffer()
{
    return "";
}

#endif
}}
