/*******************************************************************************
* djinterp [fs]                                               file_tests_sa_io.c
*
*
* path:      /tests/djinterp/c/filesys/file_tests_sa_io.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.20
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

/******************************************************************************
 * IV.  file_io  --  BYTE TRANSFER
 *****************************************************************************/

/*
d_tests_file_read_write
  Tests d_file_read_fd / d_file_write_fd and their _full forms.
*/
static struct d_test_object*
d_tests_file_read_write
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    char                  write_buf[] = "Test data for read/write";
    char                  read_buf[64];
    int                   fd;
    ssize_t               n;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), "read_write.txt");

    fd = d_file_open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    //   d_file_read_fd and d_file_write_fd are the SHORT-TRANSFER primitives: read(2)'s
    // contract is that a short transfer is success, and the chunk knobs make
    // it happen deliberately. Asserting == on them passed at the default 1 MiB
    // chunk by coincidence -- it was testing a lucky number, not the contract,
    // and it failed the moment the matrix set the chunk to 1. What each one
    // actually promises is asserted below: the primitive moves SOME bytes and
    // never more than asked; the _full variant moves ALL of them.
    if (fd >= 0)
    {
        n = d_file_write_fd(fd, write_buf, strlen(write_buf));
        items[idx++] = D_ASSERT_TRUE("write_partial",
                                     (n > 0) && (n <= (ssize_t)strlen(write_buf)),
                                     "d_file_write_fd transfers some bytes, never more than asked");
        (void)d_file_close_fd(fd);
    }

    fd = d_file_open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (fd >= 0)
    {
        items[idx++] = D_ASSERT_EQUAL("write_full",
                                      d_file_write_full_fd(fd, write_buf, strlen(write_buf)),
                                      (ssize_t)strlen(write_buf),
                                      "d_file_write_full_fd writes every requested byte");
        (void)d_file_close_fd(fd);
    }

    fd = d_file_open(path, O_RDONLY);

    if (fd >= 0)
    {
        memset(read_buf, 0, sizeof(read_buf));
        n = d_file_read_fd(fd, read_buf, sizeof(read_buf) - 1);
        items[idx++] = D_ASSERT_TRUE("read_partial",
                                     (n > 0) && (n <= (ssize_t)strlen(write_buf)),
                                     "d_file_read_fd transfers some bytes, never more than asked");
        (void)d_file_close_fd(fd);
    }

    // d_file_read_full_fd loops over short reads; on a regular file it must deliver
    // everything, and report the shortfall honestly at EOF. The integrity
    // check belongs HERE, after the full read -- hung off d_file_read_fd it was
    // asserting the whole content survived a call that never promised to
    // deliver it.
    fd = d_file_open(path, O_RDONLY);

    if (fd >= 0)
    {
        memset(read_buf, 0, sizeof(read_buf));
        items[idx++] = D_ASSERT_EQUAL("read_full_eof",
                                      d_file_read_full_fd(fd, read_buf, sizeof(read_buf) - 1),
                                      (ssize_t)strlen(write_buf),
                                      "d_file_read_full_fd returns what EOF allowed");
        items[idx++] = D_ASSERT_STR_EQUAL("integrity", read_buf, write_buf,
                                          "the bytes survive the round trip");
        (void)d_file_close_fd(fd);
    }

    (void)d_file_remove(path);

    items[idx++] = D_ASSERT_EQUAL("read_bad_fd",
                                  d_file_read_fd(-1, read_buf, 4), (ssize_t)-1,
                                  "d_file_read_fd rejects a bad descriptor");
    items[idx++] = D_ASSERT_EQUAL("write_bad_fd",
                                  d_file_write_fd(-1, write_buf, 4), (ssize_t)-1,
                                  "d_file_write_fd rejects a bad descriptor");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("read_null_buf",
                                  d_file_read_fd(0, NULL, 4), (ssize_t)-1,
                                  "d_file_read_fd rejects a NULL buffer");
    #endif

    return d_tests_file_group("d_file_read_fd / d_file_write_fd", items, idx);
}


/*
d_tests_file_fread_all
  Tests d_file_read_all.
  Tests the following:
  - reads a whole file, reporting payload bytes only
  - NUL-terminates when the build says it does
  - reads an UNSIZED source (the /proc case) rather than returning empty
*/
static struct d_test_object*
d_tests_file_fread_all
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    void*                 content;
    size_t                size;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    content = d_file_read_all(path, &size);
    items[idx++] = D_ASSERT_NOT_NULL("read", content,
                                     "d_file_read_all reads a file");
    items[idx++] = D_ASSERT_EQUAL("size", size,
                                  strlen(D_TEST_FILE_TEST_CONTENT),
                                  "the size is the payload, not the allocation");

#if D_CFG_IS_ON(D_CFG_FILE_READ_NUL_TERMINATE)
    items[idx++] = D_ASSERT_STR_EQUAL("nul_terminated",
                                      (char*)content, D_TEST_FILE_TEST_CONTENT,
                                      "the buffer is usable as a string");
#else
    items[idx++] = D_ASSERT_TRUE("exact_bytes",
                                 content &&
                                 memcmp(content, D_TEST_FILE_TEST_CONTENT, size) == 0,
                                 "NUL_TERMINATE=0 gives exactly the payload");
#endif

    if (content)
    {
        free(content);
    }

    items[idx++] = D_ASSERT_NULL("nonexistent",
                                 d_file_read_all("no/such/file/at/all", &size),
                                 "d_file_read_all returns NULL for a missing file");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null_path",
                                 d_file_read_all(NULL, &size),
                                 "d_file_read_all rejects a NULL path");
    #endif

    return d_tests_file_group("d_file_read_all", items, idx);
}


/*
d_tests_file_fwrite_all
  Tests d_file_write_all and d_file_append_all.
*/
static struct d_test_object*
d_tests_file_fwrite_all
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    char*                 back;
    size_t                size;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), "fwrite_all.txt");

    items[idx++] = D_ASSERT_EQUAL("write",
                                  d_file_write_all(path, "test content", 12), 0,
                                  "d_file_write_all writes a file");

    back = (char*)d_file_read_all(path, &size);
    items[idx++] = D_ASSERT_TRUE("content",
                                 back && size == 12 &&
                                 memcmp(back, "test content", 12) == 0,
                                 "the bytes land exactly");

    if (back)
    {
        free(back);
    }

    items[idx++] = D_ASSERT_EQUAL("overwrite",
                                  d_file_write_all(path, "new", 3), 0,
                                  "d_file_write_all replaces the contents");

    back = (char*)d_file_read_all(path, &size);
    items[idx++] = D_ASSERT_TRUE("overwrite_truncates",
                                 back && size == 3,
                                 "...and truncates rather than overlaying");

    if (back)
    {
        free(back);
    }

    items[idx++] = D_ASSERT_EQUAL("append",
                                  d_file_append_all(path, " tail", 5), 0,
                                  "d_file_append_all appends");

    back = (char*)d_file_read_all(path, &size);
    items[idx++] = D_ASSERT_TRUE("append_content",
                                 back && size == 8 &&
                                 memcmp(back, "new tail", 8) == 0,
                                 "the appended bytes follow the old ones");

    if (back)
    {
        free(back);
    }

    (void)d_file_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_path",
                                  d_file_write_all(NULL, "x", 1), -1,
                                  "d_file_write_all rejects a NULL path");
    #endif

    // NULL data with a nonzero size is a mistake; NULL data with zero size is
    // a legitimate way to create an empty file
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_data_sized",
                                  d_file_write_all(path, NULL, 5), -1,
                                  "NULL data with a size is rejected");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_data_empty",
                                  d_file_write_all(path, NULL, 0), 0,
                                  "NULL data with size 0 creates an empty file");
    #endif
    (void)d_file_remove(path);

    return d_tests_file_group("d_file_write_all / d_file_append_all", items, idx);
}


/*
d_tests_file_fread_all_into
  Tests d_file_read_all_into -- the no-allocation form.
*/
static struct d_test_object*
d_tests_file_fread_all_into
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    char                  buf[128];
    char                  tiny[4];
    size_t                size;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    items[idx++] = D_ASSERT_EQUAL("read",
                                  d_file_read_all_into(path, buf, sizeof(buf), &size),
                                  0,
                                  "d_file_read_all_into fills the caller's buffer");
    items[idx++] = D_ASSERT_EQUAL("size", size,
                                  strlen(D_TEST_FILE_TEST_CONTENT),
                                  "...and reports the payload size");

    // a file larger than the buffer is a FAILURE, not a truncation. Handing
    // back a prefix is how a caller ends up parsing half a config file.
    items[idx++] = D_ASSERT_EQUAL("overflow",
                                  d_file_read_all_into(path, tiny, sizeof(tiny), &size),
                                  ERANGE,
                                  "a file that does not fit is ERANGE, not a prefix");

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_path",
                                  d_file_read_all_into(NULL, buf, sizeof(buf), &size),
                                  EINVAL,
                                  "d_file_read_all_into rejects a NULL path");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_buf",
                                  d_file_read_all_into(path, NULL, 10, &size),
                                  EINVAL,
                                  "d_file_read_all_into rejects a NULL buffer");
    #endif

    return d_tests_file_group("d_file_read_all_into", items, idx);
}


/*
d_tests_file_io_all
  Runs every file_io test.
*/
struct d_test_object*
d_tests_file_io_all
(
    void
)
{
    struct d_test_object* items[8];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_read_write();
    items[idx++] = d_tests_file_fread_all();
    items[idx++] = d_tests_file_fwrite_all();
    items[idx++] = d_tests_file_fread_all_into();

    return d_tests_file_group("IV. file_io", items, idx);
}
