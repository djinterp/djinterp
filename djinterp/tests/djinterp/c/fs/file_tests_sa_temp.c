/*******************************************************************************
* djinterp [fs]                                             file_tests_sa_temp.c
*
*
* path:      /tests/djinterp/c/fs/file_tests_sa_temp.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"

/******************************************************************************
 * XII.  file_temp  --  TEMPORARY FILES
 *****************************************************************************/

/*
d_tests_file_tmpfile
  Tests d_file_temp_stream / d_file_temp_stream_s.
*/
static struct d_test_object*
d_tests_file_tmpfile
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    FILE*                 tmp;
    FILE*                 stream;
    size_t                idx;

    idx = 0;

    tmp = d_file_temp_stream();
    items[idx++] = D_ASSERT_NOT_NULL("create", tmp,
                                     "d_file_temp_stream creates an anonymous file");

    if (tmp)
    {
        items[idx++] = D_ASSERT_TRUE("writable",
                                     fprintf(tmp, "test data") > 0,
                                     "...that is writable");
        (void)fclose(tmp);
    }

    stream = NULL;
    items[idx++] = D_ASSERT_EQUAL("s_create", d_file_temp_stream_s(&stream), 0,
                                  "d_file_temp_stream_s reports success");
    items[idx++] = D_ASSERT_NOT_NULL("s_stream", stream,
                                     "...and sets the stream");

    if (stream)
    {
        (void)fclose(stream);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("s_null", d_file_temp_stream_s(NULL), EINVAL,
                                  "d_file_temp_stream_s rejects a NULL out-parameter");
    #endif

    return d_tests_file_group("d_file_temp_stream / d_file_temp_stream_s", items, idx);
}


/*
d_tests_file_mkstemp
  Tests d_file_temp_create -- the only SAFE named form.
*/
static struct d_test_object*
d_tests_file_mkstemp
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    struct d_stat_t       st;
    char                  tpl[D_TEST_FILE_PATH_BUF];
    char                  original[D_TEST_FILE_PATH_BUF];
    char                  bad[D_TEST_FILE_PATH_BUF];
    int                   fd;
    size_t                idx;

    idx = 0;
    d_tests_file_path(tpl, sizeof(tpl), "tmpXXXXXX");
    memcpy(original, tpl, strlen(tpl) + 1);

    fd = d_file_temp_create(tpl);
    items[idx++] = D_ASSERT_TRUE("create", fd >= 0,
                                 "d_file_temp_create creates and opens atomically");
    items[idx++] = D_ASSERT_TRUE("template_rewritten",
                                 strcmp(tpl, original) != 0,
                                 "the template is rewritten with the chosen name");

    if (fd >= 0)
    {
#if !D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
        // 0600 is the whole point: a temporary in a world-writable directory
        // is the most attacked object a program creates
        (void)d_file_stat(tpl, &st);
        items[idx++] = D_ASSERT_EQUAL("mode",
                                      st.st_mode & 0777,
                                      (uint32_t)D_INTERNAL_FILE_TEMP_MODE,
                                      "the file is private, whatever the umask says");
#else
        (void)st;
#endif
        (void)d_file_close_fd(fd);
        (void)d_file_remove(tpl);
    }

    // a template the platform would reject arbitrarily, checked here so the
    // failure is the same on every libc
    memcpy(bad, "no_x_suffix", 12);
    items[idx++] = D_ASSERT_EQUAL("bad_template", d_file_temp_create(bad), -1,
                                  "a template without XXXXXX is rejected");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_EQUAL("null", d_file_temp_create(NULL), -1,
                                  "d_file_temp_create rejects NULL");
    #endif

    return d_tests_file_group("d_file_temp_create (atomic; no race)", items, idx);
}


/*
d_tests_file_tempdir
  Tests d_dir_temp, and d_file_temp_name where the build still has it.
*/
static struct d_test_object*
d_tests_file_tempdir
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  buf[D_FILE_PATH_MAX];
    size_t                length;
    size_t                idx;

    idx = 0;

    items[idx++] = D_ASSERT_NOT_NULL("get", d_dir_temp(buf, sizeof(buf)),
                                     "d_dir_temp reports a directory");
    items[idx++] = D_ASSERT_TRUE("exists", d_dir_exists(buf),
                                 "...that exists");

    // no trailing separator, so d_path_join needs no special case
    length = strlen(buf);
    items[idx++] = D_ASSERT_TRUE("no_trailing_sep",
                                 (length > 0) &&
                                 (buf[length - 1] != '/') &&
                                 (buf[length - 1] != D_FILE_PATH_SEP),
                                 "...with no trailing separator");

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null", d_dir_temp(NULL, 100),
                                 "d_dir_temp rejects a NULL buffer");
    #endif

#if D_FILE_TEMP_HAS_TMPNAM
    // present for compatibility, and racy by construction -- the flaw is the
    // interface, not the implementation. Set D_CFG_FILE_TEMP_ALLOW_TMPNAM=0
    // and the symbol disappears from the object file entirely.
    memset(buf, 0, sizeof(buf));
    items[idx++] = D_ASSERT_EQUAL("tmpnam", d_file_temp_name(buf, sizeof(buf)), 0,
                                  "d_file_temp_name generates a name");
    items[idx++] = D_ASSERT_TRUE("tmpnam_nonempty", strlen(buf) > 0,
                                 "...a non-empty one");
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_TRUE("tmpnam_null", d_file_temp_name(NULL, 100) != 0,
                                 "d_file_temp_name rejects a NULL buffer");
    #endif
    items[idx++] = D_ASSERT_TRUE("tmpnam_tiny", d_file_temp_name(buf, 1) != 0,
                                 "d_file_temp_name rejects a tiny buffer");
#endif

    return d_tests_file_group("d_dir_temp", items, idx);
}


/*
d_tests_file_temp_all
  Runs every file_temp test.
*/
struct d_test_object*
d_tests_file_temp_all
(
    void
)
{
    struct d_test_object* items[4];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_tmpfile();
    items[idx++] = d_tests_file_mkstemp();
    items[idx++] = d_tests_file_tempdir();

    return d_tests_file_group("XII. file_temp", items, idx);
}
