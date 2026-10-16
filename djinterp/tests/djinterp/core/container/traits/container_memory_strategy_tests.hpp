/*******************************************************************************
* djinterp [testing]                         container_memory_strategy_tests.hpp
*
* Unit tests for container_memory_strategy.hpp.  This is a spec-mode suite (see
* djinterp_testing.md): a two-faced header whose normal face (the section TUs)
* pulls in the code under test and the fixtures, and whose spec face (the
* runner) exposes the container_memory_strategy_spec() provider.  The
* bool tests_*() declarations are visible in both.
*
* SELF-CONTAINED BY DESIGN:
*   The dependency between the two axes runs ONE WAY - the container binding
* knows the memory-strategy core, the core knows nothing of containers.  This
* suite honours that: it defines its own fixtures below rather than including
* the memory tests' shared fixture header, so nothing under tests/.../memory is
* reached from the container axis and no memory suite carries a container test.
*   The two strategy fixtures (cn_element_dynamic, cn_element_static_monotonic)
* are the minimal declarations the resolver needs as a step-1 alias target;
* they intentionally mirror the memory axis's equivalents without sharing them.
*
* THE FIXTURES:
*   DECLARATION-ONLY: member functions are declared and never defined.  Every
* structural trait probes them in an unevaluated context (decltype / declval),
* so declarations suffice - and the missing definitions are a guard rail: any
* test that accidentally odr-uses a fixture fails at link rather than reading
* garbage.
*
*
* path:      /tests/djinterp/core/container/traits/container_memory_strategy_tests.hpp
*                                        container_memory_strategy_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_CONTAINER_MEMORY_STRATEGY_TESTS_
#define DJINTERP_TESTING_CONTAINER_MEMORY_STRATEGY_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <memory>
// djinterp
// -- mode-gated includes ----------------------------------------------------
// djinterp core is always first and unconditional (NS_*, D_* qualifiers), so
// both faces have it.
#include <djinterp/djinterp.hpp>
#include <djinterp/test/test_defaults.hpp>
#ifndef DTEST_SPEC_MODE
// normal mode (the section TUs): the header under test + the fixtures.
#include <djinterp/core/container/traits/container_memory_strategy.hpp>
#endif  // !DTEST_SPEC_MODE


NS_DJINTERP
NS_TESTING

// dt names the framework entities.  These live in ::djinterp::test (opened by
// test_defaults.hpp), NOT in ::djinterp::testing, which is the namespace the
// SUITES live in and which encloses this line - aliasing dt to ::djinterp::
// testing would alias it to itself and leave dt::module_spec unresolvable.
// Declared unconditionally because the spec provider builds a dt::module_spec.
namespace dt = ::djinterp::test;


// ===========================================================================
// I.   Check macro
// ===========================================================================

// D_CN_CHECK
//   macro: evaluate the argument once; on failure print the stringized
// expression and its location and early-return false from the enclosing
// tests_* body.  Self-contained (printf only), so it is left OUTSIDE the
// fixture guard.  Variadic so a top-level comma in a trait expression
// (std::is_same<A, B>::value) passes through whole.
#define D_CN_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            std::printf("    check failed: %s\n    at: %s:%d\n",              \
                        #__VA_ARGS__, __FILE__, __LINE__);                    \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (false)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only (they name types from
                         // the header under test, absent in spec mode)


// ===========================================================================
// II.  Fixtures - strategies (alias targets)
// ===========================================================================

// cn_element_dynamic
//   struct: the canonical operational element strategy - dynamic, unstable,
// individually releasing.  Used as a step-1 memory_strategy alias target.
struct cn_element_dynamic
{
    using value_type = int;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::dynamic_storage;
    static constexpr bool         pointer_stable              = false;
    static constexpr bool         supports_individual_release = true;

    value_type*  allocate(std::size_t _n);
    void         deallocate(value_type* _p, std::size_t _n);
};

// cn_element_static_monotonic
//   struct: element-typed, static, pointer-stable, monotonic.  The second
// alias target, chosen so the precedence tests can tell the two apart by their
// declared kind.
struct cn_element_static_monotonic
{
    using value_type = double;

    static constexpr storage_kind strategy_storage_kind =
        storage_kind::static_storage;
    static constexpr bool         pointer_stable              = true;
    static constexpr bool         supports_individual_release = false;

    value_type*  allocate(std::size_t _n);
    void         deallocate(value_type* _p, std::size_t _n);
};


// ===========================================================================
// III. Fixtures - container shapes
// ===========================================================================

// cn_container_explicit
//   struct: names its strategy outright - precedence step 1.  The member alias
// deliberately shares the spelling of the `memory_strategy` CONCEPT; class
// scope shadows the namespace name, and the resolver must still find it.
struct cn_container_explicit
{
    using value_type      = int;
    using memory_strategy = cn_element_dynamic;
};

// cn_container_alloc_and_alias
//   struct: declares BOTH an explicit strategy alias and an allocator_type.
// The precedence edge case: step 1 must beat step 2, and the two resolver
// specializations must not be ambiguous.
struct cn_container_alloc_and_alias
{
    using value_type      = int;
    using allocator_type  = std::allocator<int>;
    using memory_strategy = cn_element_static_monotonic;
};

// cn_container_allocator
//   struct: an allocator_type and no alias - precedence step 2.
struct cn_container_allocator
{
    using value_type     = char;
    using allocator_type = std::allocator<char>;
};

// cn_container_extent
//   struct: a compile-time extent plus a value_type - precedence step 3.
struct cn_container_extent
{
    using value_type = double;

    static constexpr std::size_t extent = 8;
};

// cn_container_extent_no_vt
//   struct: an extent with NO value_type.  Edge case: the resolver's
// detected_or_t fallback substitutes `unsigned char`.
struct cn_container_extent_no_vt
{
    static constexpr std::size_t extent = 4;
};

// cn_container_growable
//   struct: capacity() + reserve(), nothing else.  Edge case: no strategy
// resolves (type is void, resolved is false) yet shape inference still yields
// dynamic_storage - resolution and inference are independent answers.
struct cn_container_growable
{
    using value_type = int;

    std::size_t  capacity() const;
    void         reserve(std::size_t _n);
};

// cn_container_fixed_cap
//   struct: capacity() WITHOUT reserve() - non-growable, so shape inference
// yields fixed_storage.  The one bit that separates fixed from dynamic.
struct cn_container_fixed_cap
{
    using value_type = int;

    std::size_t  capacity() const;
};

// cn_container_opaque
//   struct: no shape at all - inference yields unknown.
struct cn_container_opaque
{};


#endif  // !DTEST_SPEC_MODE  (fixtures)


// ===========================================================================
// IV.  Test declarations   (visible in BOTH faces)
// ===========================================================================

bool tests_container_shape_detection();
bool tests_container_strategy_resolution();
bool tests_container_strategy_precedence();
bool tests_container_kind_inference();
bool tests_container_strategy_concepts();


// ===========================================================================
// V.   Spec provider   (spec mode only)
// ===========================================================================
#ifdef DTEST_SPEC_MODE

// container_memory_strategy_spec
//   the suite as data: one block per section TU, one row per test, each row
// carrying the address of its declaration and a descriptor of what it verifies.
inline dt::module_spec
container_memory_strategy_spec()
{
    return dt::module_spec{
        "container_memory_strategy",
        "The container-shape to strategy binding: the shape detectors, "
        "strategy_resolver and the resolved type, the four-step "
        "precedence, kind inference, and the container concepts.",
        {
            dt::block_spec{
                "container",
                "The container-shape to strategy resolution binding.",
                {
                    { "tests_container_shape_detection",
                      "the cms_has_* container-shape detectors",
                      &tests_container_shape_detection },
                    { "tests_container_strategy_resolution",
                      "strategy_resolver and container_memory_strategy::type",
                      &tests_container_strategy_resolution },
                    { "tests_container_strategy_precedence",
                      "the four-step precedence, esp. the step-1/step-2 boundary",
                      &tests_container_strategy_precedence },
                    { "tests_container_kind_inference",
                      "cms_infer_kind and the declared-kind-wins rule",
                      &tests_container_kind_inference },
                    { "tests_container_strategy_concepts",
                      "container_uses_*_storage_v and the container concepts",
                      &tests_container_strategy_concepts },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_CONTAINER_MEMORY_STRATEGY_TESTS_
