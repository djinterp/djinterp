// djinterp
#include "test_traits_tests.hpp"

// std
#include <memory>
#include <type_traits>
#include <utility>


NS_DJINTERP
NS_TESTING


/*
tests_is_nothrow_probe
  is_nothrow_probe - folds a D_TEST_NOEXCEPT_PROBE's two facts into the single
  bool a test wants: VALID and NON-THROWING.
  Tests the following:
  - valid and non-throwing  -> true
  - valid and throwing      -> false
  - ill-formed              -> false, NOT a diagnostic

  The third row is what the trait is for.  It is spelled

      struct is_nothrow_probe : detected_or_t<std::false_type, _Probe, _Types...>

  so it derives from the probe's RESULT when the probe forms, and from
  std::false_type when it does not.  That is why the ill-formed row comes back
  false instead of exploding, and it is also why the trait is a bool trait: its
  base is a bool_constant either way.

  NOTE what it does NOT check: that the probe handed to it IS a noexcept probe.
  Hand it a probe whose detection succeeds and yields a non-class type - an EXPR
  probe over a size(), say - and the trait becomes `struct : std::size_t`, which
  is a hard error.  Pinned in tests_build_time_hazards.
*/
bool
tests_is_nothrow_probe()
{
    bool ok = true;

    D_TT_CHECK(is_nothrow_probe<p_swap_noexcept, sized>::value);
    D_TT_CHECK(!is_nothrow_probe<p_swap_noexcept, throwing_swap>::value);
    D_TT_CHECK(!is_nothrow_probe<p_swap_noexcept, plain>::value);
    D_TT_CHECK(!is_nothrow_probe<p_swap_noexcept, int>::value);

    // the three rows are genuinely three: the middle one FORMS and answers
    // false, the last one does not form at all
    D_TT_CHECK(is_detected<p_swap_noexcept, throwing_swap>::value &&
               !is_nothrow_probe<p_swap_noexcept, throwing_swap>::value);
    D_TT_CHECK(!is_detected<p_swap_noexcept, plain>::value &&
               !is_nothrow_probe<p_swap_noexcept, plain>::value);

    // the fixtures that exist for exactly this question
    D_TT_CHECK(std::is_nothrow_default_constructible<
                   fixtures::nothrowing>::value);
    D_TT_CHECK(!std::is_nothrow_default_constructible<
                   fixtures::throwing>::value);

    // and the trait is itself a bool trait, in all three rows
    D_TT_CHECK(is_bool_trait<is_nothrow_probe<p_swap_noexcept,
                                              sized>>::value);
    D_TT_CHECK(is_bool_trait<is_nothrow_probe<p_swap_noexcept,
                                              throwing_swap>>::value);
    D_TT_CHECK(is_bool_trait<is_nothrow_probe<p_swap_noexcept,
                                              plain>>::value);

    return ok;
}

/*
tests_yields_lvalue
  yields_lvalue - is the probe's result an lvalue, i.e. is decltype(EXPR) an `X&`?
  Tests the following:
  - an accessor returning a reference is an lvalue; one returning a copy is not
  - an accessor returning an rvalue reference is not an lvalue either
  - an ill-formed probe is false, because detected_t yields nonesuch and nonesuch
    is not a reference

  The question every accessor test asks and no plain detection trait answers:
  `front()` returning a reference and `front()` returning a copy are BOTH
  "detected".  Only the value category separates them, and only this trait reads
  it.
*/
bool
tests_yields_lvalue()
{
    bool ok = true;

    D_TT_CHECK(yields_lvalue<p_front, sized>::value);    // int&

    D_TT_CHECK(!yields_lvalue<p_size, sized>::value);    // std::size_t (prvalue)
    D_TT_CHECK(!yields_lvalue<p_take, sized>::value);    // int&& (xvalue)
    D_TT_CHECK(!yields_lvalue<p_front, plain>::value);   // ill-formed

    // exactly one of the three categories holds for any well-formed probe
    D_TT_CHECK(yields_lvalue<p_front, sized>::value &&
               !yields_xvalue<p_front, sized>::value &&
               !yields_prvalue<p_front, sized>::value);

    return ok;
}

/*
tests_yields_xvalue
  yields_xvalue - is the probe's result an xvalue, i.e. an `X&&`?
  Tests the following:
  - an accessor returning an rvalue reference is an xvalue
  - a reference-returning and a copy-returning accessor are not
  - an ill-formed probe is false
*/
bool
tests_yields_xvalue()
{
    bool ok = true;

    D_TT_CHECK(yields_xvalue<p_take, sized>::value);     // int&&

    D_TT_CHECK(!yields_xvalue<p_front, sized>::value);   // int& (lvalue)
    D_TT_CHECK(!yields_xvalue<p_size, sized>::value);    // prvalue
    D_TT_CHECK(!yields_xvalue<p_take, plain>::value);    // ill-formed

    D_TT_CHECK(!yields_lvalue<p_take, sized>::value &&
               yields_xvalue<p_take, sized>::value &&
               !yields_prvalue<p_take, sized>::value);

    return ok;
}

/*
tests_yields_prvalue
  yields_prvalue - is the probe's result a prvalue, i.e. a NON-reference?

  THE ONE READING THAT CANNOT BE WRITTEN NAIVELY.  The obvious spelling is

      !std::is_reference<detected_t<_Probe, _Types...>>::value

  and it is WRONG, because an ill-formed probe makes detected_t yield `nonesuch`
  - which is not a reference either.  The naive trait therefore reports every
  ill-formed expression as a prvalue: the single most dangerous kind of wrong
  answer a trait test can give, because it is a false POSITIVE on the negative
  case, which is the case the test existed to check.

  The header guards it with an explicit `is_detected && !is_reference`.  The
  checks below assert the guarded answer AND, side by side, compute the naive
  one - so that if the guard is ever removed, this test does not merely fail, it
  fails while printing the wrong answer next to the right one.

  Tests the following:
  - a copy-returning accessor is a prvalue
  - a reference-returning one is not, either kind
  - an ILL-FORMED probe is not a prvalue - and the naive spelling says it is
*/
bool
tests_yields_prvalue()
{
    bool ok = true;

    D_TT_CHECK(yields_prvalue<p_size, sized>::value);     // std::size_t

    D_TT_CHECK(!yields_prvalue<p_front, sized>::value);   // int&
    D_TT_CHECK(!yields_prvalue<p_take, sized>::value);    // int&&

    // THE GUARD.  Ill-formed -> not a prvalue...
    D_TT_CHECK(!yields_prvalue<p_size, plain>::value);
    D_TT_CHECK(!yields_prvalue<p_front, plain>::value);
    D_TT_CHECK(!yields_prvalue<p_size, void>::value);

    // ...and here is what the unguarded spelling would have said about the very
    // same probe.  If this line ever stops being true, nonesuch has become a
    // reference and the guard is no longer load-bearing; if the line ABOVE ever
    // stops being true, the guard is gone
    D_TT_CHECK((!std::is_reference<detected_t<p_size, plain>>::value));
    D_TT_CHECK((std::is_same<detected_t<p_size, plain>, nonesuch>::value));

    // the three categories partition the well-formed probes and are all false
    // for the ill-formed ones
    D_TT_CHECK(!yields_lvalue<p_size, plain>::value &&
               !yields_xvalue<p_size, plain>::value &&
               !yields_prvalue<p_size, plain>::value);

    return ok;
}

/*
tests_is_valid
  is_valid - the INLINE spelling of a probe.  Hand it a generic lambda whose
  trailing return type names the expression under test, and it answers on the
  spot, with no alias template declared and no name added to the namespace.
  Tests the following:
  - a nullary probe (the lambda takes nothing, `_Args...` is empty)
  - a unary probe
  - a binary probe
  - the negative case in each arity
  - the lambda is never INVOKED - only its declaration is considered - so an
    empty body is correct even for a lambda whose trailing return type mentions
    an expression that could not be evaluated
  - it is constexpr: the result is usable in a constant expression, which is the
    only way to prove the whole dispatch happened at compile time
*/
bool
tests_is_valid()
{
    bool ok = true;

    // nullary
    D_TT_CHECK(is_valid<>([]() -> decltype(void(0)) {}));
    D_TT_CHECK(!is_valid<>([](auto&& _x) -> decltype(void(_x)) {}));

    // unary
    D_TT_CHECK(is_valid<sized&>(
                   [](auto&& _x) -> decltype(void(_x.size())) {}));
    D_TT_CHECK(!is_valid<plain&>(
                   [](auto&& _x) -> decltype(void(_x.size())) {}));
    D_TT_CHECK(is_valid<sized&>(
                   [](auto&& _x) -> decltype(void(_x.front())) {}));
    D_TT_CHECK(!is_valid<int&>(
                   [](auto&& _x) -> decltype(void(_x.size())) {}));

    // binary
    D_TT_CHECK((is_valid<sized&, int>(
                    [](auto&& _x, auto&& _i) -> decltype(void(_x.at(_i))) {})));
    D_TT_CHECK((!is_valid<plain&, int>(
                    [](auto&& _x, auto&& _i) -> decltype(void(_x.at(_i))) {})));
    D_TT_CHECK((!is_valid<sized&, plain>(
                    [](auto&& _x, auto&& _i) -> decltype(void(_x.at(_i))) {})));

    // it agrees with the declared probe over the same expression - the inline
    // and the declared spellings are two ways of asking one question
    D_TT_CHECK(is_valid<sized&>(
                   [](auto&& _x) -> decltype(void(_x.size())) {}) ==
               is_detected<p_size, sized>::value);
    D_TT_CHECK(is_valid<plain&>(
                   [](auto&& _x) -> decltype(void(_x.size())) {}) ==
               is_detected<p_size, plain>::value);

    // constexpr: the whole dispatch is a constant expression
    D_CONSTEXPR bool inline_yes = is_valid<sized&>(
        [](auto&& _x) -> decltype(void(_x.size())) {});
    D_CONSTEXPR bool inline_no = is_valid<plain&>(
        [](auto&& _x) -> decltype(void(_x.size())) {});

    D_TT_CHECK(inline_yes);
    D_TT_CHECK(!inline_no);

    return ok;
}

/*
tests_is_valid_evil
  is_valid's dispatch runs the probed call through `void(...)` rather than a bare
  comma, and the trailing return type of every probe lambda is documented as
  `decltype(void( EXPR ))` for the same reason.  fixtures::evil is what that
  reason looks like.

  evil overloads `operator,` as a template, so a bare comma with an evil operand
  is not the built-in comma at all - it is a call, and it changes the type of the
  whole expression.  It also overloads unary `operator&`, so `&x` does not yield
  a pointer to x.

  Tests the following:
  - a bare comma over an evil operand IS hijacked - the result type is `evil`,
    not the type of the right-hand operand
  - `void(...)` around the left operand defeats the hijack: the built-in comma is
    restored and the result is the right-hand operand's type
  - `&evil` yields `void*`, not `evil*`; std::addressof yields `evil*`
  - is_valid over an evil subject is not fooled: it reports FALSE for a member
    the type does not have, rather than being talked into true by the operator

  The last one is the check that matters.  The first three establish that the
  hijacks are live; the fourth establishes that the header's guard holds against
  them.
*/
bool
tests_is_valid_evil()
{
    bool ok = true;

    using evil = fixtures::evil;

    // the hijacks are live
    D_TT_CHECK((std::is_same<decltype((std::declval<evil&>(), 0)),
                             evil>::value));
    D_TT_CHECK((std::is_same<decltype(&std::declval<evil&>()),
                             void*>::value));

    // ...and the guards defeat them
    D_TT_CHECK((std::is_same<decltype((void(std::declval<evil&>()), 0)),
                             int>::value));
    D_TT_CHECK((std::is_same<
                    decltype(std::addressof(std::declval<evil&>())),
                    evil*>::value));

    // is_valid is not fooled: evil has no size(), and the answer is false
    D_TT_CHECK(!is_valid<evil&>(
                   [](auto&& _x) -> decltype(void(_x.size())) {}));
    D_TT_CHECK(!is_valid<evil&>(
                   [](auto&& _x) -> decltype(void(_x.value_type_that_is_not_there)) {}));

    // nor are the declared probes
    D_TT_CHECK(!is_detected<p_size, evil>::value);
    D_TT_CHECK(!is_detected<p_value_type, evil>::value);
    D_TT_CHECK(!yields_prvalue<p_size, evil>::value);

    return ok;
}

/*
tests_reading_value_companions
  The four `_v` companions of section II.
  Tests the following:
  - is_nothrow_probe_v, yields_lvalue_v, yields_xvalue_v and yields_prvalue_v
    each carry the same value as their trait, on a positive and on a negative
    subject
  - each is typed const bool
*/
bool
tests_reading_value_companions()
{
    bool ok = true;

    D_TT_CHECK((is_nothrow_probe_v<p_swap_noexcept, sized>) ==
               (is_nothrow_probe<p_swap_noexcept, sized>::value));
    D_TT_CHECK((is_nothrow_probe_v<p_swap_noexcept, throwing_swap>) ==
               (is_nothrow_probe<p_swap_noexcept, throwing_swap>::value));
    D_TT_CHECK((is_nothrow_probe_v<p_swap_noexcept, plain>) ==
               (is_nothrow_probe<p_swap_noexcept, plain>::value));

    D_TT_CHECK((yields_lvalue_v<p_front, sized>) ==
               (yields_lvalue<p_front, sized>::value));
    D_TT_CHECK((yields_lvalue_v<p_front, plain>) ==
               (yields_lvalue<p_front, plain>::value));

    D_TT_CHECK((yields_xvalue_v<p_take, sized>) ==
               (yields_xvalue<p_take, sized>::value));
    D_TT_CHECK((yields_xvalue_v<p_take, plain>) ==
               (yields_xvalue<p_take, plain>::value));

    D_TT_CHECK((yields_prvalue_v<p_size, sized>) ==
               (yields_prvalue<p_size, sized>::value));
    D_TT_CHECK((yields_prvalue_v<p_size, plain>) ==
               (yields_prvalue<p_size, plain>::value));

    // the shorthands really are the values
    D_TT_CHECK((is_nothrow_probe_v<p_swap_noexcept, sized>));
    D_TT_CHECK((yields_lvalue_v<p_front, sized>));
    D_TT_CHECK((yields_xvalue_v<p_take, sized>));
    D_TT_CHECK((yields_prvalue_v<p_size, sized>));

    D_TT_CHECK((std::is_same<decltype(yields_prvalue_v<p_size, sized>),
                             const bool>::value));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
