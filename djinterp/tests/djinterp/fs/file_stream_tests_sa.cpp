/*******************************************************************************
* djinterp [test]                                       file_stream_tests_sa.cpp
*
*   Unit tests for djinterp::file (inc/djinterp/core/filesys/file_stream.hpp) and the
* djinterp::error it reports through (inc/djinterp/core/filesys/file_common.hpp). One file for
* the class, sibling to path_tests_sa.cpp -- same three rules, same harness,
* one difference that matters: `file` is a RESOURCE, not a value. `path` could
* be tested with pure comparisons and no world; `file` owns an open FILE*, so
* these assertions do real I/O against a real scratch file and then check what
* the class did. That is the point of testing it at all.
*
*   THE THREE RULES, unchanged from the sibling suite.
*
*   1. A test that cannot run is OMITTED, never passed.
*      The move handoff is a C++11 addition. On C++98 the ownership group holds
*      fewer assertions and its name says why; it does not report a green
*      "skipped" for a move that the language cannot express.
*
*   2. Capability is asked at COMPILE time, not probed at runtime.
*      D_MOVE_ENABLED is a fact the language already settled. The move group
*      is #if'd on it. There is nothing to discover at run time.
*
*   3. The tier rule is DEMONSTRATED, not asserted in prose.
*      One source, built C++98 -> C++23. The higher tiers ADD the move group
*      and change nothing else. Building the same file on every standard is the
*      only check that can catch an additive-contract break; no assertion
*      inside the file can.
*
*   WHAT IS NOT ASSERTED HERE, AND WHY.
*      `file` is non-copyable on every tier -- the copy constructor and
*   copy-assignment are DELETED. That is a COMPILE-TIME guarantee: there is no
*   run-time behaviour to observe, because the program that copies a file does
*   not compile. It is verified by a negative-compile check outside this file
*   (a TU that copies a file and must fail to build), not by a leaf in the tree
*   -- asserting it at run time would require the very code that cannot exist.
*
*   WHY THIS FILE CARRIES ITS OWN HARNESS.
*      Identical reason to the sibling: the C harness is not yet
*   C++-consumable (no extern "C"; it pulls dfile.h / dstring.h / dtime.h), so
*   the Section 0 shim stands in and mirrors that API exactly. The day the real
*   header is C++-ready, this file drops the shim, includes it, and the bodies
*   below do not change a line.
*
*   THE SCRATCH FILE.
*      Groups that need a file make one with d_tests_file_scratch() (a distinct
*   name per call) and remove() it when done -- remove() and fopen() are
*   standard C, so the test itself keeps the header's discipline: no platform
*   call anywhere. A missing-file path uses a name under a directory that does
*   not exist, so the open fails for the reason the assertion expects.
*
*
* path:      /tests/djinterp/fs/file_stream_tests_sa.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    STANDALONE HARNESS         (self-contained; no framework dependency)
      --------------------------------------------------------------------
      1.    d_test_object + constructors
      2.    D_ASSERT_* macros
      3.    d_tests_file_group (derived count)
      4.    printer + counters
      5.    d_tests_file_scratch (a real, disposable path)

I.    CONSTRUCTION / STATE      default file owns nothing
      ---------------------------------------------------

II.   OPEN / CLOSE              open, close, double-close, re-open
      ------------------------------------------------------------

III.  I/O                       write/read roundtrip; EOF is not an error
      -------------------------------------------------------------------

IV.   POSITIONING               tell / seek / truncate
      ------------------------------------------------

V.    DURABILITY                flush / sync
      --------------------------------------

VI.   ERROR TYPE                djinterp::error, in isolation
      -------------------------------------------------------

VII.  FAILURE PATHS             the file operations that populate an error
      --------------------------------------------------------------------

VIII. OWNERSHIP                 move handoff (C++11+); non-copyable is compile-time
      -----------------------------------------------------------------------------

IX.   DESCRIPTOR & LOCKING      the fd bridge and advisory locks (Phase 4)
      --------------------------------------------------------------------

X.    STATUS (fstat)            metadata for the open descriptor (Phase 5)
      --------------------------------------------------------------------

XI.   TEMP                      anonymous temp file + temp_directory_path (Phase 8)
      -----------------------------------------------------------------------------

      master runner + main
*/

// std
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// djinterp
#include "../../../inc/djinterp/core/filesys/file_stream.hpp"
#include "../../../inc/djinterp/core/filesys/file_temp.hpp"   // temp_directory_path (temp group)


// ===========================================================================
// 0.   STANDALONE HARNESS
// ===========================================================================

// D_TEST_FILE_MAX_ASSERTS
//   constant: the largest number of assertions any one group may hold. Groups
// build into a local array of this size and report the count they actually
// filled, rather than declaring a count up front and hoping it matches.
#define D_TEST_FILE_MAX_ASSERTS 32


// --- 0.1  d_test_object ---

// d_test_object
//   struct: a node in the result tree -- a leaf (assertion) or an interior
// group. The layout matches the C harness's, so the bodies below are
// source-compatible with the real framework once it is C++-consumable.
struct d_test_object
{
    bool                   is_leaf;
    const char*            name;
    const char*            message;
    bool                   result;
    size_t                 count;
    struct d_test_object** elements;
};


// d_test_object_new_leaf
//   function: an assertion node carrying its pass/fail result.
static struct d_test_object*
d_test_object_new_leaf
(
    const char* _name,
    const char* _message,
    bool        _result
)
{
    struct d_test_object* obj;

    obj = (struct d_test_object*)malloc(sizeof(struct d_test_object));

    if (!obj)
    {
        return 0;
    }

    obj->is_leaf  = true;
    obj->name     = _name;
    obj->message  = _message;
    obj->result   = _result;
    obj->count    = 0;
    obj->elements = 0;

    return obj;
}


// d_test_object_new_interior
//   function: a group node with room for _count children.
static struct d_test_object*
d_test_object_new_interior
(
    const char* _name,
    size_t      _count
)
{
    struct d_test_object* obj;

    obj = (struct d_test_object*)malloc(sizeof(struct d_test_object));

    if (!obj)
    {
        return 0;
    }

    obj->is_leaf  = false;
    obj->name     = _name;
    obj->message  = 0;
    obj->result   = true;
    obj->count    = _count;
    obj->elements = 0;

    if (_count > 0)
    {
        obj->elements =
            (struct d_test_object**)calloc(_count, sizeof(struct d_test_object*));

        if (!obj->elements)
        {
            free(obj);

            return 0;
        }
    }

    return obj;
}


// d_test_object_free
//   function: release a tree. Present so the whole run is leak-clean under a
// sanitizer -- the same standard the c/filesys suite is held to.
static void
d_test_object_free
(
    struct d_test_object* _obj
)
{
    size_t idx;

    if (!_obj)
    {
        return;
    }

    for (idx = 0; idx < _obj->count; ++idx)
    {
        d_test_object_free(_obj->elements[idx]);
    }

    free(_obj->elements);
    free(_obj);

    return;
}


// --- 0.2  Assertion macros ---

// D_ASSERT_TRUE / _FALSE / _NULL / _NOT_NULL / _EQUAL
//   macro: leaf constructors, in the shape the C harness used.
#define D_ASSERT_TRUE(_name, _cond, _msg)                                    \
    d_test_object_new_leaf(_name, _msg, (_cond))
#define D_ASSERT_FALSE(_name, _cond, _msg)                                   \
    d_test_object_new_leaf(_name, _msg, !(_cond))
#define D_ASSERT_NULL(_name, _ptr, _msg)                                     \
    d_test_object_new_leaf(_name, _msg, (_ptr) == 0)
#define D_ASSERT_NOT_NULL(_name, _ptr, _msg)                                 \
    d_test_object_new_leaf(_name, _msg, (_ptr) != 0)
#define D_ASSERT_EQUAL(_name, _a, _b, _msg)                                  \
    d_test_object_new_leaf(_name, _msg, (_a) == (_b))


// --- 0.3  Group helper (derived count) ---

// d_tests_file_group
//   function: build an interior node from assertions the caller already
// created, taking the count from what the caller actually filled.
//   The count is DERIVED, not asserted -- so a conditionally-omitted assertion
// simply makes the group smaller, and there is no second number to drift out
// of sync. Same mechanism the fs suite introduced to delete a bug class.
static struct d_test_object*
d_tests_file_group
(
    const char*            _name,
    struct d_test_object** _items,
    size_t                 _count
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior(_name, _count);

    if (!group)
    {
        return 0;
    }

    for (idx = 0; idx < _count; ++idx)
    {
        group->elements[idx] = _items[idx];
    }

    return group;
}


// --- 0.4  Printer + counters ---

// d_tests_file_counter
//   struct: running pass/fail tally for the walk below.
struct d_tests_file_counter
{
    size_t passed;
    size_t failed;
};


// d_tests_file_print
//   function: walk the tree depth-first, printing each assertion's result and
// each group's header, and accumulating the tally.
static void
d_tests_file_print
(
    const struct d_test_object*  _obj,
    size_t                       _depth,
    struct d_tests_file_counter* _counter
)
{
    size_t idx;
    size_t pad;

    if (!_obj)
    {
        return;
    }

    for (pad = 0; pad < _depth * 2; ++pad)
    {
        (void)fputc(' ', stdout);
    }

    if (_obj->is_leaf)
    {
        if (_obj->result)
        {
            ++_counter->passed;
        }
        else
        {
            ++_counter->failed;
        }

        (void)printf("[%s] %-28s %s\n",
                     _obj->result ? "PASS" : "FAIL",
                     _obj->name,
                     _obj->message ? _obj->message : "");
    }
    else
    {
        (void)printf("%s\n", _obj->name);

        for (idx = 0; idx < _obj->count; ++idx)
        {
            d_tests_file_print(_obj->elements[idx], _depth + 1, _counter);
        }
    }

    return;
}


// --- 0.5  A real, disposable scratch path ---

// d_tests_file_scratch
//   function: a distinct filename per call, in the working directory, for a
// group that needs a real file. Standard C throughout -- no platform call --
// so the test keeps the header's no-OS discipline. The caller remove()s it.
static const char*
d_tests_file_scratch
(
    void
)
{
    static char buf[64];
    static int  seq = 0;

    (void)snprintf(buf, sizeof(buf), "djinterp_ftest_%d.tmp", seq);
    ++seq;

    return buf;
}


// ===========================================================================
// I.   CONSTRUCTION / STATE
// ===========================================================================

/*
d_tests_file_construct
  A default-constructed file owns nothing, and every observer says so. This is
the state open() moves it out of and close() returns it to.
*/
static struct d_test_object*
d_tests_file_construct
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    {
        djinterp::file f;

        items[idx++] = D_ASSERT_FALSE("default_not_open",
                                      f.is_open(),
                                      "a default file owns nothing");
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        // operator bool is C++11's (decision 3.2); is_open() serves below
        items[idx++] = D_ASSERT_FALSE("default_bool",
                                      (bool)f,
                                      "...so operator bool is false");
#endif
        items[idx++] = D_ASSERT_NULL("default_handle",
                                     f.native_handle(),
                                     "...and native_handle is NULL, not a stale FILE*");
    }

    return d_tests_file_group("construction / state", items, idx);
}


// ===========================================================================
// II.  OPEN / CLOSE
// ===========================================================================

/*
d_tests_file_lifetime
  open acquires, close releases, and the observers track the transition.
  Tests the following:
  - open for write succeeds and is_open() flips true
  - close succeeds and is_open() flips false
  - closing a file that is not open is SUCCESS, not an error
  - open on an already-open handle re-opens it rather than refusing
*/
static struct d_test_object*
d_tests_file_lifetime
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;
    const char*           scratch;

    bool open_ok    = false;
    bool is_open    = false;
    bool close_ok   = false;
    bool now_closed = false;
    bool dbl_ok     = false;
    bool reopen_ok  = false;

    idx     = 0;
    scratch = d_tests_file_scratch();

    {
        djinterp::file  f;
        djinterp::error ec;

        open_ok    = f.open(djinterp::path(scratch), "wb", ec) && !ec.failed();
        is_open    = f.is_open();
        close_ok   = f.close(ec) && !ec.failed();
        now_closed = !f.is_open();
        dbl_ok     = f.close(ec) && !ec.failed();   // closing a closed file: fine

        {
            djinterp::error ec2;
            bool            first;

            f.open(djinterp::path(scratch), "wb", ec2);
            first = f.is_open();
            f.open(djinterp::path(scratch), "rb", ec2);   // re-open drops the old
            reopen_ok = first && f.is_open() && !ec2.failed();
        }
    }

    items[idx++] = D_ASSERT_TRUE("open_write",       open_ok,    "open for write acquires a stream");
    items[idx++] = D_ASSERT_TRUE("is_open",          is_open,    "...is_open() is true while held");
    items[idx++] = D_ASSERT_TRUE("close",            close_ok,   "close releases it");
    items[idx++] = D_ASSERT_TRUE("closed_after",     now_closed, "...is_open() is false after");
    items[idx++] = D_ASSERT_TRUE("double_close_ok",  dbl_ok,     "closing a closed file is success, not error");
    items[idx++] = D_ASSERT_TRUE("reopen",           reopen_ok,  "open on an open handle re-opens it");

    (void)remove(scratch);

    return d_tests_file_group("open / close", items, idx);
}


// ===========================================================================
// III. I/O   (stdio; a short read at end-of-file is NOT an error)
// ===========================================================================

/*
d_tests_file_io
  Bytes written come back byte-for-byte; the boundary conditions read the way
the header promises.
  Tests the following:
  - write returns the full count, read returns it and the bytes match
  - asking for more than the file holds returns a short count and NO error
  - writing a read-only stream is a failure (there is no benign EOF for writes)
  - a count with no buffer is EINVAL, read or write; no bytes need no buffer
*/
static struct d_test_object*
d_tests_file_io
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;
    const char*           scratch;
    const char            msg[] = "hello djinterp";   // 14 bytes written

    bool write_ok   = false;
    bool read_ok    = false;
    bool data_ok    = false;
    bool eof_ok     = false;
    bool write_ro   = false;
    bool null_read  = false;
    bool null_write = false;
    bool zero_null  = false;

    idx     = 0;
    scratch = d_tests_file_scratch();

    {
        djinterp::file  f(djinterp::path(scratch), "wb");
        djinterp::error ec;

        write_ok = (f.write(msg, 14, ec) == (size_t)14) && !ec.failed();
        f.close(ec);
    }
    {
        djinterp::file  f(djinterp::path(scratch), "rb");
        djinterp::error ec;
        char            buf[32];

        std::memset(buf, 0, sizeof(buf));
        read_ok = (f.read(buf, 14, ec) == (size_t)14) && !ec.failed();
        data_ok = (std::memcmp(buf, msg, 14) == 0);

        {
            char   big[64];
            size_t got = f.read(big, sizeof(big), ec);   // more than remains

            eof_ok = (got < sizeof(big)) && !ec.failed();
        }
    }
    {
        djinterp::file  f(djinterp::path(scratch), "rb");
        djinterp::error ec;

        write_ro = (f.write("x", 1, ec) < (size_t)1) && ec.failed();
    }

    items[idx++] = D_ASSERT_TRUE("write_count",       write_ok, "write returns the count written");
    items[idx++] = D_ASSERT_TRUE("read_count",        read_ok,  "read returns the count read");
    items[idx++] = D_ASSERT_TRUE("roundtrip_data",    data_ok,  "the bytes come back unchanged");
    items[idx++] = D_ASSERT_TRUE("eof_short_not_err", eof_ok,   "a short read at EOF is success, not an error");
    items[idx++] = D_ASSERT_TRUE("write_readonly",    write_ro, "a short write IS an error -- no benign EOF for writes");

    {
        djinterp::file  f(djinterp::path(scratch), "rb");
        djinterp::error ec;
        null_read  = (f.read(NULL, 4, ec) == (size_t)0) && (ec.value() == EINVAL);
        null_write = (f.write(NULL, 4, ec) == (size_t)0) && (ec.value() == EINVAL);
        zero_null  = (f.read(NULL, 0, ec) == (size_t)0) && !ec.failed();
    }
    items[idx++] = D_ASSERT_TRUE("null_read",        null_read,  "a count with no buffer to read into is EINVAL");
    items[idx++] = D_ASSERT_TRUE("null_write",       null_write, "...and with no bytes to write from");
    items[idx++] = D_ASSERT_TRUE("zero_null",        zero_null,  "no buffer for no bytes is nothing to do");
    (void)remove(scratch);

    return d_tests_file_group("i/o (stdio; EOF is not an error)", items, idx);
}


// ===========================================================================
// IV.  POSITIONING
// ===========================================================================

/*
d_tests_file_position
  tell reports the offset, seek moves it, truncate resizes the file.
  Tests the following:
  - tell after reading 5 bytes is 5; seek to 0 then tell is 0
  - truncate to 5 leaves a 5-byte file whose bytes are the first 5
*/
static struct d_test_object*
d_tests_file_position
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;
    const char*           scratch;
    const char            body[] = "hello world!!";   // 13 bytes

    bool tell_read = false;
    bool seek_ok   = false;
    bool tell_seek = false;
    bool trunc_ok  = false;
    bool trunc_len = false;

    idx     = 0;
    scratch = d_tests_file_scratch();

    {
        djinterp::file  f(djinterp::path(scratch), "wb");
        djinterp::error ec;

        f.write(body, 13, ec);
        f.close(ec);
    }
    {
        djinterp::file  f(djinterp::path(scratch), "rb");
        djinterp::error ec;
        char            buf[16];

        f.read(buf, 5, ec);                              // consume "hello"
        tell_read = (f.tell(ec) == (d_off_t)5) && !ec.failed();
        seek_ok   = f.seek(0, SEEK_SET, ec);
        tell_seek = (f.tell(ec) == (d_off_t)0);
    }
    {
        djinterp::file  f(djinterp::path(scratch), "r+b");
        djinterp::error ec;

        trunc_ok = f.truncate(5, ec) && !ec.failed();
        f.close(ec);
    }
    {
        djinterp::file  f(djinterp::path(scratch), "rb");
        djinterp::error ec;
        char            buf[32];
        size_t          got = f.read(buf, sizeof(buf), ec);

        trunc_len = (got == (size_t)5) &&
                    (std::memcmp(buf, "hello", 5) == 0) &&
                    !ec.failed();
    }

    items[idx++] = D_ASSERT_TRUE("tell_after_read",  tell_read, "tell reports the offset after a read");
    items[idx++] = D_ASSERT_TRUE("seek_to_start",    seek_ok,   "seek to SEEK_SET 0 succeeds");
    items[idx++] = D_ASSERT_TRUE("tell_after_seek",  tell_seek, "...and tell then reads 0");
    items[idx++] = D_ASSERT_TRUE("truncate",         trunc_ok,  "truncate resizes the file");
    items[idx++] = D_ASSERT_TRUE("truncate_length",  trunc_len, "...to exactly the requested bytes");

    (void)remove(scratch);

    return d_tests_file_group("positioning", items, idx);
}


// ===========================================================================
// V.   DURABILITY
// ===========================================================================

/*
d_tests_file_durability
  flush and sync both succeed on an open, written file. (That the bytes reach
the platter is the OS's promise, not something a unit test can observe; that
the calls report success on a healthy handle is what is checked here.)
*/
static struct d_test_object*
d_tests_file_durability
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;
    const char*           scratch;

    bool flush_ok = false;
    bool sync_ok  = false;

    idx     = 0;
    scratch = d_tests_file_scratch();

    {
        djinterp::file  f(djinterp::path(scratch), "wb");
        djinterp::error ec;

        f.write("data", 4, ec);
        flush_ok = f.flush(ec) && !ec.failed();
        sync_ok  = f.sync(ec)  && !ec.failed();
        f.close(ec);
    }

    items[idx++] = D_ASSERT_TRUE("flush", flush_ok, "flush pushes stdio's buffer to the OS");
    items[idx++] = D_ASSERT_TRUE("sync",  sync_ok,  "sync pushes the OS's buffer to the device");

    (void)remove(scratch);

    return d_tests_file_group("durability", items, idx);
}


// ===========================================================================
// VI.  ERROR TYPE
// ===========================================================================

/*
d_tests_file_error_type
  djinterp::error in isolation -- the failure channel, before any file touches
it. A pure value type, tested the way path's value assertions are.
  Tests the following:
  - a default error is success; one from a code has failed() and carries it
  - message() is never empty, even for a code strerror does not know
  - clear() returns to success; assign() sets a specific code
  - equality is by code, so two successes are equal and two codes differ
*/
static struct d_test_object*
d_tests_file_error_type
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    {
        djinterp::error ok;

        items[idx++] = D_ASSERT_FALSE("default_ok",
                                      ok.failed(),
                                      "a default error is success");
        items[idx++] = D_ASSERT_EQUAL("default_value",
                                      ok.value(), 0,
                                      "...its value is 0");
    }
    {
        djinterp::error e(EACCES);

        items[idx++] = D_ASSERT_TRUE("from_code_failed",
                                     e.failed(),
                                     "an error from a code has failed()");
        items[idx++] = D_ASSERT_EQUAL("from_code_value",
                                      e.value(), EACCES,
                                      "...and carries that code");
        items[idx++] = D_ASSERT_TRUE("message_nonempty",
                                     e.message() != 0 && e.message()[0] != '\0',
                                     "message() is never empty");
    }
    {
        djinterp::error e(EINVAL);

        e.clear();
        items[idx++] = D_ASSERT_FALSE("clear_resets",
                                      e.failed(),
                                      "clear() returns to success");
    }
    {
        djinterp::error e;

        e.assign(ENOENT);
        items[idx++] = D_ASSERT_EQUAL("assign_sets",
                                      e.value(), ENOENT,
                                      "assign() sets a specific code");
    }
    {
        items[idx++] = D_ASSERT_TRUE("equal_same_code",
                                     djinterp::error(EIO) == djinterp::error(EIO),
                                     "same code compares equal");
        items[idx++] = D_ASSERT_TRUE("equal_success",
                                     djinterp::error() == djinterp::error(),
                                     "two successes are equal");
        items[idx++] = D_ASSERT_TRUE("unequal_diff_code",
                                     djinterp::error(EIO) != djinterp::error(EACCES),
                                     "different codes are unequal");
    }

    return d_tests_file_group("error type", items, idx);
}


// ===========================================================================
// VII. FAILURE PATHS
// ===========================================================================

/*
d_tests_file_failure
  The operations that fail, and the exact code each reports. This is where the
error type earns its place: every one of these would otherwise be a bare int
the caller has to remember the meaning of.
  Tests the following:
  - open of a missing path fails with ENOENT and opens nothing
  - open of an INVALID path is EINVAL -- rejected, not handed to fopen as ""
  - read / seek / tell on a closed handle are each EBADF
*/
static struct d_test_object*
d_tests_file_failure
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    {
        djinterp::file  f;
        djinterp::error ec;
        bool            ok = f.open(djinterp::path("/no/such/dir/nope.bin"), "rb", ec);

        items[idx++] = D_ASSERT_FALSE("open_missing",
                                      ok,
                                      "opening a missing path fails");
        items[idx++] = D_ASSERT_EQUAL("open_missing_enoent",
                                      ec.value(), ENOENT,
                                      "...reported as ENOENT");
        items[idx++] = D_ASSERT_FALSE("not_open_after_fail",
                                      f.is_open(),
                                      "...and nothing is opened");
    }
    {
        djinterp::file  f;
        djinterp::error ec;
        bool            ok = f.open(djinterp::path((const char*)0), "rb", ec);

        items[idx++] = D_ASSERT_FALSE("open_invalid_path",
                                      ok,
                                      "opening an invalid path fails");
        items[idx++] = D_ASSERT_EQUAL("open_invalid_einval",
                                      ec.value(), EINVAL,
                                      "...as EINVAL, not passed to fopen as \"\"");
    }
    {
        djinterp::file  f;
        djinterp::error ec;
        char            b[4];
        size_t          n = f.read(b, sizeof(b), ec);

        items[idx++] = D_ASSERT_TRUE("read_closed_ebadf",
                                     n == (size_t)0 && ec.value() == EBADF,
                                     "reading a closed handle is EBADF");
    }
    {
        djinterp::file  f;
        djinterp::error ec;
        bool            ok = f.seek(0, SEEK_SET, ec);

        items[idx++] = D_ASSERT_TRUE("seek_closed_ebadf",
                                     !ok && ec.value() == EBADF,
                                     "seeking a closed handle is EBADF");
    }
    {
        djinterp::file  f;
        djinterp::error ec;
        d_off_t         pos = f.tell(ec);

        items[idx++] = D_ASSERT_TRUE("tell_closed_ebadf",
                                     pos == (d_off_t)-1 && ec.value() == EBADF,
                                     "telling a closed handle is EBADF");
    }

    return d_tests_file_group("failure paths", items, idx);
}


// ===========================================================================
// VIII. OWNERSHIP
// ===========================================================================

/*
d_tests_file_ownership
  The move handoff -- a C++11 ADDITION, absent on C++98 (the group is smaller
there and its name says so, not a green skip).
  Non-copyability is NOT here: the copy operations are deleted, so a program
that copies a file does not compile, and there is no run-time behaviour to
assert. That guarantee is checked by a negative-compile test outside this file.
  Tests the following (C++11+):
  - move-construction gives the stream to the target and empties the source,
    and the moved-to file still works
  - move-assignment does the same
*/
static struct d_test_object*
d_tests_file_ownership
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

#if (D_MOVE_ENABLED == 1)
    {
        const char* scratch = d_tests_file_scratch();

        {
            djinterp::file  f(djinterp::path(scratch), "wb");
            djinterp::error ec;

            f.write("hello", 5, ec);
            f.close(ec);
        }
        {
            djinterp::error ec;
            djinterp::file  a(djinterp::path(scratch), "rb");
            FILE*           raw = a.native_handle();
            djinterp::file  b(static_cast<djinterp::file&&>(a));
            char            buf[8];

            items[idx++] = D_ASSERT_TRUE("move_ctor_target",
                                         b.is_open() && b.native_handle() == raw,
                                         "move gives the stream to the target");
            items[idx++] = D_ASSERT_FALSE("move_ctor_source",
                                          a.is_open(),
                                          "...and leaves the source owning nothing");
            items[idx++] = D_ASSERT_TRUE("move_ctor_reads",
                                         b.read(buf, 4, ec) == (size_t)4 && !ec.failed(),
                                         "...the moved-to file works");
        }
        {
            djinterp::file a(djinterp::path(scratch), "rb");
            djinterp::file b;

            b = static_cast<djinterp::file&&>(a);
            items[idx++] = D_ASSERT_TRUE("move_assign_target",
                                         b.is_open(),
                                         "move-assign gives the stream to the target");
            items[idx++] = D_ASSERT_FALSE("move_assign_source",
                                          a.is_open(),
                                          "...and empties the source");
        }

        (void)remove(scratch);
    }
#endif

    return d_tests_file_group(
               (D_MOVE_ENABLED == 1) ?
                   "ownership (+ C++11 move)" :
                   "ownership (non-copyable; move is C++11+)",
               items,
               idx);
}


// ===========================================================================
// IX.  DESCRIPTOR & LOCKING   (Phase 4)
// ===========================================================================

/*
d_tests_file_desc_lock
  The fd bridge and advisory locking -- both available on every tier (they are
methods, not a language feature), so this group is the same size everywhere.
  Tests the following:
  - descriptor() borrows a valid fd from an open file, and is EBADF when closed
  - an exclusive lock then unlock succeed; try_lock succeeds when uncontended;
    locking a closed handle is EBADF
  - the lock actually LOCKS: a second, independent handle to the same file
    cannot take the lock the first holds (false + EWOULDBLOCK, not a real error)
*/
static struct d_test_object*
d_tests_file_desc_lock
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;
    const char*           scratch;

    bool fd_open      = false;
    bool fd_closed    = false;
    bool lock_ok      = false;
    bool unlock_ok    = false;
    bool trylock_ok   = false;
    bool lock_closed  = false;
    bool contended    = false;

    idx     = 0;
    scratch = d_tests_file_scratch();

    {
        djinterp::file  f(djinterp::path(scratch), "wb");
        djinterp::error ec;

        f.write("x", 1, ec);
        f.close(ec);
    }

    {
        djinterp::file  f(djinterp::path(scratch), "rb");
        djinterp::error ec;
        int             fd = f.descriptor(ec);

        fd_open = (fd >= 0) && !ec.failed();
    }
    {
        djinterp::file  f;
        djinterp::error ec;
        int             fd = f.descriptor(ec);

        fd_closed = (fd == -1) && (ec.value() == EBADF);
    }

    {
        djinterp::file  f(djinterp::path(scratch), "rb");
        djinterp::error ec;

        lock_ok    = f.lock_exclusive(ec) && !ec.failed();
        unlock_ok  = f.unlock(ec) && !ec.failed();
        trylock_ok = f.try_lock_exclusive(ec) && !ec.failed();   // uncontended now
        f.unlock(ec);
    }
    {
        djinterp::file  f;
        djinterp::error ec;

        lock_closed = !f.lock_exclusive(ec) && (ec.value() == EBADF);
    }

    // the lock is real: two independent open descriptions of the same file
    // contend even inside one process (flock binds the open file description).
    {
        djinterp::file  a(djinterp::path(scratch), "rb");
        djinterp::file  b(djinterp::path(scratch), "rb");
        djinterp::error ea;
        djinterp::error eb;
        bool            got_a = a.lock_exclusive(ea);
        bool            got_b = b.try_lock_exclusive(eb);        // must fail: a holds it

        contended = got_a && !got_b &&
                    (eb.value() == EWOULDBLOCK || eb.value() == EAGAIN);
        a.unlock(ea);
    }

    items[idx++] = D_ASSERT_TRUE("descriptor_open",       fd_open,     "descriptor() borrows a valid fd from an open file");
    items[idx++] = D_ASSERT_TRUE("descriptor_closed",     fd_closed,   "...and is -1 + EBADF on a closed file");
    items[idx++] = D_ASSERT_TRUE("lock_exclusive",        lock_ok,     "an exclusive lock on an open file succeeds");
    items[idx++] = D_ASSERT_TRUE("unlock",                unlock_ok,   "...and unlock releases it");
    items[idx++] = D_ASSERT_TRUE("try_lock_uncontended",  trylock_ok,  "try_lock succeeds when no one else holds it");
    items[idx++] = D_ASSERT_TRUE("lock_closed_ebadf",     lock_closed, "locking a closed handle is EBADF");
    items[idx++] = D_ASSERT_TRUE("lock_contention",       contended,   "a held exclusive lock blocks another handle's try_lock");

    (void)remove(scratch);

    return d_tests_file_group("descriptor & advisory locking", items, idx);
}


// ===========================================================================
// X.   STATUS (fstat)   (Phase 5)
// ===========================================================================

/*
d_tests_file_status
  file::status() is the TOCTOU-free metadata query -- an fstat on the open
descriptor, describing the very file the handle holds rather than whatever a
path might resolve to. All-tier, like the other methods.
  Tests the following:
  - status() on an open regular file is type_regular, exists, and sized to the
    bytes written
  - status() on a closed handle is EBADF and yields an empty (type_none) status
*/
static struct d_test_object*
d_tests_file_status
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;
    const char*           scratch;

    bool st_regular = false;
    bool st_size    = false;
    bool st_exists  = false;
    bool st_closed  = false;

    idx     = 0;
    scratch = d_tests_file_scratch();

    {
        djinterp::file  f(djinterp::path(scratch), "wb");
        djinterp::error ec;

        f.write("0123456789", 10, ec);   // 10 bytes
        f.close(ec);
    }
    {
        djinterp::file        f(djinterp::path(scratch), "rb");
        djinterp::error       ec;
        djinterp::file_status s = f.status(ec);

        st_regular = s.is_regular_file() && !ec.failed();
        st_size    = (s.size() == (uint64_t)10);
        st_exists  = s.exists();
    }
    {
        djinterp::file        f;
        djinterp::error       ec;
        djinterp::file_status s = f.status(ec);

        st_closed = (ec.value() == EBADF) &&
                    (s.type() == djinterp::file_status::type_none);
    }

    items[idx++] = D_ASSERT_TRUE("fstat_regular", st_regular, "status() of an open file is type_regular");
    items[idx++] = D_ASSERT_TRUE("fstat_size",    st_size,    "...sized to the bytes written");
    items[idx++] = D_ASSERT_TRUE("fstat_exists",  st_exists,  "...and exists()");
    items[idx++] = D_ASSERT_TRUE("fstat_closed",  st_closed,  "status() on a closed handle is EBADF / type_none");

    (void)remove(scratch);

    return d_tests_file_group("status (fstat on the descriptor)", items, idx);
}


// ===========================================================================
// XI.  TEMP   (Phase 8)
// ===========================================================================

/*
d_tests_file_temp
  open_temp() acquires an anonymous auto-deleting scratch file that behaves like
any other open file, and temp_directory_path() reports where the system keeps
temporaries. Both all-tier.
  Tests the following:
  - open_temp() opens, and the temp file round-trips bytes like a normal file
  - temp_directory_path() returns a valid, non-empty path
*/
static struct d_test_object*
d_tests_file_temp
(
    void
)
{
    struct d_test_object* items[D_TEST_FILE_MAX_ASSERTS];
    size_t                idx;

    bool temp_opens = false;
    bool temp_io    = false;
    bool dir_ok     = false;

    idx = 0;

    {
        djinterp::file  f;
        djinterp::error ec;

        temp_opens = f.open_temp(ec) && f.is_open() && !ec.failed();

        if (temp_opens)
        {
            char   buf[8];
            bool   wrote = (f.write("temp", 4, ec) == (size_t)4) && !ec.failed();
            bool   back  = f.seek(0, SEEK_SET, ec);
            bool   readb = (f.read(buf, 4, ec) == (size_t)4) && !ec.failed();

            temp_io = wrote && back && readb && (std::memcmp(buf, "temp", 4) == 0);
        }
    }
    {
        djinterp::error ec;
        djinterp::path  d = djinterp::temp_directory_path(ec);

        dir_ok = !ec.failed() && d.valid() && (d.size() > 0);
    }

    items[idx++] = D_ASSERT_TRUE("open_temp",        temp_opens, "open_temp() acquires an anonymous scratch file");
    items[idx++] = D_ASSERT_TRUE("temp_roundtrip",   temp_io,    "...that reads back what was written");
    items[idx++] = D_ASSERT_TRUE("temp_directory",   dir_ok,     "temp_directory_path() returns a real path");

    return d_tests_file_group("temp (anonymous file + location)", items, idx);
}


// ===========================================================================
//      MASTER RUNNER
// ===========================================================================

/*
d_tests_file_all
  Builds the whole result tree. The count is derived from what actually ran, so
a group made empty by configuration contributes nothing -- not an empty group,
not a green "skipped" -- exactly as the c/filesys master runner does.
*/
static struct d_test_object*
d_tests_file_all
(
    void
)
{
    struct d_test_object* items[11];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_file_construct();
    items[idx++] = d_tests_file_lifetime();
    items[idx++] = d_tests_file_io();
    items[idx++] = d_tests_file_position();
    items[idx++] = d_tests_file_durability();
    items[idx++] = d_tests_file_error_type();
    items[idx++] = d_tests_file_failure();
    items[idx++] = d_tests_file_ownership();
    items[idx++] = d_tests_file_desc_lock();
    items[idx++] = d_tests_file_status();
    items[idx++] = d_tests_file_temp();

    return d_tests_file_group("djinterp::file (RAII over a FILE*)", items, idx);
}


int
main
(
    void
)
{
    struct d_test_object*       root;
    struct d_tests_file_counter counter;

    counter.passed = 0;
    counter.failed = 0;

    (void)printf("=============================================="
                 "==================================\n");
    (void)printf(" djinterp::file -- standalone test  "
                 "(__cplusplus=%ld, move=%d)\n",
                 (long)__cplusplus,
                 (int)D_MOVE_ENABLED);
    (void)printf("=============================================="
                 "==================================\n\n");

    root = d_tests_file_all();

    if (!root)
    {
        (void)fprintf(stderr, "out of memory building the test tree\n");

        return 2;
    }

    d_tests_file_print(root, 0, &counter);
    d_test_object_free(root);

    (void)printf("\npassed: %lu  failed: %lu\n",
                 (unsigned long)counter.passed,
                 (unsigned long)counter.failed);

    return (counter.failed == 0) ? 0 : 1;
}
