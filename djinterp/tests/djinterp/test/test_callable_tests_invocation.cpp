/******************************************************************************
* djinterp [test]                          test_callable_tests_invocation.cpp
*
*   Running a row.  invoke returns the bound thunk's boolean when the id names
* a live row, and returns false - never throwing - when it does not: the
* sentinel, an out-of-range id, or an empty row (where a naive call on the
* stored std::function would raise bad_function_call).  invoke also re-runs the
* thunk on every call rather than caching, witnessed through a counting_thunk
* whose counter advances once per invocation.  Finally, the read surface (has /
* invoke / size / empty) is exercised through a const table, the way the
* framework dispatches during a tree walk.
*
* path:      /tests/djinterp/test/test_callable/test_callable_tests_invocation.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_callable_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_invoke_bound_returns_thunk_result
//   invoke returns exactly what the bound thunk returns.
bool
tests_invoke_bound_returns_thunk_result()
{
    bool ok = true;

    dt::test_callable_table tbl;
    dt::test_callable_id t = tbl.add([]{ return true;  });
    dt::test_callable_id f = tbl.add([]{ return false; });

    ok = D_CALL_CHECK(tbl.invoke(t))   && ok;
    ok = D_CALL_CHECK(!tbl.invoke(f))  && ok;

    return ok;
}

// tests_invoke_unbound_and_sentinel_false
//   invoke of an id that names no live row is false and does not throw -
// sentinel, one-past-the-end, far out of range, and the maximal id value.
bool
tests_invoke_unbound_and_sentinel_false()
{
    bool ok = true;

    dt::test_callable_table tbl;
    tbl.add([]{ return true; });   // id 1 is live

    ok = D_CALL_CHECK(!tbl.invoke(dt::k_no_callable))                        && ok;
    ok = D_CALL_CHECK(!tbl.invoke(dt::test_callable_id(2)))                  && ok;
    ok = D_CALL_CHECK(!tbl.invoke(dt::test_callable_id(9999)))              && ok;
    ok = D_CALL_CHECK(!tbl.invoke(static_cast<dt::test_callable_id>(-1)))    && ok;

    dt::test_callable_table e;
    ok = D_CALL_CHECK(!e.invoke(dt::test_callable_id(1)))                    && ok;

    return ok;
}

// tests_invoke_empty_row_false
//   invoke of an empty row returns false via the has() guard - crucially, it
// does not call the empty std::function (which would throw).
bool
tests_invoke_empty_row_false()
{
    bool ok = true;

    dt::test_callable_table tbl;
    dt::test_callable_id id = tbl.add(thunk_type{});   // empty row

    ok = D_CALL_CHECK(!tbl.has(id))     && ok;
    ok = D_CALL_CHECK(!tbl.invoke(id))  && ok;         // false, not a thrown exception

    return ok;
}

// tests_invoke_reruns_each_call
//   invoke runs the thunk every time it is called; there is no memoization, so
// N invocations mean N evaluations.
bool
tests_invoke_reruns_each_call()
{
    bool ok = true;

    dt::test_callable_table tbl;

    int calls = 0;
    dt::test_callable_id id = tbl.add(counting_thunk{true, &calls});
    ok = D_CALL_CHECK(tbl.invoke(id))   && ok;   // 1
    ok = D_CALL_CHECK(tbl.invoke(id))   && ok;   // 2
    ok = D_CALL_CHECK(tbl.invoke(id))   && ok;   // 3
    ok = D_CALL_CHECK(calls == 3)       && ok;

    int cf = 0;
    dt::test_callable_id idf = tbl.add(counting_thunk{false, &cf});
    ok = D_CALL_CHECK(!tbl.invoke(idf)) && ok;
    ok = D_CALL_CHECK(!tbl.invoke(idf)) && ok;
    ok = D_CALL_CHECK(cf == 2)          && ok;

    return ok;
}

// tests_invoke_const_table
//   has / invoke / size / empty are const-qualified and work through a const
// table - the read surface the framework uses to dispatch a bound leaf.
bool
tests_invoke_const_table()
{
    bool ok = true;

    dt::test_callable_table tbl;
    int calls = 0;
    dt::test_callable_id id = tbl.add(counting_thunk{true, &calls});

    const dt::test_callable_table& c = tbl;
    ok = D_CALL_CHECK(!c.empty())                    && ok;
    ok = D_CALL_CHECK(c.size() == std::size_t(1))    && ok;
    ok = D_CALL_CHECK(c.has(id))                     && ok;
    ok = D_CALL_CHECK(c.invoke(id))                  && ok;   // invoke is const
    ok = D_CALL_CHECK(calls == 1)                    && ok;
    ok = D_CALL_CHECK(!c.has(dt::k_no_callable))     && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
