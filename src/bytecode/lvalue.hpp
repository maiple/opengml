#pragma once

#include "generate.hpp"
#include <memory>

namespace ogm::bytecode
{
    
inline accessor_type_t get_accessor_type_from_spec(ogm_ast_spec_t s)
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

// an LValue represents a "location" that can be read from or written to,
// assuming that certain pre-defined values are on the stack.
// (For example, arr[4] could be an lvalue, and it requires that the index 4 is on the stack to be used.)
// The following functions relate to LValue usage:
// - bytecode_generate_get_lvalue(): produces an lvalue, and writes the bytecode which produces the pre-requisite stack values.
// - bytecode_generate_load(): takes an lvalue, and writes the bytecode which pops the pre-requisite values frmo the stack, pushing the evaluated lvalue.
// - bytecode_generate_store(): takes an lvalue, and writes the bytecode which pops the value to store and then the pre-requisite values frmo the stack.
// - bytecode_generate_reuse_lvalue(): duplicates the pre-requisite values on the stack.

struct LValue
{
    friend void bytecode_generate_load(std::ostream& out, const LValue& lv, const GenerateContextArgs& context_args);
    friend void bytecode_generate_store(std::ostream& out, const LValue& lv, const GenerateContextArgs& context_args);
    friend void bytecode_generate_reuse_lvalue(std::ostream& out, const LValue& lv);
    template<bool owned, bool macro>
    friend LValue bytecode_generate_get_lvalue(std::ostream& out, const ogm_ast_t& ast, GenerateContextArgs context_args);
    
    LValue()
        : m_pop_count(0)
        , m_array_access(false)
        , m_accessor_type(accessor_none)
        , m_no_copy(false)
        , m_read_only(false)
        , m_nest_depth(0)
    { }

private:
    LValue(memspace_t memspace, variable_id_t address, size_t pop_count, bool array_access, accessor_type_t accessor_type, bool no_copy, bool read_only, size_t nest_depth)
        : m_memspace(memspace)
        , m_address(address)
        , m_pop_count(pop_count)
        , m_array_access(array_access)
        , m_accessor_type(accessor_type)
        , m_no_copy(no_copy)
        , m_read_only(read_only)
        , m_nest_depth(nest_depth)
    { }

public:    
    // number of values that need to be popped from the stack to dereference this.
    size_t m_pop_count;

    memspace_t m_memspace;
    variable_id_t m_address;

private:
    bool m_array_access;
    
    accessor_type_t m_accessor_type;
    
    // array is accessed as no-copy.
    bool m_no_copy;
    
    // can't write to this.
    bool m_read_only;
    
    // for nested array access
    size_t m_nest_depth;
};

// if an LValue needs to be used twice, this duplicates the
// stack values needed to dereference it.
inline void bytecode_generate_reuse_lvalue(std::ostream& out, const LValue& lv)
{
    using namespace ogm::asset;
    using namespace opcode;
    
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
            if (lv.m_pop_count >= 0x100)
            {
                throw MiscError("lvalue context cannot exceed 255 bytes.");
            }
            uint8_t pc = lv.m_pop_count;
            write(out, pc);
            return;
    }
}

namespace
{
    bool bytecode_generate_id_for_lvalue_access(std::ostream& out, const LValue& lv, const GenerateContextArgs& context_args)
    {
        int32_t id;
        switch (lv.m_memspace)
        {
            case memspace_instance:
                context_args.m_library->generate_constant_bytecode(out, "self");
                return true;
            case memspace_other:
                // already on stack.
                break;
            case memspace_global:
                context_args.m_library->generate_constant_bytecode(out, "global");
                return true;
            case memspace_builtin_instance:
            case memspace_builtin_other:
            case memspace_builtin_global:
                throw MiscError("nested array access on builtin variables is invalid.");
                break;
            default:
                assert(false);
                break;
        }
        return false;
    }
}

// generates bytecode which puts the value at the given lvalue onto the stack.
// bc precondition: pre-requisite values are on the stack.
// bc postcondition: pre-requisite values are popped
inline void bytecode_generate_load(std::ostream& out, const LValue& lv, const GenerateContextArgs& context_args)
{
    using namespace ogm::asset;
    using namespace opcode;
    
    if (lv.m_nest_depth)
    {
        assert(lv.m_array_access);
        
        if (lv.m_memspace != memspace_local)
        {
            bytecode_generate_id_for_lvalue_access(out, lv, context_args);
            write_op(out, ldoax);
        }
        else
        {
            write_op(out, ldlax);
        }
        
        write(out, lv.m_address);
        
        int32_t depth = lv.m_nest_depth;
        write(out, depth);
    }
    else
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
            assert(!lv.m_array_access);
            if (!context_args.m_library->generate_accessor_bytecode(out, lv.m_accessor_type, lv.m_pop_count, false))
            {
                throw MiscError("Could not compile accessor.");
            }
            break;
        }
    }
}

// generates bytecode which stores the shallowest value on the stack at the given lv location.
// bc precondition: pre-requisite values are on the stack (before the value).
// bc postcondition: value and pre-requisite values are popped
inline void bytecode_generate_store(std::ostream& out, const LValue& lv, const GenerateContextArgs& context_args)
{
    using namespace ogm::asset;
    using namespace opcode;
    
    if (lv.m_read_only)
    {
        throw MiscError("Cannot write to read-only variable");
    }
    if (lv.m_no_copy)
    {
        write_op(out, sfx);
    }
    if (lv.m_nest_depth)
    {
        assert(lv.m_array_access);
        
        if (lv.m_memspace != memspace_local)
        {
            if (bytecode_generate_id_for_lvalue_access(out, lv, context_args))
            {
                // id *then* value is what is needed.
                write_op(out, swap);
            }
            write_op(out, stoax);
        }
        else
        {
            write_op(out, stlax);
        }
        
        write(out, lv.m_address);
        
        int32_t depth = lv.m_nest_depth;
        write(out, depth);
        return;
    }
    else
    {
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
            if (!context_args.m_library->generate_variable_bytecode(out, lv.m_address, lv.m_pop_count, true))
            {
                throw MiscError("Unable to generate bytecode to access builtin variable no. " + std::to_string(lv.m_address) + " (" + std::to_string(lv.m_pop_count) + ")");
            }
            break;
        case memspace_constant:
            throw MiscError("Cannot assign to constant literal value.");
            break;
        case memspace_accessor:
            assert(!lv.m_array_access);
            if (!context_args.m_library->generate_accessor_bytecode(out, lv.m_accessor_type, lv.m_pop_count, true))
            {
                throw MiscError("Could not compile accessor.");
            }
            break;
        }
    }
    if (lv.m_no_copy)
    {
        write_op(out, ufx);
    }
}

template<bool owned, bool macro>
inline LValue bytecode_generate_get_lvalue(std::ostream& out, const ogm_ast_t& ast, GenerateContextArgs context_args);

template<bool owned=false>
inline LValue bytecode_generate_get_lvalue(std::ostream& out, const ogm_ast_t& ast, GenerateContextArgs context_args)
{
    return bytecode_generate_get_lvalue<owned, false>(out, ast, context_args);
}

// gets the lvalue (and loading bytecode) for the given expression.
// template arguments are intended for internal recursion only.
// owned: whether this is an owned property, e.g. the part after the `.` in `other.var`.
// macro: whether this is an extended macro value.
template<bool owned, bool macro>
inline LValue bytecode_generate_get_lvalue(std::ostream& out, const ogm_ast_t& ast, GenerateContextArgs context_args)
{
    using namespace ogm::asset;
    using namespace opcode;
    
    if (ast.m_type == ogm_ast_t_imp)
    {
        throw MiscError("Cannot obtain LValue from imperative statement.");
    }
    // identify address of lvalue
    variable_id_t address_lhs;
    memspace_t memspace_lhs;
    size_t pop_count = 0;
    bool array_access = false;
    accessor_type_t accessor_type = accessor_none;
    bool read_only = false;
    bool no_copy = false;
    size_t nest_count = 0;
    switch (ast.m_subtype)
    {
    case ogm_ast_st_exp_identifier:
        {
            char* var_name = (char*) ast.m_payload;
            BuiltInVariableDefinition def;
            asset_index_t asset_index;
            const auto& macros = context_args.m_reflection->m_ast_macros;
            
            // locals (cannot appear as `other.local`)
            if (!owned && context_args.m_symbols->m_namespace_local.has_id(var_name))
            {
                // check locals for variable name
                address_lhs = context_args.m_symbols->m_namespace_local.get_id(var_name);
                memspace_lhs = memspace_local;
            }
            // statics (cannot appear as other.constant)
            else if (!owned && context_args.m_statics->find(var_name) != context_args.m_statics->end())
            {
                address_lhs = context_args.m_statics->at(var_name);
                memspace_lhs = memspace_global;
            }
            // constants (canot appear as other.constant)
            else if (!owned && context_args.m_library->generate_constant_bytecode(out, var_name))
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
                    #ifdef OGM_2DARRAY
                    pop_count = 2;
                    #else
                    pop_count = 1;
                    #endif
                    
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
                        #ifdef OGM_2DARRAY
                        int32_t row = 0;
                        write_op(out, ldi_s32);
                        write(out, row);
                        #endif

                        // column is specified
                        bytecode_generate_ast(out, ast.m_sub[1], context_args);
                    }
                    else
                    {
                        if (ast.m_sub_count != 3)
                        {
                            throw MiscError("Arrays must have 1 or 2 indices.");
                        }
                        #ifndef OGM_2DARRAY
                        nest_count = 1;
                        bytecode_generate_ast(out, ast.m_sub[2], context_args);
                        bytecode_generate_ast(out, ast.m_sub[1], context_args);
                        #else
                        bytecode_generate_ast(out, ast.m_sub[1], context_args);
                        bytecode_generate_ast(out, ast.m_sub[2], context_args);
                        #endif
                    }

                    // because the array variable itself is an lvalue, we recurse to find out what it is
                    // before decorating it.
                    LValue sub = bytecode_generate_get_lvalue<owned, macro>(out, ast.m_sub[0], context_args);
                    
                    if (sub.m_memspace == memspace_constant)
                    {
                        throw MiscError("Accessing constants as arrays is not presently supported.");
                    }

                    // decorate the lvalue to make it an array access.
                    if (sub.m_memspace == memspace_accessor)
                    {
                        throw MiscError("nesting array inside data structure not supported yet.");
                    }
                    else if (sub.m_array_access)
                    {
                        #ifdef OGM_GARBAGE_COLLECTOR
                        // when wrapping an existing array
                        memspace_lhs = sub.m_memspace;
                        address_lhs = sub.m_address;
                        read_only = sub.m_read_only;
                        if (no_copy != sub.m_no_copy)
                        {
                            throw MiscError("mixing array copy-on-write status is not supported yet.");
                        }
                        
                        if (
                            sub.m_memspace == memspace_builtin_other
                            || sub.m_memspace == memspace_builtin_instance
                            || sub.m_memspace == memspace_builtin_global
                        )
                        {
                            // builtin arrays are only indexed by the deepest-level index.
                            size_t ignored_count = nest_count +
                            #ifdef OGM_2DARRAY
                                2
                            #else
                                1
                            #endif
                                * (sub.m_nest_depth + 1);
                            pop_count += sub.m_pop_count - ignored_count;
                            
                            // sub should at most have the ID in addition to the indices
                            assert(sub.m_pop_count - ignored_count <= 1);
                            
                            // remove all from sub except the ID.
                            for (size_t i = 0; i < ignored_count; ++i)
                            {
                                if (sub.m_pop_count - ignored_count == 1)
                                {
                                    // preserve the index at the top.
                                    write_op(out, swap);
                                }
                                write_op(out, pop);
                            }
                            
                            nest_count = 0;
                        }
                        else
                        {
                            pop_count += sub.m_pop_count;
                            nest_count = sub.m_nest_depth + 1;
                        }
                        #else
                        throw MiscError("Nested array access requires garbage collector to be enabled. Compile ogm with -DOGM_GARBAGE_COLLECTOR.");
                        #endif
                    }
                    else
                    {
                        memspace_lhs = sub.m_memspace;
                        address_lhs = sub.m_address;
                        read_only = sub.m_read_only;
                        pop_count += sub.m_pop_count;
                        
                        #ifndef OGM_2DARRAY
                        // fix for builtin_array[0, i] == builtin-array[i]
                        if (
                            sub.m_memspace == memspace_builtin_other
                            || sub.m_memspace == memspace_builtin_instance
                            || sub.m_memspace == memspace_builtin_global
                        )
                        {
                            for (size_t i = 0; i < nest_count; ++i)
                            {
                                if (sub.m_pop_count == 1)
                                {
                                    write_op(out, swap);
                                }
                                write_op(out, pop);
                            }
                            nest_count = 0;
                        }
                        #endif

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
                }
                break;
            default:
                // data structure accessor
                {
                    // evaluate data structure id and put it on the stack
                    {
                        LValue lv_ds = bytecode_generate_get_lvalue<owned>(out, ast.m_sub[0], context_args);
                        bytecode_generate_load(out, lv_ds, context_args);
                    }

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

    return {memspace_lhs, address_lhs, pop_count, array_access, accessor_type, no_copy, read_only, nest_count};
}

}