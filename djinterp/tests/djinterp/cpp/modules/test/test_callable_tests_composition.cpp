/*******************************************************************************
* djinterp [cpp][test]                       test_callable_tests_composition.cpp
*
*   The boolean algebra over thunks.  compose_and / compose_or fold a clause
* onto the thunk already bound to an id under short-circuit and / or, replacing
* the row with a value-captured closure over (existing, clause).  Pinned here:
* the full four-row truth table for each; that the fold short-circuits (a false
* AND-left, or a true OR-left, never evaluates the clause - witnessed by a
* DCountingThunk whose counter stays at zero); that repeated composition
* accumulates by wrapping the whole current thunk, so nested short-circuit
* flows through a chain and mixed and/or folds left in call order; and the two
* edges in compose_with's own branching - composing an unbound or sentinel id
* is a silent no-op, while composing onto an empty row binds the clause alone
* (the branch that avoids calling the empty std::function, which would throw).
* A final case grows the table past a reallocation to confirm a composed thunk,
* holding its operands by value, does not dangle on the vector's slot.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_callable_tests_composition.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_callable_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_compose_and_truth_table
//   compose_and yields (existing AND clause) across all four operand pairs.
bool tests_compose_and_truth_table()
{
    bool ok = true;

    // T && T -> T
    {
        dt::DTestCallableTable tbl;
        dt::DTestCallableId id = tbl.add([]{ return true; });
        tbl.compose_and(id, []{ return true; });
        ok = D_CALL_CHECK(tbl.invoke(id))  && ok;
    }
    // T && F -> F
    {
        dt::DTestCallableTable tbl;
        dt::DTestCallableId id = tbl.add([]{ return true; });
        tbl.compose_and(id, []{ return false; });
        ok = D_CALL_CHECK(!tbl.invoke(id)) && ok;
    }
    // F && T -> F
    {
        dt::DTestCallableTable tbl;
        dt::DTestCallableId id = tbl.add([]{ return false; });
        tbl.compose_and(id, []{ return true; });
        ok = D_CALL_CHECK(!tbl.invoke(id)) && ok;
    }
    // F && F -> F
    {
        dt::DTestCallableTable tbl;
        dt::DTestCallableId id = tbl.add([]{ return false; });
        tbl.compose_and(id, []{ return false; });
        ok = D_CALL_CHECK(!tbl.invoke(id)) && ok;
    }

    return ok;
}

// tests_compose_or_truth_table
//   compose_or yields (existing OR clause) across all four operand pairs.
bool tests_compose_or_truth_table()
{
    bool ok = true;

    // F || F -> F
    {
        dt::DTestCallableTable tbl;
        dt::DTestCallableId id = tbl.add([]{ return false; });
        tbl.compose_or(id, []{ return false; });
        ok = D_CALL_CHECK(!tbl.invoke(id)) && ok;
    }
    // F || T -> T
    {
        dt::DTestCallableTable tbl;
        dt::DTestCallableId id = tbl.add([]{ return false; });
        tbl.compose_or(id, []{ return true; });
        ok = D_CALL_CHECK(tbl.invoke(id))  && ok;
    }
    // T || F -> T
    {
        dt::DTestCallableTable tbl;
        dt::DTestCallableId id = tbl.add([]{ return true; });
        tbl.compose_or(id, []{ return false; });
        ok = D_CALL_CHECK(tbl.invoke(id))  && ok;
    }
    // T || T -> T
    {
        dt::DTestCallableTable tbl;
        dt::DTestCallableId id = tbl.add([]{ return true; });
        tbl.compose_or(id, []{ return true; });
        ok = D_CALL_CHECK(tbl.invoke(id))  && ok;
    }

    return ok;
}

// tests_compose_and_short_circuits
//   a false left operand of an AND fold never evaluates the clause; a true
// left operand does.
bool tests_compose_and_short_circuits()
{
    bool ok = true;

    // left false -> clause skipped
    {
        dt::DTestCallableTable tbl;
        int ec = 0, cc = 0;
        dt::DTestCallableId id = tbl.add(DCountingThunk{false, &ec});
        tbl.compose_and(id, DCountingThunk{true, &cc});
        ok = D_CALL_CHECK(!tbl.invoke(id)) && ok;
        ok = D_CALL_CHECK(ec == 1)         && ok;   // left evaluated
        ok = D_CALL_CHECK(cc == 0)         && ok;   // clause short-circuited away
    }
    // left true -> clause evaluated
    {
        dt::DTestCallableTable tbl;
        int ec = 0, cc = 0;
        dt::DTestCallableId id = tbl.add(DCountingThunk{true, &ec});
        tbl.compose_and(id, DCountingThunk{true, &cc});
        ok = D_CALL_CHECK(tbl.invoke(id))     && ok;
        ok = D_CALL_CHECK(ec == 1 && cc == 1) && ok;
    }

    return ok;
}

// tests_compose_or_short_circuits
//   a true left operand of an OR fold never evaluates the clause; a false left
// operand does.
bool tests_compose_or_short_circuits()
{
    bool ok = true;

    // left true -> clause skipped
    {
        dt::DTestCallableTable tbl;
        int ec = 0, cc = 0;
        dt::DTestCallableId id = tbl.add(DCountingThunk{true, &ec});
        tbl.compose_or(id, DCountingThunk{false, &cc});
        ok = D_CALL_CHECK(tbl.invoke(id)) && ok;
        ok = D_CALL_CHECK(ec == 1)        && ok;
        ok = D_CALL_CHECK(cc == 0)        && ok;   // clause short-circuited away
    }
    // left false -> clause evaluated
    {
        dt::DTestCallableTable tbl;
        int ec = 0, cc = 0;
        dt::DTestCallableId id = tbl.add(DCountingThunk{false, &ec});
        tbl.compose_or(id, DCountingThunk{true, &cc});
        ok = D_CALL_CHECK(tbl.invoke(id))     && ok;
        ok = D_CALL_CHECK(ec == 1 && cc == 1) && ok;
    }

    return ok;
}

// tests_compose_chaining_accumulates
//   each compose wraps the whole current thunk, so add p; and q; and r builds
// ((p AND q) AND r) - and nested short-circuit flows through the chain: a
// falsehood anywhere stops every clause to its right.
bool tests_compose_chaining_accumulates()
{
    bool ok = true;

    // all true -> true; all three run
    {
        dt::DTestCallableTable tbl;
        int pc = 0, qc = 0, rc = 0;
        dt::DTestCallableId id = tbl.add(DCountingThunk{true, &pc});
        tbl.compose_and(id, DCountingThunk{true, &qc});
        tbl.compose_and(id, DCountingThunk{true, &rc});
        ok = D_CALL_CHECK(tbl.invoke(id))               && ok;
        ok = D_CALL_CHECK(pc == 1 && qc == 1 && rc == 1) && ok;
    }
    // middle false -> false; the outer AND short-circuits r
    {
        dt::DTestCallableTable tbl;
        int pc = 0, qc = 0, rc = 0;
        dt::DTestCallableId id = tbl.add(DCountingThunk{true, &pc});
        tbl.compose_and(id, DCountingThunk{false, &qc});
        tbl.compose_and(id, DCountingThunk{true, &rc});
        ok = D_CALL_CHECK(!tbl.invoke(id))    && ok;
        ok = D_CALL_CHECK(pc == 1 && qc == 1) && ok;
        ok = D_CALL_CHECK(rc == 0)            && ok;   // r never reached
    }
    // first false -> both q and r skipped (inner AND, then outer AND)
    {
        dt::DTestCallableTable tbl;
        int pc = 0, qc = 0, rc = 0;
        dt::DTestCallableId id = tbl.add(DCountingThunk{false, &pc});
        tbl.compose_and(id, DCountingThunk{true, &qc});
        tbl.compose_and(id, DCountingThunk{true, &rc});
        ok = D_CALL_CHECK(!tbl.invoke(id))    && ok;
        ok = D_CALL_CHECK(pc == 1)            && ok;
        ok = D_CALL_CHECK(qc == 0 && rc == 0) && ok;
    }

    return ok;
}

// tests_compose_mixed_and_or
//   composition folds LEFT in call order: add p; and q; or r builds
// ((p AND q) OR r), so both short-circuit directions appear in one thunk.
bool tests_compose_mixed_and_or()
{
    bool ok = true;

    // p=F: (F AND q) short-circuits q; then (F OR r) evaluates r=T -> T
    {
        dt::DTestCallableTable tbl;
        int pc = 0, qc = 0, rc = 0;
        dt::DTestCallableId id = tbl.add(DCountingThunk{false, &pc});
        tbl.compose_and(id, DCountingThunk{true, &qc});
        tbl.compose_or(id,  DCountingThunk{true, &rc});
        ok = D_CALL_CHECK(tbl.invoke(id))                 && ok;   // = T
        ok = D_CALL_CHECK(pc == 1 && qc == 0 && rc == 1)  && ok;   // q skipped, r run
    }
    // p=T: (T AND T)=T; then (T OR r) short-circuits r -> T
    {
        dt::DTestCallableTable tbl;
        int pc = 0, qc = 0, rc = 0;
        dt::DTestCallableId id = tbl.add(DCountingThunk{true, &pc});
        tbl.compose_and(id, DCountingThunk{true, &qc});
        tbl.compose_or(id,  DCountingThunk{false, &rc});
        ok = D_CALL_CHECK(tbl.invoke(id))     && ok;               // = T
        ok = D_CALL_CHECK(pc == 1 && qc == 1) && ok;
        ok = D_CALL_CHECK(rc == 0)            && ok;               // r skipped (OR left true)
    }

    return ok;
}

// tests_compose_unbound_is_noop
//   composing onto an id that names no row - the sentinel, or an out-of-range
// id - changes nothing (compose_with returns without touching storage).
bool tests_compose_unbound_is_noop()
{
    bool ok = true;

    // sentinel on an empty table: nothing is added
    {
        dt::DTestCallableTable tbl;
        tbl.compose_and(dt::k_no_callable, []{ return true; });
        ok = D_CALL_CHECK(tbl.empty())                       && ok;
        ok = D_CALL_CHECK(tbl.size() == std::size_t(0))      && ok;
        ok = D_CALL_CHECK(!tbl.has(dt::k_no_callable))       && ok;
    }
    // out-of-range id: table unchanged, that id still absent, existing row intact
    {
        dt::DTestCallableTable tbl;
        dt::DTestCallableId id = tbl.add([]{ return true; });   // id 1
        tbl.compose_or(dt::DTestCallableId(5), []{ return true; });
        ok = D_CALL_CHECK(tbl.size() == std::size_t(1))      && ok;
        ok = D_CALL_CHECK(!tbl.has(dt::DTestCallableId(5)))  && ok;
        ok = D_CALL_CHECK(tbl.invoke(id))                    && ok;
    }
    // sentinel with rows present: still a no-op
    {
        dt::DTestCallableTable tbl;
        dt::DTestCallableId id = tbl.add([]{ return true; });
        tbl.compose_and(dt::k_no_callable, []{ return false; });
        ok = D_CALL_CHECK(tbl.size() == std::size_t(1))      && ok;
        ok = D_CALL_CHECK(tbl.invoke(id))                    && ok;
    }

    return ok;
}

// tests_compose_empty_row_binds_clause
//   composing onto a row that exists but holds no callable binds the clause
// ALONE - the branch that must NOT try to fold against the empty function
// (which would throw).  invoke then returns exactly the clause's result.
bool tests_compose_empty_row_binds_clause()
{
    bool ok = true;

    // compose_and onto an empty row -> clause bound alone (true)
    {
        dt::DTestCallableTable tbl;
        int cc = 0;
        dt::DTestCallableId id = tbl.add(thunk_type{});     // empty row
        ok = D_CALL_CHECK(!tbl.has(id))    && ok;
        tbl.compose_and(id, DCountingThunk{true, &cc});
        ok = D_CALL_CHECK(tbl.has(id))     && ok;            // now live
        ok = D_CALL_CHECK(tbl.invoke(id))  && ok;            // == clause() (no phantom AND, no throw)
        ok = D_CALL_CHECK(cc == 1)         && ok;
    }
    // compose_or onto an empty row -> clause bound alone (false proves it is
    // the clause itself, not folded against a true)
    {
        dt::DTestCallableTable tbl;
        dt::DTestCallableId id = tbl.add(thunk_type{});
        tbl.compose_or(id, []{ return false; });
        ok = D_CALL_CHECK(tbl.has(id))     && ok;
        ok = D_CALL_CHECK(!tbl.invoke(id)) && ok;            // == clause() == false
    }

    return ok;
}

// tests_compose_survives_reallocation
//   a composed thunk captures its operands by value, so it keeps working after
// the table grows (and reallocates) - and its short-circuit behavior is
// preserved.
bool tests_compose_survives_reallocation()
{
    bool ok = true;

    dt::DTestCallableTable tbl;
    int pc = 0, qc = 0;
    dt::DTestCallableId id = tbl.add(DCountingThunk{true, &pc});
    tbl.compose_and(id, DCountingThunk{true, &qc});          // row 1 = (p AND q)

    for (int i = 0; i < 1000; ++i)
    {
        tbl.add([]{ return true; });                         // force growth/realloc
    }
    ok = D_CALL_CHECK(tbl.size() == std::size_t(1001))       && ok;
    ok = D_CALL_CHECK(tbl.invoke(id))                        && ok;   // still valid
    ok = D_CALL_CHECK(pc == 1 && qc == 1)                    && ok;

    // short-circuit still holds across the reallocation
    int pc2 = 0, qc2 = 0;
    dt::DTestCallableId id2 = tbl.add(DCountingThunk{false, &pc2});
    tbl.compose_and(id2, DCountingThunk{true, &qc2});
    for (int i = 0; i < 1000; ++i)
    {
        tbl.add([]{ return true; });
    }
    ok = D_CALL_CHECK(!tbl.invoke(id2))                      && ok;
    ok = D_CALL_CHECK(pc2 == 1 && qc2 == 0)                  && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
