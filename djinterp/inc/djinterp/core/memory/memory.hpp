/*******************************************************************************
* djinterp [core]                                                     memory.hpp
*
* Umbrella header for the memory subframework -- C++ face.
*   Includes the C core (through memory.h) and every C++ face over it. See
* memory.h for the shape of the subframework; this header adds only the
* wrappers, which add only lifetime and notation.
*
* WHAT THE C++ FACES ADD, AND NOTHING MORE:
*
*   arena / raw_pool / pool<T>   lifetime. Construction initializes,
*                                destruction releases. Every other member
*                                forwards to the kernel.
*
*   pool<T>::create / destroy    OBJECT lifetime. The one genuine capability
*                                the kernel cannot have: it deals in raw slots
*                                and cannot know one holds an object, so it
*                                can never run a constructor or a destructor.
*
*   arena_scope                  a mark and rewind that cannot be skipped by
*                                an early return or an exception.
*
*   pool_allocator<T>            Allocator conformance, which has no C
*   arena_allocator<T>           counterpart because C has no Allocator.
*
*   Every wrapper's size is asserted equal to its kernel's. See
* AGENT_README.md section 6 for why that is a test rather than a convention.
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/core/memory/memory.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_MEMORY_MEMORY_HPP
#define DJINTERP_MEMORY_MEMORY_HPP 1

// djinterp -- the C core first, then the faces over it
#include "../../c/memory/memory.h"
#include "./mem_common.hpp"
#include "./mem_source.hpp"
#include "./arena.hpp"
#include "./pool.hpp"
#include "./pool_allocator.hpp"
#include "./memory_strategy_common.hpp"
#include "./memory_strategy.hpp"

#endif  // DJINTERP_MEMORY_MEMORY_HPP
