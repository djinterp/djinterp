/*******************************************************************************
* djinterp [test]                                   concepts_tests_composite.cpp
*
*   Tests for concepts.hpp section II.2: composite type concepts.
* Covers: is_arithmetic_c, is_fundamental_c, is_scalar_c, is_object_c,
* is_compound_c.
*
*   These are the high-level "category" concepts of the type system,
* defined as inclusion relationships rather than independent properties.
* We exercise representative members of each category AND verify the
* inclusion structure:
*     arithmetic   -> fundamental, scalar, object  (positive)
*     scalar       -> object                       (positive)
*     fundamental  -> NOT compound                 (disjoint)
*     compound     -> NOT fundamental              (disjoint)
*
*
* path:      /tests/djinterp/core/meta/concepts_tests_composite.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "concepts_tests.hpp"


NS_DJINTERP_TEST


void
concepts_tests_composite(
    test_handler& _test_handler,
    test_option   _options...)
{
    using fixtures::trivial_class;
    using fixtures::unscoped_enum_t;

    // -------------------------------------------------------------------
    // is_arithmetic_c -- integers + floating point + (cv-qualified, ref-stripped)
    // -------------------------------------------------------------------

    DT_CONCEPT    (is_arithmetic_c<int>);
    DT_CONCEPT    (is_arithmetic_c<unsigned long>);
    DT_CONCEPT    (is_arithmetic_c<float>);
    DT_CONCEPT    (is_arithmetic_c<double>);
    DT_CONCEPT    (is_arithmetic_c<char>);
    DT_CONCEPT    (is_arithmetic_c<bool>);
    DT_CONCEPT    (is_arithmetic_c<const int>);          // cv preserved
    DT_NOT_CONCEPT(is_arithmetic_c<void>);
    DT_NOT_CONCEPT(is_arithmetic_c<int*>);
    DT_NOT_CONCEPT(is_arithmetic_c<unscoped_enum_t>);    // enum is not arithmetic
    DT_NOT_CONCEPT(is_arithmetic_c<trivial_class>);

    // -------------------------------------------------------------------
    // is_fundamental_c -- arithmetic + void + nullptr_t
    // -------------------------------------------------------------------

    DT_CONCEPT    (is_fundamental_c<int>);
    DT_CONCEPT    (is_fundamental_c<double>);
    DT_CONCEPT    (is_fundamental_c<void>);
    DT_CONCEPT    (is_fundamental_c<std::nullptr_t>);
    DT_CONCEPT    (is_fundamental_c<const void>);
    DT_NOT_CONCEPT(is_fundamental_c<int*>);              // pointer is compound
    DT_NOT_CONCEPT(is_fundamental_c<trivial_class>);
    DT_NOT_CONCEPT(is_fundamental_c<unscoped_enum_t>);

    // -------------------------------------------------------------------
    // is_scalar_c -- arithmetic + enum + pointer + member-pointer + nullptr_t
    // -------------------------------------------------------------------

    DT_CONCEPT    (is_scalar_c<int>);
    DT_CONCEPT    (is_scalar_c<int*>);
    DT_CONCEPT    (is_scalar_c<unscoped_enum_t>);
    DT_CONCEPT    (is_scalar_c<std::nullptr_t>);
    DT_NOT_CONCEPT(is_scalar_c<void>);                   // void is NOT scalar
    DT_NOT_CONCEPT(is_scalar_c<trivial_class>);
    DT_NOT_CONCEPT(is_scalar_c<int[3]>);                 // array is NOT scalar
    DT_NOT_CONCEPT(is_scalar_c<int&>);                   // reference is NOT scalar

    // -------------------------------------------------------------------
    // is_object_c -- scalar + array + union + class
    // -------------------------------------------------------------------

    DT_CONCEPT    (is_object_c<int>);
    DT_CONCEPT    (is_object_c<int[3]>);
    DT_CONCEPT    (is_object_c<trivial_class>);
    DT_NOT_CONCEPT(is_object_c<void>);                   // void is NOT object
    DT_NOT_CONCEPT(is_object_c<int&>);                   // reference is NOT object
    DT_NOT_CONCEPT(is_object_c<int()>);                  // function is NOT object

    // -------------------------------------------------------------------
    // is_compound_c -- everything except fundamental
    // -------------------------------------------------------------------

    DT_CONCEPT    (is_compound_c<int*>);
    DT_CONCEPT    (is_compound_c<int[3]>);
    DT_CONCEPT    (is_compound_c<int&>);
    DT_CONCEPT    (is_compound_c<trivial_class>);
    DT_CONCEPT    (is_compound_c<unscoped_enum_t>);
    DT_CONCEPT    (is_compound_c<int()>);                // function is compound
    DT_NOT_CONCEPT(is_compound_c<int>);                  // fundamental
    DT_NOT_CONCEPT(is_compound_c<void>);                 // fundamental
    DT_NOT_CONCEPT(is_compound_c<std::nullptr_t>);       // fundamental

    // -------------------------------------------------------------------
    // Inclusion / partition invariants
    // -------------------------------------------------------------------
    //   These cross-checks verify that the composite concepts wire into
    // the categories the standard says they do.  If anyone ever swaps
    // a concept's definition out from under us, one of these will
    // catch it.

    //   arithmetic implies fundamental, scalar, AND object
    DT_CONCEPT(!is_arithmetic_c<int>  || is_fundamental_c<int>);
    DT_CONCEPT(!is_arithmetic_c<int>  || is_scalar_c<int>);
    DT_CONCEPT(!is_arithmetic_c<int>  || is_object_c<int>);

    //   scalar implies object
    DT_CONCEPT(!is_scalar_c<int*>     || is_object_c<int*>);

    //   fundamental and compound are disjoint
    DT_NOT_CONCEPT(is_fundamental_c<int>     && is_compound_c<int>);
    DT_NOT_CONCEPT(is_fundamental_c<int*>    && is_compound_c<int*>);

    //   fundamental OR compound covers everything except... actually,
    // every type is exactly one of fundamental or compound
    DT_CONCEPT(is_fundamental_c<int>   || is_compound_c<int>);
    DT_CONCEPT(is_fundamental_c<void>  || is_compound_c<void>);
    DT_CONCEPT(is_fundamental_c<int*>  || is_compound_c<int*>);
    DT_CONCEPT(is_fundamental_c<trivial_class> || is_compound_c<trivial_class>);

    return;
}


NS_END  // test
NS_END  // djinterp
