/*******************************************************************************
* djinterp [c][utility]                                      counter_tests_ref.c
*
* The counter suite, block "ref": counter.h's interface over any counter --
* struct dj_counter_vtable, struct dj_counter_ref and the d_counter_ref_*
* functions -- on each of the three kinds, and across them.
*
*
* path:      /tests/djinterp/c/util/counter/counter_tests_ref.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./counter_tests.h"  // corresponding header

// std
#include <string.h>  // memset


// D_TESTS_COUNTER_REF_KINDS
//   constant: the kinds of counter: plain, atomic and guarded.
#define D_TESTS_COUNTER_REF_KINDS  3

/**
 * @brief Whether a kind's table has every slot set.
 *
 * @param[in] _vtable  the table; may be `NULL`.
 * @return `true` when it is a table and no slot is `NULL`.
 */
static bool
d_tests_counter_ref_is_full(
    const struct dj_counter_vtable* _vtable
)
{
    return ( (_vtable)                 &&
             (_vtable->increment)      &&
             (_vtable->decrement)      &&
             (_vtable->merge)          &&
             (_vtable->reset)          &&
             (_vtable->value)          &&
             (_vtable->snapshot) );
}

/*
d_tests_counter_ref_make
  Making a reference.
  Tests the following:
  - D_COUNTER_REF_NONE and a zero-filled reference are no counter
  - each kind's maker refers to the counter it is given, through a table
    with every slot set, one table per kind
  - each maker given NULL gives no counter
*/
bool
d_tests_counter_ref_make(void)
{
    const struct dj_counter_ref none    = D_COUNTER_REF_NONE;
    struct dj_counter_ref       filled  = D_COUNTER_REF_NONE;
    struct dj_counter           plain   = D_COUNTER_INIT;
    struct dj_counter_atomic    shared  = { 0 };
    struct dj_counter_mutex     guarded = { D_COUNTER_INIT };
    struct dj_counter_ref       a       = D_COUNTER_REF_NONE;
    struct dj_counter_ref       b       = D_COUNTER_REF_NONE;
    struct dj_counter_ref       c       = D_COUNTER_REF_NONE;

    memset(&filled,
           0,
           sizeof(filled));
    d_counter_atomic_init(&shared,
                          0);
    D_TEST_REQUIRE(d_counter_mutex_init(&guarded,
                                        0));

    D_TEST_REQUIRE(!d_counter_ref_is_valid(none));
    D_TEST_REQUIRE(!d_counter_ref_is_valid(filled));

    a = d_counter_ref(&plain);
    b = d_counter_atomic_ref(&shared);
    c = d_counter_mutex_ref(&guarded);
    D_TEST_REQUIRE(d_counter_ref_is_valid(a));
    D_TEST_REQUIRE(d_counter_ref_is_valid(b));
    D_TEST_REQUIRE(d_counter_ref_is_valid(c));
    D_TEST_REQUIRE(a.counter == &plain);
    D_TEST_REQUIRE(b.counter == &shared);
    D_TEST_REQUIRE(c.counter == &guarded);
    D_TEST_REQUIRE(d_tests_counter_ref_is_full(a.vtable));
    D_TEST_REQUIRE(d_tests_counter_ref_is_full(b.vtable));
    D_TEST_REQUIRE(d_tests_counter_ref_is_full(c.vtable));
    D_TEST_REQUIRE(a.vtable != b.vtable);
    D_TEST_REQUIRE(b.vtable != c.vtable);
    D_TEST_REQUIRE(a.vtable != c.vtable);
    D_TEST_REQUIRE(d_counter_ref(&plain).vtable == a.vtable);

    D_TEST_REQUIRE(!d_counter_ref_is_valid(d_counter_ref(NULL)));
    D_TEST_REQUIRE(!d_counter_ref_is_valid(d_counter_atomic_ref(NULL)));
    D_TEST_REQUIRE(!d_counter_ref_is_valid(d_counter_mutex_ref(NULL)));

    d_counter_mutex_destroy(&guarded);

    return true;
}

/*
d_tests_counter_ref_operations
  The operations and queries, through a reference to each kind.
  Tests the following:
  - increment and decrement move the counter and stop at its bounds, and
    report the cut, as the kind's own functions do
  - reset returns it to its start
  - value and snapshot read it; the snapshot keeps the start and the bounds
*/
bool
d_tests_counter_ref_operations(void)
{
    const struct d_interval_imax range   = d_tests_counter_range(0,
                                                                 10);
    struct dj_counter            plain                           = D_COUNTER_INIT;
    struct dj_counter_atomic     shared                          = { 0 };
    struct dj_counter_mutex      guarded                         = { D_COUNTER_INIT };
    struct dj_counter            copy                            = D_COUNTER_INIT;
    struct dj_counter_ref        refs[D_TESTS_COUNTER_REF_KINDS] = { 0 };

    D_TEST_REQUIRE(d_counter_init_bounded(&plain,
                                          4,
                                          &range));
    D_TEST_REQUIRE(d_counter_atomic_init_bounded(&shared,
                                                 4,
                                                 &range));
    D_TEST_REQUIRE(d_counter_mutex_init_bounded(&guarded,
                                                4,
                                                &range));
    refs[0] = d_counter_ref(&plain);
    refs[1] = d_counter_atomic_ref(&shared);
    refs[2] = d_counter_mutex_ref(&guarded);

    // the same moves on every kind
    for (int i = 0; i < D_TESTS_COUNTER_REF_KINDS; ++i)
    {
        D_TEST_NOTE("kind %d (0 plain, 1 atomic, 2 guarded)",
                    i);

        D_TEST_REQUIRE(d_counter_ref_increment(refs[i],
                                               5));
        D_TEST_REQUIRE(d_counter_ref_value(refs[i]) == 9);
        D_TEST_REQUIRE(!d_counter_ref_increment(refs[i],
                                                5));
        D_TEST_REQUIRE(d_counter_ref_value(refs[i]) == 10);
        D_TEST_REQUIRE(d_counter_ref_decrement(refs[i],
                                               4));
        D_TEST_REQUIRE(d_counter_ref_value(refs[i]) == 6);
        D_TEST_REQUIRE(!d_counter_ref_decrement(refs[i],
                                                20));
        D_TEST_REQUIRE(d_counter_ref_value(refs[i]) == 0);

        d_counter_ref_reset(refs[i]);
        D_TEST_REQUIRE(d_counter_ref_value(refs[i]) == 4);

        memset(&copy,
               0,
               sizeof(copy));
        D_TEST_REQUIRE(d_counter_ref_snapshot(refs[i],
                                              &copy));
        D_TEST_REQUIRE(copy.value == 4);
        D_TEST_REQUIRE(copy.initial == 4);
        D_TEST_REQUIRE(copy.has_bounds);
        D_TEST_REQUIRE(copy.bounds.upper == 10);
    }

    d_counter_mutex_destroy(&guarded);

    return true;
}

/*
d_tests_counter_ref_compare
  Ordering counters of any kinds.
  Tests the following:
  - three-way by value across kinds
  - no counter orders before any counter, and equal to no counter
*/
bool
d_tests_counter_ref_compare(void)
{
    const struct dj_counter_ref none    = D_COUNTER_REF_NONE;
    struct dj_counter           plain   = D_COUNTER_INIT;
    struct dj_counter_atomic    shared  = { 0 };
    struct dj_counter_mutex     guarded = { D_COUNTER_INIT };
    struct dj_counter_ref       a       = D_COUNTER_REF_NONE;
    struct dj_counter_ref       b       = D_COUNTER_REF_NONE;
    struct dj_counter_ref       c       = D_COUNTER_REF_NONE;

    d_counter_init(&plain,
                   1);
    d_counter_atomic_init(&shared,
                          2);
    D_TEST_REQUIRE(d_counter_mutex_init(&guarded,
                                        3));
    a = d_counter_ref(&plain);
    b = d_counter_atomic_ref(&shared);
    c = d_counter_mutex_ref(&guarded);

    D_TEST_REQUIRE(d_counter_ref_compare(a,
                                         b) == -1);
    D_TEST_REQUIRE(d_counter_ref_compare(c,
                                         b) == 1);
    D_TEST_REQUIRE(d_counter_ref_compare(b,
                                         b) == 0);
    D_TEST_REQUIRE(d_counter_ref_increment(a,
                                           2));
    D_TEST_REQUIRE(d_counter_ref_compare(a,
                                         c) == 0);

    D_TEST_REQUIRE(d_counter_ref_compare(none,
                                         a) == -1);
    D_TEST_REQUIRE(d_counter_ref_compare(a,
                                         none) == 1);
    D_TEST_REQUIRE(d_counter_ref_compare(none,
                                         none) == 0);

    d_counter_mutex_destroy(&guarded);

    return true;
}

/*
d_tests_counter_ref_merge
  Merging counters of any kinds.
  Tests the following:
  - each kind merges each other kind's movement since its start
  - a guarded counter merged into itself doubles its movement, without a
    deadlock, since the source is read through a snapshot first
  - a merge cut at a bound reports false
  - no counter on either side, or a source that cannot be read, fails and
    moves nothing
*/
bool
d_tests_counter_ref_merge(void)
{
    const struct dj_counter_ref  none    = D_COUNTER_REF_NONE;
    const struct d_interval_imax range   = d_tests_counter_range(0,
                                                                 10);
    struct dj_counter            plain   = D_COUNTER_INIT;
    struct dj_counter            capped  = D_COUNTER_INIT;
    struct dj_counter_atomic     shared  = { 0 };
    struct dj_counter_mutex      guarded = { D_COUNTER_INIT };
    struct dj_counter_mutex      gone    = { D_COUNTER_INIT };
    struct dj_counter_ref        a       = D_COUNTER_REF_NONE;
    struct dj_counter_ref        b       = D_COUNTER_REF_NONE;
    struct dj_counter_ref        c       = D_COUNTER_REF_NONE;

    d_counter_init(&plain,
                   0);
    d_counter_atomic_init(&shared,
                          10);
    D_TEST_REQUIRE(d_counter_mutex_init(&guarded,
                                        100));
    a = d_counter_ref(&plain);
    b = d_counter_atomic_ref(&shared);
    c = d_counter_mutex_ref(&guarded);
    D_TEST_REQUIRE(d_counter_ref_increment(a,
                                           5));
    D_TEST_REQUIRE(d_counter_ref_increment(b,
                                           3));
    D_TEST_REQUIRE(d_counter_ref_increment(c,
                                           2));

    // atomic into plain, plain into guarded, guarded into atomic
    D_TEST_REQUIRE(d_counter_ref_merge(a,
                                       b));
    D_TEST_REQUIRE(d_counter_ref_value(a) == 8);
    D_TEST_REQUIRE(d_counter_ref_merge(c,
                                       a));
    D_TEST_REQUIRE(d_counter_ref_value(c) == 110);
    D_TEST_REQUIRE(d_counter_ref_merge(b,
                                       c));
    D_TEST_REQUIRE(d_counter_ref_value(b) == 23);

    D_TEST_REQUIRE(d_counter_ref_merge(c,
                                       c));
    D_TEST_REQUIRE(d_counter_ref_value(c) == 120);

    D_TEST_REQUIRE(d_counter_init_bounded(&capped,
                                          0,
                                          &range));
    D_TEST_REQUIRE(d_counter_ref_merge(d_counter_ref(&capped),
                                       a));
    D_TEST_REQUIRE(!d_counter_ref_merge(d_counter_ref(&capped),
                                        a));
    D_TEST_REQUIRE(capped.value == 10);

    D_TEST_REQUIRE(!d_counter_ref_merge(none,
                                        a));
    D_TEST_REQUIRE(!d_counter_ref_merge(a,
                                        none));
    D_TEST_REQUIRE(!d_counter_ref_merge(a,
                                        d_counter_mutex_ref(&gone)));
    D_TEST_REQUIRE(d_counter_ref_value(a) == 8);

    d_counter_mutex_destroy(&guarded);

    return true;
}

/*
d_tests_counter_ref_none
  A reference to no counter, and to a counter that cannot be read.
  Tests the following:
  - on no counter (no table, or no counter), the operations fail, reset
    does nothing and the queries answer 0
  - a snapshot needs somewhere to go
  - a guarded counter not set up is referred to, but cannot be moved or read
*/
bool
d_tests_counter_ref_none(void)
{
    const struct dj_counter_ref none     = D_COUNTER_REF_NONE;
    struct dj_counter           plain    = D_COUNTER_INIT;
    struct dj_counter           copy     = D_COUNTER_INIT;
    struct dj_counter_mutex     gone     = { D_COUNTER_INIT };
    struct dj_counter_ref       headless = D_COUNTER_REF_NONE;
    struct dj_counter_ref       refs[2]  = { 0 };

    headless.vtable = d_counter_ref(&plain).vtable;
    refs[0]         = none;
    refs[1]         = headless;

    // no counter, either way
    for (int i = 0; i < 2; ++i)
    {
        D_TEST_REQUIRE(!d_counter_ref_is_valid(refs[i]));
        D_TEST_REQUIRE(!d_counter_ref_increment(refs[i],
                                                1));
        D_TEST_REQUIRE(!d_counter_ref_decrement(refs[i],
                                                1));
        d_counter_ref_reset(refs[i]);
        D_TEST_REQUIRE(d_counter_ref_value(refs[i]) == 0);
        D_TEST_REQUIRE(!d_counter_ref_snapshot(refs[i],
                                               &copy));
    }

    D_TEST_REQUIRE(!d_counter_ref_snapshot(d_counter_ref(&plain),
                                           NULL));

    D_TEST_REQUIRE(d_counter_ref_is_valid(d_counter_mutex_ref(&gone)));
    D_TEST_REQUIRE(!d_counter_ref_increment(d_counter_mutex_ref(&gone),
                                            1));
    D_TEST_REQUIRE(!d_counter_ref_snapshot(d_counter_mutex_ref(&gone),
                                           &copy));
    D_TEST_REQUIRE(d_counter_ref_value(d_counter_mutex_ref(&gone)) == 0);

    return true;
}
