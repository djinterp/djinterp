/******************************************************************************
* djinterp [test]                                     atomic_tests_traits.cpp
*
*   Section V - the compile-time type / trait surface every atomic wrapper
* advertises.  These are the guarantees generic code (and the concurrency
* trait system) relies on, so each is enforced with a static_assert AND
* mirrored into a runtime bool the report records:
*
*     - value_type aliases          (atomic_size -> size_t, version -> uint64_t)
*     - concurrency_strategy_tag     (both -> atomic_strategy_tag: the tag-alias
*                                     fast path read by the trait system)
*     - copy AND move deletion       (a user-declared deleted copy ctor also
*                                     suppresses the move members, so these
*                                     types are neither copyable nor movable)
*     - atomic_stamped_ptr::stamp_type == uint16_t
*     - the noexcept surface         (ctors, load/store, fetch, convenience,
*                                     CAS, conversion are all noexcept)
*
*
* path:      /tests/djinterp/core/sync/atomic_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.13
******************************************************************************/

// std
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <type_traits>

// djinterp
#include "atomic_tests.hpp"


#define D_AT_CHECK(_cond)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            std::printf("      [check] FAILED: %s\n"                           \
                        "               at %s:%d\n",                           \
                        #_cond, __FILE__, __LINE__);                           \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
    while (false)


NS_DJINTERP
NS_TESTING


// -------------------------------------------------------------------------
//  value_type
// -------------------------------------------------------------------------

bool
tests_atomic_size_value_type()
{
    static_assert(std::is_same<atomic_size::value_type, std::size_t>::value,
                  "atomic_size::value_type must be std::size_t");

    const bool ok =
        std::is_same<atomic_size::value_type, std::size_t>::value;
    D_AT_CHECK(ok);

    return true;
}


bool
tests_atomic_version_value_type()
{
    static_assert(std::is_same<atomic_version::value_type, std::uint64_t>::value,
                  "atomic_version::value_type must be std::uint64_t");

    const bool ok =
        std::is_same<atomic_version::value_type, std::uint64_t>::value;
    D_AT_CHECK(ok);

    return true;
}


// -------------------------------------------------------------------------
//  concurrency_strategy_tag (the trait-system fast path)
// -------------------------------------------------------------------------

bool
tests_atomic_size_strategy_tag()
{
    static_assert(
        std::is_same<atomic_size::concurrency_strategy_tag,
                     atomic_strategy_tag>::value,
        "atomic_size must self-tag as atomic_strategy_tag");

    const bool ok =
        std::is_same<atomic_size::concurrency_strategy_tag,
                     atomic_strategy_tag>::value;
    D_AT_CHECK(ok);

    return true;
}


bool
tests_atomic_version_strategy_tag()
{
    static_assert(
        std::is_same<atomic_version::concurrency_strategy_tag,
                     atomic_strategy_tag>::value,
        "atomic_version must self-tag as atomic_strategy_tag");

    const bool ok =
        std::is_same<atomic_version::concurrency_strategy_tag,
                     atomic_strategy_tag>::value;
    D_AT_CHECK(ok);

    return true;
}


// -------------------------------------------------------------------------
//  copy / move deletion
// -------------------------------------------------------------------------

bool
tests_atomic_size_noncopyable()
{
    static_assert(!std::is_copy_constructible<atomic_size>::value,
                  "atomic_size must not be copy-constructible");
    static_assert(!std::is_copy_assignable<atomic_size>::value,
                  "atomic_size must not be copy-assignable");
    static_assert(!std::is_move_constructible<atomic_size>::value,
                  "atomic_size must not be move-constructible");
    static_assert(!std::is_move_assignable<atomic_size>::value,
                  "atomic_size must not be move-assignable");

    const bool ok =
        !std::is_copy_constructible<atomic_size>::value &&
        !std::is_copy_assignable<atomic_size>::value    &&
        !std::is_move_constructible<atomic_size>::value &&
        !std::is_move_assignable<atomic_size>::value;
    D_AT_CHECK(ok);

    return true;
}


bool
tests_atomic_version_noncopyable()
{
    static_assert(!std::is_copy_constructible<atomic_version>::value, "");
    static_assert(!std::is_copy_assignable<atomic_version>::value,    "");
    static_assert(!std::is_move_constructible<atomic_version>::value, "");
    static_assert(!std::is_move_assignable<atomic_version>::value,    "");

    const bool ok =
        !std::is_copy_constructible<atomic_version>::value &&
        !std::is_copy_assignable<atomic_version>::value    &&
        !std::is_move_constructible<atomic_version>::value &&
        !std::is_move_assignable<atomic_version>::value;
    D_AT_CHECK(ok);

    return true;
}


bool
tests_flag_guard_noncopyable()
{
    static_assert(!std::is_copy_constructible<atomic_flag_guard>::value, "");
    static_assert(!std::is_copy_assignable<atomic_flag_guard>::value,    "");
    static_assert(!std::is_move_constructible<atomic_flag_guard>::value, "");
    static_assert(!std::is_move_assignable<atomic_flag_guard>::value,    "");

    const bool ok =
        !std::is_copy_constructible<atomic_flag_guard>::value &&
        !std::is_copy_assignable<atomic_flag_guard>::value    &&
        !std::is_move_constructible<atomic_flag_guard>::value &&
        !std::is_move_assignable<atomic_flag_guard>::value;
    D_AT_CHECK(ok);

    return true;
}


bool
tests_stamped_ptr_noncopyable()
{
    typedef atomic_stamped_ptr<int> sp_t;

    static_assert(!std::is_copy_constructible<sp_t>::value, "");
    static_assert(!std::is_copy_assignable<sp_t>::value,    "");
    static_assert(!std::is_move_constructible<sp_t>::value, "");
    static_assert(!std::is_move_assignable<sp_t>::value,    "");

    const bool ok =
        !std::is_copy_constructible<sp_t>::value &&
        !std::is_copy_assignable<sp_t>::value    &&
        !std::is_move_constructible<sp_t>::value &&
        !std::is_move_assignable<sp_t>::value;
    D_AT_CHECK(ok);

    return true;
}


// -------------------------------------------------------------------------
//  atomic_stamped_ptr::stamp_type
// -------------------------------------------------------------------------

bool
tests_stamped_stamp_type()
{
    static_assert(
        std::is_same<atomic_stamped_ptr<int>::stamp_type,
                     std::uint16_t>::value,
        "atomic_stamped_ptr::stamp_type must be std::uint16_t");

    const bool ok =
        std::is_same<atomic_stamped_ptr<int>::stamp_type,
                     std::uint16_t>::value;
    D_AT_CHECK(ok);

    return true;
}


// -------------------------------------------------------------------------
//  noexcept surface
// -------------------------------------------------------------------------

bool
tests_atomic_size_noexcept()
{
    atomic_size s;
    std::size_t e = 0;

    static_assert(noexcept(atomic_size()),                     "");
    static_assert(noexcept(atomic_size(std::size_t(1))),       "");
    static_assert(noexcept(s.load()),                          "");
    static_assert(noexcept(s.store(1)),                        "");
    static_assert(noexcept(s.fetch_add(1)),                    "");
    static_assert(noexcept(s.fetch_sub(1)),                    "");
    static_assert(noexcept(s.increment()),                     "");
    static_assert(noexcept(s.decrement()),                     "");
    static_assert(noexcept(s.compare_exchange_weak(e, 1)),     "");
    static_assert(noexcept(s.compare_exchange_strong(e, 1)),   "");
    static_assert(noexcept(static_cast<std::size_t>(s)),       "");

    const bool ok =
        noexcept(s.load())                        &&
        noexcept(s.store(1))                      &&
        noexcept(s.fetch_add(1))                  &&
        noexcept(s.fetch_sub(1))                  &&
        noexcept(s.increment())                   &&
        noexcept(s.decrement())                   &&
        noexcept(s.compare_exchange_weak(e, 1))   &&
        noexcept(s.compare_exchange_strong(e, 1)) &&
        noexcept(static_cast<std::size_t>(s));
    D_AT_CHECK(ok);

    return true;
}


bool
tests_atomic_version_noexcept()
{
    atomic_version v;
    std::uint64_t  e = 0;

    static_assert(noexcept(atomic_version()),                   "");
    static_assert(noexcept(atomic_version(std::uint64_t(1))),   "");
    static_assert(noexcept(v.load()),                           "");
    static_assert(noexcept(v.store(1)),                         "");
    static_assert(noexcept(v.fetch_add(1)),                     "");
    static_assert(noexcept(v.bump()),                           "");
    static_assert(noexcept(v.compare_exchange_weak(e, 1)),      "");
    static_assert(noexcept(v.compare_exchange_strong(e, 1)),    "");
    static_assert(noexcept(static_cast<std::uint64_t>(v)),      "");

    const bool ok =
        noexcept(v.load())                        &&
        noexcept(v.store(1))                      &&
        noexcept(v.fetch_add(1))                  &&
        noexcept(v.bump())                        &&
        noexcept(v.compare_exchange_weak(e, 1))   &&
        noexcept(v.compare_exchange_strong(e, 1)) &&
        noexcept(static_cast<std::uint64_t>(v));
    D_AT_CHECK(ok);

    return true;
}


NS_END  // testing
NS_END  // djinterp

#undef D_AT_CHECK
