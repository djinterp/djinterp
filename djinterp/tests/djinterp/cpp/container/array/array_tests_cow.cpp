/*******************************************************************************
* djinterp [test]                                            array_tests_cow.cpp
*
*   DCowArray tests: construction, reads, snapshots that writers never
* disturb, clones taken only while a snapshot shares the storage, modify and
* replace, the lock policy option, and snapshots taken while a writer runs.
*
*
* path:      /tests/djinterp/cpp/container/array/array_tests_cow.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

// std
#include <atomic>       // std::atomic
#include <cstddef>      // std::size_t
#include <thread>       // std::thread
#include <type_traits>  // std::is_same
#include <utility>      // std::move
// djinterp
#include "array_tests.hpp"  // suite header (normal face)


NS_DJINTERP
NS_TESTING

// cow4_array
//   alias: the array a cow4 wraps.
using cow4_array = cow4::underlying_type;

// cow4_exclusive / cow4_single
//   alias: DCowArrays naming exclusive_lock_policy, and null_lock_policy
// (single-threaded use).
using cow4_exclusive = DCowArray<
    array_opt_type<int>,
    DArrayOptExtent<4>,
    DContainerOptLockPolicy<exclusive_lock_policy>>;

using cow4_single = DCowArray<
    array_opt_type<int>,
    DArrayOptExtent<4>,
    DContainerOptLockPolicy<null_lock_policy>>;

/*
tests_cow_construction
  Construction and the constants.
  Tests the following:
  - default construction value-initializes
  - construction from a const array and from an rvalue array
  - extent, lifetime and iterable follow the array
  - underlying_type is array<Options...>; no copy or move
*/
bool tests_cow_construction()
{
    cow4 d;
    D_AY_CHECK(d.at(0) == 0 && d.at(3) == 0);

    const cow4_array init(1, 2, 3, 4);
    cow4             c(init);
    D_AY_CHECK(c.at(0) == 1 && c.at(3) == 4);

    cow4 m(cow4_array(5, 6, 7, 8));
    D_AY_CHECK(m.at(0) == 5 && m.at(3) == 8);

    D_AY_CHECK(cow4::extent == 4);
    D_AY_CHECK(cow4::lifetime == DArrayLifetime::mutable_lifetime);
    D_AY_CHECK(cow4::iterable && !cow4_ni::iterable);

    D_AY_CHECK((std::is_same<cow4_array,
                             array<array_opt_type<int>,
                                   DArrayOptExtent<4>>>::value));
    D_AY_CHECK(!std::is_copy_constructible<cow4>::value);
    D_AY_CHECK(!std::is_move_constructible<cow4>::value);

    return true;
}

/*
tests_cow_read_access
  read(), at(), size() and empty().
  Tests the following:
  - read() is a const reference to the current array
  - at() returns by value
  - size() is the extent; empty() is false
*/
bool tests_cow_read_access()
{
    cow4 c(cow4_array(1, 2, 3, 4));

    const cow4_array& r = c.read();
    D_AY_CHECK(r[2] == 3 && r.size() == 4);
    D_AY_CHECK((std::is_same<decltype(c.read()), const cow4_array&>::value));
    D_AY_CHECK((std::is_same<decltype(c.at(0)), int>::value));
    D_AY_CHECK(c.at(1) == 2);
    D_AY_CHECK(c.size() == 4 && !c.empty());

    return true;
}

/*
tests_cow_snapshot_isolation
  A snapshot keeps its values through every kind of write.
  Tests the following:
  - after set, the snapshot reads the old value and the array the new
  - likewise after modify and after replace
  - get(), operator* and operator-> read the same array
  - the snapshot converts to true, and carries the version it was taken at
*/
bool tests_cow_snapshot_isolation()
{
    cow4 c(cow4_array(1, 2, 3, 4));

    auto s = c.snapshot();
    D_AY_CHECK(static_cast<bool>(s));
    D_AY_CHECK(s.version() == c.version());

    c.set(0, 9);
    D_AY_CHECK(s.get()[0] == 1 && c.at(0) == 9);

    c.modify([](cow4_array& _a) { _a[1] = 8; });
    D_AY_CHECK((*s)[1] == 2 && c.at(1) == 8);

    c.replace(cow4_array(5, 5, 5, 5));
    D_AY_CHECK(s->at(3) == 4 && c.at(3) == 5);
    D_AY_CHECK(s->size() == 4);

    D_AY_CHECK(s.version() < c.version());

    return true;
}

/*
tests_cow_clones_only_when_shared
  A write clones the storage only while a snapshot shares it.
  Tests the following:
  - with no snapshot, set writes in place: data() is unchanged
  - a live snapshot shares the storage: use_count() is 2
  - set then clones: the array's data() moves, the snapshot's doesn't,
    and the snapshot is left alone with its block (use_count() 1)
  - a second set, now unshared, writes in place again
*/
bool tests_cow_clones_only_when_shared()
{
    cow4 c(cow4_array(1, 2, 3, 4));

    const int* p0 = c.read().data();
    c.set(0, 10);
    D_AY_CHECK(c.read().data() == p0);

    auto s = c.snapshot();
    D_AY_CHECK(s.use_count() == 2);
    D_AY_CHECK(s->data() == p0);

    c.set(0, 20);
    const int* p1 = c.read().data();
    D_AY_CHECK(p1 != p0);
    D_AY_CHECK(s->data() == p0 && (*s)[0] == 10);
    D_AY_CHECK(s.use_count() == 1);

    c.set(1, 30);
    D_AY_CHECK(c.read().data() == p1);

    return true;
}

/*
tests_cow_modify_and_replace
  modify's result, and version() on every write.
  Tests the following:
  - modify returns its function's result and applies its change
  - set, modify, replace(const&) and replace(&&) each advance version()
    by one
*/
bool tests_cow_modify_and_replace()
{
    cow4 c;
    const auto v0 = c.version();

    const int r = c.modify(
        [](cow4_array& _a)
        {
            _a[0] = 5;

            return 42;
        });
    D_AY_CHECK(r == 42 && c.at(0) == 5);
    D_AY_CHECK(c.version() == v0 + 1);

    c.set(1, 6);
    D_AY_CHECK(c.version() == v0 + 2);

    const cow4_array next(7, 7, 7, 7);
    c.replace(next);
    D_AY_CHECK(c.version() == v0 + 3 && c.at(2) == 7);

    c.replace(cow4_array(8, 8, 8, 8));
    D_AY_CHECK(c.version() == v0 + 4 && c.at(2) == 8);

    return true;
}

/*
tests_cow_lock_policy
  The lock policy option.
  Tests the following:
  - the default is default_lock_policy, a real lock (before 2026.11.14 it
    was null_lock_policy, and two threads corrupted the heap)
  - DContainerOptLockPolicy<exclusive_lock_policy> selects it, with its
    mutex type
  - DContainerOptLockPolicy<null_lock_policy> selects no locking
  - each works the same on one thread
*/
bool tests_cow_lock_policy()
{
    D_AY_CHECK((std::is_same<cow4::lock_policy_type,
                             default_lock_policy>::value));
    D_AY_CHECK((std::is_same<cow4_exclusive::lock_policy_type,
                             exclusive_lock_policy>::value));
    D_AY_CHECK((std::is_same<cow4_exclusive::mutex_type,
                             exclusive_lock_policy::mutex_type>::value));
    D_AY_CHECK((std::is_same<cow4_single::lock_policy_type,
                             null_lock_policy>::value));

    cow4_exclusive e(cow4_exclusive::underlying_type(1, 2, 3, 4));
    auto           se = e.snapshot();
    e.set(0, 9);
    D_AY_CHECK((*se)[0] == 1 && e.at(0) == 9);

    cow4_single s(cow4_single::underlying_type(1, 2, 3, 4));
    auto        ss = s.snapshot();
    s.set(0, 9);
    D_AY_CHECK((*ss)[0] == 1 && s.at(0) == 9);

    return true;
}

/*
tests_cow_concurrent_snapshots_are_whole
  Snapshots taken while a writer runs are never torn.
  Tests the following:
  - a writer thread sets all four elements to k, k = 1..2000, in one
    modify each, on an array with the default lock policy
  - the main thread takes snapshots meanwhile; every one holds four equal
    elements, and the values never go backwards
*/
bool tests_cow_concurrent_snapshots_are_whole()
{
    const int rounds = 2000;

    cow4              c;
    std::atomic<bool> done(false);

    std::thread writer(
        [&c, &done]()
        {
            for (int k = 1; k <= rounds; ++k)
            {
                c.modify(
                    [k](cow4_array& _a)
                    {
                        _a.fill(k);
                    });
            }

            done.store(true);
        });

    int  last  = 0;
    bool whole = true;
    bool order = true;

    while (!done.load())
    {
        auto s = c.snapshot();

        whole = whole && ( (*s)[0] == (*s)[1] &&
                           (*s)[1] == (*s)[2] &&
                           (*s)[2] == (*s)[3] );
        order = order && ( (*s)[0] >= last );
        last  = (*s)[0];
    }

    writer.join();

    D_AY_CHECK(whole);
    D_AY_CHECK(order);
    D_AY_CHECK(c.at(0) == rounds && c.at(3) == rounds);

    return true;
}

NS_END  // testing
NS_END  // djinterp
