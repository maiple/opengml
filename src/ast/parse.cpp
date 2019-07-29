#include "ogm/ast/parse.h"

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Production.hpp"
#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"

#include <string>
#include <cstring>

#define make_ast(n) ((n == 0) ? nullptr : (ogm_ast_t*) malloc( sizeof(ogm_ast_t) * n))

const char* ogm_ast_spec_string[] =
{
    "",
    "with",
    "while",
    "do",
    "repeat",

    "continue",
    "break",
    "exit",
    "return",

    // accessors
    "",
    "@",
    "?",
    "#",
    "|",

    // possessive
    ".",

    // operations
    "+",
    "-",
    "*",
    "/",
    "div",
    "mod",
    "%",

    "<<",
    ">>",

    // comparisons
    "==",
    "!=",
    "<",
    "<=",
    ">",
    ">=",
    "<>",

    "&&",
    "and",
    "||",
    "or",
    "^^",
    "xor",

    "&",
    "|",
    "^",

    "!",
    "not",
    "~",

    // assignments
    "=",
    "+=",
    "-=",
    "*=",
    "/=",
    "&=",
    "|=",
    "^=",
    "<<=",
    ">>=",
    "++",
    "++",
    "--",
    "--",
    "{ }",
    ""
};

const char* ogm_ast_subtype_string[] =
{
    // expressions
    "literal primitive",
    "identifier",
    "accessor",
    "paren",
    "arithmetic",
    "function",
    "possessive",
    "global",

    // statements
    "#define",
    "empty",
    "body",
    "assignment",
    "var",
    "if",
    "for",
    "loop",
    "switch",
    "control",
    "enum"
};

namespace
{
    ogm_ast_line_column lc(LineColumn lc)
    {
        return{ static_cast<int32_t>(lc.m_line), static_cast<int32_t>(lc.m_col) };
    }

    char* buffer(std::string s)
    {
        int32_t l = s.length() + 1;
        char* c = (char*) malloc( l );
        memcpy(c, s.c_str(), l);
        return c;
    }

    ogm_ast_spec_t op_to_spec(std::string op)
    {
        ogm_ast_spec_t to_return = ogm_ast_spec_none;
        while (to_return < ogm_ast_spec_count)
        {
            if (op == ogm_ast_spec_string[to_return])
            {
                return to_return;
            }
            ++*(int*)&to_return;
        }
        return ogm_ast_spec_none;
    }

    void
    initialize_ast_from_production(
        ogm_ast_t& out,
        Production* production
    )
    {
        out.m_start = lc(production->m_start);
        out.m_end   = lc(production->m_end);

        // apply this node's infixes (whitespace / comments):
        {
            production->flattenPostfixes();
            int32_t payload_list_count = production->infixes.size();
            out.m_decor_list_length = payload_list_count;
            if (out.m_decor_list_length > 0)
            {
                out.m_decor_count = (int32_t*) malloc( sizeof(int32_t) * payload_list_count );
                out.m_decor = (ogm_ast_decor_t**) malloc( sizeof(ogm_ast_decor_t*) * payload_list_count );
                for (int32_t i = 0; i < payload_list_count; i++)
                {
                    PrInfixWS* infix = production->infixes[i];
                    if (infix)
                    {
                        infix->flattenPostfixes();
                        int32_t payload_count = infix->infixes.size() + 1;
                        out.m_decor_count[i] = payload_count;
                        if (payload_count > 0)
                        {
                            out.m_decor[i] = (ogm_ast_decor_t*) malloc( sizeof(ogm_ast_decor_t) * payload_count );
                            for (int32_t j = 0; j < payload_count; j++)
                            {
                                ogm_ast_decor_t& payload = out.m_decor[i][j];
                                switch (infix->val.type)
                                {
                                case ENX:
                                    payload.m_type = ogm_ast_decor_t_whitespace;
                                    break;
                                case COMMENT:
                                    if (infix->val.value[1] == '*')
                                    {
                                        payload.m_type = ogm_ast_decor_t_comment_ml;
                                    }
                                    else
                                    {
                                        payload.m_type = ogm_ast_decor_t_comment_sl;
                                    }
                                    payload.m_content = buffer(infix->val.value);
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        out.m_decor_count[i] = 0;
                    }
                }
            }
            else
            {
                out.m_decor_list_length = 0;
            }
        }

        // handle node type.
        handle_type(p, PrExprParen*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_paren;
            out.m_sub = make_ast(1);
            out.m_sub_count = 1;
            initialize_ast_from_production(*out.m_sub, p->content);
        }
        else handle_type(p, PrExpressionFn*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_fn;
            out.m_payload = buffer(p->identifier.value);
            int32_t arg_count = p->args.size();
            out.m_sub_count = arg_count;
            out.m_sub = make_ast(arg_count);
            for (int32_t i = 0; i < arg_count; i++)
            {
                initialize_ast_from_production(out.m_sub[i], p->args[i]);
            }
        }
        else handle_type(p, PrExprArithmetic*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_spec = op_to_spec(p->op.value);

            // number of children. if 1, just use rhs.
            int8_t i = 0;

            if (out.m_spec == ogm_ast_spec_possessive)
            // possessives are a different type of ast node altogether.
            {
                PrIdentifier* lhs_identifier = dynamic_cast<PrIdentifier*>(p->lhs);

                if ((lhs_identifier) && lhs_identifier->identifier.value == "global")
                // not a possessive -- actually global variable
                {
                    out.m_subtype = ogm_ast_st_exp_global;
                    i = 1;
                    goto arithmetic_recurse_ast;
                }
                else
                // honest possessive
                {
                    out.m_subtype = ogm_ast_st_exp_possessive;
                }
            }
            else
            // honest arithmetic
            {
                out.m_subtype = ogm_ast_st_exp_arithmetic;
            }
            if (out.m_spec == ogm_ast_spec_acc_list)
            {
                // hardcoding here is required because
                // | is ambiguously a list accessor or bitwise or.
                out.m_spec = ogm_ast_spec_op_bitwise_or;
            }

            // handle pre plusplus and pre-minusminus
            if (p->lhs && !p ->rhs)
            {
                if (out.m_spec == ogm_ast_spec_op_unary_pre_plusplus ||
                    out.m_spec == ogm_ast_spec_op_unary_pre_minusminus)
                {
                    // set to be post_plusplus / post_minusminus
                    ++*(int*)&out.m_spec;

                    out.m_sub = make_ast(1);
                    out.m_sub_count = 1;
                    initialize_ast_from_production(*out.m_sub, p->lhs);
                    return;
                }
            }
            else if (!p->lhs && p->rhs)
            {
                i = 1;
            }
            else
            {
                i = 2;
            }

        arithmetic_recurse_ast:
            out.m_sub = make_ast(i);
            out.m_sub_count = i;
            if (i == 1)
            {
                initialize_ast_from_production(*out.m_sub, p->rhs);
            }
            else
            {
                initialize_ast_from_production(out.m_sub[0], p->lhs);
                initialize_ast_from_production(out.m_sub[1], p->rhs);
            }
        }
        else handle_type(p, PrFinal*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_literal_primitive;
            auto* payload = (ogm_ast_literal_primitive_t*) malloc( sizeof(ogm_ast_literal_primitive_t) );
            switch (p->final.type)
            {
            case NUM:
                // hex number
                payload->m_type = ogm_ast_literal_t_number;
                break;
            case STR:
                payload->m_type = ogm_ast_literal_t_string;
                break;
            default:
                throw MiscError("Unknown literal type");
            }
            payload->m_value = buffer(p->final.value);
            out.m_payload = payload;
            out.m_sub_count = 0;
        }
        else handle_type(p, PrIdentifier*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_identifier;
            out.m_payload = buffer(p->identifier.value);
            out.m_sub_count = 0;
        }
        else handle_type(p, PrAccessorExpression*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_accessor;
            out.m_spec = op_to_spec(p->acc);
            if (out.m_spec == ogm_ast_spec_none)
            {
                out.m_spec = ogm_ast_spec_acc_none;
            }
            out.m_sub_count = 1 + p->indices.size();
            out.m_sub = make_ast(out.m_sub_count);
            initialize_ast_from_production(out.m_sub[0], p->ds);
            for (int32_t i = 1; i < out.m_sub_count; i++)
            {
                initialize_ast_from_production(out.m_sub[i], p->indices[i - 1]);
            }
        }
        else handle_type(p, PrEmptyStatement*, production)
        {
            (void)p;
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_empty;
            out.m_sub_count = 0;
        }
        else handle_type(p, PrStatementFn*, production)
        {
            // we allow expressions to be a type of statement,
            // so there is no need to distinguish between
            // functions as a statement and functions as an expression.
            initialize_ast_from_production(out, p->fn);

            // there is no need for the m_type field so we will probably get rid of it.
            out.m_type = ogm_ast_t_imp;
        }
        else handle_type(p, PrStatementVar*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_var;

            auto& declaration = *(ogm_ast_declaration_t*) malloc( sizeof(ogm_ast_declaration_t) );
            declaration.m_identifier_count = p->declarations.size();
            declaration.m_identifier = (char**) malloc( sizeof(char*) * p->declarations.size() );
            declaration.m_type = buffer(p->type);
            out.m_sub_count = declaration.m_identifier_count;
            out.m_sub = make_ast(declaration.m_identifier_count);
            out.m_payload = &declaration;
            for (int32_t i = 0; i < declaration.m_identifier_count; i++)
            {
                PrVarDeclaration* subDeclaration = p->declarations[i];
                declaration.m_identifier[i] = buffer(subDeclaration->identifier.value);
                if (subDeclaration->definition == nullptr)
                {
                    // empty definition
                    out.m_sub[i].m_type = ogm_ast_t_imp;
                    out.m_sub[i].m_subtype = ogm_ast_st_imp_empty;
                    out.m_sub[i].m_sub_count = 0;
                    out.m_sub[i].m_decor_list_length = 0;
                }
                else
                {
                    // proper definition
                    initialize_ast_from_production(out.m_sub[i], subDeclaration->definition);
                }
            }
        }
        else handle_type(p, PrStatementEnum*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_enum;

            auto& declaration = *(ogm_ast_declaration_t*) malloc( sizeof(ogm_ast_declaration_t) );
            declaration.m_type = buffer(p->identifier.value);
            declaration.m_identifier_count = p->declarations.size();
            declaration.m_identifier = (char**) malloc( sizeof(char*) * p->declarations.size() );
            out.m_sub_count = declaration.m_identifier_count;
            out.m_sub = make_ast(declaration.m_identifier_count);
            out.m_payload = &declaration;
            for (int32_t i = 0; i < declaration.m_identifier_count; i++)
            {
                PrVarDeclaration* subDeclaration = p->declarations[i];
                declaration.m_identifier[i] = buffer(subDeclaration->identifier.value);
                if (subDeclaration->definition == nullptr)
                {
                    // empty definition
                    out.m_sub[i].m_type = ogm_ast_t_imp;
                    out.m_sub[i].m_subtype = ogm_ast_st_imp_empty;
                    out.m_sub[i].m_sub_count = 0;
                    out.m_sub[i].m_decor_list_length = 0;
                }
                else
                {
                    // proper definition
                    initialize_ast_from_production(out.m_sub[i], subDeclaration->definition);
                }
            }
        }
        else handle_type(p, PrBodyContainer*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_body_list;
            out.m_payload = malloc(p->bodies.size() * sizeof(char*));
            out.m_sub_count = p->bodies.size();
            out.m_sub = make_ast(out.m_sub_count);
            for (int32_t i = 0; i < out.m_sub_count; ++i)
            {
                static_cast<char**>(out.m_payload)[i] = buffer(p->names[i]);
                initialize_ast_from_production(out.m_sub[i], p->bodies[i]);
            }
        }
        else handle_type(p, PrBody*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_body;
            if (p->is_root)
            {
                out.m_spec = ogm_ast_spec_body_braceless;
            }
            else
            {
                out.m_spec = ogm_ast_spec_body_braced;
            }
            int32_t count = p->productions.size();
            out.m_sub_count = count;
            out.m_sub = make_ast(count);
            for (int32_t i = 0; i < count; i++)
            {
                initialize_ast_from_production(out.m_sub[i], p->productions[i]);
            }
        }
        else handle_type(p, PrAssignment*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_spec = op_to_spec(p->op.value);
            if (out.m_spec == ogm_ast_spec_acc_list)
            {
                // have to manually disambiguate "|" here
                out.m_spec = ogm_ast_spec_op_bitwise_or;
            }
            if (!p->lhs != !p->rhs)
            {
                // unary assignment production -- handle this as an expression.
                out.m_subtype = ogm_ast_st_exp_arithmetic;
                if (p->rhs)
                {
                    // post
                    ++*(int*)& out.m_spec;
                }
                out.m_sub_count = 1;
                out.m_sub = make_ast(1);
                initialize_ast_from_production(
                    *out.m_sub,
                    (p->lhs) ? p->lhs : p->rhs
                );
            }
            else
            {
                out.m_subtype = ogm_ast_st_imp_assignment;
                out.m_sub_count = 2;
                out.m_spec = op_to_spec(p->op.value);
                out.m_sub = make_ast(2);
                initialize_ast_from_production(out.m_sub[0], p->lhs);
                initialize_ast_from_production(out.m_sub[1], p->rhs);
            }
        }
        else handle_type(p, PrStatementIf*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_if;
            int32_t i;
            if (p->otherwise)
            {
                i = 3;
            }
            else
            {
                i = 2;
            }
            out.m_sub_count = i;
            out.m_sub = make_ast(i);
            initialize_ast_from_production(out.m_sub[0], p->condition);
            if (i == 2)
            {
                initialize_ast_from_production(out.m_sub[1], p->result);
            }
            else
            {
                initialize_ast_from_production(out.m_sub[1], p->result);
                initialize_ast_from_production(out.m_sub[2], p->otherwise);
            }
        }
        else handle_type(p, PrFor*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_for;
            out.m_sub_count = 4;
            out.m_sub = make_ast(4);
            initialize_ast_from_production(out.m_sub[0], p->init);
            initialize_ast_from_production(out.m_sub[1], p->condition);
            initialize_ast_from_production(out.m_sub[2], p->second);
            initialize_ast_from_production(out.m_sub[3], p->first);
        }
        else handle_type(p, PrWhile*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_loop;
            out.m_spec = ogm_ast_spec_loop_while;
            out.m_sub_count = 2;
            out.m_sub = make_ast(2);
            initialize_ast_from_production(out.m_sub[0], p->condition);
            initialize_ast_from_production(out.m_sub[1], p->event);
        }
        else handle_type(p, PrRepeat*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_loop;
            out.m_spec = ogm_ast_spec_loop_repeat;
            out.m_sub_count = 2;
            out.m_sub = make_ast(2);
            initialize_ast_from_production(out.m_sub[0], p->count);
            initialize_ast_from_production(out.m_sub[1], p->event);
        }
        else handle_type(p, PrDo*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_loop;
            out.m_spec = ogm_ast_spec_loop_do_until;
            out.m_sub_count = 2;
            out.m_sub = make_ast(2);
            initialize_ast_from_production(out.m_sub[0], p->condition);
            initialize_ast_from_production(out.m_sub[1], p->event);
        }
        else handle_type(p, PrWith*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_loop;
            out.m_spec = ogm_ast_spec_loop_with;
            out.m_sub_count = 2;
            out.m_sub = make_ast(2);
            initialize_ast_from_production(out.m_sub[0], p->objid);
            initialize_ast_from_production(out.m_sub[1], p->event);
        }
        else handle_type(p, PrSwitch*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_switch;
            out.m_sub_count = 1 + (p->cases.size() << 1);
            out.m_sub = make_ast(out.m_sub_count);
            initialize_ast_from_production(out.m_sub[0], p->condition);
            for (int32_t i = 0; i < p->cases.size(); i++)
            {
                PrCase* pCase = p->cases[i];
                size_t case_i = 1 + (i << 1);
                if (pCase->value)
                {
                    // case
                    initialize_ast_from_production(out.m_sub[case_i], pCase->value);
                }
                else
                {
                    // default
                    out.m_sub[case_i].m_type = ogm_ast_t_imp;
                    out.m_sub[case_i].m_subtype = ogm_ast_st_imp_empty;
                    out.m_sub[case_i].m_sub_count = 0;
                    out.m_sub[case_i].m_decor_list_length = 0;
                }
                int32_t body_index = (1 + i) << 1;
                initialize_ast_from_production(out.m_sub[body_index], pCase);
            }
        }
        else handle_type(p, PrCase*, production)
        {
            // ignore the case value as it will be handled by the switch handler above.
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_body;
            out.m_spec = ogm_ast_spec_body_braceless;
            int32_t sub_count = p->productions.size();
            out.m_sub_count = sub_count;
            out.m_sub = make_ast(sub_count);
            for (int32_t j = 0; j < sub_count; j++)
            {
                initialize_ast_from_production(out.m_sub[j], p->productions[j]);
            }
        }
        else handle_type(p, PrControl*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_control;
            out.m_spec = op_to_spec(p->kw.value);
            if (p->val)
            {
                out.m_sub_count = 1;
                out.m_sub = make_ast(1);
                initialize_ast_from_production(*out.m_sub, p->val);
            }
            else
            {
                out.m_sub_count = 0;
            }
        }
        else
        {
            throw ParseError(std::string("Unknown production encountered: ") + production->to_string());
        }
    }
}

ogm_ast_t* ogm_ast_parse(
    const char* code
)
{
    Parser p(code);
    PrBodyContainer* bodies = p.parse();
    ogm_ast_t* out = static_cast<ogm_ast_t*>(malloc(sizeof(ogm_ast_t)));

    initialize_ast_from_production(*out, bodies);
    delete bodies;
    return out;
}

ogm_ast_t* ogm_ast_parse_expression(
    const char* code
)
{
    Parser p(code);
    PrExpression* expression = p.parse_expression();
    ogm_ast_t* out = static_cast<ogm_ast_t*>(malloc(sizeof(ogm_ast_t)));

    initialize_ast_from_production(*out, expression);

    delete expression;

    return out;
}

// free AST's components
void ogm_ast_free_components(
    ogm_ast_t* tree
)
{
    // free children
    for (size_t i = 0; i < tree->m_sub_count; ++i)
    {
        ogm_ast_free_components(&tree->m_sub[i]);
    }

    // free infixes
    if (tree->m_decor_list_length > 0)
    {
        for (size_t i = 0; i < tree->m_decor_list_length; ++i)
        {
            for (size_t j = 0; j < tree->m_decor_count[i]; ++j)
            {
                const auto type = tree->m_decor[i][j].m_type;
                if (type == ogm_ast_decor_t_comment_sl || type == ogm_ast_decor_t_comment_ml)
                {
                    // free comment string
                    free(tree->m_decor[i][j].m_content);
                }
            }
            if (tree->m_decor_count[i] > 0)
            {
                free(tree->m_decor[i]);
            }
        }

        free(tree->m_decor);
        free(tree->m_decor_count);
        tree->m_decor_list_length = 0;
    }

    // free payload
    switch (tree->m_subtype)
    {
    case ogm_ast_st_imp_body_list:
        for (size_t i = 0; i < tree->m_sub_count; i++)
        {
            free(static_cast<char**>(tree->m_payload)[i]);
        }
        free(tree->m_payload);
        break;
    case ogm_ast_st_exp_fn:
    case ogm_ast_st_exp_identifier:
        free(tree->m_payload);
        break;
    case ogm_ast_st_imp_var:
    case ogm_ast_st_imp_enum:
        {
            auto* declaration = static_cast<ogm_ast_declaration_t*>(tree->m_payload);
            for (size_t i = 0; i < declaration->m_identifier_count; ++i)
            {
                free(declaration->m_identifier[i]);
            }
            free(declaration->m_identifier);
            free(declaration->m_type);
            free(tree->m_payload);
        }
        break;
    case ogm_ast_st_exp_literal_primitive:
        free(static_cast<ogm_ast_literal_primitive_t*>(tree->m_payload)->m_value);
        free(tree->m_payload);
        break;
    }

    if (tree->m_sub_count > 0 && tree->m_sub)
    {
        free(tree->m_sub);
    }
}

// free AST
void ogm_ast_free(
    ogm_ast_t* tree
)
{
    ogm_ast_free_components(tree);
    free(tree);
}
