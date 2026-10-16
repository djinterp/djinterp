/*******************************************************************************
* djinterp [test]                           pool_allocator_tests_integration.cpp
*
*   Section VI of the pool allocator suite - integration.
*
*   The convenience aliases, the classification traits from pool.hpp
* applied to a genuine allocator rather than a hand-rolled stand-in, and a
* standard container placed on top of one.
*
*   The last test is the important one.  A node-based container rebinds its
* allocator to an internal node type, and the converting constructor that
* rebinding goes through yields a handle with NO pool - so the container
* silently allocates from operator new and the pool it was handed stays
* empty.  That is a real, documented consequence of a typed pool, and it is
* pinned here so it cannot regress into a surprise.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_allocator_tests_integration.cpp
*            pool_allocator_tests_integration.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <list>
#include <memory>
#include <vector>
// djinterp
#include "pool_allocator_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // a vector standing on a pool allocator, and the plain equivalent
    using pooled_vector = std::vector<pa_element, pa_alloc>;
    using plain_vector  = std::vector<pa_element>;
}


/*
tests_default_pool_allocator_alias
  Verifies default_pool_allocator expands to free-list release, chunked
  blocks and exponential growth, and that its pool type matches the
  pool.hpp alias built on the same triple.
*/
bool
tests_default_pool_allocator_alias()
{
    using allocator_type = default_pool_allocator<pa_element>;
    using expected       = pool_allocator<pa_element,
                                          free_list_release_policy,
                                          chunked_block_policy<>,
                                          exponential_growth_policy>;

    static_assert(std::is_same<allocator_type, expected>::value,
                  "default_pool_allocator expansion changed.");

    D_PA_CHECK((std::is_same<allocator_type, expected>::value));
    D_PA_CHECK((std::is_same<allocator_type::value_type,
                             pa_element>::value));

    // its pool is the pool.hpp default for the same element
    D_PA_CHECK((std::is_same<allocator_type::pool_type,
                             default_pool_resource<pa_element>>::value));
    D_PA_CHECK((std::is_same<allocator_type::pool_type::release_policy,
                             free_list_release_policy>::value));
    D_PA_CHECK((std::is_same<allocator_type::pool_type::block_policy,
                             chunked_block_policy<>>::value));
    D_PA_CHECK((std::is_same<allocator_type::pool_type::growth_policy,
                             exponential_growth_policy>::value));

    // and it works end to end
    default_pool_resource<pa_element> pool;
    allocator_type                    allocator(pool);

    pa_element* slot = allocator.allocate(1);
    D_PA_CHECK(slot != nullptr);
    D_PA_CHECK(pool.size() == 1u);

    allocator.deallocate(slot, 1);
    D_PA_CHECK(pool.size() == 0u);
    D_PA_CHECK(allocator.allocate(1) == slot);

    return true;
}

/*
tests_monotonic_pool_allocator_alias
  Verifies monotonic_pool_allocator differs from the default on the
  release axis alone, and that the difference is observable - a released
  slot is not handed back.
*/
bool
tests_monotonic_pool_allocator_alias()
{
    using allocator_type = monotonic_pool_allocator<pa_element>;
    using expected       = pool_allocator<pa_element,
                                          monotonic_release_policy,
                                          chunked_block_policy<>,
                                          exponential_growth_policy>;

    static_assert(std::is_same<allocator_type, expected>::value,
                  "monotonic_pool_allocator expansion changed.");

    D_PA_CHECK((std::is_same<allocator_type, expected>::value));
    D_PA_CHECK((std::is_same<allocator_type::pool_type,
                             monotonic_pool_resource<pa_element>>::value));

    // it differs from the default on exactly one axis
    D_PA_CHECK((!std::is_same<allocator_type,
                              default_pool_allocator<pa_element>>::value));
    D_PA_CHECK((std::is_same<
                    allocator_type::pool_type::block_policy,
                    default_pool_allocator<pa_element>
                        ::pool_type::block_policy>::value));
    D_PA_CHECK((std::is_same<
                    allocator_type::pool_type::growth_policy,
                    default_pool_allocator<pa_element>
                        ::pool_type::growth_policy>::value));
    D_PA_CHECK((!std::is_same<
                     allocator_type::pool_type::release_policy,
                     default_pool_allocator<pa_element>
                         ::pool_type::release_policy>::value));

    // the difference is observable through the allocator
    monotonic_pool_resource<pa_element> pool;
    allocator_type                      allocator(pool);

    pa_element* first = allocator.allocate(1);
    D_PA_CHECK(pool.size() == 1u);

    allocator.deallocate(first, 1);
    D_PA_CHECK(pool.size() == 0u);

    // monotonic storage is not recycled, so the next slot is a new one
    pa_element* second = allocator.allocate(1);
    D_PA_CHECK(second != nullptr);
    D_PA_CHECK(second != first);

    return true;
}

/*
tests_allocator_is_classified_by_the_pool_traits
  Verifies the pool.hpp classification layer recognises a real
  pool_allocator: it is a pool allocator, its resource type is extracted,
  and the aggregate reports the properties of that resource.
*/
bool
tests_allocator_is_classified_by_the_pool_traits()
{
    // the allocator itself
    D_PA_CHECK(is_pool_allocator_v<pa_alloc>);
    D_PA_CHECK(is_pool_allocator_v<const pa_alloc&>);
    D_PA_CHECK(is_pool_allocator_v<pa_mono_alloc>);
    D_PA_CHECK(is_pool_allocator_v<default_pool_allocator<pa_element>>);

    // it is not itself a pool
    D_PA_CHECK(!is_pool_resource_v<pa_alloc>);
    D_PA_CHECK(is_pool_resource_v<pa_pool>);

    // resource extraction names the pool, not a pointer to it
    D_PA_CHECK((std::is_same<pool_resource_type_t<pa_alloc>,
                             pa_pool>::value));
    D_PA_CHECK((std::is_same<pool_resource_type_t<const pa_alloc&>,
                             pa_pool>::value));
    D_PA_CHECK((std::is_same<pool_resource_type_t<pa_mono_alloc>,
                             pa_mono_pool>::value));
    D_PA_CHECK(!std::is_pointer<pool_resource_type_t<pa_alloc>>::value);

    // the aggregate reads the resource's properties
    {
        using classified = pool_allocator_class<pa_alloc>;

        D_PA_CHECK(classified::is_pool_alloc);
        D_PA_CHECK((std::is_same<classified::resource_type,
                                 pa_pool>::value));
        D_PA_CHECK(classified::pointer_stable);
        D_PA_CHECK(classified::individual_release);
        D_PA_CHECK(!classified::monotonic);
        D_PA_CHECK(!classified::generational);
    }

    // ... and follows the release policy when it changes
    {
        using classified = pool_allocator_class<pa_mono_alloc>;

        D_PA_CHECK(classified::is_pool_alloc);
        D_PA_CHECK(classified::monotonic);
        D_PA_CHECK(!classified::individual_release);
        D_PA_CHECK(classified::pointer_stable);
    }

    // a standard allocator classifies as nothing
    D_PA_CHECK(!is_pool_allocator_v<std::allocator<pa_element>>);
    D_PA_CHECK(std::is_void<
                   pool_resource_type_t<std::allocator<pa_element>>>::value);
    D_PA_CHECK(!pool_allocator_class<std::allocator<pa_element>>
                    ::is_pool_alloc);

    return true;
}

/*
tests_container_with_a_pool_allocator_is_pool_backed
  Verifies a container parameterised on a real pool allocator is
  classified as pool-backed, and that a container on a stock allocator is
  not.
*/
bool
tests_container_with_a_pool_allocator_is_pool_backed()
{
    D_PA_CHECK(is_pool_backed_container_v<pooled_vector>);
    D_PA_CHECK(is_pool_backed_container_v<const pooled_vector&>);
    D_PA_CHECK(is_pool_backed_container_v<
                   std::vector<pa_element, pa_mono_alloc>>);

    D_PA_CHECK(!is_pool_backed_container_v<plain_vector>);
    D_PA_CHECK(!is_pool_backed_container_v<pa_plain_container>);
    D_PA_CHECK(!is_pool_backed_container_v<int>);

    // the verdict is exactly "the allocator_type is a pool allocator"
    D_PA_CHECK(has_allocator_type_v<pooled_vector>);
    D_PA_CHECK((std::is_same<pooled_vector::allocator_type,
                             pa_alloc>::value));
    D_PA_CHECK(is_pool_backed_container_v<pooled_vector> ==
               is_pool_allocator_v<pooled_vector::allocator_type>);
    D_PA_CHECK(is_pool_backed_container_v<plain_vector> ==
               is_pool_allocator_v<plain_vector::allocator_type>);

    // a pool is not a container, and has no allocator at all
    D_PA_CHECK(!has_allocator_type_v<pa_pool>);
    D_PA_CHECK(!is_pool_backed_container_v<pa_pool>);

    return true;
}

/*
tests_vector_backed_by_a_pool_allocator
  Verifies a std::vector standing on a pool allocator stores and retrieves
  values correctly, reports the allocator it was given, and balances every
  allocation it makes - the pool must be empty again once the vector is
  gone.
*/
bool
tests_vector_backed_by_a_pool_allocator()
{
    pa_pool pool(16);

    {
        pooled_vector values{ pa_alloc(pool) };

        // the vector kept the handle it was handed
        D_PA_CHECK(values.get_allocator().resource() == &pool);
        D_PA_CHECK(values.get_allocator() == pa_alloc(pool));
        D_PA_CHECK(values.empty());

        // a vector's first buffer is a single element, so it comes out
        // of the pool
        values.push_back(pa_element{ 1u, 2u });
        D_PA_CHECK(values.size() == 1u);
        D_PA_CHECK(pool.size() == 1u);

        // subsequent growth is a bulk request and bypasses the pool -
        // and the discarded single-element buffer returns to it
        values.push_back(pa_element{ 3u, 4u });
        D_PA_CHECK(values.size() == 2u);
        D_PA_CHECK(pool.size() == 0u);

        for (std::uint64_t i = 2; i < 40; ++i)
        {
            values.push_back(pa_element{ i, i * 2u });
        }

        D_PA_CHECK(values.size() == 40u);

        // every value survived the reallocations intact
        D_PA_CHECK(values[0].m_lo == 1u);
        D_PA_CHECK(values[0].m_hi == 2u);
        D_PA_CHECK(values[1].m_lo == 3u);
        D_PA_CHECK(values[1].m_hi == 4u);

        for (std::uint64_t i = 2; i < 40; ++i)
        {
            D_PA_CHECK(values[static_cast<std::size_t>(i)].m_lo == i);
            D_PA_CHECK(values[static_cast<std::size_t>(i)].m_hi ==
                       (i * 2u));
        }

        // a copy shares the same pool, per the propagation traits
        pooled_vector copy(values);
        D_PA_CHECK(copy.get_allocator().resource() == &pool);
        D_PA_CHECK(copy.size() == values.size());
        D_PA_CHECK(copy[7].m_lo == values[7].m_lo);

        // clearing and shrinking returns the buffer
        values.clear();
        values.shrink_to_fit();
        D_PA_CHECK(values.empty());
    }

    // once every vector is destroyed the pool is balanced again
    D_PA_CHECK(pool.size() == 0u);
    D_PA_CHECK(pool.bytes_in_use() == 0u);

    // a vector that never grows past one element holds a pool slot for
    // its whole life
    {
        pooled_vector single{ pa_alloc(pool) };

        single.reserve(1);
        D_PA_CHECK(pool.size() == 1u);

        single.push_back(pa_element{ 9u, 9u });
        D_PA_CHECK(single.size() == 1u);
        D_PA_CHECK(single[0].m_lo == 9u);
        D_PA_CHECK(pool.size() == 1u);
    }

    D_PA_CHECK(pool.size() == 0u);

    return true;
}

/*
tests_rebound_allocator_silently_bypasses_the_pool
  Verifies the documented consequence of a typed pool: rebinding produces
  a handle with no pool, so a node-based container built on a pool
  allocator allocates from operator new and never touches the pool it was
  handed.  Pinned here as a property, not a defect.
*/
bool
tests_rebound_allocator_silently_bypasses_the_pool()
{
    pa_pool pool(16);

    // the rebind itself loses the binding
    {
        const pa_alloc source(pool);
        D_PA_CHECK(source.resource() == &pool);

        using rebound =
            std::allocator_traits<pa_alloc>::rebind_alloc<pa_other>;

        rebound target(source);
        D_PA_CHECK(target.resource() == nullptr);
        D_PA_CHECK(pool.size() == 0u);
    }

    // a node-based container rebinds internally, so every node comes
    // from operator new and the pool is never consulted
    {
        std::list<pa_element, pa_alloc> nodes{ pa_alloc(pool) };

        // the binding is lost in BOTH directions: the container stores
        // the rebound node allocator, so reading the allocator back out
        // converts a second time and the pool is gone from the answer
        // too - a vector, which never rebinds, keeps it (checked below)
        D_PA_CHECK(nodes.get_allocator().resource() == nullptr);

        for (std::uint64_t i = 0; i < 20; ++i)
        {
            nodes.push_back(pa_element{ i, i + 100u });
        }

        D_PA_CHECK(nodes.size() == 20u);

        // the container works perfectly - it simply is not pooled
        std::uint64_t expected = 0;

        for (const pa_element& value : nodes)
        {
            D_PA_CHECK(value.m_lo == expected);
            D_PA_CHECK(value.m_hi == (expected + 100u));
            ++expected;
        }

        D_PA_CHECK(expected == 20u);

        // the pool it was handed was never touched
        D_PA_CHECK(pool.size() == 0u);
        D_PA_CHECK(pool.bytes_in_use() == 0u);
        D_PA_CHECK(pool.capacity() == 16u);
    }

    D_PA_CHECK(pool.size() == 0u);

    // a vector, by contrast, allocates its element buffer directly and
    // so does reach the pool for a single-element buffer
    {
        pooled_vector values{ pa_alloc(pool) };

        // no rebinding occurs, so the handle survives the round trip
        D_PA_CHECK(values.get_allocator().resource() == &pool);

        values.reserve(1);
        D_PA_CHECK(pool.size() == 1u);
    }

    D_PA_CHECK(pool.size() == 0u);

    return true;
}


NS_END  // testing
NS_END  // djinterp
