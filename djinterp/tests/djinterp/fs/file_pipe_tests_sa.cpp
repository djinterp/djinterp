/*******************************************************************************
* djinterp [test]                                         file_pipe_tests_sa.cpp
*
*   Unit tests for djinterp::process (inc/djinterp/core/fs/file_pipe.hpp) -- the
* popen-model pipe to a shell-run command. One file for the class, sibling to
* the other suites, same harness and same three rules.
*
*   Unlike the ISO-C-pure suites, this one necessarily runs real shell commands
* (echo, exit, cat, false) -- that IS the facility under test. Every command
* here is a fixed literal, which is also the only safe way to call the class
* (see the SECURITY note in file_pipe.hpp); nothing is built from input.
*
*   process is a resource type (it owns a pipe), so its ladder is NOT flat: the
* move group is added on C++11+. Non-copyability is a compile-time guarantee
* (deleted copy ops), verified by a negative-compile check outside this file.
*
*
* path:      /tests/djinterp/fs/file_pipe_tests_sa.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    STANDALONE HARNESS
      ------------------

I.    READ OUTPUT       read a command's stdout back
      ----------------------------------------------

II.   EXIT STATUS       a command's exit code is reported after close
      ---------------------------------------------------------------

III.  WRITE INPUT       write to a command's stdin
      --------------------------------------------

IV.   FAILURE STATUS    a failing / missing command surfaces as an exit code
      ----------------------------------------------------------------------

V.    LIFECYCLE         double-close, closed-after
      --------------------------------------------

VI.   OWNERSHIP         move handoff (C++11+); non-copyable is compile-time
      ---------------------------------------------------------------------

      master runner + main
*/

// std
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// djinterp
#include "../../../inc/djinterp/core/fs/file_pipe.hpp"


// ===========================================================================
// 0.   STANDALONE HARNESS   (identical in shape to the sibling suites)
// ===========================================================================

#define D_TEST_PROC_MAX_ASSERTS 16

struct d_test_object
{
    bool                   is_leaf;
    const char*            name;
    const char*            message;
    bool                   result;
    size_t                 count;
    struct d_test_object** elements;
};

static struct d_test_object*
d_test_object_new_leaf(const char* _name, const char* _message, bool _result)
{
    struct d_test_object* o = (struct d_test_object*)malloc(sizeof(struct d_test_object));
    if (!o) { return 0; }
    o->is_leaf = true; o->name = _name; o->message = _message;
    o->result = _result; o->count = 0; o->elements = 0;
    return o;
}

static struct d_test_object*
d_test_object_new_interior(const char* _name, size_t _count)
{
    struct d_test_object* o = (struct d_test_object*)malloc(sizeof(struct d_test_object));
    if (!o) { return 0; }
    o->is_leaf = false; o->name = _name; o->message = 0;
    o->result = true; o->count = _count; o->elements = 0;
    if (_count > 0)
    {
        o->elements = (struct d_test_object**)calloc(_count, sizeof(struct d_test_object*));
        if (!o->elements) { free(o); return 0; }
    }
    return o;
}

static void
d_test_object_free(struct d_test_object* _o)
{
    size_t i;
    if (!_o) { return; }
    for (i = 0; i < _o->count; ++i) { d_test_object_free(_o->elements[i]); }
    free(_o->elements); free(_o);
}

#define D_ASSERT_TRUE(_name, _cond, _msg)  d_test_object_new_leaf(_name, _msg, (_cond))
#define D_ASSERT_FALSE(_name, _cond, _msg) d_test_object_new_leaf(_name, _msg, !(_cond))

static struct d_test_object*
d_tests_proc_group(const char* _name, struct d_test_object** _items, size_t _count)
{
    struct d_test_object* g = d_test_object_new_interior(_name, _count);
    size_t i;
    if (!g) { return 0; }
    for (i = 0; i < _count; ++i) { g->elements[i] = _items[i]; }
    return g;
}

struct d_tests_proc_counter { size_t passed; size_t failed; };

static void
d_tests_proc_print(const struct d_test_object* _o, size_t _depth, struct d_tests_proc_counter* _c)
{
    size_t i, pad;
    if (!_o) { return; }
    for (pad = 0; pad < _depth * 2; ++pad) { (void)fputc(' ', stdout); }
    if (_o->is_leaf)
    {
        if (_o->result) { ++_c->passed; } else { ++_c->failed; }
        (void)printf("[%s] %-28s %s\n", _o->result ? "PASS" : "FAIL", _o->name, _o->message ? _o->message : "");
    }
    else
    {
        (void)printf("%s\n", _o->name);
        for (i = 0; i < _o->count; ++i) { d_tests_proc_print(_o->elements[i], _depth + 1, _c); }
    }
}


// ===========================================================================
// I.   READ OUTPUT
// ===========================================================================

/*
d_tests_proc_read
  A command opened for reading yields its standard output.
*/
static struct d_test_object*
d_tests_proc_read(void)
{
    struct d_test_object* items[D_TEST_PROC_MAX_ASSERTS];
    size_t                idx;

    bool is_open   = false;
    bool bool_true = false;
    bool got_out   = false;

    idx = 0;

    {
        djinterp::process p("echo djinterp", "r");
        djinterp::error   ec;
        char              buf[32];
        size_t            n;

        is_open   = p.is_open();
        bool_true = (bool)p;

        std::memset(buf, 0, sizeof(buf));
        n = p.read(buf, sizeof(buf), ec);
        // echo appends a newline
        got_out = (n >= 8) && (std::memcmp(buf, "djinterp", 8) == 0) && !ec.failed();

        p.close(ec);
    }

    items[idx++] = D_ASSERT_TRUE("is_open",     is_open,   "an opened pipe is open");
    items[idx++] = D_ASSERT_TRUE("bool_true",   bool_true, "...and operator bool is true");
    items[idx++] = D_ASSERT_TRUE("reads_stdout", got_out,  "read() returns the command's output");

    return d_tests_proc_group("read output", items, idx);
}


// ===========================================================================
// II.  EXIT STATUS
// ===========================================================================

/*
d_tests_proc_exit
  After close(), the command's exit code is available, and exited() says so.
*/
static struct d_test_object*
d_tests_proc_exit(void)
{
    struct d_test_object* items[D_TEST_PROC_MAX_ASSERTS];
    size_t                idx;

    bool zero_ok = false;
    bool exited  = false;
    bool seven   = false;

    idx = 0;

    {
        djinterp::process p("exit 0", "r");
        djinterp::error   ec;

        bool closed = p.close(ec) && !ec.failed();
        zero_ok = closed && (p.exit_status() == 0);
        exited  = p.exited();
    }
    {
        djinterp::process p("exit 7", "r");
        djinterp::error   ec;

        p.close(ec);
        seven = (p.exit_status() == 7);
    }

    items[idx++] = D_ASSERT_TRUE("exit_zero",   zero_ok, "a clean command exits 0");
    items[idx++] = D_ASSERT_TRUE("exited_flag", exited,  "...and exited() is true after close");
    items[idx++] = D_ASSERT_TRUE("exit_seven",  seven,   "exit 7 is reported as exit_status 7");

    return d_tests_proc_group("exit status", items, idx);
}


// ===========================================================================
// III. WRITE INPUT
// ===========================================================================

/*
d_tests_proc_write
  A command opened for writing receives what we send to its stdin. `cat > file`
copies its input to a file we can then read back.
*/
static struct d_test_object*
d_tests_proc_write(void)
{
    struct d_test_object* items[D_TEST_PROC_MAX_ASSERTS];
    size_t                idx;

    bool wrote_ok = false;

    idx = 0;

    {
        const char*       out = "djinterp_ptest_out.txt";
        djinterp::error   ec;

        {
            djinterp::process p("cat > djinterp_ptest_out.txt", "w");
            wrote_ok = (p.write("piped payload", 13, ec) == (size_t)13) && !ec.failed();
            p.close(ec);
        }

        // read the file cat produced
        {
            char  buf[32];
            FILE* f = fopen(out, "rb");
            if (f)
            {
                size_t n = fread(buf, 1, sizeof(buf), f);
                (void)fclose(f);
                wrote_ok = wrote_ok && (n == 13) && (std::memcmp(buf, "piped payload", 13) == 0);
            }
            else
            {
                wrote_ok = false;
            }
        }

        (void)remove(out);
    }

    items[idx++] = D_ASSERT_TRUE("writes_stdin", wrote_ok, "write() reaches the command's input");

    return d_tests_proc_group("write input", items, idx);
}


// ===========================================================================
// IV.  FAILURE STATUS
// ===========================================================================

/*
d_tests_proc_failure
  A command that runs and fails, or one that cannot be found, is reported as an
EXIT STATUS -- not as a close error. `false` exits 1; a missing command is the
shell's 127.
*/
static struct d_test_object*
d_tests_proc_failure(void)
{
    struct d_test_object* items[D_TEST_PROC_MAX_ASSERTS];
    size_t                idx;

    bool false_one   = false;
    bool notfound    = false;
    bool close_clean = false;

    idx = 0;

    {
        djinterp::process p("false", "r");
        djinterp::error   ec;

        close_clean = p.close(ec) && !ec.failed();   // the reap itself succeeds
        false_one   = (p.exit_status() == 1);
    }
    {
        djinterp::process p("this_command_does_not_exist_xyz", "r");
        djinterp::error   ec;

        p.close(ec);
        notfound = (p.exit_status() == 127);
    }

    items[idx++] = D_ASSERT_TRUE("false_exits_1",   false_one,   "`false` is reported as exit_status 1");
    items[idx++] = D_ASSERT_TRUE("close_not_error", close_clean, "...and a non-zero exit is NOT a close error");
    items[idx++] = D_ASSERT_TRUE("missing_is_127",  notfound,    "a missing command is the shell's 127");

    return d_tests_proc_group("failure status", items, idx);
}


// ===========================================================================
// V.   LIFECYCLE
// ===========================================================================

/*
d_tests_proc_lifecycle
  Closing is idempotent and leaves the handle not-open.
*/
static struct d_test_object*
d_tests_proc_lifecycle(void)
{
    struct d_test_object* items[D_TEST_PROC_MAX_ASSERTS];
    size_t                idx;

    bool closed_after = false;
    bool double_close = false;

    idx = 0;

    {
        djinterp::process p("echo x", "r");
        djinterp::error   ec;

        p.close(ec);
        closed_after = !p.is_open();
        double_close = p.close(ec) && !ec.failed();
    }

    items[idx++] = D_ASSERT_TRUE("closed_after", closed_after, "the handle is not open after close");
    items[idx++] = D_ASSERT_TRUE("double_close", double_close, "closing an already-closed handle is success");

    return d_tests_proc_group("lifecycle", items, idx);
}


// ===========================================================================
// VI.  OWNERSHIP
// ===========================================================================

/*
d_tests_proc_ownership
  The move handoff -- C++11+, ADDITIVE. Non-copyability is compile-time (deleted
copy ops) and is verified by a negative-compile check outside this file.
*/
static struct d_test_object*
d_tests_proc_ownership(void)
{
    struct d_test_object* items[D_TEST_PROC_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

#if (D_MOVE_ENABLED == 1)
    {
        djinterp::error ec;
        djinterp::process a("echo moved", "r");
        FILE*             raw = a.native_handle();
        djinterp::process b(static_cast<djinterp::process&&>(a));
        char              buf[16];

        items[idx++] = D_ASSERT_TRUE("move_ctor_target",
                                     b.is_open() && b.native_handle() == raw,
                                     "move gives the pipe to the target");
        items[idx++] = D_ASSERT_FALSE("move_ctor_source",
                                      a.is_open(),
                                      "...and leaves the source owning nothing");

        std::memset(buf, 0, sizeof(buf));
        items[idx++] = D_ASSERT_TRUE("move_ctor_reads",
                                     b.read(buf, sizeof(buf), ec) >= (size_t)5 && !ec.failed(),
                                     "...the moved-to pipe reads the output");
        b.close(ec);
    }
    {
        djinterp::error   ec;
        djinterp::process a("echo again", "r");
        djinterp::process b;

        b = static_cast<djinterp::process&&>(a);
        items[idx++] = D_ASSERT_TRUE("move_assign_target",
                                     b.is_open(),
                                     "move-assign gives the pipe to the target");
        items[idx++] = D_ASSERT_FALSE("move_assign_source",
                                      a.is_open(),
                                      "...and empties the source");
        b.close(ec);
    }
#endif

    return d_tests_proc_group(
               (D_MOVE_ENABLED == 1) ?
                   "ownership (+ C++11 move)" :
                   "ownership (non-copyable; move is C++11+)",
               items,
               idx);
}


// ===========================================================================
//      MASTER RUNNER
// ===========================================================================

static struct d_test_object*
d_tests_proc_all(void)
{
    struct d_test_object* items[6];
    size_t                idx = 0;
    items[idx++] = d_tests_proc_read();
    items[idx++] = d_tests_proc_exit();
    items[idx++] = d_tests_proc_write();
    items[idx++] = d_tests_proc_failure();
    items[idx++] = d_tests_proc_lifecycle();
    items[idx++] = d_tests_proc_ownership();
    return d_tests_proc_group("djinterp::process (shell-run command pipe)", items, idx);
}

int
main(void)
{
    struct d_test_object*       root;
    struct d_tests_proc_counter counter;

    counter.passed = 0; counter.failed = 0;
    (void)printf("=== djinterp::process -- standalone test (__cplusplus=%ld, move=%d) ===\n\n",
                 (long)__cplusplus, (int)D_MOVE_ENABLED);

    root = d_tests_proc_all();
    if (!root) { (void)fprintf(stderr, "oom\n"); return 2; }

    d_tests_proc_print(root, 0, &counter);
    d_test_object_free(root);
    (void)printf("\npassed: %lu  failed: %lu\n",
                 (unsigned long)counter.passed, (unsigned long)counter.failed);
    return (counter.failed == 0) ? 0 : 1;
}
