/*******************************************************************************
* djinterp [parse]                                                         c.hpp
*
* C-specific language constants:
*   This header defines the symbol kinds and qualifiers that exist only
* in C (not C++).  Most C constructs are shared with C++ and live in
* common.hpp; this file covers the few C-only distinctions.
*
* Contents:
*   - symbol_kind extensions for C-only constructs
*   - C version helpers
*
* Dependencies:
*   Includes common.hpp (shared symbol kinds and qualifier flags).
*
*
* path:      /inc/djinterp/parse/parsers/c/c.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSERS_C_C_HPP
#define DJINTERP_PARSE_PARSERS_C_C_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../../command/command.hpp"  // symbol_kind, which this header extends
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint16_t, uint8_t


NS_DJINTERP

// ================================================================
//  symbol_kind  (C-only extensions, 0x0080-0x009F)
// ================================================================

// symbol_kind (continued)
//   C-only declaration kinds that have no direct C++ counterpart.
namespace symbol_kind
{
    // union_decl is technically shared, but in C++ it's usually
    // mapped to struct_decl.  In C it has distinct semantics.
    constexpr re_std::uint16_t union_decl     = 0x0080;
};


// ================================================================
//  C language version
// ================================================================

// c_standard
//   constants: C standard version identifiers.  Used to tag
// which standard a parsed C file conforms to.
namespace c_standard
{
    constexpr re_std::uint8_t c89  = 0x01;
    constexpr re_std::uint8_t c99  = 0x02;
    constexpr re_std::uint8_t c11  = 0x03;
    constexpr re_std::uint8_t c17  = 0x04;
    constexpr re_std::uint8_t c23  = 0x05;
};


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_PARSERS_C_C_HPP
