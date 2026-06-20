/******************************************************************************
* djinterp [option]                                         option_compose.hpp
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
*     1. defopt<_Key, _Args...>
*        Define an option surface.  A thin, intention-revealing alias for
*        option<_Key, _Args...> - the "surface" you are describing.  Use
*        it so call sites read as a declaration rather than a raw template
*        instantiation.
*
*     2. with_option_t<_Set, _Key, _Args...>
*        Define the surface AND add it, in one statement.  Builds
*        defopt<_Key, _Args...> and folds it into _Set under a default
*        policy (override_replace: a colliding key takes the new args).
*        with_option_as_t<_Policy, ...> lets the caller pick the merge
*        policy explicitly.
*
*     3. with_options_t<_Set, _Surfaces...> / compose_options_t<...>
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.03
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    defopt                      (define a surface)
II.   with_option_as / with_option (define + add, policy-aware / default)
III.  with_options                (define + add many surfaces)
IV.   compose_options             (build a set from empty in one statement)
*/

#ifndef DJINTERP_OPTION_COMPOSE_
#define DJINTERP_OPTION_COMPOSE_ 1

// djinterp
#include "../djinterp.hpp"
#include "../paradigm/override.hpp"     // OverridePolicy
#include "./option.hpp"
#include "./option_set.hpp"
#include "./option_override.hpp"        // option_set_override_t + policies


NS_DJINTERP


// ===========================================================================
// I.   defopt
// ===========================================================================

// defopt
//   type: defines an option SURFACE.  An intention-revealing alias for
// option<_Key, _Args...> so call sites read as a declaration of the
// surface being described rather than a bare template instantiation.
// Carries no semantics of its own; args remain opaque per option.hpp.
//
// Usage:
//   using title_surface = defopt<window_opt::title, value<"Untitled">>;
template<auto        _Key,
         typename... _Args>
using defopt = option<_Key, _Args...>;


// ===========================================================================
// II.  with_option_as / with_option
// ===========================================================================

NS_INTERNAL

    // as_set
    //   trait: wrap a single surface as a one-element option_set so it
    // can be fed to the merge engine, which speaks only in sets.
    template<typename _Surface>
    struct as_set
    {
        using type = option_set<_Surface>;
    };

    template<typename _Surface>
    using as_set_t = typename as_set<_Surface>::type;

NS_END  // internal


// with_option_as_t
//   trait: define the surface option<_Key, _Args...> AND add it to _Set
// under an explicit _Policy, in one statement.  Yields a new option_set.
// The new surface is the DELTA, so policy hooks (on_both / on_delta_only)
// fire with the fresh surface as the override candidate.
//
// Usage:
//   using s2 = with_option_as_t<override_strict,
//                               s1, window_opt::title, value<"Untitled">>;
template<OverridePolicy _Policy,
         typename                    _Set,
         auto                        _Key,
         typename...                 _Args>
    using with_option_as_t = option_set_override_t<
        _Set,
        internal::as_set_t<defopt<_Key, _Args...>>,
        _Policy
    >;

// with_option_t
//   trait: define the surface AND add it to _Set under the default
// policy (override_replace - a colliding key takes the new surface's
// args).  The common case: "this option now holds for this key."
//
// Usage:
//   using s2 = with_option_t<s1, window_opt::title, value<"Untitled">>;
template<typename    _Set,
         auto        _Key,
         typename... _Args>
using with_option_t = with_option_as_t<override_replace, _Set, _Key, _Args...>;


// ===========================================================================
// III. with_options
// ===========================================================================

NS_INTERNAL

    // as_delta_set
    //   trait: normalize one fold input to an option_set.  A surface
    // (option<>) becomes a one-element set; an already-formed
    // option_set passes through unchanged.  Lets with_options_fold
    // accept a mixed pack of surfaces and sub-sets uniformly.
    template<typename _Entry>
    struct as_delta_set
    {
        using type = option_set<_Entry>;
    };

    template<typename... _Opts>
    struct as_delta_set<option_set<_Opts...>>
    {
        using type = option_set<_Opts...>;
    };

    template<typename _Entry>
    using as_delta_set_t = typename as_delta_set<_Entry>::type;


    // with_options_fold
    //   trait: left fold of _Deltas... into _Acc under _Policy.  Each
    // delta is normalized to a set, then merged via option_set_override.
    template<OverridePolicy _Policy,
             typename                    _Acc,
             typename...                 _Deltas>
    struct with_options_fold
    {
        using type = _Acc;
    };

    template<OverridePolicy _Policy,
             typename                    _Acc,
             typename                    _Head,
             typename...                 _Tail>
    struct with_options_fold<_Policy, _Acc, _Head, _Tail...>
    {
    private:
        using merged = option_set_override_t<_Acc, as_delta_set_t<_Head>, _Policy>;

    public:
        using type = typename with_options_fold<
            _Policy, merged, _Tail...>::type;
    };

NS_END  // internal


// with_options_as_t
//   trait: fold a pack of already-defined surfaces (and/or sub-sets) into
// _Base, left to right, under an explicit _Policy.  Later entries win per
// the policy's collision rule.  Yields a new option_set.
template<OverridePolicy _Policy,
         typename                    _Base,
         typename...                 _Surfaces>
using with_options_as_t =
    typename internal::with_options_fold<_Policy, _Base, _Surfaces...>::type;


// with_options_t
//   trait: fold a pack of surfaces (and/or sub-sets) into _Base under the
// default policy (override_replace).  The multi-surface counterpart to
// with_option_t.
//
// Usage:
//   using full = with_options_t<base_set, title_surface, size_surface>;
template<typename    _Base,
         typename... _Surfaces>
using with_options_t = with_options_as_t<override_replace, _Base, _Surfaces...>;


// ===========================================================================
// IV.  compose_options
// ===========================================================================

// compose_options_as_t
//   trait: build an option_set from EMPTY by folding _Surfaces... under
// an explicit _Policy.  The from-scratch counterpart to with_options_as_t.
template<OverridePolicy _Policy,
         typename...                 _Surfaces>
using compose_options_as_t = with_options_as_t<_Policy, option_set<>, _Surfaces...>;

// compose_options_t
//   trait: build an option_set from EMPTY by folding _Surfaces... under
// the default policy (override_replace) - define every surface and add
// each one, in a single declaration.
//
// Usage:
//   using window_opts = compose_options_t<
//       defopt<window_opt::title, value<"Untitled">>,
//       defopt<window_opt::width, value<800>>,
//       defopt<window_opt::height, value<600>>>;
template<typename... _Surfaces>
using compose_options_t = compose_options_as_t<override_replace, _Surfaces...>;


NS_END  // djinterp


#endif  // DJINTERP_OPTION_COMPOSE_