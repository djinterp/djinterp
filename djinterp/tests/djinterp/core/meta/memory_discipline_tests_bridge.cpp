/*******************************************************************************
* djinterp [core]                             memory_discipline_tests_bridge.cpp
*
*
* path:      /tests/djinterp/core/meta/memory_discipline_tests_bridge.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
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
bool
tests_memory_discipline_implies_siting()
{
    D_MD_CHECK(discipline_implies_siting(memory_discipline::none)
               == storage_duration::static_storage);
    D_MD_CHECK(discipline_implies_siting(memory_discipline::individual)
               == storage_duration::dynamic_storage);
    D_MD_CHECK(discipline_implies_siting(memory_discipline::pooled)
               == storage_duration::dynamic_storage);
    D_MD_CHECK(discipline_implies_siting(memory_discipline::arena)
               == storage_duration::unknown);
    D_MD_CHECK(discipline_implies_siting(memory_discipline::unknown)
               == storage_duration::unknown);

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
bool
tests_memory_discipline_siting_consistent()
{
    // none -> static implied
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::none, storage_duration::static_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::none, storage_duration::hybrid_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::none, storage_duration::unknown));
    D_MD_CHECK(!discipline_siting_consistent(memory_discipline::none, storage_duration::dynamic_storage));

    // individual -> dynamic implied
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::individual, storage_duration::dynamic_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::individual, storage_duration::hybrid_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::individual, storage_duration::unknown));
    D_MD_CHECK(!discipline_siting_consistent(memory_discipline::individual, storage_duration::static_storage));

    // pooled -> dynamic implied (same shape as individual)
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::pooled, storage_duration::dynamic_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::pooled, storage_duration::hybrid_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::pooled, storage_duration::unknown));
    D_MD_CHECK(!discipline_siting_consistent(memory_discipline::pooled, storage_duration::static_storage));

    // arena -> forces no siting: consistent with everything
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::arena, storage_duration::static_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::arena, storage_duration::dynamic_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::arena, storage_duration::hybrid_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::arena, storage_duration::unknown));

    // unknown discipline -> forces no siting: consistent with everything
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::unknown, storage_duration::static_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::unknown, storage_duration::dynamic_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::unknown, storage_duration::hybrid_storage));
    D_MD_CHECK(discipline_siting_consistent(memory_discipline::unknown, storage_duration::unknown));

    return true;
}

NS_END  // testing
NS_END  // djinterp
