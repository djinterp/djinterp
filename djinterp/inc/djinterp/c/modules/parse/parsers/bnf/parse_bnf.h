/*******************************************************************************
* djinterp [c/parse]                                                 parse_bnf.h
*
*   Simple BNF-style grammar parser. Supports:
*     - %start Nonterminal ;
*     - Nonterminal ::= alt ('|' alt)* ;
*     - Alternative = sequence of IDENT and 'string' symbols (or empty)
*
*
* path:      /inc/djinterp/c/modules/parse/parsers/bnf/parse_bnf.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.09
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_C_PARSE_PARSERS_BNF_PARSE_BNF_H
#define DJINTERP_C_PARSE_PARSERS_BNF_PARSE_BNF_H 1

// std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// djinterp
#include "../../../../djinterp.h"
#include "../../../../memory/dmemory.h"
#include "../../../../dstring.h"
#include "../../../../string_fn.h"
#include "parse_bnf_common.h"


int d_parse_bnf_parser_parse_symbol(
    struct dj_parse_parser* _parser
);
void d_parse_bnf_parser_parse_alternative(
    struct dj_parse_parser* _parser,
    int                     _lhs_index
);
void d_parse_bnf_parser_parse_rhs(
    struct dj_parse_parser* _parser,
    int                     _lhs_index
);
void d_parse_bnf_parser_parse_rule(
    struct dj_parse_parser* _parser
);
void d_parse_bnf_parser_parse_start(
    struct dj_parse_parser* _parser
);

void d_parse_grammar_from_bnf_with_config(
    struct dj_parse_grammar*          _grammar,
    const char*                       _source,
    const struct dj_parse_bnf_config* _config
);
void d_parse_grammar_from_bnf(
    struct dj_parse_grammar* _grammar,
    const char*              _source
);


#endif  // DJINTERP_C_PARSE_PARSERS_BNF_PARSE_BNF_H
