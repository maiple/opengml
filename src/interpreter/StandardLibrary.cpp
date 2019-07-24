
#include "library/library.h"

#include "ogm/interpreter/StandardLibrary.hpp"
#include "ogm/interpreter/Variable.hpp"

#include "ogm/bytecode/bytecode.hpp"
#include "ogm/bytecode/stream_macro.hpp"

#include <iostream>
#include <string>
#include <map>
#include <cstring>

#define CONST(name, v)
#define FNDEF0(name) {#name, (void*)static_cast<void (*)(VO)>(&name), 0},
#define FNDEF1(name, ...) {#name, (void*)static_cast<void (*)(VO, V)>(&name), 1},
#define FNDEF2(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V)>(&name), 2},
#define FNDEF3(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V)>(&name), 3},
#define FNDEF4(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V)>(&name), 4},
#define FNDEF5(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V)>(&name), 5},
#define FNDEF6(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V, V)>(&name), 6},
#define FNDEF7(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V, V, V)>(&name), 7},
#define FNDEF8(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V, V, V, V)>(&name), 8},
#define FNDEF9(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V, V, V, V, V)>(&name), 9},
#define FNDEF10(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V, V, V, V, V, V)>(&name), 10},
#define FNDEF11(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V, V, V, V, V, V, V)>(&name), 11},
#define FNDEF12(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V, V, V, V, V, V, V, V)>(&name), 12},
#define FNDEF13(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V, V, V, V, V, V, V, V, V)>(&name), 13},
#define FNDEF14(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V, V, V, V, V, V, V, V, V, V)>(&name), 14},
#define FNDEF15(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V)>(&name), 15},
#define FNDEF16(name, ...) {#name, (void*)static_cast<void (*)(VO, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V)>(&name), 16},
#define FNDEFN(name, ...) {#name, (void*)static_cast<void (*)(VO, unsigned char, const Variable*)>(&name), -1},
#define IGNORE_WARN(name, ...) {#name, (void*)static_cast<void (*)(VO)>(nullptr), -1},
#define ALIAS(name, ...)

// prepend s^ and g^ to prevent namespace conflict
#define SETVAR(name, ...) {"s^" #name, (void*)static_cast<void (*)(V)>(&setv::name), static_cast<int8_t>(-2)},
#define GETVAR(name, ...) {"g^" #name, (void*)static_cast<void (*)(VO)>(&getv::name), 0},
#define SETVARA(name, ...) {"s^" #name, (void*)static_cast<void (*)(VO, V, V, V)>(&setv::name), 3},
#define GETVARA(name, ...) {"g^" #name, (void*)static_cast<void (*)(VO, V, V)>(&getv::name), 2},

namespace
{
    using namespace ogm::interpreter;
    using namespace ogm::interpreter::fn;

    struct FunctionMapEntry
    {
        const char* m_name;
        void* m_ptr;
        int8_t m_argc;

        FunctionMapEntry(const char* name, void* ptr, int8_t argc)
        : m_name(name)
        , m_ptr(ptr)
        , m_argc(argc)
        { }
    };

    const std::vector< FunctionMapEntry >
    fnmap =
    {
        #include "library/all.h"
    };


    struct VariableDefinition
    {
        variable_id_t m_id;
        const char* m_name;
        bool m_read_only;
        bool m_global;
        bool m_function;
        // number of arguments to the function, not counting get/set value/output
        size_t m_function_argc;

        VariableDefinition(variable_id_t id, const char* name, bool read_only, bool global, bool function=false, size_t function_argc=0)
            : m_name(name)
            , m_id(id)
            , m_read_only(read_only)
            , m_global(global)
            , m_function(function)
            , m_function_argc(function_argc)
        { }
    };

    variable_id_t g_next_variable_id = 0;
    variable_id_t g_next_global_id = 0;

    std::vector<VariableDefinition>
    vars =
    {
        #undef FNDEF0
        #undef FNDEF1
        #undef FNDEF2
        #undef FNDEF3
        #undef FNDEF4
        #undef FNDEF5
        #undef FNDEF6
        #undef FNDEF7
        #undef FNDEF8
        #undef FNDEF9
        #undef FNDEF10
        #undef FNDEF11
        #undef FNDEF12
        #undef FNDEF13
        #undef FNDEF14
        #undef FNDEF15
        #undef FNDEF16
        #undef FNDEFN
        #undef IGNORE_WARN
        #define FNDEF0(...)
        #define FNDEF1(...)
        #define FNDEF2(...)
        #define FNDEF3(...)
        #define FNDEF4(...)
        #define FNDEF5(...)
        #define FNDEF6(...)
        #define FNDEF7(...)
        #define FNDEF8(...)
        #define FNDEF9(...)
        #define FNDEF10(...)
        #define FNDEF11(...)
        #define FNDEF12(...)
        #define FNDEF13(...)
        #define FNDEF14(...)
        #define FNDEF15(...)
        #define FNDEF16(...)
        #define FNDEFN(...)
        #define IGNORE_WARN(...)
        #define DEF(name) {g_next_variable_id++, #name, false, false, false},
        #define DEFREADONLY(name) {g_next_variable_id++, #name, true, false, false},

        #include "library/ivars.h"

        #undef DEF
        #undef DEFREADONLY
        #undef SETVAR
        #undef GETVAR
        #undef SETVARA
        #undef GETVARA
        #define DEF(name) {g_next_global_id++, #name, false, true, false},
        #define DEFREADONLY(name) {g_next_global_id++, #name, true, true, false},
        #define SETVAR(name) {g_next_global_id++, #name, false, true, true},
        #define GETVAR(name) {g_next_global_id++, #name, true, true, true},
        #define SETVARA(name) {g_next_global_id++, #name, false, true, true, 2},
        #define GETVARA(name) {g_next_global_id++, #name, true, true, true, 2},


        #include "library/all.h"

        #undef DEF
        #undef DEFREADONLY
        #undef SETVAR
        #undef GETVAR
        #undef SETVARA
        #undef GETVARA
        #undef FNDEF0
        #undef FNDEF1
        #undef FNDEF2
        #undef FNDEF3
        #undef FNDEF4
        #undef FNDEF5
        #undef FNDEF6
        #undef FNDEF9
        #undef FNDEF10
        #undef FNDEF11
        #undef FNDEF12
        #undef FNDEF13
        #undef FNDEF14
        #undef FNDEF15
        #undef FNDEF16
        #undef IGNORE_WARN
        #undef FNDEFN
        #undef CONST
    };

    // startup remap the variable IDs so that variables with the same name share an ID.
    struct STARTUPFN
    {
        STARTUPFN()
        {
            std::map<std::string, variable_id_t> map;
            for (VariableDefinition& vd : vars)
            {
                if (map.find(vd.m_name) == map.end())
                {
                    map[vd.m_name] = vd.m_id;
                }
                else
                {
                    vd.m_id = map[vd.m_name];
                }
            }
        }
    } _;

    struct ConstantDefinition
    {
        const char* m_name;
        Variable m_value;

        ConstantDefinition(const char* name, const Variable& value)
            : m_name(name)
        {
            m_value.copy(value);
        }

        ConstantDefinition(const ConstantDefinition& other)
            : m_name(other.m_name)
        {
            m_value.copy(other.m_value);
        }

        ConstantDefinition(ConstantDefinition&& other)
            : m_name(other.m_name)
            , m_value(std::move(other.m_value))
        { }
    };

    const std::vector<ConstantDefinition> constants =
    {
        #define DEF(...)
        #define DEFREADONLY(...)
        #define SETVAR(...)
        #define GETVAR(...)
        #define SETVARA(...)
        #define GETVARA(...)
        #define FNDEF0(...)
        #define FNDEF1(...)
        #define FNDEF2(...)
        #define FNDEF3(...)
        #define FNDEF4(...)
        #define FNDEF5(...)
        #define FNDEF6(...)
        #define FNDEF7(...)
        #define FNDEF8(...)
        #define FNDEF9(...)
        #define FNDEF10(...)
        #define FNDEF11(...)
        #define FNDEF12(...)
        #define FNDEF13(...)
        #define FNDEF14(...)
        #define FNDEF15(...)
        #define FNDEF16(...)
        #define IGNORE_WARN(...)
        #define FNDEFN(...)
        #define CONST(name, value) {#name, value},
        #include "library/all.h"
        #undef CONST
    };

    const std::map<std::string, std::string> rename =
    {
        #undef ALIAS
        #define ALIAS(dst, src) {#src, #dst},
        #define CONST(...)
        #include "library/all.h"
        #undef ALIAS
    };

    const char* rename_lookup(const char* s)
    {
        if (rename.find(s) != rename.end())
        {
            return rename.at(s).c_str();
        }
        return s;
    }

    std::string rename_lookup(std::string s)
    {
        if (rename.find(s) != rename.end())
        {
            return rename.at(s);
        }
        return s;
    }
}

namespace ogm { namespace interpreter
{

// definition for ogm/interpreter/Compare.hpp
real_t ds_epsilon = 0.0000001;

using namespace ogm::bytecode;
const StandardLibrary _stl;
const StandardLibrary* standardLibrary{ &_stl };

bool StandardLibrary::generate_function_bytecode(std::ostream& out, const char* functionName, uint8_t argc) const
{
    functionName = rename_lookup(functionName);
    const FunctionMapEntry* fme = nullptr;
    for (auto& entry : fnmap)
    {
        if (strcmp(entry.m_name , functionName) == 0)
        {
            if (entry.m_argc == -1 || entry.m_argc == argc || (entry.m_argc == -2 && argc == 1))
            {
                fme = &entry;
                break;
            }
        }
    }

    if (fme == nullptr)
    {
        return false;
    }

    if (fme->m_ptr == nullptr)
    {
        //std::cout << "WARNING: function \"" + std::string(functionName) + "\" is unimplemented." << std::endl;
        for (size_t i = 0; i < argc; ++i)
        {
            write_op(out, ogm::bytecode::opcode::pop);
        }

        // write to stdout about function
        write_op(out, ogm::bytecode::opcode::ldi_string);
        out << "WARNING: executing unimplemented function \"" << functionName << "\"";
        out << (char) 0;
        generate_function_bytecode(out, "show_debug_message", 1);
        write_op(out, ogm::bytecode::opcode::pop);
        write_op(out, ogm::bytecode::opcode::ldi_undef);
    }
    else
    {
        if (fme->m_argc == -1)
        {
            // variadic -- push number of arguments onto stack.
            write_op(out, ogm::bytecode::opcode::ldi_s32);
            int32_t argc_i = argc;
            write(out, argc_i);
        }
        write_op(out, ogm::bytecode::opcode::nat);
        write(out, fme->m_ptr);
        write(out, fme->m_argc);
    }

    return true;
}

bool StandardLibrary::generate_constant_bytecode(std::ostream& out, const char* kName) const
{
    kName = rename_lookup(kName);
    if (strcmp(kName, "false") == 0)
    {
        write_op(out, ogm::bytecode::opcode::ldi_f64);
        real_t v = 0;
        write(out, v);
        return true;
    }
    else if (strcmp(kName, "true") == 0)
    {
        write_op(out, ogm::bytecode::opcode::ldi_f64);
        real_t v = 1;
        write(out, v);
        return true;
    }
    else if (strcmp(kName, "self") == 0)
    {
        write_op(out, ogm::bytecode::opcode::ldi_f64);
        real_t v = -1;
        write(out, v);
        return true;
    }
    else if (strcmp(kName, "other") == 0)
    {
        write_op(out, ogm::bytecode::opcode::ldi_f64);
        real_t v = -2;
        write(out, v);
        return true;
    }
    else if (strcmp(kName, "all") == 0)
    {
        write_op(out, ogm::bytecode::opcode::ldi_f64);
        real_t v = -3;
        write(out, v);
        return true;
    }
    else if (strcmp(kName, "noone") == 0)
    {
        write_op(out, ogm::bytecode::opcode::ldi_f64);
        real_t v = -4;
        write(out, v);
        return true;
    }
    else if (strcmp(kName, "undefined") == 0)
    {
        write_op(out, ogm::bytecode::opcode::ldi_undef);
        return true;
    }
    for (const ConstantDefinition& def : constants)
    {
        if (def.m_name == std::string(kName))
        {
            switch(def.m_value.get_type())
            {
                case VT_UNDEFINED:
                    write_op(out, ogm::bytecode::opcode::ldi_undef);
                    break;
                case VT_INT:
                    write_op(out, ogm::bytecode::opcode::ldi_s32);
                    write(out, def.m_value.get<int32_t>());
                    break;
                case VT_REAL:
                    write_op(out, ogm::bytecode::opcode::ldi_f64);
                    write(out, def.m_value.get<real_t>());
                    break;
                default:
                    throw MiscError("Not Implemented");
            }
            return true;
        }
    }
    return false;
}

bool StandardLibrary::variable_definition(const char* variableName, BuiltInVariableDefinition& outDefinition) const
{
    for (const VariableDefinition& vd : vars)
    {
        if (strcmp(vd.m_name, variableName) == 0)
        {
            outDefinition.m_global = vd.m_global;
            outDefinition.m_read_only = vd.m_read_only;
            outDefinition.m_address = vd.m_id;
            return true;
        }
    }

    return false;
}

bool StandardLibrary::generate_variable_bytecode(std::ostream& out, variable_id_t address, size_t pop_count, bool store) const
{
    // look up in variable list
    // this implementation isn't nice, but it works.
    for (const VariableDefinition& vd : vars)
    {
        if (vd.m_global)
        {
            if (vd.m_id == address && pop_count == vd.m_function_argc)
            {
                std::string function_name = vd.m_name;
                if (store)
                {
                    if (!vd.m_read_only)
                    {
                        function_name = "s^" + function_name;
                        generate_function_bytecode(out, function_name.c_str(), pop_count + 1);
                        return true;
                    }
                }
                else
                {
                    function_name = "g^" + function_name;
                    generate_function_bytecode(out, function_name.c_str(), pop_count);
                    return true;
                }
            }
        }
    }

    return false;
}

bool StandardLibrary::dis_function_name(bytecode::BytecodeStream& in, std::ostream& out) const
{
    void* fptr;
    uint8_t argc;
    read(in, fptr);
    read(in, argc);
    const char* name = nullptr;
    for (const FunctionMapEntry& iter : fnmap)
    {
        if (iter.m_ptr == fptr)
        {
            name = iter.m_name;
            break;
        }
    }

    if (!name)
    {
        return false;
    }

    out << name;
    if (argc == 0)
    {
        out << "()";
    }
    else
    {
        out << "(" ;
        if (argc == static_cast<uint8_t>(-1))
        {
            out << "*";
        }
        else if (argc == static_cast<uint8_t>(-2))
        {
            out << "1v";
        }
        else
        {
            // number of arguments
            out << (int)argc;
        }
        out << ")";
    }
    out << " [" << fptr << "]";
    return true;
}
}}
