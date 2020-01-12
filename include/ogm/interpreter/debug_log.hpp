#pragma once

#include <string>

// this is used by unit tests.

namespace ogm::interpreter
{
    void set_collect_debug_info(bool);
    void clear_debug_log();
    std::string get_debug_log();
    std::string get_debug_expected();
}
