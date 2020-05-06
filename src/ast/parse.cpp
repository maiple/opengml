#ifdef OPTIMIZE_PARSE
#ifdef __GNUC__
#pragma GCC optimize ("O3")
#endif
#endif

#include "ogm/ast/parse.h"

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Production.hpp"
#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/compile_time.h"

#include <string>
#include <cstring>
#include <unordered_map>
#include <time.h>

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
    "%=",
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
    "primitive literal",
    "identifier",
    "accessor",
    "array literal",
    "struct literal",
    "function literal",
    "paren",
    "arithmetic",
    "function",
    "ternary if",
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

    std::unordered_map<std::string, ogm_ast_spec_t> spec_lookup;

    struct STARTUPFN
    {
        STARTUPFN()
        {
            for (size_t i = ogm_ast_spec_count; i > 0; --i)
            {
                spec_lookup[ogm_ast_spec_string[i - 1]] = static_cast<ogm_ast_spec_t>(i - 1);
            }
        }
    } _;

    ogm_ast_line_column lc(LineColumn lc)
    {
        return{ static_cast<int32_t>(lc.m_line), static_cast<int32_t>(lc.m_col) };
    }

    char* buffer(const std::string& s)
    {
        int32_t l = s.length() + 1;
        char* c = (char*) malloc( l );
        memcpy(c, s.c_str(), l);
        return c;
    }

    ogm_ast_spec_t op_to_spec(const std::string& op)
    {
        auto iter = spec_lookup.find(op);
        if (iter != spec_lookup.end()) return iter->second;
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
                                    if (infix->val.value->at(1) == '*')
                                    {
                                        payload.m_type = ogm_ast_decor_t_comment_ml;
                                    }
                                    else
                                    {
                                        payload.m_type = ogm_ast_decor_t_comment_sl;
                                    }
                                    payload.m_content = buffer(*infix->val.value);
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
        else handle_type(p, PrArrayLiteral*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_literal_array;
            out.m_sub = make_ast(p->vector.size());
            out.m_sub_count = p->vector.size();
            for (size_t i = 0; i < out.m_sub_count; ++i)
            {
                initialize_ast_from_production(out.m_sub[i], p->vector.at(i));
            }
        }
        else handle_type(p, PrStructLiteral*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_literal_struct;

            auto& declaration = *(ogm_ast_declaration_t*) malloc( sizeof(ogm_ast_declaration_t) );
            declaration.m_type = nullptr;
            declaration.m_identifier_count = p->declarations.size();
            declaration.m_identifier = (char**) malloc( sizeof(char*) * p->declarations.size() );
            out.m_sub_count = declaration.m_identifier_count;
            out.m_sub = make_ast(declaration.m_identifier_count);
            out.m_payload = &declaration;
            for (int32_t i = 0; i < declaration.m_identifier_count; i++)
            {
                PrVarDeclaration* subDeclaration = p->declarations[i];
                declaration.m_identifier[i] = buffer(*subDeclaration->identifier.value);
                ogm_assert(subDeclaration->definition);
                {
                    // proper definition
                    initialize_ast_from_production(out.m_sub[i], subDeclaration->definition);
                }
            }
        }
        else handle_type(p, PrTernary*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_ternary_if;
            out.m_sub = make_ast(3);
            out.m_sub_count = 3;
            initialize_ast_from_production(out.m_sub[0], p->condition);
            initialize_ast_from_production(out.m_sub[1], p->result);
            initialize_ast_from_production(out.m_sub[2], p->otherwise);
        }
        else handle_type(p, PrExpressionFn*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_fn;
            out.m_payload = buffer(*p->identifier.value);
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
            out.m_spec = op_to_spec(*p->op.value);

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
        else handle_type(p, PrStatementFunctionLiteral*, production)
        {
            initialize_ast_from_production(out, p->fn.get());
            out.m_type = ogm_ast_t_imp;
        }
        else handle_type(p, PrFunctionLiteral*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_literal_function;
            auto* payload = (ogm_ast_literal_function_t*) malloc( sizeof(ogm_ast_literal_function_t) );
            payload->m_constructor = p->constructor;
            payload->m_name = nullptr;
            if (p->name.value->length())
            {
                payload->m_name = buffer(p->name.value->c_str());
            }
            payload->m_arg_count = p->args.size();
            if (payload->m_arg_count > 0)
            {
                payload->m_arg = (char**) malloc(sizeof(char*) * payload->m_arg_count);
                for (int32_t i = 0; i < payload->m_arg_count; ++i)
                {
                    payload->m_arg[i] = buffer(p->args.at(i).value->c_str());
                }
            }
            else
            {
                payload->m_arg = nullptr;
            }
            out.m_sub_count = 1;
            out.m_sub = make_ast(out.m_sub_count);
            initialize_ast_from_production(out.m_sub[0], p->body.get());
        }
        else handle_type(p, PrLiteral*, production)
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
            payload->m_value = buffer(*p->final.value);
            out.m_payload = payload;
            out.m_sub_count = 0;
        }
        else handle_type(p, PrIdentifier*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_identifier;
            out.m_payload = buffer(*p->identifier.value);
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
            ogm_assert(p->types.size() == p->declarations.size());
            declaration.m_identifier_count = p->declarations.size();
            declaration.m_identifier = (char**) malloc( sizeof(char*) * p->declarations.size() );
            declaration.m_types = (char**) malloc( sizeof(char*) * p->types.size() );
            out.m_sub_count = declaration.m_identifier_count;
            out.m_sub = make_ast(declaration.m_identifier_count);
            out.m_payload = &declaration;
            for (int32_t i = 0; i < declaration.m_identifier_count; i++)
            {
                PrVarDeclaration* subDeclaration = p->declarations[i];
                if (p->types.at(i) != "")
                {
                    declaration.m_types[i] = buffer(p->types.at(i));
                }
                else
                {
                    declaration.m_types[i] = nullptr;
                }
                declaration.m_identifier[i] = buffer(*subDeclaration->identifier.value);
                if (subDeclaration->definition == nullptr)
                {
                    // empty definition
                    out.m_sub[i].m_type = ogm_ast_t_imp;
                    out.m_sub[i].m_subtype = ogm_ast_st_imp_empty;
                    out.m_sub[i].m_sub_count = 0;
                    out.m_sub[i].m_decor_list_length = 0;
                    out.m_sub[i].m_end = {0, 0};
                    out.m_sub[i].m_start = {0, 0};
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
            declaration.m_type = buffer(*p->identifier.value);
            declaration.m_identifier_count = p->declarations.size();
            declaration.m_identifier = (char**) malloc( sizeof(char*) * p->declarations.size() );
            out.m_sub_count = declaration.m_identifier_count;
            out.m_sub = make_ast(declaration.m_identifier_count);
            out.m_payload = &declaration;
            for (int32_t i = 0; i < declaration.m_identifier_count; i++)
            {
                PrVarDeclaration* subDeclaration = p->declarations[i];
                declaration.m_identifier[i] = buffer(*subDeclaration->identifier.value);
                if (subDeclaration->definition == nullptr)
                {
                    // empty definition
                    out.m_sub[i].m_type = ogm_ast_t_imp;
                    out.m_sub[i].m_subtype = ogm_ast_st_imp_empty;
                    out.m_sub[i].m_sub_count = 0;
                    out.m_sub[i].m_decor_list_length = 0;
                    out.m_sub[i].m_start = {0, 0};
                    out.m_sub[i].m_end = {0, 0};
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
            out.m_spec = op_to_spec(*p->op.value);
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
                out.m_spec = op_to_spec(*p->op.value);
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
                    out.m_sub[case_i].m_start = {0, 0};
                    out.m_sub[case_i].m_end = {0, 0};
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
            out.m_spec = op_to_spec(*p->kw.value);
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
    const char* code,
    int flags
)
{
    Parser p(code, flags);
    PrBodyContainer* bodies = p.parse();
    ogm_ast_t* out = static_cast<ogm_ast_t*>(malloc(sizeof(ogm_ast_t)));

    initialize_ast_from_production(*out, bodies);
    delete bodies;
    return out;
}

ogm_ast_t* ogm_ast_parse_expression(
    const char* code,
    int flags
)
{
    Parser p(code, flags);
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
        if (tree->m_payload)
        {
            for (size_t i = 0; i < tree->m_sub_count; i++)
            {
                free(static_cast<char**>(tree->m_payload)[i]);
            }
            free(tree->m_payload);
        }
        break;
    case ogm_ast_st_exp_fn:
    case ogm_ast_st_exp_identifier:
        free(tree->m_payload);
        break;
    case ogm_ast_st_imp_var:
        {
            auto* declaration = static_cast<ogm_ast_declaration_t*>(tree->m_payload);
            for (size_t i = 0; i < declaration->m_identifier_count; ++i)
            {
                free(declaration->m_identifier[i]);
                if (declaration->m_type[i])
                {
                    free(declaration->m_types[i]);
                }
            }
            free(declaration->m_identifier);
            free(declaration->m_type);
            free(tree->m_payload);
        }
        break;
    case ogm_ast_st_imp_enum:
    case ogm_ast_st_exp_literal_struct:
        {
            auto* declaration = static_cast<ogm_ast_declaration_t*>(tree->m_payload);
            for (size_t i = 0; i < declaration->m_identifier_count; ++i)
            {
                free(declaration->m_identifier[i]);
            }
            free(declaration->m_identifier);
            if (declaration->m_type) free(declaration->m_type);
            free(tree->m_payload);
        }
        break;
    case ogm_ast_st_exp_literal_function:
        {
            ogm_ast_literal_function_t* lit = static_cast<ogm_ast_literal_function_t*>(tree->m_payload);
            if (lit->m_name) free(lit->m_name);
            if (lit->m_arg_count)
            {
                for (int32_t i = 0; i < lit->m_arg_count; ++i)
                {
                    free(lit->m_arg[i]);
                }
                free(lit->m_arg);
            }
            free(lit);
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

ogm_ast_t* ogm_ast_copy(
    const ogm_ast_t* tree
)
{
    ogm_ast* out = static_cast<ogm_ast*>(malloc(sizeof(ogm_ast)));
    ogm_ast_copy_to(out, tree);
    return out;
}

// copy AST
void ogm_ast_copy_to(
    ogm_ast_t* out,
    const ogm_ast_t* tree
)
{
    out->m_type = tree->m_type;
    out->m_subtype = tree->m_subtype;
    out->m_spec = tree->m_spec;

    out->m_start = tree->m_start;
    out->m_end = tree->m_end;

    // copy payload
    switch (tree->m_subtype)
    {
    case ogm_ast_st_imp_body_list:
        out->m_payload = malloc(sizeof(char*) * tree->m_sub_count);
        for (size_t i = 0; i < tree->m_sub_count; ++i)
        {
            static_cast<char**>(out->m_payload)[i] = buffer(static_cast<char**>(tree->m_payload)[i]);
        }
        break;
    case ogm_ast_st_exp_fn:
    case ogm_ast_st_exp_identifier:
        out->m_payload = buffer(static_cast<char*>(tree->m_payload));
        break;
    case ogm_ast_st_imp_var:
        {
            out->m_payload = malloc(sizeof(ogm_ast_declaration_t*));
            ogm_ast_declaration_t* declaration = static_cast<ogm_ast_declaration_t*>(out->m_payload);
            declaration->m_identifier_count = static_cast<ogm_ast_declaration_t*>(tree->m_payload)->m_identifier_count;
            declaration->m_identifier = static_cast<char**>(malloc(sizeof(char*) * declaration->m_identifier_count));
            declaration->m_types = static_cast<char**>(malloc(sizeof(char*) * declaration->m_identifier_count));
            for (size_t i = 0; i < declaration->m_identifier_count; ++i)
            {
                declaration->m_identifier[i] = buffer(static_cast<ogm_ast_declaration_t*>(tree->m_payload)->m_identifier[i]);
                declaration->m_types[i] = buffer(static_cast<ogm_ast_declaration_t*>(tree->m_payload)->m_types[i]);
            }
            declaration->m_type = buffer(static_cast<ogm_ast_declaration_t*>(tree->m_payload)->m_type);
        }
        break;
    case ogm_ast_st_imp_enum:
    case ogm_ast_st_exp_literal_struct:
        {
            out->m_payload = malloc(sizeof(ogm_ast_declaration_t*));
            ogm_ast_declaration_t* declaration = static_cast<ogm_ast_declaration_t*>(out->m_payload);
            declaration->m_identifier_count = static_cast<ogm_ast_declaration_t*>(tree->m_payload)->m_identifier_count;
            declaration->m_identifier = static_cast<char**>(malloc(sizeof(char*) * declaration->m_identifier_count));
            for (size_t i = 0; i < declaration->m_identifier_count; ++i)
            {
                declaration->m_identifier[i] = buffer(static_cast<ogm_ast_declaration_t*>(tree->m_payload)->m_identifier[i]);
            }
            if (static_cast<ogm_ast_declaration_t*>(tree->m_payload)->m_type)
            {
                declaration->m_type = buffer(static_cast<ogm_ast_declaration_t*>(tree->m_payload)->m_type);
            }
            else
            {
                declaration->m_type = nullptr;
            }
        }
        break;
    case ogm_ast_st_exp_literal_function:
        {
            out->m_payload = malloc(sizeof(ogm_ast_literal_function_t));
            ogm_ast_literal_function_t* lit = static_cast<ogm_ast_literal_function_t*>(out->m_payload);
            const ogm_ast_literal_function_t* srclit = static_cast<const ogm_ast_literal_function_t*>(tree->m_payload);
            
            lit->m_constructor = srclit->m_constructor;
            if (srclit->m_name)
            {
                lit->m_name = buffer(srclit->m_name);
            }
            else
            {
                lit->m_name = nullptr;
            }
            lit->m_arg_count = srclit->m_arg_count;
            lit->m_arg = nullptr;
            if (srclit->m_arg_count > 0)
            {
                lit->m_arg = (char**) malloc(lit->m_arg_count * sizeof(char*));
                for (int32_t i = 0; i < lit->m_arg_count; ++i)
                {
                    lit->m_arg[i] = buffer(srclit->m_arg[i]);
                }
            }
        }
    case ogm_ast_st_exp_literal_primitive:
        out->m_payload = malloc(sizeof(ogm_ast_literal_primitive_t));
        static_cast<ogm_ast_literal_primitive_t*>(out->m_payload)->m_type = static_cast<ogm_ast_literal_primitive_t*>(tree->m_payload)->m_type;
        static_cast<ogm_ast_literal_primitive_t*>(out->m_payload)->m_value = buffer(static_cast<ogm_ast_literal_primitive_t*>(tree->m_payload)->m_value);
        break;
    default:
        // uses m_spec instead.
        break;
    }

    out->m_decor_list_length = tree->m_decor_list_length;
    out->m_decor_count = static_cast<int32_t*>(malloc(sizeof(int32_t) * out->m_decor_list_length));
    out->m_decor = static_cast<ogm_ast_decor_t**>(malloc(sizeof(ogm_ast_decor_t*) * out->m_decor_list_length));
    for (size_t i = 0; i < out->m_decor_list_length; ++i)
    {
        out->m_decor_count[i] = tree->m_decor_count[i];
        out->m_decor[i] = static_cast<ogm_ast_decor_t*>(malloc(sizeof(ogm_ast_decor_t) * out->m_decor_count[i]));
        for (size_t j = 0; j < out->m_decor_count[i]; ++j)
        {
            ogm_ast_decor_t* out_decor = out->m_decor[i] + j;
            ogm_ast_decor_t* tree_decor = tree->m_decor[i] + j;
            out_decor->m_type = tree_decor->m_type;
            if (tree_decor->m_content)
            {
                out_decor->m_content = buffer(tree_decor->m_content);
            }
            else
            {
                out_decor->m_content = nullptr;
            }
        }
    }

    out->m_start = tree->m_start;
    out->m_end = tree->m_end;
    out->m_sub_count = tree->m_sub_count;
    out->m_sub = static_cast<ogm_ast*>(malloc(sizeof(ogm_ast) * out->m_sub_count));

    // recurse on children.
    for (size_t i = 0; i < out->m_sub_count; ++i)
    {
        ogm_ast_copy_to(&out->m_sub[i], &tree->m_sub[i]);
    }
}

namespace
{
    decltype(strcmp("", "")) safe_strcmp(const char* a, const char* b)
    {
        if (a == nullptr)
        {
            return b != nullptr;
        }
        if (b == nullptr) return -1;
        return strcmp(a, b);
    }
}

bool ogm_ast_tree_equal(
    const ogm_ast_t* a,
    const ogm_ast_t* b
)
{
    if (a->m_type != b->m_type)
    {
        return false;
    }

    if (a->m_subtype != b->m_subtype)
    {
        return false;
    }

    if (a->m_start != b->m_start) return false;
    if (a->m_end != b->m_end) return false;

    if (a->m_sub_count != b->m_sub_count) return false;

    // FIXME ignore decor for now.

    // recurse
    for (size_t i = 0; i < a->m_sub_count; ++i)
    {
        if (!ogm_ast_tree_equal(a->m_sub + i, b->m_sub + i))
        {
            return false;
        }
    }

    // check payload
    auto payload_type = ogm_ast_tree_get_payload_type(a);
    if (payload_type != ogm_ast_payload_t_none && a->m_payload != b->m_payload)
    {
        switch (payload_type)
        {
        case ogm_ast_payload_t_none:
            break;
        case ogm_ast_payload_t_spec:
            if (a->m_spec != b->m_spec) return false;
            break;
        case ogm_ast_payload_t_string:
            if (safe_strcmp(
                ogm_ast_tree_get_payload_string(a),
                ogm_ast_tree_get_payload_string(b)
                )
            ) return false;
            break;
        case ogm_ast_payload_t_string_list:
            for (size_t i = 0; i < a->m_sub_count; ++i)
            {
                if (safe_strcmp(
                    ogm_ast_tree_get_payload_string_list(a, i),
                    ogm_ast_tree_get_payload_string_list(b, i)
                    )
                ) return false;
            }
            break;
        case ogm_ast_payload_t_literal_primitive:
            {
                if (a->m_payload == nullptr) return false;
                ogm_ast_literal_primitive* pa;
                ogm_ast_tree_get_payload_literal_primitive(a, &pa);
                ogm_ast_literal_primitive* pb;
                ogm_ast_tree_get_payload_literal_primitive(b, &pb);

                if (pa->m_type != pb->m_type)
                {
                    return false;
                }

                if (strcmp(pa->m_value, pb->m_value))
                {
                    return false;
                }
            }
            break;
        case ogm_ast_payload_t_declaration:
        case ogm_ast_payload_t_declaration_enum:
            {
                const bool is_enum = a->m_subtype == ogm_ast_st_imp_enum || a->m_subtype == ogm_ast_st_exp_literal_struct;
                if (a->m_payload == nullptr) return false;

                ogm_ast_declaration* da;
                ogm_ast_tree_get_payload_declaration(a, &da);
                ogm_ast_declaration* db;
                ogm_ast_tree_get_payload_declaration(a, &db);

                // compare declarations
                if (da->m_identifier_count != db->m_identifier_count)
                {
                    return false;
                }

                if (is_enum)
                {
                    if (safe_strcmp(da->m_type, db->m_type)) return false;
                }

                for (size_t i = 0; i < da->m_identifier_count; ++i)
                {
                    if (safe_strcmp(da->m_identifier[i], db->m_identifier[i]))
                    {
                        return false;
                    }
                    if (!is_enum)
                    {
                        if (safe_strcmp(da->m_types[i], db->m_types[i]))
                        {
                            return false;
                        }
                    }
                }
            }
            break;
        }
    }

    return true;
}

namespace
{
    int32_t read_int(std::istream& in)
    {
        int32_t i;
        in.read(static_cast<char*>((void*)&i), sizeof(int32_t));
        return i;
    }

    void write_int(std::ostream& out, int32_t i)
    {
        out.write(static_cast<char*>((void*)&i), sizeof(int32_t));
    }

    char* read_string(std::istream& in)
    {
        int32_t len = read_int(in);
        ogm_assert(len == 0x34354)
        len = read_int(in);
        if (len >= 0)
        {
            char* c = (char*) malloc(len + 1);
            in.read(c, len);
            c[len] = 0;
            return c;
        }
        else
        {
            return nullptr;
        }
    }

    void write_string(std::ostream& out, const char* c)
    {
        write_int(out, 0x34354);
        if (c)
        {
            write_int(out, strlen(c));
            out.write(c, strlen(c));
        }
        else
        {
            write_int(out, -1);
        }
    }

    static const int32_t k_canary = 0x0101DEAD;

    void ogm_ast_load_helper(ogm_ast_t* ast, std::istream& in)
    {
        int32_t canary = read_int(in);
        ogm_assert(canary == k_canary/11);
        int32_t type = read_int(in);
        ast->m_type = static_cast<ogm_ast_type_t>(type);
        ogm_assert(static_cast<uint32_t>(type) < 200);
        ast->m_subtype = static_cast<ogm_ast_subtype_t>(read_int(in));
        ast->m_start.m_column = read_int(in);
        ast->m_start.m_line = read_int(in);
        ast->m_end.m_column = read_int(in);
        ast->m_end.m_line = read_int(in);
        ast->m_sub_count = read_int(in);
        canary = read_int(in);
        ogm_assert(canary == k_canary/7);

        // read payload
        switch (ogm_ast_tree_get_payload_type(ast))
        {
        case ogm_ast_payload_t_none:
            ast->m_spec = ogm_ast_spec_none;
            break;
        case ogm_ast_payload_t_spec:
            ast->m_spec = static_cast<ogm_ast_spec_t>(read_int(in));
            break;
        case ogm_ast_payload_t_string:
            {
                ast->m_payload = read_string(in);
            }
            break;
        case ogm_ast_payload_t_string_list:
            {
                int32_t count = read_int(in);
                ogm_assert(count == ast->m_sub_count);
                char** strings = (char**)malloc(ast->m_sub_count * sizeof(char*));
                ast->m_payload = strings;
                for (size_t i = 0; i < ast->m_sub_count; ++i)
                {
                    strings[i] = read_string(in);
                }
            }
            break;
        case ogm_ast_payload_t_literal_primitive:
            {
                ogm_ast_literal_primitive_t* primitive =
                    (ogm_ast_literal_primitive_t*)
                    malloc( sizeof(ogm_ast_literal_primitive_t) );
                ast->m_payload = primitive;

                // type
                primitive->m_type = static_cast<ogm_ast_literal_primitive_type_t>(read_int(in));

                // value
                primitive->m_value = read_string(in);
            }
            break;
        case ogm_ast_payload_t_declaration:
        case ogm_ast_payload_t_declaration_enum:
            {
                const bool is_enum = ast->m_subtype == ogm_ast_st_imp_enum || ast->m_subtype == ogm_ast_st_exp_literal_struct;
                ogm_ast_declaration* declaration =
                    (ogm_ast_declaration*)
                    malloc( sizeof(ogm_ast_declaration) );
                ast->m_payload = declaration;

                // type
                int32_t d = read_int(in);
                declaration->m_identifier_count = d;

                // values
                declaration->m_identifier = (char**)malloc(d * sizeof(char*));


                if (!is_enum)
                {
                    declaration->m_types = (char**)malloc(d * sizeof(char*));
                }
                else
                {
                    declaration->m_type = read_string(in);
                }

                canary = read_int(in);
                ogm_assert(canary == k_canary/5);

                for (size_t i = 0; i < d; ++i)
                {
                    declaration->m_identifier[i] = read_string(in);
                    if (!is_enum)
                    {
                        declaration->m_types[i] = read_string(in);
                    }
                }

                canary = read_int(in);
                ogm_assert(canary == k_canary/5);
            }
            break;
        default:
            throw MiscError("Unknown payload type");
        }

        canary = read_int(in);
        ogm_assert(canary == k_canary/3);

        // FIXME ignore decor for now.
        ast->m_decor_list_length = 0;

        ast->m_sub = make_ast(ast->m_sub_count);
        for (size_t i = 0; i < ast->m_sub_count; ++i)
        {
            ogm_assert(ast->m_sub);
            ogm_ast_load_helper(ast->m_sub + i, in);
        }

        canary = read_int(in);
        ogm_assert(canary == k_canary);
        canary = read_int(in);
        ogm_assert(canary == k_canary/2);
    }

    void ogm_ast_write_helper(const ogm_ast_t* ast, std::ostream& out)
    {
        write_int(out, k_canary/11);
        write_int(out, ast->m_type);
        write_int(out, ast->m_subtype);
        write_int(out, ast->m_start.m_column);
        write_int(out, ast->m_start.m_line);
        write_int(out, ast->m_end.m_column);
        write_int(out, ast->m_end.m_line);
        write_int(out, ast->m_sub_count);
        write_int(out, k_canary/7);

        // write payload
        switch (ogm_ast_tree_get_payload_type(ast))
        {
        case ogm_ast_payload_t_none:
            break;
        case ogm_ast_payload_t_spec:
            write_int(out, ast->m_spec);
            break;
        case ogm_ast_payload_t_string:
            write_string(out, (char*) ast->m_payload);
            break;
        case ogm_ast_payload_t_string_list:
            write_int(out, ast->m_sub_count);
            for (size_t i = 0; i < ast->m_sub_count; ++i)
            {
                write_string(out, ((char**) ast->m_payload)[i]);
            }
            break;
        case ogm_ast_payload_t_literal_primitive:
            {
                ogm_ast_literal_primitive_t* primitive;
                ogm_ast_tree_get_payload_literal_primitive(ast, &primitive);

                // type
                write_int(out, primitive->m_type);

                // value
                write_string(out, primitive->m_value);
            }
            break;
        case ogm_ast_payload_t_declaration:
        case ogm_ast_payload_t_declaration_enum:
            {
                const bool is_enum = ast->m_subtype == ogm_ast_st_imp_enum || ast->m_subtype == ogm_ast_st_exp_literal_struct;
                ogm_ast_declaration* declaration;
                ogm_ast_tree_get_payload_declaration(ast, &declaration);

                // type
                write_int(out, declaration->m_identifier_count);

                // values
                if (is_enum)
                {
                    write_string(out, declaration->m_type);
                }

                write_int(out, k_canary/5);

                for (size_t i = 0; i < declaration->m_identifier_count; ++i)
                {
                    write_string(out, declaration->m_identifier[i]);
                    if (!is_enum)
                    {
                        write_string(out, declaration->m_types[i]);
                    }
                }
                write_int(out, k_canary/5);
            }
            break;
        default:
            throw MiscError("Unknown payload type");
        }

        write_int(out, k_canary/3);

        // FIXME ignore decor for now.

        // write subs
        for (size_t i = 0; i < ast->m_sub_count; ++i)
        {
            ogm_ast_write_helper(ast->m_sub + i, out);
        }

        write_int(out, k_canary);
        write_int(out, k_canary/2);
    }
}

ogm_ast_t* ogm_ast_load(std::istream& in)
{
    int32_t canary = read_int(in);
    ogm_assert(canary == 0x5050);

    ogm_ast_t* ast = make_ast(1);
    ogm_assert(ast);
    ogm_ast_load_helper(ast, in);
    return ast;
}

void ogm_ast_write(const ogm_ast_t* ast, std::ostream& out)
{
    write_int(out, 0x5050);

    ogm_ast_write_helper(ast, out);
}

uint64_t ogm_ast_parse_version()
{
    #ifdef OGM_BUILD_GMTOFF
    return __TIME_UNIX__ - (OGM_BUILD_GMTOFF);
    #else
    time_t t = time(NULL);
    struct tm lt = {0};
    localtime_r(&t, &lt);
    return __TIME_UNIX__ - lt.tm_gmtoff;
    #endif
}

void ogm_ast_deleter_t::operator()(ogm_ast_t* ast) const
{
    if (ast)
    {
        ogm_ast_free(ast);
    }
};
