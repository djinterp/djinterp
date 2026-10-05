/*******************************************************************************
* djinterp [parse]                                                       cpp.hpp
*
* C++-specific language constants:
*   This header defines the symbol kinds, qualifier flags, and other
* classification constants specific to C++.  It extends common.hpp
* with C++-only declaration forms (namespaces, classes, methods,
* constructors, templates) and C++-only qualifier bits (virtual,
* constexpr, noexcept, final, override, etc.).
*
* Qualifier bit alignment:
*   The C++ qualifier flags in this header occupy bits 32-47 of a
* uint64_t, matching the bit positions defined in type_info_cpp.hpp.
* This means a symbol's qualifier field can be passed directly to
* the D_TYPE_IS_VIRTUAL / D_TYPE_SET_CONSTEXPR / etc. macros with
* no translation.  The correspondence is exact:
*
*     qualifier::lvalue_ref   = D_TYPE_LVALREF_BIT     (bit 32)
*     qualifier::rvalue_ref   = D_TYPE_RVALREF_BIT     (bit 33)
*     qualifier::mutable_     = D_TYPE_MUTABLE_BIT     (bit 34)
*     qualifier::virtual_     = D_TYPE_VIRTUAL_BIT     (bit 35)
*     qualifier::constexpr_   = D_TYPE_CONSTEXPR_BIT   (bit 36)
*     qualifier::noexcept_    = D_TYPE_NOEXCEPT_BIT    (bit 37)
*     qualifier::final_       = D_TYPE_FINAL_BIT       (bit 38)
*     qualifier::override_    = D_TYPE_OVERRIDE_BIT    (bit 39)
*     qualifier::explicit_    = D_TYPE_EXPLICIT_BIT    (bit 40)
*     qualifier::consteval_   = D_TYPE_CONSTEVAL_BIT   (bit 41)
*     qualifier::constinit_   = D_TYPE_CONSTINIT_BIT   (bit 42)
*     qualifier::template_    = D_TYPE_TEMPLATE_BIT    (bit 43)
*
* Dependencies:
*   Includes common.hpp (shared symbol kinds and qualifier flags).
*
*
* path:      /inc/djinterp/parse/parsers/cpp/cpp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSERS_CPP_CPP_HPP
#define DJINTERP_PARSE_PARSERS_CPP_CPP_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../../command/command.hpp"  // symbol_kind, which this header extends
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint16_t,
                                                   // uint64_t, uint8_t


NS_DJINTERP


// ================================================================
//  symbol_kind  (C++-only extensions, 0x00A0-0x00FF)
// ================================================================

// symbol_kind (continued)
//   C++-only declaration kinds.
namespace symbol_kind
{
    // namespaces
    constexpr re_std::uint16_t namespace_decl     = 0x00A0;

    // classes
    constexpr re_std::uint16_t class_decl         = 0x00A1;

    // methods
    constexpr re_std::uint16_t method_decl        = 0x00A2;
    constexpr re_std::uint16_t constructor_decl   = 0x00A3;
    constexpr re_std::uint16_t destructor_decl    = 0x00A4;

    // type aliasing (C++ uses 'using', distinct from C typedef)
    constexpr re_std::uint16_t type_alias_decl    = 0x00A5;

    // templates
    constexpr re_std::uint16_t template_decl      = 0x00A6;

    // concept (C++20)
    constexpr re_std::uint16_t concept_decl       = 0x00A7;

    // using-directive / using-declaration
    constexpr re_std::uint16_t using_directive     = 0x00A8;
    constexpr re_std::uint16_t using_declaration   = 0x00A9;

    // friend declaration
    constexpr re_std::uint16_t friend_decl         = 0x00AA;

    // static_assert
    constexpr re_std::uint16_t static_assert_decl  = 0x00AB;
};

// is_cpp_kind
//   returns true if _kind falls in the C++-only symbol_kind
// range (0x00A0-0x00FF).
D_CONSTEXPR_INLINE_VAR bool
is_cpp_kind
(
    re_std::uint16_t _kind
)
{
    return ( (_kind >= 0x00A0) &&
             (_kind <= 0x00FF) );
}

// is_class_like_kind
//   returns true if _kind represents a class-like declaration
// (class, struct).
D_CONSTEXPR_INLINE_VAR bool
is_class_like_kind
(
    re_std::uint16_t _kind
)
{
    return ( (_kind == symbol_kind::struct_decl) ||
             (_kind == symbol_kind::class_decl) );
}

// is_method_kind
//   returns true if _kind represents a method-like declaration
// (method, constructor, destructor).
D_CONSTEXPR_INLINE_VAR bool
is_method_kind
(
    re_std::uint16_t _kind
)
{
    return ( (_kind == symbol_kind::method_decl)      ||
             (_kind == symbol_kind::constructor_decl) ||
             (_kind == symbol_kind::destructor_decl) );
}

// is_callable_kind
//   returns true if _kind represents any callable (function
// or method-like).
D_CONSTEXPR_INLINE_VAR bool
is_callable_kind
(
    re_std::uint16_t _kind
)
{
    return ( (_kind == symbol_kind::function_decl)    ||
             (_kind == symbol_kind::method_decl)      ||
             (_kind == symbol_kind::constructor_decl) ||
             (_kind == symbol_kind::destructor_decl) );
}


// ================================================================
//  C++ symbol_kind to string
// ================================================================

// cpp_symbol_kind_to_string
//   returns a human-readable name for C++-specific symbol
// kinds.  Falls back to the shared symbol_kind_to_string
// for common kinds.
inline const char*
cpp_symbol_kind_to_string
(
    re_std::uint16_t _kind
)
{
    switch (_kind)
    {
        case symbol_kind::namespace_decl:     return "namespace";
        case symbol_kind::class_decl:         return "class";
        case symbol_kind::method_decl:        return "method";
        case symbol_kind::constructor_decl:   return "constructor";
        case symbol_kind::destructor_decl:    return "destructor";
        case symbol_kind::type_alias_decl:    return "type_alias";
        case symbol_kind::template_decl:      return "template";
        case symbol_kind::concept_decl:       return "concept";
        case symbol_kind::using_directive:    return "using_directive";
        case symbol_kind::using_declaration:  return "using_declaration";
        case symbol_kind::friend_decl:        return "friend";
        case symbol_kind::static_assert_decl: return "static_assert";
        default:                              break;
    }

    // fall back to shared
    return symbol_kind_to_string(_kind);
}


// ================================================================
//  qualifier  (C++-only extensions, bits 32-47)
// ================================================================

// qualifier (continued)
//   C++ modifier bits occupying bits 32-47.  Each value matches
// the corresponding D_TYPE_*_BIT from type_info_cpp.hpp exactly.
namespace qualifier
{
    // --------------------------------------------------------
    //  references (bits 32-33)
    // --------------------------------------------------------
    constexpr re_std::uint64_t lvalue_ref   = (1ULL << 32);
    constexpr re_std::uint64_t rvalue_ref   = (1ULL << 33);

    // --------------------------------------------------------
    //  C++ specifiers (bits 34-43)
    // --------------------------------------------------------
    constexpr re_std::uint64_t mutable_     = (1ULL << 34);
    constexpr re_std::uint64_t virtual_     = (1ULL << 35);
    constexpr re_std::uint64_t constexpr_   = (1ULL << 36);
    constexpr re_std::uint64_t noexcept_    = (1ULL << 37);
    constexpr re_std::uint64_t final_       = (1ULL << 38);
    constexpr re_std::uint64_t override_    = (1ULL << 39);
    constexpr re_std::uint64_t explicit_    = (1ULL << 40);
    constexpr re_std::uint64_t consteval_   = (1ULL << 41);
    constexpr re_std::uint64_t constinit_   = (1ULL << 42);
    constexpr re_std::uint64_t template_    = (1ULL << 43);

    // pure virtual is virtual + a semantic marker;
    // we encode it as virtual | bit 44 (no type_info
    // counterpart — this is parser-internal)
    constexpr re_std::uint64_t pure_virtual = (1ULL << 44);

    // --------------------------------------------------------
    //  additional semantic markers (bits 45-47)
    //  These have no type_info counterpart; they are
    //  parser/DOM-internal for tracking declaration forms.
    // --------------------------------------------------------
    constexpr re_std::uint64_t deleted_     = (1ULL << 45);
    constexpr re_std::uint64_t defaulted_   = (1ULL << 46);
    constexpr re_std::uint64_t deprecated_  = (1ULL << 47);

    // --------------------------------------------------------
    //  masks
    // --------------------------------------------------------
    constexpr re_std::uint64_t ref_mask     = (lvalue_ref | rvalue_ref);

    constexpr re_std::uint64_t cpp_modifier_mask =
        ( lvalue_ref | rvalue_ref  | mutable_    | virtual_   |
          constexpr_ | noexcept_   | final_      | override_  |
          explicit_  | consteval_  | constinit_  | template_  |
          pure_virtual );

    constexpr re_std::uint64_t cpp_semantic_mask =
        (deleted_ | defaulted_ | deprecated_);

    constexpr re_std::uint64_t all_mask =
        (cv_mask | c_storage_mask | cpp_modifier_mask |
         cpp_semantic_mask);
};


// ================================================================
//  C++ qualifier predicates
// ================================================================

// qualifier predicates
//   functions: whether a qualifier word carries each qualifier. They live
// in namespace qualifier, beside the flags, so that names such as
// is_template and is_const stay clear of the type traits of the same name.
namespace qualifier
{

// is_virtual
//   returns true if the virtual qualifier is present
// (includes pure virtual).
D_CONSTEXPR_INLINE_VAR bool
is_virtual
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::virtual_);
}

// is_pure_virtual
//   returns true if the pure virtual marker is present.
D_CONSTEXPR_INLINE_VAR bool
is_pure_virtual
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::pure_virtual);
}

// is_constexpr
//   returns true if the constexpr specifier is present.
D_CONSTEXPR_INLINE_VAR bool
is_constexpr
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::constexpr_);
}

// is_consteval
//   returns true if the consteval specifier is present.
D_CONSTEXPR_INLINE_VAR bool
is_consteval
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::consteval_);
}

// is_noexcept
//   returns true if the noexcept specifier is present.
D_CONSTEXPR_INLINE_VAR bool
is_noexcept
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::noexcept_);
}

// is_final
//   returns true if the final specifier is present.
D_CONSTEXPR_INLINE_VAR bool
is_final
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::final_);
}

// is_override
//   returns true if the override specifier is present.
D_CONSTEXPR_INLINE_VAR bool
is_override
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::override_);
}

// is_explicit
//   returns true if the explicit specifier is present.
D_CONSTEXPR_INLINE_VAR bool
is_explicit
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::explicit_);
}

// is_template
//   returns true if the template specifier is present.
D_CONSTEXPR_INLINE_VAR bool
is_template
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::template_);
}

// is_deleted
//   returns true if the = delete marker is present.
D_CONSTEXPR_INLINE_VAR bool
is_deleted
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::deleted_);
}

// is_defaulted
//   returns true if the = default marker is present.
D_CONSTEXPR_INLINE_VAR bool
is_defaulted
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::defaulted_);
}

// is_deprecated
//   returns true if the D_DEPRECATED marker is present.
D_CONSTEXPR_INLINE_VAR bool
is_deprecated
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::deprecated_);
}

// is_mutable
//   returns true if the mutable specifier is present.
D_CONSTEXPR_INLINE_VAR bool
is_mutable
(
    re_std::uint64_t _quals
)
{
    return has_qualifier(_quals, qualifier::mutable_);
}

}  // namespace qualifier


// ================================================================
//  def_symbol_kind  (definition-internal, 0x0100-0x01FF)
// ================================================================

// def_symbol_kind
//   constants: extend symbol_kind for nodes that appear only
// inside function/method definition bodies (produced by
// cpp_def_parser).
namespace def_symbol_kind
{
    constexpr re_std::uint16_t function_def    = 0x0100;
    constexpr re_std::uint16_t call_expr       = 0x0101;
    constexpr re_std::uint16_t local_var       = 0x0102;
    constexpr re_std::uint16_t return_stmt     = 0x0103;
    constexpr re_std::uint16_t member_ref      = 0x0104;
    constexpr re_std::uint16_t decl_ref        = 0x0105;
};

// is_def_symbol_kind
//   returns true if a uint16_t kind value falls in the
// definition-internal range.
D_CONSTEXPR_INLINE_VAR bool
is_def_symbol_kind
(
    re_std::uint16_t _kind
)
{
    return ( (_kind >= 0x0100) &&
             (_kind <= 0x01FF) );
}


// ================================================================
//  change_kind
// ================================================================

// change_kind
//   constants: classifies the nature of a source change.
// Used by the diff pipeline.
namespace change_kind
{
    constexpr re_std::uint8_t added     = 0x01;
    constexpr re_std::uint8_t removed   = 0x02;
    constexpr re_std::uint8_t modified  = 0x03;
    constexpr re_std::uint8_t moved     = 0x04;
    constexpr re_std::uint8_t renamed   = 0x05;
};


// ================================================================
//  C++ language version
// ================================================================

// cpp_standard
//   constants: C++ standard version identifiers.
namespace cpp_standard
{
    constexpr re_std::uint8_t cpp98  = 0x01;
    constexpr re_std::uint8_t cpp03  = 0x02;
    constexpr re_std::uint8_t cpp11  = 0x03;
    constexpr re_std::uint8_t cpp14  = 0x04;
    constexpr re_std::uint8_t cpp17  = 0x05;
    constexpr re_std::uint8_t cpp20  = 0x06;
    constexpr re_std::uint8_t cpp23  = 0x07;
};


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_PARSERS_CPP_CPP_HPP
