/*******************************************************************************
* djinterp [core]                                     less_template_concepts.hpp
*
*   C++20 concepts for the Less rule / stylesheet / backend
* protocols. Mirrors the structural traits in
* `less_template_traits.hpp` but exposes them as concept declarations
* usable in template constraints, requires-clauses, and abbreviated
* function-template syntax.
*
*   The whole header is gated behind
* `D_ENV_CPP_FEATURE_LANG_CONCEPTS` -- it produces nothing on
* pre-C++20 toolchains so the rest of the less module remains
* language-version-agnostic.
*
*   USAGE EXAMPLES:
*
*     // Constrain to a Less rule.
*     template<less::less_rule_type Rule>
*     void process(const Rule& r);
*
*     // Constrain to a Less mixin that has a guard.
*     template<less::guarded_less_mixin Rule>
*     void instantiate(const Rule& r);
*
*     // Constrain to a Less stylesheet that compiles to CSS.
*     template<less::compilable_less_stylesheet Sheet>
*     std::string compile(const Sheet& s);
*
*
* path:      /inc/djinterp/core/text/css/less/less_template_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.10
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    RULE CONCEPTS
      -------------

II.   VARIABLE CONCEPTS
      -----------------

III.  MIXIN CONCEPTS
      --------------

IV.   GUARD CONCEPTS
      --------------

V.    EXTEND CONCEPTS
      ---------------

VI.   IMPORT CONCEPTS
      ---------------

VII.  STYLESHEET CONCEPTS
      -------------------

VIII. RENDER-TARGET CONCEPTS
      ----------------------

IX.   COMPOSITE CONCEPTS
      ------------------

X.    BACKEND CONCEPTS
      ----------------
*/

#ifndef DJINTERP_TEXT_CSS_LESS_LESS_TEMPLATE_CONCEPTS_HPP
#define DJINTERP_TEXT_CSS_LESS_LESS_TEMPLATE_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../../djinterp.hpp"
#include "./less.hpp"
#include "./less_template_traits.hpp"


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// std
#include <concepts>


NS_DJINTERP

namespace less {


///////////////////////////////////////////////////////////////////////////////
///                I.   RULE CONCEPTS                                       ///
///////////////////////////////////////////////////////////////////////////////

// less_rule_type
//   concept: satisfied by any type that satisfies the CSS rule
// protocol AND exposes a Less rule-kind discriminator.
template<typename Type>
concept less_rule_type =
    is_less_rule<Type>::value;


// less_rule_loose_type
//   concept: looser variant -- any CSS rule qualifies.
template<typename Type>
concept less_rule_loose_type =
    is_less_rule_loose<Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                II.   VARIABLE CONCEPTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// less_variable_declaration_type
//   concept: a rule exposing variable-name and value access.
template<typename Type>
concept less_variable_declaration_type =
    is_less_variable_declaration<Type>::value;


// detached_ruleset_declaration_type
//   concept: a variable declaration whose value is a curly-
// brace block.
template<typename Type>
concept detached_ruleset_declaration_type =
       ( less_variable_declaration_type<Type> )
    && ( has_is_detached_ruleset_method<Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                III.   MIXIN CONCEPTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// less_mixin_type
//   concept: a rule exposing mixin-selector access.
template<typename Type>
concept less_mixin_type =
    is_less_mixin<Type>::value;


// parametric_less_mixin
//   concept: a mixin exposing parameter access.
template<typename Type>
concept parametric_less_mixin =
       ( less_mixin_type<Type> )
    && ( has_parameters_access<Type>::value );


// callable_less_mixin
//   concept: a mixin exposing argument access (i.e. usable
// as the call-site form).
template<typename Type>
concept callable_less_mixin =
       ( less_mixin_type<Type> )
    && ( has_arguments_method<Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                IV.   GUARD CONCEPTS                                     ///
///////////////////////////////////////////////////////////////////////////////

// guarded_less_rule
//   concept: a rule exposing a guard-clause accessor.
template<typename Type>
concept guarded_less_rule =
    is_less_guarded_rule<Type>::value;


// guarded_less_mixin
//   concept: a mixin with a guard clause.
template<typename Type>
concept guarded_less_mixin =
       ( less_mixin_type<Type> )
    && ( guarded_less_rule<Type> );


///////////////////////////////////////////////////////////////////////////////
///                V.   EXTEND CONCEPTS                                     ///
///////////////////////////////////////////////////////////////////////////////

// less_extend_statement_type
//   concept: a rule or selector exposing extend-target access.
template<typename Type>
concept less_extend_statement_type =
    is_less_extend_statement<Type>::value;


// all_extend_statement
//   concept: an extend statement exposing the `all` flag.
template<typename Type>
concept all_extend_statement =
       ( less_extend_statement_type<Type> )
    && ( has_extend_all_method<Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                VI.   IMPORT CONCEPTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// less_import_rule_type
//   concept: a rule exposing import-url access.
template<typename Type>
concept less_import_rule_type =
    is_less_import_rule<Type>::value;


// optionful_less_import_rule
//   concept: an import rule exposing option-bitmask access.
template<typename Type>
concept optionful_less_import_rule =
       ( less_import_rule_type<Type> )
    && ( has_import_options_method<Type>::value );


// less_plugin_rule_type
//   concept: a rule exposing plugin-url access.
template<typename Type>
concept less_plugin_rule_type =
    has_plugin_url_method<Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VII.   STYLESHEET CONCEPTS                               ///
///////////////////////////////////////////////////////////////////////////////

// less_stylesheet_type
//   concept: a stylesheet exposing dialect accessor plus the
// inherited CSS stylesheet protocol.
template<typename Type>
concept less_stylesheet_type =
    is_less_stylesheet<Type>::value;


// less_stylesheet_loose_type
//   concept: looser variant -- any CSS stylesheet qualifies.
template<typename Type>
concept less_stylesheet_loose_type =
    is_less_stylesheet_loose<Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VIII.   RENDER-TARGET CONCEPTS                           ///
///////////////////////////////////////////////////////////////////////////////

// less_source_renderable_stylesheet
//   concept: a stylesheet that emits Less source.
template<typename Type>
concept less_source_renderable_stylesheet =
    has_render_to_less_source_method<Type>::value;


// compilable_less_stylesheet
//   concept: a stylesheet that compiles to CSS.
template<typename Type>
concept compilable_less_stylesheet =
    has_compile_less_to_css_method<Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                IX.   COMPOSITE CONCEPTS                                 ///
///////////////////////////////////////////////////////////////////////////////

// full_less_stylesheet
//   concept: a stylesheet exposing both Less render targets.
template<typename Type>
concept full_less_stylesheet =
       ( less_stylesheet_type<Type> )
    && ( less_source_renderable_stylesheet<Type> )
    && ( compilable_less_stylesheet<Type> );


///////////////////////////////////////////////////////////////////////////////
///                X.   BACKEND CONCEPTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// less_backend_type
//   concept: satisfied by any type tagged with
// `less_backend_tag`.
template<typename Type>
concept less_backend_type =
    is_less_backend<Type>::value;


// complete_less_backend
//   concept: a Less backend that additionally exposes the
// full nested-type-alias protocol and the
// `make_less_stylesheet` factory.
template<typename Type>
concept complete_less_backend =
       ( less_backend_type<Type> )
    && ( is_less_backend_complete<Type>::value );


}   // namespace less
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // floor, for now

#endif  // DJINTERP_TEXT_CSS_LESS_LESS_TEMPLATE_CONCEPTS_HPP
