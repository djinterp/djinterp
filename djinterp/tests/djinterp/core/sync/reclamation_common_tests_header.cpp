/*******************************************************************************
* djinterp [core]                            reclamation_common_tests_header.cpp
*
*
* path:      /tests/djinterp/core/sync/reclamation_common_tests_header.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]         reclamation_common_tests_header.cpp
//   Section: the header contract.  The claims about the FILE - the include
// guard, double inclusion, that it CAN stand first in a TU (self-sufficient,
// like sync_common.hpp and atomic.hpp), that it really pulls in sync_common.hpp
// (which it depends on for the ownership bases), the namespace, that its
// vocabulary is types rather than macros, and the C++11 gating.

// djinterp
#include "reclamation_common_tests.hpp"
// ... and the header under test a SECOND time, directly.  Redefinitions would
// collide here if the include guard did not hold; the successful build IS
// tests_header_is_idempotent's assertion.
#include "../../../../inc/djinterp/core/sync/reclamation_common.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_header_include_guard
  the header's include guard is present, and carries the value the file gives
  it.
  Tests the following:
  - DJINTERP_THREADSAFE_RECLAMATION_ is defined after inclusion
  - it is defined to 1
*/
bool
tests_header_include_guard()
{
#if defined(DJINTERP_THREADSAFE_RECLAMATION_)
    const bool is_defined = true;
    const bool is_one     = (DJINTERP_THREADSAFE_RECLAMATION_ == 1);
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
  - the second #include above redefined nothing
  - after both inclusions the templates are still single, whole templates
*/
bool
tests_header_is_idempotent()
{
    const bool intact =
        ( (sizeof(reclaim_list<int>) > 0) &&
          (sizeof(published_ptr<int>) > 0) );

    return intact;
}

/*
tests_header_can_stand_first
  reclamation_common.hpp does NOT need another djinterp header ahead of it.
  Tests the following:
  - DJINTERP_ENV_ENV_H is defined by the time the suite's own header has
    been processed, and the only djinterp include in the test header is
    reclamation_common.hpp itself - so the core arrived through its own
    `#include "../djinterp.hpp"`
  A foundation header that could not stand first would be awkward, since its
  dependents will often include it before anything else.
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
tests_header_pulls_in_sync_common
  the header brings its own dependency along.
  Tests the following:
  - sync_common.hpp's include guard is defined here, so `#include
    "./sync_common.hpp"` really fired
  - reclaim_list is non-copyable, which is the OBSERVABLE consequence: it
    inherits the noncopyable base that sync_common.hpp provides
  A consumer that includes only reclamation_common.hpp must not have to know
  about the layering underneath it.
*/
bool
tests_header_pulls_in_sync_common()
{
#if defined(DJINTERP_CORE_SYNC_COMMON_)
    const bool dependency_present = true;
#else
    const bool dependency_present = false;
#endif

    const bool base_took_effect =
        !std::is_copy_constructible<reclaim_list<int> >::value;

    return (dependency_present && base_took_effect);
}

/*
tests_entities_live_in_the_djinterp_namespace
  the reclamation types sit directly in djinterp.
  Tests the following:
  - reclaim_list<int> and published_ptr<int>, found by unqualified lookup from
    djinterp::testing, ARE ::djinterp's
  These are the names hazard_pointer.hpp and rcu.hpp will use once they rebase
  onto this engine.
*/
bool
tests_entities_live_in_the_djinterp_namespace()
{
    const bool public_surface =
        ( std::is_same<reclaim_list<int>,
                       ::djinterp::reclaim_list<int> >::value &&
          std::is_same<published_ptr<int>,
                       ::djinterp::published_ptr<int> >::value );

    static_assert(
        std::is_same<reclaim_list<int>,
                     ::djinterp::reclaim_list<int> >::value,
        "the reclamation types must live directly in djinterp");

    return public_surface;
}

/*
tests_header_defines_no_stray_macros
  the header publishes types, not macros.
  Tests the following:
  - neither reclaim_list nor published_ptr is a macro
  Unlike sync_common.hpp, which deliberately exports two in-class macros, this
  header has no reason to define any.
*/
bool
tests_header_defines_no_stray_macros()
{
#if ( defined(reclaim_list) || defined(published_ptr) )
    const bool names_are_types = false;
#else
    const bool names_are_types = true;
#endif

    return names_are_types;
}

/*
tests_header_is_gated_to_cpp11
  the engine appears only at C++11 and above.
  Tests the following:
  - at the suite's C++11 floor (and up) both templates are present and
    instantiable
  This matches the header's banner: it needs <atomic> and <functional>, so
  C++98/03 is out.  Since the suite floors at C++11 the gate is always
  satisfied here; the test records the correspondence.
*/
bool
tests_header_is_gated_to_cpp11()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    reclaim_list<int>  list;
    published_ptr<int> pointer;

    const bool usable =
        ( (list.pending() == 0) &&
          (pointer.load() == NULL) );

    return usable;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
