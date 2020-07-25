#pragma once

#include "ogm/common/error.hpp"
#include "ogm/bytecode/bytecode.hpp"

// compiles and sets a code macro's ast value
inline void set_macro(const char* name, const char* value, ogm::bytecode::ReflectionAccumulator& reflection)
{
    WRITE_LOCK(reflection.m_mutex_macros);
    auto& macros = reflection.m_ast_macros;
    ogm_ast_t* ast;
    try
    {
        // try parsing as an expression
        ast = ogm_ast_parse_expression(value);
    }
    catch (...)
    {
        // not a valid expression -- try as a statement instead
        try
        {
            ast = ogm_ast_parse(value);
        }
        catch (...)
        {
            throw ogm::ProjectError(1006, "Cannot parse macro '{} as either an expression nor statement.", name);
        }
    }

    if (macros.find(name) != macros.end())
    {
        ogm_ast_free(macros[name]);
    }
    macros[name] = ast;
}
