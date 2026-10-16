/*******************************************************************************
* djinterp [test]                                        file_space_tests_sa.cpp
*
*   Unit tests for djinterp::space / space_info (inc/djinterp/core/fs/file_space.hpp).
* One file for the facility, sibling to the other suites, same harness and same
* three rules. A free query over a value type -- FLAT tier ladder.
*
*   The capacity numbers are real (from the filesystem the build runs on), so
* the checks are INVARIANTS, not exact values: capacity is positive, and the
* three counts nest as available <= free <= capacity -- available excludes the
* root reserve that free still counts, and neither can exceed the whole.
*
*
* path:      /tests/djinterp/fs/file_space_tests_sa.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
// djinterp
#include "../../../inc/djinterp/core/fs/file_space.hpp"


// --- harness (identical in shape to the sibling suites) ---
#define D_TEST_SPACE_MAX_ASSERTS 16

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

#define D_ASSERT_TRUE(_name, _cond, _msg) d_test_object_new_leaf(_name, _msg, (_cond))

static struct d_test_object*
d_tests_space_group(const char* _name, struct d_test_object** _items, size_t _count)
{
    struct d_test_object* g = d_test_object_new_interior(_name, _count);
    size_t i;
    if (!g) { return 0; }
    for (i = 0; i < _count; ++i) { g->elements[i] = _items[i]; }
    return g;
}

struct d_tests_space_counter { size_t passed; size_t failed; };

static void
d_tests_space_print(const struct d_test_object* _o, size_t _depth, struct d_tests_space_counter* _c)
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
        for (i = 0; i < _o->count; ++i) { d_tests_space_print(_o->elements[i], _depth + 1, _c); }
    }
}


// ===========================================================================
// I.   QUERY
// ===========================================================================

/*
d_tests_space_query
  space(".") returns a live capacity snapshot with the three counts in their
required order.
*/
static struct d_test_object*
d_tests_space_query(void)
{
    struct d_test_object* items[D_TEST_SPACE_MAX_ASSERTS];
    size_t                idx;

    bool cap_pos    = false;
    bool avail_free = false;
    bool free_cap   = false;
    bool no_error   = false;

    idx = 0;

    {
        djinterp::error      ec;
        djinterp::space_info s = djinterp::space(djinterp::path("."), ec);

        no_error   = !ec.failed();
        cap_pos    = (s.capacity > 0);
        avail_free = (s.available <= s.free);
        free_cap   = (s.free <= s.capacity);
    }

    items[idx++] = D_ASSERT_TRUE("no_error",       no_error,   "space(\".\") succeeds");
    items[idx++] = D_ASSERT_TRUE("capacity_pos",   cap_pos,    "the filesystem has a positive capacity");
    items[idx++] = D_ASSERT_TRUE("available_le_free", avail_free, "available <= free (available omits the root reserve)");
    items[idx++] = D_ASSERT_TRUE("free_le_capacity", free_cap,  "free <= capacity (neither exceeds the whole)");

    return d_tests_space_group("query", items, idx);
}


// ===========================================================================
// II.  FAILURE
// ===========================================================================

/*
d_tests_space_failure
  An invalid path is EINVAL; a path that is not there is ENOENT.
*/
static struct d_test_object*
d_tests_space_failure(void)
{
    struct d_test_object* items[D_TEST_SPACE_MAX_ASSERTS];
    size_t                idx;

    bool einval = false;
    bool enoent = false;

    idx = 0;

    {
        djinterp::error ec;
        (void)djinterp::space(djinterp::path((const char*)0), ec);
        einval = (ec.value() == EINVAL);
    }
    {
        djinterp::error ec;
        (void)djinterp::space(djinterp::path("/no/such/djinterp/path/here"), ec);
        enoent = (ec.value() == ENOENT);
    }

    items[idx++] = D_ASSERT_TRUE("invalid_einval", einval, "an invalid path is EINVAL");
    items[idx++] = D_ASSERT_TRUE("missing_enoent", enoent, "a missing path is ENOENT");

    return d_tests_space_group("failure", items, idx);
}


// ===========================================================================
//      MASTER RUNNER
// ===========================================================================

static struct d_test_object*
d_tests_space_all(void)
{
    struct d_test_object* items[2];
    size_t                idx = 0;
    items[idx++] = d_tests_space_query();
    items[idx++] = d_tests_space_failure();
    return d_tests_space_group("djinterp::space (filesystem capacity)", items, idx);
}

int
main(void)
{
    struct d_test_object*        root;
    struct d_tests_space_counter counter;

    counter.passed = 0; counter.failed = 0;
    (void)printf("=== djinterp::space -- standalone test (__cplusplus=%ld) ===\n\n", (long)__cplusplus);

    root = d_tests_space_all();
    if (!root) { (void)fprintf(stderr, "oom\n"); return 2; }

    d_tests_space_print(root, 0, &counter);
    d_test_object_free(root);
    (void)printf("\npassed: %lu  failed: %lu\n",
                 (unsigned long)counter.passed, (unsigned long)counter.failed);
    return (counter.failed == 0) ? 0 : 1;
}
