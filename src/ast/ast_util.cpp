#include "ogm/ast/parse.h"

#include <iostream>
#include <cstring>

payload_type_t ogm_ast_tree_get_payload_type(
    const ogm_ast_t* tree
)
{
    switch (tree->m_subtype)
    {
    case ogm_ast_st_imp_assignment:
    case ogm_ast_st_exp_arithmetic:
    case ogm_ast_st_exp_accessor:
    case ogm_ast_st_imp_body:
    case ogm_ast_st_imp_loop:
    case ogm_ast_st_imp_control:
        return ogm_ast_payload_t_spec;
    case ogm_ast_st_exp_literal_primitive:
        return ogm_ast_payload_t_literal_primitive;
    case ogm_ast_st_imp_var:
        return ogm_ast_payload_t_declaration;
    case ogm_ast_st_imp_enum:
    case ogm_ast_st_exp_literal_struct:
        return ogm_ast_payload_t_declaration_enum;
    case ogm_ast_st_exp_literal_function:
        return ogm_ast_payload_t_literal_function;
    case ogm_ast_st_exp_identifier:
        return ogm_ast_payload_t_string;
    case ogm_ast_st_imp_body_list:
        return ogm_ast_payload_t_string_list;
    default:
        return ogm_ast_payload_t_none;
    }
}

const char* ogm_ast_tree_get_payload_string(
    const ogm_ast_t* tree
)
{
    if (ogm_ast_tree_get_payload_type(tree) == ogm_ast_payload_t_string)
    {
        return (const char*) tree->m_payload;
    }
    else
    {
        return nullptr;
    }
}

const char* ogm_ast_tree_get_payload_string_list(
    const ogm_ast_t* tree,
    size_t i
)
{
    if (ogm_ast_tree_get_payload_type(tree) == ogm_ast_payload_t_string_list)
    {
        if (i >= tree->m_sub_count) return nullptr;
        return ((const char**) tree->m_payload)[i];
    }
    else
    {
        return nullptr;
    }
}

bool ogm_ast_tree_get_spec(
    const ogm_ast_t* tree,
    ogm_ast_spec_t* out_spec
)
{
    if (ogm_ast_tree_get_payload_type(tree) == ogm_ast_payload_t_spec)
    {
        *out_spec = tree->m_spec;
        return true;
    }
    else
    {
        *out_spec = ogm_ast_spec_none;
        return false;
    }
}

bool ogm_ast_tree_get_payload_literal_primitive(
    const ogm_ast_t* tree,
    ogm_ast_literal_primitive_t** out_payload
)
{
    if (tree->m_subtype == ogm_ast_st_exp_literal_primitive)
    {
        *out_payload = (ogm_ast_literal_primitive_t*)tree->m_payload;
        return true;
    }
    return false;
}

bool ogm_ast_tree_get_payload_declaration(
    const ogm_ast_t* tree,
    ogm_ast_declaration_t** out_payload
)
{
    if (tree->m_subtype == ogm_ast_st_imp_var || tree->m_subtype == ogm_ast_st_imp_enum || tree->m_subtype == ogm_ast_st_exp_literal_struct)
    {
        *out_payload = (ogm_ast_declaration_t*)tree->m_payload;
        return true;
    }
    return false;
}

bool ogm_ast_tree_get_payload_function_literal(
    const ogm_ast_t* tree,
    ogm_ast_literal_function_t** out_payload
)
{
    if (tree->m_subtype == ogm_ast_st_exp_literal_function)
    {
        *out_payload = (ogm_ast_literal_function_t*)tree->m_payload;
        return true;
    }
    return false;
}

void print_indent(
    int32_t indent
)
{
    for (int32_t i = 0; i < indent; i++)
    {
        std::cout << "    ";
    }
}

void ogm_ast_tree_print_helper(
    const ogm_ast_t* tree,
    int32_t indent
)
{
    print_indent(indent);
    if (tree->m_type == ogm_ast_t_exp)
    {
        std::cout << "& ";
    }
    else
    {
        std::cout << "> ";
    }

    std::cout << ogm_ast_subtype_string[tree->m_subtype];

    ogm_ast_spec_t spec;
    if (ogm_ast_tree_get_spec(tree, &spec))
    {
        std::cout << " " << ogm_ast_spec_string[tree->m_spec];
    }

    // extra payload:
    switch(tree->m_subtype)
    {
        case ogm_ast_st_exp_literal_primitive:
            {
                ogm_ast_literal_primitive_t* payload;
                ogm_ast_tree_get_payload_literal_primitive(tree, &payload);
                std::cout << ": " << payload->m_value;
            }
            break;
        case ogm_ast_st_exp_identifier:
            std::cout << ": " << (char*) tree->m_payload;
            break;
        default:
            break;
    }

    std::cout << " [" << tree->m_start.m_line + 1<< ":" << tree->m_start.m_column + 1
        << " - " << tree->m_end.m_line + 1<< ":" << tree->m_end.m_column + 1 << "]";

    std::cout << std::endl;

    // subtrees
    for (int32_t i = 0; i < tree->m_sub_count; i++)
    {
        ogm_ast_tree_print_helper(
            tree->m_sub + i,
            indent + 1
        );
    }
}

void ogm_ast_tree_print(
    const ogm_ast_t* tree
)
{
    ogm_ast_tree_print_helper(tree, 0);
}
