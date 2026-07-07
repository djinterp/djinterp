/******************************************************************************
* djinterp [test]                                          option_set_tests.hpp
*
*   Declarations, fixtures, and section block-providers for the option_set.hpp
* unit suite.
*
*   option_set.hpp has three tiers, and the suite tracks them:
*     - a TYPE-LEVEL core available down to the header's base standard
*       (expansion, flattening, the construction checks, the option_set pack
*       surface: size / empty / flat_options_t / option_at, and the query
*       traits is_option_set / option_set_contains / option_set_find);
*     - a C++20 VALUE-CARRYING face (values_type, the values constructor, and
*       key-addressed get / set / contains / values);
*     - C++20 CONCEPTS (Keyed / OptionSet / ...) plus the one constrained query
*       trait option_set_key_type.
*
*   Gating therefore is NOT wholesale: most tests compile at the header's base
* level and run everywhere; the value-face tests are gated on C++20; the
* concept and option_set_key_type tests are gated on C++20 concepts.  Below a
* given tier the corresponding tests are emitted empty (with a "skipped"
* descriptor) so the runner still links and reports.
*
*   VERIFICATION MODEL:
*   Type-level facets are pinned with a `static_assert` on a `constexpr bool`,
* then that bool is returned for the runner to record (the static_assert is the
* real check; the return drives instantiation and gives a verdict).  The
* value-face facets that exercise runtime state (construct / set / get) are
* evaluated inside a `constexpr` immediately-invoked lambda so they are checked
* at COMPILE time as well, then returned - a get/set regression fails the build
* at the assertion rather than only at run time.
*
*   LAYOUT:
*     - one .hpp (this file): declarations + fixtures + block-providers,
*     - one .cpp per section group of option_set.hpp:
*         option_set_tests_expand.cpp    (I + II. expansion + flattening)
*         option_set_tests_checks.cpp    (III.   construction checks)
*         option_set_tests_core.cpp      (IV.    option_set pack surface)
*         option_set_tests_values.cpp    (V.     field marker + value face)
*         option_set_tests_queries.cpp   (VI.    is_option_set / contains / find)
*         option_set_tests_concepts.cpp  (VII.   C++20 concepts)
*     - one runner (option_set_tests_runner.cpp) with main().
*
*   All unit tests live FLAT in namespace djinterp::testing.  The framework spec
* model (module_spec / block_spec / test_spec) and run_module come from
* test_defaults.hpp; the header under test and its dependencies come from
* option_set.hpp.  Both resolve through the project's configured include path.
*
*
* TABLE OF CONTENTS
* =================
* I.    FIXTURES              (key enums / expanders / passthrough / non-option)
* II.   TEST DECLARATIONS     (per section, flat in djinterp::testing)
* III.  BLOCK PROVIDERS        (one block_spec per section group)
*
*
* path:      /tests/djinterp/core/option/option_set_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_OPTION_SET_TESTS_
#define DJINTERP_OPTION_SET_TESTS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp (resolved via the project include path)
#include "djinterp/test/test_defaults.hpp"    // module_spec / block_spec / test_spec, run_module
#include "djinterp/core/option/option_set.hpp" // option_set<>, expand_option, field, queries, concepts


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   FIXTURES
// ===========================================================================
//   A single scoped key enum is used for members of any one set (option_set
// enforces a uniform key_type); a second enum exists only to exercise the
// key_type-uniformity check at the trait level.  The expander / passthrough
// fixtures drive the ::expanded_t customization point, and os_not_option is a
// non-option used for the strict all-options check.

enum class os_key  { a, b, c, d, e };
enum class os_key2 { z };                 // a DIFFERENT key_type (uniformity check)

// a type that is NOT an option (no ::key / ::key_type) - fails is_option_v
struct os_not_option {};

// multi-expander: contributes two options (keys b, c) through ::expanded_t
struct os_expander_bc
{
    using expanded_t = std::tuple<option<os_key::b>, option<os_key::c>>;
};

// multi-expander that contributes a single option (key d)
struct os_expander_d
{
    using expanded_t = std::tuple<option<os_key::d>>;
};

// passthrough marker: expands to nothing (empty tuple) - contributes no key
struct os_passthrough
{
    using expanded_t = std::tuple<>;
};


// ===========================================================================
// II.  TEST DECLARATIONS
// ===========================================================================

// -- I + II.  expansion + flattening --
bool expand_option_default_wraps();
bool expand_option_multi_expander();
bool expand_option_passthrough_empty();
bool flatten_tuples_concatenates();
bool flatten_tuples_edge_cases();

// -- III.  construction checks --
bool checks_are_all_options();
bool checks_all_same_type();
bool checks_run_set_checks_pass();

// -- IV.  option_set pack surface --
bool core_empty_set();
bool core_single_option();
bool core_multi_preserves_order();
bool core_expander_flattens();
bool core_mixed_direct_and_expander();
bool core_passthrough_contributes_nothing();

// -- V.  field marker + value face --
bool values_field_type();
bool values_unary_option_alias();
bool values_option_field_cases();
bool values_store_values();
bool values_os_slot();
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
bool values_values_type();
bool values_roundtrip_get_set();
bool values_default_value_initialized();
bool values_member_contains();
#endif

// -- VI.  queries --
bool queries_is_option_set();
bool queries_is_option_set_cleans_cvref();
bool queries_option_set_contains();
bool queries_option_set_find_hit();
bool queries_option_set_find_miss();
bool queries_find_and_contains_via_expansion();
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
bool queries_option_set_key_type();
#endif

// -- VII.  concepts (C++20) --
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
bool concept_keyed();
bool concept_option_set();
bool concept_option_set_contains();
bool concept_option_set_findable();
bool concept_option_set_non_empty();
#endif


// ===========================================================================
// III. BLOCK PROVIDERS
// ===========================================================================
//   One block per section group.  Declared unconditionally; each fills only
// the tests available at the current standard tier (empty otherwise), so the
// runner needs no version gate at its call site.

::djinterp::test::block_spec option_set_expand_block();
::djinterp::test::block_spec option_set_checks_block();
::djinterp::test::block_spec option_set_core_block();
::djinterp::test::block_spec option_set_values_block();
::djinterp::test::block_spec option_set_queries_block();
::djinterp::test::block_spec option_set_concepts_block();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_TESTS_
