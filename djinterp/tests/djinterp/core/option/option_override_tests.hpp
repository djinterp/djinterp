/******************************************************************************
* djinterp [test]                                    option_override_tests.hpp
*
*   Declarations, fixtures, and section block-providers for the
* option_override.hpp unit suite.  option_override.hpp is a purely TYPE-LEVEL
* header - the option-aware override engine (option_set_override), the
* option-aware merge metafn (merge_args_union), the lazy on_delta_only SFINAE
* plumbing, and the ready-made policy aliases - so the suite verifies its
* contract at COMPILE TIME.
*
*   VERIFICATION MODEL (why static_assert AND a returned bool):
*   Every test function pins its facet with a `static_assert` on a
* `constexpr bool`, then RETURNS that same bool.  For a metaprogramming header
* the static_assert is the real, strongest check (a regression fails the build
* at the exact assertion, with a message); the returned bool gives the
* framework runner a verdict to record and forces instantiation of every
* engine walk, merge, and policy named here (that is the coverage).
*
*   STANDARD REQUIREMENT (C++20):
*   option_override.hpp is C++20-only - option_set_override constrains its
* policy parameter with the `OverridePolicy` concept, and override.hpp defines
* that concept unconditionally.  The whole suite is therefore gated on C++20
* concepts: below that the section blocks are emitted empty, so the runner
* still links and reports (0 tests) instead of failing to compile.
*
*   LAYOUT:
*     - one .hpp (this file): declarations + fixtures + block-providers,
*     - one .cpp per section of option_override.hpp:
*         option_override_helpers_tests.cpp  (I.   (re)construction helpers)
*         option_override_merge_tests.cpp    (II.  merge_args_union)
*         option_override_lazy_tests.cpp     (III. lazy on_delta_only + append)
*         option_override_engine_tests.cpp   (IV.  option_set_override engine)
*         option_override_policies_tests.cpp (V.   ready-made policies)
*     - one runner (option_override_tests_runner.cpp) with main().
*
*   All unit tests live FLAT in namespace djinterp::testing (no nesting).  The
* framework spec model (module_spec / block_spec / test_spec) and run_module
* come from test_defaults.hpp; the header under test and its dependencies come
* from option_override.hpp.  Both resolve through the project's configured
* include path (the `inc/` root), matching the rest of the framework.
*
*
* TABLE OF CONTENTS
* =================
* I.    FIXTURES              (key enum / arg tags / swap markers)
* II.   PREDICATE HELPER      (one-arg trait for the swap-helper tests)
* III.  TEST DECLARATIONS     (per section, flat in djinterp::testing)
* IV.   BLOCK PROVIDERS        (one block_spec per section)
*
*
* path:      /tests/djinterp/core/option/option_override_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_OPTION_OVERRIDE_TESTS_
#define DJINTERP_OPTION_OVERRIDE_TESTS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp (resolved via the project include path)
#include "djinterp/test/test_defaults.hpp"   // module_spec / block_spec / test_spec, run_module

// The header under test is C++20-only (OverridePolicy concept); include it -
// and everything that names its types - only where concepts are available.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
#  include "djinterp/core/option/option_override.hpp"  // engine, merge_args_union, policies
#endif


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ===========================================================================
// I.   FIXTURES
// ===========================================================================
//   option<> / option_set<> impose no meaning on keys or args, so the
// fixtures are the most neutral shapes that still exercise every axis.  A
// SINGLE scoped key enum is used throughout because option_set enforces a
// uniform key_type across a set; distinct enumerators give distinct,
// order-comparable keys.  The arg tags are opaque (the engine and the merge
// metafn store them verbatim and never inspect them).

enum class ov_key { a, b, c, d, e };

// opaque arg tags (roles - base arg, delta arg, extra - are assigned per test)
struct ov_p {};
struct ov_q {};
struct ov_r {};
struct ov_s {};

// swap-helper fixtures: a target arg the predicate matches, and its replacement
struct ov_swap_target {};
struct ov_swap_new    {};


// ===========================================================================
// II.  PREDICATE HELPER
// ===========================================================================
//   A one-argument trait template (a `template<typename> class`) is required
// by the section I swap helpers (replace_or_append_arg / option_swap_arg).
// This one matches exactly the ov_swap_target fixture.

template<typename _Arg>
struct ov_is_swap_target : std::is_same<_Arg, ov_swap_target> {};


// ===========================================================================
// III. TEST DECLARATIONS
// ===========================================================================
//   Every test is a nullary predicate returning true iff its facet holds.
// Grouped by the section of option_override.hpp it covers; all flat here.

// -- I.  option (re)construction helpers (internal::) --
bool helper_args_as_tuple();
bool helper_rebuild_option();
bool helper_args_rebuild_roundtrip();
bool helper_replace_or_append_arg();
bool helper_replace_or_append_base_cases();
bool helper_option_swap_arg();

// -- II.  merge_args_union --
bool merge_both_have_args();
bool merge_base_unary();
bool merge_delta_unary();
bool merge_both_unary();
bool merge_delta_wins_order();
bool merge_no_dedupe();
bool merge_preserves_arg_qualifiers();

// -- III.  lazy on_delta_only + append_if_kept (internal::) --
bool lazy_drop_true_yields_dropped();
bool lazy_false_keep_delta_keeps();
bool lazy_false_keep_base_drops();
bool lazy_false_drop_extras_drops();
bool lazy_false_drop_unmatched_base_keeps();
bool lazy_true_guards_strict_assert();
bool append_if_kept_appends();
bool append_if_kept_skips_dropped();

// -- IV.  option_set_override engine --
bool engine_tuple_to_option_set();
bool engine_result_is_option_set();
bool engine_empty_base();
bool engine_empty_delta();
bool engine_both_empty();
bool engine_disjoint_preserves_order();
bool engine_overlap_replaces_whole_option();
bool engine_overlap_preserves_base_order();
bool engine_t_alias_matches_struct();

// -- V.  ready-made policies --
bool policy_override_replace();
bool policy_override_keep();
bool policy_override_subset();
bool policy_override_filter();
bool policy_override_strict_accepts_subset();
bool policy_arg_union_delta_hooks();
bool policy_arg_union_delta_engine();

#endif  // C++20 concepts available


// ===========================================================================
// IV.  BLOCK PROVIDERS
// ===========================================================================
//   One block per section.  Declared unconditionally; below C++20 each yields
// an empty block (there is no engine surface to test), so the runner needs no
// version gate at its call site.

::djinterp::test::block_spec option_override_helpers_block();
::djinterp::test::block_spec option_override_merge_block();
::djinterp::test::block_spec option_override_lazy_block();
::djinterp::test::block_spec option_override_engine_block();
::djinterp::test::block_spec option_override_policies_block();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_OPTION_OVERRIDE_TESTS_
