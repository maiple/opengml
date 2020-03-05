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

#include <string>
#include <cstring>
#include <unordered_map>

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
    
    // deletes self when decremented to zero.
    template<typename T>
    class referenced
    {
    public:
        T m_data;
        int m_refcount;
        
        template<typename... Args>
        referenced(Args... args)
            : m_data(args...)
            , m_refcount{ 0 }
        { }
        
        referenced<T>* increment()
        {
            m_refcount++;
            return this;
        }
        
        void decrement()
        {
            m_refcount--;
            if (m_refcount <= 0) delete this;
        }
    };
    
    // data for creating asts.
    struct ast_accumulator_t
    {
        ogm_ast_line_column_t m_start_location;
        referenced<std::string>* m_file;
    };
    
    ogm_ast_line_column_t operator+(const ogm_ast_line_column_t& a, const ogm_ast_line_column_t& b)
    {
        ogm_ast_line_column_t c;
        if (a.m_line == 0)
        {
            c.m_column = a.m_column + b.m_column;
        }
        else
        {
            c.m_column = a.m_column;
        }
        c.m_line = a.m_line + b.m_line;
        return c;
    }

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
        Production* production,
        ast_accumulator_t accumulator
    )
    {
        out.m_start = lc(production->m_start) + accumulator.m_start_location;
        out.m_end   = lc(production->m_end) + accumulator.m_start_location;
        ogm_assert(accumulator.m_file != nullptr);
        out.m_file  = accumulator.m_file->increment();

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
            initialize_ast_from_production(*out.m_sub, p->content, accumulator);
        }
        else handle_type(p, PrArrayLiteral*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_literal_array;
            out.m_sub = make_ast(p->vector.size());
            out.m_sub_count = p->vector.size();
            for (size_t i = 0; i < out.m_sub_count; ++i)
            {
                initialize_ast_from_production(out.m_sub[i], p->vector.at(i), accumulator);
            }
        }
        else handle_type(p, PrTernary*, production)
        {
            out.m_type = ogm_ast_t_exp;
            out.m_subtype = ogm_ast_st_exp_ternary_if;
            out.m_sub = make_ast(3);
            out.m_sub_count = 3;
            initialize_ast_from_production(out.m_sub[0], p->condition, accumulator);
            initialize_ast_from_production(out.m_sub[1], p->result, accumulator);
            initialize_ast_from_production(out.m_sub[2], p->otherwise, accumulator);
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
                initialize_ast_from_production(out.m_sub[i], p->args[i], accumulator);
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
                    initialize_ast_from_production(*out.m_sub, p->lhs, accumulator);
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
                initialize_ast_from_production(*out.m_sub, p->rhs, accumulator);
            }
            else
            {
                initialize_ast_from_production(out.m_sub[0], p->lhs, accumulator);
                initialize_ast_from_production(out.m_sub[1], p->rhs, accumulator);
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
                throw ParseError("Unknown literal type");
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
            initialize_ast_from_production(out.m_sub[0], p->ds, accumulator);
            for (int32_t i = 1; i < out.m_sub_count; i++)
            {
                initialize_ast_from_production(out.m_sub[i], p->indices[i - 1], accumulator);
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
            initialize_ast_from_production(out, p->fn, accumulator);

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
                }
                else
                {
                    // proper definition
                    initialize_ast_from_production(out.m_sub[i], subDeclaration->definition, accumulator);
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
                }
                else
                {
                    // proper definition
                    initialize_ast_from_production(out.m_sub[i], subDeclaration->definition, accumulator);
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
                initialize_ast_from_production(out.m_sub[i], p->bodies[i], accumulator);
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
                initialize_ast_from_production(out.m_sub[i], p->productions[i], accumulator);
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
                    (p->lhs) ? p->lhs : p->rhs,
                    accumulator
                );
            }
            else
            {
                out.m_subtype = ogm_ast_st_imp_assignment;
                out.m_sub_count = 2;
                out.m_spec = op_to_spec(*p->op.value);
                out.m_sub = make_ast(2);
                initialize_ast_from_production(out.m_sub[0], p->lhs, accumulator);
                initialize_ast_from_production(out.m_sub[1], p->rhs, accumulator);
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
            initialize_ast_from_production(out.m_sub[0], p->condition, accumulator);
            if (i == 2)
            {
                initialize_ast_from_production(out.m_sub[1], p->result, accumulator);
            }
            else
            {
                initialize_ast_from_production(out.m_sub[1], p->result, accumulator);
                initialize_ast_from_production(out.m_sub[2], p->otherwise, accumulator);
            }
        }
        else handle_type(p, PrFor*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_for;
            out.m_sub_count = 4;
            out.m_sub = make_ast(4);
            initialize_ast_from_production(out.m_sub[0], p->init, accumulator);
            initialize_ast_from_production(out.m_sub[1], p->condition, accumulator);
            initialize_ast_from_production(out.m_sub[2], p->second, accumulator);
            initialize_ast_from_production(out.m_sub[3], p->first, accumulator);
        }
        else handle_type(p, PrWhile*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_loop;
            out.m_spec = ogm_ast_spec_loop_while;
            out.m_sub_count = 2;
            out.m_sub = make_ast(2);
            initialize_ast_from_production(out.m_sub[0], p->condition, accumulator);
            initialize_ast_from_production(out.m_sub[1], p->event, accumulator);
        }
        else handle_type(p, PrRepeat*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_loop;
            out.m_spec = ogm_ast_spec_loop_repeat;
            out.m_sub_count = 2;
            out.m_sub = make_ast(2);
            initialize_ast_from_production(out.m_sub[0], p->count, accumulator);
            initialize_ast_from_production(out.m_sub[1], p->event, accumulator);
        }
        else handle_type(p, PrDo*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_loop;
            out.m_spec = ogm_ast_spec_loop_do_until;
            out.m_sub_count = 2;
            out.m_sub = make_ast(2);
            initialize_ast_from_production(out.m_sub[0], p->condition, accumulator);
            initialize_ast_from_production(out.m_sub[1], p->event, accumulator);
        }
        else handle_type(p, PrWith*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_loop;
            out.m_spec = ogm_ast_spec_loop_with;
            out.m_sub_count = 2;
            out.m_sub = make_ast(2);
            initialize_ast_from_production(out.m_sub[0], p->objid, accumulator);
            initialize_ast_from_production(out.m_sub[1], p->event, accumulator);
        }
        else handle_type(p, PrSwitch*, production)
        {
            out.m_type = ogm_ast_t_imp;
            out.m_subtype = ogm_ast_st_imp_switch;
            out.m_sub_count = 1 + (p->cases.size() << 1);
            out.m_sub = make_ast(out.m_sub_count);
            initialize_ast_from_production(out.m_sub[0], p->condition, accumulator);
            for (int32_t i = 0; i < p->cases.size(); i++)
            {
                PrCase* pCase = p->cases[i];
                size_t case_i = 1 + (i << 1);
                if (pCase->value)
                {
                    // case
                    initialize_ast_from_production(out.m_sub[case_i], pCase->value, accumulator);
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
                initialize_ast_from_production(out.m_sub[body_index], pCase, accumulator);
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
                initialize_ast_from_production(out.m_sub[j], p->productions[j], accumulator);
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
                initialize_ast_from_production(*out.m_sub, p->val, accumulator);
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

namespace
{
    ogm_ast_t* ogm_ast_parse_helper(
        parse_args_t args,
        int flags,
        bool expression
    )
    {
        ast_accumulator_t accumulator;
        accumulator.m_start_location = args.m_first_line_column;
        accumulator.m_file = new referenced<std::string>(args.m_source_file);
        accumulator.m_file->increment();
        
        Production* pr;
        try
        {
            Parser p(args.m_code, flags);
            if (expression)
            {
                pr = p.parse_expression();
            }
            else
            {
                pr = p.parse();
            }
        }
        catch (const ParseError& p)
        {
            // adjust parse error location to offset location.
            ogm_ast_line_column_t loc{ p.location.first, p.location.second };
            loc = loc + accumulator.m_start_location;
            throw ParseError(p.base_message, {loc.m_line, loc.m_column}, accumulator.m_file->m_data);
        }
        
        ogm_ast_t* out = static_cast<ogm_ast_t*>(malloc(sizeof(ogm_ast_t)));

        initialize_ast_from_production(*out, pr, accumulator);
        delete pr;
        accumulator.m_file->decrement();
        return out;
    }
}

ogm_ast_t* ogm_ast_parse(
    parse_args_t args,
    int flags
)
{
    return ogm_ast_parse_helper(args, flags, false);
}

ogm_ast_t* ogm_ast_parse_expression(
    parse_args_t args,
    int flags
)
{
    return ogm_ast_parse_helper(args, flags, true);
}

// free AST's components
void ogm_ast_free_components(
    ogm_ast_t* tree
)
{
    // free filename string.
    if (tree->m_file != nullptr)
    {
        static_cast<referenced<std::string>*>(tree->m_file)->decrement();
    }
    
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
        {
            out->m_payload = malloc(sizeof(ogm_ast_declaration_t*));
            ogm_ast_declaration_t* declaration = static_cast<ogm_ast_declaration_t*>(out->m_payload);
            declaration->m_identifier_count = static_cast<ogm_ast_declaration_t*>(tree->m_payload)->m_identifier_count;
            declaration->m_identifier = static_cast<char**>(malloc(sizeof(char*) * declaration->m_identifier_count));
            for (size_t i = 0; i < declaration->m_identifier_count; ++i)
            {
                declaration->m_identifier[i] = buffer(static_cast<ogm_ast_declaration_t*>(tree->m_payload)->m_identifier[i]);
            }
            declaration->m_type = buffer(static_cast<ogm_ast_declaration_t*>(tree->m_payload)->m_type);
        }
        break;
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
