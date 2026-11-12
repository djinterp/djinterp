/*******************************************************************************
* djinterp [cpp][memory]                                   memory_tests_pool.cpp
*
* The memory suite, block "pool": pool.hpp's raw_pool, the typed pool<T>,
* and the generational policy's DPoolHandle<T>.
*
*
* path:      /tests/djinterp/cpp/memory/memory_tests_pool.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./memory_tests.hpp"  // corresponding header

// std
#include <cstring>
#include <utility>


NS_DJINTERP
NS_TESTING


/*
tests_memory_pool_raw
  raw_pool.
  Tests the following:
  - the slot is at least the size asked and aligned as asked
  - acquire gives distinct aligned slots; size counts the ones held
  - a released slot is handed out again
  - reserve makes room ahead; reset gives every slot back and keeps it
*/
bool
tests_memory_pool_raw()
{
    raw_pool p(24,
               16);
    void*    slots[40] = {};
    void*    again     = nullptr;

    D_TEST_REQUIRE(p.slot_size() >= 24);
    D_TEST_REQUIRE(p.slot_align() >= 16);
    D_TEST_REQUIRE(p.empty());

    // forty slots, each distinct and aligned
    for (int i = 0; i < 40; ++i)
    {
        slots[i] = p.acquire();
        D_TEST_REQUIRE(slots[i] != nullptr);
        D_TEST_REQUIRE(is_aligned(slots[i],
                                  16));
        std::memset(slots[i],
                    i,
                    24);
#if (D_INTERNAL_POOL_OWNS == 1)
        D_TEST_REQUIRE(p.owns(slots[i]));
#endif  // D_INTERNAL_POOL_OWNS
    }

    D_TEST_REQUIRE(slots[0] != slots[1]);
    D_TEST_REQUIRE(p.size() == 40);
    D_TEST_REQUIRE(p.capacity() >= 40);
    D_TEST_REQUIRE(p.available() == p.capacity() - p.size());
    D_TEST_REQUIRE(p.block_count() >= 1);
    D_TEST_REQUIRE(p.bytes_allocated() >= 40 * 24);
    D_TEST_REQUIRE( (p.utilization() > 0.0) && (p.utilization() <= 1.0) );

    D_TEST_REQUIRE(p.release_slot(slots[7]) == D_MEM_OK);
    D_TEST_REQUIRE(p.size() == 39);
    again = p.acquire();
    D_TEST_REQUIRE(again == slots[7]);

    D_TEST_REQUIRE(p.reserve(p.capacity() + 64) == D_MEM_OK);
    D_TEST_REQUIRE(p.available() >= 64);

    p.reset();
    D_TEST_REQUIRE(p.empty());
    D_TEST_REQUIRE(p.capacity() >= 40);

    return true;
}

/*
tests_memory_pool_refusals
  What a pool refuses.
  Tests the following:
  - releasing a pointer it never gave is refused as foreign
  - a second release of a slot is refused, where the check is built (as
    released, or as corrupt where guard bands are checked first)
  - a pool at its ceiling gives no more
  - a pool over the refusing source gives nothing
*/
bool
tests_memory_pool_refusals()
{
    ::dj_pool_config config = ::d_pool_config_default(32,
                                                      8);
    char             elsewhere[64] = {};
    void*            slot          = nullptr;

    config.slots_per_block = 4;
    config.max_slots       = 4;

    {
        raw_pool  p(config);
        DMemBlock block = {};

        slot = p.acquire();
        D_TEST_REQUIRE(slot != nullptr);
        D_TEST_REQUIRE(p.release_slot(elsewhere) == D_MEM_ERR_FOREIGN);

#if (D_INTERNAL_POOL_DOUBLE_FREE_CHECK == 1)
        D_TEST_REQUIRE(p.release_slot(slot) == D_MEM_OK);
        D_TEST_REQUIRE(p.release_slot(slot) != D_MEM_OK);
#   if (D_INTERNAL_MEM_REDZONE == 0)
        // with guard bands, the check of the bands comes first and reports a
        // freed slot as corrupt; without them, the free check names it
        D_TEST_REQUIRE(p.release_slot(slot) == D_MEM_ERR_RELEASED);
#   endif  // D_INTERNAL_MEM_REDZONE
#else
        D_TEST_REQUIRE(p.release_slot(slot) == D_MEM_OK);
#endif  // D_INTERNAL_POOL_DOUBLE_FREE_CHECK

        // fill it, then one more
        for (int i = 0; i < 4; ++i)
        {
            D_TEST_REQUIRE(p.acquire() != nullptr);
        }

        D_TEST_REQUIRE(p.acquire() == nullptr);
        D_TEST_REQUIRE(p.acquire_ex(block) == D_MEM_ERR_EXHAUSTED);
    }

#if (D_INTERNAL_MEM_SOURCE_NULL == 1)
    {
        pool<double> starved(DMemorySource::refusing());

        D_TEST_REQUIRE(starved.acquire() == nullptr);
        D_TEST_REQUIRE(starved.create(1.0) == nullptr);
    }
#endif  // D_INTERNAL_MEM_SOURCE_NULL

    return true;
}

/*
tests_memory_pool_objects
  pool<T>.
  Tests the following:
  - its slot geometry is T's, fixed at compile time
  - create forwards to T's constructor; destroy runs T's destructor and
    gives the slot back
  - destroying nullptr is harmless
  - a monotonic pool gives slots and takes none back one at a time
*/
bool
tests_memory_pool_objects()
{
    pool<DMemoryTestsCounted> p;
    const int                 before  = DMemoryTestsCounted::live();
    DMemoryTestsCounted*      a       = nullptr;
    DMemoryTestsCounted*      b       = nullptr;

    static_assert(pool<DMemoryTestsThing>::slot_bytes ==
                      sizeof(DMemoryTestsThing),
                  "pool<T>'s slot is T's size");
    static_assert(pool<DMemoryTestsThing>::slot_alignment ==
                      alignof(DMemoryTestsThing),
                  "pool<T>'s slot is T's alignment");
    D_TEST_REQUIRE(p.slot_size() >= sizeof(DMemoryTestsCounted));
    D_TEST_REQUIRE(p.slot_align() >= alignof(DMemoryTestsCounted));

    a = p.create(11);
    b = p.create(22);
    D_TEST_REQUIRE( (a != nullptr) && (b != nullptr) );
    D_TEST_REQUIRE( (a->m_value == 11) && (b->m_value == 22) );
    D_TEST_REQUIRE(DMemoryTestsCounted::live() == before + 2);
    D_TEST_REQUIRE(p.size() == 2);

    D_TEST_REQUIRE(p.destroy(a) == D_MEM_OK);
    D_TEST_REQUIRE(DMemoryTestsCounted::live() == before + 1);
    D_TEST_REQUIRE(p.size() == 1);
    D_TEST_REQUIRE(p.destroy(nullptr) == D_MEM_OK);
    D_TEST_REQUIRE(p.destroy(b) == D_MEM_OK);
    D_TEST_REQUIRE(DMemoryTestsCounted::live() == before);

    {
        pool<int, D_POOL_POLICY_MONOTONIC> bump;
        int*                               slot = bump.acquire();

        D_TEST_REQUIRE(slot != nullptr);
        D_TEST_REQUIRE(bump.acquire() != slot);
        D_TEST_REQUIRE(bump.release_slot(slot) != D_MEM_OK);
        D_TEST_REQUIRE(bump.size() == 2);
        bump.reset();
        D_TEST_REQUIRE(bump.empty());
    }

    return true;
}

/*
tests_memory_pool_handles
  The generational policy's handles.
  Tests the following:
  - a handle made by create_handle resolves to its object, and is live
  - once destroyed it resolves to nothing, is not live, and a second destroy
    is refused as stale
  - the slot's next occupant has a different handle at the same address
  - the null handle is null and never resolves
*/
bool
tests_memory_pool_handles()
{
#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    using handle_pool = pool<DMemoryTestsThing, D_POOL_POLICY_GENERATIONAL>;

    handle_pool              p;
    handle_pool::handle_type first;
    handle_pool::handle_type second;
    handle_pool::handle_type none;
    DMemoryTestsThing*       object = nullptr;

    D_TEST_REQUIRE(none.is_null());
    D_TEST_REQUIRE(p.resolve(none) == nullptr);

    first = p.create_handle(5u);
    D_TEST_REQUIRE(!first.is_null());
    D_TEST_REQUIRE(p.is_live(first));
    object = p.resolve(first);
    D_TEST_REQUIRE(object != nullptr);
    D_TEST_REQUIRE(object->m_tag == 5u);

    D_TEST_REQUIRE(p.destroy_handle(first) == D_MEM_OK);
    D_TEST_REQUIRE(p.resolve(first) == nullptr);
    D_TEST_REQUIRE(!p.is_live(first));
    D_TEST_REQUIRE(p.destroy_handle(first) == D_MEM_ERR_STALE);

    second = p.create_handle(6u);
    D_TEST_REQUIRE(second != first);
    D_TEST_REQUIRE(p.resolve(second) == object);
    D_TEST_REQUIRE(p.resolve(first) == nullptr);
    D_TEST_REQUIRE(p.destroy_handle(second) == D_MEM_OK);
#else
    D_TEST_SKIP("built without D_CFG_POOL_GENERATIONAL");
#endif  // D_INTERNAL_POOL_GENERATIONAL

    return true;
}

/*
tests_memory_pool_lifetime
  Moving a pool.
  Tests the following:
  - a moved-to pool holds the slots, which keep their contents
  - the moved-from pool is empty, keeps its geometry, and is usable
*/
bool
tests_memory_pool_lifetime()
{
    raw_pool a(40,
               8);
    void*    slot = a.acquire();

    D_TEST_REQUIRE(slot != nullptr);
    std::memset(slot,
                0x3c,
                40);

    {
        raw_pool b(std::move(a));

        D_TEST_REQUIRE(b.size() == 1);
#if (D_INTERNAL_POOL_OWNS == 1)
        D_TEST_REQUIRE(b.owns(slot));
#endif  // D_INTERNAL_POOL_OWNS
        D_TEST_REQUIRE(static_cast<unsigned char*>(slot)[39] == 0x3c);
        D_TEST_REQUIRE(a.size() == 0);
        D_TEST_REQUIRE(a.slot_size() >= 40);
        D_TEST_REQUIRE(a.acquire() != nullptr);

        a = std::move(b);
        D_TEST_REQUIRE(b.size() == 0);
    }

    D_TEST_REQUIRE(a.size() == 1);
    D_TEST_REQUIRE(a.release_slot(slot) == D_MEM_OK);

    return true;
}


NS_END  // testing
NS_END  // djinterp
