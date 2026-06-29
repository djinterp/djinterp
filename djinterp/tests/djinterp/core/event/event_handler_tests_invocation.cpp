/******************************************************************************
* djinterp [test]                            event_handler_tests_invocation.cpp
*
*   Section II -- VERDICT NORMALIZATION + tuple-apply.  Covers the internal
* invocation helpers: invoke_normalized (a void-returning handler is run and
* normalized to the unit verdict pass; a verdict-returning handler's verdict
* is forwarded unchanged) and apply_handler (unpacks a payload tuple and
* invokes the handler, normalizing the result).  Apply is exercised across
* arities 0..3 for both void and verdict handlers, and the payload values are
* checked to confirm they reach the handler in order.
*
*
* path:      /tests/djinterp/core/event/event_handler/event_handler_tests_invocation.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_handler_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_invoke_normalized_void
bool
tests_invoke_normalized_void()
{
    bool ok = true;

    // a void-returning handler is normalized to the unit verdict pass, and is
    // actually invoked (the side effect fires exactly once).
    int      calls = 0;
    rec_void h{&calls};

    verdict v = internal::invoke_normalized(h, 5);

    ok = D_EH_CHECK(v == verdict::pass) && ok;
    ok = D_EH_CHECK(calls == 1) && ok;

    return ok;
}


// tests_invoke_normalized_verdict
bool
tests_invoke_normalized_verdict()
{
    bool ok = true;

    // a verdict-returning handler's verdict is forwarded unchanged, for both
    // points of the verdict set.
    h_pass_unary    hp;
    h_consume_unary hc;

    ok = D_EH_CHECK(internal::invoke_normalized(hp, 1) == verdict::pass) && ok;
    ok = D_EH_CHECK(
        internal::invoke_normalized(hc, 1) == verdict::consume
    ) && ok;

    return ok;
}


// tests_apply_handler_arities
bool
tests_apply_handler_arities()
{
    bool ok = true;

    // apply_handler unpacks payload tuples of every arity 0..3 and invokes the
    // matching handler, normalizing void to pass.
    std::tuple<>                  t0;
    std::tuple<int>               t1(7);
    std::tuple<int, double>       t2(1, 2.0);
    std::tuple<int, double, char> t3(1, 2.0, 'x');

    h_void_nullary hn;
    h_void_unary   hu;
    h_void_binary  hb;
    h_void_ternary ht;

    ok = D_EH_CHECK(
        internal::apply_handler(hn, t0, internal::make_index_sequence<0>{})
            == verdict::pass
    ) && ok;
    ok = D_EH_CHECK(
        internal::apply_handler(hu, t1, internal::make_index_sequence<1>{})
            == verdict::pass
    ) && ok;
    ok = D_EH_CHECK(
        internal::apply_handler(hb, t2, internal::make_index_sequence<2>{})
            == verdict::pass
    ) && ok;
    ok = D_EH_CHECK(
        internal::apply_handler(ht, t3, internal::make_index_sequence<3>{})
            == verdict::pass
    ) && ok;

    return ok;
}


// tests_apply_handler_void_normalization
bool
tests_apply_handler_void_normalization()
{
    bool ok = true;

    std::tuple<int> t1(7);

    // a void handler routed through apply_handler is normalized to pass and is
    // actually invoked...
    int      calls = 0;
    rec_void hv{&calls};

    ok = D_EH_CHECK(
        internal::apply_handler(hv, t1, internal::make_index_sequence<1>{})
            == verdict::pass
    ) && ok;
    ok = D_EH_CHECK(calls == 1) && ok;

    // ...while a verdict handler's consume is forwarded through apply.
    h_consume_unary hc;
    ok = D_EH_CHECK(
        internal::apply_handler(hc, t1, internal::make_index_sequence<1>{})
            == verdict::consume
    ) && ok;

    return ok;
}


// tests_apply_handler_forwards_values
bool
tests_apply_handler_forwards_values()
{
    bool ok = true;

    // the payload elements must reach the handler in order and unmodified.
    int         got_one = 0;
    std::tuple<int> t1(99);
    capture_int     ci{&got_one};

    internal::apply_handler(ci, t1, internal::make_index_sequence<1>{});
    ok = D_EH_CHECK(got_one == 99) && ok;

    // a two-element payload is unpacked positionally (int then double).
    int         got_sum = 0;
    std::tuple<int, double> t2(40, 2.0);
    capture_sum             cs{&got_sum};

    internal::apply_handler(cs, t2, internal::make_index_sequence<2>{});
    ok = D_EH_CHECK(got_sum == 42) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
