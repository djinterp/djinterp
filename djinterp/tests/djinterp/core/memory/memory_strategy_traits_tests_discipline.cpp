/*******************************************************************************
* djinterp [testing]                 memory_strategy_traits_tests_discipline.cpp
*
* Section: storage-discipline confirmation.
*   memory_strategy_traits.hpp VI - memory_strategy_kind_of, is_static_strategy,
*                                   is_fixed_strategy, is_dynamic_strategy
*
*   The kind is read straight from the declared constant, so these tests are
* mostly a table: each fixture against the value it declares.  Two things make
* the table worth writing.  First, a type with NO constant resolves to
* `unknown` rather than failing - the accessor is total over non-members (the
* ONE type it is not total over is the mistyped-kind fixture, which is excluded
* here by construction and pinned in its own place).  Second, `unknown` is a
* first-class answer: the explicitly-unknown fixture is a member whose kind is
* unknown and whose three discipline predicates are therefore all false, which
* is asserted directly so that "member" and "has a known discipline" stay
* visibly distinct.
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_traits_tests_discipline.cpp
*                                       memory_strategy_tests_discipline.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <memory>
#include <type_traits>
// djinterp
#include "memory_strategy_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_storage_discipline_kind
  memory_strategy_kind_of: the declared storage_kind of a strategy (core VI).
  Tests the following:
  - each declared kind is read back exactly, across descriptive fixtures,
    operational fixtures, and concrete providers
  - a type with no kind constant resolves to `unknown`, not a hard error: the
    accessor is total over non-members (ms_plain_type, int)
  - the concrete providers pin their contracts: the arenas split static vs
    fixed, the allocator wrapper is dynamic, both static buffers are static
  - cv-ref agnostic, and the `_v` companion agrees
  NOTE: ms_mistyped_kind_constant is now included in the totality checks - the
  type gate rejects its wrong-typed constant, so the accessor returns unknown on
  it rather than hard-erroring.
*/
bool
tests_storage_discipline_kind()
{

    // --- declared kind read back exactly (descriptive fixtures) ---
    D_MT_CHECK(memory_strategy_kind_of<ms_descriptive_static>::value
                   == storage_kind::static_storage);
    D_MT_CHECK(memory_strategy_kind_of<ms_descriptive_fixed>::value
                   == storage_kind::fixed_storage);
    D_MT_CHECK(memory_strategy_kind_of<ms_descriptive_dynamic>::value
                   == storage_kind::dynamic_storage);
    D_MT_CHECK(memory_strategy_kind_of<ms_descriptive_unknown>::value
                   == storage_kind::unknown);

    // --- operational fixtures carry a kind too ---
    D_MT_CHECK(memory_strategy_kind_of<ms_element_dynamic>::value
                   == storage_kind::dynamic_storage);
    D_MT_CHECK(memory_strategy_kind_of<ms_element_static_monotonic>::value
                   == storage_kind::static_storage);
    D_MT_CHECK(memory_strategy_kind_of<ms_byte_fixed>::value
                   == storage_kind::fixed_storage);

    // --- concrete providers ---
    D_MT_CHECK(memory_strategy_kind_of<monotonic_byte_arena<64>>::value
                   == storage_kind::static_storage);
    D_MT_CHECK(memory_strategy_kind_of<dynamic_byte_arena>::value
                   == storage_kind::fixed_storage);
    D_MT_CHECK(memory_strategy_kind_of<static_buffer_strategy<int, 8>>::value
                   == storage_kind::static_storage);
    D_MT_CHECK(memory_strategy_kind_of<static_extent_strategy<int, 8>>::value
                   == storage_kind::static_storage);
    D_MT_CHECK(
        memory_strategy_kind_of<allocator_memory_strategy<std::allocator<int>>>
            ::value == storage_kind::dynamic_storage);

    // --- no constant resolves to `unknown`, not a hard error ---
    D_MT_CHECK(memory_strategy_kind_of<ms_plain_type>::value
                   == storage_kind::unknown);
    D_MT_CHECK(memory_strategy_kind_of<int>::value == storage_kind::unknown);
    //     a full element surface with no constant is likewise unknown
    D_MT_CHECK(memory_strategy_kind_of<std::allocator<int>>::value
                   == storage_kind::unknown);
    //     a WRONG-TYPED constant is rejected by the type gate, so the accessor is
    //     total on it too: unknown, not the former hard error
    D_MT_CHECK(memory_strategy_kind_of<ms_mistyped_kind_constant>::value
                   == storage_kind::unknown);

    // --- cv-ref agnostic ---
    D_MT_CHECK(memory_strategy_kind_of<const ms_descriptive_fixed>::value
                   == storage_kind::fixed_storage);
    D_MT_CHECK(memory_strategy_kind_of<ms_descriptive_fixed&>::value
                   == storage_kind::fixed_storage);
    D_MT_CHECK(memory_strategy_kind_of<const ms_descriptive_fixed&>::value
                   == storage_kind::fixed_storage);

    // --- the `_v` companion agrees ---
    D_MT_CHECK(memory_strategy_kind_of_v<ms_descriptive_dynamic>
                   == memory_strategy_kind_of<ms_descriptive_dynamic>::value);
    D_MT_CHECK(memory_strategy_kind_of_v<ms_plain_type>
                   == memory_strategy_kind_of<ms_plain_type>::value);

    return true;
}


/*
tests_storage_discipline_predicates
  is_static_strategy / is_fixed_strategy / is_dynamic_strategy (core VI).
  Tests the following:
  - each predicate is true for exactly the declared kind and false otherwise
  - the three are MUTUALLY EXCLUSIVE for any type with a known kind: at most one
    holds, asserted over several fixtures
  - the explicitly-unknown fixture is the sharp edge: it is a member, yet all
    three predicates are false.  Membership does not imply a known discipline
  - a non-member answers false to all three (kind is unknown), with no hard
    error - the predicates are total exactly where the kind accessor is
  - each predicate agrees with the kind accessor it abbreviates
  - cv-ref agnostic, and the `_v` companions agree
*/
bool
tests_storage_discipline_predicates()
{

    // --- static ---
    D_MT_CHECK(is_static_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(!is_fixed_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(!is_dynamic_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(is_static_strategy<monotonic_byte_arena<64>>::value);
    D_MT_CHECK(is_static_strategy<static_buffer_strategy<int, 8>>::value);
    D_MT_CHECK(is_static_strategy<static_extent_strategy<int, 8>>::value);

    // --- fixed ---
    D_MT_CHECK(is_fixed_strategy<ms_descriptive_fixed>::value);
    D_MT_CHECK(!is_static_strategy<ms_descriptive_fixed>::value);
    D_MT_CHECK(!is_dynamic_strategy<ms_descriptive_fixed>::value);
    D_MT_CHECK(is_fixed_strategy<dynamic_byte_arena>::value);
    D_MT_CHECK(is_fixed_strategy<ms_byte_fixed>::value);

    // --- dynamic ---
    D_MT_CHECK(is_dynamic_strategy<ms_descriptive_dynamic>::value);
    D_MT_CHECK(!is_static_strategy<ms_descriptive_dynamic>::value);
    D_MT_CHECK(!is_fixed_strategy<ms_descriptive_dynamic>::value);
    D_MT_CHECK(is_dynamic_strategy<ms_element_dynamic>::value);
    D_MT_CHECK(
        is_dynamic_strategy<allocator_memory_strategy<std::allocator<int>>>
            ::value);

    // --- the explicitly-unknown member: all three false ---
    D_MT_CHECK(is_memory_strategy<ms_descriptive_unknown>::value);
    D_MT_CHECK(!is_static_strategy<ms_descriptive_unknown>::value);
    D_MT_CHECK(!is_fixed_strategy<ms_descriptive_unknown>::value);
    D_MT_CHECK(!is_dynamic_strategy<ms_descriptive_unknown>::value);

    // --- a non-member: all three false, no hard error ---
    D_MT_CHECK(!is_static_strategy<ms_plain_type>::value);
    D_MT_CHECK(!is_fixed_strategy<ms_plain_type>::value);
    D_MT_CHECK(!is_dynamic_strategy<ms_plain_type>::value);
    D_MT_CHECK(!is_static_strategy<std::allocator<int>>::value);
    D_MT_CHECK(!is_dynamic_strategy<std::allocator<int>>::value);

    // --- mutual exclusivity over known kinds ---
    D_MT_CHECK(( int(is_static_strategy<ms_descriptive_static>::value)
                 + int(is_fixed_strategy<ms_descriptive_static>::value)
                 + int(is_dynamic_strategy<ms_descriptive_static>::value) )
                   == 1);
    D_MT_CHECK(( int(is_static_strategy<dynamic_byte_arena>::value)
                 + int(is_fixed_strategy<dynamic_byte_arena>::value)
                 + int(is_dynamic_strategy<dynamic_byte_arena>::value) )
                   == 1);
    //     unknown / non-member: none hold
    D_MT_CHECK(( int(is_static_strategy<ms_descriptive_unknown>::value)
                 + int(is_fixed_strategy<ms_descriptive_unknown>::value)
                 + int(is_dynamic_strategy<ms_descriptive_unknown>::value) )
                   == 0);
    D_MT_CHECK(( int(is_static_strategy<ms_plain_type>::value)
                 + int(is_fixed_strategy<ms_plain_type>::value)
                 + int(is_dynamic_strategy<ms_plain_type>::value) )
                   == 0);

    // --- each predicate agrees with the kind accessor ---
    D_MT_CHECK(is_static_strategy<ms_descriptive_static>::value
                   == ( memory_strategy_kind_of<ms_descriptive_static>::value
                        == storage_kind::static_storage ));
    D_MT_CHECK(is_dynamic_strategy<ms_element_dynamic>::value
                   == ( memory_strategy_kind_of<ms_element_dynamic>::value
                        == storage_kind::dynamic_storage ));

    // --- cv-ref agnostic ---
    D_MT_CHECK(is_static_strategy<const ms_descriptive_static&>::value);
    D_MT_CHECK(is_fixed_strategy<const dynamic_byte_arena&>::value);
    D_MT_CHECK(is_dynamic_strategy<ms_element_dynamic&>::value);

    // --- the `_v` companions agree ---
    D_MT_CHECK(is_static_strategy_v<ms_descriptive_static>
                   == is_static_strategy<ms_descriptive_static>::value);
    D_MT_CHECK(is_fixed_strategy_v<ms_descriptive_fixed>
                   == is_fixed_strategy<ms_descriptive_fixed>::value);
    D_MT_CHECK(is_dynamic_strategy_v<ms_descriptive_dynamic>
                   == is_dynamic_strategy<ms_descriptive_dynamic>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
