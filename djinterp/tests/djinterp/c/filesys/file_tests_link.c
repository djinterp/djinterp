/*******************************************************************************
* djinterp [c][filesys]                                        file_tests_link.c
*
*   XIII. Links (file_link) -- symbolic links. The whole section is compiled
* out where the platform has no symbolic links at all (D_FILE_LINK_IS_AVAILABLE
* is 0), and the runner splices it in only then. Even where the API exists,
* actually creating a link can fail for want of privilege (notably on Windows),
* so the creation-dependent cases run behind a one-time runtime probe. The
* parameter-validation cases and is_symlink-on-a-regular-file run regardless,
* since they need no link to exist.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_link.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.06
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
static
bool d_fs_symlinks_creatable(void)
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


void d_fs_tests_link(
    struct dj_test_suite* _suite
)
{
    char    target[D_FS_TEST_PATH_BUF];
    char    link[D_FS_TEST_PATH_BUF];
    char    regular[D_FS_TEST_PATH_BUF];
    char    buf[D_FS_TEST_PATH_BUF];
    ssize_t n;
    bool    creatable;

    d_test_suite_module(_suite, "XIII. file_link");

    d_fs_test_path(target, sizeof(target), D_FS_TEST_FILE);
    d_fs_test_path(link, sizeof(link), "link.symlink");
    d_fs_test_path(regular, sizeof(regular), D_FS_TEST_FILE);

    creatable = d_fs_symlinks_creatable();

    d_test_suite_block(_suite,
                       "d_file_symlink / d_file_readlink / d_file_is_symlink");

    if (creatable)
    {
        d_test_suite_test(_suite,
                          "create a link, recognise it, and read it back");
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

    d_test_suite_test(_suite, "a regular file is not a symlink");
    D_FS_CHECK(_suite, !d_file_is_symlink(regular));

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "is_symlink(NULL) is false, not a crash");
    D_FS_CHECK(_suite, !d_file_is_symlink(NULL));
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "d_file_symlink: NULL target -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_symlink(NULL, link) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "d_file_symlink: NULL linkpath -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_symlink(target, NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "d_file_readlink: NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_readlink(NULL, buf, sizeof(buf)) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "d_file_readlink: NULL buffer -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_readlink(regular, NULL, 10) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

    return;
#endif
}

#endif  // D_FILE_LINK_IS_AVAILABLE

/******************************************************************************
 * d_tests_file_link_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * XIII.  file_link  --  SYMBOLIC LINKS
 *
 *   THIS IS THE SUITE THAT WAS LYING.
 *
 *   The old dfile_tests_sa_symlink.c did this, five times:
 *       else
 *       {
 *           // symlinks not available (Windows without admin privileges)
 *           // pass these tests as they cannot be performed
 *           test_create = true;
 *           test_link_exists = true;
 *       }
 *   Five assertions reported GREEN for code that never executed, under names
 *   like "d_file_symlink creates symbolic link (skipped: no privilege)". A reader
 *   scanning the output sees PASS. CI sees PASS. The claim is false.
 *
 *   The framework has no third state -- dj_test_object carries a bool -- so
 *   there is no honest way to say "skipped" inside an assertion. The honest
 *   way is not to make the assertion. This suite therefore OMITS what it
 *   cannot run: nothing green, nothing red, nothing claimed, and the group
 *   name says why.
 *
 *   TWO SEPARATE QUESTIONS, which the old suite conflated into one runtime
 *   probe:
 *     1. Does the PLATFORM have symbolic links?  -> D_FILE_LINK_IS_AVAILABLE,
 *        a compile-time fact the config already resolved. The old suite
 *        answered this by creating a symlink to see whether it could create a
 *        symlink, which is a test of the probe.
 *     2. May THIS PROCESS create one?  -> genuinely runtime, and only on
 *        Windows, where it needs SeCreateSymbolicLinkPrivilege that an
 *        ordinary account does not have. That one still has to be tried --
 *        but its failure is an omission, not a pass.
 *****************************************************************************/

#if D_FILE_LINK_IS_AVAILABLE

/*
d_tests_file_link_permitted
  Reports whether this PROCESS may create a symbolic link, by trying once and
caching the answer.
  Note what this is not: it is not a test of whether symlinks exist. That is
D_FILE_LINK_IS_AVAILABLE and it was answered at compile time. This asks the
one question a macro cannot -- whether the account running the suite holds
SeCreateSymbolicLinkPrivilege -- and it exists only so the tests that need the
privilege can be omitted rather than failed.

Parameter(s):
  none.
Return:
  true when a link can be created, false otherwise.
*/
static
bool d_tests_file_link_permitted(void)
{
    static int cached = -1;   // -1 untested, 0 no, 1 yes
    char       target[D_TEST_FILE_PATH_BUF];
    char       link[D_TEST_FILE_PATH_BUF];

    if (cached >= 0)
    {
        return (cached == 1);
    }

    cached = 0;

    if (!d_tests_file_path(target, sizeof(target), D_TEST_FILE_TEST_FILENAME))
    {
        return false;
    }

    if (!d_tests_file_path(link, sizeof(link), "probe.lnk"))
    {
        return false;
    }

    (void)d_file_unlink(link);

    if (d_file_symlink(target, link) == 0)
    {
        (void)d_file_unlink(link);
        cached = 1;
    }

    return (cached == 1);
}


/*
d_tests_file_symlink
  Tests d_file_symlink.
  Tests the following:
  - creates a link, and d_file_is_symlink sees it   (only where permitted)
  - rejects a NULL target
  - rejects a NULL link path
*/
static
bool d_tests_file_symlink(struct dj_test_suite* _suite)
{
    char                  target[D_TEST_FILE_PATH_BUF];
    char                  link[D_TEST_FILE_PATH_BUF];

    d_test_suite_test(_suite,
                      d_tests_file_link_permitted()
                          ? "d_file_symlink"
                          : "d_file_symlink (creation omitted: no privilege)");

    d_tests_file_path(target, sizeof(target), D_TEST_FILE_TEST_FILENAME);
    d_tests_file_path(link, sizeof(link), "symlink_test.lnk");

    // the privileged half. Where the process cannot create a link, these
    // assertions are ABSENT -- not green. The group simply has fewer.
    if (d_tests_file_link_permitted())
    {
        (void)d_file_unlink(link);

        D_TEST_CHECK_MSG(_suite,
                         "create",
                         (d_file_symlink(target, link)) == (0),
                         "d_file_symlink creates a symbolic link");
        D_TEST_CHECK_MSG(_suite,
                         "exists",
                         d_file_is_symlink(link),
                         "the created link is seen as a link");

        // a link's target is not resolved at creation, so a dangling one is
        // a legitimate object rather than an error
        {
            char dangling[D_TEST_FILE_PATH_BUF];

            d_tests_file_path(dangling, sizeof(dangling), "dangling.lnk");
            (void)d_file_unlink(dangling);

            D_TEST_CHECK_MSG(_suite,
                             "dangling",
                             (d_file_symlink("no/such/target", dangling)) == (0),
                             "a link to a nonexistent target is legal");
            D_TEST_CHECK_MSG(_suite,
                             "dangling_is_link",
                             d_file_is_symlink(dangling),
                             "...and is still a link");
            (void)d_file_unlink(dangling);
        }

        (void)d_file_unlink(link);
    }

    // the unprivileged half: these need no link, so they always run
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_target",
                     (d_file_symlink(NULL, link)) == (-1),
                     "d_file_symlink rejects a NULL target");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_linkpath",
                     (d_file_symlink(target, NULL)) == (-1),
                     "d_file_symlink rejects a NULL link path");
    #endif

    return true;
}


/*
d_tests_file_readlink
  Tests d_file_readlink.
  Tests the following:
  - reads a link's target text     (only where permitted)
  - a non-symlink is EINVAL
  - NULL is rejected
*/
static
bool d_tests_file_readlink(struct dj_test_suite* _suite)
{
    char                  target[D_TEST_FILE_PATH_BUF];
    char                  link[D_TEST_FILE_PATH_BUF];
    char                  read_buf[D_TEST_FILE_PATH_BUF];
    ssize_t               got;

    d_test_suite_test(_suite,
                      d_tests_file_link_permitted()
                          ? "d_file_readlink"
                          : "d_file_readlink (reading omitted: no privilege)");

    d_tests_file_path(target, sizeof(target), D_TEST_FILE_TEST_FILENAME);
    d_tests_file_path(link, sizeof(link), "readlink_test.lnk");

    if (d_tests_file_link_permitted())
    {
        (void)d_file_unlink(link);

        if (d_file_symlink(target, link) == 0)
        {
            memset(read_buf, 0, sizeof(read_buf));

            // readlink does NOT NUL-terminate and truncates silently, so the
            // caller must pass bufsize-1 and terminate at the returned length
            got = d_file_readlink(link, read_buf, sizeof(read_buf) - 1);

            D_TEST_CHECK_MSG(_suite,
                             "read",
                             got > 0,
                             "d_file_readlink returns a byte count");

            if (got > 0)
            {
                read_buf[got] = '\0';
            }

            D_TEST_CHECK_MSG(_suite,
                             "target_text",
                             strstr(read_buf, D_TEST_FILE_TEST_FILENAME) != NULL,
                             "the target text comes back intact");

            (void)d_file_unlink(link);
        }
    }

    // a regular file is not a link, and readlink must say EINVAL rather than
    // describing the file
    D_TEST_CHECK_MSG(_suite,
                     "non_symlink",
                     (d_file_readlink(target, read_buf, sizeof(read_buf))) == ((ssize_t)-1),
                     "d_file_readlink rejects a non-link");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     (d_file_readlink(NULL, read_buf, sizeof(read_buf))) == ((ssize_t)-1),
                     "d_file_readlink rejects a NULL path");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_buf",
                     (d_file_readlink(target, NULL, 10)) == ((ssize_t)-1),
                     "d_file_readlink rejects a NULL buffer");
    #endif

    return true;
}


/*
d_tests_file_is_symlink
  Tests d_file_is_symlink.
  Tests the following:
  - a link is a link                (only where permitted)
  - a regular file is not
  - a directory is not
  - NULL is not
*/
static
bool d_tests_file_is_symlink(struct dj_test_suite* _suite)
{
    char                  target[D_TEST_FILE_PATH_BUF];
    char                  link[D_TEST_FILE_PATH_BUF];

    d_test_suite_test(_suite,
                      d_tests_file_link_permitted()
                          ? "d_file_is_symlink"
                          : "d_file_is_symlink (positive case omitted: no privilege)");

    d_tests_file_path(target, sizeof(target), D_TEST_FILE_TEST_FILENAME);
    d_tests_file_path(link, sizeof(link), "is_symlink_test.lnk");

    if (d_tests_file_link_permitted())
    {
        (void)d_file_unlink(link);

        if (d_file_symlink(target, link) == 0)
        {
            D_TEST_CHECK_MSG(_suite,
                             "symlink",
                             d_file_is_symlink(link),
                             "d_file_is_symlink sees a link");
            (void)d_file_unlink(link);
        }
    }

    D_TEST_CHECK_MSG(_suite,
                     "regular_file",
                     !(d_file_is_symlink(target)),
                     "a regular file is not a link");
    D_TEST_CHECK_MSG(_suite,
                     "directory",
                     !(d_file_is_symlink(D_TEST_FILE_TEMP_DIR)),
                     "a directory is not a link");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     !(d_file_is_symlink(NULL)),
                     "NULL is not a link");
    #endif

    return true;
}


/*
d_tests_file_link_all
  Runs every file_link test.
*/
bool d_tests_file_link_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "XIII. file_link");

    d_tests_file_symlink(_suite);
    d_tests_file_readlink(_suite);
    d_tests_file_is_symlink(_suite);

    return true;
}

#endif  // D_FILE_LINK_IS_AVAILABLE
