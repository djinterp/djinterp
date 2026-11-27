/*******************************************************************************
* djinterp [c][parse]                                                     expr.h
*
* Expression core v2: the umbrella, every header of the C core.
*   The core parses text into a term by a signature - a language's operators
* and roles - with diagnostics at the character; folds and walks the term
* with no recursion; and writes it back as text that reads as the same term.
* It is generic: it knows no language. The design, and how math, the command
* language and awk sit on it, is
* docs/djinterp/c/modules/parse/expr/expression_core_v2.html.
*
* HOW TO USE
* ----------
*   Include it for the whole C core, or include only the header needed. Build
* and seal a signature (signature.h); parse tokens into a term (parse.h), from
* a source of one's own or the default scanner (scan.h); then fold or walk the
* term (fold.h), or write it back as text (render.h).
*
* OPTIONS
* -------
*   Each header's own: a parse's dj_parse_expr_options, a scanner's
* configuration, a fold's and a walk's callbacks, a render's options.
*
* CONFIGURATION
* -------------
*   D_CFG_PARSE_EXPR_HEAP (the allocating constructors), D_CFG_PARSE_EXPR_SCAN
* (the default scanner) and D_CFG_PARSE_EXPR_RENDER (the renderer), each on
* unless set to 0, and D_CFG_PARSE_ALL for every gate left unset; all in
* cfg_parse.h. A parse's defaults are D_CFG_PARSE_EXPR_MAX_DEPTH, _MAX_ERRORS
* and _ERROR_ORDER, and the rows of the djin `parse` (settings.h) override
* them for a run.
*
*
* path:      /inc/djinterp/c/modules/parse/expr/expr.h
* link(s):   /docs/djinterp/c/modules/parse/expr/expression_core_v2.html
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.27
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
