#pragma once

#include "ogm/ast/parse.h"
#include <string>
#include <iostream>

namespace ogm
{
namespace beautify
{
    std::string beautify(const ogm_ast_t* ast);
    void beautify(std::ostream& out, const ogm_ast_t* ast);
}
}