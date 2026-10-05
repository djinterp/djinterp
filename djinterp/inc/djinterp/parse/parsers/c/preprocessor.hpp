/*******************************************************************************
* djinterp [parse]                                              preprocessor.hpp
*
* Preprocessor directive classification:
*   This header defines the symbol kinds for preprocessor directives,
* which are shared identically between C and C++.  Both languages use
* the same preprocessor; the directive set (#define, #include, #if,
* #pragma, etc.) is the same in both.
*
*   C++17 added __has_include and C23 adopted it, but the directives
* themselves are unchanged.  This header covers the directive kinds
* only — the preprocessor's actual parsing is a separate concern.
*
* Contents:
*   - pp_directive      preprocessor directive classification
*
* Note on constants vs enum class:
*   These are mutually exclusive categories (a directive is one kind).
* enum class would provide type safety but requires static_cast for
* switch/comparison/indexing.  We use namespace constexpr to match
* the codebase convention and allow free implicit conversions.
*
*
* path:      /inc/djinterp/parse/parsers/c/preprocessor.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSERS_C_PREPROCESSOR_HPP
#define DJINTERP_PARSE_PARSERS_C_PREPROCESSOR_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint16_t


NS_DJINTERP

// ================================================================
//  pp_directive
// ================================================================

// pp_directive
//   constants: classifies a preprocessor directive.  These are
// sequential IDs (not bitfields) because a directive is exactly
// one kind.
//
//   Values start at 0x0200 to avoid collision with symbol_kind
// (0x0000-0x00FF) and def_symbol_kind (0x0100-0x01FF), so all
// three can coexist in a single uint16_t kind field.
namespace pp_directive
{
    constexpr re_std::uint16_t unknown       = 0x0200;
    constexpr re_std::uint16_t include_      = 0x0201;
    constexpr re_std::uint16_t define_       = 0x0202;
    constexpr re_std::uint16_t undef_        = 0x0203;
    constexpr re_std::uint16_t ifdef_        = 0x0204;
    constexpr re_std::uint16_t ifndef_       = 0x0205;
    constexpr re_std::uint16_t if_           = 0x0206;
    constexpr re_std::uint16_t elif_         = 0x0207;
    constexpr re_std::uint16_t else_         = 0x0208;
    constexpr re_std::uint16_t endif_        = 0x0209;
    constexpr re_std::uint16_t pragma_       = 0x020A;
    constexpr re_std::uint16_t error_        = 0x020B;
    constexpr re_std::uint16_t warning_      = 0x020C;
    constexpr re_std::uint16_t line_         = 0x020D;
    constexpr re_std::uint16_t has_include   = 0x020E;
};


// is_pp_directive
//   returns true if a uint16_t kind value falls in the
// preprocessor directive range.
D_CONSTEXPR_INLINE_VAR bool
is_pp_directive
(
    re_std::uint16_t _kind
)
{
    return ( (_kind >= pp_directive::unknown) &&
             (_kind <= pp_directive::has_include) );
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_PARSERS_C_PREPROCESSOR_HPP
