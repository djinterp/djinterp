/******************************************************************************
* djinterp [test]                                event_handler_tests_monoid.cpp
*
*   Section V -- THE HANDLER MONOID (seq, skip).  Covers the monoid unit skip
* (always yields pass, accepts any arguments) and the sequencing operation
* seq: pass;pass runs both and yields pass; a consuming first handler is the
* left zero (the second is skipped and consume is returned); a void first
* handler is normalized to pass so the second still runs.  Also checks
* sequencing of more than two handlers (the fold is associative in effect),
* that arguments are routed as shared lvalues through both stages (a mutation
* in the first is visible to the second, and a payload is never moved), and
* that seq decays cv/reference-qualified handler operands.
*
*
* path:      /tests/djinterp/core/event/event_handler/event_handler_tests_monoid.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_handler_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_skip_always_pass
bool
tests_skip_always_pass()
{
    bool ok = true;

    // the unit handler ignores its arguments and always yields pass, for any
    // argument list...
    skip_t s;
    ok = D_EH_CHECK(s() == verdict::pass) && ok;
    ok = D_EH_CHECK(s(1, 2.0, 'z') == verdict::pass) && ok;

    // ...and the skip() factory returns such a unit.
    ok = D_EH_CHECK(skip()(42) == verdict::pass) && ok;

    return ok;
}


// tests_seq_pass_pass
bool
tests_seq_pass_pass()
{
    bool ok = true;

    // pass ; pass folds to pass, and both handlers run exactly once.
    int x = 0;
    int y = 0;

    auto s = seq(rec_pass{&x}, rec_pass{&y});
    verdict v = s(5);

    ok = D_EH_CHECK(v == verdict::pass) && ok;
    ok = D_EH_CHECK(x == 1) && ok;
    ok = D_EH_CHECK(y == 1) && ok;

    return ok;
}


// tests_seq_left_zero
bool
tests_seq_left_zero()
{
    bool ok = true;

    // a consuming first handler is the left zero: it short-circuits, the
    // second handler never runs, and the fold returns consume.
    int x = 0;
    int y = 0;

    auto s = seq(rec_consume{&x}, rec_pass{&y});
    verdict v = s(5);

    ok = D_EH_CHECK(v == verdict::consume) && ok;
    ok = D_EH_CHECK(x == 1) && ok;
    ok = D_EH_CHECK(y == 0) && ok;

    return ok;
}


// tests_seq_void_normalization
bool
tests_seq_void_normalization()
{
    bool ok = true;

    // a void first handler is normalized to pass, so the second still runs;
    // here the second consumes, so the fold returns consume.
    {
        int x = 0;
        int y = 0;

        auto s = seq(rec_void{&x}, rec_consume{&y});
        verdict v = s(5);

        ok = D_EH_CHECK(v == verdict::consume) && ok;
        ok = D_EH_CHECK(x == 1) && ok;
        ok = D_EH_CHECK(y == 1) && ok;
    }

    // void ; void folds to pass with both stages run.
    {
        int x = 0;
        int y = 0;

        auto s = seq(rec_void{&x}, rec_void{&y});
        verdict v = s(5);

        ok = D_EH_CHECK(v == verdict::pass) && ok;
        ok = D_EH_CHECK(x == 1) && ok;
        ok = D_EH_CHECK(y == 1) && ok;
    }

    return ok;
}


// tests_seq_associativity
bool
tests_seq_associativity()
{
    bool ok = true;

    // sequencing more than two handlers folds left-to-right; with no consume
    // until the last stage, every stage runs and the final verdict is the
    // last one. (seq(seq(a, b), c) and seq(a, seq(b, c)) agree in effect.)
    {
        int x = 0;
        int y = 0;
        int z = 0;

        auto left = seq(seq(rec_pass{&x}, rec_pass{&y}), rec_consume{&z});
        verdict v = left(5);

        ok = D_EH_CHECK(v == verdict::consume) && ok;
        ok = D_EH_CHECK(x == 1 && y == 1 && z == 1) && ok;
    }
    {
        int x = 0;
        int y = 0;
        int z = 0;

        auto right = seq(rec_pass{&x}, seq(rec_pass{&y}, rec_consume{&z}));
        verdict v = right(5);

        ok = D_EH_CHECK(v == verdict::consume) && ok;
        ok = D_EH_CHECK(x == 1 && y == 1 && z == 1) && ok;
    }

    // a consume in the middle stops the right-most stage under either nesting.
    {
        int x = 0;
        int y = 0;
        int z = 0;

        auto s = seq(rec_pass{&x}, seq(rec_consume{&y}, rec_pass{&z}));
        verdict v = s(5);

        ok = D_EH_CHECK(v == verdict::consume) && ok;
        ok = D_EH_CHECK(x == 1 && y == 1 && z == 0) && ok;
    }

    return ok;
}


// tests_seq_lvalue_passthrough
bool
tests_seq_lvalue_passthrough()
{
    bool ok = true;

    // the same argument lvalue is routed through both stages: a mutation made
    // by the first handler is visible to the second.
    {
        int x = 1;

        auto s = seq(add_one_ref{}, expect_two_ref{});
        verdict v = s(x);

        ok = D_EH_CHECK(v == verdict::consume) && ok;
        ok = D_EH_CHECK(x == 2) && ok;
    }

    // the shared argument is never moved across the two stages.
    {
        int moves = 0;
        move_probe p(&moves);

        auto s = seq(mp_pass{}, mp_pass{});
        verdict v = s(p);

        ok = D_EH_CHECK(v == verdict::pass) && ok;
        ok = D_EH_CHECK(moves == 0) && ok;
    }

    return ok;
}


// tests_seq_clean_type
bool
tests_seq_clean_type()
{
    bool ok = true;

    // seq decays its handler operands (clean_t), so a const lvalue and an
    // rvalue compose just as well as plain values.
    const h_pass_unary cf;

    auto s = seq(cf, h_consume_unary{});
    verdict v = s(1);

    ok = D_EH_CHECK(v == verdict::consume) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
