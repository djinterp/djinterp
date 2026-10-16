/******************************************************************************
* djinterp [test]                         pool_allocator_tests_construction.cpp
*
*   Section II of the pool allocator suite - the handle's own lifetime.
*
*   A pool_allocator stores one non-owning pointer.  Copying it copies the
* binding, never the pool; assigning it repoints the binding; and the
* converting constructor used by rebinding deliberately produces a handle
* with no binding at all, because a pool_resource is typed and cannot be
* shared across element types.  That last case is the one with teeth, so it
* is tested as a property rather than treated as an accident.
*
*
* path:      /tests/djinterp/core/memory/pool_allocator_tests_construction.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

// std
#include <memory>
// djinterp
#include "pool_allocator_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_allocator_binds_to_its_pool
  Verifies an allocator constructed from a pool reports exactly that pool
  through resource(), and that resource() is a const observer returning a
  pointer to the pool type.
*/
bool
tests_allocator_binds_to_its_pool()
{
    pa_pool        pool;
    const pa_alloc allocator(pool);

    D_PA_CHECK(allocator.resource() == &pool);
    D_PA_CHECK(allocator.resource() != nullptr);

    D_PA_CHECK((std::is_same<decltype(allocator.resource()),
                             pa_pool*>::value));
    D_PA_CHECK(noexcept(allocator.resource()));

    // two pools, two allocators, no crosstalk
    pa_pool  first;
    pa_pool  second;
    pa_alloc bound_first(first);
    pa_alloc bound_second(second);

    D_PA_CHECK(bound_first.resource()  == &first);
    D_PA_CHECK(bound_second.resource() == &second);
    D_PA_CHECK(bound_first.resource()  != bound_second.resource());

    // the binding is a view: the pool observed through it is live
    D_PA_CHECK(bound_first.resource()->capacity() == 0u);
    first.reserve(4);
    D_PA_CHECK(bound_first.resource()->capacity() == 4u);

    return true;
}

/*
tests_allocator_pool_constructor_is_explicit
  Verifies a pool does not convert implicitly into an allocator, so an
  allocator parameter can never be filled in by accident.
*/
bool
tests_allocator_pool_constructor_is_explicit()
{
    D_PA_CHECK((std::is_constructible<pa_alloc, pa_pool&>::value));
    D_PA_CHECK((!std::is_convertible<pa_pool&, pa_alloc>::value));

    // and the constructor really is noexcept
    D_PA_CHECK((std::is_nothrow_constructible<pa_alloc, pa_pool&>::value));

    // a pool of another element type is not accepted at all
    D_PA_CHECK((!std::is_constructible<pa_alloc, pa_other_pool&>::value));

    return true;
}

/*
tests_allocator_copy_shares_the_pool
  Verifies a copied allocator names the same pool - the handle is copied,
  never the pool - so allocations made through one are visible through the
  other.
*/
bool
tests_allocator_copy_shares_the_pool()
{
    pa_pool  pool(8);
    pa_alloc original(pool);
    pa_alloc copy(original);

    D_PA_CHECK(copy.resource() == original.resource());
    D_PA_CHECK(copy.resource() == &pool);

    // an allocation through the copy is visible through the original
    pa_element* slot = copy.allocate(1);
    D_PA_CHECK(slot != nullptr);
    D_PA_CHECK(pool.size() == 1u);
    D_PA_CHECK(original.resource()->size() == 1u);

    // and the original can free what the copy allocated
    original.deallocate(slot, 1);
    D_PA_CHECK(pool.size() == 0u);

    // copying is noexcept and does not disturb the source
    D_PA_CHECK(std::is_nothrow_copy_constructible<pa_alloc>::value);
    D_PA_CHECK(original.resource() == &pool);

    // a chain of copies all name the same pool
    pa_alloc second(copy);
    pa_alloc third(second);
    D_PA_CHECK(third.resource() == &pool);

    return true;
}

/*
tests_allocator_copy_assignment_rebinds
  Verifies copy assignment repoints the handle at the source's pool,
  including assigning from a handle that names no pool at all.
*/
bool
tests_allocator_copy_assignment_rebinds()
{
    pa_pool  first(4);
    pa_pool  second(4);

    pa_alloc allocator(first);
    D_PA_CHECK(allocator.resource() == &first);

    const pa_alloc other(second);
    allocator = other;

    D_PA_CHECK(allocator.resource() == &second);
    D_PA_CHECK(allocator.resource() != &first);

    // allocations now come from the new pool
    pa_element* slot = allocator.allocate(1);
    D_PA_CHECK(second.size() == 1u);
    D_PA_CHECK(first.size() == 0u);
    allocator.deallocate(slot, 1);

    // assignment is noexcept and returns a usable reference
    D_PA_CHECK(std::is_nothrow_copy_assignable<pa_alloc>::value);

    pa_alloc third(first);
    D_PA_CHECK((&(third = other)) == &third);
    D_PA_CHECK(third.resource() == &second);

    // self-assignment leaves the binding intact
    third = third;
    D_PA_CHECK(third.resource() == &second);

    return true;
}

/*
tests_allocator_converting_constructor_yields_no_pool
  Verifies converting from an allocator of another element type produces a
  handle with NO pool.  A pool_resource is typed, so sharing one across
  element types would be unsound; the header trades that for a handle the
  caller must rebind explicitly.
*/
bool
tests_allocator_converting_constructor_yields_no_pool()
{
    pa_pool        pool(8);
    const pa_alloc source(pool);

    // the conversion is available - the Allocator requirements need it
    D_PA_CHECK((std::is_constructible<pa_other_alloc,
                                      const pa_alloc&>::value));
    D_PA_CHECK((std::is_nothrow_constructible<pa_other_alloc,
                                              const pa_alloc&>::value));

    pa_other_alloc rebound(source);

    // ... and it deliberately carries no binding across
    D_PA_CHECK(rebound.resource() == nullptr);
    D_PA_CHECK(source.resource() == &pool);

    // the same holds through the rebind alias
    using via_rebind = pa_alloc::rebind<pa_other>::other;
    via_rebind through_alias(source);
    D_PA_CHECK(through_alias.resource() == nullptr);

    // the handle is still usable - it simply falls back
    pa_other* storage = rebound.allocate(1);
    D_PA_CHECK(storage != nullptr);
    D_PA_CHECK(pool.size() == 0u);
    rebound.deallocate(storage, 1);

    // supplying a correctly typed pool restores pool-backed behaviour
    pa_other_pool other_pool(4);
    rebound.set_resource(other_pool);
    D_PA_CHECK(rebound.resource() == &other_pool);

    pa_other* pooled = rebound.allocate(1);
    D_PA_CHECK(other_pool.size() == 1u);
    rebound.deallocate(pooled, 1);
    D_PA_CHECK(other_pool.size() == 0u);

    return true;
}

/*
tests_allocator_set_resource_rebinds
  Verifies set_resource repoints an existing handle at a different pool
  and takes effect immediately on subsequent allocations.
*/
bool
tests_allocator_set_resource_rebinds()
{
    pa_pool  first(4);
    pa_pool  second(4);
    pa_alloc allocator(first);

    D_PA_CHECK(allocator.resource() == &first);

    allocator.set_resource(second);
    D_PA_CHECK(allocator.resource() == &second);

    pa_element* slot = allocator.allocate(1);
    D_PA_CHECK(second.size() == 1u);
    D_PA_CHECK(first.size()  == 0u);

    // returning it goes to the currently bound pool
    allocator.deallocate(slot, 1);
    D_PA_CHECK(second.size() == 0u);

    // rebinding back again works
    allocator.set_resource(first);
    D_PA_CHECK(allocator.resource() == &first);

    slot = allocator.allocate(1);
    D_PA_CHECK(first.size() == 1u);
    allocator.deallocate(slot, 1);

    // set_resource is noexcept and returns nothing
    D_PA_CHECK(noexcept(allocator.set_resource(first)));
    D_PA_CHECK((std::is_same<decltype(allocator.set_resource(first)),
                             void>::value));

    return true;
}

/*
tests_allocator_is_not_default_constructible
  Verifies there is no default constructor: an allocator always names a
  pool, or is converted from one that does.  A container that requires a
  default-constructible allocator therefore will not compile against it,
  which is intentional.
*/
bool
tests_allocator_is_not_default_constructible()
{
    D_PA_CHECK(!std::is_default_constructible<pa_alloc>::value);
    D_PA_CHECK(!std::is_default_constructible<pa_other_alloc>::value);
    D_PA_CHECK(!std::is_default_constructible<pa_mono_alloc>::value);

    // ... unlike std::allocator, which is
    D_PA_CHECK(std::is_default_constructible<std::allocator<int>>::value);

    // the only routes in are a pool reference, a copy, or a conversion
    D_PA_CHECK((std::is_constructible<pa_alloc, pa_pool&>::value));
    D_PA_CHECK((std::is_constructible<pa_alloc, const pa_alloc&>::value));
    D_PA_CHECK((std::is_constructible<pa_alloc,
                                      const pa_other_alloc&>::value));

    // and never from an unrelated type
    D_PA_CHECK((!std::is_constructible<pa_alloc, int>::value));
    D_PA_CHECK((!std::is_constructible<pa_alloc, pa_pool*>::value));

    return true;
}

/*
tests_allocator_operations_are_noexcept
  Verifies the handle operations a container relies on - construction,
  copying, assignment, destruction and the pool accessors - are all
  noexcept.
*/
bool
tests_allocator_operations_are_noexcept()
{
    D_PA_CHECK((std::is_nothrow_constructible<pa_alloc, pa_pool&>::value));
    D_PA_CHECK(std::is_nothrow_copy_constructible<pa_alloc>::value);
    D_PA_CHECK(std::is_nothrow_copy_assignable<pa_alloc>::value);
    D_PA_CHECK(std::is_nothrow_destructible<pa_alloc>::value);
    D_PA_CHECK((std::is_nothrow_constructible<pa_other_alloc,
                                              const pa_alloc&>::value));

    pa_pool  pool;
    pa_alloc allocator(pool);

    D_PA_CHECK(noexcept(allocator.resource()));
    D_PA_CHECK(noexcept(allocator.set_resource(pool)));
    D_PA_CHECK(noexcept(allocator.max_size()));
    D_PA_CHECK(noexcept(allocator.select_on_container_copy_construction()));

    // deallocate is noexcept; allocate is NOT, because it may throw
    // bad_alloc when the pool cannot satisfy the request
    D_PA_CHECK(noexcept(allocator.deallocate(nullptr, 1)));
    D_PA_CHECK(!noexcept(allocator.allocate(1)));

    return true;
}

/*
tests_select_on_container_copy_shares_the_pool
  Verifies a container copy inherits the same pool rather than a fresh
  one, which is what the false propagation traits imply.
*/
bool
tests_select_on_container_copy_shares_the_pool()
{
    pa_pool        pool(8);
    const pa_alloc original(pool);

    const pa_alloc selected =
        original.select_on_container_copy_construction();

    D_PA_CHECK(selected.resource() == original.resource());
    D_PA_CHECK(selected.resource() == &pool);
    D_PA_CHECK(selected == original);

    D_PA_CHECK((std::is_same<
                    decltype(original
                                 .select_on_container_copy_construction()),
                    pa_alloc>::value));

    // std::allocator_traits routes through the same member
    const pa_alloc through_traits =
        std::allocator_traits<pa_alloc>::
            select_on_container_copy_construction(original);

    D_PA_CHECK(through_traits.resource() == &pool);
    D_PA_CHECK(through_traits == original);

    // a pool-less handle selects another pool-less handle
    pa_other_alloc unbound(original);
    D_PA_CHECK(unbound.resource() == nullptr);
    D_PA_CHECK(unbound.select_on_container_copy_construction().resource()
               == nullptr);

    return true;
}


NS_END  // testing
NS_END  // djinterp
