/*******************************************************************************
* djinterp [test]                                  concepts_tests_properties.cpp
*
*   Tests for concepts.hpp section II.3: type-property concepts.
* Covers: is_const_c, is_volatile_c, is_trivial_c,
* is_trivially_copyable_c, is_standard_layout_c, pod_type,
* is_empty_c, is_polymorphic_c, is_abstract_c, is_final_c,
* is_aggregate_c.
*
*   Edge cases of particular interest:
*     - is_const_c looks at the top-level cv-qualifier on the type, so
*       `const int&` is NOT const (the reference is, the int isn't);
*       `int*` is NOT const but `int* const` is.  Same shape for volatile.
*     - pod_type is the compound trivial AND standard_layout - both
*       halves must hold.
*     - abstract implies polymorphic (an abstract class always has
*       virtuals), but the reverse doesn't (polymorphic_class is
*       polymorphic but not abstract).
*
*
* path:      /tests/djinterp/core/meta/concepts_tests_properties.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "concepts_tests.hpp"


NS_DJINTERP_TEST


void
concepts_tests_properties(
    test_handler& _test_handler,
    test_option   _options...)
{
    using fixtures::empty_class;
    using fixtures::trivial_class;
    using fixtures::nontrivial_class;
    using fixtures::polymorphic_class;
    using fixtures::abstract_class;
    using fixtures::final_class;
    using fixtures::aggregate_class;

    // -------------------------------------------------------------------
    // II.3.a  CV-qualification concepts
    // -------------------------------------------------------------------

    // is_const_c
    DT_CONCEPT    (is_const_c<const int>);
    DT_CONCEPT    (is_const_c<const volatile int>);
    DT_CONCEPT    (is_const_c<int* const>);                // top-level const
    DT_NOT_CONCEPT(is_const_c<int>);
    DT_NOT_CONCEPT(is_const_c<const int*>);                // pointer-to-const
    DT_NOT_CONCEPT(is_const_c<const int&>);                // ref-to-const
    DT_NOT_CONCEPT(is_const_c<volatile int>);              // volatile, not const

    // is_volatile_c
    DT_CONCEPT    (is_volatile_c<volatile int>);
    DT_CONCEPT    (is_volatile_c<const volatile int>);
    DT_CONCEPT    (is_volatile_c<int* volatile>);          // top-level volatile
    DT_NOT_CONCEPT(is_volatile_c<int>);
    DT_NOT_CONCEPT(is_volatile_c<volatile int*>);          // pointer-to-volatile
    DT_NOT_CONCEPT(is_volatile_c<const int>);              // const, not volatile

    // -------------------------------------------------------------------
    // II.3.b  Triviality concepts
    // -------------------------------------------------------------------

    // is_trivial_c
    DT_CONCEPT    (is_trivial_c<int>);
    DT_CONCEPT    (is_trivial_c<trivial_class>);
    DT_CONCEPT    (is_trivial_c<empty_class>);
    DT_NOT_CONCEPT(is_trivial_c<nontrivial_class>);
    DT_NOT_CONCEPT(is_trivial_c<polymorphic_class>);       // virtuals defeat triviality

    // is_trivially_copyable_c
    DT_CONCEPT    (is_trivially_copyable_c<int>);
    DT_CONCEPT    (is_trivially_copyable_c<trivial_class>);
    DT_CONCEPT    (is_trivially_copyable_c<empty_class>);
    DT_NOT_CONCEPT(is_trivially_copyable_c<nontrivial_class>);
    DT_NOT_CONCEPT(is_trivially_copyable_c<polymorphic_class>);

    // is_standard_layout_c
    DT_CONCEPT    (is_standard_layout_c<int>);
    DT_CONCEPT    (is_standard_layout_c<trivial_class>);
    DT_CONCEPT    (is_standard_layout_c<empty_class>);
    DT_CONCEPT    (is_standard_layout_c<aggregate_class>);
    DT_NOT_CONCEPT(is_standard_layout_c<polymorphic_class>);

    // pod_type -- trivial AND standard_layout
    DT_CONCEPT    (pod_type<int>);
    DT_CONCEPT    (pod_type<trivial_class>);
    DT_CONCEPT    (pod_type<empty_class>);
    DT_NOT_CONCEPT(pod_type<nontrivial_class>);            // fails triviality
    DT_NOT_CONCEPT(pod_type<polymorphic_class>);           // fails both halves

    //   structural invariant: pod_type IS the conjunction of trivial
    // and standard_layout, no more and no less
    DT_CONCEPT(pod_type<int> ==
              (is_trivial_c<int> && is_standard_layout_c<int>));
    DT_CONCEPT(pod_type<trivial_class> ==
              (is_trivial_c<trivial_class> && is_standard_layout_c<trivial_class>));
    DT_CONCEPT(pod_type<polymorphic_class> ==
              (is_trivial_c<polymorphic_class> && is_standard_layout_c<polymorphic_class>));

    // -------------------------------------------------------------------
    // II.3.c  Lifetime / structure concepts
    // -------------------------------------------------------------------

    // is_empty_c
    DT_CONCEPT    (is_empty_c<empty_class>);
    DT_NOT_CONCEPT(is_empty_c<trivial_class>);             // has data members
    DT_NOT_CONCEPT(is_empty_c<polymorphic_class>);         // has vptr
    DT_NOT_CONCEPT(is_empty_c<int>);

    // is_polymorphic_c
    DT_CONCEPT    (is_polymorphic_c<polymorphic_class>);
    DT_CONCEPT    (is_polymorphic_c<abstract_class>);      // abstract IS polymorphic
    DT_NOT_CONCEPT(is_polymorphic_c<empty_class>);
    DT_NOT_CONCEPT(is_polymorphic_c<trivial_class>);
    DT_NOT_CONCEPT(is_polymorphic_c<int>);

    // is_abstract_c
    DT_CONCEPT    (is_abstract_c<abstract_class>);
    DT_NOT_CONCEPT(is_abstract_c<polymorphic_class>);      // virtuals but not pure
    DT_NOT_CONCEPT(is_abstract_c<empty_class>);
    DT_NOT_CONCEPT(is_abstract_c<int>);

    //   structural invariant: abstract implies polymorphic
    DT_CONCEPT(!is_abstract_c<abstract_class> || is_polymorphic_c<abstract_class>);

    // is_final_c
    DT_CONCEPT    (is_final_c<final_class>);
    DT_NOT_CONCEPT(is_final_c<empty_class>);
    DT_NOT_CONCEPT(is_final_c<int>);

    // is_aggregate_c
    DT_CONCEPT    (is_aggregate_c<aggregate_class>);
    DT_CONCEPT    (is_aggregate_c<trivial_class>);         // also an aggregate
    DT_CONCEPT    (is_aggregate_c<empty_class>);           // empty is aggregate
    DT_CONCEPT    (is_aggregate_c<int[5]>);                // arrays are aggregates
    DT_NOT_CONCEPT(is_aggregate_c<polymorphic_class>);     // virtuals defeat aggregate
    DT_NOT_CONCEPT(is_aggregate_c<int>);                   // scalars are not aggregates

    return;
}


NS_END  // test
NS_END  // djinterp
