/*******************************************************************************
* djinterp [test]                                          file_dir_tests_sa.cpp
*
*   Unit tests for djinterp::directory, its range-for iterator, directory_entry,
* and the free create_directory / remove_directory (inc/djinterp/core/filesys/
* file_dir.hpp). One file for the class, sibling to the path/file/status
* suites, same harness and same three rules.
*
*   FIXTURES. A directory test needs directories, and ISO C cannot make one --
* so the fixtures go through djinterp's own create_directory (also tested on its
* own, in group VI) and fill them with fopen, which IS ISO C. The test itself
* calls no mkdir/opendir/readdir directly; it keeps the no-platform-call
* discipline of its siblings, driving the real syscalls only through the API
* under test. Entry ORDER from a directory is not guaranteed, so every check is
* by COUNT and by NAME-membership, never by position.
*
*   WHAT IS NOT ASSERTED HERE. directory is non-copyable (deleted copy ops) --
* a compile-time guarantee with no run-time behaviour, verified by a negative-
* compile check outside this file, exactly as in the file suite. The move group
* is C++11+ and ADDITIVE; on C++98 it is absent, not a green skip.
*
*
* path:      /tests/djinterp/fs/file_dir_tests_sa.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    STANDALONE HARNESS      (self-contained; no framework dependency)
      -----------------------------------------------------------------

I.    CONSTRUCTION / STATE    default directory owns nothing
      ------------------------------------------------------

II.   READ WALK               open + read() yields every entry, . and .. skipped
      --------------------------------------------------------------------------

III.  RANGE-FOR               the iterator walks the same entries
      -----------------------------------------------------------

IV.   ENTRY                   name is bare; type predicates
      -----------------------------------------------------

V.    REWIND                  a rewound directory walks again
      -------------------------------------------------------

VI.   CREATE / REMOVE         create_directory / remove_directory + their errors
      --------------------------------------------------------------------------

VII.  FAILURE                 invalid path, closed handle
      ---------------------------------------------------

VIII. OWNERSHIP               move handoff (C++11+); non-copyable is compile-time
      ---------------------------------------------------------------------------

      master runner + main
*/

// std
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// djinterp
#include "../../../inc/djinterp/core/filesys/file_dir.hpp"


// ===========================================================================
// 0.   STANDALONE HARNESS   (identical in shape to the sibling suites)
// ===========================================================================

#define D_TEST_DIR_MAX_ASSERTS 32


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


static struct d_test_object*
d_test_object_new_interior(const char* _name, size_t _count)
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


static void
d_test_object_free(struct d_test_object* _obj)
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


#define D_ASSERT_TRUE(_name, _cond, _msg)                                    \
    d_test_object_new_leaf(_name, _msg, (_cond))
#define D_ASSERT_FALSE(_name, _cond, _msg)                                   \
    d_test_object_new_leaf(_name, _msg, !(_cond))
#define D_ASSERT_EQUAL(_name, _a, _b, _msg)                                  \
    d_test_object_new_leaf(_name, _msg, (_a) == (_b))


static struct d_test_object*
d_tests_dir_group
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


struct d_tests_dir_counter
{
    size_t passed;
    size_t failed;
};


static void
d_tests_dir_print
(
    const struct d_test_object* _obj,
    size_t                      _depth,
    struct d_tests_dir_counter* _counter
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
            d_tests_dir_print(_obj->elements[idx], _depth + 1, _counter);
        }
    }

    return;
}


// --- fixtures: a directory of known contents, built through the API ---

// d_tests_dir_child
//   function: join a bare name onto a directory name, into a static buffer, for
// the fopen/remove calls that need a full path. (The directory class joins with
// operator/; here plain formatting keeps the fixture obvious.)
static const char*
d_tests_dir_child(const char* _dir, const char* _name)
{
    static char buf[128];

    (void)snprintf(buf, sizeof(buf), "%s/%s", _dir, _name);

    return buf;
}

// d_tests_dir_touch
//   function: create a one-byte file at a path. ISO C.
static void
d_tests_dir_touch(const char* _path)
{
    FILE* f = fopen(_path, "wb");

    if (f)
    {
        (void)fputc('x', f);
        (void)fclose(f);
    }
}

// d_tests_dir_make
//   function: build a fresh directory holding three files and one subdirectory
// -- four real entries once "." and ".." are skipped -- and return its name.
// The directory is made with the API under test; the files with fopen.
static const char*
d_tests_dir_make(void)
{
    static char     dir[64];
    static int      seq = 0;
    djinterp::error ec;

    (void)snprintf(dir, sizeof(dir), "djinterp_dtest_%d", seq);
    ++seq;

    djinterp::create_directory(djinterp::path(dir), ec);
    d_tests_dir_touch(d_tests_dir_child(dir, "file_a.txt"));
    d_tests_dir_touch(d_tests_dir_child(dir, "file_b.txt"));
    d_tests_dir_touch(d_tests_dir_child(dir, "file_c.txt"));
    djinterp::create_directory(djinterp::path(d_tests_dir_child(dir, "sub")), ec);

    return dir;
}

// d_tests_dir_teardown
//   function: undo d_tests_dir_make. remove() is ISO C; the subdirectory and
// the now-empty directory go through remove_directory.
static void
d_tests_dir_teardown(const char* _dir)
{
    djinterp::error ec;

    (void)remove(d_tests_dir_child(_dir, "file_a.txt"));
    (void)remove(d_tests_dir_child(_dir, "file_b.txt"));
    (void)remove(d_tests_dir_child(_dir, "file_c.txt"));
    djinterp::remove_directory(djinterp::path(d_tests_dir_child(_dir, "sub")), ec);
    djinterp::remove_directory(djinterp::path(_dir), ec);
}

// d_tests_dir_collect
//   function: walk a directory with read(), copying every entry into _out (each
// entry owns its name, so this is safe), and return how many there were.
static size_t
d_tests_dir_collect(const char* _dir, djinterp::directory_entry* _out, size_t _max)
{
    djinterp::error         ec;
    djinterp::directory     d(djinterp::path(_dir), ec);
    djinterp::directory_entry e;
    size_t                  n = 0;

    while (n < _max && d.read(e, ec))
    {
        _out[n] = e;
        ++n;
    }

    return n;
}

// d_tests_dir_find
//   function: index of the entry with this name, or _n if absent.
static size_t
d_tests_dir_find(const djinterp::directory_entry* _a, size_t _n, const char* _name)
{
    size_t i;

    for (i = 0; i < _n; ++i)
    {
        if (std::strcmp(_a[i].name().c_str(), _name) == 0)
        {
            return i;
        }
    }

    return _n;
}


// ===========================================================================
// I.   CONSTRUCTION / STATE
// ===========================================================================

/*
d_tests_dir_construct
  A default directory owns nothing and carries no error.
*/
static struct d_test_object*
d_tests_dir_construct(void)
{
    struct d_test_object* items[D_TEST_DIR_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    {
        djinterp::directory d;

        items[idx++] = D_ASSERT_FALSE("default_not_open", d.is_open(),  "a default directory owns nothing");
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        // operator bool is C++11's (decision 3.2); is_open() serves below
        items[idx++] = D_ASSERT_FALSE("default_bool",     (bool)d,      "...so operator bool is false");
#endif
        items[idx++] = D_ASSERT_FALSE("default_no_error", d.failed(),   "...and it carries no error");
    }

    return d_tests_dir_group("construction / state", items, idx);
}


// ===========================================================================
// II.  READ WALK
// ===========================================================================

/*
d_tests_dir_read
  open() then read() yields every real entry, with "." and ".." skipped, and
ends cleanly (read() returns false with a clear error at the end).
*/
static struct d_test_object*
d_tests_dir_read(void)
{
    struct d_test_object* items[D_TEST_DIR_MAX_ASSERTS];
    size_t                idx;
    const char*           dir;

    bool   open_ok    = false;
    size_t count      = 0;
    bool   no_dots    = true;
    bool   clean_end  = false;

    idx = 0;
    dir = d_tests_dir_make();

    {
        djinterp::error           ec;
        djinterp::directory       d(djinterp::path(dir), ec);
        djinterp::directory_entry e;

        open_ok = d.is_open() && !ec.failed();

        while (d.read(e, ec))
        {
            if (std::strcmp(e.name().c_str(), ".") == 0 ||
                std::strcmp(e.name().c_str(), "..") == 0)
            {
                no_dots = false;
            }

            ++count;
        }

        clean_end = !ec.failed();   // the loop ended, and not on an error
    }

    items[idx++] = D_ASSERT_TRUE("open",           open_ok,          "the directory opens");
    items[idx++] = D_ASSERT_EQUAL("entry_count",   count, (size_t)4, "read() yields all four real entries");
    items[idx++] = D_ASSERT_TRUE("skips_dot",      no_dots,          "\".\" and \"..\" are skipped");
    items[idx++] = D_ASSERT_TRUE("clean_end",      clean_end,        "the walk ends with a clear error, not a failure");

    d_tests_dir_teardown(dir);

    return d_tests_dir_group("read walk", items, idx);
}


// ===========================================================================
// III. RANGE-FOR
// ===========================================================================

/*
d_tests_dir_rangefor
  The iterator walks the same set the read() loop does. A known name is present,
entry names are bare (no separator), and last_error() is clear after a clean
loop.
*/
static struct d_test_object*
d_tests_dir_rangefor(void)
{
    struct d_test_object* items[D_TEST_DIR_MAX_ASSERTS];
    size_t                idx;
    const char*           dir;

    size_t count      = 0;
    bool   found_a    = false;
    bool   names_bare = true;
    bool   no_error   = false;

    idx = 0;
    dir = d_tests_dir_make();

    {
        djinterp::error     ec;
        djinterp::directory d(djinterp::path(dir), ec);

        for (djinterp::directory_iterator it = d.begin(); it != d.end(); ++it)
        {
            const djinterp::directory_entry& e = *it;

            if (std::strcmp(e.name().c_str(), "file_a.txt") == 0)
            {
                found_a = true;
            }

            if (std::strchr(e.name().c_str(), '/') != 0)
            {
                names_bare = false;   // a directory entry name is a filename, not a path
            }

            ++count;
        }

        no_error = !d.failed();
    }

    items[idx++] = D_ASSERT_EQUAL("rangefor_count", count, (size_t)4, "range-for walks all four entries");
    items[idx++] = D_ASSERT_TRUE("found_known",     found_a,          "a known entry is present");
    items[idx++] = D_ASSERT_TRUE("names_bare",      names_bare,       "an entry name is bare, not a path");
    items[idx++] = D_ASSERT_TRUE("no_error_after",  no_error,         "last_error() is clear after a clean walk");

    d_tests_dir_teardown(dir);

    return d_tests_dir_group("range-for", items, idx);
}


// ===========================================================================
// IV.  ENTRY
// ===========================================================================

/*
d_tests_dir_entry
  A file entry reports as a regular file and the subdirectory as a directory --
WHEN the filesystem reported the type in the readdir (type_known()). If it did
not, those two checks are skipped rather than failed, because the type is then
genuinely unavailable without a stat -- omit, do not fake.
*/
static struct d_test_object*
d_tests_dir_entry(void)
{
    struct d_test_object*     items[D_TEST_DIR_MAX_ASSERTS];
    size_t                    idx;
    const char*               dir;
    djinterp::directory_entry set[16];
    size_t                    n;
    size_t                    fi;
    size_t                    si;

    idx = 0;
    dir = d_tests_dir_make();
    n   = d_tests_dir_collect(dir, set, 16);

    fi = d_tests_dir_find(set, n, "file_a.txt");
    si = d_tests_dir_find(set, n, "sub");

    items[idx++] = D_ASSERT_TRUE("file_present", fi < n, "the file entry is found");
    items[idx++] = D_ASSERT_TRUE("sub_present",  si < n, "the subdirectory entry is found");

    if (fi < n && set[fi].type_known())
    {
        items[idx++] = D_ASSERT_TRUE("file_is_regular",
                                     set[fi].is_regular_file(),
                                     "the file entry is a regular file");
    }
    if (si < n && set[si].type_known())
    {
        items[idx++] = D_ASSERT_TRUE("sub_is_directory",
                                     set[si].is_directory(),
                                     "the subdirectory entry is a directory");
    }

    d_tests_dir_teardown(dir);

    return d_tests_dir_group("entry", items, idx);
}


// ===========================================================================
// V.   REWIND
// ===========================================================================

/*
d_tests_dir_rewind
  A directory is single-pass, but rewind() starts it over: two walks of one
handle, with a rewind between, see the same four entries.
*/
static struct d_test_object*
d_tests_dir_rewind(void)
{
    struct d_test_object* items[D_TEST_DIR_MAX_ASSERTS];
    size_t                idx;
    const char*           dir;

    size_t first  = 0;
    size_t second = 0;
    bool   rewound = false;

    idx = 0;
    dir = d_tests_dir_make();

    {
        djinterp::error           ec;
        djinterp::directory       d(djinterp::path(dir), ec);
        djinterp::directory_entry e;

        while (d.read(e, ec)) { ++first; }

        rewound = d.rewind(ec) && !ec.failed();

        while (d.read(e, ec)) { ++second; }
    }

    items[idx++] = D_ASSERT_EQUAL("first_walk",  first,  (size_t)4, "the first walk sees four entries");
    items[idx++] = D_ASSERT_TRUE("rewind",       rewound,           "rewind() succeeds");
    items[idx++] = D_ASSERT_EQUAL("second_walk", second, (size_t)4, "...and the second walk sees them again");

    d_tests_dir_teardown(dir);

    return d_tests_dir_group("rewind", items, idx);
}


// ===========================================================================
// VI.  CREATE / REMOVE
// ===========================================================================

/*
d_tests_dir_create_remove
  create_directory makes one and remove_directory removes an empty one, and the
error paths report the exact reason: making one that exists is EEXIST, removing
a non-empty one is ENOTEMPTY, opening a missing one is ENOENT.
*/
static struct d_test_object*
d_tests_dir_create_remove(void)
{
    struct d_test_object* items[D_TEST_DIR_MAX_ASSERTS];
    size_t                idx;

    bool create_ok   = false;
    bool remove_ok   = false;
    bool exists_err  = false;
    bool nonempty    = false;
    bool missing_err = false;

    idx = 0;

    {
        djinterp::error ec;
        create_ok = djinterp::create_directory(djinterp::path("djinterp_dtest_cr"), ec) && !ec.failed();
    }
    {
        djinterp::error ec;
        djinterp::create_directory(djinterp::path("djinterp_dtest_cr"), ec);   // again
        exists_err = (ec.value() == EEXIST);
    }
    {
        djinterp::error ec;
        remove_ok = djinterp::remove_directory(djinterp::path("djinterp_dtest_cr"), ec) && !ec.failed();
    }
    {
        // a non-empty directory cannot be removed
        const char*     dir = d_tests_dir_make();
        djinterp::error ec;

        djinterp::remove_directory(djinterp::path(dir), ec);
        nonempty = (ec.value() == ENOTEMPTY);

        d_tests_dir_teardown(dir);
    }
    {
        djinterp::error     ec;
        djinterp::directory d(djinterp::path("no_such_djinterp_dir"), ec);

        missing_err = !d.is_open() && (ec.value() == ENOENT);
    }

    items[idx++] = D_ASSERT_TRUE("create",           create_ok,   "create_directory makes a directory");
    items[idx++] = D_ASSERT_TRUE("remove",           remove_ok,   "remove_directory removes an empty one");
    items[idx++] = D_ASSERT_TRUE("create_existing",  exists_err,  "creating an existing directory is EEXIST");
    items[idx++] = D_ASSERT_TRUE("remove_nonempty",  nonempty,    "removing a non-empty directory is ENOTEMPTY");
    items[idx++] = D_ASSERT_TRUE("open_missing",     missing_err, "opening a missing directory is ENOENT");

    return d_tests_dir_group("create / remove", items, idx);
}


// ===========================================================================
// VII. FAILURE
// ===========================================================================

/*
d_tests_dir_failure
  An invalid path is EINVAL; reading a closed handle is EBADF.
*/
static struct d_test_object*
d_tests_dir_failure(void)
{
    struct d_test_object* items[D_TEST_DIR_MAX_ASSERTS];
    size_t                idx;

    bool einval = false;
    bool ebadf  = false;

    idx = 0;

    {
        djinterp::error     ec;
        djinterp::directory d(djinterp::path((const char*)0), ec);

        einval = !d.is_open() && (ec.value() == EINVAL);
    }
    {
        djinterp::directory       d;   // never opened
        djinterp::error           ec;
        djinterp::directory_entry e;

        ebadf = !d.read(e, ec) && (ec.value() == EBADF);
    }

    items[idx++] = D_ASSERT_TRUE("open_invalid",  einval, "an invalid path is EINVAL");
    items[idx++] = D_ASSERT_TRUE("read_closed",   ebadf,  "reading a closed handle is EBADF");

    return d_tests_dir_group("failure", items, idx);
}


// ===========================================================================
// VIII. OWNERSHIP
// ===========================================================================

/*
d_tests_dir_ownership
  The move handoff -- C++11+, ADDITIVE. Non-copyability is compile-time (deleted
copy ops; verified by a negative-compile check elsewhere) and is not asserted
here.
*/
static struct d_test_object*
d_tests_dir_ownership(void)
{
    struct d_test_object* items[D_TEST_DIR_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

#if (D_MOVE_ENABLED == 1)
    {
        const char* dir = d_tests_dir_make();

        {
            djinterp::error     ec;
            djinterp::directory a(djinterp::path(dir), ec);
            djinterp::directory b(static_cast<djinterp::directory&&>(a));
            djinterp::directory_entry e;

            items[idx++] = D_ASSERT_TRUE("move_ctor_target",
                                         b.is_open(),
                                         "move gives the handle to the target");
            items[idx++] = D_ASSERT_FALSE("move_ctor_source",
                                          a.is_open(),
                                          "...and leaves the source owning nothing");
            items[idx++] = D_ASSERT_TRUE("move_ctor_reads",
                                         b.read(e, ec),
                                         "...the moved-to directory walks");
        }
        {
            djinterp::error     ec;
            djinterp::directory a(djinterp::path(dir), ec);
            djinterp::directory b;

            b = static_cast<djinterp::directory&&>(a);
            items[idx++] = D_ASSERT_TRUE("move_assign_target",
                                         b.is_open(),
                                         "move-assign gives the handle to the target");
            items[idx++] = D_ASSERT_FALSE("move_assign_source",
                                          a.is_open(),
                                          "...and empties the source");
        }

        d_tests_dir_teardown(dir);
    }
#endif

    return d_tests_dir_group(
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
d_tests_dir_all(void)
{
    struct d_test_object* items[8];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_dir_construct();
    items[idx++] = d_tests_dir_read();
    items[idx++] = d_tests_dir_rangefor();
    items[idx++] = d_tests_dir_entry();
    items[idx++] = d_tests_dir_rewind();
    items[idx++] = d_tests_dir_create_remove();
    items[idx++] = d_tests_dir_failure();
    items[idx++] = d_tests_dir_ownership();

    return d_tests_dir_group("djinterp::directory (a walkable handle)", items, idx);
}


int
main(void)
{
    struct d_test_object*      root;
    struct d_tests_dir_counter counter;

    counter.passed = 0;
    counter.failed = 0;

    (void)printf("=============================================="
                 "==================================\n");
    (void)printf(" djinterp::directory -- standalone test  "
                 "(__cplusplus=%ld, move=%d)\n",
                 (long)__cplusplus,
                 (int)D_MOVE_ENABLED);
    (void)printf("=============================================="
                 "==================================\n\n");

    root = d_tests_dir_all();

    if (!root)
    {
        (void)fprintf(stderr, "out of memory building the test tree\n");

        return 2;
    }

    d_tests_dir_print(root, 0, &counter);
    d_test_object_free(root);

    (void)printf("\npassed: %lu  failed: %lu\n",
                 (unsigned long)counter.passed,
                 (unsigned long)counter.failed);

    return (counter.failed == 0) ? 0 : 1;
}
