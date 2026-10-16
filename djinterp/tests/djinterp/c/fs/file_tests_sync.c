/*******************************************************************************
* djinterp [test]                                              file_tests_sync.c
*
*   VI. DURABILITY (file_sync) -- flushing to the operating system and to the
* device. The two descriptor-flushing forms report ENOSYS on the ISO C backend
* and are checked directly there; elsewhere a written file is flushed to
* success. d_file_flush_stream runs on every backend and, following C, treats a NULL
* stream as "flush every open output stream" rather than an error.
*
*
* path:      /tests/djinterp/c/fs/file_tests_sync.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * VI.a  d_file_sync_fd
 *****************************************************************************/

static void
d_fs_tests_sync_fsync
(
    dtest_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char path[D_FS_TEST_PATH_BUF];
    int  fd;
    bool synced;

    dtest_block(_suite, "d_file_sync_fd");

    d_fs_test_path(path, sizeof(path), "fsync.txt");

    dtest_test(_suite, "flushes a written descriptor to success");
    fd     = d_file_open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    synced = false;

    if (fd >= 0)
    {
        d_file_write_fd(fd, "durable", 7);
        synced = (d_file_sync_fd(fd) == 0);
        d_file_close_fd(fd);
    }

    D_FS_CHECK(_suite, synced);

    d_file_remove(path);

    dtest_test(_suite, "negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_sync_fd(-1) == -1);
    D_FS_CHECK(_suite, errno == EBADF);
#else
    dtest_block(_suite, "d_file_sync_fd");

    dtest_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_sync_fd(0) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * VI.b  d_file_sync_stream
 *****************************************************************************/

static void
d_fs_tests_sync_fsync_stream
(
    dtest_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  synced;

    dtest_block(_suite, "d_file_sync_stream");

    d_fs_test_path(path, sizeof(path), "fsync_stream.txt");

    dtest_test(_suite, "flushes a written stream to success");
    file   = d_file_open_stream(path, "wb");
    synced = false;

    if (file)
    {
        fwrite("durable", 1, 7, file);
        synced = (d_file_sync_stream(file) == 0);
        d_file_close_stream(file);
    }

    D_FS_CHECK(_suite, synced);

    d_file_remove(path);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_sync_stream(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
#else
    dtest_block(_suite, "d_file_sync_stream");

    dtest_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_sync_stream(stdout) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * VI.c  d_file_flush_stream
 *****************************************************************************/

static void
d_fs_tests_sync_fflush
(
    dtest_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  flushed;

    dtest_block(_suite, "d_file_flush_stream");

    d_fs_test_path(path, sizeof(path), "fflush.txt");

    dtest_test(_suite, "flushes a writable stream -> 0");
    file    = d_file_open_stream(path, "wb");
    flushed = false;

    if (file)
    {
        fwrite("buffered", 1, 8, file);
        flushed = (d_file_flush_stream(file) == 0);
        d_file_close_stream(file);
    }

    D_FS_CHECK(_suite, flushed);

    d_file_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream flushes all open streams -> 0");
    D_FS_CHECK(_suite, d_file_flush_stream(NULL) == 0);
    #endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_sync
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "VI. file_sync");

    d_fs_tests_sync_fsync(_suite);
    d_fs_tests_sync_fsync_stream(_suite);
    d_fs_tests_sync_fflush(_suite);

    return;
}
