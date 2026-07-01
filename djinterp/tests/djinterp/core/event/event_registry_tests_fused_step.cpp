/******************************************************************************
* djinterp [test]                         event_registry_tests_fused_step.cpp
*
*   Section II -- FUSED STEP.  Drives fused_step directly with hand-built words
* of erased verdict(void*) letters.  Covers the empty step (size zero, folds to
* pass); a multi-letter step folded in order with size reported; the consume
* left-zero that cuts off the remainder of the word; and the equivalence of
* the two entry points -- operator()(args...) which builds the payload from
* arguments and run_one(payload) which folds over an already-built payload.
*
*
* path:      /tests/djinterp/core/event/event_registry/event_registry_tests_fused_step.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_registry_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_fused_step_empty
bool
tests_fused_step_empty()
{
    bool ok = true;

    // an empty word folds to pass and reports size zero, through both entry
    // points.
    std::vector<std::function<verdict(void*)>> word;
    fused_step<ev_int> fs(word);

    ok = D_ER_CHECK(fs.size() == 0u) && ok;
    ok = D_ER_CHECK(fs(0) == verdict::pass) && ok;

    fused_step<ev_int>::payload_type p(0);
    ok = D_ER_CHECK(fs.run_one(p) == verdict::pass) && ok;

    return ok;
}


// tests_fused_step_fold_order_and_size
bool
tests_fused_step_fold_order_and_size()
{
    bool ok = true;

    // three all-pass letters: every one runs, in order, and the outcome is the
    // last letter's verdict (pass).
    std::vector<int> log;
    std::vector<std::function<verdict(void*)>> word;
    word.push_back(step_rec{&log, 1, verdict::pass});
    word.push_back(step_rec{&log, 2, verdict::pass});
    word.push_back(step_rec{&log, 3, verdict::pass});

    fused_step<ev_int> fs(word);
    ok = D_ER_CHECK(fs.size() == 3u) && ok;

    verdict v = fs(0);
    ok = D_ER_CHECK(v == verdict::pass) && ok;
    ok = D_ER_CHECK(log.size() == 3u) && ok;
    if (log.size() == 3u)
    {
        ok = D_ER_CHECK(log[0] == 1 && log[1] == 2 && log[2] == 3) && ok;
    }

    return ok;
}


// tests_fused_step_short_circuit
bool
tests_fused_step_short_circuit()
{
    bool ok = true;

    // a consume in the middle stops the fold: the third letter never runs and
    // the outcome is consume (the left zero).
    std::vector<int> log;
    std::vector<std::function<verdict(void*)>> word;
    word.push_back(step_rec{&log, 1, verdict::pass});
    word.push_back(step_rec{&log, 2, verdict::consume});
    word.push_back(step_rec{&log, 3, verdict::pass});

    fused_step<ev_int> fs(word);

    verdict v = fs(0);
    ok = D_ER_CHECK(v == verdict::consume) && ok;
    ok = D_ER_CHECK(log.size() == 2u) && ok;
    if (log.size() == 2u)
    {
        ok = D_ER_CHECK(log[0] == 1 && log[1] == 2) && ok;
    }

    return ok;
}


// tests_fused_step_operator_and_run_one
bool
tests_fused_step_operator_and_run_one()
{
    bool ok = true;

    // operator()(args) and run_one(payload) drive the same fold; here over a
    // word that consumes, both report consume and both run exactly two letters.
    std::vector<int> log;
    std::vector<std::function<verdict(void*)>> word;
    word.push_back(step_rec{&log, 7, verdict::pass});
    word.push_back(step_rec{&log, 8, verdict::consume});

    fused_step<ev_int> fs(word);

    log.clear();
    verdict v1 = fs(42);                       // builds payload from the arg
    ok = D_ER_CHECK(v1 == verdict::consume) && ok;
    ok = D_ER_CHECK(log.size() == 2u) && ok;

    log.clear();
    fused_step<ev_int>::payload_type p(42);    // reuse an existing payload
    verdict v2 = fs.run_one(p);
    ok = D_ER_CHECK(v2 == verdict::consume) && ok;
    ok = D_ER_CHECK(log.size() == 2u) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
