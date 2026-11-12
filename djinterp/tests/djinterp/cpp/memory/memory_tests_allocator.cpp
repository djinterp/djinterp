/*******************************************************************************
* djinterp [cpp][memory]                              memory_tests_allocator.cpp
*
* The memory suite, block "allocator": pool_allocator.hpp's node_pool, the
* pool_allocator and arena_allocator over it and over an arena, and the
* standard containers on them.
*
*
* path:      /tests/djinterp/cpp/memory/memory_tests_allocator.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./memory_tests.hpp"  // corresponding header

// std
#include <list>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>


NS_DJINTERP
NS_TESTING


/*
tests_memory_allocator_node_pool
  node_pool.
  Tests the following:
  - it is not configured until its first acquire, which fixes its slot
  - requests that fit the slot come from the pool; larger ones from its
    source, and are released there
*/
bool
tests_memory_allocator_node_pool()
{
    node_pool nodes;
    void*     small = nullptr;
    void*     large = nullptr;

    D_TEST_REQUIRE(!nodes.is_configured());
    D_TEST_REQUIRE(nodes.size() == 0);
    D_TEST_REQUIRE(nodes.slot_size() == 0);

    small = nodes.acquire(24,
                          8);
    D_TEST_REQUIRE(small != nullptr);
    D_TEST_REQUIRE(nodes.is_configured());
    D_TEST_REQUIRE(nodes.slot_size() >= 24);
    D_TEST_REQUIRE(nodes.size() == 1);

    large = nodes.acquire(4096,
                          8);
    D_TEST_REQUIRE(large != nullptr);
    D_TEST_REQUIRE(nodes.size() == 1);

    nodes.release(large,
                  4096,
                  8);
    nodes.release(small,
                  24,
                  8);
    D_TEST_REQUIRE(nodes.size() == 0);
    D_TEST_REQUIRE(nodes.block_count() >= 1);

    return true;
}

/*
tests_memory_allocator_pool
  pool_allocator<T>.
  Tests the following:
  - it is an Allocator: its typedefs, and rebinding to another type over the
    same node_pool
  - two are equal when they share a node_pool, unequal otherwise
  - allocate(1) comes from the pool; deallocate gives it back
*/
bool
tests_memory_allocator_pool()
{
    node_pool                       nodes;
    node_pool                       others;
    pool_allocator<int>             ints(nodes);
    pool_allocator<double>          doubles(ints);
    pool_allocator<int>             elsewhere(others);
    pool_allocator<int>::pointer    one = nullptr;

    static_assert(std::is_same<pool_allocator<int>::value_type, int>::value,
                  "value_type is the element");
    static_assert(std::is_same<pool_allocator<int>::rebind<char>::other,
                               pool_allocator<char> >::value,
                  "rebind names the same allocator for another type");
    static_assert(!pool_allocator<int>::is_always_equal::value,
                  "equality depends on the node_pool");

    D_TEST_REQUIRE(doubles.resource() == &nodes);
    D_TEST_REQUIRE(ints == doubles);
    D_TEST_REQUIRE(ints != elsewhere);
    D_TEST_REQUIRE(ints.max_size() > 0);

    one = ints.allocate(1);
    D_TEST_REQUIRE(one != nullptr);
    D_TEST_REQUIRE(nodes.size() == 1);
    *one = 42;
    ints.deallocate(one,
                    1);
    D_TEST_REQUIRE(nodes.size() == 0);

    return true;
}

/*
tests_memory_allocator_pool_containers
  The containers it is for.
  Tests the following:
  - a std::list on a pool_allocator takes its nodes from the pool, and gives
    them back as it shrinks
  - a std::map runs on one, in order
*/
bool
tests_memory_allocator_pool_containers()
{
    node_pool list_nodes;
    node_pool map_nodes;

    {
        std::list<int, pool_allocator<int> > values(
            (pool_allocator<int>(list_nodes)));

        for (int i = 0; i < 100; ++i)
        {
            values.push_back(i);
        }

        D_TEST_REQUIRE(values.size() == 100);
        D_TEST_REQUIRE(list_nodes.is_configured());
        D_TEST_REQUIRE(list_nodes.size() >= 100);
        D_TEST_REQUIRE( (values.front() == 0) && (values.back() == 99) );

        values.clear();
        D_TEST_REQUIRE(list_nodes.size() == 0);
    }

    {
        using map_allocator = pool_allocator<std::pair<const int, int> >;

        const std::less<int>                              order;
        std::map<int, int, std::less<int>, map_allocator> squares(
            order,
            map_allocator(map_nodes));

        for (int i = 10; i > 0; --i)
        {
            squares[i] = i * i;
        }

        D_TEST_REQUIRE(squares.size() == 10);
        D_TEST_REQUIRE(squares.begin()->first == 1);
        D_TEST_REQUIRE(squares[7] == 49);
        D_TEST_REQUIRE(map_nodes.size() >= 10);
    }

    D_TEST_REQUIRE(map_nodes.size() == 0);

    return true;
}

/*
tests_memory_allocator_arena
  arena_allocator<T>.
  Tests the following:
  - it allocates aligned storage from its arena, and deallocate frees nothing
  - equality follows the arena; rebinding keeps it
  - a std::vector grows on it; the arena keeps every buffer it outgrew
*/
bool
tests_memory_allocator_arena()
{
    arena                  a;
    arena                  b;
    arena_allocator<int>   ints(a);
    arena_allocator<char>  chars(ints);
    arena_allocator<int>   other(b);
    int*                   block = nullptr;
    DMemSize               used  = 0;

    D_TEST_REQUIRE(chars.resource() == &a);
    D_TEST_REQUIRE(ints == chars);
    D_TEST_REQUIRE(ints != other);

    block = ints.allocate(16);
    D_TEST_REQUIRE(block != nullptr);
    D_TEST_REQUIRE(is_aligned(block,
                              align_of<int>::value));
    used = a.used();
    ints.deallocate(block,
                    16);
    D_TEST_REQUIRE(a.used() == used);

    {
        std::vector<int, arena_allocator<int> > values(ints);

        for (int i = 0; i < 1000; ++i)
        {
            values.push_back(i);
        }

        D_TEST_REQUIRE(values.size() == 1000);
        D_TEST_REQUIRE( (values[0] == 0) && (values[999] == 999) );
        D_TEST_REQUIRE(a.used() >= used + 1000 * sizeof(int));
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
