/*******************************************************************************
* djinterp [test]                                           file_tests_error.cpp
*
*   I. ERROR (file_common.hpp) -- the errno carrier every operation in the
* layer reports through. Small surface, entirely pure, so every case here is
* deterministic on a machine with no filesystem at all.
*
*   THE ABSENT CONVERSION IS A FEATURE. file_common.hpp is explicit that error
* has NO operator bool, because `if (ec)` reads as "if error" to one author and
* "if ok" to the next. That is a design decision, and a decision worth testing:
* the case below asserts the conversion does not exist, so re-adding it fails
* here rather than silently making every call site ambiguous. It is checked
* with a type trait at compile time -- the only way to assert that something is
* NOT callable without failing to compile.
*
*
* path:      /tests/djinterp/core/filesys/file_tests_error.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.20
*******************************************************************************/

// std
#include <cerrno>
#include <type_traits>
// djinterp
#include "file_tests.hpp"


namespace djinterp
{
namespace testing
{

void
tests_fs_error(suite_type& _suite)
{
    _suite.test_module("I. error");

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    _suite.test_block("error :: construction");

    _suite.test("a default-constructed error is success")
          .assert_([]() -> bool
          {
              error ec;

              return ( (ec.value() == 0) &&
                       (!ec.failed()) );
          });

    _suite.test("an error built from a code carries it and reads as failed")
          .assert_([]() -> bool
          {
              error ec(EACCES);

              return ( (ec.value() == EACCES) &&
                       (ec.failed()) );
          });

    _suite.test("an error built from zero is still success")
          .assert_([]() -> bool
          {
              error ec(0);

              return (!ec.failed());
          });

    // -----------------------------------------------------------------
    //  mutation
    // -----------------------------------------------------------------

    _suite.test_block("error :: mutation");

    _suite.test("assign replaces the code")
          .assert_([]() -> bool
          {
              error ec;

              ec.assign(ENOENT);

              return ( (ec.value() == ENOENT) &&
                       (ec.failed()) );
          });

    _suite.test("clear returns it to success")
          .assert_([]() -> bool
          {
              error ec(EIO);

              ec.clear();

              return ( (ec.value() == 0) &&
                       (!ec.failed()) );
          });

    _suite.test("from_errno captures the live errno")
          .assert_([]() -> bool
          {
              error ec;

              errno = EPERM;
              ec    = error::from_errno();

              return (ec.value() == EPERM);
          });

    // -----------------------------------------------------------------
    //  message
    // -----------------------------------------------------------------

    _suite.test_block("error :: message");

    _suite.test("a known code has a non-empty message")
          .assert_([]() -> bool
          {
              error ec(ENOENT);

              return ( (ec.message() != 0) &&
                       (ec.message()[0] != '\0') );
          });

    _suite.test("success has a message too, and it is never NULL")
          .assert_([]() -> bool
          {
              error ec;

              return (ec.message() != 0);
          });

    _suite.test("an unrecognised code still yields a string, not NULL")
          .assert_([]() -> bool
          {
              // strerror is documented to describe unknown codes rather than
              // fail, and message() promises to pass that promise on
              error ec(999999);

              return (ec.message() != 0);
          });

    // -----------------------------------------------------------------
    //  comparison
    // -----------------------------------------------------------------

    _suite.test_block("error :: comparison");

    _suite.test("two errors with the same code are equal")
          .assert_([]() -> bool
          {
              return (error(EINVAL) == error(EINVAL));
          });

    _suite.test("errors with different codes are unequal")
          .assert_([]() -> bool
          {
              return (error(EINVAL) != error(ENOENT));
          });

    _suite.test("comparing against a default error is the success test")
          .assert_([]() -> bool
          {
              error ok;
              error bad(EIO);

              return ( (ok == error()) &&
                       (bad != error()) );
          });

    // -----------------------------------------------------------------
    //  design constraints
    // -----------------------------------------------------------------

    _suite.test_block("error :: design constraints");

    _suite.test("error does NOT convert to bool (deliberately)")
          .assert_([]() -> bool
          {
              // the whole reason failed() exists: `if (ec)` would be
              // ambiguous to a reader, so the conversion must stay absent
              return (!::std::is_convertible<error, bool>::value);
          });

    _suite.test("error is copyable, being a value")
          .assert_([]() -> bool
          {
              return ( (::std::is_copy_constructible<error>::value) &&
                       (::std::is_copy_assignable<error>::value) );
          });

    _suite.test("a copy carries the same code")
          .assert_([]() -> bool
          {
              error original(EBADF);
              error copy(original);

              return ( (copy == original) &&
                       (copy.value() == EBADF) );
          });

    return;
}

} // namespace testing
} // namespace djinterp
