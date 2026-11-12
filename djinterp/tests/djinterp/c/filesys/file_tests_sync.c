/*******************************************************************************
* djinterp [c][filesys]                                        file_tests_sync.c
*
*   VI. Durability (file_sync) -- flushing to the operating system and to the
* device. The two descriptor-flushing forms report ENOSYS on the ISO C backend
* and are checked directly there; elsewhere a written file is flushed to
* success. d_file_flush_stream runs on every backend and, following C, treats a NULL
* stream as "flush every open output stream" rather than an error.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_sync.c
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
 * VI.a  d_file_sync_fd
 *****************************************************************************/

static
void d_fs_tests_sync_fsync(
    struct dj_test_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char path[D_FS_TEST_PATH_BUF];
    int  fd;
    bool synced;

    d_test_suite_block(_suite, "d_file_sync_fd");

    d_fs_test_path(path, sizeof(path), "fsync.txt");

    d_test_suite_test(_suite, "flushes a written descriptor to success");
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

    d_test_suite_test(_suite, "negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_sync_fd(-1) == -1);
    D_FS_CHECK(_suite, errno == EBADF);
#else
    d_test_suite_block(_suite, "d_file_sync_fd");

    d_test_suite_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_sync_fd(0) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * VI.b  d_file_sync_stream
 *****************************************************************************/

static
void d_fs_tests_sync_fsync_stream(
    struct dj_test_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  synced;

    d_test_suite_block(_suite, "d_file_sync_stream");

    d_fs_test_path(path, sizeof(path), "fsync_stream.txt");

    d_test_suite_test(_suite, "flushes a written stream to success");
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
    d_test_suite_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_sync_stream(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
#else
    d_test_suite_block(_suite, "d_file_sync_stream");

    d_test_suite_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_sync_stream(stdout) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * VI.c  d_file_flush_stream
 *****************************************************************************/

static
void d_fs_tests_sync_fflush(
    struct dj_test_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  flushed;

    d_test_suite_block(_suite, "d_file_flush_stream");

    d_fs_test_path(path, sizeof(path), "fflush.txt");

    d_test_suite_test(_suite, "flushes a writable stream -> 0");
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
    d_test_suite_test(_suite, "NULL stream flushes all open streams -> 0");
    D_FS_CHECK(_suite, d_file_flush_stream(NULL) == 0);
    #endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void d_fs_tests_sync(
    struct dj_test_suite* _suite
)
{
    d_test_suite_module(_suite, "VI. file_sync");

    d_fs_tests_sync_fsync(_suite);
    d_fs_tests_sync_fsync_stream(_suite);
    d_fs_tests_sync_fflush(_suite);

    return;
}

/******************************************************************************
 * d_tests_file_sync_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * VI.  file_sync  --  DURABILITY
 *
 *   These tests can verify that the calls SUCCEED. They cannot verify that
 * the data survives a power cut, which is what the module is actually for --
 * that needs hardware nobody has in CI. So what is asserted is the contract's
 * shape: the calls work, they reject nonsense, and D_FILE_SYNC_REACHES_PLATTER
 * says which promise this build makes.
 *****************************************************************************/

/*
d_tests_file_fsync
  Tests d_file_sync_fd and d_file_sync_stream.
*/
static
bool d_tests_file_fsync(struct dj_test_suite* _suite)
{
    char                  path[D_TEST_FILE_PATH_BUF];
    FILE*                 file;
    int                   fd;

    d_test_suite_test(_suite,
                      D_FILE_SYNC_REACHES_PLATTER
                          ? "d_file_sync_fd (reaches the device)"
                          : "d_file_sync_fd (kernel only -- see D_CFG_FILE_SYNC_FULL)");

    d_tests_file_path(path, sizeof(path), "fsync.txt");

    fd = d_file_open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd >= 0)
    {
        (void)d_file_write_fd(fd, "test", 4);
        D_TEST_CHECK_MSG(_suite,
                         "valid_fd",
                         (d_file_sync_fd(fd)) == (0),
                         "d_file_sync_fd succeeds on a valid descriptor");
        (void)d_file_close_fd(fd);
    }

    D_TEST_CHECK_MSG(_suite,
                     "bad_fd",
                     (d_file_sync_fd(-1)) == (-1),
                     "d_file_sync_fd rejects a bad descriptor");

    file = d_file_open_stream(path, "wb");

    if (file)
    {
        (void)fprintf(file, "test data");
        D_TEST_CHECK_MSG(_suite,
                         "stream",
                         (d_file_sync_stream(file)) == (0),
                         "d_file_sync_stream flushes then syncs");
        (void)d_file_close_stream(file);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "stream_null",
                     (d_file_sync_stream(NULL)) == (-1),
                     "d_file_sync_stream rejects NULL");
    #endif

    (void)d_file_remove(path);

    return true;
}


/*
d_tests_file_fflush
  Tests d_file_flush_stream.
*/
static
bool d_tests_file_fflush(struct dj_test_suite* _suite)
{
    char                  path[D_TEST_FILE_PATH_BUF];
    FILE*                 file;

    d_test_suite_test(_suite, "d_file_flush_stream");

    d_tests_file_path(path, sizeof(path), "fflush.txt");

    file = d_file_open_stream(path, "wb");

    if (file)
    {
        (void)fprintf(file, "test data");
        D_TEST_CHECK_MSG(_suite,
                         "valid",
                         (d_file_flush_stream(file)) == (0),
                         "d_file_flush_stream hands the buffer to the kernel");
        (void)d_file_close_stream(file);
    }

    // NULL is a documented, meaningful argument here -- flush every output
    // stream -- rather than a mistake to reject
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_flushes_all",
                     (d_file_flush_stream(NULL)) == (0),
                     "d_file_flush_stream(NULL) flushes all streams, per ISO C");
    #endif

    (void)d_file_remove(path);

    return true;
}


/*
d_tests_file_sync_all
  Runs every file_sync test.
*/
bool d_tests_file_sync_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "VI. file_sync");

    d_tests_file_fsync(_suite);
    d_tests_file_fflush(_suite);

    return true;
}
