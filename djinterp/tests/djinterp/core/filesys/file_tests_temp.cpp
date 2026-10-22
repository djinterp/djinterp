/*******************************************************************************
* djinterp [test]                                            file_tests_temp.cpp
*
*   VIII. TEMPORARIES (file_temp.hpp) -- the one temp query that returns a path
* rather than a handle.
*
*   WHAT IS DELIBERATELY ABSENT. file_temp.h offers d_file_temp_name and labels it
* racy; this layer does not surface it, because a name handed back is not a
* file and another process can win it before you open it. The safe spelling is
* tested in the file section. So this section is small on purpose -- the
* missing surface is the design, not a gap.
*
*
* path:      /tests/djinterp/core/filesys/file_tests_temp.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.20
*******************************************************************************/

// djinterp
#include "file_tests.hpp"


namespace djinterp
{
namespace testing
{

void
tests_fs_temp(suite_type& _suite)
{
    _suite.test_module("VIII. temporaries");

    _suite.test_block("temp_directory_path");

    _suite.test("yields a valid path with the error clear")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  p = temp_directory_path(ec);

              return ( (!ec.failed()) &&
                       (p.valid())    &&
                       (!p.empty()) );
          });

    _suite.test("the path names a directory that actually exists")
          .assert_([]() -> bool
          {
              error ec;
              path  p = temp_directory_path(ec);

              if (ec.failed())
              {
                  return false;
              }

              return ( (fs_test_exists(p)) &&
                       (is_directory(p, ec)) );
          });

    _suite.test("it is absolute, so it resolves from anywhere")
          .assert_([]() -> bool
          {
              error ec;
              path  p = temp_directory_path(ec);

              return ( (!ec.failed()) &&
                       (p.is_absolute()) );
          });

    _suite.test("repeated calls agree")
          .assert_([]() -> bool
          {
              error ec_a;
              error ec_b;

              // a pure query of the environment: nothing about it should
              // change between two calls in one process
              return (temp_directory_path(ec_a) == temp_directory_path(ec_b));
          });

    _suite.test("a temporary file can actually be created there")
          .assert_([]() -> bool
          {
              error ec;
              path  dir = temp_directory_path(ec);
              path  probe;
              bool  ok;

              if (ec.failed())
              {
                  return false;
              }

              probe = dir / path("djinterp_core_fs_temp_probe.txt");

              // the query is only useful if the answer is writable, which is
              // the claim worth checking
              if (!fs_test_write(probe, "probe", 5))
              {
                  return false;
              }

              ok = fs_test_exists(probe);

              remove(probe, ec);

              return ok;
          });

    return;
}

} // namespace testing
} // namespace djinterp
