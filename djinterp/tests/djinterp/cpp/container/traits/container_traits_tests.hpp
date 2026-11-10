/*******************************************************************************
* djinterp [testing]                                  container_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the container_traits.hpp
* unit-test suite.  container_traits.hpp is the legacy UMBRELLA header: it
* defines its own family of structural detectors and composed classifiers
* (constexpr-size, bounds, storage, uniqueness, underlying, array detection,
* interval extraction) and re-exports the newer axis traits, uniting them in the
* DContainerClass snapshot.  These tests cover the traits this header OWNS.
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
*     - DEmptyCt        empty + constexpr size(), no allocator -> compile-time container
*     - fixed4          constexpr size()==max_size()==4        -> fixed size, static storage
*     - DBoundedCap     constexpr max_size()=100 (< SIZE_MAX/2)-> bounded capacity, static storage
*     - DAllocBounded   bounded capacity BUT an allocator_type -> dynamic (not static) storage
*     - min5            constexpr min_size()=5 (> 0)           -> lower bounded
*     - DUnboundedHuge  constexpr max_size()=SIZE_MAX          -> unbounded, dynamic storage
*     - adaptor         an underlying_container_type           -> underlying (not fundamental)
*   Plus detection-only fixtures: DIntervalsBogus (size/depth/multiplicity_interval
* aliases that are NOT valid math intervals), DMultAccessors (multiplicity_min/max),
* hier (hierarchical accessors + node/depth types), and pusher (a push_back member).
*
*   NOTE (intervals).  math/interval/interval.hpp is not available in this test
* environment, so the POSITIVE has_valid_*_interval branch (a genuine math
* interval) cannot be value-exercised; these tests cover interval-alias DETECTION
* and the NEGATIVE validity path (a non-interval alias is rejected).
*
*
* path:      /tests/djinterp/cpp/container/traits/container_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.16
*                                                            revised: 2026.11.09
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
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/cpp/container/traits)
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/container/traits/container_traits.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


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


#endif  // !DTEST_SPEC_MODE
// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


// ===========================================================================
// Helper types
// ===========================================================================

// -- composed-trait control types (explicit constexpr members -> deterministic) --

// empty + constexpr size(), no allocator -> is_compile_time_container
struct DEmptyCt
{
    constexpr std::size_t size() const noexcept { return 0; }
};

// constexpr size() == max_size() == 4 -> has_fixed_size, static storage
struct DFixed4
{
    constexpr std::size_t size()     const noexcept { return 4; }
    constexpr std::size_t max_size() const noexcept { return 4; }
};

// constexpr max_size() = 100 (< SIZE_MAX/2) -> has_bounded_capacity, static storage
struct DBoundedCap
{
    constexpr std::size_t size()     const noexcept { return 0; }
    constexpr std::size_t max_size() const noexcept { return 100; }
};

// bounded capacity BUT an allocator_type -> dynamic storage (not static)
struct DAllocBounded
{
    using value_type     = int;
    using allocator_type = std::allocator<int>;
    constexpr std::size_t size()     const noexcept { return 0; }
    constexpr std::size_t max_size() const noexcept { return 100; }
};

// constexpr min_size() = 5 (> 0) -> is_lower_bounded
struct DMin5
{
    constexpr std::size_t size()     const noexcept { return 5; }
    constexpr std::size_t min_size() const noexcept { return 5; }
};

// constexpr max_size() = SIZE_MAX -> NOT bounded capacity -> unbounded, dynamic
struct DUnboundedHuge
{
    constexpr std::size_t size()     const noexcept { return 0; }
    constexpr std::size_t max_size() const noexcept
    { return (std::numeric_limits<std::size_t>::max)(); }
};

// an underlying_container_type -> underlying-based (not fundamental)
struct DAdaptor
{
    using value_type               = int;
    using underlying_container_type = std::vector<int>;
    std::size_t size() const noexcept { return 0; }
};

// -- detection-only fixtures --

// interval-protocol aliases that are NOT valid math intervals (int is no interval)
struct DIntervalsBogus
{
    using size_interval         = int;
    using depth_interval        = int;
    using multiplicity_interval = int;
};

// legacy multiplicity accessor pair -> has_bounded_multiplicity
struct DMultAccessors
{
    std::size_t multiplicity_min() const noexcept { return 1; }
    std::size_t multiplicity_max() const noexcept { return 5; }
};

// hierarchical protocol: node/depth types + parent/children/root/depth accessors
struct DHier
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

// a push_back member and nothing else of a container (the umbrella's
// mutation probe)
struct pusher
{
    using value_type = int;
    void push_back(int) {}
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



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


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// container_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
container_traits_spec()
{
    return dt::module_spec{
        "container_traits",
        "djinterp container compile-time classification traits SFINAE-based "
        "detection and classification...",
        {
            dt::block_spec{
                "method_detection",
                "Method detection",
                {
                    { "tests_size_capacity_detection",
                      "checks (has_data_method<std::vector<int>>::value)",
                      &tests_size_capacity_detection },
                    { "tests_type_member_detection",
                      "checks (has_key_compare<std::set<int>>::value)",
                      &tests_type_member_detection },
                    { "tests_hierarchical_detection",
                      "checks (has_parent_accessor<DHier>::value)",
                      &tests_hierarchical_detection },
                    { "tests_mutation_detection",
                      "checks (has_push_back<pusher>::value)",
                      &tests_mutation_detection },
                    { "tests_iteration_detection",
                      "checks (has_begin_accessor<std::vector<int>>::value)",
                      &tests_iteration_detection },
                    { "tests_invariant_detection",
                      "checks (has_unique_accessor<std::list<int>>::value)",
                      &tests_invariant_detection },
                }
            },
            dt::block_spec{
                "constexpr_traits",
                "Constexpr traits",
                {
                    { "tests_constexpr_size_traits",
                      "checks (has_constexpr_size<DEmptyCt>::value)",
                      &tests_constexpr_size_traits },
                    { "tests_size_shape_traits",
                      "checks (has_fixed_size<std::array<int, 4>>::value)",
                      &tests_size_shape_traits },
                    { "tests_interval_validity",
                      "checks "
                      "(!has_valid_size_interval<DIntervalsBogus>::value)",
                      &tests_interval_validity },
                }
            },
            dt::block_spec{
                "array",
                "Array",
                {
                    { "tests_array_detection",
                      "checks (is_c_array<int[4]>::value)",
                      &tests_array_detection },
                }
            },
            dt::block_spec{
                "lifetime",
                "Lifetime",
                {
                    { "tests_lifetime_classification",
                      "checks (is_compile_time_container<DEmptyCt>::value)",
                      &tests_lifetime_classification },
                }
            },
            dt::block_spec{
                "bounds",
                "Bounds",
                {
                    { "tests_is_lower_bounded",
                      "checks (is_lower_bounded<DMin5>::value)",
                      &tests_is_lower_bounded },
                    { "tests_is_upper_bounded",
                      "checks (is_upper_bounded<std::array<int, 4>>::value)",
                      &tests_is_upper_bounded },
                    { "tests_is_unbounded_and_interval_bounded",
                      "checks (is_unbounded<DEmptyCt>::value)",
                      &tests_is_unbounded_and_interval_bounded },
                }
            },
            dt::block_spec{
                "storage",
                "Storage",
                {
                    { "tests_is_static_storage",
                      "checks (is_static_storage_container<std::array<int, "
                      "4>>::value)",
                      &tests_is_static_storage },
                    { "tests_is_dynamic_storage",
                      "checks "
                      "(is_dynamic_storage_container<std::vector<int>>::value)",
                      &tests_is_dynamic_storage },
                }
            },
            dt::block_spec{
                "multiplicity",
                "Multiplicity",
                {
                    { "tests_enforces_uniqueness",
                      "checks (enforces_uniqueness<std::set<int>>::value)",
                      &tests_enforces_uniqueness },
                    { "tests_allows_duplicates",
                      "checks (allows_duplicates<std::map<int, int>>::value)",
                      &tests_allows_duplicates },
                    { "tests_has_bounded_multiplicity",
                      "checks "
                      "(has_bounded_multiplicity<DMultAccessors>::value)",
                      &tests_has_bounded_multiplicity },
                }
            },
            dt::block_spec{
                "underlying",
                "Underlying",
                {
                    { "tests_is_underlying_container",
                      "checks (is_underlying_container<DAdaptor>::value)",
                      &tests_is_underlying_container },
                    { "tests_is_fundamental_container",
                      "checks (is_fundamental_container<std::array<int, "
                      "4>>::value)",
                      &tests_is_fundamental_container },
                    { "tests_underlying_container_type_of",
                      "checks "
                      "(std::is_same<underlying_container_type_of_t<DAdaptor>, "
                      "std::vector",
                      &tests_underlying_container_type_of },
                    { "tests_interval_extraction",
                      "checks "
                      "(std::is_same<size_interval_of_t<DIntervalsBogus>, "
                      "void>::value)",
                      &tests_interval_extraction },
                }
            },
            dt::block_spec{
                "classification",
                "Classification",
                {
                    { "tests_container_class",
                      "checks (!C::is_compile_time)",
                      &tests_container_class },
                    { "tests_container_variable_templates",
                      "checks (has_size_accessor_v<std::vector<int>> == "
                      "has_size_accessor<std::ve",
                      &tests_container_variable_templates },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TRAITS_TESTS_
