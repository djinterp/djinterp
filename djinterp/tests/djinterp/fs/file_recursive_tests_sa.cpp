/*******************************************************************************
* djinterp [test]                                    file_recursive_tests_sa.cpp
*
*   Unit tests for the recursive traversal in inc/djinterp/core/fs/file_recursive.hpp --
* walk and remove_all. One file for the facility, sibling to the other suites,
* same harness and same three rules. Free functions, so a FLAT tier ladder.
*
*   The visitor is a functor holding pointers to the counters it updates -- the
* C++98-clean form of a stateful visitor, and proof that walk's by-value visitor
* still accumulates external state (every copy shares the pointees). Fixtures
* build real trees through create_directory / fopen / create_symlink and tear
* them down with remove_all (itself under test in its own group); the test calls
* no opendir/mkdir/unlink directly.
*
*   The symlink group is the one that matters most: it proves walk does NOT
* descend through a directory symlink, by counting -- following it would visit
* the target's contents a second time.
*
*
* path:      /tests/djinterp/fs/file_recursive_tests_sa.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    STANDALONE HARNESS
      ------------------

I.    WALK              pre-order visit of a whole tree; depth; a deep entry
      ----------------------------------------------------------------------

II.   NO-FOLLOW         walk does not descend through a directory symlink
      -------------------------------------------------------------------

III.  REMOVE_ALL        a tree is removed entirely
      --------------------------------------------

IV.   REMOVE_ALL EDGES  idempotent on absent; removes a lone file
      -----------------------------------------------------------

V.    FAILURE           walk of a missing root
      ----------------------------------------

      master runner + main
*/

// std
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// djinterp
#include "../../../inc/djinterp/core/fs/file_recursive.hpp"
#include "../../../inc/djinterp/core/fs/file_link.hpp"   // create_symlink (no-follow fixture)


// ===========================================================================
// 0.   STANDALONE HARNESS   (identical in shape to the sibling suites)
// ===========================================================================

#define D_TEST_REC_MAX_ASSERTS 16

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
d_tests_rec_group(const char* _name, struct d_test_object** _items, size_t _count)
{
    struct d_test_object* g = d_test_object_new_interior(_name, _count);
    size_t i;
    if (!g) { return 0; }
    for (i = 0; i < _count; ++i) { g->elements[i] = _items[i]; }
    return g;
}

struct d_tests_rec_counter { size_t passed; size_t failed; };

static void
d_tests_rec_print(const struct d_test_object* _o, size_t _depth, struct d_tests_rec_counter* _c)
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
        for (i = 0; i < _o->count; ++i) { d_tests_rec_print(_o->elements[i], _depth + 1, _c); }
    }
}


// --- fixtures + a stateful visitor ---

static const char*
d_tests_rec_root(void)
{
    static char buf[64];
    static int  seq = 0;
    (void)snprintf(buf, sizeof(buf), "djinterp_rtest_%d", seq);
    ++seq;
    return buf;
}

static void
d_tests_rec_touch(const djinterp::path& _p)
{
    FILE* f = fopen(_p.c_str(), "wb");
    if (f) { (void)fputc('x', f); (void)fclose(f); }
}

// walk_recorder
//   a stateful visitor in C++98-clean form: it holds POINTERS to the counters
// it updates, so that even though walk threads a by-value copy of it through the
// recursion, every copy updates the same external state. operator() is const;
// the pointees are what change.
struct walk_recorder
{
    int*        count;
    unsigned*   max_depth;
    bool*       found_deep;
    const char* deep_target;

    void operator()(const djinterp::path& _p, unsigned _depth, bool _is_dir) const
    {
        ++(*count);
        if (_depth > *max_depth) { *max_depth = _depth; }
        if (std::strcmp(_p.c_str(), deep_target) == 0) { *found_deep = true; }
        (void)_is_dir;
    }
};


// ===========================================================================
// I.   WALK
// ===========================================================================

/*
d_tests_rec_walk
  A three-deep tree walked in pre-order visits every entry once; the deepest
entry is reached at the expected depth.
  Tree:  root/{ file_a.txt, sub1/{ file_b.txt, sub2/{ file_c.txt } } }
  Entries: file_a, sub1 (d0); file_b, sub2 (d1); file_c (d2)  -> 5, max depth 2.
*/
static struct d_test_object*
d_tests_rec_walk(void)
{
    struct d_test_object* items[D_TEST_REC_MAX_ASSERTS];
    size_t                idx;

    int      count      = 0;
    unsigned max_depth  = 0;
    bool     found_deep = false;
    bool     ok         = false;

    idx = 0;

    {
        djinterp::error ec;
        djinterp::path  root(d_tests_rec_root());
        djinterp::path  sub1 = root / djinterp::path("sub1");
        djinterp::path  sub2 = sub1 / djinterp::path("sub2");
        djinterp::path  deep = sub2 / djinterp::path("file_c.txt");

        djinterp::create_directory(root, ec);
        d_tests_rec_touch(root / djinterp::path("file_a.txt"));
        djinterp::create_directory(sub1, ec);
        d_tests_rec_touch(sub1 / djinterp::path("file_b.txt"));
        djinterp::create_directory(sub2, ec);
        d_tests_rec_touch(deep);

        walk_recorder rec;
        rec.count       = &count;
        rec.max_depth   = &max_depth;
        rec.found_deep  = &found_deep;
        rec.deep_target = deep.c_str();

        ok = djinterp::walk(root, rec, ec) && !ec.failed();

        djinterp::remove_all(root, ec);
    }

    items[idx++] = D_ASSERT_TRUE("walk_ok",     ok,               "walk completes without error");
    items[idx++] = D_ASSERT_TRUE("entry_count", count == 5,       "every entry is visited once");
    items[idx++] = D_ASSERT_TRUE("max_depth",   max_depth == 2u,  "the deepest entry is at depth 2");
    items[idx++] = D_ASSERT_TRUE("found_deep",  found_deep,       "the deepest file is reached");

    return d_tests_rec_group("walk", items, idx);
}


// ===========================================================================
// II.  NO-FOLLOW
// ===========================================================================

/*
d_tests_rec_nofollow
  walk must NOT descend through a directory symlink. Tree:
    root/{ realdir/{ inside.txt }, alink -> realdir }
  A correct walk visits realdir, inside.txt, and alink -> 3 entries. Following
  alink would visit inside.txt a second time (4+), so the count IS the proof.
*/
static struct d_test_object*
d_tests_rec_nofollow(void)
{
    struct d_test_object* items[D_TEST_REC_MAX_ASSERTS];
    size_t                idx;

    int  count = 0;
    bool ok    = false;

    idx = 0;

    {
        djinterp::error ec;
        djinterp::path  root(d_tests_rec_root());
        djinterp::path  realdir = root / djinterp::path("realdir");
        djinterp::path  alink   = root / djinterp::path("alink");

        djinterp::create_directory(root, ec);
        djinterp::create_directory(realdir, ec);
        d_tests_rec_touch(realdir / djinterp::path("inside.txt"));
        djinterp::create_symlink(djinterp::path("realdir"), alink, ec);   // link -> realdir

        unsigned      max_depth  = 0;
        bool          found_deep = false;
        walk_recorder rec;
        rec.count       = &count;
        rec.max_depth   = &max_depth;
        rec.found_deep  = &found_deep;
        rec.deep_target = "";

        ok = djinterp::walk(root, rec, ec) && !ec.failed();

        djinterp::remove_all(root, ec);
    }

    items[idx++] = D_ASSERT_TRUE("walk_ok",        ok,          "walk completes");
    items[idx++] = D_ASSERT_TRUE("no_descend",     count == 3,  "the directory symlink is not descended into");

    return d_tests_rec_group("no-follow symlink", items, idx);
}


// ===========================================================================
// III. REMOVE_ALL
// ===========================================================================

/*
d_tests_rec_remove_all
  remove_all deletes a populated tree entirely, so the root no longer exists.
*/
static struct d_test_object*
d_tests_rec_remove_all(void)
{
    struct d_test_object* items[D_TEST_REC_MAX_ASSERTS];
    size_t                idx;

    bool removed = false;
    bool gone    = false;

    idx = 0;

    {
        djinterp::error ec;
        djinterp::path  root(d_tests_rec_root());
        djinterp::path  sub = root / djinterp::path("sub");

        djinterp::create_directory(root, ec);
        d_tests_rec_touch(root / djinterp::path("f1.txt"));
        djinterp::create_directory(sub, ec);
        d_tests_rec_touch(sub / djinterp::path("f2.txt"));

        removed = djinterp::remove_all(root, ec) && !ec.failed();

        djinterp::error    probe;
        djinterp::file_status st = djinterp::symlink_status(root, probe);
        gone = (st.type() == djinterp::file_status::type_not_found);
    }

    items[idx++] = D_ASSERT_TRUE("remove_all_ok", removed, "remove_all reports success");
    items[idx++] = D_ASSERT_TRUE("tree_gone",     gone,    "...and the whole tree is gone");

    return d_tests_rec_group("remove_all", items, idx);
}


// ===========================================================================
// IV.  REMOVE_ALL EDGES
// ===========================================================================

/*
d_tests_rec_remove_edges
  remove_all is idempotent (a path already absent is success), and removes a
lone file as readily as a tree.
*/
static struct d_test_object*
d_tests_rec_remove_edges(void)
{
    struct d_test_object* items[D_TEST_REC_MAX_ASSERTS];
    size_t                idx;

    bool absent_ok = false;
    bool file_gone = false;

    idx = 0;

    {
        djinterp::error ec;
        absent_ok = djinterp::remove_all(djinterp::path("djinterp_rtest_absent"), ec) && !ec.failed();
    }
    {
        djinterp::error ec;
        djinterp::path  f(d_tests_rec_root());   // a plain file this time
        d_tests_rec_touch(f);

        bool rm = djinterp::remove_all(f, ec) && !ec.failed();

        djinterp::error       probe;
        djinterp::file_status st = djinterp::symlink_status(f, probe);
        file_gone = rm && (st.type() == djinterp::file_status::type_not_found);
    }

    items[idx++] = D_ASSERT_TRUE("absent_is_success", absent_ok, "removing an absent path is success");
    items[idx++] = D_ASSERT_TRUE("removes_lone_file", file_gone, "remove_all removes a single file too");

    return d_tests_rec_group("remove_all edges", items, idx);
}


// ===========================================================================
// V.   FAILURE
// ===========================================================================

/*
d_tests_rec_failure
  walk of a root that cannot be opened fails with the reason set.
*/
static struct d_test_object*
d_tests_rec_failure(void)
{
    struct d_test_object* items[D_TEST_REC_MAX_ASSERTS];
    size_t                idx;

    bool walk_missing = false;

    idx = 0;

    {
        djinterp::error count_ec;
        int             n = 0;
        unsigned        d = 0;
        bool            fd = false;
        walk_recorder   rec;
        rec.count = &n; rec.max_depth = &d; rec.found_deep = &fd; rec.deep_target = "";

        bool r = djinterp::walk(djinterp::path("/no/such/djinterp/root"), rec, count_ec);
        walk_missing = !r && (count_ec.value() == ENOENT);
    }

    items[idx++] = D_ASSERT_TRUE("walk_missing_enoent", walk_missing, "walk of a missing root is false + ENOENT");

    return d_tests_rec_group("failure", items, idx);
}


// ===========================================================================
//      MASTER RUNNER
// ===========================================================================

static struct d_test_object*
d_tests_rec_all(void)
{
    struct d_test_object* items[5];
    size_t                idx = 0;
    items[idx++] = d_tests_rec_walk();
    items[idx++] = d_tests_rec_nofollow();
    items[idx++] = d_tests_rec_remove_all();
    items[idx++] = d_tests_rec_remove_edges();
    items[idx++] = d_tests_rec_failure();
    return d_tests_rec_group("djinterp recursive traversal", items, idx);
}

int
main(void)
{
    struct d_test_object*      root;
    struct d_tests_rec_counter counter;

    counter.passed = 0; counter.failed = 0;
    (void)printf("=== djinterp recursive traversal -- standalone test (__cplusplus=%ld) ===\n\n", (long)__cplusplus);

    root = d_tests_rec_all();
    if (!root) { (void)fprintf(stderr, "oom\n"); return 2; }

    d_tests_rec_print(root, 0, &counter);
    d_test_object_free(root);
    (void)printf("\npassed: %lu  failed: %lu\n",
                 (unsigned long)counter.passed, (unsigned long)counter.failed);
    return (counter.failed == 0) ? 0 : 1;
}
