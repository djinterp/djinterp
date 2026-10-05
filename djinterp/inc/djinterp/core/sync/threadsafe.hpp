/*******************************************************************************
* djinterp [core]                                                 threadsafe.hpp
*
* Umbrella header for the thread-safe foundation module.
* Includes all container-agnostic threadsafe submodules:
*   lock_policy.hpp     - thread_safety_level, all policy structs,
*                         select_lock_policy, default_lock_policy
*   lock_guard.hpp      - scoped_read_lock, scoped_write_lock,
*                         scoped_try_lock, scoped_timed_lock,
*                         upgrade_lock
*   lock_policy_c.hpp   - C-backed RAII guards and lock policies
*                         (c_exclusive, c_shared, c_recursive,
*                         c_spinlock)
*   atomic.hpp          - atomic_size, atomic_version,
*                         atomic_flag_guard, atomic_stamped_ptr
*   condvar.hpp         - portable_condvar, portable_once,
*                         hardware_concurrency(), d_thread_yield()
*   cow.hpp             - cow_ptr, immutable_snapshot, cow_state
*                         (copy-on-write and snapshot primitives)
*   hazard_pointer.hpp  - hazard_record, hazard_domain,
*                         scoped_hazard, typed_hazard_ptr,
*                         multi_hazard_domain, retired_list
*   rcu.hpp             - epoch_counter, epoch_guard,
*                         epoch_registry, deferred_reclaimer,
*                         rcu_protected
*
*   All types live in namespace threadsafe.
*   Users who need only a subset can include the individual
* submodule headers directly. Existing code that includes
* this umbrella header continues to work unchanged.
*
*
* path:      /inc/djinterp/core/sync/threadsafe.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.23
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_SYNC_THREADSAFE_HPP
#define DJINTERP_SYNC_THREADSAFE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// --- container-agnostic threadsafe submodules ---

// djinterp
#include "./lock_policy.hpp"
#include "./lock_guard.hpp"
#include "./lock_policy_c.hpp"
#include "./atomic.hpp"
#include "./condvar.hpp"
#include "./cow.hpp"
#include "./hazard_pointer.hpp"
#include "./rcu.hpp"

#endif  // floor, for now


#endif  // DJINTERP_SYNC_THREADSAFE_HPP
