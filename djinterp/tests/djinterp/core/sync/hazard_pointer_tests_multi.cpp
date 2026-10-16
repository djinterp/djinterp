/*******************************************************************************
* djinterp [core]                                 hazard_pointer_tests_multi.cpp
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests_multi.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                     hazard_pointer_tests_multi.cpp
//   Section VII: multi_hazard_domain - N hazard slots per thread.
//
//   Some lock-free algorithms need to protect more than one pointer at once - a
// lock-free list traversal holds both the current node and its successor.  This
// domain gives each thread a contiguous GROUP of _SlotsPerThread records,
// claimed
// as a unit.  The claim is the subtle part: acquire_slot_group() CASes only the
// FIRST slot of a group to stake it, then plain-stores the rest active - which
// is
// safe only because no other thread can win the same first slot.  The suite
// pins
// the geometry (capacity groups, each of _SlotsPerThread slots), the contiguity
// and full activation of a claimed group, exhaustion at capacity groups, and
// the
// scan queries reaching across every slot of every group.
//
//   Gated on D_HAS_CUSTOM_HAZARD throughout.  Everything is instantiated at
// _SlotsPerThread == 2 (the common case) unless a test is specifically about
// the
// template argument.

// djinterp
#include "hazard_pointer_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_multi_reports_its_geometry
  the domain remembers its two dimensions.
  Tests the following:
  - capacity() is the group count (the constructor argument)
  - total_slots() is capacity * slots_per_thread
  - a capacity-3, 2-slot domain therefore has 3 groups and 6 slots
  - the default group count is 64
*/
bool
tests_multi_reports_its_geometry()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(3);
    multi_hazard_domain<2> defaulted;

    return ( (domain.capacity() == 3)          &&
             (domain.total_slots() == 6)       &&
             (defaulted.capacity() == 64)      &&
             (defaulted.total_slots() == 128) );
#else
    return true;
#endif
}

/*
tests_multi_declares_the_hazard_strategy_tag
  the multi domain is hazard-strategy too.
  Tests the following:
  - multi_hazard_domain<2>::concurrency_strategy_tag IS hazard_strategy_tag
  - the alias is public
  So a container built on the multi domain classifies the same way as one built
  on the single domain.
*/
bool
tests_multi_declares_the_hazard_strategy_tag()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool tagged_hazard =
        std::is_same<multi_hazard_domain<2>::concurrency_strategy_tag,
                     hazard_strategy_tag>::value;

    static_assert(tagged_hazard,
                  "multi_hazard_domain must declare the hazard strategy tag");

    return tagged_hazard;
#else
    return true;
#endif
}

/*
tests_multi_marks_itself_with_hazard_domain_type
  its self-marker names itself.
  Tests the following:
  - multi_hazard_domain<2>::hazard_domain_type IS multi_hazard_domain<2>
  - and a different slot count is a different marker
*/
bool
tests_multi_marks_itself_with_hazard_domain_type()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool names_itself =
        std::is_same<multi_hazard_domain<2>::hazard_domain_type,
                     multi_hazard_domain<2> >::value;

    D_CONSTEXPR bool specific_per_arity =
        !std::is_same<multi_hazard_domain<2>::hazard_domain_type,
                      multi_hazard_domain<3> >::value;

    static_assert(names_itself,
                  "the multi domain's self-marker must name itself");

    return (names_itself && specific_per_arity);
#else
    return true;
#endif
}

/*
tests_multi_is_not_copyable
  the multi domain owns a record array and cannot be copied.
  Tests the following:
  - not copy-constructible, not copy-assignable
*/
bool
tests_multi_is_not_copyable()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<multi_hazard_domain<2> >::value &&
          !std::is_copy_assignable<multi_hazard_domain<2> >::value );

    static_assert(no_copy, "multi_hazard_domain must not be copyable");

    return no_copy;
#else
    return true;
#endif
}

/*
tests_acquire_group_hands_out_a_group
  acquire_slot_group() gives a caller a group to work with.
  Tests the following:
  - a fresh domain hands back a non-null group pointer
  - the group's first slot is active and holds null
*/
bool
tests_acquire_group_hands_out_a_group()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(2);

    hazard_record* group = domain.acquire_slot_group();

    const bool handed_out =
        ( (group != nullptr) &&
          (group[0].active.load() == true) &&
          (group[0].protected_ptr.load() == nullptr) );

    if (group)
    {
        domain.release_slot_group(group);
    }

    return handed_out;
#else
    return true;
#endif
}

/*
tests_group_is_contiguous_and_fully_active
  a claimed group is _SlotsPerThread consecutive, all-active records.
  Tests the following:
  - for a 3-slot domain, every one of the group's three slots is active - not
    just the first, which was CASed; the other two were plain-stored active
  - a second group begins exactly _SlotsPerThread records after the first, so
    the groups are laid out contiguously with no gap
  This pins the claim protocol: the first slot stakes the group, the rest are
  marked without contention because no other thread can reach them.
*/
bool
tests_group_is_contiguous_and_fully_active()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<3> domain(2);

    hazard_record* first  = domain.acquire_slot_group();
    hazard_record* second = domain.acquire_slot_group();

    const bool all_three_active =
        ( first[0].active.load() &&
          first[1].active.load() &&
          first[2].active.load() );

    const bool contiguous =
        ( (first != nullptr)  &&
          (second != nullptr) &&
          ((second - first) == 3) );   // groups sit 3 records apart

    domain.release_slot_group(first);
    domain.release_slot_group(second);

    return (all_three_active && contiguous);
#else
    return true;
#endif
}

/*
tests_acquire_group_fills_then_returns_null
  the group pool is fixed at `capacity` groups.
  Tests the following:
  - a capacity-3 domain hands out exactly three groups
  - the fourth acquire_slot_group returns nullptr
  - after releasing one group, a group is available again
*/
bool
tests_acquire_group_fills_then_returns_null()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(3);

    hazard_record* a = domain.acquire_slot_group();
    hazard_record* b = domain.acquire_slot_group();
    hazard_record* c = domain.acquire_slot_group();
    hazard_record* overflow = domain.acquire_slot_group();

    const bool filled_then_failed =
        ( (a != nullptr) &&
          (b != nullptr) &&
          (c != nullptr) &&
          (overflow == nullptr) );

    domain.release_slot_group(b);

    hazard_record* reused = domain.acquire_slot_group();

    const bool reuse_works = (reused != nullptr);

    domain.release_slot_group(a);
    domain.release_slot_group(c);
    domain.release_slot_group(reused);

    return (filled_then_failed && reuse_works);
#else
    return true;
#endif
}

/*
tests_release_group_clears_every_slot
  releasing a group returns and clears all of its slots.
  Tests the following:
  - a group with pointers published in both slots, once released, protects
    neither
  - every slot's active flag is cleared, so the group can be re-claimed
  A group half-released would leave a slot pinning a node forever, so release
  must clear the whole group.
*/
bool
tests_release_group_clears_every_slot()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(2);
    int                    first_object  = 0;
    int                    second_object = 0;

    hazard_record* group = domain.acquire_slot_group();

    group[0].protected_ptr.store(&first_object);
    group[1].protected_ptr.store(&second_object);

    const bool both_protected =
        ( domain.is_protected(&first_object) &&
          domain.is_protected(&second_object) );

    domain.release_slot_group(group);

    const bool both_cleared =
        ( (!domain.is_protected(&first_object)) &&
          (!domain.is_protected(&second_object)) );

    // and the slots are idle: a re-acquire yields a group whose slots are null
    hazard_record* reacquired = domain.acquire_slot_group();

    const bool slots_reset =
        ( (reacquired[0].protected_ptr.load() == nullptr) &&
          (reacquired[1].protected_ptr.load() == nullptr) );

    domain.release_slot_group(reacquired);

    return (both_protected && both_cleared && slots_reset);
#else
    return true;
#endif
}

/*
tests_multi_is_protected_scans_every_slot
  is_protected() checks every slot of every group.
  Tests the following:
  - a pointer published in the SECOND slot of a group is found by is_protected -
    the scan does not stop at first slots
  - a pointer nobody published is not found
*/
bool
tests_multi_is_protected_scans_every_slot()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(2);
    int                    in_second_slot = 0;
    int                    nowhere        = 0;

    hazard_record* group = domain.acquire_slot_group();

    group[1].protected_ptr.store(&in_second_slot);   // the non-first slot

    const bool found_second_slot = domain.is_protected(&in_second_slot);
    const bool missing_is_absent = (!domain.is_protected(&nowhere));

    domain.release_slot_group(group);

    return (found_second_slot && missing_is_absent);
#else
    return true;
#endif
}

/*
tests_multi_collect_gathers_across_groups
  collect_protected() gathers from every slot of every group.
  Tests the following:
  - two groups, each protecting a pointer in a different slot, produce a vector
    with both pointers
  - collect clears its output first (a pre-seeded vector comes back with only
    the
    real entries)
*/
bool
tests_multi_collect_gathers_across_groups()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(3);
    int                    first_object  = 0;
    int                    second_object = 0;

    hazard_record* group_one = domain.acquire_slot_group();
    hazard_record* group_two = domain.acquire_slot_group();

    group_one[0].protected_ptr.store(&first_object);
    group_two[1].protected_ptr.store(&second_object);

    std::vector<void*> gathered;
    gathered.push_back(reinterpret_cast<void*>(std::size_t(0xDEAD)));

    domain.collect_protected(gathered);

    bool found_first  = false;
    bool found_second = false;
    std::size_t i = 0;

    for (i = 0; i < gathered.size(); ++i)
    {
        if (gathered[i] == &first_object)  { found_first  = true; }
        if (gathered[i] == &second_object) { found_second = true; }
    }

    domain.release_slot_group(group_one);
    domain.release_slot_group(group_two);

    return ( (gathered.size() == 2) &&
             found_first            &&
             found_second );
#else
    return true;
#endif
}

/*
tests_multi_should_scan_crosses_at_twice_total_slots
  the multi domain's scan heuristic trips at 2x TOTAL slots, not 2x capacity.
  Tests the following:
  - a capacity-4, 2-slot domain has 8 total slots, so its threshold is 16
  - should_scan() is false at 15 retired and true at 16
  This differs from the single domain, whose threshold is 2*capacity - here the
  slot count, not the group count, sets the bar.  Pinned because the two domains
  compute it from different fields.
*/
bool
tests_multi_should_scan_crosses_at_twice_total_slots()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(4);       // total_slots 8, threshold 16
    std::size_t            i = 0;

    for (i = 0; i < 15; ++i)
    {
        domain.increment_retired();
    }

    const bool not_yet = (!domain.should_scan());

    domain.increment_retired();             // now 16

    const bool now_yes = domain.should_scan();

    return (not_yet && now_yes);
#else
    return true;
#endif
}

/*
tests_multi_slots_per_thread_is_the_template_argument
  the slot count is a compile-time constant equal to the template argument.
  Tests the following:
  - multi_hazard_domain<2>::slots_per_thread is 2, <4>::slots_per_thread is 4
  - it is a static constexpr, usable in a constant expression
*/
bool
tests_multi_slots_per_thread_is_the_template_argument()
{
#if D_HAS_CUSTOM_HAZARD
    D_STATIC D_CONSTEXPR std::size_t two  =
        multi_hazard_domain<2>::slots_per_thread;
    D_STATIC D_CONSTEXPR std::size_t four =
        multi_hazard_domain<4>::slots_per_thread;

    static_assert(two == 2,  "slots_per_thread<2> must be 2");
    static_assert(four == 4, "slots_per_thread<4> must be 4");

    return ( (two == 2) && (four == 4) );
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
