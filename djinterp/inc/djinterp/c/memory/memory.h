/*******************************************************************************
* djinterp [c]                                                          memory.h
*
* Umbrella header for the memory subframework -- C face.
*   Includes the shared vocabulary and every allocator, in dependency order.
* Include this when a translation unit wants the whole subframework; include
* the individual headers when it wants one allocator and no more, since each
* is self-contained and pulls in only what it needs.
*
* THE SUBFRAMEWORK, IN ONE PICTURE:
*
*   mem_common      the alphabet -- counter type, status set, block view,
*                   alignment arithmetic, accounting, poison and guard bands.
*                   Allocates nothing.
*        |
*   mem_source      where bytes come from. Two pointers and four contract
*                   clauses. Built-in: system (malloc), buffer (the caller's
*                   own memory, no heap at all), null (refuses, so "this
*                   subsystem does not allocate" becomes a test), counting
*                   (a decorator that measures what passes through).
*        |
*        +---- arena     a pointer that moves forward. Mark and rewind make it
*        |               a stack allocator; chaining keeps every pointer it
*        |               ever handed out valid across growth.
*        |
*        +---- pool      one slot size, three release policies. Generational
*                        handles turn use-after-free from silent corruption
*                        into a checked failure.
*
*   d_arena_as_source closes the loop: an arena IS a source, so a pool can be
* carved out of one, and the whole graph freed by one d_arena_release.
*
* WHAT TO COMPILE AND LINK:
*   mem_common.c, mem_source.c, arena.c, pool.c
*
* PORTABLE ACROSS:
*   C99, C11, C17, C23  /  C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/c/memory/memory.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_C_MEMORY_MEMORY_H
#define DJINTERP_C_MEMORY_MEMORY_H 1

// djinterp -- dependency order: vocabulary, then supply, then allocators
#include "./mem_common.h"
#include "./mem_source.h"
#include "./arena.h"
#include "./pool.h"

#endif  // DJINTERP_C_MEMORY_MEMORY_H
