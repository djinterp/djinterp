/*******************************************************************************
* djinterp [core]               container_conversion_traits_tests_robustness.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_conversion_traits_tests_robustness.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <type_traits>
#include <utility>
// djinterp
#include "container_conversion_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                PROBES + BINDERS                                          ///
///////////////////////////////////////////////////////////////////////////////
//   Every trait here is BINARY, and the toolkit's quantifiers and cv-ref matrix
// take a ONE-parameter template-template argument.  So each battery binds one
// operand and varies the other - which is the right shape anyway: cv-ref
// agnosticism is a property of each operand SEPARATELY, and a matrix that varied
// both at once would confound them.

// p_nested_type
//   probe: does the instantiated trait carry a nested `type`?  Probed rather than
// named, because naming a member that is not there is a hard error, not a false.
D_TEST_TYPE_PROBE(p_nested_type, typename _Type::type)

// p_bool_conv
//   probe: is the instantiated trait convertible to bool?  conversion_tier_of must
// NOT be - see tests_trait_shape for why that is a feature and not an oversight.
D_TEST_TYPE_PROBE(p_bool_conv,
                  decltype(static_cast<bool>(std::declval<const _Type&>())))

// views_view_to / viewed_from_view_from
//   traits: is_view_conversion with one operand pinned, so the toolkit can run its
// cv-ref matrix over the other.  LEFT-varying and RIGHT-varying, because a trait
// that cleaned one operand and not the other would pass a one-sided check.
template<typename _Type>
struct views_view_to : is_view_conversion<_Type, view_to>
{};

template<typename _Type>
struct viewed_from_view_from : is_view_conversion<view_from, _Type>
{};

// self_view
//   trait: is a type a view of ITSELF?  The reflexive form, as a one-parameter
// trait the quantifiers can take.
template<typename _Type>
struct self_view : is_view_conversion<_Type, _Type>
{};

// no_path_from_view_from
//   trait: has_constructive_path with the left operand pinned - the trait that
// tolerates an incomplete type, so the battery over the FULL zoo can use it.
template<typename _Type>
struct no_path_from_view_from
    : ::djinterp::bool_constant<
          !has_constructive_path<view_from, _Type>::value>
{};


///////////////////////////////////////////////////////////////////////////////
///                BUILD-TIME PINS                                           ///
///////////////////////////////////////////////////////////////////////////////
//   Properties of the TRAITS, not of any one answer.  If a trait stops cleaning
// its operands, or starts diagnosing where it should report, every other check in
// this suite becomes meaningless.
//
//   Note which list each uses.  The PATH traits tolerate an incomplete type; the
// tier and everything built on it does not, because the view predicate reads
// lifetime_relation.  See tests_incomplete_type.

D_TEST_STATIC(::djinterp::test::holds_for_all<no_path_from_view_from,
                                              D_TEST_HOSTILE_TYPES>::value);

D_TEST_STATIC(
    ::djinterp::test::holds_for_all<self_view,
                                    D_TEST_HOSTILE_TYPES_COMPLETE>::value);

D_TEST_STATIC(
    ::djinterp::test::trait_ignores_cvref<views_view_to, view_from>::value);


/*
tests_cvref
  Every trait must answer identically for T, const T, volatile T, const volatile T,
  T&, const T&, T&& and const T&& - on BOTH operands.

  Each trait cleans its arguments with clean_t at the top of its definition.  That
  is one clean_t per operand per trait, a dozen traits, and a single omission is
  invisible to every test that only ever passes bare types.  This is that test.

  Tests the following:
  - the LEFT operand: the matrix over views_view_to, which varies the left
  - the RIGHT operand: the matrix over viewed_from_view_from, which varies the right
  - the tier itself, cell by cell over both operands - it is not a bool trait, so it
    cannot go through the matrix
  - the path traits, the per-concern traits and conversion_class likewise
*/
bool
tests_cvref()
{
    bool ok = true;

    // the matrix, on each operand in turn
    D_CVT_CHECK((trait_ignores_cvref<views_view_to, view_from>::value));
    D_CVT_CHECK((trait_ignores_cvref<views_view_to, lever_layout>::value));
    D_CVT_CHECK((trait_ignores_cvref<viewed_from_view_from,
                                     view_to>::value));
    D_CVT_CHECK((trait_ignores_cvref<viewed_from_view_from,
                                     lever_layout>::value));

    D_CVT_CHECK((trait_across_cvref<views_view_to, view_from>().all()));
    D_CVT_CHECK((trait_across_cvref<views_view_to, lever_layout>().none()));

    // the TIER, over every cv-ref form of the LEFT operand
    #define D_CVT_CVREF_LEFT(EXPECTED, FROM, TO)                              \
        D_CVT_TIER((EXPECTED), FROM, TO);                                     \
        D_CVT_TIER((EXPECTED), const FROM, TO);                               \
        D_CVT_TIER((EXPECTED), volatile FROM, TO);                            \
        D_CVT_TIER((EXPECTED), const volatile FROM, TO);                      \
        D_CVT_TIER((EXPECTED), FROM&, TO);                                    \
        D_CVT_TIER((EXPECTED), const FROM&, TO);                              \
        D_CVT_TIER((EXPECTED), FROM&&, TO);                                   \
        D_CVT_TIER((EXPECTED), const FROM&&, TO)

    D_CVT_CVREF_LEFT(conversion_tier::view,         view_from, view_to);
    D_CVT_CVREF_LEFT(conversion_tier::constructive, view_from, lever_layout);
    D_CVT_CVREF_LEFT(conversion_tier::lossy,        view_from, lossy_forget);
    D_CVT_CVREF_LEFT(conversion_tier::structural,   view_from, lever_topology);
    D_CVT_CVREF_LEFT(conversion_tier::none,         view_from, no_path);

    #undef D_CVT_CVREF_LEFT

    // ...and of the RIGHT operand
    #define D_CVT_CVREF_RIGHT(EXPECTED, FROM, TO)                             \
        D_CVT_TIER((EXPECTED), FROM, TO);                                     \
        D_CVT_TIER((EXPECTED), FROM, const TO);                               \
        D_CVT_TIER((EXPECTED), FROM, volatile TO);                            \
        D_CVT_TIER((EXPECTED), FROM, const volatile TO);                      \
        D_CVT_TIER((EXPECTED), FROM, TO&);                                    \
        D_CVT_TIER((EXPECTED), FROM, const TO&);                              \
        D_CVT_TIER((EXPECTED), FROM, TO&&);                                   \
        D_CVT_TIER((EXPECTED), FROM, const TO&&)

    D_CVT_CVREF_RIGHT(conversion_tier::view,         view_from, view_to);
    D_CVT_CVREF_RIGHT(conversion_tier::constructive, view_from, lever_elements);
    D_CVT_CVREF_RIGHT(conversion_tier::lossy,        view_from, lossy_clamp);

    #undef D_CVT_CVREF_RIGHT

    // the path traits, the per-concern traits and the class all clean too
    D_CVT_CHECK((is_range_constructible<const view_from&,
                                        view_to&&>::value) ==
                (is_range_constructible<view_from, view_to>::value));
    D_CVT_CHECK((has_constructive_path<volatile view_from,
                                       const view_to>::value) ==
                (has_constructive_path<view_from, view_to>::value));
    D_CVT_CHECK((elements_compatible<const view_from&,
                                     const view_to&>::value) ==
                (elements_compatible<view_from, view_to>::value));
    D_CVT_CHECK((conversion_forgets_content<view_from&&,
                                            const lossy_forget&>::value) ==
                (conversion_forgets_content<view_from, lossy_forget>::value));
    D_CVT_CHECK((conversion_clamps_capacity<const view_from,
                                            volatile lossy_clamp>::value) ==
                (conversion_clamps_capacity<view_from, lossy_clamp>::value));
    D_CVT_CHECK((conversion_changes_topology<const view_from&,
                                             lever_topology&&>::value) ==
                (conversion_changes_topology<view_from,
                                             lever_topology>::value));
    D_CVT_CHECK((conversion_class<const view_from&, view_to&&>::tier) ==
                (conversion_class<view_from, view_to>::tier));

    // and the five bool traits
    D_CVT_CHECK((is_view_conversion<const view_from&, view_to&&>::value) ==
                (is_view_conversion<view_from, view_to>::value));
    D_CVT_CHECK((is_lossless_conversion<view_from&&,
                                        const view_to&>::value) ==
                (is_lossless_conversion<view_from, view_to>::value));
    D_CVT_CHECK((is_convertible_between<volatile view_from,
                                        const view_to>::value) ==
                (is_convertible_between<view_from, view_to>::value));

    return ok;
}

/*
tests_hostile_types
  Every trait must survive every adversarial shape - void, function types, arrays,
  references, member pointers, enums, and the class-type adversaries - and must
  REPORT rather than DIAGNOSE.
  Tests the following:
  - the path traits reject every hostile From, because none of them is an iterable
    container.  Over the FULL zoo, incomplete fixture included, since the path traits
    tolerate it
  - the tier is reflexive over the zoo - every type is a view of itself.  Over the
    COMPLETE list, since the view predicate does not tolerate an incomplete type
  - the traits instantiate over every hostile shape without a hard error.
    count_holds does not short-circuit, so a trait that explodes on the twentieth of
    twenty-five types is still reached

  And the finding, restated as a battery: every hostile type is a VIEW of itself -
  which is fine, reflexively - and every PAIR of them is a view of the other, which
  is not.
*/
bool
tests_hostile_types()
{
    bool ok = true;

    // no hostile shape offers a construction path
    D_CVT_CHECK((holds_for_all<no_path_from_view_from,
                               D_TEST_HOSTILE_TYPES>::value));
    D_CVT_CHECK((count_holds<no_path_from_view_from,
                             D_TEST_HOSTILE_TYPES>::value == 26));

    // reflexivity: every type is a view of itself
    D_CVT_CHECK((holds_for_all<self_view,
                               D_TEST_HOSTILE_TYPES_COMPLETE>::value));
    D_CVT_CHECK((count_holds<self_view,
                             D_TEST_HOSTILE_TYPES_COMPLETE>::value == 25));

    // the shapes most likely to break a probe, by hand
    D_CVT_TIER(conversion_tier::view, void, void);
    D_CVT_TIER(conversion_tier::view, fn_type, fn_type);
    D_CVT_TIER(conversion_tier::view, raw_array, raw_array);
    D_CVT_TIER(conversion_tier::view, fixtures::evil, fixtures::evil);
    D_CVT_TIER(conversion_tier::view, fixtures::greedy, fixtures::greedy);

    D_CVT_CHECK((!has_constructive_path<void, std_vector>::value));
    D_CVT_CHECK((!has_constructive_path<fn_type, std_vector>::value));
    D_CVT_CHECK((!elements_compatible<std_vector, fixtures::abstract>::value));

    // *** THE FINDING, as a battery.  Every PAIR of non-containers is a view ***
    D_CVT_TIER(conversion_tier::view, void, fn_type);
    D_CVT_TIER(conversion_tier::view, fixtures::empty, fixtures::final_type);
    D_CVT_TIER(conversion_tier::view, fixtures::plain_enum, void);
    D_CVT_TIER(conversion_tier::view, raw_array, int);

    // ...and a real container against a hostile type is still rejected, at rung 2
    D_CVT_TIER(conversion_tier::none, std_vector, void);
    D_CVT_TIER(conversion_tier::none, void, std_vector);
    D_CVT_TIER(conversion_tier::none, std_vector, fixtures::empty);

    return ok;
}

/*
tests_incomplete_type
  THE COMPLETENESS BOUNDARY.  An incomplete type is tolerated by exactly half of
  this header, and the line falls in a place nobody chose.

  Tests the following:
  - the PATH traits, the four per-concern traits and conversion_changes_topology all
    answer for an incomplete type, without diagnosing
  - conversion_tier_of, the five bool traits and conversion_class HARD-ERROR, so they
    cannot be checked here at all

  WHY.  The view predicate reads lifetime_relation, which routes through
  container_lifetime (constexpr_container_traits.hpp) -> lifetime_of
  (meta/lifetime.hpp) -> is_literal_type, which on C++17+ goes through
  std::is_trivially_destructible, whose mandate is a complete type.  libstdc++
  enforces it with a static_assert.

  It is the SAME defect constexpr_container_traits.hpp and
  container_comparison_traits.hpp both have, inherited by the same route, and it
  costs the same thing here: a conversion trait that cannot be asked about an
  incomplete type cannot be used in a forward-declared context, which is most of the
  places a conversion is wanted.  Fixing lifetime.hpp fixes all three.

  The checks below pin what CAN be pinned: the boundary.  Half of the header
  tolerates incompleteness, and that is a real property worth protecting - it is what
  would let the other half be fixed without touching it.
*/
bool
tests_incomplete_type()
{
    bool ok = true;

    using incomplete = fixtures::incomplete;

    // the PATH traits tolerate it - on both operands
    D_CVT_CHECK((!is_range_constructible<incomplete, std_vector>::value));
    D_CVT_CHECK((!is_range_insertable<incomplete, std_set>::value));
    D_CVT_CHECK((!has_constructive_path<incomplete, std_vector>::value));
    D_CVT_CHECK((!has_constructive_path<std_vector, incomplete>::value));
    D_CVT_CHECK((!has_constructive_path<view_from, incomplete>::value));

    // ...as do the four per-concern traits
    D_CVT_CHECK((elements_compatible<incomplete, int>::value));   // both `void`
    D_CVT_CHECK((!elements_compatible<incomplete, std_vector>::value));
    D_CVT_CHECK((conversion_forgets_content<std_vector, incomplete>::value));
    D_CVT_CHECK((!conversion_clamps_capacity<std_vector, incomplete>::value));
    D_CVT_CHECK((!conversion_changes_topology<incomplete, int>::value));
    D_CVT_CHECK((conversion_changes_topology<incomplete, std_vec_vec>::value));

    // through the cv-ref forms as well
    D_CVT_CHECK((!has_constructive_path<const incomplete&,
                                        std_vector>::value));
    D_CVT_CHECK((elements_compatible<incomplete&&, const int&>::value));

    // conversion_tier_of<incomplete, ...>, is_view_conversion<incomplete, ...>,
    // is_lossless_conversion<...>, is_convertible_between<...> and
    // conversion_class<...> are all HARD ERRORS and cannot be named here.  Uncomment
    // any of them to reproduce:
    //
    //   D_CVT_TIER(conversion_tier::view, incomplete, incomplete);
    //
    //   error: invalid use of incomplete type 'struct ...::incomplete'
    //   error: static assertion failed: template argument must be a complete class
    //          or an unbounded array
    //   note: in lifetime.hpp, is_trivially_destructible<clean_t<_Type>>
    //   note: reached from is_reinterpretation_helper -> lifetime_relation

    return ok;
}

/*
tests_trait_shape
  Is each trait well-shaped?
  Tests the following:
  - EVERY bool trait in this header derives from std::integral_constant, and is
    therefore a well-formed bool trait: the two path traits, has_constructive_path,
    the four per-concern traits, and the five classification predicates.  That is
    twelve, and all twelve are right - which is worth saying, because
    bounded_container_traits.hpp's two verdicts are not, and the toolkit's shape
    check exists because of them
  - the two VALUE traits - conversion_tier_of and conversion_class - are NOT bool
    traits, and correctly so: one carries a conversion_tier and the other five
    heterogeneous members
  - and the trap that makes that matter: conversion_tier::view is ZERO, so
    `if (conversion_tier_of<A, B>::value)` would be false exactly when the conversion
    is FREE.  conversion_tier_of has no conversion to bool, so that reading is a
    compile error rather than a silent inversion - which is the next best thing to
    it being impossible, and is worth checking rather than assuming
  - neither value trait supplies a `::type` carrier, so a caller who wants to
    tag-dispatch on a tier has to build the integral_constant by hand
*/
bool
tests_trait_shape()
{
    bool ok = true;

    // all TWELVE bool traits are well-formed bool traits
    D_CVT_CHECK(is_bool_trait<is_range_constructible<view_from,
                                                     view_to>>::value);
    D_CVT_CHECK(is_bool_trait<is_range_insertable<view_from,
                                                  insert_only>>::value);
    D_CVT_CHECK(is_bool_trait<has_constructive_path<view_from,
                                                    view_to>>::value);
    D_CVT_CHECK(is_bool_trait<elements_compatible<view_from,
                                                  view_to>>::value);
    D_CVT_CHECK(is_bool_trait<conversion_forgets_content<view_from,
                                                         lossy_forget>>::value);
    D_CVT_CHECK(is_bool_trait<conversion_clamps_capacity<view_from,
                                                         lossy_clamp>>::value);
    D_CVT_CHECK(is_bool_trait<conversion_changes_topology<
                                  view_from, lever_topology>>::value);
    D_CVT_CHECK(is_bool_trait<is_convertible_between<view_from,
                                                     view_to>>::value);
    D_CVT_CHECK(is_bool_trait<is_view_conversion<view_from, view_to>>::value);
    D_CVT_CHECK(is_bool_trait<is_lossless_conversion<view_from,
                                                     view_to>>::value);
    D_CVT_CHECK(is_bool_trait<is_lossy_conversion<view_from,
                                                  lossy_forget>>::value);
    D_CVT_CHECK(is_bool_trait<conversion_requires_restructuring<
                                  view_from, lever_topology>>::value);

    // ...and well-formed over the whole zoo
    D_CVT_CHECK((trait_is_well_formed<self_view,
                                      D_TEST_HOSTILE_TYPES_COMPLETE>::value));
    D_CVT_CHECK((trait_is_well_formed<no_path_from_view_from,
                                      D_TEST_HOSTILE_TYPES>::value));

    // the two VALUE traits are NOT bool traits - and must not be
    D_CVT_CHECK(!is_bool_trait<conversion_tier_of<view_from,
                                                  view_to>>::value);
    D_CVT_CHECK(!is_bool_trait<conversion_class<view_from, view_to>>::value);

    // *** THE TRAP.  `view` is enumerator ZERO, so a boolean reading of a tier would
    // be false exactly when the conversion is FREE ***
    D_CVT_CHECK(static_cast<int>(conversion_tier::view) == 0);
    D_CVT_TIER(conversion_tier::view, view_from, view_to);

    // the value trait has no conversion to bool, so that reading is a COMPILE ERROR
    // rather than a silent inversion.  Probed rather than named
    D_CVT_CHECK((!::djinterp::is_detected<
                     p_bool_conv,
                     conversion_tier_of<view_from, view_to>>::value));

    // ...and the twelve BOOL traits do have it, which is what makes them safe to
    // branch on
    D_CVT_CHECK((::djinterp::is_detected<
                     p_bool_conv,
                     is_view_conversion<view_from, view_to>>::value));
    D_CVT_CHECK((::djinterp::is_detected<
                     p_bool_conv,
                     has_constructive_path<view_from, view_to>>::value));

    // neither value trait supplies a ::type carrier
    D_CVT_CHECK((!::djinterp::is_detected<
                     p_nested_type,
                     conversion_tier_of<view_from, view_to>>::value));
    D_CVT_CHECK((!::djinterp::is_detected<
                     p_nested_type,
                     conversion_class<view_from, view_to>>::value));

    // ...whereas the bool traits do, through their integral_constant base
    D_CVT_CHECK((::djinterp::is_detected<
                     p_nested_type,
                     is_view_conversion<view_from, view_to>>::value));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
