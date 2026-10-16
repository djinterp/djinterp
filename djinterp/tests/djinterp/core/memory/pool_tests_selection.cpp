/******************************************************************************
* djinterp [test]                                      pool_tests_selection.cpp
*
*   Sections VII and VIII of the pool suite - policy selection and the
* default aliases.
*
*   select_release_policy_t and select_block_policy_t let a caller name a
* policy by enumerator instead of by type; the three convenience aliases
* pin a common policy triple onto pool_resource.  Both are pure type
* identities, so the checks are static_asserts mirrored at runtime, plus a
* live instantiation proving each selection is a usable policy argument
* rather than merely a name.
*
*
* path:      /tests/djinterp/core/memory/pool_tests_selection.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

// djinterp
#include "pool_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    using element = pool_medium_t;

    // a pool assembled entirely from selector-resolved policies
    using selected_pool =
        pool_resource<
            element,
            select_release_policy_t<pool_release::free_list>,
            select_block_policy_t<pool_block_layout::chunked>,
            pool_growth_exact>;

    // hand-spelled expansions the aliases must match exactly
    using expected_default =
        pool_resource<element,
                      free_list_release_policy,
                      chunked_block_policy<>,
                      exponential_growth_policy>;

    using expected_monotonic =
        pool_resource<element,
                      monotonic_release_policy,
                      chunked_block_policy<>,
                      exponential_growth_policy>;

    using expected_flat =
        pool_resource<element,
                      free_list_release_policy,
                      contiguous_block_policy,
                      exponential_growth_policy>;
}


/*
tests_select_release_policy_mapping
  Verifies each pool_release enumerator maps to its release policy type
  and that the three results are mutually distinct.
*/
bool
tests_select_release_policy_mapping()
{
    static_assert(
        std::is_same<select_release_policy_t<pool_release::monotonic>,
                     monotonic_release_policy>::value,
        "monotonic selection is wrong.");
    static_assert(
        std::is_same<select_release_policy_t<pool_release::free_list>,
                     free_list_release_policy>::value,
        "free_list selection is wrong.");
    static_assert(
        std::is_same<select_release_policy_t<pool_release::generational>,
                     generational_release_policy>::value,
        "generational selection is wrong.");

    D_PR_CHECK((std::is_same<
                    select_release_policy_t<pool_release::monotonic>,
                    monotonic_release_policy>::value));
    D_PR_CHECK((std::is_same<
                    select_release_policy_t<pool_release::free_list>,
                    free_list_release_policy>::value));
    D_PR_CHECK((std::is_same<
                    select_release_policy_t<pool_release::generational>,
                    generational_release_policy>::value));

    D_PR_CHECK((!std::is_same<
                     select_release_policy_t<pool_release::monotonic>,
                     select_release_policy_t<pool_release::free_list>
                 >::value));
    D_PR_CHECK((!std::is_same<
                     select_release_policy_t<pool_release::free_list>,
                     select_release_policy_t<pool_release::generational>
                 >::value));
    D_PR_CHECK((!std::is_same<
                     select_release_policy_t<pool_release::monotonic>,
                     select_release_policy_t<pool_release::generational>
                 >::value));

    // the round trip closes: the selected policy reports the enumerator
    // it was selected by
    D_PR_CHECK(select_release_policy_t<pool_release::monotonic>::strategy
               == pool_release::monotonic);
    D_PR_CHECK(select_release_policy_t<pool_release::free_list>::strategy
               == pool_release::free_list);
    D_PR_CHECK(
        select_release_policy_t<pool_release::generational>::strategy
        == pool_release::generational);

    return true;
}

/*
tests_select_block_policy_mapping
  Verifies each pool_block_layout enumerator maps to its block policy,
  that the chunked selection carries the default 256-slot block, and that
  the two results are distinct.
*/
bool
tests_select_block_policy_mapping()
{
    static_assert(
        std::is_same<select_block_policy_t<pool_block_layout::contiguous>,
                     contiguous_block_policy>::value,
        "contiguous selection is wrong.");
    static_assert(
        std::is_same<select_block_policy_t<pool_block_layout::chunked>,
                     chunked_block_policy<>>::value,
        "chunked selection is wrong.");

    D_PR_CHECK((std::is_same<
                    select_block_policy_t<pool_block_layout::contiguous>,
                    contiguous_block_policy>::value));
    D_PR_CHECK((std::is_same<
                    select_block_policy_t<pool_block_layout::chunked>,
                    chunked_block_policy<>>::value));

    D_PR_CHECK(select_block_policy_t<pool_block_layout::chunked>
                   ::slots_per_block == 256);

    D_PR_CHECK((!std::is_same<
                     select_block_policy_t<pool_block_layout::contiguous>,
                     select_block_policy_t<pool_block_layout::chunked>
                 >::value));

    // the round trip closes here too
    D_PR_CHECK(select_block_policy_t<pool_block_layout::contiguous>::layout
               == pool_block_layout::contiguous);
    D_PR_CHECK(select_block_policy_t<pool_block_layout::chunked>::layout
               == pool_block_layout::chunked);

    return true;
}

/*
tests_selected_policies_compose_into_a_pool
  Verifies selector-resolved policies are usable as real policy arguments:
  the resulting pool exposes them unchanged through its own aliases and
  operates normally.
*/
bool
tests_selected_policies_compose_into_a_pool()
{
    D_PR_CHECK((std::is_same<selected_pool::release_policy,
                             free_list_release_policy>::value));
    D_PR_CHECK((std::is_same<selected_pool::block_policy,
                             chunked_block_policy<>>::value));
    D_PR_CHECK((std::is_same<selected_pool::value_type, element>::value));

    D_PR_CHECK(selected_pool::release_strategy == pool_release::free_list);
    D_PR_CHECK(selected_pool::block_layout == pool_block_layout::chunked);
    D_PR_CHECK(selected_pool::pointer_stable == true);
    D_PR_CHECK(selected_pool::supports_individual_release == true);
    D_PR_CHECK(selected_pool::supports_generational_sweep == false);

    // and it really works
    selected_pool pool(4);

    void* slot = pool.acquire();
    D_PR_CHECK(slot != nullptr);
    D_PR_CHECK(pool.size() == 1u);

    pool.release(slot);
    D_PR_CHECK(pool.empty());
    D_PR_CHECK(pool.acquire() == slot);

    return true;
}

/*
tests_default_pool_resource_alias
  Verifies default_pool_resource expands to exactly free-list release,
  chunked blocks, and exponential growth.
*/
bool
tests_default_pool_resource_alias()
{
    static_assert(std::is_same<default_pool_resource<element>,
                               expected_default>::value,
                  "default_pool_resource expansion changed.");

    D_PR_CHECK((std::is_same<default_pool_resource<element>,
                             expected_default>::value));

    D_PR_CHECK((std::is_same<default_pool_resource<element>::release_policy,
                             free_list_release_policy>::value));
    D_PR_CHECK((std::is_same<default_pool_resource<element>::block_policy,
                             chunked_block_policy<>>::value));
    D_PR_CHECK((std::is_same<default_pool_resource<element>::growth_policy,
                             exponential_growth_policy>::value));

    // the element type flows through unchanged
    D_PR_CHECK((std::is_same<default_pool_resource<element>::value_type,
                             element>::value));
    D_PR_CHECK((std::is_same<default_pool_resource<int>::value_type,
                             int>::value));

    // it is the pointer-stable, individually releasing default
    D_PR_CHECK(default_pool_resource<element>::pointer_stable == true);
    D_PR_CHECK(default_pool_resource<element>
                   ::supports_individual_release == true);

    return true;
}

/*
tests_monotonic_pool_resource_alias
  Verifies monotonic_pool_resource expands to monotonic release, chunked
  blocks, and exponential growth.
*/
bool
tests_monotonic_pool_resource_alias()
{
    static_assert(std::is_same<monotonic_pool_resource<element>,
                               expected_monotonic>::value,
                  "monotonic_pool_resource expansion changed.");

    D_PR_CHECK((std::is_same<monotonic_pool_resource<element>,
                             expected_monotonic>::value));

    D_PR_CHECK((std::is_same<
                    monotonic_pool_resource<element>::release_policy,
                    monotonic_release_policy>::value));
    D_PR_CHECK((std::is_same<
                    monotonic_pool_resource<element>::block_policy,
                    chunked_block_policy<>>::value));
    D_PR_CHECK((std::is_same<
                    monotonic_pool_resource<element>::growth_policy,
                    exponential_growth_policy>::value));

    D_PR_CHECK(monotonic_pool_resource<element>
                   ::supports_individual_release == false);
    D_PR_CHECK(monotonic_pool_resource<element>::pointer_stable == true);

    return true;
}

/*
tests_flat_pool_resource_alias
  Verifies flat_pool_resource expands to free-list release, a contiguous
  block, and exponential growth - the layout that trades pointer stability
  for linear slots.
*/
bool
tests_flat_pool_resource_alias()
{
    static_assert(std::is_same<flat_pool_resource<element>,
                               expected_flat>::value,
                  "flat_pool_resource expansion changed.");

    D_PR_CHECK((std::is_same<flat_pool_resource<element>,
                             expected_flat>::value));

    D_PR_CHECK((std::is_same<flat_pool_resource<element>::release_policy,
                             free_list_release_policy>::value));
    D_PR_CHECK((std::is_same<flat_pool_resource<element>::block_policy,
                             contiguous_block_policy>::value));
    D_PR_CHECK((std::is_same<flat_pool_resource<element>::growth_policy,
                             exponential_growth_policy>::value));

    D_PR_CHECK(flat_pool_resource<element>::pointer_stable == false);
    D_PR_CHECK(flat_pool_resource<element>::block_layout ==
               pool_block_layout::contiguous);

    return true;
}

/*
tests_default_aliases_are_distinct_and_operational
  Verifies the three aliases are mutually distinct types, share the growth
  axis, and each instantiates into a working pool.
*/
bool
tests_default_aliases_are_distinct_and_operational()
{
    // distinct along exactly one axis each
    D_PR_CHECK((!std::is_same<default_pool_resource<element>,
                              monotonic_pool_resource<element>>::value));
    D_PR_CHECK((!std::is_same<default_pool_resource<element>,
                              flat_pool_resource<element>>::value));
    D_PR_CHECK((!std::is_same<monotonic_pool_resource<element>,
                              flat_pool_resource<element>>::value));

    // all three share the growth policy
    D_PR_CHECK((std::is_same<
                    default_pool_resource<element>::growth_policy,
                    monotonic_pool_resource<element>::growth_policy
                >::value));
    D_PR_CHECK((std::is_same<
                    monotonic_pool_resource<element>::growth_policy,
                    flat_pool_resource<element>::growth_policy>::value));

    // each is operational
    {
        default_pool_resource<element> pool;

        void* slot = pool.acquire();
        D_PR_CHECK(slot != nullptr);
        D_PR_CHECK(pool.size() == 1u);

        pool.release(slot);
        D_PR_CHECK(pool.empty());
        D_PR_CHECK(pool.acquire() == slot);
    }
    {
        monotonic_pool_resource<element> pool;

        void* slot = pool.acquire();
        D_PR_CHECK(slot != nullptr);

        pool.release(slot);
        D_PR_CHECK(pool.empty());

        // monotonic does not recycle, so the next slot is a new one
        D_PR_CHECK(pool.acquire() != slot);
    }
    {
        flat_pool_resource<element> pool;

        void* slot = pool.acquire();
        D_PR_CHECK(slot != nullptr);

        pool.release(slot);
        D_PR_CHECK(pool.empty());
        D_PR_CHECK(pool.acquire() == slot);
    }

    return true;
}

/*
tests_non_generational_policies_instantiate
  Regression pin.  The generation counter's type must be resolved lazily,
  so that a release policy without a generation_type member still yields a
  usable pool.  Naming both arms of the choice eagerly makes every
  free-list and monotonic pool - and therefore every default alias -
  fail to compile.
*/
bool
tests_non_generational_policies_instantiate()
{
    // the two release policies that publish no generation_type
    D_PR_CHECK(free_list_release_policy::supports_generational_sweep ==
               false);
    D_PR_CHECK(monotonic_release_policy::supports_generational_sweep ==
               false);

    // each must still compose into a live, working pool
    {
        pool_resource<element,
                      free_list_release_policy,
                      chunked_block_policy<4>,
                      pool_growth_exact> pool(2);

        D_PR_CHECK(pool.acquire() != nullptr);
    }
    {
        pool_resource<element,
                      monotonic_release_policy,
                      contiguous_block_policy,
                      pool_growth_exact> pool(2);

        D_PR_CHECK(pool.acquire() != nullptr);
    }

    // and so must all three shipped aliases
    {
        default_pool_resource<element>   defaulted;
        monotonic_pool_resource<element> monotonic;
        flat_pool_resource<element>      flat;

        D_PR_CHECK(defaulted.acquire() != nullptr);
        D_PR_CHECK(monotonic.acquire() != nullptr);
        D_PR_CHECK(flat.acquire()      != nullptr);
    }

    // the generational policy keeps its counter regardless
    {
        pool_gen_chunked pool;
        D_PR_CHECK(pool.current_generation() == 0u);
    }

    // a non-generational pool must not have grown a generation API
    D_PR_CHECK(!has_current_generation_v<default_pool_resource<element>>);
    D_PR_CHECK(!has_current_generation_v<monotonic_pool_resource<element>>);
    D_PR_CHECK(!has_current_generation_v<flat_pool_resource<element>>);

    return true;
}


NS_END  // testing
NS_END  // djinterp
