#pragma once

#include "ogm/bytecode/bytecode.hpp"

// compiles and sets a code macro's ast value
static void set_macro(const char* name, const char* value, ogm::bytecode::ReflectionAccumulator& reflection)
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
            throw MiscError("Cannot parse macro '" + std::string(name) + "' as either an expression nor statement.");
        }
    }

    if (macros.find(name) != macros.end())
    {
        ogm_ast_free(macros[name]);
    }
    macros[name] = ast;
}
