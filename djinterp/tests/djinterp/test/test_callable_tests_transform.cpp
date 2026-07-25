/******************************************************************************
* djinterp [test]                           test_callable_tests_transform.cpp
*
*   Row rewriting.  transform replaces the thunk bound to an id with the result
* of a mapper applied to it - the general knob behind wrapping a row (negate,
* memoize, fold a side effect).  Pinned here: that the mapper's result becomes
* the row (a negating mapper flips invoke); that the mapper actually RECEIVES
* the row's current thunk (a mapper that calls its argument twice doubles a
* counter); that transform is guarded by has(), so an unbound / sentinel id, or
* an EMPTY row, leaves the mapper uncalled - the pointed contrast with
* compose_with, which binds onto an empty row; and that a mapper returning an
* empty thunk DISABLES the row (has() goes false, invoke reads false) while the
* row itself remains counted.
*
* path:      /tests/djinterp/test/test_callable/test_callable_tests_transform.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_callable_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_transform_wraps_existing
//   the mapper's returned thunk replaces the row: a negating mapper flips the
// result; an AND-with-false mapper forces a failure.
bool
tests_transform_wraps_existing()
{
    bool ok = true;

    // negate a true row -> false
    {
        dt::test_callable_table tbl;
        dt::test_callable_id id = tbl.add([]{ return true; });
        tbl.transform(id, [](thunk_type t) -> thunk_type {
            return thunk_type([t]() -> bool { return !t(); });
        });
        ok = D_CALL_CHECK(!tbl.invoke(id)) && ok;
    }
    // negate a false row -> true
    {
        dt::test_callable_table tbl;
        dt::test_callable_id id = tbl.add([]{ return false; });
        tbl.transform(id, [](thunk_type t) -> thunk_type {
            return thunk_type([t]() -> bool { return !t(); });
        });
        ok = D_CALL_CHECK(tbl.invoke(id))  && ok;
    }
    // wrap: conjoin a constant false -> always false
    {
        dt::test_callable_table tbl;
        dt::test_callable_id id = tbl.add([]{ return true; });
        tbl.transform(id, [](thunk_type t) -> thunk_type {
            return thunk_type([t]() -> bool { return t() && false; });
        });
        ok = D_CALL_CHECK(!tbl.invoke(id)) && ok;
    }

    return ok;
}

// tests_transform_receives_current_thunk
//   the mapper is handed the row's CURRENT thunk: a mapper that calls its
// argument twice advances the underlying counter by two per invoke, proving it
// received the real thunk rather than a fresh or empty one.
bool
tests_transform_receives_current_thunk()
{
    bool ok = true;

    dt::test_callable_table tbl;
    int calls = 0;
    dt::test_callable_id id = tbl.add(counting_thunk{true, &calls});

    tbl.transform(id, [](thunk_type t) -> thunk_type {
        return thunk_type([t]() -> bool { return t() && t(); });   // inner called twice
    });

    ok = D_CALL_CHECK(tbl.invoke(id))  && ok;   // true && true
    ok = D_CALL_CHECK(calls == 2)      && ok;   // the current thunk ran twice

    ok = D_CALL_CHECK(tbl.invoke(id))  && ok;   // re-run -> two more
    ok = D_CALL_CHECK(calls == 4)      && ok;

    return ok;
}

// tests_transform_unbound_is_noop
//   transform is guarded by has(): an id that names no live row leaves the
// mapper uncalled and the table untouched.
bool
tests_transform_unbound_is_noop()
{
    bool ok = true;

    // sentinel: mapper never runs, nothing added
    {
        dt::test_callable_table tbl;
        int mapper_calls = 0;
        tbl.transform(dt::k_no_callable,
                      [&mapper_calls](thunk_type t) -> thunk_type {
                          ++mapper_calls;
                          return t;
                      });
        ok = D_CALL_CHECK(mapper_calls == 0) && ok;
        ok = D_CALL_CHECK(tbl.empty())       && ok;
    }
    // out-of-range: mapper never runs, existing row intact
    {
        dt::test_callable_table tbl;
        dt::test_callable_id id = tbl.add([]{ return true; });   // id 1
        int mapper_calls = 0;
        tbl.transform(dt::test_callable_id(7),
                      [&mapper_calls](thunk_type t) -> thunk_type {
                          ++mapper_calls;
                          return t;
                      });
        ok = D_CALL_CHECK(mapper_calls == 0)            && ok;
        ok = D_CALL_CHECK(tbl.size() == std::size_t(1)) && ok;
        ok = D_CALL_CHECK(tbl.invoke(id))               && ok;
    }

    return ok;
}

// tests_transform_empty_row_is_noop
//   an empty row fails has(), so transform is a no-op there - it does NOT bind
// the mapper's output (the deliberate contrast with compose_with, which does
// bind onto an empty row).
bool
tests_transform_empty_row_is_noop()
{
    bool ok = true;

    dt::test_callable_table tbl;
    int mapper_calls = 0;
    dt::test_callable_id id = tbl.add(thunk_type{});   // empty row
    ok = D_CALL_CHECK(!tbl.has(id)) && ok;

    tbl.transform(id, [&mapper_calls](thunk_type /*t*/) -> thunk_type {
        ++mapper_calls;
        return thunk_type([]() -> bool { return true; });
    });

    ok = D_CALL_CHECK(mapper_calls == 0) && ok;   // guard returned before the mapper
    ok = D_CALL_CHECK(!tbl.has(id))      && ok;   // row still empty
    ok = D_CALL_CHECK(!tbl.invoke(id))   && ok;

    return ok;
}

// tests_transform_to_empty_disables_row
//   a mapper returning an empty thunk disables the row: has() goes false and
// invoke reads false, though the row remains counted by size().
bool
tests_transform_to_empty_disables_row()
{
    bool ok = true;

    dt::test_callable_table tbl;
    dt::test_callable_id id = tbl.add([]{ return true; });
    ok = D_CALL_CHECK(tbl.has(id))    && ok;
    ok = D_CALL_CHECK(tbl.invoke(id)) && ok;

    tbl.transform(id, [](thunk_type /*t*/) -> thunk_type {
        return thunk_type{};   // replace with an empty function
    });

    ok = D_CALL_CHECK(!tbl.has(id))                 && ok;   // now disabled
    ok = D_CALL_CHECK(!tbl.invoke(id))              && ok;   // reads false
    ok = D_CALL_CHECK(tbl.size() == std::size_t(1)) && ok;   // still a row

    return ok;
}


NS_END  // testing
NS_END  // djinterp
