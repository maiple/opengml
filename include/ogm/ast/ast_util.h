#ifndef LIBOGMLAST_PARSE_UTIL_H
#define LIBOGMLAST_PARSE_UTIL_H

#include "parse.h"

#ifdef __cplusplus
extern "C"
{
#endif

// print tree
void ogm_ast_tree_print(
  const ogm_ast_t* tree
);

bool ogm_ast_tree_get_spec(
  const ogm_ast_t* tree,
  ogm_ast_spec_t* out_spec
);

bool ogm_ast_tree_get_payload_literal_primitive(
    const ogm_ast_t* tree,
    ogm_ast_literal_primitive_t** out_payload
);

bool ogm_ast_tree_get_payload_declaration(
    const ogm_ast_t* tree,
    ogm_ast_declaration_t** out_payload
);

#ifdef __cplusplus
}
#endif
#endif
