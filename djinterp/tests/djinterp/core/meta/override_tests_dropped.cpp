/*******************************************************************************
* djinterp [core]                                     override_tests_dropped.cpp
*
*
* path:      /tests/djinterp/core/meta/override_tests_dropped.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "override_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_override_dropped_shape
  The `dropped` sentinel's own type properties.
  Tests the following:
  - it is a complete, empty class type occupying one byte
  - it is trivial in every respect (default construction, copy, move,
    destruction) so an engine may hold or discard it freely
  - it is an aggregate of standard layout
  - it is distinct from every element type a policy could return
  - it does NOT convert to bool, so a `dropped` result can never be
    mistaken for a truthy value by an engine that forgets to test for it
*/
bool
tests_override_dropped_shape()
{
    bool ok = true;

    // a complete, empty, one-byte class type
    D_OV_CHECK(ok, std::is_class_v<dropped>);
    D_OV_CHECK(ok, std::is_empty_v<dropped>);
    D_OV_CHECK(ok, sizeof(dropped) == 1);

    // trivial throughout - the sentinel costs nothing to carry
    D_OV_CHECK(ok, std::is_trivially_default_constructible_v<dropped>);
    D_OV_CHECK(ok, std::is_trivially_copy_constructible_v<dropped>);
    D_OV_CHECK(ok, std::is_trivially_move_constructible_v<dropped>);
    D_OV_CHECK(ok, std::is_trivially_copy_assignable_v<dropped>);
    D_OV_CHECK(ok, std::is_trivially_destructible_v<dropped>);
    D_OV_CHECK(ok, std::is_nothrow_default_constructible_v<dropped>);

    // aggregate, standard layout, no polymorphism
    D_OV_CHECK(ok, std::is_aggregate_v<dropped>);
    D_OV_CHECK(ok, std::is_standard_layout_v<dropped>);
    D_OV_CHECK(ok, !std::is_polymorphic_v<dropped>);
    D_OV_CHECK(ok, !std::is_final_v<dropped>);

    // distinct from every element a policy could hand back
    D_OV_CHECK(ok, !std::is_same_v<dropped, ov_base>);
    D_OV_CHECK(ok, !std::is_same_v<dropped, ov_delta>);
    D_OV_CHECK(ok, !std::is_same_v<dropped, void>);
    D_OV_CHECK(ok, std::is_same_v<dropped, dropped>);

    // no silent conversion to a truth value
    D_OV_CHECK(ok, !std::is_convertible_v<dropped, bool>);
    D_OV_CHECK(ok, !std::is_convertible_v<bool, dropped>);
    D_OV_CHECK(ok, !std::is_constructible_v<dropped, int>);

    return ok;
}

/*
tests_override_is_dropped_trait
  The is_dropped detector's answers and its SHAPE.
  Tests the following:
  - it is true for `dropped` and false for ordinary element types
  - it satisfies the standard bool-trait contract (value_type, type,
    derivation from bool_constant, conversion and call operators),
    which is what lets a downstream engine use it as a constraint
  - it derives from std::is_same, as documented
  - is_dropped_v agrees with is_dropped<...>::value
*/
bool
tests_override_is_dropped_trait()
{
    bool ok = true;

    // the answers
    D_OV_CHECK(ok, is_dropped<dropped>::value);
    D_OV_CHECK(ok, !is_dropped<ov_base>::value);
    D_OV_CHECK(ok, !is_dropped<ov_delta>::value);
    D_OV_CHECK(ok, !is_dropped<int>::value);
    D_OV_CHECK(ok, !is_dropped<void>::value);

    // documented implementation: it IS std::is_same against the sentinel
    D_OV_CHECK(ok, std::is_base_of_v<std::is_same<dropped, dropped>,
                                     is_dropped<dropped>>);
    D_OV_CHECK(ok, std::is_base_of_v<std::is_same<int, dropped>,
                                     is_dropped<int>>);

    // the bool-trait contract, both polarities
    D_OV_CHECK(ok, ::djinterp::test::is_bool_trait<
                       is_dropped<dropped>>::value);
    D_OV_CHECK(ok, ::djinterp::test::is_bool_trait<
                       is_dropped<int>>::value);

    // the members that contract names
    D_OV_CHECK(ok, std::is_same_v<is_dropped<dropped>::value_type, bool>);
    D_OV_CHECK(ok, std::is_same_v<is_dropped<dropped>::type,
                                  std::true_type>);
    D_OV_CHECK(ok, std::is_same_v<is_dropped<int>::type, std::false_type>);
    D_OV_CHECK(ok, is_dropped<dropped>()());
    D_OV_CHECK(ok, static_cast<bool>(is_dropped<dropped>()));

    // the _v companion has not drifted from the trait
    D_OV_CHECK(ok, is_dropped_v<dropped>);
    D_OV_CHECK(ok, !is_dropped_v<ov_base>);
    D_OV_CHECK(ok, D_TEST_TRAIT_V_AGREES(is_dropped, dropped));
    D_OV_CHECK(ok, D_TEST_TRAIT_V_AGREES(is_dropped, int));
    D_OV_CHECK(ok, D_TEST_TRAIT_V_AGREES(is_dropped, ov_base));

    return ok;
}

/*
tests_override_is_dropped_cvref
  The trait's behaviour on QUALIFIED spellings of the sentinel.
  Tests the following:
  - is_dropped is strict type identity, so const / volatile / reference /
    pointer / array forms of `dropped` all read as false
  This is not a defect: it pins the contract an engine must honour, namely
  that a hook returns `dropped` itself and never a qualified variant of it.
  A hook that returned `const dropped` would be silently KEPT by every
  engine that filters on this trait, so the boundary is worth stating.
*/
bool
tests_override_is_dropped_cvref()
{
    bool ok = true;

    // qualified forms are not the sentinel
    D_OV_CHECK(ok, !is_dropped_v<const dropped>);
    D_OV_CHECK(ok, !is_dropped_v<volatile dropped>);
    D_OV_CHECK(ok, !is_dropped_v<const volatile dropped>);
    D_OV_CHECK(ok, !is_dropped_v<dropped&>);
    D_OV_CHECK(ok, !is_dropped_v<const dropped&>);
    D_OV_CHECK(ok, !is_dropped_v<dropped&&>);
    D_OV_CHECK(ok, !is_dropped_v<dropped*>);
    D_OV_CHECK(ok, !is_dropped_v<dropped[2]>);

    // stripping the qualifiers recovers the answer, which is the escape
    // hatch a caller has if it ever needs the loose reading
    D_OV_CHECK(ok, is_dropped_v<std::remove_cvref_t<const dropped&>>);
    D_OV_CHECK(ok, is_dropped_v<std::remove_pointer_t<dropped*>>);

    // and every one of those answers is exactly std::is_same's
    D_OV_CHECK(ok, is_dropped_v<const dropped> ==
                   (std::is_same_v<const dropped, dropped>));
    D_OV_CHECK(ok, is_dropped_v<dropped&> ==
                   (std::is_same_v<dropped&, dropped>));

    return ok;
}

/*
tests_override_is_dropped_hostile
  The trait run over the adversarial type zoo.
  Tests the following:
  - it answers false for every hostile type and hard-errors on none;
    because count_holds instantiates every cell with no short-circuit,
    a type that made the trait ill-formed would break the BUILD here
  - it keeps its bool-trait shape across that whole zoo, including on
    an incomplete type, a void type, function / array / reference types
    and member pointers
  - a mixed pack containing the sentinel once yields a count of exactly
    one, so the trait is neither vacuously false nor over-eager
*/
bool
tests_override_is_dropped_hostile()
{
    bool ok = true;

    // no hostile type is the sentinel, and none breaks the trait
    D_OV_CHECK(ok, ::djinterp::test::holds_for_none<
                       is_dropped, D_TEST_HOSTILE_TYPES>::value);

    // the shape survives the same zoo
    D_OV_CHECK(ok, ::djinterp::test::trait_is_well_formed<
                       is_dropped, D_TEST_HOSTILE_TYPES>::value);

    // the positive battery
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       is_dropped, dropped, dropped>::value);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_any<
                       is_dropped, int, dropped, ov_base>::value);

    // exactly one hit in a mixed pack - not vacuous, not over-eager
    D_OV_CHECK(ok, (::djinterp::test::count_holds<
                        is_dropped,
                        ov_base, dropped, ov_delta, int>::value == 1));
    D_OV_CHECK(ok, (::djinterp::test::count_holds<
                        is_dropped, D_TEST_HOSTILE_TYPES>::value == 0));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
