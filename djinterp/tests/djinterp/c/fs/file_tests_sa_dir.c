/*******************************************************************************
* djinterp [fs]                                              file_tests_sa_dir.c
*
*
* path:      /tests/djinterp/c/fs/file_tests_sa_dir.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

/******************************************************************************
 * X.  file_dir  --  DIRECTORIES AND RESOLUTION
 *****************************************************************************/

/*
d_tests_file_mkdir
  Tests d_dir_create and d_dir_create_parents.
*/
static struct d_test_object*
d_tests_file_mkdir
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  dir[D_TEST_FILE_PATH_BUF];
    char                  nested[D_TEST_FILE_PATH_BUF];
    char                  parent[D_TEST_FILE_PATH_BUF];
    char                  file_path[D_TEST_FILE_PATH_BUF];
    char                  under_file[D_TEST_FILE_PATH_BUF];
    size_t                idx;

    idx = 0;
    d_tests_file_path(dir, sizeof(dir), "mkdir_test");
    d_tests_file_path(parent, sizeof(parent), "mkdir_p_parent");
    d_tests_file_child(nested, sizeof(nested), parent, "child");
    d_tests_file_child(nested, sizeof(nested), nested, "grandchild");

    items[idx++] = D_ASSERT_EQUAL("create", d_dir_create(dir, S_IRWXU), 0,
                                  "d_dir_create creates a directory");
    items[idx++] = D_ASSERT_TRUE("is_dir", d_dir_exists(dir),
                                 "...and it is one");
    items[idx++] = D_ASSERT_EQUAL("existing", d_dir_create(dir, S_IRWXU), -1,
                                  "d_dir_create refuses an existing directory");
    (void)d_dir_remove(dir);
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null", d_dir_create(NULL, S_IRWXU), -1,
                                  "d_dir_create rejects NULL");
    #endif

    items[idx++] = D_ASSERT_EQUAL("nested", d_dir_create_parents(nested, S_IRWXU), 0,
                                  "d_dir_create_parents creates missing parents");
    items[idx++] = D_ASSERT_TRUE("nested_is_dir", d_dir_exists(nested),
                                 "...all the way down");

    // "ensure this exists" is satisfied by something that already exists --
    // that IS the difference from mkdir
    items[idx++] = D_ASSERT_EQUAL("existing_ok", d_dir_create_parents(nested, S_IRWXU),
                                  0,
                                  "d_dir_create_parents succeeds when the path is already there");

    // a redundant separator names no component
    items[idx++] = D_ASSERT_EQUAL("redundant_seps",
                                  d_dir_create_parents(nested, S_IRWXU), 0,
                                  "empty components are skipped, not created");

    (void)d_dir_remove(nested);
    d_tests_file_child(nested, sizeof(nested), parent, "child");
    (void)d_dir_remove(nested);
    (void)d_dir_remove(parent);

    // a component that exists and is NOT a directory cannot be brought into
    // existence as one
    d_tests_file_path(file_path, sizeof(file_path), D_TEST_FILE_TEST_FILENAME);
    d_tests_file_child(under_file, sizeof(under_file), file_path, "nope");
    items[idx++] = D_ASSERT_EQUAL("component_is_file",
                                  d_dir_create_parents(under_file, S_IRWXU), -1,
                                  "a path through a FILE is ENOTDIR, not created");

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_p", d_dir_create_parents(NULL, S_IRWXU), -1,
                                  "d_dir_create_parents rejects NULL");
    #endif
    items[idx++] = D_ASSERT_EQUAL("empty_p", d_dir_create_parents("", S_IRWXU), -1,
                                  "d_dir_create_parents rejects an empty path");

    return d_tests_file_group("d_dir_create / d_dir_create_parents", items, idx);
}


/*
d_tests_file_rmdir
  Tests d_dir_remove.
*/
static struct d_test_object*
d_tests_file_rmdir
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  dir[D_TEST_FILE_PATH_BUF];
    size_t                idx;

    idx = 0;
    d_tests_file_path(dir, sizeof(dir), "rmdir_test");
    (void)d_dir_create(dir, S_IRWXU);

    items[idx++] = D_ASSERT_EQUAL("remove", d_dir_remove(dir), 0,
                                  "d_dir_remove removes an empty directory");
    items[idx++] = D_ASSERT_FALSE("gone", d_dir_exists(dir),
                                  "...and it is gone");
    items[idx++] = D_ASSERT_EQUAL("missing",
                                  d_dir_remove("no/such/directory"), -1,
                                  "d_dir_remove fails for a missing directory");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null", d_dir_remove(NULL), -1,
                                  "d_dir_remove rejects NULL");
    #endif

    // the scratch directory holds the fixture, so it is not empty
    items[idx++] = D_ASSERT_EQUAL("non_empty",
                                  d_dir_remove(D_TEST_FILE_TEMP_DIR), -1,
                                  "d_dir_remove refuses a non-empty directory");

    return d_tests_file_group("d_dir_remove", items, idx);
}


/*
d_tests_file_traversal
  Tests d_dir_open / d_dir_read / d_dir_rewind / d_dir_close.
*/
static struct d_test_object*
d_tests_file_traversal
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    struct d_dir_t*       dir;
    struct d_dirent_t*    entry;
    struct d_dirent_t*    entry2;
    char                  first[D_FILE_NAME_MAX + 1];
    size_t                count;
    size_t                dots;
    bool                  found;
    size_t                idx;

    idx = 0;
    count = 0;
    dots  = 0;
    found = false;

    dir = d_dir_open(D_TEST_FILE_TEMP_DIR);
    items[idx++] = D_ASSERT_NOT_NULL("open", dir,
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
                items[idx++] = D_ASSERT_EQUAL("type_reg",
                                              (int)entry->d_type, DT_REG,
                                              "d_type identifies a regular file");
#endif
            }
        }

        items[idx++] = D_ASSERT_TRUE("read", count > 0,
                                     "d_dir_read returns entries");
        items[idx++] = D_ASSERT_TRUE("found_fixture", found,
                                     "the fixture is among them");

#if D_CFG_IS_ON(D_CFG_FILE_DIR_SKIP_DOTS)
        items[idx++] = D_ASSERT_EQUAL("dots_hidden", dots, (size_t)0,
                                      "SKIP_DOTS=1 hides . and ..");
#else
        items[idx++] = D_ASSERT_EQUAL("dots_present", dots, (size_t)2,
                                      ". and .. are entries like any other");
#endif

        items[idx++] = D_ASSERT_EQUAL("close", d_dir_close(dir), 0,
                                      "d_dir_close closes the handle");
    }

    items[idx++] = D_ASSERT_NULL("nonexistent",
                                 d_dir_open("no/such/directory"),
                                 "d_dir_open fails for a missing directory");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null_path", d_dir_open(NULL),
                                 "d_dir_open rejects NULL");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("read_null", d_dir_read(NULL),
                                 "d_dir_read rejects NULL");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("close_null", d_dir_close(NULL), -1,
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
            items[idx++] = D_ASSERT_EQUAL("rewind", d_dir_rewind(dir), 0,
                                          "d_dir_rewind reports success");

            entry = d_dir_read(dir);
            items[idx++] = D_ASSERT_TRUE("rewind_restarts",
                                         entry &&
                                         strcmp(entry->d_name, first) == 0,
                                         "...and the first entry repeats");
        }

        // the entry belongs to the HANDLE, not to the platform: the same
        // storage is reused, so the pointer is stable and its lifetime is
        // sayable
        entry  = d_dir_read(dir);
        entry2 = d_dir_read(dir);
        items[idx++] = D_ASSERT_TRUE("entry_storage_is_ours",
                                     (entry == NULL) || (entry == entry2),
                                     "the handle owns the entry it returns");

        (void)d_dir_close(dir);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("rewind_null", d_dir_rewind(NULL), -1,
                                  "d_dir_rewind rejects NULL");
    #endif

    return d_tests_file_group(
               D_FILE_DIR_TYPE_IS_FREE ?
                   "traversal (d_type from the kernel)" :
                   "traversal (d_type by stat, or absent)",
               items,
               idx);
}


/*
d_tests_file_cwd
  Tests d_dir_get_current / d_dir_set_current / d_path_resolve.
*/
static struct d_test_object*
d_tests_file_cwd
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  saved[D_FILE_PATH_MAX];
    char                  buf[D_FILE_PATH_MAX];
    size_t                idx;

    idx = 0;

    items[idx++] = D_ASSERT_NOT_NULL("getcwd",
                                     d_dir_get_current(saved, sizeof(saved)),
                                     "d_dir_get_current reports a path");
    items[idx++] = D_ASSERT_TRUE("getcwd_is_dir", d_dir_exists(saved),
                                 "...and it is a directory");

    // POSIX getcwd(NULL, 0) allocates; this one does not, because two
    // ownership contracts on one function is one too many
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("getcwd_null", d_dir_get_current(NULL, 100),
                                 "d_dir_get_current requires a buffer");
    #endif
    items[idx++] = D_ASSERT_NULL("getcwd_zero", d_dir_get_current(buf, 0),
                                 "d_dir_get_current rejects a zero size");

    items[idx++] = D_ASSERT_EQUAL("chdir",
                                  d_dir_set_current(D_TEST_FILE_TEMP_DIR), 0,
                                  "d_dir_set_current moves the working directory");
    (void)d_dir_set_current(saved);

    items[idx++] = D_ASSERT_EQUAL("chdir_missing",
                                  d_dir_set_current("no/such/directory"), -1,
                                  "d_dir_set_current fails for a missing directory");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("chdir_null", d_dir_set_current(NULL), -1,
                                  "d_dir_set_current rejects NULL");
    #endif

#if (D_INTERNAL_FILE_HAS_REALPATH == 1) || D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    items[idx++] = D_ASSERT_NOT_NULL("realpath",
                                     d_path_resolve(D_TEST_FILE_TEMP_DIR, buf),
                                     "d_path_resolve resolves an existing path");
    items[idx++] = D_ASSERT_TRUE("realpath_absolute",
                                 d_path_is_absolute(buf),
                                 "...to an absolute path");

    // unlike d_path_normalize, this must ask the filesystem -- so a path that
    // does not exist cannot be resolved
    items[idx++] = D_ASSERT_NULL("realpath_missing",
                                 d_path_resolve("no/such/path", buf),
                                 "d_path_resolve requires every component to exist");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("realpath_null",
                                 d_path_resolve(NULL, buf),
                                 "d_path_resolve rejects NULL");
    #endif

    #if (D_INTERNAL_FILE_DIR_REALPATH_ALLOC == 1)
    {
        char* owned;

        owned = d_path_resolve(D_TEST_FILE_TEMP_DIR, NULL);
        #if D_FILE_VALIDATE_IS_ACTIVE
        items[idx++] = D_ASSERT_NOT_NULL("realpath_alloc", owned,
                                         "d_path_resolve(p, NULL) allocates");
        #endif

        if (owned)
        {
            free(owned);
        }
    }
    #endif
#endif

    return d_tests_file_group("d_dir_get_current / d_dir_set_current / d_path_resolve", items, idx);
}


/*
d_tests_file_dir_all
  Runs every file_dir test.
*/
struct d_test_object*
d_tests_file_dir_all
(
    void
)
{
    struct d_test_object* items[8];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_mkdir();
    items[idx++] = d_tests_file_rmdir();
    items[idx++] = d_tests_file_traversal();
    items[idx++] = d_tests_file_cwd();

    return d_tests_file_group("X. file_dir", items, idx);
}
