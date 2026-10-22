/*******************************************************************************
* djinterp [test]                                              file_tests_open.c
*
*   II. STREAMS (file_open) -- opening, reopening, and closing FILE* streams.
* Success paths exercise the standard fixture and staged siblings; the Annex K
* forms are checked for their clear-the-out-parameter-first contract and their
* errno-style returns; and d_file_open_stream_fd's descriptor-adoption path is exercised
* only where the backend actually has descriptors.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_open.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.10.20
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * II.a  d_file_open_stream
 *****************************************************************************/

static void
d_fs_tests_open_fopen
(
    dtest_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  opened;
    bool  created;

    dtest_block(_suite, "d_file_open_stream");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "opens an existing file for reading");
    file   = d_file_open_stream(path, "rb");
    opened = (file != NULL);

    if (file)
    {
        d_file_close_stream(file);
        file = NULL;   // the guards below test this pointer
    }

    D_FS_CHECK(_suite, opened);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL filename -> NULL, EINVAL");
    errno = 0;
    file  = d_file_open_stream(NULL, "rb");
    D_FS_CHECK(_suite, file == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL mode -> NULL, EINVAL");
    errno = 0;
    file  = d_file_open_stream(path, NULL);
    D_FS_CHECK(_suite, file == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "a missing file for reading -> NULL");
    d_fs_test_path(path, sizeof(path), "no_such_fopen_file.txt");
    file = d_file_open_stream(path, "rb");
    D_FS_CHECK(_suite, file == NULL);

    dtest_test(_suite, "creates a file for writing");
    d_fs_test_path(path, sizeof(path), "fopen_created.txt");
    file    = d_file_open_stream(path, "wb");
    created = (file != NULL);

    if (file)
    {
        d_file_close_stream(file);
        file = NULL;   // the guards below test this pointer
    }

    D_FS_CHECK(_suite, created);
    D_FS_CHECK(_suite, d_file_exists(path));

    d_file_remove(path);
}


/******************************************************************************
 * II.b  d_file_open_stream_s
 *****************************************************************************/

static void
d_fs_tests_open_fopen_s
(
    dtest_suite* _suite
)
{
    char   path[D_FS_TEST_PATH_BUF];
    FILE*  stream;
    int    rc;

    dtest_block(_suite, "d_file_open_stream_s");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "success -> 0 and a valid stream");
    stream = NULL;
    rc     = d_file_open_stream_s(&stream, path, "rb");
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite, stream != NULL);

    if (stream)
    {
        d_file_close_stream(stream);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL out-parameter -> EINVAL");
    rc = d_file_open_stream_s(NULL, path, "rb");
    D_FS_CHECK(_suite, rc == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL filename clears the stream and returns EINVAL");
    stream = (FILE*)0x1;
    rc     = d_file_open_stream_s(&stream, NULL, "rb");
    D_FS_CHECK(_suite, rc == EINVAL);
    D_FS_CHECK(_suite, stream == NULL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL mode -> EINVAL");
    stream = NULL;
    rc     = d_file_open_stream_s(&stream, path, NULL);
    D_FS_CHECK(_suite, rc == EINVAL);
    #endif

    dtest_test(_suite, "a missing file -> non-zero and a NULL stream");
    d_fs_test_path(path, sizeof(path), "no_such_fopen_s_file.txt");
    stream = (FILE*)0x1;
    rc     = d_file_open_stream_s(&stream, path, "rb");
    D_FS_CHECK(_suite, rc != 0);
    D_FS_CHECK(_suite, stream == NULL);
}


/******************************************************************************
 * II.c  d_file_reopen_stream
 *****************************************************************************/

static void
d_fs_tests_open_freopen
(
    dtest_suite* _suite
)
{
    char  first[D_FS_TEST_PATH_BUF];
    char  second[D_FS_TEST_PATH_BUF];
    FILE* file;
    FILE* reopened;
    char  buf[64];
    bool  read_ok;

    dtest_block(_suite, "d_file_reopen_stream");

    d_fs_test_path(first, sizeof(first), D_FS_TEST_FILE);
    d_fs_test_path(second, sizeof(second), "freopen_target.txt");
    d_fs_test_write(second, "SECOND", 6);

    dtest_test(_suite, "reopens a stream onto another file");
    file     = d_file_open_stream(first, "rb");
    reopened = file ? d_file_reopen_stream(second, "rb", file) : NULL;
    read_ok  = false;

    if (reopened)
    {
        memset(buf, 0, sizeof(buf));

        if (fread(buf, 1, 6, reopened) == 6)
        {
            read_ok = (memcmp(buf, "SECOND", 6) == 0);
        }

        //   d_file_reopen_stream REUSES the stream it is handed, so `reopened` and
        // `file` are one handle, not two. Closing it closes both, and the
        // `if (file)` cleanup further down would close it a second time --
        // which is a double free, not a leak. It went unnoticed because the
        // case in between reassigned `file` before the cleanup ran, so the
        // stale pointer was overwritten by luck rather than by design.
        d_file_close_stream(reopened);
        file = NULL;   // `reopened` is asserted below, so it keeps its value
    }
    else if (file)
    {
        d_file_close_stream(file);
        file = NULL;   // the guards below test this pointer
    }

    D_FS_CHECK(_suite, reopened != NULL);
    D_FS_CHECK(_suite, read_ok);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL mode -> NULL, EINVAL");
    file  = d_file_open_stream(first, "rb");
    errno = 0;
    D_FS_CHECK(_suite, d_file_reopen_stream(second, NULL, file) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    if (file)
    {
        d_file_close_stream(file);
        file = NULL;   // the guards below test this pointer
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_reopen_stream(second, "rb", NULL) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    d_file_remove(second);
}


/******************************************************************************
 * II.d  d_file_reopen_stream_s
 *****************************************************************************/

static void
d_fs_tests_open_freopen_s
(
    dtest_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    FILE* out;
    int   rc;

    dtest_block(_suite, "d_file_reopen_stream_s");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "success -> 0 and a valid stream");
    file = d_file_open_stream(path, "rb");
    out  = NULL;
    rc   = d_file_reopen_stream_s(&out, path, "rb", file);
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite, out != NULL);

    if (out)
    {
        //   Same aliasing as d_file_reopen_stream above: d_file_reopen_stream_s reuses the stream it
        // is given, so `out` and `file` are one handle. Closing it closes both.
        d_file_close_stream(out);
        file = NULL;   // `out` is asserted above, so it keeps its value
    }
    else if (file)
    {
        d_file_close_stream(file);
        file = NULL;   // the guards below test this pointer
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL out-parameter -> EINVAL");
    file = d_file_open_stream(path, "rb");
    rc   = d_file_reopen_stream_s(NULL, path, "rb", file);
    D_FS_CHECK(_suite, rc == EINVAL);
    #endif

    if (file)
    {
        d_file_close_stream(file);
        file = NULL;   // the guards below test this pointer
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL mode -> EINVAL");
    file = d_file_open_stream(path, "rb");
    out  = NULL;
    rc   = d_file_reopen_stream_s(&out, path, NULL, file);
    D_FS_CHECK(_suite, rc == EINVAL);
    #endif

    if (file)
    {
        d_file_close_stream(file);
        file = NULL;   // the guards below test this pointer
    }

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> EINVAL");
    out = NULL;
    rc  = d_file_reopen_stream_s(&out, path, "rb", NULL);
    D_FS_CHECK(_suite, rc == EINVAL);
#endif
}


/******************************************************************************
 * II.e  d_file_open_stream_fd
 *****************************************************************************/

static void
d_fs_tests_open_fdopen
(
    dtest_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char  path[D_FS_TEST_PATH_BUF];
    int   fd;
    FILE* stream;
    bool  wrapped;

    dtest_block(_suite, "d_file_open_stream_fd");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "adopts an open descriptor into a stream");
    fd      = d_file_open(path, O_RDONLY);
    stream  = (fd >= 0) ? d_file_open_stream_fd(fd, "rb") : NULL;
    wrapped = (stream != NULL);

    if (stream)
    {
        // closing the stream closes the adopted descriptor
        d_file_close_stream(stream);
    }
    else if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    D_FS_CHECK(_suite, wrapped);

    dtest_test(_suite, "negative descriptor -> NULL, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_open_stream_fd(-1, "rb") == NULL);
    D_FS_CHECK(_suite, errno == EBADF);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL mode -> NULL, EINVAL");
    fd = d_file_open(path, O_RDONLY);
    errno = 0;
    D_FS_CHECK(_suite, d_file_open_stream_fd(fd, NULL) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }
#else
    dtest_block(_suite, "d_file_open_stream_fd");

    dtest_test(_suite, "ISO C backend has no descriptors -> NULL, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_open_stream_fd(0, "rb") == NULL);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * II.f  d_file_close_stream
 *****************************************************************************/

static void
d_fs_tests_open_fclose
(
    dtest_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    int   rc;

    dtest_block(_suite, "d_file_close_stream");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "closing a valid stream -> 0");
    file = d_file_open_stream(path, "rb");
    rc   = file ? d_file_close_stream(file) : -1;
    D_FS_CHECK(_suite, rc == 0);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> EOF, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_close_stream(NULL) == EOF);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_open
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "II. file_open");

    d_fs_tests_open_fopen(_suite);
    d_fs_tests_open_fopen_s(_suite);
    d_fs_tests_open_freopen(_suite);
    d_fs_tests_open_freopen_s(_suite);
    d_fs_tests_open_fdopen(_suite);
    d_fs_tests_open_fclose(_suite);

    return;
}
