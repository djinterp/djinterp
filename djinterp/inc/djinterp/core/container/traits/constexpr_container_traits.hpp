/******************************************************************************
* djinterp [container]                           constexpr_container_traits.hpp
*
*   Structural traits for the COMPILE-TIME end of a container's Lifetime axis.
* A container has a STATIC lifetime - is usable in constant evaluation - when
* both its size and its component values are fixed by the program text and
* types alone; it is DYNAMIC otherwise.  This module decides that question and
* expresses the verdict in the shared vocabulary of meta/lifetime.hpp.
*
*   THE COMPOSITION (the formal axis, in code):
*   The Lifetime axis defines a container's stage as the latest among its
* defining data,
*       stage(c) = max( stage|c|, max_p stage(val_c(p)) )      (c < r),
* i.e. static iff the SIZE and ALL component values are determined at compile
* time.  Under the one-bit-per-stage lattice of meta/lifetime.hpp that maximum
* is exactly the lattice MEET (bitwise AND): the compile-time bit survives only
* when both operands carry it.  Hence
*       container_lifetime<T> = lifetime_meet( size_lifetime<T>,
*                                              lifetime_of<T> )
*   where size_lifetime<T> answers "is the size compile-time-expressible?" from
* container-shaped signals, and lifetime_of<T> (from meta/lifetime.hpp) answers
* "are the contents compile-time-constructible?" from the type's literal-ness
* or its opt-in lifetime_category.  is_constexpr_container is then simply the
* compile-time bit of that meet.
*
*   DETECTION of a compile-time-expressible SIZE (any one suffices):
*     1. opt-in `is_constexpr_container` member alias equal to std::true_type
*        - an explicit claim of static-lifetime capability (forces `both`);
*     2. a static `extent` member (our array<>::extent, std::span<>::extent);
*     3. a structural probe that `T{}.size()` is a constant expression;
*     4. has_constexpr_iteration (constexpr_iterator_traits.hpp).
*
*   ORTHOGONALITY:
*   The compile-time axis is independent of mutability (relaxed-constexpr in
* C++14+ permits constexpr mutators), of iterability (a constexpr container may
* or may not expose begin()/end()), and of storage (a static-lifetime value may
* still be held in dynamic storage; only the converse entailment holds).
*
*   PORTABILITY:
*   C++11 baseline.  Every `_v` companion is emitted through the canonical
* trait_detect macros, which degrade with the language (inline variable on
* C++17+, variable template on C++14, absent on C++11 - the `::value` member is
* always present).
*
*
* path:      /inc/djinterp/core/container/traits/constexpr_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_CONSTEXPR_CONTAINER_TRAITS_
#define DJINTERP_CONSTEXPR_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"                        // clean_t, NS_*, D_ENV_*
#include "../../meta/trait_detect.hpp"               // D_TYPE_TRAIT_* macros
#include "../../meta/lifetime.hpp"                   // lifetime, lifetime_meet, lifetime_of
#include "../iterator/constexpr_iterator_traits.hpp" // has_constexpr_iteration


NS_DJINTERP


// ===========================================================================
// I.   Compile-time-expressible-SIZE signals
// ===========================================================================
//   Each answers, structurally, "is the container's size fixed by its type?"
// All probes strip cv-ref via clean_t so the answer agrees for T, const T, T&.

// has_constexpr_container_tag
//   trait: detects an opt-in `is_constexpr_container` member alias - an
// explicit assertion of static-lifetime capability - and takes its boolean
// value (the member is itself a bool_constant, so the success case inherits
// it directly: present-and-true_type yields true, present-and-false_type
// yields false, absent yields false).
D_TYPE_TRAIT_TRUE_AS(has_constexpr_container_tag,
    typename clean_t<_Type>::is_constexpr_container,
    clean_t<_Type>::is_constexpr_container)
D_TYPE_TRAIT_VALUE_BOOL(has_constexpr_container_tag)

// has_constexpr_extent
//   trait: detects a compile-time `extent` static member (mirrors our
// array<>::extent and std::span<>::extent conventions).
D_TYPE_TRAIT_TRUE(has_constexpr_extent,
    decltype(clean_t<_Type>::extent))

// has_constexpr_size_expression
//   trait: structural test that `T{}.size()` yields a value usable in a
// constant expression.  Requires _Type to be BOTH default-constructible AND a
// type whose size() is constexpr; reports false otherwise - never errors.  The
// detection type is a single std::integral_constant whose value argument is the
// probed size(); the comma is interior to its template-id and the variadic
// engine re-emits it verbatim into the void_t sink.
D_TYPE_TRAIT_TRUE(has_constexpr_size_expression,
    std::integral_constant<std::size_t, clean_t<_Type>{}.size()>)

// has_tuple_size
//   trait: detects a std::tuple_size specialization - the canonical mark of a
// FIXED-SIZE AGGREGATE (std::array, std::tuple, std::pair, and conforming
// djinterp types).  Its ::value is a compile-time count INDEPENDENT of whether
// the element type is literal, so it is the robust compile-time-SIZE signal
// where has_constexpr_size_expression cannot reach: an array<std::string, N>
// has a compile-time size of N even though string is not a literal type and
// array<string, N>{}.size() is therefore not a constant expression.  The
// std::tuple_size primary template is incomplete for non-aggregates, so the
// probe is SFINAE-clean (false) for vector, list, and friends.
D_TYPE_TRAIT_TRUE(has_tuple_size,
    decltype(std::tuple_size<clean_t<_Type>>::value))


// ===========================================================================
// II.  Lifetime of a container
// ===========================================================================

// size_lifetime
//   trait: the Lifetime contribution of the container's SIZE.  Compile-time-
// expressible (an extent, a std::tuple_size, a constexpr size(), or the opt-in
// tag) yields lifetime::both - a compile-time size is a fortiori a runtime
// size; otherwise the size is an instance property, lifetime::runtime.  The
// tuple_size disjunct is what lets a fixed-size aggregate of NON-literal
// elements (array<std::string, N>) be recognised as compile-time-sized, which
// in turn is what makes the Storage entailment site = stat => stage(|c|) = c
// hold against this trait: the same fixed-extent evidence drives both axes.
template<typename _Type>
struct size_lifetime
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr lifetime value =
        ( ( has_constexpr_container_tag<clean_type>::value   ||
            has_constexpr_extent<clean_type>::value          ||
            has_tuple_size<clean_type>::value                ||
            has_constexpr_size_expression<clean_type>::value ||
            has_constexpr_iteration<clean_type>::value )
              ? lifetime::both
              : lifetime::runtime );
};

// container_lifetime
//   trait: the container's position on the Lifetime axis - the lattice MEET of
// its size lifetime and its contents lifetime (lifetime_of), reproducing the
// formal stage(c) = max(stage|c|, stage(contents)).  An explicit opt-in tag is
// a coarse assertion of static-lifetime capability and resolves to
// lifetime::both (static and, a fortiori, dynamic).  Exposes the value plus a
// lifetime_constant carrier as `::type`.
template<typename _Type>
struct container_lifetime
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr lifetime value =
        ( has_constexpr_container_tag<clean_type>::value
              ? lifetime::both
              : lifetime_meet( size_lifetime<clean_type>::value,
                               lifetime_of<clean_type>::value ) );

    using type = lifetime_constant<value>;
};

// container_lifetime_t / container_lifetime_v
//   type / value: the carrier and (where the language permits) the value
// companion of container_lifetime.
template<typename _Type>
using container_lifetime_t = typename container_lifetime<_Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr lifetime container_lifetime_v =
        container_lifetime<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr lifetime container_lifetime_v =
        container_lifetime<_Type>::value;
#endif


// ===========================================================================
// III. Classification umbrella
// ===========================================================================

// is_constexpr_container
//   trait: true iff the container is usable in constant evaluation - the
// compile-time bit of its container_lifetime, i.e. a STATIC lifetime: both
// size and contents are fixed at compile time.
template<typename _Type>
struct is_constexpr_container
    : std::integral_constant<bool,
          is_compile_time(container_lifetime<clean_t<_Type>>::value)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_constexpr_container)

// is_not_constexpr_container
//   trait: explicit negation - a DYNAMIC lifetime; useful in disjoint
// requires-clauses and SFINAE branches.
template<typename _Type>
struct is_not_constexpr_container
    : std::integral_constant<bool,
          !is_constexpr_container<_Type>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_not_constexpr_container)


// ===========================================================================
// IV.  Aggregate snapshot
// ===========================================================================

// constexpr_container_class
//   struct: a one-stop summary of the compile-time signals and the resulting
// Lifetime verdict, for diagnostics and agent-facing reports.
template<typename _Type>
struct constexpr_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool has_tag =
        has_constexpr_container_tag<clean_type>::value;
    static constexpr bool has_extent =
        has_constexpr_extent<clean_type>::value;
    static constexpr bool has_size_expr =
        has_constexpr_size_expression<clean_type>::value;
    static constexpr bool has_iteration =
        has_constexpr_iteration<clean_type>::value;
    static constexpr lifetime life =
        container_lifetime<clean_type>::value;
    static constexpr bool is_constexpr =
        is_constexpr_container<clean_type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_CONSTEXPR_CONTAINER_TRAITS_
