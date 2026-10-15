/*******************************************************************************
* bitwisedom [bitwisedom]                                               clex.hpp
*
* C and C++ text as tokens, read by djinterp's scanners.
*   bitwisedom reads C in three places -- a macro row's body, a header it is
* asked to import, and the environment of #defines a sheet is evaluated in --
* and reads it as a compiler would: djinterp's lexer, at the sheet's
* language and level, does the scanning. This is the bridge. djinterp's
* tokens are C records over a borrowed source; these own their spelling, so
* they outlive the scanner and can be spliced, pasted and scanned again by
* the macro expander.
*   Integer and character literals are decoded here too, by djinterp's
* literal decoder: the value, the base and the suffix, which together give a
* literal its type.
*   The scanners run in the preprocessor's view: identifiers are never
* resolved to keywords, since a directive's names and a macro's body are
* read before phase 7, and comments are reported only when asked for.
*
* path:      /inc/djinterp/tools/bitwisedom/clex.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.09
*                                                            revised: 2026.10.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TOKENS
    ------
    1.  c_class
    2.  Token flags
    3.  c_token
2.  OPERATIONS
    ----------
    1.  Scanning
    2.  Literals
*/

#ifndef BITWISEDOM_CLEX_HPP
#define BITWISEDOM_CLEX_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <vector>       // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./bits.hpp"   // u64
#include "./sheet.hpp"  // standard, data_model


namespace bitwisedom
{


//==============================================================================
// 1.  TOKENS
//==============================================================================


// 1.1    c_class
//------------------------------------------------------------------------------
// 1.1.1
// c_class
//   enum: what a token is, as the expander and the evaluator see it. A
// placemarker is the expander's own, standing for an empty argument beside
// `##`; the scanner never makes one.
enum class c_class : int
{
    identifier = 0,
    number,
    character,
    string,
    punctuator,
    other,
    comment,
    placemarker
};

// 1.2    Token flags
//------------------------------------------------------------------------------
// 1.2.1
// C_*
//   constants: bits of a token's `flags`, djinterp's own values. LINE_START
// means no token but a comment stands before it on its line; LEADING_SPACE
// that whitespace or a comment does; SPLICED that it spans a
// backslash-newline. IN_DIRECTIVE marks every token of a `#` line, the `#`
// included; DIRECTIVE_NAME the name after the `#`; MACRO_NAME the name a
// directive defines, undefines or tests.
const unsigned C_LINE_START     = 0x0001u;
const unsigned C_LEADING_SPACE  = 0x0002u;
const unsigned C_SPLICED        = 0x0004u;
const unsigned C_UNTERMINATED   = 0x0100u;
const unsigned C_IN_DIRECTIVE   = 0x0200u;
const unsigned C_DIRECTIVE_NAME = 0x0400u;
const unsigned C_MACRO_NAME     = 0x0800u;
const unsigned C_COMMENT_LINE   = 0x1000u;
const unsigned C_COMMENT_DOC    = 0x2000u;

// 1.3    c_token
//------------------------------------------------------------------------------
// 1.3.1
// c_token
//   struct: one token. `text` is its spelling with splices removed -- a
// punctuator's primary spelling, whichever way it was written. `offset` and
// `length` are the bytes it covers in the text it was read from, splices
// included; `line` and `column` (one-based, columns counting code points)
// where it starts, `end_line` and `end_column` just past where it ends.
struct c_token
{
    c_class     kind       = c_class::other;
    std::string text;
    unsigned    flags      = 0;
    std::size_t offset     = 0;
    std::size_t length     = 0;
    int         line       = 0;
    int         column     = 0;
    int         end_line   = 0;
    int         end_column = 0;

    // is
    //   whether the token is the punctuator spelled _spelling.
    D_NODISCARD bool is(const char* _spelling) const
    {
        return ( (kind == c_class::punctuator) &&
                 (text == _spelling) );
    }

    // named
    //   whether the token is the identifier _name.
    D_NODISCARD bool named(const char* _name) const
    {
        return ( (kind == c_class::identifier) &&
                 (text == _name) );
    }

    D_NODISCARD bool has(unsigned _flag) const
    {
        return ((flags & _flag) != 0);
    }
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Scanning
//------------------------------------------------------------------------------
// 2.1.1
// c_lexed
//   struct: a text's tokens, and the scanner's complaints about it, each as
// "line:column: message".
struct c_lexed
{
    std::vector<c_token>     tokens;
    std::vector<std::string> errors;
};

/**
 * @brief Scans a text as C or C++.
 *
 * @param[in] _text      the text.
 * @param[in] _level     the language and level to read it at: a C++ level
 *                       reads C++.
 * @param[in] _comments  whether comments are tokens; otherwise they are
 *                       whitespace.
 * @return every token, in order, and the errors the scanner raised. An
 *         ill-formed construct is a token of class other.
 */
D_NODISCARD c_lexed lex_c(std::string_view _text,
                          standard         _level,
                          bool             _comments);

// spelled
//   tokens written back as text: one space where the source had whitespace
// before a token, none elsewhere -- the spelling `#` gives an argument.
D_NODISCARD std::string spelled(const std::vector<c_token>& _tokens);

// 2.2    Literals
//------------------------------------------------------------------------------
// 2.2.1
// c_literal
//   struct: a decoded integer, floating or character literal. `value` is an
// integer's value, or a character literal's code unit or point (a
// multicharacter literal's, as gcc and clang compute it). `longs` is 1 for
// an `l` suffix and 2 for `ll`; `encoding` a character literal's prefix: 0
// none, 1 L, 2 u8, 3 u, 4 U.
struct c_literal
{
    bool        ok          = false;
    bool        integer     = false;
    bool        floating    = false;
    bool        character   = false;
    u64         value       = 0;
    unsigned    base        = 10;
    bool        is_unsigned = false;
    int         longs       = 0;
    bool        size        = false;   // z
    bool        bit_precise = false;   // wb
    bool        overflow    = false;   // more than 64 bits
    bool        user_suffix = false;   // C++'s user-defined literals
    bool        multichar   = false;
    int         encoding    = 0;
    int         digits      = 0;       // an integer's digits, prefix aside
    bool        upper_hex   = false;   // its hex digits include A-F
    bool        lower_hex   = false;   // ...or a-f
    bool        upper_suffix = false;  // its suffix has U or L
    std::string error;
    std::string warning;
};

/**
 * @brief Decodes a number or character literal.
 *
 * @param[in] _token  a token of class number or character; any other is
 *                    not a literal.
 * @param[in] _level  the language and level it was written at.
 * @param[in] _model  the target, for the width of `wchar_t`.
 * @return the literal, or `ok == false` and the decoder's error.
 */
D_NODISCARD c_literal decode_literal(const c_token& _token,
                                     standard       _level,
                                     data_model     _model);

// levels -- a standard's djinterp level (its year), and whether it is C++
D_NODISCARD unsigned  lex_level(standard _level);
D_NODISCARD bool      cpp_level(standard _level);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_CLEX_HPP
