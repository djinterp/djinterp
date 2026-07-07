/******************************************************************************
* djinterp [test]                             option_override_tests_helpers.cpp
*
*   Section I of the option_override.hpp suite: the internal option
* (re)construction helpers.
*
*     internal::option_args_as_tuple<_Opt>   - option<>'s args as a tuple
*                                               (empty tuple for a unary option)
*     internal::rebuild_option_from_tuple     - inverse: option<_Key, Args...>
*                                               from a key + a tuple of args
*     internal::replace_or_append_arg          - walk args, replace each match
*                                               with _New; append _New if none
*     internal::option_swap_arg                - the above lifted to an option:
*                                               swap a matching arg, else append
*
*   These are `internal::` implementation details.  The header notes that the
* swap family is currently unused (it belonged to the retired actual<>
* value-merge feature) but is left in place; the suite still pins its
* behavior so the code stays correct if a consumer is reintroduced, and so the
* section reaches full coverage.  Detection uses the fixtures' scoped key enum
* and a one-arg predicate (ov_is_swap_target) declared in the test header.
*
*
* path:      /tests/djinterp/core/option/option_override_tests_helpers.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// std
#include <tuple>
#include <type_traits>
// djinterp
#include "option_override_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace  // internal-helper alias, local to this TU
{
    namespace ih = ::djinterp::internal;
}

// helper_args_as_tuple
//   option_args_as_tuple_t yields the option's args as a std::tuple: an empty
// tuple for a unary option (the primary template, which a unary option<K>
// falls through to since it lacks a _First), and the exact ordered arg tuple
// for an args option.
bool
helper_args_as_tuple()
{
    constexpr bool ok =
        std::is_same<ih::option_args_as_tuple_t<option<ov_key::a>>,
                     std::tuple<>>::value                                        &&
        std::is_same<ih::option_args_as_tuple_t<option<ov_key::a, ov_p>>,
                     std::tuple<ov_p>>::value                                    &&
        std::is_same<ih::option_args_as_tuple_t<option<ov_key::a, ov_p, ov_q>>,
                     std::tuple<ov_p, ov_q>>::value;

    static_assert(ok, "option_args_as_tuple: unary -> tuple<>, args -> ordered tuple");
    return ok;
}

// helper_rebuild_option
//   rebuild_option_from_tuple reconstructs option<_Key, Args...> from a key
// and a tuple of arg types: the empty-tuple specialization gives the unary
// option; the general specialization spreads the tuple into the arg pack.
bool
helper_rebuild_option()
{
    constexpr bool ok =
        std::is_same<typename ih::rebuild_option_from_tuple<ov_key::a, std::tuple<>>::type,
                     option<ov_key::a>>::value                                   &&
        std::is_same<typename ih::rebuild_option_from_tuple<ov_key::a, std::tuple<ov_p>>::type,
                     option<ov_key::a, ov_p>>::value                             &&
        std::is_same<typename ih::rebuild_option_from_tuple<ov_key::a, std::tuple<ov_p, ov_q>>::type,
                     option<ov_key::a, ov_p, ov_q>>::value;

    static_assert(ok, "rebuild_option_from_tuple: key + arg tuple -> option<key, args...>");
    return ok;
}

// helper_args_rebuild_roundtrip
//   the two helpers are inverses for a known key: decomposing an option's
// args and rebuilding from them recovers the original option type.
bool
helper_args_rebuild_roundtrip()
{
    using opt = option<ov_key::b, ov_p, ov_q, ov_r>;

    constexpr bool ok =
        std::is_same<
            typename ih::rebuild_option_from_tuple<
                ov_key::b, ih::option_args_as_tuple_t<opt>>::type,
            opt>::value;

    static_assert(ok, "rebuild_option_from_tuple o option_args_as_tuple == identity");
    return ok;
}

// helper_replace_or_append_arg
//   replace_or_append_arg walks the input args, replacing every arg the
// predicate matches with _New: a single interior match is swapped in place,
// no match appends _New once at the end, and multiple matches are each
// replaced (no early stop).
bool
helper_replace_or_append_arg()
{
    // interior match: ov_swap_target -> ov_swap_new
    using replaced_mid = typename ih::replace_or_append_arg<
        ov_is_swap_target, ov_swap_new, std::tuple<>, false,
        ov_p, ov_swap_target, ov_q>::type;

    // no match: ov_swap_new appended once
    using appended = typename ih::replace_or_append_arg<
        ov_is_swap_target, ov_swap_new, std::tuple<>, false,
        ov_p, ov_q>::type;

    // several matches: each replaced
    using replaced_all = typename ih::replace_or_append_arg<
        ov_is_swap_target, ov_swap_new, std::tuple<>, false,
        ov_swap_target, ov_swap_target>::type;

    constexpr bool ok =
        std::is_same<replaced_mid, std::tuple<ov_p, ov_swap_new, ov_q>>::value  &&
        std::is_same<appended,     std::tuple<ov_p, ov_q, ov_swap_new>>::value  &&
        std::is_same<replaced_all, std::tuple<ov_swap_new, ov_swap_new>>::value;

    static_assert(ok, "replace_or_append_arg: replace matches, else append once");
    return ok;
}

// helper_replace_or_append_base_cases
//   the base case's conditional both ways: with empty input and no prior
// match, _New is appended (tuple<_New>); with empty input but _Replaced
// already set, nothing is appended (tuple<>).
bool
helper_replace_or_append_base_cases()
{
    using empty_append = typename ih::replace_or_append_arg<
        ov_is_swap_target, ov_swap_new, std::tuple<>, false>::type;

    using empty_noappend = typename ih::replace_or_append_arg<
        ov_is_swap_target, ov_swap_new, std::tuple<>, true>::type;

    constexpr bool ok =
        std::is_same<empty_append,   std::tuple<ov_swap_new>>::value  &&
        std::is_same<empty_noappend, std::tuple<>>::value;

    static_assert(ok, "replace_or_append_arg base case: append iff not already replaced");
    return ok;
}

// helper_option_swap_arg
//   option_swap_arg lifts the arg swap onto an option: a matching arg is
// replaced in the reconstructed option, a non-matching option has _NewArg
// appended, and the unary specialization simply gains _NewArg as its sole arg.
bool
helper_option_swap_arg()
{
    constexpr bool ok =
        std::is_same<
            typename ih::option_swap_arg<
                option<ov_key::a, ov_p, ov_swap_target, ov_q>,
                ov_is_swap_target, ov_swap_new>::type,
            option<ov_key::a, ov_p, ov_swap_new, ov_q>>::value                  &&
        std::is_same<
            typename ih::option_swap_arg<
                option<ov_key::a, ov_p, ov_q>,
                ov_is_swap_target, ov_swap_new>::type,
            option<ov_key::a, ov_p, ov_q, ov_swap_new>>::value                  &&
        std::is_same<
            typename ih::option_swap_arg<
                option<ov_key::a>,
                ov_is_swap_target, ov_swap_new>::type,
            option<ov_key::a, ov_swap_new>>::value;

    static_assert(ok, "option_swap_arg: swap matching arg, else append; unary -> option<key,new>");
    return ok;
}

#endif  // C++20 concepts available


// ---------------------------------------------------------------------------
// block provider  (empty below C++20)
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_override_helpers_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "I. (re)construction helpers";
    b.descriptor = "internal option<->tuple helpers: args_as_tuple, rebuild, replace/append, swap";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.tests = {
        { "helper_args_as_tuple",
          "option_args_as_tuple: unary -> tuple<>, args -> ordered arg tuple",
          &helper_args_as_tuple },
        { "helper_rebuild_option",
          "rebuild_option_from_tuple: key + arg tuple -> option<key, args...>",
          &helper_rebuild_option },
        { "helper_args_rebuild_roundtrip",
          "rebuild o args_as_tuple recovers the original option",
          &helper_args_rebuild_roundtrip },
        { "helper_replace_or_append_arg",
          "replace_or_append_arg: replace every match, else append _New once",
          &helper_replace_or_append_arg },
        { "helper_replace_or_append_base_cases",
          "replace_or_append_arg base case appends iff not already replaced",
          &helper_replace_or_append_base_cases },
        { "helper_option_swap_arg",
          "option_swap_arg: swap matching arg / append / unary -> option<key,new>",
          &helper_option_swap_arg },
    };
#else
    b.descriptor = "I. (re)construction helpers (skipped: option_override.hpp requires C++20)";
#endif

    return b;
}


NS_END  // testing
NS_END  // djinterp
