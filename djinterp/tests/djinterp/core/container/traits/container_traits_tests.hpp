/*******************************************************************************
* djinterp [testing]                                  container_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the container_traits.hpp
* unit-test suite.  container_traits.hpp is the legacy UMBRELLA header: it
* defines its own family of structural detectors and composed classifiers
* (constexpr-size, bounds, storage, uniqueness, underlying, array detection,
* interval extraction) and re-exports the newer axis traits, uniting them in the
* container_class snapshot.  These tests cover the traits this header OWNS.
*
*   MECHANISM.  The traits are compile-time facts; each check compares a constant
* reading against its expected result.  A failing check prints the offending
* expression and drives the function's bool result to false, which the runner
* turns into a non-zero exit code.
*
*   DETERMINISM.  Several of this header's traits pivot on whether `T{}.size()`
* (etc.) is a constant expression, which for std:: containers is standard- and
* stdlib-dependent (e.g. std::vector is constexpr-sized in C++20 but not C++17).
* To keep the branch coverage deterministic and standard-independent, the custom
* helper types below expose explicit `constexpr` size / max_size / min_size, so
* their classification is fixed regardless of toolchain:
*     - empty_ct        empty + constexpr size(), no allocator -> compile-time container
*     - fixed4          constexpr size()==max_size()==4        -> fixed size, static storage
*     - bounded_cap     constexpr max_size()=100 (< SIZE_MAX/2)-> bounded capacity, static storage
*     - alloc_bounded   bounded capacity BUT an allocator_type -> dynamic (not static) storage
*     - min5            constexpr min_size()=5 (> 0)           -> lower bounded
*     - unbounded_huge  constexpr max_size()=SIZE_MAX          -> unbounded, dynamic storage
*     - adaptor         an underlying_container_type           -> underlying (not fundamental)
*   Plus detection-only fixtures: intervals_bogus (size/depth/multiplicity_interval
* aliases that are NOT valid math intervals), mult_accessors (multiplicity_min/max),
* hier (hierarchical accessors + node/depth types), and pusher (a push_ member).
*
*   NOTE (intervals).  math/interval/interval.hpp is not available in this test
* environment, so the POSITIVE has_valid_*_interval branch (a genuine math
* interval) cannot be value-exercised; these tests cover interval-alias DETECTION
* and the NEGATIVE validity path (a non-interval alias is rejected).
*
*
* path:      /tests/djinterp/core/container/traits/container_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.16
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRAITS_TESTS_
#define DJINTERP_CONTAINER_TRAITS_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <forward_list>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>
// djinterp
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/core/container/traits)
#include "../../../../../inc/djinterp/core/container/traits/container_traits.hpp"


#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


// D_CT_CHECK
//   macro: the per-suite runtime check (unique letters: CT = Container Traits).
// Records a failure into the enclosing function's `_ct_ok` accumulator and prints
// the offending expression.  Always pass the condition already wrapped in
// parentheses so interior template commas survive macro argument splitting.
#define D_CT_CHECK(_cond)                                                     \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _ct_ok = false;                                                   \
        }                                                                     \
    } while (false)


NS_DJINTERP
NS_TESTING


// ===========================================================================
// Helper types
// ===========================================================================

// -- composed-trait control types (explicit constexpr members -> deterministic) --

// empty + constexpr size(), no allocator -> is_compile_time_container
struct empty_ct
{
    constexpr std::size_t size() const noexcept { return 0; }
};

// constexpr size() == max_size() == 4 -> has_fixed_size, static storage
struct fixed4
{
    constexpr std::size_t size()     const noexcept { return 4; }
    constexpr std::size_t max_size() const noexcept { return 4; }
};

// constexpr max_size() = 100 (< SIZE_MAX/2) -> has_bounded_capacity, static storage
struct bounded_cap
{
    constexpr std::size_t size()     const noexcept { return 0; }
    constexpr std::size_t max_size() const noexcept { return 100; }
};

// bounded capacity BUT an allocator_type -> dynamic storage (not static)
struct alloc_bounded
{
    using value_type     = int;
    using allocator_type = std::allocator<int>;
    constexpr std::size_t size()     const noexcept { return 0; }
    constexpr std::size_t max_size() const noexcept { return 100; }
};

// constexpr min_size() = 5 (> 0) -> is_lower_bounded
struct min5
{
    constexpr std::size_t size()     const noexcept { return 5; }
    constexpr std::size_t min_size() const noexcept { return 5; }
};

// constexpr max_size() = SIZE_MAX -> NOT bounded capacity -> unbounded, dynamic
struct unbounded_huge
{
    constexpr std::size_t size()     const noexcept { return 0; }
    constexpr std::size_t max_size() const noexcept
    { return (std::numeric_limits<std::size_t>::max)(); }
};

// an underlying_container_type -> underlying-based (not fundamental)
struct adaptor
{
    using value_type               = int;
    using underlying_container_type = std::vector<int>;
    std::size_t size() const noexcept { return 0; }
};

// -- detection-only fixtures --

// interval-protocol aliases that are NOT valid math intervals (int is no interval)
struct intervals_bogus
{
    using size_interval         = int;
    using depth_interval        = int;
    using multiplicity_interval = int;
};

// legacy multiplicity accessor pair -> has_bounded_multiplicity
struct mult_accessors
{
    std::size_t multiplicity_min() const noexcept { return 1; }
    std::size_t multiplicity_max() const noexcept { return 5; }
};

// hierarchical protocol: node/depth types + parent/children/root/depth accessors
struct hier
{
    using node_type  = int;
    using depth_type = int;
    int         parent()    const { return 0; }
    int         children()  const { return 0; }
    int         root()      const { return 0; }
    std::size_t depth()     const { return 0; }
    std::size_t max_depth() const { return 0; }
    std::size_t min_depth() const { return 0; }
};

// a push_ member (the umbrella's mutation probe)
struct pusher
{
    using value_type = int;
    void push_(int) {}
};


// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.   Method detection --
bool tests_size_capacity_detection();
bool tests_type_member_detection();
bool tests_hierarchical_detection();
bool tests_mutation_detection();
bool tests_iteration_detection();
bool tests_invariant_detection();

// -- III. Public constexpr traits --
bool tests_constexpr_size_traits();
bool tests_size_shape_traits();
bool tests_interval_validity();

// -- IV.  Array detection --
bool tests_array_detection();

// -- V.   Lifetime classification --
bool tests_lifetime_classification();

// -- VI.  Bounds classification --
bool tests_is_lower_bounded();
bool tests_is_upper_bounded();
bool tests_is_unbounded_and_interval_bounded();

// -- VII. Storage classification --
bool tests_is_static_storage();
bool tests_is_dynamic_storage();

// -- XII. Uniqueness and multiplicity --
bool tests_enforces_uniqueness();
bool tests_allows_duplicates();
bool tests_has_bounded_multiplicity();

// -- XIII-XV. Underlying + extraction --
bool tests_is_underlying_container();
bool tests_is_fundamental_container();
bool tests_underlying_container_type_of();
bool tests_interval_extraction();

// -- XVI. Combined classification + variable-template companions --
bool tests_container_class();
bool tests_container_variable_templates();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TRAITS_TESTS_
