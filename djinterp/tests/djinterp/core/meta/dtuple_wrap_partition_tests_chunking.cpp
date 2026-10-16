/*******************************************************************************
* djinterp [test]                       dtuple_wrap_partition_tests_chunking.cpp
*
*   Section II of dtuple_wrap_partition.hpp: the core chunk-and-wrap behavior
* over runs of consecutive NON-passthrough slots (no passthroughs here -- that
* is section III).  This drives the walk's step, its schema-exhausted base, and
* the dispatch accumulate/complete branches:
*
*     - _N == 1: every slot completes its chunk immediately (dispatch (3) with
*       no prior accumulate) -> each slot wrapped singly;
*     - _N == 2 / 3 / 4: accumulate the first _N-1 slots (dispatch (2)) then
*       complete on the _N-th (dispatch (3)); depth grows with _N;
*     - multiple back-to-back chunks over one run;
*     - slot ORDER is preserved inside each wrapped chunk and across chunks.
*
*
* path:      /tests/djinterp/core/meta/dtuple_wrap_partition_tests_chunking.cpp
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
// I.   _N == 1  (each slot wrapped singly; immediate completion)
// =========================================================================

static_assert(std::is_same<partition_grouped<1, slot<0>>,
                           std::tuple<group<slot<0>>>>::value,
              "N=1: single slot");
static_assert(std::is_same<partition_grouped<1, slot<0>, slot<1>, slot<2>>,
                           std::tuple<group<slot<0>>, group<slot<1>>, group<slot<2>>>>::value,
              "N=1: three slots -> three singleton wraps");


// =========================================================================
// II.  _N == 2  (accumulate 1, complete on 2)
// =========================================================================

static_assert(std::is_same<partition_grouped<2, slot<0>, slot<1>>,
                           std::tuple<group<slot<0>, slot<1>>>>::value,
              "N=2: one chunk");
static_assert(std::is_same<partition_grouped<2, slot<0>, slot<1>, slot<2>, slot<3>>,
                           std::tuple<group<slot<0>, slot<1>>, group<slot<2>, slot<3>>>>::value,
              "N=2: two chunks");


// =========================================================================
// III. _N == 3  (accumulate 1,2, complete on 3)
// =========================================================================

static_assert(std::is_same<partition_grouped<3, slot<0>, slot<1>, slot<2>>,
                           std::tuple<group<slot<0>, slot<1>, slot<2>>>>::value,
              "N=3: one chunk");
static_assert(std::is_same<partition_grouped<3, slot<0>, slot<1>, slot<2>,
                                                slot<3>, slot<4>, slot<5>>,
                           std::tuple<group<slot<0>, slot<1>, slot<2>>,
                                      group<slot<3>, slot<4>, slot<5>>>>::value,
              "N=3: two chunks");


// =========================================================================
// IV.  _N == 4  (accumulate 1,2,3, complete on 4)
// =========================================================================

static_assert(std::is_same<partition_grouped<4, slot<0>, slot<1>, slot<2>, slot<3>>,
                           std::tuple<group<slot<0>, slot<1>, slot<2>, slot<3>>>>::value,
              "N=4: one chunk");
static_assert(std::is_same<partition_grouped<4, slot<0>, slot<1>, slot<2>, slot<3>,
                                                slot<4>, slot<5>, slot<6>, slot<7>>,
                           std::tuple<group<slot<0>, slot<1>, slot<2>, slot<3>>,
                                      group<slot<4>, slot<5>, slot<6>, slot<7>>>>::value,
              "N=4: two chunks");


// =========================================================================
// V.   MANY CHUNKS
// =========================================================================

static_assert(std::is_same<partition_grouped<2, slot<0>, slot<1>, slot<2>, slot<3>,
                                                slot<4>, slot<5>>,
                           std::tuple<group<slot<0>, slot<1>>,
                                      group<slot<2>, slot<3>>,
                                      group<slot<4>, slot<5>>>>::value,
              "N=2: three chunks");


// =========================================================================
// VI.  ORDER PRESERVATION
// =========================================================================
//   slots are wrapped in the order given (never sorted / reversed)

static_assert(std::is_same<partition_grouped<3, slot<5>, slot<3>, slot<9>>,
                           std::tuple<group<slot<5>, slot<3>, slot<9>>>>::value,
              "order: slots keep their given order inside a chunk");
static_assert(std::is_same<partition_grouped<2, slot<9>, slot<8>, slot<1>, slot<0>>,
                           std::tuple<group<slot<9>, slot<8>>, group<slot<1>, slot<0>>>>::value,
              "order: preserved across chunks");


// =========================================================================
// VII. RUN-TIME MIRRORS
// =========================================================================

bool
tests_pwp_chunking_n1()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<1, slot<0>>, std::tuple<group<slot<0>>>>::value;
    _ok = _ok && std::is_same<partition_grouped<1, slot<0>, slot<1>, slot<2>>,
                              std::tuple<group<slot<0>>, group<slot<1>>, group<slot<2>>>>::value;
    return _ok;
}

bool
tests_pwp_chunking_n2()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<2, slot<0>, slot<1>>,
                              std::tuple<group<slot<0>, slot<1>>>>::value;
    _ok = _ok && std::is_same<partition_grouped<2, slot<0>, slot<1>, slot<2>, slot<3>>,
                              std::tuple<group<slot<0>, slot<1>>, group<slot<2>, slot<3>>>>::value;
    return _ok;
}

bool
tests_pwp_chunking_n3()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<3, slot<0>, slot<1>, slot<2>>,
                              std::tuple<group<slot<0>, slot<1>, slot<2>>>>::value;
    _ok = _ok && std::is_same<partition_grouped<3, slot<0>, slot<1>, slot<2>,
                                                   slot<3>, slot<4>, slot<5>>,
                              std::tuple<group<slot<0>, slot<1>, slot<2>>,
                                         group<slot<3>, slot<4>, slot<5>>>>::value;
    return _ok;
}

bool
tests_pwp_chunking_n4()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<4, slot<0>, slot<1>, slot<2>, slot<3>>,
                              std::tuple<group<slot<0>, slot<1>, slot<2>, slot<3>>>>::value;
    return _ok;
}

bool
tests_pwp_chunking_multi()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<2, slot<0>, slot<1>, slot<2>, slot<3>,
                                                   slot<4>, slot<5>>,
                              std::tuple<group<slot<0>, slot<1>>,
                                         group<slot<2>, slot<3>>,
                                         group<slot<4>, slot<5>>>>::value;
    return _ok;
}

bool
tests_pwp_chunking_order()
{
    bool _ok = true;
    _ok = _ok && std::is_same<partition_grouped<3, slot<5>, slot<3>, slot<9>>,
                              std::tuple<group<slot<5>, slot<3>, slot<9>>>>::value;
    _ok = _ok && std::is_same<partition_grouped<2, slot<9>, slot<8>, slot<1>, slot<0>>,
                              std::tuple<group<slot<9>, slot<8>>, group<slot<1>, slot<0>>>>::value;
    return _ok;
}


NS_END  // testing
NS_END  // djinterp
