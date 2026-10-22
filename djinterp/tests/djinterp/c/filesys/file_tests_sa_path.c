/*******************************************************************************
* djinterp [fs]                                             file_tests_sa_path.c
*
*
* path:      /tests/djinterp/c/filesys/file_tests_sa_path.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.20
*******************************************************************************/
// djinterp
#include "./file_tests_sa.h"


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
static struct d_test_object*
d_tests_file_dirname
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  buf[D_TEST_FILE_PATH_BUF];
    size_t                idx;

    idx = 0;

    d_path_dirname("/path/to/file.txt", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("nested", buf, "/path/to",
                                      "d_path_dirname extracts the directory");

    d_path_dirname("file.txt", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("bare", buf, ".",
                                      "a bare filename's parent is .");

    d_path_dirname("/file.txt", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("at_root", buf, "/",
                                      "a file at the root has / for a parent");

    // the old dfile left this unspecified; "a/b/" and "a/b" name the same
    // thing, so they must decompose the same way -- as the shell's basename
    // and dirname do
    d_path_dirname("a/b/", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("trailing_sep", buf, "a",
                                      "trailing separators are ignored");

    d_path_dirname("/", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("root", buf, "/",
                                      "the root is its own parent");

    d_path_dirname("", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("empty", buf, ".",
                                      "an empty path's parent is .");

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null_path",
                                 d_path_dirname(NULL, buf, sizeof(buf)),
                                 "d_path_dirname rejects a NULL path");
    #endif
    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null_buf",
                                 d_path_dirname("/a/b", NULL, 10),
                                 "d_path_dirname rejects a NULL buffer");
    #endif

    // truncation is a FAILURE, not a shortened path. A truncated path is how
    // a program deletes the wrong directory.
    {
        char tiny[5];

        items[idx++] = D_ASSERT_NULL("truncation",
                                     d_path_dirname("/aaa/bbb/ccc", tiny, sizeof(tiny)),
                                     "a result that does not fit is an error, not a prefix");
    }

    return d_tests_file_group("d_path_dirname", items, idx);
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
static struct d_test_object*
d_tests_file_basename
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  buf[D_TEST_FILE_PATH_BUF];
    size_t                idx;

    idx = 0;

    d_path_basename("/path/to/file.txt", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("nested", buf, "file.txt",
                                      "d_path_basename extracts the final component");

    d_path_basename("file.txt", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("bare", buf, "file.txt",
                                      "a bare filename is its own basename");

    d_path_basename("a/b/", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("trailing_sep", buf, "b",
                                      "trailing separators are ignored");

    d_path_basename("/", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("root", buf, "/",
                                      "the root names itself");

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null_path",
                                 d_path_basename(NULL, buf, sizeof(buf)),
                                 "d_path_basename rejects a NULL path");
    #endif

    return d_tests_file_group("d_path_basename", items, idx);
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
static struct d_test_object*
d_tests_file_extension
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  buf[D_TEST_FILE_PATH_BUF];
    const char*           ext;
    size_t                idx;

    idx = 0;

    ext = d_path_extension("file.txt");
    items[idx++] = D_ASSERT_STR_EQUAL("simple", ext, ".txt",
                                      "d_path_extension includes the dot");

    ext = d_path_extension("archive.tar.gz");
    items[idx++] = D_ASSERT_STR_EQUAL("double", ext, ".gz",
                                      "only the LAST extension is returned");

    items[idx++] = D_ASSERT_NULL("none",
                                 d_path_extension("filename"),
                                 "a file with no dot has no extension");

    // ".bashrc" is a hidden file whose whole name is ".bashrc" -- not a file
    // with a ".bashrc" extension
    items[idx++] = D_ASSERT_NULL("hidden",
                                 d_path_extension(".bashrc"),
                                 "a leading dot hides a file, it does not type it");

    // the dot is in a PARENT, not in the final component
    items[idx++] = D_ASSERT_NULL("dot_in_parent",
                                 d_path_extension("/etc/x.d/file"),
                                 "a dot in a parent directory is not an extension");

    ext = d_path_extension("file.");
    items[idx++] = D_ASSERT_STR_EQUAL("trailing_dot", ext, ".",
                                      "a trailing dot is an empty extension");

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null",
                                 d_path_extension(NULL),
                                 "d_path_extension rejects NULL");
    #endif

    // stem and extension are complements, so they must agree for every input
    // -- including the hidden-file case, where the extension is nothing and
    // the stem is therefore the whole name
    d_path_stem("/a/file.txt", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("stem", buf, "file",
                                      "d_path_stem drops the extension");

    d_path_stem(".bashrc", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("stem_hidden", buf, ".bashrc",
                                      "a hidden file's stem is its whole name");

    d_path_stem("archive.tar.gz", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("stem_double", buf, "archive.tar",
                                      "stem + extension reconstructs the basename");

    return d_tests_file_group("d_path_extension / d_path_stem", items, idx);
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
static struct d_test_object*
d_tests_file_path_join
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  buf[D_TEST_FILE_PATH_BUF];
    size_t                idx;

    idx = 0;

    d_path_join(buf, sizeof(buf), "path/to", "file.txt");
    //   Only the INSERTED separator follows the grammar. d_path_join joins;
    // it does not normalize, so a separator already inside an argument is
    // passed through untouched -- "path/to" stays "path/to" even where the
    // emitted one is a backslash. d_path_normalize is the function that
    // rewrites them, and its own assertions below expect exactly that.
    items[idx++] = D_ASSERT_STR_EQUAL("simple", buf, "path/to" D_TEST_FILE_EMIT_SEP "file.txt",
                                      "d_path_join inserts one separator");

    d_path_join(buf, sizeof(buf), "a/", "b");
    items[idx++] = D_ASSERT_STR_EQUAL("dedup_sep", buf, "a" D_TEST_FILE_EMIT_SEP "b",
                                      "one separator, however many were given");

    d_path_join(buf, sizeof(buf), "/", "b");
    items[idx++] = D_ASSERT_STR_EQUAL("from_root", buf, "/b",
                                      "joining onto the root does not eat it");

    // a NULL component is skipped, so a caller may pass an optional base
    // directory straight through without branching
    d_path_join(buf, sizeof(buf), NULL, "file.txt");
    items[idx++] = D_ASSERT_STR_EQUAL("null_first", buf, "file.txt",
                                      "an absent first component leaves the second alone");

    d_path_join(buf, sizeof(buf), "a", NULL);
    items[idx++] = D_ASSERT_STR_EQUAL("null_second", buf, "a",
                                      "an absent second component leaves the first alone");

#if D_CFG_IS_ON(D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS)
    // this build says an absolute second component REPLACES the first, which
    // is what every other path library does. The alternative silently builds
    // "/base/etc/passwd" for code that meant to be handed an override.
    d_path_join(buf, sizeof(buf), "/base", "/etc/passwd");
    items[idx++] = D_ASSERT_STR_EQUAL("absolute_wins", buf, "/etc/passwd",
                                      "an absolute second component replaces the first");
#else
    d_path_join(buf, sizeof(buf), "/base", "/etc/passwd");
    items[idx++] = D_ASSERT_STR_EQUAL("absolute_concat", buf, "/base/etc/passwd",
                                      "JOIN_ABSOLUTE_WINS=0 concatenates textually");
#endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null_buf",
                                 d_path_join(NULL, 100, "a", "b"),
                                 "d_path_join rejects a NULL buffer");
    #endif

    {
        char tiny[5];

        items[idx++] = D_ASSERT_NULL("truncation",
                                     d_path_join(tiny, sizeof(tiny), "aaaa", "bbbb"),
                                     "a join that does not fit is an error, not a prefix");
    }

    return d_tests_file_group("d_path_join", items, idx);
}


/*
d_tests_file_path_absolute
  Tests d_path_is_absolute and d_path_root_length.
  Tests the following:
  - POSIX roots
  - Windows roots, when this build parses that grammar
  - "C:x" is drive-RELATIVE, which string comparison gets wrong
*/
static struct d_test_object*
d_tests_file_path_absolute
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

#if D_FILE_PATH_UNDERSTANDS_WINDOWS
    items[idx++] = D_ASSERT_TRUE("win_drive",
                                 d_path_is_absolute("C:\\path\\to\\file"),
                                 "a drive-anchored path is absolute");
    items[idx++] = D_ASSERT_TRUE("win_fwdslash",
                                 d_path_is_absolute("C:/path"),
                                 "Win32 accepts / too, so this build must");

    // THE one every hand-rolled path parser gets wrong. "C:x" means "x,
    // relative to whatever the current directory on drive C happens to be" --
    // Win32 keeps a per-drive cursor. Only "C:\x" is anchored.
    items[idx++] = D_ASSERT_FALSE("win_drive_relative",
                                  d_path_is_absolute("C:x"),
                                  "C:x is drive-RELATIVE, not absolute");
    items[idx++] = D_ASSERT_EQUAL("win_drive_rel_root",
                                  d_path_root_length("C:x"), (size_t)2,
                                  "C: is a root to climb from, but not absolute");
    items[idx++] = D_ASSERT_EQUAL("win_drive_abs_root",
                                  d_path_root_length("C:\\x"), (size_t)3,
                                  "C:\\ includes the separator");
    items[idx++] = D_ASSERT_TRUE("win_unc",
                                 d_path_is_absolute("\\\\server\\share\\f"),
                                 "a UNC path is absolute");
    items[idx++] = D_ASSERT_FALSE("win_relative",
                                  d_path_is_absolute("relative\\path"),
                                  "a relative Windows path is not absolute");
#else
    items[idx++] = D_ASSERT_TRUE("posix_root",
                                 d_path_is_absolute("/path/to/file"),
                                 "a leading / is absolute");
    items[idx++] = D_ASSERT_FALSE("posix_relative",
                                  d_path_is_absolute("path/to/file"),
                                  "no leading / is relative");

    // POSIX gives exactly two leading slashes an implementation-defined
    // meaning, and three or more none at all
    items[idx++] = D_ASSERT_EQUAL("posix_double_slash",
                                  d_path_root_length("//net/share"), (size_t)2,
                                  "// is a root POSIX reserves");
    items[idx++] = D_ASSERT_EQUAL("posix_triple_slash",
                                  d_path_root_length("///x"), (size_t)1,
                                  "/// is just the root");

    // under POSIX syntax a backslash is an ordinary filename byte, and
    // treating it as a separator would corrupt legitimate names
    items[idx++] = D_ASSERT_FALSE("posix_backslash_is_data",
                                  d_path_is_absolute("C:\\path"),
                                  "under POSIX syntax, C:\\path is one relative name");
#endif

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_FALSE("null",
                                  d_path_is_absolute(NULL),
                                  "NULL is not absolute");
    #endif
    items[idx++] = D_ASSERT_FALSE("empty",
                                  d_path_is_absolute(""),
                                  "an empty path is not absolute");

    return d_tests_file_group("d_path_is_absolute", items, idx);
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
static struct d_test_object*
d_tests_file_path_normalize
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    char                  buf[D_TEST_FILE_PATH_BUF];
    size_t                idx;

    idx = 0;

#if D_CFG_IS_ON(D_CFG_FILE_PATH_COLLAPSE_SEPARATORS)
    d_path_normalize("a//b", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("collapse", buf, "a" D_TEST_FILE_EMIT_SEP "b",
                                      "separator runs collapse");
#endif

    d_path_normalize("./a/./b", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("dot", buf, "a" D_TEST_FILE_EMIT_SEP "b",
                                      ". means stay here, so it drops out");

#if D_CFG_IS_ON(D_CFG_FILE_PATH_NORMALIZE_DOTDOT)
    d_path_normalize("/path/to/../file.txt", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("dotdot", buf, D_TEST_FILE_EMIT_SEP "path" D_TEST_FILE_EMIT_SEP "file.txt",
                                      ".. cancels the component before it");

    d_path_normalize("/a/b/../c", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("dotdot_mid", buf, D_TEST_FILE_EMIT_SEP "a" D_TEST_FILE_EMIT_SEP "c",
                                      ".. resolves mid-path");

    // a .. with nothing above it to cancel must be KEPT: "../b" is a real
    // path and dropping the .. would silently change which file it names
    d_path_normalize("../../a", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("dotdot_leading", buf, ".." D_TEST_FILE_EMIT_SEP ".." D_TEST_FILE_EMIT_SEP "a",
                                      "a .. with nothing to cancel is kept");

    d_path_normalize("a/../../b", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("dotdot_escapes", buf, ".." D_TEST_FILE_EMIT_SEP "b",
                                      "the surplus .. survives the cancellation");

    // POSIX: the root's parent is the root
    d_path_normalize("/../a", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("dotdot_at_root", buf, D_TEST_FILE_EMIT_SEP "a",
                                      "the root has no parent to climb to");

    d_path_normalize("a/b/../..", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("cancels_out", buf, ".",
                                      "a path that fully cancels is .");
#endif

#if D_CFG_IS_ON(D_CFG_FILE_PATH_STRIP_TRAILING_SEP)
    d_path_normalize("a/b/", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("strip_trailing", buf, "a" D_TEST_FILE_EMIT_SEP "b",
                                      "a trailing separator is punctuation, and drops");

    d_path_normalize("/", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("keep_root", buf, D_TEST_FILE_EMIT_SEP,
                                      "...but the root's separator IS the path");
#endif

    d_path_normalize("", buf, sizeof(buf));
    items[idx++] = D_ASSERT_STR_EQUAL("empty", buf, ".",
                                      "an empty path normalizes to .");

    #if D_FILE_VALIDATE_IS_ACTIVE
    items[idx++] = D_ASSERT_NULL("null",
                                 d_path_normalize(NULL, buf, sizeof(buf)),
                                 "d_path_normalize rejects NULL");
    #endif

    return d_tests_file_group("d_path_normalize", items, idx);
}


/*
d_tests_file_path_all
  Runs every file_path test.
*/
struct d_test_object*
d_tests_file_path_all
(
    void
)
{
    struct d_test_object* items[8];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_dirname();
    items[idx++] = d_tests_file_basename();
    items[idx++] = d_tests_file_extension();
    items[idx++] = d_tests_file_path_join();
    items[idx++] = d_tests_file_path_absolute();
    items[idx++] = d_tests_file_path_normalize();

    return d_tests_file_group("IX. file_path (lexical; no filesystem)",
                              items,
                              idx);
}
