// djinterp
#include "test_traits_tests.hpp"

// std
#include <type_traits>


NS_DJINTERP
NS_TESTING


/*
tests_count_holds
  count_holds - how many of `_Types...` satisfy `_Trait`.
  Tests the following:
  - 0, 1, some and all
  - the count is a std::size_t and the trait derives from
    integral_constant<size_t, N> - so it is NOT a bool trait, and correctly so:
    it is a count, and a caller that folded it into a boolean would be reading
    "at least one" where it meant "exactly N"
  - the answer is independent of the ORDER of the pack
  - it works over the hostile lists, which is what pins their cardinalities
*/
bool
tests_count_holds()
{
    bool ok = true;

    // 0, 1, some, all
    D_TT_CHECK((count_holds<always_false, int, char, long>::value == 0));
    D_TT_CHECK((count_holds<is_int, char, long>::value == 0));
    D_TT_CHECK((count_holds<is_int, int>::value == 1));
    D_TT_CHECK((count_holds<is_int, char, int, long>::value == 1));
    D_TT_CHECK((count_holds<is_int, int, int, char>::value == 2));
    D_TT_CHECK((count_holds<always_true, int, char, long>::value == 3));

    // order does not matter
    D_TT_CHECK((count_holds<is_int, int, char, long>::value ==
                count_holds<is_int, long, char, int>::value));

    // it counts, it does not merely test
    D_TT_CHECK((std::is_same<count_holds<is_int, int>::value_type,
                             std::size_t>::value));
    D_TT_CHECK(!is_bool_trait<count_holds<is_int, int>>::value);

    // and it survives the zoo - which is the property the whole header rests on
    D_TT_CHECK((count_holds<always_false, D_TEST_HOSTILE_TYPES>::value == 0));

    return ok;
}

/*
tests_count_holds_empty_pack
  THE VACUOUS ROW.  With no types at all:

      count_holds   == 0
      holds_for_all == TRUE      (0 == sizeof...(0))
      holds_for_any == false     (0 > 0 is false)
      holds_for_none == TRUE     (0 == 0)

  So holds_for_all and holds_for_none are BOTH true over an empty pack, which is
  correct - vacuous truth - and is also the kind of correct that surprises a
  caller who reached for `all` expecting it to imply `any`.  It does not.

  Tests the following:
  - all four traits over the empty pack
  - the identity that makes it safe to rely on: for a non-empty pack,
    all -> any, and none -> !any.  For the empty pack the first implication is
    vacuously... false in the antecedent-free sense: all is true and any is not
  - trait_is_well_formed over an empty pack is likewise vacuously true, which is
    worth knowing before someone writes trait_is_well_formed<T> and reads the
    `true` as an endorsement
*/
bool
tests_count_holds_empty_pack()
{
    bool ok = true;

    D_TT_CHECK((count_holds<always_true>::value == 0));
    D_TT_CHECK((count_holds<always_false>::value == 0));

    D_TT_CHECK(holds_for_all<always_true>::value);     // vacuously TRUE
    D_TT_CHECK(holds_for_all<always_false>::value);    // ...and for a false trait
    D_TT_CHECK(!holds_for_any<always_true>::value);    // FALSE
    D_TT_CHECK(!holds_for_any<always_false>::value);
    D_TT_CHECK(holds_for_none<always_true>::value);    // vacuously TRUE
    D_TT_CHECK(holds_for_none<always_false>::value);

    // all AND none, simultaneously - the surprise, pinned
    D_TT_CHECK(holds_for_all<always_true>::value &&
               holds_for_none<always_true>::value);

    // ...which is precisely what does NOT happen for a non-empty pack
    D_TT_CHECK(!(holds_for_all<always_true, int>::value &&
                 holds_for_none<always_true, int>::value));

    // and for a non-empty pack, all implies any
    D_TT_CHECK(!holds_for_all<always_true, int, char>::value ||
               holds_for_any<always_true, int, char>::value);

    // trait_is_well_formed inherits the vacuity through holds_for_all
    D_TT_CHECK(trait_is_well_formed<always_true>::value);
    D_TT_CHECK(trait_is_well_formed<value_only>::value);   // a BROKEN trait!

    return ok;
}

/*
tests_holds_for_all
  holds_for_all - true iff every type in the pack satisfies the trait.
  Tests the following:
  - all satisfy    -> true
  - one fails      -> false, wherever in the pack it sits
  - none satisfy   -> false
  - it is defined as `count == sizeof...(_Types)`, which is checked against
    count_holds directly rather than assumed
*/
bool
tests_holds_for_all()
{
    bool ok = true;

    D_TT_CHECK((holds_for_all<always_true, int, char, long>::value));
    D_TT_CHECK((holds_for_all<is_int, int, int>::value));

    // one failure anywhere is enough - first, middle and last
    D_TT_CHECK((!holds_for_all<is_int, char, int, int>::value));
    D_TT_CHECK((!holds_for_all<is_int, int, char, int>::value));
    D_TT_CHECK((!holds_for_all<is_int, int, int, char>::value));

    D_TT_CHECK((!holds_for_all<always_false, int, char>::value));

    // it IS the count, restated
    D_TT_CHECK((holds_for_all<is_int, int, char>::value ==
                (count_holds<is_int, int, char>::value == 2)));
    D_TT_CHECK((holds_for_all<is_int, int, int>::value ==
                (count_holds<is_int, int, int>::value == 2)));

    return ok;
}

/*
tests_holds_for_any
  holds_for_any - true iff at least one type satisfies the trait.
  Tests the following:
  - one satisfies -> true, wherever in the pack it sits
  - none satisfy  -> false
  - it is `count > 0`, checked against count_holds
*/
bool
tests_holds_for_any()
{
    bool ok = true;

    D_TT_CHECK((holds_for_any<is_int, int, char, long>::value));
    D_TT_CHECK((holds_for_any<is_int, char, int, long>::value));
    D_TT_CHECK((holds_for_any<is_int, char, long, int>::value));
    D_TT_CHECK((holds_for_any<always_true, int>::value));

    D_TT_CHECK((!holds_for_any<is_int, char, long>::value));
    D_TT_CHECK((!holds_for_any<always_false, int, char>::value));

    D_TT_CHECK((holds_for_any<is_int, char, int>::value ==
                (count_holds<is_int, char, int>::value > 0)));

    // any and none are complements over a non-empty pack
    D_TT_CHECK((holds_for_any<is_int, char, int>::value !=
                holds_for_none<is_int, char, int>::value));
    D_TT_CHECK((holds_for_any<is_int, char, long>::value !=
                holds_for_none<is_int, char, long>::value));

    return ok;
}

/*
tests_holds_for_none
  holds_for_none - true iff no type satisfies the trait.  The negative battery,
  and - paired with the D_TEST_HOSTILE_* lists - the one-line statement that a
  trait rejects everything it should reject AND survives everything it cannot
  classify.
  Tests the following:
  - none satisfy -> true
  - one satisfies -> false
  - it is `count == 0`, checked against count_holds
  - the battery form: a trait that recognizes nothing in the zoo
*/
bool
tests_holds_for_none()
{
    bool ok = true;

    D_TT_CHECK((holds_for_none<is_int, char, long>::value));
    D_TT_CHECK((holds_for_none<always_false, int, char, long>::value));

    D_TT_CHECK((!holds_for_none<is_int, int, char>::value));
    D_TT_CHECK((!holds_for_none<is_int, char, int>::value));
    D_TT_CHECK((!holds_for_none<always_true, int>::value));

    D_TT_CHECK((holds_for_none<is_int, char>::value ==
                (count_holds<is_int, char>::value == 0)));

    // the battery, over the whole zoo
    D_TT_CHECK((holds_for_none<is_int, D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((holds_for_none<always_false, D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((!holds_for_none<always_true, D_TEST_HOSTILE_TYPES>::value));

    return ok;
}

/*
tests_quantifier_binding
  The quantifiers take a ONE-parameter template-template argument:

      template<template<typename> typename _Trait, typename... _Types>

  and the framework's traits are almost all TWO-parameter, because that is what
  trait_detect.hpp emits:

      template<typename _Type, typename = void> struct is_foo;

  Those bind only because of C++17's P0522, which relaxed template-template
  matching to allow a template with defaulted extra parameters.  It is the
  quietest load-bearing thing in the header: if it ever stops holding - a
  compiler without P0522, a flag that disables it - every suite in the framework
  stops compiling at once, and nothing in any of them says why.

  Tests the following:
  - a genuinely one-parameter trait binds
  - a two-parameter-with-default trait (the framework's own style) binds
  - both give the right answers through the quantifiers
  - a two-parameter trait from the standard library binds too (std::is_same has
    no default, so it does NOT - which is why the check below uses a bound one)
*/
bool
tests_quantifier_binding()
{
    bool ok = true;

    // one parameter
    D_TT_CHECK((count_holds<is_int, int, char>::value == 1));

    // two parameters, the second defaulted - the trait_detect.hpp style
    D_TT_CHECK((count_holds<has_member_value_type,
                            sized, plain, fixtures::value_type_a>::value == 2));
    D_TT_CHECK((holds_for_all<has_member_value_type,
                              sized, fixtures::value_type_a>::value));
    D_TT_CHECK((holds_for_none<has_member_value_type,
                               plain, int, void>::value));

    // and the same trait survives the zoo through the quantifier
    D_TT_CHECK((holds_for_none<has_member_value_type,
                               D_TEST_HOSTILE_TYPES>::value));

    // std::is_trivially_destructible is a one-parameter std trait, and binds
    D_TT_CHECK((count_holds<std::is_trivially_destructible,
                            int, fixtures::throwing>::value == 1));

    return ok;
}

/*
tests_no_short_circuit
  THE CENTRAL CLAIM OF SECTION III, and the reason count_holds exists at all
  rather than a fold over std::conjunction.

  std::conjunction SHORT-CIRCUITS: it stops instantiating at the first false.  In
  production that is a feature.  In a trait test it is a hole, because the cell
  that would have failed to COMPILE is precisely the one the test needed to
  reach - a trait that hard-errors on the fourth of five types hides behind the
  third, and the suite passes.

  count_holds expands `_Trait<_Types>::value...` into a template-argument list,
  so every cell is instantiated, always, before anything is counted.  A trait
  that explodes on a late type therefore breaks the BUILD.

  That claim has two halves and only one of them can be asserted at run time:

    the premise    std::conjunction really does short-circuit
    the payoff     count_holds really does not

  The premise is checked here, unconditionally, with a subject whose late cell
  contains a static_assert that would fire if it were ever instantiated - and
  does not, because conjunction never gets there.  The payoff is the same subject
  handed to count_holds, which does not compile; it lives behind D_TT_HAZARD_TESTS
  in tests_build_time_hazards, because a suite that does not build reports
  nothing.

  Tests the following:
  - conjunction stops at the first false and never touches what follows
  - conjunction stops at the first false even when what follows would explode
  - count_holds agrees with conjunction on every case where conjunction is
    willing to look
*/
bool
tests_no_short_circuit()
{
    bool ok = true;

    // the premise: conjunction short-circuits.  If it did not, the third operand
    // below would be instantiated - and detonating is exactly what
    // detonates_on_poison<poison> does.  This TU compiling is the assertion
    D_TT_CHECK((!std::conjunction<detonates_on_poison<int>,
                                  detonates_on_poison<char>,
                                  detonates_on_poison<poison>>::value));

    // ...and with no false to stop at, it would reach the third.  Not written.

    // conjunction and count_holds agree wherever conjunction is willing to look
    D_TT_CHECK((std::conjunction<is_int<int>, is_int<int>>::value ==
                holds_for_all<is_int, int, int>::value));
    D_TT_CHECK((std::conjunction<is_int<int>, is_int<char>>::value ==
                holds_for_all<is_int, int, char>::value));
    D_TT_CHECK((std::disjunction<is_int<char>, is_int<int>>::value ==
                holds_for_any<is_int, char, int>::value));
    D_TT_CHECK((std::disjunction<is_int<char>, is_int<long>>::value ==
                holds_for_any<is_int, char, long>::value));

    // the difference is invisible at run time and total at compile time.  See
    // tests_build_time_hazards, and -DD_TT_HAZARD_TESTS=1
    D_TT_CHECK((count_holds<detonates_on_poison, int, char>::value == 1));

    return ok;
}

/*
tests_quantifier_value_companions
  The four `_v` companions of section III.
  Tests the following:
  - count_holds_v, holds_for_all_v, holds_for_any_v and holds_for_none_v each
    carry the same value as their trait
  - count_holds_v is a std::size_t, not a bool - it cannot be folded into a
    boolean expression by accident
*/
bool
tests_quantifier_value_companions()
{
    bool ok = true;

    D_TT_CHECK((count_holds_v<is_int, int, char> ==
                count_holds<is_int, int, char>::value));
    D_TT_CHECK((count_holds_v<always_true, D_TEST_HOSTILE_TYPES> ==
                count_holds<always_true, D_TEST_HOSTILE_TYPES>::value));

    D_TT_CHECK((holds_for_all_v<is_int, int> ==
                holds_for_all<is_int, int>::value));
    D_TT_CHECK((holds_for_all_v<is_int, int, char> ==
                holds_for_all<is_int, int, char>::value));

    D_TT_CHECK((holds_for_any_v<is_int, char, int> ==
                holds_for_any<is_int, char, int>::value));
    D_TT_CHECK((holds_for_any_v<is_int, char, long> ==
                holds_for_any<is_int, char, long>::value));

    D_TT_CHECK((holds_for_none_v<is_int, char> ==
                holds_for_none<is_int, char>::value));
    D_TT_CHECK((holds_for_none_v<is_int, int> ==
                holds_for_none<is_int, int>::value));

    // the shorthands really are the values
    D_TT_CHECK((count_holds_v<is_int, int, int, char> == 2));
    D_TT_CHECK((holds_for_all_v<always_true, int, char>));
    D_TT_CHECK((holds_for_any_v<is_int, char, int>));
    D_TT_CHECK((holds_for_none_v<always_false, int, char>));

    // count_holds_v is a size_t, the other three are bools
    D_TT_CHECK((std::is_same<decltype(count_holds_v<is_int, int>),
                             const std::size_t>::value));
    D_TT_CHECK((std::is_same<decltype(holds_for_all_v<is_int, int>),
                             const bool>::value));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
