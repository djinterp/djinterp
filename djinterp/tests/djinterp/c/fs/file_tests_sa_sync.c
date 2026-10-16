/*******************************************************************************
* djinterp [fs]                                             file_tests_sa_sync.c
*
*
* path:      /tests/djinterp/c/fs/file_tests_sa_sync.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

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
static struct d_test_object*
d_tests_file_fsync
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    FILE*                 file;
    int                   fd;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), "fsync.txt");

    fd = d_file_open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd >= 0)
    {
        (void)d_file_write_fd(fd, "test", 4);
        items[idx++] = D_ASSERT_EQUAL("valid_fd", d_file_sync_fd(fd), 0,
                                      "d_file_sync_fd succeeds on a valid descriptor");
        (void)d_file_close_fd(fd);
    }

    items[idx++] = D_ASSERT_EQUAL("bad_fd", d_file_sync_fd(-1), -1,
                                  "d_file_sync_fd rejects a bad descriptor");

    file = d_file_open_stream(path, "wb");

    if (file)
    {
        (void)fprintf(file, "test data");
        items[idx++] = D_ASSERT_EQUAL("stream", d_file_sync_stream(file), 0,
                                      "d_file_sync_stream flushes then syncs");
        (void)d_file_close_stream(file);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("stream_null", d_file_sync_stream(NULL), -1,
                                  "d_file_sync_stream rejects NULL");
    #endif

    (void)d_file_remove(path);

    return d_tests_file_group(
               D_FILE_SYNC_REACHES_PLATTER ?
                   "d_file_sync_fd (reaches the device)" :
                   "d_file_sync_fd (kernel only -- see D_CFG_FILE_SYNC_FULL)",
               items,
               idx);
}


/*
d_tests_file_fflush
  Tests d_file_flush_stream.
*/
static struct d_test_object*
d_tests_file_fflush
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    FILE*                 file;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), "fflush.txt");

    file = d_file_open_stream(path, "wb");

    if (file)
    {
        (void)fprintf(file, "test data");
        items[idx++] = D_ASSERT_EQUAL("valid", d_file_flush_stream(file), 0,
                                      "d_file_flush_stream hands the buffer to the kernel");
        (void)d_file_close_stream(file);
    }

    // NULL is a documented, meaningful argument here -- flush every output
    // stream -- rather than a mistake to reject
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_flushes_all", d_file_flush_stream(NULL), 0,
                                  "d_file_flush_stream(NULL) flushes all streams, per ISO C");
    #endif

    (void)d_file_remove(path);

    return d_tests_file_group("d_file_flush_stream", items, idx);
}


/*
d_tests_file_sync_all
  Runs every file_sync test.
*/
struct d_test_object*
d_tests_file_sync_all
(
    void
)
{
    struct d_test_object* items[4];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_fsync();
    items[idx++] = d_tests_file_fflush();

    return d_tests_file_group("VI. file_sync", items, idx);
}
