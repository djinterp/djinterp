/*******************************************************************************
* djinterp [test]                                               file_tests_dir.c
*
*   X. DIRECTORIES (file_dir) -- creation, removal, traversal, and the working
* directory. Several configured behaviours are pinned here deliberately:
* d_dir_create_parents treats an existing directory as success (MKDIR_P_EXISTING_OK);
* d_dir_read yields "." and ".." because dot-entries are NOT skipped by default,
* which the traversal test asserts rather than assumes; and d_path_resolve with a
* NULL buffer ALLOCATES (REALPATH_ALLOC) and hands back memory the caller must
* free -- it does not error.
*
*   The d_dir_set_current case is the one piece of global state in the suite: it saves
* the working directory, moves, checks the move functionally (a relative name
* now resolves), and ALWAYS restores -- using only non-fatal checks so a
* failure cannot leave the process in the wrong directory for later sections.
*
*
* path:      /tests/djinterp/c/fs/file_tests_dir.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <errno.h>
#include <stdlib.h>
#include <string.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * X.a  d_dir_create
 *****************************************************************************/

static void
d_fs_tests_dir_mkdir
(
    dtest_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    dtest_block(_suite, "d_dir_create");

    dtest_test(_suite, "creates a new directory");
    d_fs_test_path(path, sizeof(path), "mkdir_new");
    d_dir_remove(path);
    D_FS_CHECK(_suite, d_dir_create(path, S_IRWXU) == 0);
    D_FS_CHECK(_suite, d_dir_exists(path));

    dtest_test(_suite, "an existing directory -> -1, EEXIST");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_create(path, S_IRWXU) == -1);
    D_FS_CHECK(_suite, errno == EEXIST);

    d_dir_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_create(NULL, S_IRWXU) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * X.b  d_dir_create_parents
 *****************************************************************************/

static void
d_fs_tests_dir_mkdir_p
(
    dtest_suite* _suite
)
{
    char nested[D_FS_TEST_PATH_BUF];
    char middle[D_FS_TEST_PATH_BUF];
    char inner[D_FS_TEST_PATH_BUF];
    //   Declared inside the gate with the only cases that use it: every
    // case in this function is a validation case, so with checking off
    // there is nothing left here to declare it for.
    #if D_FILE_VALIDATE_IS_ACTIVE
    char tmp[D_FS_TEST_PATH_BUF];
    #endif

    dtest_block(_suite, "d_dir_create_parents");

    // build "<root>/mkdir_p_a/b/c" and remember each level for teardown
    d_path_join(middle, sizeof(middle), d_fs_test_root(), "mkdir_p_a");
    d_path_join(inner, sizeof(inner), middle, "b");
    d_path_join(nested, sizeof(nested), inner, "c");

    dtest_test(_suite, "creates a directory and its missing parents");
    D_FS_CHECK(_suite, d_dir_create_parents(nested, S_IRWXU) == 0);
    D_FS_CHECK(_suite, d_dir_exists(nested));

    dtest_test(_suite, "an existing directory tree is accepted -> 0");
    D_FS_CHECK(_suite, d_dir_create_parents(nested, S_IRWXU) == 0);

    // unwind deepest-first
    d_dir_remove(nested);
    d_dir_remove(inner);
    d_dir_remove(middle);

    dtest_test(_suite, "an empty path -> -1, ENOENT");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_create_parents("", S_IRWXU) == -1);
    D_FS_CHECK(_suite, errno == ENOENT);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    (void)tmp;
    D_FS_CHECK(_suite, d_dir_create_parents(NULL, S_IRWXU) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * X.c  d_dir_remove
 *****************************************************************************/

static void
d_fs_tests_dir_rmdir
(
    dtest_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    dtest_block(_suite, "d_dir_remove");

    dtest_test(_suite, "removes an empty directory");
    d_fs_test_path(path, sizeof(path), "rmdir_target");
    d_dir_create(path, S_IRWXU);
    D_FS_CHECK(_suite, d_dir_remove(path) == 0);
    D_FS_CHECK(_suite, !d_dir_exists(path));

    dtest_test(_suite, "a missing directory -> -1");
    D_FS_CHECK(_suite, d_dir_remove(path) == -1);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_remove(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * X.d  d_dir_open / d_dir_read / d_dir_rewind / d_dir_close
 *****************************************************************************/

static void
d_fs_tests_dir_traversal
(
    dtest_suite* _suite
)
{
    struct d_dir_t*    dir;
    struct d_dirent_t* entry;
    bool               found_fixture;
    bool               found_dot;
    bool               found_dotdot;
    bool               found_again;

    dtest_block(_suite, "d_dir_open / d_dir_read / d_dir_rewind / d_dir_close");

    dtest_test(_suite, "lists entries, dot-entries included, and rewinds");
    dir           = d_dir_open(d_fs_test_root());
    found_fixture = false;
    found_dot     = false;
    found_dotdot  = false;
    found_again   = false;

    if (dir)
    {
        while ((entry = d_dir_read(dir)) != NULL)
        {
            if (strcmp(entry->d_name, D_FS_TEST_FILE) == 0)
            {
                found_fixture = true;
            }
            else if (strcmp(entry->d_name, ".") == 0)
            {
                found_dot = true;
            }
            else if (strcmp(entry->d_name, "..") == 0)
            {
                found_dotdot = true;
            }
        }

        // rewind and confirm the fixture is seen on a second pass
        d_dir_rewind(dir);

        while ((entry = d_dir_read(dir)) != NULL)
        {
            if (strcmp(entry->d_name, D_FS_TEST_FILE) == 0)
            {
                found_again = true;
            }
        }

        D_FS_CHECK(_suite, d_dir_close(dir) == 0);
    }

    D_FS_CHECK(_suite, found_fixture);
    // dot-entries are NOT filtered out by default
    D_FS_CHECK(_suite, found_dot);
    D_FS_CHECK(_suite, found_dotdot);
    D_FS_CHECK(_suite, found_again);

    dtest_test(_suite, "opening a missing directory -> NULL");
    {
        char missing[D_FS_TEST_PATH_BUF];

        d_fs_test_path(missing, sizeof(missing), "no_such_dir");
        D_FS_CHECK(_suite, d_dir_open(missing) == NULL);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "opening NULL -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_open(NULL) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "reading a NULL handle -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_read(NULL) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * X.e  d_dir_get_current
 *****************************************************************************/

static void
d_fs_tests_dir_getcwd
(
    dtest_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF];

    dtest_block(_suite, "d_dir_get_current");

    dtest_test(_suite, "returns the buffer holding an absolute path");
    D_FS_CHECK(_suite, d_dir_get_current(buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, d_path_is_absolute(buf) != 0);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_get_current(NULL, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "zero size -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_get_current(buf, 0) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
}


/******************************************************************************
 * X.f  d_dir_set_current  (saves and restores the working directory)
 *****************************************************************************/

static void
d_fs_tests_dir_chdir
(
    dtest_suite* _suite
)
{
    char saved[D_FS_TEST_PATH_BUF];
    bool have_saved;
    bool moved;
    bool visible;

    dtest_block(_suite, "d_dir_set_current");

    // remember where we are so we can always come back
    have_saved = (d_dir_get_current(saved, sizeof(saved)) == saved);

    dtest_test(_suite, "changes directory; a relative name then resolves");
    moved   = (d_dir_set_current(d_fs_test_root()) == 0);
    // the fixture is now reachable by its bare name from inside the scratch dir
    visible = (d_file_access(D_FS_TEST_FILE, F_OK) == 0);

    // restore before asserting, so a failed assertion cannot strand the
    // process in the scratch directory for every later section
    if (have_saved)
    {
        d_dir_set_current(saved);
    }

    D_FS_CHECK(_suite, moved);
    D_FS_CHECK(_suite, visible);
    D_FS_CHECK(_suite, have_saved);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_set_current(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "a missing directory -> -1");
    {
        char missing[D_FS_TEST_PATH_BUF];

        d_fs_test_path(missing, sizeof(missing), "no_such_chdir_dir");
        D_FS_CHECK(_suite, d_dir_set_current(missing) == -1);
    }
}


/******************************************************************************
 * X.g  d_path_resolve
 *****************************************************************************/

static void
d_fs_tests_dir_realpath
(
    dtest_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    char  resolved[D_FILE_PATH_MAX] = {0};
    char* allocated;

    dtest_block(_suite, "d_path_resolve");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "resolves an existing path into the caller's buffer");
    D_FS_CHECK(_suite, d_path_resolve(path, resolved) == resolved);
    D_FS_CHECK(_suite, d_path_is_absolute(resolved) != 0);

    //   NOT gated on validation: a NULL buffer here is a documented MODE --
    // "allocate the result for me" -- not a rejected parameter. It behaves the
    // same whether or not the build checks its arguments, so gating it removed
    // a case that was never about validation and, worse, left the free() below
    // holding an uninitialised pointer.
    dtest_test(_suite, "a NULL buffer allocates the result (not an error)");
    allocated = d_path_resolve(path, NULL);
    D_FS_CHECK(_suite, allocated != NULL);
    D_FS_CHECK(_suite, (allocated != NULL) && (d_path_is_absolute(allocated)));

    // the allocated form is owned by the caller
    free(allocated);

    dtest_test(_suite, "a missing component -> NULL (every part must exist)");
    d_fs_test_path(path, sizeof(path), "no_such_realpath.txt");
    D_FS_CHECK(_suite, d_path_resolve(path, resolved) == NULL);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_resolve(NULL, resolved) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_dir
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "X. file_dir");

    d_fs_tests_dir_mkdir(_suite);
    d_fs_tests_dir_mkdir_p(_suite);
    d_fs_tests_dir_rmdir(_suite);
    d_fs_tests_dir_traversal(_suite);
    d_fs_tests_dir_getcwd(_suite);
    d_fs_tests_dir_chdir(_suite);
    d_fs_tests_dir_realpath(_suite);

    return;
}
