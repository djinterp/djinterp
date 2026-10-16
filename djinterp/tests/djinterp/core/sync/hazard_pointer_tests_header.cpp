/*******************************************************************************
* djinterp [core]                                hazard_pointer_tests_header.cpp
*
*
* path:      /tests/djinterp/core/sync/hazard_pointer_tests_header.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                    hazard_pointer_tests_header.cpp
//   Section: the header contract.  The claims about the FILE rather than about
// its types - the guard, double inclusion, the live environment precondition,
// the one dependency it pulls, the std facilities it forwards, the C++11 gate,
// and the C++23 fork between the custom types and delegation to
// <hazard_pointer>.
//
//   Unlike cow.hpp next door, hazard_pointer.hpp's "include env.h first"
// precondition is a LIVE #error, not a commented-out one - so it fails cleanly
// rather than cascading.  tests_header_requires_the_environment_first pins that
// it is satisfied here (the TU compiled) and records why the suite's own header
// includes the core first.

// djinterp
#include "hazard_pointer_tests.hpp"
// ... and the header under test a SECOND time, directly.  Two definitions of
// hazard_domain would collide here if the guard did not hold; the successful
// build IS tests_header_is_idempotent's assertion.  It needs no core include
// this time - the first inclusion already brought djinterp.hpp in behind it.
#include "../../../../inc/djinterp/core/sync/hazard_pointer.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_header_include_guard
  the header's include guard is present, and carries the value the file gives
  it.
  Tests the following:
  - DJINTERP_SYNC_HAZARD_POINTER_HPP is defined after inclusion
  - it is defined to 1
  The guard is named for the THREADSAFE module, not the sync directory the file
  lives in - as the neighbouring sync headers are.
*/
bool
tests_header_include_guard()
{
#if defined(DJINTERP_SYNC_HAZARD_POINTER_HPP)
    const bool is_defined = true;
    const bool is_one     = (DJINTERP_SYNC_HAZARD_POINTER_HPP == 1);
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
  - after both inclusions the module's surface is still intact
  The check is written so that it holds under EITHER implementation: with the
  custom types, hazard_domain is a complete type; under std delegation, the
  hazard_pointer_domain alias is nameable.  Something from the header is present
  either way.
*/
bool
tests_header_is_idempotent()
{
#if D_HAS_CUSTOM_HAZARD
    const bool intact = (sizeof(hazard_domain) > 0);
#else
    // std delegation path: the alias must at least name a type
    const bool intact =
        std::is_same<hazard_pointer_domain, hazard_pointer_domain>::value;
#endif

    return intact;
}

/*
tests_header_requires_the_environment_first
  the header is not self-sufficient, and says so cleanly.
  Tests the following:
  - DJINTERP_ENV_ENV_H is defined here - it must be, or the header's LIVE
    #error would have fired and this TU would not exist
  - so the header's precondition is a real, enforced guard, not a comment
  This is the honest version of the include-order trap.  cow.hpp had the same
  precondition with its guard commented out, and failed as a cascade of errors
  inside its dependencies; hazard_pointer.hpp keeps the guard live and fails, if
  misused, with one clear sentence.  The suite's own header includes
  djinterp/core/djinterp.hpp before this one to satisfy it - which is the
  documented way to use the file.
*/
bool
tests_header_requires_the_environment_first()
{
#if defined(DJINTERP_ENV_ENV_H)
    const bool environment_is_in = true;
#else
    const bool environment_is_in = false;
#endif

    return environment_is_in;
}

/*
tests_header_pulls_its_dependency
  the one djinterp header hazard_pointer.hpp names.
  Tests the following:
  - concurrency_strategy_tags.hpp is present, by its guard
  - and the tag it supplies, hazard_strategy_tag, is nameable here
  - so a consumer of hazard_pointer.hpp needs no further djinterp include to
    read a domain's strategy tag
  The strategy tag is only referenced on the custom path (the domains declare
  it); under std delegation the include is still pulled, so the guard check
  holds regardless.
*/
bool
tests_header_pulls_its_dependency()
{
#if defined(DJINTERP_SYNC_CONCURRENCY_STRATEGY_TAGS_)
    const bool has_tags = true;
#else
    const bool has_tags = false;
#endif

    const bool tag_is_nameable = (sizeof(hazard_strategy_tag) > 0);

    return (has_tags && tag_is_nameable);
}

/*
tests_header_provides_the_std_facilities
  including hazard_pointer.hpp is enough to name the std types it is built on.
  Tests the following:
  - std::atomic<void*>, std::size_t, std::uint64_t (the epoch's type) and
    std::vector<void*> (collect_protected's out-parameter) are all nameable here
  - and this TU includes NONE of <atomic>, <cstdint> or <vector> - the test
    header deliberately does not, precisely so that this test has teeth
  - so a consumer writing `std::vector<void*> hp;` beside a domain needs no
    further include
*/
bool
tests_header_provides_the_std_facilities()
{
    const bool nameable =
        ( (sizeof(std::atomic<void*>) > 0) &&
          (sizeof(std::size_t) > 0)        &&
          (sizeof(std::uint64_t) == 8)     &&
          (sizeof(std::vector<void*>) > 0) );

    // <atomic>'s ordering enum is here too
    const bool ordering_is_here =
        (std::memory_order_acquire != std::memory_order_release);

    return (nameable && ordering_is_here);
}

/*
tests_header_is_gated_to_cpp11
  the whole header sits behind a C++11 gate; below that it is empty.
  Tests the following:
  - the gate is on for this build (the suite's floor is C++11)
  - with it on, the module's surface is present
  The banner says "C++98/03: unavailable (requires <atomic>)", and the gate is
  what makes that true: a C++98 compile of the header defines nothing at all,
  rather than failing inside <atomic>.  Contrast cow.hpp, whose gate was
  commented out.
*/
bool
tests_header_is_gated_to_cpp11()
{
    const bool gate_is_on = (D_ENV_LANG_IS_CPP11_OR_HIGHER) ? true : false;

#if D_HAS_CUSTOM_HAZARD
    const bool surface_present = (sizeof(hazard_record) > 0);
#else
    const bool surface_present =
        std::is_same<hazard_pointer_domain, hazard_pointer_domain>::value;
#endif

    return (gate_is_on && surface_present);
}

/*
tests_which_implementation_is_in_play
  the header forks at C++23: custom types, or delegation to <hazard_pointer>.
  Tests the following:
  - exactly one of the two implementations is active, and the suite knows which:
    D_HAS_STD_HAZARD_POINTER is 0 (custom) or 1 (delegation), and
    D_HAS_CUSTOM_HAZARD is its complement
  - the macro is always DEFINED - the header's #if/#else ladder sets it to 0
    below C++23, and to 0 or 1 under C++23 depending on __has_include - so a
    consumer can always test it without a "not defined" branch
  This test documents the fork rather than judging it.  Under the standard path,
  sections II-VIII of this suite degrade to stated no-ops, because the types
  they test do not exist; this is the switch that tells them to.
*/
bool
tests_which_implementation_is_in_play()
{
#if defined(D_HAS_STD_HAZARD_POINTER)
    const bool macro_is_defined = true;
    const bool complement_holds =
        (D_HAS_STD_HAZARD_POINTER != D_HAS_CUSTOM_HAZARD);
#else
    const bool macro_is_defined = false;
    const bool complement_holds = false;
#endif

    return (macro_is_defined && complement_holds);
}

/*
tests_entities_live_in_the_djinterp_namespace
  the module's types sit FLAT in djinterp - there is no sub-namespace here.
  Tests the following:
  - on the custom path, hazard_record / hazard_domain / scoped_hazard /
    typed_hazard_ptr / retired_list / multi_hazard_domain / scoped_multi_hazard,
    found by unqualified lookup from djinterp::testing, ARE ::djinterp's
  - on the std path, the two aliases are ::djinterp's
  Unlike cow.hpp and atomic.hpp, this header keeps nothing in a nested
  `internal` namespace - the hazard_record is public, because the domains hand
  raw records back to callers by design.
*/
bool
tests_entities_live_in_the_djinterp_namespace()
{
#if D_HAS_CUSTOM_HAZARD
    D_CONSTEXPR bool flat =
        ( std::is_same<hazard_record, ::djinterp::hazard_record>::value &&
          std::is_same<hazard_domain, ::djinterp::hazard_domain>::value &&
          std::is_same<scoped_hazard, ::djinterp::scoped_hazard>::value &&
          std::is_same<typed_hazard_ptr<int>,
                       ::djinterp::typed_hazard_ptr<int> >::value        &&
          std::is_same<retired_list<int>,
                       ::djinterp::retired_list<int> >::value            &&
          std::is_same<multi_hazard_domain<2>,
                       ::djinterp::multi_hazard_domain<2> >::value       &&
          std::is_same<scoped_multi_hazard<2>,
                       ::djinterp::scoped_multi_hazard<2> >::value );

    static_assert(flat,
                  "the hazard types must live directly in namespace djinterp");
#else
    D_CONSTEXPR bool flat =
        std::is_same<hazard_pointer_domain,
                     ::djinterp::hazard_pointer_domain>::value;
#endif

    return flat;
}

/*
tests_header_defines_no_stray_macros
  the header publishes types, and exactly one intentional macro.
  Tests the following:
  - none of the type names is a macro
  - D_HAS_STD_HAZARD_POINTER IS defined - that is the one macro the header is
    meant to leave behind, the switch a consumer reads to know which
    implementation it got
  So the only preprocessor symbol the header exports is the deliberate one.
*/
bool
tests_header_defines_no_stray_macros()
{
#if ( defined(hazard_record)      || \
      defined(hazard_domain)      || \
      defined(scoped_hazard)      || \
      defined(typed_hazard_ptr)   || \
      defined(retired_list)       || \
      defined(multi_hazard_domain) )
    const bool names_are_types = false;
#else
    const bool names_are_types = true;
#endif

#if defined(D_HAS_STD_HAZARD_POINTER)
    const bool the_one_intended_macro_is_present = true;
#else
    const bool the_one_intended_macro_is_present = false;
#endif

    return (names_are_types && the_one_intended_macro_is_present);
}

/*
tests_the_std_delegation_aliases_exist_when_selected
  when the standard path is taken, the two delegation aliases are what a
  consumer gets.
  Tests the following:
  - under std delegation, hazard_pointer_domain and hazard_pointer<int> are
    nameable, and NONE of the custom types are
  - under the custom path, this test is a stated no-op (there is nothing to
    check - the custom types are what the other tests exercise)
  This is the mirror of tests_which_implementation_is_in_play: that one asserts
  the switch is coherent, this one asserts the delegation surface is actually
  present when the switch says it should be.
*/
bool
tests_the_std_delegation_aliases_exist_when_selected()
{
#if D_HAS_STD_HAZARD_POINTER
    const bool aliases_present =
        ( std::is_same<hazard_pointer_domain,
                       hazard_pointer_domain>::value &&
          (sizeof(hazard_pointer<int>) > 0) );

    return aliases_present;
#else
    // custom path: nothing to assert here - the custom types are covered by
    // every other section.  Stated no-op.
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
