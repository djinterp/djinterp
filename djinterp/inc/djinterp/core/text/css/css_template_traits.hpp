/******************************************************************************
* djinterp [css]                                       css_template_traits.hpp
*
*   Structural SFINAE detection traits for CSS rule / declaration
* / stylesheet / backend types. Mirrors the shape and idioms of the
* xml / html / markdown trait headers but targets the CSS rule-tree
* model: stylesheets contain rules, style rules carry selectors and
* declaration blocks, declarations are property-value-importance
* triples, at-rules carry an at-keyword + prelude + body.
*
*   Both naming conventions are detected for every accessor:
* short-form (`property()`, `value()`) AND getter-form
* (`get_property()`, `get_value()`). This keeps backend adapters
* from being forced into one convention -- libcss uses getter-form,
* katana-parser uses short-form, both classify automatically.
*
*   DETECTED PROTOCOLS:
*
*   css_declaration protocol:
*     A declaration exposes property-name, value, and importance
*     accessors.
*
*   css_rule protocol:
*     A rule exposes `rule_kind()` (or `get_rule_kind()`) and --
*     depending on kind -- selectors / declarations / at-keyword /
*     prelude / nested rules.
*
*   css_stylesheet protocol:
*     A stylesheet exposes a level / syntax-mode accessor plus
*     rule traversal.
*
*   css_backend protocol:
*     A type tagged with `css_backend_tag` (detected by
*     `is_css_backend` in `css.hpp`) plus the nested type aliases
*     `rule_type`, `declaration_type`, `stylesheet_type`.
*
*
* path:      /inc/djinterp/core/util/css/css_template_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    RULE KIND ACCESSOR DETECTION
II.   AT-RULE ACCESSOR DETECTION
III.  SELECTOR ACCESSOR DETECTION
IV.   DECLARATION ACCESSOR DETECTION (PROPERTY / VALUE / IMPORTANCE)
V.    DECLARATION BLOCK / DECLARATIONS ACCESSORS
VI.   NESTED RULES / CHILDREN ACCESSORS
VII.  STYLESHEET LEVEL / SYNTAX MODE DETECTION
VIII. RENDER METHOD DETECTION
IX.   COMPOSITE CLASSIFIERS
X.    CLASSIFICATION STRUCTS
XI.   BACKEND COMPLETENESS
XII.  VARIABLE TEMPLATES
*/

#ifndef DJINTERP_CSS_TEMPLATE_TRAITS_
#define DJINTERP_CSS_TEMPLATE_TRAITS_ 1

// std
#include <cstddef>
#include <ostream>
#include <string>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"


NS_DJINTERP

namespace css {


///////////////////////////////////////////////////////////////////////////////
///                I.   RULE KIND ACCESSOR DETECTION                        ///
///////////////////////////////////////////////////////////////////////////////

// has_rule_kind_method
//   trait: true if `_Type` exposes `rule_kind()` const.
template<typename _Type, typename = void>
struct has_rule_kind_method : std::false_type
{};

template<typename _Type>
struct has_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().rule_kind())
>> : std::true_type
{};


// has_get_rule_kind_method
template<typename _Type, typename = void>
struct has_get_rule_kind_method : std::false_type
{};

template<typename _Type>
struct has_get_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_rule_kind())
>> : std::true_type
{};


// has_rule_kind_access
//   trait: true if either form is available.
template<typename _Type>
struct has_rule_kind_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_rule_kind_method<_Type>::value ||
          has_get_rule_kind_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                II.   AT-RULE ACCESSOR DETECTION                         ///
///////////////////////////////////////////////////////////////////////////////

// has_at_rule_kind_method / has_get_at_rule_kind_method
template<typename _Type, typename = void>
struct has_at_rule_kind_method : std::false_type {};
template<typename _Type>
struct has_at_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().at_rule_kind())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_at_rule_kind_method : std::false_type {};
template<typename _Type>
struct has_get_at_rule_kind_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_at_rule_kind())
>> : std::true_type {};

template<typename _Type>
struct has_at_rule_kind_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_at_rule_kind_method<_Type>::value ||
          has_get_at_rule_kind_method<_Type>::value );
};


// has_at_keyword_method / has_get_at_keyword_method
template<typename _Type, typename = void>
struct has_at_keyword_method : std::false_type {};
template<typename _Type>
struct has_at_keyword_method<_Type, void_t<
    decltype(std::declval<const _Type&>().at_keyword())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_at_keyword_method : std::false_type {};
template<typename _Type>
struct has_get_at_keyword_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_at_keyword())
>> : std::true_type {};

template<typename _Type>
struct has_at_keyword_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_at_keyword_method<_Type>::value ||
          has_get_at_keyword_method<_Type>::value );
};


// has_prelude_method / has_get_prelude_method
template<typename _Type, typename = void>
struct has_prelude_method : std::false_type {};
template<typename _Type>
struct has_prelude_method<_Type, void_t<
    decltype(std::declval<const _Type&>().prelude())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_prelude_method : std::false_type {};
template<typename _Type>
struct has_get_prelude_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_prelude())
>> : std::true_type {};

template<typename _Type>
struct has_prelude_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_prelude_method<_Type>::value ||
          has_get_prelude_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                III.   SELECTOR ACCESSOR DETECTION                       ///
///////////////////////////////////////////////////////////////////////////////

// has_selector_method / has_get_selector_method
template<typename _Type, typename = void>
struct has_selector_method : std::false_type {};
template<typename _Type>
struct has_selector_method<_Type, void_t<
    decltype(std::declval<const _Type&>().selector())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_selector_method : std::false_type {};
template<typename _Type>
struct has_get_selector_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_selector())
>> : std::true_type {};

template<typename _Type>
struct has_selector_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_selector_method<_Type>::value ||
          has_get_selector_method<_Type>::value );
};


// has_selectors_method
//   trait: true if `_Type` exposes `selectors()` const
// returning a list of selector strings (or selector facades).
// Distinct from `selector()` which typically returns the joined
// selector list as a single string.
template<typename _Type, typename = void>
struct has_selectors_method : std::false_type {};
template<typename _Type>
struct has_selectors_method<_Type, void_t<
    decltype(std::declval<const _Type&>().selectors())
>> : std::true_type {};


// has_set_selector_method
template<typename _Type, typename = void>
struct has_set_selector_method : std::false_type {};
template<typename _Type>
struct has_set_selector_method<_Type, void_t<
    decltype(std::declval<_Type&>().set_selector(
        std::declval<const std::string&>()))
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                IV.   DECLARATION ACCESSOR DETECTION                     ///
///////////////////////////////////////////////////////////////////////////////

// has_property_method / has_get_property_method
template<typename _Type, typename = void>
struct has_property_method : std::false_type {};
template<typename _Type>
struct has_property_method<_Type, void_t<
    decltype(std::declval<const _Type&>().property())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_property_method : std::false_type {};
template<typename _Type>
struct has_get_property_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_property())
>> : std::true_type {};

template<typename _Type>
struct has_property_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_property_method<_Type>::value ||
          has_get_property_method<_Type>::value );
};


// has_value_method / has_get_value_method
template<typename _Type, typename = void>
struct has_value_method : std::false_type {};
template<typename _Type>
struct has_value_method<_Type, void_t<
    decltype(std::declval<const _Type&>().value())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_get_value_method : std::false_type {};
template<typename _Type>
struct has_get_value_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_value())
>> : std::true_type {};

template<typename _Type>
struct has_value_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_value_method<_Type>::value ||
          has_get_value_method<_Type>::value );
};


// has_importance_method / has_is_important_method
template<typename _Type, typename = void>
struct has_importance_method : std::false_type {};
template<typename _Type>
struct has_importance_method<_Type, void_t<
    decltype(std::declval<const _Type&>().importance())
>> : std::true_type {};

template<typename _Type, typename = void>
struct has_is_important_method : std::false_type {};
template<typename _Type>
struct has_is_important_method<_Type, void_t<
    decltype(std::declval<const _Type&>().is_important())
>> : std::true_type {};

template<typename _Type>
struct has_importance_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_importance_method<_Type>::value ||
          has_is_important_method<_Type>::value );
};


// has_origin_method
template<typename _Type, typename = void>
struct has_origin_method : std::false_type {};
template<typename _Type>
struct has_origin_method<_Type, void_t<
    decltype(std::declval<const _Type&>().origin())
>> : std::true_type {};


// has_set_property_method
template<typename _Type, typename = void>
struct has_set_property_method : std::false_type {};
template<typename _Type>
struct has_set_property_method<_Type, void_t<
    decltype(std::declval<_Type&>().set_property(
        std::declval<const std::string&>()))
>> : std::true_type {};


// has_set_value_method
template<typename _Type, typename = void>
struct has_set_value_method : std::false_type {};
template<typename _Type>
struct has_set_value_method<_Type, void_t<
    decltype(std::declval<_Type&>().set_value(
        std::declval<const std::string&>()))
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                V.   DECLARATION BLOCK ACCESSORS                         ///
///////////////////////////////////////////////////////////////////////////////

// has_declarations_method
//   trait: true if `_Type` exposes `declarations()` const
// returning an iterable of declaration nodes.
template<typename _Type, typename = void>
struct has_declarations_method : std::false_type {};
template<typename _Type>
struct has_declarations_method<_Type, void_t<
    decltype(std::declval<const _Type&>().declarations())
>> : std::true_type {};


// has_get_declarations_method
template<typename _Type, typename = void>
struct has_get_declarations_method : std::false_type {};
template<typename _Type>
struct has_get_declarations_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_declarations())
>> : std::true_type {};


// has_declarations_access
template<typename _Type>
struct has_declarations_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_declarations_method<_Type>::value ||
          has_get_declarations_method<_Type>::value );
};


// has_declaration_count_method
template<typename _Type, typename = void>
struct has_declaration_count_method : std::false_type {};
template<typename _Type>
struct has_declaration_count_method<_Type, void_t<
    decltype(std::declval<const _Type&>().declaration_count())
>> : std::true_type {};


// has_find_declaration_method
//   trait: true if `_Type` exposes `find_declaration(name)`.
template<typename _Type, typename = void>
struct has_find_declaration_method : std::false_type {};
template<typename _Type>
struct has_find_declaration_method<_Type, void_t<
    decltype(std::declval<const _Type&>().find_declaration(
        std::declval<const std::string&>()))
>> : std::true_type {};


// has_add_declaration_method
template<typename _Type, typename = void>
struct has_add_declaration_method : std::false_type {};
template<typename _Type>
struct has_add_declaration_method<_Type, void_t<
    decltype(std::declval<_Type&>().add_declaration(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()))
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VI.   NESTED RULES / CHILDREN ACCESSORS                  ///
///////////////////////////////////////////////////////////////////////////////

// has_rules_method
//   trait: true if `_Type` exposes `rules()` const returning
// an iterable of nested rules. Stylesheets and grouping at-
// rules expose this; style rules expose it for CSS Nesting.
template<typename _Type, typename = void>
struct has_rules_method : std::false_type {};
template<typename _Type>
struct has_rules_method<_Type, void_t<
    decltype(std::declval<const _Type&>().rules())
>> : std::true_type {};


// has_get_rules_method
template<typename _Type, typename = void>
struct has_get_rules_method : std::false_type {};
template<typename _Type>
struct has_get_rules_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_rules())
>> : std::true_type {};


// has_rules_access
template<typename _Type>
struct has_rules_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_rules_method<_Type>::value ||
          has_get_rules_method<_Type>::value );
};


// has_rule_count_method
template<typename _Type, typename = void>
struct has_rule_count_method : std::false_type {};
template<typename _Type>
struct has_rule_count_method<_Type, void_t<
    decltype(std::declval<const _Type&>().rule_count())
>> : std::true_type {};


// has_add_rule_method
template<typename _Type, typename = void>
struct has_add_rule_method : std::false_type {};
template<typename _Type>
struct has_add_rule_method<_Type, void_t<
    decltype(std::declval<_Type&>().add_rule())
>> : std::true_type {};


///////////////////////////////////////////////////////////////////////////////
///                VII.   STYLESHEET LEVEL / SYNTAX MODE DETECTION          ///
///////////////////////////////////////////////////////////////////////////////

// has_css_level_method
//   trait: true if `_Type` exposes `level()` const returning a
// `css_level` enum value.
template<typename _Type, typename = void>
struct has_css_level_method : std::false_type {};
template<typename _Type>
struct has_css_level_method<_Type, void_t<
    decltype(std::declval<const _Type&>().level())
>> : std::true_type {};


// has_get_css_level_method
template<typename _Type, typename = void>
struct has_get_css_level_method : std::false_type {};
template<typename _Type>
struct has_get_css_level_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_level())
>> : std::true_type {};


// has_css_level_access
template<typename _Type>
struct has_css_level_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_css_level_method<_Type>::value ||
          has_get_css_level_method<_Type>::value );
};


// has_syntax_mode_method
template<typename _Type, typename = void>
struct has_syntax_mode_method : std::false_type {};
template<typename _Type>
struct has_syntax_mode_method<_Type, void_t<
    decltype(std::declval<const _Type&>().syntax_mode())
>> : std::true_type {};


// has_get_syntax_mode_method
template<typename _Type, typename = void>
struct has_get_syntax_mode_method : std::false_type {};
template<typename _Type>
struct has_get_syntax_mode_method<_Type, void_t<
    decltype(std::declval<const _Type&>().get_syntax_mode())
>> : std::true_type {};


// has_syntax_mode_access
template<typename _Type>
struct has_syntax_mode_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_syntax_mode_method<_Type>::value ||
          has_get_syntax_mode_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VIII.   RENDER METHOD DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// has_render_to_css_method
//   trait: true if `_Type` exposes
// `render_to_css(std::ostream&)` const.
template<typename _Type, typename = void>
struct has_render_to_css_method : std::false_type {};
template<typename _Type>
struct has_render_to_css_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_css(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_render_to_minified_css_method
template<typename _Type, typename = void>
struct has_render_to_minified_css_method : std::false_type {};
template<typename _Type>
struct has_render_to_minified_css_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_minified_css(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_render_to_scss_method
template<typename _Type, typename = void>
struct has_render_to_scss_method : std::false_type {};
template<typename _Type>
struct has_render_to_scss_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_scss(
        std::declval<std::ostream&>()))
>> : std::true_type {};


// has_any_render_method
template<typename _Type>
struct has_any_render_method
{
    D_STATIC_CONSTEXPR bool value = (
           has_render_to_css_method<_Type>::value
        || has_render_to_minified_css_method<_Type>::value
        || has_render_to_scss_method<_Type>::value
    );
};


///////////////////////////////////////////////////////////////////////////////
///                IX.   COMPOSITE CLASSIFIERS                              ///
///////////////////////////////////////////////////////////////////////////////

// is_css_declaration
//   trait: true if `_Type` satisfies the declaration protocol
// (property-name accessor + value accessor).
template<typename _Type>
struct is_css_declaration
{
    D_STATIC_CONSTEXPR bool value =
        ( has_property_access<_Type>::value &&
          has_value_access<_Type>::value );
};


// is_css_declaration_loose
//   trait: looser variant -- property-name accessor alone.
template<typename _Type>
struct is_css_declaration_loose
{
    D_STATIC_CONSTEXPR bool value =
        has_property_access<_Type>::value;
};


// is_css_rule
//   trait: true if `_Type` satisfies the rule protocol --
// rule-kind accessor plus selector OR at-keyword OR
// declarations OR rules access.
template<typename _Type>
struct is_css_rule
{
    D_STATIC_CONSTEXPR bool value =
        ( has_rule_kind_access<_Type>::value &&
          ( has_selector_access<_Type>::value     ||
            has_at_keyword_access<_Type>::value   ||
            has_declarations_access<_Type>::value ||
            has_rules_access<_Type>::value ) );
};


// is_css_rule_loose
//   trait: looser variant -- rule-kind accessor alone.
template<typename _Type>
struct is_css_rule_loose
{
    D_STATIC_CONSTEXPR bool value =
        has_rule_kind_access<_Type>::value;
};


// is_css_style_rule
//   trait: true if `_Type` is identifiable as a style rule --
// has selector access AND declaration access.
template<typename _Type>
struct is_css_style_rule
{
    D_STATIC_CONSTEXPR bool value =
        ( has_selector_access<_Type>::value &&
          has_declarations_access<_Type>::value );
};


// is_css_at_rule
//   trait: true if `_Type` is identifiable as an at-rule --
// has at-keyword access.
template<typename _Type>
struct is_css_at_rule
{
    D_STATIC_CONSTEXPR bool value =
        has_at_keyword_access<_Type>::value;
};


// is_css_stylesheet
//   trait: true if `_Type` satisfies the stylesheet protocol
// -- level / syntax accessor plus rule traversal.
template<typename _Type>
struct is_css_stylesheet
{
    D_STATIC_CONSTEXPR bool value =
        ( ( has_css_level_access<_Type>::value ||
            has_syntax_mode_access<_Type>::value ) &&
          has_rules_access<_Type>::value );
};


// is_css_stylesheet_loose
//   trait: looser variant -- rules access alone.
template<typename _Type>
struct is_css_stylesheet_loose
{
    D_STATIC_CONSTEXPR bool value =
        has_rules_access<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                X.   CLASSIFICATION STRUCTS                              ///
///////////////////////////////////////////////////////////////////////////////

// css_declaration_class
//   struct: comprehensive classification of a declaration-
// shaped type.
template<typename _Type>
struct css_declaration_class
{
    D_STATIC_CONSTEXPR bool is_decl         =
        is_css_declaration<_Type>::value;
    D_STATIC_CONSTEXPR bool has_property    =
        has_property_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_value       =
        has_value_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_importance  =
        has_importance_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_origin      =
        has_origin_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_set_property =
        has_set_property_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_set_value   =
        has_set_value_method<_Type>::value;
};


// css_rule_class
//   struct: comprehensive classification of a rule-shaped
// type.
template<typename _Type>
struct css_rule_class
{
    D_STATIC_CONSTEXPR bool is_rule          =
        is_css_rule<_Type>::value;
    D_STATIC_CONSTEXPR bool is_style_rule    =
        is_css_style_rule<_Type>::value;
    D_STATIC_CONSTEXPR bool is_at_rule       =
        is_css_at_rule<_Type>::value;
    D_STATIC_CONSTEXPR bool has_kind         =
        has_rule_kind_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_at_kind      =
        has_at_rule_kind_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_at_keyword   =
        has_at_keyword_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_prelude      =
        has_prelude_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_selector     =
        has_selector_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_selectors    =
        has_selectors_method<_Type>::value;
    D_STATIC_CONSTEXPR bool has_declarations =
        has_declarations_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_rules        =
        has_rules_access<_Type>::value;
    D_STATIC_CONSTEXPR bool can_find_decl    =
        has_find_declaration_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_add_decl     =
        has_add_declaration_method<_Type>::value;
};


// css_stylesheet_class
//   struct: comprehensive classification of a stylesheet-
// shaped type.
template<typename _Type>
struct css_stylesheet_class
{
    D_STATIC_CONSTEXPR bool is_sheet         =
        is_css_stylesheet<_Type>::value;
    D_STATIC_CONSTEXPR bool has_level        =
        has_css_level_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_syntax       =
        has_syntax_mode_access<_Type>::value;
    D_STATIC_CONSTEXPR bool has_rules        =
        has_rules_access<_Type>::value;
    D_STATIC_CONSTEXPR bool can_render_css   =
        has_render_to_css_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_render_min   =
        has_render_to_minified_css_method<_Type>::value;
    D_STATIC_CONSTEXPR bool can_render_scss  =
        has_render_to_scss_method<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                XI.   BACKEND COMPLETENESS                               ///
///////////////////////////////////////////////////////////////////////////////

// has_rule_type_alias
//   trait: true if `_Type` exposes a nested `rule_type` alias.
template<typename _Type, typename = void>
struct has_rule_type_alias : std::false_type {};
template<typename _Type>
struct has_rule_type_alias<_Type, void_t<
    typename _Type::rule_type
>> : std::true_type {};


// has_declaration_type_alias
template<typename _Type, typename = void>
struct has_declaration_type_alias : std::false_type {};
template<typename _Type>
struct has_declaration_type_alias<_Type, void_t<
    typename _Type::declaration_type
>> : std::true_type {};


// has_stylesheet_type_alias
template<typename _Type, typename = void>
struct has_stylesheet_type_alias : std::false_type {};
template<typename _Type>
struct has_stylesheet_type_alias<_Type, void_t<
    typename _Type::stylesheet_type
>> : std::true_type {};


// has_make_stylesheet_method
//   trait: true if `_Type` exposes a static factory
// `make_stylesheet()` returning a `stylesheet_type`.
template<typename _Type, typename = void>
struct has_make_stylesheet_method : std::false_type {};
template<typename _Type>
struct has_make_stylesheet_method<_Type, void_t<
    decltype(_Type::make_stylesheet())
>> : std::true_type {};


// is_css_backend_complete
//   trait: true if `_Type` exposes the full CSS backend
// protocol -- every nested type alias.
template<typename _Type>
struct is_css_backend_complete
{
    D_STATIC_CONSTEXPR bool value =
        ( has_rule_type_alias<_Type>::value        &&
          has_declaration_type_alias<_Type>::value &&
          has_stylesheet_type_alias<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                XII.   VARIABLE TEMPLATES                                ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool has_rule_kind_access_v =
        has_rule_kind_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_property_access_v =
        has_property_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_value_access_v =
        has_value_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_importance_access_v =
        has_importance_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_selector_access_v =
        has_selector_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_declarations_access_v =
        has_declarations_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_rules_access_v =
        has_rules_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_at_keyword_access_v =
        has_at_keyword_access<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_any_render_method_v =
        has_any_render_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_css_declaration_v =
        is_css_declaration<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_css_rule_v =
        is_css_rule<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_css_rule_loose_v =
        is_css_rule_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_css_style_rule_v =
        is_css_style_rule<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_css_at_rule_v =
        is_css_at_rule<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_css_stylesheet_v =
        is_css_stylesheet<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_css_stylesheet_loose_v =
        is_css_stylesheet_loose<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_css_backend_complete_v =
        is_css_backend_complete<_Type>::value;

#endif  // variable templates


}   // namespace css
NS_END  // djinterp


#endif  // DJINTERP_CSS_TEMPLATE_TRAITS_
