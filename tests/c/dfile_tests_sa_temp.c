/******************************************************************************
* djinterp [test]                                         dfile_tests_sa_temp.c
*
*   Tests for temporary file operations (tmpfile, mkstemp, tmpnam).
*
*
* path:      \src	est\dfile_tests_sa_temp.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include ".\dfile_tests_sa.h"


/******************************************************************************
 * VIII. TEMPORARY FILE TESTS
 *****************************************************************************/

/*
d_tests_dfile_tmpfile
  Tests d_tmpfile for creating temporary files.
  Tests the following:
  - creates a temporary file stream
  - stream is readable and writable
*/
struct d_test_object*
d_tests_dfile_tmpfile
(
    void
)
{
    struct d_test_object* group;
    FILE*                 tmp;
    int                   write_result;
    bool                  test_create;
    bool                  test_writable;
    size_t                idx;

    // test 1: create temporary file
    tmp = d_tmpfile();
    test_create = (tmp != NULL);
    test_writable = false;

    if (tmp)
    {
        // test 2: file is writable
        write_result = fprintf(tmp, "test data");
        test_writable = (write_result > 0);
        fclose(tmp);
    }

    // build result tree
    group = d_test_object_new_interior("d_tmpfile", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("create",
                                           test_create,
                                           "d_tmpfile creates temporary file");
    group->elements[idx++] = D_ASSERT_TRUE("writable",
                                           test_writable,
                                           "temporary file is writable");

    return group;
}


/*
d_tests_dfile_tmpfile_s
  Tests d_tmpfile_s for secure temporary file creation.
  Tests the following:
  - returns 0 and valid stream on success
  - returns EINVAL for NULL stream pointer
*/
struct d_test_object*
d_tests_dfile_tmpfile_s
(
    void
)
{
    struct d_test_object* group;
    FILE*                 stream;
    int                   result;
    bool                  test_create;
    bool                  test_null_ptr;
    size_t                idx;

    // test 1: create temporary file
    stream = NULL;
    result = d_tmpfile_s(&stream);
    test_create = (result == 0) && (stream != NULL);

    if (stream)
    {
        fclose(stream);
    }

    // test 2: NULL stream pointer
    result = d_tmpfile_s(NULL);
    test_null_ptr = (result == EINVAL);

    // build result tree
    group = d_test_object_new_interior("d_tmpfile_s", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("create",
                                           test_create,
                                           "d_tmpfile_s creates temporary file");
    group->elements[idx++] = D_ASSERT_TRUE("null_ptr",
                                           test_null_ptr,
                                           "d_tmpfile_s returns EINVAL for NULL");

    return group;
}


/*
d_tests_dfile_mkstemp
  Tests d_mkstemp for template-based temporary files.
  Tests the following:
  - creates file with modified template
  - returns valid fd
  - template is modified in place
*/
struct d_test_object*
d_tests_dfile_mkstemp
(
    void
)
{
    struct d_test_object* group;
    char                  template_buf[D_INTERNAL_TEST_PATH_BUF_SIZE];
    char                  original[D_INTERNAL_TEST_PATH_BUF_SIZE];
    int                   fd;
    bool                  test_create;
    bool                  test_template_modified;
    size_t                idx;

    // setup template
    d_tests_dfile_get_test_path(template_buf,
                               sizeof(template_buf),
                               "tmpXXXXXX");
    d_strcpy_s(original, sizeof(original), template_buf);

    // test 1 & 2: create file and check template modification
    fd = d_mkstemp(template_buf);
    test_create = (fd >= 0);
    test_template_modified = (strcmp(template_buf, original) != 0);

    if (fd >= 0)
    {
        d_close(fd);
        d_remove(template_buf);
    }

    // build result tree
    group = d_test_object_new_interior("d_mkstemp", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("create",
                                           test_create,
                                           "d_mkstemp creates temporary file");
    group->elements[idx++] = D_ASSERT_TRUE("template_modified",
                                           test_template_modified,
                                           "d_mkstemp modifies template");

    return group;
}


/*
d_tests_dfile_tmpnam_s
  Tests d_tmpnam_s for generating temporary filenames.
  Tests the following:
  - returns 0 and generates name on success
  - returns error for NULL buffer
  - returns error for buffer too small
*/
struct d_test_object*
d_tests_dfile_tmpnam_s
(
    void
)
{
    struct d_test_object* group;
    char                  name_buf[D_FILE_PATH_MAX];
    int                   result;
    bool                  test_generate;
    bool                  test_null_buf;
    bool                  test_small_buf;
    size_t                idx;

    // test 1: generate temporary name
    memset(name_buf, 0, sizeof(name_buf));
    result = d_tmpnam_s(name_buf, sizeof(name_buf));
    test_generate = (result == 0) && (strlen(name_buf) > 0);

    // test 2: NULL buffer
    result = d_tmpnam_s(NULL, 100);
    test_null_buf = (result != 0);

    // test 3: buffer too small
    result = d_tmpnam_s(name_buf, 1);
    test_small_buf = (result != 0);

    // build result tree
    group = d_test_object_new_interior("d_tmpnam_s", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("generate",
                                           test_generate,
                                           "d_tmpnam_s generates filename");
    group->elements[idx++] = D_ASSERT_TRUE("null_buf",
                                           test_null_buf,
                                           "d_tmpnam_s returns error for NULL");
    group->elements[idx++] = D_ASSERT_TRUE("small_buf",
                                           test_small_buf,
                                           "d_tmpnam_s returns error for small buffer");

    return group;
}


/*
d_tests_dfile_tempdir
  Tests d_tempdir for getting system temp directory.
  Tests the following:
  - returns valid temp directory path
  - returned path exists
  - returns NULL for NULL buffer
*/
struct d_test_object*
d_tests_dfile_tempdir
(
    void
)
{
    struct d_test_object* group;
    char                  dir_buf[D_FILE_PATH_MAX];
    char*                 result;
    bool                  test_get_dir;
    bool                  test_dir_exists;
    bool                  test_null_buf;
    size_t                idx;

    // test 1 & 2: get temp directory and verify it exists
    result = d_tempdir(dir_buf, sizeof(dir_buf));
    test_get_dir = (result != NULL) && (strlen(dir_buf) > 0);
    test_dir_exists = test_get_dir && d_is_dir(dir_buf);

    // test 3: NULL buffer
    result = d_tempdir(NULL, 100);
    test_null_buf = (result == NULL);

    // build result tree
    group = d_test_object_new_interior("d_tempdir", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("get_dir",
                                           test_get_dir,
                                           "d_tempdir returns temp directory");
    group->elements[idx++] = D_ASSERT_TRUE("dir_exists",
                                           test_dir_exists,
                                           "returned directory exists");
    group->elements[idx++] = D_ASSERT_TRUE("null_buf",
                                           test_null_buf,
                                           "d_tempdir returns NULL for NULL buffer");

    return group;
}


/*
d_tests_dfile_temporary_files_all
  Runs all temporary file tests.
  Tests the following:
  - d_tmpfile
  - d_tmpfile_s
  - d_mkstemp
  - d_tmpnam_s
  - d_tempdir
*/
struct d_test_object*
d_tests_dfile_temporary_files_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("VIII. Temporary Files", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dfile_tmpfile();
    group->elements[idx++] = d_tests_dfile_tmpfile_s();
    group->elements[idx++] = d_tests_dfile_mkstemp();
    group->elements[idx++] = d_tests_dfile_tmpnam_s();
    group->elements[idx++] = d_tests_dfile_tempdir();

    return group;
}


