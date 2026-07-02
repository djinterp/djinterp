/******************************************************************************
* djinterp [container]                              bounded_container_traits.hpp
*
*   SFINAE structural traits for the BOUNDEDNESS axis.  A container may be
* bounded along two independent sub-axes (the spec, Boundedness):
*
*     CAPACITY.   The capacity kappa in N-bar_{>=0} = {0,1,2,...} U {inf} is the
*                 bound on total size - c is valid only if |c| <= kappa.  The type
*                 is BOUNDED (in size) iff kappa < inf and UNBOUNDED iff kappa = inf.
*     DOMAIN.     A container is domain-bounded by a closed interval I = [x,y,z]
*                 iff every element value lies in its carrier <I>.  Orthogonal to
*                 capacity: a fixed array is size-bounded but domain-free, whereas
*                 a closed interval is bounded on both.
*
*   CAPACITY DETECTION.  The reliable POSITIVE evidence for kappa < inf is a fixed
* compile-time extent; a growable heap container (vector, list, deque, set, ...)
* is unbounded.  The promoting signals are:
*     1. a static `extent` constant            (djinterp fixed-capacity convention)
*     2. a std::tuple_size specialization       (std::array and fixed-size aggregates)
*     3. static interval bounds                 (lower_bound / upper_bound -> finite
*                                                carrier -> finite capacity)
*     4. a capacity() accessor without reserve()  (fixed, non-extensible capacity)
*   max_size() is deliberately NOT a promoting signal: a growable std::vector also
* exposes max_size() (returning SIZE_MAX), so its presence does not separate kappa = N
* from kappa = inf.  It is retained below only as an informational accessor.
*
*   DOMAIN DETECTION.  Static `lower_bound` / `upper_bound` mark a closed-interval
* domain (the spec's I = [x,y,z]); such a type is domain-bounded, and - its carrier
* being finite - capacity-bounded as well.
*
*   The boundedness axis is orthogonal to the other intrinsic axes.
*
*   PORTABILITY:
*   C++11 baseline; `_v` companions degrade with the language as the rest do.
*
*
* path:      /inc/djinterp/core/container/traits/bounded_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
*                                                          revised: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_BOUNDED_CONTAINER_TRAITS_
#define DJINTERP_BOUNDED_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"           // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"  // D_TYPE_TRAIT_* detection macros
#include "./container_traits.hpp"       // D_TYPE_TRAIT_* detection macros


NS_DJINTERP


// ===========================================================================
// I.   Structural signals
// ===========================================================================

// has_fixed_extent_signal
//   trait: detects a static `extent` constant - the djinterp compile-time
// fixed-capacity convention.  A promoting (bounded) signal.
D_TYPE_TRAIT_TRUE(has_fixed_extent_signal,
    decltype(clean_t<_Type>::extent))

// has_tuple_size_signal
//   trait: detects a std::tuple_size specialization - the canonical mark of a
// fixed-size aggregate (std::array, and conforming types).  Its ::value is a
// compile-time count, so it is the robust compile-time-capacity signal; the
// std::tuple_size primary template is incomplete for non-aggregates, so the
// probe is SFINAE-clean (false) for vector, list, and friends.  A promoting
// (bounded) signal.
D_TYPE_TRAIT_TRUE(has_tuple_size_signal,
    decltype(std::tuple_size<clean_t<_Type>>::value))

// has_static_bounds_signal
//   trait: detects static `lower_bound` AND `upper_bound` - a closed-interval
// domain I = [x,y,z].  A finite domain has a finite carrier, so this is both the
// DOMAIN signal and a promoting CAPACITY (bounded) signal.
D_TYPE_TRAIT_TRUE(has_static_bounds_signal,
    decltype(clean_t<_Type>::lower_bound),
    decltype(clean_t<_Type>::upper_bound))

// has_capacity_signal
//   trait: detects a capacity() accessor on a const lvalue.
D_TYPE_TRAIT_TRUE(has_capacity_signal,
    decltype(std::declval<const clean_t<_Type>&>().capacity()))

// has_reserve_signal
//   trait: detects a reserve(size_type) call - the growability ANTI-signal that
// disqualifies a capacity() accessor from meaning a FIXED capacity.
D_TYPE_TRAIT_TRUE(has_reserve_signal,
    decltype(std::declval<clean_t<_Type>&>().reserve(std::declval<std::size_t>())))

// has_max_size_signal
//   trait: detects a max_size() accessor on a const lvalue.  Informational only -
// NOT a promoting signal (see the file header: a growable vector exposes it too).
D_TYPE_TRAIT_TRUE(has_max_size_signal,
    decltype(std::declval<const clean_t<_Type>&>().max_size()))


// ===========================================================================
// II.  Capacity classification
// ===========================================================================

// is_bounded_container
//   trait: true iff the container has compile-time-or-fixed capacity evidence -
// kappa < inf.  (Not gated on size(): an interval's bound is its static endpoints,
// not a size() accessor.)
template<typename _Type>
struct is_bounded_container
{
public:
    static constexpr bool value =
        ( has_fixed_extent_signal<_Type>::value  ||
          has_tuple_size_signal<_Type>::value    ||
          has_static_bounds_signal<_Type>::value ||
          (  has_capacity_signal<_Type>::value &&
            !has_reserve_signal<_Type>::value ) );
};

D_TYPE_TRAIT_VALUE_BOOL(is_bounded_container)

// is_unbounded_container
//   trait: true iff the type looks like a container (has size()) but exposes no
// capacity-bounding evidence - kappa = inf.
template<typename _Type>
struct is_unbounded_container
{
public:
    static constexpr bool value =
        (  has_size_signal<_Type>::value &&
          !is_bounded_container<_Type>::value );
};

D_TYPE_TRAIT_VALUE_BOOL(is_unbounded_container)


// ===========================================================================
// III. Domain classification (orthogonal sub-axis)
// ===========================================================================

// is_domain_bounded_container
//   trait: true iff the container confines its values to a closed-interval
// domain - detected by static interval bounds.
template<typename _Type>
struct is_domain_bounded_container
    : std::integral_constant<bool,
          has_static_bounds_signal<clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_domain_bounded_container)


// ===========================================================================
// IV.  Capacity-bound summary
// ===========================================================================

// capacity_bound
//   enum: the container's position on the capacity sub-axis.  Ordered by
// permissiveness: bounded (kappa < inf) is the more restrictive, unbounded the
// more permissive; unknown is reserved for types that are not containers.
enum class capacity_bound
{
    unknown,    // not a container
    bounded,    // kappa < inf
    unbounded   // kappa = inf
};

// capacity_bound_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
capacity_bound_name(capacity_bound _b) noexcept
{
    return ( _b == capacity_bound::unknown ? "unknown"
           : _b == capacity_bound::bounded ? "bounded"
           :                                 "unbounded" );
}

// capacity_bound_of
//   trait: classifies a type - bounded when there is capacity evidence,
// unbounded when it is a container without it, unknown otherwise.
template<typename _Type>
struct capacity_bound_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr capacity_bound value =
        (  is_bounded_container<clean_type>::value )
              ? capacity_bound::bounded
      : (  has_size_signal<clean_type>::value )
              ? capacity_bound::unbounded
      :         capacity_bound::unknown;

    using type = std::integral_constant<capacity_bound, value>;
};

template<typename _Type>
using capacity_bound_of_t = typename capacity_bound_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr capacity_bound capacity_bound_of_v =
        capacity_bound_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr capacity_bound capacity_bound_of_v =
        capacity_bound_of<_Type>::value;
#endif


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct bounded_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    // signals
    static constexpr bool has_extent =
        has_fixed_extent_signal<clean_type>::value;
    static constexpr bool has_tuple_size =
        has_tuple_size_signal<clean_type>::value;
    static constexpr bool has_static_bounds =
        has_static_bounds_signal<clean_type>::value;
    static constexpr bool has_capacity =
        has_capacity_signal<clean_type>::value;
    static constexpr bool has_reserve =
        has_reserve_signal<clean_type>::value;
    static constexpr bool has_max_size =
        has_max_size_signal<clean_type>::value;

    // verdicts
    static constexpr bool is_bounded =
        is_bounded_container<clean_type>::value;
    static constexpr bool is_unbounded =
        is_unbounded_container<clean_type>::value;
    static constexpr bool is_domain_bounded =
        is_domain_bounded_container<clean_type>::value;

    // summary
    static constexpr capacity_bound capacity =
        capacity_bound_of<clean_type>::value;
    static constexpr const char*   capacity_name =
        capacity_bound_name(capacity);
};


NS_END  // djinterp


#endif  // DJINTERP_BOUNDED_CONTAINER_TRAITS_
