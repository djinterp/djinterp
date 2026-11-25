/*******************************************************************************
* djinterp [cpp][meta]                                 math_vocabulary_tests.hpp
*
* Unit test suite for math_vocabulary.hpp: the enums and small types that
* describe a property of a set, a relation or a function, moved from graph's
* math_common.hpp, in djinterp::math, into meta, in djinterp, on 2026.11.25.
*   Two faces, selected by DTEST_SPEC_MODE:
*     - normal mode (the section TUs): the header under test and the
*       D_MV_CHECK macro;
*     - spec mode (the runner): test_defaults.hpp and math_vocabulary_spec().
*
*   WHAT IS UNDER TEST.
*     1. The values: each enum's unknown member is 0 and its default; each
*        underlying type is the one documented; the order strength is a
*        ladder; the sentinels and the compile-time pairs.
*     2. The traits: each over every member of its enum, and the relation
*        properties' bit operations.
*     3. The names: every name that left djinterp::math is still there,
*        until 0.9.0-rc.1, as the same entity; math's ordering enum is
*        DOrderStrength, a different type from container's DOrderingKind,
*        both in one unit; graph's multiplicity enum is in djinterp::math
*        alone.
*
* CONTENTS
*   I.    VALUES   (math_vocabulary_tests_values.cpp)
*   II.   TRAITS   (math_vocabulary_tests_traits.cpp)
*   III.  NAMES    (math_vocabulary_tests_names.cpp)
*
*
* path:      /tests/djinterp/cpp/meta/math_vocabulary_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#ifndef DJINTERP_TESTS_MATH_VOCABULARY_TESTS_
#define DJINTERP_TESTS_MATH_VOCABULARY_TESTS_ 1

// std
#include <cstddef>      // std::size_t
#include <cstdio>       // std::printf
#include <type_traits>  // std::is_same, std::underlying_type
// djinterp
#include <djinterp/cpp/djinterp.hpp>  // framework root
#ifndef DTEST_SPEC_MODE
#include <djinterp/cpp/meta/math_vocabulary.hpp>  // under test
#include <djinterp/cpp/container/traits/ordered_container_traits.hpp>  // DOrderingKind
#endif
#ifdef DTEST_SPEC_MODE
#include <djinterp/cpp/modules/test/test_defaults.hpp>  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


// math_vocabulary_check
//   function: routes one D_MV_CHECK evaluation: prints the failing expression
// and its place, and hands the condition back.
inline
bool math_vocabulary_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}

// D_MV_CHECK
//   macro: evaluate a checked expression once; on failure, report it and
// return false from the enclosing tests_* body. Variadic, so a top-level
// comma inside a trait expression passes through whole.
#define D_MV_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!::djinterp::testing::math_vocabulary_check(                      \
                 (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__))            \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (0)


// I.   VALUES   (math_vocabulary_tests_values.cpp)
bool tests_values_unknown_is_zero_and_default();
bool tests_values_underlying_types();
bool tests_values_order_strength_is_a_ladder();
bool tests_values_sentinels_and_sizes();
bool tests_values_compile_time_pairs();

// II.  TRAITS   (math_vocabulary_tests_traits.cpp)
bool tests_traits_cardinality();
bool tests_traits_closure();
bool tests_traits_algebraic_structure();
bool tests_traits_relation_property_bits();

// III. NAMES    (math_vocabulary_tests_names.cpp)
bool tests_names_old_types_are_the_new();
bool tests_names_old_templates_are_the_new();
bool tests_names_old_constants_and_function();
bool tests_names_order_strength_beside_ordering_kind();
bool tests_names_multiplicity_stays_in_math();


#ifdef DTEST_SPEC_MODE

// math_vocabulary_spec
//   function: the suite's description: one block per section, one row per
// tests_* body.
inline
dt::module_spec math_vocabulary_spec()
{
    return dt::module_spec{
        "math_vocabulary",
        "The math vocabulary in djinterp: its enums' values and underlying "
        "types, its traits over every member, the relation properties' bit "
        "operations, and the old names in djinterp::math, each the same "
        "entity as its new name until 0.9.0-rc.1.",
        {
            dt::block_spec{
                "values",
                "Each enum's unknown member, its underlying type, the order "
                "strength's ladder, the sentinels and the compile-time pairs.",
                {
                    { "tests_values_unknown_is_zero_and_default",
                      "Every enum's unknown (or none) member is 0, and a "
                      "value-initialized enum equals it.",
                      &tests_values_unknown_is_zero_and_default },
                    { "tests_values_underlying_types",
                      "Each enum's underlying type is the documented one: "
                      "uint8_t, but int8_t for DSign and uint16_t for "
                      "DRelationProperty.",
                      &tests_values_underlying_types },
                    { "tests_values_order_strength_is_a_ladder",
                      "DOrderStrength's members rise from unordered to "
                      "well_ordered, so a stronger order compares greater.",
                      &tests_values_order_strength_is_a_ladder },
                    { "tests_values_sentinels_and_sizes",
                      "arity_t and degree_t are std::size_t, and "
                      "kVariadicArity, kInvalidSize and kInvalidIndex are "
                      "its largest value.",
                      &tests_values_sentinels_and_sizes },
                    { "tests_values_compile_time_pairs",
                      "DTypePair, DTypeTriple, DValuePair and DIndexedValue "
                      "carry their arguments as members.",
                      &tests_values_compile_time_pairs },
                }
            },
            dt::block_spec{
                "traits",
                "The traits over every member of their enums, and the "
                "relation properties' bit operations.",
                {
                    { "tests_traits_cardinality",
                      "is_finite_cardinality and is_countable_cardinality "
                      "over all six cardinalities.",
                      &tests_traits_cardinality },
                    { "tests_traits_closure",
                      "closure_includes_left and closure_includes_right over "
                      "all five closures.",
                      &tests_traits_closure },
                    { "tests_traits_algebraic_structure",
                      "has_identity_element, has_inverses and "
                      "is_commutative_structure over all eleven structures.",
                      &tests_traits_algebraic_structure },
                    { "tests_traits_relation_property_bits",
                      "|, &, ^ and ~ on DRelationProperty, and "
                      "has_relation_property, which asks for every bit of "
                      "its query.",
                      &tests_traits_relation_property_bits },
                }
            },
            dt::block_spec{
                "names",
                "The old names in djinterp::math, and the two ordering "
                "enums in one unit.",
                {
                    { "tests_names_old_types_are_the_new",
                      "Each type's old names in djinterp::math, its snake "
                      "name and its D name, are the type in djinterp.",
                      &tests_names_old_types_are_the_new },
                    { "tests_names_old_templates_are_the_new",
                      "Each template's old name in djinterp::math gives the "
                      "same type, and each trait the same value.",
                      &tests_names_old_templates_are_the_new },
                    { "tests_names_old_constants_and_function",
                      "math's constants and has_relation_property are the "
                      "same objects and function as djinterp's.",
                      &tests_names_old_constants_and_function },
                    { "tests_names_order_strength_beside_ordering_kind",
                      "math::ordering_kind is DOrderStrength, a different "
                      "type from container's DOrderingKind, and both are "
                      "usable in one unit.",
                      &tests_names_order_strength_beside_ordering_kind },
                    { "tests_names_multiplicity_stays_in_math",
                      "graph's multiplicity enum is in djinterp::math, "
                      "under its two old names, with its four members.",
                      &tests_names_multiplicity_stays_in_math },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_MATH_VOCABULARY_TESTS_
