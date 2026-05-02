/*
text_radix_tree.hpp
  Compressed string trie (Patricia / radix trie) keyed on
  std::string_view. Each node stores a partial edge label; insertion
  splits edges on first-byte divergence. Controlled by DContainerOption
  flags following the djinterp container conventions.

  Named subtypes:
    dyn_text_radix_tree<V>          — dynamic, writable, unordered
    fixed_text_radix_tree<V, N>     — fixed node pool, writable, unordered
    immutable_text_radix_tree<V>    — heap, immutable (read-only)
    ordered_text_radix_tree<V>      — dynamic, writable, lexicographic order

  Depends:
    radix_tree_common.hpp  (provides DContainerOption, shared node types,
                            prefix utilities, and radix_tree_traits.hpp)
*/

#pragma once
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include "radix_tree_common.hpp"

namespace djinterp
{

// ==========================================================================
// text_radix_node
// ==========================================================================

// text_radix_node
//   struct: node in a compressed string trie. Stores a partial edge
//   label (the longest common prefix along the path from the parent),
//   an optional terminal value, and a 256-slot child pointer array
//   indexed by the first character of the remaining key suffix after
//   the edge is fully consumed. Inherits parent pointer and
//   is_terminal flag from radix_node_base via CRTP.
template<typename _Value>
struct text_radix_node
    : public radix_node_base<text_radix_node<_Value>>
{
    // ---------------------------------------------------------------
    // Member type aliases required by is_radix_node detection
    // ---------------------------------------------------------------
    using value_type = _Value;
    using node_type  = text_radix_node<_Value>;
    using children   = node_type*[D_RADIX_ALPHA_SIZE];

    // ---------------------------------------------------------------
    // Data members
    // ---------------------------------------------------------------

    // compressed edge label from parent to this node
    std::string edge;

    // mapped value storage (present == false at non-terminal nodes)
    radix_terminal<_Value> terminal;

    // 256 outgoing child pointers, indexed by the first byte of the
    // remaining key suffix after consuming this node's edge label
    node_type* child_ptrs[D_RADIX_ALPHA_SIZE];

    // ---------------------------------------------------------------
    // Constructors
    // ---------------------------------------------------------------

    // text_radix_node()
    //   constructor: default — empty edge, non-terminal, no children.
    text_radix_node() noexcept
        : radix_node_base<node_type>()
        , edge()
        , terminal()
    {
        // zero all 256 child slots
        for (std::size_t i = 0u; i < D_RADIX_ALPHA_SIZE; ++i)
        {
            child_ptrs[i] = nullptr;
        }
    }

    // text_radix_node(std::string_view, bool)
    //   constructor: sets the edge label and terminal flag; no children.
    explicit
    text_radix_node
    (
        std::string_view _edge_label,
        bool             _is_terminal = false
    )
        : radix_node_base<node_type>()
        , edge(_edge_label)
        , terminal()
    {
        this->is_terminal = _is_terminal;

        // zero all 256 child slots
        for (std::size_t i = 0u; i < D_RADIX_ALPHA_SIZE; ++i)
        {
            child_ptrs[i] = nullptr;
        }
    }

    // text_radix_node(const text_radix_node&) — deleted (deep copy is
    // handled at the tree level via m_copy_subtree).
    text_radix_node(const text_radix_node&)            = delete;
    text_radix_node& operator=(const text_radix_node&) = delete;
};

// ==========================================================================
// text_radix_tree — runtime base class
// ==========================================================================

// text_radix_tree
//   class: compressed string trie mapping std::string_view keys to
//   _Value. Each node carries a partial edge label; on insertion, edges
//   are split at the point of first divergence. Flag axes control
//   mutability, iteration ordering, and storage strategy. Mutating
//   methods are SFINAE-gated on the relevant compile-time booleans.
//
//   Storage strategy (controlled by k_fixed):
//     k_fixed == true  — static node pool of _Capacity nodes allocated
//                        on the stack; insertion throws std::bad_alloc
//                        when the pool is exhausted.
//     k_fixed == false — each node allocated individually via operator
//                        new; tree grows without a hard cap.
template<typename         _Value,
         std::size_t      _Capacity = 0,
         DContainerOption _Flags    = DContainerOption::none>
class text_radix_tree
{
private:
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
        "compile_time mutability is handled by text_radix_tree_ct.");

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

    // ---------------------------------------------------------------
    // Node type and storage strategy
    // ---------------------------------------------------------------
    using node_type = text_radix_node<_Value>;

    // fixed_node_pool — stack-allocated slab of _Capacity nodes.
    // The used[] bitfield tracks which slots are live.
    struct fixed_node_pool
    {
        node_type   nodes[_Capacity > 0u ? _Capacity : 1u];
        bool        used[_Capacity > 0u ? _Capacity : 1u];
        std::size_t count;
    };

    // dynamic_storage — heap-allocated; only the root pointer is held.
    struct dynamic_storage
    {
        node_type*  root;
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
    using key_type        = std::string_view;
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
    static constexpr bool is_writable  = k_writable;
    static constexpr bool is_immutable = k_immutable;
    static constexpr bool is_ordered   = k_ordered;
    static constexpr bool is_fixed     = k_fixed;
    static constexpr bool is_dynamic   = k_dynamic;

    // ---------------------------------------------------------------
    // Constructors and destructor
    // ---------------------------------------------------------------

    text_radix_tree() noexcept;
    text_radix_tree(const text_radix_tree& _other);
    text_radix_tree(text_radix_tree&& _other) noexcept;

    ~text_radix_tree();

    // ---------------------------------------------------------------
    // Assignment operators
    // ---------------------------------------------------------------
    text_radix_tree& operator=(const text_radix_tree& _other);
    text_radix_tree& operator=(text_radix_tree&& _other) noexcept;

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

    // at — returns a const reference; throws std::out_of_range when
    // _key is not present.
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
    //   Returns true when a new terminal was created (false on update).
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
    //   Returns true when a terminal was found and removed. Merges
    //   single-child non-terminal nodes after removal when possible.
    template<bool _W = k_writable,
             typename std::enable_if<_W, int>::type = 0>
    bool erase(key_type _key);

    // clear — removes all keys; the root is reset to empty.
    template<bool _W = k_writable,
             typename std::enable_if<_W, int>::type = 0>
    void clear() noexcept;

    // ---------------------------------------------------------------
    // Prefix queries — always available (read-only)
    // ---------------------------------------------------------------

    // has_prefix — true when at least one stored key begins with
    // _prefix.
    bool has_prefix(key_type _prefix) const noexcept;

    // count_with_prefix — number of stored keys that begin with
    // _prefix. O(keys with the prefix + internal nodes visited).
    size_type count_with_prefix(key_type _prefix) const noexcept;

    // longest_match — returns the length of the longest stored key
    // that is a prefix of _key. Returns 0 when no prefix exists.
    size_type longest_match(key_type _key) const noexcept;

private:
    // ---------------------------------------------------------------
    // Storage and count
    // ---------------------------------------------------------------
    storage_type m_store;
    size_type    m_count;

    // ---------------------------------------------------------------
    // Internal helper declarations
    // ---------------------------------------------------------------
    node_type* m_root_ptr()  const noexcept;

    // m_allocate_node — acquires one node from the pool or heap.
    //   Throws std::bad_alloc when the fixed pool is exhausted.
    node_type* m_allocate_node(std::string_view _edge,
                               bool             _is_terminal);

    // m_free_node — returns _node to the pool or calls delete.
    void m_free_node(node_type* _node) noexcept;

    // m_destroy_subtree — recursively frees _node and all descendants.
    void m_destroy_subtree(node_type* _node) noexcept;

    // m_copy_subtree — deep-copies the subtree rooted at _src.
    //   Sets parent pointers correctly. Returns the new root.
    node_type* m_copy_subtree(const node_type* _src,
                              node_type*       _parent);

    // m_find_node — descends the trie for _key.
    //   Returns the terminal node when found, nullptr otherwise.
    node_type* m_find_node(key_type _key) const noexcept;

    // m_find_prefix_root — descends to the deepest node that covers
    //   _prefix, returning nullptr when no such node exists.
    node_type* m_find_prefix_root(key_type _prefix) const noexcept;

    // m_count_terminals — counts terminal nodes in the subtree rooted
    //   at _node using an iterative depth-first traversal.
    size_type m_count_terminals(const node_type* _node) const noexcept;

    // m_split_edge — splits the edge of _node at byte position
    //   _split_pos, inserting a new intermediate node whose edge is
    //   _node->edge[0.._split_pos) and reconnecting the child.
    //   Returns the newly created intermediate node.
    node_type* m_split_edge(node_type*       _node,
                            std::size_t      _split_pos,
                            std::string_view _remaining_key);

    // m_insert_impl — shared implementation for both insert overloads.
    template<typename _FwdValue>
    bool m_insert_impl(key_type _key, _FwdValue&& _value);
};

// ==========================================================================
// Named subtypes via type aliases
// ==========================================================================

// dyn_text_radix_tree
//   type: dynamic, writable, unordered text radix tree (the default).
//   Nodes are heap-allocated and the tree grows without bound.
template<typename _Value>
using dyn_text_radix_tree = text_radix_tree<
    _Value, 0,
    ( DContainerOption::writable     |
      DContainerOption::unordered    |
      DContainerOption::dynamic_size )>;

// fixed_text_radix_tree
//   type: writable, unordered text radix tree backed by a fixed node
//   pool of _N slots. Insertion throws when the pool is exhausted.
template<typename    _Value,
         std::size_t _N>
using fixed_text_radix_tree = text_radix_tree<
    _Value, _N,
    ( DContainerOption::writable  |
      DContainerOption::unordered |
      DContainerOption::fixed_size )>;

// immutable_text_radix_tree
//   type: heap-allocated, immutable (read-only) text radix tree.
//   All mutating methods are SFINAE-removed. Useful for lookup-only
//   keyword tables and read-only routing structures.
template<typename _Value>
using immutable_text_radix_tree = text_radix_tree<
    _Value, 0,
    ( DContainerOption::immutable    |
      DContainerOption::unordered    |
      DContainerOption::dynamic_size )>;

// ordered_text_radix_tree
//   type: dynamic, writable, lexicographically ordered text radix tree.
//   Iteration visits terminals in lexicographic (sorted) key order.
template<typename _Value>
using ordered_text_radix_tree = text_radix_tree<
    _Value, 0,
    ( DContainerOption::writable     |
      DContainerOption::ordered      |
      DContainerOption::dynamic_size )>;

// ==========================================================================
// Deduction guides
// ==========================================================================

// Deduction guide: text_radix_tree{} with explicit value type deduces
// dyn_text_radix_tree<_Value>.
template<typename _Value>
text_radix_tree(_Value) -> dyn_text_radix_tree<_Value>;

}   // djinterp
