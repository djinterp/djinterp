/*******************************************************************************
* djinterp [test]                                              file_tests_temp.c
*
*   XII. TEMPORARIES (file_temp) -- temporary files and the temporary
* directory. d_file_temp_stream / d_file_temp_create choose a name and open it in one step;
* d_file_temp_create is descriptor-based and reports ENOSYS on the ISO C backend, and it
* rewrites the caller's template in place, so the template must be a mutable
* buffer ending in exactly six X's (a template that does not is rejected).
* d_file_temp_name exists only where the platform still offers it and is gated on
* D_FILE_TEMP_HAS_TMPNAM. d_dir_temp yields an existing directory and requires a
* buffer larger than one byte.
*
*
* path:      /tests/djinterp/c/filesys/file_tests_temp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <errno.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * XII.a  d_file_temp_stream
 *****************************************************************************/

static
void d_fs_tests_temp_tmpfile(
    struct dj_test_suite* _suite
)
{
    FILE* file;
    char  buf[8];
    bool  usable;

    d_test_suite_block(_suite, "d_file_temp_stream");

    d_test_suite_test(_suite,
                      "returns a stream that can be written and read back");
    file   = d_file_temp_stream();
    usable = false;

    if (file)
    {
        if (fwrite("tmp", 1, 3, file) == 3)
        {
            rewind(file);
            memset(buf, 0, sizeof(buf));
            usable = (fread(buf, 1, 3, file) == 3) &&
                     (memcmp(buf, "tmp", 3) == 0);
        }

        d_file_close_stream(file);
    }

    D_FS_CHECK(_suite, usable);
}


/******************************************************************************
 * XII.b  d_file_temp_stream_s
 *****************************************************************************/

static
void d_fs_tests_temp_tmpfile_s(
    struct dj_test_suite* _suite
)
{
    FILE* stream;
    int   rc;

    d_test_suite_block(_suite, "d_file_temp_stream_s");

    d_test_suite_test(_suite, "success -> 0 and a valid stream");
    stream = NULL;
    rc     = d_file_temp_stream_s(&stream);
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite, stream != NULL);

    if (stream)
    {
        d_file_close_stream(stream);
    }

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL out-parameter -> EINVAL");
    rc = d_file_temp_stream_s(NULL);
    D_FS_CHECK(_suite, rc == EINVAL);
#endif
}


/******************************************************************************
 * XII.c  d_file_temp_create
 *****************************************************************************/

static
void d_fs_tests_temp_mkstemp(
    struct dj_test_suite* _suite
)
{
#if !D_FILE_BACKEND_IS_STDC
    char tmpl[D_FS_TEST_PATH_BUF];
    char bad[D_FS_TEST_PATH_BUF];
    int  fd;
    bool made;

    d_test_suite_block(_suite, "d_file_temp_create");

    d_test_suite_test(_suite, "opens a unique file and rewrites the template");
    snprintf(tmpl, sizeof(tmpl), "%s%cmkstempXXXXXX",
             d_fs_test_root(), D_FILE_PATH_SEP);
    fd   = d_file_temp_create(tmpl);
    made = (fd >= 0);

    if (fd >= 0)
    {
        // the six X's have been replaced, and the named file now exists
        made = made &&
               (strstr(tmpl, "XXXXXX") == NULL) &&
               d_file_exists(tmpl);
        d_file_close_fd(fd);
        d_file_remove(tmpl);
    }

    D_FS_CHECK(_suite, made);

    d_test_suite_test(_suite, "a template not ending in six X's -> -1, EINVAL");
    snprintf(bad, sizeof(bad), "%s%cmkstemp_noplace",
             d_fs_test_root(), D_FILE_PATH_SEP);
    errno = 0;
    D_FS_CHECK(_suite, d_file_temp_create(bad) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

    d_test_suite_test(_suite, "a too-short template -> -1, EINVAL");
    strcpy(bad, "XXXXX");
    errno = 0;
    D_FS_CHECK(_suite, d_file_temp_create(bad) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL template -> -1, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_file_temp_create(NULL) == -1);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
#else
    d_test_suite_block(_suite, "d_file_temp_create");

    d_test_suite_test(_suite, "ISO C backend -> -1, ENOSYS");
    {
        char tmpl[] = "mkstempXXXXXX";

        errno = 0;
        D_FS_CHECK(_suite, d_file_temp_create(tmpl) == -1);
    }
    D_FS_CHECK(_suite, errno == ENOSYS);
#endif
}


/******************************************************************************
 * XII.d  d_file_temp_name
 *****************************************************************************/

static
void d_fs_tests_temp_tmpnam_s(
    struct dj_test_suite* _suite
)
{
#if D_FILE_TEMP_HAS_TMPNAM
    char buf[D_FS_TEST_PATH_BUF];
    int  rc;

    d_test_suite_block(_suite, "d_file_temp_name");

    d_test_suite_test(_suite, "produces a non-empty candidate name -> 0");
    memset(buf, 0, sizeof(buf));
    rc = d_file_temp_name(buf, sizeof(buf));
    D_FS_CHECK(_suite, rc == 0);
    D_FS_CHECK(_suite, strlen(buf) > 0);

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "a one-byte buffer -> EINVAL");
    D_FS_CHECK(_suite, d_file_temp_name(buf, 1) == EINVAL);

#endif
#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL buffer -> EINVAL");
    D_FS_CHECK(_suite, d_file_temp_name(NULL, sizeof(buf)) == EINVAL);
#endif
#else
    // d_file_temp_name is not offered on this platform; nothing to exercise
    d_test_suite_block(_suite, "d_file_temp_name");

    d_test_suite_test(_suite, "not available on this platform (skipped)");
    D_FS_CHECK(_suite, true);
#endif
}


/******************************************************************************
 * XII.e  d_dir_temp
 *****************************************************************************/

static
void d_fs_tests_temp_tempdir(
    struct dj_test_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF];

    d_test_suite_block(_suite, "d_dir_temp");

    d_test_suite_test(_suite, "yields an existing directory");
    memset(buf, 0, sizeof(buf));
    D_FS_CHECK(_suite, d_dir_temp(buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, d_dir_exists(buf));

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_temp(NULL, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "a one-byte buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_dir_temp(buf, 1) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
#endif
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void d_fs_tests_temp(
    struct dj_test_suite* _suite
)
{
    d_test_suite_module(_suite, "XII. file_temp");

    d_fs_tests_temp_tmpfile(_suite);
    d_fs_tests_temp_tmpfile_s(_suite);
    d_fs_tests_temp_mkstemp(_suite);
    d_fs_tests_temp_tmpnam_s(_suite);
    d_fs_tests_temp_tempdir(_suite);

    return;
}

/******************************************************************************
 * d_tests_file_temp_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * XII.  file_temp  --  TEMPORARY FILES
 *****************************************************************************/

/*
d_tests_file_tmpfile
  Tests d_file_temp_stream / d_file_temp_stream_s.
*/
static
bool d_tests_file_tmpfile(struct dj_test_suite* _suite)
{
    FILE*                 tmp;
    FILE*                 stream;

    d_test_suite_test(_suite, "d_file_temp_stream / d_file_temp_stream_s");

    tmp = d_file_temp_stream();
    D_TEST_CHECK_MSG(_suite,
                     "create",
                     (tmp) != NULL,
                     "d_file_temp_stream creates an anonymous file");

    if (tmp)
    {
        D_TEST_CHECK_MSG(_suite,
                         "writable",
                         fprintf(tmp, "test data") > 0,
                         "...that is writable");
        (void)fclose(tmp);
    }

    stream = NULL;
    D_TEST_CHECK_MSG(_suite,
                     "s_create",
                     (d_file_temp_stream_s(&stream)) == (0),
                     "d_file_temp_stream_s reports success");
    D_TEST_CHECK_MSG(_suite,
                     "s_stream",
                     (stream) != NULL,
                     "...and sets the stream");

    if (stream)
    {
        (void)fclose(stream);
    }

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "s_null",
                     (d_file_temp_stream_s(NULL)) == (EINVAL),
                     "d_file_temp_stream_s rejects a NULL out-parameter");
    #endif

    return true;
}


/*
d_tests_file_mkstemp
  Tests d_file_temp_create -- the only SAFE named form.
*/
static
bool d_tests_file_mkstemp(struct dj_test_suite* _suite)
{
    struct dj_stat_t       st;
    char                  tpl[D_TEST_FILE_PATH_BUF];
    char                  original[D_TEST_FILE_PATH_BUF];
    char                  bad[D_TEST_FILE_PATH_BUF];
    int                   fd;

    d_test_suite_test(_suite, "d_file_temp_create (atomic; no race)");

    d_tests_file_path(tpl, sizeof(tpl), "tmpXXXXXX");
    memcpy(original, tpl, strlen(tpl) + 1);

    fd = d_file_temp_create(tpl);
    D_TEST_CHECK_MSG(_suite,
                     "create",
                     fd >= 0,
                     "d_file_temp_create creates and opens atomically");
    D_TEST_CHECK_MSG(_suite,
                     "template_rewritten",
                     strcmp(tpl, original) != 0,
                     "the template is rewritten with the chosen name");

    if (fd >= 0)
    {
#if !D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
        // 0600 is the whole point: a temporary in a world-writable directory
        // is the most attacked object a program creates
        (void)d_file_stat(tpl, &st);
        D_TEST_CHECK_MSG(_suite,
                         "mode",
                         (st.st_mode & 0777) == ((uint32_t)D_INTERNAL_FILE_TEMP_MODE),
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
    D_TEST_CHECK_MSG(_suite,
                     "bad_template",
                     (d_file_temp_create(bad)) == (-1),
                     "a template without XXXXXX is rejected");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     (d_file_temp_create(NULL)) == (-1),
                     "d_file_temp_create rejects NULL");
    #endif

    return true;
}


/*
d_tests_file_tempdir
  Tests d_dir_temp, and d_file_temp_name where the build still has it.
*/
static
bool d_tests_file_tempdir(struct dj_test_suite* _suite)
{
    char                  buf[D_FILE_PATH_MAX];
    size_t                length;

    d_test_suite_test(_suite, "d_dir_temp");

    D_TEST_CHECK_MSG(_suite,
                     "get",
                     (d_dir_temp(buf, sizeof(buf))) != NULL,
                     "d_dir_temp reports a directory");
    D_TEST_CHECK_MSG(_suite, "exists", d_dir_exists(buf), "...that exists");

    // no trailing separator, so d_path_join needs no special case
    length = strlen(buf);
    D_TEST_CHECK_MSG(_suite,
                     "no_trailing_sep",
                     (length > 0) && (buf[length - 1] != '/') && (buf[length - 1] != D_FILE_PATH_SEP),
                     "...with no trailing separator");

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     (d_dir_temp(NULL, 100)) == NULL,
                     "d_dir_temp rejects a NULL buffer");
    #endif

#if D_FILE_TEMP_HAS_TMPNAM
    // present for compatibility, and racy by construction -- the flaw is the
    // interface, not the implementation. Set D_CFG_FILE_TEMP_ALLOW_TMPNAM=0
    // and the symbol disappears from the object file entirely.
    memset(buf, 0, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "tmpnam",
                     (d_file_temp_name(buf, sizeof(buf))) == (0),
                     "d_file_temp_name generates a name");
    D_TEST_CHECK_MSG(_suite,
                     "tmpnam_nonempty",
                     strlen(buf) > 0,
                     "...a non-empty one");
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "tmpnam_null",
                     d_file_temp_name(NULL, 100) != 0,
                     "d_file_temp_name rejects a NULL buffer");
    #endif
    D_TEST_CHECK_MSG(_suite,
                     "tmpnam_tiny",
                     d_file_temp_name(buf, 1) != 0,
                     "d_file_temp_name rejects a tiny buffer");
#endif

    return true;
}


/*
d_tests_file_temp_all
  Runs every file_temp test.
*/
bool d_tests_file_temp_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "XII. file_temp");

    d_tests_file_tmpfile(_suite);
    d_tests_file_mkstemp(_suite);
    d_tests_file_tempdir(_suite);

    return true;
}
