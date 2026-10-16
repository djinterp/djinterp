/*******************************************************************************
* djinterp [core]                          memory_discipline_tests_detection.cpp
*
*
* path:      /tests/djinterp/core/meta/memory_discipline_tests_detection.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "memory_discipline_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_memory_discipline_signal_kind_and_allocate
  the marker and the two allocate shapes are each detected precisely
  Tests the following:
  - has_strategy_kind_signal is true for a type declaring strategy_storage_kind,
    false otherwise (it reads the type as written - the classifier strips cv/ref
    upstream, so the raw marker signal does not detect it through a reference)
  - has_element_allocate_signal is true only for an allocate(size_t) surface
  - has_byte_allocate_signal is true only for an allocate(size_t, size_t) surface
  - the allocate signals use clean_t, so they DO see through cv/ref
  - the two allocate signals are independent (each fixture triggers exactly one)
*/
bool
tests_memory_discipline_signal_kind_and_allocate()
{
    // has_strategy_kind_signal: reads the type as written (no cv/ref stripping;
    // that is the classifier's job).  A reference form therefore does not detect.
    D_MD_CHECK(has_strategy_kind_signal<md_none>::value);
    D_MD_CHECK(has_strategy_kind_signal<md_arena_absent>::value);
    D_MD_CHECK(!has_strategy_kind_signal<md_unknown>::value);
    D_MD_CHECK(!has_strategy_kind_signal<const md_none&>::value);

    // has_element_allocate_signal: allocate(size_t) only
    D_MD_CHECK(has_element_allocate_signal<md_arena_absent>::value);
    D_MD_CHECK(has_element_allocate_signal<md_pooled>::value);
    D_MD_CHECK(!has_element_allocate_signal<md_byte_alloc>::value);
    D_MD_CHECK(!has_element_allocate_signal<md_none>::value);
    D_MD_CHECK(!has_element_allocate_signal<md_unknown>::value);

    // has_byte_allocate_signal: allocate(size_t, size_t) only
    D_MD_CHECK(has_byte_allocate_signal<md_byte_alloc>::value);
    D_MD_CHECK(!has_byte_allocate_signal<md_arena_absent>::value);
    D_MD_CHECK(!has_byte_allocate_signal<md_none>::value);
    D_MD_CHECK(!has_byte_allocate_signal<md_unknown>::value);

    // the allocate signals strip cv/ref (they route through clean_t), unlike the
    // marker signal above
    D_MD_CHECK(has_element_allocate_signal<const md_arena_absent&>::value);
    D_MD_CHECK(has_byte_allocate_signal<const md_byte_alloc&>::value);

    return true;
}

/*
tests_memory_discipline_signal_constants
  the two optional constants are detected by presence, not by value
  Tests the following:
  - has_pointer_stable_constant_signal is true exactly when pointer_stable exists
    (whether it is true or false), false when it is absent
  - has_individual_release_constant_signal is true exactly when
    supports_individual_release exists, false when absent
*/
bool
tests_memory_discipline_signal_constants()
{
    // pointer_stable present (true or false) vs absent
    D_MD_CHECK(has_pointer_stable_constant_signal<md_pooled>::value);
    D_MD_CHECK(has_pointer_stable_constant_signal<md_individual_false>::value);
    D_MD_CHECK(!has_pointer_stable_constant_signal<md_individual_absent>::value);
    D_MD_CHECK(!has_pointer_stable_constant_signal<md_arena_absent>::value);
    D_MD_CHECK(!has_pointer_stable_constant_signal<md_unknown>::value);

    // supports_individual_release present (true or false) vs absent
    D_MD_CHECK(has_individual_release_constant_signal<md_pooled>::value);
    D_MD_CHECK(has_individual_release_constant_signal<md_arena_false>::value);
    D_MD_CHECK(has_individual_release_constant_signal<md_individual_absent>::value);
    D_MD_CHECK(!has_individual_release_constant_signal<md_arena_absent>::value);
    D_MD_CHECK(!has_individual_release_constant_signal<md_unknown>::value);

    return true;
}

/*
tests_memory_discipline_read_constants
  the constant readers yield the value when present and false when absent
  Tests the following:
  - read_pointer_stable is the declared value when present (true or false), and
    the conservative false when absent
  - read_individual_release is the declared value when present (true or false),
    and the conservative false when absent
*/
bool
tests_memory_discipline_read_constants()
{
    // read_pointer_stable
    D_MD_CHECK(djinterp::internal::read_pointer_stable<md_pooled>::value == true);
    D_MD_CHECK(djinterp::internal::read_pointer_stable<md_individual_false>::value == false);
    D_MD_CHECK(djinterp::internal::read_pointer_stable<md_individual_absent>::value == false);
    D_MD_CHECK(djinterp::internal::read_pointer_stable<md_none>::value == false);

    // read_individual_release
    D_MD_CHECK(djinterp::internal::read_individual_release<md_pooled>::value == true);
    D_MD_CHECK(djinterp::internal::read_individual_release<md_arena_false>::value == false);
    D_MD_CHECK(djinterp::internal::read_individual_release<md_arena_absent>::value == false);
    D_MD_CHECK(djinterp::internal::read_individual_release<md_none>::value == false);

    return true;
}

NS_END  // testing
NS_END  // djinterp
