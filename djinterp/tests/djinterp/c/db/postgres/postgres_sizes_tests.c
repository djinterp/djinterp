/*******************************************************************************
* djinterp [test]                                         postgres_sizes_tests.c
*
*   Checks that the sizes postgres_connection.c computes for a connect's
* keyword and value arrays refuse to wrap. Their count is the layer's
* defaults, the caller's pairs, two more, and a connection string's parts; a
* caller's count is read in full before it reaches them, so no call through
* the layer's functions reaches a count that wraps a size_t. The suite
* therefore includes postgres_connection.c, as a white-box test, and gives
* d_internal_postgres_plan_fill a count too large at each of its four
* checks: every one must report D_POSTGRES_SQLSTATE_OUT_OF_MEMORY with
* nothing allocated and nothing it was handed read.
*   postgres_statement.c's room for a statement's parameters is the one site
* not here: its count is the int libpq reports, so its products cannot wrap
* a 64-bit size_t; the check guards 32-bit builds.
*   It needs libpq's headers and library, but no server.
*
*   Build: CMake's c_postgres_sizes_tests, or scripts/ci/check_db_postgres.sh,
* which runs it at every C standard. By hand, from the repo root: this file
* and its runner, postgres_sizes_tests_runner.c in the CMake leaf
* (build/cmake/config/testing/djinterp/c/db/postgres/), with DTest's C face
* (src/djinterp/c/modules/test/test_suite.c), db_*.c and the layer's sources
* but postgres_connection.c, which this file includes, and -Iinc
* -Itests/djinterp/c/db/postgres, against libpq.
*
*
* path:      /tests/djinterp/c/db/postgres/postgres_sizes_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
// the unit first, whole: it defines D_INTERNAL_POSTGRES_SOURCE before any
// include, as the layer's own sources must
#include "../../../../../src/djinterp/c/db/postgres/postgres_connection.c"  // under test
#include "./postgres_sizes_tests.h"  // corresponding header
// std
#include <string.h>  // memset
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // SIZE_MAX


//==============================================================================
// HARNESS
//==============================================================================

// dj_tests_postgres_sizes_case
//   struct: one count too large for d_internal_postgres_plan_fill, and where
// it is refused.
struct dj_tests_postgres_sizes_case
{
    const char* test;      // the test's name: the check that refuses it
    size_t      defaults;  // the plan's defaults
    size_t      count;     // the caller's pairs
    bool        parsed;    // whether a connection string's parts follow
};

/*
d_tests_postgres_sizes_refused
  One case: a plan with the case's defaults, and its parts where it has
them, filled with the case's count and no arrays, which a read would fault
on. Checks the following:
  - the result is D_POSTGRES_SQLSTATE_OUT_OF_MEMORY
  - nothing was allocated: no keyword or value array, and none owned
  - the plan is otherwise as it was: its defaults' count, and its parts
*/
static
void d_tests_postgres_sizes_refused(
    struct dj_test_suite*                      _suite,
    const struct dj_tests_postgres_sizes_case* _case
)
{
    struct dj_internal_postgres_plan plan;
    char*                            reason = NULL;
    PQconninfoOption*                parsed = NULL;
    int                              result = 0;

    (void)d_test_suite_test(_suite, _case->test);
    memset(&plan, 0, sizeof(plan));
    plan.defaults.count = _case->defaults;

    // a string's parts: libpq's every option, one of them given
    if (_case->parsed)
    {
        parsed = PQconninfoParse("host=localhost", &reason);

        if (!D_TEST_CHECK(_suite, parsed != NULL))
        {
            PQfreemem(reason);

            return;
        }

        plan.parsed = parsed;
    }

    result = d_internal_postgres_plan_fill(&plan, NULL, NULL, NULL,
                                           _case->count);
    D_TEST_CHECK(_suite, result == D_POSTGRES_SQLSTATE_OUT_OF_MEMORY);
    D_TEST_CHECK(_suite, plan.keywords == NULL);
    D_TEST_CHECK(_suite, plan.values == NULL);
    D_TEST_CHECK(_suite, !plan.owned);
    D_TEST_CHECK(_suite, plan.defaults.count == _case->defaults);
    D_TEST_CHECK(_suite, plan.parsed == parsed);

    if (parsed)
    {
        PQconninfoFree(parsed);
    }

    return;
}

/*
d_tests_postgres_sizes_fits
  The same function at a count that fits, so that the refusals are the
counts', not the function's. Checks the following:
  - the result is 0, the arrays allocated and owned
  - the caller's pair first in them
*/
static
void d_tests_postgres_sizes_fits(
    struct dj_test_suite* _suite
)
{
    const char* const                keywords[1] = { "application_name" };
    const char* const                values[1]   = { "d_tests" };
    struct dj_internal_postgres_plan plan;
    int                              result      = 0;

    (void)d_test_suite_test(_suite, "a count that fits");
    memset(&plan, 0, sizeof(plan));
    result = d_internal_postgres_plan_fill(&plan, NULL, keywords, values, 1u);
    D_TEST_CHECK(_suite, result == 0);
    D_TEST_CHECK(_suite, plan.owned);

    // the pair, where the arrays are
    if ( (plan.keywords) &&
         (plan.values) )
    {
        D_TEST_CHECK(_suite, plan.keywords[0] == keywords[0]);
        D_TEST_CHECK(_suite, plan.values[0] == values[0]);
    }

    d_internal_postgres_plan_free(&plan);

    return;
}


//==============================================================================
// ENTRY POINT
//==============================================================================

/*
d_tests_postgres_sizes
  The suite's section: a count that fits, then one too large for each of
d_internal_postgres_plan_fill's four checks.
*/
void d_tests_postgres_sizes(
    struct dj_test_suite* _suite
)
{
    const struct dj_tests_postgres_sizes_case cases[4] =
    {
        { "the defaults and the caller's pairs",           2u,
          SIZE_MAX - 1u,                                   false },
        { "the two more: a database's word and the end",   0u,
          SIZE_MAX - 1u,                                   false },
        { "one more for each of a string's parts",         0u,
          SIZE_MAX - 2u,                                   true  },
        { "the arrays' bytes",                             0u,
          SIZE_MAX / sizeof(const char*),                  false }
    };

    (void)d_test_suite_module(_suite, "postgres_sizes");
    (void)d_test_suite_describe(_suite,
                                "c/db/postgres: sizes that would wrap");
    (void)d_test_suite_block(_suite, "postgres_connection");
    d_tests_postgres_sizes_fits(_suite);

    for (size_t i = 0u; i < 4u; ++i)
    {
        d_tests_postgres_sizes_refused(_suite, &cases[i]);
    }

    return;
}
