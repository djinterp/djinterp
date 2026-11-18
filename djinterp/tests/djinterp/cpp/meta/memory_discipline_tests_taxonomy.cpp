/*******************************************************************************
* djinterp [cpp]                            memory_discipline_tests_taxonomy.cpp
*
*
* path:      /tests/djinterp/cpp/meta/memory_discipline_tests_taxonomy.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.11.14
*******************************************************************************/
// djinterp
#include "memory_discipline_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_memory_discipline_enum_distinct
  the five discipline values are mutually exclusive positions
  Tests the following:
  - unknown, none, individual, pooled, arena are pairwise distinct
  - a discipline compares equal only to itself
*/
bool tests_memory_discipline_enum_distinct()
{
    const DMemoryDiscipline::value _all[] =
    {
        DMemoryDiscipline::unknown,
        DMemoryDiscipline::none,
        DMemoryDiscipline::individual,
        DMemoryDiscipline::pooled,
        DMemoryDiscipline::arena
    };

    for (std::size_t _i = 0; _i < 5; ++_i)
    {
        for (std::size_t _j = 0; _j < 5; ++_j)
        {
            if (_i == _j)
            {
                D_MD_CHECK(_all[_i] == _all[_j]);
            }
            else
            {
                D_MD_CHECK(_all[_i] != _all[_j]);
            }
        }
    }

    return true;
}

/*
tests_memory_discipline_predicates
  each single-value predicate recognises exactly its own discipline
  Tests the following:
  - is_no_allocation_discipline is true only for none
  - is_individual_discipline is true only for individual
  - is_pooled_discipline is true only for pooled
  - is_arena_discipline is true only for arena
  - is_unknown_discipline is true only for unknown
  - the predicates are usable in a constant expression
*/
bool tests_memory_discipline_predicates()
{
    // is_no_allocation_discipline
    D_MD_CHECK(is_no_allocation_discipline(DMemoryDiscipline::none));
    D_MD_CHECK(!is_no_allocation_discipline(DMemoryDiscipline::individual));
    D_MD_CHECK(!is_no_allocation_discipline(DMemoryDiscipline::pooled));
    D_MD_CHECK(!is_no_allocation_discipline(DMemoryDiscipline::arena));
    D_MD_CHECK(!is_no_allocation_discipline(DMemoryDiscipline::unknown));

    // is_individual_discipline
    D_MD_CHECK(is_individual_discipline(DMemoryDiscipline::individual));
    D_MD_CHECK(!is_individual_discipline(DMemoryDiscipline::pooled));
    D_MD_CHECK(!is_individual_discipline(DMemoryDiscipline::none));

    // is_pooled_discipline
    D_MD_CHECK(is_pooled_discipline(DMemoryDiscipline::pooled));
    D_MD_CHECK(!is_pooled_discipline(DMemoryDiscipline::individual));
    D_MD_CHECK(!is_pooled_discipline(DMemoryDiscipline::arena));

    // is_arena_discipline
    D_MD_CHECK(is_arena_discipline(DMemoryDiscipline::arena));
    D_MD_CHECK(!is_arena_discipline(DMemoryDiscipline::none));
    D_MD_CHECK(!is_arena_discipline(DMemoryDiscipline::pooled));

    // is_unknown_discipline
    D_MD_CHECK(is_unknown_discipline(DMemoryDiscipline::unknown));
    D_MD_CHECK(!is_unknown_discipline(DMemoryDiscipline::none));
    D_MD_CHECK(!is_unknown_discipline(DMemoryDiscipline::arena));

    // constant-expression usability
    static_assert(is_no_allocation_discipline(DMemoryDiscipline::none),
                  "none allocates nothing");
    static_assert(is_arena_discipline(DMemoryDiscipline::arena),
                  "arena is arena");

    return true;
}

/*
tests_memory_discipline_family_and_allocates
  the two grouping predicates partition the disciplines correctly
  Tests the following:
  - is_individual_family_discipline is true for individual and pooled only
  - it is false for none, arena, and unknown
  - discipline_allocates is true for individual, pooled, and arena
  - it is false for none and unknown
*/
bool tests_memory_discipline_family_and_allocates()
{
    // individual family = individual OR pooled
    D_MD_CHECK(is_individual_family_discipline(DMemoryDiscipline::individual));
    D_MD_CHECK(is_individual_family_discipline(DMemoryDiscipline::pooled));
    D_MD_CHECK(!is_individual_family_discipline(DMemoryDiscipline::none));
    D_MD_CHECK(!is_individual_family_discipline(DMemoryDiscipline::arena));
    D_MD_CHECK(!is_individual_family_discipline(DMemoryDiscipline::unknown));

    // allocates = individual OR pooled OR arena
    D_MD_CHECK(discipline_allocates(DMemoryDiscipline::individual));
    D_MD_CHECK(discipline_allocates(DMemoryDiscipline::pooled));
    D_MD_CHECK(discipline_allocates(DMemoryDiscipline::arena));
    D_MD_CHECK(!discipline_allocates(DMemoryDiscipline::none));
    D_MD_CHECK(!discipline_allocates(DMemoryDiscipline::unknown));

    return true;
}

/*
tests_memory_discipline_name
  the name spelling is stable and correct for every value
  Tests the following:
  - each discipline maps to its documented lowercase spelling
  - the spellings are distinct across the five values
*/
bool tests_memory_discipline_name()
{
    D_MD_CHECK(str_eq(memory_discipline_name(DMemoryDiscipline::none), "none"));
    D_MD_CHECK(str_eq(memory_discipline_name(DMemoryDiscipline::individual), "individual"));
    D_MD_CHECK(str_eq(memory_discipline_name(DMemoryDiscipline::pooled), "pooled"));
    D_MD_CHECK(str_eq(memory_discipline_name(DMemoryDiscipline::arena), "arena"));
    D_MD_CHECK(str_eq(memory_discipline_name(DMemoryDiscipline::unknown), "unknown"));

    return true;
}

NS_END  // testing
NS_END  // djinterp
