#include "cli_input.hpp"

#include "ogm/common/util.hpp"

#include <iostream>
#include <string>

#ifdef OGM_CROSSLINE
#include <crossline.h>
#endif

namespace ogm { namespace interpreter
{
#ifdef OGM_CROSSLINE

namespace
{
    #define BUFFSIZE 2048
    char buff[BUFFSIZE];

    // currently-registered input completer
    input_completer_t input_completer = nullptr;

    void completer_dispatch(const char* text, crossline_completions_t* acc)
    {
        if (input_completer)
        {
            std::vector<std::string> completions;
            input_completer(text, completions);
            for (const std::string& c : completions)
            {
                crossline_completion_add(acc, c.c_str(), nullptr);
            }
        }
    }
}

std::string input(const char* prompt, input_completer_t completer)
{
    input_completer = completer;
    crossline_completion_register(completer_dispatch);
    const char* result = crossline_readline(prompt, buff, BUFFSIZE);
    if (result) return result;
    return "";
}

#else

std::string input(const char* prompt, char** (*completer)(const char* text, int start, int end))
{
    std::cout << prompt << std::flush;
    std::string _input;
    std::getline(std::cin, _input);
    return _input;
}

#endif
}}
