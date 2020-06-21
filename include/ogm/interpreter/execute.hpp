// execute bytecode.

#pragma once

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/asset/AssetTable.hpp"
#include "ogm/ast/parse.h"
#include "ogm/interpreter/Instance.hpp"

#include "StandardLibrary.hpp"

#include <iostream>

namespace ogm::interpreter
{
    using namespace ogm::bytecode;
    // all the functions here return true if execution suspended and can be resumed.
    // (execution suspension primarily occurs when using Emscripten for reasons relating to how browsers work.)

    // executes bytecode with the given arguments and copies return values to retv
    // retv must be an array of variables of size at least `b.retc`
    // contents of retv will be overwritten (will not be cleanup'd).
    // caller is responsible for cleaning up the contents of retv afterward.
    bool call_bytecode(bytecode::Bytecode b, Variable* retv, uint8_t argc=0, const Variable* argv=nullptr);

    // executes the given bytecode directly.
    //
    // if "args" is set to true, then it is expected that the caller will have
    // already pushed arguments onto the stack in the correct format
    // (a series of arg variables followed by the number of args.)
    //
    // as a postcondition, those arguments will be popped and cleanup'd
    // (regardless of if "args" is true.)
    bool execute_bytecode(bytecode_index_t bytecode_index, bool args=false);
    bool execute_bytecode(bytecode::Bytecode b, bool args=false);

    // in very specific circumstances, bytecode can suspend execution
    // and be resumed later. (mostly intended for use with Emscripten)
    bool execute_resume();

    // calls the bytecode index if it exists
    // not compatible with arguments (since they won't be popped)
    bool execute_bytecode_safe(bytecode_index_t bytecode_index);
}
