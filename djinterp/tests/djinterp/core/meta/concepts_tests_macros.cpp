/*******************************************************************************
* djinterp [test]                                      concepts_tests_macros.cpp
*
*   Tests for concepts.hpp section 0: the D_CONCEPT_HAS_* macro family.
* Each macro emits a unary concept; this TU instantiates one concept per
* macro using the `method_haver` / `method_lacker` fixtures and checks
* both the positive (method present) and negative (method absent) sides.
*
*   The macros are instantiated at file scope here, in an unnamed
* namespace, so the emitted concept names cannot collide with anything
* in djinterp or with the suite-internal fixtures.
*
*
* path:      /tests/djinterp/core/meta/concepts_tests_macros.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "concepts_tests.hpp"


// -----------------------------------------------------------------------------
// macro instantiations (file scope, unnamed namespace)
// -----------------------------------------------------------------------------
// Each macro below produces a concept whose name is given as the first
// argument.  The macros are namespace-agnostic by design, so they're
// emitted here at unnamed-namespace scope to keep their names local.

namespace {

    // D_CONCEPT_HAS_METHOD : detects T.foo() with no args
    D_CONCEPT_HAS_METHOD(has_foo, foo)

    // D_CONCEPT_HAS_METHOD_ARGS : detects T.bar(int)
    D_CONCEPT_HAS_METHOD_ARGS(has_bar_int, bar, int)

    // D_CONCEPT_HAS_METHOD_TYPED : detects T.baz() -> int
    D_CONCEPT_HAS_METHOD_TYPED(has_baz_returns_int, baz, int)

    //   negative cross-check: the same method exists, but the requested
    // return type is wrong; the concept must NOT be satisfied.
    D_CONCEPT_HAS_METHOD_TYPED(has_baz_returns_double, baz, double)

    // D_CONCEPT_HAS_METHOD_ARGS_TYPED : detects T.qux(int) -> int
    D_CONCEPT_HAS_METHOD_ARGS_TYPED(has_qux_int_returns_int, qux, int, int)

    //   negative cross-check: right arg type, wrong return type
    D_CONCEPT_HAS_METHOD_ARGS_TYPED(has_qux_int_returns_void, qux, void, int)

    // D_CONCEPT_HAS_TYPE : detects nested type T::nested
    D_CONCEPT_HAS_TYPE(has_nested_type, nested)

    //   negative cross-check: the type alias name is wrong
    D_CONCEPT_HAS_TYPE(has_missing_type, missing_type_name)

    // D_CONCEPT_HAS_STATIC_MEMBER : detects T::kStatic
    D_CONCEPT_HAS_STATIC_MEMBER(has_kStatic, kStatic)

    //   negative cross-check: the static member name is wrong
    D_CONCEPT_HAS_STATIC_MEMBER(has_missing_static, missing_static_name)

}  // unnamed namespace


NS_DJINTERP_TEST


void
concepts_tests_macros(
    test_handler& _test_handler,
    test_option   _options...
)
{
    using fixtures::method_haver;
    using fixtures::method_lacker;

    // -- D_CONCEPT_HAS_METHOD ------------------------------------------------
    DT_CONCEPT    (has_foo<method_haver>);
    DT_NOT_CONCEPT(has_foo<method_lacker>);
    //   negative case: a type that has bar but not foo also fails has_foo,
    // because foo() specifically is what's being detected
    DT_NOT_CONCEPT(has_foo<int>);

    // -- D_CONCEPT_HAS_METHOD_ARGS -------------------------------------------
    DT_CONCEPT    (has_bar_int<method_haver>);
    DT_NOT_CONCEPT(has_bar_int<method_lacker>);
    //   negative: method exists but with a different signature - e.g. on
    // a type that has foo() (no args) but not bar(int)
    DT_NOT_CONCEPT(has_bar_int<int>);

    // -- D_CONCEPT_HAS_METHOD_TYPED ------------------------------------------
    DT_CONCEPT    (has_baz_returns_int<method_haver>);
    DT_NOT_CONCEPT(has_baz_returns_int<method_lacker>);
    //   the method exists but returns int, not double - the typed
    // form must reject it
    DT_NOT_CONCEPT(has_baz_returns_double<method_haver>);

    // -- D_CONCEPT_HAS_METHOD_ARGS_TYPED -------------------------------------
    DT_CONCEPT    (has_qux_int_returns_int<method_haver>);
    DT_NOT_CONCEPT(has_qux_int_returns_int<method_lacker>);
    //   right arg type, wrong return type
    DT_NOT_CONCEPT(has_qux_int_returns_void<method_haver>);

    // -- D_CONCEPT_HAS_TYPE --------------------------------------------------
    DT_CONCEPT    (has_nested_type<method_haver>);
    DT_NOT_CONCEPT(has_nested_type<method_lacker>);
    DT_NOT_CONCEPT(has_missing_type<method_haver>);
    //   primitive types have no nested types
    DT_NOT_CONCEPT(has_nested_type<int>);

    // -- D_CONCEPT_HAS_STATIC_MEMBER -----------------------------------------
    DT_CONCEPT    (has_kStatic<method_haver>);
    DT_NOT_CONCEPT(has_kStatic<method_lacker>);
    DT_NOT_CONCEPT(has_missing_static<method_haver>);
    DT_NOT_CONCEPT(has_kStatic<int>);

    return;
}


NS_END  // test
NS_END  // djinterp
