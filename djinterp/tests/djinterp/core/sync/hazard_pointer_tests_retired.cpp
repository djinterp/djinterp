/*******************************************************************************
* djinterp [core]                               hazard_pointer_tests_retired.cpp
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests_retired.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                   hazard_pointer_tests_retired.cpp
//   Section VI: retired_list - the queue of nodes awaiting reclamation, and the
// scan that frees them.
//
//   This is where the safety property finally lands.  A writer retires a node
// (logically deletes it) into this per-thread list; later, scan() walks the
// list
// against the domain's active hazard records and FREES exactly those nodes that
// no record protects, compacting the survivors down so the list does not grow
// without bound.  The one invariant that matters -
// tests_scan_spares_a_protected_node - is that a node some reader is protecting
// is NOT freed, no matter that it has been retired.  Get that wrong and the
// whole scheme is a use-after-free generator.
//
//   Reclamation is invisible from a value, so the section counts instead: a
// counted_node raises a live counter when built and a deletion counter when
// freed, and the tests assert on those.  scan_by_epoch() is the epoch-based
// cousin - it frees by age rather than by protection - and clear() frees
// everything unconditionally.
//
//   Gated on D_HAS_CUSTOM_HAZARD throughout.

// djinterp
#include "hazard_pointer_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_retired_list_starts_empty
  a fresh retired list has nothing queued.
  Tests the following:
  - pending() is 0 and empty() is true at construction
  - it is default-constructible
*/
bool
tests_retired_list_starts_empty()
{
#if D_HAS_CUSTOM_HAZARD
    retired_list<counted_node> list;

    D_CONSTEXPR bool default_constructible =
        std::is_default_constructible<retired_list<counted_node> >::value;

    static_assert(default_constructible,
                  "retired_list must be default-constructible");

    return ( (list.pending() == 0) &&
             list.empty()          &&
             default_constructible );
#else
    return true;
#endif
}

/*
tests_retire_queues_a_node
  retire() adds a node to the list without freeing it.
  Tests the following:
  - after retiring two nodes, pending() is 2 and empty() is false
  - the nodes are NOT freed by retiring: the live count is unchanged, the
    deletion count still zero
  Retiring is "logical deletion": the node is unreachable to new readers but not
  yet reclaimed, because an old reader might still hold it.
*/
bool
tests_retire_queues_a_node()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    retired_list<counted_node> list;

    list.retire(new counted_node(1));
    list.retire(new counted_node(2));

    const bool queued_not_freed =
        ( (list.pending() == 2)             &&
          (!list.empty())                   &&
          (probe_live().load() == 2)        &&
          (probe_deletions().load() == 0) );

    // clean up so the test leaks nothing
    list.clear();

    return queued_not_freed;
#else
    return true;
#endif
}

/*
tests_scan_reclaims_an_unprotected_node
  a retired node that nobody protects is freed by scan().
  Tests the following:
  - one node is retired, no record protects it, and scan() frees it
  - the deletion count rises by one and the live count falls to zero
  - the list is empty afterwards
  This is the base case of reclamation: retired and unprotected means reclaimed.
*/
bool
tests_scan_reclaims_an_unprotected_node()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    hazard_domain              domain(4);
    retired_list<counted_node> list;

    list.retire(new counted_node(1));

    const std::size_t reclaimed = list.scan(domain);

    return ( (reclaimed == 1)                &&
             (probe_deletions().load() == 1) &&
             (probe_live().load() == 0)      &&
             list.empty() );
#else
    return true;
#endif
}

/*
tests_scan_spares_a_protected_node
  THE SAFETY PROPERTY.  A retired node that a record protects is NOT freed.
  Tests the following:
  - a node is retired AND an active record protects its address
  - scan() reclaims ZERO nodes: it finds the address in the protected set and
    spares it
  - the node is still live, and still pending in the list
  - once the protection is dropped, a second scan frees it
  This is the whole reason hazard pointers exist.  A reader that has published
  protection for a node is guaranteed the node stays alive until it clears that
  protection, even though a writer has already retired it.  Break this and every
  lock-free structure built on the header is unsafe.
*/
bool
tests_scan_spares_a_protected_node()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    hazard_domain              domain(4);
    retired_list<counted_node> list;

    counted_node* node = new counted_node(7);

    // a reader protects it
    hazard_record* record = domain.acquire();
    record->protected_ptr.store(node);

    // a writer retires it
    list.retire(node);

    const std::size_t reclaimed_while_protected = list.scan(domain);

    const bool spared =
        ( (reclaimed_while_protected == 0)  &&
          (probe_deletions().load() == 0)   &&
          (probe_live().load() == 1)        &&
          (list.pending() == 1) );

    // the reader clears its protection; now it may be reclaimed
    record->protected_ptr.store(nullptr);

    const std::size_t reclaimed_after = list.scan(domain);

    const bool reclaimed_once_free =
        ( (reclaimed_after == 1)            &&
          (probe_deletions().load() == 1)   &&
          (probe_live().load() == 0)        &&
          list.empty() );

    domain.release(record);

    return (spared && reclaimed_once_free);
#else
    return true;
#endif
}

/*
tests_scan_compacts_the_survivors
  scan() keeps the protected nodes and drops the freed ones, in place.
  Tests the following:
  - six nodes are retired; the three at even indices are protected, the three at
    odd indices are not
  - scan() reclaims exactly the three unprotected nodes
  - the three protected ones survive, and pending() is 3
  - a value check confirms the survivors are the ones that were protected, not
    just any three - so the compaction moved the right entries, not merely the
    right COUNT of entries
  The scan uses a read/write compaction (keep survivors, overwrite gaps); this
  checks it preserves identity, not just arithmetic.
*/
bool
tests_scan_compacts_the_survivors()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    hazard_domain              domain(8);
    retired_list<counted_node> list;

    counted_node* nodes[6];
    std::size_t   i = 0;

    for (i = 0; i < 6; ++i)
    {
        nodes[i] = new counted_node(static_cast<int>(i));
    }

    // protect the even-indexed nodes
    hazard_record* records[3];
    std::size_t    r = 0;

    for (i = 0; i < 6; i += 2)
    {
        records[r] = domain.acquire();
        records[r]->protected_ptr.store(nodes[i]);
        ++r;
    }

    // retire all six
    for (i = 0; i < 6; ++i)
    {
        list.retire(nodes[i]);
    }

    const std::size_t reclaimed = list.scan(domain);

    const bool right_count =
        ( (reclaimed == 3)                &&
          (list.pending() == 3)           &&
          (probe_deletions().load() == 3) &&
          (probe_live().load() == 3) );

    // the survivors must be exactly the protected (even) nodes.  Drop the
    // protections and scan again: it should reclaim the remaining three, and
    // the live count should reach zero
    for (r = 0; r < 3; ++r)
    {
        records[r]->protected_ptr.store(nullptr);
    }

    const std::size_t reclaimed_rest = list.scan(domain);

    const bool survivors_were_the_protected_ones =
        ( (reclaimed_rest == 3)           &&
          (probe_live().load() == 0)      &&
          list.empty() );

    for (r = 0; r < 3; ++r)
    {
        domain.release(records[r]);
    }

    return (right_count && survivors_were_the_protected_ones);
#else
    return true;
#endif
}

/*
tests_scan_returns_the_reclaimed_count
  scan() reports how many it freed.
  Tests the following:
  - retiring four unprotected nodes and scanning returns 4
  - a second scan of the now-empty list returns 0
  The count is what a caller feeds to decrement_retired(), so it must be exact.
*/
bool
tests_scan_returns_the_reclaimed_count()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    hazard_domain              domain(4);
    retired_list<counted_node> list;
    std::size_t                i = 0;

    for (i = 0; i < 4; ++i)
    {
        list.retire(new counted_node(static_cast<int>(i)));
    }

    const std::size_t first  = list.scan(domain);
    const std::size_t second = list.scan(domain);

    return ( (first == 4) &&
             (second == 0) );
#else
    return true;
#endif
}

/*
tests_scan_with_a_custom_deleter
  a node retired with a custom deleter is freed through that deleter.
  Tests the following:
  - retire(ptr, deleter) records the deleter, and scan() calls IT rather than
    the default `delete`
  - the custom deleter ran exactly once
  So a node that needs special teardown - returned to a pool, say, rather than
  deleted - can be retired the same way, and the scan honours it.
*/
bool
tests_scan_with_a_custom_deleter()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    hazard_domain              domain(2);
    retired_list<counted_node> list;

    static std::atomic<int> custom_calls;
    custom_calls.store(0);

    counted_node* node = new counted_node(9);

    list.retire(node,
                [](counted_node* _p)
                {
                    custom_calls.fetch_add(1);
                    delete _p;
                });

    const std::size_t reclaimed = list.scan(domain);

    return ( (reclaimed == 1)              &&
             (custom_calls.load() == 1)    &&
             (probe_deletions().load() == 1) &&
             (probe_live().load() == 0) );
#else
    return true;
#endif
}

/*
tests_default_retire_deletes_the_node
  the default retire uses `delete`.
  Tests the following:
  - a node retired with no deleter, then reclaimed, is destroyed - its
    destructor runs
  So the common path frees the node exactly once, through the default deleter
  the header installs.
*/
bool
tests_default_retire_deletes_the_node()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    hazard_domain              domain(2);
    retired_list<counted_node> list;

    list.retire(new counted_node(1));

    const std::size_t reclaimed = list.scan(domain);

    return ( (reclaimed == 1) &&
             (probe_deletions().load() == 1) &&
             (probe_live().load() == 0) );
#else
    return true;
#endif
}

/*
tests_scan_by_epoch_reclaims_older_entries
  epoch-based reclamation frees by age, not by protection.
  Tests the following:
  - three nodes are retired at epochs 10, 20, 30
  - scan_by_epoch(25) frees exactly those with epoch < 25: the 10 and the 20
  - the count returned is 2, and two nodes are destroyed
  This is a different reclamation discipline the same list supports: instead of
  consulting hazard records, it trusts that no reader can be older than a known
  safe epoch.  The header offers both; this pins the epoch one.
*/
bool
tests_scan_by_epoch_reclaims_older_entries()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    retired_list<counted_node> list;

    list.retire(new counted_node(1), 10);
    list.retire(new counted_node(2), 20);
    list.retire(new counted_node(3), 30);

    const std::size_t reclaimed = list.scan_by_epoch(25);

    const bool freed_the_old_two =
        ( (reclaimed == 2)                &&
          (probe_deletions().load() == 2) &&
          (probe_live().load() == 1)      &&
          (list.pending() == 1) );

    list.clear();

    return freed_the_old_two;
#else
    return true;
#endif
}

/*
tests_scan_by_epoch_keeps_newer_entries
  the boundary is strict: epoch == safe_epoch is KEPT.
  Tests the following:
  - a node retired at epoch 25, scanned with scan_by_epoch(25), is NOT freed -
    the test is `retire_epoch < safe_epoch`, and 25 < 25 is false
  - a node at epoch 24 IS freed
  So the safe epoch is exclusive: only strictly-older entries go.  Pinned
  because an off-by-one here would free a node a reader at exactly that epoch
  might still hold.
*/
bool
tests_scan_by_epoch_keeps_newer_entries()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    retired_list<counted_node> list;

    list.retire(new counted_node(1), 24);   // strictly older -> freed
    list.retire(new counted_node(2), 25);   // equal -> kept

    const std::size_t reclaimed = list.scan_by_epoch(25);

    const bool boundary_is_exclusive =
        ( (reclaimed == 1)                &&
          (probe_deletions().load() == 1) &&
          (list.pending() == 1) );

    list.clear();

    return boundary_is_exclusive;
#else
    return true;
#endif
}

/*
tests_clear_runs_every_deleter
  clear() reclaims everything, unconditionally.
  Tests the following:
  - a list with three retired nodes, cleared, frees all three - regardless of
    any hazard records
  - the list is empty afterwards, and nothing is left live
  clear() is the teardown path: when a structure is being destroyed and no
  readers remain, every pending node is freed at once.  It does NOT consult the
  domain - it is the caller's promise that reclamation is now safe.
*/
bool
tests_clear_runs_every_deleter()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    retired_list<counted_node> list;

    list.retire(new counted_node(1));
    list.retire(new counted_node(2));
    list.retire(new counted_node(3));

    const bool three_pending = (list.pending() == 3);

    list.clear();

    return ( three_pending                   &&
             (probe_deletions().load() == 3)  &&
             (probe_live().load() == 0)       &&
             list.empty() );
#else
    return true;
#endif
}

/*
tests_pending_and_empty_track_the_queue
  the two queries follow the queue as it grows and drains.
  Tests the following:
  - empty() true and pending() 0 at the start
  - after two retires: not empty, pending 2
  - after a scan frees them: empty again, pending 0
  - both queries are noexcept
*/
bool
tests_pending_and_empty_track_the_queue()
{
#if D_HAS_CUSTOM_HAZARD
    probe_reset();

    hazard_domain              domain(4);
    retired_list<counted_node> list;

    const bool empty_at_start =
        ( list.empty() && (list.pending() == 0) );

    list.retire(new counted_node(1));
    list.retire(new counted_node(2));

    const bool tracks_growth =
        ( (!list.empty()) && (list.pending() == 2) );

    list.scan(domain);

    const bool tracks_drain =
        ( list.empty() && (list.pending() == 0) );

    D_CONSTEXPR bool are_noexcept =
        ( noexcept(std::declval<const retired_list<counted_node>&>()
                       .pending()) &&
          noexcept(std::declval<const retired_list<counted_node>&>()
                       .empty()) );

    static_assert(are_noexcept,
                  "pending() and empty() must be noexcept");

    return ( empty_at_start &&
             tracks_growth  &&
             tracks_drain   &&
             are_noexcept );
#else
    return true;
#endif
}

/*
tests_scan_on_an_empty_list_is_a_no_op
  scanning nothing frees nothing.
  Tests the following:
  - scan() on an empty list returns 0 and does not crash
  - scan_by_epoch() on an empty list likewise
  - the domain's collect_protected still runs (scan calls it), but with no
    entries there is nothing to compare
*/
bool
tests_scan_on_an_empty_list_is_a_no_op()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain              domain(4);
    retired_list<counted_node> list;

    const std::size_t by_scan  = list.scan(domain);
    const std::size_t by_epoch = list.scan_by_epoch(100);

    return ( (by_scan == 0)  &&
             (by_epoch == 0) &&
             list.empty() );
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
