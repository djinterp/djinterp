/*******************************************************************************
* djinterp [core]                                  override_tests_primitives.cpp
*
*
* path:      /tests/djinterp/core/meta/override_tests_primitives.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "override_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_override_keep_base
  keep_base: base wins everywhere, delta-only keys are discarded.
  Tests the following:
  - on_base_only yields the base element, on_both yields the BASE operand,
    on_delta_only yields the sentinel
  - the answers are POSITIONAL: re-running the hooks with the operands
    swapped still yields the first operand, so the policy cannot be
    passing by coincidence of type
  - the identity case (both operands the same) collapses to that type
  - the hooks are element-agnostic: they behave the same over the whole
    adversarial zoo, including void, function, array, reference and
    incomplete types
*/
bool
tests_override_keep_base()
{
    bool ok = true;

    using h = ov_hooks<keep_base, ov_base, ov_delta>;

    D_OV_CHECK(ok, std::is_same_v<h::base_only,  ov_base>);
    D_OV_CHECK(ok, std::is_same_v<h::delta_only, dropped>);
    D_OV_CHECK(ok, std::is_same_v<h::both,       ov_base>);

    // positional, not type-matched: swap the operands and on_both must
    // follow the BASE position, not the type it saw last time
    using s = ov_hooks<keep_base, ov_delta, ov_base>;

    D_OV_CHECK(ok, std::is_same_v<s::base_only, ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<s::both,      ov_delta>);
    D_OV_CHECK(ok, !std::is_same_v<s::both,     ov_base>);

    // the identity case
    D_OV_CHECK(ok, std::is_same_v<
                       keep_base::on_both<ov_third, ov_third>, ov_third>);

    // element-agnostic across the zoo
    using bind = ov_hook_binder<keep_base>;

    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::base_only_is_identity,
                       D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::both_is_identity,
                       D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::delta_only_is_dropped,
                       D_TEST_HOSTILE_TYPES>::value);

    return ok;
}

/*
tests_override_keep_delta
  keep_delta: the standard override - delta wins, extensions allowed.
  Tests the following:
  - on_base_only yields base, on_both yields the DELTA operand,
    on_delta_only yields the delta element rather than the sentinel
  - the answers are positional under a swap
  - nothing is ever dropped by this policy, for any element type
  - it differs from keep_base exactly at on_both and on_delta_only
*/
bool
tests_override_keep_delta()
{
    bool ok = true;

    using h = ov_hooks<keep_delta, ov_base, ov_delta>;

    D_OV_CHECK(ok, std::is_same_v<h::base_only,  ov_base>);
    D_OV_CHECK(ok, std::is_same_v<h::delta_only, ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<h::both,       ov_delta>);

    // positional under a swap
    using s = ov_hooks<keep_delta, ov_delta, ov_base>;

    D_OV_CHECK(ok, std::is_same_v<s::base_only,  ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<s::delta_only, ov_base>);
    D_OV_CHECK(ok, std::is_same_v<s::both,       ov_base>);

    // the identity case
    D_OV_CHECK(ok, std::is_same_v<
                       keep_delta::on_both<ov_third, ov_third>, ov_third>);

    // this policy never drops anything, whatever the element
    using bind = ov_hook_binder<keep_delta>;

    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::base_only_is_identity,
                       D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::delta_only_is_identity,
                       D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::both_is_identity,
                       D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_none<
                       bind::delta_only_is_dropped,
                       D_TEST_HOSTILE_TYPES>::value);

    // and it differs from keep_base exactly where documented
    D_OV_CHECK(ok, std::is_same_v<keep_delta::on_base_only<ov_base>,
                                  keep_base::on_base_only<ov_base>>);
    D_OV_CHECK(ok, !std::is_same_v<
                       keep_delta::on_both<ov_base, ov_delta>,
                       keep_base::on_both<ov_base, ov_delta>>);
    D_OV_CHECK(ok, !std::is_same_v<keep_delta::on_delta_only<ov_delta>,
                                   keep_base::on_delta_only<ov_delta>>);

    return ok;
}

/*
tests_override_drop_extras
  drop_extras: delta wins on overlap but may not introduce new keys.
  Tests the following:
  - the hook triple is base / dropped / delta
  - it agrees with keep_delta on the two overlap hooks and with keep_base
    on the extension hook - which is precisely the definition of "the
    third combination", and is stated here as an equality against those
    two policies rather than restated by hand
  - the behaviour holds for every element type
*/
bool
tests_override_drop_extras()
{
    bool ok = true;

    using h = ov_hooks<drop_extras, ov_base, ov_delta>;

    D_OV_CHECK(ok, std::is_same_v<h::base_only,  ov_base>);
    D_OV_CHECK(ok, std::is_same_v<h::delta_only, dropped>);
    D_OV_CHECK(ok, std::is_same_v<h::both,       ov_delta>);

    // positional under a swap
    using s = ov_hooks<drop_extras, ov_delta, ov_base>;

    D_OV_CHECK(ok, std::is_same_v<s::base_only, ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<s::both,      ov_base>);

    // it is exactly "keep_delta on overlap, keep_base on extension"
    D_OV_CHECK(ok, std::is_same_v<
                       drop_extras::on_both<ov_base, ov_delta>,
                       keep_delta::on_both<ov_base, ov_delta>>);
    D_OV_CHECK(ok, std::is_same_v<
                       drop_extras::on_base_only<ov_base>,
                       keep_delta::on_base_only<ov_base>>);
    D_OV_CHECK(ok, std::is_same_v<
                       drop_extras::on_delta_only<ov_delta>,
                       keep_base::on_delta_only<ov_delta>>);

    // element-agnostic
    using bind = ov_hook_binder<drop_extras>;

    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::base_only_is_identity,
                       D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::both_is_identity,
                       D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::delta_only_is_dropped,
                       D_TEST_HOSTILE_TYPES>::value);

    return ok;
}

/*
tests_override_strict_subset
  strict_subset: drop_extras, but an extension is a hard compile error.
  Tests the following:
  - the two hooks that CAN be instantiated behave exactly as drop_extras'
    do, over the whole element zoo
  - on_delta_only and its assert_extension helper exist and have unary
    arity, proved by BINDING them as template template arguments, which
    names them without instantiating them
  - neither the concept, nor binding, nor a lazy engine wrapper fires the
    assert - the whole of this function compiling is that claim
  NOTE: `assert_extension<_D>::type` is deliberately unreachable.  Its
  static_assert holds for no type a C++ program can name (nothing has
  sizeof 0), and a failed static_assert is a hard error rather than a
  substitution failure, so it can be neither caught nor probed.  Testing
  it would mean failing the build on purpose; the contract AROUND it is
  what is testable, and is what is tested here.
*/
bool
tests_override_strict_subset()
{
    bool ok = true;

    // only the probed hooks may be read
    using h = ov_probed_hooks<strict_subset, ov_base, ov_delta>;

    D_OV_CHECK(ok, std::is_same_v<h::base_only, ov_base>);
    D_OV_CHECK(ok, std::is_same_v<h::both,      ov_delta>);

    // positional under a swap
    using s = ov_probed_hooks<strict_subset, ov_delta, ov_base>;

    D_OV_CHECK(ok, std::is_same_v<s::base_only, ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<s::both,      ov_base>);

    // identical to drop_extras wherever it can be observed
    D_OV_CHECK(ok, std::is_same_v<
                       strict_subset::on_both<ov_base, ov_delta>,
                       drop_extras::on_both<ov_base, ov_delta>>);
    D_OV_CHECK(ok, std::is_same_v<
                       strict_subset::on_base_only<ov_base>,
                       drop_extras::on_base_only<ov_base>>);

    // ... over the whole element zoo
    using bind = ov_hook_binder<strict_subset>;

    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::base_only_is_identity,
                       D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::both_is_identity,
                       D_TEST_HOSTILE_TYPES>::value);

    // the strict hooks exist, with unary arity, and are only NAMED
    D_OV_CHECK(ok, ov_binds_unary<strict_subset::on_delta_only>::value);
    D_OV_CHECK(ok, ov_binds_unary<strict_subset::assert_extension>::value);

    // a lazy engine never consults them for a key base already carries
    D_OV_CHECK(ok, std::is_same_v<
                       ov_lazy_delta_only<true, strict_subset,
                                          ov_delta>::type,
                       dropped>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_lazy_delta_only<true, strict_subset,
                                          void>::type,
                       dropped>);

    // and the type is still an ordinary empty policy tag
    D_OV_CHECK(ok, std::is_empty_v<strict_subset>);
    D_OV_CHECK(ok, OverridePolicy<strict_subset>);

    return ok;
}

/*
tests_override_drop_unmatched_base
  drop_unmatched_base: filter to delta - only delta's keys survive.
  Tests the following:
  - it is the only primitive whose on_base_only drops, and it drops for
    every element type
  - on_both and on_delta_only both yield the delta element, so the result
    is value-equal to delta on the surviving keys
  - it agrees with keep_delta on both delta-facing hooks and disagrees
    with every other primitive on on_base_only
*/
bool
tests_override_drop_unmatched_base()
{
    bool ok = true;

    using h = ov_hooks<drop_unmatched_base, ov_base, ov_delta>;

    D_OV_CHECK(ok, std::is_same_v<h::base_only,  dropped>);
    D_OV_CHECK(ok, std::is_same_v<h::delta_only, ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<h::both,       ov_delta>);

    // positional under a swap
    using s = ov_hooks<drop_unmatched_base, ov_delta, ov_base>;

    D_OV_CHECK(ok, std::is_same_v<s::base_only,  dropped>);
    D_OV_CHECK(ok, std::is_same_v<s::delta_only, ov_base>);
    D_OV_CHECK(ok, std::is_same_v<s::both,       ov_base>);

    // it is keep_delta on both delta-facing hooks
    D_OV_CHECK(ok, std::is_same_v<
                       drop_unmatched_base::on_both<ov_base, ov_delta>,
                       keep_delta::on_both<ov_base, ov_delta>>);
    D_OV_CHECK(ok, std::is_same_v<
                       drop_unmatched_base::on_delta_only<ov_delta>,
                       keep_delta::on_delta_only<ov_delta>>);

    // ... and the only one that discards an unmatched base element
    D_OV_CHECK(ok, !std::is_same_v<
                       drop_unmatched_base::on_base_only<ov_base>,
                       keep_base::on_base_only<ov_base>>);
    D_OV_CHECK(ok, !std::is_same_v<
                       drop_unmatched_base::on_base_only<ov_base>,
                       keep_delta::on_base_only<ov_base>>);
    D_OV_CHECK(ok, !std::is_same_v<
                       drop_unmatched_base::on_base_only<ov_base>,
                       drop_extras::on_base_only<ov_base>>);

    // element-agnostic
    using bind = ov_hook_binder<drop_unmatched_base>;

    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::base_only_is_dropped,
                       D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::delta_only_is_identity,
                       D_TEST_HOSTILE_TYPES>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       bind::both_is_identity,
                       D_TEST_HOSTILE_TYPES>::value);

    return ok;
}

/*
tests_override_primitive_shapes
  The primitives AS TYPES, and the matrix that separates them.
  Tests the following:
  - each is an empty, stateless, trivially constructible tag, so a policy
    may be passed and stored for nothing
  - the five are pairwise distinct types
  - no two of them agree on all three hooks: the matrix below is the
    statement that the module ships five genuinely different policies and
    not four plus a duplicate
*/
bool
tests_override_primitive_shapes()
{
    bool ok = true;

    // stateless tags
    D_OV_CHECK(ok, std::is_empty_v<keep_base>);
    D_OV_CHECK(ok, std::is_empty_v<keep_delta>);
    D_OV_CHECK(ok, std::is_empty_v<drop_extras>);
    D_OV_CHECK(ok, std::is_empty_v<strict_subset>);
    D_OV_CHECK(ok, std::is_empty_v<drop_unmatched_base>);

    D_OV_CHECK(ok, std::is_trivially_default_constructible_v<keep_base>);
    D_OV_CHECK(ok, std::is_trivially_copyable_v<keep_delta>);
    D_OV_CHECK(ok, std::is_trivially_destructible_v<drop_extras>);
    D_OV_CHECK(ok, std::is_aggregate_v<drop_unmatched_base>);
    D_OV_CHECK(ok, sizeof(keep_base) == 1);

    // pairwise distinct
    D_OV_CHECK(ok, !std::is_same_v<keep_base, keep_delta>);
    D_OV_CHECK(ok, !std::is_same_v<keep_base, drop_extras>);
    D_OV_CHECK(ok, !std::is_same_v<keep_delta, drop_extras>);
    D_OV_CHECK(ok, !std::is_same_v<drop_extras, strict_subset>);
    D_OV_CHECK(ok, !std::is_same_v<keep_delta, drop_unmatched_base>);
    D_OV_CHECK(ok, !std::is_same_v<strict_subset, drop_unmatched_base>);

    // the on_base_only column: only drop_unmatched_base discards
    D_OV_CHECK(ok, std::is_same_v<keep_base::on_base_only<ov_base>,
                                  ov_base>);
    D_OV_CHECK(ok, std::is_same_v<keep_delta::on_base_only<ov_base>,
                                  ov_base>);
    D_OV_CHECK(ok, std::is_same_v<drop_extras::on_base_only<ov_base>,
                                  ov_base>);
    D_OV_CHECK(ok, std::is_same_v<strict_subset::on_base_only<ov_base>,
                                  ov_base>);
    D_OV_CHECK(ok, is_dropped_v<
                       drop_unmatched_base::on_base_only<ov_base>>);

    // the on_both column: only keep_base returns the base operand
    D_OV_CHECK(ok, std::is_same_v<
                       keep_base::on_both<ov_base, ov_delta>, ov_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       keep_delta::on_both<ov_base, ov_delta>, ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<
                       drop_extras::on_both<ov_base, ov_delta>, ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<
                       strict_subset::on_both<ov_base, ov_delta>, ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<
                       drop_unmatched_base::on_both<ov_base, ov_delta>,
                       ov_delta>);

    // the on_delta_only column (strict_subset omitted - see its own test)
    D_OV_CHECK(ok, is_dropped_v<keep_base::on_delta_only<ov_delta>>);
    D_OV_CHECK(ok, is_dropped_v<drop_extras::on_delta_only<ov_delta>>);
    D_OV_CHECK(ok, !is_dropped_v<keep_delta::on_delta_only<ov_delta>>);
    D_OV_CHECK(ok, !is_dropped_v<
                       drop_unmatched_base::on_delta_only<ov_delta>>);

    // keep_base and drop_extras share two hooks but differ on the third,
    // which is the closest pair in the matrix
    D_OV_CHECK(ok, std::is_same_v<
                       keep_base::on_delta_only<ov_delta>,
                       drop_extras::on_delta_only<ov_delta>>);
    D_OV_CHECK(ok, !std::is_same_v<
                       keep_base::on_both<ov_base, ov_delta>,
                       drop_extras::on_both<ov_base, ov_delta>>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
