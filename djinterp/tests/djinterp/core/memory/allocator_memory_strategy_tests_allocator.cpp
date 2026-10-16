/*******************************************************************************
* djinterp [testing]               allocator_memory_strategy_tests_allocator.cpp
*
* Section: the standard-allocator adapter.
*   allocator_memory_strategy.hpp - allocator_memory_strategy<Alloc>,
*                                   is_allocator_memory_strategy,
*                                   allocator_memory_strategy_c,
*                                   make_allocator_strategy
*
*   This is the bridge from std::allocator (an element SURFACE that is not a
* strategy) to a first-class dynamic element strategy: it adds the one thing the
* allocator lacks, a declared storage discipline, and forwards allocation to
* allocator_traits.  The contract test confirms the added discipline (dynamic,
* unstable, individually releasing) and the recognition test confirms the
* adapter is distinguishable both from a bare allocator and from a container
* that merely HAS an allocator_type.  The runtime test does a real heap
* round-trip through the traits, because the whole point of this strategy is
* that it actually allocates.
*
*
* path:      /tests/djinterp/core/memory/allocator_memory_strategy_tests_allocator.cpp
*                                        memory_strategy_tests_allocator.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <memory>
#include <type_traits>
#include <vector>
// djinterp
#include "allocator_memory_strategy_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_allocator_strategy_contract
  allocator_memory_strategy<Alloc>: the descriptive surface it adds.
  Tests the following:
  - the member aliases: allocator_type is the cleaned allocator, value_type is
    drawn from allocator_traits, and both track the element type
  - the added discipline: dynamic storage, NOT pointer-stable, individually
    releasing - std::allocator's actual semantics, now declared
  - it is a full element strategy and a member, hence allocating and NOT
    monotonic and NOT descriptive-only
  - cv-ref agnostic through the composites
*/
bool
tests_allocator_strategy_contract()
{

    using ams_i = allocator_memory_strategy<std::allocator<int>>;
    using ams_d = allocator_memory_strategy<std::allocator<double>>;

    // --- member aliases ---
    D_AL_CHECK((std::is_same<ams_i::allocator_type,
                             std::allocator<int>>::value));
    D_AL_CHECK((std::is_same<ams_i::value_type, int>::value));
    D_AL_CHECK((std::is_same<ams_d::value_type, double>::value));

    // --- the added discipline ---
    D_AL_CHECK(ams_i::strategy_storage_kind == storage_kind::dynamic_storage);
    D_AL_CHECK(!ams_i::pointer_stable);
    D_AL_CHECK(ams_i::supports_individual_release);

    // --- a full element strategy and a member ---
    D_AL_CHECK(is_element_strategy<ams_i>::value);
    D_AL_CHECK(!is_byte_strategy<ams_i>::value);
    D_AL_CHECK(is_memory_strategy<ams_i>::value);
    D_AL_CHECK(is_allocating_strategy<ams_i>::value);
    D_AL_CHECK(!is_descriptive_only_strategy<ams_i>::value);
    D_AL_CHECK(is_dynamic_strategy<ams_i>::value);
    D_AL_CHECK(supports_individual_release_strategy<ams_i>::value);
    D_AL_CHECK(!is_monotonic_strategy<ams_i>::value);
    D_AL_CHECK(!is_pointer_stable_strategy<ams_i>::value);

    // --- cv-ref agnostic ---
    D_AL_CHECK(is_element_strategy<const ams_i&>::value);
    D_AL_CHECK(is_dynamic_strategy<ams_i&&>::value);
    D_AL_CHECK(memory_strategy_kind_of<const ams_i&>::value
                   == storage_kind::dynamic_storage);

    return true;
}


/*
tests_allocator_strategy_runtime
  allocator_memory_strategy<Alloc>: a real heap round-trip.
  Tests the following:
  - default construction, then allocate(n) yields non-null, writable storage for
    n objects that reads back
  - allocator() returns the wrapped allocator (equal to a default std::allocator
    of the same type, which compares equal by definition)
  - deallocate returns the storage without error
  - explicit construction from a provided allocator instance also works
  - a second allocate/deallocate cycle reuses the strategy
*/
bool
tests_allocator_strategy_runtime()
{

    allocator_memory_strategy<std::allocator<int>> s;

    // --- allocate: non-null, writable, reads back ---
    int* p = s.allocate(4);
    D_AL_CHECK(p != nullptr);
    for (int i = 0; i < 4; ++i)
    {
        p[i] = i * 10;
    }
    D_AL_CHECK(p[0] == 0 && p[1] == 10 && p[2] == 20 && p[3] == 30);

    // --- allocator() returns the wrapped allocator ---
    D_AL_CHECK(s.allocator() == std::allocator<int>());

    // --- deallocate returns the storage ---
    s.deallocate(p, 4);

    // --- explicit construction from a provided allocator ---
    std::allocator<int> raw;
    allocator_memory_strategy<std::allocator<int>> s2(raw);
    int* q = s2.allocate(2);
    D_AL_CHECK(q != nullptr);
    q[0] = 100;
    q[1] = 200;
    D_AL_CHECK(q[0] == 100 && q[1] == 200);
    s2.deallocate(q, 2);

    // --- a second cycle reuses the strategy ---
    int* r = s.allocate(1);
    D_AL_CHECK(r != nullptr);
    *r = 7;
    D_AL_CHECK(*r == 7);
    s.deallocate(r, 1);

    return true;
}


/*
tests_allocator_strategy_recognition
  is_allocator_memory_strategy / allocator_memory_strategy_c.
  Tests the following:
  - the adapter is recognized (it exposes an allocator_type AND is an element
    strategy)
  - a bare std::allocator is NOT recognized: it is an element surface but
    exposes no nested allocator_type
  - a std::vector is NOT recognized: it HAS an allocator_type but is not an
    element strategy (no allocate/deallocate surface).  This is the check that
    keeps "has an allocator" apart from "is an allocator strategy"
  - an element strategy without an allocator_type is not recognized, and neither
    is a plain type
  - the concept agrees with the trait
*/
bool
tests_allocator_strategy_recognition()
{

    using ams_i = allocator_memory_strategy<std::allocator<int>>;

    // --- the adapter is recognized ---
    D_AL_CHECK(is_allocator_memory_strategy<ams_i>::value);
    D_AL_CHECK(allocator_memory_strategy_c<ams_i>);

    // --- a bare allocator is not: no nested allocator_type ---
    D_AL_CHECK(is_element_strategy<std::allocator<int>>::value);
    D_AL_CHECK(!is_allocator_memory_strategy<std::allocator<int>>::value);

    // --- a vector is not: has allocator_type but no allocate surface ---
    D_AL_CHECK(!is_allocator_memory_strategy<std::vector<int>>::value);

    // --- an element strategy without an allocator_type is not ---
    D_AL_CHECK(is_element_strategy<ms_element_dynamic>::value);
    D_AL_CHECK(!is_allocator_memory_strategy<ms_element_dynamic>::value);
    D_AL_CHECK(
        !is_allocator_memory_strategy<static_buffer_strategy<int, 8>>::value);

    // --- nothing at all ---
    D_AL_CHECK(!is_allocator_memory_strategy<ms_plain_type>::value);

    // --- the concept agrees with the trait ---
    D_AL_CHECK(allocator_memory_strategy_c<ams_i>
                   == is_allocator_memory_strategy_v<ams_i>);
    D_AL_CHECK(allocator_memory_strategy_c<std::vector<int>>
                   == is_allocator_memory_strategy_v<std::vector<int>>);

    // --- cv-ref agnostic ---
    D_AL_CHECK(is_allocator_memory_strategy<const ams_i&>::value);
    D_AL_CHECK(is_allocator_memory_strategy_v<ams_i>
                   == is_allocator_memory_strategy<ams_i>::value);

    return true;
}


/*
tests_allocator_strategy_factory
  make_allocator_strategy: deduce the allocator and wrap it.
  Tests the following:
  - the return type is allocator_memory_strategy over the cleaned allocator type
  - the factory result is itself a recognized allocator strategy and allocates
  - a const / reference-qualified argument is cleaned in the result type
*/
bool
tests_allocator_strategy_factory()
{

    // --- return type over the cleaned allocator ---
    auto s = make_allocator_strategy(std::allocator<int>());
    D_AL_CHECK((std::is_same<decltype(s),
                   allocator_memory_strategy<std::allocator<int>>>::value));

    auto sd = make_allocator_strategy(std::allocator<double>());
    D_AL_CHECK((std::is_same<decltype(sd),
                   allocator_memory_strategy<std::allocator<double>>>::value));

    // --- the result is a recognized allocator strategy that allocates ---
    D_AL_CHECK(is_allocator_memory_strategy<decltype(s)>::value);
    D_AL_CHECK(is_allocating_strategy<decltype(s)>::value);
    int* p = s.allocate(3);
    D_AL_CHECK(p != nullptr);
    p[0] = 1;
    p[2] = 3;
    D_AL_CHECK(p[0] == 1 && p[2] == 3);
    s.deallocate(p, 3);

    // --- a const / reference argument is cleaned in the result ---
    const std::allocator<char> ca;
    const std::allocator<char>& car = ca;
    auto sc = make_allocator_strategy(car);
    D_AL_CHECK((std::is_same<decltype(sc),
                   allocator_memory_strategy<std::allocator<char>>>::value));

    return true;
}


NS_END  // testing
NS_END  // djinterp
