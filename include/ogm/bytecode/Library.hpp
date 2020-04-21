// bytecode generator leaves the specification of
// standard ogm function calls to the target platform.

#ifndef OGM_BYTECODE_LIBRARY_HPP
#define OGM_BYTECODE_LIBRARY_HPP

#include "Namespace.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"

#include <iostream>

namespace ogm { namespace bytecode {

// information about a built-in variable name.
struct BuiltInVariableDefinition
{
    // false: use stj/ldj. true: use  stb/ldb.
    bool m_global;

    // variable canot be edited.
    bool m_read_only;

    // id of this variable.
    variable_id_t m_address;
};

class Library
{
public:
    // this must either return false or
    // generate bytecode that pops argc variables and
    // puts a single variable onto the stack.
    // (void function calls must put an undefined value on the stack.)
    // return false if the function name is not in the library implementation.
    virtual bool generate_function_bytecode(std::ostream& out, const char* functionName, unsigned char argc) const = 0;

    // this must either return false or
    // generate bytecode that pushes constant value onto the stack.
    virtual bool generate_constant_bytecode(std::ostream& out, const char* constantName) const = 0;
    
    // this must either return false or
    // generate bytecode that accesses a datastructure according to the given accessor type.
    // if store is true, pop_count+1 stack variables must be popped,
    // with the shallowest being the value to store and the deepest being the datastructure index.
    // (intermediate values depend on the accessor -- for example, grids would have x and y)
    // if store is false, pop_count stack variables must be popped and 1 stack variable pushed (the value loaded).
    virtual bool generate_accessor_bytecode(std::ostream& out, accessor_type_t, size_t pop_count, bool store) const;

    // retrieves information about the given built-in variable.
    // returns false if variable is not in library.
    virtual bool variable_definition(const char* variableName, BuiltInVariableDefinition& outDefinition) const = 0;
    
    // writes bytecode for built-in variable
    // returns false if not sure how to handle the variable
    virtual bool generate_variable_bytecode(std::ostream& out, variable_id_t address, size_t pop_count, bool store) const
    {
        return false;
    }

    // this is called after a NAT bytecode is encountered.
    // this should either (a) return false or
    // (b) read in the instruction immediates and write out a description of
    // the function (without a newline ending)
    virtual bool dis_function_name(bytecode::BytecodeStream& in, std::ostream& disOut) const = 0;
};

// the default/empty library.
class EmptyLibrary : public Library
{
    bool generate_function_bytecode(std::ostream&, const char* functionName, unsigned char argc) const
    { return false; }

    bool generate_constant_bytecode(std::ostream& out, const char* constantName) const
    { return false; }
    
    bool generate_accessor_bytecode(std::ostream& out, accessor_type_t, size_t pop_count, bool store) const
    { return false; }

    bool variable_definition(const char* variableName, BuiltInVariableDefinition&) const
    { return false; }

    virtual bool dis_function_name(bytecode::BytecodeStream& in, std::ostream& out) const
    { return false; }
};

extern const EmptyLibrary defaultLibrary;

}}

#endif
