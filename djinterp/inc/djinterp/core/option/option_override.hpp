/******************************************************************************
* djinterp [option]                                        option_override.hpp
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
*     merge_args_union<_B,_D>    - concatenated args (D first, then B);
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/
 
/*
TABLE OF CONTENTS
=================
I.    option (re)construction helpers
II.   option-aware merge metafns
III.  lazy on_delta_only SFINAE
IV.   option_set_override engine
V.    ready-made policies (lifted + named)
*/
 
#ifndef DJINTERP_OPTION_OVERRIDE_
#define DJINTERP_OPTION_OVERRIDE_ 1
 
// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
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
    template<typename _Opt>
    struct option_args_as_tuple
    {
        using type = std::tuple<>;
    };
 
    template<auto        _Key,
             typename    _First,
             typename... _Rest>
    struct option_args_as_tuple<option<_Key, _First, _Rest...>>
    {
        using type = std::tuple<_First, _Rest...>;
    };
 
    template<typename _Opt>
    using option_args_as_tuple_t =
        typename option_args_as_tuple<_Opt>::type;
 
 
    // rebuild_option_from_tuple
    //   helper: rebuilds option<_Key, args...> from a std::tuple
    // of arg types.  Inverse of option_args_as_tuple for a known key.
    template<auto     _Key,
             typename _Tup>
    struct rebuild_option_from_tuple;
 
    template<auto _Key>
    struct rebuild_option_from_tuple<_Key, std::tuple<>>
    {
        using type = option<_Key>;
    };
 
    template<auto        _Key,
             typename... _Args>
    struct rebuild_option_from_tuple<_Key, std::tuple<_Args...>>
    {
        using type = option<_Key, _Args...>;
    };
 
 
    // replace_or_append_arg
    //   helper: walk _In..., emit _Out... where each arg satisfying
    // _Predicate is replaced by _New.  If no arg matched at the end,
    // append _New.  (Formerly used by the removed merge_actual_only;
    //  currently unused � see the section II note.)
    template<template<typename> class _Predicate,
             typename                  _New,
             typename                  _Out,
             bool                      _Replaced,
             typename...               _In>
    struct replace_or_append_arg;
 
    // base case: no more input.  Append _New if we didn't replace.
    template<template<typename> class _Predicate,
             typename                  _New,
             typename...               _Out,
             bool                      _Replaced>
    struct replace_or_append_arg<_Predicate, _New,
                                 std::tuple<_Out...>, _Replaced>
    {
        using type = std::conditional_t<
            _Replaced,
            std::tuple<_Out...>,
            std::tuple<_Out..., _New>>;
    };
 
    // recursive case.
    template<template<typename> class _Predicate,
             typename                  _New,
             typename...               _Out,
             bool                      _Replaced,
             typename                  _Head,
             typename...               _Tail>
    struct replace_or_append_arg<_Predicate, _New,
                                 std::tuple<_Out...>, _Replaced,
                                 _Head, _Tail...>
    {
    private:
        static constexpr bool head_matches = _Predicate<_Head>::value;
 
        using head_emit = std::conditional_t<head_matches, _New, _Head>;
 
    public:
        using type = typename replace_or_append_arg<
            _Predicate, _New,
            std::tuple<_Out..., head_emit>,
            (_Replaced || head_matches),
            _Tail...>::type;
    };
 
 
    // option_swap_arg
    //   helper: produce a new option<_Key, ...> with the same args as
    // _Opt except that any arg matching _Predicate is replaced by
    // _NewArg.  If no arg matched, _NewArg is appended.
    template<typename                  _Opt,
             template<typename> class  _Predicate,
             typename                  _NewArg>
    struct option_swap_arg;
 
    template<auto                      _Key,
             typename...               _Args,
             template<typename> class  _Predicate,
             typename                  _NewArg>
    struct option_swap_arg<option<_Key, _Args...>, _Predicate, _NewArg>
    {
    private:
        using new_args_tuple = typename replace_or_append_arg<
            _Predicate, _NewArg,
            std::tuple<>, false,
            _Args...>::type;
 
    public:
        using type = typename rebuild_option_from_tuple<
            _Key, new_args_tuple>::type;
    };
 
    template<auto                      _Key,
             template<typename> class  _Predicate,
             typename                  _NewArg>
    struct option_swap_arg<option<_Key>, _Predicate, _NewArg>
    {
        using type = option<_Key, _NewArg>;
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
//   metafn: concatenated args with _D's first, _B's second.  Because
// option_find_arg / find_arg return the FIRST match, putting _D's
// args first means _D wins for any role queried by predicate (actual,
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
template<typename _B,
         typename _D>
struct merge_args_union
{
    using type = dropped;
};
 
template<auto _Key,
         typename... _BArgs,
         typename... _DArgs>
struct merge_args_union<option<_Key, _BArgs...>, option<_Key, _DArgs...>>
{
    using type = option<_Key, _DArgs..., _BArgs...>;
};
 
template<auto _Key,
         typename... _DArgs>
struct merge_args_union<option<_Key>, option<_Key, _DArgs...>>
{
    using type = option<_Key, _DArgs...>;
};
 
template<auto _Key,
         typename... _BArgs>
struct merge_args_union<option<_Key, _BArgs...>, option<_Key>>
{
    using type = option<_Key, _BArgs...>;
};
 
template<auto _Key>
struct merge_args_union<option<_Key>, option<_Key>>
{
    using type = option<_Key>;
};
 
 
// ===========================================================================
// III. lazy on_delta_only SFINAE
// ===========================================================================
 
NS_INTERNAL
 
    // lazy_delta_only
    //   helper: lazy access to a policy's on_delta_only<_D> alias.
    // When _Drop is true, the policy alias is NEVER instantiated -
    // this is what protects strict_subset's static_assert from firing
    // for keys that already exist in base.  When _Drop is false (the
    // key is genuinely delta-only), the policy alias is instantiated
    // and any embedded assert fires legitimately.
    template<bool     _Drop,
             typename _Policy,
             typename _D>
    struct lazy_delta_only
    {
        using type = typename _Policy::template on_delta_only<_D>;
    };
 
    template<typename _Policy,
             typename _D>
    struct lazy_delta_only<true, _Policy, _D>
    {
        using type = dropped;
    };
 
 
    // append_if_kept
    //   helper: appends _Type to a std::tuple<...> unless _Type is the
    // dropped sentinel.
    template<typename _Tup,
             typename _Type>
    struct append_if_kept
    {
        using type = decltype(
            std::tuple_cat(std::declval<_Tup>(),
                           std::declval<std::tuple<_Type>>()));
    };
 
    template<typename _Tup>
    struct append_if_kept<_Tup, dropped>
    {
        using type = _Tup;
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
    // option_set_find_t are named ONLY in the _InB == true specialization, so a
    // base-only key never instantiates the merge (crucial for merge-based
    // policies such as arg_union_delta) and the concept probe is never engaged.
    template<bool     _InB,
             typename _Policy,
             typename _B,
             typename _Head>
    struct ov_pick
    {
        using type = typename _Policy::template on_base_only<_Head>;
    };

    template<typename _Policy,
             typename _B,
             typename _Head>
    struct ov_pick<true, _Policy, _B, _Head>
    {
        using type = typename _Policy::template on_both<
            _Head,
            option_set_find_t<_B, _Head::key>>;
    };

    // override_walk_a
    //   helper: for each option in A, look it up in B and apply the
    // appropriate policy hook (on_both or on_base_only).
    template<typename    _Policy,
             typename    _B,
             typename    _Acc,
             typename... _AOpts>
    struct override_walk_a
    {
        using type = _Acc;
    };
 
    template<typename    _Policy,
             typename    _B,
             typename    _Acc,
             typename    _Head,
             typename... _Tail>
    struct override_walk_a<_Policy, _B, _Acc, _Head, _Tail...>
    {
    private:
        static constexpr bool in_b =
            option_set_contains_v<_B, _Head::key>;
 
        using produced =
            typename ov_pick<in_b, _Policy, _B, _Head>::type;
 
        using next_acc =
            typename append_if_kept<_Acc, produced>::type;
 
    public:
        using type = typename override_walk_a<
            _Policy, _B, next_acc, _Tail...>::type;
    };
 
 
    // override_walk_b_extras
    //   helper: walk B's options, emit on_delta_only for keys NOT
    // already produced from A.  Uses lazy_delta_only so a strict
    // policy's assert is only triggered when an actual extension is
    // detected.
    template<typename    _Policy,
             typename    _A,
             typename    _Acc,
             typename... _BOpts>
    struct override_walk_b_extras
    {
        using type = _Acc;
    };
 
    template<typename    _Policy,
             typename    _A,
             typename    _Acc,
             typename    _Head,
             typename... _Tail>
    struct override_walk_b_extras<_Policy, _A, _Acc, _Head, _Tail...>
    {
    private:
        static constexpr bool in_a =
            option_set_contains_v<_A, _Head::key>;
 
        // in_a == true  -> already handled by walk_a, drop here.
        // in_a == false -> ask the policy what to do with the extension.
        using produced =
            typename lazy_delta_only<in_a, _Policy, _Head>::type;
 
        using next_acc =
            typename append_if_kept<_Acc, produced>::type;
 
    public:
        using type = typename override_walk_b_extras<
            _Policy, _A, next_acc, _Tail...>::type;
    };
 
 
    // tuple_to_option_set
    //   helper: lift a std::tuple<options...> back into option_set<...>.
    template<typename _Tup>
    struct tuple_to_option_set;
 
    template<typename... _Opts>
    struct tuple_to_option_set<std::tuple<_Opts...>>
    {
        using type = option_set<_Opts...>;
    };
 
NS_END  // internal
 
 
// option_set_override
//   trait: yields option_set<...> = A overridden by B under _Policy.
//   The result preserves A's ordering for keys present in A, with
// B-only extensions (if the policy allows them) appended in B's
// order.  Duplicate keys are caught by option_set's own checks at
// final instantiation - so a policy that produces a colliding key
// is a hard error at the right moment.
template<typename _A,
         typename _B,
         OverridePolicy _Policy>
struct option_set_override;
 
template<typename... _AOpts,
         typename... _BOpts,
         OverridePolicy _Policy>
struct option_set_override<option_set<_AOpts...>,
                           option_set<_BOpts...>,
                           _Policy>
{
private:
    using a_flat = typename option_set<_AOpts...>::flat_options_t;
    using b_flat = typename option_set<_BOpts...>::flat_options_t;
 
    template<typename _Type>
    struct unpack_a;
 
    template<typename... _O>
    struct unpack_a<std::tuple<_O...>>
    {
        using type = typename internal::override_walk_a<
            _Policy, option_set<_BOpts...>, std::tuple<>, _O...>::type;
    };
 
    using after_a = typename unpack_a<a_flat>::type;
 
    template<typename _Type>
    struct unpack_b;
 
    template<typename... _O>
    struct unpack_b<std::tuple<_O...>>
    {
        using type = typename internal::override_walk_b_extras<
            _Policy, option_set<_AOpts...>, after_a, _O...>::type;
    };
 
    using merged_tuple = typename unpack_b<b_flat>::type;
 
public:
    using type =
        typename internal::tuple_to_option_set<merged_tuple>::type;
};
 
template<typename       _A,
         typename       _B,
         OverridePolicy _Policy>
using option_set_override_t =
    typename option_set_override<_A, _B, _Policy>::type;
 
 
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
 
 
#endif  // DJINTERP_OPTION_OVERRIDE_
