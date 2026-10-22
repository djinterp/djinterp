/*******************************************************************************
* djinterp [test]                                         file_link_tests_sa.cpp
*
*   Unit tests for the symlink operations in inc/djinterp/core/filesys/file_link.hpp --
* create_symlink, read_symlink, is_symlink -- and, because this suite can
* finally CREATE a symlink, the follow-vs-no-follow difference that
* status_tests_sa.cpp had to defer: status() follows a link, symlink_status()
* does not. That deferral is closed here, on a real symlink, not faked there.
*
*   These are free functions with no handle -- FLAT tier ladder, same count on
* every standard. Fixtures create real symlinks through create_symlink (the API
* under test) and remove them with remove() (ISO C, which removes the LINK, not
* its target); no symlink()/readlink() is called directly.
*
*
* path:      /tests/djinterp/fs/file_link_tests_sa.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    STANDALONE HARNESS      (self-contained; no framework dependency)
      -----------------------------------------------------------------

I.    CREATE / READ           make a link, read its target back
      ---------------------------------------------------------

II.   IS_SYMLINK              a link is one; a regular file is not
      ------------------------------------------------------------

III.  FOLLOW VS NO-FOLLOW     status() follows, symlink_status() does not
      -------------------------------------------------------------------

IV.   TEXTUAL                 a dangling link still reads (it is just text)
      ---------------------------------------------------------------------

V.    FAILURE                 invalid paths, reading a non-symlink
      ------------------------------------------------------------

      master runner + main
*/

// std
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// djinterp
#include "../../../inc/djinterp/core/filesys/file_link.hpp"
#include "../../../inc/djinterp/core/filesys/file_stat.hpp"   // status / symlink_status cross-check


// ===========================================================================
// 0.   STANDALONE HARNESS   (identical in shape to the sibling suites)
// ===========================================================================

#define D_TEST_LINK_MAX_ASSERTS 32


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
d_tests_link_group
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


struct d_tests_link_counter
{
    size_t passed;
    size_t failed;
};


static void
d_tests_link_print
(
    const struct d_test_object*  _obj,
    size_t                       _depth,
    struct d_tests_link_counter* _counter
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
            d_tests_link_print(_obj->elements[idx], _depth + 1, _counter);
        }
    }

    return;
}


// --- fixtures (ISO C for files; the API under test for the links) ---

// d_tests_link_names
//   function: fill two distinct scratch names -- a target and a link -- from a
// rotating buffer set (so the two held at once do not alias).
static const char*
d_tests_link_name(void)
{
    static char bufs[4][64];
    static int  seq  = 0;
    int         slot = seq % 4;

    (void)snprintf(bufs[slot], sizeof(bufs[slot]), "djinterp_ltest_%d.tmp", seq);
    ++seq;

    return bufs[slot];
}

static void
d_tests_link_touch(const char* _path)
{
    FILE* f = fopen(_path, "wb");

    if (f)
    {
        (void)fputc('x', f);
        (void)fclose(f);
    }
}


// ===========================================================================
// I.   CREATE / READ
// ===========================================================================

/*
d_tests_link_create_read
  create_symlink makes a link at one path pointing at another, and read_symlink
gives that target text back verbatim.
*/
static struct d_test_object*
d_tests_link_create_read(void)
{
    struct d_test_object* items[D_TEST_LINK_MAX_ASSERTS];
    size_t                idx;

    bool created = false;
    bool read_ok = false;

    idx = 0;

    {
        const char*     target = d_tests_link_name();
        const char*     link   = d_tests_link_name();
        djinterp::error ec;

        d_tests_link_touch(target);

        created = djinterp::create_symlink(djinterp::path(target),
                                           djinterp::path(link), ec) && !ec.failed();

        {
            djinterp::path got = djinterp::read_symlink(djinterp::path(link), ec);
            read_ok = !ec.failed() && (std::strcmp(got.c_str(), target) == 0);
        }

        (void)remove(link);
        (void)remove(target);
    }

    items[idx++] = D_ASSERT_TRUE("create",       created, "create_symlink makes the link");
    items[idx++] = D_ASSERT_TRUE("read_target",  read_ok, "read_symlink returns the target text");

    return d_tests_link_group("create / read", items, idx);
}


// ===========================================================================
// II.  IS_SYMLINK
// ===========================================================================

/*
d_tests_link_is
  is_symlink is true for a link and false for the ordinary file it points at --
without following the link in either case.
*/
static struct d_test_object*
d_tests_link_is(void)
{
    struct d_test_object* items[D_TEST_LINK_MAX_ASSERTS];
    size_t                idx;

    bool link_yes = false;
    bool file_no  = false;

    idx = 0;

    {
        const char*     target = d_tests_link_name();
        const char*     link   = d_tests_link_name();
        djinterp::error ec;

        d_tests_link_touch(target);
        djinterp::create_symlink(djinterp::path(target), djinterp::path(link), ec);

        link_yes = djinterp::is_symlink(djinterp::path(link), ec) && !ec.failed();
        file_no  = !djinterp::is_symlink(djinterp::path(target), ec) && !ec.failed();

        (void)remove(link);
        (void)remove(target);
    }

    items[idx++] = D_ASSERT_TRUE("link_is_symlink", link_yes, "a symlink is a symlink");
    items[idx++] = D_ASSERT_TRUE("file_not_symlink", file_no, "the target file is not");

    return d_tests_link_group("is_symlink", items, idx);
}


// ===========================================================================
// III. FOLLOW VS NO-FOLLOW   (the status suite's deferred case, closed here)
// ===========================================================================

/*
d_tests_link_follow
  The difference status_tests_sa.cpp could not create: given a link to a regular
file, status() FOLLOWS it and reports the target (type_regular), while
symlink_status() does NOT and reports the link itself (type_symlink). This is
the one behaviour a symlink exists to exercise.
*/
static struct d_test_object*
d_tests_link_follow(void)
{
    struct d_test_object* items[D_TEST_LINK_MAX_ASSERTS];
    size_t                idx;

    bool follows  = false;
    bool nofollow = false;

    idx = 0;

    {
        const char*     target = d_tests_link_name();
        const char*     link   = d_tests_link_name();
        djinterp::error ec;

        d_tests_link_touch(target);
        djinterp::create_symlink(djinterp::path(target), djinterp::path(link), ec);

        {
            djinterp::file_status s = djinterp::status(djinterp::path(link), ec);
            follows = !ec.failed() && s.is_regular_file();      // followed to the target
        }
        {
            djinterp::file_status s = djinterp::symlink_status(djinterp::path(link), ec);
            nofollow = !ec.failed() && s.is_symlink();          // saw the link itself
        }

        (void)remove(link);
        (void)remove(target);
    }

    items[idx++] = D_ASSERT_TRUE("status_follows",
                                 follows,
                                 "status() follows the link to the regular file");
    items[idx++] = D_ASSERT_TRUE("symlink_status_nofollow",
                                 nofollow,
                                 "symlink_status() sees the link as a link");

    return d_tests_link_group("follow vs no-follow", items, idx);
}


// ===========================================================================
// IV.  TEXTUAL
// ===========================================================================

/*
d_tests_link_textual
  A symlink is stored text, not a resolved reference: a link to a target that
does not exist is still a valid link, and read_symlink returns its text without
error -- reading a link cannot fail merely because the target is absent.
*/
static struct d_test_object*
d_tests_link_textual(void)
{
    struct d_test_object* items[D_TEST_LINK_MAX_ASSERTS];
    size_t                idx;

    bool dangling_ok = false;

    idx = 0;

    {
        const char*     link   = d_tests_link_name();
        const char*     target = "this_target_does_not_exist.dat";
        djinterp::error ec;

        djinterp::create_symlink(djinterp::path(target), djinterp::path(link), ec);

        {
            djinterp::path got = djinterp::read_symlink(djinterp::path(link), ec);
            dangling_ok = !ec.failed() && (std::strcmp(got.c_str(), target) == 0);
        }

        (void)remove(link);
    }

    items[idx++] = D_ASSERT_TRUE("dangling_readable",
                                 dangling_ok,
                                 "a link to a missing target still reads its text");

    return d_tests_link_group("textual (dangling link)", items, idx);
}


// ===========================================================================
// V.   FAILURE
// ===========================================================================

/*
d_tests_link_failure
  Invalid paths are EINVAL; reading a plain file as a link fails (it is not one).
*/
static struct d_test_object*
d_tests_link_failure(void)
{
    struct d_test_object* items[D_TEST_LINK_MAX_ASSERTS];
    size_t                idx;

    bool create_inval = false;
    bool is_inval     = false;
    bool read_notlink = false;

    idx = 0;

    {
        djinterp::error ec;
        create_inval = !djinterp::create_symlink(djinterp::path((const char*)0),
                                                 djinterp::path("x"), ec) &&
                       (ec.value() == EINVAL);
    }
    {
        djinterp::error ec;
        is_inval = !djinterp::is_symlink(djinterp::path((const char*)0), ec) &&
                   (ec.value() == EINVAL);
    }
    {
        const char*     target = d_tests_link_name();
        djinterp::error ec;

        d_tests_link_touch(target);
        // reading a plain file as a symlink fails -- it is not a link
        (void)djinterp::read_symlink(djinterp::path(target), ec);
        read_notlink = ec.failed();

        (void)remove(target);
    }

    items[idx++] = D_ASSERT_TRUE("create_invalid",    create_inval, "create_symlink of an invalid path is EINVAL");
    items[idx++] = D_ASSERT_TRUE("is_symlink_invalid", is_inval,    "is_symlink of an invalid path is EINVAL");
    items[idx++] = D_ASSERT_TRUE("read_non_symlink",  read_notlink, "read_symlink of a plain file fails");

    return d_tests_link_group("failure", items, idx);
}


// ===========================================================================
//      MASTER RUNNER
// ===========================================================================

static struct d_test_object*
d_tests_link_all(void)
{
    struct d_test_object* items[5];
    size_t                idx;

    idx = 0;
    items[idx++] = d_tests_link_create_read();
    items[idx++] = d_tests_link_is();
    items[idx++] = d_tests_link_follow();
    items[idx++] = d_tests_link_textual();
    items[idx++] = d_tests_link_failure();

    return d_tests_link_group("djinterp symlink operations", items, idx);
}


int
main(void)
{
    struct d_test_object*       root;
    struct d_tests_link_counter counter;

    counter.passed = 0;
    counter.failed = 0;

    (void)printf("=============================================="
                 "==================================\n");
    (void)printf(" djinterp symlinks -- standalone test  "
                 "(__cplusplus=%ld)\n",
                 (long)__cplusplus);
    (void)printf("=============================================="
                 "==================================\n\n");

    root = d_tests_link_all();

    if (!root)
    {
        (void)fprintf(stderr, "out of memory building the test tree\n");

        return 2;
    }

    d_tests_link_print(root, 0, &counter);
    d_test_object_free(root);

    (void)printf("\npassed: %lu  failed: %lu\n",
                 (unsigned long)counter.passed,
                 (unsigned long)counter.failed);

    return (counter.failed == 0) ? 0 : 1;
}
