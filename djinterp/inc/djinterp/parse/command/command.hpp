/******************************************************************************
* djinterp [parse]                                                  common.hpp
*
*
* path:      /inc/djinterp/parse/command/command.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.06.18
******************************************************************************/

#ifndef DJINTERP_PARSE_COMMAND_
#define DJINTERP_PARSE_COMMAND_ 1

// std
#include <cstdint>
// djinterp
#include "../../djinterp.hpp"
#include "../parsers/c/preprocessor.hpp"


NS_DJINTERP


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

    // translation unit (root of a parsed file)
    constexpr std::uint16_t translation_unit   = 0x0070;

    // base class specifier (child of a class/struct node)
    constexpr std::uint16_t base_specifier     = 0x0071;

    // template parameter (child of a template node)
    constexpr std::uint16_t template_param     = 0x0072;
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
//  symbol_kind classification predicates
// ================================================================

// is_type_kind
//   returns true if _kind represents a type declaration
// (struct, enum, class, union, typedef, type alias, template).
inline constexpr bool
is_type_kind
(
    std::uint16_t _kind
)
{
    return ( (_kind == symbol_kind::struct_decl)   ||
             (_kind == symbol_kind::enum_decl)     ||
             (_kind == symbol_kind::typedef_decl)  ||
             (_kind >= 0x0080 && _kind <= 0x00A6) );
}

// is_function_kind
//   returns true if _kind represents a callable declaration
// (function, method, constructor, destructor).
inline constexpr bool
is_function_kind
(
    std::uint16_t _kind
)
{
    return ( (_kind == symbol_kind::function_decl) ||
             (_kind >= 0x00A2 && _kind <= 0x00A4) );
}

// is_data_kind
//   returns true if _kind represents a data declaration
// (field, variable, parameter, enum constant).
inline constexpr bool
is_data_kind
(
    std::uint16_t _kind
)
{
    return ( (_kind == symbol_kind::field_decl)     ||
             (_kind == symbol_kind::variable_decl)  ||
             (_kind == symbol_kind::parameter_decl) ||
             (_kind == symbol_kind::enum_constant) );
}

// is_scope_kind
//   returns true if _kind represents a declaration that
// introduces a new scope (struct, class, namespace, enum,
// function, method, template, translation unit).
inline constexpr bool
is_scope_kind
(
    std::uint16_t _kind
)
{
    return ( (_kind == symbol_kind::struct_decl)        ||
             (_kind == symbol_kind::enum_decl)          ||
             (_kind == symbol_kind::function_decl)      ||
             (_kind == symbol_kind::translation_unit)   ||
             (_kind >= 0x00A0 && _kind <= 0x00A6) );
}


// ================================================================
//  symbol_kind to string
// ================================================================

// symbol_kind_to_string
//   returns a human-readable name for shared symbol kinds.
// C-only and C++-only kinds return nullptr here; the
// language-specific headers provide their own overloads.
inline const char*
symbol_kind_to_string
(
    std::uint16_t _kind
)
{
    switch (_kind)
    {
        case symbol_kind::unknown:           return "unknown";
        case symbol_kind::struct_decl:       return "struct";
        case symbol_kind::enum_decl:         return "enum";
        case symbol_kind::enum_constant:     return "enum_constant";
        case symbol_kind::function_decl:     return "function";
        case symbol_kind::field_decl:        return "field";
        case symbol_kind::variable_decl:     return "variable";
        case symbol_kind::parameter_decl:    return "parameter";
        case symbol_kind::typedef_decl:      return "typedef";
        case symbol_kind::macro_def:         return "macro";
        case symbol_kind::include_directive: return "include";
        case symbol_kind::translation_unit:  return "translation_unit";
        case symbol_kind::base_specifier:    return "base_specifier";
        case symbol_kind::template_param:    return "template_param";
        default:                             break;
    }

    return nullptr;
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
//  qualifier predicates (shared)
// ================================================================

// has_qualifier
//   returns true if _quals contains the specified flag(s).
inline constexpr bool
has_qualifier
(
    std::uint64_t _quals,
    std::uint64_t _flag
)
{
    return ((_quals & _flag) != 0);
}

// is_const
//   returns true if the const qualifier is present.
inline constexpr bool
is_const
(
    std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::const_);
}

// is_volatile
//   returns true if the volatile qualifier is present.
inline constexpr bool
is_volatile
(
    std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::volatile_);
}

// is_static
//   returns true if the static storage class is present.
inline constexpr bool
is_static
(
    std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::static_);
}

// is_extern
//   returns true if the extern storage class is present.
inline constexpr bool
is_extern
(
    std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::extern_);
}

// is_inline
//   returns true if the inline specifier is present.
inline constexpr bool
is_inline
(
    std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::inline_);
}


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

// access_specifier_to_string
//   returns a human-readable name for the access level.
inline const char*
access_specifier_to_string
(
    std::uint8_t _access
)
{
    switch (_access)
    {
        case access_specifier::public_:     return "public";
        case access_specifier::protected_:  return "protected";
        case access_specifier::private_:    return "private";
        case access_specifier::unspecified: return "unspecified";
        default:                            break;
    }

    return "unspecified";
}


NS_END  // djinterp


#endif  // DJINTERP_PARSE_COMMAND_