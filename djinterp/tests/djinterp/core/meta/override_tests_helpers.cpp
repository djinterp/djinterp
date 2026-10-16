/*******************************************************************************
* djinterp [core]                                     override_tests_helpers.cpp
*
*
* path:      /tests/djinterp/core/meta/override_tests_helpers.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "override_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_override_identity_t
  identity_t: the no-op slot for a combinator that demands a metafunction.
  Tests the following:
  - it yields its argument unchanged for ordinary types and for every
    member of the adversarial zoo, including void, function, array,
    reference and member-pointer types
  - it never requires a COMPLETE type, which is what lets a policy be
    composed for elements that are only forward-declared
  - it agrees with std::type_identity everywhere
  - it is idempotent, and it is accepted as a unary metafunction by all
    three combinators
*/
bool
tests_override_identity_t()
{
    bool ok = true;

    // the basic contract
    D_OV_CHECK(ok, std::is_same_v<identity_t<ov_base>::type, ov_base>);
    D_OV_CHECK(ok, std::is_same_v<identity_t<int>::type, int>);
    D_OV_CHECK(ok, std::is_same_v<identity_t<dropped>::type, dropped>);

    // qualifiers and non-object types survive intact - no decay
    D_OV_CHECK(ok, std::is_same_v<identity_t<const int>::type, const int>);
    D_OV_CHECK(ok, std::is_same_v<identity_t<int&>::type, int&>);
    D_OV_CHECK(ok, std::is_same_v<identity_t<int&&>::type, int&&>);
    D_OV_CHECK(ok, std::is_same_v<identity_t<int[3]>::type, int[3]>);
    D_OV_CHECK(ok, std::is_same_v<identity_t<int(int)>::type, int(int)>);
    D_OV_CHECK(ok, std::is_same_v<identity_t<void>::type, void>);

    // the whole zoo, including the incomplete fixture
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       ov_identity_holds, D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       ov_identity_matches_std,
                       D_TEST_HOSTILE_TYPES>::value);

    // idempotent
    D_OV_CHECK(ok, std::is_same_v<
                       identity_t<identity_t<ov_base>::type>::type,
                       ov_base>);

    // it carries a type and nothing else
    D_OV_CHECK(ok, std::is_empty_v<identity_t<ov_base>>);
    D_OV_CHECK(ok, !::djinterp::test::is_bool_trait<
                       identity_t<ov_base>>::value);

    // accepted as the unary slot of every combinator that takes one
    D_OV_CHECK(ok, ov_binds_unary<identity_t>::value);
    D_OV_CHECK(ok, std::is_same_v<
                       with_on_base_only<keep_base,
                                         identity_t>::on_base_only<ov_base>,
                       ov_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       with_on_delta_only<keep_base,
                                          identity_t>::on_delta_only<
                                              ov_delta>,
                       ov_delta>);

    return ok;
}

/*
tests_override_always_drop
  always_drop: the metafunction that discards whatever it is handed.
  Tests the following:
  - it yields the sentinel for every type, over the whole zoo
  - the result is recognised by is_dropped, which is the pairing an
    engine's filter depends on
  - it is accepted by both unary combinators, and composing it onto a
    hook makes that hook drop
*/
bool
tests_override_always_drop()
{
    bool ok = true;

    // always the sentinel, whatever the argument
    D_OV_CHECK(ok, std::is_same_v<always_drop<ov_base>::type, dropped>);
    D_OV_CHECK(ok, std::is_same_v<always_drop<int>::type, dropped>);
    D_OV_CHECK(ok, std::is_same_v<always_drop<void>::type, dropped>);
    D_OV_CHECK(ok, std::is_same_v<always_drop<dropped>::type, dropped>);
    D_OV_CHECK(ok, std::is_same_v<always_drop<int(int)>::type, dropped>);

    // and the sentinel it yields is the one the filter recognises
    D_OV_CHECK(ok, is_dropped_v<always_drop<ov_base>::type>);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       ov_always_drop_holds,
                       D_TEST_HOSTILE_TYPES>::value);

    // it is the exact opposite of identity_t everywhere but at `dropped`
    D_OV_CHECK(ok, !std::is_same_v<always_drop<ov_base>::type,
                                   identity_t<ov_base>::type>);
    D_OV_CHECK(ok, std::is_same_v<always_drop<dropped>::type,
                                  identity_t<dropped>::type>);

    // accepted as a unary metafunction slot, and it makes a hook drop
    D_OV_CHECK(ok, ov_binds_unary<always_drop>::value);
    D_OV_CHECK(ok, is_dropped_v<
                       with_on_base_only<keep_delta,
                                         always_drop>::on_base_only<
                                             ov_base>>);
    D_OV_CHECK(ok, is_dropped_v<
                       with_on_delta_only<keep_delta,
                                          always_drop>::on_delta_only<
                                              ov_delta>>);

    return ok;
}

/*
tests_override_always_keep_left
  always_keep_left: the binary projection onto the first operand.
  Tests the following:
  - it yields the LEFT operand and ignores the right, for every type on
    either side
  - it is positional, proved by feeding it two different orders of the
    same pair
  - it is accepted by with_on_both, where composing it onto keep_delta
    reproduces keep_base's overlap behaviour
*/
bool
tests_override_always_keep_left()
{
    bool ok = true;

    // projection onto the left
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_left<ov_base, ov_delta>::type, ov_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_left<ov_delta, ov_base>::type, ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_left<ov_base, ov_base>::type, ov_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_left<void, int>::type, void>);
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_left<dropped, ov_base>::type, dropped>);

    // every type works in the kept position, and in the ignored one
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       ov_keep_left_holds, D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       ov_keep_left_ignores_right,
                       D_TEST_HOSTILE_TYPES>::value);

    // accepted by with_on_both, where it reproduces keep_base's overlap
    D_OV_CHECK(ok, ov_binds_binary<always_keep_left>::value);
    D_OV_CHECK(ok, std::is_same_v<
                       with_on_both<keep_delta,
                                    always_keep_left>::on_both<ov_base,
                                                               ov_delta>,
                       keep_base::on_both<ov_base, ov_delta>>);

    return ok;
}

/*
tests_override_always_keep_right
  always_keep_right: the binary projection onto the second operand.
  Tests the following:
  - it yields the RIGHT operand and ignores the left, for every type on
    either side
  - it is the mirror of always_keep_left: the two disagree on every
    distinct pair and agree on every identical one
  - composing it onto keep_base reproduces keep_delta's overlap behaviour
*/
bool
tests_override_always_keep_right()
{
    bool ok = true;

    // projection onto the right
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_right<ov_base, ov_delta>::type,
                       ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_right<ov_delta, ov_base>::type,
                       ov_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_right<ov_base, ov_base>::type, ov_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_right<int, void>::type, void>);
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_right<ov_base, dropped>::type, dropped>);

    // every type works in the kept position, and in the ignored one
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       ov_keep_right_holds, D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       ov_keep_right_ignores_left,
                       D_TEST_HOSTILE_TYPES>::value);

    // the exact mirror of always_keep_left
    D_OV_CHECK(ok, !std::is_same_v<
                       always_keep_right<ov_base, ov_delta>::type,
                       always_keep_left<ov_base, ov_delta>::type>);
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_right<ov_base, ov_base>::type,
                       always_keep_left<ov_base, ov_base>::type>);
    D_OV_CHECK(ok, std::is_same_v<
                       always_keep_right<ov_base, ov_delta>::type,
                       always_keep_left<ov_delta, ov_base>::type>);

    // accepted by with_on_both, where it reproduces keep_delta's overlap
    D_OV_CHECK(ok, ov_binds_binary<always_keep_right>::value);
    D_OV_CHECK(ok, std::is_same_v<
                       with_on_both<keep_base,
                                    always_keep_right>::on_both<ov_base,
                                                                ov_delta>,
                       keep_delta::on_both<ov_base, ov_delta>>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
