/******************************************************************************
* djinterp [test]                                     option_compose_tests.hpp
*
*   Declarations, fixtures, and section block-providers for the
* option_compose.hpp unit suite.
*
*   option_compose.hpp is the "declare + add" sugar over option_set and the
* override engine.  It is C++20-ONLY: every fold routes through
* option_set_override and its template parameters are constrained by the
* OverridePolicy concept (meta/override.hpp), so the header does not compile
* below C++20 concepts.  The suite is therefore gated WHOLESALE on
* `D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS`: at C++20 it
* is the full suite; below that the header is not even included, every section
* is emitted empty (with a "skipped" descriptor), and the runner links and
* reports a clean 0/0 rather than failing to build.
*
*   Only Section I (defopt) is concept-free in principle - it is a bare alias
* for option<> - but it lives in this header, which as a whole needs C++20, so
* it is gated with the rest.
*
*   VERIFICATION MODEL:
*   Every idiom yields a TYPE, so each facet is pinned with a `static_assert`
* on a `constexpr bool` (an exact std::is_same on the produced option_set) that
* is then returned for the runner to record.
*
*   LAYOUT:
*     - one .hpp (this file): declarations + fixtures + block-providers,
*     - one .cpp per section of option_compose.hpp:
*         option_compose_tests_defopt.cpp        (I.   defopt)
*         option_compose_tests_with_option.cpp   (II.  with_option / _as)
*         option_compose_tests_with_options.cpp  (III. with_options / _as)
*         option_compose_tests_compose.cpp       (IV.  compose_options / _as)
*     - one runner (option_compose_tests_runner.cpp) with main(), emitting a PDF.
*
*   All unit tests live FLAT in namespace djinterp::testing.
*
*
* TABLE OF CONTENTS
* =================
* I.    FIXTURES              (key enum / opaque value carrier)
* II.   TEST DECLARATIONS     (per section, flat in djinterp::testing)
* III.  BLOCK PROVIDERS        (one block_spec per section)
*
*
* path:      /tests/djinterp/core/option/option_compose_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_OPTION_COMPOSE_TESTS_
#define DJINTERP_OPTION_COMPOSE_TESTS_ 1

// std
#include <type_traits>
// djinterp
#include "djinterp/test/test_defaults.hpp"   // module_spec / block_spec / test_spec, run_module
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
#  include "djinterp/core/option/option_compose.hpp" // defopt, with_option(s), compose_options
#endif


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ===========================================================================
// I.   FIXTURES
// ===========================================================================
//   One scoped key enum for set members, and an opaque value carrier that sits
// in an option's args (the compose layer keeps args verbatim, so its only role
// is to make two surfaces at the same key distinguishable for the collision
// tests).

enum class oc_key { a, b, c, d };

template<auto _V>
struct oc_val
{};


// ===========================================================================
// II.  TEST DECLARATIONS
// ===========================================================================

// -- I.  defopt --
bool defopt_unary();
bool defopt_with_args();
bool defopt_multi_arg();

// -- II.  with_option / with_option_as --
bool with_option_as_set_internal();
bool with_option_add_to_empty();
bool with_option_add_new_key();
bool with_option_collision_replaces();
bool with_option_as_explicit_policy();
bool with_option_as_strict_same_key();

// -- III.  with_options / with_options_as --
bool with_options_as_delta_set_internal();
bool with_options_fold_surfaces();
bool with_options_mixed_surface_and_subset();
bool with_options_collision_later_wins();
bool with_options_pack_same_key_last_wins();
bool with_options_empty_pack_is_base();
bool with_options_as_explicit_policy();

// -- IV.  compose_options / compose_options_as --
bool compose_build_from_empty();
bool compose_empty();
bool compose_collision_later_wins();
bool compose_as_explicit_policy();
bool compose_doc_example();

#endif  // C++20 concepts


// ===========================================================================
// III. BLOCK PROVIDERS
// ===========================================================================
//   Declared unconditionally; each fills its tests only at C++20 (empty with a
// skipped descriptor otherwise), so the runner needs no gate at its call site.

::djinterp::test::block_spec option_compose_defopt_block();
::djinterp::test::block_spec option_compose_with_option_block();
::djinterp::test::block_spec option_compose_with_options_block();
::djinterp::test::block_spec option_compose_compose_block();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_OPTION_COMPOSE_TESTS_
