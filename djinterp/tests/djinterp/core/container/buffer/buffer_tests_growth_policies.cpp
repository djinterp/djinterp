/*******************************************************************************
* djinterp [test]                               buffer_tests_growth_policies.cpp
*
*   Unit tests for the four growth-policy structs (section III of buffer.hpp):
* fixed_growth_policy, linear_growth_policy, exponential_growth_policy, and
* page_growth_policy.  Each policy is covered for its declared constants
* (strategy / can_grow / additional fields) and for every branch of its
* compute() method, including no-growth-needed, single-step growth, multi-
* step growth, and boundary inputs.
*
*
* path:      /tests/djinterp/core/container/buffer/buffer_tests_growth_policies.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "buffer_tests.hpp"


NS_DJINTERP
NS_TEST


/*
tests_fixed_growth_policy
  Covers the fixed_growth_policy struct.
  Tests the following:
  - strategy constant equals buffer_growth_strategy::none
  - can_grow constant is false
  - compute() returns the current capacity unchanged when the
    required capacity is less than, equal to, or greater than
    the current capacity (the _required argument is ignored
    by contract)
  - compute() is noexcept and usable in a constexpr context
*/
void
tests_fixed_growth_policy(
    test_handler& _handler
)
{
    using policy = fixed_growth_policy;

    // --- constants ---

    static_assert(
        policy::strategy == buffer_growth_strategy::none,
        "fixed_growth_policy::strategy must be ::none.");
    static_assert(
        policy::can_grow == false,
        "fixed_growth_policy::can_grow must be false.");

    record_assertion(
        _handler,
        policy::strategy == buffer_growth_strategy::none,
        "fixed_growth_policy::strategy == buffer_growth_strategy::none");

    record_assertion(
        _handler,
        policy::can_grow == false,
        "fixed_growth_policy::can_grow == false");

    // --- compute(): _required is ignored ---

    record_assertion(
        _handler,
        policy::compute(0, 0) == 0,
        "fixed: compute(0, 0) == 0");

    record_assertion(
        _handler,
        policy::compute(0, 1000) == 0,
        "fixed: compute(0, 1000) == 0  "
        "(required ignored when current is 0)");

    record_assertion(
        _handler,
        policy::compute(128, 64) == 128,
        "fixed: compute(128, 64) == 128");

    record_assertion(
        _handler,
        policy::compute(128, 128) == 128,
        "fixed: compute(128, 128) == 128");

    record_assertion(
        _handler,
        policy::compute(128, 1024) == 128,
        "fixed: compute(128, 1024) == 128  (required ignored)");

    // --- constexpr usability ---

    constexpr std::size_t k_cx = policy::compute(64, 1024);

    static_assert(
        k_cx == 64,
        "fixed_growth_policy::compute must be constexpr-evaluable "
        "and return current unchanged.");

    record_assertion(
        _handler,
        k_cx == 64,
        "fixed: compute is constexpr-evaluable");

    return;
}


/*
tests_linear_growth_policy
  Covers the linear_growth_policy template.
  Tests the following:
  - strategy constant equals buffer_growth_strategy::linear
  - can_grow constant is true
  - default increment is 4096
  - a custom increment is honored
  - compute() returns current unchanged when current >= required
  - compute() grows by exactly one increment when one is enough
  - compute() grows by multiple increments when more are needed
  - compute() is constexpr-evaluable
  - the static_assert on _Increment > 0 is implicitly verified by
    successful instantiation with non-zero increments
*/
void
tests_linear_growth_policy(
    test_handler& _handler
)
{
    using default_policy = linear_growth_policy<>;
    using small_policy   = linear_growth_policy<16>;
    using one_policy     = linear_growth_policy<1>;

    // --- constants ---

    static_assert(
        default_policy::strategy == buffer_growth_strategy::linear,
        "linear_growth_policy::strategy must be ::linear.");
    static_assert(
        default_policy::can_grow == true,
        "linear_growth_policy::can_grow must be true.");
    static_assert(
        default_policy::increment == 4096,
        "linear_growth_policy<>::increment default must be 4096.");
    static_assert(
        small_policy::increment == 16,
        "linear_growth_policy<16>::increment must be 16.");
    static_assert(
        one_policy::increment == 1,
        "linear_growth_policy<1>::increment must be 1.");

    record_assertion(
        _handler,
        default_policy::strategy == buffer_growth_strategy::linear,
        "linear: strategy == ::linear");

    record_assertion(
        _handler,
        default_policy::can_grow == true,
        "linear: can_grow == true");

    record_assertion(
        _handler,
        default_policy::increment == 4096,
        "linear: default increment == 4096");

    record_assertion(
        _handler,
        small_policy::increment == 16,
        "linear<16>::increment == 16");

    // --- compute(): no growth path (current >= required) ---

    record_assertion(
        _handler,
        small_policy::compute(32, 0) == 32,
        "linear<16>: compute(32, 0) == 32  (no growth needed)");

    record_assertion(
        _handler,
        small_policy::compute(32, 16) == 32,
        "linear<16>: compute(32, 16) == 32  (no growth needed)");

    record_assertion(
        _handler,
        small_policy::compute(32, 32) == 32,
        "linear<16>: compute(32, 32) == 32  (boundary, no growth)");

    // --- compute(): single-step growth ---

    record_assertion(
        _handler,
        small_policy::compute(32, 33) == 48,
        "linear<16>: compute(32, 33) == 48  (one increment)");

    record_assertion(
        _handler,
        small_policy::compute(0, 16) == 16,
        "linear<16>: compute(0, 16) == 16  (one increment from zero)");

    record_assertion(
        _handler,
        small_policy::compute(0, 1) == 16,
        "linear<16>: compute(0, 1) == 16  (rounds up to one increment)");

    // --- compute(): multi-step growth ---

    record_assertion(
        _handler,
        small_policy::compute(0, 100) == 112,
        "linear<16>: compute(0, 100) == 112  (seven increments)");

    record_assertion(
        _handler,
        small_policy::compute(32, 100) == 112,
        "linear<16>: compute(32, 100) == 112  (five increments above 32)");

    record_assertion(
        _handler,
        one_policy::compute(0, 5) == 5,
        "linear<1>: compute(0, 5) == 5  (five increments of one)");

    // --- compute(): constexpr usability ---

    constexpr std::size_t k_cx = small_policy::compute(0, 50);

    static_assert(
        k_cx == 64,
        "linear<16>::compute(0, 50) should be 64 at compile time.");

    record_assertion(
        _handler,
        k_cx == 64,
        "linear<16>: compute is constexpr-evaluable");

    return;
}


/*
tests_exponential_growth_policy
  Covers the exponential_growth_policy struct.
  Tests the following:
  - strategy constant equals buffer_growth_strategy::exponential
  - can_grow constant is true
  - min_capacity constant equals 64
  - compute(0, r) bootstraps from min_capacity and doubles until
    capacity >= required
  - compute(c, r) where c > 0 doubles starting from c
  - compute(c, r) returns current unchanged when c is already
    sufficient
  - compute() requires multiple doublings for large required values
  - compute() is constexpr-evaluable
*/
void
tests_exponential_growth_policy(
    test_handler& _handler
)
{
    using policy = exponential_growth_policy;

    // --- constants ---

    static_assert(
        policy::strategy == buffer_growth_strategy::exponential,
        "exponential_growth_policy::strategy must be ::exponential.");
    static_assert(
        policy::can_grow == true,
        "exponential_growth_policy::can_grow must be true.");
    static_assert(
        policy::min_capacity == 64,
        "exponential_growth_policy::min_capacity must be 64.");

    record_assertion(
        _handler,
        policy::strategy == buffer_growth_strategy::exponential,
        "exponential: strategy == ::exponential");

    record_assertion(
        _handler,
        policy::can_grow == true,
        "exponential: can_grow == true");

    record_assertion(
        _handler,
        policy::min_capacity == 64,
        "exponential: min_capacity == 64");

    // --- compute(): bootstrap from zero uses min_capacity ---

    record_assertion(
        _handler,
        policy::compute(0, 0) == 64,
        "exponential: compute(0, 0) == 64  "
        "(zero current bootstraps to min_capacity even when "
        "required is zero, because loop terminates immediately)");

    record_assertion(
        _handler,
        policy::compute(0, 1) == 64,
        "exponential: compute(0, 1) == 64  (bootstrap to min)");

    record_assertion(
        _handler,
        policy::compute(0, 64) == 64,
        "exponential: compute(0, 64) == 64  (bootstrap exact match)");

    record_assertion(
        _handler,
        policy::compute(0, 65) == 128,
        "exponential: compute(0, 65) == 128  (bootstrap then one doubling)");

    record_assertion(
        _handler,
        policy::compute(0, 1000) == 1024,
        "exponential: compute(0, 1000) == 1024  "
        "(bootstrap + four doublings: 64->128->256->512->1024)");

    // --- compute(): doubling from non-zero current ---

    record_assertion(
        _handler,
        policy::compute(16, 17) == 32,
        "exponential: compute(16, 17) == 32  (one doubling)");

    record_assertion(
        _handler,
        policy::compute(8, 100) == 128,
        "exponential: compute(8, 100) == 128  "
        "(8->16->32->64->128, four doublings)");

    record_assertion(
        _handler,
        policy::compute(1, 1) == 1,
        "exponential: compute(1, 1) == 1  (current already sufficient)");

    record_assertion(
        _handler,
        policy::compute(128, 50) == 128,
        "exponential: compute(128, 50) == 128  (current already sufficient)");

    record_assertion(
        _handler,
        policy::compute(128, 128) == 128,
        "exponential: compute(128, 128) == 128  (exact boundary)");

    // --- compute(): constexpr usability ---

    constexpr std::size_t k_cx = policy::compute(0, 200);

    static_assert(
        k_cx == 256,
        "exponential::compute(0, 200) should be 256 at compile time.");

    record_assertion(
        _handler,
        k_cx == 256,
        "exponential: compute is constexpr-evaluable");

    return;
}


/*
tests_page_growth_policy
  Covers the page_growth_policy template.
  Tests the following:
  - strategy constant equals buffer_growth_strategy::page_aligned
  - can_grow constant is true
  - default page_size is 4096
  - a custom page_size (still power-of-two) is honored
  - compute() rounds the required value up to the next page-aligned
    multiple
  - compute() with required equal to a page boundary returns that
    value unchanged
  - compute() with required equal to zero returns zero (no allocation)
  - compute() ignores the _current parameter (page-aligned policy
    is stateless across calls)
  - compute() is constexpr-evaluable
*/
void
tests_page_growth_policy(
    test_handler& _handler
)
{
    using default_policy = page_growth_policy<>;
    using small_policy   = page_growth_policy<16>;
    using tiny_policy    = page_growth_policy<2>;

    // --- constants ---

    static_assert(
        default_policy::strategy == buffer_growth_strategy::page_aligned,
        "page_growth_policy::strategy must be ::page_aligned.");
    static_assert(
        default_policy::can_grow == true,
        "page_growth_policy::can_grow must be true.");
    static_assert(
        default_policy::page_size == 4096,
        "page_growth_policy<>::page_size default must be 4096.");
    static_assert(
        small_policy::page_size == 16,
        "page_growth_policy<16>::page_size must be 16.");

    record_assertion(
        _handler,
        default_policy::strategy == buffer_growth_strategy::page_aligned,
        "page: strategy == ::page_aligned");

    record_assertion(
        _handler,
        default_policy::can_grow == true,
        "page: can_grow == true");

    record_assertion(
        _handler,
        default_policy::page_size == 4096,
        "page: default page_size == 4096");

    record_assertion(
        _handler,
        small_policy::page_size == 16,
        "page<16>::page_size == 16");

    // --- compute(): zero required ---

    record_assertion(
        _handler,
        small_policy::compute(0, 0) == 0,
        "page<16>: compute(0, 0) == 0  (zero required => zero)");

    record_assertion(
        _handler,
        small_policy::compute(999, 0) == 0,
        "page<16>: compute(999, 0) == 0  (current is ignored)");

    // --- compute(): boundary (exact page multiple) ---

    record_assertion(
        _handler,
        small_policy::compute(0, 16) == 16,
        "page<16>: compute(0, 16) == 16  (exact one-page boundary)");

    record_assertion(
        _handler,
        small_policy::compute(0, 32) == 32,
        "page<16>: compute(0, 32) == 32  (exact two-page boundary)");

    record_assertion(
        _handler,
        small_policy::compute(0, 64) == 64,
        "page<16>: compute(0, 64) == 64  (exact four-page boundary)");

    // --- compute(): rounding up ---

    record_assertion(
        _handler,
        small_policy::compute(0, 1) == 16,
        "page<16>: compute(0, 1) == 16  (rounds up to one page)");

    record_assertion(
        _handler,
        small_policy::compute(0, 15) == 16,
        "page<16>: compute(0, 15) == 16  (rounds up to one page)");

    record_assertion(
        _handler,
        small_policy::compute(0, 17) == 32,
        "page<16>: compute(0, 17) == 32  (rounds up to two pages)");

    record_assertion(
        _handler,
        small_policy::compute(0, 33) == 48,
        "page<16>: compute(0, 33) == 48  (rounds up to three pages)");

    // --- compute(): current is unused ---

    record_assertion(
        _handler,
        small_policy::compute(1024, 17) == 32,
        "page<16>: compute(1024, 17) == 32  "
        "(current is documented as ignored)");

    // --- compute(): tiny page size still works (minimum power of two) ---

    record_assertion(
        _handler,
        tiny_policy::compute(0, 3) == 4,
        "page<2>: compute(0, 3) == 4");

    record_assertion(
        _handler,
        tiny_policy::compute(0, 7) == 8,
        "page<2>: compute(0, 7) == 8");

    // --- compute(): default 4096-byte page ---

    record_assertion(
        _handler,
        default_policy::compute(0, 1) == 4096,
        "page<>: compute(0, 1) == 4096");

    record_assertion(
        _handler,
        default_policy::compute(0, 4096) == 4096,
        "page<>: compute(0, 4096) == 4096");

    record_assertion(
        _handler,
        default_policy::compute(0, 4097) == 8192,
        "page<>: compute(0, 4097) == 8192");

    record_assertion(
        _handler,
        default_policy::compute(0, 12345) == 16384,
        "page<>: compute(0, 12345) == 16384");

    // --- compute(): constexpr usability ---

    constexpr std::size_t k_cx = small_policy::compute(0, 17);

    static_assert(
        k_cx == 32,
        "page<16>::compute(0, 17) should be 32 at compile time.");

    record_assertion(
        _handler,
        k_cx == 32,
        "page<16>: compute is constexpr-evaluable");

    return;
}


NS_END  // test
NS_END  // djinterp
