/*******************************************************************************
* djinterp [test]                    threadsafe_container_tests_locked_range.cpp
*
*   Layer 8 - locked_range, the read-lock-held iteration range.  Defines the
* "locked_range" block.
*
*
* path:      /tests/djinterp/core/container/threadsafe_container_tests_locked_range.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "threadsafe_container_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_locked_range_iterate
//   Verifies the range exposes size and begin/end over the container while the
//   read lock is held (released on scope exit).
//   Tests the following:
//   - size() reports the element count
//   - forward iteration over begin/end visits every element
bool
tests_locked_range_iterate()
{
    tc_vec<tc_excl> c;
    {
        locked_ref<tc_vec<tc_excl>, tc_excl> w(c, c.mutex());
        w->push_(1);
        w->push_(2);
        w->push_(3);
    }

    locked_range<tc_vec<tc_excl>, tc_excl> lr(c, c.mutex());
    D_TC_CHECK(lr.size() == 3u);

    int sum = 0;
    for (locked_range<tc_vec<tc_excl>, tc_excl>::const_iterator it =
             lr.begin();
         it != lr.end(); ++it)
    {
        sum += *it;
    }
    D_TC_CHECK(sum == 6);
    return true;
}

// tests_locked_range_empty
//   Verifies a range over an empty container.
//   Tests the following:
//   - size() is zero and begin() equals end()
bool
tests_locked_range_empty()
{
    tc_vec<tc_excl> c;
    locked_range<tc_vec<tc_excl>, tc_excl> lr(c, c.mutex());
    D_TC_CHECK(lr.size() == 0u);
    D_TC_CHECK(lr.begin() == lr.end());
    return true;
}

NS_END  // testing
NS_END  // djinterp
