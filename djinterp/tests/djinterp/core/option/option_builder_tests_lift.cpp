/******************************************************************************
* djinterp [test]                                 option_builder_tests_lift.cpp
*
*   Section II of the option_builder.hpp suite: the tuple-to-set lift.
*
*     tuple_to_option_set<std::tuple<_Entries...>> / _t
*         - lifts a std::tuple of entries into option_set<_Entries...>.
*
*   The lift is a pure re-seat of the pack; it imposes nothing itself.  The
* interesting behavior is downstream: option_set applies its own strict
* contract after expanding each entry through ::expanded_t, so a passthrough
* entry (expanded_t == tuple<>) flattens to nothing and the resulting set is
* valid.  The tests cover a plain tuple of options, the empty tuple, and a
* tuple mixing an option with a passthrough marker - checking that the lifted
* set flattens the passthrough away (size and flat tuple).
*
*
* path:      /tests/djinterp/core/option/option_builder_tests_lift.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <tuple>
#include <type_traits>
// djinterp
#include "option_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


// lift_basic
//   a tuple of options lifts to the corresponding option_set, entries in order.
bool
lift_basic()
{
    constexpr bool ok =
        std::is_same<
            tuple_to_option_set_t<std::tuple<option<ob_enum::a>,
                                             option<ob_enum::b, ob_slot<5>>>>,
            option_set<option<ob_enum::a>, option<ob_enum::b, ob_slot<5>>>>::value;

    static_assert(ok, "tuple_to_option_set: tuple<entries...> -> option_set<entries...>");
    return ok;
}

// lift_empty
//   the empty tuple lifts to the empty option_set.
bool
lift_empty()
{
    constexpr bool ok =
        std::is_same<tuple_to_option_set_t<std::tuple<>>, option_set<>>::value;

    static_assert(ok, "tuple_to_option_set: tuple<> -> option_set<>");
    return ok;
}

// lift_with_passthrough
//   a tuple mixing an option with a passthrough marker lifts to a set that
// flattens the passthrough away (its expanded_t is tuple<>), leaving just the
// option.
bool
lift_with_passthrough()
{
    using set = tuple_to_option_set_t<std::tuple<option<ob_enum::a>, ob_pass>>;

    constexpr bool ok =
        std::is_same<set, option_set<option<ob_enum::a>, ob_pass>>::value       &&
        (set::size == 1)                                                        &&
        std::is_same<set::flat_options_t, std::tuple<option<ob_enum::a>>>::value;

    static_assert(ok, "tuple_to_option_set: a mixed option/passthrough tuple flattens the passthrough");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_builder_lift_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "II. tuple_to_option_set";
    b.descriptor = "lift a tuple of entries into an option_set (passthroughs flatten downstream)";
    b.tests = {
        { "lift_basic",
          "tuple<entries...> -> option_set<entries...>",
          &lift_basic },
        { "lift_empty",
          "tuple<> -> option_set<>",
          &lift_empty },
        { "lift_with_passthrough",
          "a mixed option/passthrough tuple flattens the passthrough",
          &lift_with_passthrough },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
