/*******************************************************************************
* djinterp [test]                                               monoid_tests.hpp
*
*   Unit-test declarations for core/functional/monoid.hpp.  One declaration
* group per section of the module under test (newtypes, protocol, instances,
* generic operations); every test is a niladic bool predicate returning true
* on pass, so the same functions are the unit tests
* the spec provider hands to run_module.  Definitions live in the per-section .cpp files:
*
*     monoid_tests_newtypes.cpp     -- I.   MONOID NEWTYPES
*     monoid_tests_protocol.cpp     -- II.  MONOID PROTOCOL
*     monoid_tests_instances.cpp    -- III. INSTANCES (semigroup + monoid)
*     monoid_tests_operations.cpp   -- IV.  GENERIC MONOID OPERATIONS
*
*   All tests are flat in djinterp::testing.
*
*
* path:      /tests/djinterp/cpp/functional/monoid_tests.hpp
* link(s):   TBA
* author(s): teer                                            created: 2026.07.10
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    TEST HELPERS
      ------------

I.    MONOID NEWTYPES
      ---------------

II.   MONOID PROTOCOL
      ---------------

III.  INSTANCES  (semigroup + monoid)
      -------------------------------

IV.   GENERIC MONOID OPERATIONS
      -------------------------
*/


#ifndef DJINTERP_TEST_MONOID_TESTS_
#define DJINTERP_TEST_MONOID_TESTS_ 1

// std
#include <cmath>
// djinterp
// djinterp (module under test; pulls semigroup.hpp + foldable.hpp + core)
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/monoid.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


///////////////////////////////////////////////////////////////////////////////
///             0.    TEST HELPERS                                          ///
///////////////////////////////////////////////////////////////////////////////

// DNotAMonoid
//   type: a plain struct with no semigroup_traits / monoid_traits
// specialization, used as the negative case in the detection-trait tests.
struct DNotAMonoid
{
};

// close_enough
//   function: tolerant floating-point equality, so the sum<double> /
// product<double> checks do not hinge on exact bit patterns.
//
// Parameter(s):
//   _a:       the first value.
//   _b:       the second value.
//   _epsilon: the maximum tolerated absolute difference.
// Return:
//   true iff |_a - _b| <= _epsilon.
inline
bool close_enough(
    double _a,
    double _b,
    double _epsilon = 1e-9
)
{
    return (std::fabs(_a - _b) <= _epsilon);
}
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///             I.    MONOID NEWTYPES                                       ///
///////////////////////////////////////////////////////////////////////////////

// newtype construction, defaults, explicitness, and constexpr behaviour
// (namespace monoids: sum / product / all / any / min / max)
bool tests_sum_newtype();
bool tests_product_newtype();
bool tests_all_newtype();
bool tests_any_newtype();
bool tests_min_newtype();
bool tests_max_newtype();
bool tests_newtype_default_constructibility();
bool tests_newtype_constexpr_construction();


///////////////////////////////////////////////////////////////////////////////
///             II.   MONOID PROTOCOL                                       ///
///////////////////////////////////////////////////////////////////////////////

// monoid_traits detection: is_monoid / is_monoid_v / Monoid, and the
// monoid-implies-semigroup invariant
bool tests_is_monoid_positive();
bool tests_is_monoid_negative();
bool tests_is_monoid_decay();
bool tests_is_monoid_v();
bool tests_monoid_concept();
bool tests_monoid_implies_semigroup();


///////////////////////////////////////////////////////////////////////////////
///             III.  INSTANCES  (semigroup + monoid)                       ///
///////////////////////////////////////////////////////////////////////////////

// per-instance combine, identity, and the monoid laws (left/right identity,
// associativity), plus the is_specialized markers
bool tests_string_instance();
bool tests_vector_instance();
bool tests_sum_instance();
bool tests_product_instance();
bool tests_all_instance();
bool tests_any_instance();
bool tests_min_instance();
bool tests_max_instance();
bool tests_instance_is_specialized();


///////////////////////////////////////////////////////////////////////////////
///             IV.   GENERIC MONOID OPERATIONS                             ///
///////////////////////////////////////////////////////////////////////////////

// mempty / mconcat / fold_monoid, including the empty-foldable identity path
// and monoid-type deduction
bool tests_mempty();
bool tests_mconcat_multi();
bool tests_mconcat_single();
bool tests_mconcat_empty();
bool tests_mconcat_nested_vector();
bool tests_mconcat_return_type();
bool tests_fold_monoid_multi();
bool tests_fold_monoid_empty();
bool tests_fold_monoid_type_deduction();
bool tests_fold_monoid_string_concat();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// monoid_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
monoid_spec()
{
    return dt::module_spec{
        "monoid",
        "sum / product / all / any / min / max: construction, defaults, "
        "explicitness, constexpr",
        {
            dt::block_spec{
                "newtypes",
                "MONOID NEWTYPES",
                {
                    { "tests_sum_newtype",
                      "Exercises the additive newtype sum<T>",
                      &tests_sum_newtype },
                    { "tests_product_newtype",
                      "Exercises the multiplicative newtype product<T>",
                      &tests_product_newtype },
                    { "tests_all_newtype",
                      "Exercises the conjunctive newtype all",
                      &tests_all_newtype },
                    { "tests_any_newtype",
                      "Exercises the disjunctive newtype any",
                      &tests_any_newtype },
                    { "tests_min_newtype",
                      "Exercises the minimum newtype min<T>",
                      &tests_min_newtype },
                    { "tests_max_newtype",
                      "Exercises the maximum newtype max<T>",
                      &tests_max_newtype },
                    { "tests_newtype_default_constructibility",
                      "Contrasts the default-constructibility of the six "
                      "newtypes",
                      &tests_newtype_default_constructibility },
                    { "tests_newtype_constexpr_construction",
                      "Confirms every newtype is constructible in a constant "
                      "expression, with the correct identity default where "
                      "one exists",
                      &tests_newtype_constexpr_construction },
                }
            },
            dt::block_spec{
                "protocol",
                "MONOID PROTOCOL",
                {
                    { "tests_is_monoid_positive",
                      "Confirms is_monoid is true for every standard instance",
                      &tests_is_monoid_positive },
                    { "tests_is_monoid_negative",
                      "Confirms is_monoid is false for types with no "
                      "monoid_traits specialization, and -- critically -- "
                      "that the detector resolves cleanly rather than hard- "
                      "erroring on the undefined primary template",
                      &tests_is_monoid_negative },
                    { "tests_is_monoid_decay",
                      "Confirms is_monoid strips cv-qualifiers and references "
                      "before testing",
                      &tests_is_monoid_decay },
                    { "tests_is_monoid_v",
                      "Confirms the is_monoid_v variable-template shorthand "
                      "agrees with the trait (skipped, reporting pass, where "
                      "variable templates are unavailable)",
                      &tests_is_monoid_v },
                    { "tests_monoid_concept",
                      "Confirms the C++20 Monoid concept mirrors is_monoid "
                      "(skipped, reporting pass, where concepts are "
                      "unavailable)",
                      &tests_monoid_concept },
                    { "tests_monoid_implies_semigroup",
                      "Confirms the documented invariant: a monoid is "
                      "necessarily a semigroup, so is_semigroup holds for "
                      "every type is_monoid holds for",
                      &tests_monoid_implies_semigroup },
                }
            },
            dt::block_spec{
                "instances",
                "INSTANCES  (semigroup + monoid)",
                {
                    { "tests_string_instance",
                      "Exercises the std::string instance (concatenation, "
                      "identity \"\")",
                      &tests_string_instance },
                    { "tests_vector_instance",
                      "Exercises the std::vector<T> instance (concatenation, "
                      "identity {})",
                      &tests_vector_instance },
                    { "tests_sum_instance",
                      "Exercises the sum<T> instance (addition, identity 0)",
                      &tests_sum_instance },
                    { "tests_product_instance",
                      "product instance",
                      &tests_product_instance },
                    { "tests_all_instance",
                      "Exercises the all instance (logical AND, identity true)",
                      &tests_all_instance },
                    { "tests_any_instance",
                      "Exercises the any instance (logical OR, identity false)",
                      &tests_any_instance },
                    { "tests_min_instance",
                      "Exercises the min<T> instance (minimum, identity "
                      "numeric_limits::max())",
                      &tests_min_instance },
                    { "tests_max_instance",
                      "Exercises the max<T> instance (maximum, identity "
                      "numeric_limits::lowest())",
                      &tests_max_instance },
                    { "tests_instance_is_specialized",
                      "Confirms the is_specialized markers on both trait "
                      "families for every instance, and that the marker is "
                      "exactly std::true_type",
                      &tests_instance_is_specialized },
                }
            },
            dt::block_spec{
                "operations",
                "GENERIC MONOID OPERATIONS",
                {
                    { "tests_mempty",
                      "Exercises mempty<T>() for every instance",
                      &tests_mempty },
                    { "tests_mconcat_multi",
                      "Exercises mconcat over multi-element foldables of each "
                      "monoid",
                      &tests_mconcat_multi },
                    { "tests_mconcat_single",
                      "Exercises mconcat over one-element foldables",
                      &tests_mconcat_single },
                    { "tests_mconcat_empty",
                      "Exercises the empty-foldable path of mconcat (the "
                      "identity seed)",
                      &tests_mconcat_empty },
                    { "tests_mconcat_nested_vector",
                      "Exercises mconcat where the element type is itself a "
                      "monoid (vector<int>), so the fold flattens",
                      &tests_mconcat_nested_vector },
                    { "tests_mconcat_return_type",
                      "Confirms mconcat's return type is the foldable's "
                      "element (monoid) type",
                      &tests_mconcat_return_type },
                    { "tests_fold_monoid_multi",
                      "Exercises fold_monoid (map each element into a monoid, "
                      "then combine)",
                      &tests_fold_monoid_multi },
                    { "tests_fold_monoid_empty",
                      "Exercises the empty-foldable path of fold_monoid",
                      &tests_fold_monoid_empty },
                    { "tests_fold_monoid_type_deduction",
                      "Confirms the resulting monoid is deduced from the "
                      "mapping function's result",
                      &tests_fold_monoid_type_deduction },
                    { "tests_fold_monoid_string_concat",
                      "Exercises fold_monoid with a string-producing mapping "
                      "function",
                      &tests_fold_monoid_string_concat },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_MONOID_TESTS_
