/*******************************************************************************
* djinterp [core]                                   sync_common_tests_header.cpp
*
*
* path:      /tests/djinterp/core/sync/sync_common_tests_header.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                sync_common_tests_header.cpp
//   Section: the header contract.  The claims about the FILE - the include
// guard, double inclusion, that it CAN stand first in a TU (it includes the
// core itself, the self-sufficient variant), the namespace its entities live
// in, that its type vocabulary is types rather than macros while its two
// ownership macros ARE deliberately macros, and the C++11 gating.

// djinterp
#include "sync_common_tests.hpp"
// ... and the header under test a SECOND time, directly.  Redefinitions would
// collide here if the include guard did not hold; the successful build IS
// tests_header_is_idempotent's assertion.
#include "../../../../inc/djinterp/core/sync/sync_common.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_header_include_guard
  the header's include guard is present, and carries the value the file gives
  it.
  Tests the following:
  - DJINTERP_CORE_SYNC_COMMON_ is defined after inclusion
  - it is defined to 1
*/
bool
tests_header_include_guard()
{
#if defined(DJINTERP_CORE_SYNC_COMMON_)
    const bool is_defined = true;
    const bool is_one     = (DJINTERP_CORE_SYNC_COMMON_ == 1);
#else
    const bool is_defined = false;
    const bool is_one     = false;
#endif

    return (is_defined && is_one);
}

/*
tests_header_is_idempotent
  including the header twice in one TU is a no-op.
  Tests the following:
  - the second #include above redefined nothing - neither the classes nor the
    two function-like macros, which would warn or error on redefinition
  - after both inclusions the types are still single, whole types
*/
bool
tests_header_is_idempotent()
{
    const bool intact =
        ( (sizeof(backoff) > 0) &&
          (sizeof(derived_noncopyable) > 0) );

    return intact;
}

/*
tests_header_can_stand_first
  sync_common.hpp does NOT need another djinterp header ahead of it.
  Tests the following:
  - DJINTERP_ENV_ENV_H is defined by the time the suite's own header has
    been processed, and the ONLY djinterp include in sync_common_tests.hpp is
    sync_common.hpp itself - so the core arrived through sync_common.hpp's own
    `#include "../djinterp.hpp"`
  This is the self-sufficient variant (like lock_guard.hpp and atomic.hpp),
  not the live-#error variant (like rcu.hpp and lock_policy.hpp) - a common
  header that could not stand first would be a poor foundation, since it is
  the first thing its dependents include.
*/
bool
tests_header_can_stand_first()
{
#if defined(DJINTERP_ENV_ENV_H)
    return true;
#else
    return false;
#endif
}

/*
tests_entities_live_in_the_djinterp_namespace
  the utilities sit directly in djinterp.
  Tests the following:
  - noncopyable, nonmovable and backoff, found by unqualified lookup from
    djinterp::testing, ARE ::djinterp's
  These are the names dependent modules will inherit from and hold by value.
*/
bool
tests_entities_live_in_the_djinterp_namespace()
{
    const bool public_surface =
        ( std::is_same<noncopyable, ::djinterp::noncopyable>::value &&
          std::is_same<nonmovable, ::djinterp::nonmovable>::value   &&
          std::is_same<backoff, ::djinterp::backoff>::value );

    static_assert(
        std::is_same<noncopyable, ::djinterp::noncopyable>::value,
        "the sync_common utilities must live directly in djinterp");

    return public_surface;
}

/*
tests_header_defines_no_stray_type_macros
  the header's TYPE vocabulary is types, not macros.
  Tests the following:
  - none of noncopyable / nonmovable / backoff / scope_guard / cpu_relax is a
    macro
  A macro here would leak into every dependent TU and collide with member or
  variable names; the ownership macros are the deliberate exception, checked
  separately below.
*/
bool
tests_header_defines_no_stray_type_macros()
{
#if ( defined(noncopyable)  || \
      defined(nonmovable)   || \
      defined(backoff)      || \
      defined(scope_guard)  || \
      defined(cpu_relax) )
    const bool names_are_types = false;
#else
    const bool names_are_types = true;
#endif

    return names_are_types;
}

/*
tests_header_publishes_its_ownership_macros
  the two ownership macros ARE macros, on purpose.
  Tests the following:
  - D_NONCOPYABLE and D_NONMOVABLE are both defined
  They must be macros rather than types because they expand to declarations
  inside a class body - something no base class or alias can do.  Their
  EFFECT is checked in the ownership section; here we only confirm the header
  publishes them.
*/
bool
tests_header_publishes_its_ownership_macros()
{
#if defined(D_NONCOPYABLE) && defined(D_NONMOVABLE)
    return true;
#else
    return false;
#endif
}

/*
tests_header_is_gated_to_cpp11
  the utilities appear only at C++11 and above.
  Tests the following:
  - at the suite's C++11 floor (and up) the types are present and complete
  This matches the header's own banner: it needs deleted / defaulted special
  members and <thread>, so C++98/03 is out.  Since the suite floors at C++11,
  the gate is always satisfied here; the test records the correspondence.
*/
bool
tests_header_is_gated_to_cpp11()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    const bool present =
        ( (sizeof(backoff) > 0)              &&
          (sizeof(derived_noncopyable) > 0)  &&
          (sizeof(derived_nonmovable) > 0) );

    return present;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
