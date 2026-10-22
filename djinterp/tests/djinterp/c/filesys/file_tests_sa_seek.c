/*******************************************************************************
* djinterp [fs]                                             file_tests_sa_seek.c
*
*
* path:      /tests/djinterp/c/filesys/file_tests_sa_seek.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.20
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

/******************************************************************************
 * V.  file_seek  --  POSITION AND EXTENT
 *****************************************************************************/

/*
d_tests_file_seek_tell
  Tests d_file_seek_stream / d_file_tell_stream / d_file_rewind_stream.
*/
static struct d_test_object*
d_tests_file_seek_tell
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
        items[idx++] = D_ASSERT_EQUAL("tell_start", d_file_tell_stream(file),
                                      (d_off_t)0,
                                      "a fresh stream is at 0");
        items[idx++] = D_ASSERT_EQUAL("seek_set",
                                      d_file_seek_stream(file, 5, SEEK_SET), 0,
                                      "d_file_seek_stream seeks from the start");
        items[idx++] = D_ASSERT_EQUAL("tell_after_set", d_file_tell_stream(file),
                                      (d_off_t)5,
                                      "...and d_file_tell_stream agrees");
        items[idx++] = D_ASSERT_EQUAL("seek_cur",
                                      d_file_seek_stream(file, 3, SEEK_CUR), 0,
                                      "d_file_seek_stream seeks relatively");
        items[idx++] = D_ASSERT_EQUAL("tell_after_cur", d_file_tell_stream(file),
                                      (d_off_t)8,
                                      "...cumulatively");
        items[idx++] = D_ASSERT_EQUAL("seek_end",
                                      d_file_seek_stream(file, -5, SEEK_END), 0,
                                      "d_file_seek_stream seeks from the end");
        items[idx++] = D_ASSERT_EQUAL("tell_after_end", d_file_tell_stream(file),
                                      (d_off_t)(strlen(D_TEST_FILE_TEST_CONTENT) - 5),
                                      "...to the right place");
        items[idx++] = D_ASSERT_EQUAL("rewind", d_file_rewind_stream(file), 0,
                                      "d_file_rewind_stream reports success, unlike rewind()");
        items[idx++] = D_ASSERT_EQUAL("rewind_pos", d_file_tell_stream(file),
                                      (d_off_t)0,
                                      "...and returns to 0");
        items[idx++] = D_ASSERT_EQUAL("bad_whence",
                                      d_file_seek_stream(file, 0, 99), -1,
                                      "an invalid whence is rejected");
        (void)d_file_close_stream(file);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("seek_null",
                                  d_file_seek_stream(NULL, 0, SEEK_SET), -1,
                                  "d_file_seek_stream rejects NULL");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("tell_null", d_file_tell_stream(NULL), (d_off_t)-1,
                                  "d_file_tell_stream rejects NULL");
    #endif

    return d_tests_file_group("d_file_seek_stream / d_file_tell_stream / d_file_rewind_stream", items, idx);
}


/*
d_tests_file_truncate
  Tests d_file_truncate_fd and d_file_truncate_stream -- including the dirty-buffer trap.
*/
static struct d_test_object*
d_tests_file_truncate
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    int                   fd;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), "truncate.txt");
    (void)d_file_write_all(path, "Hello World! This is test content.", 34);

    fd = d_file_open(path, O_RDWR);

    if (fd >= 0)
    {
        items[idx++] = D_ASSERT_EQUAL("shrink", d_file_truncate_fd(fd, 10), 0,
                                      "d_file_truncate_fd shrinks a file");
        (void)d_file_close_fd(fd);
        items[idx++] = D_ASSERT_EQUAL("shrink_size", d_file_size(path),
                                      (int64_t)10,
                                      "...to exactly the requested length");
    }

    fd = d_file_open(path, O_RDWR);

    if (fd >= 0)
    {
        // extending is not a no-op: it zero-fills, and may make the file
        // sparse
        items[idx++] = D_ASSERT_EQUAL("extend", d_file_truncate_fd(fd, 50), 0,
                                      "d_file_truncate_fd extends a file");
        (void)d_file_close_fd(fd);
        items[idx++] = D_ASSERT_EQUAL("extend_size", d_file_size(path),
                                      (int64_t)50,
                                      "...to exactly the requested length");
    }

    items[idx++] = D_ASSERT_EQUAL("bad_fd", d_file_truncate_fd(-1, 10), -1,
                                  "d_file_truncate_fd rejects a bad descriptor");
    items[idx++] = D_ASSERT_EQUAL("negative", d_file_truncate_fd(0, -5), -1,
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

            items[idx++] = D_ASSERT_EQUAL("dirty_buffer",
                                          d_file_truncate_stream(file, 10), 0,
                                          "d_file_truncate_stream flushes first");
            (void)d_file_close_stream(file);
            items[idx++] = D_ASSERT_EQUAL("dirty_buffer_size",
                                          d_file_size(dirty), (int64_t)10,
                                          "...so the truncation is not undone by stdio");
        }

        (void)d_file_remove(dirty);
    }
#endif

    (void)d_file_remove(path);

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("stream_null",
                                  d_file_truncate_stream(NULL, 10), -1,
                                  "d_file_truncate_stream rejects NULL");
    #endif

    return d_tests_file_group("d_file_truncate_fd", items, idx);
}


/*
d_tests_file_seek_all
  Runs every file_seek test.
*/
struct d_test_object*
d_tests_file_seek_all
(
    void
)
{
    struct d_test_object* items[4];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_seek_tell();
    items[idx++] = d_tests_file_truncate();

    return d_tests_file_group("V. file_seek", items, idx);
}
