/******************************************************************************
* djinterp [container]                                    threadsafe_tables.hpp
*
* Umbrella header for the threadsafe table family.
*   Includes every threadsafe variant of every appropriate table type
* in one convenient include.  Users who need only a single strategy
* should include the corresponding individual header instead.
*
* STRATEGY MATRIX:
*
*                   table        registry_table
*   -------------+------------+--------------------------
*   mutex        | mutex_table  mutex_registry_table
*   cow          | cow_table    cow_registry_table
*   rcu          | rcu_table    rcu_registry_table
*   atomic       |     -        atomic_registry_table
*
*   Following the registry_table redesign, registry_table is BOTH the
* lookup surface AND the optional cvar surface.  A registry_table
* without a value column IS a lookup_table - so each *_registry_table
* wrapper covers both modes via SFINAE-gated cvar methods.  Pure-lookup
* users wrap a value-column-less registry_table; cvar users supply a
* value member.  Same wrapper type, same threadsafety contract.
*
*   The mutex_* family shares mutex_table_base.hpp.  The cow_* family
* composes with threadsafe::cow_state.  The rcu_* family composes with
* threadsafe::rcu_protected.  atomic_registry_table is self-contained,
* using std::atomic<_CvarType> per entry.
*
* CHOOSING A STRATEGY:
*   - mutex_*  : default choice; correct for any read/write mix
*   - cow_*    : long-lived snapshots are common; refcounted snapshots
*                preferred over deep copies
*   - rcu_*    : reads VASTLY outnumber writes; whole-table cloning
*                per write is acceptable; SINGLE-WRITER assumed
*   - atomic_registry_table : cvar-only scenarios with trivially-
*                copyable values; lock-free hot path for both reads
*                AND writes
*
* COVERAGE GAPS (intentional):
*   - atomic_table             : no clean atomic primitive for variable-
*                                size storage
*   - threadsafe_table_overlay : overlay adds no storage; threadsafety
*                                belongs to the underlying
*   - threadsafe_database_table: the database connection is the sync
*                                boundary
*   - threadsafe_option_set    : use threadsafe::cow_state<option_set<...>>
*                                from cow.hpp directly
*
*
* path:      /inc/djinterp/container/table/threadsafe_tables.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_TABLES_
#define DJINTERP_THREADSAFE_TABLES_ 1

// --- mutex strategy ---
#include "./mutex_table_base.hpp"
#include "./mutex_table.hpp"
#include "./mutex_registry_table.hpp"

// --- copy-on-write strategy ---
#include "./cow_table.hpp"
#include "./cow_registry_table.hpp"

// --- read-copy-update strategy ---
#include "./rcu_table.hpp"
#include "./rcu_registry_table.hpp"

// --- per-cvar atomic strategy ---
#include "./atomic_registry_table.hpp"


#endif  // DJINTERP_THREADSAFE_TABLES_
