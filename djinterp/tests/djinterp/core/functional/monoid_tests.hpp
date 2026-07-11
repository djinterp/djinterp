/******************************************************************************
* djinterp [test]                                             monoid_tests.hpp
*
*   Unit-test declarations for core/functional/monoid.hpp.  One declaration
* group per section of the module under test (newtypes, protocol, instances,
* generic operations); every test is a niladic bool predicate returning true
* on pass, so the same functions drive both the report_builder runner and any
* standalone harness.  Definitions live in the per-section .cpp files:
*
*     monoid_tests_newtypes.cpp     -- I.   MONOID NEWTYPES
*     monoid_tests_protocol.cpp     -- II.  MONOID PROTOCOL
*     monoid_tests_instances.cpp    -- III. INSTANCES (semigroup + monoid)
*     monoid_tests_operations.cpp   -- IV.  GENERIC MONOID OPERATIONS
*
*   All tests are flat in djinterp::testing.
*
* path:      /inc/djinterp/test/functional/monoid_tests.hpp
* link(s):   TBA
* author(s): teer                                          created: 2026.07.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    TEST HELPERS
I.    MONOID NEWTYPES
II.   MONOID PROTOCOL
III.  INSTANCES  (semigroup + monoid)
IV.   GENERIC MONOID OPERATIONS
*/


#ifndef DJINTERP_TEST_MONOID_TESTS_
#define DJINTERP_TEST_MONOID_TESTS_ 1

// std
#include <cmath>
// djinterp (module under test; pulls semigroup.hpp + foldable.hpp + core)
#include "../../core/functional/monoid.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///             0.    TEST HELPERS                                          ///
///////////////////////////////////////////////////////////////////////////////

// not_a_monoid
//   type: a plain struct with no semigroup_traits / monoid_traits
// specialization, used as the negative case in the detection-trait tests.
struct not_a_monoid
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
inline bool
close_enough(
    double _a,
    double _b,
    double _epsilon = 1e-9
)
{
    return (std::fabs(_a - _b) <= _epsilon);
}


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


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_MONOID_TESTS_
