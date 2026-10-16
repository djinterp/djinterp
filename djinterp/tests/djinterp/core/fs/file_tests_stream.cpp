/*******************************************************************************
* djinterp [test]                                          file_tests_stream.cpp
*
*   IV. FILE (file_stream.hpp) -- the RAII core of the layer, and the section
* with the most to say that the c/fs suite could not.
*
*   WHAT OWNERSHIP MEANS, TESTED. A file owns a FILE*, so the cases below check
* the things ownership promises rather than the things fread does:
*
*     - copying is ILL-FORMED, not merely discouraged (a trait, at compile time)
*     - a move TRANSFERS the stream and leaves the source closed, so the
*       destructor of the husk cannot double-close
*     - self-move does not destroy the stream it is about to adopt
*     - the destructor closes, and closing FLUSHES -- verified by reading the
*       bytes back with raw stdio after the file has gone out of scope, which
*       is the only way to see a flush that never happened
*
*   THE CLOSED HANDLE. Every operation on a closed file reports EBADF and
* changes nothing; close() itself is the exception, being idempotent success so
* that closing twice -- or closing a default-constructed file -- is not an
* error. Both spellings are asserted, since "EBADF everywhere except close" is
* easy to get half right.
*
*   CONSTRUCTORS CANNOT REPORT. file(p, mode) has no error channel, so a failed
* open leaves is_open() false and the reason unreadable; open(p, mode, ec) on a
* default-constructed file is the form that says why. Both are exercised, and
* the difference is the point rather than an oversight.
*
*
* path:      /tests/djinterp/core/fs/file_tests_stream.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cerrno>
#include <cstring>
#include <type_traits>
// djinterp
#include "file_tests.hpp"


namespace djinterp
{
namespace testing
{

void
tests_fs_stream(suite_type& _suite)
{
    _suite.test_module("IV. file");

    // -----------------------------------------------------------------
    //  ownership: the compile-time shape
    // -----------------------------------------------------------------

    _suite.test_block("file :: ownership");

    _suite.test("file is NOT copyable -- two owners would double-close")
          .assert_([]() -> bool
          {
              return ( (!::std::is_copy_constructible<file>::value) &&
                       (!::std::is_copy_assignable<file>::value) );
          });

    _suite.test("file IS movable")
          .assert_([]() -> bool
          {
              return ( (::std::is_move_constructible<file>::value) &&
                       (::std::is_move_assignable<file>::value) );
          });

    _suite.test("the move operations are noexcept")
          .assert_([]() -> bool
          {
              // a throwing move would make file unusable in a container that
              // offers the strong guarantee
              return ( (::std::is_nothrow_move_constructible<file>::value) &&
                       (::std::is_nothrow_move_assignable<file>::value) );
          });

    _suite.test("the bool conversion is EXPLICIT")
          .assert_([]() -> bool
          {
              // explicit keeps `if (f)` working while rejecting `int n = f;`
              // and the arithmetic that would silently follow
              return (!::std::is_convertible<file&, bool>::value);
          });

    // -----------------------------------------------------------------
    //  construction and lifetime
    // -----------------------------------------------------------------

    _suite.test_block("file :: construction");

    _suite.test("a default-constructed file owns nothing")
          .assert_([]() -> bool
          {
              file f;

              return ( (!f.is_open()) &&
                       (!static_cast<bool>(f)) &&
                       (f.native_handle() == 0) );
          });

    _suite.test("the convenience constructor opens an existing file")
          .assert_([]() -> bool
          {
              file f(fs_test_path(D_CF_TEST_FILE), "rb");

              return ( (f.is_open()) &&
                       (static_cast<bool>(f)) &&
                       (f.native_handle() != 0) );
          });

    _suite.test("a failed construction leaves the file closed, silently")
          .assert_([]() -> bool
          {
              // the constructor has no error channel; this is the documented
              // trade for the convenient spelling
              file f(fs_test_path("no_such_ctor_file"), "rb");

              return (!f.is_open());
          });

    _suite.test("open reports WHY it failed, which the constructor cannot")
          .assert_([]() -> bool
          {
              error ec;
              file  f;

              return ( (!f.open(fs_test_path("no_such_open_file"), "rb", ec)) &&
                       (ec.failed()) &&
                       (!f.is_open()) );
          });

    _suite.test("open succeeds and clears the error")
          .assert_([]() -> bool
          {
              error ec(EIO);
              file  f;

              return ( (f.open(fs_test_path(D_CF_TEST_FILE), "rb", ec)) &&
                       (!ec.failed()) &&
                       (f.is_open()) );
          });

    _suite.test("opening with an invalid path is EINVAL")
          .assert_([]() -> bool
          {
              error ec;
              file  f;

              return ( (!f.open(fs_test_invalid_path(), "rb", ec)) &&
                       (ec.value() == EINVAL) );
          });

    _suite.test("a write mode creates the file")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("stream_created.txt");
              bool  ok;

              {
                  file f;

                  ok = f.open(p, "wb", ec) && f.is_open();
              }

              ok = ok && fs_test_exists(p);
              remove(p, ec);

              return ok;
          });

    // -----------------------------------------------------------------
    //  closing
    // -----------------------------------------------------------------

    _suite.test_block("file :: closing");

    _suite.test("close releases the stream")
          .assert_([]() -> bool
          {
              error ec(EIO);
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");

              return ( (f.close(ec)) &&
                       (!ec.failed()) &&
                       (!f.is_open()) &&
                       (f.native_handle() == 0) );
          });

    _suite.test("closing twice is idempotent success, not an error")
          .assert_([]() -> bool
          {
              error ec;
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");

              f.close(ec);

              return ( (f.close(ec)) &&
                       (!ec.failed()) );
          });

    _suite.test("closing a never-opened file also succeeds")
          .assert_([]() -> bool
          {
              error ec(EIO);
              file  f;

              return ( (f.close(ec)) &&
                       (!ec.failed()) );
          });

    _suite.test("the DESTRUCTOR closes, and closing flushes")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("stream_dtor_flush.txt");
              bool  ok;

              {
                  file f(p, "wb");

                  if (!f.is_open())
                  {
                      return false;
                  }

                  f.write("flushed by the destructor", 25, ec);

                  // NO explicit close: the scope ending must do it, and must
                  // flush stdio's buffer on the way out
              }

              // read with raw stdio -- an unflushed buffer would show up here
              // as a short or empty file
              ok = fs_test_contents_are(p, "flushed by the destructor");

              remove(p, ec);

              return ok;
          });

    // -----------------------------------------------------------------
    //  move semantics
    // -----------------------------------------------------------------

    _suite.test_block("file :: move semantics");

    _suite.test("a move transfers the stream and closes the source")
          .assert_([]() -> bool
          {
              file source(fs_test_path(D_CF_TEST_FILE), "rb");
              FILE* raw = source.native_handle();
              file sink(static_cast<file&&>(source));

              return ( (sink.is_open())              &&
                       (sink.native_handle() == raw) &&
                       (!source.is_open())           &&
                       (source.native_handle() == 0) );
          });

    _suite.test("move-assignment transfers and closes the source")
          .assert_([]() -> bool
          {
              file source(fs_test_path(D_CF_TEST_FILE), "rb");
              file sink;

              sink = static_cast<file&&>(source);

              return ( (sink.is_open()) &&
                       (!source.is_open()) );
          });

    _suite.test("move-assignment closes whatever the target already held")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("stream_move_over.txt");
              bool  ok;

              if (!fs_test_write(p, "x", 1))
              {
                  return false;
              }

              {
                  file target(p, "rb");
                  file source(fs_test_path(D_CF_TEST_FILE), "rb");

                  target = static_cast<file&&>(source);

                  // the stream target used to own must have been closed, not
                  // leaked -- it now holds source's
                  ok = target.is_open() && (!source.is_open());
              }

              remove(p, ec);

              return ok;
          });

    _suite.test("self move-assignment does not destroy the stream")
          .assert_([]() -> bool
          {
              error ec;
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");
              char  buf[8];

              f = static_cast<file&&>(f);

              // still usable: a naive move-assign would have closed the
              // stream and then adopted the closed handle
              return ( (f.is_open()) &&
                       (f.read(buf, 4, ec) == 4) &&
                       (!ec.failed()) );
          });

    _suite.test("a moved-from file is reusable after a fresh open")
          .assert_([]() -> bool
          {
              error ec;
              file  source(fs_test_path(D_CF_TEST_FILE), "rb");
              file  sink(static_cast<file&&>(source));

              // moved-from is a valid, closed object -- not a poisoned one
              return ( (source.open(fs_test_path(D_CF_TEST_FILE), "rb", ec)) &&
                       (source.is_open()) );
          });

    // -----------------------------------------------------------------
    //  byte I/O
    // -----------------------------------------------------------------

    _suite.test_block("file :: read / write");

    _suite.test("bytes written are read back intact")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("stream_roundtrip.txt");
              char  buf[32];
              bool  ok = false;

              {
                  file w(p, "wb");

                  if (w.write("round trip", 10, ec) != 10)
                  {
                      return false;
                  }
              }

              {
                  file r(p, "rb");

                  std::memset(buf, 0, sizeof(buf));

                  ok = ( (r.read(buf, 10, ec) == 10) &&
                         (!ec.failed())              &&
                         (std::memcmp(buf, "round trip", 10) == 0) );
              }

              remove(p, ec);

              return ok;
          });

    _suite.test("reading the standard fixture yields its exact bytes")
          .assert_([]() -> bool
          {
              error         ec;
              file          f(fs_test_path(D_CF_TEST_FILE), "rb");
              char          buf[128];
              ::std::size_t want = std::strlen(D_CF_TEST_CONTENT);
              ::std::size_t got;

              std::memset(buf, 0, sizeof(buf));
              got = f.read(buf, sizeof(buf), ec);

              return ( (got == want) &&
                       (std::memcmp(buf, D_CF_TEST_CONTENT, want) == 0) );
          });

    _suite.test("a read at end of file returns 0 and is NOT an error")
          .assert_([]() -> bool
          {
              error ec(EIO);
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");
              char  buf[256];

              // drain it first
              f.read(buf, sizeof(buf), ec);

              // end of file is a condition, not a failure
              return ( (f.read(buf, sizeof(buf), ec) == 0) &&
                       (!ec.failed()) );
          });

    _suite.test("a zero-length read is a no-op")
          .assert_([]() -> bool
          {
              error ec(EIO);
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");
              char  buf[8];

              return ( (f.read(buf, 0, ec) == 0) &&
                       (!ec.failed()) );
          });

    _suite.test("reading a CLOSED file is EBADF")
          .assert_([]() -> bool
          {
              error ec;
              file  f;
              char  buf[8];

              return ( (f.read(buf, sizeof(buf), ec) == 0) &&
                       (ec.value() == EBADF) );
          });

    _suite.test("writing a CLOSED file is EBADF")
          .assert_([]() -> bool
          {
              error ec;
              file  f;

              return ( (f.write("x", 1, ec) == 0) &&
                       (ec.value() == EBADF) );
          });

    // -----------------------------------------------------------------
    //  positioning
    // -----------------------------------------------------------------

    _suite.test_block("file :: seek / tell / truncate");

    _suite.test("seek and tell agree, on all three origins")
          .assert_([]() -> bool
          {
              error ec;
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");

              return ( (f.seek(5, SEEK_SET, ec)) &&
                       (f.tell(ec) == 5)         &&
                       (f.seek(3, SEEK_CUR, ec)) &&
                       (f.tell(ec) == 8)         &&
                       (f.seek(0, SEEK_END, ec)) &&
                       (f.tell(ec)
                            == static_cast<d_off_t>(std::strlen(D_CF_TEST_CONTENT))) );
          });

    _suite.test("a fresh handle starts at offset zero")
          .assert_([]() -> bool
          {
              error ec;
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");

              return (f.tell(ec) == 0);
          });

    _suite.test("seeking a CLOSED file is EBADF")
          .assert_([]() -> bool
          {
              error ec;
              file  f;

              return ( (!f.seek(0, SEEK_SET, ec)) &&
                       (ec.value() == EBADF) );
          });

    _suite.test("telling a CLOSED file is EBADF")
          .assert_([]() -> bool
          {
              error ec;
              file  f;

              return ( (f.tell(ec) == -1) &&
                       (ec.value() == EBADF) );
          });

    _suite.test("truncate shrinks, discarding the tail")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("stream_truncate.txt");
              bool  ok;

              if (!fs_test_write(p, "0123456789ABCDEF", 16))
              {
                  return false;
              }

              {
                  file f(p, "r+b");

                  ok = f.truncate(10, ec) && (!ec.failed());
              }

              ok = ok && fs_test_contents_are(p, "0123456789");
              remove(p, ec);

              return ok;
          });

    _suite.test("truncate extends, and the file grows")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("stream_extend.txt");
              bool  ok;

              if (!fs_test_write(p, "abc", 3))
              {
                  return false;
              }

              {
                  file f(p, "r+b");

                  ok = f.truncate(32, ec) && (!ec.failed());
              }

              ok = ok && (file_size(p, ec) == 32);
              remove(p, ec);

              return ok;
          });

    _suite.test("truncating a CLOSED file is EBADF")
          .assert_([]() -> bool
          {
              error ec;
              file  f;

              return ( (!f.truncate(0, ec)) &&
                       (ec.value() == EBADF) );
          });

    // -----------------------------------------------------------------
    //  durability
    // -----------------------------------------------------------------

    _suite.test_block("file :: flush / sync");

    _suite.test("flush pushes buffered bytes to the file")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("stream_flush.txt");
              bool  ok;

              {
                  file f(p, "wb");

                  f.write("buffered", 8, ec);

                  ok = f.flush(ec) && (!ec.failed());

                  // visible to another reader WITHOUT closing, which is what
                  // flushing is for
                  ok = ok && fs_test_contents_are(p, "buffered");
              }

              remove(p, ec);

              return ok;
          });

    _suite.test("sync succeeds on a written file")
          .assert_([]() -> bool
          {
              error ec;
              path  p = fs_test_path("stream_sync.txt");
              bool  ok;

              {
                  file f(p, "wb");

                  f.write("durable", 7, ec);
                  ok = f.sync(ec) && (!ec.failed());
              }

              remove(p, ec);

              return ok;
          });

    _suite.test("flush and sync on a CLOSED file are EBADF")
          .assert_([]() -> bool
          {
              error ec_flush;
              error ec_sync;
              file  f;

              return ( (!f.flush(ec_flush))         &&
                       (ec_flush.value() == EBADF)  &&
                       (!f.sync(ec_sync))           &&
                       (ec_sync.value() == EBADF) );
          });

    // -----------------------------------------------------------------
    //  advisory locking
    // -----------------------------------------------------------------

    _suite.test_block("file :: locking");

    _suite.test("an exclusive lock is taken and released")
          .assert_([]() -> bool
          {
              error ec;
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");

              return ( (f.try_lock_exclusive(ec)) &&
                       (!ec.failed())             &&
                       (f.unlock(ec))             &&
                       (!ec.failed()) );
          });

    _suite.test("a shared lock is taken and released")
          .assert_([]() -> bool
          {
              error ec;
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");

              return ( (f.try_lock_shared(ec)) &&
                       (f.unlock(ec)) );
          });

    _suite.test("the blocking forms work on an uncontended file")
          .assert_([]() -> bool
          {
              error ec;
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");

              // uncontended, so these cannot block the suite
              return ( (f.lock_shared(ec)) &&
                       (f.unlock(ec))      &&
                       (f.lock_exclusive(ec)) &&
                       (f.unlock(ec)) );
          });

    _suite.test("locking a CLOSED file is EBADF")
          .assert_([]() -> bool
          {
              error ec;
              file  f;

              return ( (!f.lock_exclusive(ec)) &&
                       (ec.value() == EBADF) );
          });

    // -----------------------------------------------------------------
    //  observers
    // -----------------------------------------------------------------

    _suite.test_block("file :: observers");

    _suite.test("descriptor borrows the live descriptor")
          .assert_([]() -> bool
          {
              error ec;
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");

              return ( (f.descriptor(ec) >= 0) &&
                       (!ec.failed()) );
          });

    _suite.test("descriptor of a CLOSED file is EBADF")
          .assert_([]() -> bool
          {
              error ec;
              file  f;

              return ( (f.descriptor(ec) == -1) &&
                       (ec.value() == EBADF) );
          });

    _suite.test("status describes THIS file, with no path to race")
          .assert_([]() -> bool
          {
              error       ec;
              file        f(fs_test_path(D_CF_TEST_FILE), "rb");
              file_status s = f.status(ec);

              return ( (!ec.failed())        &&
                       (s.is_regular_file()) &&
                       (s.size() == std::strlen(D_CF_TEST_CONTENT)) );
          });

    _suite.test("status of a CLOSED file is EBADF")
          .assert_([]() -> bool
          {
              error ec;
              file  f;

              f.status(ec);

              return (ec.value() == EBADF);
          });

    // -----------------------------------------------------------------
    //  temporaries
    // -----------------------------------------------------------------

    _suite.test_block("file :: open_temp");

    _suite.test("a temporary opens, and round-trips bytes")
          .assert_([]() -> bool
          {
              error ec;
              file  f;
              char  buf[8];

              if (!f.open_temp(ec))
              {
                  return false;
              }

              std::memset(buf, 0, sizeof(buf));

              return ( (f.is_open())                    &&
                       (f.write("tmp", 3, ec) == 3)     &&
                       (f.seek(0, SEEK_SET, ec))        &&
                       (f.read(buf, 3, ec) == 3)        &&
                       (std::memcmp(buf, "tmp", 3) == 0) );
          });

    _suite.test("open_temp closes whatever the file already held")
          .assert_([]() -> bool
          {
              error ec;
              file  f(fs_test_path(D_CF_TEST_FILE), "rb");
              FILE* before = f.native_handle();

              if (!f.open_temp(ec))
              {
                  return false;
              }

              // the previous stream was replaced, not leaked alongside
              return ( (f.is_open()) &&
                       (f.native_handle() != before) );
          });

    return;
}

} // namespace testing
} // namespace djinterp
