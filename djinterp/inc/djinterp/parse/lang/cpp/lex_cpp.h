/*******************************************************************************
* djinterp [parse]                                                     lex_cpp.h
*
* The C++ dialect: a descriptor, and the convenience that builds a scanner
* from it.
*   This module is the C module with a second list file and four more
* table rows.  There is no C++ scanner and no line of scanning logic here;
* the engine in lex_scan.h is the same object file for both languages, which
* is the whole of what the shared token list was for.
*   Two C++ facts do not fit a table and are carried as feature bits.  A
* raw string's closing sequence is built from text read at its opening, so no
* row can describe it.  An alternative token is a punctuator spelled as a
* word, so its rows live in the keyword table with a punctuator's kind and a
* consumer counting `&&` finds `and` without knowing the spelling exists.
*
*
* path:      /inc/djinterp/parse/lang/cpp/lex_cpp.h
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

#ifndef DJINTERP_PARSE_LANG_CPP_LEX_CPP_H
#define DJINTERP_PARSE_LANG_CPP_LEX_CPP_H 1

// djinterp
#include "../../lex/lex_dialect.h"  // d_lex_dialect
#include "../../lex/lex_scan.h"     // d_lexer
#include "../../source/source_reader.h"


//==============================================================================
// 1.  OPERATIONS
//==============================================================================


// 1.1    Descriptors
//------------------------------------------------------------------------------
// d_lex_cpp_dialect returns the descriptor for one standard level, which is
// one of the D_LEX_CPP* constants in lex_token.h.  An unrecognized level is
// answered with the newest supported rather than with NULL, for the reason
// given in lex_c.h.
const struct d_lex_dialect*  d_lex_cpp_dialect(unsigned _level);
unsigned                     d_lex_cpp_default_level(void);

// 1.2    Scanner construction
//------------------------------------------------------------------------------
/**
 * @brief Creates a scanner that reads a source as C++.
 *
 * @param[in] _source   the source to scan; borrowed.
 * @param[in] _level    a `D_LEX_CPP*` level; zero selects the newest.
 * @param[in] _options  `D_LEX_OPT_*` bits.
 * @post The scanner is released with d_lex_destroy.
 * @return the scanner, or `NULL` on failure.
 */
struct d_lexer*              d_lex_cpp_create(const struct d_source* _source,
                                              unsigned               _level,
                                              unsigned               _options);


#endif  // DJINTERP_PARSE_LANG_CPP_LEX_CPP_H
