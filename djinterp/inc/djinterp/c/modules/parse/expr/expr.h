/*******************************************************************************
* djinterp [c/parse]                                                      expr.h
*
* Expression core v2: the umbrella, every header of the C core.
*   The core parses text into a term by a signature -- a language's operators
* and roles -- with diagnostics at the character; folds and walks the term
* with no recursion; and writes it back as text that reads as the same term.
* It is generic: it knows no language. The design, and how math, the command
* language and awk sit on it, is docs/djinterp/parse/expression_core_v2.md.
*
*
* path:      /inc/djinterp/c/modules/parse/expr/expr.h
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_C_PARSE_EXPR_H
#define DJINTERP_C_PARSE_EXPR_H 1

// djinterp
#include "./term.h"       // the term
#include "./signature.h"  // the signature
#include "./parse.h"      // the parser
#include "./fold.h"       // folds and walks
#include "./render.h"     // the renderer
#include "./scan.h"       // the default scanner


#endif  // DJINTERP_C_PARSE_EXPR_H
