/*******************************************************************************
* djinterp [test]                                        pool_tests_concepts.cpp
*
*   Sections XV to XIX of the pool suite - the C++20 concept face.
*
*   Each concept is a thin restatement of a trait, so the contract under
* test is agreement: a concept must be satisfied by exactly the types its
* underlying trait accepts, and its complement concept must partition the
* same set.  Satisfaction is read through requires-expressions so a
* mismatch is a failed check rather than a failed compile.
*
*   This translation unit is gated on __cpp_concepts, matching the gate the
* module puts on the declarations themselves.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <memory>
#include <vector>
// djinterp
#include "pool_tests.hpp"


#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

NS_DJINTERP
NS_TESTING


namespace
{
    // duck allocators over pools of each release policy, so the
    // extraction concepts have something to classify.
    struct free_list_backed_allocator
    {
        using value_type = pool_medium_t;

        pool_fl_chunked* resource() const noexcept { return nullptr; }
    };

    struct relocatable_backed_allocator
    {
        using value_type = pool_medium_t;

        pool_fl_contig* resource() const noexcept { return nullptr; }
    };

    struct monotonic_backed_allocator
    {
        using value_type = pool_medium_t;

        pool_mono_chunked* resource() const noexcept { return nullptr; }
    };

    struct generational_backed_allocator
    {
        using value_type = pool_medium_t;

        pool_gen_chunked* resource() const noexcept { return nullptr; }
    };

    struct free_list_backed_container
    {
        using allocator_type = free_list_backed_allocator;
    };
}


/*
tests_pool_resource_concepts
  Verifies pool_resource_c is satisfied by exactly the types
  is_pool_resource_v accepts, that non_pool_resource is its complement,
  and that the protocol-fragment concepts track their own detectors.
*/
bool
tests_pool_resource_concepts()
{
    // agreement with the trait, across the policy matrix
    D_POOL_CHECK(pool_resource_c<pool_fl_chunked>);
    D_POOL_CHECK(pool_resource_c<pool_fl_contig>);
    D_POOL_CHECK(pool_resource_c<pool_mono_chunked>);
    D_POOL_CHECK(pool_resource_c<pool_gen_contig>);
    D_POOL_CHECK(pool_resource_c<default_pool_resource<int>>);
    D_POOL_CHECK(pool_resource_c<pool_duck_resource>);

    D_POOL_CHECK(!pool_resource_c<pool_not_a_pool>);
    D_POOL_CHECK(!pool_resource_c<pool_duck_partial>);
    D_POOL_CHECK(!pool_resource_c<std::vector<int>>);
    D_POOL_CHECK(!pool_resource_c<int>);

    D_POOL_CHECK(pool_resource_c<pool_fl_chunked> ==
               is_pool_resource_v<pool_fl_chunked>);
    D_POOL_CHECK(pool_resource_c<std::vector<int>> ==
               is_pool_resource_v<std::vector<int>>);

    // cv and reference qualified forms are accepted alike
    D_POOL_CHECK(pool_resource_c<const pool_fl_chunked&>);
    D_POOL_CHECK(pool_resource_c<pool_fl_chunked&&>);

    // non_pool_resource is the exact complement
    D_POOL_CHECK(!non_pool_resource<pool_fl_chunked>);
    D_POOL_CHECK(non_pool_resource<pool_not_a_pool>);
    D_POOL_CHECK(non_pool_resource<int>);
    D_POOL_CHECK(non_pool_resource<std::vector<int>>);

    // the protocol fragments
    D_POOL_CHECK(acquiring_pool<pool_fl_chunked>);
    D_POOL_CHECK(acquiring_pool<pool_duck_partial>);
    D_POOL_CHECK(!acquiring_pool<pool_not_a_pool>);

    D_POOL_CHECK(releasing_pool<pool_fl_chunked>);
    D_POOL_CHECK(releasing_pool<pool_duck_partial>);
    D_POOL_CHECK(!releasing_pool<pool_not_a_pool>);

    D_POOL_CHECK(sized_pool<pool_fl_chunked>);
    D_POOL_CHECK(sized_pool<std::vector<int>>);
    D_POOL_CHECK(!sized_pool<pool_duck_partial>);

    D_POOL_CHECK(typed_pool<pool_fl_chunked>);
    D_POOL_CHECK(typed_pool<std::vector<int>>);
    D_POOL_CHECK(!typed_pool<pool_duck_partial>);

    // classified_pool is the disjunction of the classifiers
    D_POOL_CHECK(classified_pool<pool_fl_chunked>);
    D_POOL_CHECK(classified_pool<pool_mono_contig>);
    D_POOL_CHECK(classified_pool<pool_duck_stable>);
    D_POOL_CHECK(!classified_pool<pool_not_a_pool>);
    D_POOL_CHECK(!classified_pool<int>);

    return true;
}

/*
tests_stability_and_release_concepts
  Verifies the stability and release concepts partition the pool matrix
  exactly as their traits do, including relocatable_pool's requirement
  that the subject be a pool at all.
*/
bool
tests_stability_and_release_concepts()
{
    // pointer stability
    D_POOL_CHECK(pointer_stable_pool<pool_fl_chunked>);
    D_POOL_CHECK(pointer_stable_pool<pool_mono_chunked>);
    D_POOL_CHECK(pointer_stable_pool<pool_gen_chunked>);
    D_POOL_CHECK(!pointer_stable_pool<pool_fl_contig>);
    D_POOL_CHECK(!pointer_stable_pool<pool_gen_contig>);

    // relocatable is "a pool, but not stable" - a non-pool is neither
    D_POOL_CHECK(relocatable_pool<pool_fl_contig>);
    D_POOL_CHECK(relocatable_pool<pool_mono_contig>);
    D_POOL_CHECK(!relocatable_pool<pool_fl_chunked>);
    D_POOL_CHECK(!relocatable_pool<pool_not_a_pool>);
    D_POOL_CHECK(!relocatable_pool<int>);

    // every pool is exactly one of stable or relocatable
    D_POOL_CHECK(pointer_stable_pool<pool_fl_chunked> !=
               relocatable_pool<pool_fl_chunked>);
    D_POOL_CHECK(pointer_stable_pool<pool_fl_contig> !=
               relocatable_pool<pool_fl_contig>);

    // individual release
    D_POOL_CHECK(individually_releasing_pool<pool_fl_chunked>);
    D_POOL_CHECK(individually_releasing_pool<pool_gen_chunked>);
    D_POOL_CHECK(!individually_releasing_pool<pool_mono_chunked>);
    D_POOL_CHECK(!individually_releasing_pool<pool_not_a_pool>);

    // monotonic
    D_POOL_CHECK(monotonic_pool<pool_mono_chunked>);
    D_POOL_CHECK(monotonic_pool<pool_mono_contig>);
    D_POOL_CHECK(!monotonic_pool<pool_fl_chunked>);
    D_POOL_CHECK(!monotonic_pool<pool_gen_chunked>);
    D_POOL_CHECK(!monotonic_pool<pool_not_a_pool>);

    // generational
    D_POOL_CHECK(generational_pool<pool_gen_chunked>);
    D_POOL_CHECK(generational_pool<pool_gen_contig>);
    D_POOL_CHECK(!generational_pool<pool_fl_chunked>);
    D_POOL_CHECK(!generational_pool<pool_mono_chunked>);

    // and each agrees with its trait
    D_POOL_CHECK(monotonic_pool<pool_mono_chunked> ==
               is_monotonic_pool_v<pool_mono_chunked>);
    D_POOL_CHECK(generational_pool<pool_gen_chunked> ==
               has_generational_sweep_v<pool_gen_chunked>);

    return true;
}

/*
tests_capability_concepts
  Verifies the capability concepts track reset, reserve, slot geometry,
  the generation pair, and the accounting triple - each satisfied by every
  pool and by nothing that lacks the member.
*/
bool
tests_capability_concepts()
{
    D_POOL_CHECK(resettable_pool<pool_fl_chunked>);
    D_POOL_CHECK(resettable_pool<pool_mono_contig>);
    D_POOL_CHECK(!resettable_pool<pool_duck_resource>);
    D_POOL_CHECK(!resettable_pool<pool_not_a_pool>);

    D_POOL_CHECK(reservable_pool<pool_fl_chunked>);
    D_POOL_CHECK(!reservable_pool<pool_duck_resource>);

    D_POOL_CHECK(slot_sized_pool<pool_fl_chunked>);
    D_POOL_CHECK(slot_aligned_pool<pool_fl_chunked>);
    D_POOL_CHECK(!slot_sized_pool<pool_duck_resource>);
    D_POOL_CHECK(!slot_aligned_pool<pool_not_a_pool>);

    // the generation pair follows the release policy
    D_POOL_CHECK(generation_tracked_pool<pool_gen_chunked>);
    D_POOL_CHECK(generation_advancing_pool<pool_gen_chunked>);
    D_POOL_CHECK(generation_tracked_pool<pool_gen_contig>);
    D_POOL_CHECK(!generation_tracked_pool<pool_fl_chunked>);
    D_POOL_CHECK(!generation_advancing_pool<pool_fl_chunked>);
    D_POOL_CHECK(!generation_tracked_pool<pool_mono_chunked>);

    // accounting: the aggregate and each component
    D_POOL_CHECK(memory_accounting_pool<pool_fl_chunked>);
    D_POOL_CHECK(bytes_allocated_pool<pool_fl_chunked>);
    D_POOL_CHECK(bytes_in_use_pool<pool_fl_chunked>);
    D_POOL_CHECK(utilization_reporting_pool<pool_fl_chunked>);

    D_POOL_CHECK(!memory_accounting_pool<pool_duck_resource>);
    D_POOL_CHECK(!bytes_allocated_pool<pool_duck_resource>);
    D_POOL_CHECK(!utilization_reporting_pool<std::vector<int>>);

    // the aggregate implies each component
    D_POOL_CHECK(memory_accounting_pool<pool_gen_contig>);
    D_POOL_CHECK(bytes_allocated_pool<pool_gen_contig>);
    D_POOL_CHECK(bytes_in_use_pool<pool_gen_contig>);
    D_POOL_CHECK(utilization_reporting_pool<pool_gen_contig>);

    return true;
}

/*
tests_allocator_and_container_concepts
  Verifies the allocator and container concepts agree with their traits,
  and that non_pool_allocator and non_pool_backed_container are exact
  complements.
*/
bool
tests_allocator_and_container_concepts()
{
    D_POOL_CHECK(pool_allocator_c<free_list_backed_allocator>);
    D_POOL_CHECK(pool_allocator_c<monotonic_backed_allocator>);
    D_POOL_CHECK(pool_allocator_c<pool_duck_allocator>);
    D_POOL_CHECK(!pool_allocator_c<std::allocator<int>>);
    D_POOL_CHECK(!pool_allocator_c<pool_fl_chunked>);
    D_POOL_CHECK(!pool_allocator_c<int>);

    D_POOL_CHECK(pool_allocator_c<free_list_backed_allocator> ==
               is_pool_allocator_v<free_list_backed_allocator>);

    // exact complement
    D_POOL_CHECK(!non_pool_allocator<free_list_backed_allocator>);
    D_POOL_CHECK(non_pool_allocator<std::allocator<int>>);
    D_POOL_CHECK(non_pool_allocator<int>);

    // the resource accessor on its own
    D_POOL_CHECK(resource_exposing_pool_allocator<
                   free_list_backed_allocator>);
    D_POOL_CHECK(!resource_exposing_pool_allocator<std::allocator<int>>);

    // containers
    D_POOL_CHECK(pool_backed_container<free_list_backed_container>);
    D_POOL_CHECK(pool_backed_container<pool_duck_container>);
    D_POOL_CHECK(!pool_backed_container<pool_duck_plain_container>);
    D_POOL_CHECK(!pool_backed_container<std::vector<int>>);
    D_POOL_CHECK(!pool_backed_container<int>);

    D_POOL_CHECK(!non_pool_backed_container<free_list_backed_container>);
    D_POOL_CHECK(non_pool_backed_container<std::vector<int>>);
    D_POOL_CHECK(non_pool_backed_container<int>);

    // allocator_aware_pool_container adds the allocator_type requirement
    D_POOL_CHECK(allocator_aware_pool_container<free_list_backed_container>);
    D_POOL_CHECK(!allocator_aware_pool_container<std::vector<int>>);
    D_POOL_CHECK(!allocator_aware_pool_container<pool_fl_chunked>);

    return true;
}

/*
tests_resource_extraction_concepts
  Verifies the extraction concepts classify an allocator by the properties
  of the pool its resource() names - stability, monotonicity and
  generational support all read through the extracted type.
*/
bool
tests_resource_extraction_concepts()
{
    // extraction succeeds only for a pool allocator with a real resource
    D_POOL_CHECK(pool_allocator_with_resource<free_list_backed_allocator>);
    D_POOL_CHECK(pool_allocator_with_resource<monotonic_backed_allocator>);
    D_POOL_CHECK(!pool_allocator_with_resource<std::allocator<int>>);
    D_POOL_CHECK(!pool_allocator_with_resource<int>);

    // stability read through the resource
    D_POOL_CHECK(stable_pool_allocator<free_list_backed_allocator>);
    D_POOL_CHECK(stable_pool_allocator<monotonic_backed_allocator>);
    D_POOL_CHECK(stable_pool_allocator<generational_backed_allocator>);
    D_POOL_CHECK(!stable_pool_allocator<relocatable_backed_allocator>);
    D_POOL_CHECK(!stable_pool_allocator<std::allocator<int>>);

    // monotonicity read through the resource
    D_POOL_CHECK(monotonic_pool_allocator_c<monotonic_backed_allocator>);
    D_POOL_CHECK(!monotonic_pool_allocator_c<free_list_backed_allocator>);
    D_POOL_CHECK(!monotonic_pool_allocator_c<generational_backed_allocator>);
    D_POOL_CHECK(!monotonic_pool_allocator_c<std::allocator<int>>);

    // generational support read through the resource
    D_POOL_CHECK(generational_pool_allocator<generational_backed_allocator>);
    D_POOL_CHECK(!generational_pool_allocator<free_list_backed_allocator>);
    D_POOL_CHECK(!generational_pool_allocator<monotonic_backed_allocator>);

    // each concept agrees with the trait applied to the extracted type
    D_POOL_CHECK(stable_pool_allocator<free_list_backed_allocator> ==
               is_pointer_stable_pool_v<
                   pool_resource_type_t<free_list_backed_allocator>>);
    D_POOL_CHECK(monotonic_pool_allocator_c<monotonic_backed_allocator> ==
               is_monotonic_pool_v<
                   pool_resource_type_t<monotonic_backed_allocator>>);

    return true;
}


NS_END  // testing
NS_END  // djinterp

#endif  // __cpp_concepts
