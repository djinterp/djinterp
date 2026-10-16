/*******************************************************************************
* djinterp [testing]                         pool_memory_strategy_tests_pool.cpp
*
* Section: the pool-backed element strategy.  *** OPT-IN via D_MS_TEST_POOL_LAYER ***
*   pool_memory_strategy.hpp - pool_memory_strategy<Pool>,
*                              is_pool_memory_strategy, pool_memory_strategy_c,
*                              make_pool_strategy
*
*   This section is GATED behind D_MS_TEST_POOL_LAYER and compiles to an EMPTY
* translation unit unless that macro is defined.  The gate is not about the
* strategy adapter (which now compiles freely against the standalone
* pool_traits.hpp) but about the RUNTIME tests below, which instantiate a real
* pool_resource and therefore pull in pool.hpp - and pool.hpp depends on the
* container buffer layer (buffer.hpp) for its default growth policy.  Keeping
* the gate lets the core memory-strategy suite build without the pool/buffer
* subtree; enabling it (see the CMake DEFINES line) activates real pool-backed
* coverage against the actual pool layer.
*
*   The tests exercise the ADAPTER, not the pool: that pool_memory_strategy
* presents the pool under the canonical element surface, forwards the pool's
* descriptive constants correctly (pointer-stability from the block policy;
* release and generational from the release policy), and maps allocate(1) /
* deallocate(p,1) onto acquire() / release() with an operator-new fallback for
* n > 1.  pool_resource itself is covered by its own suite.  To keep growth
* deterministic and independent of buffer.hpp tuning, the pools here use local
* exact- and no-growth policies.
*
*
* path:      /tests/djinterp/core/memory/pool_memory_strategy_tests_pool.cpp
*                                            memory_strategy_tests_pool.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

#if defined(D_MS_TEST_POOL_LAYER)

// std
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
// djinterp
#include "pool_memory_strategy_tests.hpp"
#include "../../../../inc/djinterp/core/memory/pool/pool.hpp"
#include "../../../../inc/djinterp/core/memory/pool_memory_strategy.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // ms_pool_exact_growth
    //   struct: grows to exactly the requested capacity, deterministically.
    // Keeps the pool's growth independent of buffer.hpp's exponential tuning so
    // the adapter tests do not couple to it.
    struct ms_pool_exact_growth
    {
        static constexpr bool can_grow = true;

        static constexpr std::size_t
        compute(
            std::size_t /*_current*/,
            std::size_t _requested
        ) noexcept
        {
            return _requested;
        }
    };

    // ms_pool_no_growth
    //   struct: refuses growth, so a full pool's acquire() returns nullptr -
    // used to drive the strategy's bad_alloc path.
    struct ms_pool_no_growth
    {
        static constexpr bool can_grow = false;

        static constexpr std::size_t
        compute(
            std::size_t _current,
            std::size_t /*_requested*/
        ) noexcept
        {
            return _current;
        }
    };

    // the four pool shapes, one per descriptive dimension the adapter forwards
    using ms_pool_free_list =
        pool_resource<int, free_list_release_policy,
                      chunked_block_policy<8>, ms_pool_exact_growth>;
    using ms_pool_monotonic =
        pool_resource<int, monotonic_release_policy,
                      chunked_block_policy<8>, ms_pool_exact_growth>;
    using ms_pool_contiguous =
        pool_resource<int, free_list_release_policy,
                      contiguous_block_policy, ms_pool_exact_growth>;
    using ms_pool_generational =
        pool_resource<int, generational_release_policy,
                      chunked_block_policy<8>, ms_pool_exact_growth>;
    using ms_pool_bounded =
        pool_resource<int, free_list_release_policy,
                      chunked_block_policy<8>, ms_pool_no_growth>;
}


/*
tests_pool_strategy_contract
  pool_memory_strategy<Pool>: the descriptive surface it presents.
  Tests the following:
  - value_type is the pool's element type, and the kind is always dynamic (a
    pool is dynamic storage regardless of its policies)
  - it is a full element strategy, a member, and allocating
  - it FORWARDS the pool's policy constants, dimension by dimension:
      * pointer-stability from the BLOCK policy - chunked pools are stable,
        contiguous pools are not
      * individual-release from the RELEASE policy - free-list/generational
        release, monotonic does not
      * generational from the RELEASE policy - only the generational pool
  - the derived refinements follow: a monotonic-release pool yields a MONOTONIC
    strategy (allocating and not individually releasing), while a free-list pool
    does not
  - it is a non-owning handle: copyable
*/
bool
tests_pool_strategy_contract()
{

    using ps_fl = pool_memory_strategy<ms_pool_free_list>;
    using ps_mo = pool_memory_strategy<ms_pool_monotonic>;
    using ps_ct = pool_memory_strategy<ms_pool_contiguous>;
    using ps_gn = pool_memory_strategy<ms_pool_generational>;

    // --- value_type and the always-dynamic kind ---
    D_STRATEGY_CHECK((std::is_same<ps_fl::value_type, int>::value));
    D_STRATEGY_CHECK(ps_fl::strategy_storage_kind == storage_kind::dynamic_storage);
    D_STRATEGY_CHECK(ps_mo::strategy_storage_kind == storage_kind::dynamic_storage);
    D_STRATEGY_CHECK(ps_ct::strategy_storage_kind == storage_kind::dynamic_storage);

    // --- element strategy, member, allocating, dynamic ---
    D_STRATEGY_CHECK(is_element_strategy<ps_fl>::value);
    D_STRATEGY_CHECK(!is_byte_strategy<ps_fl>::value);
    D_STRATEGY_CHECK(is_memory_strategy<ps_fl>::value);
    D_STRATEGY_CHECK(is_allocating_strategy<ps_fl>::value);
    D_STRATEGY_CHECK(is_dynamic_strategy<ps_fl>::value);

    // --- pointer-stability forwarded from the BLOCK policy ---
    D_STRATEGY_CHECK(is_pointer_stable_strategy<ps_fl>::value);   // chunked
    D_STRATEGY_CHECK(is_pointer_stable_strategy<ps_mo>::value);   // chunked
    D_STRATEGY_CHECK(!is_pointer_stable_strategy<ps_ct>::value);  // contiguous

    // --- individual-release forwarded from the RELEASE policy ---
    D_STRATEGY_CHECK(supports_individual_release_strategy<ps_fl>::value);
    D_STRATEGY_CHECK(!supports_individual_release_strategy<ps_mo>::value);
    D_STRATEGY_CHECK(supports_individual_release_strategy<ps_gn>::value);

    // --- generational forwarded from the RELEASE policy ---
    D_STRATEGY_CHECK(!is_generational_strategy<ps_fl>::value);
    D_STRATEGY_CHECK(is_generational_strategy<ps_gn>::value);

    // --- derived refinement: monotonic-release pool -> monotonic strategy ---
    D_STRATEGY_CHECK(is_monotonic_strategy<ps_mo>::value);
    D_STRATEGY_CHECK(!is_monotonic_strategy<ps_fl>::value);
    D_STRATEGY_CHECK(!is_monotonic_strategy<ps_ct>::value);

    // --- non-owning handle: copyable ---
    D_STRATEGY_CHECK(std::is_copy_constructible<ps_fl>::value);

    // --- cv-ref agnostic through the composites ---
    D_STRATEGY_CHECK(is_element_strategy<const ps_fl&>::value);
    D_STRATEGY_CHECK(is_monotonic_strategy<const ps_mo&>::value);
    D_STRATEGY_CHECK(is_pointer_stable_strategy<ps_ct&&>::value == false);

    return true;
}


/*
tests_pool_strategy_runtime
  pool_memory_strategy<Pool>: acquire/release mapping and the fallback path.
  Tests the following:
  - resource() returns the wrapped pool
  - allocate(1) draws a slot from the pool (pool size rises by one), and the
    slot is writable; a second allocate yields a distinct slot
  - deallocate(p, 1) returns the slot to the pool (size falls)
  - allocate(n) with n > 1 does NOT touch the pool (operator-new fallback), and
    deallocate(p, n) frees it without disturbing pool size
  - on a monotonic-release pool, release still decrements the live count even
    though storage is not reclaimed until reset
  - a bounded (non-growable) pool at capacity makes allocate(1) throw bad_alloc
*/
bool
tests_pool_strategy_runtime()
{

    // --- free-list pool: individual release round-trips ---
    ms_pool_free_list pool;
    pool_memory_strategy<ms_pool_free_list> s(pool);

    D_STRATEGY_CHECK(s.resource() == &pool);

    int* a = s.allocate(1);
    D_STRATEGY_CHECK(a != nullptr);
    D_STRATEGY_CHECK(pool.size() == 1);
    *a = 42;
    D_STRATEGY_CHECK(*a == 42);

    int* b = s.allocate(1);
    D_STRATEGY_CHECK(b != nullptr);
    D_STRATEGY_CHECK(a != b);
    D_STRATEGY_CHECK(pool.size() == 2);

    s.deallocate(a, 1);
    D_STRATEGY_CHECK(pool.size() == 1);
    s.deallocate(b, 1);
    D_STRATEGY_CHECK(pool.size() == 0);

    // --- n > 1 bypasses the pool (operator new) ---
    int* block = s.allocate(4);
    D_STRATEGY_CHECK(block != nullptr);
    D_STRATEGY_CHECK(pool.size() == 0);
    block[0] = 1;
    block[3] = 4;
    D_STRATEGY_CHECK(block[0] == 1 && block[3] == 4);
    s.deallocate(block, 4);
    D_STRATEGY_CHECK(pool.size() == 0);

    // --- monotonic pool: release decrements the live count ---
    ms_pool_monotonic mpool;
    pool_memory_strategy<ms_pool_monotonic> ms(mpool);
    int* m1 = ms.allocate(1);
    int* m2 = ms.allocate(1);
    D_STRATEGY_CHECK(mpool.size() == 2);
    ms.deallocate(m1, 1);
    D_STRATEGY_CHECK(mpool.size() == 1);
    ms.deallocate(m2, 1);
    D_STRATEGY_CHECK(mpool.size() == 0);

    // --- bounded pool: exhaustion makes allocate(1) throw ---
    ms_pool_bounded bpool(2);   // capacity 2, cannot grow
    pool_memory_strategy<ms_pool_bounded> bs(bpool);
    int* x = bs.allocate(1);
    int* y = bs.allocate(1);
    D_STRATEGY_CHECK(x != nullptr && y != nullptr);
    bool threw = false;
    try
    {
        bs.allocate(1);
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }
    D_STRATEGY_CHECK(threw);
    bs.deallocate(x, 1);
    bs.deallocate(y, 1);

    return true;
}


/*
tests_pool_strategy_recognition
  is_pool_memory_strategy / pool_memory_strategy_c.
  Tests the following:
  - a pool-backed strategy is recognized (it is an element strategy whose
    resource() returns a pool_resource)
  - a non-pool element strategy is NOT recognized (the static buffer, the
    allocator wrapper): the resource()/pool-resource term is what separates them
  - a plain type is not recognized
  - the concept agrees with the trait
  - recognition is cv-ref agnostic
*/
bool
tests_pool_strategy_recognition()
{

    using ps_fl = pool_memory_strategy<ms_pool_free_list>;

    // --- a pool-backed strategy is recognized ---
    D_STRATEGY_CHECK(is_pool_memory_strategy<ps_fl>::value);
    D_STRATEGY_CHECK(pool_memory_strategy_c<ps_fl>);

    // --- non-pool element strategies are not ---
    D_STRATEGY_CHECK(is_element_strategy<static_buffer_strategy<int, 8>>::value);
    D_STRATEGY_CHECK(!is_pool_memory_strategy<static_buffer_strategy<int, 8>>::value);
    D_STRATEGY_CHECK(
        !is_pool_memory_strategy<
            allocator_memory_strategy<std::allocator<int>>>::value);

    // --- a plain type is not ---
    D_STRATEGY_CHECK(!is_pool_memory_strategy<ms_plain_type>::value);
    D_STRATEGY_CHECK(!is_pool_memory_strategy<int>::value);

    // --- the concept agrees with the trait ---
    D_STRATEGY_CHECK(pool_memory_strategy_c<ps_fl>
                   == is_pool_memory_strategy_v<ps_fl>);
    D_STRATEGY_CHECK(pool_memory_strategy_c<static_buffer_strategy<int, 8>>
                   == is_pool_memory_strategy_v<static_buffer_strategy<int, 8>>);

    // --- cv-ref agnostic ---
    D_STRATEGY_CHECK(is_pool_memory_strategy<const ps_fl&>::value);
    D_STRATEGY_CHECK(is_pool_memory_strategy_v<ps_fl>
                   == is_pool_memory_strategy<ps_fl>::value);

    return true;
}


/*
tests_pool_strategy_factory
  make_pool_strategy: deduce the pool type and bind a strategy to it.
  Tests the following:
  - the return type is pool_memory_strategy over the cleaned pool type
  - the result is bound to the supplied pool (resource() matches) and is itself
    a recognized pool strategy
  - the bound strategy is operational: allocate(1)/deallocate(1) round-trips
    through the pool
*/
bool
tests_pool_strategy_factory()
{

    ms_pool_free_list pool;

    // --- return type over the cleaned pool ---
    auto s = make_pool_strategy(pool);
    D_STRATEGY_CHECK((std::is_same<decltype(s),
                   pool_memory_strategy<ms_pool_free_list>>::value));

    // --- bound to the supplied pool, and recognized ---
    D_STRATEGY_CHECK(s.resource() == &pool);
    D_STRATEGY_CHECK(is_pool_memory_strategy<decltype(s)>::value);

    // --- operational ---
    int* p = s.allocate(1);
    D_STRATEGY_CHECK(p != nullptr);
    D_STRATEGY_CHECK(pool.size() == 1);
    *p = 7;
    D_STRATEGY_CHECK(*p == 7);
    s.deallocate(p, 1);
    D_STRATEGY_CHECK(pool.size() == 0);

    return true;
}


NS_END  // testing
NS_END  // djinterp

#endif  // D_MS_TEST_POOL_LAYER
