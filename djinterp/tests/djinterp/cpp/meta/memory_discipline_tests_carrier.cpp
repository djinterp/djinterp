/*******************************************************************************
* djinterp [cpp][meta]                       memory_discipline_tests_carrier.cpp
*
*
* path:      /tests/djinterp/cpp/meta/memory_discipline_tests_carrier.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.11.14
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
  - it is an integral_constant whose value_type is memory_discipline::value
    (the enum inside the memory_discipline struct)
  - the five named aliases carry their corresponding value
  - distinct values yield distinct carrier types
*/
bool tests_memory_discipline_carrier()
{
    // the carrier reports its value
    D_MD_CHECK(memory_discipline_constant<DMemoryDiscipline::none>::value
               == DMemoryDiscipline::none);
    D_MD_CHECK(memory_discipline_constant<DMemoryDiscipline::arena>::value
               == DMemoryDiscipline::arena);

    // it is an integral_constant over memory_discipline::value
    D_MD_CHECK((std::is_same<
                    memory_discipline_constant<
                        DMemoryDiscipline::pooled>::value_type,
                    DMemoryDiscipline::value>::value));
    D_MD_CHECK((std::is_same<
                    std::integral_constant<DMemoryDiscipline::value,
                                           DMemoryDiscipline::pooled>,
                    memory_discipline_constant<
                        DMemoryDiscipline::pooled>>::value));

    // the five named aliases
    D_MD_CHECK(unknown_discipline::value    == DMemoryDiscipline::unknown);
    D_MD_CHECK(none_discipline::value       == DMemoryDiscipline::none);
    D_MD_CHECK(individual_discipline::value == DMemoryDiscipline::individual);
    D_MD_CHECK(pooled_discipline::value     == DMemoryDiscipline::pooled);
    D_MD_CHECK(arena_discipline::value      == DMemoryDiscipline::arena);

    // distinct values are distinct types
    D_MD_CHECK((!std::is_same<none_discipline, arena_discipline>::value));
    D_MD_CHECK((!std::is_same<individual_discipline, pooled_discipline>::value));

    // an alias matches the corresponding carrier instantiation
    D_MD_CHECK((std::is_same<pooled_discipline,
                             memory_discipline_constant<DMemoryDiscipline::pooled>>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
