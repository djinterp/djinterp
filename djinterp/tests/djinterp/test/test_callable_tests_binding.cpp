/******************************************************************************
* djinterp [test]                             test_callable_tests_binding.cpp
*
*   The storage and query surface of test_callable_table: add issues stable,
* monotonic, 1-based ids (never the reserved sentinel); size / empty / clear
* track the row set; and has distinguishes a live row from the three ways an id
* can fail to name one - it is the sentinel (0), it is out of range, or its row
* exists but holds no callable (an added empty thunk).  That last case is the
* one that separates has() from a mere bounds check, and it is set up here by
* adding an empty thunk_type{} - a row that occupies an id yet reads as absent.
*
* path:      /tests/djinterp/test/test_callable/test_callable_tests_binding.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_callable_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_binding_add_issues_1based_ids
//   add returns 1, 2, 3, ... in order - 1-based, never the sentinel - and an
// id stays valid as later rows are appended (issued ids are stable).
bool
tests_binding_add_issues_1based_ids()
{
    bool ok = true;

    dt::test_callable_table tbl;
    ok = D_CALL_CHECK(tbl.empty())                            && ok;

    dt::test_callable_id a = tbl.add([]{ return true;  });
    dt::test_callable_id b = tbl.add([]{ return false; });
    dt::test_callable_id c = tbl.add([]{ return true;  });

    ok = D_CALL_CHECK(a == dt::test_callable_id(1))           && ok;
    ok = D_CALL_CHECK(b == dt::test_callable_id(2))           && ok;
    ok = D_CALL_CHECK(c == dt::test_callable_id(3))           && ok;
    ok = D_CALL_CHECK(a != dt::k_no_callable)                 && ok;   // never the sentinel
    ok = D_CALL_CHECK(tbl.size() == std::size_t(3))           && ok;
    ok = D_CALL_CHECK(tbl.has(a) && tbl.has(b) && tbl.has(c)) && ok;

    // an earlier id survives further appends
    dt::test_callable_id d = tbl.add([]{ return true; });
    ok = D_CALL_CHECK(d == dt::test_callable_id(4))           && ok;
    ok = D_CALL_CHECK(tbl.has(a) && tbl.has(d))               && ok;

    return ok;
}

// tests_binding_empty_and_size
//   empty is true exactly on the empty table; size counts the rows issued.
bool
tests_binding_empty_and_size()
{
    bool ok = true;

    dt::test_callable_table tbl;
    ok = D_CALL_CHECK(tbl.empty())                  && ok;
    ok = D_CALL_CHECK(tbl.size() == std::size_t(0)) && ok;

    tbl.add([]{ return true; });
    ok = D_CALL_CHECK(!tbl.empty())                 && ok;
    ok = D_CALL_CHECK(tbl.size() == std::size_t(1)) && ok;

    tbl.add([]{ return true; });
    tbl.add([]{ return true; });
    ok = D_CALL_CHECK(tbl.size() == std::size_t(3)) && ok;
    ok = D_CALL_CHECK(!tbl.empty())                 && ok;

    return ok;
}

// tests_binding_has_sentinel_and_out_of_range
//   has rejects the reserved sentinel id 0 and every id past the last row -
// including the maximal id value - and reports absent for any id on an empty
// table.
bool
tests_binding_has_sentinel_and_out_of_range()
{
    bool ok = true;

    dt::test_callable_table tbl;
    dt::test_callable_id id = tbl.add([]{ return true; });   // id 1, size 1

    ok = D_CALL_CHECK(tbl.has(id))                                          && ok;   // the one live id
    ok = D_CALL_CHECK(!tbl.has(dt::k_no_callable))                          && ok;   // sentinel
    ok = D_CALL_CHECK(!tbl.has(dt::test_callable_id(2)))                    && ok;   // one past the end
    ok = D_CALL_CHECK(!tbl.has(dt::test_callable_id(9999)))                 && ok;   // far out of range
    ok = D_CALL_CHECK(!tbl.has(static_cast<dt::test_callable_id>(-1)))      && ok;   // maximal id

    // empty table: nothing is present, sentinel included
    dt::test_callable_table e;
    ok = D_CALL_CHECK(!e.has(dt::test_callable_id(1)))                      && ok;
    ok = D_CALL_CHECK(!e.has(dt::k_no_callable))                            && ok;

    return ok;
}

// tests_binding_has_empty_row_is_false
//   adding an empty thunk still issues an id and grows the table, but the row
// holds no callable, so has() is false - the third has() condition (the row is
// a non-empty std::function) failing while the bounds condition holds.
bool
tests_binding_has_empty_row_is_false()
{
    bool ok = true;

    dt::test_callable_table tbl;
    dt::test_callable_id id = tbl.add(thunk_type{});         // empty function

    ok = D_CALL_CHECK(id == dt::test_callable_id(1))         && ok;   // an id was issued
    ok = D_CALL_CHECK(tbl.size() == std::size_t(1))          && ok;   // the row exists
    ok = D_CALL_CHECK(!tbl.empty())                          && ok;
    ok = D_CALL_CHECK(!tbl.has(id))                          && ok;   // ...but reads as absent

    // a later real add is unaffected and takes the next id
    dt::test_callable_id id2 = tbl.add([]{ return true; });
    ok = D_CALL_CHECK(id2 == dt::test_callable_id(2))        && ok;
    ok = D_CALL_CHECK(tbl.has(id2))                          && ok;

    return ok;
}

// tests_binding_clear_empties
//   clear drops every row; the table is empty again and every previously
// issued id reads as absent (and invokes to false).
bool
tests_binding_clear_empties()
{
    bool ok = true;

    dt::test_callable_table tbl;
    dt::test_callable_id a = tbl.add([]{ return true; });
    dt::test_callable_id b = tbl.add([]{ return true; });
    ok = D_CALL_CHECK(tbl.size() == std::size_t(2))          && ok;

    tbl.clear();
    ok = D_CALL_CHECK(tbl.empty())                           && ok;
    ok = D_CALL_CHECK(tbl.size() == std::size_t(0))          && ok;
    ok = D_CALL_CHECK(!tbl.has(a) && !tbl.has(b))            && ok;   // prior ids gone
    ok = D_CALL_CHECK(!tbl.invoke(a))                        && ok;   // and read false

    return ok;
}

// tests_binding_add_after_clear_reissues
//   because ids are size-based, the first add after a clear reissues id 1 -
// now naming the fresh row.  (The header warns callers not to reuse ids from
// before a clear; the table itself does not track them, so this pins the
// mechanical behavior, not a reuse guarantee.)
bool
tests_binding_add_after_clear_reissues()
{
    bool ok = true;

    dt::test_callable_table tbl;
    tbl.add([]{ return false; });     // id 1
    tbl.add([]{ return false; });     // id 2
    tbl.clear();

    int calls = 0;
    dt::test_callable_id r = tbl.add(counting_thunk{true, &calls});

    ok = D_CALL_CHECK(r == dt::test_callable_id(1))          && ok;   // id 1 reissued
    ok = D_CALL_CHECK(tbl.has(r))                            && ok;
    ok = D_CALL_CHECK(tbl.invoke(r))                         && ok;   // the NEW row (true)
    ok = D_CALL_CHECK(calls == 1)                            && ok;
    ok = D_CALL_CHECK(tbl.size() == std::size_t(1))          && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
