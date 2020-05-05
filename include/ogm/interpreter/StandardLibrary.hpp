// bytecode generator leaves the specification of
// standard ogm function calls to the target platform.

#pragma once

#include "ogm/bytecode/Library.hpp"

namespace ogm { namespace interpreter {
    using namespace ogm;

class StandardLibrary : public ogm::bytecode::Library
{
public:
    virtual bool generate_function_bytecode(std::ostream& out, const char* functionName, uint8_t argc) const override;
    virtual bool generate_constant_bytecode(std::ostream& out, const char* constantName) const override;
    virtual bool variable_definition(const char* variableName, ogm::bytecode::BuiltInVariableDefinition& outDefinition) const override;
    virtual bool generate_variable_bytecode(std::ostream& out, variable_id_t address, size_t pop_count, bool store) const override;
    virtual bool dis_function_name(bytecode::BytecodeStream& in, std::ostream& out) const override;
    virtual void reflection_add_instance_variables(bytecode::ReflectionAccumulator&, bool dry=false) const override;
};

extern const StandardLibrary* standardLibrary;

}}
