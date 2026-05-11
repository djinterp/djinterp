/******************************************************************************
* djinterp [sass]                                   sass_template_concepts.hpp
*
*   C++20 concepts for the Sass / SCSS rule / stylesheet / backend
* protocols. Mirrors the structural traits in
* `sass_template_traits.hpp` but exposes them as concept declarations
* usable in template constraints, requires-clauses, and abbreviated
* function-template syntax.
*
*   The whole header is gated behind
* `D_ENV_CPP_FEATURE_LANG_CONCEPTS` -- it produces nothing on
* pre-C++20 toolchains so the rest of the sass module remains
* language-version-agnostic.
*
*   USAGE EXAMPLES:
*
*     // Constrain to a Sass rule.
*     template<sass::sass_rule_type _Rule>
*     void process(const _Rule& r);
*
*     // Constrain to a Sass stylesheet that can compile to CSS.
*     template<sass::compilable_sass_stylesheet _Sheet>
*     std::string compile(const _Sheet& s);
*
*     // Constrain to a complete Sass backend.
*     template<sass::complete_sass_backend _Backend>
*     auto build();
*
*
* path:      /inc/djinterp/core/util/sass/sass_template_concepts.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    RULE CONCEPTS
II.   VARIABLE / MIXIN / FUNCTION CONCEPTS
III.  INCLUDE / EXTEND CONCEPTS
IV.   CONTROL FLOW CONCEPTS
V.    MODULE CONCEPTS
VI.   STYLESHEET CONCEPTS
VII.  RENDER-TARGET CONCEPTS
VIII. COMPOSITE CONCEPTS
IX.   BACKEND CONCEPTS
*/

#ifndef DJINTERP_SASS_TEMPLATE_CONCEPTS_
#define DJINTERP_SASS_TEMPLATE_CONCEPTS_ 1

// djinterp
#include "../../../djinterp.hpp"
#include "./sass.hpp"
#include "./sass_template_traits.hpp"
#include "../css/css_template_concepts.hpp"


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// std
#include <concepts>


NS_DJINTERP

namespace sass {


///////////////////////////////////////////////////////////////////////////////
///                I.   RULE CONCEPTS                                       ///
///////////////////////////////////////////////////////////////////////////////

// sass_rule_type
//   concept: satisfied by any type that satisfies the CSS rule
// protocol AND exposes a Sass rule-kind discriminator.
template<typename _Type>
concept sass_rule_type =
    is_sass_rule<_Type>::value;


// sass_rule_loose_type
//   concept: looser variant -- any CSS rule qualifies.
template<typename _Type>
concept sass_rule_loose_type =
    is_sass_rule_loose<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                II.   VARIABLE / MIXIN / FUNCTION CONCEPTS               ///
///////////////////////////////////////////////////////////////////////////////

// sass_variable_declaration_type
//   concept: a rule exposing variable-name and value access.
template<typename _Type>
concept sass_variable_declaration_type =
    is_sass_variable_declaration<_Type>::value;


// defaulted_variable_declaration
//   concept: a variable declaration that exposes the
// `!default` flag.
template<typename _Type>
concept defaulted_variable_declaration =
       ( sass_variable_declaration_type<_Type> )
    && ( has_is_default_method<_Type>::value );


// global_variable_declaration
//   concept: a variable declaration that exposes the
// `!global` flag.
template<typename _Type>
concept global_variable_declaration =
       ( sass_variable_declaration_type<_Type> )
    && ( has_is_global_method<_Type>::value );


// sass_mixin_declaration_type
//   concept: a rule exposing mixin-name and parameter access.
template<typename _Type>
concept sass_mixin_declaration_type =
    is_sass_mixin_declaration<_Type>::value;


// content_aware_mixin
//   concept: a mixin declaration exposing the
// `accepts_content_block()` predicate.
template<typename _Type>
concept content_aware_mixin =
       ( sass_mixin_declaration_type<_Type> )
    && ( has_accepts_content_block_method<_Type>::value );


// sass_function_declaration_type
//   concept: a rule exposing function-name and parameter
// access.
template<typename _Type>
concept sass_function_declaration_type =
    is_sass_function_declaration<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                III.   INCLUDE / EXTEND CONCEPTS                         ///
///////////////////////////////////////////////////////////////////////////////

// sass_include_statement_type
//   concept: a rule exposing include-target access.
template<typename _Type>
concept sass_include_statement_type =
    is_sass_include_statement<_Type>::value;


// sass_extend_statement_type
//   concept: a rule exposing extend-target access.
template<typename _Type>
concept sass_extend_statement_type =
    is_sass_extend_statement<_Type>::value;


// optional_extend_statement
//   concept: an extend statement exposing the `!optional`
// flag.
template<typename _Type>
concept optional_extend_statement =
       ( sass_extend_statement_type<_Type> )
    && ( has_extend_optional_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                IV.   CONTROL FLOW CONCEPTS                              ///
///////////////////////////////////////////////////////////////////////////////

// conditional_sass_rule
//   concept: a rule exposing a condition expression (i.e.
// classifiable as @if / @while).
template<typename _Type>
concept conditional_sass_rule =
       ( sass_rule_type<_Type> )
    && ( has_condition_access<_Type>::value );


// loop_sass_rule
//   concept: a rule exposing a loop binding (i.e.
// classifiable as @each / @for).
template<typename _Type>
concept loop_sass_rule =
       ( sass_rule_type<_Type> )
    && ( has_loop_variable_method<_Type>::value );


// each_sass_rule
//   concept: a loop rule whose iterable is a list / map.
template<typename _Type>
concept each_sass_rule =
       ( loop_sass_rule<_Type> )
    && ( has_loop_iterable_method<_Type>::value );


// numeric_loop_sass_rule
//   concept: a loop rule with from / to numeric bounds (@for).
template<typename _Type>
concept numeric_loop_sass_rule =
       ( loop_sass_rule<_Type> )
    && ( has_loop_from_method<_Type>::value )
    && ( has_loop_to_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                V.   MODULE CONCEPTS                                     ///
///////////////////////////////////////////////////////////////////////////////

// sass_module_rule_type
//   concept: a rule exposing module-url access.
template<typename _Type>
concept sass_module_rule_type =
    is_sass_module_rule<_Type>::value;


// namespaced_module_rule
//   concept: a module rule exposing namespace-alias access.
template<typename _Type>
concept namespaced_module_rule =
       ( sass_module_rule_type<_Type> )
    && ( has_module_namespace_method<_Type>::value );


// configured_module_rule
//   concept: a module rule exposing `with (...)` configuration
// access.
template<typename _Type>
concept configured_module_rule =
       ( sass_module_rule_type<_Type> )
    && ( has_module_configuration_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                VI.   STYLESHEET CONCEPTS                                ///
///////////////////////////////////////////////////////////////////////////////

// sass_stylesheet_type
//   concept: a stylesheet exposing syntax accessor plus the
// inherited CSS stylesheet protocol.
template<typename _Type>
concept sass_stylesheet_type =
    is_sass_stylesheet<_Type>::value;


// sass_stylesheet_loose_type
//   concept: looser variant -- any CSS stylesheet qualifies.
template<typename _Type>
concept sass_stylesheet_loose_type =
    is_sass_stylesheet_loose<_Type>::value;


// flavoured_sass_stylesheet
//   concept: a stylesheet exposing the dialect accessor.
template<typename _Type>
concept flavoured_sass_stylesheet =
       ( sass_stylesheet_type<_Type> )
    && ( has_sass_dialect_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                VII.   RENDER-TARGET CONCEPTS                            ///
///////////////////////////////////////////////////////////////////////////////

// scss_renderable_sass_stylesheet
//   concept: a stylesheet that emits SCSS source.
template<typename _Type>
concept scss_renderable_sass_stylesheet =
    has_render_to_scss_source_method<_Type>::value;


// indented_renderable_sass_stylesheet
//   concept: a stylesheet that emits Sass-indented source.
template<typename _Type>
concept indented_renderable_sass_stylesheet =
    has_render_to_sass_source_method<_Type>::value;


// compilable_sass_stylesheet
//   concept: a stylesheet that compiles to CSS.
template<typename _Type>
concept compilable_sass_stylesheet =
    has_compile_to_css_method<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VIII.   COMPOSITE CONCEPTS                               ///
///////////////////////////////////////////////////////////////////////////////

// full_sass_stylesheet
//   concept: a stylesheet exposing every Sass render target
// plus syntax and dialect metadata.
template<typename _Type>
concept full_sass_stylesheet =
       ( sass_stylesheet_type<_Type> )
    && ( flavoured_sass_stylesheet<_Type> )
    && ( scss_renderable_sass_stylesheet<_Type> )
    && ( indented_renderable_sass_stylesheet<_Type> )
    && ( compilable_sass_stylesheet<_Type> );


///////////////////////////////////////////////////////////////////////////////
///                IX.   BACKEND CONCEPTS                                   ///
///////////////////////////////////////////////////////////////////////////////

// sass_backend_type
//   concept: satisfied by any type tagged with
// `sass_backend_tag`.
template<typename _Type>
concept sass_backend_type =
    is_sass_backend<_Type>::value;


// complete_sass_backend
//   concept: a Sass backend that additionally exposes the
// full nested-type-alias protocol and the
// `make_sass_stylesheet` factory.
template<typename _Type>
concept complete_sass_backend =
       ( sass_backend_type<_Type> )
    && ( is_sass_backend_complete<_Type>::value );


}   // namespace sass
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // DJINTERP_SASS_TEMPLATE_CONCEPTS_
