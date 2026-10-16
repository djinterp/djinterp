/*******************************************************************************
* djinterp [test]                                        file_tests_expected.cpp
*
*   XII. EXPECTED (file_expected.hpp) -- the C++23 return surface, where the
* value and the reason travel together instead of through an out-parameter.
*
*   WHY THIS SECTION IS USUALLY EMPTY. The suite builds at C++20 by default, so
* everything below compiles to nothing: the gate is __cpp_lib_expected, the
* same library feature-test macro the header itself uses, NOT __cplusplus
* (whose C++23 value varies by compiler) and NOT a hand-rolled version guess.
* Raising the standard turns these on with no other change.
*
*   THE SEMANTICS MUST MATCH THE OTHER FORM. These overloads exist beside the
* error& ones, and a caller must be able to switch between them without
* learning new rules. The subtle one is status(): an absent file is a VALUE
* whose exists() is false, not an unexpected -- exactly as the error-code form
* reports it with a CLEARED error. If the expected form treated absence as a
* failure the two spellings would disagree about what "not there" means, so
* that equivalence is what the cases below pin, function by function, against
* the error& form's answer rather than against a restated constant.
*
*
* path:      /tests/djinterp/core/fs/file_tests_expected.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cerrno>
#include <cstring>
// djinterp
#include "file_tests.hpp"


#if defined(__cpp_lib_expected) && (__cpp_lib_expected >= 202202L)

namespace djinterp
{
namespace testing
{

void
tests_fs_expected(suite_type& _suite)
{
    _suite.test_module("XII. expected (C++23)");

    // -----------------------------------------------------------------
    //  status
    // -----------------------------------------------------------------

    _suite.test_block("status (expected form)");

    _suite.test("a real file yields a value describing it")
          .assert_([]() -> bool
          {
              auto result = status(fs_test_path(D_CF_TEST_FILE));

              return ( (result.has_value())              &&
                       (result->is_regular_file())       &&
                       (result->size() == std::strlen(D_CF_TEST_CONTENT)) );
          });

    _suite.test("an ABSENT file is a VALUE, not an unexpected")
          .assert_([]() -> bool
          {
              auto result = status(fs_test_path("no_such_expected_status"));

              // absence is an answer in both forms; making it an unexpected
              // here would put the two spellings at odds
              return ( (result.has_value()) &&
                       (!result->exists())  &&
                       (result->type() == file_status::type_not_found) );
          });

    _suite.test("an invalid path IS an unexpected, carrying EINVAL")
          .assert_([]() -> bool
          {
              auto result = status(fs_test_invalid_path());

              return ( (!result.has_value()) &&
                       (result.error().value() == EINVAL) );
          });

    _suite.test("it agrees with the error-code form on the same input")
          .assert_([]() -> bool
          {
              error ec;
              path  missing = fs_test_path("no_such_expected_agree");

              file_status by_ec       = status(missing, ec);
              auto        by_expected = status(missing);

              // both must say "found nothing, nothing went wrong"
              return ( (!ec.failed())                          &&
                       (by_expected.has_value())               &&
                       (by_ec.type() == by_expected->type()) );
          });

    // -----------------------------------------------------------------
    //  file_size
    // -----------------------------------------------------------------

    _suite.test_block("file_size (expected form)");

    _suite.test("a regular file yields its byte count")
          .assert_([]() -> bool
          {
              auto result = file_size(fs_test_path(D_CF_TEST_FILE));

              return ( (result.has_value()) &&
                       (*result == std::strlen(D_CF_TEST_CONTENT)) );
          });

    _suite.test("an absent file is an unexpected ENOENT")
          .assert_([]() -> bool
          {
              auto result = file_size(fs_test_path("no_such_expected_size"));

              // unlike status(), being asked the size of nothing has no
              // answer -- so here absence IS a failure, in both forms
              return ( (!result.has_value()) &&
                       (result.error().value() == ENOENT) );
          });

    _suite.test("a directory is an unexpected EINVAL")
          .assert_([]() -> bool
          {
              auto result = file_size(fs_test_root());

              return ( (!result.has_value()) &&
                       (result.error().value() == EINVAL) );
          });

    _suite.test("value_or supplies a default without branching")
          .assert_([]() -> bool
          {
              auto missing = file_size(fs_test_path("no_such_expected_or"));

              return (missing.value_or(42) == 42);
          });

    // -----------------------------------------------------------------
    //  space
    // -----------------------------------------------------------------

    _suite.test_block("space (expected form)");

    _suite.test("an existing path yields a capacity snapshot")
          .assert_([]() -> bool
          {
              auto result = space(fs_test_root());

              return ( (result.has_value())            &&
                       (result->capacity > 0)          &&
                       (result->available <= result->free) &&
                       (result->free <= result->capacity) );
          });

    _suite.test("an absent path is an unexpected")
          .assert_([]() -> bool
          {
              auto result = space(fs_test_path("no_such_expected_space"));

              return (!result.has_value());
          });

    _suite.test("an invalid path is an unexpected EINVAL")
          .assert_([]() -> bool
          {
              auto result = space(fs_test_invalid_path());

              return ( (!result.has_value()) &&
                       (result.error().value() == EINVAL) );
          });

    // -----------------------------------------------------------------
    //  read_symlink
    // -----------------------------------------------------------------

    _suite.test_block("read_symlink (expected form)");

    _suite.test("a regular file is an unexpected")
          .assert_([]() -> bool
          {
              auto result = read_symlink(fs_test_path(D_CF_TEST_FILE));

              return (!result.has_value());
          });

    _suite.test("an invalid path is an unexpected EINVAL")
          .assert_([]() -> bool
          {
              auto result = read_symlink(fs_test_invalid_path());

              return ( (!result.has_value()) &&
                       (result.error().value() == EINVAL) );
          });

    _suite.add_if(fs_test_symlinks_creatable(),
                  [](suite_type& _b)
                  {
                      _b.test("a real link yields its target text")
                        .assert_([]() -> bool
                        {
                            error ec;
                            path  target = fs_test_path(D_CF_TEST_FILE);
                            path  link   = fs_test_path("expected_link.symlink");
                            bool  ok;

                            remove(link, ec);

                            if (!create_symlink(target, link, ec))
                            {
                                return false;
                            }

                            {
                                auto result = read_symlink(link);

                                ok = result.has_value() && (*result == target);
                            }

                            remove(link, ec);

                            return ok;
                        });
                  });

    return;
}

} // namespace testing
} // namespace djinterp

#endif // __cpp_lib_expected
