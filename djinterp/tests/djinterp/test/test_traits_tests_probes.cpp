// djinterp
#include "test_traits_tests.hpp"

// std
#include <type_traits>
#include <utility>


NS_DJINTERP
NS_TESTING


/*
tests_type_probe
  D_TEST_TYPE_PROBE - a one-parameter TYPE probe.
  Tests the following:
  - the emitted alias template is well-formed exactly when the type expression
    is, so is_detected reads it: `sized` has a value_type, `plain` does not
  - the parameter really is named `_Type` - the body was written against that
    name and nothing else
  - detected_t hands back the type itself, not merely "yes"
  - detected_or_t supplies the fallback, and nonesuch is what detected_t yields
    on failure
  - is_detected_exact and is_detected_convertible sharpen the answer
  - a probe over a type with no members at all does not diagnose - it reports
    false.  This is the property the whole header exists for, and it is the one
    a test can only state by pointing the probe at a type it must fail on
*/
bool
tests_type_probe()
{
    bool ok = true;

    D_TT_CHECK(is_detected<p_value_type, sized>::value);
    D_TT_CHECK(!is_detected<p_value_type, plain>::value);
    D_TT_CHECK(!is_detected<p_value_type, int>::value);
    D_TT_CHECK(!is_detected<p_value_type, void>::value);

    // the probe yields the type, not a yes
    D_TT_CHECK((std::is_same<detected_t<p_value_type, sized>, int>::value));
    D_TT_CHECK((std::is_same<detected_t<p_value_type, plain>,
                             nonesuch>::value));
    D_TT_CHECK((std::is_same<detected_or_t<char, p_value_type, plain>,
                             char>::value));
    D_TT_CHECK((std::is_same<detected_or_t<char, p_value_type, sized>,
                             int>::value));

    // ...which is what lets the sharper readings work
    D_TT_CHECK((is_detected_exact<int, p_value_type, sized>::value));
    D_TT_CHECK((!is_detected_exact<char, p_value_type, sized>::value));
    D_TT_CHECK((is_detected_convertible<long, p_value_type, sized>::value));

    // the fixtures whose whole job is to be probed and not to explode
    D_TT_CHECK(!is_detected<p_value_type, fixtures::empty>::value);
    D_TT_CHECK(!is_detected<p_value_type, fixtures::incomplete>::value);
    D_TT_CHECK(!is_detected<p_value_type, fixtures::abstract>::value);
    D_TT_CHECK(!is_detected<p_value_type, fixtures::function_type>::value);
    D_TT_CHECK(!is_detected<p_value_type, fixtures::array_type>::value);

    return ok;
}

/*
tests_type_probe_2
  D_TEST_TYPE_PROBE_2 - the binary form, over `_Type` and `_Other`.
  Tests the following:
  - both parameters are usable in the body
  - the probe is read by the same is_detected, with two arguments
  - it declines when the pair has no common type

  AND THE MACRO'S OWN EDGE.  The body of p_common is

      std::common_type_t<_Type, _Other>

  which contains a TOP-LEVEL COMMA as far as the preprocessor is concerned:
  angle brackets do not protect a comma, only parentheses do.  So that body
  reaches D_TEST_TYPE_PROBE_2 as TWO macro arguments, and is reassembled only
  because the declarator is variadic and pastes __VA_ARGS__ back with the comma
  intact.  A non-variadic spelling would reject the commonest shape a binary
  type probe takes; that the probe below exists at all is the test.
*/
bool
tests_type_probe_2()
{
    bool ok = true;

    D_TT_CHECK((is_detected<p_common, int, long>::value));
    D_TT_CHECK((is_detected<p_common, int, char>::value));
    D_TT_CHECK((is_detected<p_common, sized, sized>::value));

    // no common type
    D_TT_CHECK((!is_detected<p_common, int, plain>::value));
    D_TT_CHECK((!is_detected<p_common, sized, plain>::value));
    D_TT_CHECK((!is_detected<p_common, int, void>::value));

    // and it hands back the common type
    D_TT_CHECK((std::is_same<detected_t<p_common, int, long>, long>::value));
    D_TT_CHECK((is_detected_exact<long, p_common, int, long>::value));

    return ok;
}

/*
tests_expr_probe
  D_TEST_EXPR_PROBE - a one-parameter EXPRESSION probe.
  Tests the following:
  - the probe is the decltype of the expression, so a successful detection hands
    back the RESULT TYPE - which is what lets section II say something sharper
    than yes/no
  - the value category is preserved in that type: size() gives a prvalue
    (std::size_t), front() an lvalue (int&), take() an xvalue (int&&)
  - an ill-formed expression reports false rather than diagnosing
  - std::declval spells the value category under test explicitly - the probe for
    a const-qualified accessor and the probe for a mutable one are different
    probes, and `sized::front()` is non-const, so a const-lvalue probe over it
    would decline
*/
bool
tests_expr_probe()
{
    bool ok = true;

    D_TT_CHECK(is_detected<p_size, sized>::value);
    D_TT_CHECK(is_detected<p_front, sized>::value);
    D_TT_CHECK(is_detected<p_take, sized>::value);

    D_TT_CHECK(!is_detected<p_size, plain>::value);
    D_TT_CHECK(!is_detected<p_front, plain>::value);
    D_TT_CHECK(!is_detected<p_take, plain>::value);

    // the result type comes back, value category and all
    D_TT_CHECK((std::is_same<detected_t<p_size, sized>,
                             std::size_t>::value));
    D_TT_CHECK((std::is_same<detected_t<p_front, sized>, int&>::value));
    D_TT_CHECK((std::is_same<detected_t<p_take, sized>, int&&>::value));

    D_TT_CHECK((is_detected_exact<std::size_t, p_size, sized>::value));
    D_TT_CHECK((is_detected_convertible<std::size_t, p_size, sized>::value));
    D_TT_CHECK((!is_detected_exact<int, p_size, sized>::value));

    // an ill-formed expression is a false, not a diagnostic, for every shape in
    // the zoo
    D_TT_CHECK(!is_detected<p_size, void>::value);
    D_TT_CHECK(!is_detected<p_size, fixtures::function_type>::value);
    D_TT_CHECK(!is_detected<p_size, fixtures::incomplete>::value);
    D_TT_CHECK(!is_detected<p_size, fixtures::abstract>::value);

    return ok;
}

/*
tests_expr_probe_2
  D_TEST_EXPR_PROBE_2 - the binary expression form, over `_Type` and `_Other`.
  The shape every cross-type question wants: comparability, assignability,
  constructibility-from, conversion-to.
  Tests the following:
  - both parameters are usable in the body
  - the second argument's TYPE is what is being probed, not merely its presence:
    `sized::at(int)` is callable with an int and with a char (which converts) and
    not with a plain (which does not)
  - the result type comes back
*/
bool
tests_expr_probe_2()
{
    bool ok = true;

    D_TT_CHECK((is_detected<p_at, sized, int>::value));
    D_TT_CHECK((is_detected<p_at, sized, char>::value));   // converts to int
    D_TT_CHECK((is_detected<p_at, sized, long>::value));

    D_TT_CHECK((!is_detected<p_at, sized, plain>::value));
    D_TT_CHECK((!is_detected<p_at, plain, int>::value));
    D_TT_CHECK((!is_detected<p_at, int, int>::value));

    D_TT_CHECK((std::is_same<detected_t<p_at, sized, int>, int>::value));
    D_TT_CHECK((is_detected_exact<int, p_at, sized, int>::value));

    return ok;
}

/*
tests_noexcept_probe
  D_TEST_NOEXCEPT_PROBE - two facts in one probe, and they have to travel
  together.
  Tests the following, which are THREE distinct rows and not two:
  - the expression is ill-formed          -> the probe does not form at all
  - the expression is valid and throwing   -> the probe forms, ::value is false
  - the expression is valid and non-throwing -> the probe forms, ::value is true

  The middle row is the one a suite reaches for `sized` and `plain` alone would
  never see, and it is the row that proves the probe is reporting noexcept-ness
  rather than merely well-formedness.  throwing_swap exists for it.

  The reason the two facts cannot be separated: `noexcept(EXPR)` on an ill-formed
  EXPR is a HARD ERROR, so "is it noexcept" is only askable of an expression
  already known to be valid.  Wrapping the noexcept operator inside the alias is
  what turns that hard error into a substitution failure.
*/
bool
tests_noexcept_probe()
{
    bool ok = true;

    // row 1: ill-formed -> the probe itself does not form
    D_TT_CHECK(!is_detected<p_swap_noexcept, plain>::value);
    D_TT_CHECK(!is_detected<p_swap_noexcept, int>::value);

    // rows 2 and 3: the probe forms, and carries the answer
    D_TT_CHECK(is_detected<p_swap_noexcept, sized>::value);
    D_TT_CHECK(is_detected<p_swap_noexcept, throwing_swap>::value);

    D_TT_CHECK(detected_t<p_swap_noexcept, sized>::value);
    D_TT_CHECK(!detected_t<p_swap_noexcept, throwing_swap>::value);

    // the probe's result is a bool_constant, which is what lets
    // is_nothrow_probe derive from it (see tests_is_nothrow_probe)
    D_TT_CHECK((std::is_same<detected_t<p_swap_noexcept, sized>,
                             std::integral_constant<bool, true>>::value));
    D_TT_CHECK((std::is_same<detected_t<p_swap_noexcept, throwing_swap>,
                             std::integral_constant<bool, false>>::value));

    return ok;
}

/*
tests_constexpr_probe
  D_TEST_CONSTEXPR_PROBE - well-formed iff the expression is a CONSTANT
  expression.
  Tests the following:
  - a constant expression is detected
  - a NON-constant expression is not, and does not diagnose - the value is
    evaluated, discarded, and replaced by a literal 0 in a template-argument
    position, so it has to survive the constant evaluator to get there
  - the expression's own type is irrelevant: sizeof yields a size_t, the literal
    fixture's `.value` an int, and both are detected

  AND THE CAVEAT THE HEADER WARNS ABOUT.  The probe conflates "ill-formed" with
  "well-formed but not constant" - both are simply "not detected".  The only way
  to separate them is a D_TEST_EXPR_PROBE over the SAME expression:

      EXPR detected + CONSTEXPR detected      constant
      EXPR detected + CONSTEXPR not detected  compiles, but is not constexpr
      EXPR not detected                       ill-formed

  runtime_valued and fixtures::nonliteral are the middle row, and they are the
  only subjects in the suite that can prove the caveat is real rather than
  theoretical.
*/
bool
tests_constexpr_probe()
{
    bool ok = true;

    // detected: a constant expression
    D_TT_CHECK(is_detected<p_default_ce, fixtures::empty>::value);
    D_TT_CHECK(is_detected<p_literal_ce, fixtures::literal>::value);
    D_TT_CHECK(is_detected<p_sizeof_ce, int>::value);

    // sizeof is a constant expression even for a NON-literal type - the probe
    // is about the expression, not about the type's literalness
    D_TT_CHECK(is_detected<p_sizeof_ce, fixtures::nonliteral>::value);
    D_TT_CHECK(is_detected<p_sizeof_ce, fixtures::throwing>::value);

    // ...and ill-formed for void, which has no size
    D_TT_CHECK(!is_detected<p_sizeof_ce, fixtures::void_type>::value);

    // THE CAVEAT, row by row.
    // ill-formed: no such constructor
    D_TT_CHECK(!is_detected<p_literal_expr, fixtures::empty>::value);
    D_TT_CHECK(!is_detected<p_literal_ce, fixtures::empty>::value);

    // well-formed AND constant
    D_TT_CHECK(is_detected<p_literal_expr, fixtures::literal>::value);
    D_TT_CHECK(is_detected<p_literal_ce, fixtures::literal>::value);

    // well-formed but NOT constant - the row that needs both probes to see
    D_TT_CHECK(is_detected<p_literal_expr, runtime_valued>::value);
    D_TT_CHECK(!is_detected<p_literal_ce, runtime_valued>::value);

    D_TT_CHECK(is_detected<p_default_expr, fixtures::nonliteral>::value);
    D_TT_CHECK(!is_detected<p_default_ce, fixtures::nonliteral>::value);

    // ...stated as the three-way classification a caller actually wants
    D_TT_CHECK(is_detected<p_default_expr, fixtures::empty>::value &&
               is_detected<p_default_ce, fixtures::empty>::value);
    D_TT_CHECK(is_detected<p_default_expr, fixtures::nonliteral>::value &&
               !is_detected<p_default_ce, fixtures::nonliteral>::value);
    D_TT_CHECK(!is_detected<p_default_expr, fixtures::abstract>::value &&
               !is_detected<p_default_ce, fixtures::abstract>::value);

    return ok;
}

/*
tests_probe_scope
  Where a probe may be declared, and what its parameters are called.
  Tests the following:
  - a probe declared at CLASS scope works, and is read by the same is_detected -
    the declarators emit alias templates, and a member alias template is still a
    valid template-template argument.  The header claims "namespace or class
    scope"; this is the class half of that claim
  - the emitted probe lands in whatever namespace the macro was invoked in: the
    probes in this suite are in djinterp::testing, and are named unqualified
    throughout
  - the parameter names are `_Type` and `_Other`, and nothing else - the bodies
    of every probe in this suite are written against those names, so the suite
    compiling at all is that test

  The remaining claim - that a probe may NOT be declared inside a function - is
  not testable: a violation is a compile error, and the suite would not build to
  report it.
*/
bool
tests_probe_scope()
{
    bool ok = true;

    // class scope
    D_TT_CHECK(is_detected<probe_holder::nested_value_type, sized>::value);
    D_TT_CHECK(!is_detected<probe_holder::nested_value_type, plain>::value);
    D_TT_CHECK(!is_detected<probe_holder::nested_value_type, int>::value);

    // and it is the same probe as the namespace-scope one, on every subject
    D_TT_CHECK(is_detected<probe_holder::nested_value_type, sized>::value ==
               is_detected<p_value_type, sized>::value);
    D_TT_CHECK(is_detected<probe_holder::nested_value_type, plain>::value ==
               is_detected<p_value_type, plain>::value);
    D_TT_CHECK((std::is_same<
                    detected_t<probe_holder::nested_value_type, sized>,
                    detected_t<p_value_type, sized>>::value));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
