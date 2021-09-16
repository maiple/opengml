#pragma once

#include <functional>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include "ogm/common/util.hpp"
#include "ogm/ast/parse.h"

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

namespace ogm
{

class ast_node : public pegtl::parse_tree::node
{
public:
    std::vector<std::unique_ptr<ast_node>> m_children_primary;
    std::vector<std::unique_ptr<ast_node>> m_children_intermediate;
#ifdef PARSE_GREYSPACE
    std::vector<std::unique_ptr<ast_node>> m_greyspace;
    // what kind of node is this?
    enum {
        NODE_TYPE_AST = 0, // a proper AST node that isn't greyspace (comment/newline)
        NODE_TYPE_AST_INTERMEDIATE = 1,
        NODE_TYPE_GREYSPACE_LIST = 2, // a sequence of comments and newlines
        NODE_TYPE_GREYSPACE_INDIVIDUAL = 6 // a comment or newline-sequence
        NODE_TYPE_GREYSPACE_COMPONENT = 8, // part of a greyspace (only used for individual newlines currently)
    } m_node_type;
#endif

    // union over greyspace features and ast features
    union
    {
        struct
        {
            // we save a function pointer to initialize_ast<Rule> so that we can call it
            // dynamically.
            // intermediate nodes do not get one of these (left as nullptr)
            void (*m_initialize_ast)(const ast_node&, ogm_ast_t&);
        } _ast;

        #ifdef PARSE_GREYSPACE
        struct
        {
            void (*m_initialize_decor)(const ast_node&, ogm_ast_decor_t&);
        } _greyspace;
        #endif
    };

    #ifdef PARSE_GREYSPACE
    // initializes a single decor list from a gs node.
    void initialize_ast_decor_list(int32_t& decor_count, ogm_ast_decor_t*& decor_list) const
    {
        assert(m_node_type == NODE_TYPE_GREYSPACE_LIST);
        
        decor_count = children.size();
        decor_list = alloc<ogm_ast_decor_t>(decor_count);
        for (size_t i = 0; i < decor_count; ++i)
        {
            children.at(i)->initialize_decor(decor_list[i]);
        }
    }

    void initialize_decor(ogm_ast_decor_t& decor) const
    {
        assert(m_node_type == NODE_TYPE_GREYSPACE_INDIVIDUAL);
        assert(_greyspace.m_initialize_decor);
        (_greyspace.m_initialize_decor)(*this, decor);        
    }
    
    bool is_greyspace() const
    {
        return m_node_type != NODE_TYPE_AST && m_node_type != M_NODE_TYPE_INTERMEDIATE;
    }
    #endif
    
    bool is_intermediate() const
    {
        if (_ast.m_initialize_ast)
        {
            #ifdef PARSE_GREYSPACE
            assert(m_node_type == NODE_TYPE_AST)
            #endif
            return false;
        }
        else
        {
            #ifdef PARSE_GREYSPACE
            assert(m_node_type == NODE_TYPE_AST_INTERMEDIATE)
            #endif
            return true;
        }
    }

    // initialize ogm_ast::m_decor, ogm_ast::m_decor_count, and ogm_ast::m_decor_list_count
    void initialize_ast_decor_lists(ogm_ast& ast) const
    {
        #ifdef PARSE_GREYSPACE
        assert(m_node_type == NODE_TYPE_AST);
        ast.m_decor_list_length = m_greyspace.size();
        if (ast.m_decor_list_length > 0)
        {
            ast.m_decor_count = alloc<int32_t>(ast.m_decor_list_length);
            ast.m_decor = alloc<ogm_ast_decor_t*>(ast.m_decor_list_length);
            for (size_t i = 0; i < ast.m_decor_list_length; ++i)
            {
                m_greyspace.at(i)->initialize_decor(
                    ast.m_decor_count[i],
                    ast.m_decor[i],
                );
            }
        }
        #else
        ast.m_decor_list_length = 0;
        #endif
    }
    
    void initialize_root(ogm_ast& ast)
    {
        assert(is_root());
        assert(children.size() == 1);
        ast_node* top_level = static_cast<ast_node*>(children.front().get());
        assert(!top_level->is_intermediate());
        top_level->initialize_ast(ast);
    }

    // initializes the given ogm_ast struct.
    void initialize_ast(ogm_ast& ast) const
    {
        assert(!is_root());
        assert(!is_intermediate());
        
        #ifdef PARSE_GREYSPACE
        assert(m_node_type == NODE_TYPE_AST);
        #endif
        
        // initialize ogm ast for all non-intermediate children
        // (intermediate child is defined as not having an associated ogm ast node.)
        ast.m_sub_count = m_children_primary.size();
        if (m_children_primary.size() > 0)
        {
            ast.m_sub = alloc<ogm_ast>(m_children_primary.size());
            for (size_t i = 0; i < m_children_primary.size(); ++i)
            {
                const ast_node& child = *m_children_primary.at(i);
                assert (!child.is_intermediate());
                child.initialize_ast(ast.m_sub[i]);
            }
        }

        // initialize this node
        assert(_ast.m_initialize_ast);
        (_ast.m_initialize_ast)(*this, ast);
        
        initialize_ast_decor_lists(ast);
    }
    
    // note: caller owns this pointer. Allocated with malloc.
    char* allocate_c_str() const
    {
        return dup_mem_to_str(begin_.data, end_.data);
    }
};

template<typename Rule>
void initialize_ast(const ast_node& n, ogm_ast& ast);

// applied to every 'selected' node (i.e., every node that appears in the ast).
template<typename Rule, bool intermediate>
inline void transform_selected(std::unique_ptr< ast_node >& n)
{
    assert(n->is<Rule>());
    #ifdef PARSE_GREYSPACE
    n->m_node_type = intermediate ? NODE_TYPE_AST_INTERMEDIATE : NODE_TYPE_AST;
    #endif

    for (auto& _child : n->children)
    {
        if (!_child) continue;
        ast_node* child = static_cast<ast_node*>(_child.release());
        #ifdef PARSE_GREYSPACE
        if (child->is_greyspace())
        {
            n->m_greyspace.emplace_back(child);
        }
        else
        #endif
        if (child->is_intermediate())
        {
            n->m_children_intermediate.emplace_back(child);
        }
        else
        {
            n->m_children_primary.emplace_back(child);
        }
    }
    n->children.clear();

    if (!intermediate)
    {
        // FIXME: if, for whatever reason, this
        // is not optimized out, then a linker error will occur.
        n->_ast.m_initialize_ast = initialize_ast<Rule>;
    }
    else
    {
        n->_ast.m_initialize_ast = nullptr;
    }
}

#ifdef PARSE_GREYSPACE
// applied to every `gs` node
template<typename Rule>
inline void transform_greyspace_list(std::unique_ptr< ast_node >& n)
{
    n->m_node_type = ast_node::NODE_TYPE_GREYSPACE_LIST;
}

template<typename Rule>
void initialize_decor(const ast_node* n, ogm_decor& decor);

// applied to every greyspace node (other than `gs` itself)
template<typename Rule>
inline void transform_greyspace(std::unique_ptr< ast_node >& n)
{
    n->m_node_type = ast_node::NODE_TYPE_GREYSPACE_INDIVIDUAL;
    n->_greyspace.m_initialize_decor = initialize_decor<Rule>;
}
#endif

// removes children which overlap with their post-siblings, meaning they shouldn't be in the AST
// because they do not 'own' their full content. Example: in seq<at<A>, A>, the first A should be
// removed.
// Note: PEGTL supposedly should not be generating AST nodes for at<> at all, but it does, so
// we need this in order to remove them.
// OPTIMIZE: these nodes shouldn't even be produced in the first place -- they don't consume!
void transform_remove_unordered_children( std::unique_ptr< ast_node >& n )
{
    for (size_t i = 0; i + 1< n->children.size(); ++i)
    {
        auto& c1 = n->children.at(i);
        auto& c2 = n->children.at(i + 1);
        if (c1->has_content() && c2->has_content() && c1->end_.data > c2->begin_.data)
        {
            c1.release();
        }
    }
}

namespace peg
{

using namespace pegtl;

template< typename Rule > struct ast_selector : std::false_type {};

// select rules which should have their own ast (or greyspace) nodes.
#define _select(rule, intermediate) template<> struct ast_selector< rule > : std::true_type {                    \
        static void transform( std::unique_ptr< ast_node >& n ) { transform_remove_unordered_children(n); transform_selected<rule, intermediate>(n); }   \
    };

// selected nodes should appear in the final ast
#define select(rule) _select(rule, false)
// intermediate nodes should be parsed as PEGTL ast nodes, but will not
// appear as ogm ast nodes.
#define select_intermediate(rule) _select(rule, true)

#ifdef PARSE_GREYSPACE
    #define select_greyspace_list(rule) template<> struct ast_selector< rule > : std::true_type {       \
        static void transform( std::unique_ptr< ast_node >& n ) { transform_remove_unordered_children(n); transform_greyspace_list<rule>(n); }  \
    }

    #define select_greyspace(rule) template<> struct ast_selector< rule > : std::true_type {            \
        static void transform( std::unique_ptr< ast_node >& n ) { transform_remove_unordered_children(n); transform_greyspace<rule>(n); }       \
    }

    #define select_greyspace_component(rule) template<> struct ast_selector< rule > : std::true_type {  \
        static void transform( std::unique_ptr< ast_node >& n ) { transform_remove_unordered_children(n); n->m_node_type = ast_node::NODE_TYPE_GREYSPACE_COMPONENT }           \
    }
#else
    #define select_greyspace_list(rule)
    #define select_greyspace(rule)
    #define select_greyspace_component(rule)
#endif

template<typename prefix>
struct prefix_parse_subtype : seq<prefix>{};
template<> struct ast_selector< prefix_parse_subtype > : std::true_type {}

template<typename prefix, typename Asuffix, typename A>
struct prefix_parse : seq<
    prefix_parse_subtype<prefix>,
    opt<prefix_parse_subtype<A>>
>{};

template<> struct ast_selector< prefix_parse_subtype > : std::true_type
{
    // TODO: magic
}

#include "peg/peg.inc"

#undef _select
#undef select

} // namespace peg

#ifdef PARSE_GREYSPACE
// specialization to describe how to initialize an ogm_ast_decor_t
// for each of the greyspace elements
template<>
void initialize_decor<peg::ws_eol>(const ast_node* n, ogm_decor& decor)
{
    decor.m_type = ogm_ast_decor_t_eol;
    decor.m_count = n->children.size();
}

template<>
void initialize_decor<peg::comment_sl>(const ast_node& n, ogm_decor& decor)
{
    decor.m_type = ogm_ast_decor_t_comment_sl;
    decor.m_content = dup_mem_to_str(n.begin(), n.end());
}
#endif

} // namespace ogm