/*******************************************************************************
* djinterp [test]                                        pool_tests_policies.cpp
*
*   Sections III and IV of the pool suite - the three release policies and
* the two block policies.
*
*   Each policy is a stateless struct whose whole contract is a handful of
* compile-time constants.  The checks pin those constants, confirm the
* structs stay empty and trivially constructible (so composing them into a
* pool costs nothing), and confirm they remain distinct types.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_policies.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "pool_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_monotonic_release_policy_constants
  Verifies monotonic_release_policy reports the monotonic strategy and
  refuses both individual release and generational sweep.
*/
bool
tests_monotonic_release_policy_constants()
{
    static_assert(monotonic_release_policy::strategy ==
                      pool_release::monotonic,
                  "monotonic strategy constant is wrong.");
    static_assert(!monotonic_release_policy::supports_individual_release,
                  "monotonic must not support individual release.");
    static_assert(!monotonic_release_policy::supports_generational_sweep,
                  "monotonic must not support generational sweep.");

    D_POOL_CHECK(monotonic_release_policy::strategy ==
               pool_release::monotonic);
    D_POOL_CHECK(monotonic_release_policy::supports_individual_release ==
               false);
    D_POOL_CHECK(monotonic_release_policy::supports_generational_sweep ==
               false);

    return true;
}

/*
tests_free_list_release_policy_constants
  Verifies free_list_release_policy reports the free_list strategy,
  supports individual release, and refuses generational sweep.
*/
bool
tests_free_list_release_policy_constants()
{
    static_assert(free_list_release_policy::strategy ==
                      pool_release::free_list,
                  "free_list strategy constant is wrong.");
    static_assert(free_list_release_policy::supports_individual_release,
                  "free_list must support individual release.");
    static_assert(!free_list_release_policy::supports_generational_sweep,
                  "free_list must not support generational sweep.");

    D_POOL_CHECK(free_list_release_policy::strategy ==
               pool_release::free_list);
    D_POOL_CHECK(free_list_release_policy::supports_individual_release ==
               true);
    D_POOL_CHECK(free_list_release_policy::supports_generational_sweep ==
               false);

    return true;
}

/*
tests_generational_release_policy_constants
  Verifies generational_release_policy reports the generational strategy,
  supports both release modes, and publishes an unsigned integral
  generation_type wide enough (at least 32 bits) to serve as a sweep
  counter.
*/
bool
tests_generational_release_policy_constants()
{
    using gen_type = generational_release_policy::generation_type;

    static_assert(generational_release_policy::strategy ==
                      pool_release::generational,
                  "generational strategy constant is wrong.");
    static_assert(
        generational_release_policy::supports_individual_release,
        "generational must support individual release.");
    static_assert(
        generational_release_policy::supports_generational_sweep,
        "generational must support generational sweep.");
    static_assert(std::is_integral<gen_type>::value,
                  "generation_type must be integral.");
    static_assert(std::is_unsigned<gen_type>::value,
                  "generation_type must be unsigned.");

    D_POOL_CHECK(generational_release_policy::strategy ==
               pool_release::generational);
    D_POOL_CHECK(
        generational_release_policy::supports_individual_release == true);
    D_POOL_CHECK(
        generational_release_policy::supports_generational_sweep == true);

    D_POOL_CHECK(std::is_integral<gen_type>::value);
    D_POOL_CHECK(std::is_unsigned<gen_type>::value);
    D_POOL_CHECK(sizeof(gen_type) >= sizeof(std::uint32_t));

    // an unsigned counter of that width must survive its own max value
    constexpr gen_type high = static_cast<gen_type>(0xFFFFFFFFu);
    D_POOL_CHECK(high > static_cast<gen_type>(0xFFFFu));

    return true;
}

/*
tests_release_policies_are_empty_and_distinct
  Verifies all three release policies are empty, trivially
  default-constructible, and pairwise distinct types - the properties that
  let pool_resource carry them at zero cost.
*/
bool
tests_release_policies_are_empty_and_distinct()
{
    D_POOL_CHECK(std::is_empty<monotonic_release_policy>::value);
    D_POOL_CHECK(std::is_empty<free_list_release_policy>::value);
    D_POOL_CHECK(std::is_empty<generational_release_policy>::value);

    D_POOL_CHECK(std::is_trivially_default_constructible<
                   monotonic_release_policy>::value);
    D_POOL_CHECK(std::is_trivially_default_constructible<
                   free_list_release_policy>::value);
    D_POOL_CHECK(std::is_trivially_default_constructible<
                   generational_release_policy>::value);

    D_POOL_CHECK(!std::is_same<monotonic_release_policy,
                             free_list_release_policy>::value);
    D_POOL_CHECK(!std::is_same<monotonic_release_policy,
                             generational_release_policy>::value);
    D_POOL_CHECK(!std::is_same<free_list_release_policy,
                             generational_release_policy>::value);

    // each really is instantiable
    monotonic_release_policy    mono;
    free_list_release_policy    flist;
    generational_release_policy gen;
    (void)mono;
    (void)flist;
    (void)gen;

    return true;
}

/*
tests_contiguous_block_policy_constants
  Verifies contiguous_block_policy reports the contiguous layout and
  advertises no pointer-stability guarantee across growth.
*/
bool
tests_contiguous_block_policy_constants()
{
    static_assert(contiguous_block_policy::layout ==
                      pool_block_layout::contiguous,
                  "contiguous layout constant is wrong.");
    static_assert(!contiguous_block_policy::pointer_stable,
                  "contiguous must not promise pointer stability.");

    D_POOL_CHECK(contiguous_block_policy::layout ==
               pool_block_layout::contiguous);
    D_POOL_CHECK(contiguous_block_policy::pointer_stable == false);

    return true;
}

/*
tests_chunked_block_policy_defaults
  Verifies chunked_block_policy<> reports the chunked layout, promises
  pointer stability, and defaults to 256 slots per block.
*/
bool
tests_chunked_block_policy_defaults()
{
    static_assert(chunked_block_policy<>::layout ==
                      pool_block_layout::chunked,
                  "chunked layout constant is wrong.");
    static_assert(chunked_block_policy<>::pointer_stable,
                  "chunked must promise pointer stability.");
    static_assert(chunked_block_policy<>::slots_per_block == 256,
                  "chunked default block size must be 256.");

    D_POOL_CHECK(chunked_block_policy<>::layout ==
               pool_block_layout::chunked);
    D_POOL_CHECK(chunked_block_policy<>::pointer_stable == true);
    D_POOL_CHECK(chunked_block_policy<>::slots_per_block == 256);

    // the bare form and the explicit default form are one type
    D_POOL_CHECK((std::is_same<chunked_block_policy<>,
                             chunked_block_policy<256>>::value));

    return true;
}

/*
tests_chunked_block_policy_parameterisation
  Verifies the _SlotsPerBlock argument propagates to slots_per_block for
  a spread of values, that each instantiation still reports the chunked
  layout and pointer stability, and that differing counts yield distinct
  types.
*/
bool
tests_chunked_block_policy_parameterisation()
{
    static_assert(chunked_block_policy<1>::slots_per_block == 1, "N=1");
    static_assert(chunked_block_policy<2>::slots_per_block == 2, "N=2");
    static_assert(chunked_block_policy<64>::slots_per_block == 64, "N=64");
    static_assert(chunked_block_policy<4096>::slots_per_block == 4096,
                  "N=4096");

    D_POOL_CHECK(chunked_block_policy<1>::slots_per_block == 1);
    D_POOL_CHECK(chunked_block_policy<2>::slots_per_block == 2);
    D_POOL_CHECK(chunked_block_policy<8>::slots_per_block == 8);
    D_POOL_CHECK(chunked_block_policy<128>::slots_per_block == 128);
    D_POOL_CHECK(chunked_block_policy<4096>::slots_per_block == 4096);

    // the constants hold at both ends of the range
    D_POOL_CHECK(chunked_block_policy<1>::layout ==
               pool_block_layout::chunked);
    D_POOL_CHECK(chunked_block_policy<1>::pointer_stable == true);
    D_POOL_CHECK(chunked_block_policy<4096>::layout ==
               pool_block_layout::chunked);
    D_POOL_CHECK(chunked_block_policy<4096>::pointer_stable == true);

    // distinct counts are distinct types
    D_POOL_CHECK((!std::is_same<chunked_block_policy<1>,
                              chunked_block_policy<2>>::value));
    D_POOL_CHECK((!std::is_same<chunked_block_policy<64>,
                              chunked_block_policy<256>>::value));

    return true;
}

/*
tests_block_policies_are_empty_and_distinct
  Verifies both block policies are empty, trivially default-constructible,
  and distinct from one another.
*/
bool
tests_block_policies_are_empty_and_distinct()
{
    D_POOL_CHECK(std::is_empty<contiguous_block_policy>::value);
    D_POOL_CHECK(std::is_empty<chunked_block_policy<>>::value);
    D_POOL_CHECK(std::is_empty<chunked_block_policy<4>>::value);

    D_POOL_CHECK(std::is_trivially_default_constructible<
                   contiguous_block_policy>::value);
    D_POOL_CHECK(std::is_trivially_default_constructible<
                   chunked_block_policy<>>::value);

    D_POOL_CHECK((!std::is_same<contiguous_block_policy,
                              chunked_block_policy<>>::value));

    contiguous_block_policy contiguous;
    chunked_block_policy<>  chunked;
    (void)contiguous;
    (void)chunked;

    return true;
}


NS_END  // testing
NS_END  // djinterp
