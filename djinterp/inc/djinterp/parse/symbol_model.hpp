/******************************************************************************
* djinterp [parse]                                             symbol_model.hpp
*
* Parser symbol model:
*   This header carries the parse subframework's output model -- the payload
* the libclang frontends and cpp_scanner allocate into the destination arena.
* It was split out of the former arena/arena_domain.hpp: the symbol side is
* parse-owned (parse_context, clang_frontend, and cpp_scanner all write it),
* so it belongs in djinterp::parse, not in the flat core and not manager-side.
* The diff / wiki / workspace instantiations that shared arena_domain.hpp were
* extension and integration concerns; they moved to the codebase manager.
*
* Contents:
*   - source_location   a located span of source text
*   - symbol_data       payload for the parser's symbol tree
*   - symbol_tree       arena<symbol_data>
*
* Design -- qualifier field:
*   symbol_data carries a uint64_t `qualifiers` field whose bit positions
* match the type_info system exactly (type_info_common.h, type_info_c.h,
* type_info_cpp.h), so the D_TYPE_IS_* / D_TYPE_SET_* macros apply directly
* to a symbol's qualifiers with no translation or packing.  The qualifier,
* symbol_kind, and access_specifier constants are the flat core vocabulary
* from lang/cpp.hpp (the codebase is flat outside djinterp::parse, so they
* are reached unqualified from here via enclosing-namespace lookup).
*
*
* path:      /inc/cpp/parse/symbol_model.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.30
******************************************************************************/

#ifndef DJINTERP_PARSE_SYMBOL_MODEL_
#define DJINTERP_PARSE_SYMBOL_MODEL_ 1

#include <cstddef>
#include <cstdint>
#include "../core/djinterp.hpp"
#include "../lang/cpp.hpp"
#include "../arena/arena.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  source_location
// ================================================================

// source_location
//   struct: identifies a span of source text within a file.
struct source_location
{
    std::uint32_t   file_id;
    std::uint32_t   line;
    std::uint16_t   column;
    std::uint32_t   offset;
    std::uint32_t   length;

    source_location
    ()
        : file_id (0),
          line    (0),
          column  (0),
          offset  (0),
          length  (0)
    {
    }

    source_location
    (
        std::uint32_t _file_id,
        std::uint32_t _line,
        std::uint16_t _column,
        std::uint32_t _offset,
        std::uint32_t _length
    )
        : file_id (_file_id),
          line    (_line),
          column  (_column),
          offset  (_offset),
          length  (_length)
    {
    }
};


// ================================================================
//  symbol_data
// ================================================================

// symbol_data
//   struct: payload for the parser's symbol tree.  Each node
// represents a located, typed code symbol.
//
//   The `qualifiers` field uses the type_info bit layout:
//     bits 11-12:  const / volatile
//     bits 24-31:  C storage class (static, extern, etc.)
//     bits 32-47:  C++ modifiers (virtual, constexpr, etc.)
//
//   The `access` field is separate because access specifiers
// are mutually exclusive (not a bitfield).
struct symbol_data
{
    source_location location;

    std::uint16_t   kind;           // symbol_kind
    std::uint8_t    access;         // access_specifier
    std::uint8_t    reserved;       // padding / future use
    std::uint64_t   qualifiers;     // qualifier (type_info-aligned)

    std::uint32_t   name_id;        // index into string_table
    std::uint32_t   type_id;        // index into string_table
    std::uint32_t   signature_id;   // index into string_table
    std::uint32_t   comment_id;     // index into string_table

    symbol_data
    ()
        : location     (),
          kind         (symbol_kind::unknown),
          access       (access_specifier::unspecified),
          reserved     (0),
          qualifiers   (qualifier::none),
          name_id      (0),
          type_id      (0),
          signature_id (0),
          comment_id   (0)
    {
    }

    // --------------------------------------------------------
    //  qualifier convenience methods
    // --------------------------------------------------------

    // has_qualifier
    //   returns true if the given qualifier bit(s) are set.
    bool
    has_qualifier
    (
        std::uint64_t _flag
    ) const
    {
        return (qualifiers & _flag) != 0;
    }

    // set_qualifier
    //   sets the given qualifier bit(s).
    void
    set_qualifier
    (
        std::uint64_t _flag
    )
    {
        qualifiers |= _flag;

        return;
    }

    // is_const
    bool is_const()     const { return has_qualifier(qualifier::const_); }

    // is_static
    bool is_static()    const { return has_qualifier(qualifier::static_); }

    // is_virtual
    bool is_virtual()   const { return has_qualifier(qualifier::virtual_); }

    // is_constexpr
    bool is_constexpr() const { return has_qualifier(qualifier::constexpr_); }

    // is_inline
    bool is_inline()    const { return has_qualifier(qualifier::inline_); }

    // is_noexcept
    bool is_noexcept()  const { return has_qualifier(qualifier::noexcept_); }

    // is_template
    bool is_template()  const { return has_qualifier(qualifier::template_); }
};


// ================================================================
//  symbol_tree
// ================================================================

// symbol_tree
//   typedef: the arena the parser allocates symbol_data into.
//   `arena` is the flat core container template (arena/arena.hpp),
//   reached unqualified from djinterp::parse.
typedef arena<symbol_data>  symbol_tree;


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_SYMBOL_MODEL_
