/******************************************************************************
* djinterp [container]                                         linked_list.hpp
*
* Unified linked_list container covering every permutation:
*   This header defines a single class template, linked_list, parameterized
* on a payload type, an option-flag set, an optional fixed capacity, and
* an allocator.  By varying the option flags, the same template covers:
*   - singly / doubly / xor-linked direction
*   - head-only / tail-only / head-and-tail end pointers
*   - linear / circular topology
*   - none / head / tail / both sentinel arrangements
*   - flat / hierarchical (skip-list) structure
*   - owning / intrusive ownership
*   - mutable / immutable / compile-time lifetime   (DContainerOption)
*   - iterable / non-iterable iteration surface     (DContainerOption)
*   - ordered / unordered iteration order           (DContainerOption)
*   - fixed_size / dynamic_size storage strategy    (DContainerOption)
*   - unique / multi multiplicity                   (DContainerOption)
*
*   Concrete configurations are provided as named convenience aliases
* near the end of this header (singly_linked_list, doubly_linked_list,
* circular_doubly_linked_list, intrusive_linked_list, ...).
*   Three node types are provided here:
*   - singly_linked_list_node<_Value>      one .next pointer
*   - doubly_linked_list_node<_Value>      .next + .prev pointers
*   - xor_linked_list_node<_Value>         single .link = xor(prev,next)
*   The container picks one based on the direction axis bits in the
* linked_list_option flag set.  All three nodes inherit from a small
* CRTP base, linked_list_node_base, that supplies the parent / sentinel
* bookkeeping bits.
*
* PORTABILITY:
*   C++11 baseline.  
*   deduction guides are gated on D_ENV_LANG_IS_CPP17_OR_HIGHER.
* DEPENDENCIES:
*   linked_list_traits.hpp     (option flags, axis traits)
*   linked_list_iterator.hpp   (iterator family)
*   list.hpp                   (CRTP foundation)
*
* 
* path:      /inc/djinterp/core/container/list/linked/linked_list.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    node definitions
II.   default-resolution helpers
III.  linked_list class template
IV.   named subtypes (convenience aliases)
V.    deduction guides (C++17+)
*/

#ifndef DJINTERP_CONTAINER_LINKED_LIST_
#define DJINTERP_CONTAINER_LINKED_LIST_ 1

// std
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./linked_list_traits.hpp"
#include "./linked_list_iterator.hpp"
#include "./list.hpp"


NS_DJINTERP


// ===========================================================================
// I.   NODE DEFINITIONS
// ===========================================================================

// linked_list_node_base
//   class: CRTP base supplying flags shared by every linked-list
// node shape.  is_sentinel marks a sentinel/dummy node so that
// iterators can recognize end-of-list without nullptr comparisons.
template<typename _Derived>
struct linked_list_node_base
{
public:
    // is_sentinel
    //   field: true when this node is a sentinel / dummy.  Real
    // payload data members on a sentinel are unspecified.
    bool is_sentinel;

    // linked_list_node_base
    //   constructor: default — non-sentinel.
    constexpr linked_list_node_base() noexcept
        : is_sentinel(false)
    {}
};

// singly_linked_list_node
//   struct: a node carrying only a forward pointer.  The .next
// member is mandatory for has_next_link to succeed.  The .value
// member is the payload (also reachable via data()).
template<typename _Value>
struct singly_linked_list_node
    : public linked_list_node_base<singly_linked_list_node<_Value>>
{
public:
    using value_type = _Value;
    using node_type  = singly_linked_list_node<_Value>;

    // value
    //   field: payload storage.
    _Value     value;

    // next
    //   field: forward link; nullptr at end of a linear list.
    node_type* next;

    // singly_linked_list_node
    //   constructor: default.
    singly_linked_list_node() noexcept
        : linked_list_node_base<node_type>(),
          value(),
          next(nullptr)
    {}

    // singly_linked_list_node
    //   constructor: copies a payload value.
    explicit
    singly_linked_list_node(
        const _Value& _v)
        : linked_list_node_base<node_type>(),
          value(_v),
          next(nullptr)
    {}

    // singly_linked_list_node
    //   constructor: moves a payload value.
    explicit
    singly_linked_list_node(
        _Value&& _v) noexcept(
            std::is_nothrow_move_constructible<_Value>::value)
        : linked_list_node_base<node_type>(),
          value(static_cast<_Value&&>(_v)),
          next(nullptr)
    {}

    // data
    //   accessor: payload reference (sequence/node uniformity).
    _Value&       data()       noexcept { return value; }
    const _Value& data() const noexcept { return value; }

    // copy operations are deleted — list-level deep copy handles the
    // rewire of .next pointers and is therefore the only correct
    // path through which nodes are duplicated.
    singly_linked_list_node(const singly_linked_list_node&) = delete;
    singly_linked_list_node& operator=(
        const singly_linked_list_node&) = delete;
};

// doubly_linked_list_node
//   struct: a node carrying both forward and backward pointers.
template<typename _Value>
struct doubly_linked_list_node
    : public linked_list_node_base<doubly_linked_list_node<_Value>>
{
public:
    using value_type = _Value;
    using node_type  = doubly_linked_list_node<_Value>;

    // value
    //   field: payload storage.
    _Value     value;

    // next
    //   field: forward link.
    node_type* next;

    // prev
    //   field: backward link.
    node_type* prev;

    doubly_linked_list_node() noexcept
        : linked_list_node_base<node_type>(),
          value(),
          next(nullptr),
          prev(nullptr)
    {}

    explicit
    doubly_linked_list_node(
        const _Value& _v)
        : linked_list_node_base<node_type>(),
          value(_v),
          next(nullptr),
          prev(nullptr)
    {}

    explicit
    doubly_linked_list_node(
        _Value&& _v) noexcept(
            std::is_nothrow_move_constructible<_Value>::value)
        : linked_list_node_base<node_type>(),
          value(static_cast<_Value&&>(_v)),
          next(nullptr),
          prev(nullptr)
    {}

    _Value&       data()       noexcept { return value; }
    const _Value& data() const noexcept { return value; }

    doubly_linked_list_node(const doubly_linked_list_node&) = delete;
    doubly_linked_list_node& operator=(
        const doubly_linked_list_node&) = delete;
};

// xor_linked_list_node
//   struct: a node carrying a single .link member that holds the
// XOR of (prev, next) pointers.  Halves per-node link storage in
// exchange for stateful traversal — the iterator must track the
// previous node so that next == link XOR prev.  Useful when a
// doubly-linked list is desired but per-node memory is tight.
template<typename _Value>
struct xor_linked_list_node
    : public linked_list_node_base<xor_linked_list_node<_Value>>
{
public:
    using value_type = _Value;
    using node_type  = xor_linked_list_node<_Value>;

    // value
    //   field: payload storage.
    _Value         value;

    // link
    //   field: XOR of the prev and next pointer bit-patterns,
    // stored as std::uintptr_t so the XOR is well-defined.
    std::uintptr_t link;

    xor_linked_list_node() noexcept
        : linked_list_node_base<node_type>(),
          value(),
          link(0u)
    {}

    explicit
    xor_linked_list_node(
        const _Value& _v)
        : linked_list_node_base<node_type>(),
          value(_v),
          link(0u)
    {}

    explicit
    xor_linked_list_node(
        _Value&& _v) noexcept(
            std::is_nothrow_move_constructible<_Value>::value)
        : linked_list_node_base<node_type>(),
          value(static_cast<_Value&&>(_v)),
          link(0u)
    {}

    _Value&       data()       noexcept { return value; }
    const _Value& data() const noexcept { return value; }

    xor_linked_list_node(const xor_linked_list_node&) = delete;
    xor_linked_list_node& operator=(
        const xor_linked_list_node&) = delete;
};


// ===========================================================================
// II.  DEFAULT RESOLUTION
// ===========================================================================

NS_INTERNAL

    // ll_pick_node
    //   trait: maps a linked_list_option flag set to the appropriate
    // node type.  Direction-axis bits select the layout; ties are
    // broken in favor of doubly (the standard list behavior).
    template<typename          _Value,
             linked_list_option _Flags>
    struct ll_pick_node
    {
    private:
        static constexpr bool is_singly =
            ( linked_list_option_direction(_Flags) ==
              linked_list_option::singly );
        static constexpr bool is_xor =
            ( linked_list_option_direction(_Flags) ==
              linked_list_option::xor_linked );

    public:
        using type = typename std::conditional<
            is_singly,
            singly_linked_list_node<_Value>,
            typename std::conditional<
                is_xor,
                xor_linked_list_node<_Value>,
                doubly_linked_list_node<_Value>
            >::type
        >::type;
    };

NS_END  // internal

// linked_list_option_resolve
//   function: fills unset axes with sensible linked-list defaults.
//   - direction : doubly
//   - ends      : head_and_tail (head_only when direction == singly)
//   - topology  : linear
//   - sentinel  : none
//   - ownership : owning
//   - structure : flat
inline constexpr linked_list_option
linked_list_option_resolve_axis(
    linked_list_option _axis_value,
    linked_list_option _default_value
) noexcept
{
    return ( (_axis_value == linked_list_option::none)
             ? _default_value
             : _axis_value );
}

// linked_list_option_resolve
//   function: returns _flags with all axes filled to their defaults
// where the user left them at none.
inline constexpr linked_list_option
linked_list_option_resolve(
    linked_list_option _flags) noexcept
{
    return
        ( linked_list_option_resolve_axis(
              linked_list_option_direction(_flags),
              linked_list_option::doubly) |
          linked_list_option_resolve_axis(
              linked_list_option_ends(_flags),
              linked_list_option::head_and_tail) |
          (_flags & ~( D_LINKED_LIST_OPTION_DIRECTION_MASK |
                       D_LINKED_LIST_OPTION_ENDS_MASK )) );
}


// ===========================================================================
// III. linked_list CLASS TEMPLATE
// ===========================================================================

// linked_list
//   class: parameterized linked list covering every permutation
// described in the file header.  The class inherits the abstract
// list_base CRTP foundation, which in turn inherits sequential_base.
// Mutating methods are SFINAE-gated on the resolved compile-time
// flag bits.
//
//   Template parameters:
//     _Value         element payload type
//     _LinkFlags     direction / ends / topology / sentinel /
//                    ownership / structure (linked_list_option)
//     _ContainerFlags lifetime / iteration / ordering / multiplicity
//                    / storage (DContainerOption — defined elsewhere)
//     _Capacity      0 for dynamic; non-zero for a fixed node pool
//     _Allocator    standard allocator type
template<typename          _Value,
         linked_list_option _LinkFlags     = linked_list_option::none,
         unsigned          _ContainerFlags = 0u,
         std::size_t       _Capacity       = 0u,
         typename          _Allocator      = std::allocator<_Value>>
class linked_list
    : public djinterp::list_base<
          linked_list<_Value, _LinkFlags, _ContainerFlags,
                      _Capacity, _Allocator>>
{
private:
    // -----------------------------------------------------------------
    // option resolution — private, computed at compile time
    // -----------------------------------------------------------------

    static constexpr linked_list_option k_link_resolved =
        linked_list_option_resolve(_LinkFlags);

    static_assert(
        linked_list_option_axis_valid(_LinkFlags),
        "linked_list_option: at most one flag per axis "
        "(direction / ends / sentinel).");

    // direction axis
    static constexpr bool k_is_singly =
        linked_list_option_has(k_link_resolved,
                               linked_list_option::singly);
    static constexpr bool k_is_doubly =
        linked_list_option_has(k_link_resolved,
                               linked_list_option::doubly);
    static constexpr bool k_is_xor =
        linked_list_option_has(k_link_resolved,
                               linked_list_option::xor_linked);

    // end-pointer axis
    static constexpr bool k_has_head =
        linked_list_option_has(k_link_resolved,
                               linked_list_option::head_only) ||
        linked_list_option_has(k_link_resolved,
                               linked_list_option::head_and_tail);
    static constexpr bool k_has_tail =
        linked_list_option_has(k_link_resolved,
                               linked_list_option::tail_only) ||
        linked_list_option_has(k_link_resolved,
                               linked_list_option::head_and_tail);

    // topology axis
    static constexpr bool k_is_circular =
        linked_list_option_has(k_link_resolved,
                               linked_list_option::circular);

    // sentinel axis
    static constexpr bool k_has_head_sent =
        linked_list_option_has(k_link_resolved,
                               linked_list_option::head_sentinel) ||
        linked_list_option_has(k_link_resolved,
                               linked_list_option::both_sentinels);
    static constexpr bool k_has_tail_sent =
        linked_list_option_has(k_link_resolved,
                               linked_list_option::tail_sentinel) ||
        linked_list_option_has(k_link_resolved,
                               linked_list_option::both_sentinels);

    // ownership axis
    static constexpr bool k_is_intrusive =
        linked_list_option_has(k_link_resolved,
                               linked_list_option::intrusive);

    // structure axis
    static constexpr bool k_is_hierarchical =
        linked_list_option_has(k_link_resolved,
                               linked_list_option::hierarchical);

    // mutability bits from the framework-wide DContainerOption (the
    // bit positions are fixed by container_options.hpp / radix_tree
    // _common.hpp; reproduced here as integer literals to avoid a
    // mandatory include cycle for the bare list module).
    static constexpr bool k_writable =
        ((_ContainerFlags & 0x001u) != 0u);
    static constexpr bool k_immutable =
        ((_ContainerFlags & 0x002u) != 0u);
    static constexpr bool k_iterable =
        ((_ContainerFlags & 0x080u) == 0u);  // default iterable
    static constexpr bool k_unique =
        ((_ContainerFlags & 0x400u) == 0u);  // default unique
    static constexpr bool k_fixed =
        ((_ContainerFlags & 0x020u) != 0u) ||
        (_Capacity > 0u);

    // assembled mutability default — "writable unless explicitly
    // immutable" so that the typical case requires no flag.
    static constexpr bool k_is_mutable =
        ( k_writable || (!k_immutable) );

public:
    // -----------------------------------------------------------------
    // public type aliases — standard container interface
    // -----------------------------------------------------------------
    using value_type      = _Value;
    using allocator_type  = _Allocator;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = _Value&;
    using const_reference = const _Value&;
    using pointer         = _Value*;
    using const_pointer   = const _Value*;

    // node_type
    //   alias: dispatched by the direction axis at compile time.
    using node_type = typename internal::ll_pick_node<
                          _Value, k_link_resolved>::type;

    // iterator types
    using iterator               =
        linked_list_iterator<node_type, false, false>;
    using const_iterator         =
        linked_list_iterator<node_type, true,  false>;
    using reverse_iterator       =
        linked_list_iterator<node_type, false, true>;
    using const_reverse_iterator =
        linked_list_iterator<node_type, true,  true>;

    // option introspection — required by trait detection elsewhere
    using link_options_type        = linked_list_option;
    using container_options_type   = unsigned;

    static constexpr linked_list_option link_option_flags =
        k_link_resolved;
    static constexpr unsigned          container_option_flags =
        _ContainerFlags;

    // axis booleans usable from generic code and trait queries
    static constexpr bool is_singly      = k_is_singly;
    static constexpr bool is_doubly      = k_is_doubly;
    static constexpr bool is_xor         = k_is_xor;
    static constexpr bool is_circular    = k_is_circular;
    static constexpr bool is_intrusive   = k_is_intrusive;
    static constexpr bool is_hierarchical= k_is_hierarchical;
    static constexpr bool is_iterable    = k_iterable;
    static constexpr bool is_writable    = k_is_mutable;
    static constexpr bool is_unique      = k_unique;
    static constexpr bool is_fixed       = k_fixed;

    // sentinel-axis booleans — surfaced as static members so that
    // has_head_sentinel / has_tail_sentinel detection in
    // linked_list_traits.hpp picks them up by name.  Detection uses
    // a two-step probe (existence + truth), so a `false` value here
    // correctly classifies the list as sentinel-less.
    static constexpr bool has_head_sentinel = k_has_head_sent;
    static constexpr bool has_tail_sentinel = k_has_tail_sent;

    // -----------------------------------------------------------------
    // constructors and destructor
    // -----------------------------------------------------------------

    linked_list() noexcept;

    explicit
    linked_list(
        const _Allocator& _alloc) noexcept;

    linked_list(
        size_type      _count,
        const _Value&  _value,
        const _Allocator& _alloc = _Allocator());

    template<typename _Iter>
    linked_list(
        _Iter _first,
        _Iter _last,
        const _Allocator& _alloc = _Allocator());

    linked_list(
        std::initializer_list<_Value> _init,
        const _Allocator& _alloc = _Allocator());

    linked_list(
        const linked_list& _other);

    linked_list(
        linked_list&& _other) noexcept;

    ~linked_list();

    // -----------------------------------------------------------------
    // assignment
    // -----------------------------------------------------------------

    linked_list& operator=(
        const linked_list& _other);

    linked_list& operator=(
        linked_list&& _other) noexcept;

    linked_list& operator=(
        std::initializer_list<_Value> _init);

    // -----------------------------------------------------------------
    // capacity
    // -----------------------------------------------------------------

    size_type size()     const noexcept;
    bool      empty()    const noexcept;
    size_type max_size() const noexcept;
    size_type capacity() const noexcept;

    // -----------------------------------------------------------------
    // end pointers — gated on the end-pointer axis
    // -----------------------------------------------------------------

    template<bool _H = k_has_head,
             typename std::enable_if<_H, int>::type = 0>
    node_type*       head() noexcept;

    template<bool _H = k_has_head,
             typename std::enable_if<_H, int>::type = 0>
    const node_type* head() const noexcept;

    template<bool _T = k_has_tail,
             typename std::enable_if<_T, int>::type = 0>
    node_type*       tail() noexcept;

    template<bool _T = k_has_tail,
             typename std::enable_if<_T, int>::type = 0>
    const node_type* tail() const noexcept;

    // head_sentinel / tail_sentinel are conditionally exposed as
    // public members so that has_head_sentinel / has_tail_sentinel
    // detection in linked_list_traits.hpp can find them.  Their
    // declarations live below, gated via using declarations on the
    // private node members; in the typical (no-sentinel) build,
    // these are absent and the traits report false.

    // -----------------------------------------------------------------
    // element access
    // -----------------------------------------------------------------

    reference       front();
    const_reference front() const;

    template<bool _T = k_has_tail,
             typename std::enable_if<_T, int>::type = 0>
    reference       back();

    template<bool _T = k_has_tail,
             typename std::enable_if<_T, int>::type = 0>
    const_reference back() const;

    // -----------------------------------------------------------------
    // iterators — gated on iterability
    // -----------------------------------------------------------------

    template<bool _I = k_iterable,
             typename std::enable_if<_I, int>::type = 0>
    iterator       begin() noexcept;

    template<bool _I = k_iterable,
             typename std::enable_if<_I, int>::type = 0>
    const_iterator begin() const noexcept;

    template<bool _I = k_iterable,
             typename std::enable_if<_I, int>::type = 0>
    iterator       end() noexcept;

    template<bool _I = k_iterable,
             typename std::enable_if<_I, int>::type = 0>
    const_iterator end() const noexcept;

    template<bool _I = k_iterable,
             typename std::enable_if<_I, int>::type = 0>
    const_iterator cbegin() const noexcept;

    template<bool _I = k_iterable,
             typename std::enable_if<_I, int>::type = 0>
    const_iterator cend()   const noexcept;

    // reverse iteration is only meaningful when the node supports
    // backward traversal (doubly- or xor-linked).
    template<bool _I = k_iterable,
             bool _B = ( k_is_doubly || k_is_xor ),
             typename std::enable_if<_I && _B, int>::type = 0>
    reverse_iterator rbegin() noexcept;

    template<bool _I = k_iterable,
             bool _B = ( k_is_doubly || k_is_xor ),
             typename std::enable_if<_I && _B, int>::type = 0>
    reverse_iterator rend()   noexcept;

    // -----------------------------------------------------------------
    // modifiers — gated on writability
    // -----------------------------------------------------------------

    template<bool _W = k_is_mutable,
             typename std::enable_if<_W, int>::type = 0>
    void
    push_front(
        const _Value& _value);

    template<bool _W = k_is_mutable,
             typename std::enable_if<_W, int>::type = 0>
    void
    push_front(
        _Value&& _value);

    // push_back is exposed only when there is a tail pointer (or
    // when we are willing to walk to the end — when k_has_tail is
    // false the push_back overload is not provided to keep the cost
    // model honest).
    template<bool _W = k_is_mutable,
             bool _T = k_has_tail,
             typename std::enable_if<_W && _T, int>::type = 0>
    void
    push_back(
        const _Value& _value);

    template<bool _W = k_is_mutable,
             bool _T = k_has_tail,
             typename std::enable_if<_W && _T, int>::type = 0>
    void
    push_back(
        _Value&& _value);

    template<bool _W = k_is_mutable,
             typename std::enable_if<_W, int>::type = 0>
    void pop_front();

    template<bool _W = k_is_mutable,
             bool _B = ( k_is_doubly || k_is_xor ),
             typename std::enable_if<_W && _B, int>::type = 0>
    void pop_back();

    // insert / erase return iterator types, so they require
    // iterability as well as writability.
    template<bool _W = k_is_mutable,
             bool _I = k_iterable,
             typename std::enable_if<_W && _I, int>::type = 0>
    iterator
    insert(
        const_iterator _pos,
        const _Value&  _value);

    template<bool _W = k_is_mutable,
             bool _I = k_iterable,
             typename std::enable_if<_W && _I, int>::type = 0>
    iterator
    insert(
        const_iterator _pos,
        _Value&&       _value);

    template<bool _W = k_is_mutable,
             bool _I = k_iterable,
             typename std::enable_if<_W && _I, int>::type = 0>
    iterator
    erase(
        const_iterator _pos);

    template<bool _W = k_is_mutable,
             bool _I = k_iterable,
             typename std::enable_if<_W && _I, int>::type = 0>
    iterator
    erase(
        const_iterator _first,
        const_iterator _last);

    template<bool _W = k_is_mutable,
             typename std::enable_if<_W, int>::type = 0>
    void clear() noexcept;

    // -----------------------------------------------------------------
    // splice — node-rewire moves between lists; O(1) when count is 1.
    // Only meaningful when the iteration surface is exposed.
    // -----------------------------------------------------------------

    template<bool _W = k_is_mutable,
             bool _I = k_iterable,
             typename std::enable_if<_W && _I, int>::type = 0>
    void
    splice(
        const_iterator _pos,
        linked_list&   _other);

    template<bool _W = k_is_mutable,
             bool _I = k_iterable,
             typename std::enable_if<_W && _I, int>::type = 0>
    void
    splice(
        const_iterator _pos,
        linked_list&   _other,
        const_iterator _other_pos);

    // -----------------------------------------------------------------
    // intrusive entry points — only when k_is_intrusive
    //   The intrusive list does not allocate; the user passes an
    // already-linked node (which they own) and the list rewires it.
    // -----------------------------------------------------------------

    template<bool _W = k_is_mutable,
             bool _X = k_is_intrusive,
             typename std::enable_if<_W && _X, int>::type = 0>
    void
    push_front_node(
        node_type* _node) noexcept;

    template<bool _W = k_is_mutable,
             bool _T = k_has_tail,
             bool _X = k_is_intrusive,
             typename std::enable_if<_W && _T && _X, int>::type = 0>
    void
    push_back_node(
        node_type* _node) noexcept;

    template<bool _W = k_is_mutable,
             bool _X = k_is_intrusive,
             typename std::enable_if<_W && _X, int>::type = 0>
    void
    erase_node(
        node_type* _node) noexcept;

private:
    // -----------------------------------------------------------------
    // storage strategy
    // -----------------------------------------------------------------
    //   Fixed-size lists use a stack-allocated node pool with a free
    // list; dynamic lists allocate each node individually via
    // _Allocator-rebind.  Intrusive lists store nothing here — the
    // user owns the nodes — and only track the head / tail / count.

    struct fixed_pool
    {
        node_type   nodes[(_Capacity > 0u) ? _Capacity : 1u];
        bool        used [(_Capacity > 0u) ? _Capacity : 1u];
        std::size_t count;
    };

    struct dynamic_storage
    {
        // node_alloc holds the rebound allocator so we can create
        // nodes of the right type via _Allocator's rebind machinery.
        using node_alloc =
            typename std::allocator_traits<_Allocator>::
                template rebind_alloc<node_type>;

        node_alloc  alloc;
        std::size_t count;
    };

    using storage_type = typename std::conditional<
                             k_fixed,
                             fixed_pool,
                             dynamic_storage>::type;

    // -----------------------------------------------------------------
    // private helpers — declared, defined in the .cpp
    // -----------------------------------------------------------------

    node_type* m_allocate_node();
    void       m_free_node(node_type* _n) noexcept;
    void       m_destroy_all() noexcept;
    void       m_init_sentinels() noexcept;

    template<typename _FwdValue>
    void       m_push_front_impl(_FwdValue&& _v);

    template<typename _FwdValue>
    void       m_push_back_impl(_FwdValue&& _v);

    // -----------------------------------------------------------------
    // state
    // -----------------------------------------------------------------
    storage_type m_store;
    node_type*   m_head;
    node_type*   m_tail;       // unused when k_has_tail == false
    size_type    m_count;
};


// ===========================================================================
// IV.  NAMED SUBTYPES
// ===========================================================================

// ---------------------------------------------------------------------
// direction × end-pointer permutations
// ---------------------------------------------------------------------

// singly_linked_list
//   alias: classic single-direction list with O(1) head access.
template<typename _Value,
         typename _Allocator = std::allocator<_Value>>
using singly_linked_list = linked_list<
    _Value,
    linked_list_option::singly | linked_list_option::head_only,
    0u, 0u, _Allocator>;

// singly_linked_list_with_tail
//   alias: forward-only links but also tracks tail for O(1) push_back.
template<typename _Value,
         typename _Allocator = std::allocator<_Value>>
using singly_linked_list_with_tail = linked_list<
    _Value,
    linked_list_option::singly | linked_list_option::head_and_tail,
    0u, 0u, _Allocator>;

// doubly_linked_list
//   alias: classic two-direction list — std::list-equivalent default.
template<typename _Value,
         typename _Allocator = std::allocator<_Value>>
using doubly_linked_list = linked_list<
    _Value,
    linked_list_option::doubly | linked_list_option::head_and_tail,
    0u, 0u, _Allocator>;

// xor_linked_list
//   alias: bidirectional traversal with single-pointer-per-node
// memory savings.
template<typename _Value,
         typename _Allocator = std::allocator<_Value>>
using xor_linked_list = linked_list<
    _Value,
    linked_list_option::xor_linked | linked_list_option::head_and_tail,
    0u, 0u, _Allocator>;

// ---------------------------------------------------------------------
// circular variants
// ---------------------------------------------------------------------

// circular_singly_linked_list
//   alias: tail's next loops to head.  Useful for round-robin
// schedulers.
template<typename _Value,
         typename _Allocator = std::allocator<_Value>>
using circular_singly_linked_list = linked_list<
    _Value,
    linked_list_option::singly        |
        linked_list_option::head_only |
        linked_list_option::circular,
    0u, 0u, _Allocator>;

// circular_doubly_linked_list
//   alias: bidirectional ring — head's prev == tail.
template<typename _Value,
         typename _Allocator = std::allocator<_Value>>
using circular_doubly_linked_list = linked_list<
    _Value,
    linked_list_option::doubly             |
        linked_list_option::head_and_tail  |
        linked_list_option::circular,
    0u, 0u, _Allocator>;

// ---------------------------------------------------------------------
// sentinel variants
// ---------------------------------------------------------------------

// sentinel_singly_linked_list
//   alias: singly-linked list with a head sentinel; simplifies
// push_front / pop_front to never need a nullptr branch.
template<typename _Value,
         typename _Allocator = std::allocator<_Value>>
using sentinel_singly_linked_list = linked_list<
    _Value,
    linked_list_option::singly       |
        linked_list_option::head_only|
        linked_list_option::head_sentinel,
    0u, 0u, _Allocator>;

// sentinel_doubly_linked_list
//   alias: doubly-linked list with both head and tail sentinels.
// Mirrors libstdc++'s std::list layout (single sentinel that doubles
// as both ends in the circular form, but here we expose them
// distinctly for non-circular layouts).
template<typename _Value,
         typename _Allocator = std::allocator<_Value>>
using sentinel_doubly_linked_list = linked_list<
    _Value,
    linked_list_option::doubly             |
        linked_list_option::head_and_tail  |
        linked_list_option::both_sentinels,
    0u, 0u, _Allocator>;

// ---------------------------------------------------------------------
// fixed-capacity variants
// ---------------------------------------------------------------------

// fixed_singly_linked_list
//   alias: stack-allocated node pool; insertion throws when the pool
// is exhausted.
template<typename    _Value,
         std::size_t _N,
         typename    _Allocator = std::allocator<_Value>>
using fixed_singly_linked_list = linked_list<
    _Value,
    linked_list_option::singly | linked_list_option::head_only,
    0x020u, _N, _Allocator>;   // 0x020u = DContainerOption::fixed_size

// fixed_doubly_linked_list
//   alias: stack-allocated, doubly-linked node pool.
template<typename    _Value,
         std::size_t _N,
         typename    _Allocator = std::allocator<_Value>>
using fixed_doubly_linked_list = linked_list<
    _Value,
    linked_list_option::doubly | linked_list_option::head_and_tail,
    0x020u, _N, _Allocator>;

// ---------------------------------------------------------------------
// intrusive variants
// ---------------------------------------------------------------------

// intrusive_singly_linked_list
//   alias: forward-only intrusive list.  Caller owns the nodes; the
// list maintains only head/tail/count.
template<typename _Value>
using intrusive_singly_linked_list = linked_list<
    _Value,
    linked_list_option::singly         |
        linked_list_option::head_only  |
        linked_list_option::intrusive,
    0u, 0u, std::allocator<_Value>>;

// intrusive_doubly_linked_list
//   alias: bidirectional intrusive list — the workhorse for boost-
// intrusive-style scenarios where allocator overhead is unwanted.
template<typename _Value>
using intrusive_doubly_linked_list = linked_list<
    _Value,
    linked_list_option::doubly             |
        linked_list_option::head_and_tail  |
        linked_list_option::intrusive,
    0u, 0u, std::allocator<_Value>>;

// ---------------------------------------------------------------------
// non-iterable variants
// ---------------------------------------------------------------------

// non_iterable_doubly_linked_list
//   alias: writable doubly-linked list with no begin()/end().  Useful
// when only push/pop/front/back are needed and iteration overhead
// (iterator traits, etc.) should be elided.
template<typename _Value,
         typename _Allocator = std::allocator<_Value>>
using non_iterable_doubly_linked_list = linked_list<
    _Value,
    linked_list_option::doubly | linked_list_option::head_and_tail,
    0x100u, 0u, _Allocator>;   // 0x100u = DContainerOption::non_iterable

// ---------------------------------------------------------------------
// immutable variant
// ---------------------------------------------------------------------

// immutable_doubly_linked_list
//   alias: read-only doubly-linked list.  All mutation methods are
// SFINAE-removed at the class-template level.
template<typename _Value,
         typename _Allocator = std::allocator<_Value>>
using immutable_doubly_linked_list = linked_list<
    _Value,
    linked_list_option::doubly | linked_list_option::head_and_tail,
    0x002u, 0u, _Allocator>;   // 0x002u = DContainerOption::immutable


// ===========================================================================
// V.   DEDUCTION GUIDES (C++17+)
// ===========================================================================

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// linked_list{x, y, z, ...} deduces a doubly_linked_list of the
// element type.
template<typename _First,
         typename... _Rest>
linked_list(_First, _Rest...) ->
    doubly_linked_list<_First>;

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_LINKED_LIST_