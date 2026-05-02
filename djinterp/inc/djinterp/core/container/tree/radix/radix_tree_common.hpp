/*
radix_tree_common.hpp
  Shared infrastructure for all djinterp radix tree specializations.
  Provides:
    — DContainerOption enum, bitwise operators, axis extraction, and
      default resolution (condensed from radix_tree_options.hpp).
    — Text prefix utilities: text_prefix_match_result,
      common_prefix_length, text_prefix_compare.
    — Binary prefix utilities: bit_at, binary_prefix_length.
    — Shared node types: radix_terminal<V>, radix_node_base<Derived>.
    — Traversal helper: radix_traversal_frame<NodePtr>.
    — Constants: D_RADIX_ALPHA_SIZE, D_RADIX_BINARY_BRANCHES.

  Dependency graph:
    radix_tree_traits.hpp  (standalone)
          |
    radix_tree_common.hpp  (this file)
          |
    text_radix_tree.hpp    binary_radix_tree.hpp
*/

#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include "radix_tree_traits.hpp"

namespace djinterp
{

// ==========================================================================
// Option system
// (Normally lives in radix_tree_options.hpp; condensed here so that
//  this four-file module set is self-contained. DContainerOption is the
//  framework-wide shared enum — bit positions are identical to every
//  other djinterp container.)
// ==========================================================================

// DContainerOption
//   enum: shared bitmask controlling mutability, ordering, and storage
//   strategy for every djinterp container. At most one bit per axis may
//   be set; enforced by container_option_axis_valid(). Radix-tree-
//   specific axes begin at bit 7 (see radix_tree_option_resolve).
enum class DContainerOption : unsigned
{
    none         = 0x00u,

    // mutability axis (bits 0–2)
    writable     = 0x01u,
    immutable    = 0x02u,
    compile_time = 0x04u,

    // ordering axis (bits 3–4)
    ordered      = 0x08u,
    unordered    = 0x10u,

    // storage axis (bits 5–6)
    fixed_size   = 0x20u,
    dynamic_size = 0x40u
};

// operator| — bitwise OR for combining DContainerOption flags.
inline constexpr DContainerOption
operator|
(
    DContainerOption _lhs,
    DContainerOption _rhs
) noexcept
{
    return static_cast<DContainerOption>(
        static_cast<unsigned>(_lhs) | static_cast<unsigned>(_rhs));
}

// operator& — bitwise AND for masking DContainerOption flags.
inline constexpr DContainerOption
operator&
(
    DContainerOption _lhs,
    DContainerOption _rhs
) noexcept
{
    return static_cast<DContainerOption>(
        static_cast<unsigned>(_lhs) & static_cast<unsigned>(_rhs));
}

// operator^ — bitwise XOR for toggling DContainerOption flags.
inline constexpr DContainerOption
operator^
(
    DContainerOption _lhs,
    DContainerOption _rhs
) noexcept
{
    return static_cast<DContainerOption>(
        static_cast<unsigned>(_lhs) ^ static_cast<unsigned>(_rhs));
}

// operator~ — bitwise NOT for inverting DContainerOption flags.
inline constexpr DContainerOption
operator~(DContainerOption _v) noexcept
{
    return static_cast<DContainerOption>(~static_cast<unsigned>(_v));
}

// operator|= — compound OR assignment.
inline constexpr DContainerOption&
operator|=
(
    DContainerOption& _lhs,
    DContainerOption  _rhs
) noexcept
{
    _lhs = _lhs | _rhs;

    return _lhs;
}

// operator&= — compound AND assignment.
inline constexpr DContainerOption&
operator&=
(
    DContainerOption& _lhs,
    DContainerOption  _rhs
) noexcept
{
    _lhs = _lhs & _rhs;

    return _lhs;
}

// --------------------------------------------------------------------------
// Axis masks and extraction
// --------------------------------------------------------------------------

// D_CONTAINER_OPTION_MUTABILITY_MASK
//   constant: covers the mutability axis bits (writable, immutable,
// compile_time).
#define D_CONTAINER_OPTION_MUTABILITY_MASK \
    (DContainerOption::writable     | \
     DContainerOption::immutable    | \
     DContainerOption::compile_time)

// D_CONTAINER_OPTION_ORDERING_MASK
//   constant: covers the ordering axis bits (ordered, unordered).
#define D_CONTAINER_OPTION_ORDERING_MASK \
    (DContainerOption::ordered | \
     DContainerOption::unordered)

// D_CONTAINER_OPTION_STORAGE_MASK
//   constant: covers the storage axis bits (fixed_size, dynamic_size).
#define D_CONTAINER_OPTION_STORAGE_MASK \
    (DContainerOption::fixed_size | \
     DContainerOption::dynamic_size)

// container_option_mutability
//   function: isolates the mutability axis bits from _flags.
inline constexpr DContainerOption
container_option_mutability(DContainerOption _flags) noexcept
{
    return (_flags & D_CONTAINER_OPTION_MUTABILITY_MASK);
}

// container_option_ordering
//   function: isolates the ordering axis bits from _flags.
inline constexpr DContainerOption
container_option_ordering(DContainerOption _flags) noexcept
{
    return (_flags & D_CONTAINER_OPTION_ORDERING_MASK);
}

// container_option_storage
//   function: isolates the storage axis bits from _flags.
inline constexpr DContainerOption
container_option_storage(DContainerOption _flags) noexcept
{
    return (_flags & D_CONTAINER_OPTION_STORAGE_MASK);
}

// container_option_has
//   function: returns true when _flags has all bits of _bit set.
inline constexpr bool
container_option_has
(
    DContainerOption _flags,
    DContainerOption _bit
) noexcept
{
    return ((_flags & _bit) == _bit);
}

// --------------------------------------------------------------------------
// Axis validation
// --------------------------------------------------------------------------

namespace internal
{

// popcount_constexpr
//   function: counts the number of set bits in an unsigned value.
//   Used by container_option_axis_valid to check single-bit-per-axis.
inline constexpr std::size_t
popcount_constexpr(unsigned _v) noexcept
{
    std::size_t count = 0u;

    // count set bits via Brian Kernighan's algorithm
    while (_v != 0u)
    {
        _v     = _v & (_v - 1u);
        count += 1u;
    }

    return count;
}

}   // internal

// container_option_axis_valid
//   function: returns true when at most one bit is set within each of
//   the mutability, ordering, and storage axes.
inline constexpr bool
container_option_axis_valid(DContainerOption _flags) noexcept
{
    return (
        ( internal::popcount_constexpr(
              static_cast<unsigned>(container_option_mutability(_flags)))
          <= 1u ) &&
        ( internal::popcount_constexpr(
              static_cast<unsigned>(container_option_ordering(_flags)))
          <= 1u ) &&
        ( internal::popcount_constexpr(
              static_cast<unsigned>(container_option_storage(_flags)))
          <= 1u ) );
}

// --------------------------------------------------------------------------
// Default resolution (radix-tree-specific)
// --------------------------------------------------------------------------

// radix_tree_option_resolve
//   function: fills unset axes with sensible defaults for a radix tree.
//   Mutability defaults to writable; ordering defaults to unordered;
//   storage defaults to fixed_size when _capacity > 0, otherwise
//   dynamic_size.
inline constexpr DContainerOption
radix_tree_option_resolve
(
    DContainerOption _flags,
    std::size_t      _capacity
) noexcept
{
    DContainerOption m = container_option_mutability(_flags);
    DContainerOption o = container_option_ordering(_flags);
    DContainerOption s = container_option_storage(_flags);

    // apply axis-specific defaults
    if (m == DContainerOption::none)
    {
        m = DContainerOption::writable;
    }

    if (o == DContainerOption::none)
    {
        o = DContainerOption::unordered;
    }

    if (s == DContainerOption::none)
    {
        s = (_capacity > 0u)
              ? DContainerOption::fixed_size
              : DContainerOption::dynamic_size;
    }

    return (m | o | s);
}

// ==========================================================================
// Text prefix utilities
// ==========================================================================

// text_prefix_match_result
//   struct: result of a common-prefix comparison between a search key
//   and a compressed edge label within a text radix tree node.
struct text_prefix_match_result
{
    // number of characters matched from the front of both strings
    std::size_t common_len;

    // true when the entire key is a prefix of (or equal to) the edge
    bool key_consumed;

    // true when the entire edge is a prefix of (or equal to) the key
    bool edge_consumed;

    // true when key == edge exactly (both fully consumed)
    bool exact;
};

// common_prefix_length
//   function: returns the count of leading characters shared between
//   _a and _b.
inline constexpr std::size_t
common_prefix_length
(
    std::string_view _a,
    std::string_view _b
) noexcept
{
    std::size_t i   = 0u;
    std::size_t len = (_a.size() < _b.size()) ? _a.size() : _b.size();

    // scan forward until the first differing character
    while ( (i < len)       &&
            (_a[i] == _b[i]) )
    {
        ++i;
    }

    return i;
}

// text_prefix_compare
//   function: compares search key _key against compressed edge label
//   _edge and returns a fully-populated text_prefix_match_result.
//   Used during trie descend and split-edge insertion.
inline constexpr text_prefix_match_result
text_prefix_compare
(
    std::string_view _key,
    std::string_view _edge
) noexcept
{
    text_prefix_match_result r;

    r.common_len    = common_prefix_length(_key, _edge);
    r.key_consumed  = (r.common_len == _key.size());
    r.edge_consumed = (r.common_len == _edge.size());
    r.exact         = (r.key_consumed && r.edge_consumed);

    return r;
}

// ==========================================================================
// Binary / bitwise prefix utilities
// ==========================================================================

// bit_at
//   function: extracts the bit at zero-based position _pos from _key,
//   where position 0 is the most-significant bit (network / big-endian
//   traversal order). _Key must satisfy is_binary_key.
template<typename _Key,
         typename = enable_if_binary_key<_Key>>
inline constexpr bool
bit_at
(
    _Key        _key,
    std::size_t _pos
) noexcept
{
    constexpr std::size_t k_width = key_bit_width_v<_Key>;

    return static_cast<bool>(
        (_key >> static_cast<_Key>(k_width - 1u - _pos)) &
        static_cast<_Key>(1));
}

// binary_prefix_length
//   function: returns the count of matching leading bits between _a
//   and _b, scanning at most _max_bits positions from the MSB.
//   _Key must satisfy is_binary_key.
template<typename _Key,
         typename = enable_if_binary_key<_Key>>
inline constexpr std::size_t
binary_prefix_length
(
    _Key        _a,
    _Key        _b,
    std::size_t _max_bits = key_bit_width_v<_Key>
) noexcept
{
    std::size_t i = 0u;

    // count matching bits from the MSB outward
    while ( (i < _max_bits)             &&
            (bit_at(_a, i) == bit_at(_b, i)) )
    {
        ++i;
    }

    return i;
}

// ==========================================================================
// Terminal value storage
// ==========================================================================

// radix_terminal
//   struct: optional mapped value stored at a leaf or internal terminal
//   node. Uses a plain bool flag rather than std::optional so that the
//   struct remains trivially-default-constructible and constexpr-safe.
template<typename _Value>
struct radix_terminal
{
    // true when this node is the terminal for a stored key
    bool   present;

    // the mapped value; indeterminate when present == false
    _Value value;

    // radix_terminal()
    //   constructor: default — no value present.
    constexpr radix_terminal() noexcept
        : present(false)
        , value()
    {
    }

    // radix_terminal(const _Value&)
    //   constructor: marks the terminal present with a copied value.
    constexpr explicit
    radix_terminal(const _Value& _v) noexcept(
        std::is_nothrow_copy_constructible<_Value>::value)
        : present(true)
        , value(_v)
    {
    }

    // radix_terminal(_Value&&)
    //   constructor: marks the terminal present with a moved value.
    constexpr explicit
    radix_terminal(_Value&& _v) noexcept(
        std::is_nothrow_move_constructible<_Value>::value)
        : present(true)
        , value(static_cast<_Value&&>(_v))
    {
    }
};

// ==========================================================================
// CRTP node base
// ==========================================================================

// radix_node_base
//   struct: CRTP base supplying common bookkeeping fields shared by
//   text_radix_node and binary_radix_node. _Derived is the concrete
//   node type, enabling typed parent pointers without virtual dispatch.
template<typename _Derived>
struct radix_node_base
{
    // pointer to the parent node; nullptr at the trie root
    _Derived* parent;

    // true when a complete key terminates at this node
    bool is_terminal;

    // radix_node_base()
    //   constructor: default — no parent, non-terminal.
    constexpr radix_node_base() noexcept
        : parent(nullptr)
        , is_terminal(false)
    {
    }
};

// ==========================================================================
// Iterative traversal frame
// ==========================================================================

// radix_traversal_frame
//   struct: one frame on an explicit traversal stack used by iterative
//   depth-first descent. Avoids recursion-depth limits on deep tries.
//   _NodePtr is typically a raw pointer to the concrete node type.
template<typename _NodePtr>
struct radix_traversal_frame
{
    // the node at this level of the traversal
    _NodePtr node;

    // index of the next child slot to visit (incremented by the caller)
    std::size_t child_index;

    // radix_traversal_frame(_NodePtr)
    //   constructor: starts traversal of _n from child slot 0.
    constexpr explicit
    radix_traversal_frame(_NodePtr _n) noexcept
        : node(_n)
        , child_index(0u)
    {
    }
};

// ==========================================================================
// Shared constants
// ==========================================================================

// D_RADIX_ALPHA_SIZE
//   constant: number of distinct first-byte values used to index child
// arrays in text_radix_node (full 8-bit / Latin-1 alphabet).
#define D_RADIX_ALPHA_SIZE   256u

// D_RADIX_BINARY_BRANCHES
//   constant: number of children per node in binary_radix_node (one
// branch per bit value: 0 and 1).
#define D_RADIX_BINARY_BRANCHES   2u

}   // djinterp
