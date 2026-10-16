/*******************************************************************************
* djinterp [testing]                                      pool_tests_runtime.cpp
*
* pool runtime: pool_resource acquire / release / reset behaviour.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_runtime.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "pool_tests.hpp"

NS_DJINTERP
NS_TESTING
namespace dj = ::djinterp;

// tests_pool_acquire_release
//   free-list pool: acquire draws distinct live slots and raises size, release
// lowers the live count, and reset returns everything to zero.
bool
tests_pool_acquire_release()
{
    pool_free_list p;
    D_PO_CHECK(p.size() == 0u);

    void* a = p.acquire();
    void* b = p.acquire();
    D_PO_CHECK(a != nullptr);
    D_PO_CHECK(b != nullptr);
    D_PO_CHECK(a != b);
    D_PO_CHECK(p.size() == 2u);

    p.release(a);
    D_PO_CHECK(p.size() == 1u);

    p.reset();
    D_PO_CHECK(p.size() == 0u);
    return true;
}

// tests_pool_monotonic_reset
//   monotonic pool: acquire raises the live count; the pool reclaims only on
// reset, which returns the count to zero.
bool
tests_pool_monotonic_reset()
{
    pool_monotonic m;
    D_PO_CHECK(m.size() == 0u);

    void* x = m.acquire();
    void* y = m.acquire();
    D_PO_CHECK(x != nullptr);
    D_PO_CHECK(y != nullptr);
    D_PO_CHECK(x != y);
    D_PO_CHECK(m.size() == 2u);

    m.reset();
    D_PO_CHECK(m.size() == 0u);
    return true;
}

NS_END  // testing
NS_END  // djinterp
