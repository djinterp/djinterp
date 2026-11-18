/*******************************************************************************
* djinterp [cpp]                              memory_discipline_tests_bridge.cpp
*
*
* path:      /tests/djinterp/cpp/meta/memory_discipline_tests_bridge.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.11.14
*******************************************************************************/
// djinterp
#include "memory_discipline_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_memory_discipline_implies_siting
  each discipline forces the storage siting the model prescribes
  Tests the following:
  - none forces static storage (inline, no allocator)
  - individual and pooled force dynamic storage (an acquired region)
  - arena forces no siting (unknown - a bump may run over inline or heap)
  - unknown forces no siting (unknown)
*/
bool tests_memory_discipline_implies_siting()
{
    D_MD_CHECK(discipline_implies_siting(DMemoryDiscipline::none)
               == DStorageDuration::static_storage);
    D_MD_CHECK(discipline_implies_siting(DMemoryDiscipline::individual)
               == DStorageDuration::dynamic_storage);
    D_MD_CHECK(discipline_implies_siting(DMemoryDiscipline::pooled)
               == DStorageDuration::dynamic_storage);
    D_MD_CHECK(discipline_implies_siting(DMemoryDiscipline::arena)
               == DStorageDuration::unknown);
    D_MD_CHECK(discipline_implies_siting(DMemoryDiscipline::unknown)
               == DStorageDuration::unknown);

    return true;
}

/*
tests_memory_discipline_siting_consistent
  a (discipline, siting) pair is consistent exactly when the bridge allows it
  Tests the following:
  - none is consistent with static or hybrid storage, and with unknown storage,
    but NOT with purely dynamic storage
  - individual and pooled are consistent with dynamic or hybrid, and unknown, but
    NOT with purely static storage (the allocator-over-inline contradiction)
  - arena is consistent with any siting (it forces none)
  - an unknown discipline is consistent with any siting
*/
bool tests_memory_discipline_siting_consistent()
{
    // none -> static implied
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::none, DStorageDuration::static_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::none, DStorageDuration::hybrid_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::none, DStorageDuration::unknown));
    D_MD_CHECK(!discipline_siting_consistent(DMemoryDiscipline::none, DStorageDuration::dynamic_storage));

    // individual -> dynamic implied
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::individual, DStorageDuration::dynamic_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::individual, DStorageDuration::hybrid_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::individual, DStorageDuration::unknown));
    D_MD_CHECK(!discipline_siting_consistent(DMemoryDiscipline::individual, DStorageDuration::static_storage));

    // pooled -> dynamic implied (same shape as individual)
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::pooled, DStorageDuration::dynamic_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::pooled, DStorageDuration::hybrid_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::pooled, DStorageDuration::unknown));
    D_MD_CHECK(!discipline_siting_consistent(DMemoryDiscipline::pooled, DStorageDuration::static_storage));

    // arena -> forces no siting: consistent with everything
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::arena, DStorageDuration::static_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::arena, DStorageDuration::dynamic_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::arena, DStorageDuration::hybrid_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::arena, DStorageDuration::unknown));

    // unknown discipline -> forces no siting: consistent with everything
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::unknown, DStorageDuration::static_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::unknown, DStorageDuration::dynamic_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::unknown, DStorageDuration::hybrid_storage));
    D_MD_CHECK(discipline_siting_consistent(DMemoryDiscipline::unknown, DStorageDuration::unknown));

    return true;
}

NS_END  // testing
NS_END  // djinterp
