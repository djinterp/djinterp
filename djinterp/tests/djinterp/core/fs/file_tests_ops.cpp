/*******************************************************************************
* djinterp [test]                                             file_tests_ops.cpp
*
*   VI. OPERATIONS (file_ops.hpp) -- remove, rename, copy over whole paths.
*
*   RENAME ALWAYS OVERWRITES HERE. The C d_file_rename takes an overwrite flag and
* the c/fs suite pins both settings; this layer passes 1 unconditionally, so
* renaming onto an existing file REPLACES it rather than failing EEXIST. That
* is a deliberate narrowing -- rename is the atomic-replace primitive, and a
* replace that refuses to replace is not one -- but it means a C caller's
* instinct is wrong here, so the case below states it outright.
*
*   THE REMOVAL FAMILY, BY WHAT EACH REFUSES. remove() takes a file or an empty
* directory; remove_file() refuses a directory. The refusal is the point of the
* narrower name, so it is tested -- and tested WITHOUT pinning an errno, since
* the header says EISDIR or EPERM by platform and asserting either one would
* make this suite fail on the other.
*
*
* path:      /tests/djinterp/core/fs/file_tests_ops.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cerrno>
// djinterp
#include "file_tests.hpp"


namespace djinterp
{
namespace testing
{

void
tests_fs_ops(suite_type& _suite)
{
    _suite.test_module("VI. operations");

    // -----------------------------------------------------------------
    //  remove
    // -----------------------------------------------------------------

    _suite.test_block("remove");

    _suite.test("a regular file is removed")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  p = fs_test_path("ops_remove.txt");

              if (!fs_test_write(p, "x", 1))
              {
                  return false;
              }

              return ( (remove(p, ec))  &&
                       (!ec.failed())   &&
                       (!fs_test_exists(p)) );
          });

    _suite.test("an EMPTY directory is removed too")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("ops_remove_dir");

              remove_all(p, ec);

              if (!create_directory(p, ec))
              {
                  return false;
              }

              return ( (remove(p, ec)) &&
                       (!fs_test_exists(p)) );
          });

    _suite.test("a missing path fails")
          .assert_([]() -> bool
          {
              error ec;

              return ( (!remove(fs_test_path("no_such_remove"), ec)) &&
                       (ec.failed()) );
          });

    _suite.test("an invalid path is EINVAL")
          .assert_([]() -> bool
          {
              error ec;

              return ( (!remove(fs_test_invalid_path(), ec)) &&
                       (ec.value() == EINVAL) );
          });

    // -----------------------------------------------------------------
    //  remove_file
    // -----------------------------------------------------------------

    _suite.test_block("remove_file");

    _suite.test("a regular file is removed")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  p = fs_test_path("ops_remove_file.txt");

              if (!fs_test_write(p, "x", 1))
              {
                  return false;
              }

              return ( (remove_file(p, ec)) &&
                       (!ec.failed())       &&
                       (!fs_test_exists(p)) );
          });

    _suite.test("a DIRECTORY is refused, and survives")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("ops_remove_file_dir");
              bool  ok;

              remove_all(p, ec);

              if (!create_directory(p, ec))
              {
                  return false;
              }

              // the errno is EISDIR or EPERM by platform, so only the refusal
              // itself is asserted -- pinning one would fail on the other
              ok = (!remove_file(p, ec)) &&
                   (ec.failed())         &&
                   fs_test_exists(p);

              remove_directory(p, ec);

              return ok;
          });

    _suite.test("an invalid path is EINVAL")
          .assert_([]() -> bool
          {
              error ec;

              return ( (!remove_file(fs_test_invalid_path(), ec)) &&
                       (ec.value() == EINVAL) );
          });

    // -----------------------------------------------------------------
    //  rename
    // -----------------------------------------------------------------

    _suite.test_block("rename");

    _suite.test("a file moves to a free destination")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  from = fs_test_path("ops_rename_from.txt");
              path  to   = fs_test_path("ops_rename_to.txt");
              bool  ok;

              remove(to, ec);

              if (!fs_test_write(from, "payload", 7))
              {
                  return false;
              }

              ok = rename(from, to, ec)      &&
                   (!ec.failed())            &&
                   (!fs_test_exists(from))   &&
                   fs_test_contents_are(to, "payload");

              remove(to, ec);

              return ok;
          });

    _suite.test("an EXISTING destination is REPLACED, not refused")
          .assert_([]() -> bool
          {
              error ec;
              path  from = fs_test_path("ops_rename_over_from.txt");
              path  to   = fs_test_path("ops_rename_over_to.txt");
              bool  ok;

              if ( (!fs_test_write(from, "new", 3)) ||
                   (!fs_test_write(to, "old", 3)) )
              {
                  return false;
              }

              // the C layer would need overwrite=1 for this; the C++ layer
              // always passes it, so this SUCCEEDS
              ok = rename(from, to, ec)    &&
                   (!ec.failed())          &&
                   (!fs_test_exists(from)) &&
                   fs_test_contents_are(to, "new");

              remove(to, ec);

              return ok;
          });

    _suite.test("a missing source fails")
          .assert_([]() -> bool
          {
              error ec;

              return ( (!rename(fs_test_path("no_such_rename"),
                                fs_test_path("ops_rename_dest.txt"),
                                ec)) &&
                       (ec.failed()) );
          });

    _suite.test("either path being invalid is EINVAL")
          .assert_([]() -> bool
          {
              error ec_from;
              error ec_to;
              path  real = fs_test_path(D_CF_TEST_FILE);

              return ( (!rename(fs_test_invalid_path(), real, ec_from)) &&
                       (ec_from.value() == EINVAL)                      &&
                       (!rename(real, fs_test_invalid_path(), ec_to))   &&
                       (ec_to.value() == EINVAL) );
          });

    // -----------------------------------------------------------------
    //  copy_file
    // -----------------------------------------------------------------

    _suite.test_block("copy_file");

    _suite.test("contents are duplicated, and the source stays")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  src = fs_test_path("ops_copy_src.txt");
              path  dst = fs_test_path("ops_copy_dst.txt");
              bool  ok;

              remove(dst, ec);

              if (!fs_test_write(src, "copy me exactly", 15))
              {
                  return false;
              }

              ok = copy_file(src, dst, ec) &&
                   (!ec.failed())          &&
                   fs_test_contents_are(dst, "copy me exactly") &&
                   fs_test_contents_are(src, "copy me exactly");

              remove(src, ec);
              remove(dst, ec);

              return ok;
          });

    _suite.test("an existing destination is replaced")
          .assert_([]() -> bool
          {
              error ec;
              path  src = fs_test_path("ops_copy_over_src.txt");
              path  dst = fs_test_path("ops_copy_over_dst.txt");
              bool  ok;

              if ( (!fs_test_write(src, "fresh", 5)) ||
                   (!fs_test_write(dst, "stale contents", 14)) )
              {
                  return false;
              }

              ok = copy_file(src, dst, ec) &&
                   fs_test_contents_are(dst, "fresh");

              remove(src, ec);
              remove(dst, ec);

              return ok;
          });

    _suite.test("an empty source copies to an empty destination")
          .assert_([]() -> bool
          {
              error ec;
              path  src = fs_test_path("ops_copy_empty_src.txt");
              path  dst = fs_test_path("ops_copy_empty_dst.txt");
              bool  ok;

              remove(dst, ec);

              if (!fs_test_write(src, "", 0))
              {
                  return false;
              }

              ok = copy_file(src, dst, ec) &&
                   fs_test_exists(dst)     &&
                   (file_size(dst, ec) == 0);

              remove(src, ec);
              remove(dst, ec);

              return ok;
          });

    _suite.test("a missing source fails")
          .assert_([]() -> bool
          {
              error ec;

              return ( (!copy_file(fs_test_path("no_such_copy_src"),
                                   fs_test_path("ops_copy_none.txt"),
                                   ec)) &&
                       (ec.failed()) );
          });

    _suite.test("a directory source is refused")
          .assert_([]() -> bool
          {
              error ec;

              return ( (!copy_file(fs_test_root(),
                                   fs_test_path("ops_copy_from_dir.txt"),
                                   ec)) &&
                       (ec.failed()) );
          });

    _suite.test("either path being invalid is EINVAL")
          .assert_([]() -> bool
          {
              error ec_src;
              error ec_dst;
              path  real = fs_test_path(D_CF_TEST_FILE);

              return ( (!copy_file(fs_test_invalid_path(), real, ec_src)) &&
                       (ec_src.value() == EINVAL)                         &&
                       (!copy_file(real, fs_test_invalid_path(), ec_dst)) &&
                       (ec_dst.value() == EINVAL) );
          });

    return;
}

} // namespace testing
} // namespace djinterp
