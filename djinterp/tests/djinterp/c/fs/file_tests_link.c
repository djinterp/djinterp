/*******************************************************************************
* djinterp [test]                                              file_tests_link.c
*
*   XIII. LINKS (file_link) -- symbolic links. The whole section is compiled
* out where the platform has no symbolic links at all (D_FILE_LINK_IS_AVAILABLE
* is 0), and the runner splices it in only then. Even where the API exists,
* actually CREATING a link can fail for want of privilege (notably on Windows),
* so the creation-dependent cases run behind a one-time runtime probe. The
* parameter-validation cases and is_symlink-on-a-regular-file run regardless,
* since they need no link to exist.
*
*
* path:      /tests/djinterp/c/fs/file_tests_link.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <errno.h>
#include <string.h>
// djinterp
#include "file_tests.h"


#if D_FILE_LINK_IS_AVAILABLE

/*
d_fs_symlinks_creatable
  One-time probe: can this process actually create a symbolic link here? The
API being present does not guarantee the privilege to use it.

Return:
  true if a probe link was created (and cleaned up), false otherwise.
*/
static bool
d_fs_symlinks_creatable
(
    void
)
{
    static int cached = -1;  // -1 unknown, 0 no, 1 yes
    char       target[D_FS_TEST_PATH_BUF];
    char       link[D_FS_TEST_PATH_BUF];

    if (cached >= 0)
    {
        return (cached == 1);
    }

    d_fs_test_path(target, sizeof(target), D_FS_TEST_FILE);
    d_fs_test_path(link, sizeof(link), "probe.symlink");
    d_file_remove(link);

    if (d_file_symlink(target, link) == 0)
    {
        cached = 1;
        d_file_remove(link);
    }
    else
    {
        cached = 0;
    }

    return (cached == 1);
}


void
d_fs_tests_link
(
    dtest_suite* _suite
)
{
    char    target[D_FS_TEST_PATH_BUF];
    char    link[D_FS_TEST_PATH_BUF];
    char    regular[D_FS_TEST_PATH_BUF];
    char    buf[D_FS_TEST_PATH_BUF];
    ssize_t n;
    bool    creatable;

    dtest_module(_suite, "XIII. file_link");

    d_fs_test_path(target, sizeof(target), D_FS_TEST_FILE);
    d_fs_test_path(link, sizeof(link), "link.symlink");
    d_fs_test_path(regular, sizeof(regular), D_FS_TEST_FILE);

    creatable = d_fs_symlinks_creatable();

    dtest_block(_suite, "d_file_symlink / d_file_readlink / d_file_is_symlink");

    if (creatable)
    {
        dtest_test(_suite, "create a link, recognise it, and read it back");
        d_file_remove(link);
        D_FS_CHECK(_suite, d_file_symlink(target, link) == 0);
        D_FS_CHECK(_suite, d_file_is_symlink(link));

        memset(buf, 0, sizeof(buf));
        n = d_file_readlink(link, buf, sizeof(buf));
        // readlink does not NUL-terminate; it returns the byte count
        D_FS_CHECK(_suite, n > 0);
        D_FS_CHECK(_suite,
                   (n == (ssize_t)strlen(target)) &&
                   (strncmp(buf, target, (size_t)n) == 0));

        d_file_remove(link);
    }

    // these hold whether or not a link could be created

    dtest_test(_suite, "a regular file is not a symlink");
    D_FS_CHECK(_suite, !d_file_is_symlink(regular));

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "is_symlink(NULL) is false, not a crash");
    D_FS_CHECK(_suite, !d_file_is_symlink(NULL));
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "d_file_symlink: NULL target -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_symlink(NULL, link) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "d_file_symlink: NULL linkpath -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_symlink(target, NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "d_file_readlink: NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_readlink(NULL, buf, sizeof(buf)) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "d_file_readlink: NULL buffer -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_readlink(regular, NULL, 10) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

    return;
#endif
}

#endif  // D_FILE_LINK_IS_AVAILABLE
