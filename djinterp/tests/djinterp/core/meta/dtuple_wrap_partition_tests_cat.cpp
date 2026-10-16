/*******************************************************************************
* djinterp [test]                            dtuple_wrap_partition_tests_cat.cpp
*
*   Section I of dtuple_wrap_partition.hpp: internal::pwe_cat, the type-level
* concatenation of two std::tuples that the walk uses to stitch each emitted
* chunk / passthrough onto the tail result.  It has a single specialization
* (tuple + tuple); pinned here across non-empty and empty operands, with order
* preserved left-to-right.
*
*
* path:      /tests/djinterp/core/meta/dtuple_wrap_partition_tests_cat.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "dtuple_wrap_partition_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace pwp_test_types;

namespace pi = ::djinterp::internal;


// =========================================================================
// I.   NON-EMPTY OPERANDS  (order preserved)
// =========================================================================

static_assert(std::is_same<pi::pwe_cat<std::tuple<slot<0>>,
                                       std::tuple<slot<1>, slot<2>>>::type,
                           std::tuple<slot<0>, slot<1>, slot<2>>>::value,
              "cat: 1 + 2");
static_assert(std::is_same<pi::pwe_cat<std::tuple<slot<0>, slot<1>>,
                                       std::tuple<slot<2>, slot<3>>>::type,
                           std::tuple<slot<0>, slot<1>, slot<2>, slot<3>>>::value,
              "cat: 2 + 2");
static_assert(std::is_same<pi::pwe_cat<std::tuple<slot<1>, slot<2>>,
                                       std::tuple<slot<3>>>::type,
                           std::tuple<slot<1>, slot<2>, slot<3>>>::value,
              "cat: order preserved (not reordered)");
// heterogeneous element types (a wrapped chunk beside a passthrough)
static_assert(std::is_same<pi::pwe_cat<std::tuple<group<slot<0>, slot<1>>>,
                                       std::tuple<pass_a>>::type,
                           std::tuple<group<slot<0>, slot<1>>, pass_a>>::value,
              "cat: wrapped chunk + passthrough");


// =========================================================================
// II.  EMPTY OPERANDS
// =========================================================================

static_assert(std::is_same<pi::pwe_cat<std::tuple<>, std::tuple<slot<0>>>::type,
                           std::tuple<slot<0>>>::value,
              "cat: empty left");
static_assert(std::is_same<pi::pwe_cat<std::tuple<slot<0>>, std::tuple<>>::type,
                           std::tuple<slot<0>>>::value,
              "cat: empty right");
static_assert(std::is_same<pi::pwe_cat<std::tuple<>, std::tuple<>>::type,
                           std::tuple<>>::value,
              "cat: both empty");


// =========================================================================
// III. RUN-TIME MIRRORS
// =========================================================================

bool
tests_pwp_cat_basic()
{
    bool _ok = true;

    _ok = _ok && std::is_same<pi::pwe_cat<std::tuple<slot<0>>,
                                          std::tuple<slot<1>, slot<2>>>::type,
                              std::tuple<slot<0>, slot<1>, slot<2>>>::value;
    _ok = _ok && std::is_same<pi::pwe_cat<std::tuple<slot<1>, slot<2>>,
                                          std::tuple<slot<3>>>::type,
                              std::tuple<slot<1>, slot<2>, slot<3>>>::value;
    _ok = _ok && std::is_same<pi::pwe_cat<std::tuple<group<slot<0>, slot<1>>>,
                                          std::tuple<pass_a>>::type,
                              std::tuple<group<slot<0>, slot<1>>, pass_a>>::value;

    return _ok;
}

bool
tests_pwp_cat_empty()
{
    bool _ok = true;

    _ok = _ok && std::is_same<pi::pwe_cat<std::tuple<>, std::tuple<slot<0>>>::type,
                              std::tuple<slot<0>>>::value;
    _ok = _ok && std::is_same<pi::pwe_cat<std::tuple<slot<0>>, std::tuple<>>::type,
                              std::tuple<slot<0>>>::value;
    _ok = _ok && std::is_same<pi::pwe_cat<std::tuple<>, std::tuple<>>::type,
                              std::tuple<>>::value;

    return _ok;
}


NS_END  // testing
NS_END  // djinterp
