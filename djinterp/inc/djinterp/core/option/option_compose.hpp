/*******************************************************************************
* djinterp [core]                                             option_compose.hpp
*
*   Fluent idioms for defining an option SURFACE and folding it into an
* option_set in ONE expression.  This is the "declare + add" sugar layer
* that sits on top of option<> (option.hpp), option_set<> (option_set.hpp),
* and the merge engine (option_override.hpp).
*
*   The framework is purely TYPE-LEVEL: an option_set is an immutable
* aggregate of option<> types, not a mutable container.  "Adding an
* option in one fluid statement" therefore means producing a NEW set type
* that is the old set extended by a freshly-described option - all at
* compile time.  Nothing here mutates; every idiom yields a type.
*
*   Three layers, increasing in fluency:
*
*     1. defopt<Key, Args...>
*        Define an option surface.  A thin, intention-revealing alias for
*        option<Key, Args...> - the "surface" you are describing.  Use
*        it so call sites read as a declaration rather than a raw template
*        instantiation.
*
*     2. with_option_t<Set, Key, Args...>
*        Define the surface AND add it, in one statement.  Builds
*        defopt<Key, Args...> and folds it into Set under a default
*        policy (override_replace: a colliding key takes the new args).
*        with_option_as_t<Policy, ...> lets the caller pick the merge
*        policy explicitly.
*
*     3. with_options_t<Set, Surfaces...> / compose_options_t<...>
*        Fold a whole pack of already-defined surfaces (or sets) into a
*        base in left-to-right order.  compose_options_t builds a set up
*        from empty in a single declaration.
*
*   POLICY: every fold routes through option_set_override, so the full
* policy vocabulary from option_override.hpp is available - override_replace
* (default), override_strict, value_only_delta, arg_union_delta, etc.  The
* default is override_replace because "add this option" most naturally means
* "this option now holds for this key", overwriting any prior surface.
*
*
* path:      /inc/djinterp/core/option/option_compose.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.03
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    defopt                      (define a surface)
      ----------------------------------------------

II.   with_option_as / with_option (define + add, policy-aware / default)
      -------------------------------------------------------------------

III.  with_options                (define + add many surfaces)
      --------------------------------------------------------

IV.   compose_options             (build a set from empty in one statement)
      ---------------------------------------------------------------------
*/

#ifndef DJINTERP_OPTION_OPTION_COMPOSE_HPP
#define DJINTERP_OPTION_OPTION_COMPOSE_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../djinterp.hpp"
#include "../meta/override.hpp"     // OverridePolicy
#include "./option.hpp"
#include "./option_set.hpp"
#include "./option_override.hpp"        // option_set_override_t + policies


NS_DJINTERP


// ===========================================================================
// I.   defopt
// ===========================================================================

// defopt
//   type: defines an option SURFACE.  An intention-revealing alias for
// option<Key, Args...> so call sites read as a declaration of the
// surface being described rather than a bare template instantiation.
// Carries no semantics of its own; args remain opaque per option.hpp.
//
// Usage:
//   using title_surface = defopt<window_opt::title, value<"Untitled">>;
template<auto        Key,
         typename... Args>
using defopt = option<Key, Args...>;


// ===========================================================================
// II.  with_option_as / with_option
// ===========================================================================

NS_INTERNAL

    // as_set
    //   trait: wrap a single surface as a one-element option_set so it
    // can be fed to the merge engine, which speaks only in sets.
    template<typename Surface>
    struct as_set
    {
        using type = option_set<Surface>;
    };

    template<typename Surface>
    using as_set_t = typename as_set<Surface>::type;

NS_END  // internal


// with_option_as_t
//   trait: define the surface option<Key, Args...> AND add it to Set
// under an explicit Policy, in one statement.  Yields a new option_set.
// The new surface is the DELTA, so policy hooks (on_both / on_delta_only)
// fire with the fresh surface as the override candidate.
//
// Usage:
//   using s2 = with_option_as_t<override_strict,
//                               s1, window_opt::title, value<"Untitled">>;
template<D_CONCEPT_PARAM(OverridePolicy) Policy,
         typename                    Set,
         auto                        Key,
         typename...                 Args>
    using with_option_as_t = option_set_override_t<
        Set,
        internal::as_set_t<defopt<Key, Args...>>,
        Policy
    >;

// with_option_t
//   trait: define the surface AND add it to Set under the default
// policy (override_replace - a colliding key takes the new surface's
// args).  The common case: "this option now holds for this key."
//
// Usage:
//   using s2 = with_option_t<s1, window_opt::title, value<"Untitled">>;
template<typename    Set,
         auto        Key,
         typename... Args>
using with_option_t = with_option_as_t<override_replace, Set, Key, Args...>;


// ===========================================================================
// III. with_options
// ===========================================================================

NS_INTERNAL

    // as_delta_set
    //   trait: normalize one fold input to an option_set.  A surface
    // (option<>) becomes a one-element set; an already-formed
    // option_set passes through unchanged.  Lets with_options_fold
    // accept a mixed pack of surfaces and sub-sets uniformly.
    template<typename Entry>
    struct as_delta_set
    {
        using type = option_set<Entry>;
    };

    template<typename... Opts>
    struct as_delta_set<option_set<Opts...>>
    {
        using type = option_set<Opts...>;
    };

    template<typename Entry>
    using as_delta_set_t = typename as_delta_set<Entry>::type;


    // with_options_fold
    //   trait: left fold of Deltas... into Acc under Policy.  Each
    // delta is normalized to a set, then merged via option_set_override.
    template<D_CONCEPT_PARAM(OverridePolicy) Policy,
             typename                    Acc,
             typename...                 Deltas>
    struct with_options_fold
    {
        using type = Acc;
    };

    template<D_CONCEPT_PARAM(OverridePolicy) Policy,
             typename                    Acc,
             typename                    Head,
             typename...                 Tail>
    struct with_options_fold<Policy, Acc, Head, Tail...>
    {
    private:
        using merged = option_set_override_t<Acc, as_delta_set_t<Head>, Policy>;

    public:
        using type = typename with_options_fold<
            Policy, merged, Tail...>::type;
    };

NS_END  // internal


// with_options_as_t
//   trait: fold a pack of already-defined surfaces (and/or sub-sets) into
// Base, left to right, under an explicit Policy.  Later entries win per
// the policy's collision rule.  Yields a new option_set.
template<D_CONCEPT_PARAM(OverridePolicy) Policy,
         typename                    Base,
         typename...                 Surfaces>
using with_options_as_t =
    typename internal::with_options_fold<Policy, Base, Surfaces...>::type;


// with_options_t
//   trait: fold a pack of surfaces (and/or sub-sets) into Base under the
// default policy (override_replace).  The multi-surface counterpart to
// with_option_t.
//
// Usage:
//   using full = with_options_t<base_set, title_surface, size_surface>;
template<typename    Base,
         typename... Surfaces>
using with_options_t = with_options_as_t<override_replace, Base, Surfaces...>;


// ===========================================================================
// IV.  compose_options
// ===========================================================================

// compose_options_as_t
//   trait: build an option_set from EMPTY by folding Surfaces... under
// an explicit Policy.  The from-scratch counterpart to with_options_as_t.
template<D_CONCEPT_PARAM(OverridePolicy) Policy,
         typename...                 Surfaces>
using compose_options_as_t = with_options_as_t<Policy, option_set<>, Surfaces...>;

// compose_options_t
//   trait: build an option_set from EMPTY by folding Surfaces... under
// the default policy (override_replace) - define every surface and add
// each one, in a single declaration.
//
// Usage:
//   using window_opts = compose_options_t<
//       defopt<window_opt::title, value<"Untitled">>,
//       defopt<window_opt::width, value<800>>,
//       defopt<window_opt::height, value<600>>>;
template<typename... Surfaces>
using compose_options_t = compose_options_as_t<override_replace, Surfaces...>;


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_OPTION_OPTION_COMPOSE_HPP
