/*******************************************************************************
* djinterp [core]                                                        cli.hpp
*
*   The CLI's formatting language as data.  A template is a tree of nested
* option<>s in which the KEY is the node-kind (a `dji_cli` enum value) and the
* opaque arg pack is the node's payload-and-children - exactly the reading
* option.hpp already invites ("the key is the node-kind, the args are the
* node's payload").  Nothing here is a new container: it is a fixed signature
* (one key per primitive) layered onto the option<> already in the framework,
* and the evaluator (dj_cli_render.hpp) is the walk.
*   This mirrors the document-template note precisely.  The BASE is pure
* substitution - lit / var / seq / the structural wrappers / let - a free monad
* over the signature; that is the "simple" half, and it is deliberately NOT
* Turing-complete.  Expressive power is a DERIVED layer one opts into through
* two further node-kinds, kept out of the base "on pain of privileging one
* engine's surface over another's":
*
*     - rewrite_ : an ordered list of rewrite rules applied to a fixpoint - a
*                  normal Markov algorithm (Turing-complete on its own), with
*                  literal patterns at compile time and full std::regex at
*                  runtime.
*     - fix_ + if_ + foreach_ : structural recursion / general control.
*
*   THE TWO SLOTS WANT DIFFERENT SHAPES (and the algebra dictates which):
*     - the TEMPLATE slot is an ORDERED, repeatable tree - a program has order
*       and repetition, so children are an ordered option<> pack (a `seq`),
*       never an option_set;
*     - the SOURCE slot (the variable environment) is an option_set - unordered,
*       unique keys, merged by option_set_override (the note's precedence union
*       (+), i.e. lexical scoping: a `let` shadows by precedence).
*   This header defines the template-slot vocabulary; the environment is just
* an ordinary option_set keyed by `dji_cli_string` names (see dj_cli_render.hpp).
*
*   PAYLOADS RIDE THE CARRIER SUBSTRATE.  A payload is no longer a bespoke
* struct: it is a `carrier::val_t<V>` (meta/carrier.hpp), the framework's
* generic single-value carrier exposing a static `::value`.  text/name carry a
* `dji_cli_string` buffer; width carries a `std::size_t`.  Authoring stays
* readable because each literal/identifier is written as a `fixed_string` NTTP
* (meta/fixed_string.hpp) and lifted into the buffer once, in the carrier
* alias - so `lit<"hi">` deduces exactly as before while the carried value is
* the uniform buffer the engine and the option_set keys want.  `rule` is the
* one exception: it carries three fields, so it stays a small bespoke carrier
* (and, exposing no `::value`, is correctly NOT a payload - it is a child-pack
* member of rewrite_).
*
*   Trait-triple convention as elsewhere: each `is_X` carries `is_X_v` and a
* parallel concept under C++20; payload detection delegates to carrier.hpp's
* `is_value_carrier`.
*
*   Requires C++20 (the payload carriers take class-type non-type template
* arguments); self-suppresses below it.
*
*
* path:      /inc/djinterp/core/cli/cli.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    dji_cli                     (the node-kind signature)
      -----------------------------------------------------

II.   payload carriers            (text, name, width via val_t; rule)
      ---------------------------------------------------------------

III.  node sugar                  (lit, var, seq, ... readable constructors)
      ----------------------------------------------------------------------

IV.   node-shape traits           (is_dji_cli_node, dji_cli_kind, is_dji_cli_payload)
      -------------------------------------------------------------------------------

V.    concepts                    (C++20 analogs)
      -------------------------------------------
*/

#ifndef DJINTERP_CLI_CLI_HPP
#define DJINTERP_CLI_CLI_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"          // NS_*, D_CONSTEXPR_INLINE, clean_t
#include "../meta/type_utility.hpp"    // clean_t
#include "../option/option.hpp"     // option<>
#include "../meta/fixed_string.hpp" // fixed_string<> (authoring-boundary NTTP)
#include "../meta/carrier.hpp"      // val_t<>, is_value_carrier_v
#include "./cli_string.hpp"         // dji_cli_string<>

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #include "../meta/concepts.hpp"
#endif


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS


NS_DJINTERP


// ===========================================================================
// I.   dji_cli
// ===========================================================================

// dji_cli
//   enum: the node-kind signature - one value per formatting primitive.  A
// node is `option<dji_cli::K, payload..., child...>`; this enum is the closed
// set of K's the evaluator dispatches over.  Adding a primitive is one
// enumerator here plus one render specialization in dj_cli_render.hpp.
//   The flag-vs-key-value distinction the option layer draws falls out as
// node arity: a FLAG node is nullary in payload (e.g. `bold`, just a wrapper),
// a KEY-VALUE node carries a payload NTTP (e.g. `indent` carries a `width<>`).
struct dji_cli
{
    enum value
    {
        // --- leaves ---
        lit,       // option<dji_cli::lit, text<"...">>      literal text
        var,       // option<dji_cli::var, name<"x">>        read a variable / hole

        // --- structural concatenation ---
        seq,       // option<dji_cli::seq, c0, c1, ...>      ordered body (the spine)
        group,     // option<dji_cli::group, c0, c1, ...>    alias of seq (grouping)

        // --- formatting (the "formatting expressions") ---
        bold,      // option<dji_cli::bold, child>           flag-shaped wrap (**...**)
        upper,     // option<dji_cli::upper, child>          ASCII upper-case
        lower,     // option<dji_cli::lower, child>          ASCII lower-case
        indent,    // option<dji_cli::indent, width<4>, child>  key-value wrap

        // --- binding + derived control ---
        let_,      // option<dji_cli::let_, name<"x">, value, body>  bind, then body
        if_,       // option<dji_cli::if_, cond, then, else>        choice (lazy)
        foreach_,  // option<dji_cli::foreach_, name<"xs">, name<"e">, body>  fold a list

        // --- Turing-complete layers ---
        rewrite_,  // option<dji_cli::rewrite_, child, rule<...>...>  Markov / regex
        fix_       // option<dji_cli::fix_, name<"self">, body>     recursion
    };
};


// ===========================================================================
// II.  payload carriers
// ===========================================================================
//   Carriers are the typed leaves of a node's arg pack: a payload is a type
// that exposes a static constexpr `::value`, distinguishing it from a CHILD
// (which is itself a node, i.e. some option<dji_cli::K, ...>).  By convention a
// node's payloads come first and its children last.
//
//   text/name/width are thin aliases over the framework carrier `val_t<V>`
// (meta/carrier.hpp); the bare aliases keep the node sugar and the render
// specializations readable while the *type* is just a value carrier.  text and
// name are deliberately the SAME carrier (a `dji_cli_string` buffer) - the
// node-kind key (lit vs var/let/...) is what distinguishes their roles, so a
// single carrier is correct and a name flows straight into option_set_find.

// text
//   alias: a literal-text payload (the body of a `lit` node) - the authored
// `fixed_string` lifted into a `dji_cli_string` buffer and carried by val_t.
template<fixed_string Str>
using text = val_t< dji_cli_string<>{ Str } >;

// name
//   alias: an identifier payload - a variable, let-binding, list, loop, or
// self name.  Carried as a `dji_cli_string` buffer, which is also exactly the
// key type of the variable environment, so a name flows straight into
// option_set_find.
template<fixed_string Id>
using name = val_t< dji_cli_string<>{ Id } >;

// width
//   alias: a numeric payload (e.g. the column count of an `indent` node).
template<std::size_t N>
using width = val_t<N>;

// rule
//   carrier: one ordered rewrite production carried in a `rewrite_` node's
// pack - replace the first occurrence of Pattern with Replacement, halting
// the system if Terminal.  Multi-field, so it stays a bespoke carrier rather
// than a val_t; exposing no `::value`, it is correctly classified a child-pack
// member, not a payload.  At compile time Pattern is a literal; the runtime
// engine reads the same fields as a (possibly regex) rule.
template<fixed_string Pattern,
         fixed_string Replacement,
         bool         Terminal = false>
struct rule
{
    static D_CONSTEXPR dji_cli_string<> pattern     = dji_cli_string<>{ Pattern };
    static D_CONSTEXPR dji_cli_string<> replacement = dji_cli_string<>{ Replacement };
    static D_CONSTEXPR bool             terminal    = Terminal;
};


// ===========================================================================
// III. node sugar
// ===========================================================================
//   Intention-revealing aliases so a template authored by hand reads as a
// program rather than a wall of option<>.  Each is exactly its option<> form;
// they carry no semantics of their own.  Literal/identifier parameters are
// `fixed_string` NTTPs (the authoring boundary); the carrier alias lifts them
// into the buffer.

// lit
//   type: a literal-text node - option<dji_cli::lit, text<Str>>.
template<fixed_string Str>
using lit = option<dji_cli::lit, text<Str>>;

// var
//   type: a variable-read node - option<dji_cli::var, name<Id>>.
template<fixed_string Id>
using var = option<dji_cli::var, name<Id>>;

// seq
//   type: an ordered concatenation node - option<dji_cli::seq, Children...>.
template<typename... Children>
using seq = option<dji_cli::seq, Children...>;

// group
//   type: a grouping node (semantically a seq) - option<dji_cli::group, _C...>.
template<typename... Children>
using group = option<dji_cli::group, Children...>;

// bold
//   type: a flag-shaped wrapper node - option<dji_cli::bold, Child>.
template<typename Child>
using bold = option<dji_cli::bold, Child>;

// upper
//   type: an upper-casing wrapper node - option<dji_cli::upper, Child>.
template<typename Child>
using upper = option<dji_cli::upper, Child>;

// lower
//   type: a lower-casing wrapper node - option<dji_cli::lower, Child>.
template<typename Child>
using lower = option<dji_cli::lower, Child>;

// indent
//   type: a key-value wrapper node - option<dji_cli::indent, width<W>, Child>.
template<std::size_t W,
         typename    Child>
using indent = option<dji_cli::indent, width<W>, Child>;

// let_
//   type: a binding node - option<dji_cli::let_, name<Id>, Value, Body>.
// Renders Value, binds it to Id in the environment (call-by-value, lexical),
// then renders Body under the extended environment.
template<fixed_string Id,
         typename     Value,
         typename     Body>
using let_ = option<dji_cli::let_, name<Id>, Value, Body>;

// if_
//   type: a choice node - option<dji_cli::if_, Cond, Then, Else>.  Only the
// taken branch is rendered (so recursion through fix_ can terminate).
template<typename Cond,
         typename Then,
         typename Else>
using if_ = option<dji_cli::if_, Cond, Then, Else>;

// foreach_
//   type: a fold node - option<dji_cli::foreach_, name<List>, name<Elem>,
// Body>.  The environment binds List to a list (a seq of items); Body is
// rendered once per item with Elem bound to that item, results concatenated.
template<fixed_string List,
         fixed_string Elem,
         typename     Body>
using foreach_ = option<dji_cli::foreach_, name<List>, name<Elem>, Body>;

// rewrite_
//   type: a Markov/regex node - option<dji_cli::rewrite_, Child, Rules...>.
// Renders Child, then drives the ordered Rules... to a fixpoint.
template<typename    Child,
         typename... Rules>
using rewrite_ = option<dji_cli::rewrite_, Child, Rules...>;

// fix_
//   type: a recursion node - option<dji_cli::fix_, name<Self>, Body>.  Binds
// Self to Body in the environment and renders Body, so a `var<Self>`
// inside re-renders the body; terminate via if_.
template<fixed_string Self,
         typename     Body>
using fix_ = option<dji_cli::fix_, name<Self>, Body>;


// ===========================================================================
// IV.  node-shape traits
// ===========================================================================

NS_INTERNAL

    // is_dji_cli_node_impl
    //   trait: detects option<dji_cli::K, ...> (primary: false).
    template<typename Type>
    struct is_dji_cli_node_impl : std::false_type
    {};

    template<dji_cli::value Kind,
             typename...    Args>
    struct is_dji_cli_node_impl<option<Kind, Args...>> : std::true_type
    {};

    // dji_cli_kind_impl
    //   trait: the node-kind of a CLI node (undefined off the domain).
    template<typename Type>
    struct dji_cli_kind_impl;

    template<dji_cli::value Kind,
             typename...    Args>
    struct dji_cli_kind_impl<option<Kind, Args...>>
    {
        static D_CONSTEXPR dji_cli::value value = Kind;
    };

NS_END  // internal


// is_dji_cli_node
//   trait: true iff Type is a CLI node - some option<dji_cli::K, ...>.
// Parallel concept: `dji_cli_node_c`.
template<typename Type>
struct is_dji_cli_node : internal::is_dji_cli_node_impl<clean_t<Type>>
{};

// is_dji_cli_node_v
//   value: variable-template shorthand for is_dji_cli_node<Type>::value.
template<typename Type>
D_CONSTEXPR_INLINE bool is_dji_cli_node_v = is_dji_cli_node<Type>::value;


// dji_cli_kind
//   trait: the `dji_cli` node-kind carried by a CLI node Type.
template<typename Type>
struct dji_cli_kind : internal::dji_cli_kind_impl<clean_t<Type>>
{};

// dji_cli_kind_v
//   value: variable-template shorthand for dji_cli_kind<Type>::value.
template<typename Type>
D_CONSTEXPR_INLINE dji_cli::value dji_cli_kind_v = dji_cli_kind<Type>::value;


// is_dji_cli_payload
//   trait: true iff Type is a payload carrier (a node arg exposing a static
// constexpr ::value) rather than a child node.  The complement, within a
// node's arg pack, of is_dji_cli_node.  Detection delegates to carrier.hpp's
// is_value_carrier, since every payload is now a val_t<> (rule, exposing no
// ::value, is correctly excluded).
template<typename Type>
struct is_dji_cli_payload
    : std::integral_constant<bool,
          ( is_value_carrier_v<clean_t<Type>> &&
            !is_dji_cli_node_v<Type> )>
{};

// is_dji_cli_payload_v
//   value: variable-template shorthand for is_dji_cli_payload<Type>::value.
template<typename Type>
D_CONSTEXPR_INLINE bool is_dji_cli_payload_v = is_dji_cli_payload<Type>::value;


// ===========================================================================
// V.   concepts   (C++20)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// dji_cli_node_c
//   concept: satisfied iff Type is a CLI node.  Parallels is_dji_cli_node_v.
template<typename Type>
concept dji_cli_node_c = is_dji_cli_node_v<Type>;

// dji_cli_payload_c
//   concept: satisfied iff Type is a payload carrier.  Parallels
// is_dji_cli_payload_v.
template<typename Type>
concept dji_cli_payload_c = is_dji_cli_payload_v<Type>;

// dji_cli_kind_is_c
//   concept: satisfied iff Type is a CLI node of node-kind Kind.
template<typename Type, dji_cli::value Kind>
concept dji_cli_kind_is_c = dji_cli_node_c<Type> && (dji_cli_kind_v<Type> == Kind);

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER ...

#endif  // floor, for now


#endif  // DJINTERP_CLI_CLI_HPP
