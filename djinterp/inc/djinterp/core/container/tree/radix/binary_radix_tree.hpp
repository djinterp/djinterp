/*
binary_radix_tree.hpp
  PATRICIA trie mapping integral (bitwise) keys of type _Key to values
  of type _Value. Each node stores the bit index used for branching and
  the key associated with that node; the actual key comparison occurs
  only at the terminal. Back-edges are permitted: a child's stored
  bit_index may be less than or equal to that of its parent, which is
  the defining property of a PATRICIA trie.

  Bit traversal convention: bit 0 is the most-significant bit (MSB),
  following network / big-endian order. ip_radix_tree<V> exploits this
  to match the standard CIDR prefix notation for IPv4 routing tables.

  Named subtypes:
    dyn_binary_radix_tree<K, V>       — dynamic, writable, unordered
    fixed_binary_radix_tree<K, V, N>  — fixed node pool, writable
    immutable_binary_radix_tree<K, V> — heap, immutable (read-only)
    ip_radix_tree<V>                  — uint32_t keys, ordered, dynamic

  Depends:
    radix_tree_common.hpp  (provides DContainerOption, shared node types,
                            bit_at, binary_prefix_length, and
                            radix_tree_traits.hpp)
*/

#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include "radix_tree_common.hpp"

namespace djinterp
{

// ==========================================================================
// binary_radix_node
// ==========================================================================

// binary_radix_node
//   struct: node in a PATRICIA trie over integral keys. Stores the bit
//   index used to route descents (bit_index == 0 is the MSB), the key
//   associated with this node (meaningful when is_terminal == true),
//   the terminal value, and exactly two child pointers — one for each
//   bit value (0 at index 0, 1 at index 1). Inherits parent pointer
//   and is_terminal flag from radix_node_base via CRTP.
//
//   Back-edges: a child's bit_index may be <= its parent's bit_index.
//   When a back-edge is detected during descent, the stored key is
//   compared against the search key to confirm or deny a match.
template<typename _Key,
         typename _Value,
         typename = enable_if_binary_key<_Key>>
struct binary_radix_node
    : public radix_node_base<binary_radix_node<_Key, _Value>>
{
    // ---------------------------------------------------------------
    // Member type aliases required by is_radix_node detection
    // ---------------------------------------------------------------
    using key_type   = _Key;
    using value_type = _Value;
    using node_type  = binary_radix_node<_Key, _Value>;
    using children   = node_type*[D_RADIX_BINARY_BRANCHES];

    // ---------------------------------------------------------------
    // Data members
    // ---------------------------------------------------------------

    // bit position (0 == MSB) tested at this node to route descent
    std::size_t bit_index;

    // key stored at this node; compared only when a back-edge leads
    // here (i.e. when this node's bit_index <= parent's bit_index)
    _Key key;

    // mapped value storage (present == false at non-terminal nodes)
    radix_terminal<_Value> terminal;

    // two child pointers:  [0] = bit is 0,  [1] = bit is 1
    node_type* child_ptrs[D_RADIX_BINARY_BRANCHES];

    // ---------------------------------------------------------------
    // Constructors
    // ---------------------------------------------------------------

    // binary_radix_node()
    //   constructor: default — bit_index 0, zero-valued key, no
    //   children.
    binary_radix_node() noexcept
        : radix_node_base<node_type>()
        , bit_index(0u)
        , key()
        , terminal()
    {
        child_ptrs[0] = nullptr;
        child_ptrs[1] = nullptr;
    }

    // binary_radix_node(std::size_t, _Key)
    //   constructor: sets bit_index and key; clears both children.
    binary_radix_node
    (
        std::size_t _bit_index,
        _Key        _key
    ) noexcept
        : radix_node_base<node_type>()
        , bit_index(_bit_index)
        , key(_key)
        , terminal()
    {
        child_ptrs[0] = nullptr;
        child_ptrs[1] = nullptr;
    }

    // binary_radix_node(const binary_radix_node&) — deleted; deep copy
    // is handled at the tree level via m_copy_all_nodes.
    binary_radix_node(const binary_radix_node&)            = delete;
    binary_radix_node& operator=(const binary_radix_node&) = delete;
};

// ==========================================================================
// binary_radix_tree — runtime base class (PATRICIA trie)
// ==========================================================================

// binary_radix_tree
//   class: PATRICIA trie mapping integral keys of type _Key to values of
//   type _Value. Descends by testing the bit at node.bit_index; back-
//   edges short-circuit descent and trigger a full key comparison.
//   Mutating methods are SFINAE-gated on k_writable. _KeyGuard enforces
//   the is_binary_key constraint at class-template instantiation.
//
//   Storage strategy (controlled by k_fixed):
//     k_fixed == true  — static node pool of _Capacity entries;
//                        insertion throws std::bad_alloc on exhaustion.
//     k_fixed == false — nodes are heap-allocated via operator new;
//                        the tree grows without a hard cap.
template<typename         _Key,
         typename         _Value,
         std::size_t      _Capacity = 0,
         DContainerOption _Flags    = DContainerOption::none,
         typename         _KeyGuard = enable_if_binary_key<_Key>>
class binary_radix_tree
{
private:
    static_assert(
        is_binary_key_v<_Key>,
        "binary_radix_tree: _Key must be an integral or pointer type.");

    // ---------------------------------------------------------------
    // Option resolution — private, computed at compile time
    // ---------------------------------------------------------------
    static constexpr DContainerOption k_resolved =
        radix_tree_option_resolve(_Flags, _Capacity);

    static_assert(
        container_option_axis_valid(_Flags),
        "DContainerOption: at most one flag per axis.");

    static_assert(
        !container_option_has(k_resolved, DContainerOption::compile_time),
        "compile_time mutability is handled by binary_radix_tree_ct.");

    static constexpr bool k_writable  =
        container_option_has(k_resolved, DContainerOption::writable);
    static constexpr bool k_immutable =
        container_option_has(k_resolved, DContainerOption::immutable);
    static constexpr bool k_ordered   =
        container_option_has(k_resolved, DContainerOption::ordered);
    static constexpr bool k_unordered =
        container_option_has(k_resolved, DContainerOption::unordered);
    static constexpr bool k_fixed     =
        container_option_has(k_resolved, DContainerOption::fixed_size);
    static constexpr bool k_dynamic   =
        container_option_has(k_resolved, DContainerOption::dynamic_size);

    static constexpr std::size_t k_key_bits = key_bit_width_v<_Key>;

    // ---------------------------------------------------------------
    // Node type and storage strategy
    // ---------------------------------------------------------------
    using node_type = binary_radix_node<_Key, _Value>;

    // fixed_node_pool — stack-allocated slab; used[] tracks liveness.
    struct fixed_node_pool
    {
        node_type   nodes[_Capacity > 0u ? _Capacity : 1u];
        bool        used[_Capacity > 0u ? _Capacity : 1u];
        std::size_t count;
    };

    // dynamic_storage — heap; the head pointer owns the trie root.
    struct dynamic_storage
    {
        node_type*  head;
        std::size_t count;
    };

    using storage_type = typename std::conditional<
                             k_fixed,
                             fixed_node_pool,
                             dynamic_storage>::type;

public:
    // ---------------------------------------------------------------
    // Public type aliases — standard container interface
    // ---------------------------------------------------------------
    using value_type      = _Value;
    using key_type        = _Key;
    using pointer         = _Value*;
    using const_pointer   = const _Value*;
    using reference       = _Value&;
    using const_reference = const _Value&;
    using difference_type = std::ptrdiff_t;
    using size_type       = std::size_t;

    // option introspection — required by cli_traits.hpp detection
    using options_type = DContainerOption;
    static constexpr DContainerOption option_flags = k_resolved;

    // compile-time property mirrors of k_ flags (public API)
    static constexpr bool      is_writable  = k_writable;
    static constexpr bool      is_immutable = k_immutable;
    static constexpr bool      is_ordered   = k_ordered;
    static constexpr bool      is_fixed     = k_fixed;
    static constexpr bool      is_dynamic   = k_dynamic;

    // bit width of the key type (e.g. 32 for uint32_t, 64 for uint64_t)
    static constexpr size_type key_bits = k_key_bits;

    // ---------------------------------------------------------------
    // Constructors and destructor
    // ---------------------------------------------------------------

    binary_radix_tree() noexcept;
    binary_radix_tree(const binary_radix_tree& _other);
    binary_radix_tree(binary_radix_tree&& _other) noexcept;

    ~binary_radix_tree();

    // ---------------------------------------------------------------
    // Assignment operators
    // ---------------------------------------------------------------
    binary_radix_tree& operator=(const binary_radix_tree& _other);
    binary_radix_tree& operator=(binary_radix_tree&& _other) noexcept;

    // ---------------------------------------------------------------
    // Size and capacity queries
    // ---------------------------------------------------------------
    size_type size()     const noexcept;
    bool      empty()    const noexcept;
    size_type capacity() const noexcept;

    // ---------------------------------------------------------------
    // Const lookup — always available regardless of flag combination
    // ---------------------------------------------------------------

    // contains — true when _key is present in the trie.
    bool           contains(key_type _key)  const noexcept;

    // find — returns a const pointer to the value, or nullptr if absent.
    const_pointer  find(key_type _key)      const noexcept;

    // at — returns a const reference; throws std::out_of_range on miss.
    const_reference at(key_type _key) const;

    // ---------------------------------------------------------------
    // Mutable access — gated on k_writable
    // ---------------------------------------------------------------

    // find (mutable) — returns a non-const pointer, or nullptr.
    template<bool _W = k_writable,
             typename std::enable_if<_W, int>::type = 0>
    pointer find(key_type _key) noexcept;

    // at (mutable) — returns a non-const reference; throws on miss.
    template<bool _W = k_writable,
             typename std::enable_if<_W, int>::type = 0>
    reference at(key_type _key);

    // ---------------------------------------------------------------
    // Insertion — gated on k_writable
    // ---------------------------------------------------------------

    // insert — inserts or overwrites the mapping for _key.
    //   Returns true when a new node was created (false on overwrite).
    template<bool _W = k_writable,
             typename std::enable_if<_W, int>::type = 0>
    bool insert(key_type _key, const value_type& _value);

    // insert (move overload) — as above, consuming _value via move.
    template<bool _W = k_writable,
             typename std::enable_if<_W, int>::type = 0>
    bool insert(key_type _key, value_type&& _value);

    // try_insert — inserts _key only if it is not already present.
    //   Returns true on a new insertion, false on a collision.
    template<bool _W = k_writable,
             typename std::enable_if<_W, int>::type = 0>
    bool try_insert(key_type _key, const value_type& _value);

    // ---------------------------------------------------------------
    // Erasure — gated on k_writable
    // ---------------------------------------------------------------

    // erase — removes the terminal for _key.
    //   Returns true when a terminal was found and removed.
    template<bool _W = k_writable,
             typename std::enable_if<_W, int>::type = 0>
    bool erase(key_type _key);

    // clear — removes all nodes; the head pointer is reset to nullptr.
    template<bool _W = k_writable,
             typename std::enable_if<_W, int>::type = 0>
    void clear() noexcept;

    // ---------------------------------------------------------------
    // Bit-level prefix queries — always available (read-only)
    // ---------------------------------------------------------------

    // longest_prefix_match — returns a const pointer to the value of
    //   the stored key that shares the greatest number of leading bits
    //   with _key. Useful for CIDR routing table lookups. Returns
    //   nullptr when the trie is empty or no prefix matches exist.
    const_pointer longest_prefix_match(key_type _key) const noexcept;

    // count_with_bit_prefix — returns the number of stored keys that
    //   share the first _prefix_len bits with _pattern.
    size_type count_with_bit_prefix(key_type    _pattern,
                                    std::size_t _prefix_len)
        const noexcept;

    // ---------------------------------------------------------------
    // Bit extraction helpers — public static utilities
    // ---------------------------------------------------------------

    // key_bit_at — extracts bit _pos (0 == MSB) from _key.
    static constexpr bool
    key_bit_at(key_type _key, std::size_t _pos) noexcept
    {
        return bit_at(_key, _pos);
    }

    // shared_prefix_bits — number of leading bits shared between _a
    //   and _b, scanning at most k_key_bits positions from the MSB.
    static constexpr std::size_t
    shared_prefix_bits(key_type _a, key_type _b) noexcept
    {
        return binary_prefix_length(_a, _b, k_key_bits);
    }

    // mask_for_prefix — returns a bitmask with the _prefix_len most-
    //   significant bits set and all lower bits cleared. Useful for
    //   constructing CIDR subnet masks from a prefix length.
    static constexpr _Key
    mask_for_prefix(std::size_t _prefix_len) noexcept
    {
        if (_prefix_len == 0u)
        {
            return static_cast<_Key>(0);
        }

        if (_prefix_len >= k_key_bits)
        {
            return ~static_cast<_Key>(0);
        }

        // shift a full-ones mask right by the suffix length, then
        // shift left to clear the suffix bits
        return static_cast<_Key>(
            ~static_cast<_Key>(0) << (k_key_bits - _prefix_len));
    }

private:
    // ---------------------------------------------------------------
    // Storage and count
    // ---------------------------------------------------------------
    storage_type m_store;
    size_type    m_count;

    // ---------------------------------------------------------------
    // Internal helper declarations
    // ---------------------------------------------------------------
    node_type* m_head_ptr() const noexcept;

    // m_allocate_node — acquires one node from the pool or heap.
    //   Throws std::bad_alloc when the fixed pool is exhausted.
    node_type* m_allocate_node(std::size_t _bit_index, key_type _key);

    // m_free_node — returns _node to the pool or calls delete.
    void m_free_node(node_type* _node) noexcept;

    // m_destroy_all — frees every node reachable from _node using an
    //   iterative post-order traversal (avoids stack overflow on large
    //   tries).
    void m_destroy_all(node_type* _node) noexcept;

    // m_copy_all_nodes — deep-copies the trie rooted at _src, setting
    //   parent pointers correctly. Returns the new root.
    node_type* m_copy_all_nodes(const node_type* _src,
                                node_type*       _parent);

    // m_patricia_search — follows bit_index slots from the head until
    //   a back-edge is detected or the trie is exhausted. Returns the
    //   last node visited (the "closest" node to _key).
    node_type* m_patricia_search(key_type _key) const noexcept;

    // m_first_differing_bit — scans from the MSB and returns the index
    //   of the first bit position where _new_key and _existing_key
    //   differ. Returns k_key_bits when they are equal.
    std::size_t m_first_differing_bit(key_type _new_key,
                                      key_type _existing_key)
        const noexcept;

    // m_insert_impl — shared implementation for both insert overloads.
    template<typename _FwdValue>
    bool m_insert_impl(key_type _key, _FwdValue&& _value);
};

// ==========================================================================
// Named subtypes via type aliases
// ==========================================================================

// dyn_binary_radix_tree
//   type: dynamic, writable, unordered binary radix tree (the default).
template<typename _Key,
         typename _Value>
using dyn_binary_radix_tree = binary_radix_tree<
    _Key, _Value, 0,
    ( DContainerOption::writable     |
      DContainerOption::unordered    |
      DContainerOption::dynamic_size )>;

// fixed_binary_radix_tree
//   type: writable, unordered binary radix tree backed by a fixed node
//   pool of _N entries. Insertion throws when the pool is exhausted.
template<typename    _Key,
         typename    _Value,
         std::size_t _N>
using fixed_binary_radix_tree = binary_radix_tree<
    _Key, _Value, _N,
    ( DContainerOption::writable  |
      DContainerOption::unordered |
      DContainerOption::fixed_size )>;

// immutable_binary_radix_tree
//   type: heap-allocated, immutable binary radix tree. All mutating
//   methods are SFINAE-removed. Suitable for read-only lookup tables
//   built once at startup.
template<typename _Key,
         typename _Value>
using immutable_binary_radix_tree = binary_radix_tree<
    _Key, _Value, 0,
    ( DContainerOption::immutable    |
      DContainerOption::unordered    |
      DContainerOption::dynamic_size )>;

// ip_radix_tree
//   type: IPv4 routing table — uint32_t keys, ordered (keys visited in
//   ascending numeric order), dynamic heap allocation. longest_prefix_
//   match implements a standard CIDR next-hop lookup when combined with
//   mask_for_prefix() and count_with_bit_prefix().
template<typename _Value>
using ip_radix_tree = binary_radix_tree<
    std::uint32_t, _Value, 0,
    ( DContainerOption::writable     |
      DContainerOption::ordered      |
      DContainerOption::dynamic_size )>;

// ==========================================================================
// Deduction guides
// ==========================================================================

// Deduction guide: binary_radix_tree{key, value} deduces
// dyn_binary_radix_tree<_Key, _Value>.
template<typename _Key,
         typename _Value>
binary_radix_tree(_Key, _Value) -> dyn_binary_radix_tree<_Key, _Value>;

}   // djinterp
