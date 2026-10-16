/*******************************************************************************
* djinterp [core]                            hazard_pointer_tests_domain_rec.cpp
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests_domain_rec.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                  hazard_pointer_tests_domain_rec.cpp
//   Section III (a): hazard_domain - claiming and returning records.
//
//   The domain owns a fixed array of records, allocated once at construction,
// so
// that the fast path never allocates.  acquire() walks the array and claims the
// first idle slot with a CAS on its active flag; when every slot is taken it
// returns nullptr rather than growing.  release() puts a slot back and clears
// whatever it protected.  This section pins that hand-off - distinct records to
// distinct callers, exhaustion at capacity, reuse after release - plus the
// self-descriptive aliases (the strategy tag and the self-marker) that the
// trait
// layer reads.
//
//   Gated on D_HAS_CUSTOM_HAZARD throughout.

// djinterp
#include "hazard_pointer_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_domain_reports_its_capacity
  the domain remembers how many records it was built with.
  Tests the following:
  - capacity() returns the constructor argument
  - the default constructor uses 64 - the documented default thread count
  - capacity() is const and noexcept
*/
bool
tests_domain_reports_its_capacity()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain small(4);
    hazard_domain defaulted;

    D_CONSTEXPR bool is_noexcept =
        noexcept(std::declval<const hazard_domain&>().capacity());

    static_assert(is_noexcept, "capacity() must be noexcept");

    return ( (small.capacity() == 4)      &&
             (defaulted.capacity() == 64) &&
             is_noexcept );
#else
    return true;
#endif
}

/*
tests_domain_is_not_copyable
  a domain cannot be copied.
  Tests the following:
  - the copy constructor and copy assignment are deleted
  - and they would have to be: the domain owns a heap array of atomics that many
    threads are reading, and a copy would be a second array nobody scans
*/
bool
tests_domain_is_not_copyable()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<hazard_domain>::value &&
          !std::is_copy_assignable<hazard_domain>::value );

    static_assert(no_copy, "hazard_domain must not be copyable");

    return no_copy;
#else
    return true;
#endif
}

/*
tests_domain_declares_the_hazard_strategy_tag
  the seam between this header and concurrency_strategy_tags.hpp.
  Tests the following:
  - hazard_domain::concurrency_strategy_tag IS hazard_strategy_tag - not rcu,
    not locked, not a tag of its own
  - the alias is PUBLIC, which it must be: the trait layer's fast path reads it
    by name
  This is what classifies a container built on hazard_domain as
  hazard-pointer-strategy, without the trait layer knowing anything else about
  it.
*/
bool
tests_domain_declares_the_hazard_strategy_tag()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool tagged_hazard =
        std::is_same<hazard_domain::concurrency_strategy_tag,
                     hazard_strategy_tag>::value;

    D_CONSTEXPR bool not_another_tag =
        ( !std::is_same<hazard_domain::concurrency_strategy_tag,
                        rcu_strategy_tag>::value &&
          !std::is_same<hazard_domain::concurrency_strategy_tag,
                        locked_strategy_tag>::value );

    static_assert(tagged_hazard,
                  "hazard_domain must declare the hazard strategy tag");

    return (tagged_hazard && not_another_tag);
#else
    return true;
#endif
}

/*
tests_domain_marks_itself_with_hazard_domain_type
  the self-marker a trait probes for.
  Tests the following:
  - hazard_domain::hazard_domain_type IS hazard_domain - the type names itself
  - so has_hazard_domain_type<X> answers true for the domain, and for any
    container that forwards the alias, without either naming the other
*/
bool
tests_domain_marks_itself_with_hazard_domain_type()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool names_itself =
        std::is_same<hazard_domain::hazard_domain_type, hazard_domain>::value;

    static_assert(names_itself,
                  "hazard_domain's self-marker must name hazard_domain");

    return names_itself;
#else
    return true;
#endif
}

/*
tests_acquire_hands_out_a_record
  acquire() gives a caller a slot to protect with.
  Tests the following:
  - a fresh domain hands back a non-null record
  - the record starts idle: null pointer, and now claimed (active)
*/
bool
tests_acquire_hands_out_a_record()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(4);
    hazard_record* record = domain.acquire();

    const bool handed_out =
        ( (record != nullptr) &&
          (record->protected_ptr.load() == nullptr) &&
          (record->active.load() == true) );

    if (record)
    {
        domain.release(record);
    }

    return handed_out;
#else
    return true;
#endif
}

/*
tests_acquire_hands_out_distinct_records
  two callers get two different slots.
  Tests the following:
  - three successive acquires return three distinct, non-null records
  - so no two readers ever share a slot, which would let one clobber the other's
    protection
*/
bool
tests_acquire_hands_out_distinct_records()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(4);
    hazard_record* first  = domain.acquire();
    hazard_record* second = domain.acquire();
    hazard_record* third  = domain.acquire();

    const bool all_distinct =
        ( (first != nullptr)  &&
          (second != nullptr) &&
          (third != nullptr)  &&
          (first != second)   &&
          (first != third)    &&
          (second != third) );

    domain.release(first);
    domain.release(second);
    domain.release(third);

    return all_distinct;
#else
    return true;
#endif
}

/*
tests_acquire_fills_the_pool_then_returns_null
  the pool is fixed; acquire() does not grow it.
  Tests the following:
  - a capacity-3 domain hands out exactly three records
  - the fourth acquire returns nullptr - silently, not by throwing or blocking
  - after a release, a slot is available again and acquire succeeds
  This is the exhaustion contract every guard type is built to survive: acquire
  reports failure by returning null, and it is the caller's job to notice.
*/
bool
tests_acquire_fills_the_pool_then_returns_null()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(3);
    hazard_record* a = domain.acquire();
    hazard_record* b = domain.acquire();
    hazard_record* c = domain.acquire();
    hazard_record* overflow = domain.acquire();     // the 4th: must be null

    const bool filled_then_failed =
        ( (a != nullptr) &&
          (b != nullptr) &&
          (c != nullptr) &&
          (overflow == nullptr) );

    // release one, and the pool has room again
    domain.release(b);

    hazard_record* reused = domain.acquire();

    const bool reuse_works = (reused != nullptr);

    domain.release(a);
    domain.release(c);
    domain.release(reused);

    return (filled_then_failed && reuse_works);
#else
    return true;
#endif
}

/*
tests_release_returns_a_record_to_the_pool
  a released record can be claimed again.
  Tests the following:
  - after exhausting a capacity-1 domain and releasing its one record, acquire
    succeeds again
  - so the pool is genuinely reusable, not one-shot
*/
bool
tests_release_returns_a_record_to_the_pool()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(1);
    hazard_record* first = domain.acquire();

    const bool empty_now = (domain.acquire() == nullptr);

    domain.release(first);

    hazard_record* again = domain.acquire();

    const bool available_again = (again != nullptr);

    if (again)
    {
        domain.release(again);
    }

    return (empty_now && available_again);
#else
    return true;
#endif
}

/*
tests_release_clears_the_protected_pointer
  releasing a record also stops it protecting anything.
  Tests the following:
  - a record protecting a pointer, once released, no longer protects it
  - release() stores null into protected_ptr AND clears active, so the slot is
    fully idle for the next claimant - a stale protection left behind would pin
    a
    node forever
*/
bool
tests_release_clears_the_protected_pointer()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(2);
    int            object = 0;
    hazard_record* record = domain.acquire();

    record->protected_ptr.store(&object);

    const bool protected_before = domain.is_protected(&object);

    domain.release(record);

    const bool cleared_after = (!domain.is_protected(&object));

    return (protected_before && cleared_after);
#else
    return true;
#endif
}

/*
tests_release_of_null_is_a_no_op
  releasing a null record is harmless.
  Tests the following:
  - release(nullptr) does nothing and does not crash - the guard types call it
    on an unacquired record, so it must tolerate null
  - the domain is unaffected: a subsequent acquire still works
*/
bool
tests_release_of_null_is_a_no_op()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(2);

    domain.release(nullptr);
    domain.release(nullptr);

    hazard_record* record = domain.acquire();

    const bool still_works = (record != nullptr);

    if (record)
    {
        domain.release(record);
    }

    return still_works;
#else
    return true;
#endif
}

/*
tests_a_reacquired_record_is_clean
  a slot handed out again carries no stale protection from its last owner.
  Tests the following:
  - a capacity-1 domain forces the SAME record to be reused: protect a pointer
    through it, release it, then acquire again - and the re-acquired record must
    NOT still protect the old pointer
  - is_protected(old) is false, and the record's own protected_ptr reads null
  This matters because acquire() only flips the active flag - it never resets
  protected_ptr.  The ONLY thing that keeps a reused slot clean is release()
  clearing the pointer.  If it did not, the next owner would inherit the
  previous
  owner's published protection, and a scan would spare a node nobody is actually
  guarding - a silent, latent pin.  This test is what stands between the header
  and that regression.
*/
bool
tests_a_reacquired_record_is_clean()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain  domain(1);        // one slot: a reacquire reuses it
    int            object = 0;

    hazard_record* first = domain.acquire();

    first->protected_ptr.store(&object);

    domain.release(first);

    hazard_record* reused = domain.acquire();   // the very same record

    const bool clean =
        ( (reused == first)                              &&   // really reused
          (reused->protected_ptr.load() == nullptr)      &&   // pointer reset
          (!domain.is_protected(&object)) );                 // nothing pinned

    if (reused)
    {
        domain.release(reused);
    }

    return clean;
#else
    return true;
#endif
}

/*
tests_a_zero_capacity_domain_acquires_nothing
  a domain with no records hands out nothing.
  Tests the following:
  - capacity() is 0
  - acquire() returns nullptr immediately (the scan loop runs zero times)
  - is_protected of any pointer is false, and collect gathers nothing
  A zero-capacity domain is a degenerate but legal construction - `new
  hazard_record[0]` is well-defined - and every query has to survive it.
*/
bool
tests_a_zero_capacity_domain_acquires_nothing()
{
#if D_HAS_CUSTOM_HAZARD
    hazard_domain domain(0);

    int object = 0;

    std::vector<void*> gathered;
    domain.collect_protected(gathered);

    return ( (domain.capacity() == 0)          &&
             (domain.acquire() == nullptr)     &&
             (!domain.is_protected(&object))   &&
             gathered.empty() );
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
