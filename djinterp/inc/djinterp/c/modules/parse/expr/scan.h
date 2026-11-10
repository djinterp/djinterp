/*******************************************************************************
* djinterp [c/parse]                                                      scan.h
*
* Expression core v2's default scanner: a source (parse.h) over a text, for
* the clients that need no lexer of their own (design section 10): the
* templates, bitwisedom, vparse's notations and the tests.
*   It skips blanks and reads names, numbers, strings, and every spelling the
* signature gives an operator's token, a mixfix's parts and a list's
* separators, longest first, with the spellings the caller adds for tokens
* its hooks read (`getline`, `|`). A word that is a spelling is that token
* only as a whole name: `and` is the operator, `android` a name. A byte that
* begins nothing is a stray, which the parser reports as unexpected.
*   Math keeps its own lexer, whose numbers are decimal.h's grammar, and the
* command language its own, whose words are the shell's; both fill
* dj_parse_expr_token as this does, with their own class numbers.
*
*
* path:      /inc/djinterp/c/modules/parse/expr/scan.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  The configuration
         1.  dj_parse_expr_scan_word
         2.  dj_parse_expr_scan_config
    2.  The scanner
         1.  dj_parse_expr_scanner
2.  OPERATIONS
    ----------
    1.  Binding
*/

#ifndef DJINTERP_C_PARSE_EXPR_SCAN_H
#define DJINTERP_C_PARSE_EXPR_SCAN_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../djinterp.h"                              // framework root
#include "../../../../config/c/modules/parse/cfg_parse.h"   // D_INTERNAL_PARSE_EXPR_SCAN
#include "./parse.h"                                        // dj_parse_expr_source
#include "./signature.h"                                    // dj_parse_expr_signature
// re_std
#include "../../../../../re_std/c/cstdint/dstdint.h"  // uint32_t


#if (D_INTERNAL_PARSE_EXPR_SCAN == 1)


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    The configuration
//------------------------------------------------------------------------------
// 1.1.1
// dj_parse_expr_scan_word
//   struct: a spelling the signature does not give, and its token's class: a
// keyword or a mark that only a custom role reads.
struct dj_parse_expr_scan_word
{
    const char* spelling;
    uint32_t    cls;
};

// 1.1.2
// dj_parse_expr_scan_config
//   struct: the classes the scanner gives what it reads. A name is
// `[A-Za-z_]` or a byte from 0x80, then those and digits, so UTF-8 passes
// whole; a number is digits, then a point and digits, then an exponent
// (`e` or `E`, a sign, digits), each part only where a digit follows it; a
// string is in `"` or `'`, a backslash keeping the byte after it. A class of
// 0 reads nothing of its kind. `newline_class` 0 makes a newline a blank;
// either way the next token carries D_PARSE_EXPR_TOKEN_LINE_BEFORE.
// `stray_class` is a byte that begins nothing, and a string never closed;
// with 0 the source fails there. `words` are more spellings, `word_count` of
// them; they are matched as the signature's are, and come first where two
// spell alike.
struct dj_parse_expr_scan_config
{
    uint32_t                              name_class;
    uint32_t                              number_class;
    uint32_t                              string_class;
    uint32_t                              newline_class;
    uint32_t                              stray_class;
    const struct dj_parse_expr_scan_word* words;
    uint32_t                              word_count;
};

// 1.2    The scanner
//------------------------------------------------------------------------------
// 1.2.1
// dj_parse_expr_scanner
//   struct: a scan in progress, the source's context. It holds no storage of
// its own: the signature, the configuration's words and the text are the
// caller's, and must outlive the parse. `pending` is the scanner's own: the
// flags a newline token leaves for the token after it.
struct dj_parse_expr_scanner
{
    const struct dj_parse_expr_signature* signature;
    struct dj_parse_expr_scan_config      config;
    const char*                           text;
    uint32_t                              length;
    uint32_t                              at;
    uint32_t                              pending;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 2.1    Binding
//------------------------------------------------------------------------------
/**
 * @brief Binds a source to a scan of a text by a signature's spellings.
 *
 * Each call of the source's `next` reads one token from where the last
 * ended; the position and the mode it is told are not consulted, so a
 * language whose tokens differ by either brings its own source. Every token
 * carries D_PARSE_EXPR_TOKEN_SPACE_BEFORE after a blank and
 * D_PARSE_EXPR_TOKEN_LINE_BEFORE after a newline, and a value of 0.
 *
 * @param[out] _source     the source, bound to the scanner.
 * @param[out] _scanner    the scan's state.
 * @param[in]  _signature  the signature whose spellings are read, sealed,
 *                         as the parse needs it.
 * @param[in]  _config     the classes and the further spellings, copied
 *                         (the words are not).
 * @param[in]  _text       the text; need not end with a NUL.
 * @param[in]  _length     its length in bytes, below 4 GiB.
 * @return true, or false where an argument is NULL or the text too long.
 */
bool d_parse_expr_scan(struct dj_parse_expr_source*            _source,
                       struct dj_parse_expr_scanner*           _scanner,
                       const struct dj_parse_expr_signature*   _signature,
                       const struct dj_parse_expr_scan_config* _config,
                       const char*                             _text,
                       size_t                                  _length);

D_EXTERN_C_END


#endif  // D_INTERNAL_PARSE_EXPR_SCAN


#endif  // DJINTERP_C_PARSE_EXPR_SCAN_H
