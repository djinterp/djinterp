/*******************************************************************************
* djinterp [test]                                              file_tests_path.c
*
*   IX. PATHS (file_path) -- purely lexical, zero syscalls, so every case here
* is deterministic on a machine with no filesystem at all. The documented
* input/output pairs in file_path.c's own headers are reproduced as assertions;
* the internal branch structure (root handling, trailing-separator trimming,
* "." / ".." resolution, buffer-fit failure) is covered case by case.
*
*   Output separators are compared through d_fs_sep, which rewrites a '/'
* template into the platform's separator, so the join/normalize expectations
* hold on a POSIX and a Windows build alike. Inputs use '/', which is a
* separator on every grammar this framework parses. Windows-only grammar
* (drive letters, UNC) is gated on D_FILE_PATH_UNDERSTANDS_WINDOWS.
*
*
* path:      /tests/djinterp/c/fs/file_tests_path.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <errno.h>
#include <string.h>
// djinterp
#include "file_tests.h"


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

/*
d_fs_sep
  Rewrites a '/'-separated template into the separator this build EMITS, so a
single expectation is correct under either grammar.
  D_FILE_PATH_OUT_SEP, not D_FILE_PATH_SEP: the latter is the PLATFORM's
separator, which is what file_temp and file_dir want because they build paths
for the running kernel. What is being predicted here is d_path_join's output,
and on a Linux build with the Windows grammar selected those two disagree --
which is why eleven expectations in this file failed while the helper meant to
prevent exactly that was already in place.

Parameter(s):
  _buf:  destination buffer.
  _n:    size of _buf.
  _tmpl: template using '/' for every separator.
Return:
  _buf, holding the rewritten string.
*/
static const char*
d_fs_sep
(
    char*       _buf,
    size_t      _n,
    const char* _tmpl
)
{
    size_t idx;

    for (idx = 0; ((idx + 1) < _n) && (_tmpl[idx] != '\0'); ++idx)
    {
        _buf[idx] = (_tmpl[idx] == '/') ? D_FILE_PATH_OUT_SEP : _tmpl[idx];
    }

    _buf[idx] = '\0';

    return _buf;
}


/******************************************************************************
 * IX.a  d_path_dirname
 *****************************************************************************/

static void
d_fs_tests_path_dirname
(
    dtest_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF] = {0};

    dtest_block(_suite, "d_path_dirname");

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_dirname(NULL, buf, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_dirname("/a/b", NULL, 10) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "zero buffer size -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_dirname("/a/b", buf, 0) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
#endif
    dtest_test(_suite, "full path -> parent directory");
    D_FS_CHECK(_suite,
               d_path_dirname("/path/to/file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite,
               strcmp(buf, "/path/to") == 0);

    dtest_test(_suite, "bare filename -> current directory");
    D_FS_CHECK(_suite, d_path_dirname("file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, ".") == 0);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
    dtest_test(_suite, "file at root -> root");
    D_FS_CHECK(_suite, d_path_dirname("/file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "/") == 0);

    dtest_test(_suite, "empty path -> current directory");
    D_FS_CHECK(_suite, d_path_dirname("", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, ".") == 0);

    dtest_test(_suite, "trailing separator ignored");
    D_FS_CHECK(_suite, d_path_dirname("a/b/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "a") == 0);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
    dtest_test(_suite, "root is its own parent");
    D_FS_CHECK(_suite, d_path_dirname("/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "/") == 0);

    dtest_test(_suite, "single relative separator");
    D_FS_CHECK(_suite, d_path_dirname("a/b", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "a") == 0);

    dtest_test(_suite, "buffer too small -> NULL, ERANGE");
    errno = 0;
    D_FS_CHECK(_suite, d_path_dirname("/path/to/file", buf, 4) == NULL);
    D_FS_CHECK(_suite, errno == ERANGE);
}


/******************************************************************************
 * IX.b  d_path_basename
 *****************************************************************************/

static void
d_fs_tests_path_basename
(
    dtest_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF] = {0};

    dtest_block(_suite, "d_path_basename");

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_basename(NULL, buf, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_basename("/a/b", NULL, 10) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "zero buffer size -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_basename("/a/b", buf, 0) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);

#endif
    dtest_test(_suite, "full path -> final component");
    D_FS_CHECK(_suite,
               d_path_basename("/path/to/file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "file.txt") == 0);

    dtest_test(_suite, "bare filename -> itself");
    D_FS_CHECK(_suite, d_path_basename("file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "file.txt") == 0);

    dtest_test(_suite, "trailing separator ignored");
    D_FS_CHECK(_suite, d_path_basename("a/b/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "b") == 0);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
    dtest_test(_suite, "root names itself");
    D_FS_CHECK(_suite, d_path_basename("/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "/") == 0);

    dtest_test(_suite, "empty path -> empty string");
    D_FS_CHECK(_suite, d_path_basename("", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "") == 0);

    dtest_test(_suite, "buffer too small -> NULL, ERANGE");
    errno = 0;
    D_FS_CHECK(_suite, d_path_basename("/a/longname", buf, 3) == NULL);
    D_FS_CHECK(_suite, errno == ERANGE);
}


/******************************************************************************
 * IX.c  d_path_extension
 *****************************************************************************/

static void
d_fs_tests_path_get_extension
(
    dtest_suite* _suite
)
{
    const char* src;
    const char* ext;

    dtest_block(_suite, "d_path_extension");

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_extension(NULL) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "simple extension, points into input");
    src = "file.txt";
    ext = d_path_extension(src);
    D_FS_CHECK(_suite, ext != NULL);
    D_FS_CHECK(_suite, ext == (src + 4));
    // guarded in the same expression: a check records and CONTINUES, so a
    // preceding NULL check does not protect a later dereference
    D_FS_CHECK(_suite, (ext != NULL) && (strcmp(ext, ".txt") == 0));

    dtest_test(_suite, "last dot wins on a double extension");
    ext = d_path_extension("archive.tar.gz");
    D_FS_CHECK(_suite, (ext != NULL) && (strcmp(ext, ".gz") == 0));

    dtest_test(_suite, "no dot -> NULL");
    D_FS_CHECK(_suite, d_path_extension("filename") == NULL);

    dtest_test(_suite, "leading dot is a hidden file, not an extension");
    D_FS_CHECK(_suite, d_path_extension(".bashrc") == NULL);

    dtest_test(_suite, "dot in a parent directory does not count");
    D_FS_CHECK(_suite, d_path_extension("/a.d/file") == NULL);

    dtest_test(_suite, "trailing dot yields a lone dot");
    ext = d_path_extension("file.");
    D_FS_CHECK(_suite, (ext != NULL) && (strcmp(ext, ".") == 0));
}


/******************************************************************************
 * IX.d  d_path_stem
 *****************************************************************************/

static void
d_fs_tests_path_stem
(
    dtest_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF] = {0};

    dtest_block(_suite, "d_path_stem");

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_stem(NULL, buf, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_stem("a", NULL, 10) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    dtest_test(_suite, "final component without its extension");
    D_FS_CHECK(_suite, d_path_stem("/a/file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "file") == 0);

    dtest_test(_suite, "hidden file keeps its whole name");
    D_FS_CHECK(_suite, d_path_stem(".bashrc", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, ".bashrc") == 0);

    dtest_test(_suite, "only the final extension is removed");
    D_FS_CHECK(_suite, d_path_stem("archive.tar.gz", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "archive.tar") == 0);

    dtest_test(_suite, "no extension leaves the name intact");
    D_FS_CHECK(_suite, d_path_stem("noext", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "noext") == 0);
}


/******************************************************************************
 * IX.e  d_path_join
 *****************************************************************************/

static void
d_fs_tests_path_join
(
    dtest_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF] = {0};
    char exp[D_FS_TEST_PATH_BUF] = {0};

    dtest_block(_suite, "d_path_join");

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_join(NULL, 10, "a", "b") == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "zero buffer size -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_join(buf, 0, "a", "b") == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);

#endif
    dtest_test(_suite, "two components get exactly one separator");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "a", "b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

    dtest_test(_suite, "a caller's trailing separator is not doubled");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "a/", "b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

    dtest_test(_suite, "several trailing separators collapse to one");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "a///", "b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "absent first component leaves the second alone");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), NULL, "b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, "b") == 0);

#endif
#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "absent second component leaves the first alone");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "a", NULL) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "a") == 0);

#endif
    dtest_test(_suite, "empty second component leaves the first alone");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "a", "") == buf);
    D_FS_CHECK(_suite, strcmp(buf, "a") == 0);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
    dtest_test(_suite, "an absolute second component replaces the first");
    D_FS_CHECK(_suite,
               d_path_join(buf, sizeof(buf), "a", "/b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, "/b") == 0);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
    dtest_test(_suite, "joining onto the root keeps a single separator");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "/", "b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, "/b") == 0);

    dtest_test(_suite, "two empty components yield an empty path");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "", "") == buf);
    D_FS_CHECK(_suite, strcmp(buf, "") == 0);

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "two NULL components yield an empty path");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), NULL, NULL) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "") == 0);

#endif
    dtest_test(_suite, "result too large -> NULL, ERANGE");
    errno = 0;
    D_FS_CHECK(_suite, d_path_join(buf, 4, "aaa", "bbb") == NULL);
    D_FS_CHECK(_suite, errno == ERANGE);
}


/******************************************************************************
 * IX.f  d_path_is_absolute
 *****************************************************************************/

static void
d_fs_tests_path_is_absolute
(
    dtest_suite* _suite
)
{
    dtest_block(_suite, "d_path_is_absolute");

    //   Not gated: NULL is a documented ANSWER here (a nothing is not an
    // absolute path), not a rejected argument, so it holds in every build.
    dtest_test(_suite, "NULL -> not absolute (and not an error)");
    D_FS_CHECK(_suite, d_path_is_absolute(NULL) == 0);

    dtest_test(_suite, "rooted path is absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("/foo") != 0);

    dtest_test(_suite, "relative path is not absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("foo") == 0);

    dtest_test(_suite, "empty path is not absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("") == 0);

    dtest_test(_suite, "bare root is absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("/") != 0);

    dtest_test(_suite, "POSIX double-slash root is absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("//foo") != 0);

#if D_FILE_PATH_UNDERSTANDS_WINDOWS
    dtest_test(_suite, "drive-absolute is absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("C:\\x") != 0);

    dtest_test(_suite, "drive-relative is NOT absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("C:x") == 0);
#endif
}


/******************************************************************************
 * IX.g  d_path_root_length
 *****************************************************************************/

static void
d_fs_tests_path_root_length
(
    dtest_suite* _suite
)
{
    dtest_block(_suite, "d_path_root_length");

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL -> 0");
    D_FS_CHECK(_suite, d_path_root_length(NULL) == 0);
    #endif

    dtest_test(_suite, "single root separator -> 1");
    D_FS_CHECK(_suite, d_path_root_length("/") == 1);

    dtest_test(_suite, "POSIX double slash reserved -> 2");
    D_FS_CHECK(_suite, d_path_root_length("//") == 2);

    dtest_test(_suite, "three or more collapse to root -> 1");
    D_FS_CHECK(_suite, d_path_root_length("///") == 1);

    dtest_test(_suite, "rooted path -> 1");
    D_FS_CHECK(_suite, d_path_root_length("/foo") == 1);

    dtest_test(_suite, "relative path -> 0");
    D_FS_CHECK(_suite, d_path_root_length("foo") == 0);

    dtest_test(_suite, "empty path -> 0");
    D_FS_CHECK(_suite, d_path_root_length("") == 0);
}


/******************************************************************************
 * IX.h  d_path_normalize
 *****************************************************************************/

static void
d_fs_tests_path_normalize
(
    dtest_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF] = {0};
    char exp[D_FS_TEST_PATH_BUF] = {0};

    dtest_block(_suite, "d_path_normalize");

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_normalize(NULL, buf, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_normalize("a", NULL, 10) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    dtest_test(_suite, "buffer too small to hold anything -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_normalize("a", buf, 1) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);

#endif
    dtest_test(_suite, "dot-dot resolves against the preceding component");
    D_FS_CHECK(_suite,
               d_path_normalize("/path/to/../file.txt", buf, sizeof(buf))
                   == buf);
    D_FS_CHECK(_suite,
               strcmp(buf, d_fs_sep(exp, sizeof(exp), "/path/file.txt")) == 0);

    dtest_test(_suite, "separator runs collapse");
    D_FS_CHECK(_suite, d_path_normalize("a//b", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

    dtest_test(_suite, "dot components are dropped");
    D_FS_CHECK(_suite, d_path_normalize("./a/./b", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

    dtest_test(_suite, "trailing separator is stripped");
    D_FS_CHECK(_suite, d_path_normalize("a/b/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

    dtest_test(_suite, "leading dot-dot is kept when nothing precedes it");
    D_FS_CHECK(_suite, d_path_normalize("../../a", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "../../a")) == 0);

    dtest_test(_suite, "dot-dot at an absolute root is dropped");
    D_FS_CHECK(_suite, d_path_normalize("/../a", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "/a")) == 0);

    dtest_test(_suite, "interior dot-dot cancels one component");
    D_FS_CHECK(_suite, d_path_normalize("a/b/../c", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/c")) == 0);

    dtest_test(_suite, "empty path normalizes to the current directory");
    D_FS_CHECK(_suite, d_path_normalize("", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, ".") == 0);

    dtest_test(_suite, "a lone dot normalizes to the current directory");
    D_FS_CHECK(_suite, d_path_normalize(".", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, ".") == 0);

    dtest_test(_suite, "the root is preserved");
    D_FS_CHECK(_suite, d_path_normalize("/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "/")) == 0);

    dtest_test(_suite, "a path longer than the buffer -> NULL, ERANGE");
    errno = 0;
    D_FS_CHECK(_suite,
               d_path_normalize("/a/very/long/path/indeed", buf, 6) == NULL);
    D_FS_CHECK(_suite, errno == ERANGE);
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void
d_fs_tests_path
(
    dtest_suite* _suite
)
{
    dtest_module(_suite, "IX. file_path");

    d_fs_tests_path_dirname(_suite);
    d_fs_tests_path_basename(_suite);
    d_fs_tests_path_get_extension(_suite);
    d_fs_tests_path_stem(_suite);
    d_fs_tests_path_join(_suite);
    d_fs_tests_path_is_absolute(_suite);
    d_fs_tests_path_root_length(_suite);
    d_fs_tests_path_normalize(_suite);

    return;
}
