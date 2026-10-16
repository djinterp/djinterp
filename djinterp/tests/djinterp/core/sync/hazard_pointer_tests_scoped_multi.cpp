/*******************************************************************************
* djinterp [core]                          hazard_pointer_tests_scoped_multi.cpp
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests_scoped_multi.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]               hazard_pointer_tests_scoped_multi.cpp
//   Section VIII: scoped_multi_hazard - RAII around a multi-slot group.
//
//   The multi-domain analogue of scoped_hazard: it claims a group on
// construction and releases it on destruction, and offers per-slot protect()
// and
// clear() plus a clear_all().  Same exhaustion story - an empty pool yields an
// invalid guard whose operations are no-ops - and one extra edge: protect() and
// clear() take a slot index, and an out-of-range index is silently ignored
// rather than writing past the group.
//
//   Gated on D_HAS_CUSTOM_HAZARD throughout.

// djinterp
#include "hazard_pointer_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_scoped_multi_acquires_a_group
  a scoped_multi_hazard claims a group as soon as it exists.
  Tests the following:
  - built on a domain with a free group, it is valid()
  - it consumed one group: a capacity-1 domain has none left
*/
bool
tests_scoped_multi_acquires_a_group()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(1);

    scoped_multi_hazard<2> guard(domain);

    const bool valid_and_took_one =
        ( guard.valid() &&
          (domain.acquire_slot_group() == nullptr) );

    return valid_and_took_one;
#else
    return true;
#endif
}

/*
tests_scoped_multi_releases_on_destruction
  the group goes back when the guard leaves scope.
  Tests the following:
  - a capacity-1 domain, consumed by a guard, has its group available again once
    the guard is destroyed
*/
bool
tests_scoped_multi_releases_on_destruction()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(1);

    {
        scoped_multi_hazard<2> guard(domain);

        if (domain.acquire_slot_group() != nullptr)
        {
            return false;
        }
    }

    hazard_record* reclaimed = domain.acquire_slot_group();

    const bool released = (reclaimed != nullptr);

    if (reclaimed)
    {
        domain.release_slot_group(reclaimed);
    }

    return released;
#else
    return true;
#endif
}

/*
tests_scoped_multi_protects_a_slot
  protect(slot, ptr) publishes into the given slot.
  Tests the following:
  - protecting slot 0 and slot 1 with two pointers makes both is_protected
  - so the guard is a working front-end to the group's per-slot publish
*/
bool
tests_scoped_multi_protects_a_slot()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(2);
    int                    first_object  = 0;
    int                    second_object = 0;

    scoped_multi_hazard<2> guard(domain);

    guard.protect(0, &first_object);
    guard.protect(1, &second_object);

    return ( domain.is_protected(&first_object) &&
             domain.is_protected(&second_object) );
#else
    return true;
#endif
}

/*
tests_scoped_multi_clears_a_slot
  clear(slot) unpublishes one slot and leaves the others alone.
  Tests the following:
  - with both slots protecting, clear(0) drops the first and keeps the second
  - so slots are independent: clearing one hazard does not disturb another the
    same thread holds
*/
bool
tests_scoped_multi_clears_a_slot()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(2);
    int                    first_object  = 0;
    int                    second_object = 0;

    scoped_multi_hazard<2> guard(domain);

    guard.protect(0, &first_object);
    guard.protect(1, &second_object);

    guard.clear(0);

    return ( (!domain.is_protected(&first_object)) &&
             domain.is_protected(&second_object) );
#else
    return true;
#endif
}

/*
tests_scoped_multi_clear_all
  clear_all() unpublishes every slot at once.
  Tests the following:
  - with both slots protecting, clear_all() drops both
  - the guard is still valid: clear_all keeps the group, so it can protect again
*/
bool
tests_scoped_multi_clear_all()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(2);
    int                    first_object  = 0;
    int                    second_object = 0;

    scoped_multi_hazard<2> guard(domain);

    guard.protect(0, &first_object);
    guard.protect(1, &second_object);

    guard.clear_all();

    const bool both_cleared =
        ( (!domain.is_protected(&first_object)) &&
          (!domain.is_protected(&second_object)) );

    // still usable
    guard.protect(0, &first_object);

    const bool reusable = domain.is_protected(&first_object);

    return (both_cleared && guard.valid() && reusable);
#else
    return true;
#endif
}

/*
tests_scoped_multi_is_not_copyable
  it owns a group and cannot be copied.
  Tests the following:
  - not copy-constructible, not copy-assignable
*/
bool
tests_scoped_multi_is_not_copyable()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<scoped_multi_hazard<2> >::value &&
          !std::is_copy_assignable<scoped_multi_hazard<2> >::value );

    static_assert(no_copy, "scoped_multi_hazard must not be copyable");

    return no_copy;
#else
    return true;
#endif
}

/*
tests_scoped_multi_out_of_range_slot_is_a_no_op
  a slot index past the group is ignored, not written.
  Tests the following:
  - protect(slot, ptr) with slot >= _SlotsPerThread does nothing: the pointer is
    not published anywhere
  - clear(slot) with an out-of-range slot likewise does nothing and does not
    crash
  The guard checks `_slot < _SlotsPerThread` before touching the group, so an
  index error is contained rather than corrupting the next group's memory.
*/
bool
tests_scoped_multi_out_of_range_slot_is_a_no_op()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(2);
    int                    object = 0;

    scoped_multi_hazard<2> guard(domain);

    guard.protect(2, &object);    // slot 2 in a 2-slot group: out of range
    guard.protect(99, &object);   // wildly out of range

    const bool not_published = (!domain.is_protected(&object));

    guard.clear(2);               // out-of-range clear: harmless

    return not_published;
#else
    return true;
#endif
}

/*
tests_scoped_multi_survives_exhaustion
  a guard built on an empty group pool holds nothing - and says so.
  Tests the following:
  - a capacity-1 domain's one group is taken; a second guard is INVALID
  - the invalid guard's protect(), clear() and clear_all() are safe no-ops that
    publish nothing
  - destroying it (it releases nothing) does not disturb the real guard
*/
bool
tests_scoped_multi_survives_exhaustion()
{
#if D_HAS_CUSTOM_HAZARD
    multi_hazard_domain<2> domain(1);
    int                    object = 0;

    scoped_multi_hazard<2> holder(domain);    // takes the only group

    {
        scoped_multi_hazard<2> invalid(domain);   // gets nothing

        if (invalid.valid())
        {
            return false;
        }

        invalid.protect(0, &object);   // no-ops, all
        invalid.clear(0);
        invalid.clear_all();

        if (domain.is_protected(&object))
        {
            return false;
        }
    }   // destroying the invalid guard must be harmless

    // the real guard still works
    holder.protect(0, &object);

    return domain.is_protected(&object);
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
