/***********************************************************************
* restd                                                            memory
*
* umbrella header for restd::memory. Includes every granular module in
* the memory group. Users may include this file to mirror the
* `#include <memory>` ergonomic of the standard library, or include
* the granular files directly for finer dependency control.
*
* current contents (Phase 1 + Phase 2 + Phase 3 + Phase 4a + Phase 4b + Phase 5):
*   Phase 1 (foundations):
*     addressof, pointer_traits, construct_at, destroy_at, destroy,
*     default_delete, allocator_arg, uses_allocator, bad_weak_ptr.
*   Phase 2 (allocator core):
*     allocator, allocator_traits.
*   Phase 3 (unique_ptr):
*     unique_ptr, unique_ptr_swap, make_unique
*     (incl. make_unique_for_overwrite C++20+).
*   Phase 4a (shared_ptr core):
*     shared_ptr, weak_ptr, enable_shared_from_this,
*     make_shared, allocate_shared.
*   Phase 4b (shared_ptr extras):
*     static_pointer_cast, dynamic_pointer_cast, const_pointer_cast,
*     reinterpret_pointer_cast, owner_less, get_deleter,
*     shared_ptr_swap, weak_ptr_swap, plus shared_ptr(p, d, alloc).
*   Phase 5 (uninitialized memory algorithms):
*     uninitialized_copy, uninitialized_copy_n, uninitialized_fill,
*     uninitialized_fill_n, uninitialized_default_construct(_n),
*     uninitialized_value_construct(_n), uninitialized_move(_n).
*   Phase 6b (shared_ptr array forms + for_overwrite):
*     shared_ptr<T[]> + weak_ptr<T[]> partial specializations,
*     make_shared<T[]>(n), allocate_shared<T[]>(alloc, n),
*     make_shared_for_overwrite (single + array),
*     allocate_shared_for_overwrite (single + array).
*   Phase 6c (bounded arrays + prototype-fill + small utilities):
*     shared_ptr<T[N]> + weak_ptr<T[N]> bounded-array specializations,
*     make_shared<T[]>(n, u), allocate_shared<T[]>(alloc, n, u),
*     make_shared<T[N]>() and make_shared<T[N]>(u) and
*     allocate_shared<T[N]>(alloc) and allocate_shared<T[N]>(alloc, u),
*     make_shared_for_overwrite<T[N]>() and
*     allocate_shared_for_overwrite<T[N]>(alloc),
*     align, to_address, assume_aligned.
*
* not yet implemented (later phases):
*   atomic shared_ptr ops (atomic_load/store/exchange/CAS for shared_ptr,
*   std::atomic<shared_ptr<T>>),
*   over-aligned types in make_shared<T[]> (needs operator new with
*   align_val_t),
*   hash<shared_ptr>, hash<unique_ptr>,
*   shared_ptr<T[N]> <-> shared_ptr<U[N]> qualification-only conversions.
*
*
* path:      /inc/djinterp/re_std/memory/memory.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.01
***********************************************************************/

#ifndef RESTD_MEMORY_
#define RESTD_MEMORY_ 1

#include "restd/memory/addressof.hpp"
#include "restd/memory/pointer_traits.hpp"
#include "restd/memory/construct_at.hpp"
#include "restd/memory/destroy_at.hpp"
#include "restd/memory/destroy.hpp"
#include "restd/memory/default_delete.hpp"
#include "restd/memory/allocator_arg.hpp"
#include "restd/memory/uses_allocator.hpp"
#include "restd/memory/bad_weak_ptr.hpp"

#include "restd/memory/allocator.hpp"
#include "restd/memory/allocator_traits.hpp"

#include "restd/memory/unique_ptr.hpp"
#include "restd/memory/unique_ptr_swap.hpp"
#include "restd/memory/make_unique.hpp"

#include "restd/memory/shared_ptr.hpp"
#include "restd/memory/weak_ptr.hpp"
#include "restd/memory/enable_shared_from_this.hpp"
#include "restd/memory/make_shared.hpp"
#include "restd/memory/allocate_shared.hpp"

#include "restd/memory/static_pointer_cast.hpp"
#include "restd/memory/dynamic_pointer_cast.hpp"
#include "restd/memory/const_pointer_cast.hpp"
#include "restd/memory/reinterpret_pointer_cast.hpp"
#include "restd/memory/owner_less.hpp"
#include "restd/memory/get_deleter.hpp"
#include "restd/memory/shared_ptr_swap.hpp"
#include "restd/memory/weak_ptr_swap.hpp"

#include "restd/memory/uninitialized_copy.hpp"
#include "restd/memory/uninitialized_copy_n.hpp"
#include "restd/memory/uninitialized_fill.hpp"
#include "restd/memory/uninitialized_fill_n.hpp"
#include "restd/memory/uninitialized_default_construct.hpp"
#include "restd/memory/uninitialized_default_construct_n.hpp"
#include "restd/memory/uninitialized_value_construct.hpp"
#include "restd/memory/uninitialized_value_construct_n.hpp"
#include "restd/memory/uninitialized_move.hpp"
#include "restd/memory/uninitialized_move_n.hpp"

#include "restd/memory/make_shared_for_overwrite.hpp"
#include "restd/memory/allocate_shared_for_overwrite.hpp"

#include "restd/memory/align.hpp"
#include "restd/memory/to_address.hpp"
#include "restd/memory/assume_aligned.hpp"

#endif  // RESTD_MEMORY_
