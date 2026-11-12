/*******************************************************************************
* djinterp [c][filesys]                                        file_tests_path.c
*
*   IX. Paths (file_path) -- purely lexical, zero syscalls, so every case here
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
* path:      /tests/djinterp/c/filesys/file_tests_path.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.06
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
static
const char* d_fs_sep(
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

static
void d_fs_tests_path_dirname(
    struct dj_test_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF] = {0};

    d_test_suite_block(_suite, "d_path_dirname");

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_dirname(NULL, buf, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_dirname("/a/b", NULL, 10) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "zero buffer size -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_dirname("/a/b", buf, 0) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
#endif
    d_test_suite_test(_suite, "full path -> parent directory");
    D_FS_CHECK(_suite,
               d_path_dirname("/path/to/file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite,
               strcmp(buf, "/path/to") == 0);

    d_test_suite_test(_suite, "bare filename -> current directory");
    D_FS_CHECK(_suite, d_path_dirname("file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, ".") == 0);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
    d_test_suite_test(_suite, "file at root -> root");
    D_FS_CHECK(_suite, d_path_dirname("/file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "/") == 0);

    d_test_suite_test(_suite, "empty path -> current directory");
    D_FS_CHECK(_suite, d_path_dirname("", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, ".") == 0);

    d_test_suite_test(_suite, "trailing separator ignored");
    D_FS_CHECK(_suite, d_path_dirname("a/b/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "a") == 0);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
    d_test_suite_test(_suite, "root is its own parent");
    D_FS_CHECK(_suite, d_path_dirname("/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "/") == 0);

    d_test_suite_test(_suite, "single relative separator");
    D_FS_CHECK(_suite, d_path_dirname("a/b", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "a") == 0);

    d_test_suite_test(_suite, "buffer too small -> NULL, ERANGE");
    errno = 0;
    D_FS_CHECK(_suite, d_path_dirname("/path/to/file", buf, 4) == NULL);
    D_FS_CHECK(_suite, errno == ERANGE);
}


/******************************************************************************
 * IX.b  d_path_basename
 *****************************************************************************/

static
void d_fs_tests_path_basename(
    struct dj_test_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF] = {0};

    d_test_suite_block(_suite, "d_path_basename");

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_basename(NULL, buf, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_basename("/a/b", NULL, 10) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "zero buffer size -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_basename("/a/b", buf, 0) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);

#endif
    d_test_suite_test(_suite, "full path -> final component");
    D_FS_CHECK(_suite,
               d_path_basename("/path/to/file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "file.txt") == 0);

    d_test_suite_test(_suite, "bare filename -> itself");
    D_FS_CHECK(_suite, d_path_basename("file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "file.txt") == 0);

    d_test_suite_test(_suite, "trailing separator ignored");
    D_FS_CHECK(_suite, d_path_basename("a/b/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "b") == 0);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
    d_test_suite_test(_suite, "root names itself");
    D_FS_CHECK(_suite, d_path_basename("/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "/") == 0);

    d_test_suite_test(_suite, "empty path -> empty string");
    D_FS_CHECK(_suite, d_path_basename("", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "") == 0);

    d_test_suite_test(_suite, "buffer too small -> NULL, ERANGE");
    errno = 0;
    D_FS_CHECK(_suite, d_path_basename("/a/longname", buf, 3) == NULL);
    D_FS_CHECK(_suite, errno == ERANGE);
}


/******************************************************************************
 * IX.c  d_path_extension
 *****************************************************************************/

static
void d_fs_tests_path_get_extension(
    struct dj_test_suite* _suite
)
{
    const char* src;
    const char* ext;

    d_test_suite_block(_suite, "d_path_extension");

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_extension(NULL) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    d_test_suite_test(_suite, "simple extension, points into input");
    src = "file.txt";
    ext = d_path_extension(src);
    D_FS_CHECK(_suite, ext != NULL);
    D_FS_CHECK(_suite, ext == (src + 4));
    // guarded in the same expression: a check records and CONTINUES, so a
    // preceding NULL check does not protect a later dereference
    D_FS_CHECK(_suite, (ext != NULL) && (strcmp(ext, ".txt") == 0));

    d_test_suite_test(_suite, "last dot wins on a double extension");
    ext = d_path_extension("archive.tar.gz");
    D_FS_CHECK(_suite, (ext != NULL) && (strcmp(ext, ".gz") == 0));

    d_test_suite_test(_suite, "no dot -> NULL");
    D_FS_CHECK(_suite, d_path_extension("filename") == NULL);

    d_test_suite_test(_suite, "leading dot is a hidden file, not an extension");
    D_FS_CHECK(_suite, d_path_extension(".bashrc") == NULL);

    d_test_suite_test(_suite, "dot in a parent directory does not count");
    D_FS_CHECK(_suite, d_path_extension("/a.d/file") == NULL);

    d_test_suite_test(_suite, "trailing dot yields a lone dot");
    ext = d_path_extension("file.");
    D_FS_CHECK(_suite, (ext != NULL) && (strcmp(ext, ".") == 0));
}


/******************************************************************************
 * IX.d  d_path_stem
 *****************************************************************************/

static
void d_fs_tests_path_stem(
    struct dj_test_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF] = {0};

    d_test_suite_block(_suite, "d_path_stem");

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_stem(NULL, buf, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_stem("a", NULL, 10) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    d_test_suite_test(_suite, "final component without its extension");
    D_FS_CHECK(_suite, d_path_stem("/a/file.txt", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "file") == 0);

    d_test_suite_test(_suite, "hidden file keeps its whole name");
    D_FS_CHECK(_suite, d_path_stem(".bashrc", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, ".bashrc") == 0);

    d_test_suite_test(_suite, "only the final extension is removed");
    D_FS_CHECK(_suite, d_path_stem("archive.tar.gz", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "archive.tar") == 0);

    d_test_suite_test(_suite, "no extension leaves the name intact");
    D_FS_CHECK(_suite, d_path_stem("noext", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "noext") == 0);
}


/******************************************************************************
 * IX.e  d_path_join
 *****************************************************************************/

static
void d_fs_tests_path_join(
    struct dj_test_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF] = {0};
    char exp[D_FS_TEST_PATH_BUF] = {0};

    d_test_suite_block(_suite, "d_path_join");

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_join(NULL, 10, "a", "b") == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "zero buffer size -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_join(buf, 0, "a", "b") == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);

#endif
    d_test_suite_test(_suite, "two components get exactly one separator");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "a", "b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

    d_test_suite_test(_suite, "a caller's trailing separator is not doubled");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "a/", "b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

    d_test_suite_test(_suite, "several trailing separators collapse to one");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "a///", "b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "absent first component leaves the second alone");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), NULL, "b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, "b") == 0);

#endif
#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "absent second component leaves the first alone");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "a", NULL) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "a") == 0);

#endif
    d_test_suite_test(_suite, "empty second component leaves the first alone");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "a", "") == buf);
    D_FS_CHECK(_suite, strcmp(buf, "a") == 0);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
    d_test_suite_test(_suite,
                      "an absolute second component replaces the first");
    D_FS_CHECK(_suite,
               d_path_join(buf, sizeof(buf), "a", "/b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, "/b") == 0);

    //   NOT rewritten: these results are SLICED from the caller's string, so
    // their separators are the caller's and pass through untouched. d_fs_sep
    // belongs on results the module CONSTRUCTS -- what it inserts follows the
    // grammar, what it copies does not.
    d_test_suite_test(_suite, "joining onto the root keeps a single separator");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "/", "b") == buf);
    D_FS_CHECK(_suite, strcmp(buf, "/b") == 0);

    d_test_suite_test(_suite, "two empty components yield an empty path");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), "", "") == buf);
    D_FS_CHECK(_suite, strcmp(buf, "") == 0);

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "two NULL components yield an empty path");
    D_FS_CHECK(_suite, d_path_join(buf, sizeof(buf), NULL, NULL) == buf);
    D_FS_CHECK(_suite, strcmp(buf, "") == 0);

#endif
    d_test_suite_test(_suite, "result too large -> NULL, ERANGE");
    errno = 0;
    D_FS_CHECK(_suite, d_path_join(buf, 4, "aaa", "bbb") == NULL);
    D_FS_CHECK(_suite, errno == ERANGE);
}


/******************************************************************************
 * IX.f  d_path_is_absolute
 *****************************************************************************/

static
void d_fs_tests_path_is_absolute(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "d_path_is_absolute");

    //   Not gated: NULL is a documented ANSWER here (a nothing is not an
    // absolute path), not a rejected argument, so it holds in every build.
    d_test_suite_test(_suite, "NULL -> not absolute (and not an error)");
    D_FS_CHECK(_suite, d_path_is_absolute(NULL) == 0);

#if D_FILE_PATH_UNDERSTANDS_WINDOWS
    // a lone root names the current drive's under Windows syntax
    d_test_suite_test(_suite, "rooted path is not absolute (Windows syntax)");
    D_FS_CHECK(_suite, d_path_is_absolute("/foo") == 0);
#else
    d_test_suite_test(_suite, "rooted path is absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("/foo") != 0);
#endif

    d_test_suite_test(_suite, "relative path is not absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("foo") == 0);

    d_test_suite_test(_suite, "empty path is not absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("") == 0);

#if D_FILE_PATH_UNDERSTANDS_WINDOWS
    d_test_suite_test(_suite, "bare root is not absolute (Windows syntax)");
    D_FS_CHECK(_suite, d_path_is_absolute("/") == 0);
#else
    d_test_suite_test(_suite, "bare root is absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("/") != 0);
#endif

    d_test_suite_test(_suite, "POSIX double-slash root is absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("//foo") != 0);

#if D_FILE_PATH_UNDERSTANDS_WINDOWS
    d_test_suite_test(_suite, "drive-absolute is absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("C:\\x") != 0);

    d_test_suite_test(_suite, "drive-relative is NOT absolute");
    D_FS_CHECK(_suite, d_path_is_absolute("C:x") == 0);
#endif
}


/******************************************************************************
 * IX.g  d_path_root_length
 *****************************************************************************/

static
void d_fs_tests_path_root_length(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "d_path_root_length");

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL -> 0");
    D_FS_CHECK(_suite, d_path_root_length(NULL) == 0);
    #endif

    d_test_suite_test(_suite, "single root separator -> 1");
    D_FS_CHECK(_suite, d_path_root_length("/") == 1);

    d_test_suite_test(_suite, "POSIX double slash reserved -> 2");
    D_FS_CHECK(_suite, d_path_root_length("//") == 2);

    d_test_suite_test(_suite, "three or more collapse to root -> 1");
    D_FS_CHECK(_suite, d_path_root_length("///") == 1);

    d_test_suite_test(_suite, "rooted path -> 1");
    D_FS_CHECK(_suite, d_path_root_length("/foo") == 1);

    d_test_suite_test(_suite, "relative path -> 0");
    D_FS_CHECK(_suite, d_path_root_length("foo") == 0);

    d_test_suite_test(_suite, "empty path -> 0");
    D_FS_CHECK(_suite, d_path_root_length("") == 0);
}


/******************************************************************************
 * IX.h  d_path_normalize
 *****************************************************************************/

static
void d_fs_tests_path_normalize(
    struct dj_test_suite* _suite
)
{
    char buf[D_FS_TEST_PATH_BUF] = {0};
    char exp[D_FS_TEST_PATH_BUF] = {0};

    d_test_suite_block(_suite, "d_path_normalize");

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL path -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_normalize(NULL, buf, sizeof(buf)) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite, "NULL buffer -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_normalize("a", NULL, 10) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);
    #endif

#if D_FILE_VALIDATE_IS_ACTIVE
    d_test_suite_test(_suite,
                      "buffer too small to hold anything -> NULL, EINVAL");
    errno = 0;
    D_FS_CHECK(_suite, d_path_normalize("a", buf, 1) == NULL);
    D_FS_CHECK(_suite, errno == EINVAL);

#endif
    d_test_suite_test(_suite,
                      "dot-dot resolves against the preceding component");
    D_FS_CHECK(_suite,
               d_path_normalize("/path/to/../file.txt", buf, sizeof(buf))
                   == buf);
    D_FS_CHECK(_suite,
               strcmp(buf, d_fs_sep(exp, sizeof(exp), "/path/file.txt")) == 0);

    d_test_suite_test(_suite, "separator runs collapse");
    D_FS_CHECK(_suite, d_path_normalize("a//b", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

    d_test_suite_test(_suite, "dot components are dropped");
    D_FS_CHECK(_suite, d_path_normalize("./a/./b", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

    d_test_suite_test(_suite, "trailing separator is stripped");
    D_FS_CHECK(_suite, d_path_normalize("a/b/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/b")) == 0);

    d_test_suite_test(_suite,
                      "leading dot-dot is kept when nothing precedes it");
    D_FS_CHECK(_suite, d_path_normalize("../../a", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "../../a")) == 0);

    d_test_suite_test(_suite, "dot-dot at an absolute root is dropped");
    D_FS_CHECK(_suite, d_path_normalize("/../a", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "/a")) == 0);

    d_test_suite_test(_suite, "interior dot-dot cancels one component");
    D_FS_CHECK(_suite, d_path_normalize("a/b/../c", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "a/c")) == 0);

    d_test_suite_test(_suite, "empty path normalizes to the current directory");
    D_FS_CHECK(_suite, d_path_normalize("", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, ".") == 0);

    d_test_suite_test(_suite, "a lone dot normalizes to the current directory");
    D_FS_CHECK(_suite, d_path_normalize(".", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, ".") == 0);

    d_test_suite_test(_suite, "the root is preserved");
    D_FS_CHECK(_suite, d_path_normalize("/", buf, sizeof(buf)) == buf);
    D_FS_CHECK(_suite, strcmp(buf, d_fs_sep(exp, sizeof(exp), "/")) == 0);

    d_test_suite_test(_suite, "a path longer than the buffer -> NULL, ERANGE");
    errno = 0;
    D_FS_CHECK(_suite,
               d_path_normalize("/a/very/long/path/indeed", buf, 6) == NULL);
    D_FS_CHECK(_suite, errno == ERANGE);
}


/******************************************************************************
 * SECTION ENTRY POINT
 *****************************************************************************/

void d_fs_tests_path(
    struct dj_test_suite* _suite
)
{
    d_test_suite_module(_suite, "IX. file_path");

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

/******************************************************************************
 * d_tests_file_path_all: this area's block in d_tests_file_run_all
 *****************************************************************************/

/******************************************************************************
 * IX.  file_path  --  LEXICAL PATH MANIPULATION
 *
 *   The only fs suite that touches no filesystem. Nothing here creates a
 * file, needs a temp directory, or cares what the disk contains -- so it runs
 * identically on a build server, in a container with a read-only root, and on
 * a target with no storage at all. That is a property of the module, not of
 * the tests, and it is why file_path was split out of file_dir.
 *
 *   It is also the suite most sensitive to configuration. D_CFG_FILE_PATH_-
 * SYNTAX decides which grammar is parsed, so the assertions branch on
 * D_FILE_PATH_UNDERSTANDS_WINDOWS rather than on the host -- which is the
 * whole point: a Linux build asked to read Windows paths must be TESTED
 * reading Windows paths, and the platform cannot tell you whether it was.
 *****************************************************************************/


/*
d_tests_file_dirname
  Tests d_path_dirname.
  Tests the following:
  - extracts the directory component
  - a bare filename has "." for a parent
  - trailing separators are ignored, so "a/b/" and "a/b" agree
  - a root is its own parent
  - NULL is rejected
*/
static
bool d_tests_file_dirname(struct dj_test_suite* _suite)
{
    char                  buf[D_TEST_FILE_PATH_BUF];

    d_test_suite_test(_suite, "d_path_dirname");

    d_path_dirname("/path/to/file.txt", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "nested",
                     d_test_suite_str_equal((buf), ("/path/to")),
                     "d_path_dirname extracts the directory");

    d_path_dirname("file.txt", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "bare",
                     d_test_suite_str_equal((buf), (".")),
                     "a bare filename's parent is .");

    d_path_dirname("/file.txt", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "at_root",
                     d_test_suite_str_equal((buf), ("/")),
                     "a file at the root has / for a parent");

    // the old dfile left this unspecified; "a/b/" and "a/b" name the same
    // thing, so they must decompose the same way -- as the shell's basename
    // and dirname do
    d_path_dirname("a/b/", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "trailing_sep",
                     d_test_suite_str_equal((buf), ("a")),
                     "trailing separators are ignored");

    d_path_dirname("/", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "root",
                     d_test_suite_str_equal((buf), ("/")),
                     "the root is its own parent");

    d_path_dirname("", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "empty",
                     d_test_suite_str_equal((buf), (".")),
                     "an empty path's parent is .");

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     (d_path_dirname(NULL, buf, sizeof(buf))) == NULL,
                     "d_path_dirname rejects a NULL path");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_buf",
                     (d_path_dirname("/a/b", NULL, 10)) == NULL,
                     "d_path_dirname rejects a NULL buffer");
    #endif

    // truncation is a FAILURE, not a shortened path. A truncated path is how
    // a program deletes the wrong directory.
    {
        char tiny[5];

        D_TEST_CHECK_MSG(_suite,
                         "truncation",
                         (d_path_dirname("/aaa/bbb/ccc", tiny, sizeof(tiny))) == NULL,
                         "a result that does not fit is an error, not a prefix");
    }

    return true;
}


/*
d_tests_file_basename
  Tests d_path_basename.
  Tests the following:
  - extracts the final component
  - trailing separators are ignored
  - a root names itself
  - NULL is rejected
*/
static
bool d_tests_file_basename(struct dj_test_suite* _suite)
{
    char                  buf[D_TEST_FILE_PATH_BUF];

    d_test_suite_test(_suite, "d_path_basename");

    d_path_basename("/path/to/file.txt", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "nested",
                     d_test_suite_str_equal((buf), ("file.txt")),
                     "d_path_basename extracts the final component");

    d_path_basename("file.txt", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "bare",
                     d_test_suite_str_equal((buf), ("file.txt")),
                     "a bare filename is its own basename");

    d_path_basename("a/b/", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "trailing_sep",
                     d_test_suite_str_equal((buf), ("b")),
                     "trailing separators are ignored");

    d_path_basename("/", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "root",
                     d_test_suite_str_equal((buf), ("/")),
                     "the root names itself");

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_path",
                     (d_path_basename(NULL, buf, sizeof(buf))) == NULL,
                     "d_path_basename rejects a NULL path");
    #endif

    return true;
}


/*
d_tests_file_extension
  Tests d_path_extension and d_path_stem.
  Tests the following:
  - finds the extension, including the dot
  - a leading dot hides a file, it does not type it
  - only the final component is examined
  - stem and extension agree by construction
*/
static
bool d_tests_file_extension(struct dj_test_suite* _suite)
{
    char                  buf[D_TEST_FILE_PATH_BUF];
    const char*           ext;

    d_test_suite_test(_suite, "d_path_extension / d_path_stem");

    ext = d_path_extension("file.txt");
    D_TEST_CHECK_MSG(_suite,
                     "simple",
                     d_test_suite_str_equal((ext), (".txt")),
                     "d_path_extension includes the dot");

    ext = d_path_extension("archive.tar.gz");
    D_TEST_CHECK_MSG(_suite,
                     "double",
                     d_test_suite_str_equal((ext), (".gz")),
                     "only the LAST extension is returned");

    D_TEST_CHECK_MSG(_suite,
                     "none",
                     (d_path_extension("filename")) == NULL,
                     "a file with no dot has no extension");

    // ".bashrc" is a hidden file whose whole name is ".bashrc" -- not a file
    // with a ".bashrc" extension
    D_TEST_CHECK_MSG(_suite,
                     "hidden",
                     (d_path_extension(".bashrc")) == NULL,
                     "a leading dot hides a file, it does not type it");

    // the dot is in a PARENT, not in the final component
    D_TEST_CHECK_MSG(_suite,
                     "dot_in_parent",
                     (d_path_extension("/etc/x.d/file")) == NULL,
                     "a dot in a parent directory is not an extension");

    ext = d_path_extension("file.");
    D_TEST_CHECK_MSG(_suite,
                     "trailing_dot",
                     d_test_suite_str_equal((ext), (".")),
                     "a trailing dot is an empty extension");

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     (d_path_extension(NULL)) == NULL,
                     "d_path_extension rejects NULL");
    #endif

    // stem and extension are complements, so they must agree for every input
    // -- including the hidden-file case, where the extension is nothing and
    // the stem is therefore the whole name
    d_path_stem("/a/file.txt", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "stem",
                     d_test_suite_str_equal((buf), ("file")),
                     "d_path_stem drops the extension");

    d_path_stem(".bashrc", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "stem_hidden",
                     d_test_suite_str_equal((buf), (".bashrc")),
                     "a hidden file's stem is its whole name");

    d_path_stem("archive.tar.gz", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "stem_double",
                     d_test_suite_str_equal((buf), ("archive.tar")),
                     "stem + extension reconstructs the basename");

    return true;
}


/*
d_tests_file_path_join
  Tests d_path_join.
  Tests the following:
  - joins with exactly one separator, however many were supplied
  - an absent component is skipped rather than being an error
  - an absolute second component replaces the first
  - truncation is an error
*/
static
bool d_tests_file_path_join(struct dj_test_suite* _suite)
{
    char                  buf[D_TEST_FILE_PATH_BUF];

    d_test_suite_test(_suite, "d_path_join");

    d_path_join(buf, sizeof(buf), "path/to", "file.txt");
    //   Only the INSERTED separator follows the grammar. d_path_join joins;
    // it does not normalize, so a separator already inside an argument is
    // passed through untouched -- "path/to" stays "path/to" even where the
    // emitted one is a backslash. d_path_normalize is the function that
    // rewrites them, and its own assertions below expect exactly that.
    D_TEST_CHECK_MSG(_suite,
                     "simple",
                     d_test_suite_str_equal((buf), ("path/to" D_TEST_FILE_EMIT_SEP "file.txt")),
                     "d_path_join inserts one separator");

    d_path_join(buf, sizeof(buf), "a/", "b");
    D_TEST_CHECK_MSG(_suite,
                     "dedup_sep",
                     d_test_suite_str_equal((buf), ("a" D_TEST_FILE_EMIT_SEP "b")),
                     "one separator, however many were given");

    d_path_join(buf, sizeof(buf), "/", "b");
    D_TEST_CHECK_MSG(_suite,
                     "from_root",
                     d_test_suite_str_equal((buf), ("/b")),
                     "joining onto the root does not eat it");

    // a NULL component is skipped, so a caller may pass an optional base
    // directory straight through without branching
    d_path_join(buf, sizeof(buf), NULL, "file.txt");
    D_TEST_CHECK_MSG(_suite,
                     "null_first",
                     d_test_suite_str_equal((buf), ("file.txt")),
                     "an absent first component leaves the second alone");

    d_path_join(buf, sizeof(buf), "a", NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_second",
                     d_test_suite_str_equal((buf), ("a")),
                     "an absent second component leaves the first alone");

#if D_CFG_IS_ON(D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS)
    // this build says an absolute second component REPLACES the first, which
    // is what every other path library does. The alternative silently builds
    // "/base/etc/passwd" for code that meant to be handed an override.
    d_path_join(buf, sizeof(buf), "/base", "/etc/passwd");
    D_TEST_CHECK_MSG(_suite,
                     "absolute_wins",
                     d_test_suite_str_equal((buf), ("/etc/passwd")),
                     "an absolute second component replaces the first");
#else
    d_path_join(buf, sizeof(buf), "/base", "/etc/passwd");
    D_TEST_CHECK_MSG(_suite,
                     "absolute_concat",
                     d_test_suite_str_equal((buf), ("/base/etc/passwd")),
                     "JOIN_ABSOLUTE_WINS=0 concatenates textually");
#endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null_buf",
                     (d_path_join(NULL, 100, "a", "b")) == NULL,
                     "d_path_join rejects a NULL buffer");
    #endif

    {
        char tiny[5];

        D_TEST_CHECK_MSG(_suite,
                         "truncation",
                         (d_path_join(tiny, sizeof(tiny), "aaaa", "bbbb")) == NULL,
                         "a join that does not fit is an error, not a prefix");
    }

    return true;
}


/*
d_tests_file_path_absolute
  Tests d_path_is_absolute and d_path_root_length.
  Tests the following:
  - POSIX roots
  - Windows roots, when this build parses that grammar
  - "C:x" is drive-RELATIVE, which string comparison gets wrong
*/
static
bool d_tests_file_path_absolute(struct dj_test_suite* _suite)
{
#if D_FILE_PATH_UNDERSTANDS_WINDOWS
    d_test_suite_test(_suite, "d_path_is_absolute");

    D_TEST_CHECK_MSG(_suite,
                     "win_drive",
                     d_path_is_absolute("C:\\path\\to\\file"),
                     "a drive-anchored path is absolute");
    D_TEST_CHECK_MSG(_suite,
                     "win_fwdslash",
                     d_path_is_absolute("C:/path"),
                     "Win32 accepts / too, so this build must");

    // THE one every hand-rolled path parser gets wrong. "C:x" means "x,
    // relative to whatever the current directory on drive C happens to be" --
    // Win32 keeps a per-drive cursor. Only "C:\x" is anchored.
    D_TEST_CHECK_MSG(_suite,
                     "win_drive_relative",
                     !(d_path_is_absolute("C:x")),
                     "C:x is drive-RELATIVE, not absolute");
    D_TEST_CHECK_MSG(_suite,
                     "win_drive_rel_root",
                     (d_path_root_length("C:x")) == ((size_t)2),
                     "C: is a root to climb from, but not absolute");
    D_TEST_CHECK_MSG(_suite,
                     "win_drive_abs_root",
                     (d_path_root_length("C:\\x")) == ((size_t)3),
                     "C:\\ includes the separator");
    D_TEST_CHECK_MSG(_suite,
                     "win_unc",
                     d_path_is_absolute("\\\\server\\share\\f"),
                     "a UNC path is absolute");
    D_TEST_CHECK_MSG(_suite,
                     "win_relative",
                     !(d_path_is_absolute("relative\\path")),
                     "a relative Windows path is not absolute");
#else
    D_TEST_CHECK_MSG(_suite,
                     "posix_root",
                     d_path_is_absolute("/path/to/file"),
                     "a leading / is absolute");
    D_TEST_CHECK_MSG(_suite,
                     "posix_relative",
                     !(d_path_is_absolute("path/to/file")),
                     "no leading / is relative");

    // POSIX gives exactly two leading slashes an implementation-defined
    // meaning, and three or more none at all
    D_TEST_CHECK_MSG(_suite,
                     "posix_double_slash",
                     (d_path_root_length("//net/share")) == ((size_t)2),
                     "// is a root POSIX reserves");
    D_TEST_CHECK_MSG(_suite,
                     "posix_triple_slash",
                     (d_path_root_length("///x")) == ((size_t)1),
                     "/// is just the root");

    // under POSIX syntax a backslash is an ordinary filename byte, and
    // treating it as a separator would corrupt legitimate names
    D_TEST_CHECK_MSG(_suite,
                     "posix_backslash_is_data",
                     !(d_path_is_absolute("C:\\path")),
                     "under POSIX syntax, C:\\path is one relative name");
#endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     !(d_path_is_absolute(NULL)),
                     "NULL is not absolute");
    #endif
    D_TEST_CHECK_MSG(_suite,
                     "empty",
                     !(d_path_is_absolute("")),
                     "an empty path is not absolute");

    return true;
}


/*
d_tests_file_path_normalize
  Tests d_path_normalize.
  Tests the following:
  - collapses . .. and separator runs
  - a .. with nothing to cancel is KEPT
  - the root has no parent
  - the result is lexical, and says so
*/
static
bool d_tests_file_path_normalize(struct dj_test_suite* _suite)
{
    char                  buf[D_TEST_FILE_PATH_BUF];

#if D_CFG_IS_ON(D_CFG_FILE_PATH_COLLAPSE_SEPARATORS)
    d_test_suite_test(_suite, "d_path_normalize");

    d_path_normalize("a//b", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "collapse",
                     d_test_suite_str_equal((buf), ("a" D_TEST_FILE_EMIT_SEP "b")),
                     "separator runs collapse");
#endif

    d_path_normalize("./a/./b", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "dot",
                     d_test_suite_str_equal((buf), ("a" D_TEST_FILE_EMIT_SEP "b")),
                     ". means stay here, so it drops out");

#if D_CFG_IS_ON(D_CFG_FILE_PATH_NORMALIZE_DOTDOT)
    d_path_normalize("/path/to/../file.txt", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "dotdot",
                     d_test_suite_str_equal((buf), (D_TEST_FILE_EMIT_SEP "path" D_TEST_FILE_EMIT_SEP "file.txt")),
                     ".. cancels the component before it");

    d_path_normalize("/a/b/../c", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "dotdot_mid",
                     d_test_suite_str_equal((buf), (D_TEST_FILE_EMIT_SEP "a" D_TEST_FILE_EMIT_SEP "c")),
                     ".. resolves mid-path");

    // a .. with nothing above it to cancel must be KEPT: "../b" is a real
    // path and dropping the .. would silently change which file it names
    d_path_normalize("../../a", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "dotdot_leading",
                     d_test_suite_str_equal((buf), (".." D_TEST_FILE_EMIT_SEP ".." D_TEST_FILE_EMIT_SEP "a")),
                     "a .. with nothing to cancel is kept");

    d_path_normalize("a/../../b", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "dotdot_escapes",
                     d_test_suite_str_equal((buf), (".." D_TEST_FILE_EMIT_SEP "b")),
                     "the surplus .. survives the cancellation");

    // POSIX: the root's parent is the root
    d_path_normalize("/../a", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "dotdot_at_root",
                     d_test_suite_str_equal((buf), (D_TEST_FILE_EMIT_SEP "a")),
                     "the root has no parent to climb to");

    d_path_normalize("a/b/../..", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "cancels_out",
                     d_test_suite_str_equal((buf), (".")),
                     "a path that fully cancels is .");
#endif

#if D_CFG_IS_ON(D_CFG_FILE_PATH_STRIP_TRAILING_SEP)
    d_path_normalize("a/b/", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "strip_trailing",
                     d_test_suite_str_equal((buf), ("a" D_TEST_FILE_EMIT_SEP "b")),
                     "a trailing separator is punctuation, and drops");

    d_path_normalize("/", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "keep_root",
                     d_test_suite_str_equal((buf), (D_TEST_FILE_EMIT_SEP)),
                     "...but the root's separator IS the path");
#endif

    d_path_normalize("", buf, sizeof(buf));
    D_TEST_CHECK_MSG(_suite,
                     "empty",
                     d_test_suite_str_equal((buf), (".")),
                     "an empty path normalizes to .");

    #if D_FILE_VALIDATE_IS_ACTIVE
    D_TEST_CHECK_MSG(_suite,
                     "null",
                     (d_path_normalize(NULL, buf, sizeof(buf))) == NULL,
                     "d_path_normalize rejects NULL");
    #endif

    return true;
}


/*
d_tests_file_path_all
  Runs every file_path test.
*/
bool d_tests_file_path_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "IX. file_path (lexical; no filesystem)");

    d_tests_file_dirname(_suite);
    d_tests_file_basename(_suite);
    d_tests_file_extension(_suite);
    d_tests_file_path_join(_suite);
    d_tests_file_path_absolute(_suite);
    d_tests_file_path_normalize(_suite);

    return true;
}
