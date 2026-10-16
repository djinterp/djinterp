/*******************************************************************************
* djinterp [core]                            memory_discipline_tests_carrier.cpp
*
*
* path:      /tests/djinterp/core/meta/memory_discipline_tests_carrier.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <type_traits>
// djinterp
#include "memory_discipline_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_memory_discipline_carrier
  the carrier reifies a discipline value as a distinct type
  Tests the following:
  - memory_discipline_constant<D>::value equals D for each of the five values
  - it is an integral_constant whose value_type is memory_discipline
  - the five named aliases carry their corresponding value
  - distinct values yield distinct carrier types
*/
bool
tests_memory_discipline_carrier()
{
    // the carrier reports its value
    D_MD_CHECK(memory_discipline_constant<memory_discipline::none>::value
               == memory_discipline::none);
    D_MD_CHECK(memory_discipline_constant<memory_discipline::arena>::value
               == memory_discipline::arena);

    // it is an integral_constant over memory_discipline
    D_MD_CHECK((std::is_same<memory_discipline_constant<memory_discipline::pooled>::value_type,
                             memory_discipline>::value));
    D_MD_CHECK((std::is_base_of<
                    std::integral_constant<memory_discipline, memory_discipline::pooled>,
                    memory_discipline_constant<memory_discipline::pooled>>::value));

    // the five named aliases
    D_MD_CHECK(unknown_discipline::value    == memory_discipline::unknown);
    D_MD_CHECK(none_discipline::value       == memory_discipline::none);
    D_MD_CHECK(individual_discipline::value == memory_discipline::individual);
    D_MD_CHECK(pooled_discipline::value     == memory_discipline::pooled);
    D_MD_CHECK(arena_discipline::value      == memory_discipline::arena);

    // distinct values are distinct types
    D_MD_CHECK((!std::is_same<none_discipline, arena_discipline>::value));
    D_MD_CHECK((!std::is_same<individual_discipline, pooled_discipline>::value));

    // an alias matches the corresponding carrier instantiation
    D_MD_CHECK((std::is_same<pooled_discipline,
                             memory_discipline_constant<memory_discipline::pooled>>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
