/*******************************************************************************
* djinterp [test]                                             file_tests_space.c
*
*   XV. CAPACITY (file_space) -- how much room the filesystem behind a path
* has. The success case sanity-checks the relationships that must hold on any
* real volume (there is some capacity; neither free nor available exceeds it),
* rather than asserting exact figures that depend on the host. On failure the
* out-parameter is zeroed, which the missing-path case checks, and NULL inputs
* are rejected.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_space.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <errno.h>
#include <string.h>
// djinterp
#include "file_tests.h"


void d_fs_tests_space(
    struct dj_test_suite* _suite
)
{
    struct dj_space_t space;
    char             path[D_FS_TEST_PATH_BUF];
    bool             sane;

    d_test_suite_module(_suite, "XV. file_space");

    d_test_suite_block(_suite, "d_file_space");

    d_test_suite_test(_suite, "reports coherent capacity for an existing path");
    memset(&space, 0, sizeof(space));
    sane = (d_file_space(d_fs_test_root(), &space) == 0);
    // some capacity exists, and neither free figure exceeds the total
    sane = sane &&
           (space.capacity > 0) &&
           (space.free <= space.capacity) &&
           (space.available <= space.capacity);
    D_FS_CHECK(_suite, sane);

    d_test_suite_test(_suite, "a missing path -> -1 and a zeroed result");
    d_fs_test_path(path, sizeof(path), "no_such_space_path");
    memset(&space, 0xFF, sizeof(space));
    D_FS_CHECK(_suite, d_file_space(path, &space) == -1);
    // the out-parameter is cleared on failure
    D_FS_CHECK(_suite, space.capacity == 0);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_space(NULL, &space) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL out-parameter -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_space(d_fs_test_root(), NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

    return;
#endif
}

/******************************************************************************
 * d_tests_file_space_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * XV.  file_space  --  FILESYSTEM CAPACITY
 *****************************************************************************/

#if D_FILE_SPACE_IS_AVAILABLE

/*
d_tests_file_space
  Tests d_file_space.
  The assertion worth having is available <= free <= capacity. It looks
trivial and is not: `free` and `available` differ by the root reserve, by
quotas and by container limits, and on the machine this suite was written on
the gap was 233 GiB -- 249 free against 10 available. A program sizing a write
off `free` would have overcommitted by that much.
*/
static
bool d_tests_file_space_query(struct dj_test_suite* _suite)
{
    struct dj_space_t      sp;

    d_test_suite_test(_suite, "d_file_space");

    D_TEST_CHECK_MSG(_suite,
                     "query",
                     (d_file_space(D_TEST_FILE_TEMP_DIR, &sp)) == (0),
                     "d_file_space reports the filesystem's capacity");
    D_TEST_CHECK_MSG(_suite,
                     "capacity",
                     sp.capacity > 0,
                     "the filesystem has a size");
    D_TEST_CHECK_MSG(_suite,
                     "free_bounded",
                     sp.free <= sp.capacity,
                     "free cannot exceed capacity");

    // the ordering that makes the free/available distinction real
    D_TEST_CHECK_MSG(_suite,
                     "available_bounded",
                     sp.available <= sp.free,
                     "available cannot exceed free (the root reserve)");

    // a failed query must ZERO the output, so a caller who ignores the return
    // code cannot read a plausible-looking stale number and act on it
    (void)d_file_space("no/such/path", &sp);
    D_TEST_CHECK_MSG(_suite,
                     "zeroed_on_failure",
                     (sp.capacity == 0) && (sp.free == 0) && (sp.available == 0),
                     "a failed query leaves zeroes, not stale numbers");

    D_TEST_CHECK_MSG(_suite,
                     "missing",
                     (d_file_space("no/such/path", &sp)) == (-1),
                     "d_file_space fails for a missing path");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     (d_file_space(NULL, &sp)) == (-1),
                     "d_file_space rejects a NULL path");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_out",
                     (d_file_space(D_TEST_FILE_TEMP_DIR, NULL)) == (-1),
                     "d_file_space rejects a NULL output");
    #endif

    return true;
}


/*
d_tests_file_space_all
  Runs every file_space test.
*/
bool d_tests_file_space_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "XV. file_space");

    d_tests_file_space_query(_suite);

    return true;
}

#endif  // D_FILE_SPACE_IS_AVAILABLE
