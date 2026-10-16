/*******************************************************************************
* djinterp [core]                           reclamation_common_tests_reclaim.cpp
*
*
* path:      /tests/djinterp/core/sync/reclamation_common_tests_reclaim.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]       reclamation_common_tests_reclaim.cpp
//   Section I(b): reclamation - the half of the engine that actually frees
// things.  reclaim_if() runs the caller's predicate over every retired node,
// deletes the ones it approves, and compacts the survivors in place with two
// cursors; force_reclaim() frees everything unconditionally.
//
//   The section's real job is to show the EXTRACTION IS FAITHFUL.  This engine
// replaced two private copies - RCU's deferred_reclaimer and hazard_pointer's
// reclaim_list - that differed only in how they judged a node safe.  So the
// last three tests drive it through the RCU rule (tag < safe epoch) and the
// hazard rule (pointer not currently protected) and check each reproduces the
// behaviour its original had, then confirm the two really are the same engine.
// If the generalization had lost anything, one of those would fail.
//
//   The compaction gets particular attention: it is an in-place two-cursor
// move, so a survivor that ended up holding another entry's deleter would be
// a silent double-free later.  The tests give every node its own deleter and
// check the recovered ORDER, not merely the count.

// djinterp
#include "reclamation_common_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_reclaim_frees_matching_nodes
  the predicate decides what dies.
  Tests the following:
  - a predicate that approves everything frees every node and empties the list
  - each freed node's own deleter ran
*/
bool
tests_reclaim_frees_matching_nodes()
{
    reclaim_log log;

    reclaim_list<int> list;
    list.retire(new int(1), log.deleter_for());
    list.retire(new int(2), log.deleter_for());

    const std::size_t freed = list.reclaim_if(
        [](const reclaim_list<int>::entry&) { return true; });

    return ( (freed == 2)          &&
             (list.pending() == 0) &&
             (log.count() == 2) );
}

/*
tests_reclaim_keeps_non_matching_nodes
  ... and what survives.
  Tests the following:
  - a predicate that approves nothing frees nothing and leaves the list intact
  - no deleter ran
  This is the case that keeps readers safe: while anyone might still be
  looking, nothing is freed.
*/
bool
tests_reclaim_keeps_non_matching_nodes()
{
    reclaim_log log;

    reclaim_list<int> list;
    list.retire(new int(1), log.deleter_for());
    list.retire(new int(2), log.deleter_for());

    const std::size_t freed = list.reclaim_if(
        [](const reclaim_list<int>::entry&) { return false; });

    const bool kept = ( (freed == 0)          &&
                        (list.pending() == 2) &&
                        (log.count() == 0) );

    list.force_reclaim();

    return kept;
}

/*
tests_reclaim_returns_the_count
  reclaim_if reports how many nodes it freed.
  Tests the following:
  - with five nodes and a predicate approving the two with an even value, the
    return is 2 and three remain pending
  Callers use the count for statistics and for deciding whether a scan was
  worth it.
*/
bool
tests_reclaim_returns_the_count()
{
    reclaim_log log;

    reclaim_list<int> list;
    for (int i = 0; i < 5; ++i)
    {
        list.retire(new int(i), log.deleter_for(),
                    static_cast<std::uint64_t>(i));
    }

    // approve tags 0 and 2 only
    const std::size_t freed = list.reclaim_if(
        [](const reclaim_list<int>::entry& _entry)
        {
            return ((_entry.tag == 0) || (_entry.tag == 2));
        });

    const bool counted = ( (freed == 2) &&
                           (list.pending() == 3) );

    list.force_reclaim();

    return counted;
}

/*
tests_reclaim_runs_each_deleter_exactly_once
  no node is freed twice, and none is skipped.
  Tests the following:
  - across a partial reclaim and a later full one, each of four nodes has its
    deleter run exactly once - the total is four calls with four distinct
    values
  A double-run would be a double-free in production; a skipped one a leak.
*/
bool
tests_reclaim_runs_each_deleter_exactly_once()
{
    reclaim_log log;

    reclaim_list<int> list;
    for (int i = 0; i < 4; ++i)
    {
        list.retire(new int(i), log.deleter_for(),
                    static_cast<std::uint64_t>(i));
    }

    list.reclaim_if(epoch_is_safe(2));   // frees tags 0,1
    list.force_reclaim();                // frees the rest

    if (log.count() != 4)
    {
        return false;
    }

    // four distinct values, each exactly once
    bool seen[4] = { false, false, false, false };
    for (std::size_t i = 0; i < log.freed.size(); ++i)
    {
        const int v = log.freed[i];

        if ((v < 0) || (v > 3) || seen[v])
        {
            return false;
        }

        seen[v] = true;
    }

    return (seen[0] && seen[1] && seen[2] && seen[3]);
}

/*
tests_reclaim_preserves_survivor_order
  compaction keeps the survivors in the order they were retired.
  Tests the following:
  - after freeing the two oldest of four nodes, the remaining two are freed
    later in their original relative order
  Order matters because each survivor must stay paired with its OWN deleter
  and tag; a compaction that shuffled them would eventually free a node with
  the wrong deleter.
*/
bool
tests_reclaim_preserves_survivor_order()
{
    reclaim_log log;

    reclaim_list<int> list;
    for (int i = 0; i < 4; ++i)
    {
        list.retire(new int(i), log.deleter_for(),
                    static_cast<std::uint64_t>(i));
    }

    list.reclaim_if(epoch_is_safe(2));        // frees {0,1}

    const bool first_pass =
        ( (log.count() == 2)  &&
          (log.freed[0] == 0) &&
          (log.freed[1] == 1) );

    list.force_reclaim();                     // frees {2,3}

    const bool second_pass =
        ( (log.count() == 4)  &&
          (log.freed[2] == 2) &&
          (log.freed[3] == 3) );

    return (first_pass && second_pass);
}

/*
tests_reclaim_compacts_non_contiguous_survivors
  the two-cursor compaction handles gaps, not just a prefix.
  Tests the following:
  - with five nodes where the ODD-tagged ones are freed, the surviving even
    ones remain, in order, and are freed correctly afterwards
  Freeing an interior run is the case that exercises the read/write cursor
  divergence; a prefix-only test would pass even with a broken compaction.
*/
bool
tests_reclaim_compacts_non_contiguous_survivors()
{
    reclaim_log log;

    reclaim_list<int> list;
    for (int i = 0; i < 5; ++i)
    {
        list.retire(new int(i), log.deleter_for(),
                    static_cast<std::uint64_t>(i));
    }

    const std::size_t freed = list.reclaim_if(
        [](const reclaim_list<int>::entry& _entry)
        {
            return ((_entry.tag % 2) == 1);       // frees 1 and 3
        });

    const bool odds_gone =
        ( (freed == 2)          &&
          (list.pending() == 3) &&
          (log.count() == 2)    &&
          (log.freed[0] == 1)   &&
          (log.freed[1] == 3) );

    list.force_reclaim();

    const bool evens_in_order =
        ( (log.count() == 5)  &&
          (log.freed[2] == 0) &&
          (log.freed[3] == 2) &&
          (log.freed[4] == 4) );

    return (odds_gone && evens_in_order);
}

/*
tests_reclaim_on_an_empty_list_is_a_no_op
  scanning nothing is harmless.
  Tests the following:
  - reclaim_if on an empty list returns 0 and leaves it empty
  - the predicate is never invoked
  A scan may fire on a threshold poll at any time, including when the list has
  just been drained.
*/
bool
tests_reclaim_on_an_empty_list_is_a_no_op()
{
    reclaim_list<int> list;

    bool predicate_ran = false;

    const std::size_t freed = list.reclaim_if(
        [&predicate_ran](const reclaim_list<int>::entry&)
        {
            predicate_ran = true;
            return true;
        });

    return ( (freed == 0)          &&
             (list.pending() == 0) &&
             (!predicate_ran) );
}

/*
tests_reclaim_can_free_everything
  a fully-approving scan empties the list in one pass.
  Tests the following:
  - six nodes, all approved, are freed together and pending() drops to 0
  - a second scan then finds nothing
*/
bool
tests_reclaim_can_free_everything()
{
    reclaim_log log;

    reclaim_list<int> list;
    for (int i = 0; i < 6; ++i)
    {
        list.retire(new int(i), log.deleter_for());
    }

    const std::size_t first = list.reclaim_if(
        [](const reclaim_list<int>::entry&) { return true; });

    const std::size_t second = list.reclaim_if(
        [](const reclaim_list<int>::entry&) { return true; });

    return ( (first == 6)          &&
             (second == 0)         &&
             (list.pending() == 0) &&
             (log.count() == 6) );
}

/*
tests_force_reclaim_frees_all_regardless
  force_reclaim ignores any notion of safety.
  Tests the following:
  - nodes that a strict predicate would have kept are freed anyway
  - the list is empty afterwards
  This is the teardown path: the owner knows no reader can reach these nodes,
  which is the ONLY circumstance in which it is correct.
*/
bool
tests_force_reclaim_frees_all_regardless()
{
    reclaim_log log;

    reclaim_list<int> list;
    for (int i = 0; i < 3; ++i)
    {
        list.retire(new int(i), log.deleter_for(),
                    static_cast<std::uint64_t>(i));
    }

    // a scan that protects everything reclaims nothing ...
    const std::size_t scanned = list.reclaim_if(epoch_is_safe(0));

    // ... but force_reclaim frees it all
    list.force_reclaim();

    return ( (scanned == 0)        &&
             (list.pending() == 0) &&
             (log.count() == 3) );
}

/*
tests_force_reclaim_returns_the_count
  force_reclaim reports how many it freed.
  Tests the following:
  - force_reclaim over four pending nodes returns 4
*/
bool
tests_force_reclaim_returns_the_count()
{
    reclaim_list<int> list;

    for (int i = 0; i < 4; ++i)
    {
        list.retire(new int(i));
    }

    return (list.force_reclaim() == 4);
}

/*
tests_force_reclaim_on_an_empty_list_is_a_no_op
  tearing down an empty list is harmless.
  Tests the following:
  - force_reclaim on an empty list returns 0 and leaves it empty
  - calling it twice in a row is safe
  A container's destructor calls this unconditionally, including when nothing
  was ever retired.
*/
bool
tests_force_reclaim_on_an_empty_list_is_a_no_op()
{
    reclaim_list<int> list;

    const std::size_t first  = list.force_reclaim();
    const std::size_t second = list.force_reclaim();

    return ( (first == 0)  &&
             (second == 0) &&
             (list.pending() == 0) );
}

/*
tests_reclaim_tolerates_a_non_deleting_deleter
  the engine calls what it was given, and assumes nothing more.
  Tests the following:
  - a deleter that returns a node to a pool instead of deleting it is invoked,
    and the object it "freed" is still intact afterwards
  The deleters are std::function, so pooled or arena storage is a legitimate
  use; the engine must not add a delete of its own.
*/
bool
tests_reclaim_tolerates_a_non_deleting_deleter()
{
    reclaim_list<int> list;

    int  pooled_node   = 42;
    int  deleter_calls = 0;

    list.retire(&pooled_node,
                [&deleter_calls](int*) { ++deleter_calls; });

    list.force_reclaim();

    return ( (deleter_calls == 1) &&
             (pooled_node == 42)  &&
             (list.pending() == 0) );
}

/*
tests_rcu_scheme_reproduces_epoch_reclamation
  the RCU call site behaves as its own engine did.
  Tests the following:
  - four nodes retired at epochs 0..3, with a reader pinned in epoch 2, leaves
    exactly the epoch-0 and epoch-1 nodes reclaimable (strictly-less boundary)
  - the survivors {2,3} are freed once the reader has gone, in order
  This mirrors, exactly, what rcu.hpp's deferred_reclaimer::scan() did with its
  min_active_epoch - the behaviour the extraction had to preserve.
*/
bool
tests_rcu_scheme_reproduces_epoch_reclamation()
{
    reclaim_log log;

    reclaim_list<int> list;
    for (int i = 0; i < 4; ++i)
    {
        list.retire(new int(i), log.deleter_for(),
                    static_cast<std::uint64_t>(i));
    }

    // a reader is sitting in epoch 2: everything retired at epoch >= 2 stays
    const std::size_t reclaimed = list.reclaim_if(epoch_is_safe(2));

    const bool boundary_ok =
        ( (reclaimed == 2)      &&
          (list.pending() == 2) &&
          (log.count() == 2)    &&
          (log.freed[0] == 0)   &&
          (log.freed[1] == 1) );

    // the reader leaves; the frontier moves past everything retired so far
    const std::size_t rest = list.reclaim_if(epoch_is_safe(4));

    const bool drained =
        ( (rest == 2)           &&
          (list.pending() == 0) &&
          (log.count() == 4)    &&
          (log.freed[2] == 2)   &&
          (log.freed[3] == 3) );

    return (boundary_ok && drained);
}

/*
tests_hazard_scheme_reproduces_protected_reclamation
  the hazard-pointer call site behaves as its own engine did.
  Tests the following:
  - with four retired nodes of which two are currently announced in hazard
    records, a scan frees exactly the two unprotected ones
  - once the records are released, a second scan frees the other two
  This mirrors hazard_pointer.hpp's retired_list::scan(), which collected the
  protected pointers from the domain and kept any node found among them.  Note
  the tag goes unused here - the same engine, a different question asked of it.
*/
bool
tests_hazard_scheme_reproduces_protected_reclamation()
{
    reclaim_log log;

    reclaim_list<int> list;

    int* node_a = new int(0);
    int* node_b = new int(1);
    int* node_c = new int(2);
    int* node_d = new int(3);

    list.retire(node_a, log.deleter_for());
    list.retire(node_b, log.deleter_for());
    list.retire(node_c, log.deleter_for());
    list.retire(node_d, log.deleter_for());

    // two readers currently protect b and d
    std::vector<int*> hazards;
    hazards.push_back(node_b);
    hazards.push_back(node_d);

    const std::size_t reclaimed =
        list.reclaim_if(pointer_is_unprotected(&hazards));

    const bool protected_survive =
        ( (reclaimed == 2)      &&
          (list.pending() == 2) &&
          (log.count() == 2)    &&
          (log.freed[0] == 0)   &&
          (log.freed[1] == 2) );

    // the readers release their records
    hazards.clear();

    const std::size_t rest =
        list.reclaim_if(pointer_is_unprotected(&hazards));

    const bool drained =
        ( (rest == 2)           &&
          (list.pending() == 0) &&
          (log.count() == 4)    &&
          (log.freed[2] == 1)   &&
          (log.freed[3] == 3) );

    return (protected_survive && drained);
}

/*
tests_both_schemes_share_one_engine
  the two call sites really are the same code.
  Tests the following:
  - the RCU predicate and the hazard predicate are applied to the SAME
    reclaim_list<int> type - not two instantiations that merely look alike
  - a single list can be scanned with one rule and then the other, each
    honoured in turn
  This is the claim the extraction rests on: one engine, two questions.  If
  the schemes had needed different storage, they could not share a list at all.
*/
bool
tests_both_schemes_share_one_engine()
{
    reclaim_log log;

    reclaim_list<int> list;

    int* survivor = new int(9);

    list.retire(new int(0), log.deleter_for(), 0);   // epoch 0
    list.retire(survivor, log.deleter_for(), 5);     // epoch 5, protected

    // RCU rule first: only the epoch-0 node is below the frontier
    const std::size_t by_epoch = list.reclaim_if(epoch_is_safe(1));

    // hazard rule next: the remaining node is still announced, so it stays
    std::vector<int*> hazards;
    hazards.push_back(survivor);

    const std::size_t by_pointer =
        list.reclaim_if(pointer_is_unprotected(&hazards));

    const bool both_honoured =
        ( (by_epoch == 1)       &&
          (by_pointer == 0)     &&
          (list.pending() == 1) );

    hazards.clear();
    list.force_reclaim();

    return (both_honoured && (log.count() == 2));
}


NS_END  // testing
NS_END  // djinterp
