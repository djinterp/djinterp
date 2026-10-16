/*******************************************************************************
* djinterp [core]                            reclamation_common_tests_retire.cpp
*
*
* path:      /tests/djinterp/core/sync/reclamation_common_tests_retire.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]        reclamation_common_tests_retire.cpp
//   Section I(a): retiring nodes and querying the list.  Everything here is
// the bookkeeping half of the engine - what retire() records, what pending()
// reports, how the advisory scan threshold behaves, and the list's ownership
// story.  Reclamation itself is the next section.
//
//   Note on "leaks": reclaim_list deliberately does NOT free pending nodes in
// its destructor - it cannot know they are unreachable, and the owning
// container reclaims at teardown instead.  So every test here drains what it
// retires, which is also how the real call sites behave.

// djinterp
#include "reclamation_common_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_list_starts_empty
  a fresh list holds nothing.
  Tests the following:
  - pending() is 0 on a default-constructed list
  - should_scan() is false, since no threshold has been armed
*/
bool
tests_list_starts_empty()
{
    reclaim_list<int> list;

    return ( (list.pending() == 0) &&
             (!list.should_scan()) );
}

/*
tests_list_retire_adds_to_pending
  retiring a node records it without freeing it.
  Tests the following:
  - pending() goes 0 -> 1 -> 2 as two nodes are retired
  The deferral is the whole point: the writer has unlinked the node, but a
  reader may still be looking at it, so it waits here.
*/
bool
tests_list_retire_adds_to_pending()
{
    reclaim_list<int> list;

    const bool empty_first = (list.pending() == 0);

    list.retire(new int(1));
    const bool after_one = (list.pending() == 1);

    list.retire(new int(2));
    const bool after_two = (list.pending() == 2);

    list.force_reclaim();

    return (empty_first && after_one && after_two);
}

/*
tests_list_retire_defaults_the_tag_to_zero
  the tag is optional, and defaults to 0.
  Tests the following:
  - a node retired without a tag carries tag 0, as seen by a predicate
  Hazard pointers do not use the tag at all, so the default keeps their call
  site free of a parameter that means nothing to them.
*/
bool
tests_list_retire_defaults_the_tag_to_zero()
{
    reclaim_list<int> list;

    list.retire(new int(5));

    std::uint64_t observed = 999;
    list.reclaim_if(
        [&observed](const reclaim_list<int>::entry& _entry)
        {
            observed = _entry.tag;
            return true;
        });

    return (observed == 0);
}

/*
tests_list_retire_records_the_tag
  a supplied tag reaches the predicate unchanged.
  Tests the following:
  - a node retired with tag 7 is seen by the predicate as tag 7
  RCU stores the retire epoch here, and the whole reclamation decision is made
  from it, so it must survive the round trip exactly.
*/
bool
tests_list_retire_records_the_tag()
{
    reclaim_list<int> list;

    list.retire(new int(5), 7);

    std::uint64_t observed = 0;
    list.reclaim_if(
        [&observed](const reclaim_list<int>::entry& _entry)
        {
            observed = _entry.tag;
            return true;
        });

    return (observed == 7);
}

/*
tests_list_retire_accepts_a_custom_deleter
  the three-argument retire stores the caller's deleter.
  Tests the following:
  - a node retired with a custom deleter has THAT deleter run on reclamation
  - the tag still arrives alongside it
  Callers with pooled or array storage depend on this hook; the default
  `delete` would be wrong for them.
*/
bool
tests_list_retire_accepts_a_custom_deleter()
{
    reclaim_list<int> list;

    bool          custom_ran = false;
    std::uint64_t seen_tag   = 0;

    list.retire(new int(3),
                [&custom_ran](int* _p)
                {
                    custom_ran = true;
                    delete _p;
                },
                4);

    list.reclaim_if(
        [&seen_tag](const reclaim_list<int>::entry& _entry)
        {
            seen_tag = _entry.tag;
            return true;
        });

    return (custom_ran && (seen_tag == 4));
}

/*
tests_list_default_deleter_destroys_the_object
  the one-argument retire really destroys the object.
  Tests the following:
  - retiring a live-counting object through the DEFAULT deleter and then
    reclaiming returns the live count to its baseline - `delete p` ran the
    destructor, rather than the pointer merely being dropped
*/
bool
tests_list_default_deleter_destroys_the_object()
{
    const int baseline = reclaim_probe_live();

    reclaim_list<reclaim_probe> list;

    list.retire(new reclaim_probe(5));
    const bool constructed = (reclaim_probe_live() == baseline + 1);

    list.force_reclaim();
    const bool destroyed = (reclaim_probe_live() == baseline);

    return (constructed && destroyed);
}

/*
tests_list_threshold_is_off_by_default
  no threshold is armed unless the owner asks for one.
  Tests the following:
  - should_scan() stays false however many nodes are retired, when no
    threshold has been set
  The list never scans on its own - only the owner holds the predicate - so an
  unarmed threshold must stay quiet.
*/
bool
tests_list_threshold_is_off_by_default()
{
    reclaim_list<int> list;

    for (int i = 0; i < 10; ++i)
    {
        list.retire(new int(i));
    }

    const bool quiet = !list.should_scan();

    list.force_reclaim();

    return quiet;
}

/*
tests_list_threshold_reports_when_reached
  an armed threshold reports once the pending count reaches it.
  Tests the following:
  - with a threshold of 4, should_scan() is false at 3 pending and true at 4
  - it stays true past the threshold
  This is the advisory signal that replaced RCU's built-in auto-scan: the
  owner polls it and decides, since only the owner can supply the predicate.
*/
bool
tests_list_threshold_reports_when_reached()
{
    reclaim_list<int> list;
    list.set_scan_threshold(4);

    for (int i = 0; i < 3; ++i)
    {
        list.retire(new int(i));
    }

    const bool quiet_below = !list.should_scan();

    list.retire(new int(3));
    const bool fires_at = list.should_scan();

    list.retire(new int(4));
    const bool stays_above = list.should_scan();

    list.force_reclaim();

    return (quiet_below && fires_at && stays_above);
}

/*
tests_list_threshold_zero_disables_the_report
  a threshold of 0 turns the signal off.
  Tests the following:
  - after explicitly setting the threshold to 0, should_scan() is false even
    with nodes pending
  0 is the documented "disabled" value, and it must not read as "every node
  crosses the threshold" - which a naive >= comparison against 0 would.
*/
bool
tests_list_threshold_zero_disables_the_report()
{
    reclaim_list<int> list;

    list.set_scan_threshold(0);
    list.retire(new int(1));
    list.retire(new int(2));

    const bool disabled = !list.should_scan();

    list.force_reclaim();

    return disabled;
}

/*
tests_list_is_move_only
  the list owns its pending nodes uniquely, but may be relocated.
  Tests the following:
  - reclaim_list is neither copy-constructible nor copy-assignable: two lists
    holding the same raw pointers would double-free
  - it IS move-constructible, so a container can hold one by value and still
    be moved
  This is the noncopyable base's contract, inherited rather than hand-written.
*/
bool
tests_list_is_move_only()
{
    typedef reclaim_list<int> list_type;

    const bool no_copy =
        ( !std::is_copy_constructible<list_type>::value &&
          !std::is_copy_assignable<list_type>::value );

    const bool movable =
        std::is_move_constructible<list_type>::value;

    static_assert(!std::is_copy_constructible<list_type>::value,
                  "reclaim_list must not be copyable");

    return (no_copy && movable);
}

/*
tests_list_move_carries_the_pending_nodes
  moving a list takes its pending nodes with it.
  Tests the following:
  - a list with two pending nodes, moved into another, leaves the destination
    holding both
  - reclaiming from the destination frees exactly those two, deleters intact
  The nodes are raw pointers the list is responsible for; a move that dropped
  them would leak, and one that copied them would double-free.
*/
bool
tests_list_move_carries_the_pending_nodes()
{
    reclaim_log log;

    reclaim_list<int> source;
    source.retire(new int(1), log.deleter_for());
    source.retire(new int(2), log.deleter_for());

    reclaim_list<int> destination(std::move(source));

    const bool carried = (destination.pending() == 2);

    const std::size_t freed = destination.force_reclaim();

    return ( carried              &&
             (freed == 2)         &&
             (log.count() == 2)   &&
             (log.freed[0] == 1)  &&
             (log.freed[1] == 2) );
}

/*
tests_list_entry_exposes_pointer_and_tag
  the predicate can see everything it needs to decide.
  Tests the following:
  - an entry hands the predicate the exact pointer that was retired, and the
    exact tag
  The two schemes each read one of those fields - RCU the tag, hazard the
  pointer - so both must be reachable from the entry the predicate receives.
*/
bool
tests_list_entry_exposes_pointer_and_tag()
{
    reclaim_list<int> list;

    int* node = new int(11);
    list.retire(node, 3);

    int*          seen_ptr = NULL;
    std::uint64_t seen_tag = 0;

    list.reclaim_if(
        [&seen_ptr, &seen_tag](const reclaim_list<int>::entry& _entry)
        {
            seen_ptr = _entry.ptr;
            seen_tag = _entry.tag;
            return true;
        });

    return ( (seen_ptr == node) &&
             (seen_tag == 3) );
}


NS_END  // testing
NS_END  // djinterp
