/*******************************************************************************
* djinterp [test]                    dtuple_wrap_partition_tests_integration.cpp
*
*   Section V of dtuple_wrap_partition.hpp: the partition_wrap_except_t alias,
* the engine's agnosticism to the wrapper it is handed, and end-to-end runs on
* longer mixed schemas.
*     - partition_wrap_except_t mirrors partition_wrap_except<...>::type for
*       both the pack and single-tuple forms;
*     - a different _Wrap (bundle) is used verbatim, and yields a result
*       distinct from the same schema wrapped with group;
*     - long schemas interleaving several chunks with passthroughs at various
*       boundaries resolve as expected, for _N == 2 and _N == 3.
*
*
* path:      /tests/djinterp/core/meta/dtuple_wrap_partition_tests_integration.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "dtuple_wrap_partition_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace pwp_test_types;


// =========================================================================
// I.   ALIAS CONSISTENCY  (partition_wrap_except_t == ::type)
// =========================================================================

static_assert(std::is_same<partition_wrap_except_t<group, 3, is_pass,
                                                   slot<0>, slot<1>, slot<2>>,
                           partition_wrap_except<group, 3, is_pass,
                                                 slot<0>, slot<1>, slot<2>>::type>::value,
              "alias == ::type (pack form)");
static_assert(std::is_same<partition_wrap_except_t<group, 2, is_pass,
                                                   std::tuple<slot<0>, slot<1>>>,
                           partition_wrap_except<group, 2, is_pass,
                                                 std::tuple<slot<0>, slot<1>>>::type>::value,
              "alias == ::type (single-tuple form)");


// =========================================================================
// II.  ARBITRARY WRAPPER
// =========================================================================

static_assert(std::is_same<partition_wrap_except_t<bundle, 2, is_pass,
                                                   slot<0>, slot<1>, pass_a, slot<2>, slot<3>>,
                           std::tuple<bundle<slot<0>, slot<1>>, pass_a,
                                      bundle<slot<2>, slot<3>>>>::value,
              "arbitrary wrapper: engine wraps with the handed template (bundle)");

// same schema, different wrapper -> different result
static_assert(!std::is_same<partition_wrap_except_t<bundle, 2, is_pass, slot<0>, slot<1>>,
                            partition_wrap_except_t<group,  2, is_pass, slot<0>, slot<1>>>::value,
              "arbitrary wrapper: bundle result differs from group result");


// =========================================================================
// III. COMPLEX MIXED SCHEMAS
// =========================================================================

// N=2: leading passthrough, two adjacent chunks, two adjacent passthroughs,
// a final chunk, and a trailing passthrough.
static_assert(std::is_same<
                  partition_grouped<2, pass_a,
                                       slot<0>, slot<1>, slot<2>, slot<3>,
                                       pass_b, pass_c,
                                       slot<4>, slot<5>,
                                       pass_a>,
                  std::tuple<pass_a,
                             group<slot<0>, slot<1>>, group<slot<2>, slot<3>>,
                             pass_b, pass_c,
                             group<slot<4>, slot<5>>,
                             pass_a>>::value,
              "complex N=2 mixed schema");

// N=3: three chunks with a single interior passthrough.
static_assert(std::is_same<
                  partition_grouped<3, slot<0>, slot<1>, slot<2>,
                                       pass_a,
                                       slot<3>, slot<4>, slot<5>,
                                       slot<6>, slot<7>, slot<8>>,
                  std::tuple<group<slot<0>, slot<1>, slot<2>>,
                             pass_a,
                             group<slot<3>, slot<4>, slot<5>>,
                             group<slot<6>, slot<7>, slot<8>>>>::value,
              "complex N=3 mixed schema");


// =========================================================================
// IV.  RUN-TIME MIRRORS
// =========================================================================

bool
tests_pwp_integration_alias()
{
    bool _ok = true;

    _ok = _ok && std::is_same<partition_wrap_except_t<group, 3, is_pass,
                                                      slot<0>, slot<1>, slot<2>>,
                              partition_wrap_except<group, 3, is_pass,
                                                    slot<0>, slot<1>, slot<2>>::type>::value;
    _ok = _ok && std::is_same<partition_wrap_except_t<group, 2, is_pass,
                                                      std::tuple<slot<0>, slot<1>>>,
                              partition_wrap_except<group, 2, is_pass,
                                                    std::tuple<slot<0>, slot<1>>>::type>::value;
    return _ok;
}

bool
tests_pwp_integration_wrapper_arbitrary()
{
    bool _ok = true;

    _ok = _ok && std::is_same<partition_wrap_except_t<bundle, 2, is_pass,
                                                      slot<0>, slot<1>, pass_a, slot<2>, slot<3>>,
                              std::tuple<bundle<slot<0>, slot<1>>, pass_a,
                                         bundle<slot<2>, slot<3>>>>::value;
    _ok = _ok && !std::is_same<partition_wrap_except_t<bundle, 2, is_pass, slot<0>, slot<1>>,
                               partition_wrap_except_t<group,  2, is_pass, slot<0>, slot<1>>>::value;
    return _ok;
}

bool
tests_pwp_integration_complex()
{
    bool _ok = true;

    _ok = _ok && std::is_same<
                     partition_grouped<2, pass_a,
                                          slot<0>, slot<1>, slot<2>, slot<3>,
                                          pass_b, pass_c,
                                          slot<4>, slot<5>,
                                          pass_a>,
                     std::tuple<pass_a,
                                group<slot<0>, slot<1>>, group<slot<2>, slot<3>>,
                                pass_b, pass_c,
                                group<slot<4>, slot<5>>,
                                pass_a>>::value;

    _ok = _ok && std::is_same<
                     partition_grouped<3, slot<0>, slot<1>, slot<2>,
                                          pass_a,
                                          slot<3>, slot<4>, slot<5>,
                                          slot<6>, slot<7>, slot<8>>,
                     std::tuple<group<slot<0>, slot<1>, slot<2>>,
                                pass_a,
                                group<slot<3>, slot<4>, slot<5>>,
                                group<slot<6>, slot<7>, slot<8>>>>::value;
    return _ok;
}


NS_END  // testing
NS_END  // djinterp
