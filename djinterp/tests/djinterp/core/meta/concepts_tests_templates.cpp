/*******************************************************************************
* djinterp [test]                                   concepts_tests_templates.cpp
*
*   Tests for concepts.hpp section II.7: template concepts.
* Covers: has_nested_template_type_c, has_variadic_constructor_c,
* template_parameter_base_of, is_template_c, is_template_with_args_c.
*
*   Edge cases of particular interest:
*     - is_template_c is satisfied by zero-arg template instantiations
*       like `Foo<>`.  Non-empty instantiations match
*       is_template_with_args_c instead - the two concepts are
*       complementary specializations of the same template
*       partitioning.
*     - has_nested_template_type_c probes `typename T::template
*       type<int>`.  Using a concrete instantiation parameter (int)
*       avoids tripping the parser without committing to deduction.
*     - template_parameter_base_of requires `T::value_type` to be a
*       base of T.  We provide a fixture where the alias names a class
*       that IS a base, and a counter-example where it does not.
*
*
* path:      /tests/djinterp/core/meta/concepts_tests_templates.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "concepts_tests.hpp"


namespace {

    // value_type_not_base
    //   helper: has a value_type alias but it's NOT a base of this class.
    // Negative case for template_parameter_base_of.
    struct unrelated_class
    {};

    struct value_type_not_base
    {
        using value_type = unrelated_class;
    };

}  // unnamed namespace


NS_DJINTERP_TEST


void
concepts_tests_templates(
    test_handler& _test_handler,
    test_option   _options...)
{
    using fixtures::empty_template;
    using fixtures::args_template;
    using fixtures::nested_template_holder;
    using fixtures::template_with_base_value_type;
    using fixtures::trivial_class;
    using fixtures::method_haver;
    using fixtures::method_lacker;

    // -------------------------------------------------------------------
    // is_template_c -- empty instantiation only (Foo<>)
    // -------------------------------------------------------------------

    DT_CONCEPT    (is_template_c<empty_template<>>);
    DT_CONCEPT    (is_template_c<args_template<>>);
    //   non-empty instantiations do NOT match
    DT_NOT_CONCEPT(is_template_c<args_template<int>>);
    DT_NOT_CONCEPT(is_template_c<empty_template<int>>);
    DT_NOT_CONCEPT(is_template_c<int>);
    DT_NOT_CONCEPT(is_template_c<trivial_class>);

    // -------------------------------------------------------------------
    // is_template_with_args_c -- non-empty instantiation
    // -------------------------------------------------------------------

    DT_CONCEPT    (is_template_with_args_c<args_template<int>>);
    DT_CONCEPT    ((is_template_with_args_c<args_template<int, double>>));
    DT_CONCEPT    (is_template_with_args_c<empty_template<int>>);
    DT_CONCEPT    ((is_template_with_args_c<std::tuple<int, double>>));
    //   empty instantiations do NOT match
    DT_NOT_CONCEPT(is_template_with_args_c<args_template<>>);
    DT_NOT_CONCEPT(is_template_with_args_c<empty_template<>>);
    DT_NOT_CONCEPT(is_template_with_args_c<int>);
    DT_NOT_CONCEPT(is_template_with_args_c<trivial_class>);

    //   structural invariant: is_template_c and is_template_with_args_c
    // are mutually exclusive (a particular instantiation is either
    // empty or non-empty, never both)
    DT_NOT_CONCEPT(is_template_c<empty_template<>>
                   && is_template_with_args_c<empty_template<>>);
    DT_NOT_CONCEPT(is_template_c<args_template<int>>
                   && is_template_with_args_c<args_template<int>>);

    // -------------------------------------------------------------------
    // has_nested_template_type_c -- `typename T::template type<int>`
    // -------------------------------------------------------------------

    DT_CONCEPT    (has_nested_template_type_c<nested_template_holder>);
    DT_NOT_CONCEPT(has_nested_template_type_c<method_haver>);
    DT_NOT_CONCEPT(has_nested_template_type_c<method_lacker>);
    DT_NOT_CONCEPT(has_nested_template_type_c<int>);

    // -------------------------------------------------------------------
    // has_variadic_constructor_c -- T(declval<T>()) is valid
    // -------------------------------------------------------------------
    //   The concept is named "variadic" but the actual check is "can
    // T be constructed from itself?", i.e. is the copy ctor or move
    // ctor accessible.  So almost any well-formed class satisfies it.

    DT_CONCEPT    (has_variadic_constructor_c<trivial_class>);
    DT_CONCEPT    (has_variadic_constructor_c<int>);
    DT_CONCEPT    (has_variadic_constructor_c<method_haver>);

    // -------------------------------------------------------------------
    // template_parameter_base_of -- T::value_type is a base of T
    // -------------------------------------------------------------------

    DT_CONCEPT    (template_parameter_base_of<template_with_base_value_type>);
    //   alias exists but is NOT a base
    DT_NOT_CONCEPT(template_parameter_base_of<value_type_not_base>);
    //   no value_type alias at all
    DT_NOT_CONCEPT(template_parameter_base_of<trivial_class>);
    DT_NOT_CONCEPT(template_parameter_base_of<int>);

    return;
}


NS_END  // test
NS_END  // djinterp
