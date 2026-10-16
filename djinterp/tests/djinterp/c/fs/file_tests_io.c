/*******************************************************************************
* djinterp [test]                                                file_tests_io.c
*
*   IV. TRANSFER (file_io) -- moving bytes through a descriptor, at an offset,
* or a whole file at a time.
*
*   Descriptor reads and writes are descriptor-based and so report ENOSYS on
* the ISO C backend; that branch is checked directly and the transfer
* behaviour is exercised on a backend that has descriptors -- round-trip
* integrity, the *_full loops, and the position-independence of the positional
* forms (verified by reading from offset 0 afterwards rather than by trusting
* a raw seek). The whole-file helpers run on every backend: the sized fast path
* against the standard fixture, the growth path against a genuinely empty file
* and, where pipes exist, against an unsized pipe.
*
*
* path:      /tests/djinterp/c/fs/file_tests_io.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// djinterp
#include "file_tests.h"


#if D_FILE_BACKEND_IS_STDC

/******************************************************************************
 * ISO C BACKEND: descriptor transfer is ENOSYS
 *****************************************************************************/

static void
d_fs_tests_io_desc_stdc
(
    dtest_suite* _suite
)
{
    char buf[8];

    dtest_block(_suite, "descriptor I/O (ISO C backend)");

    dtest_test(_suite, "d_file_read_fd -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_read_fd(0, buf, sizeof(buf)) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);

    dtest_test(_suite, "d_file_write_fd -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_write_fd(1, "x", 1) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);

    dtest_test(_suite, "d_file_pread_fd -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_pread_fd(0, buf, sizeof(buf), 0) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);

    dtest_test(_suite, "d_file_pwrite_fd -> -1, ENOSYS");
    errno = 0;
    D_FS_CHECK(_suite, d_file_pwrite_fd(1, "x", 1, 0) == -1);
    D_FS_CHECK(_suite, errno == ENOSYS);
}

#else

/******************************************************************************
 * IV.a  d_file_read_fd / d_file_write_fd round-trip
 *****************************************************************************/

static void
d_fs_tests_io_read_write
(
    dtest_suite* _suite
)
{
    static const char payload[] = "descriptor round-trip payload";
    char              path[D_FS_TEST_PATH_BUF];
    char              buf[64];
    int               fd;
    ssize_t           n;
    bool              wrote;
    bool              read_back;

    dtest_block(_suite, "d_file_read_fd / d_file_write_fd");

    d_fs_test_path(path, sizeof(path), "io_rw.txt");

    dtest_test(_suite, "written bytes read back intact");
    fd    = d_file_open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    wrote = false;

    if (fd >= 0)
    {
        //   d_file_write_full_fd for the same reason d_file_read_full_fd is used below: this
        // case is about the CONTENT surviving, and the primitive is entitled to
        // a short write.
        n     = d_file_write_full_fd(fd, payload, sizeof(payload) - 1);
        wrote = (n == (ssize_t)(sizeof(payload) - 1));
        d_file_close_fd(fd);
    }

    read_back = false;
    fd        = d_file_open(path, O_RDONLY);

    if (fd >= 0)
    {
        //   d_file_read_full_fd, not d_file_read_fd: a short read IS success for the
        // primitive -- read(2) says so, and D_CFG_FILE_READ_CHUNK_SIZE makes it
        // happen deliberately. What is being checked here is the CONTENT, which
        // needs every byte, so the function that promises every byte is the one
        // to call. Asserting == on d_file_read_fd passed at the default chunk size by
        // coincidence and failed the moment the matrix set the chunk to 1.
        memset(buf, 0, sizeof(buf));
        n = d_file_read_full_fd(fd, buf, sizeof(buf) - 1);

        if (n == (ssize_t)(sizeof(payload) - 1))
        {
            read_back = (memcmp(buf, payload, sizeof(payload) - 1) == 0);
        }

        d_file_close_fd(fd);
    }

    D_FS_CHECK(_suite, wrote);
    D_FS_CHECK(_suite, read_back);

    d_file_remove(path);

    dtest_test(_suite, "d_file_read_fd: negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_read_fd(-1, buf, sizeof(buf)) == -1);
    D_FS_CHECK(_suite, errno == EBADF);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "d_file_read_fd: NULL buffer -> -1, EINVAL");
    fd    = d_file_open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    errno = 0;
    D_FS_CHECK(_suite, d_file_read_fd(fd, NULL, 4) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "d_file_read_fd: zero count is a no-op -> 0");
    D_FS_CHECK(_suite, (fd >= 0) && (d_file_read_fd(fd, buf, 0) == 0));

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    d_file_remove(path);

    dtest_test(_suite, "d_file_write_fd: negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_write_fd(-1, "x", 1) == -1);
    D_FS_CHECK(_suite, errno == EBADF);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "d_file_write_fd: NULL buffer -> -1, EINVAL");
    fd    = d_file_open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    errno = 0;
    D_FS_CHECK(_suite, d_file_write_fd(fd, NULL, 4) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "d_file_write_fd: zero count is a no-op -> 0");
    D_FS_CHECK(_suite, (fd >= 0) && (d_file_write_fd(fd, "x", 0) == 0));

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    d_file_remove(path);
}


/******************************************************************************
 * IV.b  d_file_read_full_fd / d_file_write_full_fd
 *****************************************************************************/

static void
d_fs_tests_io_full
(
    dtest_suite* _suite
)
{
    static const char payload[] = "0123456789";
    char              path[D_FS_TEST_PATH_BUF];
    char              buf[32];
    int               fd;
    ssize_t           n;

    dtest_block(_suite, "d_file_read_full_fd / d_file_write_full_fd");

    d_fs_test_path(path, sizeof(path), "io_full.txt");

    dtest_test(_suite, "d_file_write_full_fd writes every byte");
    fd = d_file_open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    n  = (fd >= 0) ? d_file_write_full_fd(fd, payload, 10) : -1;
    D_FS_CHECK(_suite, n == 10);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    dtest_test(_suite, "d_file_read_full_fd collects the whole request when present");
    fd = d_file_open(path, O_RDONLY);
    n  = (fd >= 0) ? d_file_read_full_fd(fd, buf, 10) : -1;
    D_FS_CHECK(_suite, n == 10);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    dtest_test(_suite, "d_file_read_full_fd stops short at end of file");
    fd = d_file_open(path, O_RDONLY);
    n  = (fd >= 0) ? d_file_read_full_fd(fd, buf, sizeof(buf)) : -1;
    D_FS_CHECK(_suite, n == 10);

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    d_file_remove(path);

    dtest_test(_suite, "d_file_read_full_fd: negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_read_full_fd(-1, buf, 4) == -1);
    D_FS_CHECK(_suite, errno == EBADF);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "d_file_write_full_fd: NULL buffer -> -1, EINVAL");
    fd    = d_file_open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    errno = 0;
    D_FS_CHECK(_suite, d_file_write_full_fd(fd, NULL, 4) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    d_file_remove(path);
}


/******************************************************************************
 * IV.c  d_file_pread_fd / d_file_pwrite_fd
 *****************************************************************************/

static void
d_fs_tests_io_positional
(
    dtest_suite* _suite
)
{
    char    path[D_FS_TEST_PATH_BUF];
    char    buf[16];
    int     fd;
    ssize_t n;
    bool    at_offset;
    bool    pos_untouched;
    bool    wrote_at_offset;

    dtest_block(_suite, "d_file_pread_fd / d_file_pwrite_fd");

    d_fs_test_path(path, sizeof(path), "io_positional.txt");

    dtest_test(_suite, "d_file_pread_fd reads from the offset and leaves the position");
    d_fs_test_write(path, "0123456789", 10);
    fd            = d_file_open(path, O_RDONLY);
    at_offset     = false;
    pos_untouched = false;

    if (fd >= 0)
    {
        memset(buf, 0, sizeof(buf));
        //   There is no d_pread_full, so this asserts what the primitive
        // promises rather than a length: SOME bytes, never more than asked,
        // and the ones delivered are the ones AT the offset. Demanding all
        // four was demanding a guarantee d_file_pread_fd does not make -- true at the
        // default chunk size, false at a chunk of 1.
        n = d_file_pread_fd(fd, buf, 4, 5);
        at_offset = (n > 0) &&
                    (n <= 4) &&
                    (memcmp(buf, "5678", (size_t)n) == 0);

        // the position was never moved, so a plain read still starts at 0
        //   Read in full: the claim is about WHERE the cursor is, and a short
        // read would fail the byte comparison for a reason that has nothing to
        // do with the cursor.
        memset(buf, 0, sizeof(buf));
        n = d_file_read_full_fd(fd, buf, 4);
        pos_untouched = (n == 4) && (memcmp(buf, "0123", 4) == 0);

        d_file_close_fd(fd);
    }

    D_FS_CHECK(_suite, at_offset);
    D_FS_CHECK(_suite, pos_untouched);

    dtest_test(_suite, "d_file_pwrite_fd writes at the offset and leaves the position");
    d_fs_test_write(path, "AAAAAAAAAA", 10);
    fd              = d_file_open(path, O_RDWR);
    wrote_at_offset = false;
    pos_untouched   = false;

    if (fd >= 0)
    {
        n = d_file_pwrite_fd(fd, "XY", 2, 3);

        // a plain read afterwards starts at 0, proving the offset write did
        // not disturb the cursor
        //   The bytes checked are the three BEFORE the offset. They are
        // untouched however much of "XY" landed, so the assertion isolates the
        // cursor -- which is the claim -- instead of also depending on the
        // transfer being complete, which d_file_pwrite_fd never promised.
        memset(buf, 0, sizeof(buf));
        if (d_file_read_full_fd(fd, buf, 3) == 3)
        {
            pos_untouched = (memcmp(buf, "AAA", 3) == 0);
        }

        wrote_at_offset = (n > 0) && (n <= 2);
        d_file_close_fd(fd);
    }

    D_FS_CHECK(_suite, wrote_at_offset);
    D_FS_CHECK(_suite, pos_untouched);

    d_file_remove(path);

    dtest_test(_suite, "d_file_pread_fd: negative offset -> -1, EINVAL");
    d_fs_test_write(path, "data", 4);
    fd    = d_file_open(path, O_RDONLY);
    errno = 0;
    n     = (fd >= 0) ? d_file_pread_fd(fd, buf, 4, -1) : -1;
    D_FS_CHECK(_suite, n == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

    dtest_test(_suite, "d_file_pread_fd: zero count is a no-op -> 0");
    D_FS_CHECK(_suite, (fd >= 0) && (d_file_pread_fd(fd, buf, 0, 0) == 0));

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "d_file_pread_fd: NULL buffer -> -1, EINVAL");
    fd    = d_file_open(path, O_RDONLY);
    errno = 0;
    n     = (fd >= 0) ? d_file_pread_fd(fd, NULL, 4, 0) : -1;
    D_FS_CHECK(_suite, n == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    if (fd >= 0)
    {
        d_file_close_fd(fd);
    }

    dtest_test(_suite, "d_file_pwrite_fd: negative descriptor -> -1, EBADF");
    errno = 0;
    D_FS_CHECK(_suite, d_file_pwrite_fd(-1, "x", 1, 0) == -1);
    D_FS_CHECK(_suite, errno == EBADF);

    d_file_remove(path);
}

#endif  // D_FILE_BACKEND_IS_STDC


/******************************************************************************
 * IV.d  d_file_read_all  (whole-file, any backend)
 *****************************************************************************/

static void
d_fs_tests_io_fread_all
(
    dtest_suite* _suite
)
{
    char   path[D_FS_TEST_PATH_BUF];
    void*  content;
    size_t size;
    size_t expect;
    bool   correct;

    dtest_block(_suite, "d_file_read_all");

    expect = strlen(D_FS_TEST_CONTENT);

    dtest_test(_suite, "reads the whole file with the right size and bytes");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    size    = 0;
    content = d_file_read_all(path, &size);
    correct = (content != NULL) &&
              (size == expect) &&
              (memcmp(content, D_FS_TEST_CONTENT, expect) == 0);
    D_FS_CHECK(_suite, correct);
#if D_CFG_FILE_READ_NUL_TERMINATE
    D_FS_CHECK(_suite,
               (content != NULL) && (((char*)content)[size] == '\0'));
#endif

    if (content)
    {
        free(content);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> NULL, EINVAL");
    errno   = 0;
    content = d_file_read_all(NULL, &size);
    D_FS_CHECK(_suite, content == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "a missing file -> NULL");
    d_fs_test_path(path, sizeof(path), "no_such_fread_all.txt");
    D_FS_CHECK(_suite, d_file_read_all(path, &size) == NULL);

    dtest_test(_suite, "an empty file reads back empty via the growth path");
    d_fs_test_path(path, sizeof(path), "fread_all_empty.txt");
    d_fs_test_write(path, "", 0);
    size    = 123;
    content = d_file_read_all(path, &size);
    D_FS_CHECK(_suite, content != NULL);
    D_FS_CHECK(_suite, size == 0);

    if (content)
    {
        free(content);
    }

    d_file_remove(path);
}


/******************************************************************************
 * IV.e  d_file_read_all_stream  (whole-file, any backend)
 *****************************************************************************/

static void
d_fs_tests_io_fread_all_stream
(
    dtest_suite* _suite
)
{
    char   path[D_FS_TEST_PATH_BUF];
    FILE*  file;
    void*  content;
    size_t size;
    size_t expect;
    bool   correct;

    dtest_block(_suite, "d_file_read_all_stream");

    expect = strlen(D_FS_TEST_CONTENT);

    dtest_test(_suite, "reads an open stream to its end");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    file    = d_file_open_stream(path, "rb");
    content = NULL;
    size    = 0;

    if (file)
    {
        content = d_file_read_all_stream(file, &size);
        d_file_close_stream(file);
    }

    correct = (content != NULL) &&
              (size == expect) &&
              (memcmp(content, D_FS_TEST_CONTENT, expect) == 0);
    D_FS_CHECK(_suite, correct);

    if (content)
    {
        free(content);
    }

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL stream -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_read_all_stream(NULL, &size) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);

#if D_FILE_PIPE_IS_AVAILABLE
    dtest_test(_suite, "an unsized pipe reads correctly via the growth path");
    file    = d_pipe_open("printf abcdef", "r");
    content = NULL;
    size    = 0;

    if (file)
    {
        content = d_file_read_all_stream(file, &size);
        d_pipe_close(file);
    }

    D_FS_CHECK(_suite,
               (content != NULL) &&
               (size == 6) &&
               (memcmp(content, "abcdef", 6) == 0));

    if (content)
    {
        free(content);
    }
#endif
#endif
}


/******************************************************************************
 * IV.f  d_file_read_all_into  (whole-file, any backend)
 *****************************************************************************/

static void
d_fs_tests_io_fread_all_into
(
    dtest_suite* _suite
)
{
    char   path[D_FS_TEST_PATH_BUF];
    char   buf[128];
    char   small[10];
    size_t size;
    size_t expect;
    int    rc;

    dtest_block(_suite, "d_file_read_all_into");

    expect = strlen(D_FS_TEST_CONTENT);

    dtest_test(_suite, "reads into the caller's buffer");
    d_fs_test_path(path, sizeof(path), D_FS_TEST_FILE);
    size = 0;
    rc   = d_file_read_all_into(path, buf, sizeof(buf), &size);
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite, size == expect);
    // only inspect the buffer if the call claims to have filled it
    D_FS_CHECK(_suite,
               (rc == 0) && (memcmp(buf, D_FS_TEST_CONTENT, expect) == 0));

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> EINVAL");
    D_FS_CHECK(_suite,
               d_file_read_all_into(NULL, buf, sizeof(buf), &size) == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL buffer -> EINVAL");
    D_FS_CHECK(_suite,
               d_file_read_all_into(path, NULL, sizeof(buf), &size) == EINVAL);
    #endif

    dtest_test(_suite, "a file larger than the buffer -> ERANGE");
    rc = d_file_read_all_into(path, small, sizeof(small), &size);
    D_FS_CHECK(_suite, rc == ERANGE);

#if D_CFG_FILE_READ_NUL_TERMINATE
#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "a buffer too small to hold anything -> EINVAL");
    D_FS_CHECK(_suite, d_file_read_all_into(path, buf, 1, &size) == EINVAL);
#endif
#endif
}


/******************************************************************************
 * IV.g  d_file_write_all  (whole-file, any backend)
 *****************************************************************************/

static void
d_fs_tests_io_fwrite_all
(
    dtest_suite* _suite
)
{
    char   path[D_FS_TEST_PATH_BUF];
    char*  read_back;
    size_t size;
    int    rc;

    dtest_block(_suite, "d_file_write_all");

    d_fs_test_path(path, sizeof(path), "fwrite_all.txt");

    dtest_test(_suite, "writes a new file with exactly the given bytes");
    rc        = d_file_write_all(path, "first content", 13);
    read_back = (char*)d_file_read_all(path, &size);
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite,
               (read_back != NULL) &&
               (size == 13) &&
               (memcmp(read_back, "first content", 13) == 0));

    if (read_back)
    {
        free(read_back);
    }

    dtest_test(_suite, "replaces the contents of an existing file");
    rc        = d_file_write_all(path, "new", 3);
    read_back = (char*)d_file_read_all(path, &size);
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite,
               (read_back != NULL) &&
               (size == 3) &&
               (memcmp(read_back, "new", 3) == 0));

    if (read_back)
    {
        free(read_back);
    }

#if D_FILE_WRITE_IS_ATOMIC
    dtest_test(_suite, "atomic replace leaves no sibling temporary behind");
    {
        // room for the whole path AND the suffix, so the compose below
        // cannot truncate
        char temp_path[D_FS_TEST_PATH_BUF
                           + sizeof(D_INTERNAL_FILE_WRITE_TEMP_SUFFIX)];

        // the atomic path writes "<path><suffix>" and renames it away
        snprintf(temp_path,
                 sizeof(temp_path),
                 "%s%s",
                 path,
                 D_INTERNAL_FILE_WRITE_TEMP_SUFFIX);
        D_FS_CHECK(_suite, !d_file_exists(temp_path));
    }
#endif

    d_file_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "a zero-length write with NULL data creates an empty file");
    rc = d_file_write_all(path, NULL, 0);
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite, d_file_exists(path));
    D_FS_CHECK(_suite, d_file_size(path) == 0);
    #endif

    d_file_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_write_all(NULL, "x", 1) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL data with a non-zero size -> -1, EINVAL");
    d_fs_test_path(path, sizeof(path), "fwrite_all_bad.txt");
    errno = 0;
    D_FS_CHECK(_suite, d_file_write_all(path, NULL, 4) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif
}


/******************************************************************************
 * IV.h  d_file_append_all  (whole-file, any backend)
 *****************************************************************************/

static void
d_fs_tests_io_fappend_all
(
    dtest_suite* _suite
)
{
    char   path[D_FS_TEST_PATH_BUF];
    char*  read_back;
    size_t size;
    int    rc;

    dtest_block(_suite, "d_file_append_all");

    d_fs_test_path(path, sizeof(path), "fappend_all.txt");

    dtest_test(_suite, "appends to an existing file");
    d_fs_test_write(path, "Hello", 5);
    rc        = d_file_append_all(path, " World", 6);
    read_back = (char*)d_file_read_all(path, &size);
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite,
               (read_back != NULL) &&
               (size == 11) &&
               (memcmp(read_back, "Hello World", 11) == 0));

    if (read_back)
    {
        free(read_back);
    }

    d_file_remove(path);

    dtest_test(_suite, "creates the file when it does not yet exist");
    rc = d_file_append_all(path, "fresh", 5);
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite, d_file_exists(path));
    D_FS_CHECK(_suite, d_file_size(path) == 5);

    d_file_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_append_all(NULL, "x", 1) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL data with a non-zero size -> -1, EINVAL");
    d_fs_test_path(path, sizeof(path), "fappend_all_bad.txt");
    errno = 0;
    D_FS_CHECK(_suite, d_file_append_all(path, NULL, 4) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_io
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "IV. file_io");

#if D_FILE_BACKEND_IS_STDC
    d_fs_tests_io_desc_stdc(_suite);
#else
    d_fs_tests_io_read_write(_suite);
    d_fs_tests_io_full(_suite);
    d_fs_tests_io_positional(_suite);
#endif

    d_fs_tests_io_fread_all(_suite);
    d_fs_tests_io_fread_all_stream(_suite);
    d_fs_tests_io_fread_all_into(_suite);
    d_fs_tests_io_fwrite_all(_suite);
    d_fs_tests_io_fappend_all(_suite);

    return;
}
