/*******************************************************************************
* djinterp [core]                  bounded_container_traits_tests_robustness.cpp
*
*
* path:      /tests/djinterp/core/container/traits/bounded_container_traits_tests_robustness.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "bounded_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                PROBES                                                    ///
///////////////////////////////////////////////////////////////////////////////
//   The four members the standard's UnaryTypeTrait contract obliges a trait to
// carry.  Each is probed rather than named, because the question is whether the
// member is THERE - and naming a member that is not there is a hard error, not
// a false.

// p_trait_value_type
//   probe: does the instantiated trait carry `value_type`?
D_TEST_TYPE_PROBE(p_trait_value_type, typename _Type::value_type)

// p_trait_type
//   probe: does it carry `type`?
D_TEST_TYPE_PROBE(p_trait_type, typename _Type::type)

// p_trait_to_bool
//   probe: is it convertible to bool - i.e. does `if (is_foo<T>{})` compile?
D_TEST_EXPR_PROBE(p_trait_to_bool,
                  static_cast<bool>(std::declval<const _Type&>()))

// p_trait_call
//   probe: is it callable - i.e. does `is_foo<T>{}()` compile?
D_TEST_EXPR_PROBE(p_trait_call, std::declval<const _Type&>()())


///////////////////////////////////////////////////////////////////////////////
///                BUILD-TIME PINS                                           ///
///////////////////////////////////////////////////////////////////////////////
//   The invariants whose regression should stop the line rather than produce a
// red line.  All three are properties of the PROBES, not of any one answer: if
// a signal trait ever stops being SFINAE-friendly, or ever starts caring about
// cv-ref, every other check in this suite becomes meaningless - so there is
// nothing worth reporting and the build should simply fail.

D_TEST_STATIC(::djinterp::test::holds_for_none<has_size_signal,
                                               D_TEST_HOSTILE_TYPES>::value);

D_TEST_STATIC(::djinterp::test::holds_for_none<is_bounded_container,
                                               D_TEST_HOSTILE_TYPES>::value);

D_TEST_STATIC(::djinterp::test::trait_ignores_cvref<is_bounded_container,
                                                    std::vector<int>>::value);


/*
tests_cvref_signals
  Every signal trait must answer identically for T, const T, volatile T, const
  volatile T, T&, const T&, T&& and const T&&.
  Tests the following:
  - each of the seven signals agrees across all eight cv-ref forms, on a subject
    for which it is TRUE and on one for which it is FALSE
  - the report names the first disagreeing cell, so a regression says "const
    _Type&" rather than merely "cv-ref"

  The traits get this right by construction - every detection expression is
  written against `clean_t<_Type>` rather than `_Type`.  That is one `clean_t`
  per probe, seven probes, and a single omission is invisible to every test that
  only ever passes a bare type.  This is that test.
*/
bool
tests_cvref_signals()
{
    bool ok = true;

    #define D_BCT_CVREF(TRAIT, ...)                                           \
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
            D_BCT_CHECK(report.agrees());                                     \
        } while (false)

    // positive subjects: all eight cells must be TRUE
    D_BCT_CVREF(has_fixed_extent_signal,  extent_static_member);
    D_BCT_CVREF(has_tuple_size_signal,    std::array<int, 3>);
    D_BCT_CVREF(has_static_bounds_signal, bounds_both);
    D_BCT_CVREF(has_capacity_signal,      capacity_const);
    D_BCT_CVREF(has_reserve_signal,       reserve_sized);
    D_BCT_CVREF(has_max_size_signal,      max_size_only);
    D_BCT_CVREF(has_size_signal,          size_const);

    // negative subjects: all eight cells must be FALSE
    D_BCT_CVREF(has_fixed_extent_signal,  no_signal);
    D_BCT_CVREF(has_tuple_size_signal,    std::vector<int>);
    D_BCT_CVREF(has_static_bounds_signal, no_signal);
    D_BCT_CVREF(has_capacity_signal,      no_signal);
    D_BCT_CVREF(has_reserve_signal,       no_signal);
    D_BCT_CVREF(has_max_size_signal,      no_signal);
    D_BCT_CVREF(has_size_signal,          no_signal);

    #undef D_BCT_CVREF

    // and the polarity, not merely the agreement
    D_BCT_CHECK((trait_across_cvref<has_size_signal,
                                    size_const>().all()));
    D_BCT_CHECK((trait_across_cvref<has_size_signal,
                                    no_signal>().none()));

    return ok;
}

/*
tests_cvref_verdicts
  The three verdicts, the summary and the snapshot all clean their argument too.
  Tests the following:
  - is_bounded_container, is_unbounded_container and is_domain_bounded_container
    agree across all eight cv-ref forms
  - capacity_bound_of does the same - it is not a bool trait, so it cannot go
    through trait_across_cvref and is checked cell by cell
  - bounded_container_class does the same
*/
bool
tests_cvref_verdicts()
{
    bool ok = true;

    D_BCT_CHECK((trait_ignores_cvref<is_bounded_container,
                                     std::array<int, 3>>::value));
    D_BCT_CHECK((trait_ignores_cvref<is_bounded_container,
                                     std::vector<int>>::value));
    D_BCT_CHECK((trait_ignores_cvref<is_unbounded_container,
                                     std::vector<int>>::value));
    D_BCT_CHECK((trait_ignores_cvref<is_unbounded_container,
                                     std::array<int, 3>>::value));
    D_BCT_CHECK((trait_ignores_cvref<is_domain_bounded_container,
                                     interval_container>::value));
    D_BCT_CHECK((trait_ignores_cvref<is_domain_bounded_container,
                                     std::vector<int>>::value));

    D_BCT_CHECK((trait_across_cvref<is_bounded_container,
                                    std::array<int, 3>>().all()));
    D_BCT_CHECK((trait_across_cvref<is_unbounded_container,
                                    std::vector<int>>().all()));
    D_BCT_CHECK((trait_across_cvref<is_bounded_container,
                                    no_signal>().none()));

    // capacity_bound_of is a VALUE trait, so it is checked by hand
    #define D_BCT_CB_CVREF(EXPECTED, ...)                                     \
        D_BCT_CHECK(capacity_bound_of<__VA_ARGS__>::value == EXPECTED);              \
        D_BCT_CHECK(capacity_bound_of<const __VA_ARGS__>::value == EXPECTED);        \
        D_BCT_CHECK(capacity_bound_of<volatile __VA_ARGS__>::value == EXPECTED);     \
        D_BCT_CHECK(capacity_bound_of<const volatile __VA_ARGS__>::value ==          \
                    EXPECTED);                                                \
        D_BCT_CHECK(capacity_bound_of<__VA_ARGS__&>::value == EXPECTED);             \
        D_BCT_CHECK(capacity_bound_of<const __VA_ARGS__&>::value == EXPECTED);       \
        D_BCT_CHECK(capacity_bound_of<__VA_ARGS__&&>::value == EXPECTED);            \
        D_BCT_CHECK(capacity_bound_of<const __VA_ARGS__&&>::value == EXPECTED);      \
        D_BCT_CHECK(bounded_container_class<const __VA_ARGS__&>::capacity ==         \
                    EXPECTED)

    D_BCT_CB_CVREF(capacity_bound::bounded,   std::array<int, 3>);
    D_BCT_CB_CVREF(capacity_bound::unbounded, std::vector<int>);
    D_BCT_CB_CVREF(capacity_bound::unknown,   no_signal);

    #undef D_BCT_CB_CVREF

    return ok;
}

/*
tests_hostile_types_rejected
  Every trait must answer FALSE for every type in the toolkit's adversarial zoo:
  void, const void, function types, function pointers and references, bounded and
  unbounded arrays, lvalue and rvalue references, member pointers, nullptr_t,
  enums, and the class-type adversaries - an abstract class, a final class, a
  class whose members are private, one whose members are ambiguously inherited,
  one that converts to anything, one that overloads operator& and operator,, and
  an incomplete type.
  Tests the following:
  - all ten traits reject all twenty-six of them

  A note on the raw array: `int[3]` has a compile-time extent, and is rejected -
  it has no MEMBER named `extent` and no std::tuple_size specialization, so
  nothing promotes it.  The archetypal fixed-extent object classifies as
  `unknown`.  Whether that is a gap or a deliberate scoping to class types, it is
  pinned here.
*/
bool
tests_hostile_types_rejected()
{
    bool ok = true;

    D_BCT_CHECK((holds_for_none<has_fixed_extent_signal,
                                D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((holds_for_none<has_tuple_size_signal,
                                D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((holds_for_none<has_static_bounds_signal,
                                D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((holds_for_none<has_capacity_signal,
                                D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((holds_for_none<has_reserve_signal,
                                D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((holds_for_none<has_max_size_signal,
                                D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((holds_for_none<has_size_signal,
                                D_TEST_HOSTILE_TYPES>::value));

    D_BCT_CHECK((holds_for_none<is_bounded_container,
                                D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((holds_for_none<is_unbounded_container,
                                D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((holds_for_none<is_domain_bounded_container,
                                D_TEST_HOSTILE_TYPES>::value));

    // a raw array has an extent but no `extent`, so it is unknown
    D_BCT_CHECK(!is_bounded_container<int[3]>::value);
    D_BCT_CHECK(!is_unbounded_container<int[3]>::value);
    D_BCT_CHECK(capacity_bound_of<int[3]>::value == capacity_bound::unknown);

    return ok;
}

/*
tests_hostile_types_survived
  SFINAE-friendliness: the traits must REPORT false on a hostile type, not
  DIAGNOSE it.
  Tests the following:
  - the value traits and the snapshot instantiate over the hostile shapes -
    void, a function type, an array, a member pointer - without a hard error

  This is the one property that cannot be observed at run time: a trait that
  hard-errors takes the translation unit with it, so a failure here is a failed
  BUILD, not a failed check.  The function returns true if it was compiled at
  all; the work it does is forcing the instantiations.  count_holds is what makes
  that reliable - it does not short-circuit, so a trait that explodes on the
  twentieth of twenty-six types is still reached.
*/
bool
tests_hostile_types_survived()
{
    bool ok = true;

    // forcing every cell: if any trait hard-errors on any hostile type, this
    // TU does not compile and the failure is a build failure, by design
    D_BCT_CHECK((count_holds<has_size_signal,
                             D_TEST_HOSTILE_TYPES>::value == 0));
    D_BCT_CHECK((count_holds<is_bounded_container,
                             D_TEST_HOSTILE_TYPES>::value == 0));
    D_BCT_CHECK((count_holds<is_unbounded_container,
                             D_TEST_HOSTILE_TYPES>::value == 0));
    D_BCT_CHECK((count_holds<is_domain_bounded_container,
                             D_TEST_HOSTILE_TYPES>::value == 0));

    // capacity_bound_of and the snapshot are not bool traits, so they are
    // instantiated by hand on the shapes most likely to break a probe
    D_BCT_CHECK(capacity_bound_of<void>::value == capacity_bound::unknown);
    D_BCT_CHECK(capacity_bound_of<const void>::value ==
                capacity_bound::unknown);
    D_BCT_CHECK(capacity_bound_of<int(int)>::value == capacity_bound::unknown);
    D_BCT_CHECK(capacity_bound_of<int (*)(int)>::value ==
                capacity_bound::unknown);
    D_BCT_CHECK(capacity_bound_of<int[3]>::value == capacity_bound::unknown);
    D_BCT_CHECK(capacity_bound_of<int[]>::value == capacity_bound::unknown);
    D_BCT_CHECK(capacity_bound_of<int&>::value == capacity_bound::unknown);
    D_BCT_CHECK(capacity_bound_of<std::nullptr_t>::value ==
                capacity_bound::unknown);

    D_BCT_CHECK(!bounded_container_class<void>::is_bounded);
    D_BCT_CHECK(!bounded_container_class<int(int)>::is_bounded);
    D_BCT_CHECK(!bounded_container_class<int[3]>::is_bounded);
    D_BCT_CHECK(bounded_container_class<void>::capacity ==
                capacity_bound::unknown);

    return ok;
}

/*
tests_incomplete_type
  An incomplete type must be tolerated, not completed.
  Tests the following:
  - every trait answers false for a type that is declared and never defined

  This is the probe property that is easiest to lose.  `std::tuple_size<T>::value`
  on an incomplete T, or `declval<const T&>().size()` on one, is an error in the
  IMMEDIATE context of the substitution - which is what makes it a substitution
  failure rather than a diagnostic.  A probe rewritten to complete the type first
  (say, by taking sizeof, or by inheriting from it) would compile against every
  other fixture in this suite and fail only here.
*/
bool
tests_incomplete_type()
{
    bool ok = true;

    using incomplete = ::djinterp::test::fixtures::incomplete;

    D_BCT_CHECK(!has_fixed_extent_signal<incomplete>::value);
    D_BCT_CHECK(!has_tuple_size_signal<incomplete>::value);
    D_BCT_CHECK(!has_static_bounds_signal<incomplete>::value);
    D_BCT_CHECK(!has_capacity_signal<incomplete>::value);
    D_BCT_CHECK(!has_reserve_signal<incomplete>::value);
    D_BCT_CHECK(!has_max_size_signal<incomplete>::value);
    D_BCT_CHECK(!has_size_signal<incomplete>::value);

    D_BCT_CHECK(!is_bounded_container<incomplete>::value);
    D_BCT_CHECK(!is_unbounded_container<incomplete>::value);
    D_BCT_CHECK(!is_domain_bounded_container<incomplete>::value);

    D_BCT_CHECK(capacity_bound_of<incomplete>::value ==
                capacity_bound::unknown);
    D_BCT_CHECK(!bounded_container_class<incomplete>::is_bounded);

    // and it holds through the cv-ref forms too
    D_BCT_CHECK(!is_bounded_container<const incomplete&>::value);
    D_BCT_CHECK(!is_bounded_container<incomplete&&>::value);

    return ok;
}

/*
tests_trait_shape
  Is each trait a well-formed bool trait?
  Tests the following:
  - the SEVEN signal traits are: emitted by D_TYPE_TRAIT_TRUE, they derive from
    std::true_type / std::false_type and carry value_type, type, a conversion to
    bool and a call operator
  - is_domain_bounded_container is: it derives from std::integral_constant
  - is_bounded_container is NOT
  - is_unbounded_container is NOT

  THE FINDING.  is_bounded_container and is_unbounded_container are hand-written
  as

      template<typename _Type>
      struct is_bounded_container
      {
      private:
          using clean_type = clean_t<_Type>;
      public:
          static constexpr bool value = ...;
      };

  - a `value` and nothing else.  No integral_constant base, and therefore no
  `value_type`, no `::type`, no conversion to bool, no operator().  The
  consequences are not hypothetical:

      conjunction<is_bounded_container<T>, ...>     compiles  (wants ::value)
      enable_if_t<is_bounded_container<T>::value>   compiles  (wants ::value)
      conditional_t<is_bounded_container<T>{}, A, B>   DOES NOT (wants a bool)
      negation<is_bounded_container<T>>                DOES NOT (wants ::type)
      static_assert(is_bounded_container<T>{})         DOES NOT (wants a bool)

  and any generic code that treats the framework's traits uniformly - a
  trait-composition combinator, a concept face forwarding to `::type` - breaks on
  exactly these two and on nothing else in the header.  The fix is one line each:
  derive from `bool_constant<...>` instead of declaring `value`.  The checks below
  pin the CURRENT shape, so that fix flips the four negative assertions and the
  suite tells you it worked.
*/
bool
tests_trait_shape()
{
    bool ok = true;

    // the seven signal traits: well-formed bool traits, on every hostile type
    D_BCT_CHECK((trait_is_well_formed<has_fixed_extent_signal,
                                      D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((trait_is_well_formed<has_tuple_size_signal,
                                      D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((trait_is_well_formed<has_static_bounds_signal,
                                      D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((trait_is_well_formed<has_capacity_signal,
                                      D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((trait_is_well_formed<has_reserve_signal,
                                      D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((trait_is_well_formed<has_max_size_signal,
                                      D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK((trait_is_well_formed<has_size_signal,
                                      D_TEST_HOSTILE_TYPES>::value));

    // is_domain_bounded_container: derives from integral_constant, so it is
    D_BCT_CHECK((trait_is_well_formed<is_domain_bounded_container,
                                      D_TEST_HOSTILE_TYPES>::value));
    D_BCT_CHECK(is_bool_trait<is_domain_bounded_container<bounds_both>>::value);

    // is_bounded_container / is_unbounded_container: they are NOT
    D_BCT_CHECK(!is_bool_trait<is_bounded_container<std::vector<int>>>::value);
    D_BCT_CHECK(!is_bool_trait<
                    is_unbounded_container<std::vector<int>>>::value);
    D_BCT_CHECK(!(trait_is_well_formed<is_bounded_container,
                                       std::vector<int>>::value));
    D_BCT_CHECK(!(trait_is_well_formed<is_unbounded_container,
                                       std::vector<int>>::value));

    // exactly which members are missing
    using bounded_verdict = is_bounded_container<std::array<int, 3>>;
    using domain_verdict  = is_domain_bounded_container<bounds_both>;
    using size_signal     = has_size_signal<size_const>;

    // the signal traits have all four
    D_BCT_CHECK((::djinterp::is_detected<p_trait_value_type,
                                         size_signal>::value));
    D_BCT_CHECK((::djinterp::is_detected<p_trait_type, size_signal>::value));
    D_BCT_CHECK((::djinterp::is_detected<p_trait_to_bool,
                                         size_signal>::value));
    D_BCT_CHECK((::djinterp::is_detected<p_trait_call, size_signal>::value));

    // is_domain_bounded_container has all four
    D_BCT_CHECK((::djinterp::is_detected<p_trait_value_type,
                                         domain_verdict>::value));
    D_BCT_CHECK((::djinterp::is_detected<p_trait_type, domain_verdict>::value));
    D_BCT_CHECK((::djinterp::is_detected<p_trait_to_bool,
                                         domain_verdict>::value));
    D_BCT_CHECK((::djinterp::is_detected<p_trait_call, domain_verdict>::value));

    // is_bounded_container has NONE of them - only `value`
    D_BCT_CHECK((!::djinterp::is_detected<p_trait_value_type,
                                          bounded_verdict>::value));
    D_BCT_CHECK((!::djinterp::is_detected<p_trait_type,
                                          bounded_verdict>::value));
    D_BCT_CHECK((!::djinterp::is_detected<p_trait_to_bool,
                                          bounded_verdict>::value));
    D_BCT_CHECK((!::djinterp::is_detected<p_trait_call,
                                          bounded_verdict>::value));

    // capacity_bound_of is a VALUE trait by design: it supplies ::type but does
    // not derive from it, and its `value` is a capacity_bound, not a bool.  It
    // is therefore correctly NOT a bool trait - a different thing from the two
    // above, which are bool traits that forgot to be
    D_BCT_CHECK((::djinterp::is_detected<p_trait_type,
                                         capacity_bound_of<
                                             std::vector<int>>>::value));
    D_BCT_CHECK((!::djinterp::is_detected<p_trait_value_type,
                                          capacity_bound_of<
                                              std::vector<int>>>::value));
    D_BCT_CHECK(!is_bool_trait<capacity_bound_of<std::vector<int>>>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
