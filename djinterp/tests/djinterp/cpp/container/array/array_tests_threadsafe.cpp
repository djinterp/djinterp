/*******************************************************************************
* djinterp [cpp][container]                           array_tests_threadsafe.cpp
*
*   DThreadsafeArray tests: construction, the lock-free queries, locked
* single operations, bulk operations, locked handles, snapshots, copy
* assignment, the policy aliases, and four threads' writes with none lost.
*
*
* path:      /tests/djinterp/cpp/container/array/array_tests_threadsafe.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

// std
#include <atomic>       // std::atomic
#include <cstddef>      // std::size_t
#include <thread>       // std::thread
#include <type_traits>  // std::is_same
#include <vector>       // std::vector
// djinterp
#include "array_tests.hpp"  // suite header (normal face)


NS_DJINTERP
NS_TESTING

// ts4_array
//   alias: the array a ts4 wraps.
using ts4_array = ts4::underlying_type;

/*
tests_threadsafe_construction
  The constructors.
  Tests the following:
  - default construction value-initializes
  - the value constructor forwards element values, and is explicit
  - construction from an array
  - copy from a const and from a non-const lvalue (before 2026.11.14 the
    second went to the forwarding constructor and failed to compile)
  - no move; the constants follow the array
*/
bool tests_threadsafe_construction()
{
    ts4 d;
    D_AY_CHECK(d.at(0) == 0 && d.at(3) == 0);

    ts4 v(1, 2, 3, 4);
    D_AY_CHECK(v.at(0) == 1 && v.at(3) == 4);
    D_AY_CHECK(!(std::is_convertible<int, ts4>::value));

    ts4 a(ts4_array(5, 6, 7, 8));
    D_AY_CHECK(a.at(1) == 6);

    ts4 copy_mut(v);
    D_AY_CHECK(copy_mut.at(2) == 3);

    const ts4& cv = v;
    ts4        copy_const(cv);
    D_AY_CHECK(copy_const.at(3) == 4);

    D_AY_CHECK(std::is_copy_constructible<ts4>::value);
    D_AY_CHECK(!std::is_move_constructible<ts4>::value);
    D_AY_CHECK(ts4::extent == 4 && ts4::iterable && !ts4_ni::iterable);
    D_AY_CHECK(ts4::lifetime == DArrayLifetime::mutable_lifetime);

    return true;
}

/*
tests_threadsafe_lockfree_queries
  size_lockfree() and empty_lockfree() agree with size() and empty().
  Tests the following:
  - after default, value and copy construction: size_lockfree() is 4
  - and after assign()
  - an extent-0 array: 0 and empty, both ways
  (Before 2026.11.14 size_lockfree() read 0 until the first assign().)
*/
bool tests_threadsafe_lockfree_queries()
{
    ts4 d;
    D_AY_CHECK(d.size_lockfree() == 4 && d.size() == 4);
    D_AY_CHECK(!d.empty_lockfree() && !d.empty());

    ts4 v(1, 2);
    D_AY_CHECK(v.size_lockfree() == 4);

    ts4 c(v);
    D_AY_CHECK(c.size_lockfree() == 4);

    c.assign(ts4_array(1, 1, 1, 1));
    D_AY_CHECK(c.size_lockfree() == 4);

    DThreadsafeArray<DArrayOptType<int>, DArrayOptExtent<0>> z;
    D_AY_CHECK(z.size_lockfree() == 0 && z.size() == 0);
    D_AY_CHECK(z.empty_lockfree() && z.empty());

    return true;
}

/*
tests_threadsafe_element_access
  at() and set().
  Tests the following:
  - set then at, on two elements
  - each set advances version() by one; at() doesn't
*/
bool tests_threadsafe_element_access()
{
    ts4        a;
    const auto v0 = a.version();

    a.set(1, 7);
    D_AY_CHECK(a.at(1) == 7);
    D_AY_CHECK(a.version() == v0 + 1);

    a.set(3, 9);
    D_AY_CHECK(a.at(3) == 9 && a.at(1) == 7);
    D_AY_CHECK(a.version() == v0 + 2);

    (void)a.at(0);
    D_AY_CHECK(a.version() == v0 + 2);

    return true;
}

/*
tests_threadsafe_bulk_operations
  assign(), apply() and apply_read().
  Tests the following:
  - assign replaces every element and advances version()
  - apply runs its function on the array, returns its result, and
    advances version()
  - apply_read returns its function's result and leaves version() alone
*/
bool tests_threadsafe_bulk_operations()
{
    ts4        a;
    const auto v0 = a.version();

    a.assign(ts4_array(1, 2, 3, 4));
    D_AY_CHECK(a.at(0) == 1 && a.at(3) == 4);
    D_AY_CHECK(a.version() == v0 + 1);

    const int r = a.apply(
        [](ts4_array& _x)
        {
            _x[0] = 10;

            return _x[0] + _x[1];
        });
    D_AY_CHECK(r == 12 && a.at(0) == 10);
    D_AY_CHECK(a.version() == v0 + 2);

    const int sum = a.apply_read(
        [](const ts4_array& _x)
        {
            return _x[0] + _x[1] + _x[2] + _x[3];
        });
    D_AY_CHECK(sum == 19);
    D_AY_CHECK(a.version() == v0 + 2);

    return true;
}

/*
tests_threadsafe_handles
  The locked handles.
  Tests the following:
  - read_access(): get(), operator* and operator-> read the array
  - write_access(): writes through each land after the handle is gone
  - batch(): record() and count(), and the lock is released at scope end
*/
bool tests_threadsafe_handles()
{
    ts4 a(1, 2, 3, 4);

    {
        auto r = a.read_access();
        D_AY_CHECK(r->size() == 4);
        D_AY_CHECK((*r)[0] == 1 && r.get()[3] == 4);
    }

    {
        auto w = a.write_access();
        (*w)[0]    = 10;
        w->at(1)   = 20;
        w.get()[2] = 30;
    }

    D_AY_CHECK(a.at(0) == 10 && a.at(1) == 20 && a.at(2) == 30);

    {
        auto g = a.batch();
        g.record();
        g.record();
        D_AY_CHECK(g.count() == 2);
    }

    D_AY_CHECK(a.at(3) == 4);

    return true;
}

/*
tests_threadsafe_snapshot_and_optimistic
  snapshot() copies; optimistic() reads.
  Tests the following:
  - the snapshot holds the array's values, by index and by iteration
  - a later set leaves the snapshot alone
  - optimistic(fn) returns fn's result on the current array
*/
bool tests_threadsafe_snapshot_and_optimistic()
{
    ts4 a(1, 2, 3, 4);

    auto s = a.snapshot();
    D_AY_CHECK(s.size() == 4 && !s.empty());
    D_AY_CHECK(s[0] == 1 && s[3] == 4);

    int sum = 0;

    for (int v : s)
    {
        sum += v;
    }

    D_AY_CHECK(sum == 10);

    a.set(0, 100);
    D_AY_CHECK(s[0] == 1 && s.data()[0] == 1);

    const int first = a.optimistic(
        [](const ts4_array& _x)
        {
            return _x[0];
        });
    D_AY_CHECK(first == 100);

    return true;
}

/*
tests_threadsafe_copy_assignment
  Copy assignment.
  Tests the following:
  - it copies every element and advances the target's version()
  - assigning an array to itself (through a second reference) changes
    neither its values nor its version()
*/
bool tests_threadsafe_copy_assignment()
{
    ts4 x(1, 2, 3, 4);
    ts4 y;

    const auto vy = y.version();
    y = x;
    D_AY_CHECK(y.at(0) == 1 && y.at(3) == 4);
    D_AY_CHECK(y.version() == vy + 1);

    ts4&       same = x;
    const auto vx   = x.version();
    x = same;
    D_AY_CHECK(x.at(2) == 3);
    D_AY_CHECK(x.version() == vx);

    return true;
}

/*
tests_threadsafe_policies
  The lock policies.
  Tests the following:
  - the default is default_lock_policy
  - DMutexArray, DSharedArray and DTimedArray select exclusive, shared and
    timed policies, and each works
*/
bool tests_threadsafe_policies()
{
    using mutex4  = DMutexArray<DArrayOptType<int>, DArrayOptExtent<4>>;
    using shared4 = DSharedArray<DArrayOptType<int>, DArrayOptExtent<4>>;
    using timed4  = DTimedArray<DArrayOptType<int>, DArrayOptExtent<4>>;

    D_AY_CHECK((std::is_same<ts4::lock_policy_type,
                             default_lock_policy>::value));
    D_AY_CHECK((std::is_same<mutex4::lock_policy_type,
                             exclusive_lock_policy>::value));
    D_AY_CHECK((std::is_same<shared4::lock_policy_type,
                             shared_lock_policy>::value));
    D_AY_CHECK((std::is_same<timed4::lock_policy_type,
                             timed_lock_policy>::value));

    mutex4  m(1, 2, 3, 4);
    shared4 s(1, 2, 3, 4);
    timed4  t(1, 2, 3, 4);

    m.set(0, 5);
    s.set(0, 6);
    t.set(0, 7);
    D_AY_CHECK(m.at(0) == 5 && s.at(0) == 6 && t.at(0) == 7);
    D_AY_CHECK(m.size_lockfree() == 4 && t.at(3) == 4);

    return true;
}

/*
tests_threadsafe_concurrent_apply
  No write is lost under contention.
  Tests the following:
  - four threads each apply() an increment of every element 5000 times
  - every element ends at 20000, and version() advanced 20000 times
*/
bool tests_threadsafe_concurrent_apply()
{
    const int threads = 4;
    const int rounds  = 5000;

    ts4                      a;
    const auto               v0 = a.version();
    std::vector<std::thread> pool;

    for (int t = 0; t < threads; ++t)
    {
        pool.emplace_back(
            [&a]()
            {
                for (int r = 0; r < rounds; ++r)
                {
                    a.apply(
                        [](ts4_array& _x)
                        {
                            for (std::size_t i = 0; i < 4; ++i)
                            {
                                ++_x[i];
                            }
                        });
                }
            });
    }

    for (auto& th : pool)
    {
        th.join();
    }

    for (std::size_t i = 0; i < 4; ++i)
    {
        D_AY_CHECK(a.at(i) == threads * rounds);
    }

    D_AY_CHECK(a.version() == v0 + threads * rounds);

    return true;
}

/*
tests_threadsafe_optimistic_is_never_torn
  optimistic() never returns a half-written array.
  Tests the following:
  - a writer thread applies {k, k, k, k}, k = 1..20000
  - the main thread's optimistic() reads meanwhile all see four equal
    elements
  (Before 2026.11.14 optimistic() read without the lock first, and
  returned torn arrays.)
*/
bool tests_threadsafe_optimistic_is_never_torn()
{
    const int rounds = 20000;

    ts4               a;
    std::atomic<bool> done(false);

    std::thread writer(
        [&a, &done]()
        {
            for (int k = 1; k <= rounds; ++k)
            {
                a.apply([k](ts4_array& _x) { _x.fill(k); });
            }

            done.store(true);
        });

    bool whole = true;

    while (!done.load())
    {
        whole = whole && a.optimistic(
            [](const ts4_array& _x)
            {
                return ( _x[0] == _x[1] &&
                         _x[1] == _x[2] &&
                         _x[2] == _x[3] );
            });
    }

    writer.join();

    D_AY_CHECK(whole);
    D_AY_CHECK(a.at(0) == rounds);

    return true;
}

NS_END  // testing
NS_END  // djinterp
