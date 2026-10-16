/*******************************************************************************
* djinterp [test]                                 concepts_tests_fundamental.cpp
*
*   Tests for concepts.hpp section II.1: fundamental type concepts.
* Covers: is_void_c, is_null_pointer_c, nonvoid, is_array_c,
* bounded_array, unbounded_array, is_enum_c, scoped_enum, unscoped_enum,
* is_pointer_c, is_member_pointer_c, is_lvalue_reference_c,
* is_rvalue_reference_c, is_reference_c, is_class_c, is_union_c,
* is_function_c.
*
*   Edge cases exercised: cv-qualified void / nullptr_t variants
* (is_void_c is documented as accepting "cv-qualified void"); the
* `int[]` vs `int[5]` split between bounded_array and unbounded_array;
* the scoped / unscoped enum split; data-member pointers vs
* member-function pointers under is_member_pointer_c; lvalue / rvalue /
* both for the reference concepts; structs vs unions vs free functions
* for is_class_c / is_union_c / is_function_c.
*
*
* path:      /tests/djinterp/core/meta/concepts_tests_fundamental.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "concepts_tests.hpp"


namespace {
    // free_function
    //   helper: a free function used to probe is_function_c.  Lives
    // in an unnamed namespace so it has no external linkage.
    void free_function() {}
}  // unnamed namespace


NS_DJINTERP_TEST


void
concepts_tests_fundamental(
    test_handler& _test_handler,
    test_option   _options...)
{
    using fixtures::trivial_class;
    using fixtures::union_t;
    using fixtures::unscoped_enum_t;
    using fixtures::scoped_enum_t;
    using fixtures::pointee_class;

    // -------------------------------------------------------------------
    // II.1.a  Void and null concepts
    // -------------------------------------------------------------------

    // is_void_c -- documented as "(cv-qualified) void"
    DT_CONCEPT    (is_void_c<void>);
    DT_CONCEPT    (is_void_c<const void>);
    DT_CONCEPT    (is_void_c<volatile void>);
    DT_CONCEPT    (is_void_c<const volatile void>);
    DT_NOT_CONCEPT(is_void_c<int>);
    DT_NOT_CONCEPT(is_void_c<void*>);              // pointer-to-void is NOT void
    DT_NOT_CONCEPT(is_void_c<void()>);             // function returning void

    // is_null_pointer_c
    DT_CONCEPT    (is_null_pointer_c<std::nullptr_t>);
    DT_CONCEPT    (is_null_pointer_c<const std::nullptr_t>);
    DT_NOT_CONCEPT(is_null_pointer_c<void*>);
    DT_NOT_CONCEPT(is_null_pointer_c<int>);

    // nonvoid -- the inverse of is_void_c
    DT_NOT_CONCEPT(nonvoid<void>);
    DT_NOT_CONCEPT(nonvoid<const void>);
    DT_CONCEPT    (nonvoid<int>);
    DT_CONCEPT    (nonvoid<void*>);
    DT_CONCEPT    (nonvoid<trivial_class>);

    // -------------------------------------------------------------------
    // II.1.b  Array concepts
    // -------------------------------------------------------------------

    // is_array_c -- both bounded and unbounded
    DT_CONCEPT    (is_array_c<int[5]>);
    DT_CONCEPT    (is_array_c<int[]>);
    DT_CONCEPT    (is_array_c<int[3][4]>);
    DT_NOT_CONCEPT(is_array_c<int>);
    DT_NOT_CONCEPT(is_array_c<int*>);              // pointer is NOT array

    // bounded_array -- only `T[N]`
    DT_CONCEPT    (bounded_array<int[5]>);
    DT_CONCEPT    (bounded_array<char[1]>);
    DT_NOT_CONCEPT(bounded_array<int[]>);          // unbounded
    DT_NOT_CONCEPT(bounded_array<int>);
    DT_NOT_CONCEPT(bounded_array<int*>);

    // unbounded_array -- only `T[]`
    DT_CONCEPT    (unbounded_array<int[]>);
    DT_NOT_CONCEPT(unbounded_array<int[5]>);       // bounded
    DT_NOT_CONCEPT(unbounded_array<int>);
    DT_NOT_CONCEPT(unbounded_array<int*>);

    //   structural invariant: bounded_array and unbounded_array partition
    // is_array_c
    DT_CONCEPT(bounded_array<int[5]>     || unbounded_array<int[5]>);
    DT_CONCEPT(bounded_array<int[]>      || unbounded_array<int[]>);
    DT_NOT_CONCEPT(bounded_array<int[5]> && unbounded_array<int[5]>);

    // -------------------------------------------------------------------
    // II.1.c  Enum concepts
    // -------------------------------------------------------------------

    // is_enum_c -- both scoped and unscoped
    DT_CONCEPT    (is_enum_c<unscoped_enum_t>);
    DT_CONCEPT    (is_enum_c<scoped_enum_t>);
    DT_NOT_CONCEPT(is_enum_c<int>);
    DT_NOT_CONCEPT(is_enum_c<trivial_class>);

    // scoped_enum -- only enum class / enum struct
    DT_CONCEPT    (scoped_enum<scoped_enum_t>);
    DT_NOT_CONCEPT(scoped_enum<unscoped_enum_t>);
    DT_NOT_CONCEPT(scoped_enum<int>);

    // unscoped_enum -- only the C-style ones
    DT_CONCEPT    (unscoped_enum<unscoped_enum_t>);
    DT_NOT_CONCEPT(unscoped_enum<scoped_enum_t>);
    DT_NOT_CONCEPT(unscoped_enum<int>);

    //   structural invariant: scoped_enum and unscoped_enum partition
    // is_enum_c
    DT_CONCEPT(scoped_enum<scoped_enum_t>       || unscoped_enum<scoped_enum_t>);
    DT_CONCEPT(scoped_enum<unscoped_enum_t>     || unscoped_enum<unscoped_enum_t>);
    DT_NOT_CONCEPT(scoped_enum<scoped_enum_t>   && unscoped_enum<scoped_enum_t>);

    // -------------------------------------------------------------------
    // II.1.d  Pointer and reference concepts
    // -------------------------------------------------------------------

    // is_pointer_c
    DT_CONCEPT    (is_pointer_c<int*>);
    DT_CONCEPT    (is_pointer_c<const int*>);
    DT_CONCEPT    (is_pointer_c<void*>);
    DT_CONCEPT    (is_pointer_c<int**>);
    DT_NOT_CONCEPT(is_pointer_c<int>);
    DT_NOT_CONCEPT(is_pointer_c<int&>);                       // reference, not pointer
    DT_NOT_CONCEPT(is_pointer_c<int pointee_class::*>);       // member pointer

    // is_member_pointer_c -- both data and function member pointers
    DT_CONCEPT    (is_member_pointer_c<int pointee_class::*>);
    DT_CONCEPT    (is_member_pointer_c<void (pointee_class::*)()>);
    DT_NOT_CONCEPT(is_member_pointer_c<int*>);                // plain pointer
    DT_NOT_CONCEPT(is_member_pointer_c<int>);

    // is_lvalue_reference_c
    DT_CONCEPT    (is_lvalue_reference_c<int&>);
    DT_CONCEPT    (is_lvalue_reference_c<const int&>);
    DT_NOT_CONCEPT(is_lvalue_reference_c<int&&>);
    DT_NOT_CONCEPT(is_lvalue_reference_c<int>);

    // is_rvalue_reference_c
    DT_CONCEPT    (is_rvalue_reference_c<int&&>);
    DT_NOT_CONCEPT(is_rvalue_reference_c<int&>);
    DT_NOT_CONCEPT(is_rvalue_reference_c<int>);

    // is_reference_c -- accepts BOTH lvalue and rvalue
    DT_CONCEPT    (is_reference_c<int&>);
    DT_CONCEPT    (is_reference_c<int&&>);
    DT_CONCEPT    (is_reference_c<const int&>);
    DT_NOT_CONCEPT(is_reference_c<int>);
    DT_NOT_CONCEPT(is_reference_c<int*>);

    //   structural invariant: lvalue + rvalue ref concepts partition
    // is_reference_c
    DT_CONCEPT    (is_lvalue_reference_c<int&>      || is_rvalue_reference_c<int&>);
    DT_CONCEPT    (is_lvalue_reference_c<int&&>     || is_rvalue_reference_c<int&&>);
    DT_NOT_CONCEPT(is_lvalue_reference_c<int&>      && is_rvalue_reference_c<int&>);

    // -------------------------------------------------------------------
    // II.1.e  Class and function concepts
    // -------------------------------------------------------------------

    // is_class_c -- structs and classes both qualify
    DT_CONCEPT    (is_class_c<trivial_class>);
    DT_NOT_CONCEPT(is_class_c<union_t>);            // union, not class
    DT_NOT_CONCEPT(is_class_c<int>);
    DT_NOT_CONCEPT(is_class_c<unscoped_enum_t>);
    DT_NOT_CONCEPT(is_class_c<int*>);

    // is_union_c
    DT_CONCEPT    (is_union_c<union_t>);
    DT_NOT_CONCEPT(is_union_c<trivial_class>);
    DT_NOT_CONCEPT(is_union_c<int>);

    //   structural invariant: classes and unions are disjoint
    DT_NOT_CONCEPT(is_class_c<trivial_class> && is_union_c<trivial_class>);
    DT_NOT_CONCEPT(is_class_c<union_t>       && is_union_c<union_t>);

    // is_function_c -- bare function types, NOT function pointers
    DT_CONCEPT    (is_function_c<void()>);
    DT_CONCEPT    (is_function_c<int(int, double)>);
    DT_NOT_CONCEPT(is_function_c<void(*)()>);                     // function POINTER
    DT_NOT_CONCEPT(is_function_c<decltype(free_function)*>);      // pointer to free fn

    //   silence unused-warning for free_function in the rare case the
    // compiler's diagnostic is finicky about uses confined to decltype
    (void)&free_function;

    return;
}


NS_END  // test
NS_END  // djinterp
