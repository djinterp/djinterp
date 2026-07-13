/******************************************************************************
* djinterp [test]                                atomic_tests_stamped_ptr.cpp
*
*   Section IV - atomic_stamped_ptr<T>.  The ABA-defeating packed pointer is
* the most edge-heavy type in the header, so this section is the largest:
*
*     - construction (default -> {nullptr, 0}; explicit; default stamp arg)
*     - store / load of pointer and stamp (default and explicit orders)
*     - the FULL 16-bit stamp range beside a live pointer (proves the 48/16
*       bit split does not corrupt either field)
*     - a null pointer carrying a non-zero stamp
*     - weak compare-exchange: the success path (retried against spurious
*       failure) and the failure path (which rewrites the expected pair)
*     - the ABA scenario proper: a CAS with the RIGHT pointer but a STALE
*       stamp must be rejected - the entire reason the type exists
*     - stamp wrap across the 16-bit boundary (0xFFFF -> 0x0000)
*     - the unpack sign-extension branch, exercised as a pure bit-pattern
*       round-trip (never dereferenced) for a canonical high-half address
*
*   PLATFORM NOTE: tests_stamped_sign_extension assumes the x86-64 48-bit
* canonical-address model the header is written against (upper 16 bits hold
* the stamp; bit 47 is sign-extended).  It forms pointer VALUES purely as bit
* patterns and never dereferences them, so it is well-defined on that model.
*
*
* path:      /tests/djinterp/core/sync/atomic_tests_stamped_ptr.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.13
******************************************************************************/

// std
#include <atomic>
#include <cstdint>
#include <cstdio>

// djinterp
#include "atomic_tests.hpp"


#define D_AT_CHECK(_cond)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            std::printf("      [check] FAILED: %s\n"                           \
                        "               at %s:%d\n",                           \
                        #_cond, __FILE__, __LINE__);                           \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
    while (false)


NS_DJINTERP
NS_TESTING


// -------------------------------------------------------------------------
//  construction
// -------------------------------------------------------------------------

bool
tests_stamped_default_ctor()
{
    atomic_stamped_ptr<int> sp;

    D_AT_CHECK(sp.load_ptr() == nullptr);
    D_AT_CHECK(sp.load_stamp() == 0);

    return true;
}


bool
tests_stamped_explicit_ctor()
{
    int obj = 0;

    atomic_stamped_ptr<int> sp(&obj, 7);
    D_AT_CHECK(sp.load_ptr() == &obj);
    D_AT_CHECK(sp.load_stamp() == 7);

    // the stamp argument defaults to 0
    atomic_stamped_ptr<int> sp2(&obj);
    D_AT_CHECK(sp2.load_ptr() == &obj);
    D_AT_CHECK(sp2.load_stamp() == 0);

    return true;
}


// -------------------------------------------------------------------------
//  store / load
// -------------------------------------------------------------------------

bool
tests_stamped_store_load()
{
    int a = 1;
    int b = 2;

    atomic_stamped_ptr<int> sp(&a, 1);

    sp.store(&b, 2);
    D_AT_CHECK(sp.load_ptr() == &b);
    D_AT_CHECK(sp.load_stamp() == 2);

    // explicit memory orders
    sp.store(&a, 3, std::memory_order_release);
    D_AT_CHECK(sp.load_ptr(std::memory_order_acquire) == &a);
    D_AT_CHECK(sp.load_stamp(std::memory_order_acquire) == 3);

    return true;
}


// -------------------------------------------------------------------------
//  stamp field width
// -------------------------------------------------------------------------

bool
tests_stamped_stamp_range()
{
    int obj = 0;

    // every representative 16-bit stamp must round-trip while the pointer
    // stays intact - the 48/16 packing must not bleed one field into the other
    const std::uint16_t stamps[] =
    {
        static_cast<std::uint16_t>(0x0000),
        static_cast<std::uint16_t>(0x0001),
        static_cast<std::uint16_t>(0x00FF),
        static_cast<std::uint16_t>(0x7FFF),
        static_cast<std::uint16_t>(0x8000),
        static_cast<std::uint16_t>(0xFFFF)
    };

    for (std::uint16_t st : stamps)
    {
        atomic_stamped_ptr<int> sp(&obj, st);
        D_AT_CHECK(sp.load_ptr() == &obj);
        D_AT_CHECK(sp.load_stamp() == st);
    }

    return true;
}


bool
tests_stamped_null_with_stamp()
{
    // a null pointer may still carry a stamp
    atomic_stamped_ptr<int> sp(nullptr, static_cast<std::uint16_t>(0xABCD));

    D_AT_CHECK(sp.load_ptr() == nullptr);
    D_AT_CHECK(sp.load_stamp() == static_cast<std::uint16_t>(0xABCD));

    return true;
}


// -------------------------------------------------------------------------
//  compare-exchange (weak only - the type offers no strong CAS)
// -------------------------------------------------------------------------

bool
tests_stamped_cas_weak_success()
{
    int a = 1;
    int b = 2;

    atomic_stamped_ptr<int> sp(&a, 1);

    int*          exp_p = &a;
    std::uint16_t exp_s = 1;
    bool          ok    = false;

    for (int i = 0; (i < 10000) && !ok; ++i)
    {
        ok = sp.compare_exchange_weak(exp_p, exp_s, &b, 2);
        // single-threaded: a spurious failure reloads (exp_p, exp_s) to the
        // still-current (&a, 1), so the retry can succeed
    }

    D_AT_CHECK(ok);
    D_AT_CHECK(sp.load_ptr() == &b);
    D_AT_CHECK(sp.load_stamp() == 2);

    return true;
}


bool
tests_stamped_cas_weak_failure_updates_expected()
{
    int a = 1;
    int b = 2;
    int c = 3;

    atomic_stamped_ptr<int> sp(&a, 5);

    // wrong expected pointer -> deterministic failure; the failure branch
    // rewrites the expected pair to the actual (pointer, stamp)
    int*          exp_p = &c;   // wrong
    std::uint16_t exp_s = 5;

    D_AT_CHECK(!sp.compare_exchange_weak(exp_p, exp_s, &b, 6));
    D_AT_CHECK(exp_p == &a);            // updated to actual pointer
    D_AT_CHECK(exp_s == 5);             // actual stamp
    D_AT_CHECK(sp.load_ptr() == &a);    // target unchanged
    D_AT_CHECK(sp.load_stamp() == 5);

    return true;
}


bool
tests_stamped_aba_defeat()
{
    int a = 1;
    int b = 2;

    // the pointer is &a with stamp 1
    atomic_stamped_ptr<int> sp(&a, 1);

    // A stale actor captured (&a, stamp 0) before the stamp advanced.  Its CAS
    // must FAIL even though the pointer is still &a - the stamp differs.  This
    // is exactly the ABA case the stamped pointer exists to reject.
    int*          exp_p = &a;
    std::uint16_t exp_s = 0;   // stale stamp

    D_AT_CHECK(!sp.compare_exchange_weak(exp_p, exp_s, &b, 2));
    D_AT_CHECK(exp_p == &a);            // reloaded actual pointer
    D_AT_CHECK(exp_s == 1);             // reloaded actual (current) stamp
    D_AT_CHECK(sp.load_ptr() == &a);    // untouched - the swap did not happen
    D_AT_CHECK(sp.load_stamp() == 1);

    return true;
}


bool
tests_stamped_stamp_wrap()
{
    int a = 0;

    atomic_stamped_ptr<int> sp(&a, static_cast<std::uint16_t>(0xFFFF));   // max

    // advance the stamp past its 16-bit range via CAS: 0xFFFF -> 0x0000
    int*                exp_p = &a;
    std::uint16_t       exp_s = static_cast<std::uint16_t>(0xFFFF);
    const std::uint16_t next  = static_cast<std::uint16_t>(exp_s + 1);   // wraps
    D_AT_CHECK(next == 0);

    bool ok = false;
    for (int i = 0; (i < 10000) && !ok; ++i)
    {
        ok = sp.compare_exchange_weak(exp_p, exp_s, &a, next);
    }

    D_AT_CHECK(ok);
    D_AT_CHECK(sp.load_stamp() == 0);   // wrapped to zero
    D_AT_CHECK(sp.load_ptr() == &a);

    return true;
}


// -------------------------------------------------------------------------
//  unpack sign-extension branch (bit-pattern round-trip; never dereferenced)
// -------------------------------------------------------------------------

bool
tests_stamped_sign_extension()
{
    // A canonical high-half address has bit 47 set and bits 48-63 all 1.
    // Packing masks off the top 16 bits (stamp room); unpacking must then
    // SIGN-EXTEND from bit 47 to reconstruct the canonical pointer.  We form
    // these as pure bit patterns and never dereference them, so this is
    // well-defined on the 48-bit canonical model the header targets.
    const std::uintptr_t bits =
        static_cast<std::uintptr_t>(0xFFFF876543210000ULL);

    int* synth = reinterpret_cast<int*>(bits);
    atomic_stamped_ptr<int> sp(synth, static_cast<std::uint16_t>(0xBEEF));

    D_AT_CHECK(reinterpret_cast<std::uintptr_t>(sp.load_ptr()) == bits);
    D_AT_CHECK(sp.load_stamp() == static_cast<std::uint16_t>(0xBEEF));

    // store/load another canonical high address round-trips identically
    const std::uintptr_t bits2 =
        static_cast<std::uintptr_t>(0xFFFFF00DCAFE0000ULL);

    sp.store(reinterpret_cast<int*>(bits2), static_cast<std::uint16_t>(0x1234));
    D_AT_CHECK(reinterpret_cast<std::uintptr_t>(sp.load_ptr()) == bits2);
    D_AT_CHECK(sp.load_stamp() == static_cast<std::uint16_t>(0x1234));

    return true;
}


NS_END  // testing
NS_END  // djinterp

#undef D_AT_CHECK
