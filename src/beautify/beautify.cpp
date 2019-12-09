#include "ogm/beautify/beautify.hpp"
#include "ogm/beautify/BAST.hpp"

#include <sstream>

namespace ogm
{
namespace beautify
{

std::string beautify(const ogm_ast_t* ast)
{
    std::stringstream ss;
    beautify(ss, ast);
    return ss.str();
}

void beautify(std::ostream& out, const ogm_ast_t* ast)
{
    
}

}
}