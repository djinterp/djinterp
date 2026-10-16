/*******************************************************************************
* djinterp [test]                                pool_tests_policy_selection.cpp
*
*   Section VII of the pool unit-test suite.
*
*   Covers the compile-time enum-to-type mapping metafunctions
* that let callers name a policy by its enumerator rather than
* its type:
*
*     - select_release_policy_t<pool_release> maps each
*       pool_release enumerator to its release-policy type:
*         monotonic    -> monotonic_release_policy
*         free_list    -> free_list_release_policy
*         generational -> generational_release_policy
*     - select_block_policy_t<pool_block_layout> maps each
*       pool_block_layout enumerator to its block-policy type:
*         contiguous -> contiguous_block_policy
*         chunked    -> chunked_block_policy<>  (default slots)
*
*   Every mapping is a pure type-identity fact, so the suite's
* primary tier is a wall of static_asserts; the runtime driver
* mirrors each one through std::is_same so the mappings appear
* in the framework report.  The selectors are also driven into
* a live pool_resource instantiation to prove the selected
* types are usable as real policy arguments, not merely names.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_policy_selection.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.27
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "./pool_tests.hpp"


NS_DJINTERP
NS_TEST


namespace
{
    // A pool built entirely from selector-resolved policies,
    // proving the selected types are usable policy arguments.
    using selected_pool =
        pool_resource<
            pool_test_medium_t,
            select_release_policy_t<pool_release::free_list>,
            select_block_policy_t<pool_block_layout::chunked>,
            pool_test_exact_growth_policy>;
}


// =========================================================================
//  Compile-time invariants
// =========================================================================

// --- release policy selection ---

static_assert(
    std::is_same<
        select_release_policy_t<pool_release::monotonic>,
        monotonic_release_policy>::value,
    "select_release_policy_t<monotonic> must be "
    "monotonic_release_policy.");

static_assert(
    std::is_same<
        select_release_policy_t<pool_release::free_list>,
        free_list_release_policy>::value,
    "select_release_policy_t<free_list> must be "
    "free_list_release_policy.");

static_assert(
    std::is_same<
        select_release_policy_t<pool_release::generational>,
        generational_release_policy>::value,
    "select_release_policy_t<generational> must be "
    "generational_release_policy.");

// the three selected release policies are mutually distinct
static_assert(
    !std::is_same<
        select_release_policy_t<pool_release::monotonic>,
        select_release_policy_t<pool_release::free_list>>::value,
    "monotonic and free_list selections must differ.");

static_assert(
    !std::is_same<
        select_release_policy_t<pool_release::free_list>,
        select_release_policy_t<pool_release::generational>>::value,
    "free_list and generational selections must differ.");

// --- block policy selection ---

static_assert(
    std::is_same<
        select_block_policy_t<pool_block_layout::contiguous>,
        contiguous_block_policy>::value,
    "select_block_policy_t<contiguous> must be "
    "contiguous_block_policy.");

static_assert(
    std::is_same<
        select_block_policy_t<pool_block_layout::chunked>,
        chunked_block_policy<>>::value,
    "select_block_policy_t<chunked> must be the "
    "default-parameterised chunked_block_policy<>.");

// the chunked selection defaults to the module default slot
// count (256), matching the bare chunked_block_policy<>
static_assert(
    select_block_policy_t<pool_block_layout::chunked>
        ::slots_per_block == 256,
    "select_block_policy_t<chunked> must carry the default "
    "256 slots per block.");

// the two selected block policies are distinct
static_assert(
    !std::is_same<
        select_block_policy_t<pool_block_layout::contiguous>,
        select_block_policy_t<pool_block_layout::chunked>>::value,
    "contiguous and chunked block selections must differ.");

// the selected policies actually compose into a usable pool,
// and that pool's exposed policy aliases round-trip back to
// the selected types
static_assert(
    std::is_same<
        selected_pool::release_policy,
        free_list_release_policy>::value,
    "a pool built from select_release_policy_t<free_list> must "
    "expose free_list_release_policy.");

static_assert(
    std::is_same<
        selected_pool::block_policy,
        chunked_block_policy<>>::value,
    "a pool built from select_block_policy_t<chunked> must "
    "expose chunked_block_policy<>.");


// =========================================================================
//  Runtime tests
// =========================================================================

/*
pool_tests_policy_selection_all
  brief description
  Tests the following:
  - select_release_policy_t maps each pool_release enumerator to
    the correct release-policy type
  - the three release-policy selections are mutually distinct
  - select_block_policy_t maps each pool_block_layout enumerator
    to the correct block-policy type
  - the chunked selection defaults to 256 slots per block
  - the two block-policy selections are distinct
  - selector-resolved policies compose into a working
    pool_resource whose policy aliases round-trip
*/
void
pool_tests_policy_selection_all(
    test_handler& _handler
)
{
    // -- release policy selection --
    record_assertion(
        _handler,
        ( std::is_same<
              select_release_policy_t<pool_release::monotonic>,
              monotonic_release_policy>::value ),
        "select_release_policy_t<monotonic> == "
        "monotonic_release_policy");

    record_assertion(
        _handler,
        ( std::is_same<
              select_release_policy_t<pool_release::free_list>,
              free_list_release_policy>::value ),
        "select_release_policy_t<free_list> == "
        "free_list_release_policy");

    record_assertion(
        _handler,
        ( std::is_same<
              select_release_policy_t<pool_release::generational>,
              generational_release_policy>::value ),
        "select_release_policy_t<generational> == "
        "generational_release_policy");

    record_assertion(
        _handler,
        ( !std::is_same<
               select_release_policy_t<pool_release::monotonic>,
               select_release_policy_t<pool_release::free_list>
           >::value &&
          !std::is_same<
               select_release_policy_t<pool_release::free_list>,
               select_release_policy_t<pool_release::generational>
           >::value ),
        "the three release-policy selections are mutually distinct");

    // -- block policy selection --
    record_assertion(
        _handler,
        ( std::is_same<
              select_block_policy_t<pool_block_layout::contiguous>,
              contiguous_block_policy>::value ),
        "select_block_policy_t<contiguous> == "
        "contiguous_block_policy");

    record_assertion(
        _handler,
        ( std::is_same<
              select_block_policy_t<pool_block_layout::chunked>,
              chunked_block_policy<>>::value ),
        "select_block_policy_t<chunked> == chunked_block_policy<>");

    record_assertion(
        _handler,
        ( select_block_policy_t<pool_block_layout::chunked>
              ::slots_per_block == 256 ),
        "select_block_policy_t<chunked> defaults to 256 slots "
        "per block");

    record_assertion(
        _handler,
        ( !std::is_same<
               select_block_policy_t<pool_block_layout::contiguous>,
               select_block_policy_t<pool_block_layout::chunked>
           >::value ),
        "the contiguous and chunked block selections are distinct");

    // -- selected policies compose into a usable pool --
    record_assertion(
        _handler,
        ( std::is_same<
              selected_pool::release_policy,
              free_list_release_policy>::value &&
          std::is_same<
              selected_pool::block_policy,
              chunked_block_policy<>>::value ),
        "selector-resolved policies compose into a pool whose "
        "policy aliases round-trip to the selected types");

    // and the composed pool is actually operational
    {
        selected_pool p;
        void* s = p.acquire();

        record_assertion(
            _handler,
            ( s != nullptr ),
            "a pool assembled purely from selector-resolved "
            "policies is operational (acquire succeeds)");

        p.release(s);
        record_assertion(
            _handler,
            ( p.empty() ),
            "the selector-composed pool releases correctly");
    }

    return;
}


NS_END  // test
NS_END  // djinterp
