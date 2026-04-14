/******************************************************************************
* djinterp [core]                                                     c.hpp
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
* path:      /inc/cpp/lang/c.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.18
******************************************************************************/

#ifndef DJINTERP_LANG_C_
#define DJINTERP_LANG_C_ 1

#include <cstdint>
#include "../core/djinterp.hpp"
#include "./common.hpp"


NS_DJINTERP
NS_LANG


// ================================================================
//  symbol_kind  (C-only extensions, 0x0080-0x009F)
// ================================================================

// symbol_kind (continued)
//   C-only declaration kinds that have no direct C++ counterpart.
namespace symbol_kind
{
    // union_decl is technically shared, but in C++ it's usually
    // mapped to struct_decl.  In C it has distinct semantics.
    constexpr std::uint16_t union_decl     = 0x0080;
};


// ================================================================
//  C language version
// ================================================================

// c_standard
//   constants: C standard version identifiers.  Used to tag
// which standard a parsed C file conforms to.
namespace c_standard
{
    constexpr std::uint8_t c89  = 0x01;
    constexpr std::uint8_t c99  = 0x02;
    constexpr std::uint8_t c11  = 0x03;
    constexpr std::uint8_t c17  = 0x04;
    constexpr std::uint8_t c23  = 0x05;
};


NS_END  // lang
NS_END  // djinterp


#endif  // DJINTERP_LANG_C_
