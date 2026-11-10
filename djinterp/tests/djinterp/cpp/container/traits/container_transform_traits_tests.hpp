/*******************************************************************************
* djinterp [testing]                        container_transform_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the
* container_transform_traits.hpp unit-test suite.  The Transformability axis is
* the twin of Filterability: a READ capability (iterability + a value_type) aligned
* with a BUILD capability (push_back or associative insert), exercised over a map
* f : tau -> sigma.  A type is a SOURCE if it can be read, TRANSFORMABLE if it can
* also gather the images, and INPUT-ONLY if it can be read but not built into.  The
* STRATEGY is the strongest realization its iterability grants; the STAGE is when a
* mapping may run; and PRESERVATION records that mapping keeps arrangement and size
* (order, both capacity bounds) but may break sortedness and uniqueness - the exact
* complement of selection.
*
*   MECHANISM.  The traits are compile-time facts; each check compares a constant
* reading against its expected result.  A failing check prints the offending
* expression and drives the function's bool result to false, which the runner turns
* into a non-zero exit code.
*
*   HELPER TYPES.  Custom fixtures pin the branches the standard containers do not
* reach on their own:
*     - DNativeBox     exposes a transform(map) member  -> native strategy
*     - DReadOnlySeq  iterable + typed, cannot build    -> input-only, external
*     - Box<E>         a clean element-parametric ctor    -> cleanly rebindable,
*                      transformable (bidirectional strategy)
*     - DPairBox<A,B>  a two-type-parameter template      -> rebind rest-preservation
*   The standard containers supply the rest: std::vector/std::string (indexed),
* std::deque/std::list/std::set/std::map (bidirectional), std::unordered_set
* (forward), std::forward_list/std::array (input-only source, external), and a
* non-container (int, unsupported).  std::array also anchors compile-time STAGE
* (a literal type), where the growable containers are runtime.
*
*
* path:      /tests/djinterp/cpp/container/traits/container_transform_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.17
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRANSFORM_TRAITS_TESTS_
#define DJINTERP_CONTAINER_TRANSFORM_TRAITS_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>
// djinterp
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/cpp/container/traits)
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/container/traits/container_transform_traits.hpp"
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


// D_TX_CHECK
//   macro: the per-suite runtime check (unique letters: TX = TransXform).  Records
// a failure into the enclosing function's `_tx_ok` accumulator and prints the
// offending expression.  Always pass the condition already wrapped in parentheses
// so interior template commas (map<int,int>, array<int,N>, DPairBox<A,B>) survive
// macro argument splitting.
#define D_TX_CHECK(_cond)                                                     \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _tx_ok = false;                                                   \
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
// Shared helpers
// ===========================================================================

// tx_cstr_eq
//   helper: constexpr C-string equality, so the strategy/stage name spellings can
// be checked directly (std::strcmp is not constexpr before C++23).
constexpr
bool tx_cstr_eq(
    const char* _a,
    const char* _b
) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : tx_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Helper types
// ===========================================================================

// DNativeBox: exposes its own transform(map) primitive (and can build) -> native
struct DNativeBox
{
    using value_type = int;
    int* begin();
    int* end();
    void push_back(
        int
    );
    DNativeBox transform(
        int (*)(const int&)
    ) const;
};

// DReadOnlySeq: iterable and typed, but cannot grow -> transform source, input-only
struct DReadOnlySeq
{
    using value_type = int;
    const int* begin() const;
    const int* end() const;
};

// Box<E>: a clean element-parametric constructor (type-pack, no allocator) that can
// be read and built -> transformable AND cleanly rebindable
template<typename _Elem>
struct Box
{
    using value_type = _Elem;
    _Elem* begin();
    _Elem* end();
    void push_back(
        _Elem
    );
};

// DPairBox<A,B>: a two-type-parameter template (not a container), used to check that
// rebind_element replaces only the FIRST argument and preserves the rest
template<typename _First, typename _Second>
struct DPairBox
{};
#endif  // !DTEST_SPEC_MODE  (fixtures)



// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.   Capability detection --
bool tests_is_transform_source();
bool tests_is_container_transformable();
bool tests_is_transform_input_only();
bool tests_has_native_transform();

// -- II.  Strategy --
bool tests_transform_strategy_name();
bool tests_container_transform_strategy();

// -- III. Stage --
bool tests_transform_stage_name();
bool tests_transform_stage_of();

// -- IV.  Preservation --
bool tests_transform_preserves_order();
bool tests_transform_preserves_size_and_capacity();
bool tests_transform_preserves_sortedness_and_uniqueness();

// -- V.   Result type and element rebind --
bool tests_rebind_element();
bool tests_transform_result_type();

// -- VI.  Combined classification + variable-template companions --
bool tests_container_transform_class();
bool tests_transform_variable_templates();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// container_transform_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
container_transform_traits_spec()
{
    return dt::module_spec{
        "container_transform_traits",
        "whether a container may be TRANSFORMED - rewritten position by "
        "position by a map on its element...",
        {
            dt::block_spec{
                "capability",
                "Capability",
                {
                    { "tests_is_transform_source",
                      "checks (is_transform_source<std::vector<int>>::value)",
                      &tests_is_transform_source },
                    { "tests_is_container_transformable",
                      "checks "
                      "(is_container_transformable<std::vector<int>>::value)",
                      &tests_is_container_transformable },
                    { "tests_is_transform_input_only",
                      "checks "
                      "(is_transform_input_only<std::forward_list<int>>::value)",
                      &tests_is_transform_input_only },
                    { "tests_has_native_transform",
                      "checks (has_native_transform<DNativeBox>::value)",
                      &tests_has_native_transform },
                }
            },
            dt::block_spec{
                "strategy",
                "Strategy",
                {
                    { "tests_transform_strategy_name",
                      "checks "
                      "(tx_cstr_eq(transform_strategy_name(DTransformStrategy::native), "
                      "\"n",
                      &tests_transform_strategy_name },
                    { "tests_container_transform_strategy",
                      "checks (DContainerTransformStrategy<DNativeBox>::value "
                      "== DTransformStrate",
                      &tests_container_transform_strategy },
                }
            },
            dt::block_spec{
                "stage",
                "Stage",
                {
                    { "tests_transform_stage_name",
                      "checks "
                      "(tx_cstr_eq(transform_stage_name(DTransformStage::none), "
                      "\"none\"))",
                      &tests_transform_stage_name },
                    { "tests_transform_stage_of",
                      "checks (transform_stage_of<std::array<int, 4>>::value "
                      "== DTransformStage::",
                      &tests_transform_stage_of },
                }
            },
            dt::block_spec{
                "preservation",
                "Preservation",
                {
                    { "tests_transform_preserves_order",
                      "checks "
                      "(transform_preserves_order<std::vector<int>>::value)",
                      &tests_transform_preserves_order },
                    { "tests_transform_preserves_size_and_capacity",
                      "checks "
                      "(transform_preserves_size<std::vector<int>>::value)",
                      &tests_transform_preserves_size_and_capacity },
                    { "tests_transform_preserves_sortedness_and_uniqueness",
                      "checks "
                      "(!transform_preserves_sortedness<std::set<int>>::value)",
                      &tests_transform_preserves_sortedness_and_uniqueness },
                }
            },
            dt::block_spec{
                "result",
                "Result",
                {
                    { "tests_rebind_element",
                      "checks (std::is_same<rebind_element_t<Box<int>, "
                      "double>, Box<double>>::val",
                      &tests_rebind_element },
                    { "tests_transform_result_type",
                      "checks (std::is_same<transform_result_type_t<Box<int>, "
                      "double>, Box<double",
                      &tests_transform_result_type },
                }
            },
            dt::block_spec{
                "classification",
                "Classification",
                {
                    { "tests_container_transform_class",
                      "checks (C::is_transformable && C::is_source && "
                      "!C::is_input_only && !C::ha",
                      &tests_container_transform_class },
                    { "tests_transform_variable_templates",
                      "checks (is_transform_source_v<std::vector<int>> == "
                      "is_transform_source<std",
                      &tests_transform_variable_templates },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TRANSFORM_TRAITS_TESTS_
