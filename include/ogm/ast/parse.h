#ifndef LIBOGMLAST_PARSE_H
#define LIBOGMLAST_PARSE_H

typedef enum ogm_ast_type
{
    //! expression
    ogm_ast_t_exp,
    //! imperative
    ogm_ast_t_imp
} ogm_ast_type_t;

typedef int int32_t;

typedef enum ogm_ast_subtype
{
    //// expressions ////

    // literal                                      x = <4>;, s = <"hello">;
    ogm_ast_st_exp_literal_primitive,
    // identifier                                   <x> = 4;
    ogm_ast_st_exp_identifier,
    // accessor                                     <arr[3]> = 4;, x = <map[? 2]>;
    ogm_ast_st_exp_accessor,
    // array literal                                x = <[4, 2, -6]>;
    ogm_ast_st_exp_literal_array,
    // parentheses                                  b = <3 * (a + 2)>
    ogm_ast_st_exp_paren,
    // arithmetic                                   x = <3 + y>;, <z != 2>
    ogm_ast_st_exp_arithmetic,
    // function                                     y = <fn()>;
    ogm_ast_st_exp_fn,
    // ternary if                                   z = <a ? b : c>;
    ogm_ast_st_exp_ternary_if,
    // possessive                                   <id.x> = 0;
    ogm_ast_st_exp_possessive,
    // global                                       <global.x> = 0;
    ogm_ast_st_exp_global,

    //// statements ////

    // body list                                   (all #defines in a ascript)
    ogm_ast_st_imp_body_list,
    // empty statement                              <;>
    ogm_ast_st_imp_empty,
    // body                                         <{;;;}>
    ogm_ast_st_imp_body,
    // assignment                                   <x = 4;>, <++i;>
    ogm_ast_st_imp_assignment,
    // (local) var statement  .                     <var x, y = 4;>
    ogm_ast_st_imp_var,
    // if statement                                 <if (x) { } else { }>
    ogm_ast_st_imp_if,
    // for loop                                     <for (;;) { }>
    ogm_ast_st_imp_for,
    // all other loops                              <with (x) { }>, <do { ...} until ()>
    ogm_ast_st_imp_loop,
    // switch                                       <switch (x) { case 1: default: }>
    ogm_ast_st_imp_switch,
    // control                                      <break;>, <return;>
    ogm_ast_st_imp_control,
    // enum                                         <enum letters {a, b, c}>
    ogm_ast_st_imp_enum
} ogm_ast_subtype_t;

// specification of the above type (optional)
typedef enum ogm_ast_spec
{
    // not all subtypes have specifications
    ogm_ast_spec_none,

    //// loop types ////
    ogm_ast_spec_loop_with,
    ogm_ast_spec_loop_while,
    ogm_ast_spec_loop_do_until,
    ogm_ast_spec_loop_repeat,

    //// control flow keywords ////
    ogm_ast_spec_control_continue,
    ogm_ast_spec_control_break,
    ogm_ast_spec_control_exit,
    ogm_ast_spec_control_return,

    //// accessor types ////

    // basic array usage                            a[0]
    ogm_ast_spec_acc_none,
    // in-place array access                        a[@ 0]
    ogm_ast_spec_acc_array,
    //                                              a[? 0]
    ogm_ast_spec_acc_map,
    //                                              a[# 0]
    ogm_ast_spec_acc_grid,
    //                                              a[| 0]
    ogm_ast_spec_acc_list,

    // possessive:
    ogm_ast_spec_possessive,

    // arithmetic operators
    ogm_ast_spec_op_plus,
    ogm_ast_spec_op_minus,
    ogm_ast_spec_op_times,
    ogm_ast_spec_op_divide,
    ogm_ast_spec_op_integer_division_kw,
    ogm_ast_spec_op_mod_kw,
    ogm_ast_spec_op_mod,

    ogm_ast_spec_op_leftshift,
    ogm_ast_spec_op_rightshift,

    // comparisons
    ogm_ast_spec_op_eqeq,
    ogm_ast_spec_op_neq,
    ogm_ast_spec_op_lt,
    ogm_ast_spec_op_lte,
    ogm_ast_spec_op_gt,
    ogm_ast_spec_op_gte,
    ogm_ast_spec_op_ltgt,

    // (binary) boolean operators
    ogm_ast_spec_op_boolean_and,
    ogm_ast_spec_op_boolean_and_kw,
    ogm_ast_spec_op_boolean_or,
    ogm_ast_spec_op_boolean_or_kw,
    ogm_ast_spec_op_boolean_xor,
    ogm_ast_spec_op_boolean_xor_kw,

    // bitwise operators
    ogm_ast_spec_op_bitwise_and,
    ogm_ast_spec_op_bitwise_or,
    ogm_ast_spec_op_bitwise_xor,

    // unary operators
    ogm_ast_spec_opun_boolean_not,
    ogm_ast_spec_opun_boolean_not_kw,
    ogm_ast_spec_opun_bitwise_not,

    // assignments
    // x = 4
    ogm_ast_spec_op_eq,
    ogm_ast_spec_op_pluseq,
    ogm_ast_spec_op_minuseq,
    ogm_ast_spec_op_timeseq,
    ogm_ast_spec_op_divideeq,
    ogm_ast_spec_op_andeq,
    ogm_ast_spec_op_oreq,
    ogm_ast_spec_op_xoreq,
    ogm_ast_spec_op_leftshifteq,
    ogm_ast_spec_op_rightshifteq,
    ogm_ast_spec_op_modeq,
    ogm_ast_spec_op_unary_pre_plusplus,
    ogm_ast_spec_op_unary_post_plusplus,
    ogm_ast_spec_op_unary_pre_minusminus,
    ogm_ast_spec_op_unary_post_minusminus,

    //// body types ////
    // this is a brace-surrounded body
    ogm_ast_spec_body_braced,
    // this body has no braces (just a list of statements)
    ogm_ast_spec_body_braceless,
    ogm_ast_spec_count
} ogm_ast_spec_t;

typedef enum ogm_ast_decor_type
{
    ogm_ast_decor_t_whitespace,
    ogm_ast_decor_t_comment_sl,
    ogm_ast_decor_t_comment_ml,
    ogm_ast_decor_t_list,
} ogm_ast_decor_type_t;

// actual decor (newline, comment, etc.)
typedef struct ogm_ast_decor
{
    ogm_ast_decor_type_t m_type;
    char* m_content;
} ogm_ast_decor_t;

typedef struct ogm_ast_line_column
{
  // first line is 0
  int m_line;

  // first column is 0
  int m_column;
  
  #ifdef __cplusplus
  // constructors
  ogm_ast_line_column()
  { }
  
  ogm_ast_line_column(int line)
    : m_line(line)
    , m_column(0)
  { }
  
  ogm_ast_line_column(int line, int column)
    : m_line(line)
    , m_column(column)
  { }
  
  ogm_ast_line_column(const ogm_ast_line_column&)=default;
  #endif
} ogm_ast_line_column_t;

typedef struct ogm_ast
{
    ogm_ast_type_t m_type;
    ogm_ast_subtype_t m_subtype;

    union
    {
        void* m_payload;
        ogm_ast_spec_t m_spec;
    };

    // location
    ogm_ast_line_column_t m_start;
    ogm_ast_line_column_t m_end;
    void* m_file;

    // subtrees
    int32_t m_sub_count;
    struct ogm_ast* m_sub;

    // payload lists for this decor
    int32_t m_decor_list_length;
    int32_t* m_decor_count;
    ogm_ast_decor_t** m_decor;
} ogm_ast_t;

// var or globalvar or enum declaration
typedef struct ogm_ast_declaration
{
    union
    {
        char* m_type; // for enums
        char** m_types; // for vars
    };
    char** m_identifier;
    int32_t m_identifier_count;
} ogm_ast_declaration_t;


typedef enum ogm_ast_literal_primitive_type
{
    ogm_ast_literal_t_number,
    ogm_ast_literal_t_string
} ogm_ast_literal_primitive_type_t;

// payload value of a literal primitive
typedef struct ogm_ast_literal_primitive
{
    ogm_ast_literal_primitive_type_t m_type;

    char* m_value;
} ogm_ast_literal_primitive_t;

#ifdef __cplusplus
extern "C"
{
#endif

// string associated with spec
extern const char* ogm_ast_spec_string[];
extern const char* ogm_ast_subtype_string[];

// ignore whitespace and comments (recommended for compiling)
const int ogm_ast_parse_flag_no_decorations = 1;

typedef struct parse_args
{
    const char* m_code;
    const char* m_source_file;
    ogm_ast_line_column_t m_first_line_column;
    
    #ifdef __cplusplus
    // constructors
    parse_args(const char* code)
        : m_code(code)
        , m_source_file("")
        , m_first_line_column(0, 0)
    { }
    
    parse_args(const char* code, const char* file, ogm_ast_line_column_t lc={0, 0})
        : m_code(code)
        , m_source_file(file)
        , m_first_line_column(lc)
    { }
    #endif
} parse_args_t;

// parse AST.
ogm_ast* ogm_ast_parse(
    parse_args_t,
    int flags

    #ifdef __cplusplus
    =0
    #endif
);

ogm_ast* ogm_ast_parse_expression(
    parse_args_t,
    int flags

    #ifdef __cplusplus
    =0
    #endif
);

// free AST
void ogm_ast_free(
    ogm_ast_t*
);

void ogm_ast_copy_to(
    ogm_ast_t* dst,
    const ogm_ast_t* src
);

ogm_ast_t* ogm_ast_copy(
    const ogm_ast_t* src
);

#ifdef __cplusplus
}
#endif

#endif
