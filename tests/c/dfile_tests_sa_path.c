/******************************************************************************
* djinterp [test]                                         dfile_tests_sa_path.c
*
*   Tests for path utility operations (getcwd, realpath, dirname, basename).
*
*
* path:      \src	est\dfile_tests_sa_path.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include ".\dfile_tests_sa.h"


/******************************************************************************
 * XII. PATH UTILITIES TESTS
 *****************************************************************************/

/*
d_tests_dfile_getcwd
  Tests d_getcwd for getting current working directory.
  Tests the following:
  - returns valid path
  - path is a directory
  - returns NULL for NULL buffer
*/
struct d_test_object*
d_tests_dfile_getcwd
(
    void
)
{
    struct d_test_object* group;
    char                  cwd_buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_get_cwd;
    bool                  test_is_dir;
    bool                  test_null_buf;
    size_t                idx;

    // test 1 & 2: get cwd and verify it's a directory
    result = d_getcwd(cwd_buf, sizeof(cwd_buf));
    test_get_cwd = (result != NULL) && (strlen(cwd_buf) > 0);
    test_is_dir = test_get_cwd && d_is_dir(cwd_buf);

    // test 3: NULL buffer
    result = d_getcwd(NULL, 100);
    test_null_buf = (result == NULL);

    // build result tree
    group = d_test_object_new_interior("d_getcwd", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("get_cwd",
                                           test_get_cwd,
                                           "d_getcwd returns current directory");
    group->elements[idx++] = D_ASSERT_TRUE("is_dir",
                                           test_is_dir,
                                           "returned path is a directory");
    group->elements[idx++] = D_ASSERT_TRUE("null_buf",
                                           test_null_buf,
                                           "d_getcwd returns NULL for NULL buffer");

    return group;
}


/*
d_tests_dfile_chdir
  Tests d_chdir for changing working directory.
  Tests the following:
  - changes to valid directory
  - returns error for nonexistent directory
  - returns error for NULL path
*/
struct d_test_object*
d_tests_dfile_chdir
(
    void
)
{
    struct d_test_object* group;
    char                  original_cwd[D_FILE_PATH_MAX];
    char                  new_cwd[D_FILE_PATH_MAX];
    int                   result;
    bool                  test_change;
    bool                  test_nonexistent;
    bool                  test_null_path;
    size_t                idx;

    // save original directory
    d_getcwd(original_cwd, sizeof(original_cwd));

    // test 1: change to test directory
    result = d_chdir(D_TEST_DFILE_TEMP_DIR);
    d_getcwd(new_cwd, sizeof(new_cwd));
    test_change = (result == 0);

    // restore original directory
    d_chdir(original_cwd);

    // test 2: nonexistent directory
    result = d_chdir("nonexistent_chdir_test_dir");
    test_nonexistent = (result != 0);

    // test 3: NULL path
    result = d_chdir(NULL);
    test_null_path = (result != 0);

    // build result tree
    group = d_test_object_new_interior("d_chdir", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("change",
                                           test_change,
                                           "d_chdir changes directory");
    group->elements[idx++] = D_ASSERT_TRUE("nonexistent",
                                           test_nonexistent,
                                           "d_chdir returns error for nonexistent");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_chdir returns error for NULL path");

    return group;
}


/*
d_tests_dfile_realpath
  Tests d_realpath for getting canonical paths.
  Tests the following:
  - returns canonical path for existing file
  - returns NULL for nonexistent file
  - returns NULL for NULL path
*/
struct d_test_object*
d_tests_dfile_realpath
(
    void
)
{
    struct d_test_object* group;
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  resolved[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_resolve;
    bool                  test_nonexistent;
    bool                  test_null_path;
    size_t                idx;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: resolve existing file
    result = d_realpath(path_buf, resolved);
    test_resolve = (result != NULL) && (strlen(resolved) > 0);

    // test 2: nonexistent file
    result = d_realpath("nonexistent_realpath_test.txt", resolved);
    test_nonexistent = (result == NULL);

    // test 3: NULL path
    result = d_realpath(NULL, resolved);
    test_null_path = (result == NULL);

    // build result tree
    group = d_test_object_new_interior("d_realpath", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("resolve",
                                           test_resolve,
                                           "d_realpath resolves path");
    group->elements[idx++] = D_ASSERT_TRUE("nonexistent",
                                           test_nonexistent,
                                           "d_realpath returns NULL for nonexistent");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_realpath returns NULL for NULL path");

    return group;
}


/*
d_tests_dfile_dirname
  Tests d_dirname for extracting directory component.
  Tests the following:
  - extracts directory from path with file
  - handles path with trailing separator
  - returns NULL for NULL path
*/
struct d_test_object*
d_tests_dfile_dirname
(
    void
)
{
    struct d_test_object* group;
    char                  buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_with_file;
    bool                  test_null_path;
    size_t                idx;

    // test 1: directory from path with file
    result = d_dirname("/path/to/file.txt", buf, sizeof(buf));
    test_with_file = (result != NULL) && (strstr(buf, "to") != NULL);

    // test 2: NULL path
    result = d_dirname(NULL, buf, sizeof(buf));
    test_null_path = (result == NULL);

    // build result tree
    group = d_test_object_new_interior("d_dirname", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("with_file",
                                           test_with_file,
                                           "d_dirname extracts directory");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_dirname returns NULL for NULL path");

    return group;
}


/*
d_tests_dfile_basename
  Tests d_basename for extracting filename component.
  Tests the following:
  - extracts filename from path
  - handles path without directory
  - returns NULL for NULL path
*/
struct d_test_object*
d_tests_dfile_basename
(
    void
)
{
    struct d_test_object* group;
    char                  buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_with_dir;
    bool                  test_no_dir;
    bool                  test_null_path;
    size_t                idx;

    // test 1: extract filename from path with directory
    result = d_basename("/path/to/file.txt", buf, sizeof(buf));
    test_with_dir = (result != NULL) && (strcmp(buf, "file.txt") == 0);

    // test 2: just filename
    result = d_basename("file.txt", buf, sizeof(buf));
    test_no_dir = (result != NULL) && (strcmp(buf, "file.txt") == 0);

    // test 3: NULL path
    result = d_basename(NULL, buf, sizeof(buf));
    test_null_path = (result == NULL);

    // build result tree
    group = d_test_object_new_interior("d_basename", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("with_dir",
                                           test_with_dir,
                                           "d_basename extracts filename from path");
    group->elements[idx++] = D_ASSERT_TRUE("no_dir",
                                           test_no_dir,
                                           "d_basename handles simple filename");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_basename returns NULL for NULL path");

    return group;
}


/*
d_tests_dfile_path_join
  Tests d_path_join for joining path components.
  Tests the following:
  - joins two path components
  - handles NULL first path (uses second path)
  - returns NULL for NULL buffer
*/
struct d_test_object*
d_tests_dfile_path_join
(
    void
)
{
    struct d_test_object* group;
    char                  buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_join;
    bool                  test_null_buf;
    bool                  test_null_path1;
    size_t                idx;

    // test 1: join paths
    result = d_path_join(buf, sizeof(buf), "path/to", "file.txt");
    test_join = (result != NULL) && (strstr(buf, "file.txt") != NULL);

    // test 2: NULL buffer
    result = d_path_join(NULL, 100, "path", "file");
    test_null_buf = (result == NULL);

    // test 3: NULL first path component (should use second path)
    result = d_path_join(buf, sizeof(buf), NULL, "file.txt");
    test_null_path1 = (result != NULL) && (strcmp(buf, "file.txt") == 0);

    // build result tree
    group = d_test_object_new_interior("d_path_join", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("join",
                                           test_join,
                                           "d_path_join joins path components");
    group->elements[idx++] = D_ASSERT_TRUE("null_buf",
                                           test_null_buf,
                                           "d_path_join returns NULL for NULL buffer");
    group->elements[idx++] = D_ASSERT_TRUE("null_path1",
                                           test_null_path1,
                                           "d_path_join uses second path when first is NULL");

    return group;
}


/*
d_tests_dfile_path_normalize
  Tests d_path_normalize for normalizing paths.
  Tests the following:
  - resolves . and .. components
  - returns NULL for NULL path
*/
struct d_test_object*
d_tests_dfile_path_normalize
(
    void
)
{
    struct d_test_object* group;
    char                  buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_normalize;
    bool                  test_null_path;
    size_t                idx;

    // test 1: normalize path with . and ..
    result = d_path_normalize("/path/to/../file.txt", buf, sizeof(buf));
    test_normalize = (result != NULL);

    // test 2: NULL path
    result = d_path_normalize(NULL, buf, sizeof(buf));
    test_null_path = (result == NULL);

    // build result tree
    group = d_test_object_new_interior("d_path_normalize", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("normalize",
                                           test_normalize,
                                           "d_path_normalize normalizes path");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_path_normalize returns NULL for NULL");

    return group;
}


/*
d_tests_dfile_path_is_absolute
  Tests d_path_is_absolute for checking absolute paths.
  Tests the following:
  - returns non-zero for absolute path
  - returns 0 for relative path
  - returns 0 for NULL path
*/
struct d_test_object*
d_tests_dfile_path_is_absolute
(
    void
)
{
    struct d_test_object* group;
    int                   result;
    bool                  test_absolute;
    bool                  test_relative;
    bool                  test_null_path;
    size_t                idx;

#if defined(D_FILE_PLATFORM_WINDOWS)
    // test 1: absolute path (Windows)
    result = d_path_is_absolute("C:\\path\\to\\file");
    test_absolute = (result != 0);
#else
    // test 1: absolute path (Unix)
    result = d_path_is_absolute("/path/to/file");
    test_absolute = (result != 0);
#endif

    // test 2: relative path
    result = d_path_is_absolute("path/to/file");
    test_relative = (result == 0);

    // test 3: NULL path
    result = d_path_is_absolute(NULL);
    test_null_path = (result == 0);

    // build result tree
    group = d_test_object_new_interior("d_path_is_absolute", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("absolute",
                                           test_absolute,
                                           "d_path_is_absolute detects absolute path");
    group->elements[idx++] = D_ASSERT_TRUE("relative",
                                           test_relative,
                                           "d_path_is_absolute returns 0 for relative");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_path_is_absolute returns 0 for NULL");

    return group;
}


/*
d_tests_dfile_get_extension
  Tests d_get_extension for extracting file extensions.
  Tests the following:
  - returns extension for file with extension
  - returns NULL for file without extension
  - returns NULL for NULL path
  - handles hidden files correctly
*/
struct d_test_object*
d_tests_dfile_get_extension
(
    void
)
{
    struct d_test_object* group;
    const char*           result;
    bool                  test_with_ext;
    bool                  test_no_ext;
    bool                  test_hidden_file;
    bool                  test_null_path;
    size_t                idx;

    // test 1: file with extension
    result = d_get_extension("file.txt");
    test_with_ext = (result != NULL) && (strcmp(result, ".txt") == 0);

    // test 2: file without extension
    result = d_get_extension("filename");
    test_no_ext = (result == NULL);

    // test 3: hidden file (should return NULL, not treat . as extension)
    result = d_get_extension(".bashrc");
    test_hidden_file = (result == NULL);

    // test 4: NULL path
    result = d_get_extension(NULL);
    test_null_path = (result == NULL);

    // build result tree
    group = d_test_object_new_interior("d_get_extension", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("with_ext",
                                           test_with_ext,
                                           "d_get_extension returns extension");
    group->elements[idx++] = D_ASSERT_TRUE("no_ext",
                                           test_no_ext,
                                           "d_get_extension returns NULL for no ext");
    group->elements[idx++] = D_ASSERT_TRUE("hidden_file",
                                           test_hidden_file,
                                           "d_get_extension handles hidden files");
    group->elements[idx++] = D_ASSERT_TRUE("null_path",
                                           test_null_path,
                                           "d_get_extension returns NULL for NULL");

    return group;
}


/*
d_tests_dfile_path_utilities_all
  Runs all path utility tests.
  Tests the following:
  - d_getcwd
  - d_chdir
  - d_realpath
  - d_dirname
  - d_basename
  - d_path_join
  - d_path_normalize
  - d_path_is_absolute
  - d_get_extension
*/
struct d_test_object*
d_tests_dfile_path_utilities_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("XII. Path Utilities", 9);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dfile_getcwd();
    group->elements[idx++] = d_tests_dfile_chdir();
    group->elements[idx++] = d_tests_dfile_realpath();
    group->elements[idx++] = d_tests_dfile_dirname();
    group->elements[idx++] = d_tests_dfile_basename();
    group->elements[idx++] = d_tests_dfile_path_join();
    group->elements[idx++] = d_tests_dfile_path_normalize();
    group->elements[idx++] = d_tests_dfile_path_is_absolute();
    group->elements[idx++] = d_tests_dfile_get_extension();

    return group;
}


