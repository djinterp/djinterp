/*******************************************************************************
* djinterp [test]                                       option_factory_tests.hpp
*
*   Shared surface for the option_factory.hpp unit suite.  Declares the three
* section block-providers - each defined in its own translation unit, one per
* section of the header under test - and the handful of helpers those sections
* share:
*
*     - fkey                a small enum-class key vocabulary for the fixtures.
*     - arg_a / arg_b / arg_c
*                           opaque empty tag types used as option<> args.  The
*                           factory imposes no meaning on args, so any distinct
*                           typenames serve; these prove the args pass through
*                           verbatim (identity, order, and count).
*     - type_pack           a bare template<typename...> slot, used to show
*                           key_carrier<> occupying a typename position - the
*                           very role (NTTP-as-typename bridge) it exists for.
*     - same / same_v       order-sensitive type identity, for asserting exact
*                           option<...> / tuple<...> results.
*
*   No default-extractor shim is needed (unlike the option_diff suite):
* option_factory.hpp depends only on djinterp.hpp and option.hpp and compiles
* stand-alone.  Nor is there any standard gating: the header is pure C++17
* (auto NTTP, value-init, noexcept) with no concept-constrained machinery, so
* every section compiles wherever the header includes at all - option.hpp's
* auto-NTTP keys already make C++17 the hard floor for the whole option layer.
*
*
* path:      /tests/djinterp/core/option/option_factory_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_OPTION_FACTORY_TESTS_
#define DJINTERP_OPTION_FACTORY_TESTS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
// djinterp [test] - framework runner
// djinterp [option] - the header under test
#ifdef DTEST_SPEC_MODE
#include <djinterp/test/test_defaults.hpp>
#endif  // DTEST_SPEC_MODE
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/option/option_factory.hpp"
#endif  // !DTEST_SPEC_MODE


NS_DJINTERP
NS_TESTING

#ifndef DTEST_SPEC_MODE   // fixtures: normal face only


// ===========================================================================
// shared helpers
// ===========================================================================

// fkey
//   enum: the key vocabulary the factory fixtures are keyed on.
enum class fkey
{
    alpha,
    beta,
    gamma,
    delta
};

// arg_a / arg_b / arg_c
//   type: opaque, distinct empty tags used as option<> args.  option_factory
// treats args as an uninterpreted typename pack, so these only need to be
// distinct types to exercise pass-through identity, order, and count.
struct arg_a
{};

struct arg_b
{};

struct arg_c
{};

// type_pack
//   type: a bare template<typename...> position, standing in for a schema
// wrapper's typename-only signature - the slot key_carrier<> is meant to fill.
template<typename... _Types>
struct type_pack
{};

// same
//   trait: ORDER-SENSITIVE type equality.  The factory results are exact
// option<...> (and, for the args form, std::tuple<...>) types whose argument
// order and count matter, so the assertions check identity, not similarity.
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

// I.   key_carrier            (option_factory_tests_carrier.cpp)
::djinterp::test::block_spec option_factory_carrier_block();

// II.  make_option_t          (option_factory_tests_make_type.cpp)
::djinterp::test::block_spec option_factory_make_type_block();

// III. make_option            (option_factory_tests_make_value.cpp)
::djinterp::test::block_spec option_factory_make_value_block();


// ===========================================================================
// SPEC PROVIDER   (spec face - the runner hands this to run_module)
// ===========================================================================
#ifdef DTEST_SPEC_MODE
inline ::djinterp::test::module_spec
option_factory_spec()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "option_factory.hpp";
    mod.descriptor =
        "key_carrier bridge + compile-time / runtime option factories";

    mod.blocks.push_back(tt::option_factory_carrier_block());     // I
    mod.blocks.push_back(tt::option_factory_make_type_block());   // II
    mod.blocks.push_back(tt::option_factory_make_value_block());  // III

    return mod;
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_OPTION_FACTORY_TESTS_
