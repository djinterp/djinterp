/*******************************************************************************
* djinterp [test]                                            file_tests_path.cpp
*
*   II. PATH (file_path.hpp) -- the lexical path value. Purely textual, no
* syscalls, so every case here is deterministic.
*
*   WHAT THIS SECTION IS FOR. file_path.hpp says every operation delegates to
* c/fs/file_path and that "if parent() ever disagrees with d_path_dirname, one of
* them is a bug". The c/fs suite already pins d_path_dirname and friends, so
* repeating their whole truth table here would test the C layer twice. What is
* NOT covered there, and is covered here, is everything the C++ type ADDS:
* value semantics, the invalid state a move leaves behind, and the two places
* the C++ surface deliberately DIFFERS from the C one --
*
*     extension()  returns ""  where d_path_extension returns NULL
*     c_str()      returns ""  where a C caller would hold a null pointer
*
*   Both differences exist so a C++ caller never dereferences a null; both are
* asserted below, because a "helpful" change back to NULL would compile fine
* and segfault at the call sites.
*
*   SEPARATORS. Expected strings are written with '/' and rewritten through
* sep_path to whatever D_FILE_PATH_SEP is, so one expectation holds on POSIX
* and Windows alike. Inputs keep '/', which every grammar this layer parses
* accepts.
*
*
* path:      /tests/djinterp/core/fs/file_tests_path.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstring>
#include <type_traits>
// djinterp
#include "file_tests.hpp"


namespace djinterp
{
namespace testing
{

namespace
{

// sep_path
//   function: build a path from a '/'-separated template, rewriting each '/'
// to the platform's output separator -- so a single expectation is correct
// wherever the suite runs.
path
sep_path(const char* _tmpl)
{
    char        buf[512];
    ::std::size_t i;

    for (i = 0; ((i + 1) < sizeof(buf)) && (_tmpl[i] != '\0'); ++i)
    {
        buf[i] = (_tmpl[i] == '/') ? D_FILE_PATH_SEP : _tmpl[i];
    }

    buf[i] = '\0';

    return path(buf);
}

} // anonymous namespace


void
tests_fs_path(suite_type& _suite)
{
    _suite.test_module("II. path");

    // -----------------------------------------------------------------
    //  construction and validity
    // -----------------------------------------------------------------

    _suite.test_block("path :: construction");

    _suite.test("a default-constructed path is valid and empty")
          .assert_([]() -> bool
          {
              path p;

              return ( (p.valid()) &&
                       (p.empty()) &&
                       (p.size() == 0) );
          });

    _suite.test("a path from a string carries it")
          .assert_([]() -> bool
          {
              path p("alpha/beta");

              return ( (p.valid()) &&
                       (!p.empty()) &&
                       (p.size() == 10) &&
                       (std::strcmp(p.c_str(), "alpha/beta") == 0) );
          });

    _suite.test("a path from an empty string is valid and empty")
          .assert_([]() -> bool
          {
              path p("");

              return ( (p.valid()) &&
                       (p.empty()) );
          });

    _suite.test("c_str is never NULL, not even on an invalid path")
          .assert_([]() -> bool
          {
              path invalid = fs_test_invalid_path();

              // the C layer would hand back a null here; the C++ type
              // promises a readable empty string instead
              return ( (!invalid.valid()) &&
                       (invalid.c_str() != 0) &&
                       (invalid.c_str()[0] == '\0') );
          });

    _suite.test("an invalid path reports empty and zero-sized")
          .assert_([]() -> bool
          {
              path invalid = fs_test_invalid_path();

              return ( (invalid.empty()) &&
                       (invalid.size() == 0) );
          });

    // -----------------------------------------------------------------
    //  decomposition
    // -----------------------------------------------------------------

    _suite.test_block("path :: decomposition");

    _suite.test("parent is the directory holding the file")
          .assert_([]() -> bool
          {
              return (path("/path/to/file.txt").parent() == sep_path("/path/to"));
          });

    _suite.test("parent of a bare name is the current directory")
          .assert_([]() -> bool
          {
              return (path("file.txt").parent() == path("."));
          });

    _suite.test("the root is its own parent")
          .assert_([]() -> bool
          {
              return (path("/").parent() == sep_path("/"));
          });

    _suite.test("filename is the final component")
          .assert_([]() -> bool
          {
              return ( (path("/path/to/file.txt").filename() == path("file.txt")) &&
                       (path("file.txt").filename() == path("file.txt")) );
          });

    _suite.test("a trailing separator does not hide the final component")
          .assert_([]() -> bool
          {
              return (path("a/b/").filename() == path("b"));
          });

    _suite.test("stem drops only the final extension")
          .assert_([]() -> bool
          {
              return ( (path("/a/file.txt").stem() == path("file")) &&
                       (path("archive.tar.gz").stem() == path("archive.tar")) );
          });

    _suite.test("a hidden file keeps its whole name as its stem")
          .assert_([]() -> bool
          {
              return (path(".bashrc").stem() == path(".bashrc"));
          });

    _suite.test("extension includes the dot")
          .assert_([]() -> bool
          {
              return (std::strcmp(path("file.txt").extension(), ".txt") == 0);
          });

    _suite.test("the LAST extension wins on a double extension")
          .assert_([]() -> bool
          {
              return (std::strcmp(path("archive.tar.gz").extension(), ".gz") == 0);
          });

    _suite.test("no extension yields \"\", NOT NULL")
          .assert_([]() -> bool
          {
              const char* ext = path("filename").extension();

              // the C layer returns NULL here; the C++ layer must not, or
              // every strcmp at a call site becomes a crash
              return ( (ext != 0) &&
                       (ext[0] == '\0') );
          });

    _suite.test("a leading dot is a hidden file, not an extension")
          .assert_([]() -> bool
          {
              const char* ext = path(".bashrc").extension();

              return ( (ext != 0) &&
                       (ext[0] == '\0') );
          });

    _suite.test("extension of an invalid path is \"\", not a crash")
          .assert_([]() -> bool
          {
              path        invalid = fs_test_invalid_path();
              const char* ext     = invalid.extension();

              return ( (ext != 0) &&
                       (ext[0] == '\0') );
          });

    // -----------------------------------------------------------------
    //  composition
    // -----------------------------------------------------------------

    _suite.test_block("path :: composition");

    _suite.test("append joins with exactly one separator")
          .assert_([]() -> bool
          {
              path p("a");

              p.append(path("b"));

              return (p == sep_path("a/b"));
          });

    _suite.test("a caller's trailing separator is not doubled")
          .assert_([]() -> bool
          {
              path p("a/");

              p.append(path("b"));

              return (p == sep_path("a/b"));
          });

    _suite.test("operator/= is append")
          .assert_([]() -> bool
          {
              path p("a");

              p /= path("b");

              return (p == sep_path("a/b"));
          });

    _suite.test("operator/ composes without touching its operands")
          .assert_([]() -> bool
          {
              path left("a");
              path right("b");
              path joined = left / right;

              return ( (joined == sep_path("a/b")) &&
                       (left == path("a")) &&
                       (right == path("b")) );
          });

    _suite.test("an absolute right-hand side replaces the left")
          .assert_([]() -> bool
          {
              return ((path("a") / path("/b")) == sep_path("/b"));
          });

    _suite.test("joining onto the root keeps a single separator")
          .assert_([]() -> bool
          {
              return ((path("/") / path("b")) == sep_path("/b"));
          });

    _suite.test("appending an empty path leaves the original")
          .assert_([]() -> bool
          {
              return ((path("a") / path("")) == path("a"));
          });

    _suite.test("append chains left to right")
          .assert_([]() -> bool
          {
              return ((path("a") / path("b") / path("c")) == sep_path("a/b/c"));
          });

    // -----------------------------------------------------------------
    //  queries
    // -----------------------------------------------------------------

    _suite.test_block("path :: queries");

    _suite.test("a rooted path is absolute and not relative")
          .assert_([]() -> bool
          {
              path p("/foo");

              return ( (p.is_absolute()) &&
                       (!p.is_relative()) );
          });

    _suite.test("a bare name is relative and not absolute")
          .assert_([]() -> bool
          {
              path p("foo");

              return ( (!p.is_absolute()) &&
                       (p.is_relative()) );
          });

    _suite.test("an empty path is not absolute")
          .assert_([]() -> bool
          {
              return (!path("").is_absolute());
          });

    _suite.test("an invalid path is neither absolute nor relative")
          .assert_([]() -> bool
          {
              path invalid = fs_test_invalid_path();

              // is_relative is defined as valid() && !is_absolute(), so the
              // invalid path must answer no to both rather than yes to one
              return ( (!invalid.is_absolute()) &&
                       (!invalid.is_relative()) );
          });

    _suite.test("root_length measures the root prefix")
          .assert_([]() -> bool
          {
              return ( (path("/").root_length() == 1) &&
                       (path("/foo").root_length() == 1) &&
                       (path("foo").root_length() == 0) );
          });

    // -----------------------------------------------------------------
    //  normalization
    // -----------------------------------------------------------------

    _suite.test_block("path :: normalization");

    _suite.test("dot-dot cancels the preceding component")
          .assert_([]() -> bool
          {
              return (path("/path/to/../file.txt").normalized()
                          == sep_path("/path/file.txt"));
          });

    _suite.test("separator runs collapse and dot components vanish")
          .assert_([]() -> bool
          {
              return ( (path("a//b").normalized() == sep_path("a/b")) &&
                       (path("./a/./b").normalized() == sep_path("a/b")) );
          });

    _suite.test("a leading dot-dot survives when nothing precedes it")
          .assert_([]() -> bool
          {
              return (path("../../a").normalized() == sep_path("../../a"));
          });

    _suite.test("dot-dot at an absolute root is dropped")
          .assert_([]() -> bool
          {
              return (path("/../a").normalized() == sep_path("/a"));
          });

    _suite.test("an empty path normalizes to the current directory")
          .assert_([]() -> bool
          {
              return (path("").normalized() == path("."));
          });

    _suite.test("normalization is lexical, and leaves the original alone")
          .assert_([]() -> bool
          {
              path original("a/b/../c");
              path result = original.normalized();

              return ( (result == sep_path("a/c")) &&
                       (original == path("a/b/../c")) );
          });

    // -----------------------------------------------------------------
    //  comparison
    // -----------------------------------------------------------------

    _suite.test_block("path :: comparison");

    _suite.test("equal strings compare equal")
          .assert_([]() -> bool
          {
              return ( (path("a/b") == path("a/b")) &&
                       (!(path("a/b") != path("a/b"))) );
          });

    _suite.test("different strings compare unequal")
          .assert_([]() -> bool
          {
              return (path("a/b") != path("a/c"));
          });

    _suite.test("comparison is textual, not lexical-equivalence")
          .assert_([]() -> bool
          {
              // "a/b/../b" and "a/b" name the same file but are different
              // TEXT; normalization is the caller's to ask for
              return (path("a/b/../b") != path("a/b"));
          });

#if defined(__cpp_impl_three_way_comparison) && (__cpp_impl_three_way_comparison >= 201907L)
    _suite.test("three-way comparison orders paths and agrees with ==")
          .assert_([]() -> bool
          {
              return ( ((path("a") <=> path("b")) < 0) &&
                       ((path("b") <=> path("a")) > 0) &&
                       ((path("a") <=> path("a")) == 0) );
          });

    _suite.test("three-way comparison is usable in relational operators")
          .assert_([]() -> bool
          {
              return ( (path("a") < path("b")) &&
                       (path("b") > path("a")) &&
                       (path("a") <= path("a")) );
          });
#endif

    // -----------------------------------------------------------------
    //  value semantics
    // -----------------------------------------------------------------

    _suite.test_block("path :: value semantics");

    _suite.test("path is copyable, being a value type")
          .assert_([]() -> bool
          {
              return ( (::std::is_copy_constructible<path>::value) &&
                       (::std::is_copy_assignable<path>::value) );
          });

    _suite.test("a copy is independent of its source")
          .assert_([]() -> bool
          {
              path original("a/b");
              path copy(original);

              copy.append(path("c"));

              return ( (copy == sep_path("a/b/c")) &&
                       (original == path("a/b")) );
          });

    _suite.test("copy-assignment replaces the target's contents")
          .assert_([]() -> bool
          {
              path target("old");
              path source("new");

              target = source;

              return ( (target == path("new")) &&
                       (source == path("new")) );
          });

    _suite.test("a move transfers the representation")
          .assert_([]() -> bool
          {
              path source("a/b");
              path sink(static_cast<path&&>(source));

              return ( (sink == path("a/b")) &&
                       (sink.valid()) );
          });

    _suite.test("a moved-from path is INVALID, not merely empty")
          .assert_([]() -> bool
          {
              path source("a/b");
              path sink(static_cast<path&&>(source));

              // this is the distinction the free functions rely on: they
              // reject !valid() with EINVAL, and an empty-but-valid path
              // would sail past that guard
              return ( (!source.valid()) &&
                       (source.empty()) );
          });

    _suite.test("move-assignment transfers and invalidates the source")
          .assert_([]() -> bool
          {
              path source("a/b");
              path sink("discarded");

              sink = static_cast<path&&>(source);

              return ( (sink == path("a/b")) &&
                       (!source.valid()) );
          });

    _suite.test("self move-assignment leaves the path intact")
          .assert_([]() -> bool
          {
              path p("a/b");

              // the guard in operator= exists precisely so this does not
              // free the representation and then adopt the freed pointer
              p = static_cast<path&&>(p);

              return ( (p.valid()) &&
                       (p == path("a/b")) );
          });

    _suite.test("swap exchanges two paths")
          .assert_([]() -> bool
          {
              path left("left");
              path right("right");

              left.swap(right);

              return ( (left == path("right")) &&
                       (right == path("left")) );
          });

    return;
}

} // namespace testing
} // namespace djinterp
