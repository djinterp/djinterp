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
* path:      /tests/djinterp/cpp/meta/dtuple_wrap_partition_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_DTUPLE_WRAP_PARTITION_TESTS_
#define DJINTERP_DTUPLE_WRAP_PARTITION_TESTS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/meta/dtuple_wrap_partition.hpp"   // system under test
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


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

// DPassA / DPassB / DPassC
//   passthrough types -- they survive UNWRAPPED at their positions.
struct DPassA {};
struct DPassB {};
struct DPassC {};

// is_pass
//   the unary passthrough predicate (a template<typename> class with ::value),
// true only for the pass_* tags.
template<typename _T>
struct is_pass : std::false_type {};

template<> struct is_pass<DPassA> : std::true_type {};
template<> struct is_pass<DPassB> : std::true_type {};
template<> struct is_pass<DPassC> : std::true_type {};

// group / bundle
//   two distinct variadic wrappers (the _Wrap template-template), so tests can
// confirm the engine wraps with whatever wrapper it is handed.
template<typename... _Ts> struct group  {};
template<typename... _Ts> struct bundle {};

// DPartitionGrouped
//   convenience: partition_wrap_except_t specialized to the common (group,
// is_pass) pair, leaving _N and the schema as the varying inputs.
template<std::size_t _N, typename... _Schema>
using DPartitionGrouped =
    partition_wrap_except_t<group, _N, is_pass, _Schema...>;

}  // namespace pwp_test_types
#endif  // !DTEST_SPEC_MODE  (fixtures)



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


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// dtuple_wrap_partition_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
dtuple_wrap_partition_spec()
{
    return dt::module_spec{
        "dtuple_wrap_partition",
        "RECONSTRUCTION NOTICE ===================== The original "
        "dtuple_wrap_partition.hpp is no longer...",
        {
            dt::block_spec{
                "cat",
                "Cat",
                {
                    { "tests_pwp_cat_basic",
                      "pwp cat basic",
                      &tests_pwp_cat_basic },
                    { "tests_pwp_cat_empty",
                      "pwp cat empty",
                      &tests_pwp_cat_empty },
                }
            },
            dt::block_spec{
                "chunking",
                "Chunking",
                {
                    { "tests_pwp_chunking_n1",
                      "pwp chunking n1",
                      &tests_pwp_chunking_n1 },
                    { "tests_pwp_chunking_n2",
                      "pwp chunking n2",
                      &tests_pwp_chunking_n2 },
                    { "tests_pwp_chunking_n3",
                      "pwp chunking n3",
                      &tests_pwp_chunking_n3 },
                    { "tests_pwp_chunking_n4",
                      "pwp chunking n4",
                      &tests_pwp_chunking_n4 },
                    { "tests_pwp_chunking_multi",
                      "pwp chunking multi",
                      &tests_pwp_chunking_multi },
                    { "tests_pwp_chunking_order",
                      "pwp chunking order",
                      &tests_pwp_chunking_order },
                }
            },
            dt::block_spec{
                "passthrough",
                "Passthrough",
                {
                    { "tests_pwp_passthrough_start",
                      "pwp passthrough start",
                      &tests_pwp_passthrough_start },
                    { "tests_pwp_passthrough_end",
                      "pwp passthrough end",
                      &tests_pwp_passthrough_end },
                    { "tests_pwp_passthrough_between",
                      "pwp passthrough between",
                      &tests_pwp_passthrough_between },
                    { "tests_pwp_passthrough_consecutive",
                      "pwp passthrough consecutive",
                      &tests_pwp_passthrough_consecutive },
                    { "tests_pwp_passthrough_only",
                      "pwp passthrough only",
                      &tests_pwp_passthrough_only },
                    { "tests_pwp_passthrough_n1",
                      "pwp passthrough n1",
                      &tests_pwp_passthrough_n1 },
                }
            },
            dt::block_spec{
                "schema",
                "Schema",
                {
                    { "tests_pwp_schema_pack",
                      "pwp schema pack",
                      &tests_pwp_schema_pack },
                    { "tests_pwp_schema_single_tuple",
                      "pwp schema single tuple",
                      &tests_pwp_schema_single_tuple },
                    { "tests_pwp_schema_tuple_precedence",
                      "pwp schema tuple precedence",
                      &tests_pwp_schema_tuple_precedence },
                    { "tests_pwp_schema_empty_tuple",
                      "pwp schema empty tuple",
                      &tests_pwp_schema_empty_tuple },
                    { "tests_pwp_schema_empty_pack",
                      "pwp schema empty pack",
                      &tests_pwp_schema_empty_pack },
                }
            },
            dt::block_spec{
                "integration",
                "Integration",
                {
                    { "tests_pwp_integration_alias",
                      "pwp integration alias",
                      &tests_pwp_integration_alias },
                    { "tests_pwp_integration_wrapper_arbitrary",
                      "pwp integration wrapper arbitrary",
                      &tests_pwp_integration_wrapper_arbitrary },
                    { "tests_pwp_integration_complex",
                      "pwp integration complex",
                      &tests_pwp_integration_complex },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_DTUPLE_WRAP_PARTITION_TESTS_
