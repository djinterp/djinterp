/******************************************************************************
* djinterp [test]                               option_override_tests_lazy.cpp
*
*   Section III of the option_override.hpp suite: the lazy on_delta_only SFINAE
* plumbing and the accumulator helper.
*
*     internal::lazy_delta_only<_Drop, _Policy, _D>
*         - when _Drop is true, yields `dropped` and NEVER touches
*           _Policy::on_delta_only (this is what keeps strict_subset's
*           static_assert from firing for a key that already exists in base);
*         - when _Drop is false, yields _Policy::on_delta_only<_D> (a real
*           delta-only key, so an embedded assert is allowed to fire).
*     internal::append_if_kept<_Tup, _Type>
*         - tuple_cat-appends _Type to _Tup, unless _Type is `dropped`, in
*           which case _Tup passes through unchanged.
*
*   The guarded-strict test is the important one: it instantiates
* lazy_delta_only<true, strict_subset, D> and asserts it yields `dropped`
* WITHOUT triggering strict_subset's extension static_assert - the exact
* laziness the engine relies on.  (`dropped` and the policy primitives are
* from meta/override.hpp, reached transitively through option_override.hpp.)
*
*
* path:      /tests/djinterp/core/option/option_override_tests_lazy.cpp
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

// lazy_drop_true_yields_dropped
//   _Drop == true short-circuits to `dropped` (the policy's on_delta_only is
// not consulted at all).
bool
lazy_drop_true_yields_dropped()
{
    constexpr bool ok =
        std::is_same<
            typename ih::lazy_delta_only<true, keep_delta, option<ov_key::a, ov_p>>::type,
            dropped>::value;

    static_assert(ok, "lazy_delta_only<true, ...> -> dropped (policy hook untouched)");
    return ok;
}

// lazy_false_keep_delta_keeps
//   _Drop == false delegates to the policy: keep_delta keeps the delta option.
bool
lazy_false_keep_delta_keeps()
{
    constexpr bool ok =
        std::is_same<
            typename ih::lazy_delta_only<false, keep_delta, option<ov_key::a, ov_p>>::type,
            option<ov_key::a, ov_p>>::value;

    static_assert(ok, "lazy_delta_only<false, keep_delta, D> -> D");
    return ok;
}

// lazy_false_keep_base_drops
//   keep_base drops delta-only keys.
bool
lazy_false_keep_base_drops()
{
    constexpr bool ok =
        std::is_same<
            typename ih::lazy_delta_only<false, keep_base, option<ov_key::a, ov_p>>::type,
            dropped>::value;

    static_assert(ok, "lazy_delta_only<false, keep_base, D> -> dropped");
    return ok;
}

// lazy_false_drop_extras_drops
//   drop_extras also drops delta-only keys (overlap-only delta).
bool
lazy_false_drop_extras_drops()
{
    constexpr bool ok =
        std::is_same<
            typename ih::lazy_delta_only<false, drop_extras, option<ov_key::a, ov_p>>::type,
            dropped>::value;

    static_assert(ok, "lazy_delta_only<false, drop_extras, D> -> dropped");
    return ok;
}

// lazy_false_drop_unmatched_base_keeps
//   drop_unmatched_base keeps delta-only keys (only delta's keys survive).
bool
lazy_false_drop_unmatched_base_keeps()
{
    constexpr bool ok =
        std::is_same<
            typename ih::lazy_delta_only<false, drop_unmatched_base, option<ov_key::a, ov_p>>::type,
            option<ov_key::a, ov_p>>::value;

    static_assert(ok, "lazy_delta_only<false, drop_unmatched_base, D> -> D");
    return ok;
}

// lazy_true_guards_strict_assert
//   THE laziness guarantee: with strict_subset (whose on_delta_only is a hard
// static_assert), _Drop == true must still yield `dropped` and must NOT fire
// that assert - because the true-specialization never instantiates the hook.
// If the guard regressed, THIS TU would fail to compile with strict_subset's
// extension message, which is exactly the regression signal we want.
bool
lazy_true_guards_strict_assert()
{
    constexpr bool ok =
        std::is_same<
            typename ih::lazy_delta_only<true, strict_subset, option<ov_key::a, ov_p>>::type,
            dropped>::value;

    static_assert(ok, "lazy_delta_only<true, strict_subset, D> -> dropped without firing the assert");
    return ok;
}

// append_if_kept_appends
//   a non-dropped type is tuple_cat-appended, whether the accumulator is
// non-empty or empty.
bool
append_if_kept_appends()
{
    constexpr bool ok =
        std::is_same<
            typename ih::append_if_kept<std::tuple<ov_p, ov_q>, ov_r>::type,
            std::tuple<ov_p, ov_q, ov_r>>::value                                &&
        std::is_same<
            typename ih::append_if_kept<std::tuple<>, ov_r>::type,
            std::tuple<ov_r>>::value;

    static_assert(ok, "append_if_kept<Tup, T> -> tuple_cat(Tup, tuple<T>)");
    return ok;
}

// append_if_kept_skips_dropped
//   the `dropped` specialization passes the accumulator through unchanged,
// whether non-empty or empty.
bool
append_if_kept_skips_dropped()
{
    constexpr bool ok =
        std::is_same<
            typename ih::append_if_kept<std::tuple<ov_p, ov_q>, dropped>::type,
            std::tuple<ov_p, ov_q>>::value                                      &&
        std::is_same<
            typename ih::append_if_kept<std::tuple<>, dropped>::type,
            std::tuple<>>::value;

    static_assert(ok, "append_if_kept<Tup, dropped> -> Tup (unchanged)");
    return ok;
}

#endif  // C++20 concepts available


// ---------------------------------------------------------------------------
// block provider  (empty below C++20)
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_override_lazy_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "III. lazy on_delta_only + append_if_kept";
    b.descriptor = "lazy policy-hook access (guards strict assert) and dropped-aware accumulation";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.tests = {
        { "lazy_drop_true_yields_dropped",
          "lazy_delta_only<true, ...> -> dropped (hook untouched)",
          &lazy_drop_true_yields_dropped },
        { "lazy_false_keep_delta_keeps",
          "lazy_delta_only<false, keep_delta, D> -> D",
          &lazy_false_keep_delta_keeps },
        { "lazy_false_keep_base_drops",
          "lazy_delta_only<false, keep_base, D> -> dropped",
          &lazy_false_keep_base_drops },
        { "lazy_false_drop_extras_drops",
          "lazy_delta_only<false, drop_extras, D> -> dropped",
          &lazy_false_drop_extras_drops },
        { "lazy_false_drop_unmatched_base_keeps",
          "lazy_delta_only<false, drop_unmatched_base, D> -> D",
          &lazy_false_drop_unmatched_base_keeps },
        { "lazy_true_guards_strict_assert",
          "lazy_delta_only<true, strict_subset, D> -> dropped, assert NOT fired",
          &lazy_true_guards_strict_assert },
        { "append_if_kept_appends",
          "append_if_kept<Tup, T> -> tuple_cat(Tup, tuple<T>)",
          &append_if_kept_appends },
        { "append_if_kept_skips_dropped",
          "append_if_kept<Tup, dropped> -> Tup unchanged",
          &append_if_kept_skips_dropped },
    };
#else
    b.descriptor = "III. lazy on_delta_only + append_if_kept (skipped: requires C++20)";
#endif

    return b;
}


NS_END  // testing
NS_END  // djinterp
