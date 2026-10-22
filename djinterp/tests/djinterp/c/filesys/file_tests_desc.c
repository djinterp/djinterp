/*******************************************************************************
* djinterp [test]                                              file_tests_desc.c
*
*   III. DESCRIPTORS (file_desc) -- acquiring, duplicating, and releasing raw
* descriptors. The module is descriptor-based to its core, so on the ISO C
* backend every entry point can only report ENOSYS; that branch is checked
* directly, and the full behaviour is exercised on a backend that has
* descriptors. Notable cases: d_file_open's O_CREAT-with-mode-0 recovery, d_file_dup2_fd's
* documented same-descriptor no-op, and the negative-descriptor guards.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_desc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.10.20
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
// djinterp
#include "file_tests.h"


#if D_FILE_BACKEND_IS_STDC

/******************************************************************************
 * ISO C BACKEND: every descriptor call is ENOSYS
 *****************************************************************************/

static void
d_fs_tests_desc_stdc
(
    dtest_suite* _suite
)
{
    dtest_block(_suite, "ISO C backend");

    dtest_test(_suite, "d_file_open -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_open("x", O_RDONLY) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);

    dtest_test(_suite, "d_file_descriptor_stream -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_descriptor_stream(stdin) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);

    dtest_test(_suite, "d_file_dup_fd -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_dup_fd(0) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);

    dtest_test(_suite, "d_file_dup2_fd -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_dup2_fd(0, 1) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);

    dtest_test(_suite, "d_file_close_fd -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_close_fd(0) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
}

#else

/******************************************************************************
 * III.a  d_file_open
 *****************************************************************************/

static void
d_fs_tests_desc_open
(
    dtest_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];
    int  fd;

    dtest_block(_suite, "d_file_open");

    dtest_test(_suite, "opens an existing file for reading");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    fd = d_file_open(path, O_RDONLY);
    D_FS_CHECK(_suite, fd >= 0);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    dtest_test(_suite, "creates a new file with O_CREAT");
    d_fs_test_path(path, sizeof(path), "open_created.txt");
    fd = d_file_open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    D_FS_CHECK(_suite, fd >= 0);
    D_FS_CHECK(_suite, d_file_exists(path));

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    d_file_remove(path);

    dtest_test(_suite, "O_CREAT with mode 0 still creates (configured default)");
    d_fs_test_path(path, sizeof(path), "open_mode0.txt");
    fd = d_file_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0);
    D_FS_CHECK(_suite, fd >= 0);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    d_file_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_open(NULL, O_RDONLY) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "a missing file without O_CREAT -> -1");
    d_fs_test_path(path, sizeof(path), "no_such_open_file.txt");
    D_FS_CHECK(_suite, d_file_open(path, O_RDONLY) == -1);
}


/******************************************************************************
 * III.b  d_file_descriptor_stream
 *****************************************************************************/

static void
d_fs_tests_desc_fileno
(
    dtest_suite* _suite
)
{
    char  path[D_FS_TEST_PATH_BUF];
    FILE* file;
    int   fd;

    dtest_block(_suite, "d_file_descriptor_stream");

    dtest_test(_suite, "returns a valid descriptor for an open stream");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    file = d_file_open_stream(path, "rb");
    fd   = file ? d_file_descriptor_stream(file) : -1;
    D_FS_CHECK(_suite, fd >= 0);

    if (file)
    {
        d_file_close_stream(file);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_descriptor_stream(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * III.c  d_file_dup_fd
 *****************************************************************************/

static void
d_fs_tests_desc_dup
(
    dtest_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];
    int  fd;
    int  dup_fd;

    dtest_block(_suite, "d_file_dup_fd");

    dtest_test(_suite, "returns a new, distinct descriptor");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    fd     = d_file_open(path, O_RDONLY);
    dup_fd = (fd >= 0) ? d_file_dup_fd(fd) : -1;
    D_FS_CHECK(_suite, dup_fd >= 0);
    D_FS_CHECK(_suite, dup_fd != fd);

    if (dup_fd >= 0)
    {
        d_file_close_fd(dup_fd);
    }

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    dtest_test(_suite, "negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_dup_fd(-1) == -1);
    D_FS_CHECK(_suite, errno == EBADF);
}


/******************************************************************************
 * III.d  d_file_dup2_fd
 *****************************************************************************/

static void
d_fs_tests_desc_dup2
(
    dtest_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];
    int  fd;
    int  other;
    int  rc;

    dtest_block(_suite, "d_file_dup2_fd");

    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);

    dtest_test(_suite, "duplicates onto a chosen descriptor number");
    fd    = d_file_open(path, O_RDONLY);
    other = d_file_open(path, O_RDONLY);
    rc    = ((fd >= 0) && (other >= 0)) ? d_file_dup2_fd(fd, other) : -2;
    D_FS_CHECK(_suite, rc == other);

    if (other >= 0)
    {
        d_file_close_fd(other);
    }

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    dtest_test(_suite, "same-descriptor duplication is a no-op that succeeds");
    fd = d_file_open(path, O_RDONLY);
    rc = (fd >= 0) ? d_file_dup2_fd(fd, fd) : -2;
    D_FS_CHECK(_suite, rc == fd);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    dtest_test(_suite, "negative source -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_dup2_fd(-1, 10) == -1);
    D_FS_CHECK(_suite, errno == EBADF);

    dtest_test(_suite, "negative target -> -1, EBADF");
    fd    = d_file_open(path, O_RDONLY);
    errno = 0;
    rc    = (fd >= 0) ? d_file_dup2_fd(fd, -1) : -2;
    D_FS_CHECK(_suite, rc == -1);
    D_FS_CHECK(_suite, errno == EBADF);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }
}


/******************************************************************************
 * III.e  d_file_close_fd
 *****************************************************************************/

static void
d_fs_tests_desc_close
(
    dtest_suite* _suite
)
{
    char path[D_FS_TEST_PATH_BUF];
    int  fd;

    dtest_block(_suite, "d_file_close_fd");

    dtest_test(_suite, "closing a valid descriptor -> 0");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    fd = d_file_open(path, O_RDONLY);
    D_FS_CHECK(_suite, (fd >= 0) && (d_file_close_fd(fd) == 0));

    dtest_test(_suite, "negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_close_fd(-1) == -1);
    D_FS_CHECK(_suite, errno == EBADF);
}

#endif  // D_FILE_BACKEND_IS_STDC


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_desc
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "III. file_desc");

#if D_FILE_BACKEND_IS_STDC
    d_fs_tests_desc_stdc(_suite);
#else
    d_fs_tests_desc_open(_suite);
    d_fs_tests_desc_fileno(_suite);
    d_fs_tests_desc_dup(_suite);
    d_fs_tests_desc_dup2(_suite);
    d_fs_tests_desc_close(_suite);
#endif

    return;
}
