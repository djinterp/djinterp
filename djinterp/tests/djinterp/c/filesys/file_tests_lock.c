/*******************************************************************************
* djinterp [c][filesys]                                        file_tests_lock.c
*
*   VII. Locking (file_lock) -- advisory whole-file locks. Descriptor-based, so
* ENOSYS on the ISO C backend. Elsewhere a process takes and releases its own
* shared and exclusive locks (non-blocking, to keep the suite from ever
* stalling), and the operation-validation guard is checked: exactly one of
* shared / exclusive / unlock is required, so both "two modes at once" and "no
* mode at all" are rejected.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_lock.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * VII.a  d_file_lock_fd
 *****************************************************************************/

static
void d_fs_tests_lock_flock(
    struct dj_test_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char path[D_FS_TEST_PATH_BUF];
    int  fd;
    bool exclusive;
    bool shared;

    d_test_suite_block(_suite, "d_file_lock_fd");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    d_test_suite_test(_suite, "take and release an exclusive lock");
    fd        = d_file_open(path, O_RDWR);
    exclusive = false;

    if (fd >= 0)
    {
        exclusive = (d_file_lock_fd(fd, D_LOCK_EX | D_LOCK_NB) == 0) &&
                    (d_file_lock_fd(fd, D_LOCK_UN) == 0);
        d_file_close_fd(fd);
    }

    D_FS_CHECK(_suite, exclusive);

    d_test_suite_test(_suite, "take and release a shared lock");
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
    d_test_suite_test(_suite, "two modes at once -> -1, EINVAL");
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
    d_test_suite_test(_suite, "no mode at all -> -1, EINVAL");
    fd    = d_file_open(path, O_RDWR);
    errno = 0;
    D_FS_CHECK(_suite, (fd >= 0) && (d_file_lock_fd(fd, 0) == -1));
    D_FS_CHECK(_suite, errno == EINVAL);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

#endif
    d_test_suite_test(_suite, "negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_lock_fd(-1, D_LOCK_EX) == -1);
    D_FS_CHECK(_suite, errno == EBADF);
#else
    d_test_suite_block(_suite, "d_file_lock_fd");

    d_test_suite_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_lock_fd(0, D_LOCK_EX) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * VII.b  d_file_lock_stream
 *****************************************************************************/

static
void d_fs_tests_lock_flock_stream(
    struct dj_test_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  locked;

    d_test_suite_block(_suite, "d_file_lock_stream");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    d_test_suite_test(_suite, "take and release a lock through a stream");
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
    d_test_suite_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_lock_stream(NULL, D_LOCK_EX) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
#else
    d_test_suite_block(_suite, "d_file_lock_stream");

    d_test_suite_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_lock_stream(stdin, D_LOCK_EX) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void d_fs_tests_lock(
    struct dj_test_suite* _suite
)
{
    d_test_suite_module(_suite, "VII. file_lock");

    d_fs_tests_lock_flock(_suite);
    d_fs_tests_lock_flock_stream(_suite);

    return;
}

/******************************************************************************
 * d_tests_file_lock_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * VII.  file_lock  --  ADVISORY LOCKING
 *
 *   Single-process tests, which is a real limit worth naming: contention is
 * the interesting behaviour and it needs a second process, which this harness
 * does not have. So what is covered is the contract -- operations are
 * accepted, contradictions are refused, the backend is what the config says.
 *****************************************************************************/

/*
d_tests_file_flock
  Tests d_file_lock_fd.
*/
static
bool d_tests_file_flock(struct dj_test_suite* _suite)
{
    char                  path[D_TEST_FILE_PATH_BUF];
    int                   fd;

    d_test_suite_test(_suite,
                      D_FILE_LOCK_IS_PER_DESCRIPTION
                          ? "d_file_lock_fd (flock: per open file description)"
                          : "d_file_lock_fd (fcntl: per process -- ANY close drops it)");

    d_tests_file_path(path, sizeof(path), "flock.txt");
    (void)d_file_write_all(path, "test", 4);

    fd = d_file_open(path, O_RDWR);

    if (fd >= 0)
    {
        D_TEST_CHECK_MSG(_suite,
                         "shared",
                         (d_file_lock_fd(fd, D_LOCK_SH)) == (0),
                         "d_file_lock_fd takes a shared lock");
        D_TEST_CHECK_MSG(_suite,
                         "unlock",
                         (d_file_lock_fd(fd, D_LOCK_UN)) == (0),
                         "d_file_lock_fd releases it");
        D_TEST_CHECK_MSG(_suite,
                         "exclusive",
                         (d_file_lock_fd(fd, D_LOCK_EX)) == (0),
                         "d_file_lock_fd takes an exclusive lock");
        D_TEST_CHECK_MSG(_suite,
                         "nonblocking",
                         (d_file_lock_fd(fd, D_LOCK_EX | D_LOCK_NB)) == (0),
                         "re-locking what we hold does not block");
        (void)d_file_lock_fd(fd, D_LOCK_UN);
        (void)d_file_close_fd(fd);
    }

    D_TEST_CHECK_MSG(_suite,
                     "bad_fd",
                     (d_file_lock_fd(-1, D_LOCK_SH)) == (-1),
                     "d_file_lock_fd rejects a bad descriptor");

    // SH|EX is not a richer request, it is a contradiction -- and a platform
    // handed the OR of two flags does something arbitrary rather than object
    //   Gated: the refusal IS the parameter check. With validation off the
    // operation word is passed through unexamined, which is what the knob
    // asks for, so there is nothing here to assert -- and asserting it anyway
    // was reporting the build as broken for doing exactly what it was told.
#if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "contradiction",
                     (d_file_lock_fd(0, D_LOCK_SH | D_LOCK_EX)) == (-1),
                     "SH|EX is refused, not guessed at");
    D_TEST_CHECK_MSG(_suite,
                     "no_operation",
                     (d_file_lock_fd(0, D_LOCK_NB)) == (-1),
                     "NB alone names no operation");
#endif

    (void)d_file_remove(path);

    return true;
}


/*
d_tests_file_flock_stream
  Tests d_file_lock_stream.
*/
static
bool d_tests_file_flock_stream(struct dj_test_suite* _suite)
{
    char                  path[D_TEST_FILE_PATH_BUF];
    FILE*                 file;

    d_test_suite_test(_suite, "d_file_lock_stream");

    d_tests_file_path(path, sizeof(path), "flock_stream.txt");
    (void)d_file_write_all(path, "test", 4);

    file = d_file_open_stream(path, "r+b");

    if (file)
    {
        D_TEST_CHECK_MSG(_suite,
                         "lock",
                         (d_file_lock_stream(file, D_LOCK_SH)) == (0),
                         "d_file_lock_stream locks through a stream");
        (void)d_file_lock_stream(file, D_LOCK_UN);
        (void)d_file_close_stream(file);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     (d_file_lock_stream(NULL, D_LOCK_SH)) == (-1),
                     "d_file_lock_stream rejects NULL");
    #endif

    (void)d_file_remove(path);

    return true;
}


/*
d_tests_file_lock_all
  Runs every file_lock test.
*/
bool d_tests_file_lock_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "VII. file_lock");

    d_tests_file_flock(_suite);
    d_tests_file_flock_stream(_suite);

    return true;
}
