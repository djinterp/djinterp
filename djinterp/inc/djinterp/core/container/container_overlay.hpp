/******************************************************************************
* djinterp [container]                                       container_overlay.hpp
*
* djinterp container overlay module:
*   This header provides the idioms for defining zero-overhead container
* overlays. An overlay is the reusable, named form of a Tier 0 (View)
* conversion: it delegates to an `underlying_container_type`, holds the
* underlying by `const&` (never copies), and is structurally transparent to
* the trait system on every classification axis except the one it changes.
*
*   Because djinterp classification is purely structural (no base class, no
* tags), an overlay changes an axis simply by choosing which nested aliases
* it re-exposes. The free-view conversions become named types:
*   - mutable   -> immutable  (immutable_overlay)
*   - sorted    -> unsorted   (unsorted_overlay)
*   - unique    -> multi      (multi_overlay)
*   - bounded   -> unbounded  (unbounded_overlay)
*
*   ZERO-OVERHEAD GUARANTEES:
*   - one reference-sized member; empty mixins fold away under EBO
*   - every forwarder is `D_CONSTEXPR_INLINE` and propagates `noexcept`
*   - no virtual functions, no RTTI, no allocation, no exceptions
*   - SFINAE-guarded forwarders preserve the underlying's iteration level,
*     contiguity, reverse/const iteration, hierarchy, and bounds intact
*
*   PORTABILITY:
*   version: C++17 or higher
*   dependencies:
*   - `djinterp.hpp`:    for the NS_* macros, clean_t, and the D_CONSTEXPR /
*                        D_INLINE / D_NODISCARD family.
*   - `type_traits.hpp`: for the detection idiom (is_detected, nonesuch).
*
* path:      /inc/djinterp/core/container/container_overlay.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_OVERLAY_
#define DJINTERP_CONTAINER_OVERLAY_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"

// require C++17 for the overlay baseline
#if !D_ENV_LANG_IS_CPP17_OR_HIGHER
    #error "container_overlay.hpp requires C++17 or later"
#endif


NS_DJINTERP

// =============================================================================
// I.   OVERLAY TAG
// =============================================================================

// overlay_tag
//   type: empty marker base inherited by every djinterp overlay. Lets the
// trait system distinguish a framework overlay from an arbitrary type that
// merely happens to expose an `underlying_container_type` alias. Empty, so
// it folds away under empty-base optimization.
struct overlay_tag
{};


// =============================================================================
// II.  CONDITIONAL ALIAS FORWARDING
// =============================================================================
// The structural heart of an overlay. Each mixin forwards one nested type
// alias from the underlying container -- but only if the underlying actually
// provides it. An overlay then inherits exactly the groups of aliases it
// wishes to preserve; the axis it changes is the group it omits.

NS_INTERNAL

// member probes for the detection idiom: each yields the corresponding
// nested alias of `_Container`, or is ill-formed when it is absent.
template<typename _Container>
using probe_value_type = typename _Container::value_type;

template<typename _Container>
using probe_size_type = typename _Container::size_type;

template<typename _Container>
using probe_difference_type = typename _Container::difference_type;

template<typename _Container>
using probe_allocator_type = typename _Container::allocator_type;

template<typename _Container>
using probe_iterator = typename _Container::iterator;

template<typename _Container>
using probe_const_iterator = typename _Container::const_iterator;

template<typename _Container>
using probe_reverse_iterator = typename _Container::reverse_iterator;

template<typename _Container>
using probe_const_reverse_iterator =
    typename _Container::const_reverse_iterator;

template<typename _Container>
using probe_node_type = typename _Container::node_type;

template<typename _Container>
using probe_depth_type = typename _Container::depth_type;

template<typename _Container>
using probe_key_compare = typename _Container::key_compare;

template<typename _Container>
using probe_value_compare = typename _Container::value_compare;

template<typename _Container>
using probe_hasher = typename _Container::hasher;

template<typename _Container>
using probe_key_type = typename _Container::key_type;

template<typename _Container>
using probe_mapped_type = typename _Container::mapped_type;

template<typename _Container>
using probe_size_interval = typename _Container::size_interval;

template<typename _Container>
using probe_depth_interval = typename _Container::depth_interval;

template<typename _Container>
using probe_multiplicity_interval =
    typename _Container::multiplicity_interval;

NS_END  // internal

// D_INTERNAL_OVERLAY_FORWARD_ALIAS
//   macro: emits a bool-specialized mixin `MIXIN_NAME<_Container>` that
// re-exposes `_Container::ALIAS_NAME` when the probe `PROBE_NAME` detects
// it, and is empty otherwise. The empty primary template means inheriting
// the mixin for a container lacking the alias costs nothing and adds no
// classification.
#define D_INTERNAL_OVERLAY_FORWARD_ALIAS(MIXIN_NAME, ALIAS_NAME, PROBE_NAME)  \
    template<typename _Container,                                             \
             bool     _Present =                                              \
                 is_detected<internal::PROBE_NAME, _Container>::value>        \
    struct MIXIN_NAME                                                         \
    {};                                                                       \
                                                                              \
    template<typename _Container>                                             \
    struct MIXIN_NAME<_Container, true>                                       \
    {                                                                         \
        using ALIAS_NAME = typename _Container::ALIAS_NAME;                   \
    };

// common aliases: forwarded by every overlay. None of these flips one of
// the axes the named overlays toggle, so they are always preserved.
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_value_type,
                                 value_type,
                                 probe_value_type)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_size_type,
                                 size_type,
                                 probe_size_type)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_difference_type,
                                 difference_type,
                                 probe_difference_type)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_allocator_type,
                                 allocator_type,
                                 probe_allocator_type)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_iterator,
                                 iterator,
                                 probe_iterator)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_const_iterator,
                                 const_iterator,
                                 probe_const_iterator)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_reverse_iterator,
                                 reverse_iterator,
                                 probe_reverse_iterator)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_const_reverse_iterator,
                                 const_reverse_iterator,
                                 probe_const_reverse_iterator)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_node_type,
                                 node_type,
                                 probe_node_type)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_depth_type,
                                 depth_type,
                                 probe_depth_type)

// ordering aliases: presence of `key_compare` without `hasher` is what the
// trait system reads as the sorted invariant. Omitting this group is how
// `unsorted_overlay` performs the sorted -> unsorted view.
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_key_compare,
                                 key_compare,
                                 probe_key_compare)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_value_compare,
                                 value_compare,
                                 probe_value_compare)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_hasher,
                                 hasher,
                                 probe_hasher)

// uniqueness alias: `key_type` without `mapped_type` is read as the
// uniqueness invariant. Omitting it performs the unique -> multi view.
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_key_type,
                                 key_type,
                                 probe_key_type)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_mapped_type,
                                 mapped_type,
                                 probe_mapped_type)

// bounds aliases: the interval types drive bounded classification. Omitting
// this group performs the bounded -> unbounded view.
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_size_interval,
                                 size_interval,
                                 probe_size_interval)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_depth_interval,
                                 depth_interval,
                                 probe_depth_interval)
D_INTERNAL_OVERLAY_FORWARD_ALIAS(forward_multiplicity_interval,
                                 multiplicity_interval,
                                 probe_multiplicity_interval)


// =============================================================================
// III. ALIAS GROUPS
// =============================================================================
// Axis-aligned bundles. Each named overlay is the common bundle plus the
// groups for the axes it preserves, minus the group for the axis it flips.

// forward_common_aliases
//   type: every alias that does not by itself flip a toggled axis. Inherited
// by overlay_base, hence by all overlays.
template<typename _Container>
struct forward_common_aliases
    : forward_value_type<_Container>,
      forward_size_type<_Container>,
      forward_difference_type<_Container>,
      forward_allocator_type<_Container>,
      forward_iterator<_Container>,
      forward_const_iterator<_Container>,
      forward_reverse_iterator<_Container>,
      forward_const_reverse_iterator<_Container>,
      forward_node_type<_Container>,
      forward_depth_type<_Container>
{};

// forward_ordering_aliases
//   type: the sorted-invariant group (key_compare / value_compare / hasher).
template<typename _Container>
struct forward_ordering_aliases
    : forward_key_compare<_Container>,
      forward_value_compare<_Container>,
      forward_hasher<_Container>
{};

// forward_uniqueness_aliases
//   type: the uniqueness-invariant group (key_type).
template<typename _Container>
struct forward_uniqueness_aliases
    : forward_key_type<_Container>
{};

// forward_associative_aliases
//   type: the associative group (mapped_type). Preserved independently of
// uniqueness so a map viewed as multi keeps its mapped_type.
template<typename _Container>
struct forward_associative_aliases
    : forward_mapped_type<_Container>
{};

// forward_bounds_aliases
//   type: the bounds group (size / depth / multiplicity intervals).
template<typename _Container>
struct forward_bounds_aliases
    : forward_size_interval<_Container>,
      forward_depth_interval<_Container>,
      forward_multiplicity_interval<_Container>
{};


// =============================================================================
// IV.  OVERLAY BASE
// =============================================================================

// overlay_base
//   class: zero-overhead base for container overlays. Holds the underlying
// by `const&`, re-exposes the read surface through SFINAE-guarded forwarders
// (so iteration level, contiguity, reverse/const iteration, and hierarchy are
// preserved exactly when present), and advertises `underlying_container_type`
// so the trait system classifies the overlay on axis 9 as backed.
template<typename _Container>
class overlay_base : public overlay_tag,
                     public forward_common_aliases<clean_t<_Container>>
{
public:
    // underlying_container_type
    //   type: the delegated container (axis 9 classification probe).
    using underlying_container_type = clean_t<_Container>;

    explicit overlay_base(
        const underlying_container_type& _underlying
    ) noexcept
        : m_underlying(_underlying)
    {}

    // underlying
    //   accessor: the wrapped container. none of the overlay's behaviour is
    // hidden -- callers may always recover the original.
    D_NODISCARD D_CONSTEXPR_INLINE
    const underlying_container_type& underlying() const noexcept
    {
        return m_underlying;
    }

    // begin / end
    //   forwarders: const iteration over the underlying. Present only when
    // the underlying is iterable, so the overlay never fabricates a surface
    // its target lacks.
    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto begin() const noexcept(noexcept(std::declval<const _C&>().begin()))
        -> decltype(std::declval<const _C&>().begin())
    {
        return m_underlying.begin();
    }

    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto end() const noexcept(noexcept(std::declval<const _C&>().end()))
        -> decltype(std::declval<const _C&>().end())
    {
        return m_underlying.end();
    }

    // cbegin / cend
    //   forwarders: explicit const iteration (preserves has_const_iteration).
    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto cbegin() const noexcept(noexcept(std::declval<const _C&>().cbegin()))
        -> decltype(std::declval<const _C&>().cbegin())
    {
        return m_underlying.cbegin();
    }

    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto cend() const noexcept(noexcept(std::declval<const _C&>().cend()))
        -> decltype(std::declval<const _C&>().cend())
    {
        return m_underlying.cend();
    }

    // rbegin / rend
    //   forwarders: reverse iteration (preserves has_reverse_iteration).
    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto rbegin() const noexcept(noexcept(std::declval<const _C&>().rbegin()))
        -> decltype(std::declval<const _C&>().rbegin())
    {
        return m_underlying.rbegin();
    }

    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto rend() const noexcept(noexcept(std::declval<const _C&>().rend()))
        -> decltype(std::declval<const _C&>().rend())
    {
        return m_underlying.rend();
    }

    // size / empty
    //   forwarders: size surface (preserves sized / bounded detection).
    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto size() const noexcept(noexcept(std::declval<const _C&>().size()))
        -> decltype(std::declval<const _C&>().size())
    {
        return m_underlying.size();
    }

    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto empty() const noexcept(noexcept(std::declval<const _C&>().empty()))
        -> decltype(std::declval<const _C&>().empty())
    {
        return m_underlying.empty();
    }

    // data
    //   forwarder: contiguous storage pointer (preserves contiguity / the
    // array classification when the underlying is contiguous).
    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto data() const noexcept(noexcept(std::declval<const _C&>().data()))
        -> decltype(std::declval<const _C&>().data())
    {
        return m_underlying.data();
    }

    // operator[]
    //   forwarder: random-access element read (preserves random_access).
    template<typename _Index,
             typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto operator[](_Index _index) const
        -> decltype(std::declval<const _C&>()[_index])
    {
        return m_underlying[_index];
    }

    // parent / children / root
    //   forwarders: hierarchical navigation (preserves the structure axis so
    // a tree viewed through an overlay stays hierarchical).
    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto parent() const noexcept(noexcept(std::declval<const _C&>().parent()))
        -> decltype(std::declval<const _C&>().parent())
    {
        return m_underlying.parent();
    }

    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto children() const
        noexcept(noexcept(std::declval<const _C&>().children()))
        -> decltype(std::declval<const _C&>().children())
    {
        return m_underlying.children();
    }

    template<typename _C = underlying_container_type>
    D_NODISCARD D_CONSTEXPR_INLINE
    auto root() const noexcept(noexcept(std::declval<const _C&>().root()))
        -> decltype(std::declval<const _C&>().root())
    {
        return m_underlying.root();
    }

private:
    const underlying_container_type& m_underlying;
};


// =============================================================================
// V.   NAMED OVERLAYS
// =============================================================================
// Each overlay preserves every axis but one. The flipped axis is the alias
// group it does not inherit; everything else is forwarded transparently.

// immutable_overlay
//   class: presents a container as immutable (mutable -> immutable view).
// Every readable axis is preserved; mutators are simply never forwarded, so
// the trait system classifies the overlay as immutable.
template<typename _Container>
class immutable_overlay
    : public overlay_base<_Container>,
      public forward_ordering_aliases<clean_t<_Container>>,
      public forward_uniqueness_aliases<clean_t<_Container>>,
      public forward_associative_aliases<clean_t<_Container>>,
      public forward_bounds_aliases<clean_t<_Container>>
{
public:
    using overlay_base<_Container>::overlay_base;
};

// unsorted_overlay
//   class: drops the sorted invariant (sorted -> unsorted view). Omits the
// ordering alias group; uniqueness, associativity, and bounds are preserved.
template<typename _Container>
class unsorted_overlay
    : public overlay_base<_Container>,
      public forward_uniqueness_aliases<clean_t<_Container>>,
      public forward_associative_aliases<clean_t<_Container>>,
      public forward_bounds_aliases<clean_t<_Container>>
{
public:
    using overlay_base<_Container>::overlay_base;
};

// multi_overlay
//   class: drops the uniqueness invariant (unique -> multi view). Omits the
// uniqueness alias group; ordering, associativity, and bounds are preserved.
template<typename _Container>
class multi_overlay
    : public overlay_base<_Container>,
      public forward_ordering_aliases<clean_t<_Container>>,
      public forward_associative_aliases<clean_t<_Container>>,
      public forward_bounds_aliases<clean_t<_Container>>
{
public:
    using overlay_base<_Container>::overlay_base;
};

// unbounded_overlay
//   class: drops size constraints (bounded -> unbounded view). Omits the
// bounds alias group; ordering, uniqueness, and associativity are preserved.
template<typename _Container>
class unbounded_overlay
    : public overlay_base<_Container>,
      public forward_ordering_aliases<clean_t<_Container>>,
      public forward_uniqueness_aliases<clean_t<_Container>>,
      public forward_associative_aliases<clean_t<_Container>>
{
public:
    using overlay_base<_Container>::overlay_base;
};


// =============================================================================
// VI.  FACTORY FUNCTIONS
// =============================================================================
// Call-site ergonomics: deduce the underlying type and build the overlay by
// value (cheap -- it holds a reference). The overlay is bound to the lifetime
// of the argument, exactly as the framework's other views are.

// as_immutable
//   function: view `_container` as immutable.
template<typename _Container>
D_NODISCARD D_CONSTEXPR_INLINE
immutable_overlay<_Container> as_immutable(
    const _Container& _container
) noexcept
{
    return immutable_overlay<_Container>(_container);
}

// as_unsorted
//   function: view `_container` with its sorted invariant ignored.
template<typename _Container>
D_NODISCARD D_CONSTEXPR_INLINE
unsorted_overlay<_Container> as_unsorted(
    const _Container& _container
) noexcept
{
    return unsorted_overlay<_Container>(_container);
}

// as_multi
//   function: view `_container` with its uniqueness invariant ignored.
template<typename _Container>
D_NODISCARD D_CONSTEXPR_INLINE
multi_overlay<_Container> as_multi(
    const _Container& _container
) noexcept
{
    return multi_overlay<_Container>(_container);
}

// as_unbounded
//   function: view `_container` with its size bounds ignored.
template<typename _Container>
D_NODISCARD D_CONSTEXPR_INLINE
unbounded_overlay<_Container> as_unbounded(
    const _Container& _container
) noexcept
{
    return unbounded_overlay<_Container>(_container);
}


// =============================================================================
// VII. OVERLAY INTROSPECTION
// =============================================================================


// is_container_overlay
//   trait: satisfied when `_Type` is a djinterp overlay (derives from
// overlay_tag). Distinguishes framework overlays from arbitrary backed
// containers that merely expose `underlying_container_type`.
template<typename _Type>
struct is_container_overlay
    : std::is_base_of<overlay_tag, clean_t<_Type>>
{};

// is_container_overlay_v
//   value: convenience variable template for is_container_overlay.
template<typename _Type>
inline constexpr bool is_container_overlay_v =
    is_container_overlay<_Type>::value;

// overlay_underlying
//   trait: the underlying container an overlay delegates to, or `_Type`
// itself when `_Type` is not an overlay. Lets generic code strip overlays
// back to ground truth.
template<typename _Type,
            bool     _IsOverlay = is_container_overlay_v<_Type>>
struct overlay_underlying
{
    using type = clean_t<_Type>;
};

template<typename _Type>
struct overlay_underlying<_Type, true>
{
    using type = typename clean_t<_Type>::underlying_container_type;
};

// overlay_underlying_t
//   type: convenience alias for overlay_underlying<_Type>::type.
template<typename _Type>
using overlay_underlying_t = typename overlay_underlying<_Type>::type;



#undef D_INTERNAL_OVERLAY_FORWARD_ALIAS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_OVERLAY_