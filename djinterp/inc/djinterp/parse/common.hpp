/******************************************************************************
* djinterp [core]                                               common.hpp
*
* Shared C/C++ language constants:
*   This header defines the symbol kinds and qualifier flags that are
* common to both C and C++.  It is the dependency root for c.hpp and
* cpp.hpp — neither language-specific header exists without it.
*
* Design — two kinds of constants, two different strategies:
*
*   symbol_kind:  Sequential IDs (not bitfields).  A declaration is
*     exactly one kind.  Values fit in uint16_t.  Ranges are reserved:
*       0x0000-0x00FF  symbol_kind      (this header + c.hpp/cpp.hpp)
*       0x0100-0x01FF  def_symbol_kind  (definition-internal, cpp.hpp)
*       0x0200-0x02FF  pp_directive     (preprocessor.hpp)
*
*   qualifier:  Combinable bitfield flags.  Values are aligned to the
*     bit positions defined in the type_info system (type_info_common.h,
*     type_info_c.h, type_info_cpp.h) so that a symbol's qualifier field
*     can be used directly with the D_TYPE_IS_* / D_TYPE_SET_* macros —
*     no translation layer.  This means the qualifier field must be
*     uint64_t, since the C++ modifier bits occupy positions 32-47.
*
* Why namespace constexpr, not enum class:
*   For symbol_kind (mutually exclusive), enum class would give type
*   safety at the cost of static_cast on every switch/comparison.
*   For qualifier (bitfield), enum class is actively harmful — every
*   | & ~ requires an overload or cast.  We use namespace constexpr
*   throughout for consistency with the codebase and free implicit
*   conversions.
*
*
* path:      /inc/cpp/lang/common.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.18
******************************************************************************/

#ifndef DJINTERP_LANG_COMMON_
#define DJINTERP_LANG_COMMON_ 1

#include <cstdint>
#include "../core/djinterp.hpp"
#include "./preprocessor.hpp"


NS_DJINTERP
NS_LANG


// ================================================================
//  symbol_kind  (shared C/C++)
// ================================================================

// symbol_kind
//   constants: classifies the kind of code declaration.  These
// are sequential IDs, not bitfields.  A node is exactly one kind.
//
//   Values below 0x0080 are shared between C and C++.
// C-only values occupy 0x0080-0x009F (see c.hpp).
// C++-only values occupy 0x00A0-0x00FF (see cpp.hpp).
namespace symbol_kind
{
    constexpr std::uint16_t unknown            = 0x0000;

    // aggregate types (shared)
    constexpr std::uint16_t struct_decl        = 0x0001;
    constexpr std::uint16_t enum_decl          = 0x0002;
    constexpr std::uint16_t enum_constant      = 0x0003;

    // functions (shared)
    constexpr std::uint16_t function_decl      = 0x0010;

    // data (shared)
    constexpr std::uint16_t field_decl         = 0x0020;
    constexpr std::uint16_t variable_decl      = 0x0021;
    constexpr std::uint16_t parameter_decl     = 0x0022;

    // type aliasing (shared)
    constexpr std::uint16_t typedef_decl       = 0x0030;

    // preprocessor (shared — included via preprocessor.hpp, but
    // we alias the most common ones here for convenience)
    constexpr std::uint16_t macro_def          = 0x0050;
    constexpr std::uint16_t include_directive  = 0x0060;
};

// is_symbol_kind
//   returns true if a uint16_t kind value falls in the
// symbol_kind range (0x0000-0x00FF).
inline constexpr bool
is_symbol_kind
(
    std::uint16_t _kind
)
{
    return (_kind <= 0x00FF);
}


// ================================================================
//  qualifier  (shared C/C++)
// ================================================================

// qualifier
//   constants: combinable bitfield flags for type qualifiers,
// storage class specifiers, and language-specific modifiers.
//
//   BIT POSITIONS MATCH type_info.h EXACTLY:
//     bits  0-15:  base type_info16 (kind, subtype, signed, cv)
//     bits 16-23:  pointer depth
//     bits 24-31:  C storage class (type_info_c.h)
//     bits 32-47:  C++ modifiers (type_info_cpp.h)
//     bits 48-63:  user type ID (when CUSTOM bit set)
//
//   Only the qualifier/modifier bits are defined here.  The type
// encoding bits (kind, subtype, pointer depth) are the province
// of the type_info system; we don't redefine them.
namespace qualifier
{
    // --------------------------------------------------------
    //  CV qualifiers (bits 11-12, from type_info_common.h)
    // --------------------------------------------------------
    constexpr std::uint64_t const_       = (1ULL << 11);
    constexpr std::uint64_t volatile_    = (1ULL << 12);

    // --------------------------------------------------------
    //  C storage class (bits 24-31, from type_info_c.h)
    // --------------------------------------------------------
    constexpr std::uint64_t restrict_    = (1ULL << 24);
    constexpr std::uint64_t atomic_      = (1ULL << 25);
    constexpr std::uint64_t static_      = (1ULL << 26);
    constexpr std::uint64_t extern_      = (1ULL << 27);
    constexpr std::uint64_t inline_      = (1ULL << 28);
    constexpr std::uint64_t noreturn_    = (1ULL << 29);

    // --------------------------------------------------------
    //  masks
    // --------------------------------------------------------
    constexpr std::uint64_t cv_mask      = (const_ | volatile_);

    constexpr std::uint64_t c_storage_mask =
        ( restrict_ | atomic_ | static_ | extern_ |
          inline_   | noreturn_ );

    constexpr std::uint64_t none         = 0;
};


// ================================================================
//  access_specifier  (used by C++ but defined here because the
//  uint8_t encoding doesn't overlap with qualifiers)
// ================================================================

// access_specifier
//   constants: C++ access levels.  These are a separate field
// (not part of the qualifier bitfield) because they are mutually
// exclusive — a declaration has exactly one access level.
namespace access_specifier
{
    constexpr std::uint8_t unspecified = 0x00;
    constexpr std::uint8_t public_     = 0x01;
    constexpr std::uint8_t protected_  = 0x02;
    constexpr std::uint8_t private_    = 0x03;
};


NS_END  // lang
NS_END  // djinterp


#endif  // DJINTERP_LANG_COMMON_
