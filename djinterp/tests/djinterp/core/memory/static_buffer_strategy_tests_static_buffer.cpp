/*******************************************************************************
* djinterp [testing]              static_buffer_strategy_tests_static_buffer.cpp
*
* Section: the inline static buffer and its descriptive twin.
*   static_buffer_strategy.hpp - static_buffer_strategy<T, N>,
*                                static_extent_strategy<T, N>,
*                                is_static_buffer_strategy,
*                                static_buffer_strategy_c
*
*   static_buffer_strategy is the concrete static, pointer-stable, monotonic
* element strategy: a bump allocator over an in-object array.  Its runtime test
* is where the section earns its keep - allocation is contiguous, bounded, and
* throws on overflow, and reset reclaims wholesale.  static_extent_strategy is
* the same shape MINUS the allocation: a descriptive-only marker the container
* fills itself.  The recognition trait accepts BOTH (static plus a compile-time
* extent), and the tests assert that shared answer as well as the ways a type
* falls out of it (no extent, or not static).
*   N is kept positive throughout: a zero-length buffer is an ill-formed array,
* which is a property of the array, not of this strategy, and out of scope.
*
*
* path:      /tests/djinterp/core/memory/static_buffer_strategy_tests_static_buffer.cpp
*                                   memory_strategy_tests_static_buffer.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstdint>
#include <new>
#include <type_traits>
// djinterp
#include "static_buffer_strategy_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_static_buffer_contract
  static_buffer_strategy<T, N>: the descriptive surface and type properties.
  Tests the following:
  - value_type is T, and the compile-time extent constant is N
  - the declared discipline: static, pointer-stable, NOT individually releasing
  - it is a full element strategy, a member, allocating, monotonic, and static
  - it is NON-COPYABLE by design (the buffer is in-object, so a copy would alias
    slots) - asserted through the type traits
  - cv-ref agnostic through the composites
*/
bool
tests_static_buffer_contract()
{

    using buf_i = static_buffer_strategy<int, 8>;
    using buf_d = static_buffer_strategy<double, 4>;

    // --- aliases and the extent constant ---
    D_SB_CHECK((std::is_same<buf_i::value_type, int>::value));
    D_SB_CHECK((std::is_same<buf_d::value_type, double>::value));
    D_SB_CHECK(buf_i::extent == 8);
    D_SB_CHECK(buf_d::extent == 4);

    // --- declared discipline ---
    D_SB_CHECK(buf_i::strategy_storage_kind == storage_kind::static_storage);
    D_SB_CHECK(buf_i::pointer_stable);
    D_SB_CHECK(!buf_i::supports_individual_release);

    // --- classification ---
    D_SB_CHECK(is_element_strategy<buf_i>::value);
    D_SB_CHECK(!is_byte_strategy<buf_i>::value);
    D_SB_CHECK(is_memory_strategy<buf_i>::value);
    D_SB_CHECK(is_allocating_strategy<buf_i>::value);
    D_SB_CHECK(!is_descriptive_only_strategy<buf_i>::value);
    D_SB_CHECK(is_static_strategy<buf_i>::value);
    D_SB_CHECK(is_pointer_stable_strategy<buf_i>::value);
    D_SB_CHECK(is_monotonic_strategy<buf_i>::value);
    D_SB_CHECK(!supports_individual_release_strategy<buf_i>::value);

    // --- non-copyable by design ---
    D_SB_CHECK(!std::is_copy_constructible<buf_i>::value);
    D_SB_CHECK(!std::is_copy_assignable<buf_i>::value);
    D_SB_CHECK(std::is_default_constructible<buf_i>::value);

    // --- cv-ref agnostic ---
    D_SB_CHECK(is_element_strategy<const buf_i&>::value);
    D_SB_CHECK(is_monotonic_strategy<buf_i&&>::value);
    D_SB_CHECK(memory_strategy_kind_of<const buf_i&>::value
                   == storage_kind::static_storage);

    return true;
}


/*
tests_static_buffer_runtime
  static_buffer_strategy<T, N>: bump allocation, bounds, and reset.
  Tests the following:
  - a fresh buffer reports capacity N and size 0
  - allocate(n) returns non-null storage aligned for T, advances size by exactly
    n, and hands out CONTIGUOUS slots (the second block begins where the first
    ended)
  - the storage is writable and reads back
  - deallocate is a no-op: size is unchanged
  - allocation that would exceed N throws std::bad_alloc, and the failed request
    does not advance size
  - reset returns size to 0 and the full capacity is available again
  - a fresh over-capacity request also throws
  - the bounds check is overflow-safe: a request so large that head + n would
    wrap still throws rather than passing a wrapped comparison
*/
bool
tests_static_buffer_runtime()
{

    static_buffer_strategy<int, 8> buf;

    // --- fresh state ---
    D_SB_CHECK(buf.capacity() == 8);
    D_SB_CHECK(buf.size() == 0);

    // --- allocate: non-null, aligned, advances size, contiguous ---
    int* p = buf.allocate(3);
    D_SB_CHECK(p != nullptr);
    D_SB_CHECK(reinterpret_cast<std::uintptr_t>(p) % alignof(int) == 0);
    D_SB_CHECK(buf.size() == 3);
    int* q = buf.allocate(5);
    D_SB_CHECK(q == p + 3);
    D_SB_CHECK(buf.size() == 8);

    // --- writable, reads back ---
    for (int i = 0; i < 3; ++i)
    {
        p[i] = i + 1;
    }
    q[0] = 40;
    q[4] = 80;
    D_SB_CHECK(p[0] == 1 && p[1] == 2 && p[2] == 3);
    D_SB_CHECK(q[0] == 40 && q[4] == 80);

    // --- deallocate is a no-op ---
    buf.deallocate(p, 3);
    D_SB_CHECK(buf.size() == 8);

    // --- overflow throws, and does not advance size ---
    bool threw = false;
    try
    {
        buf.allocate(1);
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }
    D_SB_CHECK(threw);
    D_SB_CHECK(buf.size() == 8);

    // --- reset reclaims wholesale ---
    buf.reset();
    D_SB_CHECK(buf.size() == 0);
    int* r = buf.allocate(8);
    D_SB_CHECK(r != nullptr);
    D_SB_CHECK(buf.size() == 8);

    // --- a fresh over-capacity request throws ---
    static_buffer_strategy<int, 8> buf2;
    bool threw2 = false;
    try
    {
        buf2.allocate(9);
    }
    catch (const std::bad_alloc&)
    {
        threw2 = true;
    }
    D_SB_CHECK(threw2);
    D_SB_CHECK(buf2.size() == 0);

    // --- a request so large that head + n would wrap still throws (the bounds
    //     check is overflow-safe, not a wrapping addition) ---
    static_buffer_strategy<int, 8> buf3;
    bool threw3 = false;
    try
    {
        buf3.allocate(SIZE_MAX);
    }
    catch (const std::bad_alloc&)
    {
        threw3 = true;
    }
    D_SB_CHECK(threw3);
    D_SB_CHECK(buf3.size() == 0);

    return true;
}


/*
tests_static_extent_contract
  static_extent_strategy<T, N>: the descriptive-only static marker.
  Tests the following:
  - value_type is T, the extent constant is N, and the discipline is static +
    pointer-stable
  - it is a member and static, but it does NOT allocate: no element surface, so
    it is descriptive-only and NOT monotonic (monotonicity needs allocation)
  - it is pointer-stable at both trait and concept level (the membership gate
    passes), unlike a bare stability constant on a non-member
  - cv-ref agnostic through the composites
*/
bool
tests_static_extent_contract()
{

    using ext_d = static_extent_strategy<double, 8>;
    using ext_c = static_extent_strategy<char, 3>;

    // --- aliases, extent, discipline ---
    D_SB_CHECK((std::is_same<ext_d::value_type, double>::value));
    D_SB_CHECK(ext_d::extent == 8);
    D_SB_CHECK(ext_c::extent == 3);
    D_SB_CHECK(ext_d::strategy_storage_kind == storage_kind::static_storage);
    D_SB_CHECK(ext_d::pointer_stable);

    // --- member and static, but does not allocate ---
    D_SB_CHECK(is_memory_strategy<ext_d>::value);
    D_SB_CHECK(is_static_strategy<ext_d>::value);
    D_SB_CHECK(is_descriptive_only_strategy<ext_d>::value);
    D_SB_CHECK(!is_allocating_strategy<ext_d>::value);
    D_SB_CHECK(!is_element_strategy<ext_d>::value);
    D_SB_CHECK(!is_byte_strategy<ext_d>::value);
    D_SB_CHECK(!is_monotonic_strategy<ext_d>::value);

    // --- pointer-stable at trait AND concept level (membership gate passes) ---
    D_SB_CHECK(is_pointer_stable_strategy<ext_d>::value);
    D_SB_CHECK(pointer_stable_strategy<ext_d>);

    // --- cv-ref agnostic ---
    D_SB_CHECK(is_descriptive_only_strategy<const ext_d&>::value);
    D_SB_CHECK(is_static_strategy<ext_d&&>::value);

    return true;
}


/*
tests_static_buffer_recognition
  is_static_buffer_strategy / static_buffer_strategy_c.
  Tests the following:
  - the trait accepts BOTH the allocating buffer and the descriptive extent
    marker: the criterion is member + static + a compile-time extent, which both
    satisfy
  - it rejects a static member with NO extent (the arena, a plain descriptive
    fixture): the extent term is not redundant
  - it rejects a member that HAS an extent-like shape but is not static
    (none here declares extent while non-static, so the discipline term is
    checked via a dynamic strategy that also lacks extent)
  - a non-member is rejected
  - the concept agrees with the trait
*/
bool
tests_static_buffer_recognition()
{

    using buf_i = static_buffer_strategy<int, 8>;
    using ext_i = static_extent_strategy<int, 8>;

    // --- accepts both the buffer and the extent marker ---
    D_SB_CHECK(is_static_buffer_strategy<buf_i>::value);
    D_SB_CHECK(is_static_buffer_strategy<ext_i>::value);
    D_SB_CHECK(static_buffer_strategy_c<buf_i>);
    D_SB_CHECK(static_buffer_strategy_c<ext_i>);

    // --- rejects a static member with no extent ---
    D_SB_CHECK(is_static_strategy<monotonic_byte_arena<64>>::value);
    D_SB_CHECK(!is_static_buffer_strategy<monotonic_byte_arena<64>>::value);
    D_SB_CHECK(is_static_strategy<ms_descriptive_static>::value);
    D_SB_CHECK(!is_static_buffer_strategy<ms_descriptive_static>::value);

    // --- rejects a member that is not static (no extent either) ---
    D_SB_CHECK(!is_static_buffer_strategy<ms_element_dynamic>::value);
    D_SB_CHECK(
        !is_static_buffer_strategy<
            allocator_memory_strategy<std::allocator<int>>>::value);

    // --- rejects a non-member ---
    D_SB_CHECK(!is_static_buffer_strategy<ms_plain_type>::value);
    D_SB_CHECK(!is_static_buffer_strategy<int>::value);

    // --- the concept agrees with the trait ---
    D_SB_CHECK(static_buffer_strategy_c<buf_i>
                   == is_static_buffer_strategy_v<buf_i>);
    D_SB_CHECK(static_buffer_strategy_c<monotonic_byte_arena<64>>
                   == is_static_buffer_strategy_v<monotonic_byte_arena<64>>);

    // --- cv-ref agnostic ---
    D_SB_CHECK(is_static_buffer_strategy<const buf_i&>::value);
    D_SB_CHECK(is_static_buffer_strategy_v<ext_i>
                   == is_static_buffer_strategy<ext_i>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
