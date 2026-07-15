// djinterp
#include "test_traits_tests.hpp"

// std
#include <type_traits>


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                THE PINS THEMSELVES                                       ///
///////////////////////////////////////////////////////////////////////////////
//
//   D_TEST_STATIC, used exactly as the header says to use it: for invariants
// whose regression should stop the line, because everything downstream of them
// is meaningless if they go.  All four below are properties OF THE TOOLKIT, and
// every other test in this suite is worthless if any of them is false - so
// there is nothing worth reporting, and a dead build is the honest outcome.
//
//   These are also the coverage of D_TEST_STATIC itself.  It expands to a
// static_assert whose message is the stringized condition, so a firing pin names
// itself; that it compiles at all here, with a comma in the condition and with a
// fully qualified name, is what proves the macro is variadic and paste-clean.

// the shape checker is itself well-shaped, for every type in the zoo.  If this
// goes, is_bool_trait cannot be trusted about anything, including itself
D_TEST_STATIC(::djinterp::test::trait_is_well_formed<
                  ::djinterp::test::is_bool_trait,
                  D_TEST_HOSTILE_TYPES>::value);

// the probes are SFINAE-clean over the zoo.  A probe that diagnosed instead of
// reporting false would take this TU with it, which is the only way to detect
// the failure from inside the same translation unit
D_TEST_STATIC(::djinterp::test::holds_for_none<has_member_value_type,
                                               D_TEST_HOSTILE_TYPES>::value);

// the cv-ref matrix is TOTAL - it runs over void without a guard.  A cell that
// stopped being total would break every suite that ever hands the matrix a
// non-object type
D_TEST_STATIC(::djinterp::test::trait_ignores_cvref<always_true, void>::value);

// a condition containing a COMMA, which only survives because D_TEST_STATIC is
// variadic.  The commonest thing anyone will ever pin is a two-argument trait,
// so a non-variadic spelling of this macro would fail on its first real use
D_TEST_STATIC(std::is_same<int, int>::value);


/*
tests_static_pin
  D_TEST_STATIC - pins a compile-time fact, failing the BUILD rather than the
  report.

  The macro cannot be tested the way everything else here is tested: a pin that
  holds produces nothing, and a pin that fails produces no report at all.  So the
  coverage is the four pins at the top of this file - which are real invariants,
  not decorations - plus the two properties of the macro itself that CAN be
  checked from inside a function.

  Tests the following:
  - the same conditions the pins assert are true at run time as well, so a
    reader can see WHAT was pinned without reading a static_assert
  - the macro is variadic: a condition containing a top-level comma survives it.
    That is not cosmetic - the commonest thing anyone pins is a two-argument
    trait, and a non-variadic spelling would fail on its first real use
  - the stringized condition becomes the diagnostic, so a firing pin names
    itself.  Demonstrated (by failing the build) under D_TT_HAZARD_TESTS
*/
bool
tests_static_pin()
{
    bool ok = true;

    // the four pins, restated as ordinary checks so the report shows them
    D_TT_CHECK((trait_is_well_formed<is_bool_trait,
                                     D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((holds_for_none<has_member_value_type,
                               D_TEST_HOSTILE_TYPES>::value));
    D_TT_CHECK((trait_ignores_cvref<always_true, void>::value));
    D_TT_CHECK((std::is_same<int, int>::value));

    // a pin inside a function body is legal too - static_assert is a declaration
    D_TEST_STATIC(is_bool_trait<std::true_type>::value);
    D_TEST_STATIC(count_holds<always_true, D_TEST_HOSTILE_TYPES>::value == 26);
    D_TEST_STATIC(!is_bool_trait<value_only<int>>::value);

    return ok;
}

/*
tests_build_time_hazards
  THE FOUR THINGS THAT CANNOT BE CHECKED AT RUN TIME.

  Four of this header's guarantees are guarantees about COMPILATION.  A passing
  run says nothing about any of them, because the failure mode of each is that
  the translation unit does not exist.  Each is demonstrated here in two halves:
  the SAFE half unconditionally, so the contrast is real, and the half that
  breaks the build behind D_TT_HAZARD_TESTS.

  Build with -DD_TT_HAZARD_TESTS=1 to reproduce all four.  The build fails four
  times, each with a named diagnostic, and that is the finding.

  ---------------------------------------------------------------------------
  1.  count_holds DOES NOT SHORT-CIRCUIT
  ---------------------------------------------------------------------------
  The reason section III exists.  std::conjunction stops instantiating at the
  first false; count_holds expands every cell into a template-argument list and
  instantiates all of them.  A trait that hard-errors on a late type therefore
  breaks the BUILD under count_holds and HIDES under conjunction.

      SAFE   conjunction<detonates<int>, detonates<char>, detonates<poison>>
             compiles - it stops at the false and never reaches the third
      MINE   count_holds<detonates, int, char, poison>
             does not compile - it reaches all three

  A hard error cannot be caught, only provoked.  This is the provocation.

  ---------------------------------------------------------------------------
  2.  is_nothrow_probe IS NOT TOTAL OVER PROBES
  ---------------------------------------------------------------------------
  It is spelled `struct is_nothrow_probe : detected_or_t<false_type, ...>`, so it
  derives from whatever the probe YIELDS.  For a D_TEST_NOEXCEPT_PROBE that is a
  bool_constant and all is well; for a probe that fails to detect it is
  std::false_type and all is still well.  For an EXPR probe whose detection
  SUCCEEDS it is the expression's type - and `struct : std::size_t` is a hard
  error.

      SAFE   is_nothrow_probe<p_value_type, plain>     probe fails  -> false_type
      MINE   is_nothrow_probe<p_size, sized>           probe yields size_t

  The header's comment says the trait "reads a D_TEST_NOEXCEPT_PROBE".  It does
  not say what happens if you hand it something else, and what happens is this.
  A static_assert on the probe's result being a bool_constant would cost one line
  and turn the base-clause error into a named diagnostic.

  ---------------------------------------------------------------------------
  3.  D_TEST_STATIC FAILS THE BUILD
  ---------------------------------------------------------------------------
  By design, and the stringized condition is the diagnostic - "djinterp test:
  <condition>".  A firing pin names itself, which is the whole reason to reach
  for it over a reported check.

  ---------------------------------------------------------------------------
  4.  WHY THE _COMPLETE LISTS EXIST
  ---------------------------------------------------------------------------
  std::is_trivially_destructible MANDATES a complete type.  Run it over
  D_TEST_HOSTILE_TYPES_COMPLETE and it answers; run it over D_TEST_HOSTILE_TYPES,
  which contains the incomplete fixture, and libstdc++ static_asserts.  That is
  not a defect in the list - it is the whole reason the list is split, and it is
  the same failure that constexpr_container_traits.hpp inherits from
  lifetime.hpp's is_literal_type.

  Tests the following (safely):
  - conjunction really does short-circuit past a cell that would detonate
  - count_holds and conjunction agree wherever conjunction is willing to look
  - is_nothrow_probe over a NON-detecting probe is safe, and answers false
  - a completeness-requiring trait runs over the _COMPLETE list
  - the four mines are behind D_TT_HAZARD_TESTS
*/
bool
tests_build_time_hazards()
{
    bool ok = true;

    // 1.  the SAFE half: conjunction short-circuits past the detonator.  This
    //     TU compiling at all is the assertion
    D_TT_CHECK((!std::conjunction<detonates_on_poison<int>,
                                  detonates_on_poison<char>,
                                  detonates_on_poison<poison>>::value));
    D_TT_CHECK((count_holds<detonates_on_poison, int, char>::value == 1));

    // 2.  the SAFE half: a probe that does not detect gives false_type, and
    //     is_nothrow_probe derives from THAT quite happily
    D_TT_CHECK(!is_nothrow_probe<p_value_type, plain>::value);
    D_TT_CHECK(!is_nothrow_probe<p_size, plain>::value);
    D_TT_CHECK(is_bool_trait<is_nothrow_probe<p_size, plain>>::value);

    //     ...and a probe that yields a bool_constant is what it was built for
    D_TT_CHECK(is_nothrow_probe<p_swap_noexcept, sized>::value);

    // 3.  D_TEST_STATIC, holding
    D_TEST_STATIC(true);

    // 4.  the SAFE half: the _COMPLETE list is the one a completeness-requiring
    //     trait can be run over
    D_TT_CHECK((count_holds<std::is_trivially_destructible,
                            D_TEST_HOSTILE_TYPES_COMPLETE>::value > 0));

#if D_TT_HAZARD_TESTS

    // *** MINE 1: count_holds does not short-circuit ***
    //
    //   error: static assertion failed: count_holds instantiated this cell --
    //          which is exactly the point: it does not short-circuit
    //
    //   The same three types, the same trait, in the same order.  conjunction
    // above compiled.  This does not.
    D_TT_CHECK((count_holds<detonates_on_poison,
                            int, char, poison>::value == 1));

    // *** MINE 2: is_nothrow_probe handed a probe that is not one ***
    //
    //   error: base type 'djinterp::detected_or_t<std::false_type, p_size,
    //          sized>' {aka 'long unsigned int'} fails to be a struct or class
    //          type
    //
    //   p_size DETECTS for sized, and yields std::size_t.  is_nothrow_probe
    // derives from the probe's result, and one cannot derive from a size_t.
    D_TT_CHECK(is_nothrow_probe<p_size, sized>::value);

    //   ...and the same for a TYPE probe whose detection succeeds
    D_TT_CHECK(is_nothrow_probe<p_value_type, sized>::value);

    // *** MINE 3: D_TEST_STATIC fails the build, and names itself ***
    //
    //   error: static assertion failed: djinterp test: is_bool_trait<
    //          value_only<int>>::value
    //
    //   The stringized condition IS the message.  That is the whole reason to
    // reach for a pin over a reported check: the diagnostic needs no lookup.
    D_TEST_STATIC(is_bool_trait<value_only<int>>::value);

    // *** MINE 4: why the _COMPLETE lists exist ***
    //
    //   error: static assertion failed: template argument must be a complete
    //          class or an unbounded array
    //
    //   std::is_trivially_destructible mandates a complete type.  The full
    // hostile list contains fixtures::incomplete.  Splitting the list is not a
    // convenience; it is the only way to run a completeness-requiring trait over
    // a battery at all.
    D_TT_CHECK((count_holds<std::is_trivially_destructible,
                            D_TEST_HOSTILE_TYPES>::value > 0));

#endif  // D_TT_HAZARD_TESTS

    return ok;
}


NS_END  // testing
NS_END  // djinterp
