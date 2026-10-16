/*******************************************************************************
* djinterp [core]                                        rcu_tests_reclaimer.cpp
*
*
* path:      /tests/djinterp/core/sync/rcu_tests_reclaimer.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                    rcu_tests_reclaimer.cpp
// Section V: deferred_reclaimer<T> - the engine that ties an epoch_counter, an
// epoch_registry and a retired list together and decides WHEN a retired node is
// safe to free.  retire() advances the epoch and records the node with the
// PREVIOUS epoch; scan() frees every retired node whose epoch is strictly below
// the registry's minimum active epoch, compacting the survivors;
// force_reclaim() frees everything regardless (shutdown only); a scan fires
// automatically once the retired list reaches max_threads * 2.
//
// These are the reclamation-safety tests, and they are DETERMINISTIC: a reader
// held in an epoch pins everything retired at or after it, and releasing the
// reader lets the next scan collect it.  No threads are needed to show that -
// the concurrency section adds real contention on top.

// djinterp
#include "rcu_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_reclaimer_starts_empty
  a fresh reclaimer has retired nothing and is at epoch 0.
  Tests the following:
  - pending() == 0
  - current_epoch() == 0
*/
bool
tests_reclaimer_starts_empty()
{
    deferred_reclaimer<int> reclaimer(4);

    return ( (reclaimer.pending() == 0) &&
             (reclaimer.current_epoch() == 0) );
}

/*
tests_reclaimer_retire_advances_the_epoch
  retiring a node moves the epoch forward.
  Tests the following:
  - current_epoch() is 0 before, 1 after a single retire()
  Each retirement opens a new generation, which is what lets later readers be
  distinguished from the one being retired against.
*/
bool
tests_reclaimer_retire_advances_the_epoch()
{
    deferred_reclaimer<int> reclaimer(4);

    const bool before = (reclaimer.current_epoch() == 0);
    reclaimer.retire(new int(1));
    const bool after = (reclaimer.current_epoch() == 1);

    // the reclaimer does not free pending nodes when it dies - it cannot know
    // they are unreachable - so the owner drains, which here is us.
    reclaimer.force_reclaim();

    return (before && after);
}

/*
tests_reclaimer_retire_adds_to_pending
  a retired node sits in the pending list until a scan clears it.
  Tests the following:
  - pending() is 0 before and 1 immediately after retire() (nothing has been
    freed yet - retire only records)
*/
bool
tests_reclaimer_retire_adds_to_pending()
{
    deferred_reclaimer<int> reclaimer(4);

    const bool before = (reclaimer.pending() == 0);
    reclaimer.retire(new int(1));
    const bool after = (reclaimer.pending() == 1);

    // drain what this test retired; pending nodes are the owner's to free.
    reclaimer.force_reclaim();

    return (before && after);
}

/*
tests_reclaimer_scan_reclaims_with_no_readers
  with nobody reading, a scan frees what was retired.
  Tests the following:
  - after retiring one node with no active readers, scan() reclaims it (the
    custom deleter runs) and pending() drops to 0
  With no readers the frontier is inactive (the maximum), so every real
  retirement is below it and collectable.
*/
bool
tests_reclaimer_scan_reclaims_with_no_readers()
{
    deferred_reclaimer<int> reclaimer(4);

    int deleted = 0;
    reclaimer.retire(new int(7),
                     [&deleted](int* p){ ++deleted; delete p; });

    const std::size_t reclaimed = reclaimer.scan();

    return ( (reclaimed == 1) &&
             (deleted == 1)   &&
             (reclaimer.pending() == 0) );
}

/*
tests_reclaimer_active_reader_blocks_reclamation
  a reader in the retiring epoch holds the node back.
  Tests the following:
  - a reader entered at epoch 0, then a node retired (recorded at epoch 0), then
    a scan: nothing is reclaimed, the node stays pending, the deleter has NOT
    run
  The boundary is `retire_epoch < min_active_epoch`; with the reader pinning
  epoch 0, a node recorded at epoch 0 is not strictly below it, so it survives.
*/
bool
tests_reclaimer_active_reader_blocks_reclamation()
{
    deferred_reclaimer<int> reclaimer(4);

    std::atomic<std::uint64_t>* slot = reclaimer.register_reader();
    if (!slot)
    {
        return false;
    }

    int  deleted = 0;
    bool ok      = false;
    {
        epoch_guard guard(reclaimer.epoch(), *slot);   // reader in epoch 0

        reclaimer.retire(new int(7),
                         [&deleted](int* p){ ++deleted; delete p; });

        const std::size_t reclaimed = reclaimer.scan();

        ok = ( (reclaimed == 0)          &&
               (deleted == 0)            &&
               (reclaimer.pending() == 1) );
    }

    reclaimer.unregister_reader(slot);

    // the reader is gone, so the node it was holding back is now ours to free.
    reclaimer.force_reclaim();

    return ok;
}

/*
tests_reclaimer_reclaims_after_reader_exits
  once the pinning reader leaves, the node becomes collectable.
  Tests the following:
  - the same setup as the blocking test: while the reader is in epoch 0 the scan
    reclaims nothing; after the reader exits, the next scan reclaims the node
    and runs its deleter
  This is the whole point of deferred reclamation - it waits out the reader.
*/
bool
tests_reclaimer_reclaims_after_reader_exits()
{
    deferred_reclaimer<int> reclaimer(4);

    std::atomic<std::uint64_t>* slot = reclaimer.register_reader();
    if (!slot)
    {
        return false;
    }

    int deleted = 0;

    {
        epoch_guard guard(reclaimer.epoch(), *slot);   // reader in epoch 0
        reclaimer.retire(new int(7),
                         [&deleted](int* p){ ++deleted; delete p; });

        if (reclaimer.scan() != 0)                     // blocked
        {
            reclaimer.unregister_reader(slot);
            return false;
        }
    }   // reader exits here

    const std::size_t reclaimed = reclaimer.scan();    // now collectable

    reclaimer.unregister_reader(slot);

    return ( (reclaimed == 1) &&
             (deleted == 1)   &&
             (reclaimer.pending() == 0) );
}

/*
tests_reclaimer_custom_deleter_is_used
  the two-argument retire runs the caller's deleter.
  Tests the following:
  - a node retired with a custom deleter has THAT deleter invoked on scan (a
    flag it sets is observed), not the default delete
  Callers that manage memory specially (pools, arrays) rely on this hook.
*/
bool
tests_reclaimer_custom_deleter_is_used()
{
    deferred_reclaimer<int> reclaimer(4);

    bool custom_ran = false;
    reclaimer.retire(new int(3),
                     [&custom_ran](int* p)
                     {
                         custom_ran = true;
                         delete p;
                     });

    reclaimer.scan();

    return custom_ran;
}

/*
tests_reclaimer_default_deleter_destroys_the_object
  the one-argument retire really destroys the object, not just drops the
  pointer.
  Tests the following:
  - retiring a live-instance-counting object through the DEFAULT deleter, then
    scanning, returns the live count to its baseline - i.e. `delete p` ran the
    destructor
  Uses destruct_probe so the destructor is observable.
*/
bool
tests_reclaimer_default_deleter_destroys_the_object()
{
    const long baseline = destruct_probe_live().load();

    deferred_reclaimer<destruct_probe> reclaimer(4);

    destruct_probe* probe = new destruct_probe(5);
    const bool constructed =
        (destruct_probe_live().load() == baseline + 1);

    reclaimer.retire(probe);                 // default deleter: delete probe
    const std::size_t reclaimed = reclaimer.scan();

    const bool destroyed =
        (destruct_probe_live().load() == baseline);

    return (constructed && (reclaimed == 1) && destroyed);
}

/*
tests_reclaimer_force_reclaim_ignores_readers
  force_reclaim frees everything, active readers notwithstanding.
  Tests the following:
  - with a reader pinned in epoch 0 (so a normal scan reclaims nothing), a
    force_reclaim frees all three retired nodes and empties the pending list
  This is the shutdown escape hatch - correct only when the caller knows no
  reader will touch the freed nodes, which the test guarantees by construction.
*/
bool
tests_reclaimer_force_reclaim_ignores_readers()
{
    deferred_reclaimer<int> reclaimer(4);

    std::atomic<std::uint64_t>* slot = reclaimer.register_reader();
    if (!slot)
    {
        return false;
    }

    int  deleted = 0;
    bool ok      = false;
    {
        epoch_guard guard(reclaimer.epoch(), *slot);   // reader in epoch 0

        for (int i = 0; i < 3; ++i)
        {
            reclaimer.retire(new int(i),
                             [&deleted](int* p){ ++deleted; delete p; });
        }

        const std::size_t blocked = reclaimer.scan();  // reader blocks all
        reclaimer.force_reclaim();                     // ... but this does not

        ok = ( (blocked == 0)             &&
               (deleted == 3)             &&
               (reclaimer.pending() == 0) );
    }

    reclaimer.unregister_reader(slot);

    return ok;
}

/*
tests_reclaimer_auto_scan_fires_at_threshold
  the retired list scans itself once it reaches max_threads * 2.
  Tests the following:
  - on a max_threads-2 reclaimer (threshold 4) with no readers, retiring the
    fourth node triggers an automatic scan that drains the list to 0
  Growth is bounded without the caller having to poll scan().
*/
bool
tests_reclaimer_auto_scan_fires_at_threshold()
{
    deferred_reclaimer<int> reclaimer(2);    // threshold = 2 * 2 = 4

    int deleted = 0;
    for (int i = 0; i < 4; ++i)
    {
        reclaimer.retire(new int(i),
                         [&deleted](int* p){ ++deleted; delete p; });
    }

    // The fourth retire made size() reach 4 == threshold, so a scan ran; with
    // no readers it collected everything.
    return ( (reclaimer.pending() == 0) &&
             (deleted == 4) );
}

/*
tests_reclaimer_scan_returns_the_count
  scan reports how many nodes it freed.
  Tests the following:
  - retiring three nodes with no readers and then scanning returns 3
*/
bool
tests_reclaimer_scan_returns_the_count()
{
    deferred_reclaimer<int> reclaimer(8);    // threshold 16, no auto-scan here

    for (int i = 0; i < 3; ++i)
    {
        reclaimer.retire(new int(i));
    }

    return (reclaimer.scan() == 3);
}

/*
tests_reclaimer_scan_compacts_and_keeps_survivors
  a partial scan frees the collectable nodes and keeps the rest, in order.
  Tests the following:
  - two nodes retired at epochs 0 and 1, then a reader enters at epoch 2, then
    two more retired at epochs 2 and 3
  - a scan frees exactly the first two (epochs 0,1 < frontier 2) and keeps the
    last two; pending() is 2, and the freed values are {0,1} in retire order
  - after the reader exits, a second scan frees the survivors, and their values
    come out {2,3} in order - proving the compaction preserved both membership
    and ordering
*/
bool
tests_reclaimer_scan_compacts_and_keeps_survivors()
{
    deferred_reclaimer<int> reclaimer(8);    // threshold 16, no auto-scan

    std::atomic<std::uint64_t>* slot = reclaimer.register_reader();
    if (!slot)
    {
        return false;
    }

    int deleted_order[4] = { -1, -1, -1, -1 };
    int deleted_count    = 0;

    // deleter records the freed value in call order
    deferred_reclaimer<int>::deleter_fn rec =
        [&deleted_order, &deleted_count](int* p)
        {
            if (deleted_count < 4)
            {
                deleted_order[deleted_count] = *p;
            }
            ++deleted_count;
            delete p;
        };

    reclaimer.retire(new int(0), rec);       // retire_epoch 0
    reclaimer.retire(new int(1), rec);       // retire_epoch 1

    bool first_ok = false;
    {
        epoch_guard guard(reclaimer.epoch(), *slot);   // reader enters epoch 2

        reclaimer.retire(new int(2), rec);   // retire_epoch 2
        reclaimer.retire(new int(3), rec);   // retire_epoch 3

        const std::size_t reclaimed = reclaimer.scan();   // frees {0,1}

        first_ok = ( (reclaimed == 2)             &&
                     (reclaimer.pending() == 2)   &&
                     (deleted_order[0] == 0)      &&
                     (deleted_order[1] == 1) );
    }   // reader exits

    const std::size_t rest = reclaimer.scan();            // frees survivors

    reclaimer.unregister_reader(slot);

    const bool second_ok = ( (rest == 2)                &&
                             (reclaimer.pending() == 0) &&
                             (deleted_order[2] == 2)    &&
                             (deleted_order[3] == 3) );

    return (first_ok && second_ok);
}

/*
tests_reclaimer_is_not_copyable_or_movable
  the reclaimer is a fixed engine, not a value.
  Tests the following:
  - deferred_reclaimer<int> is not copy-constructible or copy-assignable
  - deferred_reclaimer<int> is not move-constructible or move-assignable
  It aggregates an epoch_counter and an epoch_registry (both pinned) and owns a
  retired list of raw pointers; duplicating or relocating it would break both
  the reader slots and the pending frees.
*/
bool
tests_reclaimer_is_not_copyable_or_movable()
{
    typedef deferred_reclaimer<int> reclaimer_type;

    D_CONSTEXPR bool pinned =
        ( !std::is_copy_constructible<reclaimer_type>::value &&
          !std::is_copy_assignable<reclaimer_type>::value    &&
          !std::is_move_constructible<reclaimer_type>::value &&
          !std::is_move_assignable<reclaimer_type>::value );

    static_assert(pinned,
                  "deferred_reclaimer must be neither copyable nor movable");

    return pinned;
}


NS_END  // testing
NS_END  // djinterp
