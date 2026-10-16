/*******************************************************************************
* djinterp [test]                                 pool_allocator_tests_types.cpp
*
*   Section I of the pool allocator suite - the type surface.
*
*   The Allocator named requirements are largely a list of nested names an
* allocator must present.  These tests pin every one of them, confirm the
* propagation traits carry the PMR-style values the header documents, and
* check that std::allocator_traits reads back exactly what the allocator
* declares rather than falling through to a default.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_allocator_tests_types.cpp
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
tests_allocator_exposes_standard_type_aliases
  Verifies the standard allocator aliases are present and correctly
  formed: value_type is the element, pointer and reference are derived
  from it, and the size and difference types are the standard ones.
*/
bool
tests_allocator_exposes_standard_type_aliases()
{
    D_PA_CHECK((std::is_same<pa_alloc::value_type, pa_element>::value));
    D_PA_CHECK((std::is_same<pa_alloc::pointer, pa_element*>::value));
    D_PA_CHECK((std::is_same<pa_alloc::const_pointer,
                             const pa_element*>::value));
    D_PA_CHECK((std::is_same<pa_alloc::reference, pa_element&>::value));
    D_PA_CHECK((std::is_same<pa_alloc::const_reference,
                             const pa_element&>::value));
    D_PA_CHECK((std::is_same<pa_alloc::size_type, std::size_t>::value));
    D_PA_CHECK((std::is_same<pa_alloc::difference_type,
                             std::ptrdiff_t>::value));

    // the aliases track the element type, not the allocator
    D_PA_CHECK((std::is_same<pa_other_alloc::value_type, pa_other>::value));
    D_PA_CHECK((std::is_same<pa_other_alloc::pointer, pa_other*>::value));
    D_PA_CHECK((std::is_same<pa_over_alloc::value_type,
                             pa_overaligned>::value));

    // the allocator is a small, copyable handle
    D_PA_CHECK(std::is_copy_constructible<pa_alloc>::value);
    D_PA_CHECK(std::is_copy_assignable<pa_alloc>::value);
    D_PA_CHECK(std::is_destructible<pa_alloc>::value);
    D_PA_CHECK(sizeof(pa_alloc) == sizeof(pa_pool*));

    return true;
}

/*
tests_allocator_pool_type_alias_matches_its_policies
  Verifies pool_type names pool_resource with exactly the allocator's own
  element type and policy triple - the allocator cannot silently name a
  differently configured pool.
*/
bool
tests_allocator_pool_type_alias_matches_its_policies()
{
    using expected = pool_resource<pa_element,
                                   free_list_release_policy,
                                   chunked_block_policy<4>,
                                   pa_growth_exact>;

    D_PA_CHECK((std::is_same<pa_alloc::pool_type, expected>::value));
    D_PA_CHECK((std::is_same<pa_pool, expected>::value));

    // each axis reaches the pool intact
    D_PA_CHECK((std::is_same<pa_alloc::pool_type::value_type,
                             pa_alloc::value_type>::value));
    D_PA_CHECK((std::is_same<pa_alloc::pool_type::release_policy,
                             free_list_release_policy>::value));
    D_PA_CHECK((std::is_same<pa_alloc::pool_type::block_policy,
                             chunked_block_policy<4>>::value));
    D_PA_CHECK((std::is_same<pa_alloc::pool_type::growth_policy,
                             pa_growth_exact>::value));

    // a different policy choice yields a different pool type
    D_PA_CHECK((!std::is_same<pa_alloc::pool_type,
                              pa_mono_alloc::pool_type>::value));
    D_PA_CHECK((std::is_same<pa_mono_alloc::pool_type::release_policy,
                             monotonic_release_policy>::value));

    return true;
}

/*
tests_allocator_propagation_traits
  Verifies all three propagate_on_container_* traits and is_always_equal
  are false_type - the allocator is a stateful handle bound to one pool,
  so a container must not silently adopt a different one.
*/
bool
tests_allocator_propagation_traits()
{
    D_PA_CHECK((std::is_same<
                    pa_alloc::propagate_on_container_copy_assignment,
                    std::false_type>::value));
    D_PA_CHECK((std::is_same<
                    pa_alloc::propagate_on_container_move_assignment,
                    std::false_type>::value));
    D_PA_CHECK((std::is_same<pa_alloc::propagate_on_container_swap,
                             std::false_type>::value));
    D_PA_CHECK((std::is_same<pa_alloc::is_always_equal,
                             std::false_type>::value));

    D_PA_CHECK(pa_alloc::propagate_on_container_copy_assignment::value ==
               false);
    D_PA_CHECK(pa_alloc::propagate_on_container_move_assignment::value ==
               false);
    D_PA_CHECK(pa_alloc::propagate_on_container_swap::value == false);
    D_PA_CHECK(pa_alloc::is_always_equal::value == false);

    // a stateful allocator must not claim to be always equal - unlike
    // std::allocator, which may
    D_PA_CHECK(std::allocator_traits<std::allocator<int>>
                   ::is_always_equal::value == true);

    // the same values hold for every specialisation
    D_PA_CHECK(pa_other_alloc::is_always_equal::value == false);
    D_PA_CHECK(pa_mono_alloc::propagate_on_container_swap::value == false);

    return true;
}

/*
tests_allocator_rebind_produces_the_expected_type
  Verifies rebind<U>::other names the allocator for U, and that rebinding
  to the allocator's own element type is the identity.
*/
bool
tests_allocator_rebind_produces_the_expected_type()
{
    using rebound_other =
        pa_alloc::rebind<pa_other>::other;
    using rebound_self =
        pa_alloc::rebind<pa_element>::other;
    using rebound_int =
        pa_alloc::rebind<int>::other;

    D_PA_CHECK((std::is_same<rebound_other, pa_other_alloc>::value));
    D_PA_CHECK((std::is_same<rebound_self, pa_alloc>::value));

    D_PA_CHECK((std::is_same<rebound_other::value_type, pa_other>::value));
    D_PA_CHECK((std::is_same<rebound_int::value_type, int>::value));

    // rebinding is idempotent and reversible
    D_PA_CHECK((std::is_same<
                    rebound_other::rebind<pa_element>::other,
                    pa_alloc>::value));
    D_PA_CHECK((std::is_same<
                    rebound_self::rebind<pa_element>::other,
                    pa_alloc>::value));

    // the rebound allocator names a pool over the NEW element type
    D_PA_CHECK((std::is_same<rebound_other::pool_type,
                             pa_other_pool>::value));
    D_PA_CHECK((!std::is_same<rebound_other::pool_type,
                              pa_pool>::value));

    return true;
}

/*
tests_allocator_rebind_preserves_the_policy_triple
  Verifies only the element type changes under rebinding - the release,
  block and growth policies are carried through unchanged, so a rebound
  allocator would draw from a pool configured the same way.
*/
bool
tests_allocator_rebind_preserves_the_policy_triple()
{
    using rebound = pa_alloc::rebind<pa_other>::other;

    D_PA_CHECK((std::is_same<rebound::pool_type::release_policy,
                             free_list_release_policy>::value));
    D_PA_CHECK((std::is_same<rebound::pool_type::block_policy,
                             chunked_block_policy<4>>::value));
    D_PA_CHECK((std::is_same<rebound::pool_type::growth_policy,
                             pa_growth_exact>::value));

    // an allocator with a different triple rebinds to that triple
    using mono_rebound = pa_mono_alloc::rebind<pa_other>::other;

    D_PA_CHECK((std::is_same<mono_rebound::pool_type::release_policy,
                             monotonic_release_policy>::value));
    D_PA_CHECK((!std::is_same<mono_rebound, rebound>::value));

    // the fixed-growth allocator likewise
    using fixed_rebound = pa_fixed_alloc::rebind<pa_other>::other;

    D_PA_CHECK((std::is_same<fixed_rebound::pool_type::growth_policy,
                             pa_growth_none>::value));

    return true;
}

/*
tests_allocator_traits_agree_with_the_allocator
  Verifies std::allocator_traits reads back the aliases, propagation
  traits and rebind the allocator declares, rather than substituting its
  own defaults.
*/
bool
tests_allocator_traits_agree_with_the_allocator()
{
    using traits = std::allocator_traits<pa_alloc>;

    D_PA_CHECK((std::is_same<traits::allocator_type, pa_alloc>::value));
    D_PA_CHECK((std::is_same<traits::value_type, pa_element>::value));
    D_PA_CHECK((std::is_same<traits::pointer, pa_element*>::value));
    D_PA_CHECK((std::is_same<traits::const_pointer,
                             const pa_element*>::value));
    D_PA_CHECK((std::is_same<traits::void_pointer, void*>::value));
    D_PA_CHECK((std::is_same<traits::const_void_pointer,
                             const void*>::value));
    D_PA_CHECK((std::is_same<traits::size_type, std::size_t>::value));
    D_PA_CHECK((std::is_same<traits::difference_type,
                             std::ptrdiff_t>::value));

    // the propagation traits are taken from the allocator, not defaulted
    D_PA_CHECK(traits::propagate_on_container_copy_assignment::value ==
               false);
    D_PA_CHECK(traits::propagate_on_container_move_assignment::value ==
               false);
    D_PA_CHECK(traits::propagate_on_container_swap::value == false);
    D_PA_CHECK(traits::is_always_equal::value == false);

    // rebind_alloc routes through the allocator's own rebind
    D_PA_CHECK((std::is_same<traits::rebind_alloc<pa_other>,
                             pa_other_alloc>::value));
    D_PA_CHECK((std::is_same<traits::rebind_alloc<pa_element>,
                             pa_alloc>::value));
    D_PA_CHECK((std::is_same<
                    traits::rebind_traits<pa_other>::allocator_type,
                    pa_other_alloc>::value));

    // and max_size is the allocator's, not the traits' fallback
    pa_pool  pool;
    pa_alloc allocator(pool);

    D_PA_CHECK(traits::max_size(allocator) == allocator.max_size());

    return true;
}


NS_END  // testing
NS_END  // djinterp
