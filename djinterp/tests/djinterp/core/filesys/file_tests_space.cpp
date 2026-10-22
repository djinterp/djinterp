/*******************************************************************************
* djinterp [test]                                           file_tests_space.cpp
*
*   IX. CAPACITY (file_space.hpp) -- three byte counts, one of which is a trap.
*
*   FREE IS NOT AVAILABLE. `free` counts unallocated bytes INCLUDING the
* reserve only root may spend; `available` counts what this user may actually
* claim. On a typical volume they differ by gigabytes, and deciding "will this
* write fit" from `free` is how an unprivileged program talks itself into space
* it cannot touch. The relationship case below asserts the ordering that makes
* that difference real -- available <= free <= capacity -- rather than exact
* figures, which depend entirely on the host and would make this suite a
* measurement of the CI runner's disk.
*
*
* path:      /tests/djinterp/core/filesys/file_tests_space.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.20
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
tests_fs_space(suite_type& _suite)
{
    _suite.test_module("IX. capacity");

    _suite.test_block("space_info");

    _suite.test("a default space_info is all zeroes")
          .assert_([]() -> bool
          {
              space_info s;

              return ( (s.capacity == 0) &&
                       (s.free == 0)     &&
                       (s.available == 0) );
          });

    _suite.test_block("space");

    _suite.test("an existing path reports a real capacity")
          .assert_([]() -> bool
          {
              error      ec(EIO);
              space_info s = space(fs_test_root(), ec);

              return ( (!ec.failed()) &&
                       (s.capacity > 0) );
          });

    _suite.test("available <= free <= capacity")
          .assert_([]() -> bool
          {
              error      ec;
              space_info s = space(fs_test_root(), ec);

              // the ordering that makes `available` the one to spend against:
              // it can only ever be the smallest of the three
              return ( (!ec.failed())           &&
                       (s.available <= s.free)  &&
                       (s.free <= s.capacity) );
          });

    _suite.test("a file and its directory sit on the same filesystem")
          .assert_([]() -> bool
          {
              error      ec_file;
              error      ec_dir;
              space_info from_file = space(fs_test_path(D_CF_TEST_FILE), ec_file);
              space_info from_dir  = space(fs_test_root(), ec_dir);

              // the query is about the FILESYSTEM holding the path, not the
              // path, so both must describe the same volume
              return ( (!ec_file.failed())  &&
                       (!ec_dir.failed())   &&
                       (from_file.capacity == from_dir.capacity) );
          });

    _suite.test("a missing path fails")
          .assert_([]() -> bool
          {
              error ec;

              space(fs_test_path("no_such_space_path"), ec);

              return (ec.failed());
          });

    _suite.test("a failure leaves the result zeroed, not stale")
          .assert_([]() -> bool
          {
              error      ec;
              space_info s = space(fs_test_path("no_such_space_zero"), ec);

              // a caller who forgets to test _ec must not read a plausible
              // capacity off a failed call
              return ( (ec.failed())     &&
                       (s.capacity == 0) &&
                       (s.free == 0)     &&
                       (s.available == 0) );
          });

    _suite.test("an invalid path is EINVAL")
          .assert_([]() -> bool
          {
              error ec;

              space(fs_test_invalid_path(), ec);

              return (ec.value() == EINVAL);
          });

    return;
}

} // namespace testing
} // namespace djinterp
