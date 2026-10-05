/*******************************************************************************
* djinterp [core]                                          binary_radix_tree.hpp
*
* Compressed binary trie (PATRICIA):
*   Maps an integral or pointer Key onto Value, descending BIT BY BIT from
* the most significant.  Each node records bit_index -- how many bits of the
* address it stands for -- so the edge from a parent to a child covers the bit
* range [parent.bit_index, child.bit_index), and a run of bits with nothing to
* choose between is SKIPPED rather than walked.  That skip is the compression.
*
*   It is the text tree with an alphabet of two, and every line of the theory
* carries over unchanged.
*
* ============================================================================
* WHAT CHANGED
* ============================================================================
*   1. IT HAD NO IMPLEMENTATION.  Every method was declared, none defined.
*
*   2. THE NODE FAILED ITS OWN NODE TRAIT -- same as the text node. It carried
*      `using children = node_type*[2];` (a TYPE ALIAS) while
*    has_radix_children
*      probes for a DATA MEMBER named children; the member was child_ptrs. The
*      array is now the member.
*
*   3. mask_for_prefix COULD NOT COMPILE FOR A POINTER KEY.  It wrote
*      `~static_cast<Key>(0)` -- and you cannot complement a pointer, any
*    more
*      than you could shift one in bit_at.  Exactly the bug the trait/utility
*      mismatch produced before, in a second place.  All bit work now goes
*      through key_bits_t, so a pointer key is masked as its uintptr_t image.
*
* ============================================================================
* THE INVARIANTS  (the same two)
* ============================================================================
*   ADDRESS.  A node's address is the first bit_index bits of its key -- which
* is the concatenation of the edge labels from the root, each edge being a bit
* RANGE.  The root has bit_index 0: THE ROOT CONTRIBUTES NO LABEL, and its
* address is the empty word.  keys() recovers every stored address from the
* structure; round_trips() checks each one resolves.
*
*   COMPRESSION.  Every node is the root, or a full key (bit_index == the key
* width), or has BOTH children. A node with exactly one child stands for a run
* of bits that had nothing to choose between -- and should have been skipped.
* is_compressed() checks it.
*
*   And, as before, they are the same invariant.  Merging a chain concatenates
* two bit RANGES into one: [a,b) + [b,c) == [a,c).  The address is the
* concatenation, so the merge cannot move it.  The split is that theorem read
* backwards: you split at bit m == binary_prefix_length(key, node.key) -- THE
* MEET -- which is the one bit where re-bracketing costs nothing.
*
*
* path:      /inc/djinterp/core/container/tree/radix/binary_radix_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.29
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_RADIX_BINARY_RADIX_TREE_HPP
#define DJINTERP_CONTAINER_TREE_RADIX_BINARY_RADIX_TREE_HPP 1

// FLOOR, FOR NOW: below C++14 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../meta/hierarchical.hpp"   // the structure_category tag
#include "./radix_tree_common.hpp"
#include "./text_radix_tree.hpp"   // radix_storage_signals
// re_std
#include "../../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t


NS_DJINTERP


// ==========================================================================
//  binary_radix_node
// ==========================================================================

// binary_radix_node
//   struct: one node. `key` is a representative of everything in its subtree
// (they all agree on the first bit_index bits, which is exactly what makes it
// a representative), and bit_index says how many of those bits are its
// ADDRESS.
//
//   TWO CHILD SLOTS, ONE PER BIT VALUE. That is the separation structure: two
// children of one node cannot agree on the next bit, because there is one slot
// for each. mu_1 is unrepresentable to violate.
template<typename Key,
         typename Value,
         typename = enable_if_binary_key<Key>>
struct binary_radix_node
    : public radix_node_base<binary_radix_node<Key, Value>>
{
    using key_type   = Key;
    using value_type = Value;
    using node_type  = binary_radix_node<Key, Value>;

    // how many bits of `key` are this node's address
    std::size_t bit_index;

    // a representative key: every key below agrees with it on the first
    // bit_index bits
    Key key;

    radix_terminal<Value> terminal;

    // THE MEMBER has_radix_children PROBES FOR. It was called child_ptrs, with
    // a type alias named `children` beside it, so the trait never fired.
    node_type* children[radix_binary_branches];

    binary_radix_node() D_NOEXCEPT
        : radix_node_base<node_type>(),
          bit_index(0u),
          key(),
          terminal()
    {
        children[0] = nullptr;
        children[1] = nullptr;
    }

    binary_radix_node(
        std::size_t _bit_index,
        Key         _key
    ) D_NOEXCEPT
        : radix_node_base<node_type>(),
          bit_index(_bit_index),
          key(_key),
          terminal()
    {
        children[0] = nullptr;
        children[1] = nullptr;
    }

    binary_radix_node(const binary_radix_node&)            = delete;
    binary_radix_node& operator=(const binary_radix_node&) = delete;

    std::size_t
    child_count() const D_NOEXCEPT
    {
        return ( ((children[0] != nullptr) ? 1u : 0u) +
                 ((children[1] != nullptr) ? 1u : 0u) );
    }
};


// ==========================================================================
//  binary_radix_tree
// ==========================================================================

template<typename         Key,
         typename         Value,
         std::size_t      Capacity = 0,
         container_option Flags     = container_option::none,
         typename         KeyGuard = enable_if_binary_key<Key>>
class binary_radix_tree
    : public radix_storage_signals<
          container_option_has(
              radix_tree_option_resolve(Flags, Capacity),
              container_option::fixed_size),
          binary_radix_node<Key, Value>,
          Capacity>
{
private:
    static_assert(
        is_binary_key<Key>::value,
        "binary_radix_tree: Key must be an integral or pointer type.");

    static D_CONSTEXPR container_option k_resolved =
        radix_tree_option_resolve(Flags, Capacity);

    static_assert(
        container_option_axis_valid(Flags),
        "container_option: at most one flag per axis.");

    static_assert(
        !container_option_has(k_resolved, container_option::compile_time),
        "compile_time mutability is handled by binary_radix_tree_ct.");

    static D_CONSTEXPR bool k_writable =
        container_option_has(k_resolved, container_option::writable);
    static D_CONSTEXPR bool k_immutable =
        container_option_has(k_resolved, container_option::immutable);
    static D_CONSTEXPR bool k_ordered =
        container_option_has(k_resolved, container_option::ordered);
    static D_CONSTEXPR bool k_unordered =
        container_option_has(k_resolved, container_option::unordered);
    static D_CONSTEXPR bool k_fixed =
        container_option_has(k_resolved, container_option::fixed_size);
    static D_CONSTEXPR bool k_dynamic =
        container_option_has(k_resolved, container_option::dynamic_size);

    // k_key_bits
    //   the length of every address in this tree. Binary keys are FIXED WIDTH,
    // so unlike the text tree there is no key that is a proper prefix of
    // another: every stored key sits at bit_index == k_key_bits.
    static D_CONSTEXPR std::size_t k_key_bits = (sizeof(Key) * 8u);

    using node_type = binary_radix_node<Key, Value>;

    struct fixed_node_pool
    {
        node_type   nodes[(Capacity > 0u) ? Capacity : 1u];
        bool        used [(Capacity > 0u) ? Capacity : 1u];
        std::size_t count;
    };

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
    using value_type      = Value;
    using key_type        = Key;
    using pointer         = Value*;
    using const_pointer   = const Value*;
    using reference       = Value&;
    using const_reference = const Value&;
    using difference_type = std::ptrdiff_t;
    using size_type       = std::size_t;

    using options_type = container_option;
    static D_CONSTEXPR container_option option_flags = k_resolved;

    static D_CONSTEXPR bool        is_writable  = k_writable;
    static D_CONSTEXPR bool        is_immutable = k_immutable;
    static D_CONSTEXPR bool        is_ordered   = k_ordered;
    static D_CONSTEXPR bool        is_fixed     = k_fixed;
    static D_CONSTEXPR bool        is_dynamic   = k_dynamic;
    static D_CONSTEXPR std::size_t key_bits     = k_key_bits;

    static D_CONSTEXPR container_structure axis_structure =
        radix_axis_structure;
    static D_CONSTEXPR container_multiplicity axis_multiplicity =
        radix_axis_multiplicity;


    // ------------------------------------------------------------------
    //  bit helpers
    // ------------------------------------------------------------------

    // key_bit_at
    //   the label at bit position _pos of _key's address (0 = MSB).
    static bool
    key_bit_at(key_type _key, std::size_t _pos) D_NOEXCEPT
    {
        return bit_at(_key, _pos);
    }

    // shared_prefix_bits
    //   THE MEET.  |lcp(a, b)| in bits -- the greatest lower bound of the two
    // addresses in the prefix order, and the place an edge splits.
    static std::size_t
    shared_prefix_bits(key_type _a, key_type _b) D_NOEXCEPT
    {
        return binary_prefix_length(_a, _b, k_key_bits);
    }

    // mask_for_prefix
    //   the mask keeping the top _prefix_len bits.
    //
    //   IT USED TO WRITE `~static_cast<Key>(0)`, which does not compile for a
    // POINTER key -- you cannot complement a pointer. Exactly the bug bit_at
    // had. The mask is computed on key_bits_t, the key's unsigned image.
    static key_bits_t<Key>
    mask_for_prefix(std::size_t _prefix_len) D_NOEXCEPT
    {
        using bits = key_bits_t<Key>;

        if (_prefix_len == 0u)
        {
            return static_cast<bits>(0);
        }

        if (_prefix_len >= k_key_bits)
        {
            return static_cast<bits>(~static_cast<bits>(0));
        }

        return static_cast<bits>(
            (~static_cast<bits>(0)) << (k_key_bits - _prefix_len));
    }


    // ------------------------------------------------------------------
    //  construction
    // ------------------------------------------------------------------

    binary_radix_tree()
        : m_store(),
          m_count(0u)
    {
        m_init_root();
    }

    binary_radix_tree(const binary_radix_tree& _other)
        : m_store(),
          m_count(0u)
    {
        m_init_root();

        m_copy_children(_other.m_root_ptr(), m_root_ptr());

        m_count = _other.m_count;
    }

    binary_radix_tree(binary_radix_tree&& _other) D_NOEXCEPT
        : m_store(),
          m_count(0u)
    {
        m_init_root();

        m_move_from(static_cast<binary_radix_tree&&>(_other));
    }

    ~binary_radix_tree()
    {
        m_destroy_all();
    }

    binary_radix_tree&
    operator=(const binary_radix_tree& _other)
    {
        if (this != &_other)
        {
            m_destroy_children(m_root_ptr());

            m_count = 0u;

            m_copy_children(_other.m_root_ptr(), m_root_ptr());

            m_count = _other.m_count;
        }

        return *this;
    }

    binary_radix_tree&
    operator=(binary_radix_tree&& _other) D_NOEXCEPT
    {
        if (this != &_other)
        {
            m_destroy_children(m_root_ptr());

            m_count = 0u;

            m_move_from(static_cast<binary_radix_tree&&>(_other));
        }

        return *this;
    }


    // ------------------------------------------------------------------
    //  capacity
    // ------------------------------------------------------------------

    size_type size()  const D_NOEXCEPT { return m_count; }
    bool      empty() const D_NOEXCEPT { return (m_count == 0u); }

    // capacity -- FIXED TREES ONLY (see radix_storage_signals): a dynamic tree
    // that answers capacity() is read as BOUNDED by bounded_container_traits.
    template<bool F = k_fixed,
             typename std::enable_if<F, int>::type = 0>
    size_type capacity() const D_NOEXCEPT { return Capacity; }


    // ------------------------------------------------------------------
    //  lookup
    // ------------------------------------------------------------------

    const_pointer
    find(key_type _key) const D_NOEXCEPT
    {
        const node_type* n = m_find_node(_key);

        if (n == nullptr || !n->terminal.present)
        {
            return nullptr;
        }

        return &n->terminal.value;
    }

    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    pointer
    find(key_type _key) D_NOEXCEPT
    {
        node_type* n = m_find_node(_key);

        if (n == nullptr || !n->terminal.present)
        {
            return nullptr;
        }

        return &n->terminal.value;
    }

    bool
    contains(key_type _key) const D_NOEXCEPT
    {
        return (find(_key) != nullptr);
    }

    const_reference
    at(key_type _key) const
    {
        const_pointer p = find(_key);

        if (p == nullptr)
        {
            throw std::out_of_range("binary_radix_tree::at: key not found");
        }

        return *p;
    }

    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    reference
    at(key_type _key)
    {
        pointer p = find<true>(_key);

        if (p == nullptr)
        {
            throw std::out_of_range("binary_radix_tree::at: key not found");
        }

        return *p;
    }


    // ------------------------------------------------------------------
    //  insertion  --  SPLIT AT THE MEET
    // ------------------------------------------------------------------

    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    bool
    insert(key_type _key, const value_type& _value)
    {
        return m_insert_impl(_key, _value, true);
    }

    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    bool
    insert(key_type _key, value_type&& _value)
    {
        return m_insert_impl(_key, static_cast<value_type&&>(_value), true);
    }

    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    bool
    try_insert(key_type _key, const value_type& _value)
    {
        return m_insert_impl(_key, _value, false);
    }


    // ------------------------------------------------------------------
    //  erasure  --  MERGE BACK
    // ------------------------------------------------------------------

    // erase
    //   removes the key, then restores compression: the parent of a dropped
    // leaf is left with one child, which means it stands for a run of bits
    // with nothing to choose between -- so it goes, and its child's edge
    // absorbs the range. [a,b) + [b,c) == [a,c): the address does not move.
    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    bool
    erase(key_type _key)
    {
        node_type* n = m_find_node(_key);

        if (n == nullptr || !n->terminal.present)
        {
            return false;
        }

        node_type* parent = n->parent;

        // a full key is always a leaf here (fixed-width keys: nothing extends
        // one), so removing it removes the node.
        std::size_t b =
            ( (parent->children[0] == n) ? 0u : 1u );

        parent->children[b] = nullptr;

        m_free_node(n);

        --m_count;

        m_recompress(parent);

        return true;
    }

    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    void
    clear() D_NOEXCEPT
    {
        m_destroy_children(m_root_ptr());

        m_count = 0u;

        return;
    }




    // ------------------------------------------------------------------
    //  AXIS DECLARATIONS  --  what the structural probes cannot see
    // ------------------------------------------------------------------
    //   Three axes read this container WRONG, and on each the framework already
    // supplies the opt-in.  Measured against std::map (the reference ordered
    // associative), the tree agreed on capacity, storage, memory, order,
    // iteration and mutability -- and disagreed on exactly these three.

    // SORTEDNESS. sortedness_of said `unordered` -- "no comparator, sortedness
    // not applicable". But sorted enumeration is the DEFINING PROPERTY of a
    // radix tree: rank is monotone in the label, so the packed children are in
    // label order, so a preorder walk emits the keys sorted. Nothing sorts
    // them. std::map declares key_compare and reads `monotone`; so must this.
    // Without it, admits_sorted_enumeration was FALSE -- the one thing the
    // structure is for was invisible.
    using key_compare = std::less<key_type>;

    // MULTIPLICITY. multiplicity_kind_of fell through to `unbounded_multiset`:
    // it probes insert(value_type) for a pair<iterator,bool>, and a radix
    // tree's insert takes (key, value) -- two arguments -- so the probe cannot
    // fire. The container was therefore claiming it admits DUPLICATE KEYS. It
    // cannot: the key IS the address, one node per address, and insert
    // OVERWRITES. m = 1. The static `multiplicity` constant is the documented
    // authoritative override for exactly this -- a bound with no structural
    // tell.
    static D_CONSTEXPR std::size_t multiplicity = 1u;

    // STRUCTURE. structure_kind_of said `flat`. has_node_summand wants a
    // node_type that is ITSELF container-shaped, and container_depth walks the
    // value_type chain -- which here is just Value, so depth 1. Neither probe
    // can see that a TRIE NESTS. This is precisely what the hierarchical tag
    // is documented for: "asserts nesting the chain cannot expose".
    using structure_category = hierarchical;

    // ------------------------------------------------------------------
    //  ITERATION  --  address order is numeric order
    // ------------------------------------------------------------------
    // Same gap, same fix as the text tree: without begin()/end() the container
    // is invisible to is_copyable_container, the filter traits, the conversion
    // traits, and everything else that walks elements. DFS preorder over
    // children[0] then children[1] -- bit 0 before bit 1 -- which for MSB-first
    // keys IS unsigned numeric order. Nothing is sorted.
    class const_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = Value;
        using reference         = const Value&;
        using pointer           = const Value*;
        using difference_type   = std::ptrdiff_t;

        const_iterator() D_NOEXCEPT : m_root(nullptr), m_cur(nullptr) {}
        const_iterator(const node_type* _r, const node_type* _c) D_NOEXCEPT
            : m_root(_r), m_cur(_c) {}

        reference
        operator*() const D_NOEXCEPT
        { return m_cur->terminal.value; }

        pointer
        operator->() const D_NOEXCEPT
        { return &m_cur->terminal.value; }

        // key -- the address. A binary node carries its own key, so unlike the
        // text tree there is nothing to reassemble: bit_index == k_key_bits
        // says every bit of it is meaningful.
        key_type key() const D_NOEXCEPT { return m_cur->key; }

        const_iterator&
        operator++()
        {
            do
            {
                m_cur = m_next_dfs(m_cur, m_root);
            }
            while (m_cur != nullptr && !m_cur->terminal.present);

            return *this;
        }

        const_iterator operator++(int)
        { const_iterator t = *this; ++(*this); return t; }

        bool operator==(const const_iterator& _o) const D_NOEXCEPT
        { return (m_cur == _o.m_cur); }
        bool operator!=(const const_iterator& _o) const D_NOEXCEPT
        { return (m_cur != _o.m_cur); }

    private:
        const node_type* m_root;
        const node_type* m_cur;

        static const node_type*
        m_next_dfs(const node_type* _n, const node_type* _root) D_NOEXCEPT
        {
            if (_n == nullptr)
            {
                return nullptr;
            }

            if (_n->children[0] != nullptr) { return _n->children[0]; }
            if (_n->children[1] != nullptr) { return _n->children[1]; }

            while (_n != _root)
            {
                const node_type* p = _n->parent;

                if (p->children[0] == _n && p->children[1] != nullptr)
                {
                    return p->children[1];
                }

                _n = p;
            }

            return nullptr;
        }
    };

    using iterator = const_iterator;

    const_iterator
    begin() const D_NOEXCEPT
    {
        const node_type* r = m_root_ptr();

        if (r->terminal.present)
        {
            return const_iterator(r, r);
        }

        const_iterator i(r, r);

        ++i;

        return i;
    }

    const_iterator end()    const D_NOEXCEPT
    { return const_iterator(m_root_ptr(), nullptr); }
    const_iterator cbegin() const D_NOEXCEPT { return begin(); }
    const_iterator cend()   const D_NOEXCEPT { return end(); }

    // ------------------------------------------------------------------
    //  THE INVARIANTS
    // ------------------------------------------------------------------

    // keys
    //   every stored key, recovered from the structure, in address order
    // (which for MSB-first bits is unsigned numeric order).
    std::vector<key_type>
    keys() const
    {
        std::vector<key_type> out;

        m_collect_keys(m_root_ptr(), out);

        return out;
    }

    // round_trips
    //   find(key_of(n)) == n, for every n.
    bool
    round_trips() const D_NOEXCEPT
    {
        std::vector<key_type> ks = keys();

        if (ks.size() != m_count)
        {
            return false;
        }

        for (std::size_t i = 0u; i < ks.size(); ++i)
        {
            if (find(ks[i]) == nullptr)
            {
                return false;
            }
        }

        return true;
    }

    // is_compressed
    //   every node is the root, a full key, or has BOTH children. A node with
    // exactly one child is a run of bits with nothing to choose between.
    bool
    is_compressed() const D_NOEXCEPT
    {
        return m_check_compressed(m_root_ptr(), true);
    }

    size_type
    node_count() const D_NOEXCEPT
    {
        return m_count_nodes(m_root_ptr());
    }


private:
    storage_type m_store;
    size_type    m_count;

    // ---- root --------------------------------------------------------

    node_type*
    m_root_ptr() const D_NOEXCEPT
    {
        return m_root_ptr_impl(
            typename std::integral_constant<bool, k_fixed>::type());
    }

    node_type*
    m_root_ptr_impl(std::true_type) const D_NOEXCEPT
    {
        return const_cast<node_type*>(&m_store.nodes[0]);
    }

    node_type*
    m_root_ptr_impl(std::false_type) const D_NOEXCEPT
    {
        return m_store.head;
    }

    void
    m_init_root()
    {
        m_init_root_impl(
            typename std::integral_constant<bool, k_fixed>::type());
    }

    void
    m_init_root_impl(std::true_type)
    {
        for (std::size_t i = 0u;
             i < ((Capacity > 0u) ? Capacity : 1u);
             ++i)
        {
            m_store.used[i] = false;
        }

        m_store.used[0] = true;
        m_store.count   = 1u;

        node_type& r = m_store.nodes[0];

        r.bit_index   = 0u;
        r.key         = key_type();
        r.terminal    = radix_terminal<Value>();
        r.is_terminal = false;
        r.parent      = nullptr;
        r.children[0] = nullptr;
        r.children[1] = nullptr;

        return;
    }

    void
    m_init_root_impl(std::false_type)
    {
        m_store.head  = new node_type();
        m_store.count = 1u;

        return;
    }

    // ---- allocation --------------------------------------------------

    node_type*
    m_allocate_node(std::size_t _bit_index, key_type _key)
    {
        return m_allocate_node_impl(
            _bit_index, _key,
            typename std::integral_constant<bool, k_fixed>::type());
    }

    node_type*
    m_allocate_node_impl(
        std::size_t    _bit_index,
        key_type       _key,
        std::true_type
    )
    {
        for (std::size_t i = 1u; i < Capacity; ++i)
        {
            if (!m_store.used[i])
            {
                node_type& n = m_store.nodes[i];

                n.bit_index   = _bit_index;
                n.key         = _key;
                n.terminal    = radix_terminal<Value>();
                n.is_terminal = false;
                n.parent      = nullptr;
                n.children[0] = nullptr;
                n.children[1] = nullptr;

                m_store.used[i] = true;
                ++m_store.count;

                return &n;
            }
        }

        throw std::bad_alloc();
    }

    node_type*
    m_allocate_node_impl(
        std::size_t     _bit_index,
        key_type        _key,
        std::false_type
    )
    {
        node_type* n = new node_type(_bit_index, _key);

        ++m_store.count;

        return n;
    }

    void
    m_free_node(node_type* _node) D_NOEXCEPT
    {
        m_free_node_impl(
            _node, typename std::integral_constant<bool, k_fixed>::type());
    }

    void
    m_free_node_impl(node_type* _node, std::true_type) D_NOEXCEPT
    {
        std::size_t i =
            static_cast<std::size_t>(_node - &m_store.nodes[0]);

        if (i == 0u)
        {
            return;
        }

        _node->terminal    = radix_terminal<Value>();
        _node->is_terminal = false;
        _node->parent      = nullptr;
        _node->children[0] = nullptr;
        _node->children[1] = nullptr;

        m_store.used[i] = false;
        --m_store.count;
    }

    void
    m_free_node_impl(node_type* _node, std::false_type) D_NOEXCEPT
    {
        delete _node;
        --m_store.count;
    }

    // ---- teardown ----------------------------------------------------

    void
    m_destroy_children(node_type* _node) D_NOEXCEPT
    {
        for (std::size_t i = 0u; i < radix_binary_branches; ++i)
        {
            if (_node->children[i] != nullptr)
            {
                m_destroy_subtree(_node->children[i]);
                _node->children[i] = nullptr;
            }
        }
    }

    void
    m_destroy_subtree(node_type* _node) D_NOEXCEPT
    {
        if (_node == nullptr)
        {
            return;
        }

        m_destroy_children(_node);
        m_free_node(_node);
    }

    void
    m_destroy_all() D_NOEXCEPT
    {
        node_type* r = m_root_ptr();

        if (r == nullptr)
        {
            return;
        }

        m_destroy_children(r);

        m_destroy_root_impl(
            typename std::integral_constant<bool, k_fixed>::type());
    }

    void m_destroy_root_impl(std::true_type)  D_NOEXCEPT { }
    void m_destroy_root_impl(std::false_type) D_NOEXCEPT
    {
        delete m_store.head;
        m_store.head = nullptr;
    }

    // ---- copy / move -------------------------------------------------

    void
    m_copy_children(const node_type* _src, node_type* _dst)
    {
        for (std::size_t i = 0u; i < radix_binary_branches; ++i)
        {
            if (_src->children[i] == nullptr)
            {
                continue;
            }

            _dst->children[i] = m_copy_subtree(_src->children[i], _dst);
        }
    }

    node_type*
    m_copy_subtree(const node_type* _src, node_type* _parent)
    {
        node_type* n = m_allocate_node(_src->bit_index, _src->key);

        n->terminal    = _src->terminal;
        n->is_terminal = _src->is_terminal;
        n->parent      = _parent;

        m_copy_children(_src, n);

        return n;
    }

    void
    m_move_from(binary_radix_tree&& _other)
    {
        m_move_children_impl(
            _other.m_root_ptr(), m_root_ptr(), _other,
            typename std::integral_constant<bool, k_fixed>::type());

        m_count = _other.m_count;

        _other.m_destroy_children(_other.m_root_ptr());
        _other.m_count = 0u;
    }

    void
    m_move_children_impl(
        node_type*         _src,
        node_type*         _dst,
        binary_radix_tree& /*_other*/,
        std::true_type     /*fixed: in-object, cannot steal*/
    )
    {
        m_copy_children(_src, _dst);
    }

    void
    m_move_children_impl(
        node_type*         _src,
        node_type*         _dst,
        binary_radix_tree& _other,
        std::false_type    /*dynamic: steal*/
    )
    {
        for (std::size_t i = 0u; i < radix_binary_branches; ++i)
        {
            _dst->children[i] = _src->children[i];

            if (_dst->children[i] != nullptr)
            {
                _dst->children[i]->parent = _dst;
            }

            _src->children[i] = nullptr;
        }

        m_store.count += (_other.m_store.count - 1u);
        _other.m_store.count = 1u;
    }

    // ---- descent -----------------------------------------------------

    // m_find_node
    //   resolve: fold the descent step along the key's bits. At each node the
    // next bit picks the only child that could match; then the child's EDGE --
    // the bit range it skips -- must agree with the key, or the key is not
    // here.
    node_type*
    m_find_node(key_type _key) const D_NOEXCEPT
    {
        node_type* n = m_root_ptr();

        for (;;)
        {
            if (n->bit_index >= k_key_bits)
            {
                return ( (shared_prefix_bits(n->key, _key) >= k_key_bits)
                             ? n : nullptr );
            }

            std::size_t b = ( key_bit_at(_key, n->bit_index) ? 1u : 0u );

            node_type* child = n->children[b];

            if (child == nullptr)
            {
                return nullptr;
            }

            // the child's edge covers [n->bit_index, child->bit_index). It
            // must
            // agree with the key over that whole range, or the key diverges
            // from this branch and is nowhere.
            if (shared_prefix_bits(child->key, _key) < child->bit_index)
            {
                return nullptr;
            }

            n = child;
        }
    }

    // ---- insertion ---------------------------------------------------

    template<typename FwdValue>
    bool
    m_insert_impl(
        key_type    _key,
        FwdValue&& _value,
        bool        _overwrite
    )
    {
        node_type* n = m_root_ptr();

        for (;;)
        {
            if (n->bit_index >= k_key_bits)
            {
                // a full key. The descent can only have brought us here if it
                // agrees with _key on every bit.
                if (n->terminal.present)
                {
                    if (!_overwrite)
                    {
                        return false;
                    }

                    n->terminal.value = static_cast<FwdValue&&>(_value);

                    return false;
                }

                n->terminal.present = true;
                n->terminal.value   = static_cast<FwdValue&&>(_value);
                n->is_terminal      = true;

                ++m_count;

                return true;
            }

            std::size_t b = ( key_bit_at(_key, n->bit_index) ? 1u : 0u );

            node_type* child = n->children[b];

            if (child == nullptr)
            {
                // nothing on this branch: the whole remainder of the key
                // becomes ONE edge. Compression, on the way in.
                node_type* leaf = m_allocate_node(k_key_bits, _key);

                leaf->terminal.present = true;
                leaf->terminal.value   = static_cast<FwdValue&&>(_value);
                leaf->is_terminal      = true;
                leaf->parent           = n;

                n->children[b] = leaf;

                ++m_count;

                return true;
            }

            std::size_t m = shared_prefix_bits(child->key, _key);

            if (m >= child->bit_index)
            {
                n = child;   // the edge matched entirely: step past it

                continue;
            }

            // THE EDGE DIVERGES at bit m -- THE MEET. Split there.
            //
            //   before: n --[ .. child.bit )--> child(subtree)
            //   after: n --[ .. m )--> mid --[ m .. child.bit )--> child
            //                              \--[ m .. width )------> leaf
            //
            //   [n.bit, m) + [m, child.bit) == [n.bit, child.bit): the range
            // is
            //   re-bracketed, not changed, so child's address does not move.
            node_type* mid = m_allocate_node(m, child->key);

            mid->parent    = n;
            n->children[b] = mid;

            std::size_t cb = ( key_bit_at(child->key, m) ? 1u : 0u );
            std::size_t nb = ( cb == 0u ) ? 1u : 0u;

            mid->children[cb] = child;
            child->parent     = mid;

            node_type* leaf = m_allocate_node(k_key_bits, _key);

            leaf->terminal.present = true;
            leaf->terminal.value   = static_cast<FwdValue&&>(_value);
            leaf->is_terminal      = true;
            leaf->parent           = mid;

            mid->children[nb] = leaf;

            ++m_count;

            return true;
        }
    }

    // ---- the merge ---------------------------------------------------

    // m_recompress
    //   _node has just lost a child. If it is now a one-child waypoint, it
    // stands for a bit range with nothing to choose between: drop it, and let
    // its child hang from its parent. The child's bit_index and key do not
    // change, so its address does not change -- the two ranges simply become
    // one.
    void
    m_recompress(node_type* _node) D_NOEXCEPT
    {
        node_type* n = _node;

        while (n != nullptr &&
               n != m_root_ptr() &&
               !n->terminal.present &&
               n->child_count() == 1u)
        {
            node_type* parent = n->parent;
            node_type* kid    =
                ( (n->children[0] != nullptr) ? n->children[0]
                                              : n->children[1] );

            std::size_t b =
                ( (parent->children[0] == n) ? 0u : 1u );

            parent->children[b] = kid;
            kid->parent         = parent;

            n->children[0] = nullptr;
            n->children[1] = nullptr;

            m_free_node(n);

            n = parent;
        }
    }

    // ---- walks -------------------------------------------------------

    void
    m_collect_keys(
        const node_type*       _node,
        std::vector<key_type>& _out
    ) const
    {
        if (_node->terminal.present)
        {
            _out.push_back(_node->key);
        }

        for (std::size_t i = 0u; i < radix_binary_branches; ++i)
        {
            if (_node->children[i] != nullptr)
            {
                m_collect_keys(_node->children[i], _out);
            }
        }
    }

    size_type
    m_count_nodes(const node_type* _node) const D_NOEXCEPT
    {
        size_type n = 1u;

        for (std::size_t i = 0u; i < radix_binary_branches; ++i)
        {
            if (_node->children[i] != nullptr)
            {
                n += m_count_nodes(_node->children[i]);
            }
        }

        return n;
    }

    bool
    m_check_compressed(
        const node_type* _node,
        bool             _is_root
    ) const D_NOEXCEPT
    {
        if (!_is_root &&
            (_node->bit_index < k_key_bits) &&
            (_node->child_count() < 2u))
        {
            return false;   // a bit range with nothing to choose between
        }

        for (std::size_t i = 0u; i < radix_binary_branches; ++i)
        {
            if (_node->children[i] != nullptr)
            {
                if (!m_check_compressed(_node->children[i], false))
                {
                    return false;
                }
            }
        }

        return true;
    }
};


// ==========================================================================
//  named subtypes
// ==========================================================================

template<typename Key,
         typename Value>
using dyn_binary_radix_tree =
    binary_radix_tree<Key, Value, 0,
                      container_option::writable |
                      container_option::dynamic_size>;

template<typename    Key,
         typename    Value,
         std::size_t Capacity>
using fixed_binary_radix_tree =
    binary_radix_tree<Key, Value, Capacity,
                      container_option::writable |
                      container_option::fixed_size>;

// ip_radix_tree
//   a routing-table-shaped tree over 32-bit addresses.
template<typename Value>
using ip_radix_tree =
    binary_radix_tree<re_std::uint32_t, Value, 0,
                      container_option::writable |
                      container_option::dynamic_size>;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_RADIX_BINARY_RADIX_TREE_HPP
