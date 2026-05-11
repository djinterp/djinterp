/******************************************************************************
* djinterp [sass]                                     sass_template_traits.hpp
*
*   Structural SFINAE detection traits for Sass / SCSS
* extension-specific accessors. Layered on top of
* `css_template_traits.hpp`: any Sass type that satisfies the CSS
* protocol automatically inherits CSS trait classification, so this
* header focuses purely on the Sass-specific surface area
* (variables, mixins, includes, control flow, modules,
* placeholder selectors, parent reference).
*
*   Both naming conventions are detected for every accessor:
* short-form (`variable_name()`, `mixin_name()`) AND getter-form
* (`get_variable_name()`, `get_mixin_name()`). This keeps backend
* adapters from being forced into one convention.
*
*   DETECTED PROTOCOLS:
*
*   sass_variable protocol:
*     A rule with `is_variable_declaration() == true` exposing
*     variable-name and value accessors plus optional default
*     (`!default`) and global (`!global`) flag accessors.
*
*   sass_mixin protocol:
*     A rule with `is_mixin_declaration() == true` exposing
*     mixin-name, parameter list, and a body of nested rules.
*
*   sass_include protocol:
*     A rule with `is_include_statement() == true` exposing the
*     target mixin name and the argument list.
*
*   sass_control_flow protocol:
*     Rules with `is_if_statement()` / `is_each_statement()` /
*     `is_for_statement()` / `is_while_statement()` exposing
*     condition or loop-variable accessors.
*
*
* path:      /inc/djinterp/core/util/sass/sass_template_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SASS RULE KIND ACCESSOR DETECTION
II.   VARIABLE ACCESSOR DETECTION
III.  MIXIN / FUNCTION ACCESSOR DETECTION
IV.   INCLUDE / EXTEND ACCESSOR DETECTION
V.    CONTROL FLOW ACCESSOR DETECTION
VI.   MODULE ACCESSOR DETECTION
VII.  STYLESHEET SYNTAX / DIALECT DETECTION
VIII. RENDER METHOD DETECTION
IX.   COMPOSITE CLASSIFIERS
X.    CLASSIFICATION STRUCTS
XI.   BACKEND COMPLETENESS
XII.  VARIABLE TEMPLATES
*/

#ifndef DJINTERP_SASS_TEMPLATE_TRAITS_
#define DJINTERP_SASS_TEMPLATE_TRAITS_ 1

// std
#include <cstddef>
#include <ostream>
#include <string>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "../css/css_template_traits.hpp"


NS_DJINTERP

namespace sass {


///////////////////////////////////////////////////////////////////////////////
///                I.   SASS RULE KIND ACCESSOR DETECTION                   ///
///////////////////////////////////////////////////////////////////////////////

// has_sass_rule_kind_method
//   trait: true if `_Type` exposes `sass_rule_kind()` const.
template<typename _Type, typename = void>
struct has_sass_rule_kind_method : std::false_type {};
template<typename _Type>
struct has_sass_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().sass_rule_kind())
>> : std::true_type {};


// has_get_sass_rule_kind_method
template<typename _Type, typename = void>
struct has_get_sass_rule_kind_method : std::false_type {};
template<typename _Type>
struct has_get_sass_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_sass_rule_kind())
>> : std::true_type {};


// has_sass_rule_kind_access
template<typename _Type>
struct has_sass_rule_kind_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_sass_rule_kind_method<_Type>::value ||
          has_get_sass_rule_kind_method<_Type>::value );
};


// has_sass_at_rule_kind_method
template<typename _Type, typename = void>
struct has_sass_at_rule_kind_method : std::false_type {};
template<typename _Type>
struct has_sass_at_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().sass_at_rule_kind())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                II.   VARIABLE ACCESSOR DETECTION                        ///
///////////////////////////////////////////////////////////////////////////////

// has_variable_name_method / has_get_variable_name_method
template<typename _Type, typename = void>
struct has_variable_name_method : std::false_type {};
template<typename _Type>
struct has_variable_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().variable_name())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_variable_name_method : std::false_type {};
template<typename _Type>
struct has_get_variable_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_variable_name())
>> : std::true_type {};

template<typename _Type>
struct has_variable_name_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_variable_name_method<_Type>::value ||
          has_get_variable_name_method<_Type>::value );
};


// has_is_default_method
//   trait: true if `_Type` exposes `is_default()` const --
// used for variable declarations carrying `!default`.
template<typename _Type, typename = void>
struct has_is_default_method : std::false_type {};
template<typename _Type>
struct has_is_default_method<_Type, void_t<
    decltype(std::declval<const _Type&>().is_default())
>> : std::true_type {};


// has_is_global_method
//   trait: true if `_Type` exposes `is_global()` const -- used
// for variable declarations carrying `!global`.
template<typename _Type, typename = void>
struct has_is_global_method : std::false_type {};
template<typename _Type>
struct has_is_global_method<_Type, void_t<
    decltype(std::declval<const _Type&>().is_global())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                III.   MIXIN / FUNCTION ACCESSOR DETECTION               ///
///////////////////////////////////////////////////////////////////////////////

// has_mixin_name_method / has_get_mixin_name_method
template<typename _Type, typename = void>
struct has_mixin_name_method : std::false_type {};
template<typename _Type>
struct has_mixin_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().mixin_name())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_mixin_name_method : std::false_type {};
template<typename _Type>
struct has_get_mixin_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_mixin_name())
>> : std::true_type {};

template<typename _Type>
struct has_mixin_name_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_mixin_name_method<_Type>::value ||
          has_get_mixin_name_method<_Type>::value );
};


// has_function_name_method / has_get_function_name_method
template<typename _Type, typename = void>
struct has_function_name_method : std::false_type {};
template<typename _Type>
struct has_function_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().function_name())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_function_name_method : std::false_type {};
template<typename _Type>
struct has_get_function_name_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_function_name())
>> : std::true_type {};

template<typename _Type>
struct has_function_name_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_function_name_method<_Type>::value ||
          has_get_function_name_method<_Type>::value );
};


// has_parameters_method
//   trait: true if `_Type` exposes `parameters()` const.
template<typename _Type, typename = void>
struct has_parameters_method : std::false_type {};
template<typename _Type>
struct has_parameters_method<_Type, void_t<
    decltype(std::declval<const _Type&>().parameters())
>> : std::true_type {};


// has_get_parameters_method
template<typename _Type, typename = void>
struct has_get_parameters_method : std::false_type {};
template<typename _Type>
struct has_get_parameters_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_parameters())
>> : std::true_type {};


// has_parameters_access
template<typename _Type>
struct has_parameters_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_parameters_method<_Type>::value ||
          has_get_parameters_method<_Type>::value );
};


// has_accepts_content_block_method
//   trait: true if `_Type` exposes `accepts_content_block()`
// const -- mixins receiving an `@content` block.
template<typename _Type, typename = void>
struct has_accepts_content_block_method : std::false_type {};
template<typename _Type>
struct has_accepts_content_block_method<_Type, void_t<
    decltype(std::declval<const _Type&>().accepts_content_block())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                IV.   INCLUDE / EXTEND ACCESSOR DETECTION                ///
///////////////////////////////////////////////////////////////////////////////

// has_include_target_method
//   trait: true if `_Type` exposes `include_target()` const --
// the mixin name being included.
template<typename _Type, typename = void>
struct has_include_target_method : std::false_type {};
template<typename _Type>
struct has_include_target_method<_Type, void_t<
    decltype(std::declval<const _Type&>().include_target())
>> : std::true_type {};


// has_arguments_method
//   trait: true if `_Type` exposes `arguments()` const.
template<typename _Type, typename = void>
struct has_arguments_method : std::false_type {};
template<typename _Type>
struct has_arguments_method<_Type, void_t<
    decltype(std::declval<const _Type&>().arguments())
>> : std::true_type {};


// has_extend_target_method
//   trait: true if `_Type` exposes `extend_target()` const --
// the selector being extended.
template<typename _Type, typename = void>
struct has_extend_target_method : std::false_type {};
template<typename _Type>
struct has_extend_target_method<_Type, void_t<
    decltype(std::declval<const _Type&>().extend_target())
>> : std::true_type {};


// has_extend_optional_method
//   trait: true if `_Type` exposes `extend_optional()` const
// -- @extend ... !optional flag.
template<typename _Type, typename = void>
struct has_extend_optional_method : std::false_type {};
template<typename _Type>
struct has_extend_optional_method<_Type, void_t<
    decltype(std::declval<const _Type&>().extend_optional())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                V.   CONTROL FLOW ACCESSOR DETECTION                     ///
///////////////////////////////////////////////////////////////////////////////

// has_condition_method / has_get_condition_method
template<typename _Type, typename = void>
struct has_condition_method : std::false_type {};
template<typename _Type>
struct has_condition_method<_Type, void_t<
    decltype(std::declval<const _Type&>().condition())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_condition_method : std::false_type {};
template<typename _Type>
struct has_get_condition_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_condition())
>> : std::true_type {};

template<typename _Type>
struct has_condition_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_condition_method<_Type>::value ||
          has_get_condition_method<_Type>::value );
};


// has_loop_variable_method
//   trait: true if `_Type` exposes `loop_variable()` const --
// `$x` in `@each $x in ...` or `@for $x from ...`.
template<typename _Type, typename = void>
struct has_loop_variable_method : std::false_type {};
template<typename _Type>
struct has_loop_variable_method<_Type, void_t<
    decltype(std::declval<const _Type&>().loop_variable())
>> : std::true_type {};


// has_loop_iterable_method
//   trait: true if `_Type` exposes `loop_iterable()` const --
// the list / range being iterated.
template<typename _Type, typename = void>
struct has_loop_iterable_method : std::false_type {};
template<typename _Type>
struct has_loop_iterable_method<_Type, void_t<
    decltype(std::declval<const _Type&>().loop_iterable())
>> : std::true_type {};


// has_loop_from_method / has_loop_to_method
template<typename _Type, typename = void>
struct has_loop_from_method : std::false_type {};
template<typename _Type>
struct has_loop_from_method<_Type, void_t<
    decltype(std::declval<const _Type&>().loop_from())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_loop_to_method : std::false_type {};
template<typename _Type>
struct has_loop_to_method<_Type, void_t<
    decltype(std::declval<const _Type&>().loop_to())
>> : std::true_type {};


// has_loop_through_method
//   trait: true if `_Type` exposes `loop_is_through()` const
// -- distinguishes `@for $i from 1 through 5` (inclusive)
// from `@for $i from 1 to 5` (exclusive).
template<typename _Type, typename = void>
struct has_loop_through_method : std::false_type {};
template<typename _Type>
struct has_loop_through_method<_Type, void_t<
    decltype(std::declval<const _Type&>().loop_is_through())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VI.   MODULE ACCESSOR DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// has_module_url_method
//   trait: true if `_Type` exposes `module_url()` const --
// the URL string of `@use 'name'` or `@forward 'name'`.
template<typename _Type, typename = void>
struct has_module_url_method : std::false_type {};
template<typename _Type>
struct has_module_url_method<_Type, void_t<
    decltype(std::declval<const _Type&>().module_url())
>> : std::true_type {};


// has_module_namespace_method
//   trait: true if `_Type` exposes `module_namespace()` const
// -- the namespace alias (`@use 'foo' as bar`).
template<typename _Type, typename = void>
struct has_module_namespace_method : std::false_type {};
template<typename _Type>
struct has_module_namespace_method<_Type, void_t<
    decltype(std::declval<const _Type&>().module_namespace())
>> : std::true_type {};


// has_module_configuration_method
//   trait: true if `_Type` exposes `module_configuration()`
// const -- the `with (...)` overrides on `@use`.
template<typename _Type, typename = void>
struct has_module_configuration_method : std::false_type {};
template<typename _Type>
struct has_module_configuration_method<_Type, void_t<
    decltype(std::declval<const _Type&>().module_configuration())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VII.   STYLESHEET SYNTAX / DIALECT DETECTION             ///
///////////////////////////////////////////////////////////////////////////////

// has_sass_syntax_method / has_get_sass_syntax_method
template<typename _Type, typename = void>
struct has_sass_syntax_method : std::false_type {};
template<typename _Type>
struct has_sass_syntax_method<_Type, void_t<
    decltype(std::declval<const _Type&>().sass_syntax())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_sass_syntax_method : std::false_type {};
template<typename _Type>
struct has_get_sass_syntax_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_sass_syntax())
>> : std::true_type {};

template<typename _Type>
struct has_sass_syntax_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_sass_syntax_method<_Type>::value ||
          has_get_sass_syntax_method<_Type>::value );
};


// has_sass_dialect_method
template<typename _Type, typename = void>
struct has_sass_dialect_method : std::false_type {};
template<typename _Type>
struct has_sass_dialect_method<_Type, void_t<
    decltype(std::declval<const _Type&>().sass_dialect())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VIII.   RENDER METHOD DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// has_render_to_scss_source_method
//   trait: true if `_Type` exposes
// `render_to_scss_source(std::ostream&)` const -- emits SCSS
// source rather than compiled CSS.
template<typename _Type, typename = void>
struct has_render_to_scss_source_method : std::false_type {};
template<typename _Type>
struct has_render_to_scss_source_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_scss_source(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_render_to_sass_source_method
//   trait: true if `_Type` exposes
// `render_to_sass_source(std::ostream&)` const -- emits Sass
// (indented) source.
template<typename _Type, typename = void>
struct has_render_to_sass_source_method : std::false_type {};
template<typename _Type>
struct has_render_to_sass_source_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_sass_source(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_compile_to_css_method
//   trait: true if `_Type` exposes `compile_to_css(...)` --
// evaluates the Sass tree and emits compiled CSS.
template<typename _Type, typename = void>
struct has_compile_to_css_method : std::false_type {};
template<typename _Type>
struct has_compile_to_css_method<_Type, void_t<
    decltype(std::declval<const _Type&>().compile_to_css(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_any_sass_render_method
template<typename _Type>
struct has_any_sass_render_method
{
    D_STATIC_CONSTEXPR bool value = (
           has_render_to_scss_source_method<_Type>::value
        || has_render_to_sass_source_method<_Type>::value
        || has_compile_to_css_method<_Type>::value
    );
};


///////////////////////////////////////////////////////////////////////////////
///                IX.   COMPOSITE CLASSIFIERS                              ///
///////////////////////////////////////////////////////////////////////////////

// is_sass_rule
//   trait: true if `_Type` satisfies the CSS rule protocol
// AND exposes a Sass rule-kind discriminator. The minimum bar
// is the CSS rule protocol; the Sass kind accessor marks it
// as Sass-aware.
template<typename _Type>
struct is_sass_rule
{
    D_STATIC_CONSTEXPR bool value =
        ( ::djinterp::css::is_css_rule<_Type>::value &&
          has_sass_rule_kind_access<_Type>::value );
};


// is_sass_rule_loose
//   trait: looser detection -- any CSS rule is treated as a
// candidate Sass rule.
template<typename _Type>
struct is_sass_rule_loose
{
    D_STATIC_CONSTEXPR bool value =
        ::djinterp::css::is_css_rule<_Type>::value;
};


// is_sass_variable_declaration
//   trait: true if `_Type` exposes the variable-declaration
// surface (variable name + value access).
template<typename _Type>
struct is_sass_variable_declaration
{
    D_STATIC_CONSTEXPR bool value =
        ( has_variable_name_access<_Type>::value &&
          ::djinterp::css::has_value_access<_Type>::value );
};


// is_sass_mixin_declaration
//   trait: true if `_Type` exposes mixin-name and parameter
// access.
template<typename _Type>
struct is_sass_mixin_declaration
{
    D_STATIC_CONSTEXPR bool value =
        ( has_mixin_name_access<_Type>::value &&
          has_parameters_access<_Type>::value );
};


// is_sass_function_declaration
//   trait: true if `_Type` exposes function-name and parameter
// access.
template<typename _Type>
struct is_sass_function_declaration
{
    D_STATIC_CONSTEXPR bool value =
        ( has_function_name_access<_Type>::value &&
          has_parameters_access<_Type>::value );
};


// is_sass_include_statement
//   trait: true if `_Type` exposes include-target access.
template<typename _Type>
struct is_sass_include_statement
{
    D_STATIC_CONSTEXPR bool value =
        has_include_target_method<_Type>::value;
};


// is_sass_extend_statement
//   trait: true if `_Type` exposes extend-target access.
template<typename _Type>
struct is_sass_extend_statement
{
    D_STATIC_CONSTEXPR bool value =
        has_extend_target_method<_Type>::value;
};


// is_sass_module_rule
//   trait: true if `_Type` exposes module-url access.
template<typename _Type>
struct is_sass_module_rule
{
    D_STATIC_CONSTEXPR bool value =
        has_module_url_method<_Type>::value;
};


// is_sass_stylesheet
//   trait: true if `_Type` satisfies the CSS stylesheet
// protocol AND exposes a Sass syntax accessor.
template<typename _Type>
struct is_sass_stylesheet
{
    D_STATIC_CONSTEXPR bool value =
        ( ::djinterp::css::is_css_stylesheet<_Type>::value &&
          has_sass_syntax_access<_Type>::value );
};


// is_sass_stylesheet_loose
//   trait: looser detection -- any CSS stylesheet qualifies.
template<typename _Type>
struct is_sass_stylesheet_loose
{
    D_STATIC_CONSTEXPR bool value =
        ::djinterp::css::is_css_stylesheet<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                X.   CLASSIFICATION STRUCTS                              ///
///////////////////////////////////////////////////////////////////////////////

// sass_rule_class
//   struct: comprehensive classification of a Sass-rule-shaped
// type. Aggregates the CSS rule classification with Sass
// extensions.
template<typename _Type>
struct sass_rule_class
{
    D_STATIC_CONSTEXPR bool is_sass_rule_  = is_sass_rule<_Type>::value;
    D_STATIC_CONSTEXPR bool is_css_rule_   =
        ::djinterp::css::is_css_rule<_Type>::value;

    // Sass-specific surface
    D_STATIC_CONSTEXPR bool has_kind       =
        has_sass_rule_kind_access<_Type>::value;

    D_STATIC_CONSTEXPR bool is_variable    =
        is_sass_variable_declaration<_Type>::value;
    D_STATIC_CONSTEXPR bool is_mixin       =
        is_sass_mixin_declaration<_Type>::value;
    D_STATIC_CONSTEXPR bool is_function    =
        is_sass_function_declaration<_Type>::value;
    D_STATIC_CONSTEXPR bool is_include     =
        is_sass_include_statement<_Type>::value;
    D_STATIC_CONSTEXPR bool is_extend      =
        is_sass_extend_statement<_Type>::value;
    D_STATIC_CONSTEXPR bool is_module      =
        is_sass_module_rule<_Type>::value;

    // capability flags
    D_STATIC_CONSTEXPR bool has_default    =
        has_is_default_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_global     =
        has_is_global_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_content    =
        has_accepts_content_block_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_condition  =
        has_condition_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_loop_var   =
        has_loop_variable_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_namespace_ =
        has_module_namespace_method<_Type>::value;
};


// sass_stylesheet_class
//   struct: comprehensive classification of a Sass-stylesheet
// shaped type.
template<typename _Type>
struct sass_stylesheet_class
{
    D_STATIC_CONSTEXPR bool is_sass_sheet  = is_sass_stylesheet<_Type>::value;
    D_STATIC_CONSTEXPR bool is_css_sheet   =
        ::djinterp::css::is_css_stylesheet<_Type>::value;
    D_STATIC_CONSTEXPR bool has_syntax     =
        has_sass_syntax_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_dialect    =
        has_sass_dialect_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_render_scss =
        has_render_to_scss_source_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_render_sass =
        has_render_to_sass_source_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_compile    =
        has_compile_to_css_method<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                XI.   BACKEND COMPLETENESS                               ///
///////////////////////////////////////////////////////////////////////////////

// has_sass_rule_type_alias
//   trait: true if `_Type` exposes a nested `sass_rule_type`
// alias (in addition to the inherited CSS `rule_type`).
template<typename _Type, typename = void>
struct has_sass_rule_type_alias : std::false_type {};
template<typename _Type>
struct has_sass_rule_type_alias<_Type, void_t<
    typename _Type::sass_rule_type
>> : std::true_type {};


// has_sass_stylesheet_type_alias
template<typename _Type, typename = void>
struct has_sass_stylesheet_type_alias : std::false_type {};
template<typename _Type>
struct has_sass_stylesheet_type_alias<_Type, void_t<
    typename _Type::sass_stylesheet_type
>> : std::true_type {};


// has_make_sass_stylesheet_method
//   trait: true if `_Type` exposes a static factory
// `make_sass_stylesheet()`.
template<typename _Type, typename = void>
struct has_make_sass_stylesheet_method : std::false_type {};
template<typename _Type>
struct has_make_sass_stylesheet_method<_Type, void_t<
    decltype(_Type::make_sass_stylesheet())
>> : std::true_type {};


// is_sass_backend_complete
//   trait: true if `_Type` exposes the full Sass backend
// protocol -- nested type aliases plus factory.
template<typename _Type>
struct is_sass_backend_complete
{
    D_STATIC_CONSTEXPR bool value =
        ( has_sass_rule_type_alias<_Type>::value       &&
          has_sass_stylesheet_type_alias<_Type>::value &&
          has_make_sass_stylesheet_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                XII.   VARIABLE TEMPLATES                                ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool has_sass_rule_kind_access_v =
        has_sass_rule_kind_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_variable_name_access_v =
        has_variable_name_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_mixin_name_access_v =
        has_mixin_name_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_parameters_access_v =
        has_parameters_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_condition_access_v =
        has_condition_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_module_url_method_v =
        has_module_url_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_rule_v =
        is_sass_rule<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_rule_loose_v =
        is_sass_rule_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_variable_declaration_v =
        is_sass_variable_declaration<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_mixin_declaration_v =
        is_sass_mixin_declaration<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_function_declaration_v =
        is_sass_function_declaration<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_include_statement_v =
        is_sass_include_statement<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_extend_statement_v =
        is_sass_extend_statement<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_module_rule_v =
        is_sass_module_rule<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_stylesheet_v =
        is_sass_stylesheet<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_stylesheet_loose_v =
        is_sass_stylesheet_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_sass_backend_complete_v =
        is_sass_backend_complete<_Type>::value;

#endif  // variable templates


}   // namespace sass
NS_END  // djinterp


#endif  // DJINTERP_SASS_TEMPLATE_TRAITS_
