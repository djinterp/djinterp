/*******************************************************************************
* djinterp [test]                                 pool_tests_default_aliases.cpp
*
*   Section VIII of the pool unit-test suite.
*
*   Covers the three convenience template aliases that pin a
* common policy combination onto pool_resource:
*
*     - default_pool_resource<T>
*         free_list release + chunked blocks + exponential
*         growth.  Pointer-stable, safe general-purpose default.
*     - monotonic_pool_resource<T>
*         monotonic release + chunked blocks + exponential
*         growth.  Fastest acquire; bulk reclaim only.
*     - flat_pool_resource<T>
*         free_list release + contiguous block + exponential
*         growth.  Linear slot layout; pointers may move on
*         growth.
*
*   Because pool.hpp pulls in buffer.hpp, exponential_growth_policy
* is in scope here, so each alias is checked for full type
* identity against its hand-spelled pool_resource expansion -
* the strongest possible assertion that the alias pins exactly
* the documented policy set.  The release and block components
* are additionally verified individually so a regression points
* straight at the offending axis.  A light runtime smoke test
* instantiates each alias to confirm it is a usable resource.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_default_aliases.cpp
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
    using elem = pool_test_medium_t;

    // hand-spelled expansions the aliases must match exactly
    using expected_default =
        pool_resource<elem,
                      free_list_release_policy,
                      chunked_block_policy<>,
                      exponential_growth_policy>;

    using expected_monotonic =
        pool_resource<elem,
                      monotonic_release_policy,
                      chunked_block_policy<>,
                      exponential_growth_policy>;

    using expected_flat =
        pool_resource<elem,
                      free_list_release_policy,
                      contiguous_block_policy,
                      exponential_growth_policy>;
}


// =========================================================================
//  Compile-time invariants
// =========================================================================

// --- full-identity checks ---

static_assert(
    std::is_same<
        default_pool_resource<elem>,
        expected_default>::value,
    "default_pool_resource must expand to "
    "pool_resource<T, free_list, chunked<>, exponential>.");

static_assert(
    std::is_same<
        monotonic_pool_resource<elem>,
        expected_monotonic>::value,
    "monotonic_pool_resource must expand to "
    "pool_resource<T, monotonic, chunked<>, exponential>.");

static_assert(
    std::is_same<
        flat_pool_resource<elem>,
        expected_flat>::value,
    "flat_pool_resource must expand to "
    "pool_resource<T, free_list, contiguous, exponential>.");

// --- per-axis breakdown: release policy ---

static_assert(
    std::is_same<
        default_pool_resource<elem>::release_policy,
        free_list_release_policy>::value,
    "default_pool_resource uses the free_list release policy.");

static_assert(
    std::is_same<
        monotonic_pool_resource<elem>::release_policy,
        monotonic_release_policy>::value,
    "monotonic_pool_resource uses the monotonic release policy.");

static_assert(
    std::is_same<
        flat_pool_resource<elem>::release_policy,
        free_list_release_policy>::value,
    "flat_pool_resource uses the free_list release policy.");

// --- per-axis breakdown: block policy ---

static_assert(
    std::is_same<
        default_pool_resource<elem>::block_policy,
        chunked_block_policy<>>::value,
    "default_pool_resource uses chunked blocks.");

static_assert(
    std::is_same<
        monotonic_pool_resource<elem>::block_policy,
        chunked_block_policy<>>::value,
    "monotonic_pool_resource uses chunked blocks.");

static_assert(
    std::is_same<
        flat_pool_resource<elem>::block_policy,
        contiguous_block_policy>::value,
    "flat_pool_resource uses a contiguous block.");

// --- per-axis breakdown: growth policy (all three share it) ---

static_assert(
    std::is_same<
        default_pool_resource<elem>::growth_policy,
        exponential_growth_policy>::value,
    "default_pool_resource uses exponential growth.");

static_assert(
    std::is_same<
        default_pool_resource<elem>::growth_policy,
        monotonic_pool_resource<elem>::growth_policy>::value &&
    std::is_same<
        monotonic_pool_resource<elem>::growth_policy,
        flat_pool_resource<elem>::growth_policy>::value,
    "all three default aliases share the same growth policy.");

// --- the aliases are genuinely different resources ---

static_assert(
    !std::is_same<
        default_pool_resource<elem>,
        monotonic_pool_resource<elem>>::value,
    "default and monotonic aliases differ (release axis).");

static_assert(
    !std::is_same<
        default_pool_resource<elem>,
        flat_pool_resource<elem>>::value,
    "default and flat aliases differ (block axis).");

// --- the element type flows through unchanged ---

static_assert(
    std::is_same<
        default_pool_resource<elem>::value_type,
        elem>::value,
    "default_pool_resource forwards its element type.");


// =========================================================================
//  Runtime tests
// =========================================================================

/*
pool_tests_default_aliases_all
  brief description
  Tests the following:
  - each alias expands to exactly its documented
    pool_resource<...> specialisation
  - the release policy axis of each alias is correct
  - the block policy axis of each alias is correct
  - all three aliases share the exponential growth policy
  - the aliases are mutually distinct types
  - the element type flows through each alias unchanged
  - each alias instantiates into a working, operational pool
*/
void
pool_tests_default_aliases_all(
    test_handler& _handler
)
{
    // -- full identity --
    record_assertion(
        _handler,
        ( std::is_same<
              default_pool_resource<elem>,
              expected_default>::value ),
        "default_pool_resource == pool_resource<T, free_list, "
        "chunked<>, exponential>");

    record_assertion(
        _handler,
        ( std::is_same<
              monotonic_pool_resource<elem>,
              expected_monotonic>::value ),
        "monotonic_pool_resource == pool_resource<T, monotonic, "
        "chunked<>, exponential>");

    record_assertion(
        _handler,
        ( std::is_same<
              flat_pool_resource<elem>,
              expected_flat>::value ),
        "flat_pool_resource == pool_resource<T, free_list, "
        "contiguous, exponential>");

    // -- release axis --
    record_assertion(
        _handler,
        ( std::is_same<
              default_pool_resource<elem>::release_policy,
              free_list_release_policy>::value &&
          std::is_same<
              monotonic_pool_resource<elem>::release_policy,
              monotonic_release_policy>::value &&
          std::is_same<
              flat_pool_resource<elem>::release_policy,
              free_list_release_policy>::value ),
        "alias release-policy axes are correct (free_list / "
        "monotonic / free_list)");

    // -- block axis --
    record_assertion(
        _handler,
        ( std::is_same<
              default_pool_resource<elem>::block_policy,
              chunked_block_policy<>>::value &&
          std::is_same<
              monotonic_pool_resource<elem>::block_policy,
              chunked_block_policy<>>::value &&
          std::is_same<
              flat_pool_resource<elem>::block_policy,
              contiguous_block_policy>::value ),
        "alias block-policy axes are correct (chunked / chunked / "
        "contiguous)");

    // -- shared growth axis --
    record_assertion(
        _handler,
        ( std::is_same<
              default_pool_resource<elem>::growth_policy,
              exponential_growth_policy>::value &&
          std::is_same<
              default_pool_resource<elem>::growth_policy,
              monotonic_pool_resource<elem>::growth_policy>::value &&
          std::is_same<
              monotonic_pool_resource<elem>::growth_policy,
              flat_pool_resource<elem>::growth_policy>::value ),
        "all three aliases share the exponential growth policy");

    // -- distinctness --
    record_assertion(
        _handler,
        ( !std::is_same<
               default_pool_resource<elem>,
               monotonic_pool_resource<elem>>::value &&
          !std::is_same<
               default_pool_resource<elem>,
               flat_pool_resource<elem>>::value ),
        "the three aliases are mutually distinct resource types");

    // -- element forwarding --
    record_assertion(
        _handler,
        ( std::is_same<
              default_pool_resource<elem>::value_type,
              elem>::value ),
        "default_pool_resource forwards its element type");

    // -- runtime smoke: each alias instantiates and operates --
    {
        default_pool_resource<elem> p;
        void* s = p.acquire();
        record_assertion(_handler,
                         (s != nullptr),
                         "default_pool_resource: acquire succeeds");
        p.release(s);
        record_assertion(_handler,
                         (p.empty()),
                         "default_pool_resource: release returns to "
                         "empty");
    }

    {
        monotonic_pool_resource<elem> p;
        void* s = p.acquire();
        record_assertion(_handler,
                         (s != nullptr),
                         "monotonic_pool_resource: acquire succeeds");
        // monotonic release is a no-op for storage but still
        // decrements the live count
        p.release(s);
        record_assertion(_handler,
                         (p.empty()),
                         "monotonic_pool_resource: release decrements "
                         "the live count to empty");
    }

    {
        flat_pool_resource<elem> p;
        void* s = p.acquire();
        record_assertion(_handler,
                         (s != nullptr),
                         "flat_pool_resource: acquire succeeds");
        p.release(s);
        record_assertion(_handler,
                         (p.empty()),
                         "flat_pool_resource: release returns to empty");
    }

    return;
}


NS_END  // test
NS_END  // djinterp
