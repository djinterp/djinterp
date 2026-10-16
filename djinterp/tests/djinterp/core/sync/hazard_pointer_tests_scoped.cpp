/*******************************************************************************
* djinterp [core]                                hazard_pointer_tests_scoped.cpp
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests_scoped.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                    hazard_pointer_tests_scoped.cpp
//   Section IV: scoped_hazard - RAII around a record.
//
//   scoped_hazard turns the acquire/release pair into a lifetime: it claims a
// record when it is built and returns it when it dies, so a reader cannot leak
// a
// slot by forgetting to release.  Its protect() and clear() forward to the
// record.  The one thing a caller MUST still do is check valid() - because if
// the pool was empty, the guard holds no record, and its protect() is a silent
// no-op.  The suite pins both the happy path (acquire, publish, release on
// scope exit) and that exhaustion path (invalid guard, protect does nothing,
// but nothing crashes).
//
//   Gated on D_HAS_CUSTOM_HAZARD throughout.

// djinterp
#include "hazard_pointer_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_scoped_acquires_on_construction
  a scoped_hazard claims a record as soon as it exists.
  Tests the following:
  - a guard built on a domain with free records is valid()
  - it consumed exactly one record: a capacity-1 domain has none left afterwards
*/
bool
tests_scoped_acquires_on_construction()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(1);

    scoped_hazard guard(domain);

    const bool valid_and_took_one =
        ( guard.valid() &&
          (domain.acquire() == nullptr) );   // the one record is taken

    return valid_and_took_one;
#else
    return true;
#endif
}

/*
tests_scoped_releases_on_destruction
  the record goes back when the guard leaves scope.
  Tests the following:
  - a capacity-1 domain, fully consumed by a guard, has its record available
    again once the guard is destroyed
  - so the slot is returned automatically, with no explicit release call
*/
bool
tests_scoped_releases_on_destruction()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(1);

    {
        scoped_hazard guard(domain);

        // check if the guard failed to take the only record
        if (domain.acquire() != nullptr)
        {
            return false;
        }
    }

    // the guard is gone; the record must be back
    hazard_record* reclaimed = domain.acquire();

    const bool released = (reclaimed != nullptr);

    if (reclaimed)
    {
        domain.release(reclaimed);
    }

    return released;
#else
    return true;
#endif
}

/*
tests_scoped_protect_publishes
  protect() through the guard makes a pointer visible to a scan.
  Tests the following:
  - after guard.protect(&object), is_protected(&object) is true
  - so the guard is a working front-end to the record's publish
*/
bool
tests_scoped_protect_publishes()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(2);
    int           object = 0;

    scoped_hazard guard(domain);

    guard.protect(&object);

    return domain.is_protected(&object);
#else
    return true;
#endif
}

/*
tests_scoped_clear_unpublishes
  clear() takes the protection back without releasing the record.
  Tests the following:
  - after protect then clear, is_protected is false
  - the guard is still valid: clear unpublishes the pointer but keeps the
    record,
    so the same guard can protect something else next
*/
bool
tests_scoped_clear_unpublishes()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(2);
    int           object = 0;

    scoped_hazard guard(domain);

    guard.protect(&object);

    const bool protected_before = domain.is_protected(&object);

    guard.clear();

    const bool cleared_after = (!domain.is_protected(&object));

    // still usable: protect a second time
    guard.protect(&object);

    const bool reusable = domain.is_protected(&object);

    return ( protected_before &&
             cleared_after    &&
             guard.valid()    &&
             reusable );
#else
    return true;
#endif
}

/*
tests_scoped_is_not_copyable
  a guard owns a record and cannot be copied.
  Tests the following:
  - scoped_hazard is not copy-constructible or copy-assignable
  - which is right: two guards over one record would both release it, returning
    the same slot to the pool twice
*/
bool
tests_scoped_is_not_copyable()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<scoped_hazard>::value &&
          !std::is_copy_assignable<scoped_hazard>::value );

    static_assert(no_copy, "scoped_hazard must not be copyable");

    return no_copy;
#else
    return true;
#endif
}

/*
tests_scoped_hazard_survives_exhaustion
  a guard built on an empty pool holds no record - and says so.
  Tests the following:
  - two guards exhaust a capacity-2 domain; a third is INVALID
  - the invalid guard did not somehow take a record: valid() is false
  - and destroying it (it releases nothing) does not disturb the two real guards
  This is the exhaustion contract at the guard level: acquire returned null, and
  the guard turned that into a checkable valid()==false rather than a crash.
*/
bool
tests_scoped_hazard_survives_exhaustion()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(2);

    scoped_hazard first(domain);
    scoped_hazard second(domain);

    const bool reals_are_valid =
        ( first.valid() && second.valid() );

    {
        scoped_hazard overflow(domain);   // the pool is empty

        const bool overflow_is_invalid = (!overflow.valid());

        if (!overflow_is_invalid)
        {
            return false;
        }
    }   // destroying the invalid guard must be harmless

    // the two real guards are unaffected: their records are still theirs
    int object = 0;
    first.protect(&object);

    const bool still_working = domain.is_protected(&object);

    return (reals_are_valid && still_working);
#else
    return true;
#endif
}

/*
tests_scoped_protect_while_invalid_is_a_no_op
  an invalid guard's protect() does nothing, safely.
  Tests the following:
  - a guard that holds no record (pool was empty) can have protect() and clear()
    called on it without crashing
  - and neither publishes anything: is_protected stays false
  A caller that forgets to check valid() gets no protection - which is a bug in
  the caller, but not a crash in the guard.  This pins that the guard degrades
  quietly rather than dereferencing its null record.
*/
bool
tests_scoped_protect_while_invalid_is_a_no_op()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(1);

    scoped_hazard holder(domain);         // takes the only record
    scoped_hazard invalid(domain);        // gets nothing

    int object = 0;

    invalid.protect(&object);             // must be a safe no-op
    invalid.clear();                      // likewise

    return ( (!invalid.valid()) &&
             (!domain.is_protected(&object)) );
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
