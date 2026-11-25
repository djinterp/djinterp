/*******************************************************************************
* djinterp [cpp][desc]                                              mem_desc.hpp
*
*   The C++ face of desc's describers of memory's types (desc/of/mem_desc.h):
* units<T> and type_name<T> for the arena, the pool, a memory source, a
* memory block and an allocation tally, so describe(arena) is the arena's
* description. Built where the C describers are (D_CFG_DESC_OF_MEMORY);
* from C++98.
*
* HOW TO USE
* ----------
*   djinterp::desc::describe(arena) is the arena's description at desc.ring;
* describe(pool, D_DESC_RING_DETAILED) is a pool's at the detailed ring.
*
* OPTIONS
* -------
*   desc's own, as for the C describers.
*
* CONFIGURATION
* -------------
*   D_CFG_DESC_OF and D_CFG_DESC_OF_MEMORY (cfg_desc_of.h).
*
*
* path:      /inc/djinterp/cpp/desc/of/mem_desc.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  UNITS
    -----
    1.  dj_arena, dj_pool, dj_mem_source, dj_mem_block, dj_mem_stats
*/

#ifndef DJINTERP_DESC_OF_MEM_DESC_HPP
#define DJINTERP_DESC_OF_MEM_DESC_HPP 1

// djinterp
#include "./desc_of.hpp"                   // D_DESC_OF_UNITS
#include "../../../c/desc/of/mem_desc.h"   // the describers


#if D_INTERNAL_DESC_OF_MEMORY

NS_DJINTERP
D_NAMESPACE(desc)


//==============================================================================
// 1.  UNITS
//==============================================================================

// 1.1
// dj_arena, dj_pool, dj_mem_source, dj_mem_block, dj_mem_stats
//   traits: each type's units and name, from its C description.
D_DESC_OF_UNITS(dj_arena);
D_DESC_OF_UNITS(dj_pool);
D_DESC_OF_UNITS(dj_mem_source);
D_DESC_OF_UNITS(dj_mem_block);
D_DESC_OF_UNITS(dj_mem_stats);


NS_END  // desc
NS_END  // djinterp

#endif  // D_INTERNAL_DESC_OF_MEMORY


#endif  // DJINTERP_DESC_OF_MEM_DESC_HPP
