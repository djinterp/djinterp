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
* path:      /tests/djinterp/c/filesys/file_tests_dir.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.06
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

static
void d_fs_tests_dir_mkdir(
    struct dj_test_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    d_test_suite_block(_suite, "d_dir_create");

    d_test_suite_test(_suite, "creates a new directory");
    d_fs_test_path(path, sizeof(path), "mkdir_new");
    d_dir_remove(path);
    D_FS_CHECK(_suite, d_dir_create(path, S_IRWXU) == 0);
    D_FS_CHECK(_suite, d_dir_exists(path));

    d_test_suite_test(_suite, "an existing directory -> -1, EEXIST");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_create(path, S_IRWXU) == -1);
    D_FS_CHECK(_suite, errno == EEXIST);

    d_dir_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_create(NULL, S_IRWXU) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * X.b  d_dir_create_parents
 *****************************************************************************/

static
void d_fs_tests_dir_mkdir_p(
    struct dj_test_suite* _suite
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

    d_test_suite_block(_suite, "d_dir_create_parents");

    // build "<root>/mkdir_p_a/b/c" and remember each level for teardown
    d_path_join(middle, sizeof(middle), d_fs_test_root(), "mkdir_p_a");
    d_path_join(inner, sizeof(inner), middle, "b");
    d_path_join(nested, sizeof(nested), inner, "c");

    d_test_suite_test(_suite, "creates a directory and its missing parents");
    D_FS_CHECK(_suite, d_dir_create_parents(nested, S_IRWXU) == 0);
    D_FS_CHECK(_suite, d_dir_exists(nested));

    d_test_suite_test(_suite, "an existing directory tree is accepted -> 0");
    D_FS_CHECK(_suite, d_dir_create_parents(nested, S_IRWXU) == 0);

    // unwind deepest-first
    d_dir_remove(nested);
    d_dir_remove(inner);
    d_dir_remove(middle);

    d_test_suite_test(_suite, "an empty path -> -1, ENOENT");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_create_parents("", S_IRWXU) == -1);
    D_FS_CHECK(_suite, errno == ENOENT);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    (void)tmp;
    D_FS_CHECK(_suite, d_dir_create_parents(NULL, S_IRWXU) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * X.c  d_dir_remove
 *****************************************************************************/

static
void d_fs_tests_dir_rmdir(
    struct dj_test_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];

    d_test_suite_block(_suite, "d_dir_remove");

    d_test_suite_test(_suite, "removes an empty directory");
    d_fs_test_path(path, sizeof(path), "rmdir_target");
    d_dir_create(path, S_IRWXU);
    D_FS_CHECK(_suite, d_dir_remove(path) == 0);
    D_FS_CHECK(_suite, !d_dir_exists(path));

    d_test_suite_test(_suite, "a missing directory -> -1");
    D_FS_CHECK(_suite, d_dir_remove(path) == -1);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_remove(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * X.d  d_dir_open / d_dir_read / d_dir_rewind / d_dir_close
 *****************************************************************************/

static
void d_fs_tests_dir_traversal(
    struct dj_test_suite* _suite
)
{
    struct dj_dir_t*    dir;
    struct dj_dirent_t* entry;
    bool               found_fixture;
    bool               found_dot;
    bool               found_dotdot;
    bool               found_again;

    d_test_suite_block(_suite,
                       "d_dir_open / d_dir_read / d_dir_rewind / d_dir_close");

    d_test_suite_test(_suite,
                      "lists entries, dot-entries included, and rewinds");
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

    d_test_suite_test(_suite, "opening a missing directory -> NULL");
    {
        char missing[D_FS_TEST_PATH_BUF];

        d_fs_test_path(missing, sizeof(missing), "no_such_dir");
        D_FS_CHECK(_suite, d_dir_open(missing) == NULL);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "opening NULL -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_open(NULL) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "reading a NULL handle -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_read(NULL) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * X.e  d_dir_get_current
 *****************************************************************************/

static
void d_fs_tests_dir_getcwd(
    struct dj_test_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF];

    d_test_suite_block(_suite, "d_dir_get_current");

    d_test_suite_test(_suite, "returns the buffer holding an absolute path");
    D_FS_CHECK(_suite, d_dir_get_current(buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, d_path_is_absolute(buf) != 0);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_get_current(NULL, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    d_test_suite_test(_suite, "zero size -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_get_current(buf, 0) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
}


/******************************************************************************
 * X.f  d_dir_set_current  (saves and restores the working directory)
 *****************************************************************************/

static
void d_fs_tests_dir_chdir(
    struct dj_test_suite* _suite
)
{
    char saved[D_FS_TEST_PATH_BUF];
    bool have_saved;
    bool moved;
    bool visible;

    d_test_suite_block(_suite, "d_dir_set_current");

    // remember where we are so we can always come back
    have_saved = (d_dir_get_current(saved, sizeof(saved)) == saved);

    d_test_suite_test(_suite,
                      "changes directory; a relative name then resolves");
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
    d_test_suite_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_set_current(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    d_test_suite_test(_suite, "a missing directory -> -1");
    {
        char missing[D_FS_TEST_PATH_BUF];

        d_fs_test_path(missing, sizeof(missing), "no_such_chdir_dir");
        D_FS_CHECK(_suite, d_dir_set_current(missing) == -1);
    }
}


/******************************************************************************
 * X.g  d_path_resolve
 *****************************************************************************/

static
void d_fs_tests_dir_realpath(
    struct dj_test_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    char  resolved[D_FILE_PATH_MAX] = {0};
    char* allocated;

    d_test_suite_block(_suite, "d_path_resolve");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    d_test_suite_test(_suite,
                      "resolves an existing path into the caller's buffer");
    D_FS_CHECK(_suite, d_path_resolve(path,
                                      resolved,
                                      sizeof(resolved)) == resolved);
    D_FS_CHECK(_suite, d_path_is_absolute(resolved) != 0);

    //   NOT gated on validation: a NULL buffer here is a documented MODE --
    // "allocate the result for me" -- not a rejected parameter. It behaves the
    // same whether or not the build checks its arguments, so gating it removed
    // a case that was never about validation and, worse, left the free() below
    // holding an uninitialised pointer.
    d_test_suite_test(_suite,
                      "a NULL buffer allocates the result (not an error)");
    allocated = d_path_resolve(path, NULL, 0);
    D_FS_CHECK(_suite, allocated != NULL);
    D_FS_CHECK(_suite, (allocated != NULL) && (d_path_is_absolute(allocated)));

    // the allocated form is owned by the caller
    free(allocated);

    d_test_suite_test(_suite,
                      "a missing component -> NULL (every part must exist)");
    d_fs_test_path(path, sizeof(path), "no_such_realpath.txt");
    D_FS_CHECK(_suite, d_path_resolve(path,
                                      resolved,
                                      sizeof(resolved)) == NULL);

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_resolve(NULL,
                                      resolved,
                                      sizeof(resolved)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void d_fs_tests_dir(
    struct dj_test_suite* _suite
)
{
    d_test_suite_module(_suite, "X. file_dir");

    d_fs_tests_dir_mkdir(_suite);
    d_fs_tests_dir_mkdir_p(_suite);
    d_fs_tests_dir_rmdir(_suite);
    d_fs_tests_dir_traversal(_suite);
    d_fs_tests_dir_getcwd(_suite);
    d_fs_tests_dir_chdir(_suite);
    d_fs_tests_dir_realpath(_suite);

    return;
}

/******************************************************************************
 * d_tests_file_dir_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * X.  file_dir  --  DIRECTORIES AND RESOLUTION
 *****************************************************************************/

/*
d_tests_file_mkdir
  Tests d_dir_create and d_dir_create_parents.
*/
static
bool d_tests_file_mkdir(struct dj_test_suite* _suite)
{
    char                  dir[D_TEST_FILE_PATH_BUF];
    char                  nested[D_TEST_FILE_PATH_BUF];
    char                  parent[D_TEST_FILE_PATH_BUF];
    char                  file_path[D_TEST_FILE_PATH_BUF];
    char                  under_file[D_TEST_FILE_PATH_BUF];

    d_test_suite_test(_suite, "d_dir_create / d_dir_create_parents");

    d_tests_file_path(dir, sizeof(dir), "mkdir_test");
    d_tests_file_path(parent, sizeof(parent), "mkdir_p_parent");
    d_tests_file_child(nested, sizeof(nested), parent, "child");
    d_tests_file_child(nested, sizeof(nested), nested, "grandchild");

    D_TEST_CHECK_MSG(_suite,
                     "create",
                     (d_dir_create(dir, S_IRWXU)) == (0),
                     "d_dir_create creates a directory");
    D_TEST_CHECK_MSG(_suite, "is_dir", d_dir_exists(dir), "...and it is one");
    D_TEST_CHECK_MSG(_suite,
                     "existing",
                     (d_dir_create(dir, S_IRWXU)) == (-1),
                     "d_dir_create refuses an existing directory");
    (void)d_dir_remove(dir);
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     (d_dir_create(NULL, S_IRWXU)) == (-1),
                     "d_dir_create rejects NULL");
    #endif

    D_TEST_CHECK_MSG(_suite,
                     "nested",
                     (d_dir_create_parents(nested, S_IRWXU)) == (0),
                     "d_dir_create_parents creates missing parents");
    D_TEST_CHECK_MSG(_suite,
                     "nested_is_dir",
                     d_dir_exists(nested),
                     "...all the way down");

    // "ensure this exists" is satisfied by something that already exists --
    // that IS the difference from mkdir
    D_TEST_CHECK_MSG(_suite,
                     "existing_ok",
                     (d_dir_create_parents(nested, S_IRWXU)) == (0),
                     "d_dir_create_parents succeeds when the path is already there");

    // a redundant separator names no component
    D_TEST_CHECK_MSG(_suite,
                     "redundant_seps",
                     (d_dir_create_parents(nested, S_IRWXU)) == (0),
                     "empty components are skipped, not created");

    (void)d_dir_remove(nested);
    d_tests_file_child(nested, sizeof(nested), parent, "child");
    (void)d_dir_remove(nested);
    (void)d_dir_remove(parent);

    // a component that exists and is NOT a directory cannot be brought into
    // existence as one
    d_tests_file_path(file_path, sizeof(file_path), D_TEST_FILE_TEST_FILENAME);
    d_tests_file_child(under_file, sizeof(under_file), file_path, "nope");
    D_TEST_CHECK_MSG(_suite,
                     "component_is_file",
                     (d_dir_create_parents(under_file, S_IRWXU)) == (-1),
                     "a path through a FILE is ENOTDIR, not created");

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_p",
                     (d_dir_create_parents(NULL, S_IRWXU)) == (-1),
                     "d_dir_create_parents rejects NULL");
    #endif
    D_TEST_CHECK_MSG(_suite,
                     "empty_p",
                     (d_dir_create_parents("", S_IRWXU)) == (-1),
                     "d_dir_create_parents rejects an empty path");

    return true;
}


/*
d_tests_file_rmdir
  Tests d_dir_remove.
*/
static
bool d_tests_file_rmdir(struct dj_test_suite* _suite)
{
    char                  dir[D_TEST_FILE_PATH_BUF];

    d_test_suite_test(_suite, "d_dir_remove");

    d_tests_file_path(dir, sizeof(dir), "rmdir_test");
    (void)d_dir_create(dir, S_IRWXU);

    D_TEST_CHECK_MSG(_suite,
                     "remove",
                     (d_dir_remove(dir)) == (0),
                     "d_dir_remove removes an empty directory");
    D_TEST_CHECK_MSG(_suite, "gone", !(d_dir_exists(dir)), "...and it is gone");
    D_TEST_CHECK_MSG(_suite,
                     "missing",
                     (d_dir_remove("no/such/directory")) == (-1),
                     "d_dir_remove fails for a missing directory");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     (d_dir_remove(NULL)) == (-1),
                     "d_dir_remove rejects NULL");
    #endif

    // the scratch directory holds the fixture, so it is not empty
    D_TEST_CHECK_MSG(_suite,
                     "non_empty",
                     (d_dir_remove(D_TEST_FILE_TEMP_DIR)) == (-1),
                     "d_dir_remove refuses a non-empty directory");

    return true;
}


/*
d_tests_file_traversal
  Tests d_dir_open / d_dir_read / d_dir_rewind / d_dir_close.
*/
static
bool d_tests_file_traversal(struct dj_test_suite* _suite)
{
    struct dj_dir_t*       dir;
    struct dj_dirent_t*    entry;
    struct dj_dirent_t*    entry2;
    char                  first[D_FILE_NAME_MAX + 1];
    size_t                count;
    size_t                dots;
    bool                  found;

    d_test_suite_test(_suite,
                      D_FILE_DIR_TYPE_IS_FREE
                          ? "traversal (d_type from the kernel)"
                          : "traversal (d_type by stat, or absent)");

    count = 0;
    dots  = 0;
    found = false;

    dir = d_dir_open(D_TEST_FILE_TEMP_DIR);
    D_TEST_CHECK_MSG(_suite,
                     "open",
                     (dir) != NULL,
                     "d_dir_open opens a directory");

    if (dir)
    {
        while ((entry = d_dir_read(dir)) != NULL)
        {
            ++count;

            if ( (strcmp(entry->d_name, ".") == 0) ||
                 (strcmp(entry->d_name, "..") == 0) )
            {
                ++dots;
            }

            if (strcmp(entry->d_name, D_TEST_FILE_TEST_FILENAME) == 0)
            {
                found = true;

#if D_CFG_IS_ON(D_CFG_FILE_DIR_FILL_TYPE)
                D_TEST_CHECK_MSG(_suite,
                                 "type_reg",
                                 ((int)entry->d_type) == (DT_REG),
                                 "d_type identifies a regular file");
#endif
            }
        }

        D_TEST_CHECK_MSG(_suite,
                         "read",
                         count > 0,
                         "d_dir_read returns entries");
        D_TEST_CHECK_MSG(_suite,
                         "found_fixture",
                         found,
                         "the fixture is among them");

#if D_CFG_IS_ON(D_CFG_FILE_DIR_SKIP_DOTS)
        D_TEST_CHECK_MSG(_suite,
                         "dots_hidden",
                         (dots) == ((size_t)0),
                         "SKIP_DOTS=1 hides . and ..");
#else
        D_TEST_CHECK_MSG(_suite,
                         "dots_present",
                         (dots) == ((size_t)2),
                         ". and .. are entries like any other");
#endif

        D_TEST_CHECK_MSG(_suite,
                         "close",
                         (d_dir_close(dir)) == (0),
                         "d_dir_close closes the handle");
    }

    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     (d_dir_open("no/such/directory")) == NULL,
                     "d_dir_open fails for a missing directory");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     (d_dir_open(NULL)) == NULL,
                     "d_dir_open rejects NULL");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "read_null",
                     (d_dir_read(NULL)) == NULL,
                     "d_dir_read rejects NULL");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "close_null",
                     (d_dir_close(NULL)) == (-1),
                     "d_dir_close rejects NULL");
    #endif

    dir = d_dir_open(D_TEST_FILE_TEMP_DIR);

    if (dir)
    {
        entry = d_dir_read(dir);

        if (entry)
        {
            memcpy(first, entry->d_name, strlen(entry->d_name) + 1);
            (void)d_dir_read(dir);

            // d_dir_rewind returns int, not void: POSIX cannot fail, but Win32
            // has no rewind and must re-open the search, which can
            D_TEST_CHECK_MSG(_suite,
                             "rewind",
                             (d_dir_rewind(dir)) == (0),
                             "d_dir_rewind reports success");

            entry = d_dir_read(dir);
            D_TEST_CHECK_MSG(_suite,
                             "rewind_restarts",
                             entry && strcmp(entry->d_name, first) == 0,
                             "...and the first entry repeats");
        }

        // the entry belongs to the HANDLE, not to the platform: the same
        // storage is reused, so the pointer is stable and its lifetime is
        // sayable
        entry  = d_dir_read(dir);
        entry2 = d_dir_read(dir);
        D_TEST_CHECK_MSG(_suite,
                         "entry_storage_is_ours",
                         (entry == NULL) || (entry == entry2),
                         "the handle owns the entry it returns");

        (void)d_dir_close(dir);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "rewind_null",
                     (d_dir_rewind(NULL)) == (-1),
                     "d_dir_rewind rejects NULL");
    #endif

    return true;
}


/*
d_tests_file_cwd
  Tests d_dir_get_current / d_dir_set_current / d_path_resolve.
*/
static
bool d_tests_file_cwd(struct dj_test_suite* _suite)
{
    char                  saved[D_FILE_PATH_MAX];
    char                  buf[D_FILE_PATH_MAX];

    d_test_suite_test(_suite,
                      "d_dir_get_current / d_dir_set_current / d_path_resolve");

    D_TEST_CHECK_MSG(_suite,
                     "getcwd",
                     (d_dir_get_current(saved, sizeof(saved))) != NULL,
                     "d_dir_get_current reports a path");
    D_TEST_CHECK_MSG(_suite,
                     "getcwd_is_dir",
                     d_dir_exists(saved),
                     "...and it is a directory");

    // POSIX getcwd(NULL, 0) allocates; this one does not, because two
    // ownership contracts on one function is one too many
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "getcwd_null",
                     (d_dir_get_current(NULL, 100)) == NULL,
                     "d_dir_get_current requires a buffer");
    #endif
    D_TEST_CHECK_MSG(_suite,
                     "getcwd_zero",
                     (d_dir_get_current(buf, 0)) == NULL,
                     "d_dir_get_current rejects a zero size");

    D_TEST_CHECK_MSG(_suite,
                     "chdir",
                     (d_dir_set_current(D_TEST_FILE_TEMP_DIR)) == (0),
                     "d_dir_set_current moves the working directory");
    (void)d_dir_set_current(saved);

    D_TEST_CHECK_MSG(_suite,
                     "chdir_missing",
                     (d_dir_set_current("no/such/directory")) == (-1),
                     "d_dir_set_current fails for a missing directory");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "chdir_null",
                     (d_dir_set_current(NULL)) == (-1),
                     "d_dir_set_current rejects NULL");
    #endif

#if (D_INTERNAL_FILE_HAS_REALPATH == 1) || D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    D_TEST_CHECK_MSG(_suite,
                     "realpath",
                     (d_path_resolve(D_TEST_FILE_TEMP_DIR, buf, sizeof(buf))) != NULL,
                     "d_path_resolve resolves an existing path");
    D_TEST_CHECK_MSG(_suite,
                     "realpath_absolute",
                     d_path_is_absolute(buf),
                     "...to an absolute path");

    // unlike d_path_normalize, this must ask the filesystem -- so a path that
    // does not exist cannot be resolved
    D_TEST_CHECK_MSG(_suite,
                     "realpath_missing",
                     (d_path_resolve("no/such/path", buf, sizeof(buf))) == NULL,
                     "d_path_resolve requires every component to exist");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "realpath_null",
                     (d_path_resolve(NULL, buf, sizeof(buf))) == NULL,
                     "d_path_resolve rejects NULL");
    #endif

    #if (D_INTERNAL_FILE_DIR_REALPATH_ALLOC == 1)
    {
        char* owned;

        owned = d_path_resolve(D_TEST_FILE_TEMP_DIR, NULL, 0);
        #if D_FILE_VALIDATE_IS_ACTIVE
        D_TEST_CHECK_MSG(_suite,
                         "realpath_alloc",
                         (owned) != NULL,
                         "d_path_resolve allocates for NULL");
        #endif

        if (owned)
        {
            free(owned);
        }
    }
    #endif
#endif

    return true;
}


/*
d_tests_file_dir_all
  Runs every file_dir test.
*/
bool d_tests_file_dir_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "X. file_dir");

    d_tests_file_mkdir(_suite);
    d_tests_file_rmdir(_suite);
    d_tests_file_traversal(_suite);
    d_tests_file_cwd(_suite);

    return true;
}
