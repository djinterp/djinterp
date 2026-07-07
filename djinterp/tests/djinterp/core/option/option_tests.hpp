/******************************************************************************
* djinterp [test]                                             option_tests.hpp
*
*   Declarations, fixtures, and section block-providers for the option.hpp
* unit suite.  option.hpp is a purely TYPE-LEVEL header (the core option<>
* aggregate, the is_option detection trait, and the C++20 Option /
* UnaryOption / ArgsOption concept analogs), so the suite verifies its
* contract at COMPILE TIME.
*
*   VERIFICATION MODEL (why static_assert AND a returned bool):
*   Every test function pins its facet with a `static_assert` on a
* `constexpr bool` - that is the real, strongest check for a metaprogramming
* header: a regression fails the build at the exact assertion, with a message.
* The very same `constexpr bool` is then RETURNED so the framework's runtime
* runner (run_module) has a verdict to record and a report to print, and so
* every option<> specialization named here is actually instantiated (that is
* the coverage).  When the header is correct the asserts hold and the report
* is green; if it ever regresses, the offending translation unit stops
* compiling at the precise facet - which for a type-level header is the ideal
* signal.
*
*   LAYOUT:
*     - one .hpp (this file): declarations + fixtures + block-providers,
*     - one .cpp per section of option.hpp:
*         option_sentinels_tests.cpp   (I.   arg_not_found / arg_npos)
*         option_core_tests.cpp        (II.  option<> core type)
*         option_is_option_tests.cpp   (III. is_option / is_option_v)
*         option_tests_concepts.cpp    (IV.  Option / UnaryOption / ArgsOption)
*     - one runner (option_tests_runner.cpp) with main().
*
*   All unit tests live FLAT in namespace djinterp::testing (no nesting).
*   The framework spec model (module_spec / block_spec / test_spec) and the
* run_module entry point come from test_defaults.hpp; the header under test
* comes from option.hpp.  Both resolve through the project's configured
* include path (the `inc/` root), matching the rest of the framework.
*
*
* TABLE OF CONTENTS
* =================
* I.    FIXTURES              (sample keys / arg tags / decoy / NTTP source)
* II.   MEMBER DETECTORS      (SFINAE presence probes for the option<> surface)
* III.  TEST DECLARATIONS     (per section, flat in djinterp::testing)
* IV.   BLOCK PROVIDERS        (one block_spec per section)
*
*
* path:      /tests/djinterp/core/option/option_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_OPTION_TESTS_
#define DJINTERP_OPTION_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
// djinterp (resolved via the project include path)
#include "djinterp/test/test_defaults.hpp"      // module_spec / block_spec / test_spec, run_module
#include "djinterp/core/option/option.hpp"      // the header under test


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   FIXTURES
// ===========================================================================
//   option<> imposes NO meaning on its key or args, so the fixtures are the
// most neutral shapes that still exercise every axis: a scoped key enum, a
// second scoped enum (for key-type distinctness), an unscoped enum, a few
// opaque arg tags, a duck-typed "looks like an option but is not" decoy, and
// a linkage-bearing object for a pointer-valued NTTP key.

// scoped key enums
enum class opt_key  { alpha, beta, gamma };
enum class opt_key2 { red, green };

// unscoped (plain) key enum
enum opt_plain_key { opk_zero, opk_one };

// opaque arg tags (args are stored verbatim; option<> never inspects them)
struct arg_a {};
struct arg_b {};
struct arg_c {};

// option_decoy
//   fixture: a type that duck-types the entire option<> member surface yet is
// NOT an option<...> specialization.  is_option must reject it - detection is
// by template identity, never by structural shape.
struct option_decoy
{
    using key_type  = opt_key;
    using args_type = std::tuple<arg_a>;

    static constexpr opt_key     key       = opt_key::alpha;
    static constexpr bool        has_args  = true;
    static constexpr std::size_t arg_count = 1;
};

// opt_nttp_object
//   fixture: a constexpr object with linkage, usable as the referent of a
// pointer-valued non-type template parameter (option<&opt_nttp_object>).
inline constexpr int opt_nttp_object = 7;


// ===========================================================================
// II.  MEMBER DETECTORS
// ===========================================================================
//   SFINAE presence probes for each member of the option<> surface.  Needed
// because the unary form deliberately OMITS args_type: naming it directly on
// a unary option would be a hard error, so absence is asserted through a
// probe, not an access.  Each yields a plain constexpr bool via ::value.

template<typename _T, typename = void>
struct has_key_type_member : std::false_type {};
template<typename _T>
struct has_key_type_member<_T, std::void_t<typename _T::key_type>> : std::true_type {};

template<typename _T, typename = void>
struct has_args_type_member : std::false_type {};
template<typename _T>
struct has_args_type_member<_T, std::void_t<typename _T::args_type>> : std::true_type {};

template<typename _T, typename = void>
struct has_key_value_member : std::false_type {};
template<typename _T>
struct has_key_value_member<_T, std::void_t<decltype(_T::key)>> : std::true_type {};

template<typename _T, typename = void>
struct has_has_args_member : std::false_type {};
template<typename _T>
struct has_has_args_member<_T, std::void_t<decltype(_T::has_args)>> : std::true_type {};

template<typename _T, typename = void>
struct has_arg_count_member : std::false_type {};
template<typename _T>
struct has_arg_count_member<_T, std::void_t<decltype(_T::arg_count)>> : std::true_type {};


// ===========================================================================
// III. TEST DECLARATIONS
// ===========================================================================
//   Every test is a nullary predicate returning true iff its facet holds.
// Grouped by the section of option.hpp it covers; all flat in this namespace.

// -- I.  arg_not_found / arg_npos sentinels --
bool sentinel_arg_not_found_shape();
bool sentinel_arg_not_found_distinct();
bool sentinel_arg_npos_value();
bool sentinel_arg_npos_type();

// -- II.  option<> core type --
bool core_unary_members();
bool core_unary_member_surface();
bool core_args_single_members();
bool core_args_single_member_surface();
bool core_args_multi_counts();
bool core_args_many();
bool core_arg_count_matches_tuple_size();
bool core_key_type_variety();
bool core_key_value_variety();
bool core_key_nullptr();
bool core_key_pointer();
bool core_args_not_flattened();
bool core_args_duplicates_preserved();
bool core_args_ref_cv_ptr();
bool core_type_identity_same();
bool core_type_identity_distinct();

// -- III.  is_option / is_option_v --
bool isopt_true_forms();
bool isopt_false_nonoptions();
bool isopt_struct_does_not_clean();
bool isopt_v_cleans_cvref();
bool isopt_decoy_rejected();
bool isopt_trait_shape();

// -- IV.  Option / UnaryOption / ArgsOption (C++20 concepts) --
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
bool concept_option_tracks_trait();
bool concept_unary_positive_negative();
bool concept_args_positive_negative();
bool concept_unary_args_exclusive();
bool concept_reference_edge();
bool concept_constrains_overloads();
#endif


// ===========================================================================
// IV.  BLOCK PROVIDERS
// ===========================================================================
//   One block per section.  Each returns a fully-populated block_spec that a
// runner folds into the option.hpp module_spec.  option_concepts_block is
// declared unconditionally; below C++20 it yields an empty block (there is no
// concept surface to test), so the runner needs no version gate at its call
// site.

::djinterp::test::block_spec option_sentinels_block();
::djinterp::test::block_spec option_core_block();
::djinterp::test::block_spec option_is_option_block();
::djinterp::test::block_spec option_concepts_block();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_OPTION_TESTS_
