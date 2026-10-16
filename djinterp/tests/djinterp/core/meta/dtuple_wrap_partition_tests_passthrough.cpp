/*******************************************************************************
* djinterp [test]                    dtuple_wrap_partition_tests_passthrough.cpp
*
*   Section III of dtuple_wrap_partition.hpp: passthrough survival -- types
* satisfying _IsPassthrough are emitted UNWRAPPED at their positions, which sit
* at chunk boundaries.  This drives dispatch specialization (1) (head is a
* passthrough, chunk empty).  Coverage:
*     - a passthrough at the START, at the END, and BETWEEN two chunks;
*     - CONSECUTIVE passthroughs at one boundary;
*     - a schema of ONLY passthroughs (no chunk ever forms);
*     - passthroughs under _N == 1, where a boundary passthrough hits the
*       dispatch (1) case with _ChunkCompletes == true (the (0 + 1 == 1) edge).
*
*
* path:      /tests/djinterp/core/meta/dtuple_wrap_partition_tests_passthrough.cpp
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
// I.   PASSTHROUGH AT START
// =========================================================================

static_assert(std::is_same<partition_grouped<3, pass_a, slot<0>, slot<1>, slot<2>>,
                           std::tuple<pass_a, group<slot<0>, slot<1>, slot<2>>>>::value,
              "passthrough at start");
static_assert(std::is_same<partition_grouped<3, pass_a, pass_b, slot<0>, slot<1>, slot<2>>,
                           std::tuple<pass_a, pass_b, group<slot<0>, slot<1>, slot<2>>>>::value,
              "two passthroughs at start");


// =========================================================================
// II.  PASSTHROUGH AT END
// =========================================================================

static_assert(std::is_same<partition_grouped<3, slot<0>, slot<1>, slot<2>, pass_a>,
                           std::tuple<group<slot<0>, slot<1>, slot<2>>, pass_a>>::value,
              "passthrough at end");
static_assert(std::is_same<partition_grouped<2, slot<0>, slot<1>, pass_a, pass_b>,
                           std::tuple<group<slot<0>, slot<1>>, pass_a, pass_b>>::value,
              "two passthroughs at end");


// =========================================================================
// III. PASSTHROUGH BETWEEN CHUNKS
// =========================================================================

static_assert(std::is_same<partition_grouped<2, slot<0>, slot<1>, pass_a, slot<2>, slot<3>>,
                           std::tuple<group<slot<0>, slot<1>>, pass_a, group<slot<2>, slot<3>>>>::value,
              "passthrough between two chunks");
// start + between + end, all in one schema
static_assert(std::is_same<partition_grouped<2, pass_a, slot<0>, slot<1>, pass_b,
                                                slot<2>, slot<3>, pass_c>,
                           std::tuple<pass_a, group<slot<0>, slot<1>>, pass_b,
                                      group<slot<2>, slot<3>>, pass_c>>::value,
              "passthroughs at start, between, and end together");


// =========================================================================
// IV.  CONSECUTIVE PASSTHROUGHS
// =========================================================================

static_assert(std::is_same<partition_grouped<2, slot<0>, slot<1>, pass_a, pass_b,
                                                slot<2>, slot<3>>,
                           std::tuple<group<slot<0>, slot<1>>, pass_a, pass_b,
                                      group<slot<2>, slot<3>>>>::value,
              "consecutive passthroughs at a boundary");
static_assert(std::is_same<partition_grouped<3, pass_a, pass_b, pass_c,
                                                slot<0>, slot<1>, slot<2>>,
                           std::tuple<pass_a, pass_b, pass_c,
                                      group<slot<0>, slot<1>, slot<2>>>>::value,
              "three consecutive leading passthroughs");


// =========================================================================
// V.   ONLY PASSTHROUGHS  (no chunk ever forms)
// =========================================================================

static_assert(std::is_same<partition_grouped<3, pass_a>,
                           std::tuple<pass_a>>::value,
              "only: single passthrough");
static_assert(std::is_same<partition_grouped<3, pass_a, pass_b, pass_c>,
                           std::tuple<pass_a, pass_b, pass_c>>::value,
              "only: several passthroughs (N irrelevant, no chunk)");


// =========================================================================
// VI.  PASSTHROUGHS UNDER _N == 1  (_ChunkCompletes == true on the boundary)
// =========================================================================

static_assert(std::is_same<partition_grouped<1, pass_a, slot<0>, pass_b, slot<1>>,
                           std::tuple<pass_a, group<slot<0>>, pass_b, group<slot<1>>>>::value,
              "N=1: passthroughs interleaved with singleton wraps");
static_assert(std::is_same<partition_grouped<1, slot<0>, pass_a, pass_b, slot<1>>,
                           std::tuple<group<slot<0>>, pass_a, pass_b, group<slot<1>>>>::value,
              "N=1: consecutive passthroughs between singleton wraps");


// =========================================================================
// VII. RUN-TIME MIRRORS
// =========================================================================

bool
tests_pwp_passthrough_start()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<3, pass_a, slot<0>, slot<1>, slot<2>>,
                              std::tuple<pass_a, group<slot<0>, slot<1>, slot<2>>>>::value;
    _ok = _ok && std::is_same<partition_grouped<3, pass_a, pass_b, slot<0>, slot<1>, slot<2>>,
                              std::tuple<pass_a, pass_b, group<slot<0>, slot<1>, slot<2>>>>::value;
    return _ok;
}

bool
tests_pwp_passthrough_end()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<3, slot<0>, slot<1>, slot<2>, pass_a>,
                              std::tuple<group<slot<0>, slot<1>, slot<2>>, pass_a>>::value;
    _ok = _ok && std::is_same<partition_grouped<2, slot<0>, slot<1>, pass_a, pass_b>,
                              std::tuple<group<slot<0>, slot<1>>, pass_a, pass_b>>::value;
    return _ok;
}

bool
tests_pwp_passthrough_between()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<2, slot<0>, slot<1>, pass_a, slot<2>, slot<3>>,
                              std::tuple<group<slot<0>, slot<1>>, pass_a,
                                         group<slot<2>, slot<3>>>>::value;
    _ok = _ok && std::is_same<partition_grouped<2, pass_a, slot<0>, slot<1>, pass_b,
                                                   slot<2>, slot<3>, pass_c>,
                              std::tuple<pass_a, group<slot<0>, slot<1>>, pass_b,
                                         group<slot<2>, slot<3>>, pass_c>>::value;
    return _ok;
}

bool
tests_pwp_passthrough_consecutive()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<2, slot<0>, slot<1>, pass_a, pass_b,
                                                   slot<2>, slot<3>>,
                              std::tuple<group<slot<0>, slot<1>>, pass_a, pass_b,
                                         group<slot<2>, slot<3>>>>::value;
    return _ok;
}

bool
tests_pwp_passthrough_only()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<3, pass_a>, std::tuple<pass_a>>::value;
    _ok = _ok && std::is_same<partition_grouped<3, pass_a, pass_b, pass_c>,
                              std::tuple<pass_a, pass_b, pass_c>>::value;
    return _ok;
}

bool
tests_pwp_passthrough_n1()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<1, pass_a, slot<0>, pass_b, slot<1>>,
                              std::tuple<pass_a, group<slot<0>>, pass_b, group<slot<1>>>>::value;
    _ok = _ok && std::is_same<partition_grouped<1, slot<0>, pass_a, pass_b, slot<1>>,
                              std::tuple<group<slot<0>>, pass_a, pass_b, group<slot<1>>>>::value;
    return _ok;
}


NS_END  // testing
NS_END  // djinterp
