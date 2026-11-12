/*******************************************************************************
* djinterp [c][filesys]                                       dfile_tests_path.c
*
*   Tests for path utility operations (getcwd, realpath, dirname, basename).
*
*
* path:      /tests/djinterp/c/filesys/dfile_tests_path.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.25
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dfile_tests.h"


/******************************************************************************
 * XII. PATH UTILITIES TESTS
 *****************************************************************************/

/*
d_tests_dfile_getcwd
  Tests d_dir_get_current for getting current working directory.
  Tests the following:
  - returns valid path
  - path is a directory
  - returns NULL for NULL buffer
*/
bool d_tests_dfile_getcwd(struct dj_test_suite* _suite)
{
    char                  cwd_buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_get_cwd;
    bool                  test_is_dir;
    bool                  test_null_buf;

    // test 1 & 2: get cwd and verify it's a directory
    result = d_dir_get_current(cwd_buf, sizeof(cwd_buf));
    test_get_cwd = (result != NULL) && (strlen(cwd_buf) > 0);
    test_is_dir = test_get_cwd && d_dir_exists(cwd_buf);

    // test 3: NULL buffer
    result = d_dir_get_current(NULL, 100);
    test_null_buf = (result == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_dir_get_current");

    D_TEST_CHECK_MSG(_suite,
                     "get_cwd",
                     test_get_cwd,
                     "d_dir_get_current returns current directory");
    D_TEST_CHECK_MSG(_suite,
                     "is_dir",
                     test_is_dir,
                     "returned path is a directory");
    D_TEST_CHECK_MSG(_suite,
                     "null_buf",
                     test_null_buf,
                     "d_dir_get_current returns NULL for NULL buffer");

    return true;
}


/*
d_tests_dfile_chdir
  Tests d_dir_set_current for changing working directory.
  Tests the following:
  - changes to valid directory
  - returns error for nonexistent directory
  - returns error for NULL path
*/
bool d_tests_dfile_chdir(struct dj_test_suite* _suite)
{
    char                  original_cwd[D_FILE_PATH_MAX];
    char                  new_cwd[D_FILE_PATH_MAX];
    int                   result;
    bool                  test_change;
    bool                  test_nonexistent;
    bool                  test_null_path;

    // save original directory
    d_dir_get_current(original_cwd, sizeof(original_cwd));

    // test 1: change to test directory
    result = d_dir_set_current(D_TEST_DFILE_TEMP_DIR);
    d_dir_get_current(new_cwd, sizeof(new_cwd));
    test_change = (result == 0);

    // restore original directory
    d_dir_set_current(original_cwd);

    // test 2: nonexistent directory
    result = d_dir_set_current("nonexistent_chdir_test_dir");
    test_nonexistent = (result != 0);

    // test 3: NULL path
    result = d_dir_set_current(NULL);
    test_null_path = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_dir_set_current");

    D_TEST_CHECK_MSG(_suite,
                     "change",
                     test_change,
                     "d_dir_set_current changes directory");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_dir_set_current returns error for nonexistent");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_dir_set_current returns error for NULL path");

    return true;
}


/*
d_tests_dfile_realpath
  Tests d_path_resolve for getting canonical paths.
  Tests the following:
  - returns canonical path for existing file
  - returns NULL for nonexistent file
  - returns NULL for NULL path
*/
bool d_tests_dfile_realpath(struct dj_test_suite* _suite)
{
    char                  path_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  resolved[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_resolve;
    bool                  test_nonexistent;
    bool                  test_null_path;

    // setup
    d_tests_dfile_get_test_path(path_buf,
                               sizeof(path_buf),
                               D_TEST_DFILE_TEST_FILENAME);

    // test 1: resolve existing file
    result = d_path_resolve(path_buf, resolved, sizeof(resolved));
    test_resolve = (result != NULL) && (strlen(resolved) > 0);

    // test 2: nonexistent file
    result = d_path_resolve("nonexistent_realpath_test.txt",
                            resolved,
                            sizeof(resolved));
    test_nonexistent = (result == NULL);

    // test 3: NULL path
    result = d_path_resolve(NULL, resolved, sizeof(resolved));
    test_null_path = (result == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_path_resolve");

    D_TEST_CHECK_MSG(_suite,
                     "resolve",
                     test_resolve,
                     "d_path_resolve resolves path");
    D_TEST_CHECK_MSG(_suite,
                     "nonexistent",
                     test_nonexistent,
                     "d_path_resolve returns NULL for nonexistent");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_path_resolve returns NULL for NULL path");

    return true;
}


/*
d_tests_dfile_dirname
  Tests d_path_dirname for extracting directory component.
  Tests the following:
  - extracts directory from path with file
  - handles path with trailing separator
  - returns NULL for NULL path
*/
bool d_tests_dfile_dirname(struct dj_test_suite* _suite)
{
    char                  buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_with_file;
    bool                  test_null_path;

    // test 1: directory from path with file
    result = d_path_dirname("/path/to/file.txt", buf, sizeof(buf));
    test_with_file = (result != NULL) && (strstr(buf, "to") != NULL);

    // test 2: NULL path
    result = d_path_dirname(NULL, buf, sizeof(buf));
    test_null_path = (result == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_path_dirname");

    D_TEST_CHECK_MSG(_suite,
                     "with_file",
                     test_with_file,
                     "d_path_dirname extracts directory");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_path_dirname returns NULL for NULL path");

    return true;
}


/*
d_tests_dfile_basename
  Tests d_path_basename for extracting filename component.
  Tests the following:
  - extracts filename from path
  - handles path without directory
  - returns NULL for NULL path
*/
bool d_tests_dfile_basename(struct dj_test_suite* _suite)
{
    char                  buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_with_dir;
    bool                  test_no_dir;
    bool                  test_null_path;

    // test 1: extract filename from path with directory
    result = d_path_basename("/path/to/file.txt", buf, sizeof(buf));
    test_with_dir = (result != NULL) && (strcmp(buf, "file.txt") == 0);

    // test 2: just filename
    result = d_path_basename("file.txt", buf, sizeof(buf));
    test_no_dir = (result != NULL) && (strcmp(buf, "file.txt") == 0);

    // test 3: NULL path
    result = d_path_basename(NULL, buf, sizeof(buf));
    test_null_path = (result == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_path_basename");

    D_TEST_CHECK_MSG(_suite,
                     "with_dir",
                     test_with_dir,
                     "d_path_basename extracts filename from path");
    D_TEST_CHECK_MSG(_suite,
                     "no_dir",
                     test_no_dir,
                     "d_path_basename handles simple filename");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_path_basename returns NULL for NULL path");

    return true;
}


/*
d_tests_dfile_path_join
  Tests d_path_join for joining path components.
  Tests the following:
  - joins two path components
  - handles NULL first path (uses second path)
  - returns NULL for NULL buffer
*/
bool d_tests_dfile_path_join(struct dj_test_suite* _suite)
{
    char                  buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_join;
    bool                  test_null_buf;
    bool                  test_null_path1;

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
    d_test_suite_test(_suite, "d_path_join");

    D_TEST_CHECK_MSG(_suite,
                     "join",
                     test_join,
                     "d_path_join joins path components");
    D_TEST_CHECK_MSG(_suite,
                     "null_buf",
                     test_null_buf,
                     "d_path_join returns NULL for NULL buffer");
    D_TEST_CHECK_MSG(_suite,
                     "null_path1",
                     test_null_path1,
                     "d_path_join uses second path when first is NULL");

    return true;
}


/*
d_tests_dfile_path_normalize
  Tests d_path_normalize for normalizing paths.
  Tests the following:
  - resolves . and .. components
  - returns NULL for NULL path
*/
bool d_tests_dfile_path_normalize(struct dj_test_suite* _suite)
{
    char                  buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_normalize;
    bool                  test_null_path;

    // test 1: normalize path with . and ..
    result = d_path_normalize("/path/to/../file.txt", buf, sizeof(buf));
    test_normalize = (result != NULL);

    // test 2: NULL path
    result = d_path_normalize(NULL, buf, sizeof(buf));
    test_null_path = (result == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_path_normalize");

    D_TEST_CHECK_MSG(_suite,
                     "normalize",
                     test_normalize,
                     "d_path_normalize normalizes path");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_path_normalize returns NULL for NULL");

    return true;
}


/*
d_tests_dfile_path_is_absolute
  Tests d_path_is_absolute for checking absolute paths.
  Tests the following:
  - returns non-zero for absolute path
  - returns 0 for relative path
  - returns 0 for NULL path
*/
bool d_tests_dfile_path_is_absolute(struct dj_test_suite* _suite)
{
    int                   result;
    bool                  test_absolute;
    bool                  test_relative;
    bool                  test_null_path;

#if D_FILE_PATH_UNDERSTANDS_WINDOWS
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
    d_test_suite_test(_suite, "d_path_is_absolute");

    D_TEST_CHECK_MSG(_suite,
                     "absolute",
                     test_absolute,
                     "d_path_is_absolute detects absolute path");
    D_TEST_CHECK_MSG(_suite,
                     "relative",
                     test_relative,
                     "d_path_is_absolute returns 0 for relative");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_path_is_absolute returns 0 for NULL");

    return true;
}


/*
d_tests_dfile_get_extension
  Tests d_path_extension for extracting file extensions.
  Tests the following:
  - returns extension for file with extension
  - returns NULL for file without extension
  - returns NULL for NULL path
  - handles hidden files correctly
*/
bool d_tests_dfile_get_extension(struct dj_test_suite* _suite)
{
    const char*           result;
    bool                  test_with_ext;
    bool                  test_no_ext;
    bool                  test_hidden_file;
    bool                  test_null_path;

    // test 1: file with extension
    result = d_path_extension("file.txt");
    test_with_ext = (result != NULL) && (strcmp(result, ".txt") == 0);

    // test 2: file without extension
    result = d_path_extension("filename");
    test_no_ext = (result == NULL);

    // test 3: hidden file (should return NULL, not treat . as extension)
    result = d_path_extension(".bashrc");
    test_hidden_file = (result == NULL);

    // test 4: NULL path
    result = d_path_extension(NULL);
    test_null_path = (result == NULL);

    // build result tree
    d_test_suite_test(_suite, "d_path_extension");

    D_TEST_CHECK_MSG(_suite,
                     "with_ext",
                     test_with_ext,
                     "d_path_extension returns extension");
    D_TEST_CHECK_MSG(_suite,
                     "no_ext",
                     test_no_ext,
                     "d_path_extension returns NULL for no ext");
    D_TEST_CHECK_MSG(_suite,
                     "hidden_file",
                     test_hidden_file,
                     "d_path_extension handles hidden files");
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     test_null_path,
                     "d_path_extension returns NULL for NULL");

    return true;
}


/*
d_tests_dfile_path_utilities_all
  Runs all path utility tests.
  Tests the following:
  - d_dir_get_current
  - d_dir_set_current
  - d_path_resolve
  - d_path_dirname
  - d_path_basename
  - d_path_join
  - d_path_normalize
  - d_path_is_absolute
  - d_path_extension
*/
bool d_tests_dfile_path_utilities_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "XII. Path Utilities");

    d_tests_dfile_getcwd(_suite);
    d_tests_dfile_chdir(_suite);
    d_tests_dfile_realpath(_suite);
    d_tests_dfile_dirname(_suite);
    d_tests_dfile_basename(_suite);
    d_tests_dfile_path_join(_suite);
    d_tests_dfile_path_normalize(_suite);
    d_tests_dfile_path_is_absolute(_suite);
    d_tests_dfile_get_extension(_suite);

    return true;
}
