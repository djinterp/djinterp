/*******************************************************************************
* djinterp [test]                                dtuple_wrap_partition_tests.hpp
*
*   Unit-test suite for the chunk-and-wrap partition engine in
* `dtuple_wrap_partition.hpp` (partition_wrap_except / _t).  Tests are split
* across translation units by semantic group:
*
*     I.   pwe_cat (type-level concat helper)  -> ..._tests_cat.cpp
*     II.  chunking (N consecutive slots)       -> ..._tests_chunking.cpp
*     III. passthrough survival                 -> ..._tests_passthrough.cpp
*     IV.  schema input forms + precedence      -> ..._tests_schema.cpp
*     V.   alias + integration                  -> ..._tests_integration.cpp
*
*   STRUCTURE:
*   The engine is entirely type-level, so every check is a `std::is_same`
* against an expected std::tuple, pinned at file scope with `static_assert`
* (a regression breaks the build) and mirrored in the run-time predicates so
* the outcome also shows in the runner's report.
*
*   API (matches AGENTS-cmake.md):
*   Every entry point is a nullary `bool tests_pwp_*()`, FLAT in
* `djinterp::testing`, driven by the runner as `&::djinterp::testing::tests_pwp_*`
* through the D_WP_RUN harness.  Shared fixtures live in the nested
* `pwp_test_types` namespace.
*
*   COVERAGE INTENT (100%):
*     - pwe_cat: the tuple/tuple concat specialization, incl. empty operands;
*     - pwe_walk: the step (>=1 entry) and the schema-exhausted base (empty
*       chunk -> std::tuple<>), the latter reached by the empty pack / empty
*       tuple;
*     - pwe_dispatch, all three specializations: passthrough at a boundary (1),
*       non-passthrough that does NOT complete the chunk -> accumulate (2), and
*       non-passthrough that COMPLETES the chunk -> wrap + emit (3);
*     - the _ChunkCompletes edge at _N == 1 (each slot completes immediately)
*       vs _N >= 2 (accumulate then complete);
*     - partition_wrap_except: the bare-pack primary AND the single-std::tuple
*       specialization, including that a lone std::tuple<> routes to the
*       specialization (empty result) while a pack CONTAINING a tuple routes to
*       the primary (the tuple is an ordinary slot).
*
*   NOT COVERABLE IN A PASSING SUITE:
*   The two malformed-schema conditions (a passthrough inside a chunk; a
*   trailing run not a whole multiple of _N) are `static_assert` HARD errors by
*   design -- they cannot be instantiated without breaking the build, and being
*   hard (not SFINAE) errors they cannot be probed either.  Exercised here only
*   by their absence: every well-formed schema below sits on the passing side.
*
*
* path:      /tests/djinterp/core/meta/dtuple_wrap_partition_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_DTUPLE_WRAP_PARTITION_TESTS_
#define DJINTERP_DTUPLE_WRAP_PARTITION_TESTS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../../../../inc/djinterp/core/meta/dtuple_wrap_partition.hpp"   // system under test


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED FIXTURES                                     ///
///////////////////////////////////////////////////////////////////////////////
//   Nested so the flat tests_pwp_* surface stays clean; each section .cpp pulls
// them in with `using namespace pwp_test_types;`.

namespace pwp_test_types
{

// slot
//   a non-passthrough schema slot; distinct _Id -> distinct type, so wrapped
// chunks are checkable by exact identity (and slot order within a chunk).
template<int _Id>
struct slot {};

// pass_a / pass_b / pass_c
//   passthrough types -- they survive UNWRAPPED at their positions.
struct pass_a {};
struct pass_b {};
struct pass_c {};

// is_pass
//   the unary passthrough predicate (a template<typename> class with ::value),
// true only for the pass_* tags.
template<typename _T>
struct is_pass : std::false_type {};

template<> struct is_pass<pass_a> : std::true_type {};
template<> struct is_pass<pass_b> : std::true_type {};
template<> struct is_pass<pass_c> : std::true_type {};

// group / bundle
//   two distinct variadic wrappers (the _Wrap template-template), so tests can
// confirm the engine wraps with whatever wrapper it is handed.
template<typename... _Ts> struct group  {};
template<typename... _Ts> struct bundle {};

// partition_grouped
//   convenience: partition_wrap_except_t specialized to the common (group,
// is_pass) pair, leaving _N and the schema as the varying inputs.
template<std::size_t _N, typename... _Schema>
using partition_grouped =
    partition_wrap_except_t<group, _N, is_pass, _Schema...>;

}  // namespace pwp_test_types


///////////////////////////////////////////////////////////////////////////////
///                II.  PER-SECTION TEST DECLARATIONS                       ///
///////////////////////////////////////////////////////////////////////////////

// ---- I.  pwe_cat ----
bool tests_pwp_cat_basic();
bool tests_pwp_cat_empty();

// ---- II. chunking ----
bool tests_pwp_chunking_n1();
bool tests_pwp_chunking_n2();
bool tests_pwp_chunking_n3();
bool tests_pwp_chunking_n4();
bool tests_pwp_chunking_multi();
bool tests_pwp_chunking_order();

// ---- III. passthrough survival ----
bool tests_pwp_passthrough_start();
bool tests_pwp_passthrough_end();
bool tests_pwp_passthrough_between();
bool tests_pwp_passthrough_consecutive();
bool tests_pwp_passthrough_only();
bool tests_pwp_passthrough_n1();

// ---- IV. schema input forms ----
bool tests_pwp_schema_pack();
bool tests_pwp_schema_single_tuple();
bool tests_pwp_schema_tuple_precedence();
bool tests_pwp_schema_empty_tuple();
bool tests_pwp_schema_empty_pack();

// ---- V. alias + integration ----
bool tests_pwp_integration_alias();
bool tests_pwp_integration_wrapper_arbitrary();
bool tests_pwp_integration_complex();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_DTUPLE_WRAP_PARTITION_TESTS_
