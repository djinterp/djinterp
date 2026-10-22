/*******************************************************************************
* djinterp [fs]                                             file_tests_sa_desc.c
*
*
* path:      /tests/djinterp/c/filesys/file_tests_sa_desc.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.20
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

/******************************************************************************
 * III.  file_desc  --  DESCRIPTOR LIFECYCLE
 *****************************************************************************/

/*
d_tests_file_open_close
  Tests d_file_open / d_file_close_fd.
*/
static struct d_test_object*
d_tests_file_open_close
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    char                  fresh[D_TEST_FILE_PATH_BUF];
    int                   fd;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);
    d_tests_file_path(fresh, sizeof(fresh), "open_new.txt");

    fd = d_file_open(path, O_RDONLY);
    items[idx++] = D_ASSERT_TRUE("open_existing", fd >= 0,
                                 "d_file_open opens an existing file");

    if (fd >= 0)
    {
        items[idx++] = D_ASSERT_EQUAL("close", d_file_close_fd(fd), 0,
                                      "d_file_close_fd closes it");
    }

    fd = d_file_open(fresh, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    items[idx++] = D_ASSERT_TRUE("create", fd >= 0,
                                 "d_file_open creates with O_CREAT");

    if (fd >= 0)
    {
        (void)d_file_close_fd(fd);
    }

    (void)d_file_remove(fresh);

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_path", d_file_open(NULL, O_RDONLY), -1,
                                  "d_file_open rejects a NULL path");
    #endif
    items[idx++] = D_ASSERT_EQUAL("nonexistent",
                                  d_file_open("no/such/file/at/all", O_RDONLY), -1,
                                  "d_file_open fails for a missing file");
    items[idx++] = D_ASSERT_EQUAL("close_bad", d_file_close_fd(-1), -1,
                                  "d_file_close_fd rejects a bad descriptor");

    return d_tests_file_group("d_file_open / d_file_close_fd", items, idx);
}


/*
d_tests_file_dup
  Tests d_file_dup_fd / d_file_dup2_fd, and the close-on-exec property POSIX would silently
drop.
*/
static struct d_test_object*
d_tests_file_dup
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    int                   fd;
    int                   dup_fd;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    fd = d_file_open(path, O_RDONLY);

    if (fd >= 0)
    {
        dup_fd = d_file_dup_fd(fd);
        items[idx++] = D_ASSERT_TRUE("dup", dup_fd >= 0,
                                     "d_file_dup_fd returns a descriptor");
        items[idx++] = D_ASSERT_TRUE("dup_distinct", dup_fd != fd,
                                     "...a different one");

#if ( D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX) &&                                    \
      D_CFG_IS_ON(D_CFG_FILE_DESC_CLOEXEC) )
        // POSIX SPECIFIES that dup() clears FD_CLOEXEC on the copy, so a
        // careful O_CLOEXEC open followed by a dup silently yields an
        // inheritable descriptor. d_file_dup_fd uses F_DUPFD_CLOEXEC instead.
        items[idx++] = D_ASSERT_TRUE("open_cloexec",
                                     (fcntl(fd, F_GETFD) & FD_CLOEXEC) != 0,
                                     "d_file_open sets close-on-exec");
        items[idx++] = D_ASSERT_TRUE("dup_keeps_cloexec",
                                     (fcntl(dup_fd, F_GETFD) & FD_CLOEXEC) != 0,
                                     "d_file_dup_fd preserves it, unlike raw dup()");
#endif

        if (dup_fd >= 0)
        {
            (void)d_file_close_fd(dup_fd);
        }

        (void)d_file_close_fd(fd);
    }

    items[idx++] = D_ASSERT_EQUAL("dup_bad", d_file_dup_fd(-1), -1,
                                  "d_file_dup_fd rejects a bad descriptor");

    fd     = d_file_open(path, O_RDONLY);
    dup_fd = d_file_open(path, O_RDONLY);

    if ( (fd >= 0) &&
         (dup_fd >= 0) )
    {
        items[idx++] = D_ASSERT_EQUAL("dup2", d_file_dup2_fd(fd, dup_fd), dup_fd,
                                      "d_file_dup2_fd returns the target descriptor");
    }

    if (fd >= 0)
    {
        (void)d_file_close_fd(fd);
    }

    if (dup_fd >= 0)
    {
        (void)d_file_close_fd(dup_fd);
    }

    items[idx++] = D_ASSERT_EQUAL("dup2_bad_src", d_file_dup2_fd(-1, 10), -1,
                                  "d_file_dup2_fd rejects a bad source");

    return d_tests_file_group("d_file_dup_fd / d_file_dup2_fd", items, idx);
}


/*
d_tests_file_fileno
  Tests d_file_descriptor_stream.
*/
static struct d_test_object*
d_tests_file_fileno
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    FILE*                 file;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    file = d_file_open_stream(path, "rb");

    if (file)
    {
        items[idx++] = D_ASSERT_TRUE("valid", d_file_descriptor_stream(file) >= 0,
                                     "d_file_descriptor_stream borrows the stream's descriptor");
        (void)d_file_close_stream(file);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null", d_file_descriptor_stream(NULL), -1,
                                  "d_file_descriptor_stream rejects NULL");
    #endif

    return d_tests_file_group("d_file_descriptor_stream", items, idx);
}


/*
d_tests_file_desc_all
  Runs every file_desc test.
*/
struct d_test_object*
d_tests_file_desc_all
(
    void
)
{
    struct d_test_object* items[4];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_open_close();
    items[idx++] = d_tests_file_dup();
    items[idx++] = d_tests_file_fileno();

    return d_tests_file_group("III. file_desc", items, idx);
}
