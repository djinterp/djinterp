/*******************************************************************************
* djinterp [core]                                            text_radix_tree.hpp
*
* Compressed string trie (text radix tree):
*   Maps re_std::string_view keys onto Value.  Each node carries a COMPRESSED
* EDGE LABEL -- the run of bytes between it and its parent -- and a 256-slot
* child array indexed by the first byte of the remaining key.  On insertion an
* edge SPLITS at the point of first divergence; on erasure a chain of
* single-child nodes MERGES back.
*
* ============================================================================
* WHAT CHANGED
* ============================================================================
*   1. IT HAD NO IMPLEMENTATION.  Thirty-three methods were DECLARED and none
*      defined -- no out-of-class definitions anywhere, the only bodies in the
*      file being the node's own constructors.  It compiled (declarations do)
*      and could not link.  Implemented, in-class.
*
*   2. THE NODE FAILED ITS OWN NODE TRAIT.  It carried
*
*          using children = node_type*[D_RADIX_ALPHA_SIZE];   // a TYPE ALIAS
*
*      under a comment reading "Member type aliases required by is_radix_node
*      detection" -- but has_radix_children probes for a DATA MEMBER named
*      children, and the node's data member was called child_ptrs.  So
*      is_radix_node<text_radix_node<V>> was FALSE, and the comment had it
*      exactly backwards.  The array is now the member named `children`; the
*      trait fires.
*
*   3. std::string_view -> re_std::string_view, so the tree builds at C++14.
*
* ============================================================================
* THE TWO INVARIANTS
* ============================================================================
*   ADDRESS. A node's address is the CONCATENATION of the edge labels from the
* root down to it. The root's edge is empty -- THE ROOT CONTRIBUTES NO LABEL
* --
* so the root's address is the EMPTY KEY, and find("") is the root. That is
* the
* base of the induction, not a special case.  keys() recovers every stored
* address by walking and concatenating; round_trips() checks that each one
* finds
* its way home. The round trip is not a property this container has, it is
* what
* the container is.
*
*   COMPRESSION.  Every node is the root, or terminal, or has AT LEAST TWO
* CHILDREN.  A non-root, non-terminal node with exactly one child is a chain
* that should have been collapsed: its edge and its child's edge should be one
* edge.  is_compressed() checks it.
*
*   And the two invariants are the same invariant. Merging a chain
* concatenates
* two edge labels into one -- it RE-BRACKETS the concatenation without
* changing
* it -- and the address IS the concatenation.  So compression cannot move an
* address, which is exactly why it is allowed.  Splitting is the same theorem
* backwards: you split an edge at THE MEET, |lcp(key, edge)|, which is the one
* place where re-bracketing costs nothing.
*
*
* path:      /inc/djinterp/core/container/tree/radix/text_radix_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.29
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_RADIX_TEXT_RADIX_TREE_HPP
#define DJINTERP_CONTAINER_TREE_RADIX_TEXT_RADIX_TREE_HPP 1

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
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../meta/hierarchical.hpp"   // the structure_category tag
#include "./radix_tree_common.hpp"
// re_std
#include "../../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint16_t,
                                                      // uint64_t


NS_DJINTERP


// ==========================================================================
//  radix_child_map  --  SEPARATION, AS A CHARACTERISTIC FUNCTION
// ==========================================================================

NS_INTERNAL

    // popcount64
    //   the number of set bits. rank() is built on it, and rank is what turns
    // a sparse bitmap back into a dense index.
    D_CONSTEXPR_INLINE std::size_t
    popcount64(re_std::uint64_t _v) D_NOEXCEPT
    {
#if defined(__GNUC__) || defined(__clang__)
        return static_cast<std::size_t>(__builtin_popcountll(_v));
#else
        std::size_t n = 0u;

        while (_v != 0u)
        {
            _v = (_v & (_v - 1u));
            ++n;
        }

        return n;
#endif
    }

NS_END  // internal


// radix_child_map
//   struct: the child slots of a text node -- 256 possible labels, stored as a
// 256-BIT CHARACTERISTIC FUNCTION plus a densely packed array of only the
// children that exist.
//
//   THE SEPARATION STRUCTURE IS UNCHANGED.  It used to be 256 pointer slots,
// one per byte; it is now 256 BITS, one per byte.  Either way there is exactly
// one place for each label, so two children of one node cannot share a first
// byte -- mu_1 is still unrepresentable to violate, not merely unenforced.  The
// table became its own characteristic function, and that is the whole change.
//
//   RANK IS THE BRIDGE.  A byte's position in the packed array is the number of
// present children BELOW it: rank(c) = |{ b < c : b is present }|.  Two
// consequences fall straight out:
//
//     - the packed array is ALWAYS IN BYTE ORDER, because rank is monotone in
//       the byte.  keys() therefore still comes out sorted with no sorting, and
//       an insert at the high end is an append.
//     - every walk is O(fanout), not O(256).  child_count() and sole_child()
//       were 256-iteration scans; they are now O(1).
//
//   And a compressed trie's nodes are NARROW -- the compression invariant says
// an internal node has at least two children, and in practice it has about two.
// So the first k_inline of them live in the object and the vast majority of
// nodes never allocate a slot array at all.
template<typename Node>
struct radix_child_map
{
    // 256 labels / 64 bits per word
    static D_CONSTEXPR std::size_t k_words = 4u;

    // children held in-object before spilling to the heap
    static D_CONSTEXPR std::size_t k_inline = 4u;

    // THE CHARACTERISTIC FUNCTION. Bit c is set iff a child's edge begins with
    // byte c. One bit per label: separation, in 32 bytes.
    re_std::uint64_t bits[k_words];

    // the packed children, in byte order.  Inline while there are few.
    Node*            inln[k_inline];
    Node**           spill;
    re_std::uint16_t count;
    re_std::uint16_t cap;

    radix_child_map() D_NOEXCEPT
        : spill(nullptr),
          count(0u),
          cap(0u)
    {
        for (std::size_t i = 0u; i < k_words; ++i)
        {
            bits[i] = 0u;
        }

        for (std::size_t i = 0u; i < k_inline; ++i)
        {
            inln[i] = nullptr;
        }
    }

    ~radix_child_map()
    {
        delete[] spill;
    }

    radix_child_map(const radix_child_map&)            = delete;
    radix_child_map& operator=(const radix_child_map&) = delete;

    // slots
    //   the packed array, wherever it currently lives.
    Node**
    slots() D_NOEXCEPT
    {
        return ( (spill != nullptr) ? spill : inln );
    }

    Node* const*
    slots() const D_NOEXCEPT
    {
        return ( (spill != nullptr) ? spill : inln );
    }

    std::size_t size()  const D_NOEXCEPT { return count; }
    bool        empty() const D_NOEXCEPT { return (count == 0u); }

    // test
    //   is byte _c present? One bit lookup.
    bool
    test(unsigned char _c) const D_NOEXCEPT
    {
        return ( ( bits[_c >> 6] >>
                   static_cast<re_std::uint64_t>(_c & 63u) ) &
                 static_cast<re_std::uint64_t>(1) ) != 0u;
    }

    // rank
    //   how many present children come BEFORE byte _c. This is the packed
    // index, and its monotonicity is why the array stays sorted.
    std::size_t
    rank(unsigned char _c) const D_NOEXCEPT
    {
        std::size_t w = static_cast<std::size_t>(_c >> 6);
        std::size_t r = 0u;

        for (std::size_t i = 0u; i < w; ++i)
        {
            r += internal::popcount64(bits[i]);
        }

        unsigned b = static_cast<unsigned>(_c & 63u);

        // a shift by 64 is undefined; b == 0 means no bits below it in the word
        re_std::uint64_t mask =
            ( (b == 0u)
                  ? static_cast<re_std::uint64_t>(0)
                  : ( (~static_cast<re_std::uint64_t>(0)) >> (64u - b) ) );

        r += internal::popcount64(bits[w] & mask);

        return r;
    }

    // get
    //   the child whose edge begins with byte _c, or null.
    Node*
    get(unsigned char _c) const D_NOEXCEPT
    {
        return ( test(_c) ? slots()[rank(_c)] : nullptr );
    }

    // sole
    //   the only child, or null when there is not exactly one. O(1) now; it
    // used to scan 256 slots.
    Node*
    sole() const D_NOEXCEPT
    {
        return ( (count == 1u) ? slots()[0] : nullptr );
    }

    // set
    //   attach _p under byte _c, inserting or overwriting. Insertion keeps the
    // packed array in byte order by construction.
    void
    set(unsigned char _c, Node* _p)
    {
        std::size_t r = rank(_c);

        if (test(_c))
        {
            slots()[r] = _p;   // overwrite: the label was already taken

            return;
        }

        m_reserve(static_cast<std::size_t>(count) + 1u);

        Node** s = slots();

        for (std::size_t i = count; i > r; --i)
        {
            s[i] = s[i - 1u];
        }

        s[r] = _p;

        bits[_c >> 6] |=
            ( static_cast<re_std::uint64_t>(1) <<
              static_cast<re_std::uint64_t>(_c & 63u) );

        ++count;

        return;
    }

    // erase
    //   detach byte _c. The array closes up, staying packed and sorted.
    void
    erase(unsigned char _c) D_NOEXCEPT
    {
        if (!test(_c))
        {
            return;
        }

        std::size_t r = rank(_c);
        Node**     s = slots();

        for (std::size_t i = r; (i + 1u) < count; ++i)
        {
            s[i] = s[i + 1u];
        }

        bits[_c >> 6] &=
            ~( static_cast<re_std::uint64_t>(1) <<
               static_cast<re_std::uint64_t>(_c & 63u) );

        --count;

        return;
    }

    // clear
    //   drop every label. Does not free the spill array -- the node may be
    // about to be refilled from a pool slot.
    void
    clear() D_NOEXCEPT
    {
        for (std::size_t i = 0u; i < k_words; ++i)
        {
            bits[i] = 0u;
        }

        count = 0u;

        return;
    }

    // adopt
    //   take everything from _o, leaving it empty. Used by the edge split (the
    // whole subtree moves down) and by the move constructor.
    void
    adopt(radix_child_map& _o) D_NOEXCEPT
    {
        delete[] spill;

        for (std::size_t i = 0u; i < k_words; ++i)
        {
            bits[i]   = _o.bits[i];
            _o.bits[i] = 0u;
        }

        count = _o.count;

        if (_o.spill != nullptr)
        {
            spill = _o.spill;   // steal the array
            cap   = _o.cap;
        }
        else
        {
            spill = nullptr;
            cap   = 0u;

            for (std::size_t i = 0u; i < _o.count; ++i)
            {
                inln[i] = _o.inln[i];
            }
        }

        _o.spill = nullptr;
        _o.cap   = 0u;
        _o.count = 0u;

        return;
    }

private:
    // m_reserve
    //   grow the packed array to hold _n children, spilling out of the object
    // when it no longer fits.
    void
    m_reserve(std::size_t _n)
    {
        if (_n <= k_inline && spill == nullptr)
        {
            return;   // still fits in the object
        }

        if (spill != nullptr && _n <= cap)
        {
            return;
        }

        std::size_t ncap = ( (cap != 0u) ? (cap * 2u) : (k_inline * 2u) );

        while (ncap < _n)
        {
            ncap *= 2u;
        }

        if (ncap > radix_alpha_size)
        {
            ncap = radix_alpha_size;
        }

        Node** fresh = new Node*[ncap];
        Node** old   = slots();

        for (std::size_t i = 0u; i < count; ++i)
        {
            fresh[i] = old[i];
        }

        delete[] spill;

        spill = fresh;
        cap   = static_cast<re_std::uint16_t>(ncap);

        return;
    }
};


// ==========================================================================
//  text_radix_node
// ==========================================================================

// text_radix_node
//   struct: one node. Carries the compressed edge label from its parent, an
// optional terminal value, and 256 child slots indexed by the first byte of
// the remaining key.
//
//   THE CHILD ARRAY IS THE SEPARATION STRUCTURE. One slot per byte means two
// children of one node cannot begin with the same byte -- mu_1 is not enforced
// here, it is unrepresentable to violate. That is why a descent never
// backtracks: the next byte of the key picks the only child that could match.
template<typename Value>
struct text_radix_node
    : public radix_node_base<text_radix_node<Value>>
{
    using value_type = Value;
    using node_type  = text_radix_node<Value>;

    // the compressed edge label from the parent to this node. The root's is
    // EMPTY -- the root contributes no label.
    std::string edge;

    // the mapped value.  present == false at a waypoint.
    radix_terminal<Value> terminal;

    // THE MEMBER has_radix_children PROBES FOR. It used to be called
    // child_ptrs, with a *type alias* named `children` beside it -- so the
    // trait never fired and is_radix_node<text_radix_node<V>> was false.
    //   It used to be 256 POINTER SLOTS -- 2048 bytes, 97% of the node,
    // whether the node had two children or none. It is now 256 BITS plus only
    // the children that exist. Same separation, same one-place-per-label: the
    // lookup table simply became its own characteristic function.
    radix_child_map<node_type> children;

    text_radix_node()
        : radix_node_base<node_type>(),
          edge(),
          terminal(),
          children()
    {}

    explicit
    text_radix_node(
        re_std::string_view _edge_label,
        bool               _is_terminal = false
    )
        : radix_node_base<node_type>(),
          edge(_edge_label.data(), _edge_label.size()),
          terminal(),
          children()
    {
        this->is_terminal = _is_terminal;
    }

    // deep copy is a tree-level operation (m_copy_subtree); a node cannot copy
    // itself without copying everything below it.
    text_radix_node(const text_radix_node&)            = delete;
    text_radix_node& operator=(const text_radix_node&) = delete;

    // These three were 256-iteration scans. The map answers all of them in
    // O(1).
    void        clear_children()       D_NOEXCEPT { children.clear(); }
    std::size_t child_count()    const D_NOEXCEPT { return children.size(); }
    node_type*  sole_child()     const D_NOEXCEPT { return children.sole(); }
};


// ==========================================================================
//  radix_storage_signals  --  SPEAKING THE TRAIT SYSTEM'S LANGUAGE
// ==========================================================================

// radix_storage_signals
//   The trait layer does not read our k_fixed flag; it reads STRUCTURAL
// SIGNALS, and we were emitting the wrong ones.
//
//     bounded_container_traits: bounded <=> extent | tuple_size |
//                               static_bounds | (capacity() && !reserve())
//
// We exposed capacity() and no reserve(), so a DYNAMIC tree -- which has no
// capacity at all -- was classified BOUNDED.  A false positive on the axis that
// says whether kappa < infinity.
//
//     container_storage_traits: static  <=> extent | tuple_size
//                               dynamic <=> allocator_type | reserve()
//
// We had neither, so container_memory_discipline resolved to UNKNOWN for every
// tree.
//
// The two cases genuinely differ, so the signals must too. Inherited, so each
// tree emits exactly the ones true of it: fixed -> extent -> bounded, static
// storage, discipline NONE (the node slab IS the container's footprint) dynamic
// -> allocator_type -> unbounded, dynamic storage, discipline INDIVIDUAL
// (per-node new)
template<bool Fixed,
         typename    Node,
         std::size_t Capacity>
struct radix_storage_signals;

template<typename    Node,
         std::size_t Capacity>
struct radix_storage_signals<true, Node, Capacity>
{
    // the compile-time fixed-capacity convention.  kappa = Capacity < inf.
    static D_CONSTEXPR std::size_t extent = Capacity;
};

template<typename    Node,
         std::size_t Capacity>
struct radix_storage_signals<false, Node, Capacity>
{
    // a per-node general-purpose heap: the INDIVIDUAL discipline, and the
    // signal that says the cells are acquired rather than inline.
    using allocator_type = std::allocator<Node>;
};


// ==========================================================================
//  text_radix_tree
// ==========================================================================

template<typename         Value,
         std::size_t      Capacity = 0,
         container_option Flags     = container_option::none>
class text_radix_tree
    : public radix_storage_signals<
          container_option_has(
              radix_tree_option_resolve(Flags, Capacity),
              container_option::fixed_size),
          text_radix_node<Value>,
          Capacity>
{
private:
    // --- option resolution --------------------------------------------
    static D_CONSTEXPR container_option k_resolved =
        radix_tree_option_resolve(Flags, Capacity);

    static_assert(
        container_option_axis_valid(Flags),
        "container_option: at most one flag per axis.");

    static_assert(
        !container_option_has(k_resolved, container_option::compile_time),
        "compile_time mutability is handled by text_radix_tree_ct.");

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

    using node_type = text_radix_node<Value>;

    // fixed_node_pool
    //   a slab of Capacity nodes. Slot 0 is ALWAYS the root, so the root
    // exists from construction and its address -- the empty key -- is always
    // resolvable.
    struct fixed_node_pool
    {
        node_type   nodes[(Capacity > 0u) ? Capacity : 1u];
        bool        used [(Capacity > 0u) ? Capacity : 1u];
        std::size_t count;
    };

    // dynamic_storage
    //   the root is heap-allocated and always present, for the same reason.
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
    using value_type      = Value;
    using key_type        = re_std::string_view;
    using pointer         = Value*;
    using const_pointer   = const Value*;
    using reference       = Value&;
    using const_reference = const Value&;
    using difference_type = std::ptrdiff_t;
    using size_type       = std::size_t;

    using options_type = container_option;
    static D_CONSTEXPR container_option option_flags = k_resolved;

    static D_CONSTEXPR bool is_writable  = k_writable;
    static D_CONSTEXPR bool is_immutable = k_immutable;
    static D_CONSTEXPR bool is_ordered   = k_ordered;
    static D_CONSTEXPR bool is_fixed     = k_fixed;
    static D_CONSTEXPR bool is_dynamic   = k_dynamic;

    // the framework's axes. multiplicity is UNIQUE: one key, at most one
    // value. That is what makes the key an ADDRESS.
    static D_CONSTEXPR container_structure axis_structure =
        radix_axis_structure;
    static D_CONSTEXPR container_multiplicity axis_multiplicity =
        radix_axis_multiplicity;


    // ------------------------------------------------------------------
    //  construction
    // ------------------------------------------------------------------

    text_radix_tree()
        : m_store(),
          m_count(0u)
    {
        m_init_root();
    }

    text_radix_tree(const text_radix_tree& _other)
        : m_store(),
          m_count(0u)
    {
        m_init_root();

        node_type* src = _other.m_root_ptr();
        node_type* dst = m_root_ptr();

        dst->edge        = src->edge;
        dst->terminal    = src->terminal;
        dst->is_terminal = src->is_terminal;

        m_copy_children(src, dst);

        m_count = _other.m_count;
    }

    text_radix_tree(text_radix_tree&& _other) D_NOEXCEPT
        : m_store(),
          m_count(0u)
    {
        m_init_root();

        m_move_from(static_cast<text_radix_tree&&>(_other));
    }

    ~text_radix_tree()
    {
        m_destroy_all();
    }

    text_radix_tree&
    operator=(const text_radix_tree& _other)
    {
        if (this != &_other)
        {
            text_radix_tree tmp(_other);

            m_swap_with(tmp);
        }

        return *this;
    }

    text_radix_tree&
    operator=(text_radix_tree&& _other) D_NOEXCEPT
    {
        if (this != &_other)
        {
            m_destroy_children(m_root_ptr());

            m_root_ptr()->terminal    = radix_terminal<Value>();
            m_root_ptr()->is_terminal = false;
            m_count                   = 0u;

            m_move_from(static_cast<text_radix_tree&&>(_other));
        }

        return *this;
    }


    // ------------------------------------------------------------------
    //  capacity
    // ------------------------------------------------------------------

    size_type size()  const D_NOEXCEPT { return m_count; }
    bool      empty() const D_NOEXCEPT { return (m_count == 0u); }

    // capacity
    //   FIXED TREES ONLY. It used to exist unconditionally, and
    // bounded_container_traits reads "capacity() and no reserve()" as
    // "compile-time fixed capacity" -- so a dynamic tree, whose capacity()
    // only ever returned 0, was reported BOUNDED. A dynamic radix tree has no
    // capacity; the honest thing is not to answer.
    template<bool F = k_fixed,
             typename std::enable_if<F, int>::type = 0>
    size_type
    capacity() const D_NOEXCEPT
    {
        return Capacity;
    }


    // ------------------------------------------------------------------
    //  lookup  --  resolve(address)
    // ------------------------------------------------------------------

    // find
    //   THE RESOLVE. Descend from the root, consuming the key one edge at a
    // time; the node you land on is the one the key names. Returns null when
    // the key names no stored value -- including when it names a WAYPOINT (a
    // node that exists only because something below it does).
    const_pointer
    find(
        key_type _key
    ) const D_NOEXCEPT
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
    find(
        key_type _key
    ) D_NOEXCEPT
    {
        node_type* n = m_find_node(_key);

        if (n == nullptr || !n->terminal.present)
        {
            return nullptr;
        }

        return &n->terminal.value;
    }

    bool
    contains(
        key_type _key
    ) const D_NOEXCEPT
    {
        return (find(_key) != nullptr);
    }

    const_reference
    at(
        key_type _key
    ) const
    {
        const_pointer p = find(_key);

        if (p == nullptr)
        {
            throw std::out_of_range("text_radix_tree::at: key not found");
        }

        return *p;
    }

    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    reference
    at(
        key_type _key
    )
    {
        pointer p = find<true>(_key);

        if (p == nullptr)
        {
            throw std::out_of_range("text_radix_tree::at: key not found");
        }

        return *p;
    }


    // ------------------------------------------------------------------
    //  insertion  --  SPLIT AT THE MEET
    // ------------------------------------------------------------------

    // insert
    //   inserts or OVERWRITES. Returns true when the key was new.
    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    bool
    insert(
        key_type          _key,
        const value_type& _value
    )
    {
        return m_insert_impl(_key, _value, true);
    }

    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    bool
    insert(
        key_type     _key,
        value_type&& _value
    )
    {
        return m_insert_impl(_key, static_cast<value_type&&>(_value), true);
    }

    // try_insert
    //   inserts only when the key is new. Returns false, and changes nothing,
    // when it is already there.
    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    bool
    try_insert(
        key_type          _key,
        const value_type& _value
    )
    {
        return m_insert_impl(_key, _value, false);
    }


    // ------------------------------------------------------------------
    //  erasure  --  MERGE BACK
    // ------------------------------------------------------------------

    // erase
    //   removes the key. Then RESTORES COMPRESSION: a node left with no
    // children and no value is dropped, and a node left with exactly one child
    // and no value is merged into it. The merge concatenates two edge labels
    // into one, which re-brackets the address without changing it -- so no
    // surviving key moves. Returns true when a key was removed.
    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    bool
    erase(
        key_type _key
    )
    {
        node_type* n = m_find_node(_key);

        if (n == nullptr || !n->terminal.present)
        {
            return false;
        }

        n->terminal    = radix_terminal<Value>();
        n->is_terminal = false;

        --m_count;

        m_recompress(n);

        return true;
    }

    template<bool W = k_writable,
             typename std::enable_if<W, int>::type = 0>
    void
    clear() D_NOEXCEPT
    {
        node_type* r = m_root_ptr();

        m_destroy_children(r);

        r->terminal    = radix_terminal<Value>();
        r->is_terminal = false;
        m_count        = 0u;

        return;
    }


    // ------------------------------------------------------------------
    //  prefix queries  --  the reason a trie exists
    // ------------------------------------------------------------------

    // has_prefix
    //   is any stored key extended by _prefix? A prefix names a SUBTREE, and
    // that is the whole point of the prefix order: everything below a node
    // shares its address as a prefix.
    bool
    has_prefix(
        key_type _prefix
    ) const D_NOEXCEPT
    {
        return (m_find_prefix_root(_prefix) != nullptr);
    }

    // count_with_prefix
    //   how many stored keys begin with _prefix -- the size of the subtree the
    // prefix names.
    size_type
    count_with_prefix(
        key_type _prefix
    ) const D_NOEXCEPT
    {
        const node_type* n = m_find_prefix_root(_prefix);

        return ( (n == nullptr) ? 0u : m_count_terminals(n) );
    }

    // longest_match
    //   the length of the longest stored key that is a PREFIX OF _key. The
    // deepest terminal on the descent path -- which is a meet, again: the
    // greatest stored address below-or-equal _key in the prefix order.
    size_type
    longest_match(
        key_type _key
    ) const D_NOEXCEPT
    {
        const node_type* n    = m_root_ptr();
        std::size_t      pos  = 0u;
        std::size_t      best = 0u;

        if (n->terminal.present)
        {
            // the empty key is stored, and it is a prefix of everything
            best = 0u;
        }

        for (;;)
        {
            if (pos >= _key.size())
            {
                break;
            }

            unsigned char c =
                static_cast<unsigned char>(_key[pos]);

            const node_type* child = n->children.get(c);

            if (child == nullptr)
            {
                break;
            }

            re_std::string_view rest(_key.data() + pos, _key.size() - pos);
            re_std::string_view edge(child->edge.data(), child->edge.size());

            text_prefix_match_result r = text_prefix_compare(rest, edge);

            if (!r.edge_consumed)
            {
                break;   // the edge diverges: the descent stops here
            }

            pos += child->edge.size();
            n    = child;

            if (n->terminal.present)
            {
                best = pos;
            }
        }

        return best;
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
    //  ITERATION  --  walking the addresses, in order
    // ------------------------------------------------------------------
    //   THE TREES HAD NO begin()/end(), AND THAT WAS NOT A COSMETIC GAP.  Two
    // thirds of the trait system keys on iterability:
    //
    //     is_copyable_container   = is_iterable_container && copy_constructible
    //     is_container_filterable, is_filter_source, is_range_constructible,
    //     is_range_insertable, the transform and merge traits ...
    //
    // all of it goes through begin()/end().  A container without them is not
    // "slightly less convenient"; it is INVISIBLE to the framework -- every one
    // of those traits reported false, and none of them was wrong to.
    //
    //   The walk is DFS PREORDER over the children in byte order, which is
    // exactly ADDRESS ORDER: the packed child array is sorted by rank, a node's
    // address extends its parent's, so preorder emits the keys sorted.  Nothing
    // is sorted to make that true.
    //
    //   And the successor needs NO STACK.  Parent pointers plus rank() give it
    // in two moves: descend to the first child, or climb until a next sibling
    // exists -- and rank(edge[0]) IS my index among my parent's children, so
    // "the next sibling" is one array step.

    class const_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = Value;
        using reference         = const Value&;
        using pointer           = const Value*;
        using difference_type   = std::ptrdiff_t;

        const_iterator() D_NOEXCEPT
            : m_root(nullptr), m_cur(nullptr) {}

        const_iterator(const node_type* _root, const node_type* _cur) D_NOEXCEPT
            : m_root(_root), m_cur(_cur) {}

        reference operator*()  const D_NOEXCEPT
        { return m_cur->terminal.value; }

        pointer   operator->() const D_NOEXCEPT
        { return &m_cur->terminal.value; }

        // key
        //   the ADDRESS of the node we are sitting on, recovered by walking to
        // the root and concatenating the edge labels in reverse. The iterator
        // stores no key -- it does not need to. The address is in the tree.
        std::string
        key() const
        {
            std::string      out;
            const node_type* n = m_cur;

            while (n != nullptr && n != m_root)
            {
                out.insert(0u, n->edge);
                n = n->parent;
            }

            return out;
        }

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

        const_iterator
        operator++(int)
        {
            const_iterator t = *this;

            ++(*this);

            return t;
        }

        bool operator==(const const_iterator& _o) const D_NOEXCEPT
        { return (m_cur == _o.m_cur); }

        bool operator!=(const const_iterator& _o) const D_NOEXCEPT
        { return (m_cur != _o.m_cur); }

    private:
        const node_type* m_root;
        const node_type* m_cur;

        // m_next_dfs the preorder successor. Descend to the first child if
        // there is one;
        // otherwise climb, and at each parent ask whether I have a next
        // sibling. rank(my first byte) is my own index, so the sibling is
        // index
        // + 1.
        static const node_type*
        m_next_dfs(
            const node_type* _n,
            const node_type* _root
        ) D_NOEXCEPT
        {
            if (_n == nullptr)
            {
                return nullptr;
            }

            if (_n->children.size() > 0u)
            {
                return _n->children.slots()[0];   // byte order: the least label
            }

            while (_n != _root)
            {
                const node_type* p = _n->parent;

                unsigned char c =
                    static_cast<unsigned char>(_n->edge[0]);

                std::size_t mine = p->children.rank(c);   // MY index

                if ((mine + 1u) < p->children.size())
                {
                    return p->children.slots()[mine + 1u];
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
            return const_iterator(r, r);   // the EMPTY key lives at the root
        }

        const_iterator i(r, r);

        ++i;

        return i;
    }

    const_iterator end() const D_NOEXCEPT
    { return const_iterator(m_root_ptr(), nullptr); }

    const_iterator cbegin() const D_NOEXCEPT { return begin(); }
    const_iterator cend()   const D_NOEXCEPT { return end(); }

    // ------------------------------------------------------------------
    //  THE INVARIANTS, MADE EXECUTABLE
    // ------------------------------------------------------------------

    // keys
    //   every stored key, recovered by WALKING AND CONCATENATING the edge
    // labels. This is address recovery: a node's address IS the concatenation
    // of the labels from the root, and this reconstructs it from the structure
    // rather than remembering it. In byte order, so the result is sorted.
    std::vector<std::string>
    keys() const
    {
        std::vector<std::string> out;
        std::string              acc;

        m_collect_keys(m_root_ptr(), acc, out);

        return out;
    }

    // round_trips
    //   THE CONTRACT. Every address recovered from the structure resolves back
    // to the node it was recovered from. find(key_of(n)) == n, for every n.
    bool
    round_trips() const D_NOEXCEPT
    {
        std::vector<std::string> ks = keys();

        if (ks.size() != m_count)
        {
            return false;   // the structure holds a different number of keys
        }                   // than we think we stored

        for (std::size_t i = 0u; i < ks.size(); ++i)
        {
            re_std::string_view k(ks[i].data(), ks[i].size());

            if (find(k) == nullptr)
            {
                return false;
            }
        }

        return true;
    }

    // is_compressed
    //   THE COMPRESSION INVARIANT. Every node is the root, or terminal, or has
    // at least two children. A non-root, non-terminal node with exactly one
    // child is a chain that should have been collapsed.
    bool
    is_compressed() const D_NOEXCEPT
    {
        return m_check_compressed(m_root_ptr(), true);
    }

    // node_count
    //   how many nodes the tree occupies. Compression is what keeps this near
    // the number of keys rather than the number of BYTES in them.
    size_type
    node_count() const D_NOEXCEPT
    {
        return m_count_nodes(m_root_ptr());
    }


private:
    storage_type m_store;
    size_type    m_count;

    // ------------------------------------------------------------------
    //  root
    // ------------------------------------------------------------------

    node_type*
    m_root_ptr() const D_NOEXCEPT
    {
        return m_root_ptr_impl(
            typename std::integral_constant<bool, k_fixed>::type());
    }

    node_type*
    m_root_ptr_impl(std::true_type /*fixed*/) const D_NOEXCEPT
    {
        return const_cast<node_type*>(&m_store.nodes[0]);
    }

    node_type*
    m_root_ptr_impl(std::false_type /*dynamic*/) const D_NOEXCEPT
    {
        return m_store.root;
    }

    void
    m_init_root()
    {
        m_init_root_impl(
            typename std::integral_constant<bool, k_fixed>::type());
    }

    void
    m_init_root_impl(std::true_type /*fixed*/)
    {
        for (std::size_t i = 0u;
             i < ((Capacity > 0u) ? Capacity : 1u);
             ++i)
        {
            m_store.used[i] = false;
        }

        m_store.used[0]  = true;   // slot 0 is ALWAYS the root
        m_store.count    = 1u;
        m_store.nodes[0].edge.clear();
        m_store.nodes[0].terminal    = radix_terminal<Value>();
        m_store.nodes[0].is_terminal = false;
        m_store.nodes[0].parent      = nullptr;
        m_store.nodes[0].children.clear();

        return;
    }

    void
    m_init_root_impl(std::false_type /*dynamic*/)
    {
        m_store.root  = new node_type();
        m_store.count = 1u;

        return;
    }

    // ------------------------------------------------------------------
    //  node allocation
    // ------------------------------------------------------------------

    node_type*
    m_allocate_node(
        re_std::string_view _edge,
        bool               _is_terminal
    )
    {
        return m_allocate_node_impl(
            _edge, _is_terminal,
            typename std::integral_constant<bool, k_fixed>::type());
    }

    node_type*
    m_allocate_node_impl(
        re_std::string_view _edge,
        bool               _is_terminal,
        std::true_type     /*fixed*/
    )
    {
        for (std::size_t i = 1u; i < Capacity; ++i)   // 0 is the root
        {
            if (!m_store.used[i])
            {
                node_type& n = m_store.nodes[i];

                n.edge.assign(_edge.data(), _edge.size());
                n.terminal    = radix_terminal<Value>();
                n.is_terminal = _is_terminal;
                n.parent      = nullptr;
                n.children.clear();

                m_store.used[i] = true;
                ++m_store.count;

                return &n;
            }
        }

        throw std::bad_alloc();
    }

    node_type*
    m_allocate_node_impl(
        re_std::string_view _edge,
        bool               _is_terminal,
        std::false_type    /*dynamic*/
    )
    {
        node_type* n = new node_type(_edge, _is_terminal);

        ++m_store.count;

        return n;
    }

    void
    m_free_node(node_type* _node) D_NOEXCEPT
    {
        m_free_node_impl(
            _node, typename std::integral_constant<bool, k_fixed>::type());

        return;
    }

    void
    m_free_node_impl(node_type* _node, std::true_type /*fixed*/) D_NOEXCEPT
    {
        std::size_t i =
            static_cast<std::size_t>(_node - &m_store.nodes[0]);

        if (i == 0u)
        {
            return;   // never free the root
        }

        _node->edge.clear();
        _node->terminal    = radix_terminal<Value>();
        _node->is_terminal = false;
        _node->parent      = nullptr;
        _node->children.clear();

        m_store.used[i] = false;
        --m_store.count;

        return;
    }

    void
    m_free_node_impl(node_type* _node, std::false_type /*dynamic*/) D_NOEXCEPT
    {
        delete _node;
        --m_store.count;

        return;
    }

    // ------------------------------------------------------------------
    //  teardown
    // ------------------------------------------------------------------

    void
    m_destroy_children(node_type* _node) D_NOEXCEPT
    {
        node_type* const* kids = _node->children.slots();
        std::size_t       n    = _node->children.size();

        for (std::size_t i = 0u; i < n; ++i)
        {
            m_destroy_subtree(kids[i]);
        }

        _node->children.clear();

        return;
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

        return;
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

        return;
    }

    void m_destroy_root_impl(std::true_type)  D_NOEXCEPT { }
    void m_destroy_root_impl(std::false_type) D_NOEXCEPT
    {
        delete m_store.root;
        m_store.root = nullptr;
    }

    // ------------------------------------------------------------------
    //  deep copy / move
    // ------------------------------------------------------------------

    void
    m_copy_children(const node_type* _src, node_type* _dst)
    {
        node_type* const* kids = _src->children.slots();
        std::size_t       n    = _src->children.size();

        // the packed array is already in byte order, so every set() below
        // lands at the end -- an append, not an insertion.
        for (std::size_t i = 0u; i < n; ++i)
        {
            unsigned char c =
                static_cast<unsigned char>(kids[i]->edge[0]);

            _dst->children.set(c, m_copy_subtree(kids[i], _dst));
        }

        return;
    }

    node_type*
    m_copy_subtree(const node_type* _src, node_type* _parent)
    {
        re_std::string_view e(_src->edge.data(), _src->edge.size());

        node_type* n = m_allocate_node(e, _src->is_terminal);

        n->terminal = _src->terminal;
        n->parent   = _parent;

        m_copy_children(_src, n);

        return n;
    }

    // m_move_from steals _other's children. A fixed pool cannot be stolen from
    // (the nodes live inside the object), so that case deep-copies and clears
    // the source -- which is what a move must do when the storage is
    // in-object.
    void
    m_move_from(text_radix_tree&& _other)
    {
        node_type* src = _other.m_root_ptr();
        node_type* dst = m_root_ptr();

        dst->terminal    = src->terminal;
        dst->is_terminal = src->is_terminal;

        m_move_children_impl(
            src, dst, _other,
            typename std::integral_constant<bool, k_fixed>::type());

        m_count = _other.m_count;

        _other.m_reset_after_move();

        return;
    }

    void
    m_move_children_impl(
        node_type*       _src,
        node_type*       _dst,
        text_radix_tree& /*_other*/,
        std::true_type   /*fixed: in-object storage, cannot steal*/
    )
    {
        m_copy_children(_src, _dst);
    }

    void
    m_move_children_impl(
        node_type*       _src,
        node_type*       _dst,
        text_radix_tree& _other,
        std::false_type  /*dynamic: steal the pointers*/
    )
    {
        _dst->children.adopt(_src->children);   // steal the whole map

        node_type* const* kids = _dst->children.slots();
        std::size_t       n    = _dst->children.size();

        for (std::size_t i = 0u; i < n; ++i)
        {
            kids[i]->parent = _dst;
        }

        m_store.count += (_other.m_store.count - 1u);
        _other.m_store.count = 1u;
    }

    void
    m_reset_after_move() D_NOEXCEPT
    {
        node_type* r = m_root_ptr();

        m_destroy_children(r);

        r->terminal    = radix_terminal<Value>();
        r->is_terminal = false;
        m_count        = 0u;

        return;
    }

    void
    m_swap_with(text_radix_tree& _other)
    {
        // a fixed pool is in-object, so a pointer swap is not available; the
        // copy-and-swap idiom degrades to a copy either way.
        node_type* r = m_root_ptr();

        m_destroy_children(r);

        r->terminal    = _other.m_root_ptr()->terminal;
        r->is_terminal = _other.m_root_ptr()->is_terminal;

        m_copy_children(_other.m_root_ptr(), r);

        m_count = _other.m_count;

        return;
    }

    // ------------------------------------------------------------------
    //  descent
    // ------------------------------------------------------------------

    // m_find_node
    //   the node whose ADDRESS is exactly _key, or null. This is resolve: fold
    // the descent step along the key, one edge at a time.
    node_type*
    m_find_node(key_type _key) const D_NOEXCEPT
    {
        node_type*  n   = m_root_ptr();
        std::size_t pos = 0u;

        while (pos < _key.size())
        {
            unsigned char c =
                static_cast<unsigned char>(_key[pos]);

            node_type* child = n->children.get(c);

            if (child == nullptr)
            {
                return nullptr;
            }

            re_std::string_view rest(_key.data() + pos, _key.size() - pos);
            re_std::string_view edge(child->edge.data(), child->edge.size());

            text_prefix_match_result r = text_prefix_compare(rest, edge);

            // The edge must be FULLY consumed to step past it. If it is not,
            // the key either diverges from it or ends inside it -- and a key
            // that ends inside an edge names no node at all, because the edge
            // is one label, not a run of them.
            if (!r.edge_consumed)
            {
                return nullptr;
            }

            pos += child->edge.size();
            n    = child;
        }

        return n;
    }

    // m_find_prefix_root
    //   the node whose subtree holds exactly the keys extended by _prefix.
    // Unlike m_find_node, the prefix MAY end inside an edge -- a prefix names
    // a
    // set of addresses, not one address, and the set is the subtree hanging
    // below the edge it ends inside.
    node_type*
    m_find_prefix_root(key_type _prefix) const D_NOEXCEPT
    {
        node_type*  n   = m_root_ptr();
        std::size_t pos = 0u;

        while (pos < _prefix.size())
        {
            unsigned char c =
                static_cast<unsigned char>(_prefix[pos]);

            node_type* child = n->children.get(c);

            if (child == nullptr)
            {
                return nullptr;
            }

            re_std::string_view rest(
                _prefix.data() + pos, _prefix.size() - pos);
            re_std::string_view edge(
                child->edge.data(), child->edge.size());

            text_prefix_match_result r = text_prefix_compare(rest, edge);

            if (r.edge_consumed)
            {
                pos += child->edge.size();
                n    = child;

                continue;
            }

            // the prefix ran out INSIDE this edge. If it is a prefix of the
            // edge, the whole subtree below matches; otherwise nothing does.
            return ( r.key_consumed ? child : nullptr );
        }

        return n;
    }

    // ------------------------------------------------------------------
    //  THE SPLIT  --  at the meet
    // ------------------------------------------------------------------

    // m_split_edge
    //   splits _node's edge at _split_pos -- which the caller has computed as
    // |lcp(key, edge)|, THE MEET -- and returns the node at which a key with
    // remainder _remaining_key should terminate.
    //
    //   Before:      parent --[abcdef]--> node(subtree)
    //   After:       parent --[abc]-----> node --[def]--> lower(subtree)
    //                                        \--[xyz]--> branch   (if any)
    //
    //   The subtree, the terminal value, and the children all move DOWN to
    //   `lower`; `node` keeps only the shared prefix and becomes a waypoint.
    // Every address below is unchanged, because abc + def == abcdef: the split
    // RE-BRACKETS the concatenation without changing it. That is the only
    // reason this is allowed, and it is why the split point must be the meet.
    node_type*
    m_split_edge(
        node_type*         _node,
        std::size_t        _split_pos,
        re_std::string_view _remaining_key
    )
    {
        // the lower half keeps everything that was below the split
        re_std::string_view lower_edge(
            _node->edge.data() + _split_pos,
            _node->edge.size() - _split_pos);

        node_type* lower =
            m_allocate_node(lower_edge, _node->is_terminal);

        lower->terminal = _node->terminal;
        lower->parent   = _node;

        // the WHOLE subtree moves down, in one adopt
        lower->children.adopt(_node->children);

        {
            node_type* const* kids = lower->children.slots();
            std::size_t       n    = lower->children.size();

            for (std::size_t i = 0u; i < n; ++i)
            {
                kids[i]->parent = lower;
            }
        }

        // the upper half keeps only the shared prefix, and becomes a waypoint
        _node->edge.resize(_split_pos);
        _node->terminal    = radix_terminal<Value>();
        _node->is_terminal = false;
        _node->children.clear();

        unsigned char lc =
            static_cast<unsigned char>(lower->edge[0]);

        _node->children.set(lc, lower);

        // and the new key branches off here
        if (_remaining_key.empty())
        {
            return _node;   // the key ends exactly at the meet
        }

        node_type* branch = m_allocate_node(_remaining_key, true);

        branch->parent = _node;

        unsigned char bc =
            static_cast<unsigned char>(_remaining_key[0]);

        _node->children.set(bc, branch);

        return branch;
    }

    // ------------------------------------------------------------------
    //  insertion
    // ------------------------------------------------------------------

    template<typename FwdValue>
    bool
    m_insert_impl(
        key_type    _key,
        FwdValue&& _value,
        bool        _overwrite
    )
    {
        node_type*  n   = m_root_ptr();
        std::size_t pos = 0u;

        for (;;)
        {
            if (pos >= _key.size())
            {
                // the key ends HERE. Either this node already holds it, or it
                // becomes a terminal.
                if (n->terminal.present)
                {
                    if (!_overwrite)
                    {
                        return false;
                    }

                    n->terminal.value =
                        static_cast<FwdValue&&>(_value);

                    return false;   // not a new key
                }

                n->terminal.present = true;
                n->terminal.value   = static_cast<FwdValue&&>(_value);
                n->is_terminal      = true;

                ++m_count;

                return true;
            }

            unsigned char c =
                static_cast<unsigned char>(_key[pos]);

            node_type* child = n->children.get(c);

            re_std::string_view rest(
                _key.data() + pos, _key.size() - pos);

            if (child == nullptr)
            {
                // nothing starts with this byte: the whole remainder becomes
                // one compressed edge. This is compression, on the way in.
                node_type* leaf = m_allocate_node(rest, true);

                leaf->terminal.present = true;
                leaf->terminal.value   =
                    static_cast<FwdValue&&>(_value);
                leaf->parent = n;

                n->children.set(c, leaf);

                ++m_count;

                return true;
            }

            re_std::string_view edge(
                child->edge.data(), child->edge.size());

            text_prefix_match_result r = text_prefix_compare(rest, edge);

            if (r.edge_consumed)
            {
                // the edge matches entirely: step past it and carry on
                pos += child->edge.size();
                n    = child;

                continue;
            }

            // THE EDGE DIVERGES.  Split it at the meet, and terminate the new
            // key at (or below) the split point.  r.common_len is |lcp|, and it
            // is >= 1 because the first byte matched by construction.
            re_std::string_view tail(
                rest.data() + r.common_len,
                rest.size() - r.common_len);

            node_type* target =
                m_split_edge(child, r.common_len, tail);

            if (target->terminal.present)
            {
                if (!_overwrite)
                {
                    return false;
                }

                target->terminal.value =
                    static_cast<FwdValue&&>(_value);

                return false;
            }

            target->terminal.present = true;
            target->terminal.value   = static_cast<FwdValue&&>(_value);
            target->is_terminal      = true;

            ++m_count;

            return true;
        }
    }

    // ------------------------------------------------------------------
    //  THE MERGE  --  restore compression
    // ------------------------------------------------------------------

    // m_recompress
    //   walks UP from a node that just lost its value, restoring the
    // invariant: every node is the root, or terminal, or has at least two
    // children.
    //
    //   Two repairs, and both preserve every surviving address:
    //     - a leaf with no value is DROPPED (it named nothing and led nowhere)
    //     - a valueless node with one child is MERGED into it: the two edge
    //       labels concatenate. abc + def == abcdef. The address is the
    //       concatenation, so it does not move.
    void
    m_recompress(node_type* _node) D_NOEXCEPT
    {
        node_type* n = _node;

        while (n != nullptr && n != m_root_ptr())
        {
            node_type* parent = n->parent;

            if (n->terminal.present)
            {
                break;   // it still holds a key: it stays, and so do its edges
            }

            std::size_t kids = n->child_count();

            if (kids == 0u)
            {
                // a leaf with no value: it names nothing.  Drop it.
                unsigned char c =
                    static_cast<unsigned char>(n->edge[0]);

                parent->children.erase(c);

                m_free_node(n);

                n = parent;

                continue;
            }

            if (kids == 1u)
            {
                // one child, no value: a chain.  MERGE.
                node_type* kid = n->sole_child();

                // the concatenation -- this is the whole of the theorem
                std::string merged = n->edge + kid->edge;

                unsigned char c =
                    static_cast<unsigned char>(n->edge[0]);

                kid->edge   = merged;
                kid->parent = parent;

                parent->children.set(c, kid);

                n->clear_children();

                m_free_node(n);
            }

            break;   // >= 2 children: the invariant holds from here up
        }

        return;
    }

    // ------------------------------------------------------------------
    //  walks
    // ------------------------------------------------------------------

    void
    m_collect_keys(
        const node_type*          _node,
        std::string&              _acc,
        std::vector<std::string>& _out
    ) const
    {
        if (_node->terminal.present)
        {
            _out.push_back(_acc);
        }

        // byte order, for free: rank is monotone in the byte, so the packed
        // array IS sorted. keys() comes out sorted without sorting.
        node_type* const* kids = _node->children.slots();
        std::size_t       n    = _node->children.size();

        for (std::size_t i = 0u; i < n; ++i)
        {
            std::size_t before = _acc.size();

            _acc += kids[i]->edge;               // the CONCATENATION

            m_collect_keys(kids[i], _acc, _out);

            _acc.resize(before);
        }

        return;
    }

    size_type
    m_count_terminals(const node_type* _node) const D_NOEXCEPT
    {
        size_type n = ( _node->terminal.present ? 1u : 0u );

        node_type* const* kids = _node->children.slots();
        std::size_t       kn   = _node->children.size();

        for (std::size_t i = 0u; i < kn; ++i)
        {
            n += m_count_terminals(kids[i]);
        }

        return n;
    }

    size_type
    m_count_nodes(const node_type* _node) const D_NOEXCEPT
    {
        size_type n = 1u;

        node_type* const* kids = _node->children.slots();
        std::size_t       kn   = _node->children.size();

        for (std::size_t i = 0u; i < kn; ++i)
        {
            n += m_count_nodes(kids[i]);
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
            !_node->terminal.present &&
            _node->child_count() < 2u)
        {
            return false;   // a chain that should have been collapsed
        }

        node_type* const* kids = _node->children.slots();
        std::size_t       n    = _node->children.size();

        for (std::size_t i = 0u; i < n; ++i)
        {
            if (!m_check_compressed(kids[i], false))
            {
                return false;
            }
        }

        return true;
    }
};


// ==========================================================================
//  named subtypes
// ==========================================================================

// dyn_text_radix_tree
//   heap-backed, writable, unbounded.
template<typename Value>
using dyn_text_radix_tree =
    text_radix_tree<Value, 0,
                    container_option::writable |
                    container_option::dynamic_size>;

// fixed_text_radix_tree
//   a slab of Capacity nodes; insertion throws bad_alloc when exhausted.
template<typename    Value,
         std::size_t Capacity>
using fixed_text_radix_tree =
    text_radix_tree<Value, Capacity,
                    container_option::writable |
                    container_option::fixed_size>;

// immutable_text_radix_tree
//   read-only: the mutators are SFINAE'd away.
template<typename Value>
using immutable_text_radix_tree =
    text_radix_tree<Value, 0,
                    container_option::immutable |
                    container_option::dynamic_size>;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_RADIX_TEXT_RADIX_TREE_HPP
