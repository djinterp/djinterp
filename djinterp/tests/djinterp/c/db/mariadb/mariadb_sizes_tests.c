/*******************************************************************************
* djinterp [test]                                          mariadb_sizes_tests.c
*
*   Checks that the size mariadb_query.c computes for a query's attribute
* names refuses to wrap. The count is the attributes the connection keeps,
* added one call at a time, so no call through the layer's functions
* reaches a count that wraps a size_t. The suite therefore includes
* mariadb_query.c, as a white-box test, and gives
* d_internal_mariadb_bind_attributes a count too large: it must report
* D_INTERNAL_MARIADB_OUT_OF_MEMORY with nothing allocated and none of the
* attributes, nor the client, touched.
*   The site exists only over a client with query attributes, libmysqlclient
* from 8.0.23 (D_INTERNAL_MARIADB_QUERY_ATTRIBUTES); over any other, the
* suite's one test is skipped, and says why. It needs the client's headers
* and library, but no server.
*
*   Build: CMake's c_mariadb_sizes_tests, or scripts/ci/check_db_mariadb.sh,
* which runs it at every C standard. By hand, from the repo root: this file
* and its runner, mariadb_sizes_tests_runner.c in the CMake leaf
* (build/cmake/config/testing/djinterp/c/db/mariadb/), with DTest's C face
* (src/djinterp/c/modules/test/test_suite.c), db_*.c and the layer's sources
* but mariadb_query.c, which this file includes, and -Iinc
* -Itests/djinterp/c/db/mariadb, against the client.
*
*
* path:      /tests/djinterp/c/db/mariadb/mariadb_sizes_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
// the unit first, whole: it defines D_INTERNAL_MARIADB_SOURCE before any
// include, as the layer's own sources must
#include "../../../../../src/djinterp/c/db/mariadb/mariadb_query.c"  // under test
#include "./mariadb_sizes_tests.h"  // corresponding header
// std
#include <string.h>  // memset
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // SIZE_MAX


/*
d_tests_mariadb_sizes_attributes
  The attribute names' bytes, at a count whose product with a pointer's size
wraps, on a connection that is zero but for the count: no client handle, and
no attributes, which a read would fault on. Checks the following:
  - the result is D_INTERNAL_MARIADB_OUT_OF_MEMORY
  - the connection is as it was: its count, no attributes, no client
*/
static
void d_tests_mariadb_sizes_attributes(
    struct dj_test_suite* _suite
)
{
#if (D_INTERNAL_MARIADB_QUERY_ATTRIBUTES == 1)
    const size_t      count = (SIZE_MAX / sizeof(char*)) + 1u;
    struct dj_mariadb db;
    int               result = 0;

    (void)d_test_suite_test(_suite, "the attribute names' bytes");
    memset(&db, 0, sizeof(db));
    db.attribute_count = count;
    result             = d_internal_mariadb_bind_attributes(&db);
    D_TEST_CHECK(_suite, result == D_INTERNAL_MARIADB_OUT_OF_MEMORY);
    D_TEST_CHECK(_suite, db.attribute_count == count);
    D_TEST_CHECK(_suite, db.attributes == NULL);
    D_TEST_CHECK(_suite, db.native == NULL);
#else
    (void)d_test_suite_test(_suite, "the attribute names' bytes");
    (void)d_test_suite_skip(_suite,
                            "this client has no query attributes, and the "
                            "layer no site");
#endif  // D_INTERNAL_MARIADB_QUERY_ATTRIBUTES

    return;
}


/*
d_tests_mariadb_sizes
  The suite's section: a count too large for each checked size.
*/
void d_tests_mariadb_sizes(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_module(_suite, "mariadb_sizes");
    (void)d_test_suite_describe(_suite,
                                "c/db/mariadb: sizes that would wrap");
    (void)d_test_suite_block(_suite, "mariadb_query");
    d_tests_mariadb_sizes_attributes(_suite);

    return;
}
