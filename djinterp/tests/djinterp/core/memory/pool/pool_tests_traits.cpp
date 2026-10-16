/*******************************************************************************
* djinterp [test]                                          pool_tests_traits.cpp
*
*   Sections IX to XIV of the pool suite - the classification layer.
*
*   The trait layer is purely structural: it duck-types the pool protocol
* and reads the policy constants off the clean_t form, with no tagging and
* no base classes.  So the tests here pair every pool_resource
* instantiation with hand-rolled subjects that match or miss the shape, and
* confirm the traits follow the shape rather than the name.
*
*   The policy-constant readers matter especially: they must yield false
* for a type that has no such constant at all, rather than failing to
* compile.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <string>
#include <vector>
// djinterp
#include "pool_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_method_detection_traits
  Verifies each has_* detector fires exactly on the types exposing the
  corresponding member, and stays false - rather than erroring - on types
  that do not.
*/
bool
tests_method_detection_traits()
{
    using pool_type = pool_fl_chunked;

    // the basic container protocol
    D_POOL_CHECK(has_value_type_v<pool_type>);
    D_POOL_CHECK(has_size_accessor_v<pool_type>);
    D_POOL_CHECK(!has_allocator_type_v<pool_type>);
    D_POOL_CHECK(!has_value_type_v<pool_not_a_pool>);
    D_POOL_CHECK(!has_size_accessor_v<pool_not_a_pool>);
    D_POOL_CHECK(has_allocator_type_v<pool_duck_container>);

    // the pool protocol proper
    D_POOL_CHECK(has_acquire_v<pool_type>);
    D_POOL_CHECK(has_release_v<pool_type>);
    D_POOL_CHECK(has_pool_reset_v<pool_type>);
    D_POOL_CHECK(has_pool_reserve_v<pool_type>);
    D_POOL_CHECK(has_bytes_per_slot_v<pool_type>);
    D_POOL_CHECK(has_slot_alignment_v<pool_type>);

    D_POOL_CHECK(!has_acquire_v<pool_not_a_pool>);
    D_POOL_CHECK(!has_release_v<pool_not_a_pool>);
    D_POOL_CHECK(!has_pool_reset_v<pool_not_a_pool>);
    D_POOL_CHECK(!has_pool_reserve_v<pool_not_a_pool>);
    D_POOL_CHECK(!has_bytes_per_slot_v<pool_not_a_pool>);
    D_POOL_CHECK(!has_slot_alignment_v<pool_not_a_pool>);

    // a partial protocol is detected member by member
    D_POOL_CHECK(has_acquire_v<pool_duck_partial>);
    D_POOL_CHECK(has_release_v<pool_duck_partial>);
    D_POOL_CHECK(!has_size_accessor_v<pool_duck_partial>);
    D_POOL_CHECK(!has_value_type_v<pool_duck_partial>);

    // the policy-constant readers
    D_POOL_CHECK(has_pointer_stable_constant_v<pool_type>);
    D_POOL_CHECK(has_supports_individual_release_constant_v<pool_type>);
    D_POOL_CHECK(has_supports_generational_sweep_constant_v<pool_type>);
    D_POOL_CHECK(has_pointer_stable_constant_v<pool_duck_stable>);
    D_POOL_CHECK(!has_pointer_stable_constant_v<pool_not_a_pool>);
    D_POOL_CHECK(
        !has_supports_individual_release_constant_v<pool_not_a_pool>);

    // the accounting triple
    D_POOL_CHECK(has_bytes_allocated_v<pool_type>);
    D_POOL_CHECK(has_bytes_in_use_v<pool_type>);
    D_POOL_CHECK(has_utilization_v<pool_type>);
    D_POOL_CHECK(!has_bytes_allocated_v<pool_duck_resource>);

    // the allocator's resource accessor
    D_POOL_CHECK(has_resource_method_v<pool_duck_allocator>);
    D_POOL_CHECK(!has_resource_method_v<pool_type>);

    // and the generation pair, gated by the release policy
    D_POOL_CHECK(has_current_generation_v<pool_gen_chunked>);
    D_POOL_CHECK(has_advance_generation_v<pool_gen_chunked>);
    D_POOL_CHECK(!has_current_generation_v<pool_type>);
    D_POOL_CHECK(!has_advance_generation_v<pool_type>);

    // library types trip none of them
    D_POOL_CHECK(!has_acquire_v<std::vector<int>>);
    D_POOL_CHECK(has_value_type_v<std::vector<int>>);
    D_POOL_CHECK(has_size_accessor_v<std::vector<int>>);

    return true;
}

/*
tests_is_pool_resource_accepts_every_pool
  Verifies every policy combination of pool_resource satisfies the minimum
  pool protocol, so classification never depends on the policies chosen.
*/
bool
tests_is_pool_resource_accepts_every_pool()
{
    D_POOL_CHECK(is_pool_resource_v<pool_fl_chunked>);
    D_POOL_CHECK(is_pool_resource_v<pool_fl_contig>);
    D_POOL_CHECK(is_pool_resource_v<pool_mono_chunked>);
    D_POOL_CHECK(is_pool_resource_v<pool_mono_contig>);
    D_POOL_CHECK(is_pool_resource_v<pool_gen_chunked>);
    D_POOL_CHECK(is_pool_resource_v<pool_gen_contig>);
    D_POOL_CHECK(is_pool_resource_v<pool_fl_chunked_fixed>);
    D_POOL_CHECK(is_pool_resource_v<pool_over_chunked>);

    // including the shipped aliases
    D_POOL_CHECK(is_pool_resource_v<default_pool_resource<int>>);
    D_POOL_CHECK(is_pool_resource_v<monotonic_pool_resource<int>>);
    D_POOL_CHECK(is_pool_resource_v<flat_pool_resource<int>>);

    // the struct form and the variable template agree
    D_POOL_CHECK(is_pool_resource<pool_fl_chunked>::value);
    D_POOL_CHECK(is_pool_resource<pool_fl_chunked>::value ==
               is_pool_resource_v<pool_fl_chunked>);

    return true;
}

/*
tests_is_pool_resource_rejects_non_pools
  Verifies plain types, partial protocols, library containers, and
  non-class types are all rejected.
*/
bool
tests_is_pool_resource_rejects_non_pools()
{
    D_POOL_CHECK(!is_pool_resource_v<pool_not_a_pool>);
    D_POOL_CHECK(!is_pool_resource_v<pool_duck_partial>);
    D_POOL_CHECK(!is_pool_resource_v<pool_duck_stable>);
    D_POOL_CHECK(!is_pool_resource_v<pool_duck_allocator>);

    // library types have value_type and size() but no acquire/release
    D_POOL_CHECK(!is_pool_resource_v<std::vector<int>>);
    D_POOL_CHECK(!is_pool_resource_v<std::string>);

    // scalars, void, and function types must not error out
    D_POOL_CHECK(!is_pool_resource_v<int>);
    D_POOL_CHECK(!is_pool_resource_v<void>);
    D_POOL_CHECK(!is_pool_resource_v<double*>);
    D_POOL_CHECK(!is_pool_resource_v<pool_release>);

    // a pointer to a pool is not itself a pool: clean_t strips cv and
    // reference, but never indirection
    D_POOL_CHECK(!is_pool_resource_v<pool_fl_chunked*>);

    return true;
}

/*
tests_is_pool_resource_is_purely_structural
  Verifies a hand-rolled type matching the protocol is accepted with no
  inheritance, tag, or specialisation - the trait duck-types the shape.
*/
bool
tests_is_pool_resource_is_purely_structural()
{
    D_POOL_CHECK(is_pool_resource_v<pool_duck_resource>);

    // it shares no base and no template with the real thing
    D_POOL_CHECK((!std::is_base_of<pool_fl_chunked,
                                 pool_duck_resource>::value));
    D_POOL_CHECK((!std::is_same<pool_duck_resource, pool_fl_chunked>::value));

    // dropping any single member of the protocol drops the verdict
    struct without_acquire
    {
        using value_type = int;

        void        release(void*)        {}
        std::size_t size() const noexcept { return 0; }
    };

    struct without_release
    {
        using value_type = int;

        void*       acquire()             { return nullptr; }
        std::size_t size() const noexcept { return 0; }
    };

    struct without_size
    {
        using value_type = int;

        void* acquire()      { return nullptr; }
        void  release(void*) {}
    };

    struct without_value_type
    {
        void*       acquire()             { return nullptr; }
        void        release(void*)        {}
        std::size_t size() const noexcept { return 0; }
    };

    D_POOL_CHECK(!is_pool_resource_v<without_acquire>);
    D_POOL_CHECK(!is_pool_resource_v<without_release>);
    D_POOL_CHECK(!is_pool_resource_v<without_size>);
    D_POOL_CHECK(!is_pool_resource_v<without_value_type>);

    return true;
}

/*
tests_traits_ignore_cv_and_reference_qualifiers
  Verifies classification runs on the clean_t form, so const, volatile and
  reference qualified pools classify exactly as the bare type does.
*/
bool
tests_traits_ignore_cv_and_reference_qualifiers()
{
    D_POOL_CHECK(is_pool_resource_v<pool_fl_chunked>);
    D_POOL_CHECK(is_pool_resource_v<const pool_fl_chunked>);
    D_POOL_CHECK(is_pool_resource_v<volatile pool_fl_chunked>);
    D_POOL_CHECK(is_pool_resource_v<const volatile pool_fl_chunked>);
    D_POOL_CHECK(is_pool_resource_v<pool_fl_chunked&>);
    D_POOL_CHECK(is_pool_resource_v<const pool_fl_chunked&>);
    D_POOL_CHECK(is_pool_resource_v<pool_fl_chunked&&>);

    // the same holds for the classifying traits
    D_POOL_CHECK(is_pointer_stable_pool_v<const pool_fl_chunked&>);
    D_POOL_CHECK(!is_pointer_stable_pool_v<const pool_fl_contig&>);
    D_POOL_CHECK(supports_individual_release_v<pool_fl_chunked&>);
    D_POOL_CHECK(!supports_individual_release_v<pool_mono_chunked&>);
    D_POOL_CHECK(has_generational_sweep_v<const pool_gen_chunked&>);
    D_POOL_CHECK(has_memory_accounting_v<const pool_fl_chunked&>);
    D_POOL_CHECK(is_monotonic_pool_v<const pool_mono_chunked&>);

    return true;
}

/*
tests_pointer_stability_classification
  Verifies chunked pools report pointer stability and contiguous pools do
  not, that the verdict follows the block axis alone, and that a type
  carrying no such constant reads false rather than failing.
*/
bool
tests_pointer_stability_classification()
{
    D_POOL_CHECK(is_pointer_stable_pool_v<pool_fl_chunked>);
    D_POOL_CHECK(is_pointer_stable_pool_v<pool_mono_chunked>);
    D_POOL_CHECK(is_pointer_stable_pool_v<pool_gen_chunked>);

    D_POOL_CHECK(!is_pointer_stable_pool_v<pool_fl_contig>);
    D_POOL_CHECK(!is_pointer_stable_pool_v<pool_mono_contig>);
    D_POOL_CHECK(!is_pointer_stable_pool_v<pool_gen_contig>);

    // the trait reads the pool's own constant
    D_POOL_CHECK(is_pointer_stable_pool_v<pool_fl_chunked> ==
               pool_fl_chunked::pointer_stable);
    D_POOL_CHECK(is_pointer_stable_pool_v<pool_fl_contig> ==
               pool_fl_contig::pointer_stable);

    // it reads any type carrying the constant, pool or not
    D_POOL_CHECK(is_pointer_stable_pool_v<pool_duck_stable>);

    // and yields false, not an error, when the constant is missing
    D_POOL_CHECK(!is_pointer_stable_pool_v<pool_not_a_pool>);
    D_POOL_CHECK(!is_pointer_stable_pool_v<int>);
    D_POOL_CHECK(!is_pointer_stable_pool_v<void>);

    return true;
}

/*
tests_individual_release_classification
  Verifies free-list and generational pools report per-slot release
  support while monotonic pools do not - the verdict follows the release
  axis alone.
*/
bool
tests_individual_release_classification()
{
    D_POOL_CHECK(supports_individual_release_v<pool_fl_chunked>);
    D_POOL_CHECK(supports_individual_release_v<pool_fl_contig>);
    D_POOL_CHECK(supports_individual_release_v<pool_gen_chunked>);
    D_POOL_CHECK(supports_individual_release_v<pool_gen_contig>);

    D_POOL_CHECK(!supports_individual_release_v<pool_mono_chunked>);
    D_POOL_CHECK(!supports_individual_release_v<pool_mono_contig>);

    D_POOL_CHECK(supports_individual_release_v<pool_fl_chunked> ==
               pool_fl_chunked::supports_individual_release);

    D_POOL_CHECK(!supports_individual_release_v<pool_not_a_pool>);
    D_POOL_CHECK(!supports_individual_release_v<int>);

    return true;
}

/*
tests_monotonic_pool_classification
  Verifies is_monotonic_pool holds only for types that are pools AND lack
  individual release - so a non-pool with no release support is still not
  a monotonic pool.
*/
bool
tests_monotonic_pool_classification()
{
    D_POOL_CHECK(is_monotonic_pool_v<pool_mono_chunked>);
    D_POOL_CHECK(is_monotonic_pool_v<pool_mono_contig>);
    D_POOL_CHECK(is_monotonic_pool_v<monotonic_pool_resource<int>>);

    D_POOL_CHECK(!is_monotonic_pool_v<pool_fl_chunked>);
    D_POOL_CHECK(!is_monotonic_pool_v<pool_gen_chunked>);

    // the conjunction really is a conjunction: pool_not_a_pool has no
    // individual release either, but it is not a pool
    D_POOL_CHECK(!supports_individual_release_v<pool_not_a_pool>);
    D_POOL_CHECK(!is_monotonic_pool_v<pool_not_a_pool>);
    D_POOL_CHECK(!is_monotonic_pool_v<int>);
    D_POOL_CHECK(!is_monotonic_pool_v<std::vector<int>>);

    // ... while a duck-typed pool with no release constant qualifies
    D_POOL_CHECK(is_pool_resource_v<pool_duck_resource>);
    D_POOL_CHECK(!supports_individual_release_v<pool_duck_resource>);
    D_POOL_CHECK(is_monotonic_pool_v<pool_duck_resource>);

    return true;
}

/*
tests_generational_sweep_classification
  Verifies only generational pools report sweep support, and that the
  verdict tracks the release policy across both block layouts.
*/
bool
tests_generational_sweep_classification()
{
    D_POOL_CHECK(has_generational_sweep_v<pool_gen_chunked>);
    D_POOL_CHECK(has_generational_sweep_v<pool_gen_contig>);

    D_POOL_CHECK(!has_generational_sweep_v<pool_fl_chunked>);
    D_POOL_CHECK(!has_generational_sweep_v<pool_fl_contig>);
    D_POOL_CHECK(!has_generational_sweep_v<pool_mono_chunked>);
    D_POOL_CHECK(!has_generational_sweep_v<pool_mono_contig>);

    D_POOL_CHECK(has_generational_sweep_v<pool_gen_chunked> ==
               pool_gen_chunked::supports_generational_sweep);

    D_POOL_CHECK(!has_generational_sweep_v<pool_not_a_pool>);
    D_POOL_CHECK(!has_generational_sweep_v<void>);

    // sweep support implies individual release support, never the reverse
    D_POOL_CHECK(supports_individual_release_v<pool_gen_chunked>);
    D_POOL_CHECK(!has_generational_sweep_v<pool_fl_chunked>);
    D_POOL_CHECK(supports_individual_release_v<pool_fl_chunked>);

    return true;
}

/*
tests_memory_accounting_classification
  Verifies has_memory_accounting requires the whole triple - a type
  offering only part of it does not qualify.
*/
bool
tests_memory_accounting_classification()
{
    D_POOL_CHECK(has_memory_accounting_v<pool_fl_chunked>);
    D_POOL_CHECK(has_memory_accounting_v<pool_fl_contig>);
    D_POOL_CHECK(has_memory_accounting_v<pool_mono_chunked>);
    D_POOL_CHECK(has_memory_accounting_v<pool_gen_contig>);
    D_POOL_CHECK(has_memory_accounting_v<default_pool_resource<int>>);

    D_POOL_CHECK(!has_memory_accounting_v<pool_duck_resource>);
    D_POOL_CHECK(!has_memory_accounting_v<pool_not_a_pool>);
    D_POOL_CHECK(!has_memory_accounting_v<std::vector<int>>);
    D_POOL_CHECK(!has_memory_accounting_v<int>);

    // exactly two of three is not enough
    struct partial_accounting
    {
        std::size_t bytes_allocated() const noexcept { return 0; }
        std::size_t bytes_in_use()    const noexcept { return 0; }
    };

    D_POOL_CHECK(has_bytes_allocated_v<partial_accounting>);
    D_POOL_CHECK(has_bytes_in_use_v<partial_accounting>);
    D_POOL_CHECK(!has_utilization_v<partial_accounting>);
    D_POOL_CHECK(!has_memory_accounting_v<partial_accounting>);

    return true;
}

/*
tests_pool_allocator_classification
  Verifies is_pool_allocator fires on the combination of value_type and
  resource(), and not on allocators that merely have a value_type.
*/
bool
tests_pool_allocator_classification()
{
    D_POOL_CHECK(is_pool_allocator_v<pool_duck_allocator>);
    D_POOL_CHECK(is_pool_allocator_v<const pool_duck_allocator&>);

    // a standard allocator has value_type but no resource()
    D_POOL_CHECK(has_value_type_v<std::allocator<int>>);
    D_POOL_CHECK(!has_resource_method_v<std::allocator<int>>);
    D_POOL_CHECK(!is_pool_allocator_v<std::allocator<int>>);

    // both halves are required
    struct value_type_only
    {
        using value_type = int;
    };

    struct resource_only
    {
        int* resource() const noexcept { return nullptr; }
    };

    D_POOL_CHECK(!is_pool_allocator_v<value_type_only>);
    D_POOL_CHECK(!is_pool_allocator_v<resource_only>);
    D_POOL_CHECK(!is_pool_allocator_v<pool_not_a_pool>);

    // a pool is not an allocator: it has no resource()
    D_POOL_CHECK(!is_pool_allocator_v<pool_fl_chunked>);

    return true;
}

/*
tests_pool_backed_container_classification
  Verifies a container counts as pool-backed exactly when its
  allocator_type is itself classified as a pool allocator.
*/
bool
tests_pool_backed_container_classification()
{
    D_POOL_CHECK(is_pool_backed_container_v<pool_duck_container>);
    D_POOL_CHECK(is_pool_backed_container_v<const pool_duck_container&>);

    D_POOL_CHECK(!is_pool_backed_container_v<pool_duck_plain_container>);
    D_POOL_CHECK(!is_pool_backed_container_v<std::vector<int>>);
    D_POOL_CHECK(!is_pool_backed_container_v<std::string>);
    D_POOL_CHECK(!is_pool_backed_container_v<pool_not_a_pool>);
    D_POOL_CHECK(!is_pool_backed_container_v<int>);

    // a pool itself has no allocator_type, so it is not pool-backed
    D_POOL_CHECK(!has_allocator_type_v<pool_fl_chunked>);
    D_POOL_CHECK(!is_pool_backed_container_v<pool_fl_chunked>);

    // the predicate must answer, not error, for types with no allocator
    // at all - including scalars and void
    D_POOL_CHECK(!is_pool_backed_container_v<void>);
    D_POOL_CHECK(!is_pool_backed_container_v<double>);
    D_POOL_CHECK(!is_pool_backed_container_v<pool_release>);

    return true;
}

/*
tests_pool_resource_type_extraction
  Verifies pool_resource_type_t names the pointee of resource() and is
  void when the type exposes no such accessor - the SFINAE-safe fallback.
*/
bool
tests_pool_resource_type_extraction()
{
    D_POOL_CHECK((std::is_same<pool_resource_type_t<pool_duck_allocator>,
                             pool_fl_chunked>::value));

    // the pointer is stripped, the pointee kept
    D_POOL_CHECK(
        !std::is_pointer<pool_resource_type_t<pool_duck_allocator>>::value);

    // cv and reference qualifiers on the allocator are irrelevant
    D_POOL_CHECK((std::is_same<
                    pool_resource_type_t<const pool_duck_allocator&>,
                    pool_fl_chunked>::value));

    // no resource() means void
    D_POOL_CHECK(std::is_void<
                   pool_resource_type_t<std::allocator<int>>>::value);
    D_POOL_CHECK(std::is_void<pool_resource_type_t<pool_not_a_pool>>::value);
    D_POOL_CHECK(std::is_void<pool_resource_type_t<int>>::value);
    D_POOL_CHECK(std::is_void<pool_resource_type_t<pool_fl_chunked>>::value);

    // the struct form and the alias agree
    D_POOL_CHECK((std::is_same<
                    pool_resource_type<pool_duck_allocator>::type,
                    pool_resource_type_t<pool_duck_allocator>>::value));

    return true;
}

/*
tests_pool_class_aggregate
  Verifies pool_class rolls every individual pool predicate up
  consistently, for a representative pool of each release policy and both
  block layouts, and degrades cleanly on a non-pool.
*/
bool
tests_pool_class_aggregate()
{
    // free-list, chunked
    {
        using classified = pool_class<pool_fl_chunked>;

        D_POOL_CHECK(classified::is_pool);
        D_POOL_CHECK(classified::pointer_stable);
        D_POOL_CHECK(classified::individual_release);
        D_POOL_CHECK(!classified::monotonic);
        D_POOL_CHECK(!classified::generational);
        D_POOL_CHECK(classified::has_reset);
        D_POOL_CHECK(classified::has_reserve);
        D_POOL_CHECK(classified::has_accounting);
        D_POOL_CHECK(std::is_void<classified::resource_type>::value);
    }

    // monotonic, contiguous
    {
        using classified = pool_class<pool_mono_contig>;

        D_POOL_CHECK(classified::is_pool);
        D_POOL_CHECK(!classified::pointer_stable);
        D_POOL_CHECK(!classified::individual_release);
        D_POOL_CHECK(classified::monotonic);
        D_POOL_CHECK(!classified::generational);
        D_POOL_CHECK(classified::has_accounting);
    }

    // generational, chunked
    {
        using classified = pool_class<pool_gen_chunked>;

        D_POOL_CHECK(classified::is_pool);
        D_POOL_CHECK(classified::pointer_stable);
        D_POOL_CHECK(classified::individual_release);
        D_POOL_CHECK(!classified::monotonic);
        D_POOL_CHECK(classified::generational);
    }

    // every member agrees with the standalone trait it aggregates
    {
        using classified = pool_class<pool_gen_contig>;

        D_POOL_CHECK(classified::is_pool ==
                   is_pool_resource_v<pool_gen_contig>);
        D_POOL_CHECK(classified::pointer_stable ==
                   is_pointer_stable_pool_v<pool_gen_contig>);
        D_POOL_CHECK(classified::individual_release ==
                   supports_individual_release_v<pool_gen_contig>);
        D_POOL_CHECK(classified::monotonic ==
                   is_monotonic_pool_v<pool_gen_contig>);
        D_POOL_CHECK(classified::generational ==
                   has_generational_sweep_v<pool_gen_contig>);
        D_POOL_CHECK(classified::has_reset ==
                   has_pool_reset_v<pool_gen_contig>);
        D_POOL_CHECK(classified::has_reserve ==
                   has_pool_reserve_v<pool_gen_contig>);
        D_POOL_CHECK(classified::has_accounting ==
                   has_memory_accounting_v<pool_gen_contig>);
    }

    // a non-pool classifies as nothing at all
    {
        using classified = pool_class<pool_not_a_pool>;

        D_POOL_CHECK(!classified::is_pool);
        D_POOL_CHECK(!classified::pointer_stable);
        D_POOL_CHECK(!classified::individual_release);
        D_POOL_CHECK(!classified::monotonic);
        D_POOL_CHECK(!classified::generational);
        D_POOL_CHECK(!classified::has_reset);
        D_POOL_CHECK(!classified::has_reserve);
        D_POOL_CHECK(!classified::has_accounting);
    }

    return true;
}

/*
tests_pool_allocator_class_aggregate
  Verifies pool_allocator_class classifies an allocator through the
  properties of its extracted resource type, and stays false throughout
  for a type that is not an allocator at all.
*/
bool
tests_pool_allocator_class_aggregate()
{
    {
        using classified = pool_allocator_class<pool_duck_allocator>;

        D_POOL_CHECK(classified::is_pool_alloc);
        D_POOL_CHECK((std::is_same<classified::resource_type,
                                 pool_fl_chunked>::value));

        // the resource is a free-list chunked pool, so:
        D_POOL_CHECK(classified::pointer_stable);
        D_POOL_CHECK(classified::individual_release);
        D_POOL_CHECK(!classified::monotonic);
        D_POOL_CHECK(!classified::generational);

        // and every member matches the trait applied to the resource
        D_POOL_CHECK(classified::pointer_stable ==
                   is_pointer_stable_pool_v<pool_fl_chunked>);
        D_POOL_CHECK(classified::individual_release ==
                   supports_individual_release_v<pool_fl_chunked>);
    }

    // a non-allocator extracts void and classifies as nothing
    {
        using classified = pool_allocator_class<std::allocator<int>>;

        D_POOL_CHECK(!classified::is_pool_alloc);
        D_POOL_CHECK(std::is_void<classified::resource_type>::value);
        D_POOL_CHECK(!classified::pointer_stable);
        D_POOL_CHECK(!classified::individual_release);
        D_POOL_CHECK(!classified::monotonic);
        D_POOL_CHECK(!classified::generational);
    }

    {
        using classified = pool_allocator_class<pool_not_a_pool>;

        D_POOL_CHECK(!classified::is_pool_alloc);
        D_POOL_CHECK(std::is_void<classified::resource_type>::value);
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
