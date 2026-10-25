/*******************************************************************************
* djinterp [test]                                            file_tests_pipe.cpp
*
*   XI. PROCESS (file_pipe.hpp) -- a child run through the shell with a pipe on
* its input or output. Compiled out where the platform has no pipes.
*
*   A NON-ZERO EXIT IS NOT A FAILURE. This is the contract worth the section.
* close() reports whether the pipe CLOSED and the child was REAPED; the child's
* own exit code is a separate question, asked through exit_status(). A command
* that runs and exits 3 has not failed as an I/O operation -- it has answered.
* An implementation that folded the exit code into close()'s bool would make
* every non-zero-exiting command look like a broken pipe, so the case below
* requires close() to SUCCEED while exit_status() is non-zero.
*
*   THE RAW STATUS IS NOT THE EXIT CODE. exit_status() hands back what pclose
* returned, undecoded -- on POSIX a wait status whose exit code sits in the
* high byte, so `exit 3` yields 768 rather than 3. The cases therefore assert
* zero versus non-zero, never a specific number: asserting 3 would pass on a
* platform that decodes and fail on one that does not, which is a bug in the
* test rather than in the code.
*
*   COMMANDS ARE LITERAL. Every command below is a fixed string. file_pipe.hpp
* is emphatic that d_pipe_open goes through a shell and that building a command
* from outside input is a command-injection hole; a test suite is exactly the
* place not to model the bad habit.
*
*
* path:      /tests/djinterp/core/filesys/file_tests_pipe.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.10.21
*******************************************************************************/

// std
#include <cerrno>
#include <cstring>
#include <type_traits>
// djinterp
#include "file_tests.hpp"


#if D_FILE_PIPE_IS_AVAILABLE

namespace djinterp
{
namespace testing
{

void
tests_fs_pipe(suite_type& _suite)
{
    _suite.test_module("XI. process");

    // -----------------------------------------------------------------
    //  ownership
    // -----------------------------------------------------------------

    _suite.test_block("process :: ownership");

    _suite.test("process is NOT copyable")
          .assert_([]() -> bool
          {
              return ( (!::std::is_copy_constructible<process>::value) &&
                       (!::std::is_copy_assignable<process>::value) );
          });

    _suite.test("process IS movable")
          .assert_([]() -> bool
          {
              return ( (::std::is_move_constructible<process>::value) &&
                       (::std::is_move_assignable<process>::value) );
          });

    _suite.test("a move transfers the pipe and closes the source")
          .assert_([]() -> bool
          {
              error   ec;
              process source(T_PRINT("moved"), "r");
              process sink(static_cast<process&&>(source));
              bool    ok;

              ok = sink.is_open() && (!source.is_open());

              sink.close(ec);

              return ok;
          });

    // -----------------------------------------------------------------
    //  reading and writing
    // -----------------------------------------------------------------

    _suite.test_block("process :: transfer");

    _suite.test("a command's output is read back")
          .assert_([]() -> bool
          {
              error   ec;
              process p(T_PRINT("hello"), "r");
              char    buf[16];
              bool    ok;

              if (!p.is_open())
              {
                  return false;
              }

              std::memset(buf, 0, sizeof(buf));

              ok = (p.read(buf, 5, ec) == 5) &&
                   (std::memcmp(buf, "hello", 5) == 0);

              p.close(ec);

              return ok;
          });

    _suite.test("bytes are streamed into a command's input")
          .assert_([]() -> bool
          {
              error   ec;
              process p(T_SINK, "w");
              bool    ok;

              if (!p.is_open())
              {
                  return false;
              }

              ok = (p.write("data", 4, ec) == 4) &&
                   (!ec.failed());

              p.close(ec);

              return ok;
          });

    _suite.test("reading past the end returns 0 without failing")
          .assert_([]() -> bool
          {
              error   ec(EIO);
              process p(T_PRINT("hi"), "r");
              char    buf[32];
              bool    ok;

              if (!p.is_open())
              {
                  return false;
              }

              p.read(buf, sizeof(buf), ec);

              ok = (p.read(buf, sizeof(buf), ec) == 0) &&
                   (!ec.failed());

              p.close(ec);

              return ok;
          });

    _suite.test("reading a CLOSED process is EBADF")
          .assert_([]() -> bool
          {
              error   ec;
              process p;
              char    buf[8];

              return ( (p.read(buf, sizeof(buf), ec) == 0) &&
                       (ec.value() == EBADF) );
          });

    _suite.test("writing a CLOSED process is EBADF")
          .assert_([]() -> bool
          {
              error   ec;
              process p;

              return ( (p.write("x", 1, ec) == 0) &&
                       (ec.value() == EBADF) );
          });

    // -----------------------------------------------------------------
    //  exit status
    // -----------------------------------------------------------------

    _suite.test_block("process :: exit status");

    _suite.test("a default-constructed process has not exited")
          .assert_([]() -> bool
          {
              process p;

              return ( (!p.is_open()) &&
                       (!p.exited()) );
          });

    _suite.test("closing reaps the child and marks it exited")
          .assert_([]() -> bool
          {
              error   ec(EIO);
              process p(T_PRINT("done"), "r");

              if (!p.is_open())
              {
                  return false;
              }

              return ( (p.close(ec)) &&
                       (!ec.failed()) &&
                       (p.exited()) );
          });

    _suite.test("a successful command reports a zero exit status")
          .assert_([]() -> bool
          {
              error   ec;
              process p("exit 0", "r");

              if (!p.is_open())
              {
                  return false;
              }

              return ( (p.close(ec))         &&
                       (p.exited())          &&
                       (p.exit_status() == 0) );
          });

    _suite.test("a FAILING command still CLOSES cleanly -- the exit is data")
          .assert_([]() -> bool
          {
              error   ec(EIO);
              process p("exit 3", "r");

              if (!p.is_open())
              {
                  return false;
              }

              // close() succeeded (the pipe closed, the child was reaped)
              // AND the command reported failure. Both, at once, is the
              // whole design: an exit code is information, not an I/O error.
              return ( (p.close(ec))          &&
                       (!ec.failed())         &&
                       (p.exited())           &&
                       (p.exit_status() != 0) );
          });

    _suite.test("a missing command surfaces as an exit status, not a close error")
          .assert_([]() -> bool
          {
              error   ec(EIO);
              process p("this_command_does_not_exist_djinterp", "r");

              if (!p.is_open())
              {
                  // the shell itself started, so the open should have worked
                  return false;
              }

              // the SHELL ran and reported 127; nothing about the pipe broke
              return ( (p.close(ec))  &&
                       (!ec.failed()) &&
                       (p.exit_status() != 0) );
          });

    // -----------------------------------------------------------------
    //  lifetime
    // -----------------------------------------------------------------

    _suite.test_block("process :: lifetime");

    _suite.test("closing twice is idempotent success")
          .assert_([]() -> bool
          {
              error   ec;
              process p(T_PRINT("x"), "r");

              p.close(ec);

              return ( (p.close(ec)) &&
                       (!ec.failed()) );
          });

    _suite.test("closing a never-opened process succeeds")
          .assert_([]() -> bool
          {
              error   ec(EIO);
              process p;

              return ( (p.close(ec)) &&
                       (!ec.failed()) );
          });

    _suite.test("the destructor closes an open pipe")
          .assert_([]() -> bool
          {
              {
                  process p(T_PRINT("destructed"), "r");

                  if (!p.is_open())
                  {
                      return false;
                  }

                  // no explicit close: leaving the scope must reap the child
                  // rather than leak the pipe
              }

              return true;
          });

    _suite.test("a bad mode fails to open")
          .assert_([]() -> bool
          {
              process p(T_PRINT("hi"), "q");

              return (!p.is_open());
          });

    _suite.test("a NULL command fails to open")
          .assert_([]() -> bool
          {
              process p(0, "r");

              return (!p.is_open());
          });

    return;
}

} // namespace testing
} // namespace djinterp

#endif // D_FILE_PIPE_IS_AVAILABLE
