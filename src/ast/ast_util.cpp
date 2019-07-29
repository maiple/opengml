#include "ogm/ast/ast_util.h"
#include "ogm/ast/parse.h"

#include <iostream>
#include <cstring>

bool ogm_ast_tree_get_spec(
    const ogm_ast_t* tree,
    ogm_ast_spec_t* out_spec
)
{
    switch (tree->m_subtype)
    {
    case ogm_ast_st_exp_arithmetic:
    case ogm_ast_st_exp_accessor:
    case ogm_ast_st_imp_body:
    case ogm_ast_st_imp_loop:
    case ogm_ast_st_imp_control:
        *out_spec = tree->m_spec;
        return true;
    default:
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
    if (tree->m_subtype == ogm_ast_st_imp_var || tree->m_subtype == ogm_ast_st_imp_enum)
    {
        *out_payload = (ogm_ast_declaration_t*)tree->m_payload;
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
