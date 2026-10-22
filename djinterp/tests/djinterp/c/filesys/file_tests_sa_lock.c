/*******************************************************************************
* djinterp [fs]                                             file_tests_sa_lock.c
*
*
* path:      /tests/djinterp/c/filesys/file_tests_sa_lock.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.20
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

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
static struct d_test_object*
d_tests_file_flock
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    int                   fd;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), "flock.txt");
    (void)d_file_write_all(path, "test", 4);

    fd = d_file_open(path, O_RDWR);

    if (fd >= 0)
    {
        items[idx++] = D_ASSERT_EQUAL("shared", d_file_lock_fd(fd, D_LOCK_SH), 0,
                                      "d_file_lock_fd takes a shared lock");
        items[idx++] = D_ASSERT_EQUAL("unlock", d_file_lock_fd(fd, D_LOCK_UN), 0,
                                      "d_file_lock_fd releases it");
        items[idx++] = D_ASSERT_EQUAL("exclusive", d_file_lock_fd(fd, D_LOCK_EX), 0,
                                      "d_file_lock_fd takes an exclusive lock");
        items[idx++] = D_ASSERT_EQUAL("nonblocking",
                                      d_file_lock_fd(fd, D_LOCK_EX | D_LOCK_NB), 0,
                                      "re-locking what we hold does not block");
        (void)d_file_lock_fd(fd, D_LOCK_UN);
        (void)d_file_close_fd(fd);
    }

    items[idx++] = D_ASSERT_EQUAL("bad_fd", d_file_lock_fd(-1, D_LOCK_SH), -1,
                                  "d_file_lock_fd rejects a bad descriptor");

    // SH|EX is not a richer request, it is a contradiction -- and a platform
    // handed the OR of two flags does something arbitrary rather than object
    //   Gated: the refusal IS the parameter check. With validation off the
    // operation word is passed through unexamined, which is what the knob
    // asks for, so there is nothing here to assert -- and asserting it anyway
    // was reporting the build as broken for doing exactly what it was told.
#if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("contradiction",
                                  d_file_lock_fd(0, D_LOCK_SH | D_LOCK_EX), -1,
                                  "SH|EX is refused, not guessed at");
    items[idx++] = D_ASSERT_EQUAL("no_operation", d_file_lock_fd(0, D_LOCK_NB), -1,
                                  "NB alone names no operation");
#endif

    (void)d_file_remove(path);

    return d_tests_file_group(
               D_FILE_LOCK_IS_PER_DESCRIPTION ?
                   "d_file_lock_fd (flock: per open file description)" :
                   "d_file_lock_fd (fcntl: per process -- ANY close drops it)",
               items,
               idx);
}


/*
d_tests_file_flock_stream
  Tests d_file_lock_stream.
*/
static struct d_test_object*
d_tests_file_flock_stream
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    FILE*                 file;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), "flock_stream.txt");
    (void)d_file_write_all(path, "test", 4);

    file = d_file_open_stream(path, "r+b");

    if (file)
    {
        items[idx++] = D_ASSERT_EQUAL("lock",
                                      d_file_lock_stream(file, D_LOCK_SH), 0,
                                      "d_file_lock_stream locks through a stream");
        (void)d_file_lock_stream(file, D_LOCK_UN);
        (void)d_file_close_stream(file);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null",
                                  d_file_lock_stream(NULL, D_LOCK_SH), -1,
                                  "d_file_lock_stream rejects NULL");
    #endif

    (void)d_file_remove(path);

    return d_tests_file_group("d_file_lock_stream", items, idx);
}


/*
d_tests_file_lock_all
  Runs every file_lock test.
*/
struct d_test_object*
d_tests_file_lock_all
(
    void
)
{
    struct d_test_object* items[4];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_flock();
    items[idx++] = d_tests_file_flock_stream();

    return d_tests_file_group("VII. file_lock", items, idx);
}
