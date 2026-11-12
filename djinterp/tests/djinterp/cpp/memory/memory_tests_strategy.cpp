/*******************************************************************************
* djinterp [cpp][memory]                               memory_tests_strategy.cpp
*
* The memory suite, block "strategy": memory_strategy_common.hpp's detection
* and classification, and memory_strategy.hpp's four strategies at run time.
*
*
* path:      /tests/djinterp/cpp/memory/memory_tests_strategy.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./memory_tests.hpp"  // corresponding header

// std
#include <memory>
#include <new>


NS_DJINTERP
NS_TESTING


namespace
{
    // DMemoryTestsHalfStrategy
    //   class: allocates and deallocates, but declares none of the contract's
    // constants, so it is not a strategy.
    struct DMemoryTestsHalfStrategy
    {
        using value_type = int;

        int*
        allocate(
            std::size_t
        )
        {
            return nullptr;
        }

        void
        deallocate(
            int*,
            std::size_t
        )
        {}
    };

    // the strategies under test, by name
    using arena_strategy     = DArenaMemoryStrategy<>;
    using pool_strategy      = DPoolMemoryStrategy<pool<double> >;
    using bump_strategy      =
        DPoolMemoryStrategy<pool<double, D_POOL_POLICY_MONOTONIC> >;
    using buffer_strategy    = DBufferMemoryStrategy<int, 16>;
    using allocator_strategy = DAllocatorMemoryStrategy<std::allocator<long> >;
}  // anonymous namespace


/*
tests_memory_strategy_classification
  What the traits say about each strategy, and about non-strategies.
  Tests the following:
  - each of the four is a strategy; a type missing the contract's constants
    and an unrelated type are not
  - the arena's is byte-typed, the others element-typed
  - storage kind and extent: the buffer's is static with its count
  - stability and release: an arena is stable and does not release; a
    free-list pool releases and a monotonic one does not; an allocator
    releases and is not stable
*/
bool
tests_memory_strategy_classification()
{
    D_TEST_REQUIRE(is_memory_strategy<arena_strategy>::value);
    D_TEST_REQUIRE(is_memory_strategy<pool_strategy>::value);
    D_TEST_REQUIRE(is_memory_strategy<buffer_strategy>::value);
    D_TEST_REQUIRE(is_memory_strategy<allocator_strategy>::value);
    D_TEST_REQUIRE(is_memory_strategy<const pool_strategy>::value);
    D_TEST_REQUIRE(!is_memory_strategy<DMemoryTestsHalfStrategy>::value);
    D_TEST_REQUIRE(!is_memory_strategy<int>::value);

    D_TEST_REQUIRE(is_byte_strategy<arena_strategy>::value);
    D_TEST_REQUIRE(!is_element_strategy<arena_strategy>::value);
    D_TEST_REQUIRE(is_element_strategy<pool_strategy>::value);
    D_TEST_REQUIRE(is_element_strategy<buffer_strategy>::value);

    D_TEST_REQUIRE(strategy_storage_kind_of<buffer_strategy>::value ==
                   storage_kind::static_storage);
    D_TEST_REQUIRE(strategy_storage_kind_of<arena_strategy>::value ==
                   storage_kind::dynamic_storage);
    D_TEST_REQUIRE(strategy_storage_kind_of<int>::value ==
                   storage_kind::unknown);
    D_TEST_REQUIRE(is_static_strategy<buffer_strategy>::value);
    D_TEST_REQUIRE(!is_static_strategy<pool_strategy>::value);
    D_TEST_REQUIRE(strategy_extent_of<buffer_strategy>::value == 16);
    D_TEST_REQUIRE(strategy_extent_of<pool_strategy>::value == 0);

    D_TEST_REQUIRE(is_stable_strategy<arena_strategy>::value);
    D_TEST_REQUIRE(!is_releasing_strategy<arena_strategy>::value);
    D_TEST_REQUIRE(is_releasing_strategy<pool_strategy>::value);
    D_TEST_REQUIRE(!is_releasing_strategy<bump_strategy>::value);
    D_TEST_REQUIRE(is_releasing_strategy<allocator_strategy>::value);
    D_TEST_REQUIRE(!is_stable_strategy<allocator_strategy>::value);

    return true;
}

/*
tests_memory_strategy_arena
  DArenaMemoryStrategy.
  Tests the following:
  - allocate gives bytes from its arena, aligned for its element type
  - deallocate frees nothing; reset resets the arena
*/
bool
tests_memory_strategy_arena()
{
    arena                                   a;
    DArenaMemoryStrategy<DMemoryTestsThing> strategy(a);
    unsigned char*                          bytes = nullptr;
    DMemSize                                used  = 0;

    D_TEST_REQUIRE(strategy.resource() == &a);
    bytes = strategy.allocate(sizeof(DMemoryTestsThing) * 4);
    D_TEST_REQUIRE(bytes != nullptr);
    D_TEST_REQUIRE(is_aligned(bytes,
                              align_of<DMemoryTestsThing>::value));
    used = a.used();
    strategy.deallocate(bytes,
                        sizeof(DMemoryTestsThing) * 4);
    D_TEST_REQUIRE(a.used() == used);
    strategy.reset();
    D_TEST_REQUIRE(a.used() == 0);

    return true;
}

/*
tests_memory_strategy_pool
  DPoolMemoryStrategy.
  Tests the following:
  - allocate(1) gives a slot of its pool; deallocate gives it back
  - any other count throws bad_alloc, as does an exhausted pool
  - reset resets the pool
*/
bool
tests_memory_strategy_pool()
{
    pool<double>  p;
    pool_strategy strategy(p);
    double*       slot = nullptr;
    bool          threw = false;

    D_TEST_REQUIRE(strategy.resource() == &p);
    slot = strategy.allocate(1);
    D_TEST_REQUIRE(slot != nullptr);
    D_TEST_REQUIRE(p.size() == 1);
    strategy.deallocate(slot,
                        1);
    D_TEST_REQUIRE(p.size() == 0);

    try
    {
        (void)strategy.allocate(2);
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }

    D_TEST_REQUIRE(threw);

#if (D_INTERNAL_MEM_SOURCE_NULL == 1)
    {
        pool<double>  starved(DMemorySource::refusing());
        pool_strategy hungry(starved);

        threw = false;

        try
        {
            (void)hungry.allocate(1);
        }
        catch (const std::bad_alloc&)
        {
            threw = true;
        }

        D_TEST_REQUIRE(threw);
    }
#endif  // D_INTERNAL_MEM_SOURCE_NULL

    D_TEST_REQUIRE(strategy.allocate(1) != nullptr);
    strategy.reset();
    D_TEST_REQUIRE(p.empty());

    return true;
}

/*
tests_memory_strategy_buffer
  DBufferMemoryStrategy.
  Tests the following:
  - allocate gives consecutive runs from its extent, until it is spent;
    then, or for a count of 0, it throws bad_alloc
  - deallocate gives back only the last run (a stack)
  - reset empties it
*/
bool
tests_memory_strategy_buffer()
{
    buffer_strategy strategy;
    int*            a     = nullptr;
    int*            b     = nullptr;
    bool            threw = false;

    D_TEST_REQUIRE(strategy.capacity() == 16);
    D_TEST_REQUIRE(strategy.size() == 0);

    a = strategy.allocate(4);
    b = strategy.allocate(8);
    D_TEST_REQUIRE(a == strategy.data());
    D_TEST_REQUIRE(b == a + 4);
    D_TEST_REQUIRE(strategy.size() == 12);
    D_TEST_REQUIRE(strategy.remaining() == 4);

    try
    {
        (void)strategy.allocate(5);
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }

    D_TEST_REQUIRE(threw);
    threw = false;

    try
    {
        (void)strategy.allocate(0);
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }

    D_TEST_REQUIRE(threw);

    strategy.deallocate(a,
                        4);
    D_TEST_REQUIRE(strategy.size() == 12);
    strategy.deallocate(b,
                        8);
    D_TEST_REQUIRE(strategy.size() == 4);

    strategy.reset();
    D_TEST_REQUIRE(strategy.size() == 0);
    D_TEST_REQUIRE(strategy.allocate(16) == strategy.data());

    return true;
}

/*
tests_memory_strategy_allocator
  DAllocatorMemoryStrategy.
  Tests the following:
  - allocate and deallocate go to its allocator, through allocator_traits
  - it holds its allocator, and gives it back by resource()
*/
bool
tests_memory_strategy_allocator()
{
    node_pool                                         nodes;
    pool_allocator<long>                              allocator(nodes);
    DAllocatorMemoryStrategy<pool_allocator<long> >   strategy(allocator);
    allocator_strategy                                plain;
    long*                                             one   = nullptr;
    long*                                             many  = nullptr;

    D_TEST_REQUIRE(strategy.resource() == allocator);
    one = strategy.allocate(1);
    D_TEST_REQUIRE(one != nullptr);
    D_TEST_REQUIRE(nodes.size() == 1);
    strategy.deallocate(one,
                        1);
    D_TEST_REQUIRE(nodes.size() == 0);

    many = plain.allocate(32);
    D_TEST_REQUIRE(many != nullptr);
    many[31] = 7;
    plain.deallocate(many,
                     32);

    return true;
}


NS_END  // testing
NS_END  // djinterp
