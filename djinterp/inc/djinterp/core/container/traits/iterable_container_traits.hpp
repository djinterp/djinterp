/******************************************************************************
* djinterp [container]                           iterable_container_traits.hpp
*
*   SFINAE structural traits for the ITERABILITY axis - in what mode a
* container's contents may be traversed.  To iterate a container is to visit its
* positions in the order of some enumeration, taking a per-position access at
* each; a container is ITERABLE if it exposes such a traversal (a begin/end
* pair), and NON-ITERABLE if it offers only positional access (data/size/
* operator[]) with no traversal.
*
*   The formal axis is classified along two INDEPENDENT sub-axes:
*
*     STAGE  the stage at which the traversal runs - compile-time (statically
*            evaluable, requiring compile-time-expressible structure) or runtime.
*            Compile-time iterability entails runtime iterability.
*     MODE   the access each visited position grants - CONST (read-only: a getter
*            leaving the container invariant) or NON-CONST (settable: the value
*            may be replaced, the position set preserved).  Non-const iteration
*            changes WHICH values sit at existing positions; it does NOT subsume
*            structural insertion or erasure, which is the Mutability axis.
*
*   This header owns the runtime base (iterable vs non-iterable), the MODE
* sub-axis, and the operational iterator-CATEGORY layer (section IV).  The STAGE
* sub-axis - compile-time iterability - is classified by
* constexpr_iterator_traits.hpp (whose is_constexpr_iterable answers it); the two
* compose into the four traversal kinds {compile-time, runtime} x {const,
* non-const}.  This split keeps the stage detection, which needs the iterator
* layer, out of this file.
*
*   MODE AND THE OTHER AXES:
*   Per the formal model the mode is a CAPABILITY order - non-const iteration
* subsumes const - so the three levels rank none < const-only < non-const.  A
* container's mode is read structurally: a non-const traversal is detected by a
* begin() whose dereference is a settable lvalue.  As with the element-mutation
* signal of mutable_container_traits.hpp, the probe is whole-element-shaped
* (assigning a value_type through *begin()); an overlay that makes the element or
* key const - a set element, a map key - therefore reads as const-only here, its
* finer per-coordinate mutability (a map's value) being a distinction the formal
* model notes but a single structural probe does not separate.
*
*   OPERATIONAL CATEGORY:
*   Beyond the formal axis, section IV classifies a container's iterator by std
* CATEGORY - forward, bidirectional, or random-access - together with contiguity
* (a data() accessor).  This is NOT a formal iterability sub-axis (the model's are
* stage and mode); it is an operational bridge to the standard iterator taxonomy,
* taken through std::iterator_traits so a pointer iterator (as std::array yields)
* resolves to random-access, and gated on iterability so the category is never
* read from a non-iterator.  It exists so operations may dispatch on backing shape
* - the sequential-layout classification and the filter strategy both consume it.
*
*   RELATION TO ITERATOR-LEVEL TRAITS:
*   The category traits here are CONTAINER-level - they read the category of a
* container's begin() iterator.  Traits over an iterator TYPE itself
* (is_forward_iterator<It> and the like) are a separate, iterator-level concern
* and do not live here.
*
*   PORTABILITY:
*   C++11 baseline; the `_v` companions the D_TYPE_TRAIT_* engine emits degrade
* with the language exactly as the rest of the trait family's do.
*
*
* path:      /inc/djinterp/core/container/traits/iterable_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
*                                                          revised: 2026.07.02
******************************************************************************/

#ifndef DJINTERP_ITERABLE_CONTAINER_TRAITS_
#define DJINTERP_ITERABLE_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"            // clean_t, NS_*, D_ENV_* feature macros
#include "../../meta/trait_detect.hpp"  // D_TYPE_TRAIT_* detection macros


NS_DJINTERP


// ===========================================================================
// I.   SFINAE method / alias detection
// ===========================================================================
//   Probes strip cv-ref via clean_t.  begin()/end() are probed on a non-const
// lvalue, so the presence of a traversal is detected whether the entry points
// are const- or non-const-qualified (the const/non-const MODE is separated in
// section III).

// has_begin_method
//   trait: detects a begin() member (const- or non-const-qualified).
D_TYPE_TRAIT_TRUE(has_begin_method,
    decltype(std::declval<clean_t<_Type>&>().begin()))

// has_end_method
//   trait: detects an end() member.
D_TYPE_TRAIT_TRUE(has_end_method,
    decltype(std::declval<clean_t<_Type>&>().end()))

// has_iterator_alias
//   trait: detects a nested `iterator` type alias.
D_TYPE_TRAIT_HAS_TYPE(has_iterator_alias, iterator)

// has_const_iterator_alias
//   trait: detects a nested `const_iterator` type alias.
D_TYPE_TRAIT_HAS_TYPE(has_const_iterator_alias, const_iterator)

// has_value_type_alias
//   trait: detects a nested `value_type` type alias (the container-hood marker).
D_TYPE_TRAIT_HAS_TYPE(has_value_type_alias, value_type)


// ===========================================================================
// II.  Base classification (iterable vs non-iterable)
// ===========================================================================

// is_iterable_container
//   trait: true iff the container exposes a begin()/end() traversal.
template<typename _Type>
struct is_iterable_container
    : std::integral_constant<bool,
          (    has_begin_method<clean_t<_Type>>::value
            && has_end_method<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_iterable_container)

// is_non_iterable_container
//   trait: true iff the type LOOKS like a container (it has value_type) but
// exposes NO begin()/end() traversal - positional access only.  The value_type
// guard keeps arbitrary non-container types from being mis-classified.
template<typename _Type>
struct is_non_iterable_container
    : std::integral_constant<bool,
          (    has_value_type_alias<clean_t<_Type>>::value
            && !is_iterable_container<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_non_iterable_container)


// ===========================================================================
// III. Mode sub-axis (const vs non-const iteration)
// ===========================================================================

// provides_const_iteration
//   trait: true iff a const traversal is available - begin() and end() are both
// callable on a const lvalue (a read-only visit).
D_TYPE_TRAIT_TRUE(provides_const_iteration,
    decltype(std::declval<const clean_t<_Type>&>().begin()),
    decltype(std::declval<const clean_t<_Type>&>().end()))

// provides_mutable_iteration
//   trait: true iff a non-const traversal is available - begin() yields an
// iterator whose dereference is a settable lvalue (a value_type may be written
// through *begin()).  Whole-element-shaped, as noted in the file header.
D_TYPE_TRAIT_TRUE(provides_mutable_iteration,
    decltype(*std::declval<clean_t<_Type>&>().begin() =
             std::declval<typename clean_t<_Type>::value_type>()))

// iteration_mode
//   enum: the access mode a container's traversal grants.  A capability order:
// none (not iterable) < const_only (observe) < non_const (observe and replace).
enum class iteration_mode
{
    none,           // no traversal
    const_only,     // const traversal only
    non_const       // non-const traversal (subsumes const observation)
};

// iteration_mode_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
iteration_mode_name(iteration_mode _m) noexcept
{
    return ( _m == iteration_mode::none       ? "none"
           : _m == iteration_mode::const_only ? "const_only"
           :                                    "non_const" );
}

// is_no_iteration_mode / is_const_only_mode / is_non_const_mode
//   function: value-level predicates over an iteration_mode.
constexpr bool
is_no_iteration_mode(iteration_mode _m) noexcept
{
    return ( _m == iteration_mode::none );
}

constexpr bool
is_const_only_mode(iteration_mode _m) noexcept
{
    return ( _m == iteration_mode::const_only );
}

constexpr bool
is_non_const_mode(iteration_mode _m) noexcept
{
    return ( _m == iteration_mode::non_const );
}

// mode_grants_observation
//   function: true iff the mode permits observing elements (const_only OR
// non_const) - i.e. any traversal at all.
constexpr bool
mode_grants_observation(iteration_mode _m) noexcept
{
    return ( _m != iteration_mode::none );
}

// mode_grants_mutation
//   function: true iff the mode permits replacing element values (non_const).
constexpr bool
mode_grants_mutation(iteration_mode _m) noexcept
{
    return ( _m == iteration_mode::non_const );
}

// iteration_mode_rank
//   function: the capability rank (none=0 < const_only=1 < non_const=2), the
// linear order the formal mode preorder collapses to.
constexpr int
iteration_mode_rank(iteration_mode _m) noexcept
{
    return ( _m == iteration_mode::none       ? 0
           : _m == iteration_mode::const_only ? 1
           :                                    2 );
}

// iteration_mode_subsumes
//   function: true iff mode _a grants at least the access _b does (rank order).
constexpr bool
iteration_mode_subsumes(iteration_mode _a, iteration_mode _b) noexcept
{
    return ( iteration_mode_rank(_a) >= iteration_mode_rank(_b) );
}

// iteration_mode_of
//   trait: the access mode of a container's traversal - none when not iterable,
// non_const when a settable traversal is exposed, else const_only.
template<typename _Type>
struct iteration_mode_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr iteration_mode value =
        ( !is_iterable_container<clean_type>::value )
              ? iteration_mode::none
      : (  provides_mutable_iteration<clean_type>::value )
              ? iteration_mode::non_const
      :         iteration_mode::const_only;

    using type = std::integral_constant<iteration_mode, value>;
};

// iteration_mode_of_t / iteration_mode_of_v
template<typename _Type>
using iteration_mode_of_t = typename iteration_mode_of<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr iteration_mode iteration_mode_of_v =
        iteration_mode_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr iteration_mode iteration_mode_of_v =
        iteration_mode_of<_Type>::value;
#endif

// is_const_iterable_container / is_mutable_iterable_container
//   trait: derived bool predicates for the two iterable modes.
template<typename _Type>
struct is_const_iterable_container
    : std::integral_constant<bool,
          mode_grants_observation(iteration_mode_of<clean_t<_Type>>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_const_iterable_container)

template<typename _Type>
struct is_mutable_iterable_container
    : std::integral_constant<bool,
          mode_grants_mutation(iteration_mode_of<clean_t<_Type>>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_mutable_iterable_container)


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================

template<typename _Type>
struct iterable_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    // detection signals
    static constexpr bool has_begin =
        has_begin_method<clean_type>::value;
    static constexpr bool has_end =
        has_end_method<clean_type>::value;
    static constexpr bool has_iter_alias =
        has_iterator_alias<clean_type>::value;
    static constexpr bool has_const_iter_alias =
        has_const_iterator_alias<clean_type>::value;

    // base classification
    static constexpr bool is_iterable =
        is_iterable_container<clean_type>::value;
    static constexpr bool is_non_iterable =
        is_non_iterable_container<clean_type>::value;

    // mode sub-axis
    static constexpr bool           provides_const =
        provides_const_iteration<clean_type>::value;
    static constexpr bool           provides_mutable =
        provides_mutable_iteration<clean_type>::value;
    static constexpr iteration_mode mode =
        iteration_mode_of<clean_type>::value;
    static constexpr const char*    mode_name =
        iteration_mode_name(mode);
};


NS_END  // djinterp


#endif  // DJINTERP_ITERABLE_CONTAINER_TRAITS_
