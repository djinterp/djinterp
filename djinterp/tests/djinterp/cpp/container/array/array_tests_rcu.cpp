/*******************************************************************************
* djinterp [cpp][container]                                  array_tests_rcu.cpp
*
*   rcu_array tests: construction, guarded reads, copy-modify-publish
* writes, value snapshots, and a reader thread that never sees a half-written
* array while a writer publishes.
*
*   One reader thread at a time: DRcuProtected registers one reader slot, so
* concurrent readers would share it (a sync/rcu.hpp limit, noted to lane B).
*
*
* path:      /tests/djinterp/cpp/container/array/array_tests_rcu.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

// std
#include <atomic>       // std::atomic
#include <thread>       // std::thread
#include <type_traits>  // std::is_same
#include <utility>      // std::move
// djinterp
#include "array_tests.hpp"  // suite header (normal face)


NS_DJINTERP
NS_TESTING

// rcu4_array
//   alias: the array an rcu4 wraps.
using rcu4_array = rcu4::underlying_type;

/*
tests_rcu_construction
  Construction and the constants.
  Tests the following:
  - default construction publishes a value-initialized array
  - construction from a const array and from an rvalue array
  - extent, lifetime, iterability and iterable follow the array
  - snapshot_type is DImmutableSnapshot<array>; no copy or move
*/
bool tests_rcu_construction()
{
    rcu4 d;
    D_AY_CHECK(d.at(0) == 0 && d.at(3) == 0);

    const rcu4_array init(1, 2, 3, 4);
    rcu4             c(init);
    D_AY_CHECK(c.at(0) == 1 && c.at(3) == 4);

    rcu4 m(rcu4_array(5, 6, 7, 8));
    D_AY_CHECK(m.at(2) == 7);

    D_AY_CHECK(rcu4::extent == 4);
    D_AY_CHECK(rcu4::lifetime == DArrayLifetime::mutable_lifetime);
    D_AY_CHECK(rcu4::iterability == DArrayIterability::iterable);
    D_AY_CHECK(rcu4::iterable);

    D_AY_CHECK((std::is_same<rcu4::snapshot_type,
                             DImmutableSnapshot<rcu4_array>>::value));
    D_AY_CHECK(!std::is_copy_constructible<rcu4>::value);
    D_AY_CHECK(!std::is_move_constructible<rcu4>::value);

    return true;
}

/*
tests_rcu_read_access
  read(fn), at(), size() and empty().
  Tests the following:
  - read(fn) passes the current array and returns fn's result
  - read(fn) may return void
  - at(), size() and empty()
*/
bool tests_rcu_read_access()
{
    rcu4 r(rcu4_array(1, 2, 3, 4));

    const int sum = r.read(
        [](const rcu4_array& _a)
        {
            return _a[0] + _a[3];
        });
    D_AY_CHECK(sum == 5);

    int seen = 0;
    r.read([&seen](const rcu4_array& _a) { seen = _a[1]; });
    D_AY_CHECK(seen == 2);

    D_AY_CHECK(r.at(2) == 3);
    D_AY_CHECK(r.size() == 4 && !r.empty());

    return true;
}

/*
tests_rcu_update_and_replace
  The writers publish new arrays.
  Tests the following:
  - update(fn) changes a copy and publishes it; the other elements carry
    over
  - replace(const&) and replace(&&) publish a whole new array
*/
bool tests_rcu_update_and_replace()
{
    rcu4 r(rcu4_array(1, 2, 3, 4));

    r.update([](rcu4_array& _a) { _a[1] = 20; });
    D_AY_CHECK(r.at(1) == 20);
    D_AY_CHECK(r.at(0) == 1 && r.at(3) == 4);

    const rcu4_array next(5, 6, 7, 8);
    r.replace(next);
    D_AY_CHECK(r.at(0) == 5 && r.at(3) == 8);

    r.replace(rcu4_array(9, 9, 9, 9));
    D_AY_CHECK(r.at(2) == 9);

    return true;
}

/*
tests_rcu_snapshot
  A snapshot is a copy that later writes leave alone.
  Tests the following:
  - the snapshot reads the values current when it was taken
  - after update and replace it still does, and the array has moved on
  - it holds its own block (use_count() 1) and converts to true
*/
bool tests_rcu_snapshot()
{
    rcu4 r(rcu4_array(1, 2, 3, 4));

    auto s = r.snapshot();
    D_AY_CHECK((*s)[0] == 1 && s->at(3) == 4);

    r.update([](rcu4_array& _a) { _a[0] = 10; });
    r.replace(rcu4_array(7, 7, 7, 7));

    D_AY_CHECK(s.get()[0] == 1 && (*s)[3] == 4);
    D_AY_CHECK(r.at(0) == 7);
    D_AY_CHECK(s.use_count() == 1 && static_cast<bool>(s));

    return true;
}

/*
tests_rcu_reader_never_sees_a_torn_array
  A reader racing a writer sees only whole arrays.
  Tests the following:
  - the main thread publishes {k, k, k, k} for k = 1..3000, alternating
    replace and update
  - a reader thread reads meanwhile: every array it sees has four equal
    elements, and the values never go backwards
  - the last value read after the writer stops is 3000
*/
bool tests_rcu_reader_never_sees_a_torn_array()
{
    const int rounds = 3000;

    rcu4              r;
    std::atomic<bool> done(false);
    bool              whole = true;
    bool              order = true;

    std::thread reader(
        [&r, &done, &whole, &order]()
        {
            int last = 0;

            while (!done.load())
            {
                r.read(
                    [&](const rcu4_array& _a)
                    {
                        whole = whole && ( _a[0] == _a[1] &&
                                           _a[1] == _a[2] &&
                                           _a[2] == _a[3] );
                        order = order && ( _a[0] >= last );
                        last  = _a[0];
                    });
            }
        });

    for (int k = 1; k <= rounds; ++k)
    {
        if (k % 2 == 0)
        {
            r.replace(rcu4_array(k, k, k, k));
        }
        else
        {
            r.update([k](rcu4_array& _a) { _a.fill(k); });
        }
    }

    done.store(true);
    reader.join();

    D_AY_CHECK(whole);
    D_AY_CHECK(order);
    D_AY_CHECK(r.at(0) == rounds && r.at(3) == rounds);

    return true;
}

NS_END  // testing
NS_END  // djinterp
