/*******************************************************************************
* djinterp [core]                                 hazard_pointer_tests_typed.cpp
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests_typed.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                     hazard_pointer_tests_typed.cpp
//   Section V: typed_hazard_ptr - the protect-then-verify loop, packaged.
//
//   This is the type a reader actually uses.  It wraps a scoped_hazard and adds
// protect_and_validate(), which encapsulates the one dance every hazard-pointer
// reader must perform: load the source, publish what you loaded, then RE-READ
// the source and check it did not change underneath you.  If it changed, the
// pointer you published may already be retired, so you retry; after enough
// failures you give up, clear your protection, and return null.
//
//   Two edges of that loop are sharp enough to have their own tests, because a
// caller will misread both:
//     - a NULL source validates.  Loading null, publishing null and re-reading
//       null all agree, so the function returns null and reports success.  The
//       caller must read that null as "nothing there", not as failure.
//     - ZERO retries runs the loop body zero times, so it returns null WITHOUT
//       publishing anything and WITHOUT clearing a prior protection.  It is a
//       no-op that looks like a failure.
//
//   Gated on D_HAS_CUSTOM_HAZARD throughout.

// djinterp
#include "hazard_pointer_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_typed_acquires_a_record
  a typed_hazard_ptr claims a record like the scoped guard it wraps.
  Tests the following:
  - built on a domain with free records, it is valid()
  - it consumed one record
*/
bool
tests_typed_acquires_a_record()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(1);

    typed_hazard_ptr<int> hazard(domain);

    return ( hazard.valid() &&
             (domain.acquire() == nullptr) );
#else
    return true;
#endif
}

/*
tests_typed_is_not_copyable
  it owns a record (through its guard) and cannot be copied.
  Tests the following:
  - typed_hazard_ptr is not copy-constructible or copy-assignable
*/
bool
tests_typed_is_not_copyable()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<typed_hazard_ptr<int> >::value &&
          !std::is_copy_assignable<typed_hazard_ptr<int> >::value );

    static_assert(no_copy, "typed_hazard_ptr must not be copyable");

    return no_copy;
#else
    return true;
#endif
}

/*
tests_validate_on_a_stable_source_succeeds
  when the source does not change, validation succeeds on the first try.
  Tests the following:
  - protect_and_validate on a source holding &node returns &node
  - the guard is still valid
  This is the common case: the pointer was loaded, published, and re-read
  unchanged, so it is safe to dereference.
*/
bool
tests_validate_on_a_stable_source_succeeds()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain      domain(2);
    int                node = 7;
    std::atomic<int*>  source(&node);

    typed_hazard_ptr<int> hazard(domain);

    int* validated = hazard.protect_and_validate(source);

    return ( (validated == &node) &&
             hazard.valid() );
#else
    return true;
#endif
}

/*
tests_validate_publishes_the_validated_pointer
  a successful validation leaves the pointer PROTECTED.
  Tests the following:
  - after protect_and_validate returns &node, is_protected(&node) is true
  - so the whole point is achieved: the returned pointer is not merely returned,
    it is published, and a concurrent scan will now spare it
  This is the difference between protect_and_validate and a bare load: the
  pointer it hands back is one the domain is actively protecting.
*/
bool
tests_validate_publishes_the_validated_pointer()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain      domain(2);
    int                node = 7;
    std::atomic<int*>  source(&node);

    typed_hazard_ptr<int> hazard(domain);

    int* validated = hazard.protect_and_validate(source);

    return ( (validated == &node) &&
             domain.is_protected(&node) );
#else
    return true;
#endif
}

/*
tests_validate_on_a_null_source_returns_null
  a null source "validates" - and the caller must not be fooled.
  Tests the following:
  - protect_and_validate on a source holding nullptr returns nullptr
  - it returns because the protocol SUCCEEDED: it loaded null, published null,
    re-read null, and they matched on the first try
  - is_protected(nullptr) is false (null is never protected), so nothing is
    actually pinned
  The trap is that null here means "there was nothing to protect", NOT
  "validation failed".  A caller writing `if (hp.protect_and_validate(head))`
  correctly skips a null head - but a caller who thinks a null return always
  means "retry exhausted" is wrong.  Pinned so the distinction is on record.
*/
bool
tests_validate_on_a_null_source_returns_null()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain      domain(2);
    std::atomic<int*>  source(nullptr);

    typed_hazard_ptr<int> hazard(domain);

    int* result = hazard.protect_and_validate(source);

    return ( (result == nullptr)              &&
             (!domain.is_protected(nullptr))  &&
             hazard.valid() );
#else
    return true;
#endif
}

/*
tests_validate_with_zero_retries_does_nothing
  zero retries is a no-op that looks like a failure.
  Tests the following:
  - protect_and_validate(source, 0) returns nullptr - the retry loop runs zero
    times, so it never loads, never publishes, and reaches the give-up path
  - it publishes NOTHING: a pointer that was NOT previously protected stays
    unprotected
  - and it does clear() on the way out, so any PRIOR protection this guard held
    is dropped
  A caller who passes 0 (perhaps computed from a variable) gets no
  protection and
  no error return distinguishable from a real validation failure.  This pins the
  degenerate bound.
*/
bool
tests_validate_with_zero_retries_does_nothing()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain      domain(2);
    int                node = 7;
    std::atomic<int*>  source(&node);

    typed_hazard_ptr<int> hazard(domain);

    int* result = hazard.protect_and_validate(source, 0);

    return ( (result == nullptr) &&
             (!domain.is_protected(&node)) );
#else
    return true;
#endif
}

/*
tests_validate_clears_protection_when_it_gives_up
  giving up leaves nothing published.
  Tests the following:
  - a guard first protects &first_node directly, so it IS publishing something
  - then protect_and_validate(source, 0) is called - which reaches the give-up
    path and calls clear()
  - afterwards, the earlier protection of &first_node is GONE: is_protected is
    false
  So a failed validation does not leave a stale pointer pinned.  The header's
  clear() on the give-up path is what guarantees it, and zero retries is the
  cleanest way to force that path deterministically.
*/
bool
tests_validate_clears_protection_when_it_gives_up()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain      domain(2);
    int                first_node  = 1;
    int                second_node = 2;
    std::atomic<int*>  source(&second_node);

    typed_hazard_ptr<int> hazard(domain);

    // publish something first
    hazard.protect(&first_node);

    const bool published_first = domain.is_protected(&first_node);

    // force the give-up path; it must clear the earlier protection
    int* result = hazard.protect_and_validate(source, 0);

    const bool first_now_cleared = (!domain.is_protected(&first_node));

    return ( published_first    &&
             (result == nullptr) &&
             first_now_cleared );
#else
    return true;
#endif
}

/*
tests_typed_protect_publishes_directly
  protect() is the manual path, for a pointer the caller already trusts.
  Tests the following:
  - typed_hazard_ptr::protect(&node) publishes it: is_protected is true
  - it takes a typed T*, and erases it to void* internally - so the caller works
    in the node's own type, not void*
*/
bool
tests_typed_protect_publishes_directly()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(2);
    int           node = 5;

    typed_hazard_ptr<int> hazard(domain);

    hazard.protect(&node);

    return domain.is_protected(&node);
#else
    return true;
#endif
}

/*
tests_typed_clear_unpublishes
  clear() drops the protection but keeps the record.
  Tests the following:
  - after protect then clear, is_protected is false
  - the guard is still valid and can protect again
*/
bool
tests_typed_clear_unpublishes()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(2);
    int           node = 5;

    typed_hazard_ptr<int> hazard(domain);

    hazard.protect(&node);

    const bool protected_before = domain.is_protected(&node);

    hazard.clear();

    const bool cleared_after = (!domain.is_protected(&node));

    return ( protected_before &&
             cleared_after    &&
             hazard.valid() );
#else
    return true;
#endif
}

/*
tests_typed_on_an_exhausted_domain_is_invalid
  a typed pointer with no record behaves like an invalid guard.
  Tests the following:
  - on an exhausted domain, a typed_hazard_ptr is not valid()
  - protect_and_validate on it still returns - it publishes into a no-op guard,
    re-reads, and hands back the pointer, but nothing is actually protected
  - protect() and clear() on it are safe no-ops
  So the typed layer inherits scoped_hazard's exhaustion behaviour: no record,
  no protection, no crash.
*/
bool
tests_typed_on_an_exhausted_domain_is_invalid()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(1);

    typed_hazard_ptr<int> holder(domain);    // takes the only record
    typed_hazard_ptr<int> starved(domain);   // gets none

    int               node = 3;
    std::atomic<int*> source(&node);

    // the guard is invalid, so nothing it does protects anything
    int* result = starved.protect_and_validate(source);

    starved.protect(&node);
    starved.clear();

    return ( (!starved.valid())               &&
             (!domain.is_protected(&node))    &&
             (result == &node) );   // it still returns the re-read value
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
