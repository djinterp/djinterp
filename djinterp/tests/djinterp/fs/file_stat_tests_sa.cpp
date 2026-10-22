/*******************************************************************************
* djinterp [test]                                         file_stat_tests_sa.cpp
*
*   Unit tests for djinterp::file_status (inc/djinterp/core/filesys/file_stat.hpp) and the
* free queries over it -- status, symlink_status, exists, is_regular_file,
* is_directory, file_size. One file for the class, sibling to path_tests_sa.cpp
* and file_tests_sa.cpp, same harness and same three rules.
*
*   file_status is a VALUE, not a resource -- a captured snapshot with no
* handle. So its tier ladder is FLAT: nothing is added on a higher standard,
* because there is no move to add. The suite still builds it on every standard
* (rule 3), and the count is the same every time -- which is exactly the claim
* being demonstrated, that a snapshot needs none of the ceremony `file` does.
*
*   WHAT THIS FILE CANNOT RUN, AND SAYS SO.
*      The difference between status() and symlink_status() shows ONLY on an
*   actual symlink -- one follows the final link, the other does not. Creating a
*   symlink is not something ISO C can do, and this file keeps the no-platform-
*   call discipline of its siblings (fixtures are a regular file via fopen, an
*   always-present directory ".", and an absent path). So the symlink_status
*   group here exercises its CODE PATH (the no-follow stat) on non-symlinks and
*   confirms it agrees with status there; the follow-vs-no-follow DIFFERENCE on
*   a real symlink is verified in the full tree, not faked with a green leaf
*   here. Same honesty as the non-copyable note in the file suite.
*
*   Fixtures use fopen/remove (ISO C) only; "." is the portable directory that
* is always there. No platform call, matching file_stat.hpp's own no-OS rule.
*
*
* path:      /tests/djinterp/fs/file_stat_tests_sa.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    STANDALONE HARNESS      (self-contained; no framework dependency)
      -----------------------------------------------------------------

I.    CONSTRUCTION / EMPTY    default is type_none, exists() false
      ------------------------------------------------------------

II.   REGULAR FILE            status of a real file: type, size, predicates
      ---------------------------------------------------------------------

III.  DIRECTORY               status of "." : type_directory
      ------------------------------------------------------

IV.   NOT FOUND               absent path is type_not_found with NO error
      -------------------------------------------------------------------

V.    SYMLINK_STATUS          the no-follow code path (difference deferred)
      ---------------------------------------------------------------------

VI.   ONE-QUESTION HELPERS    exists / is_regular_file / is_directory / file_size
      ---------------------------------------------------------------------------

VII.  FAILURE                 invalid path is EINVAL / type_none
      ----------------------------------------------------------

      master runner + main
*/

// std
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// djinterp
#include "../../../inc/djinterp/core/filesys/file_stat.hpp"


// ===========================================================================
// 0.   STANDALONE HARNESS   (identical in shape to the sibling suites)
// ===========================================================================

#define D_TEST_STATUS_MAX_ASSERTS 32


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
d_tests_status_group
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


struct d_tests_status_counter
{
    size_t passed;
    size_t failed;
};


static void
d_tests_status_print
(
    const struct d_test_object*    _obj,
    size_t                         _depth,
    struct d_tests_status_counter* _counter
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
            d_tests_status_print(_obj->elements[idx], _depth + 1, _counter);
        }
    }

    return;
}


// d_tests_status_make
//   function: write _n bytes to a fresh scratch file and return its name, for
// the groups that need a real regular file to stat. Standard C only. The
// caller remove()s it.
static const char*
d_tests_status_make(size_t _n)
{
    static char buf[64];
    static int  seq = 0;
    FILE*       f;
    size_t      i;

    (void)snprintf(buf, sizeof(buf), "djinterp_stest_%d.tmp", seq);
    ++seq;

    f = fopen(buf, "wb");

    if (f)
    {
        for (i = 0; i < _n; ++i)
        {
            (void)fputc('A', f);
        }

        (void)fclose(f);
    }

    return buf;
}


// ===========================================================================
// I.   CONSTRUCTION / EMPTY
// ===========================================================================

/*
d_tests_status_construct
  A default file_status is the "no query has run" state: type_none, not
existing, and its field accessors read a defined 0 rather than garbage.
*/
static struct d_test_object*
d_tests_status_construct(void)
{
    struct d_test_object* items[D_TEST_STATUS_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    {
        djinterp::file_status s;

        items[idx++] = D_ASSERT_EQUAL("default_type_none",
                                      s.type(), djinterp::file_status::type_none,
                                      "a default status has run no query");
        items[idx++] = D_ASSERT_FALSE("default_not_exist",
                                      s.exists(),
                                      "...so it does not 'exist'");
        items[idx++] = D_ASSERT_EQUAL("default_size_zero",
                                      s.size(), (uint64_t)0,
                                      "...and its fields read a defined 0");
    }

    return d_tests_status_group("construction / empty", items, idx);
}


// ===========================================================================
// II.  REGULAR FILE
// ===========================================================================

/*
d_tests_status_regular
  status of a real file reports it as a regular file, sized to the bytes on
disk, with every other type predicate false.
*/
static struct d_test_object*
d_tests_status_regular(void)
{
    struct d_test_object* items[D_TEST_STATUS_MAX_ASSERTS];
    size_t                idx;
    const char*           scratch;

    bool is_reg    = false;
    bool exists    = false;
    bool size_ok   = false;
    bool not_dir   = false;
    bool not_other = false;
    bool mtime_ok  = false;

    idx     = 0;
    scratch = d_tests_status_make(11);   // write 11 bytes

    {
        djinterp::error       ec;
        djinterp::file_status s = djinterp::status(djinterp::path(scratch), ec);

        is_reg    = s.is_regular_file() && !ec.failed();
        exists    = s.exists();
        size_ok   = (s.size() == (uint64_t)11);
        not_dir   = !s.is_directory();
        not_other = !s.is_other();
        mtime_ok  = (s.modified_time() > 0);   // a real file has a real mtime
    }

    items[idx++] = D_ASSERT_TRUE("is_regular",    is_reg,    "a real file is type_regular");
    items[idx++] = D_ASSERT_TRUE("exists",        exists,    "...and exists()");
    items[idx++] = D_ASSERT_TRUE("size_matches",  size_ok,   "...with size equal to the bytes written");
    items[idx++] = D_ASSERT_TRUE("not_directory", not_dir,   "...and is not a directory");
    items[idx++] = D_ASSERT_TRUE("not_other",     not_other, "...nor an 'other' type");
    items[idx++] = D_ASSERT_TRUE("mtime_set",     mtime_ok,  "...and carries a modification time");

    (void)remove(scratch);

    return d_tests_status_group("regular file", items, idx);
}


// ===========================================================================
// III. DIRECTORY
// ===========================================================================

/*
d_tests_status_directory
  status of "." -- always present, no fixture to build -- reports a directory.
*/
static struct d_test_object*
d_tests_status_directory(void)
{
    struct d_test_object* items[D_TEST_STATUS_MAX_ASSERTS];
    size_t                idx;

    bool is_dir  = false;
    bool exists  = false;
    bool not_reg = false;

    idx = 0;

    {
        djinterp::error       ec;
        djinterp::file_status s = djinterp::status(djinterp::path("."), ec);

        is_dir  = s.is_directory() && !ec.failed();
        exists  = s.exists();
        not_reg = !s.is_regular_file();
    }

    items[idx++] = D_ASSERT_TRUE("is_directory",   is_dir,  "\".\" is a directory");
    items[idx++] = D_ASSERT_TRUE("exists",         exists,  "...and exists()");
    items[idx++] = D_ASSERT_TRUE("not_regular",    not_reg, "...and is not a regular file");

    return d_tests_status_group("directory", items, idx);
}


// ===========================================================================
// IV.  NOT FOUND
// ===========================================================================

/*
d_tests_status_not_found
  The behaviour that separates a status query from a bare stat: an absent file
is type_not_found and reports NO error -- successfully learning that nothing is
there is not a failure. This is what lets exists() be a clean false.
*/
static struct d_test_object*
d_tests_status_not_found(void)
{
    struct d_test_object* items[D_TEST_STATUS_MAX_ASSERTS];
    size_t                idx;

    bool type_nf   = false;
    bool no_error  = false;
    bool not_exist = false;

    idx = 0;

    {
        djinterp::error       ec;
        djinterp::file_status s = djinterp::status(djinterp::path("no_such_djinterp_file.xyz"), ec);

        type_nf   = (s.type() == djinterp::file_status::type_not_found);
        no_error  = !ec.failed();
        not_exist = !s.exists();
    }

    items[idx++] = D_ASSERT_TRUE("type_not_found", type_nf,   "an absent path is type_not_found");
    items[idx++] = D_ASSERT_TRUE("no_error",       no_error,  "...and that is NOT reported as an error");
    items[idx++] = D_ASSERT_TRUE("not_exists",     not_exist, "...so exists() is a clean false");

    return d_tests_status_group("not found", items, idx);
}


// ===========================================================================
// V.   SYMLINK_STATUS   (the no-follow code path; difference deferred)
// ===========================================================================

/*
d_tests_status_symlink
  symlink_status runs the no-follow stat (d_file_stat_nofollow). On a NON-symlink it must
agree with status -- a regular file is regular either way. The follow-vs-no-
follow DIFFERENCE needs a real symlink, which ISO C cannot create; it is
verified in the full tree, not asserted here (see the file banner).
*/
static struct d_test_object*
d_tests_status_symlink(void)
{
    struct d_test_object* items[D_TEST_STATUS_MAX_ASSERTS];
    size_t                idx;
    const char*           scratch;

    bool lstat_reg = false;
    bool lstat_dir = false;

    idx     = 0;
    scratch = d_tests_status_make(3);

    {
        djinterp::error       ec;
        djinterp::file_status s = djinterp::symlink_status(djinterp::path(scratch), ec);

        lstat_reg = s.is_regular_file() && !ec.failed();
    }
    {
        djinterp::error       ec;
        djinterp::file_status s = djinterp::symlink_status(djinterp::path("."), ec);

        lstat_dir = s.is_directory() && !ec.failed();
    }

    items[idx++] = D_ASSERT_TRUE("nofollow_regular",   lstat_reg, "symlink_status of a plain file is regular");
    items[idx++] = D_ASSERT_TRUE("nofollow_directory", lstat_dir, "symlink_status of a directory is a directory");

    (void)remove(scratch);

    return d_tests_status_group("symlink_status (no-follow path)", items, idx);
}


// ===========================================================================
// VI.  ONE-QUESTION HELPERS
// ===========================================================================

/*
d_tests_status_helpers
  The free predicates for the single-question case, and the one with teeth:
file_size returns the byte count of a regular file, EINVAL for a directory
(whose st_size is not a byte count), and ENOENT for an absent path -- so a 0
return always came from a real, empty file with a cleared error.
*/
static struct d_test_object*
d_tests_status_helpers(void)
{
    struct d_test_object* items[D_TEST_STATUS_MAX_ASSERTS];
    size_t                idx;
    const char*           scratch;

    bool ex_true   = false;
    bool ex_false  = false;
    bool reg_true  = false;
    bool dir_true  = false;
    bool sz_ok     = false;
    bool sz_dir    = false;
    bool sz_absent = false;

    idx     = 0;
    scratch = d_tests_status_make(7);

    {
        djinterp::error ec;
        ex_true = djinterp::exists(djinterp::path(scratch), ec) && !ec.failed();
    }
    {
        djinterp::error ec;
        ex_false = !djinterp::exists(djinterp::path("no_such_djinterp_file.xyz"), ec) && !ec.failed();
    }
    {
        djinterp::error ec;
        reg_true = djinterp::is_regular_file(djinterp::path(scratch), ec);
    }
    {
        djinterp::error ec;
        dir_true = djinterp::is_directory(djinterp::path("."), ec);
    }
    {
        djinterp::error ec;
        sz_ok = (djinterp::file_size(djinterp::path(scratch), ec) == (uint64_t)7) && !ec.failed();
    }
    {
        djinterp::error ec;
        (void)djinterp::file_size(djinterp::path("."), ec);
        sz_dir = (ec.value() == EINVAL);                 // a directory is not sizeable
    }
    {
        djinterp::error ec;
        (void)djinterp::file_size(djinterp::path("no_such_djinterp_file.xyz"), ec);
        sz_absent = (ec.value() == ENOENT);
    }

    items[idx++] = D_ASSERT_TRUE("exists_true",    ex_true,   "exists() is true for a real file");
    items[idx++] = D_ASSERT_TRUE("exists_false",   ex_false,  "...false, without error, for an absent one");
    items[idx++] = D_ASSERT_TRUE("is_regular",     reg_true,  "is_regular_file() is true for a file");
    items[idx++] = D_ASSERT_TRUE("is_directory",   dir_true,  "is_directory() is true for \".\"");
    items[idx++] = D_ASSERT_TRUE("file_size",      sz_ok,     "file_size() returns the byte count");
    items[idx++] = D_ASSERT_TRUE("file_size_dir",  sz_dir,    "...EINVAL for a directory");
    items[idx++] = D_ASSERT_TRUE("file_size_gone", sz_absent, "...ENOENT for an absent path");

    (void)remove(scratch);

    return d_tests_status_group("one-question helpers", items, idx);
}


// ===========================================================================
// VII. FAILURE
// ===========================================================================

/*
d_tests_status_failure
  An invalid path is rejected as EINVAL and yields type_none -- it is neither a
file that exists nor one confirmed absent, because no query was made.
*/
static struct d_test_object*
d_tests_status_failure(void)
{
    struct d_test_object* items[D_TEST_STATUS_MAX_ASSERTS];
    size_t                idx;

    bool einval    = false;
    bool type_none = false;

    idx = 0;

    {
        djinterp::error       ec;
        djinterp::file_status s = djinterp::status(djinterp::path((const char*)0), ec);

        einval    = (ec.value() == EINVAL);
        type_none = (s.type() == djinterp::file_status::type_none);
    }

    items[idx++] = D_ASSERT_TRUE("invalid_einval",    einval,    "an invalid path is EINVAL");
    items[idx++] = D_ASSERT_TRUE("invalid_type_none", type_none, "...and yields type_none, not not_found");

    return d_tests_status_group("failure", items, idx);
}


// ===========================================================================
//      MASTER RUNNER
// ===========================================================================

static struct d_test_object*
d_tests_status_all(void)
{
    struct d_test_object* items[7];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_status_construct();
    items[idx++] = d_tests_status_regular();
    items[idx++] = d_tests_status_directory();
    items[idx++] = d_tests_status_not_found();
    items[idx++] = d_tests_status_symlink();
    items[idx++] = d_tests_status_helpers();
    items[idx++] = d_tests_status_failure();

    return d_tests_status_group("djinterp::file_status (a captured snapshot)", items, idx);
}


int
main(void)
{
    struct d_test_object*         root;
    struct d_tests_status_counter counter;

    counter.passed = 0;
    counter.failed = 0;

    (void)printf("=============================================="
                 "==================================\n");
    (void)printf(" djinterp::file_status -- standalone test  "
                 "(__cplusplus=%ld)\n",
                 (long)__cplusplus);
    (void)printf("=============================================="
                 "==================================\n\n");

    root = d_tests_status_all();

    if (!root)
    {
        (void)fprintf(stderr, "out of memory building the test tree\n");

        return 2;
    }

    d_tests_status_print(root, 0, &counter);
    d_test_object_free(root);

    (void)printf("\npassed: %lu  failed: %lu\n",
                 (unsigned long)counter.passed,
                 (unsigned long)counter.failed);

    return (counter.failed == 0) ? 0 : 1;
}
