/*******************************************************************************
* djinterp [core]                                            option_override.hpp
*
*   Option-aware override engine plus option-specific policies built on top
* of the abstract override foundation.
*   The engine `option_set_override` walks two flat option tuples (A: base,
* B: delta) and emits a new option_set under a user-chosen policy:
*
*     - for each option in A:
*         if its key is in B  -> apply policy::on_both<A_opt, B_opt>
*         else                -> apply policy::on_base_only<A_opt>
*     - for each option in B whose key is NOT in A:
*         apply policy::on_delta_only<B_opt>
*
*   A policy may return `dropped` to filter a position out of
* the result; otherwise it returns an option<>-shaped type that is
* appended to the accumulator.
*
*   on_delta_only is invoked through a LAZY SFINAE wrapper: it is only
* instantiated for keys that genuinely appear in B but not in A.  This is
* what allows `strict_subset` to hard-error on extension without
* the concept probe firing the assert prematurely.
*   Option-aware merge metafns shipped here:
*     merge_args_union<B,D>    - concatenated args (D first, then B);
*                                  first-match find_arg semantics give
*                                  "delta wins" without explicit dedupe.
*   Lifted policies for direct use:
*     override_replace      = keep_delta
*     override_subset       = drop_extras
*     override_strict       = strict_subset
*     arg_union_delta       = with_on_both<keep_delta, merge_args_union>
*
*
* path:      /inc/djinterp/core/option/option_override.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.25
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    option (re)construction helpers
      -------------------------------

II.   option-aware merge metafns
      --------------------------

III.  lazy on_delta_only SFINAE
      -------------------------

IV.   option_set_override engine
      --------------------------

V.    ready-made policies (lifted + named)
      ------------------------------------
*/

#ifndef DJINTERP_OPTION_OPTION_OVERRIDE_HPP
#define DJINTERP_OPTION_OPTION_OVERRIDE_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../meta/override.hpp"
#include "./option.hpp"                 // option<>, is_option_v
#include "./option_set.hpp"             // option_set<> + queries (contains, find)


NS_DJINTERP

// ===========================================================================
// I.   option (re)construction helpers
// ===========================================================================

NS_INTERNAL

    // option_args_as_tuple
    //   helper: yields std::tuple<args...> of an option (empty
    // tuple for a unary option).
    template<typename Opt>
    struct option_args_as_tuple
    {
        using type = std::tuple<>;
    };

    template<auto        Key,
             typename    First,
             typename... Rest>
    struct option_args_as_tuple<option<Key, First, Rest...>>
    {
        using type = std::tuple<First, Rest...>;
    };

    template<typename Opt>
    using option_args_as_tuple_t =
        typename option_args_as_tuple<Opt>::type;


    // rebuild_option_from_tuple
    //   helper: rebuilds option<Key, args...> from a std::tuple
    // of arg types.  Inverse of option_args_as_tuple for a known key.
    template<auto     Key,
             typename Tup>
    struct rebuild_option_from_tuple;

    template<auto Key>
    struct rebuild_option_from_tuple<Key, std::tuple<>>
    {
        using type = option<Key>;
    };

    template<auto        Key,
             typename... Args>
    struct rebuild_option_from_tuple<Key, std::tuple<Args...>>
    {
        using type = option<Key, Args...>;
    };


    // replace_or_append_arg
    //   helper: walk In..., emit Out... where each arg satisfying
    // Predicate is replaced by New.  If no arg matched at the end,
    // append New.  (Formerly used by the removed merge_actual_only;
    //  currently unused � see the section II note.)
    template<template<typename> class Predicate,
             typename                  New,
             typename                  Out,
             bool                      Replaced,
             typename...               In>
    struct replace_or_append_arg;

    // base case: no more input.  Append New if we didn't replace.
    template<template<typename> class Predicate,
             typename                  New,
             typename...               Out,
             bool                      Replaced>
    struct replace_or_append_arg<Predicate, New,
                                 std::tuple<Out...>, Replaced>
    {
        using type = std::conditional_t<
            Replaced,
            std::tuple<Out...>,
            std::tuple<Out..., New>>;
    };

    // recursive case.
    template<template<typename> class Predicate,
             typename                  New,
             typename...               Out,
             bool                      Replaced,
             typename                  Head,
             typename...               Tail>
    struct replace_or_append_arg<Predicate, New,
                                 std::tuple<Out...>, Replaced,
                                 Head, Tail...>
    {
    private:
        static constexpr bool head_matches = Predicate<Head>::value;

        using head_emit = std::conditional_t<head_matches, New, Head>;

    public:
        using type = typename replace_or_append_arg<
            Predicate, New,
            std::tuple<Out..., head_emit>,
            (Replaced || head_matches),
            Tail...>::type;
    };


    // option_swap_arg
    //   helper: produce a new option<Key, ...> with the same args as
    // Opt except that any arg matching Predicate is replaced by
    // NewArg.  If no arg matched, NewArg is appended.
    template<typename                  Opt,
             template<typename> class  Predicate,
             typename                  NewArg>
    struct option_swap_arg;

    template<auto                      Key,
             typename...               Args,
             template<typename> class  Predicate,
             typename                  NewArg>
    struct option_swap_arg<option<Key, Args...>, Predicate, NewArg>
    {
    private:
        using new_args_tuple = typename replace_or_append_arg<
            Predicate, NewArg,
            std::tuple<>, false,
            Args...>::type;

    public:
        using type = typename rebuild_option_from_tuple<
            Key, new_args_tuple>::type;
    };

    template<auto                      Key,
             template<typename> class  Predicate,
             typename                  NewArg>
    struct option_swap_arg<option<Key>, Predicate, NewArg>
    {
        using type = option<Key, NewArg>;
    };

NS_END  // internal


// ===========================================================================
// II.  option-aware merge metafns
// ===========================================================================

// NOTE: `merge_actual_only` was removed here.  It belonged to the
// obsolete `actual<>` value-merge feature: it depended on the
// `is_actual` / `option_actual_tag_t<>` / `option_has_actual_v<>`
// vocabulary, which was retired together with the `actual<>` option
// carrier (no longer defined anywhere in the option layer).  Its only
// consumers, the `value_only_delta` / `value_only_strict` policies in
// section V, were never instantiated and have likewise been removed.
// The generic swap helpers in section I (option_swap_arg,
// replace_or_append_arg, rebuild_option_from_tuple) are left in place
// but are now unused; remove them too if a full cleanup is desired.


// merge_args_union
//   metafn: concatenated args with D's first, B's second.  Because
// option_find_arg / find_arg return the FIRST match, putting D's
// args first means D wins for any role queried by predicate (actual,
// default_, verifier, description, opposes, ...) without requiring
// the metafn to know any specific predicate.  The result tuple may
// carry duplicates by tag role; downstream queries skip them by
// design.
// Primary template: a neutral fallback for operands that are NOT a matching
// pair of same-keyed options.  It exists so arg_union_delta (section V) can
// satisfy the container-agnostic OverridePolicy concept, whose probe forms
// on_both<int, int> == merge_args_union<int, int>::type.  Every REAL engine
// call supplies two options sharing a key and therefore selects one of the
// partial specializations below; the primary is only ever reached by the
// concept probe (or by deliberate misuse, which now yields `dropped` rather
// than a hard error).
template<typename B,
         typename D>
struct merge_args_union
{
    using type = dropped;
};

template<auto Key,
         typename... BArgs,
         typename... DArgs>
struct merge_args_union<option<Key, BArgs...>, option<Key, DArgs...>>
{
    using type = option<Key, DArgs..., BArgs...>;
};

template<auto Key,
         typename... DArgs>
struct merge_args_union<option<Key>, option<Key, DArgs...>>
{
    using type = option<Key, DArgs...>;
};

template<auto Key,
         typename... BArgs>
struct merge_args_union<option<Key, BArgs...>, option<Key>>
{
    using type = option<Key, BArgs...>;
};

template<auto Key>
struct merge_args_union<option<Key>, option<Key>>
{
    using type = option<Key>;
};


// ===========================================================================
// III. lazy on_delta_only SFINAE
// ===========================================================================

NS_INTERNAL

    // lazy_delta_only
    //   helper: lazy access to a policy's on_delta_only<D> alias.
    // When Drop is true, the policy alias is NEVER instantiated -
    // this is what protects strict_subset's static_assert from firing
    // for keys that already exist in base.  When Drop is false (the
    // key is genuinely delta-only), the policy alias is instantiated
    // and any embedded assert fires legitimately.
    template<bool     Drop,
             typename Policy,
             typename D>
    struct lazy_delta_only
    {
        using type = typename Policy::template on_delta_only<D>;
    };

    template<typename Policy,
             typename D>
    struct lazy_delta_only<true, Policy, D>
    {
        using type = dropped;
    };


    // append_if_kept
    //   helper: appends Type to a std::tuple<...> unless Type is the
    // dropped sentinel.
    template<typename Tup,
             typename Type>
    struct append_if_kept
    {
        using type = decltype(
            std::tuple_cat(std::declval<Tup>(),
                           std::declval<std::tuple<Type>>()));
    };

    template<typename Tup>
    struct append_if_kept<Tup, dropped>
    {
        using type = Tup;
    };

NS_END  // internal


// ===========================================================================
// IV.  option_set_override engine
// ===========================================================================

NS_INTERNAL

    // ov_pick
    //   helper: the base-vs-both branch of override_walk_a, at namespace scope
    // so it is portable (an equivalent member specialization is a non-standard
    // extension GCC rejects).  Laziness is preserved exactly: on_both and
    // option_set_find_t are named ONLY in the InB == true specialization, so a
    // base-only key never instantiates the merge (crucial for merge-based
    // policies such as arg_union_delta) and the concept probe is never engaged.
    template<bool     InB,
             typename Policy,
             typename B,
             typename Head>
    struct ov_pick
    {
        using type = typename Policy::template on_base_only<Head>;
    };

    template<typename Policy,
             typename B,
             typename Head>
    struct ov_pick<true, Policy, B, Head>
    {
        using type = typename Policy::template on_both<
            Head,
            option_set_find_t<B, Head::key>>;
    };

    // override_walk_a
    //   helper: for each option in A, look it up in B and apply the
    // appropriate policy hook (on_both or on_base_only).
    template<typename    Policy,
             typename    B,
             typename    Acc,
             typename... AOpts>
    struct override_walk_a
    {
        using type = Acc;
    };

    template<typename    Policy,
             typename    B,
             typename    Acc,
             typename    Head,
             typename... Tail>
    struct override_walk_a<Policy, B, Acc, Head, Tail...>
    {
    private:
        static constexpr bool in_b =
            option_set_contains_v<B, Head::key>;

        using produced =
            typename ov_pick<in_b, Policy, B, Head>::type;

        using next_acc =
            typename append_if_kept<Acc, produced>::type;

    public:
        using type = typename override_walk_a<
            Policy, B, next_acc, Tail...>::type;
    };


    // override_walk_b_extras
    //   helper: walk B's options, emit on_delta_only for keys NOT
    // already produced from A.  Uses lazy_delta_only so a strict
    // policy's assert is only triggered when an actual extension is
    // detected.
    template<typename    Policy,
             typename    A,
             typename    Acc,
             typename... BOpts>
    struct override_walk_b_extras
    {
        using type = Acc;
    };

    template<typename    Policy,
             typename    A,
             typename    Acc,
             typename    Head,
             typename... Tail>
    struct override_walk_b_extras<Policy, A, Acc, Head, Tail...>
    {
    private:
        static constexpr bool in_a =
            option_set_contains_v<A, Head::key>;

        // in_a == true  -> already handled by walk_a, drop here.
        // in_a == false -> ask the policy what to do with the extension.
        using produced =
            typename lazy_delta_only<in_a, Policy, Head>::type;

        using next_acc =
            typename append_if_kept<Acc, produced>::type;

    public:
        using type = typename override_walk_b_extras<
            Policy, A, next_acc, Tail...>::type;
    };


    // tuple_to_option_set
    //   helper: lift a std::tuple<options...> back into option_set<...>.
    template<typename Tup>
    struct tuple_to_option_set;

    template<typename... Opts>
    struct tuple_to_option_set<std::tuple<Opts...>>
    {
        using type = option_set<Opts...>;
    };

NS_END  // internal


// option_set_override
//   trait: yields option_set<...> = A overridden by B under Policy.
//   The result preserves A's ordering for keys present in A, with
// B-only extensions (if the policy allows them) appended in B's
// order.  Duplicate keys are caught by option_set's own checks at
// final instantiation - so a policy that produces a colliding key
// is a hard error at the right moment.
template<typename A,
         typename B,
         D_CONCEPT_PARAM(OverridePolicy) Policy>
struct option_set_override;

template<typename... AOpts,
         typename... BOpts,
         D_CONCEPT_PARAM(OverridePolicy) Policy>
struct option_set_override<option_set<AOpts...>,
                           option_set<BOpts...>,
                           Policy>
{
    //   THE CONTRACT, restored below C++20. D_CONCEPT_PARAM elides the
    // constraint on tiers without concepts, so this is where it is checked
    // instead. One assertion covers the whole family: every alias in
    // option_compose.hpp funnels through option_set_override_t into this
    // specialization, and an alias template has no body to assert in.
    static_assert(is_override_policy<Policy>::value,
        "option_set_override: Policy must expose nested template aliases "
        "on_base_only<B> and on_both<B, D>. See is_override_policy in "
        "core/meta/override.hpp.");

private:
    using a_flat = typename option_set<AOpts...>::flat_options_t;
    using b_flat = typename option_set<BOpts...>::flat_options_t;

    template<typename Type>
    struct unpack_a;

    template<typename... O>
    struct unpack_a<std::tuple<O...>>
    {
        using type = typename internal::override_walk_a<
            Policy, option_set<BOpts...>, std::tuple<>, O...>::type;
    };

    using after_a = typename unpack_a<a_flat>::type;

    template<typename Type>
    struct unpack_b;

    template<typename... O>
    struct unpack_b<std::tuple<O...>>
    {
        using type = typename internal::override_walk_b_extras<
            Policy, option_set<AOpts...>, after_a, O...>::type;
    };

    using merged_tuple = typename unpack_b<b_flat>::type;

public:
    using type =
        typename internal::tuple_to_option_set<merged_tuple>::type;
};

template<typename       A,
         typename       B,
         D_CONCEPT_PARAM(OverridePolicy) Policy>
using option_set_override_t =
    typename option_set_override<A, B, Policy>::type;


// ===========================================================================
// V.   ready-made policies
// ===========================================================================

// Direct re-exports of paradigm primitives at the option-aware level.
// Names chosen to read well at call sites in the option vocabulary.
using override_replace = keep_delta;       // standard override
using override_keep    = keep_base;        // base wins, ignore delta
using override_subset  = drop_extras;      // delta must overlap
using override_strict  = strict_subset;    // delta extension = error
using override_filter  = drop_unmatched_base;  // keep only B's keys

// NOTE: `value_only_delta` / `value_only_strict` removed with the
// obsolete `merge_actual_only` (see section II).  If the value-only
// merge feature is reinstated, restore the `actual<>` vocabulary
// first, then re-add these two aliases.

// arg_union_delta
//   policy: union of args (D first, B second).  All B args are
// preserved; D args win on any tag-role lookup via find_arg's
// first-match semantics.
using arg_union_delta =
    with_on_both<keep_delta, merge_args_union>;


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_OPTION_OPTION_OVERRIDE_HPP
