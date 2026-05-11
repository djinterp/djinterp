/******************************************************************************
* djinterp [less]                                   less_template_concepts.hpp
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
*     template<less::less_rule_type _Rule>
*     void process(const _Rule& r);
*
*     // Constrain to a Less mixin that has a guard.
*     template<less::guarded_less_mixin _Rule>
*     void instantiate(const _Rule& r);
*
*     // Constrain to a Less stylesheet that compiles to CSS.
*     template<less::compilable_less_stylesheet _Sheet>
*     std::string compile(const _Sheet& s);
*
*
* path:      /inc/djinterp/core/util/less/less_template_concepts.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    RULE CONCEPTS
II.   VARIABLE CONCEPTS
III.  MIXIN CONCEPTS
IV.   GUARD CONCEPTS
V.    EXTEND CONCEPTS
VI.   IMPORT CONCEPTS
VII.  STYLESHEET CONCEPTS
VIII. RENDER-TARGET CONCEPTS
IX.   COMPOSITE CONCEPTS
X.    BACKEND CONCEPTS
*/

#ifndef DJINTERP_LESS_TEMPLATE_CONCEPTS_
#define DJINTERP_LESS_TEMPLATE_CONCEPTS_ 1

// djinterp
#include "../../../djinterp.hpp"
#include "./less.hpp"
#include "./less_template_traits.hpp"
#include "../css/css_template_concepts.hpp"


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
template<typename _Type>
concept less_rule_type =
    is_less_rule<_Type>::value;


// less_rule_loose_type
//   concept: looser variant -- any CSS rule qualifies.
template<typename _Type>
concept less_rule_loose_type =
    is_less_rule_loose<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                II.   VARIABLE CONCEPTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// less_variable_declaration_type
//   concept: a rule exposing variable-name and value access.
template<typename _Type>
concept less_variable_declaration_type =
    is_less_variable_declaration<_Type>::value;


// detached_ruleset_declaration_type
//   concept: a variable declaration whose value is a curly-
// brace block.
template<typename _Type>
concept detached_ruleset_declaration_type =
       ( less_variable_declaration_type<_Type> )
    && ( has_is_detached_ruleset_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                III.   MIXIN CONCEPTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// less_mixin_type
//   concept: a rule exposing mixin-selector access.
template<typename _Type>
concept less_mixin_type =
    is_less_mixin<_Type>::value;


// parametric_less_mixin
//   concept: a mixin exposing parameter access.
template<typename _Type>
concept parametric_less_mixin =
       ( less_mixin_type<_Type> )
    && ( has_parameters_access<_Type>::value );


// callable_less_mixin
//   concept: a mixin exposing argument access (i.e. usable
// as the call-site form).
template<typename _Type>
concept callable_less_mixin =
       ( less_mixin_type<_Type> )
    && ( has_arguments_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                IV.   GUARD CONCEPTS                                     ///
///////////////////////////////////////////////////////////////////////////////

// guarded_less_rule
//   concept: a rule exposing a guard-clause accessor.
template<typename _Type>
concept guarded_less_rule =
    is_less_guarded_rule<_Type>::value;


// guarded_less_mixin
//   concept: a mixin with a guard clause.
template<typename _Type>
concept guarded_less_mixin =
       ( less_mixin_type<_Type> )
    && ( guarded_less_rule<_Type> );


///////////////////////////////////////////////////////////////////////////////
///                V.   EXTEND CONCEPTS                                     ///
///////////////////////////////////////////////////////////////////////////////

// less_extend_statement_type
//   concept: a rule or selector exposing extend-target access.
template<typename _Type>
concept less_extend_statement_type =
    is_less_extend_statement<_Type>::value;


// all_extend_statement
//   concept: an extend statement exposing the `all` flag.
template<typename _Type>
concept all_extend_statement =
       ( less_extend_statement_type<_Type> )
    && ( has_extend_all_method<_Type>::value );


///////////////////////////////////////////////////////////////////////////////
///                VI.   IMPORT CONCEPTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// less_import_rule_type
//   concept: a rule exposing import-url access.
template<typename _Type>
concept less_import_rule_type =
    is_less_import_rule<_Type>::value;


// optionful_less_import_rule
//   concept: an import rule exposing option-bitmask access.
template<typename _Type>
concept optionful_less_import_rule =
       ( less_import_rule_type<_Type> )
    && ( has_import_options_method<_Type>::value );


// less_plugin_rule_type
//   concept: a rule exposing plugin-url access.
template<typename _Type>
concept less_plugin_rule_type =
    has_plugin_url_method<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VII.   STYLESHEET CONCEPTS                               ///
///////////////////////////////////////////////////////////////////////////////

// less_stylesheet_type
//   concept: a stylesheet exposing dialect accessor plus the
// inherited CSS stylesheet protocol.
template<typename _Type>
concept less_stylesheet_type =
    is_less_stylesheet<_Type>::value;


// less_stylesheet_loose_type
//   concept: looser variant -- any CSS stylesheet qualifies.
template<typename _Type>
concept less_stylesheet_loose_type =
    is_less_stylesheet_loose<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VIII.   RENDER-TARGET CONCEPTS                           ///
///////////////////////////////////////////////////////////////////////////////

// less_source_renderable_stylesheet
//   concept: a stylesheet that emits Less source.
template<typename _Type>
concept less_source_renderable_stylesheet =
    has_render_to_less_source_method<_Type>::value;


// compilable_less_stylesheet
//   concept: a stylesheet that compiles to CSS.
template<typename _Type>
concept compilable_less_stylesheet =
    has_compile_less_to_css_method<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                IX.   COMPOSITE CONCEPTS                                 ///
///////////////////////////////////////////////////////////////////////////////

// full_less_stylesheet
//   concept: a stylesheet exposing both Less render targets.
template<typename _Type>
concept full_less_stylesheet =
       ( less_stylesheet_type<_Type> )
    && ( less_source_renderable_stylesheet<_Type> )
    && ( compilable_less_stylesheet<_Type> );


///////////////////////////////////////////////////////////////////////////////
///                X.   BACKEND CONCEPTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// less_backend_type
//   concept: satisfied by any type tagged with
// `less_backend_tag`.
template<typename _Type>
concept less_backend_type =
    is_less_backend<_Type>::value;


// complete_less_backend
//   concept: a Less backend that additionally exposes the
// full nested-type-alias protocol and the
// `make_less_stylesheet` factory.
template<typename _Type>
concept complete_less_backend =
       ( less_backend_type<_Type> )
    && ( is_less_backend_complete<_Type>::value );


}   // namespace less
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // DJINTERP_LESS_TEMPLATE_CONCEPTS_
