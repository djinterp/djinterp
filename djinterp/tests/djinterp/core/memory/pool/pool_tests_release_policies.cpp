/*******************************************************************************
* djinterp [test]                                pool_tests_release_policies.cpp
*
*   Section III of the pool unit-test suite.
*
*   Covers the three release-policy structs:
*     - monotonic_release_policy
*     - free_list_release_policy
*     - generational_release_policy
*
*   Each policy is a stateless struct exposing three compile-time
* constants (`strategy`, `supports_individual_release`,
* `supports_generational_sweep`) plus, for generational, a
* `generation_type` alias.
*
*   The contract is:
*     - monotonic: release is a no-op, no per-slot reclamation,
*                  no generational sweep.
*     - free_list: per-slot release supported, no generational sweep.
*     - generational: per-slot release supported AND generational
*                     sweep supported, with an unsigned integer
*                     generation_type.
*
*   These are structural contracts verified almost entirely at
* compile time; the runtime mirror is provided so the framework's
* report carries a positive record for each.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_release_policies.cpp
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


// =========================================================================
//  Compile-time invariants - monotonic_release_policy
// =========================================================================

static_assert(monotonic_release_policy::strategy ==
                  pool_release::monotonic,
              "monotonic_release_policy::strategy must be "
              "pool_release::monotonic.");

static_assert(monotonic_release_policy::supports_individual_release
                  == false,
              "monotonic_release_policy must NOT support "
              "individual release.");

static_assert(monotonic_release_policy::supports_generational_sweep
                  == false,
              "monotonic_release_policy must NOT support "
              "generational sweep.");

// monotonic policy is stateless and trivially constructible
static_assert(std::is_trivially_default_constructible<
                  monotonic_release_policy>::value,
              "monotonic_release_policy must be trivially "
              "default-constructible.");

static_assert(std::is_empty<monotonic_release_policy>::value,
              "monotonic_release_policy must be empty.");


// =========================================================================
//  Compile-time invariants - free_list_release_policy
// =========================================================================

static_assert(free_list_release_policy::strategy ==
                  pool_release::free_list,
              "free_list_release_policy::strategy must be "
              "pool_release::free_list.");

static_assert(free_list_release_policy::supports_individual_release
                  == true,
              "free_list_release_policy must support individual "
              "release.");

static_assert(free_list_release_policy::supports_generational_sweep
                  == false,
              "free_list_release_policy must NOT support "
              "generational sweep.");

static_assert(std::is_trivially_default_constructible<
                  free_list_release_policy>::value,
              "free_list_release_policy must be trivially "
              "default-constructible.");

static_assert(std::is_empty<free_list_release_policy>::value,
              "free_list_release_policy must be empty.");


// =========================================================================
//  Compile-time invariants - generational_release_policy
// =========================================================================

static_assert(generational_release_policy::strategy ==
                  pool_release::generational,
              "generational_release_policy::strategy must be "
              "pool_release::generational.");

static_assert(
    generational_release_policy::supports_individual_release == true,
    "generational_release_policy must support individual release.");

static_assert(
    generational_release_policy::supports_generational_sweep == true,
    "generational_release_policy must support generational sweep.");

static_assert(std::is_trivially_default_constructible<
                  generational_release_policy>::value,
              "generational_release_policy must be trivially "
              "default-constructible.");

static_assert(std::is_empty<generational_release_policy>::value,
              "generational_release_policy must be empty.");

// generation_type contract: unsigned integral
static_assert(
    std::is_integral<
        generational_release_policy::generation_type>::value,
    "generational_release_policy::generation_type must be integral.");

static_assert(
    std::is_unsigned<
        generational_release_policy::generation_type>::value,
    "generational_release_policy::generation_type must be unsigned.");


// =========================================================================
//  Cross-policy distinctness
// =========================================================================

static_assert(!std::is_same<monotonic_release_policy,
                            free_list_release_policy>::value,
              "monotonic_release_policy and free_list_release_policy "
              "must be distinct types.");

static_assert(!std::is_same<monotonic_release_policy,
                            generational_release_policy>::value,
              "monotonic_release_policy and "
              "generational_release_policy must be distinct types.");

static_assert(!std::is_same<free_list_release_policy,
                            generational_release_policy>::value,
              "free_list_release_policy and "
              "generational_release_policy must be distinct types.");


// =========================================================================
//  Runtime mirror
// =========================================================================

/*
pool_tests_release_policies_all
  brief description
  Tests the following:
  - monotonic_release_policy carries the expected strategy and
    release / sweep flags
  - free_list_release_policy carries the expected strategy and
    release / sweep flags
  - generational_release_policy carries the expected strategy
    and release / sweep flags, plus an integral unsigned
    generation_type alias
  - all three policy types are empty, trivially default-
    constructible, and pairwise distinct
  - instantiating each policy at runtime gives the same constant
    values as the static_assert tier
*/
void
pool_tests_release_policies_all(
    test_handler& _handler
)
{
    // -- monotonic_release_policy --
    record_assertion(
        _handler,
        (monotonic_release_policy::strategy ==
         pool_release::monotonic),
        "monotonic_release_policy::strategy == monotonic");

    record_assertion(
        _handler,
        (monotonic_release_policy::supports_individual_release ==
         false),
        "monotonic_release_policy does NOT support individual "
        "release");

    record_assertion(
        _handler,
        (monotonic_release_policy::supports_generational_sweep ==
         false),
        "monotonic_release_policy does NOT support generational "
        "sweep");

    // instantiate to verify it really is constructible
    monotonic_release_policy m_inst;
    (void)m_inst;
    record_assertion(_handler,
                     std::is_empty<monotonic_release_policy>::value,
                     "monotonic_release_policy is empty");

    // -- free_list_release_policy --
    record_assertion(
        _handler,
        (free_list_release_policy::strategy ==
         pool_release::free_list),
        "free_list_release_policy::strategy == free_list");

    record_assertion(
        _handler,
        (free_list_release_policy::supports_individual_release ==
         true),
        "free_list_release_policy supports individual release");

    record_assertion(
        _handler,
        (free_list_release_policy::supports_generational_sweep ==
         false),
        "free_list_release_policy does NOT support generational "
        "sweep");

    free_list_release_policy f_inst;
    (void)f_inst;
    record_assertion(_handler,
                     std::is_empty<free_list_release_policy>::value,
                     "free_list_release_policy is empty");

    // -- generational_release_policy --
    record_assertion(
        _handler,
        (generational_release_policy::strategy ==
         pool_release::generational),
        "generational_release_policy::strategy == generational");

    record_assertion(
        _handler,
        (generational_release_policy::supports_individual_release ==
         true),
        "generational_release_policy supports individual release");

    record_assertion(
        _handler,
        (generational_release_policy::supports_generational_sweep ==
         true),
        "generational_release_policy supports generational sweep");

    record_assertion(
        _handler,
        std::is_integral<
            generational_release_policy::generation_type>::value,
        "generational_release_policy::generation_type is integral");

    record_assertion(
        _handler,
        std::is_unsigned<
            generational_release_policy::generation_type>::value,
        "generational_release_policy::generation_type is unsigned");

    generational_release_policy g_inst;
    (void)g_inst;
    record_assertion(
        _handler,
        std::is_empty<generational_release_policy>::value,
        "generational_release_policy is empty");

    // -- cross-policy distinctness --
    record_assertion(
        _handler,
        !std::is_same<monotonic_release_policy,
                      free_list_release_policy>::value,
        "monotonic_release_policy != free_list_release_policy");

    record_assertion(
        _handler,
        !std::is_same<monotonic_release_policy,
                      generational_release_policy>::value,
        "monotonic_release_policy != generational_release_policy");

    record_assertion(
        _handler,
        !std::is_same<free_list_release_policy,
                      generational_release_policy>::value,
        "free_list_release_policy != generational_release_policy");

    // -- generation_type is at least 32 bits (sweep counter
    //    needs significant range; pool.hpp uses uint32_t) --
    record_assertion(
        _handler,
        ( sizeof(generational_release_policy::generation_type) >=
          sizeof(std::uint32_t) ),
        "generational_release_policy::generation_type is at "
        "least 32 bits");

    return;
}


NS_END  // test
NS_END  // djinterp
