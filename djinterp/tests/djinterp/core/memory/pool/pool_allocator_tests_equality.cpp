/*******************************************************************************
* djinterp [test]                              pool_allocator_tests_equality.cpp
*
*   Section V of the pool allocator suite - allocator equality.
*
*   The standard requires that memory obtained from one allocator can be
* released through any allocator that compares equal to it.  For a handle
* that owns nothing and merely points at a pool, that reduces to a single
* question: do the two name the same pool?  The comparison forks on the
* element type - two handles of the same type compare their resources
* directly, while handles of different types are equal only when neither
* names a pool at all, because a typed pool can never be shared across
* element types.
*
*   Every test here checks the substantive promise as well as the boolean:
* where two allocators compare equal, one of them frees what the other
* allocated.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_allocator_tests_equality.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <memory>
// djinterp
#include "pool_allocator_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_allocators_on_one_pool_compare_equal
  Verifies two independently constructed handles on one pool compare
  equal, that equality is reflexive and symmetric, and that the promise
  behind it holds - either handle can free what the other allocated.
*/
bool
tests_allocators_on_one_pool_compare_equal()
{
    pa_pool  pool(8);
    pa_alloc first(pool);
    pa_alloc second(pool);

    // constructed separately, never copied from one another
    D_PA_CHECK(first == second);
    D_PA_CHECK(second == first);

    // reflexive
    D_PA_CHECK(first == first);
    D_PA_CHECK(second == second);

    // and the substantive promise: equal allocators are interchangeable
    pa_element* slot = first.allocate(1);
    D_PA_CHECK(pool.size() == 1u);

    second.deallocate(slot, 1);
    D_PA_CHECK(pool.size() == 0u);

    // the slot really did return to the shared pool
    D_PA_CHECK(second.allocate(1) == slot);
    D_PA_CHECK(pool.size() == 1u);

    // a third handle reached by rebinding back and forth still matches
    pa_alloc third(pool);
    D_PA_CHECK(third == first);

    // and set_resource onto the same pool preserves equality
    pa_pool  elsewhere(4);
    pa_alloc wanderer(elsewhere);
    D_PA_CHECK(!(wanderer == first));

    wanderer.set_resource(pool);
    D_PA_CHECK(wanderer == first);

    return true;
}

/*
tests_allocators_on_different_pools_compare_unequal
  Verifies handles naming different pools of the same type compare
  unequal, even when the pools are configured identically and hold the
  same contents - identity is the pool address, not its shape.
*/
bool
tests_allocators_on_different_pools_compare_unequal()
{
    pa_pool  first(8);
    pa_pool  second(8);

    pa_alloc bound_first(first);
    pa_alloc bound_second(second);

    D_PA_CHECK(!(bound_first == bound_second));
    D_PA_CHECK(!(bound_second == bound_first));

    // identical configuration and identical use do not make them equal
    bound_first.allocate(1);
    bound_second.allocate(1);
    D_PA_CHECK(first.size() == second.size());
    D_PA_CHECK(first.capacity() == second.capacity());
    D_PA_CHECK(!(bound_first == bound_second));

    // a handle repointed at the other pool becomes equal to its handle
    bound_first.set_resource(second);
    D_PA_CHECK(bound_first == bound_second);

    // ... and unequal to what it used to match
    pa_alloc still_on_first(first);
    D_PA_CHECK(!(bound_first == still_on_first));

    // an unbound handle matches neither
    pa_other_alloc unbound(bound_second);
    D_PA_CHECK(unbound.resource() == nullptr);
    D_PA_CHECK(!(unbound == bound_second));

    return true;
}

/*
tests_copies_compare_equal_to_their_source
  Verifies copying and copy-assigning preserve equality along a whole
  chain of handles, and that a copy really can free what its source
  allocated.
*/
bool
tests_copies_compare_equal_to_their_source()
{
    pa_pool  pool(8);
    pa_alloc original(pool);

    // copy construction
    pa_alloc copy(original);
    D_PA_CHECK(copy == original);
    D_PA_CHECK(original == copy);

    // a chain of copies stays equal end to end
    pa_alloc second(copy);
    pa_alloc third(second);
    D_PA_CHECK(third == original);
    D_PA_CHECK(third == copy);

    // copy assignment
    pa_pool  other(4);
    pa_alloc assigned(other);
    D_PA_CHECK(!(assigned == original));

    assigned = original;
    D_PA_CHECK(assigned == original);

    // select_on_container_copy_construction preserves it too
    D_PA_CHECK(original.select_on_container_copy_construction() ==
               original);

    // the promise: a copy frees what the source allocated
    pa_element* slot = original.allocate(1);
    D_PA_CHECK(pool.size() == 1u);

    third.deallocate(slot, 1);
    D_PA_CHECK(pool.size() == 0u);
    D_PA_CHECK(original.allocate(1) == slot);

    return true;
}

/*
tests_cross_type_allocators_compare_by_null_resource
  Verifies allocators of different element types are equal only when both
  name no pool at all - the branch that exists because a pool_resource is
  typed and cannot be shared across element types.
*/
bool
tests_cross_type_allocators_compare_by_null_resource()
{
    pa_pool       element_pool(8);
    pa_other_pool other_pool(8);

    pa_alloc       bound_element(element_pool);
    pa_other_alloc bound_other(other_pool);

    // both bound, to genuinely different pools: never equal
    D_PA_CHECK(!(bound_element == bound_other));
    D_PA_CHECK(!(bound_other == bound_element));

    // one bound, one not: still not equal
    pa_other_alloc unbound_other(bound_element);
    D_PA_CHECK(unbound_other.resource() == nullptr);
    D_PA_CHECK(!(bound_element == unbound_other));
    D_PA_CHECK(!(unbound_other == bound_element));

    // both unbound: equal, because neither can allocate from a pool and
    // both therefore route to the same global allocator
    pa_alloc unbound_element(unbound_other);
    D_PA_CHECK(unbound_element.resource() == nullptr);
    D_PA_CHECK(unbound_element == unbound_other);
    D_PA_CHECK(unbound_other == unbound_element);

    // and that equality carries its usual promise: storage from one is
    // releasable through the other, since both use operator new
    pa_element* storage = unbound_element.allocate(1);
    D_PA_CHECK(storage != nullptr);
    D_PA_CHECK(element_pool.size() == 0u);
    unbound_element.deallocate(storage, 1);

    // binding one of them breaks the equality again
    unbound_element.set_resource(element_pool);
    D_PA_CHECK(!(unbound_element == unbound_other));

    // same-type comparison still uses the resource, not this branch
    pa_alloc also_element(element_pool);
    D_PA_CHECK(unbound_element == also_element);

    return true;
}

/*
tests_inequality_is_the_negation_of_equality
  Verifies operator!= is exactly the complement of operator== in every
  configuration the suite exercises - same pool, different pools, copies,
  cross-type, and both-unbound.
*/
bool
tests_inequality_is_the_negation_of_equality()
{
    pa_pool       pool(8);
    pa_pool       other_pool(8);
    pa_other_pool cross_pool(8);

    pa_alloc       same_a(pool);
    pa_alloc       same_b(pool);
    pa_alloc       different(other_pool);
    pa_alloc       copied(same_a);
    pa_other_alloc cross(cross_pool);
    pa_other_alloc unbound_cross(same_a);
    pa_alloc       unbound_same(unbound_cross);

    // same pool
    D_PA_CHECK((same_a != same_b) == !(same_a == same_b));
    D_PA_CHECK(!(same_a != same_b));

    // different pools
    D_PA_CHECK((same_a != different) == !(same_a == different));
    D_PA_CHECK(same_a != different);

    // a copy
    D_PA_CHECK((same_a != copied) == !(same_a == copied));
    D_PA_CHECK(!(same_a != copied));

    // cross type, both bound
    D_PA_CHECK((same_a != cross) == !(same_a == cross));
    D_PA_CHECK(same_a != cross);

    // cross type, one bound
    D_PA_CHECK((same_a != unbound_cross) == !(same_a == unbound_cross));
    D_PA_CHECK(same_a != unbound_cross);

    // cross type, both unbound
    D_PA_CHECK((unbound_same != unbound_cross) ==
               !(unbound_same == unbound_cross));
    D_PA_CHECK(!(unbound_same != unbound_cross));

    // reflexive inequality is always false
    D_PA_CHECK(!(same_a != same_a));
    D_PA_CHECK(!(cross != cross));

    return true;
}

/*
tests_equality_is_noexcept_and_const
  Verifies both comparison operators are noexcept, operate on const
  references, and yield bool - the form std::allocator_traits and the
  containers require.
*/
bool
tests_equality_is_noexcept_and_const()
{
    pa_pool       pool(4);
    pa_other_pool cross_pool(4);

    const pa_alloc       left(pool);
    const pa_alloc       right(pool);
    const pa_other_alloc cross(cross_pool);

    // callable on const objects at all
    D_PA_CHECK(left == right);
    D_PA_CHECK(!(left != right));

    D_PA_CHECK(noexcept(left == right));
    D_PA_CHECK(noexcept(left != right));
    D_PA_CHECK(noexcept(left == cross));
    D_PA_CHECK(noexcept(left != cross));

    D_PA_CHECK((std::is_same<decltype(left == right), bool>::value));
    D_PA_CHECK((std::is_same<decltype(left != right), bool>::value));
    D_PA_CHECK((std::is_same<decltype(left == cross), bool>::value));

    // the comparison does not disturb either operand
    D_PA_CHECK(left.resource() == &pool);
    D_PA_CHECK(right.resource() == &pool);
    D_PA_CHECK(pool.size() == 0u);

    return true;
}


NS_END  // testing
NS_END  // djinterp
