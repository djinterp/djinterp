/*******************************************************************************
* djinterp [testing]                                     pool_tests_snapshot.cpp
*
* pool snapshot: the pool_class aggregate and the capacity/accounting flags.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_snapshot.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "pool_tests.hpp"

NS_DJINTERP
NS_TESTING
namespace dj = ::djinterp;

// tests_pool_class_matrix
//   pool_class collects the per-pool traits; its fields agree with the matrix
// across all four shapes.
bool
tests_pool_class_matrix()
{
    using fl = dj::pool_class<pool_free_list>;
    D_PO_CHECK(fl::is_pool);
    D_PO_CHECK(fl::pointer_stable);
    D_PO_CHECK(fl::individual_release);
    D_PO_CHECK(!fl::monotonic);
    D_PO_CHECK(!fl::generational);

    using mo = dj::pool_class<pool_monotonic>;
    D_PO_CHECK(mo::is_pool);
    D_PO_CHECK(mo::pointer_stable);
    D_PO_CHECK(!mo::individual_release);
    D_PO_CHECK(mo::monotonic);
    D_PO_CHECK(!mo::generational);

    using ct = dj::pool_class<pool_contiguous>;
    D_PO_CHECK(ct::is_pool);
    D_PO_CHECK(!ct::pointer_stable);
    D_PO_CHECK(ct::individual_release);
    D_PO_CHECK(!ct::monotonic);
    D_PO_CHECK(!ct::generational);

    using gn = dj::pool_class<pool_generational>;
    D_PO_CHECK(gn::is_pool);
    D_PO_CHECK(gn::pointer_stable);
    D_PO_CHECK(gn::individual_release);
    D_PO_CHECK(!gn::monotonic);
    D_PO_CHECK(gn::generational);
    return true;
}

// tests_pool_class_capacity
//   reset/reserve/accounting are flagged for a pool; pool_class over a non-pool
// reports is_pool false and no derived properties.
bool
tests_pool_class_capacity()
{
    using fl = dj::pool_class<pool_free_list>;
    D_PO_CHECK(fl::has_reset);
    D_PO_CHECK(fl::has_reserve);
    D_PO_CHECK(fl::has_accounting);

    using np = dj::pool_class<int>;
    D_PO_CHECK(!np::is_pool);
    D_PO_CHECK(!np::pointer_stable);
    D_PO_CHECK(!np::individual_release);
    D_PO_CHECK(!np::monotonic);
    D_PO_CHECK(!np::generational);
    return true;
}

NS_END  // testing
NS_END  // djinterp
