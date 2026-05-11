/******************************************************************************
* djinterp [less]                                     less_template_traits.hpp
*
*   Structural SFINAE detection traits for Less extension-specific
* accessors. Layered on top of `css_template_traits.hpp`: any Less
* type that satisfies the CSS protocol automatically inherits CSS
* trait classification, so this header focuses purely on the Less-
* specific surface area (variables, mixins, mixin calls, guards,
* extensions, namespaces, plugin imports, detached rulesets,
* interpolation).
*
*   Both naming conventions are detected for every accessor:
* short-form (`variable_name()`, `mixin_selector()`) AND getter-form
* (`get_variable_name()`, `get_mixin_selector()`).
*
*   DETECTED PROTOCOLS:
*
*   less_variable protocol:
*     A rule with `is_variable_declaration() == true` exposing
*     variable-name (with `@` prefix) and value accessors.
*
*   less_mixin protocol:
*     A rule whose selector is callable (begins with `.` or `#`),
*     exposing parameter access and an optional guard accessor.
*
*   less_extend protocol:
*     A rule or pseudo-selector exposing extend-target and
*     extend-all accessors.
*
*   less_guard protocol:
*     A rule exposing a `when (...)` clause via `guard_clause()`.
*
*   less_import protocol:
*     A rule exposing `import_url()` and `import_options()`
*     accessors.
*
*
* path:      /inc/djinterp/core/util/less/less_template_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    LESS RULE KIND ACCESSOR DETECTION
II.   VARIABLE ACCESSOR DETECTION
III.  MIXIN ACCESSOR DETECTION
IV.   GUARD ACCESSOR DETECTION
V.    EXTEND ACCESSOR DETECTION
VI.   IMPORT / PLUGIN ACCESSOR DETECTION
VII.  NAMESPACE / DETACHED RULESET DETECTION
VIII. STYLESHEET DIALECT DETECTION
IX.   RENDER METHOD DETECTION
X.    COMPOSITE CLASSIFIERS
XI.   CLASSIFICATION STRUCTS
XII.  BACKEND COMPLETENESS
XIII. VARIABLE TEMPLATES
*/

#ifndef DJINTERP_LESS_TEMPLATE_TRAITS_
#define DJINTERP_LESS_TEMPLATE_TRAITS_ 1

// std
#include <cstddef>
#include <ostream>
#include <string>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "../css/css_template_traits.hpp"


NS_DJINTERP

namespace less {


///////////////////////////////////////////////////////////////////////////////
///                I.   LESS RULE KIND ACCESSOR DETECTION                   ///
///////////////////////////////////////////////////////////////////////////////

// has_less_rule_kind_method
template<typename _Type, typename = void>
struct has_less_rule_kind_method : std::false_type {};
template<typename _Type>
struct has_less_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().less_rule_kind())
>> : std::true_type {};


// has_get_less_rule_kind_method
template<typename _Type, typename = void>
struct has_get_less_rule_kind_method : std::false_type {};
template<typename _Type>
struct has_get_less_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_less_rule_kind())
>> : std::true_type {};


// has_less_rule_kind_access
template<typename _Type>
struct has_less_rule_kind_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_less_rule_kind_method<_Type>::value ||
          has_get_less_rule_kind_method<_Type>::value );
};


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


// has_is_detached_ruleset_method
//   trait: true if `_Type` exposes
// `is_detached_ruleset()` const -- variable declarations whose
// value is a curly-brace block.
template<typename _Type, typename = void>
struct has_is_detached_ruleset_method : std::false_type {};
template<typename _Type>
struct has_is_detached_ruleset_method<_Type, void_t<
    decltype(std::declval<const _Type&>().is_detached_ruleset())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                III.   MIXIN ACCESSOR DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// has_mixin_selector_method / has_get_mixin_selector_method
template<typename _Type, typename = void>
struct has_mixin_selector_method : std::false_type {};
template<typename _Type>
struct has_mixin_selector_method<_Type, void_t<
    decltype(std::declval<const _Type&>().mixin_selector())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_mixin_selector_method : std::false_type {};
template<typename _Type>
struct has_get_mixin_selector_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_mixin_selector())
>> : std::true_type {};

template<typename _Type>
struct has_mixin_selector_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_mixin_selector_method<_Type>::value ||
          has_get_mixin_selector_method<_Type>::value );
};


// has_parameters_method / has_get_parameters_method
template<typename _Type, typename = void>
struct has_parameters_method : std::false_type {};
template<typename _Type>
struct has_parameters_method<_Type, void_t<
    decltype(std::declval<const _Type&>().parameters())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_parameters_method : std::false_type {};
template<typename _Type>
struct has_get_parameters_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_parameters())
>> : std::true_type {};

template<typename _Type>
struct has_parameters_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_parameters_method<_Type>::value ||
          has_get_parameters_method<_Type>::value );
};


// has_arguments_method
//   trait: true if `_Type` exposes `arguments()` const --
// the argument list of a mixin call.
template<typename _Type, typename = void>
struct has_arguments_method : std::false_type {};
template<typename _Type>
struct has_arguments_method<_Type, void_t<
    decltype(std::declval<const _Type&>().arguments())
>> : std::true_type {};


// has_is_mixin_call_method
//   trait: true if `_Type` exposes `is_mixin_call()` const.
template<typename _Type, typename = void>
struct has_is_mixin_call_method : std::false_type {};
template<typename _Type>
struct has_is_mixin_call_method<_Type, void_t<
    decltype(std::declval<const _Type&>().is_mixin_call())
>> : std::true_type {};


// has_is_mixin_definition_method
//   trait: true if `_Type` exposes `is_mixin_definition()`
// const.
template<typename _Type, typename = void>
struct has_is_mixin_definition_method : std::false_type {};
template<typename _Type>
struct has_is_mixin_definition_method<_Type, void_t<
    decltype(std::declval<const _Type&>().is_mixin_definition())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                IV.   GUARD ACCESSOR DETECTION                           ///
///////////////////////////////////////////////////////////////////////////////

// has_guard_clause_method / has_get_guard_clause_method
template<typename _Type, typename = void>
struct has_guard_clause_method : std::false_type {};
template<typename _Type>
struct has_guard_clause_method<_Type, void_t<
    decltype(std::declval<const _Type&>().guard_clause())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_guard_clause_method : std::false_type {};
template<typename _Type>
struct has_get_guard_clause_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_guard_clause())
>> : std::true_type {};

template<typename _Type>
struct has_guard_clause_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_guard_clause_method<_Type>::value ||
          has_get_guard_clause_method<_Type>::value );
};


// has_is_guarded_method
//   trait: true if `_Type` exposes `is_guarded()` const.
template<typename _Type, typename = void>
struct has_is_guarded_method : std::false_type {};
template<typename _Type>
struct has_is_guarded_method<_Type, void_t<
    decltype(std::declval<const _Type&>().is_guarded())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                V.   EXTEND ACCESSOR DETECTION                           ///
///////////////////////////////////////////////////////////////////////////////

// has_extend_target_method
//   trait: true if `_Type` exposes `extend_target()` const --
// the selector being extended.
template<typename _Type, typename = void>
struct has_extend_target_method : std::false_type {};
template<typename _Type>
struct has_extend_target_method<_Type, void_t<
    decltype(std::declval<const _Type&>().extend_target())
>> : std::true_type {};


// has_extend_all_method
//   trait: true if `_Type` exposes `extend_all()` const --
// the `all` keyword on an :extend pseudo (`:extend(target all)`).
template<typename _Type, typename = void>
struct has_extend_all_method : std::false_type {};
template<typename _Type>
struct has_extend_all_method<_Type, void_t<
    decltype(std::declval<const _Type&>().extend_all())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VI.   IMPORT / PLUGIN ACCESSOR DETECTION                 ///
///////////////////////////////////////////////////////////////////////////////

// has_import_url_method
//   trait: true if `_Type` exposes `import_url()` const --
// the URL string in `@import 'path'`.
template<typename _Type, typename = void>
struct has_import_url_method : std::false_type {};
template<typename _Type>
struct has_import_url_method<_Type, void_t<
    decltype(std::declval<const _Type&>().import_url())
>> : std::true_type {};


// has_import_options_method
//   trait: true if `_Type` exposes `import_options()` const --
// the bitmask of Less import-option keywords (reference,
// inline, css, less, once, multiple, optional).
template<typename _Type, typename = void>
struct has_import_options_method : std::false_type {};
template<typename _Type>
struct has_import_options_method<_Type, void_t<
    decltype(std::declval<const _Type&>().import_options())
>> : std::true_type {};


// has_plugin_url_method
//   trait: true if `_Type` exposes `plugin_url()` const --
// the URL on `@plugin 'name'`.
template<typename _Type, typename = void>
struct has_plugin_url_method : std::false_type {};
template<typename _Type>
struct has_plugin_url_method<_Type, void_t<
    decltype(std::declval<const _Type&>().plugin_url())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VII.   NAMESPACE / DETACHED RULESET DETECTION            ///
///////////////////////////////////////////////////////////////////////////////

// has_namespace_path_method
//   trait: true if `_Type` exposes `namespace_path()` const --
// the chain of identifiers in `#ns > .mixin`.
template<typename _Type, typename = void>
struct has_namespace_path_method : std::false_type {};
template<typename _Type>
struct has_namespace_path_method<_Type, void_t<
    decltype(std::declval<const _Type&>().namespace_path())
>> : std::true_type {};


// has_detached_ruleset_body_method
//   trait: true if `_Type` exposes
// `detached_ruleset_body()` const -- the body of a detached
// ruleset value.
template<typename _Type, typename = void>
struct has_detached_ruleset_body_method : std::false_type {};
template<typename _Type>
struct has_detached_ruleset_body_method<_Type, void_t<
    decltype(std::declval<const _Type&>().detached_ruleset_body())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VIII.   STYLESHEET DIALECT DETECTION                     ///
///////////////////////////////////////////////////////////////////////////////

// has_less_dialect_method / has_get_less_dialect_method
template<typename _Type, typename = void>
struct has_less_dialect_method : std::false_type {};
template<typename _Type>
struct has_less_dialect_method<_Type, void_t<
    decltype(std::declval<const _Type&>().less_dialect())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_less_dialect_method : std::false_type {};
template<typename _Type>
struct has_get_less_dialect_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_less_dialect())
>> : std::true_type {};

template<typename _Type>
struct has_less_dialect_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_less_dialect_method<_Type>::value ||
          has_get_less_dialect_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                IX.   RENDER METHOD DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

// has_render_to_less_source_method
//   trait: true if `_Type` exposes
// `render_to_less_source(std::ostream&)` const.
template<typename _Type, typename = void>
struct has_render_to_less_source_method : std::false_type {};
template<typename _Type>
struct has_render_to_less_source_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_less_source(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_compile_less_to_css_method
//   trait: true if `_Type` exposes
// `compile_to_css(std::ostream&)` const.
template<typename _Type, typename = void>
struct has_compile_less_to_css_method : std::false_type {};
template<typename _Type>
struct has_compile_less_to_css_method<_Type, void_t<
    decltype(std::declval<const _Type&>().compile_to_css(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_any_less_render_method
template<typename _Type>
struct has_any_less_render_method
{
    D_STATIC_CONSTEXPR bool value = (
           has_render_to_less_source_method<_Type>::value
        || has_compile_less_to_css_method<_Type>::value
    );
};


///////////////////////////////////////////////////////////////////////////////
///                X.   COMPOSITE CLASSIFIERS                               ///
///////////////////////////////////////////////////////////////////////////////

// is_less_rule
//   trait: true if `_Type` satisfies the CSS rule protocol
// AND exposes a Less rule-kind discriminator.
template<typename _Type>
struct is_less_rule
{
    D_STATIC_CONSTEXPR bool value =
        ( ::djinterp::css::is_css_rule<_Type>::value &&
          has_less_rule_kind_access<_Type>::value );
};


// is_less_rule_loose
//   trait: looser variant -- any CSS rule qualifies.
template<typename _Type>
struct is_less_rule_loose
{
    D_STATIC_CONSTEXPR bool value =
        ::djinterp::css::is_css_rule<_Type>::value;
};


// is_less_variable_declaration
//   trait: true if `_Type` exposes variable-name + value
// access.
template<typename _Type>
struct is_less_variable_declaration
{
    D_STATIC_CONSTEXPR bool value =
        ( has_variable_name_access<_Type>::value &&
          ::djinterp::css::has_value_access<_Type>::value );
};


// is_less_mixin
//   trait: true if `_Type` exposes mixin-selector access.
template<typename _Type>
struct is_less_mixin
{
    D_STATIC_CONSTEXPR bool value =
        has_mixin_selector_access<_Type>::value;
};


// is_less_guarded_rule
//   trait: true if `_Type` exposes guard-clause access.
template<typename _Type>
struct is_less_guarded_rule
{
    D_STATIC_CONSTEXPR bool value =
        has_guard_clause_access<_Type>::value;
};


// is_less_extend_statement
//   trait: true if `_Type` exposes extend-target access.
template<typename _Type>
struct is_less_extend_statement
{
    D_STATIC_CONSTEXPR bool value =
        has_extend_target_method<_Type>::value;
};


// is_less_import_rule
//   trait: true if `_Type` exposes import-url access.
template<typename _Type>
struct is_less_import_rule
{
    D_STATIC_CONSTEXPR bool value =
        has_import_url_method<_Type>::value;
};


// is_less_stylesheet
//   trait: true if `_Type` satisfies the CSS stylesheet
// protocol AND exposes a Less dialect accessor.
template<typename _Type>
struct is_less_stylesheet
{
    D_STATIC_CONSTEXPR bool value =
        ( ::djinterp::css::is_css_stylesheet<_Type>::value &&
          has_less_dialect_access<_Type>::value );
};


// is_less_stylesheet_loose
//   trait: looser variant -- any CSS stylesheet qualifies.
template<typename _Type>
struct is_less_stylesheet_loose
{
    D_STATIC_CONSTEXPR bool value =
        ::djinterp::css::is_css_stylesheet<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                XI.   CLASSIFICATION STRUCTS                             ///
///////////////////////////////////////////////////////////////////////////////

// less_rule_class
//   struct: comprehensive classification of a Less-rule-shaped
// type.
template<typename _Type>
struct less_rule_class
{
    D_STATIC_CONSTEXPR bool is_less_rule_  = is_less_rule<_Type>::value;
    D_STATIC_CONSTEXPR bool is_css_rule_   =
        ::djinterp::css::is_css_rule<_Type>::value;

    D_STATIC_CONSTEXPR bool has_kind        =
        has_less_rule_kind_access<_Type>::value;

    D_STATIC_CONSTEXPR bool is_variable     =
        is_less_variable_declaration<_Type>::value;
    D_STATIC_CONSTEXPR bool is_mixin        =
        is_less_mixin<_Type>::value;
    D_STATIC_CONSTEXPR bool is_guarded      =
        is_less_guarded_rule<_Type>::value;
    D_STATIC_CONSTEXPR bool is_extend       =
        is_less_extend_statement<_Type>::value;
    D_STATIC_CONSTEXPR bool is_import       =
        is_less_import_rule<_Type>::value;

    D_STATIC_CONSTEXPR bool has_params      =
        has_parameters_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_arguments   =
        has_arguments_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_guard       =
        has_guard_clause_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_namespace_  =
        has_namespace_path_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_detached    =
        has_detached_ruleset_body_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_plugin_url  =
        has_plugin_url_method<_Type>::value;
};


// less_stylesheet_class
//   struct: comprehensive classification of a Less-stylesheet-
// shaped type.
template<typename _Type>
struct less_stylesheet_class
{
    D_STATIC_CONSTEXPR bool is_less_sheet  = is_less_stylesheet<_Type>::value;
    D_STATIC_CONSTEXPR bool is_css_sheet   =
        ::djinterp::css::is_css_stylesheet<_Type>::value;
    D_STATIC_CONSTEXPR bool has_dialect    =
        has_less_dialect_access<_Type>::value;
    D_STATIC_CONSTEXPR bool can_render_less =
        has_render_to_less_source_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_compile    =
        has_compile_less_to_css_method<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                XII.   BACKEND COMPLETENESS                              ///
///////////////////////////////////////////////////////////////////////////////

// has_less_rule_type_alias
template<typename _Type, typename = void>
struct has_less_rule_type_alias : std::false_type {};
template<typename _Type>
struct has_less_rule_type_alias<_Type, void_t<
    typename _Type::less_rule_type
>> : std::true_type {};


// has_less_stylesheet_type_alias
template<typename _Type, typename = void>
struct has_less_stylesheet_type_alias : std::false_type {};
template<typename _Type>
struct has_less_stylesheet_type_alias<_Type, void_t<
    typename _Type::less_stylesheet_type
>> : std::true_type {};


// has_make_less_stylesheet_method
template<typename _Type, typename = void>
struct has_make_less_stylesheet_method : std::false_type {};
template<typename _Type>
struct has_make_less_stylesheet_method<_Type, void_t<
    decltype(_Type::make_less_stylesheet())
>> : std::true_type {};


// is_less_backend_complete
template<typename _Type>
struct is_less_backend_complete
{
    D_STATIC_CONSTEXPR bool value =
        ( has_less_rule_type_alias<_Type>::value       &&
          has_less_stylesheet_type_alias<_Type>::value &&
          has_make_less_stylesheet_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                XIII.   VARIABLE TEMPLATES                               ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool has_less_rule_kind_access_v =
        has_less_rule_kind_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_variable_name_access_v =
        has_variable_name_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_mixin_selector_access_v =
        has_mixin_selector_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_parameters_access_v =
        has_parameters_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_guard_clause_access_v =
        has_guard_clause_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_import_url_method_v =
        has_import_url_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_less_rule_v =
        is_less_rule<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_less_rule_loose_v =
        is_less_rule_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_less_variable_declaration_v =
        is_less_variable_declaration<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_less_mixin_v =
        is_less_mixin<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_less_guarded_rule_v =
        is_less_guarded_rule<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_less_extend_statement_v =
        is_less_extend_statement<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_less_import_rule_v =
        is_less_import_rule<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_less_stylesheet_v =
        is_less_stylesheet<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_less_stylesheet_loose_v =
        is_less_stylesheet_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_less_backend_complete_v =
        is_less_backend_complete<_Type>::value;

#endif  // variable templates


}   // namespace less
NS_END  // djinterp


#endif  // DJINTERP_LESS_TEMPLATE_TRAITS_
