/*******************************************************************************
* djinterp [test]                                         file_path_tests_sa.cpp
*
*   Unit tests for djinterp::path (inc/djinterp/core/fs/file_path.hpp). One file for one
* class, so the test mirrors the thing it tests -- exactly as the c/fs suite is
* one source per module.
*
*   THREE RULES, the first two inherited from the file_tests_sa suite and the
* third specific to the C++ layer.
*
*   1. A test that cannot run is OMITTED, never passed.
*      djinterp::path is portable C++98 -> C++26, and a move constructor is a
*      C++11 addition. On C++98 the move assertions are ABSENT, not a green
*      "skipped" -- the group simply has fewer, and the group name says why.
*      The old dfile suite reported PASS for symlink code that never executed;
*      this suite says nothing about what it did not run.
*
*   2. Capability is asked at COMPILE time, not probed at runtime.
*      D_MOVE_ENABLED, D_FILE_PATH_UNDERSTANDS_WINDOWS and D_INTERNAL_PATH_-
*      THROW are compile-time facts the language and the config already
*      resolved. The tests #if on them; there is nothing to discover at run
*      time, and a runtime probe for a compile-time fact is a test of the
*      probe.
*
*   3. The tier rule is DEMONSTRATED, not asserted in prose.
*      This is one source. It is meant to be built on every standard from
*      C++98 to C++23 -- and the higher tiers only ADD assertions (the move
*      group), they never change one. If a tier needs different source, the
*      additive-never-divergent contract has already broken, and no assertion
*      inside the file can catch that; building the same file everywhere can.
*
*   WHY THIS FILE CARRIES ITS OWN HARNESS.
*      The C test framework was not C++-consumable, and it is not yet
*   djinterp::path was the framework's first C++ consumer of dstring and found
*   the restrict / D_INLINE / extern "C" blockers there. This test carries the
*   same lesson one layer up: the d_test_object harness in Section 0 is
*   self-contained, so no C header has to be made C++-consumable for it to
*   build. It provides d_test_object_new_leaf / _new_interior, the D_ASSERT_*
*   macros and the derived-count group helper directly.
*
*   It also carries D_TEST_PATH_STR_EQUAL over a FUNCTION rather than the
*   framework's D_ASSERT_STR_EQUAL macro, for the reason SHORTCOMINGS 2.2 gives:
*   the macro's `(_str1) && (_str2)` tests an array's address when handed a
*   string literal, so `-Waddress` fires on every call. A function parameter
*   decays the array to a pointer and the NULL check is real. Same shape as the
*   fs suite's D_TEST_FILE_STR_EQUAL; it collapses into the framework's macro
*   the day that one is fixed.
*
*
* path:      /tests/djinterp/fs/file_path_tests_sa.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    STANDALONE HARNESS         (self-contained; no framework dependency)
      --------------------------------------------------------------------
      1.    d_test_object + constructors
      2.    D_ASSERT_* / D_TEST_PATH_STR_EQUAL
      3.    d_tests_path_group (derived count)
      4.    printer + counters

I.    CONSTRUCTION / VALIDITY    empty, from-cstr, NULL, c_str, bool, size
      --------------------------------------------------------------------

II.   DECOMPOSITION              parent, filename, stem, extension
      ------------------------------------------------------------

III.  COMPOSITION                append, operator/=, operator/, absolute-wins
      -----------------------------------------------------------------------

IV.   INSPECTION                 is_absolute / is_relative / root_length
      ------------------------------------------------------------------

V.    CANONICALIZATION           normalized (lexical)
      -----------------------------------------------

VI.   COMPARISON                 operator== / != (textual)
      ----------------------------------------------------

VII.  VALUE SEMANTICS            copy independence, swap, self-assign, MOVE
      ---------------------------------------------------------------------

VIII. FAILURE MODEL              contagious invalid, the throw carve-out
      ------------------------------------------------------------------

      master runner + main
*/

// std
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// djinterp
#include "../../../inc/djinterp/core/fs/file_path.hpp"


// ===========================================================================
// 0.   STANDALONE HARNESS
// ===========================================================================

// D_TEST_PATH_MAX_ASSERTS
//   constant: the largest number of assertions any one group may hold. Groups
// build into a local array of this size and report the count they actually
// filled, rather than declaring a count up front and hoping it matches.
#define D_TEST_PATH_MAX_ASSERTS 32


// D_TEST_PATH_SEP
//   macro: the canonical output separator for the grammar this build parses.
// The c/fs path module CANONICALIZES the separator on any operation that
// REBUILDS a path -- d_path_join and d_path_normalize -- so their output is
// "/" under POSIX syntax and "\" under Windows syntax, even for "/"-form
// input. Decomposition (d_path_dirname etc.) merely TRUNCATES and so preserves the
// input's bytes; those assertions need no separator awareness. Expected values
// for the rebuilding operations concatenate this at compile time (adjacent
// string literals), so one source is correct under both grammars rather than
// silently failing under one -- which is the whole reason to build the config
// matrix at all.
#if D_FILE_PATH_UNDERSTANDS_WINDOWS
#  define D_TEST_PATH_SEP "\\"
#else
#  define D_TEST_PATH_SEP "/"
#endif


// --- 0.1  d_test_object ---

// d_test_object
//   struct: a node in the result tree -- a leaf (assertion) or an interior
// group. The layout matches the C harness's, so the bodies below are
// source-compatible with the real framework once it is C++-consumable.
struct d_test_object
{
    bool                   is_leaf;
    const char*            name;
    const char*            message;
    bool                   result;
    size_t                 count;
    struct d_test_object** elements;
};


// d_test_object_new_leaf
//   function: an assertion node carrying its pass/fail result.
static struct d_test_object*
d_test_object_new_leaf
(
    const char* _name,
    const char* _message,
    bool        _result
)
{
    struct d_test_object* obj;

    obj = (struct d_test_object*)malloc(sizeof(struct d_test_object));

    if (!obj)
    {
        return 0;
    }

    obj->is_leaf  = true;
    obj->name     = _name;
    obj->message  = _message;
    obj->result   = _result;
    obj->count    = 0;
    obj->elements = 0;

    return obj;
}


// d_test_object_new_interior
//   function: a group node with room for _count children.
static struct d_test_object*
d_test_object_new_interior
(
    const char* _name,
    size_t      _count
)
{
    struct d_test_object* obj;

    obj = (struct d_test_object*)malloc(sizeof(struct d_test_object));

    if (!obj)
    {
        return 0;
    }

    obj->is_leaf  = false;
    obj->name     = _name;
    obj->message  = 0;
    obj->result   = true;
    obj->count    = _count;
    obj->elements = 0;

    if (_count > 0)
    {
        obj->elements =
            (struct d_test_object**)calloc(_count, sizeof(struct d_test_object*));

        if (!obj->elements)
        {
            free(obj);

            return 0;
        }
    }

    return obj;
}


// d_test_object_free
//   function: release a tree. Present so the whole run is leak-clean under a
// sanitizer -- the same standard the c/fs suite is held to.
static void
d_test_object_free
(
    struct d_test_object* _obj
)
{
    size_t idx;

    if (!_obj)
    {
        return;
    }

    for (idx = 0; idx < _obj->count; ++idx)
    {
        d_test_object_free(_obj->elements[idx]);
    }

    free(_obj->elements);
    free(_obj);

    return;
}


// --- 0.2  Assertion macros ---

// D_ASSERT_TRUE / _FALSE / _NULL / _NOT_NULL / _EQUAL
//   macro: leaf constructors, in the shape the C harness used.
#define D_ASSERT_TRUE(_name, _cond, _msg)                                    \
    d_test_object_new_leaf(_name, _msg, (_cond))
#define D_ASSERT_FALSE(_name, _cond, _msg)                                   \
    d_test_object_new_leaf(_name, _msg, !(_cond))
#define D_ASSERT_NULL(_name, _ptr, _msg)                                     \
    d_test_object_new_leaf(_name, _msg, (_ptr) == 0)
#define D_ASSERT_NOT_NULL(_name, _ptr, _msg)                                 \
    d_test_object_new_leaf(_name, _msg, (_ptr) != 0)
#define D_ASSERT_EQUAL(_name, _a, _b, _msg)                                  \
    d_test_object_new_leaf(_name, _msg, (_a) == (_b))


// d_tests_path_streq
//   function: string equality that is safe on array arguments. A macro doing
// `(_a) && (_b)` tests the ADDRESS of a string literal (always true, and
// -Waddress says so); a parameter decays the array and the NULL check is real.
static bool
d_tests_path_streq
(
    const char* _a,
    const char* _b
)
{
    return _a && _b && (strcmp(_a, _b) == 0);
}

// D_TEST_PATH_STR_EQUAL
//   macro: the string-equal assertion, over the function above.
#define D_TEST_PATH_STR_EQUAL(_name, _a, _b, _msg)                           \
    d_test_object_new_leaf(_name, _msg, d_tests_path_streq(_a, _b))


// --- 0.3  Group helper (derived count) ---

// d_tests_path_group
//   function: build an interior node from assertions the caller already
// created, taking the count from what the caller actually filled.
//   The count is DERIVED, not asserted -- so a conditionally-omitted assertion
// simply makes the group smaller, and there is no second number to drift out
// of sync. This is the mechanism the file_tests_sa suite introduced to delete
// a whole bug class; it is reproduced here for the same reason.
static struct d_test_object*
d_tests_path_group
(
    const char*            _name,
    struct d_test_object** _items,
    size_t                 _count
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior(_name, _count);

    if (!group)
    {
        return 0;
    }

    for (idx = 0; idx < _count; ++idx)
    {
        group->elements[idx] = _items[idx];
    }

    return group;
}


// --- 0.4  Printer + counters ---

// d_tests_path_counter
//   struct: running pass/fail tally for the walk below.
struct d_tests_path_counter
{
    size_t passed;
    size_t failed;
};


// d_tests_path_print
//   function: walk the tree depth-first, printing each assertion's result and
// each group's header, and accumulating the tally.
static void
d_tests_path_print
(
    const struct d_test_object*  _obj,
    size_t                       _depth,
    struct d_tests_path_counter* _counter
)
{
    size_t idx;
    size_t pad;

    if (!_obj)
    {
        return;
    }

    for (pad = 0; pad < _depth * 2; ++pad)
    {
        (void)fputc(' ', stdout);
    }

    if (_obj->is_leaf)
    {
        if (_obj->result)
        {
            ++_counter->passed;
        }
        else
        {
            ++_counter->failed;
        }

        (void)printf("[%s] %-24s %s\n",
                     _obj->result ? "PASS" : "FAIL",
                     _obj->name,
                     _obj->message ? _obj->message : "");
    }
    else
    {
        (void)printf("%s\n", _obj->name);

        for (idx = 0; idx < _obj->count; ++idx)
        {
            d_tests_path_print(_obj->elements[idx], _depth + 1, _counter);
        }
    }

    return;
}


// ===========================================================================
// I.   CONSTRUCTION / VALIDITY
// ===========================================================================

/*
d_tests_path_construct
  Tests construction, the validity channel, and the observers.
  Tests the following:
  - an empty path is valid, and empty
  - a path from a C string carries its text
  - a NULL argument yields an INVALID path -- not an empty one
  - an invalid path reads as "", never NULL
  - operator bool tracks valid()
*/
static struct d_test_object*
d_tests_path_construct
(
    void
)
{
    struct d_test_object* items[D_TEST_PATH_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    items[idx++] = D_ASSERT_TRUE("empty_valid",
                                 djinterp::path().valid(),
                                 "an empty path is valid");
    items[idx++] = D_ASSERT_TRUE("empty_is_empty",
                                 djinterp::path().empty(),
                                 "...and empty");
    items[idx++] = D_TEST_PATH_STR_EQUAL("from_cstr",
                                         djinterp::path("/usr/bin").c_str(),
                                         "/usr/bin",
                                         "a path carries the text it was built from");
    items[idx++] = D_ASSERT_EQUAL("size",
                                  djinterp::path("hello").size(),
                                  (size_t)5,
                                  "size is the byte count, excluding the terminator");

    // a NULL path is not an empty path. valid() is what tells "" from "it
    // failed"; empty() reads true for both, and conflating them is how a
    // caller ends up operating on "" believing it has something.
    {
        djinterp::path bad((const char*)0);

        items[idx++] = D_ASSERT_FALSE("null_invalid",
                                      bad.valid(),
                                      "a NULL argument yields an INVALID path");
        items[idx++] = D_ASSERT_TRUE("null_reads_empty",
                                     bad.empty(),
                                     "an invalid path is also empty -- so ask valid(), not empty()");
        items[idx++] = D_TEST_PATH_STR_EQUAL("null_cstr_safe",
                                             bad.c_str(),
                                             "",
                                             "an invalid path reads as \"\", never NULL");
        items[idx++] = D_ASSERT_FALSE("bool_invalid",
                                      (bool)bad,
                                      "operator bool is false for an invalid path");
    }

    items[idx++] = D_ASSERT_TRUE("bool_valid",
                                 (bool)djinterp::path("x"),
                                 "...and true for a valid one");

    return d_tests_path_group("construction / validity", items, idx);
}


// ===========================================================================
// II.  DECOMPOSITION
// ===========================================================================

/*
d_tests_path_decompose
  Tests parent / filename / stem / extension.
  Every one delegates to c/fs/file_path, so this group is really asserting that
the delegation is faithful: if parent() ever disagrees with d_path_dirname, one of
them is a bug. The expected values are exactly those the file_tests_sa path
suite pins for the C functions.
*/
static struct d_test_object*
d_tests_path_decompose
(
    void
)
{
    struct d_test_object* items[D_TEST_PATH_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    items[idx++] = D_ASSERT_TRUE("parent_nested",
                                 djinterp::path("/path/to/file.txt").parent()
                                     == djinterp::path("/path/to"),
                                 "parent() is the directory component");
    items[idx++] = D_ASSERT_TRUE("parent_bare",
                                 djinterp::path("file.txt").parent()
                                     == djinterp::path("."),
                                 "a bare filename's parent is .");
    items[idx++] = D_ASSERT_TRUE("filename_trailing_sep",
                                 djinterp::path("a/b/").filename()
                                     == djinterp::path("b"),
                                 "trailing separators are ignored");
    items[idx++] = D_ASSERT_TRUE("filename_root",
                                 djinterp::path("/").filename()
                                     == djinterp::path("/"),
                                 "the root names itself");
    items[idx++] = D_ASSERT_TRUE("stem_double",
                                 djinterp::path("archive.tar.gz").stem()
                                     == djinterp::path("archive.tar"),
                                 "stem drops only the last extension");
    items[idx++] = D_ASSERT_TRUE("stem_hidden",
                                 djinterp::path(".bashrc").stem()
                                     == djinterp::path(".bashrc"),
                                 "a hidden file's stem is its whole name");

    items[idx++] = D_TEST_PATH_STR_EQUAL("ext_simple",
                                         djinterp::path("file.txt").extension(),
                                         ".txt",
                                         "extension() includes the dot");
    items[idx++] = D_TEST_PATH_STR_EQUAL("ext_double",
                                         djinterp::path("archive.tar.gz").extension(),
                                         ".gz",
                                         "only the last extension is returned");

    // path maps d_path_extension's NULL to "" -- a caller comparing to ".txt"
    // should not have to null-check first
    items[idx++] = D_TEST_PATH_STR_EQUAL("ext_none",
                                         djinterp::path("filename").extension(),
                                         "",
                                         "no extension reads as \"\", not NULL");
    items[idx++] = D_TEST_PATH_STR_EQUAL("ext_hidden",
                                         djinterp::path(".bashrc").extension(),
                                         "",
                                         "a leading dot hides a file, it does not type it");

    // every decomposition of an invalid path is invalid -- the failure
    // propagates rather than being dropped
    {
        djinterp::path bad((const char*)0);

        items[idx++] = D_ASSERT_FALSE("of_invalid",
                                      bad.parent().valid(),
                                      "a decomposition of an invalid path is invalid");
    }

    return d_tests_path_group("decomposition", items, idx);
}


// ===========================================================================
// III. COMPOSITION
// ===========================================================================

/*
d_tests_path_compose
  Tests append / operator/= / operator/.
  Tests the following:
  - exactly one separator, however many were supplied
  - operator/= mutates in place; operator/ yields a third path
  - an absolute second component's fate is the config's decision, not this
    layer's
  - joining with an invalid operand yields an invalid path
*/
static struct d_test_object*
d_tests_path_compose
(
    void
)
{
    struct d_test_object* items[D_TEST_PATH_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    items[idx++] = D_ASSERT_TRUE("join_simple",
                                 (djinterp::path("path/to") / djinterp::path("file.txt"))
                                     == djinterp::path("path/to" D_TEST_PATH_SEP "file.txt"),
                                 "operator/ inserts one separator");
    items[idx++] = D_ASSERT_TRUE("join_dedup_sep",
                                 (djinterp::path("a/") / djinterp::path("b"))
                                     == djinterp::path("a" D_TEST_PATH_SEP "b"),
                                 "one separator, however many were given");

    {
        djinterp::path p("a");

        p /= djinterp::path("b");
        items[idx++] = D_ASSERT_TRUE("append_mutates",
                                     p == djinterp::path("a" D_TEST_PATH_SEP "b"),
                                     "operator/= appends in place");
    }

    // an absolute second component -- the fate is D_CFG_FILE_PATH_JOIN_-
    // ABSOLUTE_WINS's, resolved in the C layer, and NOT re-decided here
#if D_CFG_IS_ON(D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS)
    items[idx++] = D_ASSERT_TRUE("absolute_wins",
                                 (djinterp::path("/base") / djinterp::path("/etc/passwd"))
                                     == djinterp::path("/etc/passwd"),
                                 "an absolute second component replaces the first");
#else
    items[idx++] = D_ASSERT_TRUE("absolute_concat",
                                 (djinterp::path("/base") / djinterp::path("/etc/passwd"))
                                     == djinterp::path("/base/etc/passwd"),
                                 "JOIN_ABSOLUTE_WINS=0 concatenates textually");
#endif

    // an invalid operand on either side poisons the result
    {
        djinterp::path bad((const char*)0);

        items[idx++] = D_ASSERT_FALSE("onto_invalid",
                                      (bad / djinterp::path("x")).valid(),
                                      "appending onto an invalid path is invalid");
        items[idx++] = D_ASSERT_FALSE("with_invalid",
                                      (djinterp::path("a") / bad).valid(),
                                      "appending an invalid path is invalid");
    }

    return d_tests_path_group("composition", items, idx);
}


// ===========================================================================
// IV.  INSPECTION
// ===========================================================================

/*
d_tests_path_inspect
  Tests is_absolute / is_relative / root_length.
  The assertions branch on D_FILE_PATH_UNDERSTANDS_WINDOWS rather than on the
host, because that is the whole point of the split: a Linux build asked to
parse Windows paths must be TESTED parsing Windows paths, and the platform
cannot tell you whether it was. The values mirror the file_tests_sa path suite.
*/
static struct d_test_object*
d_tests_path_inspect
(
    void
)
{
    struct d_test_object* items[D_TEST_PATH_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

#if D_FILE_PATH_UNDERSTANDS_WINDOWS
    items[idx++] = D_ASSERT_TRUE("win_drive",
                                 djinterp::path("C:\\path\\to\\file").is_absolute(),
                                 "a drive-anchored path is absolute");
    items[idx++] = D_ASSERT_TRUE("win_fwdslash",
                                 djinterp::path("C:/path").is_absolute(),
                                 "Win32 accepts / too, so this build must");

    // the one every hand-rolled parser gets wrong: "C:x" is relative to the
    // per-drive cursor, only "C:\x" is anchored
    items[idx++] = D_ASSERT_FALSE("win_drive_relative",
                                  djinterp::path("C:x").is_absolute(),
                                  "C:x is drive-RELATIVE, not absolute");
    items[idx++] = D_ASSERT_EQUAL("win_drive_rel_root",
                                  djinterp::path("C:x").root_length(), (size_t)2,
                                  "C: is a root to climb from, but not absolute");
    items[idx++] = D_ASSERT_EQUAL("win_drive_abs_root",
                                  djinterp::path("C:\\x").root_length(), (size_t)3,
                                  "C:\\ includes the separator");
    items[idx++] = D_ASSERT_TRUE("win_unc",
                                 djinterp::path("\\\\server\\share\\f").is_absolute(),
                                 "a UNC path is absolute");
#else
    items[idx++] = D_ASSERT_TRUE("posix_root",
                                 djinterp::path("/path/to/file").is_absolute(),
                                 "a leading / is absolute");
    items[idx++] = D_ASSERT_FALSE("posix_relative",
                                  djinterp::path("path/to/file").is_absolute(),
                                  "no leading / is relative");
    items[idx++] = D_ASSERT_TRUE("is_relative",
                                 djinterp::path("path").is_relative(),
                                 "is_relative is the complement of is_absolute");
    items[idx++] = D_ASSERT_EQUAL("posix_double_slash",
                                  djinterp::path("//net/share").root_length(), (size_t)2,
                                  "// is a root POSIX reserves");
    items[idx++] = D_ASSERT_EQUAL("posix_triple_slash",
                                  djinterp::path("///x").root_length(), (size_t)1,
                                  "/// is just the root");

    // under POSIX syntax a backslash is an ordinary filename byte
    items[idx++] = D_ASSERT_FALSE("posix_backslash_is_data",
                                  djinterp::path("C:\\path").is_absolute(),
                                  "under POSIX syntax, C:\\path is one relative name");
#endif

    // an invalid path is not absolute -- true on either grammar
    items[idx++] = D_ASSERT_FALSE("invalid_not_absolute",
                                  djinterp::path((const char*)0).is_absolute(),
                                  "an invalid path is not absolute");
    items[idx++] = D_ASSERT_FALSE("invalid_not_relative",
                                  djinterp::path((const char*)0).is_relative(),
                                  "...nor relative -- is_relative needs a valid path");

    return d_tests_path_group(
               D_FILE_PATH_UNDERSTANDS_WINDOWS ?
                   "inspection (Windows grammar)" :
                   "inspection (POSIX grammar)",
               items,
               idx);
}


// ===========================================================================
// V.   CANONICALIZATION
// ===========================================================================

/*
d_tests_path_normalize
  Tests normalized().
  Tests the following:
  - separator runs and "." drop
  - ".." cancels the component before it, and a surplus ".." is KEPT
  - the result is lexical, and the group name says so
  Each behaviour is gated on the knob that provides it, so a build that turns
one off omits its assertions rather than failing them.
*/
static struct d_test_object*
d_tests_path_normalize
(
    void
)
{
    struct d_test_object* items[D_TEST_PATH_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

#if D_CFG_IS_ON(D_CFG_FILE_PATH_COLLAPSE_SEPARATORS)
    items[idx++] = D_ASSERT_TRUE("collapse",
                                 djinterp::path("a//b").normalized()
                                     == djinterp::path("a" D_TEST_PATH_SEP "b"),
                                 "separator runs collapse");
#endif

    items[idx++] = D_ASSERT_TRUE("dot",
                                 djinterp::path("./a/./b").normalized()
                                     == djinterp::path("a" D_TEST_PATH_SEP "b"),
                                 ". means stay here, so it drops out");

#if D_CFG_IS_ON(D_CFG_FILE_PATH_NORMALIZE_DOTDOT)
    items[idx++] = D_ASSERT_TRUE("dotdot_mid",
                                 djinterp::path("/a/b/../c").normalized()
                                     == djinterp::path(D_TEST_PATH_SEP "a" D_TEST_PATH_SEP "c"),
                                 ".. cancels the component before it");
    // a .. with nothing above it to cancel must be KEPT: "../b" is a real path
    // and dropping the .. would silently change which file it names
    items[idx++] = D_ASSERT_TRUE("dotdot_leading",
                                 djinterp::path("../../a").normalized()
                                     == djinterp::path(".." D_TEST_PATH_SEP ".." D_TEST_PATH_SEP "a"),
                                 "a .. with nothing to cancel is kept");
    items[idx++] = D_ASSERT_TRUE("cancels_out",
                                 djinterp::path("a/b/../..").normalized()
                                     == djinterp::path("."),
                                 "a path that fully cancels is .");
    items[idx++] = D_ASSERT_TRUE("dotdot_at_root",
                                 djinterp::path("/../a").normalized()
                                     == djinterp::path(D_TEST_PATH_SEP "a"),
                                 "the root has no parent to climb to");

    // LEXICAL: given /x/link -> elsewhere, this resolves the TEXT ("/x"), not
    // what the kernel would (the link target). That is the distinction the
    // group name names, and it is why d_path_resolve exists in file_dir.
    items[idx++] = D_ASSERT_TRUE("lexical_not_filesystem",
                                 djinterp::path("/x/link/..").normalized()
                                     == djinterp::path(D_TEST_PATH_SEP "x"),
                                 "normalized() is textual -- it cannot see a symlink");
#endif

#if D_CFG_IS_ON(D_CFG_FILE_PATH_STRIP_TRAILING_SEP)
    items[idx++] = D_ASSERT_TRUE("strip_trailing",
                                 djinterp::path("a/b/").normalized()
                                     == djinterp::path("a" D_TEST_PATH_SEP "b"),
                                 "a trailing separator is punctuation, and drops");
    items[idx++] = D_ASSERT_TRUE("keep_root",
                                 djinterp::path("/").normalized()
                                     == djinterp::path(D_TEST_PATH_SEP),
                                 "...but the root's separator IS the path");
#endif

    items[idx++] = D_ASSERT_TRUE("empty",
                                 djinterp::path("").normalized()
                                     == djinterp::path("."),
                                 "an empty path normalizes to .");

    // an invalid path normalizes to an invalid path
    items[idx++] = D_ASSERT_FALSE("of_invalid",
                                  djinterp::path((const char*)0).normalized().valid(),
                                  "normalizing an invalid path is invalid");

    return d_tests_path_group("canonicalization (lexical)", items, idx);
}


// ===========================================================================
// VI.  COMPARISON
// ===========================================================================

/*
d_tests_path_compare
  Tests operator== / operator!=.
  Comparison is TEXTUAL by design: "a/b" and "a//b" name the same file and
compare UNEQUAL, because deciding otherwise means normalizing on every
comparison -- and normalizing is lexical, so it would still be wrong across a
symlink. Compare normalized() when that is what you meant.
*/
static struct d_test_object*
d_tests_path_compare
(
    void
)
{
    struct d_test_object* items[D_TEST_PATH_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    items[idx++] = D_ASSERT_TRUE("equal_text",
                                 djinterp::path("a/b") == djinterp::path("a/b"),
                                 "equal text compares equal");
    items[idx++] = D_ASSERT_FALSE("unequal_text",
                                  djinterp::path("a/b") == djinterp::path("a//b"),
                                  "same file, different text -> unequal (textual)");
    items[idx++] = D_ASSERT_TRUE("equal_when_normalized",
                                 djinterp::path("a/b").normalized()
                                     == djinterp::path("a//b").normalized(),
                                 "...and equal once you normalize, if that is what you meant");
    items[idx++] = D_ASSERT_TRUE("not_equal_op",
                                 djinterp::path("a") != djinterp::path("b"),
                                 "operator!= is the negation");

    // two invalid paths are equal; an invalid path equals nothing else,
    // including an empty one
    {
        djinterp::path a((const char*)0);
        djinterp::path b((const char*)0);

        items[idx++] = D_ASSERT_TRUE("two_invalids_equal",
                                     a == b,
                                     "two invalid paths are equal");
        items[idx++] = D_ASSERT_FALSE("invalid_ne_empty",
                                      a == djinterp::path(),
                                      "an invalid path does not equal an empty one");
    }

    return d_tests_path_group("comparison (textual)", items, idx);
}


// ===========================================================================
// VII. VALUE SEMANTICS
// ===========================================================================

/*
d_tests_path_value
  Tests copy independence, swap, self-assignment, and -- on C++11 and later --
move.
  The move assertions are the tier rule made concrete: they are ADDED here,
under #if D_MOVE_ENABLED, and they are ABSENT on C++98. The group is smaller
on the older standard; nothing in it behaves differently. A C++98 caller writes
copies, and that source compiles on every later tier too.
*/
static struct d_test_object*
d_tests_path_value
(
    void
)
{
    struct d_test_object* items[D_TEST_PATH_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    // a copy is deep: mutating one must not touch the other
    {
        djinterp::path a("orig");
        djinterp::path b(a);

        b /= djinterp::path("x");
        items[idx++] = D_ASSERT_TRUE("copy_independent",
                                     (a == djinterp::path("orig")) &&
                                     (b == djinterp::path("orig" D_TEST_PATH_SEP "x")),
                                     "a copy is deep -- two paths never share a buffer");
    }

    // copy-assign, then self-assign: copy-and-swap makes both safe
    {
        djinterp::path a("dst");
        djinterp::path b("src");

        a = b;
        items[idx++] = D_ASSERT_TRUE("copy_assign",
                                     a == djinterp::path("src"),
                                     "copy-assign replaces the contents");
    }
    {
        djinterp::path a("keep");

        a = a;
        items[idx++] = D_ASSERT_TRUE("self_assign",
                                     a == djinterp::path("keep"),
                                     "self-assignment is a no-op, not a free");
    }

    // swap exchanges without allocating
    {
        djinterp::path a("A");
        djinterp::path b("B");

        swap(a, b);
        items[idx++] = D_ASSERT_TRUE("swap",
                                     (a == djinterp::path("B")) &&
                                     (b == djinterp::path("A")),
                                     "swap exchanges two paths");
    }

#if D_MOVE_ENABLED
    // ADDITIVE: present only where the language has moves. The moved-to path
    // takes the value; the moved-from is left INVALID (its buffer was handed
    // over), not dangling -- which is the only safe post-move state.
    {
        djinterp::path a("moveme");
        djinterp::path b(static_cast<djinterp::path&&>(a));

        items[idx++] = D_ASSERT_TRUE("move_ctor",
                                     b == djinterp::path("moveme"),
                                     "the move constructor takes the value");
        items[idx++] = D_ASSERT_FALSE("move_ctor_source",
                                      a.valid(),
                                      "a moved-from path is left invalid, not dangling");
    }
    {
        djinterp::path a("src");
        djinterp::path b("dst");

        b = static_cast<djinterp::path&&>(a);
        items[idx++] = D_ASSERT_TRUE("move_assign",
                                     b == djinterp::path("src"),
                                     "move-assign takes the value");
        items[idx++] = D_ASSERT_FALSE("move_assign_source",
                                      a.valid(),
                                      "...and leaves the source invalid");
    }
#endif

    return d_tests_path_group(
               D_MOVE_ENABLED ?
                   "value semantics (+ C++11 move)" :
                   "value semantics (C++98: copy only, move omitted)",
               items,
               idx);
}


// ===========================================================================
// VIII. FAILURE MODEL
// ===========================================================================

/*
d_tests_path_failure
  Tests that an invalid path is contagious, and -- where the build throws --
that the throw is reserved for resource failures only.
  The throwing assertion is present only under #if D_INTERNAL_PATH_THROW. In a
build with throwing off it is OMITTED, not faked: the group is smaller and the
name says the model is off.
*/
static struct d_test_object*
d_tests_path_failure
(
    void
)
{
    struct d_test_object* items[D_TEST_PATH_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    // contagion, end to end: a chain of operations on an invalid path stays
    // invalid, so the failure arrives wherever the caller finally checks
    {
        djinterp::path bad((const char*)0);

        items[idx++] = D_ASSERT_FALSE("contagious",
                                      bad.parent().filename().normalized().valid(),
                                      "an invalid path stays invalid through any chain");
    }

#if D_INTERNAL_PATH_THROW
    // THE carve-out (SHORTCOMINGS 4.3). With throwing enabled, path(NULL) must
    // still NOT throw: a NULL argument is a caller bug, not memory exhaustion,
    // and bad_alloc would be a lie about what went wrong. Only a genuine
    // allocation failure throws. The first throwing build aborted on exactly
    // this, when the invalid-path idiom went through the throwing constructor.
    {
        bool threw = false;

        try
        {
            djinterp::path bad((const char*)0);
            (void)bad;
        }
        catch (...)
        {
            threw = true;
        }

        items[idx++] = D_ASSERT_FALSE("null_no_throw",
                                      threw,
                                      "path(NULL) does not throw even with D_CFG_PATH_THROW on");
    }

    // the sibling of the carve-out above, and the one this suite's throwing
    // build actually caught: COPYING an invalid path must not throw either. An
    // already-invalid source is not a resource this ran out of, and operator/
    // copies its left operand -- so a throw here aborts `invalid / anything`.
    {
        bool threw = false;

        try
        {
            djinterp::path bad((const char*)0);
            djinterp::path copy(bad);

            (void)copy;
        }
        catch (...)
        {
            threw = true;
        }

        items[idx++] = D_ASSERT_FALSE("copy_invalid_no_throw",
                                      threw,
                                      "copying an invalid path does not throw -- it is not a resource failure");
    }
#endif

    return d_tests_path_group(
               D_INTERNAL_PATH_THROW ?
                   "failure model (throwing enabled)" :
                   "failure model (error-code only; throw off)",
               items,
               idx);
}


// ===========================================================================
//      MASTER RUNNER
// ===========================================================================

#if (D_INTERNAL_PATH_HAS_SPACESHIP == 1)
/*
d_tests_path_ordering
  operator<=> (C++20) supplies relational ordering, consistent with ==. Present
only where the language has three-way comparison -- the C++20 rung of the
ladder, ADDING what the lower tiers neither have nor need. This is the tier rule
at the top of the range, exactly as the move group is at C++11.
*/
static struct d_test_object*
d_tests_path_ordering
(
    void
)
{
    struct d_test_object* items[D_TEST_PATH_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    items[idx++] = D_ASSERT_TRUE("less",
                                 djinterp::path("a") < djinterp::path("b"),
                                 "byte-lexicographic <");
    items[idx++] = D_ASSERT_TRUE("greater",
                                 djinterp::path("b") > djinterp::path("a"),
                                 "...and >");
    items[idx++] = D_ASSERT_TRUE("ge_on_equal",
                                 djinterp::path("a") >= djinterp::path("a"),
                                 ">= holds on equal paths");
    items[idx++] = D_ASSERT_TRUE("spaceship_equal",
                                 (djinterp::path("x") <=> djinterp::path("x"))
                                     == std::strong_ordering::equal,
                                 "equal paths are <=> equivalent");
    items[idx++] = D_ASSERT_TRUE("consistent_with_eq",
                                 (djinterp::path("x") == djinterp::path("x")) ==
                                 ((djinterp::path("x") <=> djinterp::path("x"))
                                     == std::strong_ordering::equal),
                                 "a <=> b is equal exactly when a == b");
    items[idx++] = D_ASSERT_TRUE("invalid_orders_first",
                                 djinterp::path((const char*)0) < djinterp::path("a"),
                                 "an invalid path orders before any valid one");

    return d_tests_path_group("ordering (C++20 <=>)", items, idx);
}
#endif  // D_INTERNAL_PATH_HAS_SPACESHIP


/*
d_tests_path_all
  Builds the whole result tree. The count is derived from what actually ran, so
a group omitted by configuration contributes nothing -- not an empty group, not
a green "skipped" -- exactly as the c/fs master runner does.
*/
static struct d_test_object*
d_tests_path_all
(
    void
)
{
    struct d_test_object* items[9];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_path_construct();
    items[idx++] = d_tests_path_decompose();
    items[idx++] = d_tests_path_compose();
    items[idx++] = d_tests_path_inspect();
    items[idx++] = d_tests_path_normalize();
    items[idx++] = d_tests_path_compare();
    items[idx++] = d_tests_path_value();
    items[idx++] = d_tests_path_failure();
#if (D_INTERNAL_PATH_HAS_SPACESHIP == 1)
    items[idx++] = d_tests_path_ordering();
#endif

    return d_tests_path_group("djinterp::path (lexical; no filesystem)",
                              items,
                              idx);
}


int
main
(
    void
)
{
    struct d_test_object*       root;
    struct d_tests_path_counter counter;

    counter.passed = 0;
    counter.failed = 0;

    (void)printf("=============================================="
                 "==================================\n");
    (void)printf(" djinterp::path -- standalone test  "
                 "(__cplusplus=%ld, move=%d, win_syntax=%d, throw=%d)\n",
                 (long)__cplusplus,
                 (int)D_MOVE_ENABLED,
                 (int)D_FILE_PATH_UNDERSTANDS_WINDOWS,
                 (int)D_INTERNAL_PATH_THROW);
    (void)printf("=============================================="
                 "==================================\n\n");

    root = d_tests_path_all();

    if (!root)
    {
        (void)fprintf(stderr, "out of memory building the test tree\n");

        return 2;
    }

    d_tests_path_print(root, 0, &counter);
    d_test_object_free(root);

    (void)printf("\npassed: %lu  failed: %lu\n",
                 (unsigned long)counter.passed,
                 (unsigned long)counter.failed);

    return (counter.failed == 0) ? 0 : 1;
}
