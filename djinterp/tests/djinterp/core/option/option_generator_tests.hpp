/*******************************************************************************
* djinterp [test]                                     option_generator_tests.hpp
*
*   Shared surface for the option_generator.hpp unit suite.  Declares the four
* section block-providers - one per section of the header under test - and the
* helpers those sections share:
*
*     - setting             a single enum-class key vocabulary; the flat form
*                           infers the key type from the first token, and every
*                           key in a set must share it, so one enum drives all
*                           fixtures.  Values are ints / chars / fixed_string,
*                           none of which is `setting`, keeping keys and values
*                           type-distinguishable as the header requires.
*     - same / same_v       order-sensitive type identity, for asserting exact
*                           spec tuples, emitted option_set<...> types, and slot
*                           value tuples.
*
*   Everything in option_generator.hpp lives behind D_ENV_LANG_IS_CPP20_OR_HIGHER
* (auto-NTTP packs, requires-distinguished partial specializations, class-type
* NTTPs for string values), so the header self-suppresses below C++20.  Each
* section's block body is therefore gated to C++20 and emits an empty block
* below it; the declarations, this enum, and same/same_v stay standard-agnostic.
*
*   fixed_string is pulled in directly (option_generator.hpp does not include it)
* so the fixtures can author a class-type NTTP string value, exercising the
* remove_cvref path in kv_spec and the field<fixed_string<N>> emission.
*
*
* path:      /tests/djinterp/core/option/option_generator_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_OPTION_GENERATOR_TESTS_
#define DJINTERP_OPTION_GENERATOR_TESTS_ 1

// std
#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>
// djinterp
// djinterp [test] - framework runner
// djinterp [option] - the header under test (pulls in option.hpp + option_set.hpp)
#ifdef DTEST_SPEC_MODE
#include <djinterp/test/test_defaults.hpp>
#endif  // DTEST_SPEC_MODE
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/option/option_generator.hpp"
#endif  // !DTEST_SPEC_MODE
// djinterp [meta] - class-type NTTP string carrier for the fixtures
#include "../../../../inc/djinterp/core/meta/fixed_string.hpp"


NS_DJINTERP
NS_TESTING

#ifndef DTEST_SPEC_MODE   // fixtures: normal face only


// ===========================================================================
// shared helpers
// ===========================================================================

// setting
//   enum: the single key vocabulary every fixture keys on.  width / height /
// depth carry int values; visible / mode are presence-only (unary) flags;
// name carries a fixed_string.  depth doubles as a "known-absent" key for
// negative contains<> checks.
enum class setting
{
    width,
    height,
    depth,
    visible,
    mode,
    name
};

// same
//   trait: ORDER-SENSITIVE type identity.  Parse results are exact
// std::tuple<spec...> types and emitted sets are exact option_set<...> types,
// both order- and count-sensitive, so the assertions check identity.
template<typename _Lhs,
         typename _Rhs>
struct same
    : std::false_type
{};

template<typename _T>
struct same<_T, _T>
    : std::true_type
{};

template<typename _Lhs,
         typename _Rhs>
D_CONSTEXPR_INLINE bool same_v = same<_Lhs, _Rhs>::value;


// ===========================================================================
// section block-providers (one per translation unit)
#endif  // !DTEST_SPEC_MODE

// ===========================================================================
//
//   Each returns the section's block_spec; the runner assembles them into a
// single module_spec.  Names mirror the header's own section numbering.

// I.   SPEC CARRIERS      (option_generator_tests_specs.cpp)
::djinterp::test::block_spec option_generator_specs_block();

// II.  STREAM PARSE       (option_generator_tests_parse.cpp)
::djinterp::test::block_spec option_generator_parse_block();

// III. EMITTER            (option_generator_tests_emit.cpp)
::djinterp::test::block_spec option_generator_emit_block();

// IV.  PUBLIC FRONT-END   (option_generator_tests_frontend.cpp)
::djinterp::test::block_spec option_generator_frontend_block();


// ===========================================================================
// SPEC PROVIDER   (spec face - the runner hands this to run_module)
// ===========================================================================
#ifdef DTEST_SPEC_MODE
inline ::djinterp::test::module_spec
option_generator_spec()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "option_generator.hpp";
    mod.descriptor =
        "flat NTTP stream -> populated option_set: specs, parse, emit, front-end";

    mod.blocks.push_back(tt::option_generator_specs_block());     // I
    mod.blocks.push_back(tt::option_generator_parse_block());     // II
    mod.blocks.push_back(tt::option_generator_emit_block());      // III
    mod.blocks.push_back(tt::option_generator_frontend_block());  // IV

    return mod;
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_OPTION_GENERATOR_TESTS_
