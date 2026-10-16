/*******************************************************************************
* djinterp [test]                                              file_tests_temp.c
*
*   XII. TEMPORARIES (file_temp) -- temporary files and the temporary
* directory. d_file_temp_stream / d_file_temp_create choose a name and open it in one step;
* d_file_temp_create is descriptor-based and reports ENOSYS on the ISO C backend, and it
* rewrites the caller's template in place, so the template must be a mutable
* buffer ending in exactly six X's (a template that does not is rejected).
* d_file_temp_name exists only where the platform still offers it and is gated on
* D_FILE_TEMP_HAS_TMPNAM. d_dir_temp yields an existing directory and requires a
* buffer larger than one byte.
*
*
* path:      /tests/djinterp/c/fs/file_tests_temp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * XII.a  d_file_temp_stream
 *****************************************************************************/

static void
d_fs_tests_temp_tmpfile
(
    dtest_suite* _suite
)
{
    FILE* file;
    char  buf[8];
    bool  usable;

    dtest_block(_suite, "d_file_temp_stream");

    dtest_test(_suite, "returns a stream that can be written and read back");
    file   = d_file_temp_stream();
    usable = false;

    if (file)
    {
        if (fwrite("tmp", 1, 3, file) == 3)
        {
            rewind(file);
            memset(buf, 0, sizeof(buf));
            usable = (fread(buf, 1, 3, file) == 3) &&
                     (memcmp(buf, "tmp", 3) == 0);
        }

        d_file_close_stream(file);
    }

    D_FS_CHECK(_suite, usable);
}


/******************************************************************************
 * XII.b  d_file_temp_stream_s
 *****************************************************************************/

static void
d_fs_tests_temp_tmpfile_s
(
    dtest_suite* _suite
)
{
    FILE* stream;
    int   rc;

    dtest_block(_suite, "d_file_temp_stream_s");

    dtest_test(_suite, "success -> 0 and a valid stream");
    stream = NULL;
    rc     = d_file_temp_stream_s(&stream);
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite, stream != NULL);

    if (stream)
    {
        d_file_close_stream(stream);
    }

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL out-parameter -> EINVAL");
    rc = d_file_temp_stream_s(NULL);
    D_FS_CHECK(_suite, rc == EINVAL);
#endif
}


/******************************************************************************
 * XII.c  d_file_temp_create
 *****************************************************************************/

static void
d_fs_tests_temp_mkstemp
(
    dtest_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char tmpl[D_FS_TEST_PATH_BUF];
    char bad[D_FS_TEST_PATH_BUF];
    int  fd;
    bool made;

    dtest_block(_suite, "d_file_temp_create");

    dtest_test(_suite, "opens a unique file and rewrites the template");
    snprintf(tmpl, sizeof(tmpl), "%s%cmkstempXXXXXX",
             d_fs_test_root(), D_FILE_PATH_SEP);
    fd   = d_file_temp_create(tmpl);
    made = (fd >= 0);

    if (fd >= 0)
    {
        // the six X's have been replaced, and the named file now exists
        made = made &&
               (strstr(tmpl, "XXXXXX") == NULL) &&
               d_file_exists(tmpl);
        d_file_close_fd(fd);
        d_file_remove(tmpl);
    }

    D_FS_CHECK(_suite, made);

    dtest_test(_suite, "a template not ending in six X's -> -1, EINVAL");
    snprintf(bad, sizeof(bad), "%s%cmkstemp_noplace",
             d_fs_test_root(), D_FILE_PATH_SEP);
    errno = 0;
    D_FS_CHECK(_suite, d_file_temp_create(bad) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

    dtest_test(_suite, "a too-short template -> -1, EINVAL");
    strcpy(bad, "XXXXX");
    errno = 0;
    D_FS_CHECK(_suite, d_file_temp_create(bad) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL template -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_temp_create(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
#else
    dtest_block(_suite, "d_file_temp_create");

    dtest_test(_suite, "ISO C backend -> -1, ENOSYS");
    {
        char tmpl[] = "mkstempXXXXXX";

        errno = 0;
        D_FS_CHECK(_suite, d_file_temp_create(tmpl) == -1);
    }
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * XII.d  d_file_temp_name
 *****************************************************************************/

static void
d_fs_tests_temp_tmpnam_s
(
    dtest_suite* _suite
)
{
#if D_FILE_TEMP_HAS_TMPNAM
    char buf[D_FS_TEST_PATH_BUF];
    int  rc;

    dtest_block(_suite, "d_file_temp_name");

    dtest_test(_suite, "produces a non-empty candidate name -> 0");
    memset(buf, 0, sizeof(buf));
    rc = d_file_temp_name(buf, sizeof(buf));
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite, strlen(buf) > 0);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "a one-byte buffer -> EINVAL");
    D_FS_CHECK(_suite, d_file_temp_name(buf, 1) == EINVAL);

#endif
#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL buffer -> EINVAL");
    D_FS_CHECK(_suite, d_file_temp_name(NULL, sizeof(buf)) == EINVAL);
#endif
#else
    // d_file_temp_name is not offered on this platform; nothing to exercise
    dtest_block(_suite, "d_file_temp_name");

    dtest_test(_suite, "not available on this platform (skipped)");
    D_FS_CHECK(_suite, true);
#endif
}


/******************************************************************************
 * XII.e  d_dir_temp
 *****************************************************************************/

static void
d_fs_tests_temp_tempdir
(
    dtest_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF];

    dtest_block(_suite, "d_dir_temp");

    dtest_test(_suite, "yields an existing directory");
    memset(buf, 0, sizeof(buf));
    D_FS_CHECK(_suite, d_dir_temp(buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, d_dir_exists(buf));

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_temp(NULL, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "a one-byte buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_temp(buf, 1) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_temp
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "XII. file_temp");

    d_fs_tests_temp_tmpfile(_suite);
    d_fs_tests_temp_tmpfile_s(_suite);
    d_fs_tests_temp_mkstemp(_suite);
    d_fs_tests_temp_tmpnam_s(_suite);
    d_fs_tests_temp_tempdir(_suite);

    return;
}
