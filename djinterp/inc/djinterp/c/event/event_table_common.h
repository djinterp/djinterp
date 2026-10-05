/*******************************************************************************
* djinterp [c]                                              event_table_common.h
*
* The erased store -- the shared C core (tier 0):
*   Type-erased handler storage: the realization of erasure. The table maps
* each key kappa(e) to an insertion-ordered WORD of handler entries, and
* carries the mask m that switches individual letters on and off without
* removing them. It provides bind, unbind, enable, disable, lookup, ordered
* iteration, statistics, pointwise merge, and clearing.
*
*   This replaces the pre-core C hash table AND backs the C++ event_table. The
* two were not the same object: the C table was a MAP (insert replaced the
* listener bound to a key, so multiplicity was 1) while the C++ table was an
* ordered MULTIMAP (bind appended a letter to a word). The word is the object
* the note defines, so the word is what the core provides; the C map behaviour
* is not recoverable from it and is not preserved. Call sites that relied on
* rebinding a key to replace its handler must now unbind first.
*
* STORAGE MODEL:
*   Entries live in one flat arena and chain by 32-bit INDEX, not by pointer,
* so a table is relocatable, memcpy-able, and fwrite-able (AGENT_README.md
* section 8). Two forms are provided:
*     fixed     -- caller supplies the arena and bucket arrays; no allocation,
*                  so the whole module is usable before the C container
*                  substrate exists.
*     allocating -- the table owns its storage and rehashes on load.
*   Both satisfy the same contract; only D_EVENT_ERR_CAPACITY distinguishes
* them, and only when the fixed form is full.
*
* FORMAL CORRESPONDENCE ("Definition of an Event"):
*   erasure  kappa : T_e -> K   -- the key stored on each entry
*   word     rho_e              -- the per-key chain, in bind order
*   mask     m : L -> {on,off}  -- d_event_entry::enabled
*   bind / unbind               -- d_event_table_bind / _unbind
*   enable / disable            -- flip the mask; the letter stays in the word
*   merge    rho (+) rho'       -- d_event_table_merge (pointwise
*                                  concatenation; identity: the empty table)
*   fibre condition             -- an erased payload is touched only by code
*                                  selected by its own key, because bind and
*                                  dispatch key by the same kappa
*
* PORTABLE ACROSS:
*   C99, C11, C17, C23  /  C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/c/event/event_table_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_EVENT_EVENT_TABLE_COMMON_H
#define DJINTERP_C_EVENT_EVENT_TABLE_COMMON_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "../../config/core/event/cfg_event_table.h"
#include "./event_common.h"
#include "./event_handler_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // uint8_t, uint32_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///        0.    RESOLVED CONFIGURATION                                     ///
///////////////////////////////////////////////////////////////////////////////
//   Reads only. Every value below is resolved in cfg_event_table.h; this
// header re-publishes them under the module's own names so that call sites
// read as intent rather than as plumbing.

// D_EVENT_TABLE_DEFAULT_CAPACITY / D_EVENT_TABLE_DEFAULT_BUCKETS
//   constant: the capacities an allocating table falls back to when given 0.
#define D_EVENT_TABLE_DEFAULT_CAPACITY                                        \
    ((size_t)D_INTERNAL_EVENT_TABLE_DEFAULT_CAPACITY)
#define D_EVENT_TABLE_DEFAULT_BUCKETS                                         \
    ((size_t)D_INTERNAL_EVENT_TABLE_DEFAULT_BUCKETS)

// D_EVENT_TABLE_LOAD_FACTOR_NUM / _DEN
//   constant: the rehash threshold as an exact rational, so that both
// languages take the decision at the same entry count. A double literal would
// make the threshold depend on rounding, and a rehash that fires at a
// different moment in C than in C++ changes iteration order.
#define D_EVENT_TABLE_LOAD_FACTOR_NUM   D_INTERNAL_EVENT_TABLE_LOAD_NUM
#define D_EVENT_TABLE_LOAD_FACTOR_DEN   D_INTERNAL_EVENT_TABLE_LOAD_DEN

// D_EVENT_TABLE_GROWTH_FACTOR
//   constant: multiplier applied to the bucket count on rehash.
#define D_EVENT_TABLE_GROWTH_FACTOR     D_INTERNAL_EVENT_TABLE_GROWTH


///////////////////////////////////////////////////////////////////////////////
///        I.    THE ENTRY (one letter of a word)                           ///
///////////////////////////////////////////////////////////////////////////////

// d_event_entry
//   struct: one bound handler. `next` is the index of the following entry in
// the same bucket chain, or D_EVENT_INDEX_NONE. `state_free` is called on the
// step's state when the entry is unbound or the table is cleared; it may be
// NULL, in which case the state is borrowed and the caller owns its lifetime.
struct d_event_entry
{
    d_handler_id        id;             // 0 when the slot is free
    d_event_key         key;            // kappa(e) -- which word this letter
                                        // belongs to
    struct d_event_step step;           // the handler itself
    fn_free             state_free;     // may be NULL (borrowed state)
    uint32_t            next;           // index-linked chain, not a pointer
    uint8_t             enabled;        // the mask m
    uint8_t             reserved[3];    // must be 0
};


///////////////////////////////////////////////////////////////////////////////
///        II.   THE TABLE                                                  ///
///////////////////////////////////////////////////////////////////////////////

// D_EVENT_TABLE_FLAG_OWNS_STORAGE
//   constant: set when the table allocated its own arena and must free it.
#define D_EVENT_TABLE_FLAG_OWNS_STORAGE     ((uint32_t)0x00000001u)

// D_EVENT_TABLE_FLAG_FIXED
//   constant: set when the arena is caller-provided and may not grow. Insert
// past capacity returns D_EVENT_ERR_CAPACITY rather than rehashing.
#define D_EVENT_TABLE_FLAG_FIXED            ((uint32_t)0x00000002u)

// d_event_table
//   struct: the erased store. Bucket chains preserve append order, so the
// relative order of two entries sharing a key is their bind order -- which is
// the word order, and therefore the dispatch priority.
struct d_event_table
{
    struct d_event_entry* entries;       // the flat entry arena
    uint32_t*             bucket_head;   // first entry index per bucket
    uint32_t*             bucket_tail;   // last entry index per bucket
    size_t                capacity;      // arena capacity, in entries
    size_t                bucket_count;  // length of the bucket arrays
    size_t                count;         // live entries
    size_t                enabled_count; // live entries with the mask on
    d_handler_id          next_id;       // id counter; first bind yields 1
    uint32_t              free_head;     // free-list head index
    uint32_t              used;          // arena high-water mark
    uint32_t              flags;         // D_EVENT_TABLE_FLAG_*
    uint32_t              reserved;      // must be 0
};

#if (D_INTERNAL_EVENT_TABLE_STATS == 1)

// d_event_table_stats
//   struct: a snapshot of table metrics.
//   NOTE: instrumentation, not formal content -- the note does not define a
// statistics record. It is nonetheless declared HERE rather than once per
// language because the pre-core forms had DIFFERENT FIELDS (the C
// d_event_hash_stats counted only elements; the C++ event_table_stats also
// counted enabled entries and distinct types), and two shapes for one report
// is a silent parity break the moment either is printed.
struct d_event_table_stats
{
    size_t total_buckets;
    size_t used_buckets;
    size_t total_entries;
    size_t enabled_entries;
    size_t key_count;
    size_t max_entries_per_key;
    double average_entries_per_key;
    double load_factor;
};


#endif  // D_INTERNAL_EVENT_TABLE_STATS


///////////////////////////////////////////////////////////////////////////////
///        III.  CREATION AND DESTRUCTION                                   ///
///////////////////////////////////////////////////////////////////////////////

// d_event_table_init_fixed
//   function: initializes a table over caller-provided storage. No allocation
// occurs and none ever will: the table is marked fixed and returns
// D_EVENT_ERR_CAPACITY when the arena fills.
//   `_entries` must hold `_capacity` entries; `_bucket_head` and
// `_bucket_tail` must each hold `_bucket_count` indices.
// returns: D_EVENT_OK, or D_EVENT_ERR_NULL on a missing argument.
int d_event_table_init_fixed(struct d_event_table* _table,
                             struct d_event_entry* _entries,
                             size_t                _capacity,
                             uint32_t*             _bucket_head,
                             uint32_t*             _bucket_tail,
                             size_t                _bucket_count);

#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)

// d_event_table_init
//   function: initializes an allocating table with the given capacities. A
// zero argument selects the corresponding D_EVENT_TABLE_DEFAULT_*; the bucket
// count is rounded up to the next prime.
// returns: D_EVENT_OK, or D_EVENT_ERR_ALLOC / D_EVENT_ERR_NULL.
int d_event_table_init(struct d_event_table* _table,
                       size_t                _capacity,
                       size_t                _bucket_count);

#endif  // D_INTERNAL_EVENT_TABLE_ALLOC

// d_event_table_dispose
//   function: releases every entry's state through its state_free hook and,
// if the table owns its storage, frees the arena. Safe on a zeroed table and
// safe to call twice.
void d_event_table_dispose(struct d_event_table* _table);

// d_event_table_clear
//   function: removes every entry, running each state_free hook, and returns
// the table to the empty state. Storage and capacity are retained.
//   The empty table is the identity of merge.
void d_event_table_clear(struct d_event_table* _table);

// d_event_table_clear_key
//   function: removes every entry bound to one key.
// returns: the number of entries removed.
size_t d_event_table_clear_key(struct d_event_table* _table,
                               d_event_key           _key);


///////////////////////////////////////////////////////////////////////////////
///        IV.   BIND AND UNBIND                                            ///
///////////////////////////////////////////////////////////////////////////////

// d_event_table_bind
//   function: appends a letter to the word for `_key`. The entry is enabled
// on bind. `_state_free`, when non-NULL, is called on the step's state when
// the entry is unbound, cleared, or the table is disposed -- which is how the
// C++ face gives the core ownership of a heap-promoted closure without adding
// a member to its wrapper.
//   `_id_out` may be NULL.
// returns: D_EVENT_OK, D_EVENT_ERR_NULL, D_EVENT_ERR_CAPACITY (fixed arena
// full), or D_EVENT_ERR_ALLOC.
int d_event_table_bind(struct d_event_table* _table,
                       d_event_key           _key,
                       struct d_event_step   _step,
                       fn_free               _state_free,
                       d_handler_id*         _id_out);

// d_event_table_unbind
//   function: removes the letter named by `_id` from whichever word holds it,
// running its state_free hook.
// returns: D_EVENT_OK or D_EVENT_ERR_NOT_FOUND.
int d_event_table_unbind(struct d_event_table* _table,
                         d_handler_id          _id);


///////////////////////////////////////////////////////////////////////////////
///        V.    THE MASK                                                   ///
///////////////////////////////////////////////////////////////////////////////
// A masked-off letter remains in the word and acts as skip during dispatch.
// Because skip is the monoid unit, masking cannot change the verdict of a
// word that would otherwise pass -- which is the algebraic content of the
// mask and the property its conformance test must assert.

// d_event_table_enable
//   function: flips the mask on for `_id`.
// returns: D_EVENT_OK if the entry was found AND was previously disabled;
// D_EVENT_ERR_STATE if it was found but already enabled;
// D_EVENT_ERR_NOT_FOUND otherwise.
//   The three-way result is deliberate: the pre-core C and C++ forms
// disagreed here (C returned true for an already-enabled listener, C++
// returned false), and reporting "already in that state" as either success or
// absence loses information one of the two call sites needs.
int d_event_table_enable(struct d_event_table* _table,
                         d_handler_id          _id);

// d_event_table_disable
//   function: flips the mask off for `_id`. Result convention as for
// d_event_table_enable.
int d_event_table_disable(struct d_event_table* _table,
                          d_handler_id          _id);

// d_event_table_is_enabled
//   function: true if the entry exists and its mask is on.
bool d_event_table_is_enabled(const struct d_event_table* _table,
                              d_handler_id                _id);

// d_event_table_contains
//   function: true if an entry with `_id` exists in any word.
bool d_event_table_contains(const struct d_event_table* _table,
                            d_handler_id                _id);


///////////////////////////////////////////////////////////////////////////////
///        VI.   WORD ACCESS                                                ///
///////////////////////////////////////////////////////////////////////////////
// Access is by index rather than by pointer so that a bind occurring during
// iteration cannot invalidate a caller's cursor by reallocating the arena.

// d_event_table_first
//   function: index of the first entry of the word for `_key`, or
// D_EVENT_INDEX_NONE if the word is empty.
uint32_t d_event_table_first(const struct d_event_table* _table,
                             d_event_key                 _key);

// d_event_table_next
//   function: index of the next entry of the same word after `_index`, or
// D_EVENT_INDEX_NONE at the end. Entries of other keys sharing the bucket are
// skipped.
uint32_t d_event_table_next(const struct d_event_table* _table,
                            d_event_key                 _key,
                            uint32_t                    _index);

// d_event_table_at
//   function: the entry at `_index`, or NULL if the index is out of range or
// names a free slot.
const struct d_event_entry* d_event_table_at(
    const struct d_event_table* _table,
    uint32_t                    _index);

// d_event_table_find
//   function: the entry named by `_id`, or NULL.
const struct d_event_entry* d_event_table_find(
    const struct d_event_table* _table,
    d_handler_id                _id);


///////////////////////////////////////////////////////////////////////////////
///        VII.  COUNTS AND ORDERED ITERATION                               ///
///////////////////////////////////////////////////////////////////////////////

// d_event_table_count
//   function: total live entries across every word.
size_t d_event_table_count(const struct d_event_table* _table);

// d_event_table_enabled_count
//   function: live entries whose mask is on.
size_t d_event_table_enabled_count(const struct d_event_table* _table);

// d_event_table_count_for
//   function: length of the word for `_key`.
size_t d_event_table_count_for(const struct d_event_table* _table,
                               d_event_key                 _key);

// d_event_table_has_entries_for
//   function: true if the word for `_key` is non-empty.
bool d_event_table_has_entries_for(const struct d_event_table* _table,
                                   d_event_key                 _key);

// d_event_table_key_count
//   function: number of distinct keys with a non-empty word.
size_t d_event_table_key_count(const struct d_event_table* _table);

// fn_event_entry_visitor
//   function pointer: receives each visited entry. Returning false stops the
// walk.
typedef bool (*fn_event_entry_visitor)(const struct d_event_entry* _entry,
                                       void*                       _context);

// d_event_table_for_each
//   function: visits every live entry in ASCENDING HANDLER ID ORDER, i.e. in
// global bind order.
//   The ordering is not an incidental choice. Bucket order depends on the
// hash and the capacity, so it differs between two tables holding the same
// bindings and would make any dump non-comparable -- and a dump that is not
// comparable cannot serve the parity law. Bind order is the one total order
// both languages agree on without coordination.
// returns: the number of entries visited.
size_t d_event_table_for_each(const struct d_event_table* _table,
                              fn_event_entry_visitor      _visitor,
                              void*                       _context);

// d_event_table_for_each_key
//   function: visits the word for one key, in word order.
// returns: the number of entries visited.
size_t d_event_table_for_each_key(const struct d_event_table* _table,
                                  d_event_key                 _key,
                                  fn_event_entry_visitor      _visitor,
                                  void*                       _context);


///////////////////////////////////////////////////////////////////////////////
///        VIII. MERGE                                                      ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_EVENT_TABLE_MERGE == 1)

// d_event_table_merge
//   function: appends every entry of `_other` into `_table`, key by key, with
// this table's entries first -- the pointwise concatenation rho (+) rho'. The
// merged entries receive FRESH ids from this table's id space, so ids from
// `_other` do not survive; the mask travels with each letter.
//   The merged entries borrow their state: `state_free` is NOT copied, since
// two tables must not both own one state block. `_other` remains valid and
// unchanged.
//   `_merged_out` may be NULL.
// returns: D_EVENT_OK, D_EVENT_ERR_NULL, or D_EVENT_ERR_CAPACITY. On
// D_EVENT_ERR_CAPACITY the merge is PARTIAL and `_merged_out` reports how far
// it got; merge is not transactional, because rolling back would require the
// allocation the fixed form exists to avoid.
int d_event_table_merge(struct d_event_table*       _table,
                        const struct d_event_table* _other,
                        size_t*                     _merged_out);


#endif  // D_INTERNAL_EVENT_TABLE_MERGE


///////////////////////////////////////////////////////////////////////////////
///        IX.   STATISTICS AND MAINTENANCE                                 ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_EVENT_TABLE_STATS == 1)

// d_event_table_get_stats
//   function: computes a metrics snapshot. Safe on a zeroed table, which
// yields an all-zero record.
struct d_event_table_stats d_event_table_get_stats(
    const struct d_event_table* _table);

#endif  // D_INTERNAL_EVENT_TABLE_STATS

// d_event_table_load_factor
//   function: live entries divided by bucket count; 0.0 when there are no
// buckets.
//   NOT gated on D_CFG_EVENT_TABLE_STATS: the load factor is what the rehash
// policy tests, not something it reports, so a build with instrumentation
// compiled out still needs it.
double d_event_table_load_factor(const struct d_event_table* _table);

#if (D_INTERNAL_EVENT_TABLE_ALLOC == 1)

// d_event_table_rehash
//   function: rebuilds the bucket arrays at `_bucket_count` buckets, rounded
// up to the next prime. Word order is preserved. A no-op on a fixed table,
// which returns D_EVENT_ERR_STATE.
// returns: D_EVENT_OK, D_EVENT_ERR_ALLOC, or D_EVENT_ERR_STATE.
int d_event_table_rehash(struct d_event_table* _table,
                         size_t                _bucket_count);

// d_event_table_reserve
//   function: grows the entry arena to hold at least `_capacity` entries.
// A no-op on a fixed table, which returns D_EVENT_ERR_STATE.
int d_event_table_reserve(struct d_event_table* _table,
                          size_t                _capacity);

#endif  // D_INTERNAL_EVENT_TABLE_ALLOC

// d_event_table_footprint
//   function: the table's total byte footprint including owned storage --
// the runtime `sizeof` that goals section 4 requires of every type.
size_t d_event_table_footprint(const struct d_event_table* _table);


///////////////////////////////////////////////////////////////////////////////
///        X.    LAYOUT ASSERTIONS                                          ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_EVENT_ASSERT_SIZES == 1)

    D_STATIC_ASSERT(sizeof(struct d_event_entry) == 48,
                    "d_event_entry layout drift");
    D_STATIC_ASSERT(sizeof(struct d_event_table) == 80,
                    "d_event_table layout drift");
#if (D_INTERNAL_EVENT_TABLE_STATS == 1)
    D_STATIC_ASSERT(sizeof(struct d_event_table_stats) == 64,
                    "d_event_table_stats layout drift");
#endif
    D_STATIC_ASSERT(offsetof(struct d_event_entry, step) == 16,
                    "d_event_entry field drift");
    D_STATIC_ASSERT(offsetof(struct d_event_table, count) == 40,
                    "d_event_table field drift");

#endif  // D_INTERNAL_EVENT_ASSERT_SIZES

#if (D_INTERNAL_EVENT_ASSERT_LAYOUT == 1)

D_STATIC_ASSERT(offsetof(struct d_event_entry, id) == 0,
                "d_event_entry field drift");
D_STATIC_ASSERT(offsetof(struct d_event_table, entries) == 0,
                "d_event_table field drift");

#endif  // D_INTERNAL_EVENT_ASSERT_LAYOUT


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_EVENT_EVENT_TABLE_COMMON_H
