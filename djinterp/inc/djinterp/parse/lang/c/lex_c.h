/*******************************************************************************
* djinterp [parse]                                                       lex_c.h
*
* The C dialect: a descriptor, and the convenience that builds a scanner
* from it.
*   There is no C scanner.  There is the engine in lex_scan.h and the tables
* below, which are generated from token_shared.def and token_c.def, so the
* whole of this module's behaviour is a consequence of those two lists plus a
* feature mask per standard level.
*   A level is requested rather than a dialect chosen, because the tables
* carry the year each spelling arrived: scanning C99 and C23 is one table read
* with a different integer.  `restrict` is an identifier at C89 and a keyword
* at C99 for that reason and no other.
*
*
* path:      /inc/djinterp/parse/lang/c/lex_c.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATIONS
    ----------
    1.  Descriptors
    2.  Scanner construction
*/

#ifndef DJINTERP_PARSE_LANG_C_LEX_C_H
#define DJINTERP_PARSE_LANG_C_LEX_C_H 1

// djinterp
#include "../../lex/lex_dialect.h"  // d_lex_dialect
#include "../../lex/lex_scan.h"     // d_lexer
#include "../../source/source_reader.h"


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Descriptors
//------------------------------------------------------------------------------
// d_lex_c_dialect returns the descriptor for one standard level, which is one
// of the D_LEX_C* constants in lex_token.h.  An unrecognized level is answered
// with the newest supported, rather than with NULL, because a scanner that
// refuses to start is a worse diagnostic than one that scans a later C than
// was asked for and reports the keyword it found.
const struct d_lex_dialect*  d_lex_c_dialect(unsigned _level);
unsigned                     d_lex_c_default_level(void);

// 1.2    Scanner construction
//------------------------------------------------------------------------------
/**
 * @brief Creates a scanner that reads a source as C.
 *
 * @param[in] _source   the source to scan; borrowed.
 * @param[in] _level    a `D_LEX_C*` level; zero selects the newest.
 * @param[in] _options  `D_LEX_OPT_*` bits.
 * @post The scanner is released with d_lex_destroy.
 * @return the scanner, or `NULL` on failure.
 */
struct d_lexer*              d_lex_c_create(const struct d_source* _source,
                                            unsigned               _level,
                                            unsigned               _options);


#endif  // DJINTERP_PARSE_LANG_C_LEX_C_H
