/*******************************************************************************
* djinterp [core]                     concurrency_strategy_tags_tests_header.cpp
*
*
* path:      /tests/djinterp/core/sync/concurrency_strategy_tags_tests_header.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]              concurrency_strategy_tags_tests_header.cpp
//   Section: the header contract.  The claims the banner makes about the FILE
// rather than about its types - the include guard, double-inclusion, the
// single dependency (djinterp.hpp), the absence of a cycle back up to the
// trait layer, macro hygiene, namespace placement, and the C++11 gate that
// section II sits behind.
//
//   This TU deliberately includes NOTHING from djinterp but the test header
// (which includes only the header under test).  That is not incidental: every
// "no dependency" assertion below is only meaningful because the TU has no
// other way to have acquired the symbols it checks for.

// djinterp
#include "concurrency_strategy_tags_tests.hpp"
// ... and the header under test a SECOND time, directly.  If the include guard
// did not hold, the seven struct definitions and four enum definitions would
// collide here and this TU would not compile.  The successful build IS
// tests_header_is_idempotent's assertion; the function only re-checks the
// guard's arithmetic.
#include "../../../../inc/djinterp/core/sync/concurrency_strategy_tags.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_header_include_guard
  the header's include guard is present, and carries the value the file gives
  it.
  Tests the following:
  - DJINTERP_SYNC_CONCURRENCY_STRATEGY_TAGS_ is defined after inclusion
  - it is defined to 1 (the codebase's convention), not merely to nothing
*/
bool
tests_header_include_guard()
{
#if defined(DJINTERP_SYNC_CONCURRENCY_STRATEGY_TAGS_)
    const bool is_defined = true;
    const bool is_one     = (DJINTERP_SYNC_CONCURRENCY_STRATEGY_TAGS_ == 1);
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
  - the second #include above did not redefine the tags or the enums
  - after both inclusions the entities are still single, complete types
*/
bool
tests_header_is_idempotent()
{
    // one definition of each survived both inclusions
    const bool tags_intact =
        ( (sizeof(none_strategy_tag)   > 0) &&
          (sizeof(hybrid_strategy_tag) > 0) );

    const bool enums_intact =
        ( (sizeof(concurrency_progress)   > 0) &&
          (sizeof(reclamation_obligation) > 0) );

    return (tags_intact && enums_intact);
}

/*
tests_header_requires_cpp
  the header's `#error "requires C++ compilation"` premise holds here.
  Tests the following:
  - __cplusplus is defined (the header refuses to compile otherwise)
  - the detected standard is at least C++98, the floor section I claims
*/
bool
tests_header_requires_cpp()
{
#if defined(__cplusplus)
    const bool is_cpp = true;
#else
    const bool is_cpp = false;
#endif

    const bool at_least_cpp98 = (D_ENV_LANG_IS_CPP98_OR_HIGHER) ? true : false;

    return (is_cpp && at_least_cpp98);
}

/*
tests_header_pulls_only_the_core
  the header's ONE declared dependency, djinterp.hpp, is the one it took.
  Tests the following:
  - including the header alone defines the C++ core's guard
      (DJINTERP_DJINTERP_HPP),
    so the header self-supplies the floor its NS_ / D_ macros come from
  - the C core reached through it (DJINTERP_) is present too
  - the suite therefore never had to name djinterp.hpp itself
*/
bool
tests_header_pulls_only_the_core()
{
#if defined(DJINTERP_DJINTERP_HPP)
    const bool has_cpp_core = true;
#else
    const bool has_cpp_core = false;
#endif

#if defined(DJINTERP_)
    const bool has_c_core = true;
#else
    const bool has_c_core = false;
#endif

    return (has_cpp_core && has_c_core);
}

/*
tests_header_has_no_trait_layer_cycle
  the header does not reach back up to the layer that consumes it.
  Tests the following:
  - after including ONLY the tags header, no strategy-trait or
    threadsafe-container-trait guard is defined
  - the banner's whole reason for existing - "the lower mechanism layer can
    name a progress grade without a cycle back to the strategy layer" - is
    therefore intact for this TU
  Note: the guard spellings below are the plausible ones for
  concurrency_strategy_traits.hpp / threadsafe_container_traits.hpp.  A false
  PASS is possible if those headers spell their guards differently; a false
  FAIL is not.  Add the exact spelling here when the cycle is worth pinning
  down harder.
*/
bool
tests_header_has_no_trait_layer_cycle()
{
#if ( defined(DJINTERP_CONCURRENCY_STRATEGY_TRAITS_)           || \
      defined(DJINTERP_SYNC_CONCURRENCY_STRATEGY_TRAITS_)      || \
      defined(DJINTERP_CONTAINER_TRAITS_CONCURRENCY_STRATEGY_TRAITS_HPP) || \
      defined(DJINTERP_CONTAINER_TRAITS_THREADSAFE_CONTAINER_TRAITS_HPP) ||    \
      defined(DJINTERP_CONTAINER_THREADSAFE_CONTAINER_TRAITS_) )
    const bool acyclic = false;
#else
    const bool acyclic = true;
#endif

    return acyclic;
}

/*
tests_header_defines_no_stray_macros
  the header contributes types, never macros, under the names it publishes.
  Tests the following:
  - none of the seven tag names is a macro
  - none of the four enum names is a macro
  - so a consumer may declare, alias, or qualify any of them without the
    preprocessor rewriting it first
*/
bool
tests_header_defines_no_stray_macros()
{
#if ( defined(none_strategy_tag)   || \
      defined(locked_strategy_tag) || \
      defined(cow_strategy_tag)    || \
      defined(rcu_strategy_tag)    || \
      defined(atomic_strategy_tag) || \
      defined(hazard_strategy_tag) || \
      defined(hybrid_strategy_tag) )
    const bool tags_are_types = false;
#else
    const bool tags_are_types = true;
#endif

#if ( defined(concurrency_progress)   || \
      defined(concurrency_arity)      || \
      defined(iteration_consistency)  || \
      defined(reclamation_obligation) )
    const bool enums_are_types = false;
#else
    const bool enums_are_types = true;
#endif

    return (tags_are_types && enums_are_types);
}

/*
tests_entities_live_in_the_djinterp_namespace
  every entity sits FLAT in djinterp - not in a nested sync / container
  namespace.
  Tests the following:
  - each of the seven tags found by unqualified lookup from djinterp::testing
    is the same type as ::djinterp::<tag>
  - each of the four enums likewise
  - so the fully-qualified spelling a consumer writes is ::djinterp::<name>
*/
bool
tests_entities_live_in_the_djinterp_namespace()
{
    D_CONSTEXPR bool tags_are_flat =
        ( std::is_same<none_strategy_tag,
                       ::djinterp::none_strategy_tag>::value   &&
          std::is_same<locked_strategy_tag,
                       ::djinterp::locked_strategy_tag>::value &&
          std::is_same<cow_strategy_tag,
                       ::djinterp::cow_strategy_tag>::value    &&
          std::is_same<rcu_strategy_tag,
                       ::djinterp::rcu_strategy_tag>::value    &&
          std::is_same<atomic_strategy_tag,
                       ::djinterp::atomic_strategy_tag>::value &&
          std::is_same<hazard_strategy_tag,
                       ::djinterp::hazard_strategy_tag>::value &&
          std::is_same<hybrid_strategy_tag,
                       ::djinterp::hybrid_strategy_tag>::value );

    static_assert(tags_are_flat,
                  "the strategy tags must live directly in namespace djinterp");

    D_CONSTEXPR bool enums_are_flat =
        ( std::is_same<concurrency_progress,
                       ::djinterp::concurrency_progress>::value   &&
          std::is_same<concurrency_arity,
                       ::djinterp::concurrency_arity>::value      &&
          std::is_same<iteration_consistency,
                       ::djinterp::iteration_consistency>::value  &&
          std::is_same<reclamation_obligation,
                       ::djinterp::reclamation_obligation>::value );

    static_assert(enums_are_flat,
                  "the signature enums must live directly in namespace "
                  "djinterp");

    return (tags_are_flat && enums_are_flat);
}

/*
tests_vocabulary_is_gated_to_cpp11
  section II sits behind D_ENV_LANG_IS_CPP11_OR_HIGHER; section I does not.
  Tests the following:
  - the gate is on for this build (the suite's floor is C++11, as the
    framework's is)
  - with the gate on, all four enums are complete, usable types
  - the tag structs of section I are usable either way - they are checked for
    their C++98 shape in tests_tag_types_are_cpp98_pods
*/
bool
tests_vocabulary_is_gated_to_cpp11()
{
    const bool gate_is_on = (D_ENV_LANG_IS_CPP11_OR_HIGHER) ? true : false;

    // with the gate on, the four coordinates exist and are complete.  Below it
    // they are absent BY DESIGN - and so is this suite, which cannot be built
    // beneath C++11 at all (see the test header's floor); what section I still
    // owes below the gate is checked by tests_tag_types_are_cpp98_pods.
    const bool vocabulary_present =
        ( (sizeof(concurrency_progress)   > 0) &&
          (sizeof(concurrency_arity)      > 0) &&
          (sizeof(iteration_consistency)  > 0) &&
          (sizeof(reclamation_obligation) > 0) );

    return (gate_is_on && vocabulary_present);
}


NS_END  // testing
NS_END  // djinterp
