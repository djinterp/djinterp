/*******************************************************************************
* djinterp [core]                           hazard_pointer_tests_domain_scan.cpp
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests_domain_scan.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                 hazard_pointer_tests_domain_scan.cpp
//   Section III (b): hazard_domain - scanning, and the retire counter.
//
//   Two jobs live here.  The scanning queries - is_protected() and
// collect_protected() - are how a writer's reclamation asks "is anyone
// protecting this address?"; they read the active records and report the
// published pointers, and they must ignore idle slots and null entries or a
// scan would either spare too much or crash.  The retire counter is the
// bookkeeping behind should_scan(), the heuristic that decides WHEN to bother:
// it trips at twice the capacity.  Neither job touches memory reclamation
// itself - that is retired_list's section - but both are what make it correct.
//
//   Gated on D_HAS_CUSTOM_HAZARD throughout.

// djinterp
#include "hazard_pointer_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_is_protected_sees_a_published_pointer
  a published pointer is visible to the scan.
  Tests the following:
  - a record that protects an address makes is_protected(address) true
  - so a writer scanning for that address will find it protected and spare it
  This is the read side of the one interaction the whole file is about.
*/
bool
tests_is_protected_sees_a_published_pointer()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(4);
    int            object = 0;
    hazard_record* record = domain.acquire();

    record->protected_ptr.store(&object);

    const bool seen = domain.is_protected(&object);

    domain.release(record);

    return seen;
#else
    return true;
#endif
}

/*
tests_is_protected_is_false_for_an_unprotected_pointer
  the scan does not spare what nobody protects.
  Tests the following:
  - is_protected of an address no record holds is false, even while OTHER
    addresses are protected
  - so a retired node that no reader is using will be found reclaimable
*/
bool
tests_is_protected_is_false_for_an_unprotected_pointer()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(4);
    int            protected_object   = 0;
    int            unprotected_object = 0;
    hazard_record* record = domain.acquire();

    record->protected_ptr.store(&protected_object);

    const bool other_is_free =
        ( domain.is_protected(&protected_object) &&
          (!domain.is_protected(&unprotected_object)) );

    domain.release(record);

    return other_is_free;
#else
    return true;
#endif
}

/*
tests_is_protected_of_null_is_false
  null is never protected.
  Tests the following:
  - is_protected(nullptr) is false, even if a record happens to hold null (which
    a freshly acquired one does)
  - the query short-circuits on a null argument, so a scan asking about a null
    address gets a fast, correct no
*/
bool
tests_is_protected_of_null_is_false()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(4);
    hazard_record* record = domain.acquire();   // holds null, and is active

    const bool null_is_unprotected = (!domain.is_protected(nullptr));

    domain.release(record);

    return null_is_unprotected;
#else
    return true;
#endif
}

/*
tests_is_protected_ignores_an_inactive_record
  only ACTIVE records count.
  Tests the following:
  - a record that still holds a pointer in protected_ptr but whose active flag
    is false does NOT protect that pointer
  - which is why release() clears active: a slot that has been given back must
    not keep pinning whatever it last held
  The test sets this state directly - active false, pointer non-null - to prove
  the active check is what gates is_protected, not merely the pointer value.
*/
bool
tests_is_protected_ignores_an_inactive_record()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(4);
    int            object = 0;
    hazard_record* record = domain.acquire();

    // publish a pointer, then mark the slot inactive WITHOUT clearing the
    // pointer - the exact state release() must never leave, and is_protected
    // must never honour
    record->protected_ptr.store(&object);
    record->active.store(false);

    const bool inactive_does_not_protect = (!domain.is_protected(&object));

    // put it back to a sane state and release
    record->active.store(true);
    domain.release(record);

    return inactive_does_not_protect;
#else
    return true;
#endif
}

/*
tests_collect_gathers_the_protected_pointers
  collect_protected() is is_protected() in bulk.
  Tests the following:
  - two records protecting two addresses produce a vector containing both
  - collect is what a bulk scan uses instead of calling is_protected once per
    retired node - it reads every active slot once
*/
bool
tests_collect_gathers_the_protected_pointers()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(4);
    int            first_object  = 0;
    int            second_object = 0;
    hazard_record* first  = domain.acquire();
    hazard_record* second = domain.acquire();

    first->protected_ptr.store(&first_object);
    second->protected_ptr.store(&second_object);

    std::vector<void*> gathered;
    domain.collect_protected(gathered);

    bool found_first  = false;
    bool found_second = false;
    std::size_t i = 0;

    for (i = 0; i < gathered.size(); ++i)
    {
        if (gathered[i] == &first_object)
        {
            found_first = true;
        }

        if (gathered[i] == &second_object)
        {
            found_second = true;
        }
    }

    domain.release(first);
    domain.release(second);

    return ( (gathered.size() == 2) &&
             found_first            &&
             found_second );
#else
    return true;
#endif
}

/*
tests_collect_clears_its_output_first
  collect_protected() replaces the vector, it does not append.
  Tests the following:
  - a vector with junk in it comes back holding only the currently protected
    pointers
  - so a caller can reuse one scratch vector across many scans without clearing
    it, and a scan never sees a stale address left over from the last one
*/
bool
tests_collect_clears_its_output_first()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(4);
    int            object = 0;
    hazard_record* record = domain.acquire();

    record->protected_ptr.store(&object);

    std::vector<void*> gathered;
    gathered.push_back(reinterpret_cast<void*>(std::size_t(0xDEAD)));
    gathered.push_back(reinterpret_cast<void*>(std::size_t(0xBEEF)));

    domain.collect_protected(gathered);

    const bool only_the_real_one =
        ( (gathered.size() == 1) &&
          (gathered[0] == &object) );

    domain.release(record);

    return only_the_real_one;
#else
    return true;
#endif
}

/*
tests_collect_skips_null_and_inactive_slots
  collect gathers only real protections.
  Tests the following:
  - an active record holding null contributes nothing to the output
  - an inactive record holding a non-null pointer contributes nothing either
  - only active, non-null slots are gathered
  So the vector a scan works from contains addresses that are genuinely
  protected, and nothing else - no nulls to test against, no ghosts from
  released slots.
*/
bool
tests_collect_skips_null_and_inactive_slots()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(8);
    int            real_object   = 0;
    int            stale_object  = 0;

    hazard_record* active_null   = domain.acquire();  // active, holds null
    hazard_record* real          = domain.acquire();  // active, holds &real
    hazard_record* inactive      = domain.acquire();  // will be made inactive

    real->protected_ptr.store(&real_object);

    inactive->protected_ptr.store(&stale_object);
    inactive->active.store(false);

    std::vector<void*> gathered;
    domain.collect_protected(gathered);

    // only &real_object should be present: not the null from active_null, not
    // the &stale_object from the inactive slot
    bool found_real  = false;
    bool found_stale = false;
    bool found_null  = false;
    std::size_t i = 0;

    for (i = 0; i < gathered.size(); ++i)
    {
        if (gathered[i] == &real_object)  { found_real  = true; }
        if (gathered[i] == &stale_object) { found_stale = true; }
        if (gathered[i] == nullptr)       { found_null  = true; }
    }

    inactive->active.store(true);   // restore before releasing
    domain.release(active_null);
    domain.release(real);
    domain.release(inactive);

    return ( found_real       &&
             (!found_stale)   &&
             (!found_null)    &&
             (gathered.size() == 1) );
#else
    return true;
#endif
}

/*
tests_retired_count_starts_at_zero
  a fresh domain has retired nothing.
  Tests the following:
  - retired_count() is 0 at construction
  - it is const and noexcept
*/
bool
tests_retired_count_starts_at_zero()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(4);

    D_CONSTEXPR bool is_noexcept =
        noexcept(std::declval<const hazard_domain&>().retired_count());

    static_assert(is_noexcept, "retired_count() must be noexcept");

    return ( (domain.retired_count() == 0) &&
             is_noexcept );
#else
    return true;
#endif
}

/*
tests_increment_and_decrement_retired
  the retire counter moves by one each way.
  Tests the following:
  - increment_retired() raises the count by one
  - decrement_retired() lowers it by one
  - both are noexcept - they sit on the retire/reclaim path, which must not
    throw
*/
bool
tests_increment_and_decrement_retired()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(4);

    domain.increment_retired();
    domain.increment_retired();
    domain.increment_retired();

    const bool up_to_three = (domain.retired_count() == 3);

    domain.decrement_retired();

    const bool down_to_two = (domain.retired_count() == 2);

    D_CONSTEXPR bool are_noexcept =
        ( noexcept(std::declval<hazard_domain&>().increment_retired()) &&
          noexcept(std::declval<hazard_domain&>().decrement_retired()) );

    static_assert(are_noexcept,
                  "the retire counter operations must be noexcept");

    return (up_to_three && down_to_two && are_noexcept);
#else
    return true;
#endif
}

/*
tests_decrement_retired_takes_a_count
  a scan that reclaims many nodes decrements once, by the batch size.
  Tests the following:
  - decrement_retired(n) lowers the count by n
  - the default argument is 1
  So after a scan frees k nodes, the caller can subtract k in one call rather
  than looping - which is how the counter stays in step with the retired list.
*/
bool
tests_decrement_retired_takes_a_count()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(8);
    std::size_t   i = 0;

    for (i = 0; i < 10; ++i)
    {
        domain.increment_retired();
    }

    domain.decrement_retired(4);

    const bool subtracted_four = (domain.retired_count() == 6);

    domain.decrement_retired();     // default of 1

    const bool default_is_one = (domain.retired_count() == 5);

    return (subtracted_four && default_is_one);
#else
    return true;
#endif
}

/*
tests_should_scan_crosses_at_twice_capacity
  the scan heuristic trips at 2x capacity.
  Tests the following:
  - should_scan() is false while the retired count is below twice the capacity
  - it becomes true exactly when the count reaches 2*capacity
  - so a capacity-4 domain starts wanting to scan at 8 retired nodes
  The threshold is a throughput knob: scan too early and you waste work, too
  late and the retired list grows unbounded.  This pins where the header draws
  it.
*/
bool
tests_should_scan_crosses_at_twice_capacity()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(4);        // threshold is 8
    std::size_t   i = 0;

    for (i = 0; i < 7; ++i)
    {
        domain.increment_retired();
    }

    const bool not_yet = (!domain.should_scan());

    domain.increment_retired();     // now at 8

    const bool now_yes = domain.should_scan();

    D_CONSTEXPR bool is_noexcept =
        noexcept(std::declval<const hazard_domain&>().should_scan());

    static_assert(is_noexcept, "should_scan() must be noexcept");

    return (not_yet && now_yes && is_noexcept);
#else
    return true;
#endif
}

/*
tests_a_zero_capacity_domain_always_wants_to_scan
  the degenerate threshold.
  Tests the following:
  - a capacity-0 domain has should_scan() TRUE from the very start
  - because the threshold is 2*0 == 0, and the retired count begins at 0, which
    is already >= 0
  A quirk, pinned: on a zero-capacity domain the heuristic is useless (it always
  fires), which is one more reason such a domain is a degenerate construction.
  If the threshold ever gains a floor, this is the test that changes.
*/
bool
tests_a_zero_capacity_domain_always_wants_to_scan()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(0);

    return domain.should_scan();
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
