#include "input.hpp"

#include "ogm/common/util.hpp"

#include <iostream>
#include <string>

#ifdef READLINE_AVAILABLE
#include <readline/readline.h>
#include <readline/history.h>
#endif

namespace ogmi
{
#ifdef READLINE_AVAILABLE

namespace
{
    std::string prev;
}

std::string input()
{
    char* _input = readline("(ogmdb) ");
    
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

#else

std::string input()
{
    std::cout << "(ogmdb) " << std::flush;
    std::string _input;
    std::getline(std::cin, _input);
    return _input;
}

#endif
}