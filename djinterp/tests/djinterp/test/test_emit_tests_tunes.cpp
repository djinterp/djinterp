/*******************************************************************************
* djinterp [test]                                      test_emit_tests_tunes.cpp
*
*   Section I - option transforms.  A tune is an option_set endomorphism; the
* builders (width / color / compression / archiving) each set one slot and
* return the set, and compose chains two - left then right - treating a null
* operand as the identity.  width's effect is asserted end-to-end through the
* portable free accessor (line_width(opts)); the other builders are exercised
* for well-formedness (non-null and invocable, which runs their bodies), since
* their aggregate slots have no scalar read surface here.  compose's order and
* null-safety are pinned with custom marker tunes that record their invocation
* through external counters rather than touching the option set.  with() (the
* stream-scope origin) is the identity wrapper.
*
*
* path:      /tests/djinterp/test/test_emit_tests_tunes.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_emit_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_tune_width_effect
//   tune::width folds the requested line_width onto the option set (read back
// through the portable accessor).
bool
tests_tune_width_effect()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::test_option_set _r = dt::tune::width(120)(dt::default_test_options());
    ok = D_TE_CHECK(dt::line_width(_r) == std::size_t(120)) && ok;

    // a different width lands a different value (not a constant)
    dt::test_option_set _r2 = dt::tune::width(40)(dt::default_test_options());
    ok = D_TE_CHECK(dt::line_width(_r2) == std::size_t(40)) && ok;

#endif
    return ok;
}

// tests_tune_builders_wellformed
//   every tune builder returns a non-null, invocable option transform (this
// also executes each builder's slot-setting body).
bool
tests_tune_builders_wellformed()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::option_tune _w = dt::tune::width(90);
    dt::option_tune _c = dt::tune::color(false);
    ::djinterp::compress_options _co;
    ::djinterp::archive_options  _ao;
    dt::option_tune _z = dt::tune::compression(_co);
    dt::option_tune _a = dt::tune::archiving(_ao);

    ok = D_TE_CHECK(static_cast<bool>(_w)) && ok;
    ok = D_TE_CHECK(static_cast<bool>(_c)) && ok;
    ok = D_TE_CHECK(static_cast<bool>(_z)) && ok;
    ok = D_TE_CHECK(static_cast<bool>(_a)) && ok;

    // invoke each (covers the lambda bodies); must not throw / must round-trip
    dt::test_option_set _base = dt::default_test_options();
    dt::test_option_set _r1 = _w(_base);
    dt::test_option_set _r2 = _c(_base);
    dt::test_option_set _r3 = _z(_base);
    dt::test_option_set _r4 = _a(_base);
    ok = D_TE_CHECK(dt::line_width(_r1) == std::size_t(90)) && ok;   // width still verifiable
    (void)_r2; (void)_r3; (void)_r4;

#endif
    return ok;
}

// tests_tune_compose_order
//   compose(a, b) applies a first, then b.
bool
tests_tune_compose_order()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    int _order = 0;
    int _a_ran = 0;
    int _b_ran = 0;

    dt::option_tune _a = [&_order, &_a_ran](dt::test_option_set _o) -> dt::test_option_set
                         { _a_ran = ++_order; return _o; };
    dt::option_tune _b = [&_order, &_b_ran](dt::test_option_set _o) -> dt::test_option_set
                         { _b_ran = ++_order; return _o; };

    dt::option_tune _ab = dt::tune::compose(_a, _b);
    dt::test_option_set _r = _ab(dt::default_test_options());
    (void)_r;

    ok = D_TE_CHECK(_a_ran == 1) && ok;    // a ran first
    ok = D_TE_CHECK(_b_ran == 2) && ok;    // then b

#endif
    return ok;
}

// tests_tune_compose_null_safety
//   compose treats a null operand as the identity: the present tune still runs,
// the null one is skipped, and the composite stays invocable.
bool
tests_tune_compose_null_safety()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    int _ran = 0;
    dt::option_tune _present = [&_ran](dt::test_option_set _o) -> dt::test_option_set
                               { ++_ran; return _o; };
    dt::option_tune _null;   // empty std::function

    // null left operand -> identity then present
    dt::test_option_set _r1 = dt::tune::compose(_null, _present)(dt::default_test_options());
    ok = D_TE_CHECK(_ran == 1) && ok;
    (void)_r1;

    // null right operand -> present then identity
    dt::test_option_set _r2 = dt::tune::compose(_present, _null)(dt::default_test_options());
    ok = D_TE_CHECK(_ran == 2) && ok;
    (void)_r2;

    // both null -> pure identity, still invocable, still passes width through
    dt::option_tune _both = dt::tune::compose(_null, _null);
    ok = D_TE_CHECK(static_cast<bool>(_both)) && ok;      // compose always yields a callable
    dt::test_option_set _r3 = _both(dt::tune::width(55)(dt::default_test_options()));
    ok = D_TE_CHECK(dt::line_width(_r3) == std::size_t(55)) && ok;  // unchanged by identity

#endif
    return ok;
}

// tests_tune_with_origin_identity
//   with(tune) returns the same transform (the stream-scope wrapper is the
// identity on tunes).
bool
tests_tune_with_origin_identity()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::option_tune _w = dt::tune::width(77);
    dt::option_tune _wrapped = dt::with(_w);

    ok = D_TE_CHECK(static_cast<bool>(_wrapped)) && ok;
    // same behaviour as the wrapped tune
    dt::test_option_set _r = _wrapped(dt::default_test_options());
    ok = D_TE_CHECK(dt::line_width(_r) == std::size_t(77)) && ok;

#endif
    return ok;
}


NS_END  // testing
NS_END  // djinterp
