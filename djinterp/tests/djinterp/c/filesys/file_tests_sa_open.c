/*******************************************************************************
* djinterp [fs]                                             file_tests_sa_open.c
*
*
* path:      /tests/djinterp/c/filesys/file_tests_sa_open.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.20
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

/******************************************************************************
 * II.  file_open  --  STREAM LIFECYCLE
 *****************************************************************************/

/*
d_tests_file_fopen
  Tests d_file_open_stream / d_file_close_stream.
  Tests the following:
  - opens an existing file for reading
  - creates a file for writing
  - returns NULL for a nonexistent file
  - rejects NULL arguments
*/
static struct d_test_object*
d_tests_file_fopen
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    char                  fresh[D_TEST_FILE_PATH_BUF];
    FILE*                 file;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);
    d_tests_file_path(fresh, sizeof(fresh), "fopen_new.txt");

    file = d_file_open_stream(path, "rb");
    items[idx++] = D_ASSERT_NOT_NULL("read_existing", file,
                                     "d_file_open_stream opens an existing file");

    if (file)
    {
        items[idx++] = D_ASSERT_EQUAL("close", d_file_close_stream(file), 0,
                                      "d_file_close_stream closes a stream");
    }

    file = d_file_open_stream(fresh, "wb");
    items[idx++] = D_ASSERT_NOT_NULL("create", file,
                                     "d_file_open_stream creates a file for writing");

    if (file)
    {
        (void)d_file_close_stream(file);
    }

    (void)d_file_remove(fresh);

    items[idx++] = D_ASSERT_NULL("nonexistent",
                                 d_file_open_stream("no/such/file/at/all", "rb"),
                                 "d_file_open_stream returns NULL for a missing file");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null_path", d_file_open_stream(NULL, "rb"),
                                 "d_file_open_stream rejects a NULL path");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null_mode", d_file_open_stream(path, NULL),
                                 "d_file_open_stream rejects a NULL mode");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("close_null", d_file_close_stream(NULL), EOF,
                                  "d_file_close_stream rejects NULL");
    #endif

    return d_tests_file_group("d_file_open_stream / d_file_close_stream", items, idx);
}


/*
d_tests_file_fopen_s
  Tests d_file_open_stream_s.
  Tests the following:
  - reports success through the return value
  - clears the out-parameter before anything else
  - rejects a NULL out-parameter
*/
static struct d_test_object*
d_tests_file_fopen_s
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  path[D_TEST_FILE_PATH_BUF];
    FILE*                 stream;
    size_t                idx;

    idx = 0;
    d_tests_file_path(path, sizeof(path), D_TEST_FILE_TEST_FILENAME);

    stream = NULL;
    items[idx++] = D_ASSERT_EQUAL("success",
                                  d_file_open_stream_s(&stream, path, "rb"), 0,
                                  "d_file_open_stream_s returns 0 on success");
    items[idx++] = D_ASSERT_NOT_NULL("stream_set", stream,
                                     "...and sets the stream");

    if (stream)
    {
        (void)d_file_close_stream(stream);
    }

    // the out-parameter must be cleared FIRST, so a caller who ignores the
    // return code is left holding NULL rather than an indeterminate pointer
    stream = (FILE*)(void*)&items;
    (void)d_file_open_stream_s(&stream, "no/such/file/at/all", "rb");
    items[idx++] = D_ASSERT_NULL("cleared_on_failure", stream,
                                 "a failed d_file_open_stream_s leaves NULL, not garbage");

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null_out",
                                  d_file_open_stream_s(NULL, path, "rb"), EINVAL,
                                  "d_file_open_stream_s rejects a NULL out-parameter");
    #endif

    return d_tests_file_group("d_file_open_stream_s", items, idx);
}


/*
d_tests_file_open_all
  Runs every file_open test.
*/
struct d_test_object*
d_tests_file_open_all
(
    void
)
{
    struct d_test_object* items[4];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_fopen();
    items[idx++] = d_tests_file_fopen_s();

    return d_tests_file_group("II. file_open", items, idx);
}
