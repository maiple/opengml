#ifdef OPTIMIZE_PARSE
#ifdef __GNUC__
#pragma GCC optimize ("O3")
#endif
#endif

#include "grammar.hpp"
#include "ogm/ast/parse.h"

#include "ogm/common/util.hpp"
#include "ogm/common/error.hpp"
#include "ogm/common/compile_time.h"

#include <string>
#include <cstring>
#include <set>
#include <map>
#include <unordered_map>
#include <time.h>
#include <iostream>

// some compilers don't recognize strdup, so we use our own.
#define strdup _strdup

// allocates an AST node with n children
inline ogm_ast_t* make_ast(size_t n)
{
    if (n == 0) return nullptr;
    size_t size = sizeof(ogm_ast_t) * n;
    ogm_ast_t* ast = (ogm_ast_t*) malloc(size);
    memset(ast, 0, size);
    return ast;
}

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
    "new",
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
    // generate spec lookup
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
}

// for debugging only
inline void print_pegtl_tree(const ogm::ast_node& node, char nodetype='R', size_t depth=0)
{
    if (!node.is_root())
    {
        for (size_t i = 0; i < depth; ++i)
        {
            std::cout << "  ";
        }
        std::cout << "[" << nodetype << "] " << node.name() << std::endl;
    }

    for (const auto& child : node.children)
    {
        if (child)
        {
            print_pegtl_tree(
                static_cast<const ogm::ast_node&>(*child.get()),
                'C',
                depth + (node.is_root() ? 0 : 1)
            );
        }
    }
    for (const auto& child : node.m_children_primary)
    {
        if (child)
        {
            print_pegtl_tree(*child.get(), 'P', depth + (node.is_root() ? 0 : 1));
        }
    }
    for (const auto& child : node.m_children_intermediate)
    {
        if (child)
        {
            print_pegtl_tree(*child.get(), 'I', depth + (node.is_root() ? 0 : 1));
        }
    }
}

namespace ogm
{   
    using namespace ogm::peg;

    #define INIT_AST(RULE, N, AST) template<> \
    void initialize_ast<RULE>(N, AST)
    
    static inline void ast_type(ogm_ast& ast, ogm_ast_type_t t, ogm_ast_subtype_t st)
    {
        ast.m_type = t;
        ast.m_subtype = st;
    }
    
    static inline void ast_spec(ogm_ast& ast, ogm_ast_spec spec)
    {
        ast.m_spec = spec;
    }

    INIT_AST(top_level, const ast_node& n, ogm_ast& ast)
    {
        ast_type(ast, ogm_ast_t_imp, ogm_ast_st_imp_body_list);
        assert(n.m_children_primary.size() >= 1);
        char** names = alloc<char*>(n.m_children_primary.size());
        ast.m_payload = names;
        names[0] = strdup(""); // first body has no name.
        assert(n.m_children_primary.at(0)->is<body_bare>());
        
        for (size_t i = 1; i < n.m_children_primary.size(); ++i)
        {
            // next bodies are 'define' statements.
            // TODO -- set payload string from define statement.
        }
    }
    
    INIT_AST(body_bare, const ast_node& n, ogm_ast& ast)
    {
        ast_type(ast, ogm_ast_t_imp, ogm_ast_st_imp_body);
        ast_spec(ast, ogm_ast_spec_body_braceless);
    }
    
    INIT_AST(body_braced, const ast_node& n, ogm_ast& ast)
    {
        ast_type(ast, ogm_ast_t_imp, ogm_ast_st_imp_body);
        ast_spec(ast, ogm_ast_spec_body_braced);
    }
    
    INIT_AST(st_var, const ast_node& n, ogm_ast& ast)
    {
        ast_type(ast, ogm_ast_t_imp, ogm_ast_st_imp_var);
        std::string type_name = "";
        
        ogm_ast_declaration_t* declaration = alloc<ogm_ast_declaration_t>();
        ast.m_payload = declaration;
        
        declaration->m_identifier_count = ast.m_sub_count;
        if (n.m_children_primary.size() > 0)
        {
            declaration->m_identifier = alloc<char*>(n.m_children_primary.size());
        }
        
        // there must be at least one declaration.
        declaration->m_types = alloc<char*>(std::max<size_t>(1, n.m_children_primary.size()));
        for (size_t i = 0; i < n.m_children_intermediate.size(); ++i)
        {
            // even intermediates are typenames (e.g. 'var')
            // odd intermediates are identifiers (i.e. local variable names)
            if (i % 2 == 0)
            {
                const ast_node& child_typename = *n.m_children_intermediate.at(i);
                assert(child_typename.is<st_var_type>() || child_typename.is<st_var_type_opt>());
                if (child_typename.content() != "")
                {
                    type_name = child_typename.content();
                }
                declaration->m_types[i / 2] = strdup(type_name.c_str());
            }
            else
            {
                const ast_node& child_varname = *n.m_children_intermediate.at(i);
                assert(child_varname.is<identifier>());
                declaration->m_identifier[i / 2] = child_varname.allocate_c_str();
            }
        }
    }
    
    INIT_AST(st_var_decl_empty_expression, const ast_node& n, ogm_ast& ast)
    {
        // TODO -- this should obviously be expression type instead of imperative type.
        ast_type(ast, ogm_ast_t_imp, ogm_ast_st_imp_empty);
    }
    
    INIT_AST(ex_lit_number, const ast_node& n, ogm_ast& ast)
    {
        ast_type(ast, ogm_ast_t_exp, ogm_ast_st_exp_literal_primitive);
        ogm_ast_literal_primitive_t* payload = alloc<ogm_ast_literal_primitive_t>();
        ast.m_payload = payload;
        payload->m_type = ogm_ast_literal_t_number;
        payload->m_value = n.allocate_c_str();
    }
    
    INIT_AST(ex_identifier, const ast_node& n, ogm_ast& ast)
    {
        ast_type(ast, ogm_ast_t_exp, ogm_ast_st_exp_identifier);
        ast.m_payload = n.allocate_c_str();
    }
    
    INIT_AST(st_opl1_pre, const ast_node& n, ogm_ast& ast)
    {
        ast_type(ast, ogm_ast_t_imp, ogm_ast_st_exp_arithmetic);
        assert(n.m_children_intermediate.at(0)->is<op_l1>());
        std::string op = n.m_children_intermediate.at(0)->content();
        ast.m_spec = op_to_spec(op.c_str());
    }
    
    INIT_AST(st_opl1_post, const ast_node& n, ogm_ast& ast)
    {
        ast_type(ast, ogm_ast_t_imp, ogm_ast_st_exp_arithmetic);
        assert(n.m_children_intermediate.at(0)->is<op_l1>());
        std::string op = n.m_children_intermediate.at(0)->content();
        ast.m_spec = op_to_spec(op.c_str());
        (*reinterpret_cast<int*>(&ast.m_spec))++;  // add one to get 'post' version
    }
    
    INIT_AST(st_assign, const ast_node& n, ogm_ast& ast)
    {
        ast_type(ast, ogm_ast_t_imp, ogm_ast_st_imp_assignment);
        assert(n.m_children_intermediate.at(0)->is<op_assign>());
        std::string op = n.m_children_intermediate.at(0)->content();
        ast.m_spec = op_to_spec(op.c_str());
    }
    
    INIT_AST(ex_function_call, const ast_node& n, ogm_ast& ast)
    {
        ast_type(ast, ogm_ast_t_exp, ogm_ast_st_exp_fn);
    }
    
    INIT_AST(st_function_call, const ast_node& n, ogm_ast& ast)
    {
        ast_type(ast, ogm_ast_t_imp, ogm_ast_st_exp_fn);
    }
}

ogm_ast_t* ogm_ast_parse(
    const char* code,
    int flags
)
{
    try
    {
        // TODO
        pegtl::string_input<> in(code, "");
        std::unique_ptr<ogm::ast_node> ast =
            (flags & ogm_ast_parse_flag_expression)
                ? pegtl::parse_tree::parse< ogm::peg::top_level_expression, ogm::ast_node, ogm::peg::ast_selector >( in )
                : pegtl::parse_tree::parse< ogm::peg::top_level, ogm::ast_node, ogm::peg::ast_selector >( in );

        if (ast)
        {
            // TODO: remove print statements.
            print_pegtl_tree(*ast.get());
            std::cout << "\n--------------------\n\n";
            
            // create OGM ast from pegtl ast.
            ogm_ast_t* out = make_ast(1);
            ast->initialize_root(*out);
            ogm_ast_tree_print(out);
            return out;
        }
        else
        {
            return nullptr;
        }
    }
    catch(ogm::Error& e)
    {
        throw e.detail<ogm::source_buffer>(code);
    }
}

// free AST's components
void ogm_ast_free_components(
    ogm_ast_t* tree
)
{
    // free line source strings
    tree->m_start.cleanup();
    tree->m_end.cleanup();

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
    ogm_ast* out = make_ast(1);
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

    // returns pointer to buffer which must be freed.
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

    static void ogm_ast_load_helper(ogm_ast_t* ast, std::istream& in, const std::map<size_t, ogm_location_t>& line_sources)
    {
        int32_t canary = read_int(in);
        ogm_assert(canary == k_canary/11);
        int32_t type = read_int(in);
        ast->m_type = static_cast<ogm_ast_type_t>(type);
        ogm_assert(static_cast<uint32_t>(type) < 200);
        ast->m_subtype = static_cast<ogm_ast_subtype_t>(read_int(in));
        ast->m_start = line_sources.at(read_int(in));
        ast->m_start.m_column = read_int(in);
        ast->m_start.m_line = read_int(in);
        ast->m_start.m_source_column = read_int(in);
        ast->m_start.m_source_line = read_int(in);
        ast->m_end = line_sources.at(read_int(in));
        ast->m_end.m_column = read_int(in);
        ast->m_end.m_line = read_int(in);
        ast->m_end.m_source_column = read_int(in);
        ast->m_end.m_source_line = read_int(in);
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
        case ogm_ast_payload_t_literal_function:
            {
                ogm_ast_literal_function_t* lit =
                    (ogm_ast_literal_function_t*)
                    malloc( sizeof(ogm_ast_literal_function_t) );
                ast->m_payload = lit;
                lit->m_name = read_string(in);
                if (strlen(lit->m_name) == 0)
                {
                    free(lit->m_name);
                    lit->m_name = nullptr;
                }
                int32_t tmp_constructor = read_int(in);
                ogm_assert(tmp_constructor == 0 || tmp_constructor == 1);
                lit->m_constructor = !!tmp_constructor;
                lit->m_arg_count = read_int(in);
                ogm_assert(lit->m_arg_count >= 0);
                lit->m_arg = nullptr;
                if (lit->m_arg_count > 0)
                {
                    lit->m_arg = (char**)malloc(sizeof(char*) * lit->m_arg_count);
                    for (int32_t i = 0; i < lit->m_arg_count; ++i)
                    {
                        lit->m_arg[i] = read_string(in);
                    }
                }
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
            ogm_ast_load_helper(ast->m_sub + i, in, line_sources);
        }

        canary = read_int(in);
        ogm_assert(canary == k_canary);
        canary = read_int(in);
        ogm_assert(canary == k_canary/2);
    }

    static void ogm_ast_write_string_helper(const char* str, std::ostream& out, std::map<const char*, size_t>& strmap)
    {
        if (str && strmap.find(str) == strmap.end())
        {
            // record stream position and then write the string.
            strmap[str] = out.tellp();
            out.write(str, strlen(str) + 1);
        }
    }

    static void ogm_ast_write_string_helper(const ogm_ast_t* ast, std::ostream& out, std::map<const char*, size_t>& strmap)
    {
        // write strings for locations.
        ogm_ast_write_string_helper(ast->m_start.m_source, out, strmap);
        ogm_ast_write_string_helper(ast->m_end.m_source, out, strmap);

        // recurse
        for (size_t i = 0; i < ast->m_sub_count; ++i)
        {
            ogm_ast_write_string_helper(ast->m_sub + i, out, strmap);
        }
    }

    static void ogm_ast_write_helper(const ogm_ast_t* ast, std::ostream& out, std::map<const char*, size_t>& strmap)
    {
        write_int(out, k_canary/11);
        write_int(out, ast->m_type);
        write_int(out, ast->m_subtype);
        write_int(out, strmap[ast->m_start.m_source]);
        write_int(out, ast->m_start.m_column);
        write_int(out, ast->m_start.m_line);
        write_int(out, ast->m_start.m_source_column);
        write_int(out, ast->m_start.m_source_line);
        write_int(out, strmap[ast->m_end.m_source]);
        write_int(out, ast->m_end.m_column);
        write_int(out, ast->m_end.m_line);
        write_int(out, ast->m_end.m_source_column);
        write_int(out, ast->m_end.m_source_line);
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
        case ogm_ast_payload_t_literal_function:
            ogm_ast_literal_function_t* lit;
            ogm_ast_tree_get_payload_function_literal(ast, &lit);
            if (lit->m_name)
            {
                write_string(out, lit->m_name);
            }
            else
            {
                write_string(out, "");
            }
            write_int(out, lit->m_constructor);
            write_int(out, lit->m_arg_count);
            for (int32_t i = 0; i < lit->m_arg_count; ++i)
            {
                write_string(out, lit->m_arg[i]);
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
            ogm_ast_write_helper(ast->m_sub + i, out, strmap);
        }

        write_int(out, k_canary);
        write_int(out, k_canary/2);
    }
}

static void ogm_ast_load_line_sources(std::istream& in, std::map<size_t, ogm_location_t>& line_sources)
{
    size_t data_end = read_int(in);
    line_sources[0] = {0, 0};
    while (in.tellg() < data_end)
    {
        size_t pos = in.tellg();
        char* str = read_string(in);
        line_sources[pos] = {0, 0, 0, 0, str};
        free(str);
    }
}

ogm_ast_t* ogm_ast_load(std::istream& in)
{
    int32_t canary = read_int(in);
    ogm_assert(canary == 0x5050);

    std::map<size_t, ogm_location_t> line_sources;
    ogm_ast_load_line_sources(in, line_sources);

    ogm_ast_t* ast = make_ast(1);
    ogm_assert(ast);
    ogm_ast_load_helper(ast, in, line_sources);
    return ast;
}

void ogm_ast_write(const ogm_ast_t* ast, std::ostream& out)
{
    write_int(out, 0x5050);

    // write string data section -----------

    // length of data section
    size_t size_tellp = out.tellp();
    write_int(out, 0);

    // contents of data section
    std::map<const char*, size_t> strmap;
    strmap[nullptr] = 0;
    ogm_ast_write_string_helper(ast, out, strmap);
    size_t tmp_tellp = out.tellp();
    out.seekp(size_tellp);
    write_int(out, tmp_tellp);
    out.seekp(tmp_tellp);

    // write tree data section --------------
    ogm_ast_write_helper(ast, out, strmap);
}

uint64_t ogm_ast_parse_version()
{
    #ifdef OGM_BUILD_GMTOFF
        return __TIME_UNIX__ - (OGM_BUILD_GMTOFF);
    #else
        #ifdef __GNUC__
            time_t t = time(NULL);
            struct tm lt = {0};
            localtime_r(&t, &lt);
            return __TIME_UNIX__ - lt.tm_gmtoff;
        #else
            // TODO.
            return __TIME_UNIX__;
        #endif
    #endif
}

void ogm_ast_deleter_t::operator()(ogm_ast_t* ast) const
{
    if (ast)
    {
        ogm_ast_free(ast);
    }
};
