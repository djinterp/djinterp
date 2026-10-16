/*******************************************************************************
* djinterp [test]                   threadsafe_container_tests_snapshot_view.cpp
*
*   Layer 6 - snapshot_view, the read-locked copy that iterates without holding
* the lock.  Defines the "snapshot_view" block.
*
*
* path:      /tests/djinterp/core/container/threadsafe_container_tests_snapshot_view.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "threadsafe_container_tests.hpp"

NS_DJINTERP
NS_TESTING

// tests_snapshot_view_copy
//   Verifies the view copies the container's elements and exposes the full
//   read surface over the copy.
//   Tests the following:
//   - size / empty / operator[] over the copied elements
//   - forward iteration visits every element
//   - data() returns the backing vector
bool
tests_snapshot_view_copy()
{
    tc_vec<tc_excl> c;
    {
        locked_ref<tc_vec<tc_excl>, tc_excl> w(c, c.mutex());
        w->push_(1);
        w->push_(2);
        w->push_(3);
    }

    snapshot_view<tc_vec<tc_excl>, tc_excl> sv(c, c.mutex());
    D_TC_CHECK(sv.size() == 3u);
    D_TC_CHECK(!sv.empty());
    D_TC_CHECK(sv[0] == 1);
    D_TC_CHECK(sv[1] == 2);
    D_TC_CHECK(sv[2] == 3);

    int sum = 0;
    for (snapshot_view<tc_vec<tc_excl>, tc_excl>::const_iterator it =
             sv.begin();
         it != sv.end(); ++it)
    {
        sum += *it;
    }
    D_TC_CHECK(sum == 6);

    D_TC_CHECK(sv.data().size() == 3u);
    D_TC_CHECK(sv.data()[0] == 1);
    return true;
}

// tests_snapshot_view_independent
//   Verifies the snapshot is an independent copy: mutating the source after
//   the snapshot leaves the view unchanged.
//   Tests the following:
//   - the view keeps its captured size and contents after source growth
//   - the source itself did change
bool
tests_snapshot_view_independent()
{
    tc_vec<tc_excl> c;
    {
        locked_ref<tc_vec<tc_excl>, tc_excl> w(c, c.mutex());
        w->push_(1);
        w->push_(2);
        w->push_(3);
    }

    snapshot_view<tc_vec<tc_excl>, tc_excl> sv(c, c.mutex());

    {
        locked_ref<tc_vec<tc_excl>, tc_excl> w(c, c.mutex());
        w->push_(4);
        w->push_(5);
    }

    D_TC_CHECK(sv.size() == 3u);
    D_TC_CHECK(sv[0] == 1);
    D_TC_CHECK(c.size() == 5u);
    return true;
}

// tests_snapshot_view_empty
//   Verifies a snapshot of an empty container.
//   Tests the following:
//   - empty() is true, size() is zero, begin() equals end()
bool
tests_snapshot_view_empty()
{
    tc_vec<tc_excl> c;
    snapshot_view<tc_vec<tc_excl>, tc_excl> sv(c, c.mutex());
    D_TC_CHECK(sv.empty());
    D_TC_CHECK(sv.size() == 0u);
    D_TC_CHECK(sv.begin() == sv.end());
    return true;
}

NS_END  // testing
NS_END  // djinterp
