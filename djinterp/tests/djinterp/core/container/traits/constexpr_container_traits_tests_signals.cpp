/*******************************************************************************
* djinterp [core]                   constexpr_container_traits_tests_signals.cpp
*
*
* path:      /tests/djinterp/core/container/traits/constexpr_container_traits_tests_signals.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <deque>
#include <list>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
// djinterp
#include "constexpr_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_tag_basic
  has_constexpr_container_tag - the opt-in `is_constexpr_container` member.
  Tests the following:
  - present-and-true_type   -> true
  - present-and-false_type  -> FALSE (the trait inherits the alias, so a
                               false_type opt-in reports false rather than
                               merely "present")
  - present-and-derived-from-true_type -> true (::value is found through the
                               base, so a user bool_constant works)
  - absent -> false
  - the standard containers do not carry it
*/
bool
tests_tag_basic()
{
    bool ok = true;

    D_CCT_CHECK(has_constexpr_container_tag<tag_true>::value);
    D_CCT_CHECK(has_constexpr_container_tag<tag_derived>::value);
    D_CCT_CHECK(has_constexpr_container_tag<tag_true_nonliteral>::value);

    D_CCT_CHECK(!has_constexpr_container_tag<tag_false>::value);
    D_CCT_CHECK(!has_constexpr_container_tag<tag_false_with_extent>::value);
    D_CCT_CHECK(!has_constexpr_container_tag<no_signal>::value);
    D_CCT_CHECK(!has_constexpr_container_tag<extent_static>::value);
    D_CCT_CHECK(!has_constexpr_container_tag<std::array<int, 3>>::value);
    D_CCT_CHECK(!has_constexpr_container_tag<std::vector<int>>::value);

    return ok;
}

/*
tests_tag_access
  What the probe declines, and what it inherits.
  Tests the following:
  - a PRIVATE opt-in alias is not seen: access checking happens during
    substitution, so the failure is a substitution failure and the trait
    reports false rather than diagnosing
  - the trait really does INHERIT the alias, rather than merely detect it -
    that is what makes false_type mean false, and it is checked here directly
    so that a future rewrite to `D_TYPE_TRAIT_TRUE` (which would inherit
    std::true_type instead, and turn every opt-out into an opt-IN) is caught

  The inheritance is also the hazard: whatever the member names becomes a base
  class, and a base-clause is never in the immediate context.  See
  tests_hazard_tag_base.
*/
bool
tests_tag_access()
{
    bool ok = true;

    D_CCT_CHECK(!has_constexpr_container_tag<tag_private>::value);

    // it inherits the alias: true_type for the opt-in, false_type for the
    // opt-out, and NOT std::true_type in both cases
    D_CCT_CHECK((std::is_base_of<std::true_type,
                                 has_constexpr_container_tag<
                                     tag_true>>::value));
    D_CCT_CHECK((std::is_base_of<std::false_type,
                                 has_constexpr_container_tag<
                                     tag_false>>::value));
    D_CCT_CHECK((std::is_base_of<true_ish,
                                 has_constexpr_container_tag<
                                     tag_derived>>::value));

    // and the absent case falls to the primary template's std::false_type
    D_CCT_CHECK((std::is_base_of<std::false_type,
                                 has_constexpr_container_tag<
                                     no_signal>>::value));

    return ok;
}

/*
tests_extent_basic
  has_constexpr_extent - a static compile-time `extent`.
  Tests the following:
  - fires on a static extent, including an extent of ZERO (zero is a
    compile-time capacity, not the absence of one)
  - declines on types with no extent, including std::array - whose size is
    carried by std::tuple_size, not by an `extent` member
*/
bool
tests_extent_basic()
{
    bool ok = true;

    D_CCT_CHECK(has_constexpr_extent<extent_static>::value);
    D_CCT_CHECK(has_constexpr_extent<extent_zero>::value);
    D_CCT_CHECK(has_constexpr_extent<extent_nonliteral>::value);
    D_CCT_CHECK(has_constexpr_extent<tag_false_with_extent>::value);
    D_CCT_CHECK(has_constexpr_extent<life_both_sized>::value);

    D_CCT_CHECK(!has_constexpr_extent<no_signal>::value);
    D_CCT_CHECK(!has_constexpr_extent<size_constexpr>::value);
    D_CCT_CHECK(!has_constexpr_extent<std::array<int, 3>>::value);
    D_CCT_CHECK(!has_constexpr_extent<std::vector<int>>::value);

    return ok;
}

/*
tests_extent_shapes
  What ELSE `decltype(clean_t<_Type>::extent)` accepts.
  Tests the following:
  - a NON-STATIC data member named `extent` fires the signal
  - a STATIC member FUNCTION named `extent` fires the signal
  - an ENUMERATOR named `extent` fires the signal
  - a non-static member function does NOT (ill-formed without an object)
  - a nested TYPE does NOT (decltype takes an expression)
  - a PRIVATE extent does NOT

  The first three are false positives against the trait's stated contract - "a
  compile-time `extent` static member".  This is the same probe, and therefore
  the same three false positives, as bounded_container_traits.hpp's
  has_fixed_extent_signal: two names for one expression, in one namespace, with
  one bug.  Tightening either should tighten both.
*/
bool
tests_extent_shapes()
{
    bool ok = true;

    // fires - intended
    D_CCT_CHECK(has_constexpr_extent<extent_static>::value);

    // fires - NOT intended
    D_CCT_CHECK(has_constexpr_extent<extent_data_member>::value);
    D_CCT_CHECK(has_constexpr_extent<extent_static_function>::value);
    D_CCT_CHECK(has_constexpr_extent<extent_enumerator>::value);

    // declines - correctly
    D_CCT_CHECK(!has_constexpr_extent<extent_member_function>::value);
    D_CCT_CHECK(!has_constexpr_extent<extent_nested_type>::value);
    D_CCT_CHECK(!has_constexpr_extent<extent_private>::value);

    return ok;
}

/*
tests_size_expression_basic
  has_constexpr_size_expression - the structural probe that `T{}.size()` is a
  constant expression.
  Tests the following:
  - fires on a default-constructible literal type with a constexpr size()
  - declines when size() is const but not constexpr
  - declines when there is no size() at all
  - fires on std::array<int, N>, whose size() is constexpr and whose default
    construction is a constant expression
*/
bool
tests_size_expression_basic()
{
    bool ok = true;

    D_CCT_CHECK(has_constexpr_size_expression<size_constexpr>::value);
    D_CCT_CHECK(has_constexpr_size_expression<std::array<int, 3>>::value);
    D_CCT_CHECK(has_constexpr_size_expression<std::array<int, 0>>::value);

    D_CCT_CHECK(!has_constexpr_size_expression<size_runtime>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<no_signal>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<extent_static>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<tuple_conformer>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<std::list<int>>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<std::deque<int>>::value);

    return ok;
}

/*
tests_size_expression_constness
  The probe calls size() on the PRVALUE `T{}`, not on a const lvalue.
  Tests the following:
  - a NON-const constexpr size() is therefore detected

  This is a real difference from every other size probe in the framework
  (bounded_container_traits.hpp binds `declval<const T&>()` and so rejects a
  non-const size()), and it is invisible until someone writes one.  Pinned so
  that a future "harmonisation" of the two probes is a deliberate change rather
  than an accidental one.
*/
bool
tests_size_expression_constness()
{
    bool ok = true;

    D_CCT_CHECK(has_constexpr_size_expression<size_constexpr>::value);
    D_CCT_CHECK(has_constexpr_size_expression<size_constexpr_nonconst>::value);

    return ok;
}

/*
tests_size_expression_return_value
  The probe's result must survive conversion to std::size_t as a NON-TYPE
  TEMPLATE ARGUMENT, which is stricter than an ordinary conversion.
  Tests the following:
  - size() returning int fires (4 converts cleanly)
  - size() returning a NEGATIVE int does NOT: converting -1 to size_t in a
    converted constant expression is a narrowing, and narrowing is ill-formed
    there.  A container claiming a negative size is rejected - on a
    technicality, but rejected
  - size() returning a pointer does NOT (not convertible to size_t at all)
*/
bool
tests_size_expression_return_value()
{
    bool ok = true;

    D_CCT_CHECK(has_constexpr_size_expression<size_returns_int>::value);

    D_CCT_CHECK(!has_constexpr_size_expression<size_returns_negative>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<size_returns_pointer>::value);

    return ok;
}

/*
tests_size_expression_constructibility
  THE THIRD FINDING.  `T{}` is LIST-initialization, not default-construction -
  and for an aggregate it never consults a constructor at all.

  A user-provided constructor, a virtual member, or an abstract base all make
  `T{}` ill-formed, and the probe correctly declines.  A DELETED DEFAULT
  CONSTRUCTOR does not:

      C++11/14/17   a class with `T() = delete;` is still an AGGREGATE - the
                    rule of the day excluded user-PROVIDED constructors, and
                    `= delete` is user-DECLARED but not user-provided.  So `T{}`
                    is aggregate initialization, the deleted constructor is never
                    called, and the probe FIRES.
      C++20         P1008R1 ("Prohibit aggregates with user-declared
                    constructors") closes it.  The type is no longer an
                    aggregate, `T{}` calls the deleted constructor, and the probe
                    DECLINES.

  So has_constexpr_size_expression gives a DIFFERENT ANSWER on C++17 and C++20
  for the same type, and the header advertises a C++11 baseline.  The expectation
  below is gated on the standard, which is the honest way to state it - but the
  gate IS the bug report.

  is_constexpr_container survives either way, because lifetime_of routes through
  is_trivially_default_constructible, which is false for a deleted constructor on
  every standard, so the meet clears the compile-time bit.  That is now the THIRD
  time the meet has rescued a verdict from a wrong intermediate (see also
  tests_stdlib_vector_string and tests_hazard_destructor).  Three symptoms, one
  cause: `T{}` inside a probe does not mean "default-construct a T".

  Tests the following:
  - a user-provided constructor taking an argument declines, on every standard
  - a virtual member declines (non-literal)
  - an abstract class declines
  - a DELETED default constructor declines only on C++20 - and the probe
    disagrees with std::is_default_constructible on C++17, which is stated here
    outright
  - the probe never DIAGNOSES any of them; it reports false
*/
bool
tests_size_expression_constructibility()
{
    bool ok = true;

    // genuinely not default-constructible, on any standard
    D_CCT_CHECK(!has_constexpr_size_expression<size_explicit_ctor>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<size_virtual>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<
                    ::djinterp::test::fixtures::abstract>::value);

    // the deleted default constructor.  Not default-constructible - the
    // standard trait says so - on EVERY standard
    D_CCT_CHECK(!std::is_default_constructible<size_no_default_ctor>::value);

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // C++20: no longer an aggregate (P1008), so T{} calls the deleted ctor
    D_CCT_CHECK(!std::is_aggregate<size_no_default_ctor>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<size_no_default_ctor>::value);

#else

    // C++17 and earlier: STILL an aggregate, so T{} is aggregate init and the
    // deleted constructor is simply never reached.  The probe fires for a type
    // that cannot be default-constructed
    D_CCT_CHECK(std::is_aggregate<size_no_default_ctor>::value);
    D_CCT_CHECK(has_constexpr_size_expression<size_no_default_ctor>::value);

    // stated as the contradiction it is
    D_CCT_CHECK(has_constexpr_size_expression<size_no_default_ctor>::value !=
                std::is_default_constructible<size_no_default_ctor>::value);

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    // the meet rescues the verdict on both standards - which is why nobody has
    // noticed
    D_CCT_CHECK(!std::is_trivially_default_constructible<
                    size_no_default_ctor>::value);
    D_CCT_LIFE(lifetime::runtime, lifetime_of<size_no_default_ctor>::value);
    D_CCT_CHECK(!is_constexpr_container<size_no_default_ctor>::value);

    return ok;
}

/*
tests_size_expression_destructor
  The temporary `T{}` is DESTROYED inside the constant expression, so the
  destructor is part of the probe.
  Tests the following:
  - a user-provided, non-constexpr destructor declines the probe
  - marking that same destructor constexpr makes the probe fire

  The pair is what proves the destructor is really being constant-evaluated -
  which is the mechanism behind the hard error in tests_hazard_destructor.  A
  failure inside a DESTRUCTOR BODY is not a substitution failure, so a type
  whose destructor is constexpr-declared but not constexpr-evaluable takes the
  build with it.
*/
bool
tests_size_expression_destructor()
{
    bool ok = true;

    // a user-provided, non-constexpr destructor: the temporary cannot be
    // destroyed inside the constant expression, so the probe declines.  True on
    // every standard - before C++20, a user-provided destructor makes the type
    // non-literal outright
    D_CCT_CHECK(!has_constexpr_size_expression<size_user_destructor>::value);

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // ...and mark that same destructor constexpr, and the probe fires.  The
    // pair is what proves the destructor is really being constant-evaluated,
    // which is the mechanism behind the hard error in tests_hazard_destructor
    D_CCT_CHECK(has_constexpr_size_expression<
                    size_constexpr_destructor>::value);

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return ok;
}

/*
tests_tuple_size
  has_tuple_size - the fixed-size-aggregate protocol, and the one signal that
  works INDEPENDENTLY of whether the element type is literal.
  Tests the following:
  - fires on std::array, std::tuple, std::pair, and a conforming user type
  - fires on the ZERO-length array and the EMPTY tuple
  - fires on a conformer whose contents are NOT literal - which is the whole
    point of having it: it is the only signal that can see the size of an
    array<std::string, N>
  - declines on the growable containers, whose std::tuple_size is the
    undefined primary template
*/
bool
tests_tuple_size()
{
    bool ok = true;

    D_CCT_CHECK(has_tuple_size<std::array<int, 3>>::value);
    D_CCT_CHECK(has_tuple_size<std::array<int, 0>>::value);
    D_CCT_CHECK(has_tuple_size<std::tuple<int, char>>::value);
    D_CCT_CHECK(has_tuple_size<std::tuple<>>::value);
    D_CCT_CHECK(has_tuple_size<std::pair<int, char>>::value);
    D_CCT_CHECK(has_tuple_size<tuple_conformer>::value);

    // the signal that reaches where the size EXPRESSION cannot
    D_CCT_CHECK(has_tuple_size<tuple_conformer_nonliteral>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<
                    tuple_conformer_nonliteral>::value);

    D_CCT_CHECK(!has_tuple_size<std::vector<int>>::value);
    D_CCT_CHECK(!has_tuple_size<std::list<int>>::value);
    D_CCT_CHECK(!has_tuple_size<no_signal>::value);
    D_CCT_CHECK(!has_tuple_size<int>::value);
    D_CCT_CHECK(!has_tuple_size<int[3]>::value);

    return ok;
}

/*
tests_constexpr_iteration
  has_constexpr_iteration (constexpr_iterator_traits.hpp) - the fifth disjunct.
  Tests the following:
  - a nested `constexpr_iterator` alias fires it
  - a constexpr_begin()/constexpr_end() PAIR fires it
  - either half ALONE does not: begin-without-end is not an iteration interface,
    even though has_constexpr_begin_method reports it
  - a plain container does not fire it
*/
bool
tests_constexpr_iteration()
{
    bool ok = true;

    D_CCT_CHECK(has_constexpr_iteration<iter_alias>::value);
    D_CCT_CHECK(has_constexpr_iteration<iter_pair>::value);

    // half a pair is not a pair
    D_CCT_CHECK(!has_constexpr_iteration<iter_begin_only>::value);
    D_CCT_CHECK(!has_constexpr_iteration<iter_end_only>::value);

    // ...even though the half IS detected on its own
    D_CCT_CHECK(has_constexpr_begin_method<iter_begin_only>::value);
    D_CCT_CHECK(has_constexpr_end_method<iter_end_only>::value);

    D_CCT_CHECK(!has_constexpr_iteration<no_signal>::value);
    D_CCT_CHECK(!has_constexpr_iteration<std::array<int, 3>>::value);
    D_CCT_CHECK(!has_constexpr_iteration<std::vector<int>>::value);

    return ok;
}

/*
tests_signal_value_companions
  Each signal carries a `_v` companion.
  Tests the following:
  - has_constexpr_container_tag_v, has_constexpr_extent_v,
    has_constexpr_size_expression_v and has_tuple_size_v each agree with their
    trait's ::value, on a positive and a negative subject
  - has_constexpr_iteration_v likewise (emitted by constexpr_iterator_traits.hpp)
*/
bool
tests_signal_value_companions()
{
    bool ok = true;

    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(has_constexpr_container_tag, tag_true));
    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(has_constexpr_container_tag, tag_false));
    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(has_constexpr_container_tag, no_signal));

    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(has_constexpr_extent, extent_static));
    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(has_constexpr_extent, no_signal));

    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(has_constexpr_size_expression,
                                      size_constexpr));
    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(has_constexpr_size_expression,
                                      no_signal));

    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(has_tuple_size, std::array<int, 3>));
    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(has_tuple_size, no_signal));

    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(has_constexpr_iteration, iter_pair));
    D_CCT_CHECK(D_TEST_TRAIT_V_AGREES(has_constexpr_iteration, no_signal));

    // the shorthands really are the values
    D_CCT_CHECK(has_constexpr_container_tag_v<tag_true>);
    D_CCT_CHECK(has_constexpr_extent_v<extent_static>);
    D_CCT_CHECK(has_constexpr_size_expression_v<size_constexpr>);
    D_CCT_CHECK(has_tuple_size_v<std::array<int, 3>>);
    D_CCT_CHECK(has_constexpr_iteration_v<iter_alias>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
