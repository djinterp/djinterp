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
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * V.a  d_file_seek_stream
 *****************************************************************************/

static
void d_fs_tests_seek_fseeko(
    struct dj_test_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  set_ok;
    bool  cur_ok;
    bool  end_ok;

    d_test_suite_block(_suite, "d_file_seek_stream");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    d_test_suite_test(_suite,
                      "SEEK_SET / SEEK_CUR / SEEK_END land where expected");
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
    d_test_suite_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_seek_stream(NULL, 0, SEEK_SET) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    d_test_suite_test(_suite, "an invalid whence -> -1, EINVAL");
    file  = d_file_open_stream(path, "rb");
    errno = 0;
    D_FS_CHECK(_suite,
               (file != NULL) && (d_file_seek_stream(file, 0, 999) == -1));
    D_FS_CHECK(_suite, errno == EINVAL);

    if (file)
    {
        d_file_close_stream(file);
    }
}


/******************************************************************************
 * V.b  d_file_tell_stream
 *****************************************************************************/

static
void d_fs_tests_seek_ftello(
    struct dj_test_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  at_start;
    bool  after_seek;

    d_test_suite_block(_suite, "d_file_tell_stream");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    d_test_suite_test(_suite,
                      "reports 0 at the start and the offset after a seek");
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
    d_test_suite_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_tell_stream(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * V.c  d_file_rewind_stream
 *****************************************************************************/

static
void d_fs_tests_seek_rewind(
    struct dj_test_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  rewound;

    d_test_suite_block(_suite, "d_file_rewind_stream");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    d_test_suite_test(_suite, "returns the position to the start");
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
    d_test_suite_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_rewind_stream(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
}


/******************************************************************************
 * V.d  d_file_truncate_fd
 *****************************************************************************/

static
void d_fs_tests_seek_ftruncate(
    struct dj_test_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char path[D_FS_TEST_PATH_BUF];
    int  fd;
    bool shrank;
    bool extended;

    d_test_suite_block(_suite, "d_file_truncate_fd");

    d_fs_test_path(path, sizeof(path), "ftruncate.txt");

    d_test_suite_test(_suite, "shrinking discards the tail");
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

    d_test_suite_test(_suite, "extending grows the file (zero-filled)");
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

    d_test_suite_test(_suite, "negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_truncate_fd(-1, 10) == -1);
    D_FS_CHECK(_suite, errno == EBADF);

    d_test_suite_test(_suite, "negative length -> -1, EINVAL");
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
    d_test_suite_block(_suite, "d_file_truncate_fd");

    d_test_suite_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_truncate_fd(0, 0) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * V.e  d_file_truncate_stream
 *****************************************************************************/

static
void d_fs_tests_seek_ftruncate_stream(
    struct dj_test_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    bool  truncated;

    d_test_suite_block(_suite, "d_file_truncate_stream");

    d_fs_test_path(path, sizeof(path), "ftruncate_stream.txt");

    d_test_suite_test(_suite, "truncates the file behind the stream");
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
    d_test_suite_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_truncate_stream(NULL, 10) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    d_test_suite_test(_suite, "negative length -> -1, EINVAL");
    d_fs_test_write(path, "data", 4);
    file  = d_file_open_stream(path, "r+b");
    errno = 0;
    D_FS_CHECK(_suite,
               (file != NULL) && (d_file_truncate_stream(file, -1) == -1));
    D_FS_CHECK(_suite, errno == EINVAL);

    if (file)
    {
        d_file_close_stream(file);
    }

    d_file_remove(path);
#else
    d_test_suite_block(_suite, "d_file_truncate_stream");

    d_test_suite_test(_suite, "ISO C backend -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_truncate_stream(stdin, 0) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void d_fs_tests_seek(
    struct dj_test_suite* _suite
)
{
    d_test_suite_module(_suite, "V. file_seek");

    d_fs_tests_seek_fseeko(_suite);
    d_fs_tests_seek_ftello(_suite);
    d_fs_tests_seek_rewind(_suite);
    d_fs_tests_seek_ftruncate(_suite);
    d_fs_tests_seek_ftruncate_stream(_suite);

    return;
}

/******************************************************************************
 * d_tests_file_seek_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * V.  file_seek  --  POSITION AND EXTENT
 *****************************************************************************/

/*
d_tests_file_seek_tell
  Tests d_file_seek_stream / d_file_tell_stream / d_file_rewind_stream.
*/
static
bool d_tests_file_seek_tell(struct dj_test_suite* _suite)
{
    char                  path[D_TEST_FILE_PATH_BUF];
    FILE*                 file;

    d_test_suite_test(_suite,
                      "d_file_seek_stream / d_file_tell_stream / d_file_rewind_stream");

    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    file = d_file_open_stream(path, "rb");

    if (file)
    {
        D_TEST_CHECK_MSG(_suite,
                         "tell_start",
                         (d_file_tell_stream(file)) == ((dj_off_t)0),
                         "a fresh stream is at 0");
        D_TEST_CHECK_MSG(_suite,
                         "seek_set",
                         (d_file_seek_stream(file, 5, SEEK_SET)) == (0),
                         "d_file_seek_stream seeks from the start");
        D_TEST_CHECK_MSG(_suite,
                         "tell_after_set",
                         (d_file_tell_stream(file)) == ((dj_off_t)5),
                         "...and d_file_tell_stream agrees");
        D_TEST_CHECK_MSG(_suite,
                         "seek_cur",
                         (d_file_seek_stream(file, 3, SEEK_CUR)) == (0),
                         "d_file_seek_stream seeks relatively");
        D_TEST_CHECK_MSG(_suite,
                         "tell_after_cur",
                         (d_file_tell_stream(file)) == ((dj_off_t)8),
                         "...cumulatively");
        D_TEST_CHECK_MSG(_suite,
                         "seek_end",
                         (d_file_seek_stream(file, -5, SEEK_END)) == (0),
                         "d_file_seek_stream seeks from the end");
        D_TEST_CHECK_MSG(_suite,
                         "tell_after_end",
                         (d_file_tell_stream(file)) == ((dj_off_t)(strlen(D_TEST_FILE_TEST_CONTENT) - 5)),
                         "...to the right place");
        D_TEST_CHECK_MSG(_suite,
                         "rewind",
                         (d_file_rewind_stream(file)) == (0),
                         "d_file_rewind_stream reports success, unlike rewind()");
        D_TEST_CHECK_MSG(_suite,
                         "rewind_pos",
                         (d_file_tell_stream(file)) == ((dj_off_t)0),
                         "...and returns to 0");
        D_TEST_CHECK_MSG(_suite,
                         "bad_whence",
                         (d_file_seek_stream(file, 0, 99)) == (-1),
                         "an invalid whence is rejected");
        (void)d_file_close_stream(file);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "seek_null",
                     (d_file_seek_stream(NULL, 0, SEEK_SET)) == (-1),
                     "d_file_seek_stream rejects NULL");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "tell_null",
                     (d_file_tell_stream(NULL)) == ((dj_off_t)-1),
                     "d_file_tell_stream rejects NULL");
    #endif

    return true;
}


/*
d_tests_file_truncate
  Tests d_file_truncate_fd and d_file_truncate_stream -- including the dirty-buffer trap.
*/
static
bool d_tests_file_truncate(struct dj_test_suite* _suite)
{
    char                  path[D_TEST_FILE_PATH_BUF];
    int                   fd;

    d_test_suite_test(_suite, "d_file_truncate_fd");

    d_tests_file_path(path, sizeof(path), "truncate.txt");
    (void)d_file_write_all(path, "Hello World! This is test content.", 34);

    fd = d_file_open(path, O_RDWR);

    if (fd >= 0)
    {
        D_TEST_CHECK_MSG(_suite,
                         "shrink",
                         (d_file_truncate_fd(fd, 10)) == (0),
                         "d_file_truncate_fd shrinks a file");
        (void)d_file_close_fd(fd);
        D_TEST_CHECK_MSG(_suite,
                         "shrink_size",
                         (d_file_size(path)) == ((int64_t)10),
                         "...to exactly the requested length");
    }

    fd = d_file_open(path, O_RDWR);

    if (fd >= 0)
    {
        // extending is not a no-op: it zero-fills, and may make the file
        // sparse
        D_TEST_CHECK_MSG(_suite,
                         "extend",
                         (d_file_truncate_fd(fd, 50)) == (0),
                         "d_file_truncate_fd extends a file");
        (void)d_file_close_fd(fd);
        D_TEST_CHECK_MSG(_suite,
                         "extend_size",
                         (d_file_size(path)) == ((int64_t)50),
                         "...to exactly the requested length");
    }

    D_TEST_CHECK_MSG(_suite,
                     "bad_fd",
                     (d_file_truncate_fd(-1, 10)) == (-1),
                     "d_file_truncate_fd rejects a bad descriptor");
    D_TEST_CHECK_MSG(_suite,
                     "negative",
                     (d_file_truncate_fd(0, -5)) == (-1),
                     "a negative length is rejected");

#if D_CFG_IS_ON(D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE)
    // THE trap. Truncation acts on the descriptor; stdio's buffer is
    // invisible to it. With 200 bytes pending and a truncate to 10, an
    // unflushed build writes them out AFTERWARDS and the file ends up 200
    // bytes -- while the call returns 0. Nothing fails. The file is wrong.
    {
        char  dirty[D_TEST_FILE_PATH_BUF];
        FILE* file;
        int   i;

        d_tests_file_path(dirty, sizeof(dirty), "dirty.txt");
        file = d_file_open_stream(dirty, "wb");

        if (file)
        {
            for (i = 0; i < 200; ++i)
            {
                (void)fputc('X', file);
            }

            D_TEST_CHECK_MSG(_suite,
                             "dirty_buffer",
                             (d_file_truncate_stream(file, 10)) == (0),
                             "d_file_truncate_stream flushes first");
            (void)d_file_close_stream(file);
            D_TEST_CHECK_MSG(_suite,
                             "dirty_buffer_size",
                             (d_file_size(dirty)) == ((int64_t)10),
                             "...so the truncation is not undone by stdio");
        }

        (void)d_file_remove(dirty);
    }
#endif

    (void)d_file_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "stream_null",
                     (d_file_truncate_stream(NULL, 10)) == (-1),
                     "d_file_truncate_stream rejects NULL");
    #endif

    return true;
}


/*
d_tests_file_seek_all
  Runs every file_seek test.
*/
bool d_tests_file_seek_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "V. file_seek");

    d_tests_file_seek_tell(_suite);
    d_tests_file_truncate(_suite);

    return true;
}
