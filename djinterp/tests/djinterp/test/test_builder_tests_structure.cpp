/******************************************************************************
* djinterp [test]                            test_builder_tests_structure.cpp
*
*   Building the forest.  test_module / test_block / test grow a rank-checked
* nesting (a module holds blocks, a block holds tests), each opener resetting
* the cursors beneath it; with no enclosing scope, a block or test lands as a
* top-level root.  The scoped module / block forms open a node, run a body, and
* RESTORE the prior cursor - checked here by proving a stray assertion after a
* scope has no open test to bind onto.  Rank safety is exercised two ways
* through the explicit-tree constructor (which keeps the caller's kinds rather
* than re-installing the defaults): a leaf `block` rejects a child test, and an
* empty kind set rejects a block under a module by the raw-id-as-rank fallback.
*
* path:      /tests/djinterp/test/test_builder/test_builder_tests_structure.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#include "test_builder_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_structure_module_block_test_counts
//   a module / block / test chain builds the three-level forest and the
// structural summary counts it.
bool
tests_structure_module_block_test_counts()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m").test_block("bl").test("t").assert_(true);
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.modules == 1)  && ok;
    ok = D_TB_CHECK(s.blocks == 1)   && ok;
    ok = D_TB_CHECK(s.tests == 1)    && ok;
    ok = D_TB_CHECK(s.passed == 1)   && ok;

    // and the forest carries one node of each kind
    auto is_module = [](const dt::basic_test& n){ return n.type_id() == dt::k_kind_module; };
    auto is_block  = [](const dt::basic_test& n){ return n.type_id() == dt::k_kind_block;  };
    auto is_test   = [](const dt::basic_test& n){ return n.type_id() == dt::k_kind_test;   };
    ok = D_TB_CHECK(b.count_if(is_module) == std::size_t(1)) && ok;
    ok = D_TB_CHECK(b.count_if(is_block)  == std::size_t(1)) && ok;
    ok = D_TB_CHECK(b.count_if(is_test)   == std::size_t(1)) && ok;

    return ok;
}

// tests_structure_multiple_and_nesting
//   several modules, each with nested blocks and tests, tally correctly.
bool
tests_structure_multiple_and_nesting()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_module("m1")
        .test_block("b1").test("t1").assert_(true)
        .test_block("b2").test("t2").assert_(true);
    b.test_module("m2")
        .test_block("b3").test("t3").assert_(true);
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.modules == 2) && ok;
    ok = D_TB_CHECK(s.blocks == 3)  && ok;
    ok = D_TB_CHECK(s.tests == 3)   && ok;
    ok = D_TB_CHECK(s.passed == 3)  && ok;

    return ok;
}

// tests_structure_block_without_module_toplevel
//   a block opened with no module in scope becomes a top-level root; a test
// then nests under it.
bool
tests_structure_block_without_module_toplevel()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test_block("bl").test("t").assert_(true);
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.modules == 0) && ok;
    ok = D_TB_CHECK(s.blocks == 1)  && ok;
    ok = D_TB_CHECK(s.tests == 1)   && ok;
    ok = D_TB_CHECK(s.passed == 1)  && ok;

    return ok;
}

// tests_structure_test_without_scope_toplevel
//   a test opened with no scope at all becomes a top-level root and still
// evaluates.
bool
tests_structure_test_without_scope_toplevel()
{
    bool ok = true;

    dt::test_builder<> b;
    b.test("t").assert_(true);
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.modules == 0 && s.blocks == 0) && ok;
    ok = D_TB_CHECK(s.tests == 1)                    && ok;
    ok = D_TB_CHECK(s.passed == 1)                   && ok;

    return ok;
}

// tests_structure_scoped_restores_cursor
//   the scoped module() and block() forms restore the cursor they saved: once
// the scope closes there is no open test, so a stray assert_ is a no-op and
// the scope's own test is left untouched (it still passes).  Were the cursor
// NOT restored, the stray assert_(false) would conjoin onto that test and fail
// it.
bool
tests_structure_scoped_restores_cursor()
{
    bool ok = true;

    // module scope
    {
        dt::test_builder<> b;
        b.module("m", [](dt::test_builder<>& m){
            m.test("inside").assert_(true);
        });
        b.assert_(false);                 // no open test -> no-op if restored
        dt::test_summary s = b.run();
        ok = D_TB_CHECK(s.modules == 1)             && ok;
        ok = D_TB_CHECK(s.tests == 1)               && ok;
        ok = D_TB_CHECK(s.passed == 1 && s.failed == 0) && ok;
    }
    // block scope
    {
        dt::test_builder<> b;
        b.block("bl", [](dt::test_builder<>& bl){
            bl.test("in").assert_(true);
        });
        b.assert_(false);                 // no open test -> no-op if restored
        dt::test_summary s = b.run();
        ok = D_TB_CHECK(s.blocks == 1)              && ok;
        ok = D_TB_CHECK(s.tests == 1)               && ok;
        ok = D_TB_CHECK(s.passed == 1 && s.failed == 0) && ok;
    }

    return ok;
}

// tests_structure_scoped_empty_body
//   a scoped form with an empty body still opens the node (the body is
// optional; only its contents are skipped).
bool
tests_structure_scoped_empty_body()
{
    bool ok = true;

    dt::test_builder<>::body_type empty;   // default-constructed -> empty

    dt::test_builder<> b;
    b.module("m", empty);
    dt::test_summary s = b.run();
    ok = D_TB_CHECK(s.modules == 1)                && ok;
    ok = D_TB_CHECK(s.blocks == 0 && s.tests == 0) && ok;

    b.block("bl", empty);
    dt::test_summary s2 = b.run();
    ok = D_TB_CHECK(s2.blocks == 1)                && ok;

    return ok;
}

// tests_structure_rank_rejection_leaf_block
//   under a kind set where `block` is a leaf, test_tree rejects a test placed
// under a block: the insertion returns null, the builder's guard leaves the
// test count untouched, and the trailing assert_ (now with no open test) is a
// safe no-op.
bool
tests_structure_rank_rejection_leaf_block()
{
    bool ok = true;

    dt::test_builder<> b{ dt::test_tree<dt::basic_test>(leaf_block_kinds()) };
    b.test_module("m").test_block("bl").test("t").assert_(true);
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.modules == 1) && ok;
    ok = D_TB_CHECK(s.blocks == 1)  && ok;
    ok = D_TB_CHECK(s.tests == 0)   && ok;   // the test was rejected under a leaf block

    auto is_test = [](const dt::basic_test& n){ return n.type_id() == dt::k_kind_test; };
    ok = D_TB_CHECK(b.count_if(is_test) == std::size_t(0)) && ok;

    return ok;
}

// tests_structure_explicit_tree_keeps_kinds
//   the explicit-tree ctor does NOT re-install the default kinds: given an
// EMPTY kind set, ids fall back to acting as their own ranks, so a block (id
// 1001) cannot nest under a module (id 1000) - 1001 > 1000 - and is rejected,
// while a module still attaches via the unconstrained add_root.
bool
tests_structure_explicit_tree_keeps_kinds()
{
    bool ok = true;

    dt::test_builder<> b{ dt::test_tree<dt::basic_test>(std::vector<dt::test_kind>()) };
    b.test_module("m");    // add_root: unconstrained -> succeeds
    b.test_block("bl");    // rank check with no kinds: 1001 <= 1000 is false -> rejected
    dt::test_summary s = b.run();

    ok = D_TB_CHECK(s.modules == 1) && ok;
    ok = D_TB_CHECK(s.blocks == 0)  && ok;   // default kinds were NOT re-installed

    return ok;
}


NS_END  // testing
NS_END  // djinterp
