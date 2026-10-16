/*******************************************************************************
* djinterp [testing]                 memory_strategy_traits_tests_vocabulary.cpp
*
* Section: the two enumerations the axis is written in.
*   memory_strategy_traits.hpp I  - storage_kind
*   memory_strategy_traits.hpp IV - strategy_typing
*
*   These are the alphabet.  Every other section spells answers with them, so
* they are checked first and on their own: that they are SCOPED (no silent
* integral decay through an API whose whole job is to be explicit), that their
* enumerators are pairwise distinct, and that they survive being declared as
* the static constexpr class constants the contract requires them to be.
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_traits_tests_vocabulary.cpp
*                                      memory_strategy_tests_vocabulary.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "memory_strategy_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_storage_kind_vocabulary
  The storage_kind enumeration: the storage-discipline vocabulary.
  Tests the following:
  - storage_kind is an enumeration
  - it is SCOPED: no implicit conversion to its underlying integral type, in
    either direction, so a kind can never be silently compared with an int
  - all four enumerators are pairwise distinct (six comparisons)
  - enumerators are constant expressions, usable in a static constexpr class
    member - which is precisely how every strategy declares its discipline
  - the constant declared by a strategy round-trips: what it declared is what
    the vocabulary reads back
*/
bool
tests_storage_kind_vocabulary()
{

    // --- an enumeration, and a scoped one ---
    D_MT_CHECK(std::is_enum<storage_kind>::value);
    D_MT_CHECK(!std::is_convertible<storage_kind, int>::value);
    D_MT_CHECK(!std::is_convertible<int, storage_kind>::value);

    // --- four pairwise-distinct enumerators ---
    D_MT_CHECK(storage_kind::static_storage != storage_kind::fixed_storage);
    D_MT_CHECK(storage_kind::static_storage != storage_kind::dynamic_storage);
    D_MT_CHECK(storage_kind::static_storage != storage_kind::unknown);
    D_MT_CHECK(storage_kind::fixed_storage  != storage_kind::dynamic_storage);
    D_MT_CHECK(storage_kind::fixed_storage  != storage_kind::unknown);
    D_MT_CHECK(storage_kind::dynamic_storage != storage_kind::unknown);

    // --- usable as constant expressions ---
    constexpr storage_kind k = storage_kind::fixed_storage;
    D_MT_CHECK(k == storage_kind::fixed_storage);

    // --- the class-constant round trip (the contract's only mandatory term) ---
    D_MT_CHECK(ms_descriptive_static::strategy_storage_kind
                   == storage_kind::static_storage);
    D_MT_CHECK(ms_descriptive_fixed::strategy_storage_kind
                   == storage_kind::fixed_storage);
    D_MT_CHECK(ms_descriptive_dynamic::strategy_storage_kind
                   == storage_kind::dynamic_storage);
    D_MT_CHECK(ms_descriptive_unknown::strategy_storage_kind
                   == storage_kind::unknown);

    return true;
}


/*
tests_strategy_typing_vocabulary
  The strategy_typing enumeration: which typed surface a strategy speaks.
  Tests the following:
  - strategy_typing is an enumeration
  - it is SCOPED, in both conversion directions
  - all three enumerators are pairwise distinct
  - enumerators are constant expressions
  - `none` is a real answer, not an error code: it is what a descriptive-only
    strategy legitimately reports
*/
bool
tests_strategy_typing_vocabulary()
{

    // --- an enumeration, and a scoped one ---
    D_MT_CHECK(std::is_enum<strategy_typing>::value);
    D_MT_CHECK(!std::is_convertible<strategy_typing, int>::value);
    D_MT_CHECK(!std::is_convertible<int, strategy_typing>::value);

    // --- three pairwise-distinct enumerators ---
    D_MT_CHECK(strategy_typing::element_typed != strategy_typing::byte_typed);
    D_MT_CHECK(strategy_typing::element_typed != strategy_typing::none);
    D_MT_CHECK(strategy_typing::byte_typed    != strategy_typing::none);

    // --- usable as constant expressions ---
    constexpr strategy_typing t = strategy_typing::byte_typed;
    D_MT_CHECK(t == strategy_typing::byte_typed);

    // --- `none` is an answer a valid strategy gives ---
    D_MT_CHECK(strategy_typing_of<ms_descriptive_static>::value
                   == strategy_typing::none);
    D_MT_CHECK(is_memory_strategy<ms_descriptive_static>::value);

    return true;
}


NS_END  // testing
NS_END  // djinterp
