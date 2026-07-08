/******************************************************************************
* djinterp [test]                                      option_builder_tests.hpp
*
*   Declarations, fixtures, and section block-providers for the
* option_builder.hpp unit suite.
*
*   option_builder.hpp is the flat-schema-to-option_set pipeline, entirely
* type-level and available at the framework's base standard (std::void_t,
* std::tuple; no concepts, no value face), so the suite is UNCONDITIONAL - every
* test compiles and runs at both C++17 and C++20 with no gating.
*
*   NOTE ON THE PARTITION ENGINE:
*   option_builder.hpp's Section III (option_set_from_flat_t) is built on
* partition_wrap_except_t, historically defined in meta/dtuple_wrap_partition.hpp.
* That header is no longer in the tree, and the surviving tuple_partition.hpp
* (the sized/variable slicer) does not carry the wrap-except engine.  A faithful
* RECONSTRUCTION of dtuple_wrap_partition.hpp (rebuilt from option_builder.hpp's
* documented contract and its worked _N=3 example) is delivered alongside this
* suite and wired in through tuple_partition.hpp.  Consequences for coverage:
*     - Sections I (option_partition_wrap) and II (tuple_to_option_set) are
*       option_builder's own code and are tested directly and faithfully.
*     - Section III exercises the end-to-end pipeline; its results therefore
*       depend on the reconstructed engine matching the documented behavior.
*       If the genuine dtuple_wrap_partition.hpp resurfaces, these tests should
*       pass against it unchanged.
*
*   VERIFICATION MODEL:
*   Every facet is a compile-time trait, pinned with a `static_assert` on a
* `constexpr bool` that is then returned for the runner to record.
*
*   LAYOUT:
*     - one .hpp (this file): declarations + fixtures + block-providers,
*     - one .cpp per section of option_builder.hpp:
*         option_builder_tests_wrap.cpp     (I.   option_partition_wrap)
*         option_builder_tests_lift.cpp     (II.  tuple_to_option_set)
*         option_builder_tests_pipeline.cpp (III. option_set_from_flat_t)
*     - one runner (option_builder_tests_runner.cpp) with main(), emitting a PDF.
*
*   All unit tests live FLAT in namespace djinterp::testing.
*
*
* TABLE OF CONTENTS
* =================
* I.    FIXTURES              (key carrier / opaque slots / passthrough / non-key)
* II.   TEST DECLARATIONS     (per section, flat in djinterp::testing)
* III.  BLOCK PROVIDERS        (one block_spec per section)
*
*
* path:      /tests/djinterp/core/option/option_builder_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

#ifndef DJINTERP_OPTION_BUILDER_TESTS_
#define DJINTERP_OPTION_BUILDER_TESTS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp (resolved via the project include path)
#include "djinterp/test/test_defaults.hpp"          // module_spec / block_spec / test_spec, run_module
#include "djinterp/core/option/option_builder.hpp"  // option_partition_wrap, tuple_to_option_set, option_set_from_flat_t


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   FIXTURES
// ===========================================================================
//   ob_key is a minimal key carrier - a type exposing a static constexpr
// ::value, the one thing option_partition_wrap requires of a chunk's first
// slot.  ob_slot / ob_desc are opaque args that the framework stores verbatim
// (they carry no ::value and no meaning to the pipeline).  ob_pass is a
// passthrough: it inherits passthrough_marker (so the partition engine skips it
// at chunk boundaries) AND defines expanded_t == tuple<> (so the final
// option_set flattens it away, per option_builder's contract).  ob_not_key has
// no ::value and drives the has_value_member false case.

template<auto _V>
struct ob_key
{
    static constexpr auto value = _V;
};

enum class ob_enum { a, b, c, d };

template<auto _V>
struct ob_slot           // opaque value-bearing slot (no ::value member)
{};

template<int _N>
struct ob_desc           // opaque distinguishing slot
{};

struct ob_pass           // passthrough: skipped by the engine, flattened by option_set
    : passthrough_marker
{
    using expanded_t = std::tuple<>;
};

struct ob_not_key        // no static ::value member
{};


// ===========================================================================
// II.  TEST DECLARATIONS
// ===========================================================================

// -- I.  option_partition_wrap --
bool wrap_has_value_member();
bool wrap_key_and_args();
bool wrap_key_only();

// -- II.  tuple_to_option_set --
bool lift_basic();
bool lift_empty();
bool lift_with_passthrough();

// -- III.  option_set_from_flat_t --
bool pipeline_partition_stage();
bool pipeline_single_chunk();
bool pipeline_multiple_chunks();
bool pipeline_n1_key_only();
bool pipeline_passthrough_survives();
bool pipeline_tuple_input();
bool pipeline_empty_schema();


// ===========================================================================
// III. BLOCK PROVIDERS
// ===========================================================================

::djinterp::test::block_spec option_builder_wrap_block();
::djinterp::test::block_spec option_builder_lift_block();
::djinterp::test::block_spec option_builder_pipeline_block();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_OPTION_BUILDER_TESTS_
