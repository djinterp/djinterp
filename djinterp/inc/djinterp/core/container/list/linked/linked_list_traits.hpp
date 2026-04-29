/******************************************************************************
* djinterp [container]                                  linked_list_traits.hpp
*
* Compile-time SFINAE detection for linked-list node and container shapes.
*   Provides structural traits for every permutation along the linked-list
* axes:
*     - link direction      (singly / doubly / xor-linked)
*     - end pointers        (head only / tail only / head and tail)
*     - topology            (linear / circular)
*     - sentinel            (none / head / tail / both)
*     - structure           (flat / hierarchical / skip-leveled)
*     - ownership           (owning / intrusive)
*   All detection is tagless: a node satisfies a trait when its public
* surface (member fields, accessors, or aliases) exhibits the required
* shape.  No registration, no marker types, no opt-in flags required.
*   The companion header linked_list.hpp uses these traits to dispatch
* node selection, iterator selection, and option-flag default
* resolution.  The sister header skip_list.hpp (future) reuses the
* skip-level traits to detect multi-level forward links.
* 
* PORTABILITY:
*   C++11 baseline.  All `_v` aliases gated on
* D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES.  C++20 concepts live in
* linked_list_concepts.hpp.
*
* 
* path:      /inc/djinterp/core/container/list/linked/linked_list_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.   link-presence detection        (next / prev / xor / skip)
2.   node-shape classification      (singly / doubly / xor / skip)
3.   sentinel detection             (head / tail sentinel)
4.   list end-pointer detection     (head pointer / tail pointer)
5.   topology detection             (linear / circular)
6.   ownership detection            (owning / intrusive)
7.   composite list classification  (is_linked_list, is_skip_list)
8.   linked_list_option enum        (axis flags + helpers)
9.   aggregate classification       (linked_list_class<T>)
10.  SFINAE guards                  (enable_if_*)
*/

#ifndef DJINTERP_CONTAINER_LINKED_LIST_TRAITS_
#define DJINTERP_CONTAINER_LINKED_LIST_TRAITS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../meta/type_traits.hpp"


NS_DJINTERP


// ===========================================================================
// 1.  LINK-PRESENCE DETECTION
// ===========================================================================
//   Probes for the canonical link-pointer fields used by linked-list
// nodes.  Each probe is purely structural — it succeeds when the node
// exposes the named member through a member-access expression.

NS_INTERNAL

    // has_next_link_helper
    //   trait: primary template (failure case).  Succeeds when _Type
    // exposes a `.next` member that participates in a member-access
    // expression.
    template<typename _Type,
             typename = void>
    struct has_next_link_helper : std::false_type
    {};

    template<typename _Type>
    struct has_next_link_helper<
        _Type,
        void_t<decltype(std::declval<_Type&>().next)>
    > : std::true_type
    {};

    // has_prev_link_helper
    //   trait: detects a `.prev` member.
    template<typename _Type,
             typename = void>
    struct has_prev_link_helper : std::false_type
    {};

    template<typename _Type>
    struct has_prev_link_helper<
        _Type,
        void_t<decltype(std::declval<_Type&>().prev)>
    > : std::true_type
    {};

    // has_xor_link_helper
    //   trait: detects a `.link` member of an integral / pointer type
    // suitable for storing XOR(prev, next).  Combined with the
    // absence of `.next` / `.prev`, this distinguishes XOR-linked
    // nodes from doubly-linked nodes.
    template<typename _Type,
             typename = void>
    struct has_xor_link_helper : std::false_type
    {};

    template<typename _Type>
    struct has_xor_link_helper<
        _Type,
        void_t<decltype(std::declval<_Type&>().link)>
    > : std::true_type
    {};

    // has_skip_levels_helper
    //   trait: detects a `.forwards` member (array or container of
    // forward links per skip level).  Skip-list nodes expose
    // `forwards[level]` to navigate toward later nodes at each level.
    template<typename _Type,
             typename = void>
    struct has_skip_levels_helper : std::false_type
    {};

    template<typename _Type>
    struct has_skip_levels_helper<
        _Type,
        void_t<decltype(std::declval<_Type&>().forwards[0])>
    > : std::true_type
    {};

    // has_node_value_helper
    //   trait: detects a `.value` data member or `.data()` accessor.
    // Either is sufficient to obtain the payload.
    template<typename _Type,
             typename = void>
    struct has_node_value_helper : std::false_type
    {};

    template<typename _Type>
    struct has_node_value_helper<
        _Type,
        void_t<decltype(std::declval<_Type&>().value)>
    > : std::true_type
    {};

    // has_node_data_method_helper
    template<typename _Type,
             typename = void>
    struct has_node_data_method_helper : std::false_type
    {};

    template<typename _Type>
    struct has_node_data_method_helper<
        _Type,
        void_t<decltype(std::declval<_Type&>().data())>
    > : std::true_type
    {};

NS_END  // internal

// has_next_link
//   trait: true when _Type exposes a `.next` member.
template<typename _Type>
struct has_next_link
    : djinterp::bool_constant<
          internal::has_next_link_helper<_Type>::value>
{};

// has_prev_link
//   trait: true when _Type exposes a `.prev` member.
template<typename _Type>
struct has_prev_link
    : djinterp::bool_constant<
          internal::has_prev_link_helper<_Type>::value>
{};

// has_xor_link
//   trait: true when _Type exposes a `.link` member but no `.next` /
// `.prev` (typical of XOR doubly-linked nodes).
template<typename _Type>
struct has_xor_link
    : djinterp::bool_constant<
          ( internal::has_xor_link_helper<_Type>::value   &&
           !internal::has_next_link_helper<_Type>::value  &&
           !internal::has_prev_link_helper<_Type>::value )>
{};

// has_skip_levels
//   trait: true when _Type exposes a `.forwards` indexable member.
template<typename _Type>
struct has_skip_levels
    : djinterp::bool_constant<
          internal::has_skip_levels_helper<_Type>::value>
{};

// has_node_payload
//   trait: true when _Type exposes either a `.value` member or a
// `.data()` accessor.
template<typename _Type>
struct has_node_payload
    : djinterp::bool_constant<
          ( internal::has_node_value_helper<_Type>::value  ||
            internal::has_node_data_method_helper<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    inline constexpr bool has_next_link_v   = has_next_link<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_prev_link_v   = has_prev_link<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_xor_link_v    = has_xor_link<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_skip_levels_v = has_skip_levels<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_node_payload_v= has_node_payload<_Type>::value;
#endif


// ===========================================================================
// 2.  NODE-SHAPE CLASSIFICATION
// ===========================================================================

// is_singly_linked_node
//   trait: a node with exactly forward (next) connectivity, no prev,
// no xor link, no skip levels.
template<typename _Type>
struct is_singly_linked_node
    : djinterp::bool_constant<
          ( has_next_link<_Type>::value  &&
           !has_prev_link<_Type>::value  &&
           !has_xor_link<_Type>::value   &&
           !has_skip_levels<_Type>::value )>
{};

// is_doubly_linked_node
//   trait: a node with both next and prev member pointers.
template<typename _Type>
struct is_doubly_linked_node
    : djinterp::bool_constant<
          ( has_next_link<_Type>::value  &&
            has_prev_link<_Type>::value  &&
           !has_skip_levels<_Type>::value )>
{};

// is_xor_linked_node
//   trait: a node carrying its prev/next as a single XOR-combined
// pointer in the `.link` member.
template<typename _Type>
struct is_xor_linked_node
    : djinterp::bool_constant<has_xor_link<_Type>::value>
{};

// is_skip_list_node
//   trait: a node carrying multi-level forward links through a
// `.forwards` array.  Strictly stronger than singly_linked.
template<typename _Type>
struct is_skip_list_node
    : djinterp::bool_constant<
          has_skip_levels<_Type>::value>
{};

// is_linked_list_node
//   trait: composite — the node is linked-list-shaped under any of
// the three direct schemes (singly / doubly / xor) and exposes a
// payload accessor.
template<typename _Type>
struct is_linked_list_node
    : djinterp::bool_constant<
          ( ( is_singly_linked_node<_Type>::value  ||
              is_doubly_linked_node<_Type>::value  ||
              is_xor_linked_node<_Type>::value     ||
              is_skip_list_node<_Type>::value )    &&
            has_node_payload<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    inline constexpr bool is_singly_linked_node_v =
        is_singly_linked_node<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_doubly_linked_node_v =
        is_doubly_linked_node<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_xor_linked_node_v =
        is_xor_linked_node<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_skip_list_node_v =
        is_skip_list_node<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_linked_list_node_v =
        is_linked_list_node<_Type>::value;
#endif


// ===========================================================================
// 3.  SENTINEL DETECTION
// ===========================================================================
//   A sentinel (or dummy) is a non-data node placed before the first
// or after the last real element to simplify boundary handling.

NS_INTERNAL

    // has_head_sentinel_helper
    //   trait: detects a static `has_head_sentinel` member equal to
    // true.  The two-step probe (existence + truth) keeps the trait
    // tagless: any list type that exposes a `has_head_sentinel`
    // static bool is automatically classified, no opt-in registration
    // required.
    template<typename _Type,
             typename = void>
    struct has_head_sentinel_helper : std::false_type
    {};

    template<typename _Type>
    struct has_head_sentinel_helper<
        _Type,
        void_t<decltype(_Type::has_head_sentinel)>
    > : djinterp::bool_constant<
            static_cast<bool>(_Type::has_head_sentinel)>
    {};

    // has_tail_sentinel_helper
    //   trait: same shape as has_head_sentinel_helper for the tail.
    template<typename _Type,
             typename = void>
    struct has_tail_sentinel_helper : std::false_type
    {};

    template<typename _Type>
    struct has_tail_sentinel_helper<
        _Type,
        void_t<decltype(_Type::has_tail_sentinel)>
    > : djinterp::bool_constant<
            static_cast<bool>(_Type::has_tail_sentinel)>
    {};

NS_END  // internal

// has_head_sentinel
//   trait: true when the list type advertises a head sentinel via
// the `has_head_sentinel` static bool member.
template<typename _Type>
struct has_head_sentinel
    : djinterp::bool_constant<
          internal::has_head_sentinel_helper<_Type>::value>
{};

// has_tail_sentinel
//   trait: true when the list type advertises a tail sentinel via
// the `has_tail_sentinel` static bool member.
template<typename _Type>
struct has_tail_sentinel
    : djinterp::bool_constant<
          internal::has_tail_sentinel_helper<_Type>::value>
{};

// has_any_sentinel
//   trait: true when the list exposes either a head or a tail
// sentinel.
template<typename _Type>
struct has_any_sentinel
    : djinterp::bool_constant<
          ( has_head_sentinel<_Type>::value ||
            has_tail_sentinel<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    inline constexpr bool has_head_sentinel_v =
        has_head_sentinel<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_tail_sentinel_v =
        has_tail_sentinel<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_any_sentinel_v =
        has_any_sentinel<_Type>::value;
#endif


// ===========================================================================
// 4.  LIST END-POINTER DETECTION
// ===========================================================================

NS_INTERNAL
    // has_head_pointer_helper
    //   trait: detects `head()` accessor on a const lvalue.
    template<typename _Type,
             typename = void>
    struct has_head_pointer_helper : std::false_type
    {};

    template<typename _Type>
    struct has_head_pointer_helper<
        _Type,
        void_t<decltype(std::declval<const _Type&>().head())>
    > : std::true_type
    {};

    // has_tail_pointer_helper
    //   trait: detects `tail()` accessor on a const lvalue.
    template<typename _Type,
             typename = void>
    struct has_tail_pointer_helper : std::false_type
    {};

    template<typename _Type>
    struct has_tail_pointer_helper<
        _Type,
        void_t<decltype(std::declval<const _Type&>().tail())>
    > : std::true_type
    {};

NS_END  // internal

// has_head_pointer
//   trait: true when the list exposes a head() accessor.
template<typename _Type>
struct has_head_pointer
    : djinterp::bool_constant<
          internal::has_head_pointer_helper<_Type>::value>
{};

// has_tail_pointer
//   trait: true when the list exposes a tail() accessor.
template<typename _Type>
struct has_tail_pointer
    : djinterp::bool_constant<
          internal::has_tail_pointer_helper<_Type>::value>
{};

// is_head_only_list
//   trait: true when the list exposes head() but no tail().
template<typename _Type>
struct is_head_only_list
    : djinterp::bool_constant<
          ( has_head_pointer<_Type>::value &&
           !has_tail_pointer<_Type>::value )>
{};

// is_head_tail_list
//   trait: true when the list exposes both head() and tail().
template<typename _Type>
struct is_head_tail_list
    : djinterp::bool_constant<
          ( has_head_pointer<_Type>::value &&
            has_tail_pointer<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    inline constexpr bool has_head_pointer_v =
        has_head_pointer<_Type>::value;

    template<typename _Type>
    inline constexpr bool has_tail_pointer_v =
        has_tail_pointer<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_head_only_list_v =
        is_head_only_list<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_head_tail_list_v =
        is_head_tail_list<_Type>::value;
#endif


// ===========================================================================
// 5.  TOPOLOGY DETECTION
// ===========================================================================

NS_INTERNAL
    // is_circular_list_helper
    //   trait: detects an `is_circular` static constant exposed
    // either as a member alias of std::true_type or as a
    // compile-time bool.
    template<typename _Type,
             typename = void>
    struct is_circular_list_helper : std::false_type
    {};

    template<typename _Type>
    struct is_circular_list_helper<
        _Type,
        void_t<decltype(_Type::is_circular)>
    > : djinterp::bool_constant<
            static_cast<bool>(_Type::is_circular)>
    {};

NS_END  // internal

// is_circular_list
//   trait: true when the list type exposes an `is_circular` static
// boolean equal to true.  Linear (non-circular) is the default.
template<typename _Type>
struct is_circular_list
    : djinterp::bool_constant<internal::is_circular_list_helper<_Type>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    inline constexpr bool is_circular_list_v =
        is_circular_list<_Type>::value;
#endif


// ===========================================================================
// 6.  OWNERSHIP DETECTION
// ===========================================================================

NS_INTERNAL

    // is_intrusive_list_helper
    //   trait: detects an `is_intrusive` static boolean.  Intrusive
    // lists do not own their nodes; the user embeds the link members
    // in their own type.
    template<typename _Type,
             typename = void>
    struct is_intrusive_list_helper : std::false_type
    {};

    template<typename _Type>
    struct is_intrusive_list_helper<
        _Type,
        void_t<decltype(_Type::is_intrusive)>
    > : djinterp::bool_constant<
            static_cast<bool>(_Type::is_intrusive)>
    {};

NS_END  // internal

// is_intrusive_list
//   trait: true when the list does not own its nodes.
template<typename _Type>
struct is_intrusive_list
    : djinterp::bool_constant<
          internal::is_intrusive_list_helper<_Type>::value>
{};

// is_owning_list
//   trait: true when the list owns its nodes (the default).
template<typename _Type>
struct is_owning_list
    : djinterp::bool_constant<
          !is_intrusive_list<_Type>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    inline constexpr bool is_intrusive_list_v =
        is_intrusive_list<_Type>::value;
    template<typename _Type>
    inline constexpr bool is_owning_list_v =
        is_owning_list<_Type>::value;
#endif


// ===========================================================================
// 7.  COMPOSITE LIST CLASSIFICATION
// ===========================================================================

NS_INTERNAL

    // has_list_node_type_helper
    //   trait: detects a nested `node_type` alias on the container.
    template<typename _Type,
             typename = void>
    struct has_list_node_type_helper : std::false_type
    {};

    template<typename _Type>
    struct has_list_node_type_helper<
        _Type,
        void_t<typename _Type::node_type>
    > : std::true_type
    {};

NS_END  // internal

// has_list_node_type
//   trait: true when the container exposes a node_type alias.
template<typename _Type>
struct has_list_node_type
    : djinterp::bool_constant<
          internal::has_list_node_type_helper<_Type>::value>
{};

// is_singly_linked_list
//   trait: composite — the container has a node_type that is
// singly-linked-shaped, and the container exposes head().
template<typename _Type,
         typename = void>
struct is_singly_linked_list : std::false_type
{};

template<typename _Type>
struct is_singly_linked_list<
    _Type,
    typename std::enable_if<
        has_list_node_type<_Type>::value>::type>
    : djinterp::bool_constant<
          ( is_singly_linked_node<typename _Type::node_type>::value &&
            has_head_pointer<_Type>::value )>
{};

// is_doubly_linked_list
//   trait: composite — node is doubly-linked-shaped and the
// container exposes head() (tail() is preferred but optional in
// the singleton-circular case).
template<typename _Type,
         typename = void>
struct is_doubly_linked_list : std::false_type
{};

template<typename _Type>
struct is_doubly_linked_list<
    _Type,
    typename std::enable_if<
        has_list_node_type<_Type>::value>::type>
    : djinterp::bool_constant<
          ( is_doubly_linked_node<typename _Type::node_type>::value &&
            has_head_pointer<_Type>::value )>
{};

// is_xor_linked_list
//   trait: composite — node uses XOR-linked layout.
template<typename _Type,
         typename = void>
struct is_xor_linked_list : std::false_type
{};

template<typename _Type>
struct is_xor_linked_list<
    _Type,
    typename std::enable_if<
        has_list_node_type<_Type>::value>::type>
    : djinterp::bool_constant<
          ( is_xor_linked_node<typename _Type::node_type>::value &&
            has_head_pointer<_Type>::value )>
{};

// is_skip_list
//   trait: composite — node has skip levels and the container
// exposes head().  Reserved for the forthcoming skip_list.hpp.
template<typename _Type,
         typename = void>
struct is_skip_list : std::false_type
{};

template<typename _Type>
struct is_skip_list<
    _Type,
    typename std::enable_if<
        has_list_node_type<_Type>::value>::type>
    : djinterp::bool_constant<
          ( is_skip_list_node<typename _Type::node_type>::value &&
            has_head_pointer<_Type>::value )>
{};

// is_linked_list
//   trait: composite — true for any of the four shapes above.
template<typename _Type>
struct is_linked_list
    : djinterp::bool_constant<
          ( is_singly_linked_list<_Type>::value ||
            is_doubly_linked_list<_Type>::value ||
            is_xor_linked_list<_Type>::value    ||
            is_skip_list<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    inline constexpr bool has_list_node_type_v =
        has_list_node_type<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_singly_linked_list_v =
        is_singly_linked_list<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_doubly_linked_list_v =
        is_doubly_linked_list<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_xor_linked_list_v =
        is_xor_linked_list<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_skip_list_v =
        is_skip_list<_Type>::value;

    template<typename _Type>
    inline constexpr bool is_linked_list_v =
        is_linked_list<_Type>::value;
#endif


// ===========================================================================
// 8.  linked_list_option ENUM
// ===========================================================================
//   Axis-organized bitmask describing every linked-list-specific
// permutation orthogonal to the framework-wide DContainerOption.
// Combined with DContainerOption, these together describe a fully
// qualified linked_list configuration.

// linked_list_option
//   enum: linked-list-specific axis bits.
//
//   Axis layout:
//     direction (bits 0-2): singly / doubly / xor_linked
//     ends      (bits 3-5): head_only / tail_only / head_and_tail
//     topology  (bit  6  ): circular   (linear is default)
//     sentinel  (bits 7-9): head_sentinel / tail_sentinel / both_sentinels
//     ownership (bit  10 ): intrusive  (owning is default)
//     structure (bit  11 ): hierarchical (flat is default)
enum class linked_list_option : unsigned
{
    none           = 0x000u,

    // direction axis (bits 0-2)
    singly         = 0x001u,
    doubly         = 0x002u,
    xor_linked     = 0x004u,

    // end-pointer axis (bits 3-5)
    head_only      = 0x008u,
    tail_only      = 0x010u,
    head_and_tail  = 0x020u,

    // topology axis (bit 6)
    circular       = 0x040u,

    // sentinel axis (bits 7-9)
    head_sentinel  = 0x080u,
    tail_sentinel  = 0x100u,
    both_sentinels = 0x200u,

    // ownership axis (bit 10)
    intrusive      = 0x400u,

    // structure axis (bit 11)
    hierarchical   = 0x800u
};

// operator|
//   operator: bitwise OR of linked_list_option flags.
inline constexpr linked_list_option
operator|(
    linked_list_option _lhs,
    linked_list_option _rhs
) noexcept
{
    return static_cast<linked_list_option>(
        static_cast<unsigned>(_lhs) | static_cast<unsigned>(_rhs)
    );
}

// operator&
//   operator: bitwise AND of linked_list_option flags.
inline constexpr linked_list_option
operator&(
    linked_list_option _lhs,
    linked_list_option _rhs
) noexcept
{
    return static_cast<linked_list_option>(
        static_cast<unsigned>(_lhs) & static_cast<unsigned>(_rhs));
}

// operator~
//   operator: bitwise NOT of a linked_list_option flag.
inline constexpr linked_list_option
operator~(
    linked_list_option _v
) noexcept
{
    return static_cast<linked_list_option>(~static_cast<unsigned>(_v));
}

// linked_list_option_has
//   function: returns true when _flags has all bits of _bit set.
inline constexpr bool
linked_list_option_has(
    linked_list_option _flags,
    linked_list_option _bit
) noexcept
{
    return ( (_flags & _bit) == _bit);
}

// D_LINKED_LIST_OPTION_DIRECTION_MASK
//   constant: covers the link-direction axis bits.
#define D_LINKED_LIST_OPTION_DIRECTION_MASK                                   \
    ( djinterp::linked_list_option::singly         |                          \
      djinterp::linked_list_option::doubly         |                          \
      djinterp::linked_list_option::xor_linked )   
                                                   
// D_LINKED_LIST_OPTION_ENDS_MASK                  
//   constant: covers the end-pointer axis bits.   
#define D_LINKED_LIST_OPTION_ENDS_MASK                                        \
    ( djinterp::linked_list_option::head_only      |                          \
      djinterp::linked_list_option::tail_only      |                          \
      djinterp::linked_list_option::head_and_tail )

// D_LINKED_LIST_OPTION_SENTINEL_MASK
//   constant: covers the sentinel axis bits.
#define D_LINKED_LIST_OPTION_SENTINEL_MASK                                    \
    ( djinterp::linked_list_option::head_sentinel  |                          \
      djinterp::linked_list_option::tail_sentinel  |                          \
      djinterp::linked_list_option::both_sentinels )

// linked_list_option_direction
//   function: isolates the direction axis bits.
inline constexpr linked_list_option
linked_list_option_direction(
    linked_list_option _flags
) noexcept
{
    return (_flags & D_LINKED_LIST_OPTION_DIRECTION_MASK);
}

// linked_list_option_ends
//   function: isolates the end-pointer axis bits.
inline constexpr linked_list_option
linked_list_option_ends(
    linked_list_option _flags
) noexcept
{
    return (_flags & D_LINKED_LIST_OPTION_ENDS_MASK);
}

// linked_list_option_sentinel
//   function: isolates the sentinel axis bits.
inline constexpr linked_list_option
linked_list_option_sentinel(
    linked_list_option _flags
) noexcept
{
    return (_flags & D_LINKED_LIST_OPTION_SENTINEL_MASK);
}

NS_INTERNAL

    // popcount_constexpr
    //   function: counts set bits using Brian Kernighan's algorithm
    // expressed recursively for C++11 constexpr compatibility.
    inline constexpr std::size_t
    ll_popcount_constexpr(
        unsigned _v
    ) noexcept
    {
        return (_v == 0u)
                 ? 0u
                 : (1u + ll_popcount_constexpr(_v & (_v - 1u)));
    }

NS_END  // internal

// linked_list_option_axis_valid
//   function: returns true when at most one bit is set within each
// of the direction, end-pointer, and sentinel axes.
inline constexpr bool
linked_list_option_axis_valid(
    linked_list_option _flags
) noexcept
{
    return (
        ( internal::ll_popcount_constexpr(
              static_cast<unsigned>(linked_list_option_direction(_flags)))
          <= 1u ) &&
        ( internal::ll_popcount_constexpr(
              static_cast<unsigned>(linked_list_option_ends(_flags)))
          <= 1u ) &&
        ( internal::ll_popcount_constexpr(
              static_cast<unsigned>(linked_list_option_sentinel(_flags)))
          <= 1u ) );
}


// ===========================================================================
// 9.  AGGREGATE CLASSIFICATION STRUCT
// ===========================================================================

// linked_list_class
//   struct: comprehensive aggregation of a list type's
// classification.  Mirrors radix_tree_class<T> /
// tree_container_class<T>.
template<typename _Type>
struct linked_list_class
{
    // identity
    static constexpr bool is_list       = is_linked_list<_Type>::value;
    static constexpr bool is_singly     = is_singly_linked_list<_Type>::value;
    static constexpr bool is_doubly     = is_doubly_linked_list<_Type>::value;
    static constexpr bool is_xor        = is_xor_linked_list<_Type>::value;
    static constexpr bool is_skip       = is_skip_list<_Type>::value;

    // ends
    static constexpr bool has_head      = has_head_pointer<_Type>::value;
    static constexpr bool has_tail      = has_tail_pointer<_Type>::value;
    static constexpr bool has_head_only = is_head_only_list<_Type>::value;
    static constexpr bool has_head_tail = is_head_tail_list<_Type>::value;

    // sentinels
    static constexpr bool has_head_sent = has_head_sentinel<_Type>::value;
    static constexpr bool has_tail_sent = has_tail_sentinel<_Type>::value;

    // topology
    static constexpr bool is_circular   = is_circular_list<_Type>::value;
    static constexpr bool is_linear     = !is_circular;

    // ownership
    static constexpr bool is_intrusive  = is_intrusive_list<_Type>::value;
    static constexpr bool is_owning     = !is_intrusive;
};


// ===========================================================================
// 10.   SFINAE GUARDS
// ===========================================================================

// enable_if_linked_list
//   trait: SFINAE guard for templates restricted to linked lists.
template<typename _Type>
struct enable_if_linked_list
{
    using type = typename std::enable_if< is_linked_list<_Type>::value>::type;
};

template<typename _Type>
using enable_if_linked_list_t = typename enable_if_linked_list<_Type>::type;

// enable_if_singly_linked_list
template<typename _Type>
struct enable_if_singly_linked_list
{
    using type = typename std::enable_if<
        is_singly_linked_list<_Type>::value
    >::type;
};

template<typename _Type>
using enable_if_singly_linked_list_t =
    typename enable_if_singly_linked_list<_Type>::type;

// enable_if_doubly_linked_list
template<typename _Type>
struct enable_if_doubly_linked_list
{
    using type = typename std::enable_if<
        is_doubly_linked_list<_Type>::value>::type;
};

template<typename _Type>
using enable_if_doubly_linked_list_t =
    typename enable_if_doubly_linked_list<_Type>::type;

// enable_if_linked_list_node
template<typename _Type>
struct enable_if_linked_list_node
{
    using type = typename std::enable_if<
        is_linked_list_node<_Type>::value>::type;
};

template<typename _Type>
using enable_if_linked_list_node_t =
    typename enable_if_linked_list_node<_Type>::type;


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_LINKED_LIST_TRAITS_
