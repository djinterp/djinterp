/*******************************************************************************
* djinterp [core]                constexpr_container_traits_tests_robustness.cpp
*
*
* path:      /tests/djinterp/core/container/traits/constexpr_container_traits_tests_robustness.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "constexpr_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                PROBES                                                    ///
///////////////////////////////////////////////////////////////////////////////
//   The four members the standard's UnaryTypeTrait contract obliges a bool
// trait to carry.  Probed rather than named: naming a member that is not there
// is a hard error, not a false.

// p_value_type
//   probe: does the instantiated trait carry `value_type`?
D_TEST_TYPE_PROBE(p_value_type, typename _Type::value_type)

// p_type
//   probe: does it carry `type`?
D_TEST_TYPE_PROBE(p_type, typename _Type::type)

// p_to_bool
//   probe: is it convertible to bool - does `if (is_foo<T>{})` compile?
D_TEST_EXPR_PROBE(p_to_bool, static_cast<bool>(std::declval<const _Type&>()))

// p_call
//   probe: is it callable - does `is_foo<T>{}()` compile?
D_TEST_EXPR_PROBE(p_call, std::declval<const _Type&>()())


///////////////////////////////////////////////////////////////////////////////
///                BUILD-TIME PINS                                           ///
///////////////////////////////////////////////////////////////////////////////
//   Properties of the PROBES, not of any one answer.  If a signal stops being
// SFINAE-friendly, or starts caring about cv-ref, every other check in this
// suite becomes meaningless - so there is nothing worth reporting and the build
// should simply fail.
//
//   Note which list each uses.  The five SIGNALS run over the full zoo,
// incomplete fixture included, because they genuinely tolerate it.  The
// UMBRELLA runs over the complete-types-only list, because it genuinely does
// not - see tests_incomplete_type.

D_TEST_STATIC(::djinterp::test::holds_for_none<has_constexpr_extent,
                                               D_TEST_HOSTILE_TYPES>::value);

D_TEST_STATIC(
    ::djinterp::test::holds_for_none<has_constexpr_size_expression,
                                     D_TEST_HOSTILE_TYPES>::value);

D_TEST_STATIC(
    ::djinterp::test::holds_for_none<is_constexpr_container,
                                     D_TEST_HOSTILE_TYPES_COMPLETE>::value);

D_TEST_STATIC(
    ::djinterp::test::trait_ignores_cvref<is_constexpr_container,
                                          std::array<int, 3>>::value);


/*
tests_cvref_signals
  Every signal must answer identically for T, const T, volatile T, const
  volatile T, T&, const T&, T&& and const T&&.
  Tests the following:
  - each of the five signals agrees across all eight cv-ref forms, on a subject
    for which it is TRUE and on one for which it is FALSE
  - the report names the first disagreeing cell, so a regression says
    "const _Type&" rather than merely "cv-ref"

  The traits get this right by construction: every detection expression is
  written against `clean_t<_Type>`, not `_Type`.  That is one clean_t per probe,
  and a single omission is invisible to every test that only ever passes a bare
  type.  This is that test.
*/
bool
tests_cvref_signals()
{
    bool ok = true;

    #define D_CCT_CVREF(TRAIT, ...)                                           \
        do                                                                    \
        {                                                                     \
            D_CONSTEXPR cvref_report report =                                 \
                trait_across_cvref<TRAIT, __VA_ARGS__>();                     \
                                                                              \
            if (!report.agrees())                                             \
            {                                                                 \
                std::printf("      !! %s<%s> disagrees at: %s\n",             \
                            #TRAIT,                                           \
                            #__VA_ARGS__,                                     \
                            report.first_disagreement());                     \
            }                                                                 \
                                                                              \
            D_CCT_CHECK(report.agrees());                                     \
        } while (false)

    // positive subjects: all eight cells TRUE
    D_CCT_CVREF(has_constexpr_container_tag,   tag_true);
    D_CCT_CVREF(has_constexpr_extent,          extent_static);
    D_CCT_CVREF(has_constexpr_size_expression, size_constexpr);
    D_CCT_CVREF(has_tuple_size,                std::array<int, 3>);
    D_CCT_CVREF(has_constexpr_iteration,       iter_pair);

    // negative subjects: all eight cells FALSE
    D_CCT_CVREF(has_constexpr_container_tag,   no_signal);
    D_CCT_CVREF(has_constexpr_extent,          no_signal);
    D_CCT_CVREF(has_constexpr_size_expression, no_signal);
    D_CCT_CVREF(has_tuple_size,                std::vector<int>);
    D_CCT_CVREF(has_constexpr_iteration,       no_signal);

    #undef D_CCT_CVREF

    // the polarity, not merely the agreement
    D_CCT_CHECK((trait_across_cvref<has_constexpr_extent,
                                    extent_static>().all()));
    D_CCT_CHECK((trait_across_cvref<has_constexpr_extent,
                                    no_signal>().none()));

    return ok;
}

/*
tests_cvref_verdicts
  The lifetime traits, the umbrella and the snapshot all clean their argument.
  Tests the following:
  - is_constexpr_container and is_not_constexpr_container agree across all eight
    cv-ref forms
  - size_lifetime and container_lifetime do the same - they are not bool traits,
    so they cannot go through trait_across_cvref and are checked cell by cell
  - constexpr_container_class does the same
*/
bool
tests_cvref_verdicts()
{
    bool ok = true;

    D_CCT_CHECK((trait_ignores_cvref<is_constexpr_container,
                                     std::array<int, 3>>::value));
    D_CCT_CHECK((trait_ignores_cvref<is_constexpr_container,
                                     std::vector<int>>::value));
    D_CCT_CHECK((trait_ignores_cvref<is_constexpr_container,
                                     tuple_conformer>::value));
    D_CCT_CHECK((trait_ignores_cvref<is_not_constexpr_container,
                                     std::vector<int>>::value));
    D_CCT_CHECK((trait_ignores_cvref<is_not_constexpr_container,
                                     std::array<int, 3>>::value));

    D_CCT_CHECK((trait_across_cvref<is_constexpr_container,
                                    std::array<int, 3>>().all()));
    D_CCT_CHECK((trait_across_cvref<is_constexpr_container,
                                    std::vector<int>>().none()));

    // the lifetime traits are VALUE traits, so they are checked by hand
    #define D_CCT_LIFE_CVREF(EXPECTED, ...)                                   \
        D_CCT_CHECK(container_lifetime<__VA_ARGS__>::value == EXPECTED);      \
        D_CCT_CHECK(container_lifetime<const __VA_ARGS__>::value ==           \
                    EXPECTED);                                                \
        D_CCT_CHECK(container_lifetime<volatile __VA_ARGS__>::value ==        \
                    EXPECTED);                                                \
        D_CCT_CHECK(container_lifetime<const volatile __VA_ARGS__>::value ==  \
                    EXPECTED);                                                \
        D_CCT_CHECK(container_lifetime<__VA_ARGS__&>::value == EXPECTED);     \
        D_CCT_CHECK(container_lifetime<const __VA_ARGS__&>::value ==          \
                    EXPECTED);                                                \
        D_CCT_CHECK(container_lifetime<__VA_ARGS__&&>::value == EXPECTED);    \
        D_CCT_CHECK(size_lifetime<const __VA_ARGS__&>::value ==               \
                    size_lifetime<__VA_ARGS__>::value);                       \
        D_CCT_CHECK(constexpr_container_class<const __VA_ARGS__&>::life ==    \
                    EXPECTED)

    D_CCT_LIFE_CVREF(lifetime::both,    std::array<int, 3>);
    D_CCT_LIFE_CVREF(lifetime::runtime, std::vector<int>);
    D_CCT_LIFE_CVREF(lifetime::runtime, no_signal);
    D_CCT_LIFE_CVREF(lifetime::none,    life_none_sized);

    #undef D_CCT_LIFE_CVREF

    return ok;
}

/*
tests_hostile_types_rejected
  Every trait must answer FALSE for every adversarial shape: void, const void,
  function types, function pointers and references, bounded and unbounded arrays,
  lvalue and rvalue references, member pointers, nullptr_t, enums, and the
  class-type adversaries - abstract, final, private-membered,
  ambiguously-inherited, convert-to-anything, operator&-overloading.
  Tests the following:
  - all five signals reject all of them
  - both verdicts reject all of them (over the complete-types list; see
    tests_incomplete_type for why)
  - size_lifetime is runtime for all of them
*/
bool
tests_hostile_types_rejected()
{
    bool ok = true;

    D_CCT_CHECK((holds_for_none<has_constexpr_container_tag,
                                D_TEST_HOSTILE_TYPES>::value));
    D_CCT_CHECK((holds_for_none<has_constexpr_extent,
                                D_TEST_HOSTILE_TYPES>::value));
    D_CCT_CHECK((holds_for_none<has_constexpr_size_expression,
                                D_TEST_HOSTILE_TYPES>::value));
    D_CCT_CHECK((holds_for_none<has_tuple_size,
                                D_TEST_HOSTILE_TYPES>::value));
    D_CCT_CHECK((holds_for_none<has_constexpr_iteration,
                                D_TEST_HOSTILE_TYPES>::value));

    D_CCT_CHECK((holds_for_none<is_constexpr_container,
                                D_TEST_HOSTILE_TYPES_COMPLETE>::value));
    D_CCT_CHECK((holds_for_all<is_not_constexpr_container,
                               D_TEST_HOSTILE_TYPES_COMPLETE>::value));

    // spot-checks on the shapes most likely to break a probe
    D_CCT_CHECK(!is_constexpr_container<void>::value);
    D_CCT_CHECK(!is_constexpr_container<int(int)>::value);
    D_CCT_CHECK(!is_constexpr_container<int[3]>::value);
    D_CCT_CHECK(!is_constexpr_container<int&>::value);
    D_CCT_CHECK(!is_constexpr_container<std::nullptr_t>::value);

    D_CCT_LIFE(lifetime::runtime, size_lifetime<void>::value);
    D_CCT_LIFE(lifetime::runtime, size_lifetime<int(int)>::value);
    D_CCT_LIFE(lifetime::runtime, size_lifetime<int[3]>::value);

    // a raw C array has a compile-time extent, and no `extent` MEMBER and no
    // std::tuple_size specialization - so nothing promotes it.  The archetypal
    // fixed-size object is not a constexpr container.  Pinned as it stands
    D_CCT_CHECK(!has_constexpr_extent<int[3]>::value);
    D_CCT_CHECK(!has_tuple_size<int[3]>::value);
    D_CCT_CHECK(!is_constexpr_container<int[3]>::value);

    return ok;
}

/*
tests_hostile_types_survived
  SFINAE-friendliness: the traits must REPORT false on a hostile type, not
  DIAGNOSE it.
  Tests the following:
  - every trait instantiates over every hostile shape without a hard error

  This is the property that cannot be observed at run time: a trait that
  hard-errors takes the translation unit with it, so a failure here is a failed
  BUILD.  count_holds is what makes it reliable - it does not short-circuit, so a
  trait that explodes on the twentieth of twenty-six types is still reached.
*/
bool
tests_hostile_types_survived()
{
    bool ok = true;

    D_CCT_CHECK((count_holds<has_constexpr_container_tag,
                             D_TEST_HOSTILE_TYPES>::value == 0));
    D_CCT_CHECK((count_holds<has_constexpr_extent,
                             D_TEST_HOSTILE_TYPES>::value == 0));
    D_CCT_CHECK((count_holds<has_constexpr_size_expression,
                             D_TEST_HOSTILE_TYPES>::value == 0));
    D_CCT_CHECK((count_holds<has_tuple_size,
                             D_TEST_HOSTILE_TYPES>::value == 0));
    D_CCT_CHECK((count_holds<has_constexpr_iteration,
                             D_TEST_HOSTILE_TYPES>::value == 0));
    D_CCT_CHECK((count_holds<is_constexpr_container,
                             D_TEST_HOSTILE_TYPES_COMPLETE>::value == 0));

    // the value traits and the snapshot are not bool traits, so they are
    // instantiated by hand on the shapes most likely to break a probe
    D_CCT_LIFE(lifetime::runtime, container_lifetime<void>::value);
    D_CCT_LIFE(lifetime::runtime, container_lifetime<int(int)>::value);
    D_CCT_LIFE(lifetime::runtime, container_lifetime<int[3]>::value);
    D_CCT_LIFE(lifetime::runtime, container_lifetime<int (*)(int)>::value);
    D_CCT_LIFE(lifetime::runtime, container_lifetime<std::nullptr_t>::value);

    D_CCT_CHECK(!constexpr_container_class<void>::is_constexpr);
    D_CCT_CHECK(!constexpr_container_class<int(int)>::is_constexpr);
    D_CCT_CHECK(!constexpr_container_class<int[3]>::is_constexpr);
    D_CCT_CHECK(!constexpr_container_class<
                    ::djinterp::test::fixtures::evil>::is_constexpr);
    D_CCT_CHECK(!constexpr_container_class<
                    ::djinterp::test::fixtures::greedy>::is_constexpr);

    return ok;
}

/*
tests_incomplete_type
  THE COMPLETENESS BOUNDARY.  An incomplete type is tolerated by exactly half of
  this header, and the line falls in a place nobody chose.

  Tests the following:
  - all FIVE signals answer false for an incomplete type, without diagnosing
  - size_lifetime does too - it consults only the signals
  - and everything downstream HARD-ERRORS, so it cannot be checked here at all

  WHY.  container_lifetime routes through lifetime_of (meta/lifetime.hpp), which
  routes through is_literal_type, which on C++17+ is spelled

      std::is_scalar<T> ||
      ( std::is_trivially_destructible<T> &&
        std::is_trivially_default_constructible<T> )

  and the standard MANDATES a complete type for both of those.  Not "returns
  false" - mandates.  libstdc++ enforces it with a static_assert, so
  is_constexpr_container<Incomplete> is a build failure, not a verdict.

  The cost is real: a trait that cannot be asked about an incomplete type cannot
  be used in a forward-declared context, which is most of the places a container
  trait is wanted.  And it is invisible from this header - the bug is in
  lifetime.hpp, and constexpr_container_traits.hpp merely inherits it.

  The checks below therefore pin what CAN be pinned: the boundary itself.  The
  five signals and size_lifetime tolerate incompleteness, and that is a real
  property worth protecting - it is what would let the umbrella be fixed (by
  gating lifetime_of on std::is_complete-style detection, or by deferring it)
  without touching the signals.
*/
bool
tests_incomplete_type()
{
    bool ok = true;

    using incomplete = ::djinterp::test::fixtures::incomplete;

    // the five signals tolerate it
    D_CCT_CHECK(!has_constexpr_container_tag<incomplete>::value);
    D_CCT_CHECK(!has_constexpr_extent<incomplete>::value);
    D_CCT_CHECK(!has_constexpr_size_expression<incomplete>::value);
    D_CCT_CHECK(!has_tuple_size<incomplete>::value);
    D_CCT_CHECK(!has_constexpr_iteration<incomplete>::value);

    // and so does size_lifetime, which consults only them
    D_CCT_LIFE(lifetime::runtime, size_lifetime<incomplete>::value);

    // through the cv-ref forms as well
    D_CCT_CHECK(!has_constexpr_extent<const incomplete&>::value);
    D_CCT_CHECK(!has_tuple_size<incomplete&&>::value);
    D_CCT_LIFE(lifetime::runtime, size_lifetime<const incomplete&>::value);

    // container_lifetime<incomplete>, is_constexpr_container<incomplete>,
    // is_not_constexpr_container<incomplete> and
    // constexpr_container_class<incomplete> are all HARD ERRORS and cannot be
    // named here.  Uncomment any of them to reproduce:
    //
    //   D_CCT_CHECK(!is_constexpr_container<incomplete>::value);
    //
    //   error: invalid use of incomplete type 'struct ...::incomplete'
    //   error: static assertion failed: template argument must be a complete
    //          class or an unbounded array
    //   note: in lifetime.hpp:252, is_trivially_destructible<clean_t<_Type>>

    return ok;
}

/*
tests_trait_shape
  Is each trait a well-formed bool trait?
  Tests the following:
  - has_constexpr_extent, has_constexpr_size_expression and has_tuple_size are:
    emitted by D_TYPE_TRAIT_TRUE, they derive from std::true_type / false_type
    and carry value_type, type, a bool conversion and a call operator
  - is_constexpr_container and is_not_constexpr_container are: they derive from
    std::integral_constant.  (Worth saying plainly, because the two verdicts in
    bounded_container_traits.hpp - is_bounded_container / is_unbounded_container -
    do NOT, and this header gets it right.)
  - has_constexpr_container_tag is, PROVIDED the opt-in alias is a bool_constant.
    It inherits the alias, so its shape is whatever the user supplied: an opt-in
    of std::integral_constant<int, 1> makes value_type `int`, and the trait stops
    being a bool trait.  That case is quarantined (see the hazards TU) because it
    is one step from the case that does not compile at all
  - has_constexpr_iteration is NOT a bool trait - it is a hand-written struct
    carrying only `static constexpr bool value`, exactly like the two verdicts in
    bounded_container_traits.hpp.  It lives in constexpr_iterator_traits.hpp, so
    the fix is not here, but this header consumes it and the defect is inherited
  - size_lifetime and container_lifetime are VALUE traits, not bool traits, and
    correctly so - but note that container_lifetime supplies a `::type` and
    size_lifetime does not
*/
bool
tests_trait_shape()
{
    bool ok = true;

    // the three D_TYPE_TRAIT_TRUE signals: well-formed over the whole zoo
    D_CCT_CHECK((trait_is_well_formed<has_constexpr_extent,
                                      D_TEST_HOSTILE_TYPES>::value));
    D_CCT_CHECK((trait_is_well_formed<has_constexpr_size_expression,
                                      D_TEST_HOSTILE_TYPES>::value));
    D_CCT_CHECK((trait_is_well_formed<has_tuple_size,
                                      D_TEST_HOSTILE_TYPES>::value));

    // the tag: a bool trait when the opt-in is a bool_constant
    D_CCT_CHECK(is_bool_trait<has_constexpr_container_tag<tag_true>>::value);
    D_CCT_CHECK(is_bool_trait<has_constexpr_container_tag<tag_false>>::value);
    D_CCT_CHECK(is_bool_trait<has_constexpr_container_tag<no_signal>>::value);
    D_CCT_CHECK((trait_is_well_formed<has_constexpr_container_tag,
                                      D_TEST_HOSTILE_TYPES>::value));

    // the two verdicts: proper bool traits
    D_CCT_CHECK(is_bool_trait<is_constexpr_container<std::array<int, 3>>>::value);
    D_CCT_CHECK(is_bool_trait<is_not_constexpr_container<std::vector<int>>>::value);
    D_CCT_CHECK((trait_is_well_formed<is_constexpr_container,
                                      D_TEST_HOSTILE_TYPES_COMPLETE>::value));
    D_CCT_CHECK((trait_is_well_formed<is_not_constexpr_container,
                                      D_TEST_HOSTILE_TYPES_COMPLETE>::value));

    // has_constexpr_iteration: NOT a bool trait (a `value` and nothing else)
    D_CCT_CHECK(!is_bool_trait<has_constexpr_iteration<iter_pair>>::value);
    D_CCT_CHECK((!::djinterp::is_detected<p_value_type,
                                          has_constexpr_iteration<
                                              iter_pair>>::value));
    D_CCT_CHECK((!::djinterp::is_detected<p_type,
                                          has_constexpr_iteration<
                                              iter_pair>>::value));
    D_CCT_CHECK((!::djinterp::is_detected<p_to_bool,
                                          has_constexpr_iteration<
                                              iter_pair>>::value));
    D_CCT_CHECK((!::djinterp::is_detected<p_call,
                                          has_constexpr_iteration<
                                              iter_pair>>::value));

    // ...whereas the verdicts have all four
    using verdict = is_constexpr_container<std::array<int, 3>>;

    D_CCT_CHECK((::djinterp::is_detected<p_value_type, verdict>::value));
    D_CCT_CHECK((::djinterp::is_detected<p_type, verdict>::value));
    D_CCT_CHECK((::djinterp::is_detected<p_to_bool, verdict>::value));
    D_CCT_CHECK((::djinterp::is_detected<p_call, verdict>::value));

    // the lifetime traits: value traits by design, and asymmetric about ::type
    D_CCT_CHECK(!is_bool_trait<container_lifetime<std::array<int, 3>>>::value);
    D_CCT_CHECK(!is_bool_trait<size_lifetime<std::array<int, 3>>>::value);
    D_CCT_CHECK((::djinterp::is_detected<p_type,
                                         container_lifetime<
                                             std::array<int, 3>>>::value));
    D_CCT_CHECK((!::djinterp::is_detected<p_type,
                                          size_lifetime<
                                              std::array<int, 3>>>::value));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
