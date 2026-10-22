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
*                                                            revised: 2026.10.20
*******************************************************************************/

// std
#include <errno.h>
#include <string.h>
// djinterp
#include "file_tests.h"


void
d_fs_tests_space
(
    dtest_suite* _suite
)
{
    struct d_space_t space;
    char             path[D_FS_TEST_PATH_BUF];
    bool             sane;

    dtest_module(_suite, "XV. file_space");

    dtest_block(_suite, "d_file_space");

    dtest_test(_suite, "reports coherent capacity for an existing path");
    memset(&space, 0, sizeof(space));
    sane = (d_file_space(d_fs_test_root(), &space) == 0);
    // some capacity exists, and neither free figure exceeds the total
    sane = sane &&
           (space.capacity > 0) &&
           (space.free <= space.capacity) &&
           (space.available <= space.capacity);
    D_FS_CHECK(_suite, sane);

    dtest_test(_suite, "a missing path -> -1 and a zeroed result");
    d_fs_test_path(path, sizeof(path), "no_such_space_path");
    memset(&space, 0xFF, sizeof(space));
    D_FS_CHECK(_suite, d_file_space(path, &space) == -1);
    // the out-parameter is cleared on failure
    D_FS_CHECK(_suite, space.capacity == 0);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_space(NULL, &space) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL out-parameter -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_space(d_fs_test_root(), NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

    return;
#endif
}
