/*******************************************************************************
* djinterp [tests]                                      test_stress_tests_op.cpp
*
*   Section TU: stress_op - the weighted operation descriptor consumed by the
* chaos runner.  A plain aggregate, so the whole section is single-threaded.
*
*
* path:      /tests/djinterp/test/sync/test_stress_tests_op.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <string>
// djinterp
#include "test_stress_tests.hpp"


NS_DJINTERP
NS_TESTING


bool tests_sop_default_construct()
{
    dt::stress_op op;

    D_HS_CHECK(op.name.empty());
    D_HS_CHECK(op.weight == 1u);      // default weight, not zero
    D_HS_CHECK(!op.action);           // no action bound

    return true;
}

bool tests_sop_value_construct()
{
    int hits = 0;

    dt::stress_op op(
        std::string("insert"),
        7,
        dt::stress_op::operation_fn(
            [&hits](std::size_t /*tid*/, std::size_t /*iter*/)
            {
                ++hits;
            }));

    D_HS_CHECK(op.name == "insert");
    D_HS_CHECK(op.weight == 7u);
    D_HS_CHECK(static_cast<bool>(op.action));
    D_HS_CHECK(hits == 0);            // constructing does not invoke

    return true;
}

bool tests_sop_action_invocable()
{
    std::size_t seen_tid  = 0;
    std::size_t seen_iter = 0;
    int         hits      = 0;

    dt::stress_op op(
        std::string("probe"),
        1,
        dt::stress_op::operation_fn(
            [&seen_tid, &seen_iter, &hits]
            (std::size_t _tid, std::size_t _iter)
            {
                seen_tid  = _tid;
                seen_iter = _iter;
                ++hits;
            }));

    op.action(3, 11);

    D_HS_CHECK(hits == 1);
    D_HS_CHECK(seen_tid == 3u);
    D_HS_CHECK(seen_iter == 11u);

    return true;
}

bool tests_sop_assignable()
{
    dt::stress_op op;

    op.name   = "renamed";
    op.weight = 0;                    // zero is representable on the struct
    op.action = dt::stress_op::operation_fn(
        [](std::size_t, std::size_t) {});

    D_HS_CHECK(op.name == "renamed");
    D_HS_CHECK(op.weight == 0u);
    D_HS_CHECK(static_cast<bool>(op.action));

    return true;
}


NS_END  // testing
NS_END  // djinterp
