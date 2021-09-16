/*
    Included in grammar.hpp
    
    This files defines the ogm parsing expression grammar. https://en.wikipedia.org/wiki/Parsing_expression_grammar
    
    PEGTL rules reference: https://github.com/taocpp/PEGTL/blob/main/doc/Rule-Reference.md
    
    In addition to standard PEGTL rules, there are also some concepts specific here.
    - "greyspace" refers to both comments and whitespace. For the beautifier, we need to parse comments instead of ignoring them.
    - "greyspace list" is one slot in the AST anywhere that greyspace can go. For example, `@ var @ x @ = @ 3 @; @` has six greyspace slots; '@' is a greyspace list.
    
    Two syntax trees are produced. One (1) by PEGTL, comprising `ast_node` structs;
    and then the PEGTL syntax tree is transformed into a second (2) ast, comprising `ogm_ast` structs.
    - "select" / "select_intermediate" / "select_gresypace" / "primary" / "intermediate" etc.
        defines whether a node is
        (a) primary: this node is part of both PEGTL and OGM abstract syntax tree,
        (b) intermediate: this node is only part of the PEGTL abstract syntax tree,
        (c) greyspace: this node is part of PEGTL abstract syntax tree, and exists as 'decor' in the OGM syntax tree,
        (default) folded: this node is part of neither PEGTL nor OGM tree, and exists only to help write the PEG (in this file).
    - "select" nodes *must* have an associated initializer function in `parse.cpp` in order to initialize the ogm_ast struct.
*/

// convenience macro to define keywords
#define KW(x) TAO_PEGTL_KEYWORD(x)
#define intermediate(rule) struct rule; select_intermediate(rule); struct rule
#define primary(rule) struct rule; select(rule); struct rule

// clang-format off

select_intermediate(identifier);

// greyspace (whitespace and/or comment) -------------

#ifdef PARSE_GREYSPACE
    struct space_not_eol : one<
        ' ', '\t', '\v', '\f'
    > {};

    struct ws_eol_discard : seq< eol > {};
    struct ws_eol : seq< eol > {};
    select_greyspace(ws_eol)

    // at least *min* newlines -- but we discard whitespace that leads only to eof.
    // if fewer than *min* newlines, we discard.
    // in all cases, if zero characters are ated/discarded, then fail.
    template<size_t Min>
    struct _wsplus : sor<
        // first, check if there are fewer than Min newlines...
        seq<
            star<space_not_eol>
            rep_max<
                Min,
                seq<ws_eol_discard, star<space_not_eol>>
            >,
            not_at<eol>
        >,

        seq<
            not_at<
                star<sor<ws_eol_discard, space_not_eol>>,
                eof
            >,
            plus<
                sor<ws_eol, space_not_eol>
            >
        >,
        // ... if eof was found, then just discard whitespace greedily
        plus<space>
    >{};
#else
    // when not parsing greyspace, we can discard whitespace greedily.
    struct _wsplus : plus<space>{};
#endif

struct comment_sl : seq <
    string<'/', '/'>, until<eolf, any>
>{};
select_greyspace(comment_sl);

struct comment_ml : seq <
    string<'/', '*'>,
    until<sor<eof, string<'*', '/'>>, any>,
    string<'*', '/'>
>{};
select_greyspace(comment_ml);

// wsplus is a mandatory (i.e. non-empty) ws
struct ws;
struct wsplus : seq<
    at<_wsplus>,
    ws
>{};
select_greyspace_list(wsplus);

struct comment : sor<
    seq<comment_sl, eolf>,
    comment_ml
>{};
struct _gsplus : plus<
    sor<
        wsplus,
        comment
    >
>{};

struct ws : opt<_wsplus> {};
select_greyspace_list(ws);

struct gs : opt<_gsplus> {};
select_greyspace_list(gs);

// gsplus is a mandatory (i.e. non-empty) gs
struct gsplus : seq<
    at<_gsplus>,
    gs
>{};
select_greyspace_list(gsplus);

// AST nodes ---------------------------------------------

// e.g. '++'
intermediate (op_l1) : sor<
    string<'+', '+'>,
    string<'-', '-'>
> {};

intermediate (op_assign) : sor<
    string<'='>,
    string<'+', '='>,
    string<'-', '='>,
    string<'*', '='>,
    string<'/', '='>,
    string<'&', '='>,
    string<'|', '='>,
    string<'^', '='>,
    string<'>', '>', '='>,
    string<'<', '<', '='>,
    string<'%', '='>
> {};

// expressions --------------------------------------------

struct rvalue;
struct lvalue;

primary (ex_lit_number) : sor<
    seq<digit>
> {};

struct ex_literal : sor<
    ex_lit_number
>{};

primary (ex_identifier): seq<
    identifier
>{};

primary (ex_function_call): seq<
    lvalue,
    gs,
    one< '<' >,
    opt<
        list_tail<rvalue, one<','>, gsplus>
    >,
    one< '>' >
>{};

struct lvalue : sor<
    ex_identifier
>{};

struct rvalue : sor<
    ex_ternary,
    ex_literal,
    ex_function_call,
    lvalue
    // TODO
>{};

// statements ----------------------------------------------

primary (st_function_call): seq<
    ex_function_call
>{};

intermediate (st_var_type) : sor<
    KW("var"), KW("globalvar")
>{};
intermediate (st_var_type_opt) : opt<st_var_type>{};

primary (st_var_decl_empty_expression) : seq<>{};

struct st_var_decl : seq<
    identifier,
    gs,
    sor<
        seq<
            one<'='>,
            gs,
            rvalue
        >,
        st_var_decl_empty_expression
    >
>{};

// e.g. '++x'
primary (st_opl1_pre) : seq<
    op_l1,
    gs,
    lvalue
>{};

// e.g. 'x++'
primary (st_opl1_post) : seq<
    lvalue,
    gs,
    op_l1
>{};

// e.g. `x = 5`
primary (st_assign) : seq<
    lvalue,
    gs,
    op_assign,
    gs,
    rvalue
>{};

// e.g. `var a = 5, c`
primary (st_var) : seq<
    // note: one additional comma is valid and supported.
    // see ptest_var_comma.gml
    at<st_var_type>,
    sor<
        list_tail<
            seq<
                st_var_type_opt,
                gs,
                st_var_decl
            >, one<','>, gsplus
        >,
        seq<
            st_var_type,
            opt<seq<
                gs,
                one<','>
            >>
        >
    >
>{};

struct statement_bare : sor<
    // start with the easiest statements to falsify.
    st_var,
    st_opl1_pre,
    st_function_call,
    st_assign,
    st_opl1_post
    // TODO
>{};
struct statement : seq<statement_bare, gs, opt<one<';'>>, gs>{};

primary (body_bare) : seq<gs, star<statement>>{};
primary (body_braced) : seq<one<'{'>, body_bare, one<'}'>>{};

// body and then possibly some defines
primary (top_level) : seq<
    body_bare,
    //until< eof, defined_section >,
    eolf
>{};

struct top_level_expression : seq <
    gs, rvalue, gs
>{};

// clang-format on
#undef KW
#undef intermediate
#undef primary