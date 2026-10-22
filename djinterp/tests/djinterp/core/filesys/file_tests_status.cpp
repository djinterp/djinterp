/*******************************************************************************
* djinterp [test]                                          file_tests_status.cpp
*
*   III. FILE_STATUS (file_stat.hpp) -- one stat, then as many questions as you
* like, every answer describing the same snapshot.
*
*   THE CONTRACT MOST LIKELY TO BE GOT WRONG. An absent file is NOT a failure
* here. status() on a path that does not exist CLEARS the error and returns
* type_not_found, mirroring std::filesystem -- "I looked, and there is nothing
* there" is an answer, not an error. A caller who tests _ec.failed() to decide
* whether a file exists will therefore conclude that every missing file exists.
* The cases below pin this, and pin the deliberate asymmetry beside it:
*
*     status(missing)      -> type_not_found, _ec CLEAR
*     file_size(missing)   -> 0,              _ec = ENOENT
*
*   file_size is a question about a file's contents, so being handed no file is
* a failure to answer; status is a question about what is there, so "nothing"
* is the answer. Both spellings are intentional and both are asserted.
*
*   THE SNAPSHOT PROPERTY. The header's whole argument for this type is that
* is_regular_file() and size() on ONE file_status cannot race, because there is
* no second syscall between them. The consistency case below reads every
* question off a single snapshot and checks they agree -- which is the property
* the type exists to provide.
*
*
* path:      /tests/djinterp/core/filesys/file_tests_status.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.20
*******************************************************************************/

// std
#include <cerrno>
#include <cstring>
// djinterp
#include "file_tests.hpp"


namespace djinterp
{
namespace testing
{

void
tests_fs_status(suite_type& _suite)
{
    _suite.test_module("III. file_status");

    // -----------------------------------------------------------------
    //  the value type
    // -----------------------------------------------------------------

    _suite.test_block("file_status :: construction");

    _suite.test("a default status is type_none and does not exist")
          .assert_([]() -> bool
          {
              file_status s;

              return ( (s.type() == file_status::type_none) &&
                       (!s.exists()) );
          });

    _suite.test("type_not_found does not exist either")
          .assert_([]() -> bool
          {
              file_status s(file_status::type_not_found);

              return ( (s.type() == file_status::type_not_found) &&
                       (!s.exists()) );
          });

    _suite.test("a regular-file status exists and knows its type")
          .assert_([]() -> bool
          {
              file_status s(file_status::type_regular);

              return ( (s.exists()) &&
                       (s.is_regular_file()) &&
                       (!s.is_directory()) &&
                       (!s.is_symlink()) );
          });

    _suite.test("the type predicates are mutually exclusive")
          .assert_([]() -> bool
          {
              file_status dir(file_status::type_directory);

              return ( (dir.is_directory()) &&
                       (!dir.is_regular_file()) &&
                       (!dir.is_symlink()) &&
                       (!dir.is_fifo()) &&
                       (!dir.is_socket()) &&
                       (!dir.is_block_file()) &&
                       (!dir.is_character_file()) );
          });

    _suite.test("is_other is exactly type_unknown")
          .assert_([]() -> bool
          {
              return ( (file_status(file_status::type_unknown).is_other()) &&
                       (!file_status(file_status::type_regular).is_other()) );
          });

    // -----------------------------------------------------------------
    //  status() over a real path
    // -----------------------------------------------------------------

    _suite.test_block("status");

    _suite.test("a regular file is described exactly")
          .assert_([]() -> bool
          {
              error       ec(EIO);
              file_status s = status(fs_test_path(D_CF_TEST_FILE), ec);

              return ( (!ec.failed()) &&
                       (s.exists()) &&
                       (s.is_regular_file()) &&
                       (s.size() == std::strlen(D_CF_TEST_CONTENT)) );
          });

    _suite.test("a directory is reported as a directory")
          .assert_([]() -> bool
          {
              error       ec(EIO);
              file_status s = status(fs_test_root(), ec);

              return ( (!ec.failed()) &&
                       (s.is_directory()) &&
                       (!s.is_regular_file()) );
          });

    _suite.test("an ABSENT path is type_not_found with the error CLEAR")
          .assert_([]() -> bool
          {
              error       ec(EIO);
              file_status s = status(fs_test_path("no_such_status_file"), ec);

              // the point of the case: nothing failed. A caller testing
              // ec.failed() to detect absence would be wrong every time.
              return ( (!ec.failed()) &&
                       (s.type() == file_status::type_not_found) &&
                       (!s.exists()) );
          });

    _suite.test("an invalid path is EINVAL and type_none")
          .assert_([]() -> bool
          {
              error       ec;
              file_status s = status(fs_test_invalid_path(), ec);

              return ( (ec.value() == EINVAL) &&
                       (s.type() == file_status::type_none) );
          });

    _suite.test("one snapshot answers every question consistently")
          .assert_([]() -> bool
          {
              error       ec;
              file_status s = status(fs_test_path(D_CF_TEST_FILE), ec);

              // every answer below comes from the SAME stat -- this is the
              // property the type exists to provide
              return ( (s.exists()) &&
                       (s.is_regular_file()) &&
                       (!s.is_directory()) &&
                       (s.type() == file_status::type_regular) &&
                       (s.size() == s.native().st_size) &&
                       (s.mode() == s.native().st_mode) );
          });

    _suite.test("a just-written file has a plausible modification time")
          .assert_([]() -> bool
          {
              error       ec;
              file_status s = status(fs_test_path(D_CF_TEST_FILE), ec);

              return ( (!ec.failed()) &&
                       (s.modified_time() > 0) );
          });

    _suite.test("every timestamp accessor reads its own native field")
          .assert_([]() -> bool
          {
              error       ec;
              file_status s = status(fs_test_path(D_CF_TEST_FILE), ec);

              // asserted against native() rather than against "> 0": creation
              // time is genuinely unavailable on some platforms and legitimately
              // reads back as zero there, so a plausibility check would fail on
              // exactly the systems the accessor exists to paper over
              return ( (!ec.failed())                             &&
                       (s.modified_time() == s.native().st_modified) &&
                       (s.access_time()   == s.native().st_accessed) &&
                       (s.change_time()   == s.native().st_changed)  &&
                       (s.creation_time() == s.native().st_created) );
          });

    _suite.test("a regular file reports at least one hard link")
          .assert_([]() -> bool
          {
              error       ec;
              file_status s = status(fs_test_path(D_CF_TEST_FILE), ec);

              return ( (!ec.failed()) &&
                       (s.hard_links() >= 1) );
          });

    _suite.test("permissions are the mode's permission bits")
          .assert_([]() -> bool
          {
              error       ec;
              file_status s = status(fs_test_path(D_CF_TEST_FILE), ec);

              return ( (!ec.failed()) &&
                       (s.permissions() == (s.mode() & 0777)) );
          });

    // -----------------------------------------------------------------
    //  symlink_status()
    // -----------------------------------------------------------------

    _suite.test_block("symlink_status");

    _suite.test("on a regular file it agrees with status")
          .assert_([]() -> bool
          {
              error       ec_a;
              error       ec_b;
              file_status followed = status(fs_test_path(D_CF_TEST_FILE), ec_a);
              file_status direct   = symlink_status(fs_test_path(D_CF_TEST_FILE), ec_b);

              return ( (!ec_a.failed()) &&
                       (!ec_b.failed()) &&
                       (followed.is_regular_file()) &&
                       (direct.is_regular_file()) );
          });

    _suite.test("an invalid path is EINVAL")
          .assert_([]() -> bool
          {
              error ec;

              symlink_status(fs_test_invalid_path(), ec);

              return (ec.value() == EINVAL);
          });

    // the pair only differ over a link, so those cases need one to exist
    _suite.add_if(fs_test_symlinks_creatable(),
                  [](suite_type& _b)
                  {
                      _b.test("the two DISAGREE over a symlink, which is the point")
                        .assert_([]() -> bool
                        {
                            error ec;
                            path  target = fs_test_path(D_CF_TEST_FILE);
                            path  link   = fs_test_path("status_link.symlink");
                            bool  made;
                            bool  ok;

                            remove(link, ec);
                            made = create_symlink(target, link, ec);

                            if (!made)
                            {
                                return false;
                            }

                            {
                                error       ec_follow;
                                error       ec_direct;
                                file_status followed = status(link, ec_follow);
                                file_status direct   = symlink_status(link, ec_direct);

                                // status follows the link and sees the FILE;
                                // symlink_status stops at the LINK itself
                                ok = ( (followed.is_regular_file()) &&
                                       (!followed.is_symlink())     &&
                                       (direct.is_symlink())        &&
                                       (!direct.is_regular_file()) );
                            }

                            remove(link, ec);

                            return ok;
                        });
                  });

    // -----------------------------------------------------------------
    //  the free one-question forms
    // -----------------------------------------------------------------

    _suite.test_block("exists / is_regular_file / is_directory");

    _suite.test("exists is true for a file and a directory, false for neither")
          .assert_([]() -> bool
          {
              error ec;

              return ( (exists(fs_test_path(D_CF_TEST_FILE), ec)) &&
                       (exists(fs_test_root(), ec))               &&
                       (!exists(fs_test_path("no_such_exists"), ec)) );
          });

    _suite.test("is_regular_file distinguishes a file from a directory")
          .assert_([]() -> bool
          {
              error ec;

              return ( (is_regular_file(fs_test_path(D_CF_TEST_FILE), ec)) &&
                       (!is_regular_file(fs_test_root(), ec)) );
          });

    _suite.test("is_directory distinguishes a directory from a file")
          .assert_([]() -> bool
          {
              error ec;

              return ( (is_directory(fs_test_root(), ec)) &&
                       (!is_directory(fs_test_path(D_CF_TEST_FILE), ec)) );
          });

    _suite.test("an absent path is simply not any of them, without failing")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  missing = fs_test_path("no_such_predicate");

              return ( (!exists(missing, ec))          &&
                       (!ec.failed())                  &&
                       (!is_regular_file(missing, ec)) &&
                       (!ec.failed())                  &&
                       (!is_directory(missing, ec))    &&
                       (!ec.failed()) );
          });

    // -----------------------------------------------------------------
    //  file_size()
    // -----------------------------------------------------------------

    _suite.test_block("file_size");

    _suite.test("reports the exact byte count")
          .assert_([]() -> bool
          {
              error ec(EIO);

              return ( (file_size(fs_test_path(D_CF_TEST_FILE), ec)
                            == std::strlen(D_CF_TEST_CONTENT)) &&
                       (!ec.failed()) );
          });

    _suite.test("an empty file is zero bytes and NOT an error")
          .assert_([]() -> bool
          {
              error ec(EIO);
              path  p = fs_test_path("size_empty.txt");
              bool  ok;

              if (!fs_test_write(p, "", 0))
              {
                  return false;
              }

              ok = ( (file_size(p, ec) == 0) &&
                     (!ec.failed()) );

              remove(p, ec);

              return ok;
          });

    _suite.test("an ABSENT file IS an error here -- ENOENT")
          .assert_([]() -> bool
          {
              error ec;

              // deliberately unlike status(): being asked the size of a file
              // that is not there is a question that cannot be answered
              return ( (file_size(fs_test_path("no_such_size"), ec) == 0) &&
                       (ec.value() == ENOENT) );
          });

    _suite.test("a directory has no file size -- EINVAL")
          .assert_([]() -> bool
          {
              error ec;

              return ( (file_size(fs_test_root(), ec) == 0) &&
                       (ec.value() == EINVAL) );
          });

    _suite.test("an invalid path is EINVAL")
          .assert_([]() -> bool
          {
              error ec;

              return ( (file_size(fs_test_invalid_path(), ec) == 0) &&
                       (ec.value() == EINVAL) );
          });

    return;
}

} // namespace testing
} // namespace djinterp
