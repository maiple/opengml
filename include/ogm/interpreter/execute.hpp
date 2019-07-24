// execute bytecode.

#pragma once

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/asset/AssetTable.hpp"
#include "ogm/ast/parse.h"
#include "ogm/interpreter/Instance.hpp"

#include "StandardLibrary.hpp"

#include <iostream>

namespace ogm { namespace interpreter
{
    using namespace ogm::bytecode;

    // executes bytecode with arguments and copies return values to caller
    // retv must be able to store b.retc variables
    // contents of retv will be overwritten (will not be cleanup'd).
    // caller is responsible for cleaning up the contents of retv afterward.
    void call_bytecode(bytecode::Bytecode b, Variable* retv, uint8_t argc=0, const Variable* argv=nullptr);

    // executes the given bytecode directly.
    //
    // if "args" is set to true, then it is expected that the caller will have
    // already pushed arguments onto the stack in the correct format
    // (a series of arg variables followed by the number of arguments.)
    // as a postcondition, those arguments will be popped and cleanup'd.
    void execute_bytecode(bytecode_index_t bytecode_index=0, bool args=false);
    void execute_bytecode(bytecode::Bytecode b, bool args=false);

    // calls the bytecode index if it exists
    // not compatible with arguments (since they won't be popped)
    void execute_bytecode_safe(bytecode_index_t bytecode_index);
}}

