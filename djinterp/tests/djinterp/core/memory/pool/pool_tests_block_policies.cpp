/*******************************************************************************
* djinterp [test]                                  pool_tests_block_policies.cpp
*
*   Section IV of the pool unit-test suite.
*
*   Covers the two block-policy structs:
*     - contiguous_block_policy
*     - chunked_block_policy<_SlotsPerBlock>
*
*   The contracts under test:
*     - contiguous_block_policy: layout == contiguous,
*       pointer_stable == false (one growable allocation, growth
*       may relocate).
*     - chunked_block_policy<N>: layout == chunked,
*       pointer_stable == true, slots_per_block == N.  The
*       template's default is 256.  The static_assert inside
*       chunked_block_policy rejects _SlotsPerBlock == 0.
*
*
* path:      /tests/djinterp/core/memory/pool/pool_tests_block_policies.cpp
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
//  Compile-time invariants - contiguous_block_policy
// =========================================================================

static_assert(contiguous_block_policy::layout ==
                  pool_block_layout::contiguous,
              "contiguous_block_policy::layout must be "
              "pool_block_layout::contiguous.");

static_assert(contiguous_block_policy::pointer_stable == false,
              "contiguous_block_policy must NOT guarantee pointer "
              "stability across growth.");

static_assert(std::is_trivially_default_constructible<
                  contiguous_block_policy>::value,
              "contiguous_block_policy must be trivially "
              "default-constructible.");

static_assert(std::is_empty<contiguous_block_policy>::value,
              "contiguous_block_policy must be empty.");


// =========================================================================
//  Compile-time invariants - chunked_block_policy (default)
// =========================================================================

static_assert(chunked_block_policy<>::layout ==
                  pool_block_layout::chunked,
              "chunked_block_policy<>::layout must be "
              "pool_block_layout::chunked.");

static_assert(chunked_block_policy<>::pointer_stable == true,
              "chunked_block_policy<> must guarantee pointer "
              "stability across growth.");

static_assert(chunked_block_policy<>::slots_per_block == 256,
              "chunked_block_policy<>::slots_per_block default "
              "must be 256.");

static_assert(std::is_trivially_default_constructible<
                  chunked_block_policy<>>::value,
              "chunked_block_policy<> must be trivially "
              "default-constructible.");

static_assert(std::is_empty<chunked_block_policy<>>::value,
              "chunked_block_policy<> must be empty.");


// =========================================================================
//  Compile-time invariants - chunked_block_policy (custom counts)
// =========================================================================

static_assert(chunked_block_policy<1>::slots_per_block == 1,
              "chunked_block_policy<1>::slots_per_block must "
              "be 1.");

static_assert(chunked_block_policy<2>::slots_per_block == 2,
              "chunked_block_policy<2>::slots_per_block must "
              "be 2.");

static_assert(chunked_block_policy<64>::slots_per_block == 64,
              "chunked_block_policy<64>::slots_per_block must "
              "be 64.");

static_assert(chunked_block_policy<1024>::slots_per_block == 1024,
              "chunked_block_policy<1024>::slots_per_block must "
              "be 1024.");

// each custom instantiation still reports the chunked layout
// and pointer stability
static_assert(chunked_block_policy<1>::layout ==
                  pool_block_layout::chunked,
              "chunked_block_policy<1>::layout must be chunked.");
static_assert(chunked_block_policy<1>::pointer_stable == true,
              "chunked_block_policy<1>::pointer_stable must be "
              "true.");

static_assert(chunked_block_policy<4096>::layout ==
                  pool_block_layout::chunked,
              "chunked_block_policy<4096>::layout must be "
              "chunked.");
static_assert(chunked_block_policy<4096>::pointer_stable == true,
              "chunked_block_policy<4096>::pointer_stable must "
              "be true.");

// custom instantiations of differing N are distinct types
static_assert(!std::is_same<chunked_block_policy<1>,
                            chunked_block_policy<2>>::value,
              "chunked_block_policy<1> and chunked_block_policy<2> "
              "must be distinct types.");

static_assert(!std::is_same<chunked_block_policy<64>,
                            chunked_block_policy<256>>::value,
              "chunked_block_policy<64> and chunked_block_policy<256> "
              "must be distinct types.");

// the default chunked_block_policy<> matches the explicit 256 form
static_assert(std::is_same<chunked_block_policy<>,
                           chunked_block_policy<256>>::value,
              "chunked_block_policy<> default must equal "
              "chunked_block_policy<256>.");


// =========================================================================
//  Cross-policy distinctness
// =========================================================================

static_assert(!std::is_same<contiguous_block_policy,
                            chunked_block_policy<>>::value,
              "contiguous_block_policy and chunked_block_policy<> "
              "must be distinct types.");


// =========================================================================
//  Runtime mirror
// =========================================================================

/*
pool_tests_block_policies_all
  brief description
  Tests the following:
  - contiguous_block_policy reports the contiguous layout and
    no pointer stability
  - chunked_block_policy<> defaults to 256 slots and reports the
    chunked layout with pointer stability
  - chunked_block_policy<N> propagates N as slots_per_block
  - chunked_block_policy<> equals chunked_block_policy<256>
  - varying N in chunked_block_policy yields distinct types
  - both policies are empty and trivially default-constructible
*/
void
pool_tests_block_policies_all(
    test_handler& _handler
)
{
    // -- contiguous_block_policy --
    record_assertion(
        _handler,
        (contiguous_block_policy::layout ==
         pool_block_layout::contiguous),
        "contiguous_block_policy::layout == contiguous");

    record_assertion(
        _handler,
        (contiguous_block_policy::pointer_stable == false),
        "contiguous_block_policy::pointer_stable == false");

    contiguous_block_policy c_inst;
    (void)c_inst;
    record_assertion(_handler,
                     std::is_empty<contiguous_block_policy>::value,
                     "contiguous_block_policy is empty");

    // -- chunked_block_policy<> default --
    record_assertion(
        _handler,
        (chunked_block_policy<>::layout ==
         pool_block_layout::chunked),
        "chunked_block_policy<>::layout == chunked");

    record_assertion(
        _handler,
        (chunked_block_policy<>::pointer_stable == true),
        "chunked_block_policy<>::pointer_stable == true");

    record_assertion(
        _handler,
        (chunked_block_policy<>::slots_per_block == 256),
        "chunked_block_policy<>::slots_per_block default == 256");

    chunked_block_policy<> k_inst;
    (void)k_inst;
    record_assertion(_handler,
                     std::is_empty<chunked_block_policy<>>::value,
                     "chunked_block_policy<> is empty");

    // -- chunked_block_policy<N> propagation --
    record_assertion(
        _handler,
        (chunked_block_policy<1>::slots_per_block == 1),
        "chunked_block_policy<1>::slots_per_block == 1");

    record_assertion(
        _handler,
        (chunked_block_policy<2>::slots_per_block == 2),
        "chunked_block_policy<2>::slots_per_block == 2");

    record_assertion(
        _handler,
        (chunked_block_policy<8>::slots_per_block == 8),
        "chunked_block_policy<8>::slots_per_block == 8");

    record_assertion(
        _handler,
        (chunked_block_policy<128>::slots_per_block == 128),
        "chunked_block_policy<128>::slots_per_block == 128");

    record_assertion(
        _handler,
        (chunked_block_policy<4096>::slots_per_block == 4096),
        "chunked_block_policy<4096>::slots_per_block == 4096");

    // -- distinctness of chunked instantiations --
    record_assertion(
        _handler,
        !std::is_same<chunked_block_policy<1>,
                      chunked_block_policy<2>>::value,
        "chunked_block_policy<1> != chunked_block_policy<2>");

    record_assertion(
        _handler,
        !std::is_same<chunked_block_policy<64>,
                      chunked_block_policy<256>>::value,
        "chunked_block_policy<64> != chunked_block_policy<256>");

    // -- default equals 256 form --
    record_assertion(
        _handler,
        ( std::is_same<chunked_block_policy<>,
                       chunked_block_policy<256>>::value ),
        "chunked_block_policy<> == chunked_block_policy<256>");

    // -- cross-policy distinctness --
    record_assertion(
        _handler,
        !std::is_same<contiguous_block_policy,
                      chunked_block_policy<>>::value,
        "contiguous_block_policy != chunked_block_policy<>");

    return;
}


NS_END  // test
NS_END  // djinterp
