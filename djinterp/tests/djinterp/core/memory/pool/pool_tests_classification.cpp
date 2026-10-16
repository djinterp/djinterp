/*******************************************************************************
* djinterp [testing]                               pool_tests_classification.cpp
*
* pool classification: the stability / release / generational trait matrix.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_classification.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "pool_tests.hpp"

NS_DJINTERP
NS_TESTING
namespace dj = ::djinterp;

// tests_pointer_stability
//   is_pointer_stable_pool tracks the BLOCK policy: chunked pools keep pointers
// stable, the contiguous pool does not.
bool
tests_pointer_stability()
{
    D_PO_CHECK(dj::is_pointer_stable_pool<pool_free_list>::value);
    D_PO_CHECK(dj::is_pointer_stable_pool<pool_monotonic>::value);
    D_PO_CHECK(dj::is_pointer_stable_pool<pool_generational>::value);
    D_PO_CHECK(!dj::is_pointer_stable_pool<pool_contiguous>::value);
    return true;
}

// tests_individual_release
//   supports_individual_release tracks the RELEASE policy: free-list, contiguous
// and generational pools reuse released slots; the monotonic pool does not.
bool
tests_individual_release()
{
    D_PO_CHECK(dj::supports_individual_release<pool_free_list>::value);
    D_PO_CHECK(dj::supports_individual_release<pool_contiguous>::value);
    D_PO_CHECK(dj::supports_individual_release<pool_generational>::value);
    D_PO_CHECK(!dj::supports_individual_release<pool_monotonic>::value);
    return true;
}

// tests_monotonic_classification
//   is_monotonic_pool is exactly "a pool_resource without individual release" -
// the monotonic pool only - and matches that definition.
bool
tests_monotonic_classification()
{
    D_PO_CHECK(dj::is_monotonic_pool<pool_monotonic>::value);
    D_PO_CHECK(!dj::is_monotonic_pool<pool_free_list>::value);
    D_PO_CHECK(!dj::is_monotonic_pool<pool_contiguous>::value);
    D_PO_CHECK(!dj::is_monotonic_pool<pool_generational>::value);

    D_PO_CHECK(dj::is_monotonic_pool<pool_monotonic>::value ==
               (dj::is_pool_resource<pool_monotonic>::value &&
                !dj::supports_individual_release<pool_monotonic>::value));
    return true;
}

// tests_generational_sweep
//   has_generational_sweep is the generational pool only.
bool
tests_generational_sweep()
{
    D_PO_CHECK(dj::has_generational_sweep<pool_generational>::value);
    D_PO_CHECK(!dj::has_generational_sweep<pool_free_list>::value);
    D_PO_CHECK(!dj::has_generational_sweep<pool_monotonic>::value);
    D_PO_CHECK(!dj::has_generational_sweep<pool_contiguous>::value);
    return true;
}

NS_END  // testing
NS_END  // djinterp
