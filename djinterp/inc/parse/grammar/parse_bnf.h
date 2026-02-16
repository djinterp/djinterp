/******************************************************************************
* djinterp [parse]                                                 parse_bnf.h
*
*   Simple BNF-style grammar parser. Supports:
*     - %start Nonterminal ;
*     - Nonterminal ::= alt ('|' alt)* ;
*     - Alternative = sequence of IDENT and 'string' symbols (or empty)
*
* 
* path:      \inc\parse\grammar\parse_bnf.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.09
******************************************************************************/

#ifndef DJINTERP_PARSE_GRAMMAR_BNF_
#define DJINTERP_PARSE_GRAMMAR_BNF_ 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "..\..\..\inc\djinterp.h"
#include "..\..\..\inc\dmemory.h"
#include "..\..\..\inc\dstring.h"
#include "..\..\..\inc\string_fn.h"
#include ".\parse_bnf_common.h"


int  d_parse_bnf_parser_parse_symbol(struct d_parse_parser* _parser);
void d_parse_bnf_parser_parse_alternative(struct d_parse_parser* _parser,
	                                      int                    _lhs_index);
void d_parse_bnf_parser_parse_rhs(struct d_parse_parser* _parser,
	                              int                    _lhs_index);
void d_parse_bnf_parser_parse_rule(struct d_parse_parser* _parser);
void d_parse_bnf_parser_parse_start(struct d_parse_parser* _parser);

void d_parse_grammar_from_bnf_with_config(struct d_parse_grammar*          _grammar, 
	                                      const char*                      _source, 
	                                      const struct d_parse_bnf_config* _config);
void d_parse_grammar_from_bnf(struct d_parse_grammar* _grammar,
	                          const char*             _source);


#endif  // DJINTERP_PARSE_GRAMMAR_BNF_
