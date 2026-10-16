/*******************************************************************************
* djinterp [test]                                        operations_tests_sa.cpp
*
*   Unit tests for the whole-file operations in inc/djinterp/cpp/operations.hpp
* -- remove, remove_file, rename, copy_file. One file for the module, sibling to
* the path/file/status/directory suites, same harness and same three rules.
*
*   These are FREE FUNCTIONS with no handle, so the tier ladder is FLAT: the
* same source builds identically C++98 -> C++23 with the same count, because
* there is no move to add. As with the status suite, that flat ladder is itself
* the claim -- an operation over paths carries none of the ceremony a handle
* does.
*
*   FIXTURES. Files are made and inspected with fopen/fread/remove (ISO C); an
* empty-directory fixture is made with djinterp's own create_directory (tested
* on its own in the directory suite, used here only as a tool). The test calls
* no unlink/rename/mkdir directly -- it drives the real syscalls only through
* the API under test, keeping the siblings' no-platform-call discipline.
*
*
* path:      /tests/djinterp/fs/operations_tests_sa.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    STANDALONE HARNESS      (self-contained; no framework dependency)
      -----------------------------------------------------------------

I.    REMOVE                  file, empty directory, and the missing-path error
      -------------------------------------------------------------------------

II.   REMOVE_FILE             removes a file; refuses a directory
      -----------------------------------------------------------

III.  RENAME                  moves, preserves content, overwrites
      ------------------------------------------------------------

IV.   COPY_FILE               duplicates content; missing source errors
      -----------------------------------------------------------------

V.    FAILURE                 invalid paths are EINVAL
      ------------------------------------------------

      master runner + main
*/

// std
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// djinterp


// ===========================================================================
// 0.   STANDALONE HARNESS   (identical in shape to the sibling suites)
// ===========================================================================

#define D_TEST_OPS_MAX_ASSERTS 32


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


static struct d_test_object*
d_tests_ops_group
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


struct d_tests_ops_counter
{
    size_t passed;
    size_t failed;
};


static void
d_tests_ops_print
(
    const struct d_test_object* _obj,
    size_t                      _depth,
    struct d_tests_ops_counter* _counter
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
            d_tests_ops_print(_obj->elements[idx], _depth + 1, _counter);
        }
    }

    return;
}


// --- fixtures / inspection (ISO C) ---

// d_tests_ops_name
//   function: a distinct scratch filename per call. Rotates a small set of
// static buffers so two names held at once -- a rename's from and to, say --
// do not alias the same storage (which would rename a file onto itself).
static const char*
d_tests_ops_name(void)
{
    static char bufs[4][64];
    static int  seq  = 0;
    int         slot = seq % 4;

    (void)snprintf(bufs[slot], sizeof(bufs[slot]), "djinterp_otest_%d.tmp", seq);
    ++seq;

    return bufs[slot];
}

// d_tests_ops_write
//   function: create a file holding _text. ISO C.
static void
d_tests_ops_write(const char* _path, const char* _text)
{
    FILE* f = fopen(_path, "wb");

    if (f)
    {
        (void)fputs(_text, f);
        (void)fclose(f);
    }
}

// d_tests_ops_exists
//   function: whether a file can be opened for reading -- our existence probe,
// kept in ISO C so the test does not lean on another module to check the one
// under test.
static bool
d_tests_ops_exists(const char* _path)
{
    FILE* f = fopen(_path, "rb");

    if (f)
    {
        (void)fclose(f);
        return true;
    }

    return false;
}

// d_tests_ops_content_is
//   function: whether a file's bytes equal _text exactly.
static bool
d_tests_ops_content_is(const char* _path, const char* _text)
{
    char   buf[256];
    size_t got;
    size_t want = std::strlen(_text);
    FILE*  f    = fopen(_path, "rb");

    if (!f)
    {
        return false;
    }

    got = fread(buf, 1, sizeof(buf), f);
    (void)fclose(f);

    return (got == want) && (std::memcmp(buf, _text, want) == 0);
}


// ===========================================================================
// I.   REMOVE
// ===========================================================================

/*
d_tests_ops_remove
  remove() takes a file or an empty directory, and reports the missing-path
case as ENOENT.
*/
static struct d_test_object*
d_tests_ops_remove(void)
{
    struct d_test_object* items[D_TEST_OPS_MAX_ASSERTS];
    size_t                idx;

    bool file_gone   = false;
    bool dir_gone    = false;
    bool missing_err = false;

    idx = 0;

    {
        const char*     p = d_tests_ops_name();
        djinterp::error ec;

        d_tests_ops_write(p, "data");
        file_gone = djinterp::remove(djinterp::path(p), ec) &&
                    !ec.failed() &&
                    !d_tests_ops_exists(p);
    }
    {
        const char*     d = "djinterp_otest_dir";
        djinterp::error ec;

        djinterp::create_directory(djinterp::path(d), ec);   // fixture tool
        dir_gone = djinterp::remove(djinterp::path(d), ec) && !ec.failed();
    }
    {
        djinterp::error ec;

        missing_err = !djinterp::remove(djinterp::path("no_such_djinterp_ops.tmp"), ec) &&
                      (ec.value() == ENOENT);
    }

    items[idx++] = D_ASSERT_TRUE("removes_file",     file_gone,   "remove() deletes a file");
    items[idx++] = D_ASSERT_TRUE("removes_empty_dir", dir_gone,   "...and an empty directory");
    items[idx++] = D_ASSERT_TRUE("missing_enoent",   missing_err, "removing a missing path is ENOENT");

    return d_tests_ops_group("remove", items, idx);
}


// ===========================================================================
// II.  REMOVE_FILE
// ===========================================================================

/*
d_tests_ops_remove_file
  remove_file() deletes a file, and refuses a directory -- the narrower promise
that it cannot delete a directory by surprise.
*/
static struct d_test_object*
d_tests_ops_remove_file(void)
{
    struct d_test_object* items[D_TEST_OPS_MAX_ASSERTS];
    size_t                idx;

    bool removed      = false;
    bool refused_dir  = false;

    idx = 0;

    {
        const char*     p = d_tests_ops_name();
        djinterp::error ec;

        d_tests_ops_write(p, "data");
        removed = djinterp::remove_file(djinterp::path(p), ec) &&
                  !ec.failed() &&
                  !d_tests_ops_exists(p);
    }
    {
        const char*     d = "djinterp_otest_dir2";
        djinterp::error ec;

        djinterp::create_directory(djinterp::path(d), ec);
        // remove_file must NOT delete a directory; the exact errno varies by
        // platform (EISDIR / EPERM), so the assertion is "failed", not a code.
        refused_dir = !djinterp::remove_file(djinterp::path(d), ec) && ec.failed();

        djinterp::remove_directory(djinterp::path(d), ec);   // clean up the fixture
    }

    items[idx++] = D_ASSERT_TRUE("removes_file",  removed,     "remove_file() deletes a file");
    items[idx++] = D_ASSERT_TRUE("refuses_dir",   refused_dir, "...and refuses a directory");

    return d_tests_ops_group("remove_file", items, idx);
}


// ===========================================================================
// III. RENAME
// ===========================================================================

/*
d_tests_ops_rename
  rename() moves a file (the source vanishes, the target appears), carries the
content across unchanged, and overwrites an existing target.
*/
static struct d_test_object*
d_tests_ops_rename(void)
{
    struct d_test_object* items[D_TEST_OPS_MAX_ASSERTS];
    size_t                idx;

    bool moved      = false;
    bool content_ok = false;
    bool overwrote  = false;

    idx = 0;

    {
        const char*     from = d_tests_ops_name();
        const char*     to   = d_tests_ops_name();
        djinterp::error ec;

        d_tests_ops_write(from, "payload");
        moved = djinterp::rename(djinterp::path(from), djinterp::path(to), ec) &&
                !ec.failed() &&
                !d_tests_ops_exists(from) &&
                d_tests_ops_exists(to);
        content_ok = d_tests_ops_content_is(to, "payload");

        (void)remove(to);
    }
    {
        const char*     from = d_tests_ops_name();
        const char*     to   = d_tests_ops_name();
        djinterp::error ec;

        d_tests_ops_write(from, "NEW");
        d_tests_ops_write(to,   "old contents");
        djinterp::rename(djinterp::path(from), djinterp::path(to), ec);
        overwrote = !ec.failed() && d_tests_ops_content_is(to, "NEW");

        (void)remove(to);
    }

    items[idx++] = D_ASSERT_TRUE("moves",             moved,      "rename() moves source to target");
    items[idx++] = D_ASSERT_TRUE("preserves_content", content_ok, "...with content intact");
    items[idx++] = D_ASSERT_TRUE("overwrites",        overwrote,  "...overwriting an existing target");

    return d_tests_ops_group("rename", items, idx);
}


// ===========================================================================
// IV.  COPY_FILE
// ===========================================================================

/*
d_tests_ops_copy_file
  copy_file() duplicates content -- both files exist afterward and the copy
matches the source -- and reports a missing source as ENOENT.
*/
static struct d_test_object*
d_tests_ops_copy_file(void)
{
    struct d_test_object* items[D_TEST_OPS_MAX_ASSERTS];
    size_t                idx;

    bool copied     = false;
    bool content_ok = false;
    bool missing    = false;

    idx = 0;

    {
        const char*     src = d_tests_ops_name();
        const char*     dst = d_tests_ops_name();
        djinterp::error ec;

        d_tests_ops_write(src, "the original bytes");
        copied = djinterp::copy_file(djinterp::path(src), djinterp::path(dst), ec) &&
                 !ec.failed() &&
                 d_tests_ops_exists(src) &&
                 d_tests_ops_exists(dst);
        content_ok = d_tests_ops_content_is(dst, "the original bytes");

        (void)remove(src);
        (void)remove(dst);
    }
    {
        const char*     dst = d_tests_ops_name();
        djinterp::error ec;

        missing = !djinterp::copy_file(djinterp::path("no_such_djinterp_src.tmp"),
                                       djinterp::path(dst), ec) &&
                  (ec.value() == ENOENT);
    }

    items[idx++] = D_ASSERT_TRUE("duplicates",       copied,     "copy_file() creates the destination");
    items[idx++] = D_ASSERT_TRUE("content_matches",  content_ok, "...with the source's bytes");
    items[idx++] = D_ASSERT_TRUE("missing_src_enoent", missing,  "a missing source is ENOENT");

    return d_tests_ops_group("copy_file", items, idx);
}


// ===========================================================================
// V.   FAILURE
// ===========================================================================

/*
d_tests_ops_failure
  Every operation rejects an invalid path as EINVAL before touching the
filesystem.
*/
static struct d_test_object*
d_tests_ops_failure(void)
{
    struct d_test_object* items[D_TEST_OPS_MAX_ASSERTS];
    size_t                idx;

    bool rm_inval   = false;
    bool ren_inval  = false;
    bool cp_inval   = false;

    idx = 0;

    {
        djinterp::error ec;
        rm_inval = !djinterp::remove(djinterp::path((const char*)0), ec) &&
                   (ec.value() == EINVAL);
    }
    {
        djinterp::error ec;
        ren_inval = !djinterp::rename(djinterp::path((const char*)0),
                                      djinterp::path("x.tmp"), ec) &&
                    (ec.value() == EINVAL);
    }
    {
        djinterp::error ec;
        cp_inval = !djinterp::copy_file(djinterp::path("x.tmp"),
                                        djinterp::path((const char*)0), ec) &&
                   (ec.value() == EINVAL);
    }

    items[idx++] = D_ASSERT_TRUE("remove_invalid",    rm_inval,  "remove() of an invalid path is EINVAL");
    items[idx++] = D_ASSERT_TRUE("rename_invalid",    ren_inval, "rename() with an invalid path is EINVAL");
    items[idx++] = D_ASSERT_TRUE("copy_file_invalid", cp_inval,  "copy_file() with an invalid path is EINVAL");

    return d_tests_ops_group("failure", items, idx);
}


// ===========================================================================
//      MASTER RUNNER
// ===========================================================================

static struct d_test_object*
d_tests_ops_all(void)
{
    struct d_test_object* items[5];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_ops_remove();
    items[idx++] = d_tests_ops_remove_file();
    items[idx++] = d_tests_ops_rename();
    items[idx++] = d_tests_ops_copy_file();
    items[idx++] = d_tests_ops_failure();

    return d_tests_ops_group("djinterp whole-file operations", items, idx);
}


int
main(void)
{
    struct d_test_object*      root;
    struct d_tests_ops_counter counter;

    counter.passed = 0;
    counter.failed = 0;

    (void)printf("=============================================="
                 "==================================\n");
    (void)printf(" djinterp operations -- standalone test  "
                 "(__cplusplus=%ld)\n",
                 (long)__cplusplus);
    (void)printf("=============================================="
                 "==================================\n\n");

    root = d_tests_ops_all();

    if (!root)
    {
        (void)fprintf(stderr, "out of memory building the test tree\n");

        return 2;
    }

    d_tests_ops_print(root, 0, &counter);
    d_test_object_free(root);

    (void)printf("\npassed: %lu  failed: %lu\n",
                 (unsigned long)counter.passed,
                 (unsigned long)counter.failed);

    return (counter.failed == 0) ? 0 : 1;
}
