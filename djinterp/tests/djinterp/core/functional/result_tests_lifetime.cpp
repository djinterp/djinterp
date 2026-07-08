/******************************************************************************
* djinterp [test]                                    result_tests_lifetime.cpp
*
* Lifetime, value/error access tests for result<T, E> (section I: the
* primitive itself).
*
*   test_construction -- ok/err copy & move constructors, plus copy/move of
*                        each branch.
*   test_assignment   -- copy & move assignment across same-branch and
*                        branch-changing transitions, plus self-assignment.
*   test_observers    -- is_ok / is_err / operator bool, value() and error()
*                        in their const / mutable / rvalue overloads, value_or,
*                        unwrap (incl. the throwing path).
*   test_lifetime     -- balanced construction / destruction of the active
*                        branch (value side and error side) via an
*                        instance-counting payload.
******************************************************************************/

#include <string>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


// definitions of the tracked payload's static counters (declared in header).
int tracked::live        = 0;
int tracked::constructed = 0;
int tracked::destroyed   = 0;


using ri = result<int, std::string>;


// the default constructor is intentionally deleted: a result is always
// explicitly an ok or an err, never a default "empty" state.
static_assert(!std::is_default_constructible<ri>::value,
    "result: default construction is deleted");


void test_construction(test::test_handler& _h)
{
    // deleted default constructor (runtime mirror of the file-scope assert)
    test::record_assertion(_h, !std::is_default_constructible<ri>::value,
        "construct: default construction is deleted");

    // ok via copy
    int value = 5;
    ri  a = ok<int, std::string>(value);
    test::record_assertion(_h, a.is_ok() && a.value() == 5,
        "construct: ok holds the value");

    // ok via move
    ri b = ok<int, std::string>(7);
    test::record_assertion(_h, b.is_ok() && b.value() == 7,
        "construct: ok (move) holds the value");

    // err via copy
    std::string msg = "bad";
    ri          c = err<int, std::string>(msg);
    test::record_assertion(_h, c.is_err() && c.error() == "bad",
        "construct: err holds the error");

    // err via move
    ri d = err<int, std::string>(std::string("worse"));
    test::record_assertion(_h, d.is_err() && d.error() == "worse",
        "construct: err (move) holds the error");

    // copy constructor (ok branch)
    ri a_copy(a);
    test::record_assertion(_h, a_copy.is_ok() && a_copy.value() == 5,
        "construct: copy of ok preserves value");

    // copy constructor (err branch)
    ri c_copy(c);
    test::record_assertion(_h, c_copy.is_err() && c_copy.error() == "bad",
        "construct: copy of err preserves error");

    // move constructor (ok branch)
    ri a_move(std::move(a_copy));
    test::record_assertion(_h, a_move.is_ok() && a_move.value() == 5,
        "construct: move of ok preserves value");

    // move constructor (err branch)
    ri c_move(std::move(c_copy));
    test::record_assertion(_h, c_move.is_err() && c_move.error() == "bad",
        "construct: move of err preserves error");

    return;
}


void test_assignment(test::test_handler& _h)
{
    // same-branch copy assign (ok = ok): in-place value assignment
    ri a = ok<int, std::string>(1);
    ri b = ok<int, std::string>(2);
    a = b;
    test::record_assertion(_h, a.is_ok() && a.value() == 2,
        "assign: ok = ok updates value in place");

    // same-branch copy assign (err = err)
    ri e1 = err<int, std::string>("x");
    ri e2 = err<int, std::string>("y");
    e1 = e2;
    test::record_assertion(_h, e1.is_err() && e1.error() == "y",
        "assign: err = err updates error in place");

    // branch-changing copy assign (ok = err): destroy value, build error
    ri okv = ok<int, std::string>(9);
    ri errv = err<int, std::string>("flip");
    okv = errv;
    test::record_assertion(_h, okv.is_err() && okv.error() == "flip",
        "assign: ok = err switches branch to err");

    // branch-changing copy assign (err = ok)
    ri back = err<int, std::string>("z");
    ri okw  = ok<int, std::string>(42);
    back = okw;
    test::record_assertion(_h, back.is_ok() && back.value() == 42,
        "assign: err = ok switches branch to ok");

    // self copy-assignment
    ri  s = ok<int, std::string>(11);
    ri& sref = s;
    s = sref;
    test::record_assertion(_h, s.is_ok() && s.value() == 11,
        "assign: self copy-assignment preserves state");

    // same-branch move assign
    ri m1 = ok<int, std::string>(3);
    ri m2 = ok<int, std::string>(8);
    m1 = std::move(m2);
    test::record_assertion(_h, m1.is_ok() && m1.value() == 8,
        "assign: move ok = ok updates value");

    // branch-changing move assign (ok = err)
    ri mok = ok<int, std::string>(1);
    ri merr = err<int, std::string>("moved");
    mok = std::move(merr);
    test::record_assertion(_h, mok.is_err() && mok.error() == "moved",
        "assign: move ok = err switches branch");

    // self move-assignment
    ri  sm = err<int, std::string>("keep");
    ri& smref = sm;
    sm = std::move(smref);
    test::record_assertion(_h, sm.is_err() && sm.error() == "keep",
        "assign: self move-assignment preserves state");

    return;
}


void test_observers(test::test_handler& _h)
{
    ri okv  = ok<int, std::string>(4);
    ri errv = err<int, std::string>("nope");

    // is_ok / is_err / operator bool
    test::record_assertion(_h, okv.is_ok() && !okv.is_err(),
        "observe: ok -> is_ok, not is_err");
    test::record_assertion(_h, errv.is_err() && !errv.is_ok(),
        "observe: err -> is_err, not is_ok");
    test::record_assertion(_h,
        static_cast<bool>(okv) && !static_cast<bool>(errv),
        "observe: operator bool tracks ok-ness");

    // value() const
    const ri& cokv = okv;
    test::record_assertion(_h, cokv.value() == 4,
        "observe: const value() reads the value");

    // value() mutable write-through
    okv.value() = 6;
    test::record_assertion(_h, okv.value() == 6,
        "observe: mutable value() permits write-through");

    // value() rvalue overload
    test::record_assertion(_h, ok<int, std::string>(10).value() == 10,
        "observe: rvalue value() yields the value");

    // error() const
    const ri& cerrv = errv;
    test::record_assertion(_h, cerrv.error() == "nope",
        "observe: const error() reads the error");

    // error() mutable write-through
    errv.error() = "changed";
    test::record_assertion(_h, errv.error() == "changed",
        "observe: mutable error() permits write-through");

    // value_or
    test::record_assertion(_h, okv.value_or(-1) == 6,
        "observe: value_or returns value when ok");
    test::record_assertion(_h, errv.value_or(-1) == -1,
        "observe: value_or returns default when err");

    // unwrap (ok path)
    test::record_assertion(_h, okv.unwrap("should not throw") == 6,
        "observe: unwrap returns value when ok");

    // unwrap (err path -> throws with message)
    bool        threw = false;
    std::string what;
    try
    {
        (void)errv.unwrap("boom");
    }
    catch (const std::runtime_error& _ex)
    {
        threw = true;
        what  = _ex.what();
    }
    test::record_assertion(_h, threw && what == "boom",
        "observe: unwrap throws runtime_error with message when err");

    return;
}


void test_lifetime(test::test_handler& _h)
{
    // ---- value branch ----
    tracked::reset_counters();
    {
        result<tracked, int> r = ok<tracked, int>(tracked(1));
        test::record_assertion(_h, r.is_ok() && tracked::live >= 1,
            "lifetime: ok branch holds a live instance");

        // branch-changing assignment destroys the tracked value
        result<tracked, int> e = err<tracked, int>(7);
        r = e;
        test::record_assertion(_h, r.is_err() && r.error() == 7,
            "lifetime: ok -> err assignment switches branch");
    }
    test::record_assertion(_h,
        tracked::live == 0 && tracked::constructed == tracked::destroyed,
        "lifetime: value-branch construction/destruction balances");

    // ---- error branch ----
    tracked::reset_counters();
    {
        result<int, tracked> r = err<int, tracked>(tracked(2));
        test::record_assertion(_h, r.is_err() && tracked::live >= 1,
            "lifetime: err branch holds a live instance");

        result<int, tracked> copy(r);
        test::record_assertion(_h, copy.is_err() && copy.error().id == 2,
            "lifetime: copy duplicates the error payload");
    }
    test::record_assertion(_h,
        tracked::live == 0 && tracked::constructed == tracked::destroyed
        && tracked::constructed > 0,
        "lifetime: error-branch construction/destruction balances");

    return;
}


NS_END  // testing
NS_END  // djinterp
