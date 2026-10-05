/*******************************************************************************
* djinterp [c]                                                    dmacro_token.h
*
* The framework's token macros: pasting, stringification, expansion control
* and separators.
*   None of them is variadic, so this header is valid at every language level,
* ISO C++98 included. The framework root includes it for D_CONCAT instead of
* all of dmacro.h, whose argument counting and iteration need variadic macros;
* dmacro.h includes it in turn, so a unit that includes dmacro.h still has
* every macro here. The variadic expansion helpers D_OBSTRUCT and D_UNPACK stay
* in dmacro.h.
*
*
* path:      /inc/djinterp/c/dmacro_token.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.01
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TOKEN MACROS
    ------------
    1.  Pasting and stringification
         1.  D_CONCAT
         2.  D_STRINGIFY
         3.  D_TOSTR
    2.  Expansion control
         1.  D_EXPAND
         2.  D_EMPTY
         3.  D_DEFER
    3.  Separator tokens
         1.  D_SEPARATOR_COMMA
         2.  D_SEPARATOR_SEMICOLON
         3.  D_SEPARATOR_SPACE
*/

#ifndef DJINTERP_C_DMACRO_TOKEN_H
#define DJINTERP_C_DMACRO_TOKEN_H 1


//==============================================================================
// 1.  TOKEN MACROS
//==============================================================================


// 1.1    Pasting and stringification
//------------------------------------------------------------------------------
// 1.1.1
// D_CONCAT
//   macro: token-pastes `a` and `b` after expanding both, so either may be a
// macro (D_CONCAT(name_, __LINE__) pastes the line number, not `__LINE__`).
#define D_CONCAT(a, b)                                                        \
    D_INTERNAL_CONCAT_HELPER(a, b)

// D_INTERNAL_CONCAT_HELPER
//   macro (internal): the paste itself; the extra level is what lets
// D_CONCAT expand its arguments first.
#define D_INTERNAL_CONCAT_HELPER(a, b) a##b

// 1.1.2
// D_STRINGIFY
//   macro: turns its argument into a string literal without expanding it.
#define D_STRINGIFY(x)                                                        \
    #x

// 1.1.3
// D_TOSTR
//   macro: expands its argument once, then stringifies the result.
#define D_TOSTR(x)                                                            \
    D_STRINGIFY(x)

// 1.2    Expansion control
//------------------------------------------------------------------------------
// 1.2.1
// D_EXPAND
//   macro: forces one more macro-expansion pass over its argument.
#define D_EXPAND(x)                                                           \
    x

// 1.2.2
// D_EMPTY
//   macro: expands to nothing; the building block of deferred expansion.
#define D_EMPTY()

// 1.2.3
// D_DEFER
//   macro: defers the expansion of a function-like macro `id` by one pass.
#define D_DEFER(id)                                                           \
    id D_EMPTY()

// 1.3    Separator tokens
//------------------------------------------------------------------------------
// 1.3.1
// D_SEPARATOR_COMMA
//   macro: a comma, for macros that take their separator as an argument.
#define D_SEPARATOR_COMMA ,

// 1.3.2
// D_SEPARATOR_SEMICOLON
//   macro: a semicolon, for macros that take their separator as an argument.
#define D_SEPARATOR_SEMICOLON ;

// 1.3.3
// D_SEPARATOR_SPACE
//   macro: no token at all, for macros that take their separator as an
// argument.
#define D_SEPARATOR_SPACE


#endif  // DJINTERP_C_DMACRO_TOKEN_H
