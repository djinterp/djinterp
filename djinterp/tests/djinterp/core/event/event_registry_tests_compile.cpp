/******************************************************************************
* djinterp [test]                            event_registry_tests_compile.cpp
*
*   Section III (compile) -- the staging operation that evaluates a registry's
* static effective word for one event into a single fused_step.  Covers an
* empty registry (a zero-size step that folds to pass); a populated registry
* (the step's size equals the number of enabled handlers and folding it
* reproduces the dispatch order and outcome); the compile-time mask (disabled
* handlers are excluded from the snapshot); and snapshot independence -- a
* fused_step captured by compile() is unaffected by later registry edits, since
* its binding time is compile, not delivery.
*
*
* path:      /tests/djinterp/core/event/event_registry/event_registry_tests_compile.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_registry_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_compile_empty
bool
tests_compile_empty()
{
    bool ok = true;

    // compiling an event with no handlers yields a zero-size step that passes.
    event_registry reg;
    fused_step<ev_int> fs = reg.compile<ev_int>();

    ok = D_ER_CHECK(fs.size() == 0u) && ok;
    ok = D_ER_CHECK(fs(0) == verdict::pass) && ok;

    return ok;
}


// tests_compile_size_and_fold
bool
tests_compile_size_and_fold()
{
    bool ok = true;

    // three enabled handlers compile into a size-three step that folds them in
    // order; with all passing the outcome is pass.
    event_registry reg;
    std::vector<int> log;
    reg.bind<ev_int>(unary_rec{&log, 1, verdict::pass});
    reg.bind<ev_int>(unary_rec{&log, 2, verdict::pass});
    reg.bind<ev_int>(unary_rec{&log, 3, verdict::pass});

    fused_step<ev_int> fs = reg.compile<ev_int>();
    ok = D_ER_CHECK(fs.size() == 3u) && ok;

    log.clear();
    verdict v = fs(0);
    ok = D_ER_CHECK(v == verdict::pass) && ok;
    ok = D_ER_CHECK(log.size() == 3u) && ok;
    if (log.size() == 3u)
    {
        ok = D_ER_CHECK(log[0] == 1 && log[1] == 2 && log[2] == 3) && ok;
    }

    // a consuming letter short-circuits the fused fold too.
    event_registry reg2;
    std::vector<int> log2;
    reg2.bind<ev_int>(unary_rec{&log2, 1, verdict::consume});
    reg2.bind<ev_int>(unary_rec{&log2, 2, verdict::pass});
    fused_step<ev_int> fs2 = reg2.compile<ev_int>();
    verdict v2 = fs2(0);
    ok = D_ER_CHECK(v2 == verdict::consume) && ok;
    ok = D_ER_CHECK(log2.size() == 1u && log2[0] == 1) && ok;

    return ok;
}


// tests_compile_masks_disabled
bool
tests_compile_masks_disabled()
{
    bool ok = true;

    // the compile-time mask drops disabled handlers from the snapshot: only
    // the two enabled letters (1 and 3) are fused.
    event_registry reg;
    std::vector<int> log;
    reg.bind<ev_int>(unary_rec{&log, 1, verdict::pass});
    handler_id b = reg.bind<ev_int>(unary_rec{&log, 2, verdict::pass});
    reg.bind<ev_int>(unary_rec{&log, 3, verdict::pass});
    reg.disable(b);

    fused_step<ev_int> fs = reg.compile<ev_int>();
    ok = D_ER_CHECK(fs.size() == 2u) && ok;

    log.clear();
    fs(0);
    ok = D_ER_CHECK(log.size() == 2u) && ok;
    if (log.size() == 2u)
    {
        ok = D_ER_CHECK(log[0] == 1 && log[1] == 3) && ok;
    }

    return ok;
}


// tests_compile_snapshot_independent
bool
tests_compile_snapshot_independent()
{
    bool ok = true;

    // a fused_step holds its own snapshot of the word; edits to the registry
    // after compile() do not change it.
    event_registry reg;
    std::vector<int> log;
    reg.bind<ev_int>(unary_rec{&log, 1, verdict::pass});
    reg.bind<ev_int>(unary_rec{&log, 2, verdict::pass});

    fused_step<ev_int> fs = reg.compile<ev_int>();
    ok = D_ER_CHECK(fs.size() == 2u) && ok;

    // wipe the registry entirely after compiling.
    reg.clear();
    ok = D_ER_CHECK(reg.handler_count() == 0u) && ok;

    // the fused step is unaffected: same size, still folds the original word.
    ok = D_ER_CHECK(fs.size() == 2u) && ok;
    log.clear();
    verdict v = fs(0);
    ok = D_ER_CHECK(v == verdict::pass) && ok;
    ok = D_ER_CHECK(log.size() == 2u && log[0] == 1 && log[1] == 2) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
