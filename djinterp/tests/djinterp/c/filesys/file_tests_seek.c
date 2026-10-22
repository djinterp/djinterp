/*******************************************************************************
* djinterp [test]                                              file_tests_seek.c
*
*   V. POSITION (file_seek) -- 64-bit positioning and truncation. The stream
* positioners run on every backend; truncation is descriptor-based and reports
* ENOSYS on the ISO C backend. d_file_truncate_fd is checked in both directions --
* shrinking discards the tail, extending zero-fills -- since neither is a
* no-op, and the stream form's flush-then-truncate contract is exercised
* through a written file.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_seek.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.10.20
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * V.a  d_file_seek_stream
 *****************************************************************************/

static void
d_fs_tests_seek_fseeko
(
    dtest_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  set_ok;
    bool  cur_ok;
    bool  end_ok;

    dtest_block(_suite, "d_file_seek_stream");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "SEEK_SET / SEEK_CUR / SEEK_END land where expected");
    file   = d_file_open_stream(path, "rb");
    set_ok = false;
    cur_ok = false;
    end_ok = false;

    if (file)
    {
        set_ok = (d_file_seek_stream(file, 5, SEEK_SET) == 0) && (d_file_tell_stream(file) == 5);
        cur_ok = (d_file_seek_stream(file, 3, SEEK_CUR) == 0) && (d_file_tell_stream(file) == 8);
        end_ok = (d_file_seek_stream(file, -5, SEEK_END) == 0);
        d_file_close_stream(file);
    }

    D_FS_CHECK(_suite, set_ok);
    D_FS_CHECK(_suite, cur_ok);
    D_FS_CHECK(_suite, end_ok);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_seek_stream(NULL, 0, SEEK_SET) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "an invalid whence -> -1, EINVAL");
    file  = d_file_open_stream(path, "rb");
    errno = 0;
    D_FS_CHECK(_suite, (file != NULL) && (d_file_seek_stream(file, 0, 999) == -1));
    D_FS_CHECK(_suite, errno == EINVAL);

    if (file)
    {
        d_file_close_stream(file);
    }
}


/******************************************************************************
 * V.b  d_file_tell_stream
 *****************************************************************************/

static void
d_fs_tests_seek_ftello
(
    dtest_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  at_start;
    bool  after_seek;

    dtest_block(_suite, "d_file_tell_stream");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "reports 0 at the start and the offset after a seek");
    file       = d_file_open_stream(path, "rb");
    at_start   = false;
    after_seek = false;

    if (file)
    {
        at_start = (d_file_tell_stream(file) == 0);
        d_file_seek_stream(file, 10, SEEK_SET);
        after_seek = (d_file_tell_stream(file) == 10);
        d_file_close_stream(file);
    }

    D_FS_CHECK(_suite, at_start);
    D_FS_CHECK(_suite, after_seek);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_tell_stream(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * V.c  d_file_rewind_stream
 *****************************************************************************/

static void
d_fs_tests_seek_rewind
(
    dtest_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  rewound;

    dtest_block(_suite, "d_file_rewind_stream");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "returns the position to the start");
    file    = d_file_open_stream(path, "rb");
    rewound = false;

    if (file)
    {
        d_file_seek_stream(file, 12, SEEK_SET);
        rewound = (d_file_rewind_stream(file) == 0) && (d_file_tell_stream(file) == 0);
        d_file_close_stream(file);
    }

    D_FS_CHECK(_suite, rewound);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_rewind_stream(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
}


/******************************************************************************
 * V.d  d_file_truncate_fd
 *****************************************************************************/

static void
d_fs_tests_seek_ftruncate
(
    dtest_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char path[D_FS_TEST_PATH_BUF];
    int  fd;
    bool shrank;
    bool extended;

    dtest_block(_suite, "d_file_truncate_fd");

    d_fs_test_path(path, sizeof(path), "ftruncate.txt");

    dtest_test(_suite, "shrinking discards the tail");
    d_fs_test_write(path, "Hello World! This is content.", 29);
    fd     = d_file_open(path, O_RDWR);
    shrank = false;

    if (fd >= 0)
    {
        shrank = (d_file_truncate_fd(fd, 10) == 0);
        d_file_close_fd(fd);
        shrank = shrank && (d_file_size(path) == 10);
    }

    D_FS_CHECK(_suite, shrank);

    dtest_test(_suite, "extending grows the file (zero-filled)");
    fd       = d_file_open(path, O_RDWR);
    extended = false;

    if (fd >= 0)
    {
        extended = (d_file_truncate_fd(fd, 50) == 0);
        d_file_close_fd(fd);
        extended = extended && (d_file_size(path) == 50);
    }

    D_FS_CHECK(_suite, extended);

    d_file_remove(path);

    dtest_test(_suite, "negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_truncate_fd(-1, 10) == -1);
    D_FS_CHECK(_suite, errno == EBADF);

    dtest_test(_suite, "negative length -> -1, EINVAL");
    d_fs_test_write(path, "data", 4);
    fd    = d_file_open(path, O_RDWR);
    errno = 0;
    D_FS_CHECK(_suite, (fd >= 0) && (d_file_truncate_fd(fd, -1) == -1));
    D_FS_CHECK(_suite, errno == EINVAL);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    d_file_remove(path);
#else
    dtest_block(_suite, "d_file_truncate_fd");

    dtest_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_truncate_fd(0, 0) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * V.e  d_file_truncate_stream
 *****************************************************************************/

static void
d_fs_tests_seek_ftruncate_stream
(
    dtest_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  truncated;

    dtest_block(_suite, "d_file_truncate_stream");

    d_fs_test_path(path, sizeof(path), "ftruncate_stream.txt");

    dtest_test(_suite, "truncates the file behind the stream");
    d_fs_test_write(path, "Hello World! This is content.", 29);
    file      = d_file_open_stream(path, "r+b");
    truncated = false;

    if (file)
    {
        truncated = (d_file_truncate_stream(file, 15) == 0);
        d_file_close_stream(file);
        truncated = truncated && (d_file_size(path) == 15);
    }

    D_FS_CHECK(_suite, truncated);

    d_file_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_truncate_stream(NULL, 10) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "negative length -> -1, EINVAL");
    d_fs_test_write(path, "data", 4);
    file  = d_file_open_stream(path, "r+b");
    errno = 0;
    D_FS_CHECK(_suite, (file != NULL) && (d_file_truncate_stream(file, -1) == -1));
    D_FS_CHECK(_suite, errno == EINVAL);

    if (file)
    {
        d_file_close_stream(file);
    }

    d_file_remove(path);
#else
    dtest_block(_suite, "d_file_truncate_stream");

    dtest_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_truncate_stream(stdin, 0) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_seek
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "V. file_seek");

    d_fs_tests_seek_fseeko(_suite);
    d_fs_tests_seek_ftello(_suite);
    d_fs_tests_seek_rewind(_suite);
    d_fs_tests_seek_ftruncate(_suite);
    d_fs_tests_seek_ftruncate_stream(_suite);

    return;
}
