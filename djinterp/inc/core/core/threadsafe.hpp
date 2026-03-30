/******************************************************************************
* djinterp [container]                                        threadsafe.hpp
*
* Umbrella header for the thread-safe foundation module.
*   Includes all non-container-specific threadsafe submodules:
*
*   lock_policy.hpp     — DThreadSafetyLevel, all policy structs,
*                         select_lock_policy, default_lock_policy
*   lock_guard.hpp      — scoped_read_lock, scoped_write_lock,
*                         scoped_try_lock
*   lock_policy_c.hpp   — C-backed RAII guards and lock policies
*                         (c_exclusive, c_shared, c_recursive,
*                         c_spinlock)
*   atomic.hpp          — atomic_size, atomic_version,
*                         atomic_flag_guard
*   condvar.hpp         — portable_condvar, portable_once,
*                         hardware_concurrency()
*
*   Users who need only a subset can include the individual
* submodule headers directly.  Existing code that includes
* this umbrella header continues to work unchanged.
*
*
* path:      \inc\container\threadsafe.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_CONTAINER_THREADSAFE_
#define DJINTERP_CONTAINER_THREADSAFE_ 1

// --- non-container-specific threadsafe submodules ---

#include "threadsafe\lock_policy.hpp"
#include "threadsafe\lock_guard.hpp"
#include "threadsafe\lock_policy_c.hpp"
#include "threadsafe\atomic.hpp"
#include "threadsafe\condvar.hpp"


#endif  // DJINTERP_CONTAINER_THREADSAFE_
