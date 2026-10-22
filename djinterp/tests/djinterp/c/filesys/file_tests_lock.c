/*******************************************************************************
* djinterp [test]                                              file_tests_lock.c
*
*   VII. LOCKING (file_lock) -- advisory whole-file locks. Descriptor-based, so
* ENOSYS on the ISO C backend. Elsewhere a process takes and releases its own
* shared and exclusive locks (non-blocking, to keep the suite from ever
* stalling), and the operation-validation guard is checked: exactly one of
* SHARED / EXCLUSIVE / UNLOCK is required, so both "two modes at once" and "no
* mode at all" are rejected.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_lock.c
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
 * VII.a  d_file_lock_fd
 *****************************************************************************/

static void
d_fs_tests_lock_flock
(
    dtest_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char path[D_FS_TEST_PATH_BUF];
    int  fd;
    bool exclusive;
    bool shared;

    dtest_block(_suite, "d_file_lock_fd");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "take and release an exclusive lock");
    fd        = d_file_open(path, O_RDWR);
    exclusive = false;

    if (fd >= 0)
    {
        exclusive = (d_file_lock_fd(fd, D_LOCK_EX | D_LOCK_NB) == 0) &&
                    (d_file_lock_fd(fd, D_LOCK_UN) == 0);
        d_file_close_fd(fd);
    }

    D_FS_CHECK(_suite, exclusive);

    dtest_test(_suite, "take and release a shared lock");
    fd     = d_file_open(path, O_RDONLY);
    shared = false;

    if (fd >= 0)
    {
        shared = (d_file_lock_fd(fd, D_LOCK_SH | D_LOCK_NB) == 0) &&
                 (d_file_lock_fd(fd, D_LOCK_UN) == 0);
        d_file_close_fd(fd);
    }

    D_FS_CHECK(_suite, shared);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "two modes at once -> -1, EINVAL");
    fd    = d_file_open(path, O_RDWR);
    errno = 0;
    D_FS_CHECK(_suite,
               (fd >= 0) && (d_file_lock_fd(fd, D_LOCK_SH | D_LOCK_EX) == -1));
    D_FS_CHECK(_suite, errno == EINVAL);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

#endif
#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "no mode at all -> -1, EINVAL");
    fd    = d_file_open(path, O_RDWR);
    errno = 0;
    D_FS_CHECK(_suite, (fd >= 0) && (d_file_lock_fd(fd, 0) == -1));
    D_FS_CHECK(_suite, errno == EINVAL);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

#endif
    dtest_test(_suite, "negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_lock_fd(-1, D_LOCK_EX) == -1);
    D_FS_CHECK(_suite, errno == EBADF);
#else
    dtest_block(_suite, "d_file_lock_fd");

    dtest_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_lock_fd(0, D_LOCK_EX) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * VII.b  d_file_lock_stream
 *****************************************************************************/

static void
d_fs_tests_lock_flock_stream
(
    dtest_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  locked;

    dtest_block(_suite, "d_file_lock_stream");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "take and release a lock through a stream");
    file   = d_file_open_stream(path, "rb");
    locked = false;

    if (file)
    {
        locked = (d_file_lock_stream(file, D_LOCK_SH | D_LOCK_NB) == 0) &&
                 (d_file_lock_stream(file, D_LOCK_UN) == 0);
        d_file_close_stream(file);
    }

    D_FS_CHECK(_suite, locked);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_lock_stream(NULL, D_LOCK_EX) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
#else
    dtest_block(_suite, "d_file_lock_stream");

    dtest_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_lock_stream(stdin, D_LOCK_EX) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_lock
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "VII. file_lock");

    d_fs_tests_lock_flock(_suite);
    d_fs_tests_lock_flock_stream(_suite);

    return;
}
