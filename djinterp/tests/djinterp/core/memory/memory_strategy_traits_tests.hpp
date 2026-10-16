/*******************************************************************************
* djinterp [testing]                            memory_strategy_traits_tests.hpp
*
* Unit tests for memory_strategy_traits.hpp.  This is a spec-mode suite (see djinterp_testing.md):
* a two-faced header whose normal face (the section TUs) pulls in the code under
* test and the shared fixtures, and whose spec face (the runner) exposes the
* memory_strategy_traits_spec() provider.  The bool tests_*() declarations are visible in both.
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_MEMORY_STRATEGY_TRAITS_TESTS_
#define DJINTERP_TESTING_MEMORY_STRATEGY_TRAITS_TESTS_ 1

// std
#include <cstdio>
// djinterp
#include <djinterp/djinterp.hpp>
#include <djinterp/test/test_defaults.hpp>
#ifndef DTEST_SPEC_MODE
#include "./memory_strategy_test_fixtures.hpp"
#endif  // !DTEST_SPEC_MODE




NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


// D_MT_CHECK: evaluate once; on failure print and early-return false.
#define D_MT_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            std::printf("    check failed: %s\n    at: %s:%d\n",             \
                        #__VA_ARGS__, __FILE__, __LINE__);                    \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (false)


// -- test declarations (both faces) -----------------------------------------
// vocabulary
bool tests_storage_kind_vocabulary();
bool tests_strategy_typing_vocabulary();

// detection
bool tests_constant_detection();
bool tests_value_type_detection();
bool tests_element_surface_detection();
bool tests_byte_surface_detection();

// typing
bool tests_element_typing_classification();
bool tests_byte_typing_classification();
bool tests_typing_precedence();

// classification
bool tests_membership_classification();
bool tests_allocating_classification();
bool tests_descriptive_only_classification();

// discipline
bool tests_storage_discipline_kind();
bool tests_storage_discipline_predicates();

// refinements
bool tests_pointer_stability();
bool tests_individual_release();
bool tests_monotonic_refinement();
bool tests_generational_refinement();

// bridges
bool tests_element_strategy_view_contract();
bool tests_element_strategy_view_runtime();
bool tests_byte_strategy_view_contract();
bool tests_byte_strategy_view_runtime();

// accessor
bool tests_memory_strategy_traits_accessor();
bool tests_memory_strategy_class_snapshot();
bool tests_cv_ref_agnosticism();


// -- spec provider (spec mode) ----------------------------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
memory_strategy_traits_spec()
{
    return dt::module_spec{
        "memory_strategy_traits",
        "The agnostic memory-strategy core: the storage and typing "
        "vocabularies, the structural surface detectors, typing and "
        "membership classification, storage discipline and the "
        "stability/release refinements, the element<->byte view bridges, "
        "and the uniform accessor and aggregate snapshot.",
        {
            dt::block_spec{
                "vocabulary",
                "The storage_kind and strategy_typing scoped enumerations.",
                {
                    { "tests_storage_kind_vocabulary",
                      "storage_kind is a scoped enum of four distinct "
                      "enumerators that round-trip through a declared constant",
                      &tests_storage_kind_vocabulary },
                    { "tests_strategy_typing_vocabulary",
                      "strategy_typing is a scoped enum naming which typed "
                      "surface a strategy speaks",
                      &tests_strategy_typing_vocabulary },
                }
            },
            dt::block_spec{
                "detection",
                "The by-name (and, for the kind, by-type) surface detectors.",
                {
                    { "tests_constant_detection",
                      "each descriptive-constant detector fires independently, "
                      "and the kind detector rejects a wrong-typed constant",
                      &tests_constant_detection },
                    { "tests_value_type_detection",
                      "the nested value_type alias is detected, void included",
                      &tests_value_type_detection },
                    { "tests_element_surface_detection",
                      "the element surface allocate(n)/deallocate(p,n) is "
                      "detected",
                      &tests_element_surface_detection },
                    { "tests_byte_surface_detection",
                      "the byte surface allocate(bytes,align)/deallocate is "
                      "detected",
                      &tests_byte_surface_detection },
                }
            },
            dt::block_spec{
                "typing",
                "Which typed surface a strategy is classified as speaking.",
                {
                    { "tests_element_typing_classification",
                      "is_element_strategy requires the complete element surface",
                      &tests_element_typing_classification },
                    { "tests_byte_typing_classification",
                      "is_byte_strategy requires the complete byte surface",
                      &tests_byte_typing_classification },
                    { "tests_typing_precedence",
                      "strategy_typing_of resolves the surface, element winning",
                      &tests_typing_precedence },
                }
            },
            dt::block_spec{
                "classification",
                "Membership and the allocating / descriptive-only split.",
                {
                    { "tests_membership_classification",
                      "is_memory_strategy is exactly a valid storage_kind "
                      "constant; a wrong-typed constant is rejected",
                      &tests_membership_classification },
                    { "tests_allocating_classification",
                      "is_allocating_strategy requires membership and a surface",
                      &tests_allocating_classification },
                    { "tests_descriptive_only_classification",
                      "is_descriptive_only_strategy is a member that does not "
                      "allocate",
                      &tests_descriptive_only_classification },
                }
            },
            dt::block_spec{
                "discipline",
                "The declared storage discipline read back.",
                {
                    { "tests_storage_discipline_kind",
                      "memory_strategy_kind_of reads the declared kind and is "
                      "total (unknown) on non-members and wrong-typed constants",
                      &tests_storage_discipline_kind },
                    { "tests_storage_discipline_predicates",
                      "is_static/fixed/dynamic_strategy agree with the kind",
                      &tests_storage_discipline_predicates },
                }
            },
            dt::block_spec{
                "refinements",
                "The stability, release, and generational refinements.",
                {
                    { "tests_pointer_stability",
                      "is_pointer_stable_strategy reads the pointer_stable "
                      "constant",
                      &tests_pointer_stability },
                    { "tests_individual_release",
                      "supports_individual_release_strategy reads the release "
                      "constant",
                      &tests_individual_release },
                    { "tests_monotonic_refinement",
                      "is_monotonic_strategy is allocating and not individually "
                      "releasing",
                      &tests_monotonic_refinement },
                    { "tests_generational_refinement",
                      "is_generational_strategy reads the generational-sweep "
                      "constant",
                      &tests_generational_refinement },
                }
            },
            dt::block_spec{
                "bridges",
                "The element <-> byte view adapters.",
                {
                    { "tests_element_strategy_view_contract",
                      "element_strategy_view presents an element surface and "
                      "forwards all four descriptive constants",
                      &tests_element_strategy_view_contract },
                    { "tests_element_strategy_view_runtime",
                      "an allocation round-trips through element_strategy_view",
                      &tests_element_strategy_view_runtime },
                    { "tests_byte_strategy_view_contract",
                      "byte_strategy_view presents a byte surface and forwards "
                      "all four constants (symmetric with the element view)",
                      &tests_byte_strategy_view_contract },
                    { "tests_byte_strategy_view_runtime",
                      "an allocation round-trips through byte_strategy_view",
                      &tests_byte_strategy_view_runtime },
                }
            },
            dt::block_spec{
                "accessor",
                "The uniform accessor and the aggregate class snapshot.",
                {
                    { "tests_memory_strategy_traits_accessor",
                      "memory_strategy_traits exposes the surface uniformly and "
                      "is a customization point",
                      &tests_memory_strategy_traits_accessor },
                    { "tests_memory_strategy_class_snapshot",
                      "memory_strategy_class aggregates the whole classification",
                      &tests_memory_strategy_class_snapshot },
                    { "tests_cv_ref_agnosticism",
                      "composites are cv/ref agnostic; raw detectors are not",
                      &tests_cv_ref_agnosticism },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_MEMORY_STRATEGY_TRAITS_TESTS_
