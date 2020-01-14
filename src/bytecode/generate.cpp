#include "ogm/bytecode/bytecode.hpp"
#include "ogm/bytecode/Library.hpp"
#include "ogm/bytecode/Namespace.hpp"
#include "ogm/bytecode/stream_macro.hpp"
#include "ogm/asset/AssetTable.hpp"

#include "ogm/ast/parse.h"
#include "ogm/ast/ast_util.h"

#include "ogm/common/error.hpp"
#include "ogm/common/util.hpp"

#include <cstring>
#include "ogm/common/error.hpp"
#include <map>
#include <string>
#include <cassert>

// && and || skip second term if result is known.
#define SHORTCIRCUIT_EVALUATIONS

namespace ogm { namespace bytecode {

const EmptyLibrary defaultLibrary;

using namespace ogm::asset;
using namespace opcode;

typedef int32_t bytecode_address_t;
constexpr bytecode_address_t k_invalid_pos = -1;
constexpr bytecode_address_t k_placeholder_pos = -2;

const BytecodeTable defaultBytecodeTable;

struct EnumData {
    std::map<std::string, ogm_ast_t*> m_map;

    ~EnumData()
    {
        for (const auto& pair : m_map)
        {
            delete std::get<1>(pair);
        }
    }
};

// FIXME: put this in its own header file.
// Having it just in this cpp file is very awkward,
// because it necessitates ReflectionAccumulator::ReflectionAccumulator()
struct EnumTable {
    std::map<std::string, EnumData> m_map;
};

ReflectionAccumulator::ReflectionAccumulator()
    : m_namespace_instance()
    , m_namespace_global()
    , m_bare_globals()
    , m_ast_macros()
    , m_enums(new EnumTable())
{ }

ReflectionAccumulator::~ReflectionAccumulator()
{
    // free macro ASTs
    for (auto& pair : m_ast_macros)
    {
        ogm_ast_free(std::get<1>(pair));
    }

    // delete enums
    assert(m_enums != nullptr);
    delete(m_enums);
}

// default implementation of generate_accessor_bytecode.
// Because it is unlikely for most implementations to change this implementation, a default is provided.
bool Library::generate_accessor_bytecode(std::ostream& out, accessor_type_t type, size_t pop_count, bool store) const
{
    switch (type)
    {
        case accessor_map:
            if (pop_count != 2)
            {
                throw MiscError("map accessor needs exactly 1 argument.");
            }
            if (store)
            {
                bool success = generate_function_bytecode(out, "ds_map_replace", pop_count + 1);
                if (success)
                {
                    // pop undefined value returned by ds_map_set.
                    write_op(out, pop);
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return generate_function_bytecode(out, "ds_map_find_value", pop_count);
            }
            break;
        case accessor_grid:
            if (pop_count != 3)
            {
                throw MiscError("grid accessor needs exactly 2 arguments.");
            }
            if (store)
            {
                bool success = generate_function_bytecode(out, "ds_grid_set", pop_count + 1);
                if (success)
                {
                    // pop undefined value returned by ds_grid_set.
                    write_op(out, pop);
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return generate_function_bytecode(out, "ds_grid_get", pop_count);
            }
            break;
        case accessor_list:
            if (pop_count != 2)
            {
                throw MiscError("list accessor needs exactly 1 argument.");
            }
            if (store)
            {
                bool success = generate_function_bytecode(out, "ds_list_set", pop_count + 1);
                if (success)
                {
                    // pop undefined value returned by ds_list_set.
                    write_op(out, pop);
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return generate_function_bytecode(out, "ds_list_find_value", pop_count);
            }
            break;
        default:
            return false;
    }
}

struct GenerateContextArgs
{
    GenerateContextArgs(uint8_t retc, uint8_t argc, Namespace& instance, Namespace& globals, const Library* library, const asset::AssetTable* asset_table, const bytecode::BytecodeTable* bytecode_table, std::vector<bytecode_address_t>& break_placeholder_vector, std::vector<bytecode_address_t>& continue_placeholder_vector, std::vector<opcode::opcode_t>& cleanup_commands, DebugSymbols* symbols, ReflectionAccumulator* reflection, GenerateConfig* config)
        : m_retc(retc)
        , m_argc(argc)
        , m_instance_variables(instance)
        , m_globals(globals)
        , m_library(library)
        , m_asset_table(asset_table)
        , m_bytecode_table(bytecode_table)
        , m_break_placeholders(break_placeholder_vector)
        , m_continue_placeholders(continue_placeholder_vector)
        , m_cleanup_commands(cleanup_commands)
        , m_continue_pop_count(0)
        , m_enum_expression(false)
        , m_symbols(symbols)
        , m_reflection(reflection)
        , m_config(config)
    { }

    GenerateContextArgs(const GenerateContextArgs& other)
        : m_retc(other.m_retc)
        , m_argc(other.m_argc)
        , m_instance_variables(other.m_instance_variables)
        , m_globals(other.m_globals)
        , m_library(other.m_library)
        , m_asset_table(other.m_asset_table)
        , m_bytecode_table(other.m_bytecode_table)
        , m_break_placeholders(other.m_break_placeholders)
        , m_continue_placeholders(other.m_continue_placeholders)
        , m_cleanup_commands(other.m_cleanup_commands)
        , m_continue_pop_count(other.m_continue_pop_count)
        , m_enum_expression(other.m_enum_expression)
        , m_symbols(other.m_symbols)
        , m_reflection(other.m_reflection)
        , m_config(other.m_config)
    { }

    uint8_t m_retc, m_argc;

    Namespace& m_instance_variables;
    Namespace& m_globals;

    const Library* m_library;
    const asset::AssetTable* m_asset_table;
    const bytecode::BytecodeTable* m_bytecode_table;

    // addresses which should later be filled in with the break address.
    std::vector<bytecode_address_t>& m_break_placeholders;

    // addresses which should later be filled in with the continue address.
    std::vector<bytecode_address_t>& m_continue_placeholders;

    // the sequence of cleanup commands required before returning.
    std::vector<opcode::opcode_t>& m_cleanup_commands;

    // number of 'pop' statements required to continue (important for switch statements)
    uint16_t m_continue_pop_count;
    bool m_enum_expression;

    DebugSymbols* m_symbols;

    ReflectionAccumulator* m_reflection;

    GenerateConfig* m_config;
};

accessor_type_t get_accessor_type_from_spec(ogm_ast_spec_t s)
{
    switch (s)
    {
        case ogm_ast_spec_acc_map:
            return accessor_map;
        case ogm_ast_spec_acc_grid:
            return accessor_grid;
        case ogm_ast_spec_acc_list:
            return accessor_list;
        default:
            return accessor_none;
    }
}

struct LValue
{
    LValue()
        : m_pop_count(0)
        , m_array_access(false)
        , m_accessor_type(accessor_none)
        , m_no_copy(false)
        , m_read_only(false)
    { }

    LValue(memspace_t memspace, variable_id_t address, size_t pop_count, bool array_access, accessor_type_t accessor_type, bool no_copy, bool read_only)
        : m_memspace(memspace)
        , m_address(address)
        , m_pop_count(pop_count)
        , m_array_access(array_access)
        , m_accessor_type(accessor_type)
        , m_no_copy(no_copy)
        , m_read_only(read_only)
    { }


    memspace_t m_memspace;
    variable_id_t m_address;

    // number of values that need to be popped from the stack to dereference this.
    size_t m_pop_count;

    bool m_array_access;
    accessor_type_t m_accessor_type;
    bool m_no_copy;
    bool m_read_only;
};

// forward declarations
void bytecode_generate_ast(std::ostream& out, const ogm_ast_t& ast, GenerateContextArgs context_args);

// replaces addresses placeholders in the given vector from the given index onward.
// if k_invalid_pos is passed into the pos value, they will be replaced by the address of their following instruction
void bytecode_replace_placeholder_addresses(std::ostream& out, std::vector<bytecode_address_t>& placeholders, size_t start, bytecode_address_t dst)
{
    while (placeholders.size() > start)
    {
        bytecode_address_t src = placeholders.back();
        placeholders.pop_back();
        bytecode_address_t _dst = dst;
        if (dst == -1)
        {
            _dst = src + sizeof(bytecode_address_t);
        }
        write_at(out, _dst, src);
    }
}

inline void bytecode_generate_pops(std::ostream& out, int16_t pop_count)
{
    for (int16_t i = 0; i < pop_count; i++)
    {
        write_op(out, pop);
    }
}

// if an LValue needs to be used twice, this duplicates the
// stack values needed to dereference it.
inline void bytecode_generate_reuse_lvalue(std::ostream& out, const LValue& lv)
{
    switch (lv.m_pop_count)
    {
        case 0:
            return;
        case 1:
            write_op(out, dup);
            return;
        case 2:
            write_op(out, dup2);
            return;
        case 3:
            write_op(out, dup3);
            return;
        default:
            write_op(out, dupn);
            write(out, lv.m_pop_count);
            return;
    }
}

void bytecode_generate_load(std::ostream& out, const LValue& lv, const GenerateContextArgs& context_args)
{
    switch (lv.m_memspace)
    {
        case memspace_local:
            write_op(out, lv.m_array_access ? ldla : ldl);
            write(out, lv.m_address);
            break;
        case memspace_instance:
            write_op(out, lv.m_array_access ? ldsa : lds);
            write(out, lv.m_address);
            break;
        case memspace_other:
            write_op(out, lv.m_array_access ? ldoa : ldo);
            write(out, lv.m_address);
            break;
        case memspace_global:
            write_op(out, lv.m_array_access ? ldga : ldg);
            write(out, lv.m_address);
            break;
        case memspace_builtin_instance:
            if (lv.m_array_access)
            {
                // there is no ldta (ldpa should be used instead).
                ogm_assert(false);
            }
            else
            {
                write_op(out, ldt);
            }
            write(out, lv.m_address);
            break;
        case memspace_builtin_other:
            write_op(out, lv.m_array_access ? ldpa : ldp);
            write(out, lv.m_address);
            break;
        case memspace_builtin_global:
            context_args.m_library->generate_variable_bytecode(out, lv.m_address, lv.m_pop_count, false);
            break;
        case memspace_constant:
            // constant value is already on stack from bytecode_generate_get_lvalue.
            break;
        case memspace_accessor:
            if (!context_args.m_library->generate_accessor_bytecode(out, lv.m_accessor_type, lv.m_pop_count, false))
            {
                throw MiscError("Could not compile accessor.");
            }
            break;
    }
}

void bytecode_generate_store(std::ostream& out, const LValue& lv, const GenerateContextArgs& context_args)
{
    if (lv.m_read_only)
    {
        throw MiscError("Cannot write to read-only variable");
    }
    if (lv.m_no_copy)
    {
        write_op(out, sfx);
    }
    switch (lv.m_memspace)
    {
        case memspace_local:
            write_op(out, lv.m_array_access ? stla : stl);
            write(out, lv.m_address);
            break;
        case memspace_instance:
            write_op(out, lv.m_array_access ? stsa : sts);
            write(out, lv.m_address);
            break;
        case memspace_other:
            write_op(out, lv.m_array_access ? stoa : sto);
            write(out, lv.m_address);
            break;
        case memspace_global:
            write_op(out, lv.m_array_access ? stga : stg);
            write(out, lv.m_address);
            break;
        case memspace_builtin_instance:
            if (lv.m_array_access)
            {
                // there is no ldta (ldpa should be used instead).
                ogm_assert(false);
            }
            else
            {
                write_op(out, stt);
            }
            write(out, lv.m_address);
            break;
        case memspace_builtin_other:
            write_op(out, lv.m_array_access ? stpa : stp);
            write(out, lv.m_address);
            break;
        case memspace_builtin_global:
            context_args.m_library->generate_variable_bytecode(out, lv.m_address, lv.m_pop_count, true);
            break;
        case memspace_constant:
            throw MiscError("Cannot assign to constant literal value.");
            break;
        case memspace_accessor:
            if (!context_args.m_library->generate_accessor_bytecode(out, lv.m_accessor_type, lv.m_pop_count, true))
            {
                throw MiscError("Could not compile accessor.");
            }
            break;
    }
    if (lv.m_no_copy)
    {
        write_op(out, ufx);
    }
}

// gets the lvalue (and loading bytecode) for the given expression.
// template arguments are intended for internal recursion only.
// owned: whether this is an owned property, e.g. the part after the `.` in `other.var`.
// macro: whether this is an extended macro value.
template<bool owned=false, bool macro=false>
LValue bytecode_generate_get_lvalue(std::ostream& out, const ogm_ast_t& ast, GenerateContextArgs context_args)
{
    if (ast.m_type == ogm_ast_t_imp)
    {
        throw MiscError("Cannot get LValue of imperative statement.");
    }
    // identify address of lvalue
    variable_id_t address_lhs;
    memspace_t memspace_lhs;
    size_t pop_count = 0;
    bool array_access = false;
    accessor_type_t accessor_type = accessor_none;
    bool read_only = false;
    bool no_copy = false;
    {
        switch (ast.m_subtype)
        {
            case ogm_ast_st_exp_identifier:
                {
                    char* var_name = (char*) ast.m_payload;
                    BuiltInVariableDefinition def;
                    asset_index_t asset_index;
                    const auto& macros = context_args.m_reflection->m_ast_macros;

                    // constants (canot appear as other.constant)
                    if (!owned && context_args.m_library->generate_constant_bytecode(out, var_name))
                    {
                        read_only = true;
                        memspace_lhs = memspace_constant;
                        pop_count = 1;
                    }
                    // built-in variables
                    else if (context_args.m_library->variable_definition(var_name, def))
                    {
                        memspace_lhs = (def.m_global) ? memspace_builtin_global : memspace_builtin_instance;
                        address_lhs = def.m_address;
                        read_only = def.m_read_only;
                    }
                    // assets (cannot appear as other.asset)
                    else if (!owned && context_args.m_asset_table->get_asset(var_name, asset_index))
                    {
                        read_only = true;
                        write_op(out, ldi_s32);
                        write(out, asset_index);
                        memspace_lhs = memspace_constant;
                        pop_count = 1;
                    }
                    // macros (cannot appear as `other.macro`)
                    else if (!owned && context_args.m_reflection->has_macro_NOMUTEX(var_name))
                    {
                        return bytecode_generate_get_lvalue<owned, true>(out, *macros.at(var_name), context_args);
                    }
                    // locals (cannot appear as `other.local`)
                    else if (!owned && context_args.m_symbols->m_namespace_local.has_id(var_name))
                    {
                        // check locals for variable name
                        address_lhs = context_args.m_symbols->m_namespace_local.get_id(var_name);
                        memspace_lhs = memspace_local;
                    }
                    // bare globals (cannot appear as other.global)
                    else if (!owned && context_args.m_reflection->has_bare_global(var_name))
                    {
                        ogm_assert(context_args.m_globals.has_id(var_name));
                        address_lhs = context_args.m_globals.get_id(var_name);
                        memspace_lhs = memspace_global;
                    }
                    else
                    {
                        // check instance variables, creating a new one if necessary.
                        address_lhs = context_args.m_instance_variables.get_id(var_name);
                        memspace_lhs = memspace_instance;
                    }
                }
                break;
            case ogm_ast_st_exp_global:
                {
                    // evaluate as as lvalue.
                    ogm_assert(ast.m_sub_count == 1);
                    ogm_assert(ast.m_sub[0].m_subtype == ogm_ast_st_exp_identifier);
                    char* var_name = (char*) ast.m_sub[0].m_payload;

                    address_lhs = context_args.m_globals.get_id(var_name);
                    memspace_lhs = memspace_global;
                }
                break;
            case ogm_ast_st_exp_possessive:
                {
                    ogm_assert(ast.m_sub_count == 2);

                    // evaluate owner as rvalue expression
                    bytecode_generate_ast(out, ast.m_sub[0], context_args);
                    pop_count = 1;

                    // evaluate property as lvalue.
                    ogm_assert(ast.m_sub[1].m_subtype == ogm_ast_st_exp_identifier);
                    LValue lv_prop = bytecode_generate_get_lvalue<true>(out, ast.m_sub[1], context_args);

                    // convert memory space to indirect access.
                    ogm_assert(!lv_prop.m_array_access);
                    ogm_assert(lv_prop.m_memspace == memspace_instance || lv_prop.m_memspace == memspace_builtin_instance);

                    switch(lv_prop.m_memspace)
                    {
                        case memspace_instance:
                            memspace_lhs = memspace_other;
                            break;
                        case memspace_builtin_instance:
                            memspace_lhs = memspace_builtin_other;
                            break;
                    }
                    address_lhs = lv_prop.m_address;
                    read_only = lv_prop.m_read_only;
                }
                break;
            case ogm_ast_st_exp_accessor:
                {
                    switch (ast.m_spec)
                    {
                        case ogm_ast_spec_acc_array:
                            no_copy = true;
                            // falthrough
                        case ogm_ast_spec_acc_none:
                        // array access
                        {
                            pop_count = 2;
                            array_access = true;
                            if (ast.m_spec == ogm_ast_spec_acc_array)
                            {
                                no_copy = true;
                            }

                            // indices are rvalues
                            if (ast.m_sub_count == 2)
                            //1d-array
                            {
                                // row is 0
                                int32_t row = 0;
                                write_op(out, ldi_s32);
                                write(out, row);

                                // column is specified
                                bytecode_generate_ast(out, ast.m_sub[1], context_args);
                            }
                            else
                            {
                                if (ast.m_sub_count != 3)
                                {
                                    throw MiscError("Arrays must have 1 or 2 indices.");
                                }
                                bytecode_generate_ast(out, ast.m_sub[1], context_args);
                                bytecode_generate_ast(out, ast.m_sub[2], context_args);
                            }

                            // because the array variable itself is an lvalue, we recurse to find out what it is
                            // before decorating it.
                            LValue lv_prop = bytecode_generate_get_lvalue<owned>(out, ast.m_sub[0], context_args);

                            // decorate the lvalue to make it an array access.
                            if (lv_prop.m_array_access)
                            {
                                throw MiscError("Cannot nest arrays.");
                            }
                            memspace_lhs = lv_prop.m_memspace;
                            address_lhs = lv_prop.m_address;
                            read_only = lv_prop.m_read_only;
                            pop_count += lv_prop.m_pop_count;

                            // because ldta and stta are not valid bytecode, we must convert them to ldpa/stpa
                            if (memspace_lhs == memspace_builtin_instance)
                            {
                                memspace_lhs = memspace_builtin_other;
                                int32_t self = -1;
                                write_op(out, ldi_s32);
                                write(out, self);
                                pop_count++;
                            }
                        }
                        break;
                    default:
                        // data structure accessor
                        {
                            // evaluate data structure id and put it on the stack
                            LValue lv_ds = bytecode_generate_get_lvalue<owned>(out, ast.m_sub[0], context_args);
                            bytecode_generate_load(out, lv_ds, context_args);

                            for (size_t i = 1; i < ast.m_sub_count; i++)
                            {
                                bytecode_generate_ast(out, ast.m_sub[i], context_args);
                            }
                            pop_count = ast.m_sub_count;
                            accessor_type = get_accessor_type_from_spec(ast.m_spec);
                            memspace_lhs = memspace_accessor;
                        }
                    }
                }
                break;
            default:
                if (!macro)
                {
                    throw MiscError("Cannot retrieve LValue from specified ast node of type");
                }
                else if (owned)
                {
                    throw MiscError("Macro cannot follow possessor (e.g. other.macro).");
                }
                else
                {
                    bytecode_generate_ast(out, ast, context_args);
                    read_only = true;
                    memspace_lhs = memspace_constant;
                    pop_count = 1;
                }
                break;
        }
    }

    return {memspace_lhs, address_lhs, pop_count, array_access, accessor_type, no_copy, read_only};
}

void preprocess_function_special(const ogm_ast_t& ast)
{
    const char* function_name = (char*) ast.m_payload;
    uint8_t argc = ast.m_sub_count;
}

// handles  special functions like pragma
bool generate_function_special(std::ostream& out, const ogm_ast_t& ast)
{
    const char* function_name = (char*) ast.m_payload;
    uint8_t argc = ast.m_sub_count;

    if (strcmp(function_name, "gml_pragma") == 0)
    {
        // TODO:
        return true;
    }

    if (strcmp(function_name, "ogm_pragma") == 0)
    {
        // TODO:
        return true;
    }

    return false;
}

void bytecode_generate_ast(std::ostream& out, const ogm_ast_t& ast, GenerateContextArgs context_args)
{
    auto start_location = out.tellp();

    switch (ast.m_subtype)
    {
        case ogm_ast_st_exp_literal_primitive:
        {
            ogm_ast_literal_primitive_t* payload;
            ogm_ast_tree_get_payload_literal_primitive(&ast, &payload);
            switch (payload->m_type)
            {
                case ogm_ast_literal_t_number:
                    {
                        char* s = payload->m_value;
                        if (s[0] == '$')
                        {
                            // hex string
                            uint64_t v = 0;
                            for (size_t i = 1; s[i] != 0; i++)
                            {
                                v <<= 4;
                                if (s[i] >= '0' && s[i] <= '9')
                                {
                                    v += s[i] - '0';
                                }
                                else
                                {
                                    v += 10 + s[i] - 'a';
                                }
                            }
                            if (v >= 1 << 31)
                            {
                                write_op(out, ldi_u64);
                                write(out, v);
                            }
                            else
                            {
                                write_op(out, ldi_f64);
                                real_t vs = static_cast<real_t>(v);
                                write(out, vs);
                            }
                        }
                        else
                        {
                            // decimal
                            if (char* p = strchr(s, '.'))
                            {
                                double v = 0;
                                size_t mult = 1;
                                for (char* c = p - 1; c >= s; c--)
                                {
                                    v += mult * (*c - '0');
                                    mult *= 10;
                                }
                                double multf = 1;
                                for (size_t i = p - s + 1; s[i] != 0; i++)
                                {
                                    multf *= 0.1;
                                    v += multf * (s[i] - '0');
                                }
                                write_op(out, ldi_f64);
                                write(out, v);
                            }
                            else
                            {
                                // int
                                uint64_t v = 0;
                                for (size_t i = 0; s[i] != 0; i++)
                                {
                                    v *= 10;
                                    v += s[i] - '0';
                                }

                                if (v >= 1 << 31)
                                {
                                    write_op(out, ldi_u64);
                                    write(out, v);
                                }
                                else
                                {
                                    write_op(out, ldi_f64);
                                    real_t vs = static_cast<real_t>(v);
                                    write(out, vs);
                                }
                            }
                        }
                    }
                    break;
                case ogm_ast_literal_t_string:
                    {
                        write_op(out, ldi_string);
                        for (size_t i = 1; payload->m_value[i + 1] != 0; i++)
                        {
                            out << payload->m_value[i];
                        }
                        out << (char) 0;
                    }
                    break;
            }
        }
        break;
        case ogm_ast_st_exp_literal_array:
        {
            write_op(out, ldi_arr);
            // write back-to-front, because that reserves the array.
            for (int32_t i = ast.m_sub_count - 1; i >= 0; --i)
            {
                write_op(out, ldi_false); // punning false and 0.
                write_op(out, ldi_s32);
                write(out, i);
                bytecode_generate_ast(out, ast.m_sub[i], context_args);
                write_op(out, seti);
            }
        }
        break;
        case ogm_ast_st_exp_ternary_if:
        {
            // condition
            bytecode_generate_ast(out, ast.m_sub[0], context_args);
            write_op(out, ncond);
            write_op(out, bcond);

            //backpatch address later
            bytecode_address_t neg_cond_src = out.tellp();
            bytecode_address_t neg_cond_dst;
            bytecode_address_t body_end_src;
            bytecode_address_t body_end_dst;

            // write a placeholder address, to be filled in later.
            write(out, neg_cond_src);
            bytecode_generate_ast(out, ast.m_sub[1], context_args);
            write_op(out, jmp);
            body_end_src = out.tellp();
            write(out, body_end_src);
            neg_cond_dst = out.tellp();
            write_at(out, neg_cond_dst, neg_cond_src);
            bytecode_generate_ast(out, ast.m_sub[2], context_args);
            body_end_dst = out.tellp();
            write_at(out, body_end_dst, body_end_src);
        }
        break;
        case ogm_ast_st_exp_possessive:
        {
            // check if this is an enum
            if (ast.m_sub[0].m_subtype == ogm_ast_st_exp_identifier)
            {
                READ_LOCK(context_args.m_reflection->m_mutex_enums);
                const auto& enum_map = context_args.m_reflection->m_enums->m_map;
                std::string enum_name = static_cast<const char*>(ast.m_sub[0].m_payload);
                auto enum_iter = enum_map.find(enum_name);
                if (enum_iter != enum_map.end())
                {
                    std::string q = static_cast<const char*>(ast.m_sub[1].m_payload);

                    const std::map<std::string, ogm_ast_t*>& map = std::get<1>(*enum_iter).m_map;
                    const auto& value_iter = map.find(q);
                    if (value_iter == map.end())
                    {
                        throw MiscError("Enum element \"" + q + "\" is not a member of enum" + enum_name);
                    }
                    const ogm_ast_t* data = value_iter->second;
                    context_args.m_enum_expression = true;
                    bytecode_generate_ast(out, *data, context_args);
                    break;
                }
            }
        }
        // fallthrough
        case ogm_ast_st_exp_identifier:
        case ogm_ast_st_exp_accessor:
        case ogm_ast_st_exp_global:
        {
            LValue lv = bytecode_generate_get_lvalue(out, ast, context_args);
            bytecode_generate_load(out, lv, context_args);
        }
        break;
        case ogm_ast_st_imp_assignment:
        {
            const ogm_ast_t& lhs = ast.m_sub[0];
            const ogm_ast_t& rhs = ast.m_sub[1];

            LValue lv = bytecode_generate_get_lvalue(out, lhs, context_args);

            // for relative assignments (+= etc.), need to get value of lhs.
            if (ast.m_spec != ogm_ast_spec_op_eq)
            {
                bytecode_generate_reuse_lvalue(out, lv);
                bytecode_generate_load(out, lv, context_args);
            }

            // generate rhs
            bytecode_generate_ast(out, rhs, context_args);

            // relative assignment must retrieve lhs:
            switch (ast.m_spec)
            {
                case ogm_ast_spec_op_pluseq:
                    write_op(out, add2);
                    break;
                case ogm_ast_spec_op_minuseq:
                    write_op(out, sub2);
                    break;
                case ogm_ast_spec_op_timeseq:
                    write_op(out, mult2);
                    break;
                case ogm_ast_spec_op_divideeq:
                    write_op(out, fdiv2);
                    break;
                case ogm_ast_spec_op_andeq:
                    write_op(out, band);
                    break;
                case ogm_ast_spec_op_oreq:
                    write_op(out, bor);
                    break;
                case ogm_ast_spec_op_xoreq:
                    write_op(out, bxor);
                    break;
                case ogm_ast_spec_op_leftshifteq:
                    write_op(out, lsh2);
                    break;
                case ogm_ast_spec_op_rightshifteq:
                    write_op(out, rsh2);
                    break;
                case ogm_ast_spec_op_modeq:
                    write_op(out, mod2);
                    break;
                case ogm_ast_spec_op_eq:
                    break;
                default:
                    throw LanguageFeatureNotImplementedError("Assignment operator not supported.");
            }

            // assign.
            bytecode_generate_store(out, lv, context_args);
        }
        break;
        case ogm_ast_st_exp_arithmetic:
            {
                bool ignore_result = ast.m_type == ogm_ast_t_imp;
                switch (ast.m_spec)
                {
                case ogm_ast_spec_op_unary_pre_plusplus:
                case ogm_ast_spec_op_unary_post_plusplus:
                case ogm_ast_spec_op_unary_pre_minusminus:
                case ogm_ast_spec_op_unary_post_minusminus:
                    {
                        LValue lv = bytecode_generate_get_lvalue(out, ast.m_sub[0], context_args);
                        bool pre = ast.m_spec == ogm_ast_spec_op_unary_pre_plusplus || ast.m_spec == ogm_ast_spec_op_unary_pre_minusminus;
                        bool plus = ast.m_spec == ogm_ast_spec_op_unary_pre_plusplus || ast.m_spec == ogm_ast_spec_op_unary_post_plusplus;

                        if (lv.m_pop_count == 0)
                        // optimization for memory access opcodes that don't pop references from the stack
                        {
                            if (!pre && !ignore_result)
                            {
                                bytecode_generate_load(out, lv, context_args);
                            }

                            // increment
                            if (lv.m_memspace == memspace_local)
                            {
                                // optimization for local variables
                                write_op(out, plus ? incl : decl)
                                write(out, lv.m_address);
                            }
                            else
                            {
                                bytecode_generate_load(out, lv, context_args);
                                write_op(out, plus ? inc : dec)
                                bytecode_generate_store(out, lv, context_args);
                            }

                            if (pre && !ignore_result)
                            {
                                bytecode_generate_load(out, lv, context_args);
                            }
                        }
                        else
                        {
                            if (!ignore_result)
                            {
                                bytecode_generate_reuse_lvalue(out, lv);
                            }
                            bytecode_generate_reuse_lvalue(out, lv);
                            bytecode_generate_load(out, lv, context_args);
                            write_op(out, plus ? inc : dec)
                            bytecode_generate_store(out, lv, context_args);
                            if (!ignore_result)
                            {
                                bytecode_generate_load(out, lv, context_args);
                                if (!pre)
                                {
                                    // undo the operation
                                    write_op(out, plus ? dec : inc)
                                }
                            }
                        }

                        ignore_result = false;
                    }
                    break;
                #ifdef SHORTCIRCUIT_EVALUATIONS
                case ogm_ast_spec_op_boolean_and:
                case ogm_ast_spec_op_boolean_and_kw:
                    // short-circuit `and`
                    {
                        // OPTIMIZE: massively optimize shortcircuit evaluations.

                        // left-hand side
                        bytecode_generate_ast(out, ast.m_sub[0], context_args);

                        write_op(out, ncond);
                        write_op(out, bcond);

                        // backpatch later
                        bytecode_address_t shortcircuit_src = out.tellp();
                        write(out, shortcircuit_src);

                        // right-hand side
                        bytecode_generate_ast(out, ast.m_sub[1], context_args);
                        write_op(out, ncond);

                        // backpatch
                        bytecode_address_t loc = out.tellp();
                        write_at(out, loc, shortcircuit_src)

                        // write flipped condition
                        // FIXME: this is hideous.
                        write_op(out, pcond);
                        write_op(out, ncond);
                        write_op(out, pcond);
                    }
                    break;
                case ogm_ast_spec_op_boolean_or:
                case ogm_ast_spec_op_boolean_or_kw:
                    // short-circuit `and`
                    {
                        // OPTIMIZE: massively optimize shortcircuit evaluations.

                        // left-hand side
                        bytecode_generate_ast(out, ast.m_sub[0], context_args);

                        write_op(out, cond);
                        write_op(out, bcond);

                        // backpatch later
                        bytecode_address_t shortcircuit_src = out.tellp();
                        write(out, shortcircuit_src);

                        // right-hand side
                        bytecode_generate_ast(out, ast.m_sub[1], context_args);
                        write_op(out, cond);

                        // backpatch
                        bytecode_address_t loc = out.tellp();
                        write_at(out, loc, shortcircuit_src)

                        // write condition
                        write_op(out, pcond);
                    }
                    break;
                #endif
                default:
                    bytecode_generate_ast(out, ast.m_sub[0], context_args);
                    if (ast.m_sub_count == 1)
                    // unary operators
                    {
                        switch (ast.m_spec)
                        {
                            case ogm_ast_spec_op_plus:
                                // unary plus is meaningless.
                                break;
                            case ogm_ast_spec_op_minus:
                                // multiply by -1. (OPTIMIZE?)
                                {
                                    real_t neg = -1;
                                    write_op(out, ldi_f64);
                                    write(out, neg);
                                    write_op(out, mult2);
                                }
                                break;
                            case ogm_ast_spec_opun_boolean_not:
                            case ogm_ast_spec_opun_boolean_not_kw:
                                write_op(out, ncond);
                                write_op(out, pcond);
                                break;
                            case ogm_ast_spec_opun_bitwise_not:
                                write_op(out, bnot);
                                break;
                            default:
                                throw LanguageFeatureNotImplementedError("Unknown unary operator.");
                        }
                    }
                    else
                    // binary operators
                    {
                        // compute right-hand side
                        bytecode_generate_ast(out, ast.m_sub[1], context_args);

                        switch (ast.m_spec)
                        {
                            case ogm_ast_spec_op_plus:
                                write_op(out, add2);
                                break;
                            case ogm_ast_spec_op_minus:
                                write_op(out, sub2);
                                break;
                            case ogm_ast_spec_op_times:
                                write_op(out, mult2);
                                break;
                            case ogm_ast_spec_op_integer_division_kw:
                                write_op(out, idiv2);
                                break;
                            case ogm_ast_spec_op_divide:
                                write_op(out, fdiv2);
                                break;
                            case ogm_ast_spec_op_mod:
                            case ogm_ast_spec_op_mod_kw:
                                write_op(out, mod2);
                                break;
                            #ifndef SHORTCIRCUIT_EVALUATIONS
                            case ogm_ast_spec_op_boolean_and:
                            case ogm_ast_spec_op_boolean_and_kw:
                                write_op(out, bland);
                                break;
                            case ogm_ast_spec_op_boolean_or:
                            case ogm_ast_spec_op_boolean_or_kw:
                                write_op(out, blor);
                                break;
                            #endif
                            case ogm_ast_spec_op_boolean_xor:
                            case ogm_ast_spec_op_boolean_xor_kw:
                                write_op(out, blxor);
                                break;
                            case ogm_ast_spec_op_bitwise_and:
                                write_op(out, band);
                                break;
                            case ogm_ast_spec_op_bitwise_or:
                                write_op(out, bor);
                                break;
                            case ogm_ast_spec_op_bitwise_xor:
                                write_op(out, bxor);
                                break;
                            case ogm_ast_spec_op_leftshift:
                                write_op(out, lsh2);
                                break;
                            case ogm_ast_spec_op_rightshift:
                                write_op(out, rsh2);
                                break;
                            case ogm_ast_spec_op_eq:
                            case ogm_ast_spec_op_eqeq:
                                write_op(out, eq);
                                write_op(out, pcond);
                                break;
                            case ogm_ast_spec_op_lt:
                                write_op(out, lt);
                                write_op(out, pcond);
                                break;
                            case ogm_ast_spec_op_gt:
                                write_op(out, gt);
                                write_op(out, pcond);
                                break;
                            case ogm_ast_spec_op_lte:
                                write_op(out, lte);
                                write_op(out, pcond);
                                break;
                            case ogm_ast_spec_op_gte:
                                write_op(out, gte);
                                write_op(out, pcond);
                                break;
                            case ogm_ast_spec_op_neq:
                            case ogm_ast_spec_op_ltgt:
                                write_op(out, neq);
                                write_op(out, pcond);
                                break;
                            default:
                                throw LanguageFeatureNotImplementedError("Unknown binary operator.");
                        }
                    }
                }

                // statements ignore result.
                if (ignore_result)
                {
                    write_op(out, pop);
                }
            }
            break;
        case ogm_ast_st_exp_paren:
            bytecode_generate_ast(out, ast.m_sub[0], context_args);
            break;
        case ogm_ast_st_exp_fn:
            {
                const char* function_name = (char*) ast.m_payload;
                uint8_t retc = 0;
                uint8_t argc = ast.m_sub_count;

                if (generate_function_special(out, ast))
                {
                    break;
                }

                {
                    // generate arg bytecode in forward order
                    for (size_t i = 0; i < argc; i++)
                    {
                        bytecode_generate_ast(out, ast.m_sub[i], context_args);
                    }
                }

                if (context_args.m_library->generate_function_bytecode(out, function_name, argc))
                // library function
                {
                    retc = 1;
                }
                else
                // function name not found in library
                {
                    // TODO: check extensions

                    // check scripts
                    asset_index_t asset_index;
                    if (const AssetScript* script = dynamic_cast<const AssetScript*>(context_args.m_asset_table->get_asset(function_name, asset_index)))
                    {
                        const Bytecode bytecode = context_args.m_bytecode_table->get_bytecode(script->m_bytecode_index);

                        write_op(out, call);
                        write(out, script->m_bytecode_index);
                        retc = bytecode.m_retc;
                        if (bytecode.m_argc != static_cast<uint8_t>(-1) && argc != bytecode.m_argc)
                        {
                            throw MiscError(std::string("Wrong number of arguments to script '") + function_name + "'; expected: " + std::to_string(bytecode.m_argc) + ", provided: " + std::to_string(argc));
                        }
                        write(out, argc);
                    }
                    else
                    {
                        throw UnknownIdentifierError(function_name);
                    }
                }

                if (ast.m_type == ogm_ast_t_imp)
                // ignore output value
                {
                    for (size_t i = 0; i < retc; ++i)
                    {
                        write_op(out, pop);
                    }
                }
            }
            break;
        case ogm_ast_st_imp_var:
            {
                ogm_ast_declaration_t* payload;
                ogm_ast_tree_get_payload_declaration(&ast, &payload);

                LValue type_lv;
                type_lv.m_memspace = memspace_local;


                const char* dectype = "";

                // add variable names to local namespace and assign values if needed.
                for (size_t i = 0; i < payload->m_identifier_count; i++)
                {
                    if (payload->m_types[i])
                    {
                        dectype = payload->m_types[i];
                    }
                    if (strcmp(dectype, "globalvar") == 0)
                    {
                        type_lv.m_memspace = memspace_global;
                    }
                    else
                    {
                        type_lv.m_memspace = memspace_local;
                    }
                    const char* identifier = payload->m_identifier[i];
                    LValue lv = type_lv;
                    if (lv.m_memspace == memspace_local)
                    {
                        // local variable
                        lv.m_address = context_args.m_symbols->m_namespace_local.add_id(identifier);
                    }
                    else if (lv.m_memspace == memspace_global)
                    {
                        // globalvar
                        lv.m_address = context_args.m_reflection->m_namespace_global.add_id(identifier);

                        // mark as a "bare" global, meaning it can be referenced without the `global.` prefix.
                        WRITE_LOCK(context_args.m_reflection->m_mutex_bare_globals);
                        context_args.m_reflection->m_bare_globals.insert(identifier);
                    }

                    if (ast.m_sub[i].m_subtype != ogm_ast_st_imp_empty)
                    // some declarations have definitions, too.
                    {
                        // calculate assignment and give value.
                        bytecode_generate_ast(out, ast.m_sub[i], context_args);
                        bytecode_generate_store(out, lv, context_args);
                    }
                }
            }
            break;
        case ogm_ast_st_imp_enum:
            // enums only matter during preprocessing.
            break;
        case ogm_ast_st_imp_empty:
            break;
        case ogm_ast_st_imp_body:
            for (size_t i = 0; i < ast.m_sub_count; i++)
            {
                bytecode_generate_ast(out, ast.m_sub[i], context_args);
            }
            break;
        case ogm_ast_st_imp_if:
            {
                bool has_else = ast.m_sub_count == 3;
                // compute condition:
                bytecode_generate_ast(out, ast.m_sub[0], context_args);

                write_op(out, ncond);
                write_op(out, bcond);
                //backpatch address later
                bytecode_address_t neg_cond_src = out.tellp();
                bytecode_address_t neg_cond_dst;
                bytecode_address_t body_end_src;

                // write a placeholder address, to be filled in later.
                write(out, neg_cond_src);

                // write body
                bytecode_generate_ast(out, ast.m_sub[1], context_args);

                if (has_else)
                {
                    // jump to after else block
                    write_op(out, jmp);
                    body_end_src = out.tellp();

                    //placeholder
                    write(out, neg_cond_src);
                }

                // fill in placeholder
                neg_cond_dst = out.tellp();
                out.seekp(neg_cond_src);
                write(out, neg_cond_dst);
                out.seekp(neg_cond_dst);

                // else block
                if (has_else)
                {
                    bytecode_generate_ast(out, ast.m_sub[2], context_args);

                    bytecode_address_t body_end_dst = out.tellp();
                    out.seekp(body_end_src);
                    write(out, body_end_dst);
                    out.seekp(body_end_dst);
                }
            }
            break;
            case ogm_ast_st_imp_for:
                {
                    context_args.m_continue_pop_count = 0;
                    const ogm_ast_t& init = ast.m_sub[0];
                    const ogm_ast_t& condition = ast.m_sub[1];
                    const ogm_ast_t& post = ast.m_sub[2];
                    const ogm_ast_t& body = ast.m_sub[3];
                    bytecode_generate_ast(out, init, context_args);

                    bytecode_address_t loop_start = out.tellp();
                    bytecode_generate_ast(out, condition, context_args);
                    bytecode_address_t loop_end_dst;
                    write_op(out, ncond);
                    write_op(out, bcond);
                    // placeholder, to be filled in later.
                    bytecode_address_t loop_end_src = out.tellp();
                    write(out, k_placeholder_pos);

                    // remember where the continue placeholder list was to start.
                    size_t start_continue_replace = context_args.m_continue_placeholders.size();
                    size_t start_break_replace = context_args.m_break_placeholders.size();
                    bytecode_generate_ast(out, body, context_args);
                    bytecode_address_t continue_address = out.tellp();
                    bytecode_generate_ast(out, post, context_args);

                    // jump to start of loop
                    write_op(out, jmp);
                    write(out, loop_start);

                    // replace break/continue addresses within loop body.
                    bytecode_address_t break_address = out.tellp();
                    bytecode_replace_placeholder_addresses(out, context_args.m_continue_placeholders, start_continue_replace, continue_address);
                    bytecode_replace_placeholder_addresses(out, context_args.m_break_placeholders, start_break_replace, break_address);

                    // end of loop
                    loop_end_dst = out.tellp();
                    write_at(out, loop_end_dst, loop_end_src);
            }
            break;
        case ogm_ast_st_imp_loop:
            {
                context_args.m_continue_pop_count = 0;
                const ogm_ast_t& condition = ast.m_sub[0];
                const ogm_ast_t& body = ast.m_sub[1];
                size_t start_continue_replace = context_args.m_continue_placeholders.size();
                size_t start_break_replace = context_args.m_break_placeholders.size();
                bytecode_address_t continue_address;
                bytecode_address_t break_address;
                switch (ast.m_spec)
                {
                    case ogm_ast_spec_loop_repeat:
                        {
                            // store repeat counter on stack anonymously.
                            bytecode_generate_ast(out, condition, context_args);
                            bytecode_address_t loop_start = out.tellp();
                            // check repeat counter
                            write_op(out, dup);
                            write_op(out, ncond);
                            write_op(out, bcond);

                            context_args.m_cleanup_commands.push_back(pop);
                            // placeholder, to be filled in later.
                            bytecode_address_t loop_end_src = out.tellp();
                            write(out, k_placeholder_pos);
                            bytecode_generate_ast(out, body, context_args);

                            continue_address = out.tellp();

                            // decrement loop counter, jump to start of loop
                            write_op(out, dec);
                            write_op(out, jmp);
                            write(out, loop_start);

                            // end of loop
                            break_address = out.tellp();
                            write_at(out, break_address, loop_end_src);
                            write_op(out, pop);

                            context_args.m_cleanup_commands.pop_back();
                        }
                        break;
                    case ogm_ast_spec_loop_while:
                        {
                            bytecode_address_t loop_start = out.tellp();
                            continue_address = out.tellp();
                            bytecode_generate_ast(out, condition, context_args);
                            bytecode_address_t loop_end_dst;
                            write_op(out, ncond);
                            write_op(out, bcond);
                            // placeholder, to be filled in later.
                            bytecode_address_t loop_end_src = out.tellp();
                            write(out, k_placeholder_pos);
                            bytecode_generate_ast(out, body, context_args);

                            // jump to start of loop
                            write_op(out, jmp);
                            write(out, loop_start);

                            // end of loop
                            loop_end_dst = out.tellp();
                            break_address = out.tellp();
                            write_at(out, loop_end_dst, loop_end_src);
                        }
                        break;
                    case ogm_ast_spec_loop_do_until:
                        {
                            bytecode_address_t loop_start = out.tellp();
                            bytecode_generate_ast(out, body, context_args);
                            continue_address = out.tellp();
                            bytecode_generate_ast(out, condition, context_args);
                            write_op(out, ncond);
                            write_op(out, bcond);
                            write(out, loop_start);
                            break_address = out.tellp();
                        }
                        break;
                    case ogm_ast_spec_loop_with:
                        {
                            bytecode_generate_ast(out, condition, context_args);
                            write_op(out, wti);
                            bytecode_address_t loop_start = out.tellp();
                            continue_address = out.tellp();
                            write_op(out, wty);
                            write_op(out, bcond);

                            context_args.m_cleanup_commands.push_back(wtd);

                            bytecode_address_t loop_end_src = out.tellp();
                            bytecode_address_t loop_end_dst;
                            write(out, k_placeholder_pos);
                            bytecode_generate_ast(out, body, context_args);

                            // jump to start of loop
                            write_op(out, jmp);
                            write(out, loop_start);

                            // place a wtd instruction below the loop which
                            // any break instruction will jump to.
                            break_address = out.tellp();
                            if (context_args.m_break_placeholders.size() > start_break_replace)
                            {
                                write_op(out, wtd);
                            }

                            context_args.m_cleanup_commands.pop_back();

                            // end of loop
                            loop_end_dst = out.tellp();
                            write_at(out, loop_end_dst, loop_end_src);
                        }
                        break;
                    default:
                        throw LanguageFeatureNotImplementedError("Loop type not supported.");
                }
                bytecode_replace_placeholder_addresses(out, context_args.m_continue_placeholders, start_continue_replace, continue_address);
                bytecode_replace_placeholder_addresses(out, context_args.m_break_placeholders, start_break_replace, break_address);
            }
            break;
        case ogm_ast_st_imp_switch:
            {
                // this could be heavily optimized with e.g. a binary search or indirect jumps.
                bytecode_generate_ast(out, ast.m_sub[0], context_args);

                if (ast.m_sub_count > 0)
                {
                    context_args.m_continue_pop_count++;
                    context_args.m_cleanup_commands.push_back(pop);
                    size_t start_break_replace = context_args.m_break_placeholders.size();
                    size_t default_case_index = ast.m_sub_count;
                    std::vector<bytecode_address_t> label_src(ast.m_sub_count + 1);
                    for (size_t i = 1; i < ast.m_sub_count; i += 2)
                    {
                        size_t case_i = i;

                        if (ast.m_sub[case_i].m_subtype == ogm_ast_st_imp_empty)
                        // default
                        {
                            // handle the default later.
                            if (default_case_index != ast.m_sub_count)
                            {
                                throw MiscError("Expected at most one \"default\" label.");
                            }
                            default_case_index = case_i;
                            continue;
                        }
                        else
                        {
                            // case
                            write_op(out, dup);
                            bytecode_generate_ast(out, ast.m_sub[case_i], context_args);
                            write_op(out, eq);
                            write_op(out, bcond);
                            label_src[i >> 1] = out.tellp();
                            write(out, k_placeholder_pos);
                            // handle body later
                        }
                    }

                    // jump to default
                    write_op(out, jmp);
                    label_src[default_case_index >> 1] = out.tellp();
                    write(out, k_placeholder_pos);

                    // make bodies
                    for (size_t i = 1; i < ast.m_sub_count; i += 2)
                    {
                        size_t body_i = i + 1;
                        bytecode_address_t label_dst = out.tellp();
                        write_at(out, label_dst, label_src[i >> 1]);
                        bytecode_generate_ast(out, ast.m_sub[body_i], context_args);
                    }

                    bytecode_address_t end_dst = out.tellp();
                    bytecode_replace_placeholder_addresses(out, context_args.m_break_placeholders, start_break_replace, end_dst);
                    if (default_case_index == ast.m_sub_count)
                    // no default statement
                    {
                        // default 'default' jump
                        write_at(out, end_dst, label_src[default_case_index >> 1]);
                    }
                    context_args.m_continue_pop_count--;
                    context_args.m_cleanup_commands.pop_back();
                }

                // end of switch
                write_op(out, pop);
            }
            break;
        case ogm_ast_st_imp_control:
            switch(ast.m_spec)
            {
                case ogm_ast_spec_control_exit:
                    // pop temporary values from stack
                    for (opcode::opcode_t op : context_args.m_cleanup_commands)
                    {
                        write_op(out, op);
                    }

                    // undefined return value
                    for (size_t i = 0; i < context_args.m_retc; i++)
                    {
                        write_op(out, ldi_undef);
                    }

                    // bytecode for ret or sus
                    if (context_args.m_config->m_return_is_suspend)
                    {
                        write_op(out, sus);
                    }
                    else
                    {
                        write_op(out, ret);
                        write(out, context_args.m_retc);
                    }
                    break;
                case ogm_ast_spec_control_return:
                    if (ast.m_sub_count == 0)
                    {
                        throw MiscError("return requires return value.");
                    }

                    // pop temporary values from stack
                    // wtd requires special workarounds, as it changes the
                    // context of the return values.
                    {
                        bool has_wtd = false;

                        for (opcode::opcode_t op : context_args.m_cleanup_commands)
                        {
                            if (op == opcode::wtd)
                            {
                                has_wtd = true;
                                break;
                            }
                        }

                        if (!has_wtd)
                        {
                            // as for exit above
                            for (opcode::opcode_t op : context_args.m_cleanup_commands)
                            {
                                write_op(out, op);
                            }

                            // return values
                            for (size_t i = 0; i < ast.m_sub_count; i++)
                            {
                                bytecode_generate_ast(out, ast.m_sub[i], context_args);
                            }
                        }
                        else
                        {
                            // this is an ugly hack to deal with wtd's context-change.
                            // it's also inefficient.
                            // OPTIMIZE: consider better options for dealing with this.

                            // wtd affects context, so we need to calculate RVs
                            // advance of the wtd op.

                            ogm_assert(ast.m_sub_count < 253); // hack requires deli: retc + 2.

                            for (size_t i = 0; i < ast.m_sub_count; i++)
                            {
                                bytecode_generate_ast(out, ast.m_sub[i], context_args);
                            }

                            // cleanup ops
                            for (opcode::opcode_t op : context_args.m_cleanup_commands)
                            {
                                // swap down the one and only return value.
                                if (op == opcode::pop && ast.m_sub_count == 1)
                                {
                                    write_op(out, swap);
                                }
                                else
                                {
                                    size_t pops = 1;
                                    if (op == opcode::wtd)
                                        pops = 2;

                                    // duplicate values to work around the fact that
                                    // we don't have arbitrary swapi opcodes.
                                    for (size_t i = 0; i < pops; ++i)
                                    {
                                        uint8_t c;
                                        write_op(out, dupi);
                                        c = ast.m_sub_count + pops - 1;
                                        write(out, c);
                                        write_op(out, deli);
                                        c++;
                                        write(out, c);
                                    }
                                }

                                // clean up.
                                write_op(out, op);
                            }
                        }
                    }

                    // pad remaining return values
                    for (size_t i = ast.m_sub_count; i < context_args.m_retc; i++)
                    {
                        write_op(out, ldi_undef);
                    }

                    // pop extra return values
                    for (size_t i = context_args.m_retc; i < ast.m_sub_count; ++i)
                    {
                        write_op(out, pop);
                    }

                    // bytecode for ret or sus
                    if (context_args.m_config->m_return_is_suspend)
                    {
                        write_op(out, sus);
                    }
                    else
                    {
                        write_op(out, ret);
                        write(out, context_args.m_retc);
                    }
                    break;
                case ogm_ast_spec_control_break:
                    write_op(out, jmp);
                    context_args.m_break_placeholders.push_back(out.tellp());
                    write(out, k_invalid_pos);
                    break;
                case ogm_ast_spec_control_continue:
                    // pop commands required by switch
                    bytecode_generate_pops(out, context_args.m_continue_pop_count);

                    // jump to placeholder value
                    write_op(out, jmp);
                    context_args.m_continue_placeholders.push_back(out.tellp());
                    write(out, k_invalid_pos);
                    break;
            }
            break;
        default:
            throw MiscError("can't handle: " + std::string(ogm_ast_subtype_string[ast.m_subtype]));
    }

    context_args.m_symbols->m_source_map.add_location(start_location, out.tellp(), ast.m_start, ast.m_end, ast.m_type == ogm_ast_t_imp);
}

void bytecode_generate(Bytecode& out_bytecode, const DecoratedAST& in, const Library* library, ProjectAccumulator* accumulator, GenerateConfig* config)
{
    // args (reassign to variables to fit with legacy code... FIXME clean this up eventually.)
    if (!in.m_ast) throw MiscError("AST required to generate bytecode.");
    const ogm_ast_t& ast = *in.m_ast;
    uint8_t retc = in.m_retc, argc = in.m_argc;
    std::string debugSymbolName = in.m_name;
    std::string debugSymbolSource = in.m_source;
    ReflectionAccumulator* inOutAccumulator;
    const asset::AssetTable* assetTable;
    const BytecodeTable* bytecodeTable;

    GenerateConfig defaultConfig;
    ReflectionAccumulator defaultReflectionAccumulator;
    asset::AssetTable defaultAssetTable;
    BytecodeTable defaultBytecodeTable;
    if (!accumulator)
    {
        inOutAccumulator = &defaultReflectionAccumulator;
        assetTable = &defaultAssetTable;
        bytecodeTable = &defaultBytecodeTable;
    }
    else
    {
        bytecodeTable = accumulator->m_bytecode;
        assetTable = accumulator->m_assets;
        inOutAccumulator = accumulator->m_reflection;
    }

    if (!config)
    {
        config = &defaultConfig;
    }

    bytecode::DebugSymbols* debugSymbols = new bytecode::DebugSymbols(debugSymbolName, debugSymbolSource);

    if (config->m_existing_locals_namespace)
    // pre-populate locals
    {
        debugSymbols->m_namespace_local = *config->m_existing_locals_namespace;
    }

    std::vector<bytecode_address_t> ph_break;
    std::vector<bytecode_address_t> ph_continue;
    std::vector<opcode::opcode_t> cleanup_commands;

    GenerateContextArgs context_args(
        retc, argc,
        inOutAccumulator->m_namespace_instance,
        inOutAccumulator->m_namespace_global,
        library, assetTable, bytecodeTable,
        ph_break, ph_continue, cleanup_commands,
        debugSymbols, inOutAccumulator,
        config
    );

    std::stringstream out;

    // cast off outer body_container ast.
    const ogm_ast_t* generate_from_ast = &ast;
    if (ast.m_subtype == ogm_ast_st_imp_body_list)
    {
        if (ast.m_sub_count == 0)
        {
            generate_from_ast = nullptr;
        }
        else if (ast.m_sub_count == 1)
        {
            generate_from_ast = &ast.m_sub[0];
        }
        else
        {
            throw MiscError("More than one body_container provided in ast. This should be handled with multiple calls to bytecode_generate.");
        }
    }

    // allocate local variables
    // TODO: don't allocate locals if no locals in function.
    write_op(out, all);
    int32_t n_locals = 0;
    auto n_locals_src = out.tellp();

    //placeholder -- we won't know the number of locals until after compiling.
    write(out, n_locals);

    if (generate_from_ast)
    {
        bytecode_generate_ast(out, *generate_from_ast, context_args);
    }

    // fix any top-level break/continue statements.
    bytecode_replace_placeholder_addresses(out, context_args.m_continue_placeholders, 0, k_invalid_pos);
    bytecode_replace_placeholder_addresses(out, context_args.m_break_placeholders,    0, k_invalid_pos);

    // set number of locals at start of bytecode
    n_locals = context_args.m_symbols->m_namespace_local.id_count();
    write_at(out, n_locals, n_locals_src);

    if (n_locals > 0 && config->m_no_locals)
    {
        throw MiscError("Locals required not to be used, but locals were used anyway.");
    }

    // default return, if no other return was hit.
    for (size_t i = 0; i < retc; i++)
    {
        write_op(out, ldi_undef);
    }
    if (config->m_return_is_suspend)
    {
        write_op(out, sus);
    }
    else
    {
        write_op(out, ret);
        write(out, retc);
    }
    write_op(out, eof);
    std::string s = out.str();

    out_bytecode = Bytecode(s.c_str(), out.tellp(), retc, argc, debugSymbols);
}

namespace
{
    void bytecode_preprocess_helper(const ogm_ast_t* ast, const ogm_ast_t* parent, uint8_t& out_retc, uint8_t& out_argc, ReflectionAccumulator& in_out_reflection_accumulator)
    {
        // argument style (argument[0] vs argument0)
        if (ast->m_subtype == ogm_ast_st_exp_identifier)
        // check if identifier is an argument
        {
            const char* charv = static_cast<char*>(ast->m_payload);
            const size_t argstrl = strlen("argument");
            if (strlen(charv) > argstrl && memcmp(charv, "argument", argstrl) == 0)
            {
                // remaining characters are all digits
                if (is_digits(charv + argstrl))
                {
                    // get digit value
                    const size_t argn = std::atoi(charv + argstrl);
                    if (argn < 16)
                    // only argument0...argument15 are keywords
                    {
                        if (out_argc == static_cast<uint8_t>(-1) || out_argc < argn + 1)
                        {
                            out_argc = argn + 1;
                        }
                    }
                }
            }
        }

        // special functions
        if (ast->m_subtype == ogm_ast_st_exp_fn)
        {
            preprocess_function_special(*ast);
        }

        // retc
        if (ast->m_subtype == ogm_ast_st_imp_control && ast->m_spec == ogm_ast_spec_control_return)
        {
            out_retc = std::max(out_retc, (uint8_t)ast->m_sub_count);
        }

        // globalvar
        if (ast->m_subtype == ogm_ast_st_imp_var)
        {
            ogm_ast_declaration_t* declaration;
            ogm_ast_tree_get_payload_declaration(ast, &declaration);
            const char* dectype = "";
            for (size_t i = 0; i < declaration->m_identifier_count; ++i)
            {
                if (declaration->m_types[i])
                {
                    dectype = declaration->m_types[i];
                }
                if (strcmp(dectype, "globalvar") == 0)
                {
                    const char* identifier = declaration->m_identifier[i];

                    // add the variable name to the list of globals
                    in_out_reflection_accumulator.m_namespace_global.add_id(identifier);

                    // mark the variable name as a *bare* global specifically
                    // (this means it can be accessed without the `global.` prefix)
                    WRITE_LOCK(in_out_reflection_accumulator.m_mutex_bare_globals);
                    in_out_reflection_accumulator.m_bare_globals.insert(identifier);
                }
            }
        }

        // enum
        if (ast->m_subtype == ogm_ast_st_imp_enum)
        {
            WRITE_LOCK(in_out_reflection_accumulator.m_mutex_enums);
            auto& map = in_out_reflection_accumulator.m_enums->m_map;
            ogm_ast_declaration_t* payload;
            ogm_ast_tree_get_payload_declaration(ast, &payload);
            std::string enum_name = payload->m_type;
            if (map.find(enum_name) == map.end())
            {
                auto& enum_data = map[enum_name];
                for (size_t i = 0; i < payload->m_identifier_count; ++i)
                {
                    if (enum_data.m_map.find(payload->m_identifier[i]) != enum_data.m_map.end())
                    {
                        throw MiscError("Multiple definitions of enum value " + std::string(payload->m_identifier[i]));
                    }

                    if (ast->m_sub[i].m_subtype != ogm_ast_st_imp_empty)
                    // some enum value declarations have definitions, too.
                    {
                        enum_data.m_map[payload->m_identifier[i]] = ogm_ast_copy(&ast->m_sub[i]);
                    }
                    else
                    // no enum value definition
                    {
                        // _ast: will be deleted by enum_data on destruction.
                        ogm_ast_t* _ast = new ogm_ast_t;
                        _ast->m_type = ogm_ast_t_exp;
                        _ast->m_subtype = ogm_ast_st_exp_literal_primitive;
                        auto* _payload = (ogm_ast_literal_primitive_t*) malloc( sizeof(ogm_ast_literal_primitive_t) );
                        _payload->m_type = ogm_ast_literal_t_number;
                        _payload->m_value = _strdup(std::to_string(i).c_str());;
                        _ast->m_payload = _payload;
                        enum_data.m_map[payload->m_identifier[i]] = _ast;
                    }
                }
            }
            else
            {
                throw MiscError("Error: multiple definitions for enum " + enum_name + ".");
            }
        }

        // recurse
        for (size_t i = 0; i < ast->m_sub_count; ++i)
        {
            bytecode_preprocess_helper(&ast->m_sub[i], ast, out_retc, out_argc, in_out_reflection_accumulator);
        }
    }
}

void bytecode_preprocess(DecoratedAST& io_a, ReflectionAccumulator& io_refl)
{
    io_a.m_retc = 0;

    // TODO: check if function takes 0 arguments or is variadic by default.
    io_a.m_argc = -1;
    bytecode_preprocess_helper(
        io_a.m_ast, nullptr, io_a.m_retc, io_a.m_argc, io_refl
    );
}

}
}
