/*******************************************************************************
* djinterp [cpp][utility]                                counter_tests_merge.cpp
*
* The counter suite's compare and merge tests, for counter, atomic_counter and
* mutex_counter, and the overflow-safe steps merge relies on.
*
*
* path:      /tests/djinterp/cpp/util/counter/counter_tests_merge.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_counter_compare
  Three-way, by value.
  Tests the following:
  - below, equal and above are -1, 0 and 1
  - initial values and bounds play no part
  - the extremes compare correctly
*/
bool tests_counter_compare()
{
    DCtr low(1);
    DCtr high(2, 0, 5);
    DCtr same(0);

    same.increment();

    D_CTR_CHECK(low.compare(high) == -1);
    D_CTR_CHECK(high.compare(low) == 1);
    D_CTR_CHECK(low.compare(same) == 0);

    const DCtr bottom(k_lowest);
    const DCtr top(k_highest);

    D_CTR_CHECK(bottom.compare(top) == -1);
    D_CTR_CHECK(top.compare(bottom) == 1);

    return true;
}

/*
tests_counter_merge_movement
  Movement, not value.
  Tests the following:
  - two counters from 10 that each count 3 merge to 16
  - a source that moved down takes the destination down
  - the source is left as it was
*/
bool tests_counter_merge_movement()
{
    DCtr dest(10);
    DCtr src(10);

    dest.increment(3);
    src.increment(3);

    D_CTR_CHECK(dest.merge(src));
    D_CTR_CHECK(dest.value() == 16);
    D_CTR_CHECK(src.value() == 13);

    src.decrement(8);

    D_CTR_CHECK(dest.merge(src));
    D_CTR_CHECK(dest.value() == 11);

    return true;
}

/*
tests_counter_merge_clamps
  The destination's bounds.
  Tests the following:
  - a merge past the ceiling stops on it and returns false
  - a merge below the floor stops on it and returns false
  - a movement too large for the value type saturates instead of
    overflowing
*/
bool tests_counter_merge_clamps()
{
    DCtr dest(18, 0, 20);
    DCtr up(0);
    DCtr down(0);

    up.increment(5);
    down.decrement(50);

    D_CTR_CHECK(!dest.merge(up));
    D_CTR_CHECK(dest.value() == 20);
    D_CTR_CHECK(!dest.merge(down));
    D_CTR_CHECK(dest.value() == 0);

    DCtr wide(k_lowest);
    DCtr small(5);

    wide.increment(k_highest);
    wide.increment(k_highest);

    D_CTR_CHECK(!small.merge(wide));
    D_CTR_CHECK(small.value() == k_highest);

    return true;
}

/*
tests_counter_merge_self
  A counter merged into itself.
  Tests the following:
  - its movement is added once
*/
bool tests_counter_merge_self()
{
    DCtr counter(5);

    counter.increment(4);

    D_CTR_CHECK(counter.merge(counter));
    D_CTR_CHECK(counter.value() == 13);

    return true;
}

/*
tests_counter_steps_never_overflow
  The steps merge relies on.
  Tests the following:
  - an increment past the type's maximum clamps there instead of wrapping
  - a decrement past the type's minimum clamps there instead of wrapping
  - distance saturates for a span wider than the type
*/
bool tests_counter_steps_never_overflow()
{
    DCtr high(k_highest - 1);
    DCtr low(k_lowest + 1);

    D_CTR_CHECK(!high.increment(k_highest));
    D_CTR_CHECK(high.value() == k_highest);
    D_CTR_CHECK(!low.decrement(k_highest));
    D_CTR_CHECK(low.value() == k_lowest);

    D_CTR_CHECK(DCtr::distance(k_lowest, k_highest) == k_highest);
    D_CTR_CHECK(DCtr::distance(-5, 5) == 10);

    return true;
}

/*
tests_atomic_counter_compare_merge
  atomic_counter.
  Tests the following:
  - compare orders by value
  - merge adds another atomic counter's movement, or a plain one's
  - the bounds hold
*/
bool tests_atomic_counter_compare_merge()
{
    atomic_counter<std::int64_t> dest(0, 0, 100);
    atomic_counter<std::int64_t> src(2);
    DCtr                         plain(7);

    src.increment(5);
    plain.increment(3);

    D_CTR_CHECK(dest.compare(src) == -1);
    D_CTR_CHECK(dest.merge(src));
    D_CTR_CHECK(dest.value() == 5);
    D_CTR_CHECK(dest.merge(plain));
    D_CTR_CHECK(dest.value() == 8);
    D_CTR_CHECK(dest.compare(src) == 1);

    plain.increment(500);

    D_CTR_CHECK(!dest.merge(plain));
    D_CTR_CHECK(dest.value() == 100);

    return true;
}

/*
tests_mutex_counter_compare_merge
  mutex_counter.
  Tests the following:
  - compare orders by value
  - merge adds another mutex counter's movement, or a plain one's
  - a mutex counter merged into itself adds its movement once, without
    deadlock
  - snapshot copies the value, start and bounds
*/
bool tests_mutex_counter_compare_merge()
{
    mutex_counter<std::int64_t> dest(1, 0, 50);
    mutex_counter<std::int64_t> src(1);
    DCtr                        plain(0);

    src.increment(4);
    plain.increment(2);

    D_CTR_CHECK(dest.compare(src) == -1);
    D_CTR_CHECK(dest.merge(src));
    D_CTR_CHECK(dest.value() == 5);
    D_CTR_CHECK(dest.merge(plain));
    D_CTR_CHECK(dest.value() == 7);
    D_CTR_CHECK(dest.merge(dest));
    D_CTR_CHECK(dest.value() == 13);

    const DCtr copy = dest.snapshot();

    D_CTR_CHECK(copy.value() == 13);
    D_CTR_CHECK(copy.initial() == 1);
    D_CTR_CHECK(copy.max() == 50);

    return true;
}


NS_END  // testing
NS_END  // djinterp
