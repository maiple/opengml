#include "catch/catch.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/ast/parse.h"
#include "ogm/interpreter/Variable.hpp"

#include <string>

using namespace ogm::bytecode;
using namespace ogm::interpreter;

TEST_CASE( "Enum names are correctly ordered", "[Enums]" )
{
    // variable type names
    using namespace std::string_literals;
    REQUIRE(variable_type_string[VT_UNDEFINED] == "undefined"s);
    REQUIRE(variable_type_string[VT_STRING] == "string"s);
    REQUIRE(variable_type_string[VT_ARRAY] == "array"s);
    #ifdef OGM_STRUCT_SUPPORT
        REQUIRE(variable_type_string[VT_STRUCT] == "object"s);
    #endif
    REQUIRE(variable_type_string[VT_PTR] == "pointer"s);
    
    // other enums
    REQUIRE(opcode::get_opcode_string(opcode::eof) == "eof"s);
    REQUIRE(ogm_ast_spec_string[ogm_ast_spec_body_braced] == "{ }"s);
    REQUIRE(ogm_ast_subtype_string[ogm_ast_st_imp_enum] == "enum"s);
}