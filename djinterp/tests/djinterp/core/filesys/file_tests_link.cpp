/*******************************************************************************
* djinterp [test]                                            file_tests_link.cpp
*
*   VII. LINKS (file_link.hpp) -- create one, read what it says, ask whether a
* path is one.
*
*   TWO GATES, FOR TWO DIFFERENT REASONS. The section is compiled out entirely
* where the platform has no symbolic links (D_FILE_LINK_IS_AVAILABLE), and the
* creation cases are additionally admitted only when this process can actually
* MAKE one -- the API existing does not imply the privilege to use it, which is
* the usual state of an unprivileged Windows runner. The second gate uses
* add_if, so the cases are ABSENT rather than vacuously passing: a suite that
* reports green for work it never did is worse than one that reports less.
*
*   A DANGLING LINK IS A VALID LINK. read_symlink returns what the link SAYS,
* not what it reaches, so it works perfectly on a link whose target does not
* exist -- and that is the case most likely to be broken by an implementation
* that "helpfully" resolves. The dangling case below asserts the whole triple:
* the link is a link, its text is readable, and following it finds nothing.
*
*
* path:      /tests/djinterp/core/filesys/file_tests_link.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.20
*******************************************************************************/

// std
#include <cerrno>
// djinterp
#include "file_tests.hpp"


#if D_FILE_LINK_IS_AVAILABLE

namespace djinterp
{
namespace testing
{

void
tests_fs_link(suite_type& _suite)
{
    _suite.test_module("VII. links");

    // -----------------------------------------------------------------
    //  the cases that need no link to exist
    // -----------------------------------------------------------------

    _suite.test_block("is_symlink");

    _suite.test("a regular file is not a symlink")
          .assert_([]() -> bool
          {
              error ec;

              return (!is_symlink(fs_test_path(D_CF_TEST_FILE), ec));
          });

    _suite.test("a directory is not a symlink")
          .assert_([]() -> bool
          {
              error ec;

              return (!is_symlink(fs_test_root(), ec));
          });

    _suite.test("an absent path is not a symlink")
          .assert_([]() -> bool
          {
              error ec;

              return (!is_symlink(fs_test_path("no_such_link_query"), ec));
          });

    _suite.test("an invalid path is EINVAL")
          .assert_([]() -> bool
          {
              error ec;

              is_symlink(fs_test_invalid_path(), ec);

              return (ec.value() == EINVAL);
          });

    _suite.test_block("validation");

    _suite.test("create_symlink rejects an invalid target or link path")
          .assert_([]() -> bool
          {
              error ec_target;
              error ec_link;
              path  real = fs_test_path(D_CF_TEST_FILE);

              return ( (!create_symlink(fs_test_invalid_path(), real, ec_target)) &&
                       (ec_target.value() == EINVAL)                              &&
                       (!create_symlink(real, fs_test_invalid_path(), ec_link))   &&
                       (ec_link.value() == EINVAL) );
          });

    _suite.test("read_symlink rejects an invalid path")
          .assert_([]() -> bool
          {
              error ec;
              path  result = read_symlink(fs_test_invalid_path(), ec);

              // the failure result is an invalid path (file_link.hpp,
              // read_symlink's @return), so a caller testing result.valid()
              // sees the failure as well as _ec; it once returned an empty
              // path that tested valid, which hid the failure from that test
              return ( (ec.value() == EINVAL) &&
                       (!result.valid()) );
          });

    _suite.test("read_symlink on a REGULAR file fails")
          .assert_([]() -> bool
          {
              error ec;

              // a file is not a link, and asking what it points at has no
              // answer -- EINVAL from the platform
              read_symlink(fs_test_path(D_CF_TEST_FILE), ec);

              return (ec.failed());
          });

    // -----------------------------------------------------------------
    //  the cases that need a link this process can create
    // -----------------------------------------------------------------

    _suite.add_if(fs_test_symlinks_creatable(),
                  [](suite_type& _b)
                  {
                      _b.test_block("create_symlink / read_symlink");

                      _b.test("a link is created, recognised, and read back")
                        .assert_([]() -> bool
                        {
                            error ec(EIO);
                            path  target = fs_test_path(D_CF_TEST_FILE);
                            path  link   = fs_test_path("link_basic.symlink");
                            bool  ok;

                            remove(link, ec);

                            if (!create_symlink(target, link, ec))
                            {
                                return false;
                            }

                            ok = (!ec.failed())            &&
                                 is_symlink(link, ec)      &&
                                 (read_symlink(link, ec) == target);

                            remove(link, ec);

                            return ok;
                        });

                      _b.test("read_symlink returns the target TEXT, unresolved")
                        .assert_([]() -> bool
                        {
                            error ec;
                            path  link = fs_test_path("link_relative.symlink");
                            path  told;
                            bool  ok;

                            remove(link, ec);

                            // a RELATIVE target: a resolving implementation
                            // would hand back an absolute path instead
                            if (!create_symlink(path("./sample.txt"), link, ec))
                            {
                                return false;
                            }

                            told = read_symlink(link, ec);
                            ok   = (told == path("./sample.txt"));

                            remove(link, ec);

                            return ok;
                        });

                      _b.test("a DANGLING link is still a perfectly valid link")
                        .assert_([]() -> bool
                        {
                            error ec;
                            path  missing = fs_test_path("no_such_link_target");
                            path  link    = fs_test_path("link_dangling.symlink");
                            bool  ok;

                            remove(link, ec);

                            if (!create_symlink(missing, link, ec))
                            {
                                return false;
                            }

                            {
                                error ec_follow;

                                // the link exists and is readable; what it
                                // points AT does not exist. Both are true at
                                // once, and neither is an error.
                                ok = is_symlink(link, ec)              &&
                                     (read_symlink(link, ec) == missing) &&
                                     (!exists(link, ec_follow))          &&
                                     (!ec_follow.failed());
                            }

                            remove(link, ec);

                            return ok;
                        });

                      _b.test("a link to a DIRECTORY is a link, not a directory")
                        .assert_([]() -> bool
                        {
                            error ec;
                            path  link = fs_test_path("link_to_dir.symlink");
                            bool  ok;

                            remove(link, ec);

                            if (!create_symlink(fs_test_root(), link, ec))
                            {
                                return false;
                            }

                            {
                                error ec_direct;

                                // symlink_status stops at the link; status
                                // follows it through to the directory
                                ok = symlink_status(link, ec_direct).is_symlink() &&
                                     is_directory(link, ec);
                            }

                            remove(link, ec);

                            return ok;
                        });

                      _b.test("creating a link where something already exists fails")
                        .assert_([]() -> bool
                        {
                            error ec;
                            path  link = fs_test_path("link_occupied.symlink");
                            bool  ok;

                            if (!fs_test_write(link, "in the way", 10))
                            {
                                return false;
                            }

                            ok = (!create_symlink(fs_test_path(D_CF_TEST_FILE),
                                                  link,
                                                  ec)) &&
                                 (ec.failed());

                            remove(link, ec);

                            return ok;
                        });

                      _b.test("removing a link removes the LINK, not its target")
                        .assert_([]() -> bool
                        {
                            error ec;
                            path  target = fs_test_path(D_CF_TEST_FILE);
                            path  link   = fs_test_path("link_removed.symlink");
                            bool  ok;

                            remove(link, ec);

                            if (!create_symlink(target, link, ec))
                            {
                                return false;
                            }

                            ok = remove(link, ec)        &&
                                 (!fs_test_exists(link)) &&
                                 fs_test_exists(target);

                            return ok;
                        });
                  });

    return;
}

} // namespace testing
} // namespace djinterp

#endif // D_FILE_LINK_IS_AVAILABLE
