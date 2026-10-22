/*******************************************************************************
* djinterp [test]                                     file_expected_tests_sa.cpp
*
*   Unit tests for the re_std::expected surface (inc/djinterp/core/filesys/
* file_expected.hpp), the top rung of the tier ladder. Same harness and same three
* rules as the sibling suites.
*
*   The ladder here has TWO rungs, and that is the demonstration: an error-code
* baseline that runs on EVERY standard, and an expected group that is ADDED on
* C++11. On C++98 the second group is omitted -- not an empty group, not a
* green "skipped" -- so the count grows from base to base+expected exactly at
* the standard where re_std::expected arrives. A higher standard adds; it never
* changes the rung below.
*
*
* path:      /tests/djinterp/fs/file_expected_tests_sa.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    STANDALONE HARNESS
      ------------------

I.    ERROR-CODE CORE    the out-parameter queries (every tier) -- the baseline
      -------------------------------------------------------------------------

II.   EXPECTED SURFACE   the re_std::expected overloads (C++11) -- the added rung
      ------------------------------------------------------------------------

      master runner + main
*/

// std
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
// djinterp
#include "../../../inc/djinterp/core/filesys/file_expected.hpp"


// ===========================================================================
// 0.   STANDALONE HARNESS   (identical in shape to the sibling suites)
// ===========================================================================

#define D_TEST_EXP_MAX_ASSERTS 16

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
d_tests_exp_group(const char* _name, struct d_test_object** _items, size_t _count)
{
    struct d_test_object* g = d_test_object_new_interior(_name, _count);
    size_t i;
    if (!g) { return 0; }
    for (i = 0; i < _count; ++i) { g->elements[i] = _items[i]; }
    return g;
}

struct d_tests_exp_counter { size_t passed; size_t failed; };

static void
d_tests_exp_print(const struct d_test_object* _o, size_t _depth, struct d_tests_exp_counter* _c)
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
        for (i = 0; i < _o->count; ++i) { d_tests_exp_print(_o->elements[i], _depth + 1, _c); }
    }
}


// ===========================================================================
// I.   ERROR-CODE CORE   (every tier -- the baseline the expected rung adds to)
// ===========================================================================

/*
d_tests_exp_errorcode
  The out-parameter queries these overloads are built on -- present and correct
on every standard, which is exactly why the expected surface can be a pure
addition rather than a replacement.
*/
static struct d_test_object*
d_tests_exp_errorcode(void)
{
    struct d_test_object* items[D_TEST_EXP_MAX_ASSERTS];
    size_t                idx;

    bool dir_ok      = false;
    bool missing_err = false;
    bool dir_einval  = false;

    idx = 0;

    {
        djinterp::error       ec;
        djinterp::file_status s = djinterp::status(djinterp::path("."), ec);
        dir_ok = s.is_directory() && !ec.failed();
    }
    {
        djinterp::error ec;
        (void)djinterp::file_size(djinterp::path("/no/such/djinterp/f"), ec);
        missing_err = (ec.value() == ENOENT);
    }
    {
        djinterp::error ec;
        (void)djinterp::file_size(djinterp::path("."), ec);   // a directory
        dir_einval = (ec.value() == EINVAL);
    }

    items[idx++] = D_ASSERT_TRUE("status_dir",      dir_ok,      "status(\".\", ec) is a directory");
    items[idx++] = D_ASSERT_TRUE("size_missing",    missing_err, "file_size(missing, ec) is ENOENT");
    items[idx++] = D_ASSERT_TRUE("size_directory",  dir_einval,  "file_size(dir, ec) is EINVAL");

    return d_tests_exp_group("error-code core", items, idx);
}


// ===========================================================================
// II.  EXPECTED SURFACE   (C++11 -- the added rung)
// ===========================================================================

#if D_INTERNAL_HAVE_EXPECTED
/*
d_tests_exp_expected
  The one-argument re_std::expected overloads: a value on success, an unexpected on
failure, and absence handled as a VALUE (not a failure) -- the same semantics as
the error-code forms, carried in the return type instead of an out-parameter.
*/
static struct d_test_object*
d_tests_exp_expected(void)
{
    struct d_test_object* items[D_TEST_EXP_MAX_ASSERTS];
    size_t                idx;

    idx = 0;

    {
        re_std::expected<djinterp::file_status, djinterp::error> r =
            djinterp::status(djinterp::path("."));

        items[idx++] = D_ASSERT_TRUE("status_value",
                                     r.has_value() && r->is_directory(),
                                     "status(p) returns a value for a directory");
    }
    {
        re_std::expected<djinterp::space_info, djinterp::error> r =
            djinterp::space(djinterp::path("."));

        items[idx++] = D_ASSERT_TRUE("space_value",
                                     r.has_value() && (r->capacity > 0),
                                     "space(p) returns a value with capacity");
    }
    {
        re_std::expected<uint64_t, djinterp::error> r =
            djinterp::file_size(djinterp::path("/no/such/djinterp/f"));

        items[idx++] = D_ASSERT_TRUE("missing_unexpected",
                                     !r.has_value() && (r.error().value() == ENOENT),
                                     "file_size(missing) is re_std::unexpected(ENOENT)");
    }
    {
        // composition: value_or supplies a default without an if/else
        uint64_t n = djinterp::file_size(djinterp::path("/no/such/djinterp/f")).value_or(99u);

        items[idx++] = D_ASSERT_TRUE("value_or",
                                     n == (uint64_t)99,
                                     "value_or() yields the fallback on failure");
    }
    {
        // absence is an ANSWER, not a failure: a path that is simply not there
        // resolves to a VALUE whose exists() is false, not to an unexpected.
        re_std::expected<djinterp::file_status, djinterp::error> r =
            djinterp::status(djinterp::path("no_such_djinterp_entry"));

        items[idx++] = D_ASSERT_TRUE("absence_is_value",
                                     r.has_value() && !r->exists(),
                                     "an absent path is a value with exists()==false, not an error");
    }

    return d_tests_exp_group("expected surface (C++11)", items, idx);
}
#endif  // D_INTERNAL_HAVE_EXPECTED


// ===========================================================================
//      MASTER RUNNER
// ===========================================================================

static struct d_test_object*
d_tests_exp_all(void)
{
    struct d_test_object* items[2];
    size_t                idx = 0;

    items[idx++] = d_tests_exp_errorcode();
#if D_INTERNAL_HAVE_EXPECTED
    items[idx++] = d_tests_exp_expected();
#endif

    return d_tests_exp_group("djinterp return surfaces (error-code + expected)", items, idx);
}

int
main(void)
{
    struct d_test_object*      root;
    struct d_tests_exp_counter counter;

    counter.passed = 0; counter.failed = 0;
    (void)printf("=== djinterp expected surface -- standalone test (__cplusplus=%ld, expected=%d) ===\n\n",
                 (long)__cplusplus, (int)D_INTERNAL_HAVE_EXPECTED);

    root = d_tests_exp_all();
    if (!root) { (void)fprintf(stderr, "oom\n"); return 2; }

    d_tests_exp_print(root, 0, &counter);
    d_test_object_free(root);
    (void)printf("\npassed: %lu  failed: %lu\n",
                 (unsigned long)counter.passed, (unsigned long)counter.failed);
    return (counter.failed == 0) ? 0 : 1;
}
