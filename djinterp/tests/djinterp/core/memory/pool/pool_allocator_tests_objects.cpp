/*******************************************************************************
* djinterp [test]                               pool_allocator_tests_objects.cpp
*
*   Section IV of the pool allocator suite - object lifecycle.
*
*   The pool deals in raw storage and never constructs or destroys
* anything; the allocator's construct() and destroy() are what turn that
* storage into objects.  A counting element type makes both observable, so
* these tests measure constructions and destructions rather than inferring
* them from the absence of a crash.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_allocator_tests_objects.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <limits>
#include <memory>
#include <utility>
// djinterp
#include "pool_allocator_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_construct_creates_an_object_in_place
  Verifies construct placement-news an object into the supplied storage -
  the object lands at exactly the address given, and no allocation occurs
  as a side effect.
*/
bool
tests_construct_creates_an_object_in_place()
{
    pa_counted_pool  pool(4);
    pa_counted_alloc allocator(pool);

    pa_counted::reset_counts();

    pa_counted* storage = allocator.allocate(1);
    D_PA_CHECK(storage != nullptr);
    D_PA_CHECK(pool.size() == 1u);

    // allocation alone constructs nothing
    D_PA_CHECK(pa_counted::s_constructed == 0);

    allocator.construct(storage);

    D_PA_CHECK(pa_counted::s_constructed == 1);
    D_PA_CHECK(pa_counted::s_destroyed   == 0);

    // the object occupies the storage it was given
    D_PA_CHECK(storage->m_first  == 0);
    D_PA_CHECK(storage->m_second == 0);

    // and constructing did not disturb the pool
    D_PA_CHECK(pool.size() == 1u);

    allocator.destroy(storage);
    allocator.deallocate(storage, 1);

    D_PA_CHECK(pa_counted::s_destroyed == 1);
    D_PA_CHECK(pool.size() == 0u);

    return true;
}

/*
tests_construct_forwards_its_arguments
  Verifies constructor arguments reach the object unchanged, including
  through an rvalue, and that construct works on any type - not only the
  allocator's own element type.
*/
bool
tests_construct_forwards_its_arguments()
{
    pa_counted_pool  pool(8);
    pa_counted_alloc allocator(pool);

    pa_counted::reset_counts();

    pa_counted* object = allocator.allocate(1);
    allocator.construct(object, 17, 42);

    D_PA_CHECK(object->m_first  == 17);
    D_PA_CHECK(object->m_second == 42);
    D_PA_CHECK(pa_counted::s_constructed == 1);

    allocator.destroy(object);
    allocator.deallocate(object, 1);

    // an rvalue argument is forwarded, not copied into a temporary that
    // then decays - the copy constructor counts, so this is observable
    pa_counted::reset_counts();

    pa_counted  source(3, 4);
    pa_counted* target = allocator.allocate(1);
    D_PA_CHECK(pa_counted::s_constructed == 1);

    allocator.construct(target, std::move(source));

    D_PA_CHECK(pa_counted::s_constructed == 2);
    D_PA_CHECK(target->m_first  == 3);
    D_PA_CHECK(target->m_second == 4);

    allocator.destroy(target);
    allocator.deallocate(target, 1);

    // construct is a template over the pointee, so it serves rebound
    // node types too
    {
        pa_pool     plain_pool(4);
        pa_alloc    plain(plain_pool);
        pa_element* raw = plain.allocate(1);

        plain.construct(raw, pa_element{ 99u, 100u });
        D_PA_CHECK(raw->m_lo == 99u);
        D_PA_CHECK(raw->m_hi == 100u);

        plain.destroy(raw);
        plain.deallocate(raw, 1);
    }

    return true;
}

/*
tests_destroy_runs_the_destructor
  Verifies destroy invokes the destructor exactly once per object and
  leaves the storage intact for the allocator to reclaim separately.
*/
bool
tests_destroy_runs_the_destructor()
{
    pa_counted_pool  pool(8);
    pa_counted_alloc allocator(pool);

    pa_counted::reset_counts();

    pa_counted* objects[4] = {};

    for (std::size_t i = 0; i < 4; ++i)
    {
        objects[i] = allocator.allocate(1);
        allocator.construct(objects[i], static_cast<int>(i), 0);
    }

    D_PA_CHECK(pa_counted::s_constructed == 4);
    D_PA_CHECK(pa_counted::s_destroyed   == 0);
    D_PA_CHECK(pool.size() == 4u);

    for (std::size_t i = 0; i < 4; ++i)
    {
        allocator.destroy(objects[i]);

        // exactly one destruction per call
        D_PA_CHECK(pa_counted::s_destroyed == static_cast<int>(i + 1));

        // and the storage is still the pool's to reclaim
        D_PA_CHECK(pool.size() == 4u);
    }

    for (std::size_t i = 0; i < 4; ++i)
    {
        allocator.deallocate(objects[i], 1);
    }

    D_PA_CHECK(pool.size() == 0u);
    D_PA_CHECK(pa_counted::s_constructed == pa_counted::s_destroyed);

    // destroy is noexcept
    D_PA_CHECK(noexcept(allocator.destroy(objects[0])));

    return true;
}

/*
tests_max_size_reports_the_element_bound
  Verifies max_size is the address space divided by the element size, is
  const and noexcept, and scales inversely with the element.
*/
bool
tests_max_size_reports_the_element_bound()
{
    pa_pool        pool;
    const pa_alloc allocator(pool);

    const std::size_t expected =
        ( std::numeric_limits<std::size_t>::max() / sizeof(pa_element) );

    D_PA_CHECK(allocator.max_size() == expected);
    D_PA_CHECK(allocator.max_size() > 0u);

    D_PA_CHECK(noexcept(allocator.max_size()));
    D_PA_CHECK((std::is_same<decltype(allocator.max_size()),
                             pa_alloc::size_type>::value));

    // a larger element admits fewer objects
    pa_over_pool        over_pool;
    const pa_over_alloc over_allocator(over_pool);

    D_PA_CHECK(sizeof(pa_overaligned) >= sizeof(pa_element));
    D_PA_CHECK(over_allocator.max_size() <= allocator.max_size());
    D_PA_CHECK(over_allocator.max_size() ==
               ( std::numeric_limits<std::size_t>::max() /
                 sizeof(pa_overaligned) ));

    // the bound does not depend on the pool's state
    pa_pool  populated(64);
    pa_alloc bound(populated);
    bound.allocate(1);
    D_PA_CHECK(bound.max_size() == expected);

    // std::allocator_traits reports the same number
    D_PA_CHECK(std::allocator_traits<pa_alloc>::max_size(allocator) ==
               expected);

    return true;
}

/*
tests_allocator_traits_construct_and_destroy
  Verifies std::allocator_traits routes construction and destruction
  through the allocator's own members rather than its default
  implementations - the counting type makes the difference visible.
*/
bool
tests_allocator_traits_construct_and_destroy()
{
    using traits = std::allocator_traits<pa_counted_alloc>;

    pa_counted_pool  pool(8);
    pa_counted_alloc allocator(pool);

    pa_counted::reset_counts();

    pa_counted* object = traits::allocate(allocator, 1);
    D_PA_CHECK(object != nullptr);
    D_PA_CHECK(pool.size() == 1u);
    D_PA_CHECK(pa_counted::s_constructed == 0);

    traits::construct(allocator, object, 5, 6);
    D_PA_CHECK(pa_counted::s_constructed == 1);
    D_PA_CHECK(object->m_first  == 5);
    D_PA_CHECK(object->m_second == 6);

    traits::destroy(allocator, object);
    D_PA_CHECK(pa_counted::s_destroyed == 1);

    traits::deallocate(allocator, object, 1);
    D_PA_CHECK(pool.size() == 0u);

    // the traits' bulk path also reaches the allocator
    pa_counted* block = traits::allocate(allocator, 4);
    D_PA_CHECK(block != nullptr);
    D_PA_CHECK(pool.size() == 0u);
    traits::deallocate(allocator, block, 4);

    // construction through the traits is balanced across many objects
    pa_counted::reset_counts();

    pa_counted* many[8] = {};

    for (std::size_t i = 0; i < 8; ++i)
    {
        many[i] = traits::allocate(allocator, 1);
        traits::construct(allocator, many[i], static_cast<int>(i),
                          static_cast<int>(i));
    }

    D_PA_CHECK(pa_counted::s_constructed == 8);

    for (std::size_t i = 0; i < 8; ++i)
    {
        D_PA_CHECK(many[i]->m_first == static_cast<int>(i));
        traits::destroy(allocator, many[i]);
        traits::deallocate(allocator, many[i], 1);
    }

    D_PA_CHECK(pa_counted::s_destroyed == 8);
    D_PA_CHECK(pool.size() == 0u);

    return true;
}

/*
tests_full_object_lifecycle_through_the_pool
  Verifies allocate, construct, use, destroy and deallocate compose over
  many rounds with balanced construction counts, no leaked slots, and no
  pool growth beyond the working set.
*/
bool
tests_full_object_lifecycle_through_the_pool()
{
    constexpr std::size_t width = 6;

    pa_counted_pool  pool(width);
    pa_counted_alloc allocator(pool);

    pa_counted::reset_counts();

    for (std::size_t round = 0; round < 50; ++round)
    {
        pa_counted* objects[width] = {};

        for (std::size_t i = 0; i < width; ++i)
        {
            objects[i] = allocator.allocate(1);
            D_PA_CHECK(objects[i] != nullptr);

            allocator.construct(objects[i],
                                static_cast<int>(round),
                                static_cast<int>(i));
        }

        D_PA_CHECK(pool.size() == width);

        // the objects hold their values and do not overlap
        for (std::size_t i = 0; i < width; ++i)
        {
            D_PA_CHECK(objects[i]->m_first  == static_cast<int>(round));
            D_PA_CHECK(objects[i]->m_second == static_cast<int>(i));
        }

        for (std::size_t i = 0; i < width; ++i)
        {
            allocator.destroy(objects[i]);
            allocator.deallocate(objects[i], 1);
        }

        D_PA_CHECK(pool.size() == 0u);
    }

    // every construction was matched by a destruction
    D_PA_CHECK(pa_counted::s_constructed == (50 * static_cast<int>(width)));
    D_PA_CHECK(pa_counted::s_destroyed   == pa_counted::s_constructed);

    // and the pool never grew past the working set
    D_PA_CHECK(pool.capacity() == width);
    D_PA_CHECK(pool.bytes_in_use() == 0u);

    return true;
}


NS_END  // testing
NS_END  // djinterp
