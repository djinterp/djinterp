/******************************************************************************
* djinterp [css]                                     css_template_concepts.hpp
*
*   C++20 concepts for the CSS declaration / rule / stylesheet /
* backend protocols. Mirrors the structural traits in
* `css_template_traits.hpp` but exposes them as concept declarations
* usable in template constraints, requires-clauses, and abbreviated
* function-template syntax.
*
*   The whole header is gated behind
* `D_ENV_CPP_FEATURE_LANG_CONCEPTS` -- it produces nothing on
* pre-C++20 toolchains so the rest of the css module remains
* language-version-agnostic.
*
*   USAGE EXAMPLES:
*
*     // Constrain a function template to CSS rules only.
*     template<css::css_rule_type _Rule>
*     void process(const _Rule& r);
*
*     // Constrain a renderer to stylesheets that emit minified CSS.
*     template<css::minifiable_stylesheet _Sheet>
*     std::string minify(const _Sheet& s);
*
*     // Constrain a builder to a complete CSS backend.
*     template<css::complete_css_backend _Backend>
*     auto build();
*
*
* path:      /inc/djinterp/core/util/css/css_template_concepts.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DECLARATION CONCEPTS
II.   RULE CONCEPTS
III.  STYLESHEET CONCEPTS
IV.   CAPABILITY CONCEPTS
V.    RENDER-TARGET CONCEPTS
VI.   COMPOSITE CONCEPTS
VII.  BACKEND CONCEPTS
*/

#ifndef DJINTERP_CSS_TEMPLATE_CONCEPTS_
#define DJINTERP_CSS_TEMPLATE_CONCEPTS_ 1

// djinterp
#include "../../../djinterp.hpp"
#include "./css.hpp"
#include "./css_template_traits.hpp"


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// std
#include <concepts>


NS_DJINTERP

namespace css {


///////////////////////////////////////////////////////////////////////////////
///                I.   DECLARATION CONCEPTS                                ///
///////////////////////////////////////////////////////////////////////////////

// css_declaration_type
//   concept: satisfied by any type that satisfies the CSS
// declaration protocol (property + value accessors).
template<typename _Type>
concept css_declaration_type =
    is_css_declaration<_Type>::value;


// css_declaration_loose_type
//   concept: looser variant -- property accessor alone.
template<typename _Type>
concept css_declaration_loose_type =
    is_css_declaration_loose<_Type>::value;


// important_aware_declaration
//   concept: a declaration exposing importance state.
template<typename _Type>
concept important_aware_declaration =
       ( css_declaration_type<_Type> )
    && ( has_importance_access<_Type>::value );


// origin_aware_declaration
//   concept: a declaration exposing cascade-origin state.
template<typename _Type>
concept origin_aware_declaration =
       ( css_declaration_type<_Type> )
    && ( has_origin_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                II.   RULE CONCEPTS                                      ///
///////////////////////////////////////////////////////////////////////////////

// css_rule_type
//   concept: satisfied by any type that satisfies the CSS
// rule protocol (rule-kind accessor plus selector / at-keyword
// / declarations / rules access).
template<typename _Type>
concept css_rule_type =
    is_css_rule<_Type>::value;


// css_rule_loose_type
//   concept: looser variant -- rule-kind accessor alone.
template<typename _Type>
concept css_rule_loose_type =
    is_css_rule_loose<_Type>::value;


// css_style_rule_type
//   concept: a rule identifiable as a style rule (selector +
// declarations).
template<typename _Type>
concept css_style_rule_type =
    is_css_style_rule<_Type>::value;


// css_at_rule_type
//   concept: a rule identifiable as an at-rule (has at-keyword
// access).
template<typename _Type>
concept css_at_rule_type =
    is_css_at_rule<_Type>::value;


// nestable_css_rule_type
//   concept: a rule whose body may host nested rules (CSS
// Nesting / SCSS).
template<typename _Type>
concept nestable_css_rule_type =
       ( css_rule_type<_Type> )
    && ( has_rules_access<_Type>::value );


// declaration_holding_rule_type
//   concept: a rule whose body is a declaration block.
template<typename _Type>
concept declaration_holding_rule_type =
       ( css_rule_type<_Type> )
    && ( has_declarations_access<_Type>::value );


// queryable_rule_type
//   concept: a rule supporting find_declaration().
template<typename _Type>
concept queryable_rule_type =
       ( css_rule_type<_Type> )
    && ( has_find_declaration_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                III.   STYLESHEET CONCEPTS                               ///
///////////////////////////////////////////////////////////////////////////////

// css_stylesheet_type
//   concept: a stylesheet exposing level-or-syntax accessor
// plus rule traversal.
template<typename _Type>
concept css_stylesheet_type =
    is_css_stylesheet<_Type>::value;


// css_stylesheet_loose_type
//   concept: looser variant -- rules access alone.
template<typename _Type>
concept css_stylesheet_loose_type =
    is_css_stylesheet_loose<_Type>::value;


// levelled_css_stylesheet
//   concept: a stylesheet exposing the css-level accessor.
template<typename _Type>
concept levelled_css_stylesheet =
       ( css_stylesheet_type<_Type> )
    && ( has_css_level_access<_Type>::value );


// flavoured_css_stylesheet
//   concept: a stylesheet exposing the syntax-mode accessor.
template<typename _Type>
concept flavoured_css_stylesheet =
       ( css_stylesheet_type<_Type> )
    && ( has_syntax_mode_access<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                IV.   CAPABILITY CONCEPTS                                ///
///////////////////////////////////////////////////////////////////////////////

// mutable_css_declaration
//   concept: a declaration exposing both property and value
// mutators.
template<typename _Type>
concept mutable_css_declaration =
       ( css_declaration_type<_Type> )
    && ( has_set_property_method<_Type>::value )
    && ( has_set_value_method<_Type>::value );


// mutable_css_rule
//   concept: a rule exposing the add_declaration mutator.
template<typename _Type>
concept mutable_css_rule =
       ( css_rule_type<_Type> )
    && ( has_add_declaration_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                V.   RENDER-TARGET CONCEPTS                              ///
///////////////////////////////////////////////////////////////////////////////

// css_renderable_stylesheet
//   concept: a stylesheet exposing render_to_css.
template<typename _Type>
concept css_renderable_stylesheet =
    has_render_to_css_method<_Type>::value;


// minifiable_stylesheet
//   concept: a stylesheet exposing render_to_minified_css.
template<typename _Type>
concept minifiable_stylesheet =
    has_render_to_minified_css_method<_Type>::value;


// scss_renderable_stylesheet
//   concept: a stylesheet exposing render_to_scss.
template<typename _Type>
concept scss_renderable_stylesheet =
    has_render_to_scss_method<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VI.   COMPOSITE CONCEPTS                                 ///
///////////////////////////////////////////////////////////////////////////////

// full_css_stylesheet
//   concept: a stylesheet exposing every render target plus
// both metadata accessors.
template<typename _Type>
concept full_css_stylesheet =
       ( css_stylesheet_type<_Type> )
    && ( levelled_css_stylesheet<_Type> )
    && ( flavoured_css_stylesheet<_Type> )
    && ( css_renderable_stylesheet<_Type> )
    && ( minifiable_stylesheet<_Type> )
    && ( scss_renderable_stylesheet<_Type> );


///////////////////////////////////////////////////////////////////////////////
///                VII.   BACKEND CONCEPTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// css_backend_type
//   concept: satisfied by any type tagged with
// `css_backend_tag`.
template<typename _Type>
concept css_backend_type =
    is_css_backend<_Type>::value;


// complete_css_backend
//   concept: a CSS backend that additionally exposes the full
// nested-type-alias protocol AND a make_stylesheet factory.
template<typename _Type>
concept complete_css_backend =
       ( css_backend_type<_Type> )
    && ( is_css_backend_complete<_Type>::value )
    && ( has_make_stylesheet_method<_Type>::value );


}   // namespace css
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // DJINTERP_CSS_TEMPLATE_CONCEPTS_
