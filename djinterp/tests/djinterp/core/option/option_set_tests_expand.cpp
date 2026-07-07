/******************************************************************************
* djinterp [test]                                  option_set_tests_expand.cpp
*
*   Sections I + II of the option_set.hpp suite: the normalization machinery
* that turns a user's entry list into the flat option tuple.
*
*     expand_option<_Entry> / expand_option_t   - per-entry expansion.  The
*         default specialization wraps ANY entry in a single-element tuple; an
*         entry opts into multi-expansion by exposing a nested `::expanded_t`
*         alias, which becomes its expansion verbatim (an empty tuple is legal
*         and is how passthrough markers contribute nothing).
*     flatten_tuples_t<_Tuples...>              - type-level tuple_cat: the
*         per-entry tuples concatenated into one.
*
*   Detection of the ::expanded_t opt-in is structural (via std::void_t), so
* these two pieces are the whole normalization contract; the strict "must be
* options" / uniformity / uniqueness checks that run over the flattened result
* are section III.
*
*
* path:      /tests/djinterp/core/option/option_set_tests_expand.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <tuple>
#include <type_traits>
// djinterp
#include "option_set_tests.hpp"


NS_DJINTERP
NS_TESTING


// expand_option_default_wraps
//   the default specialization (no ::expanded_t) wraps the entry in a
// single-element tuple - for an option, and equally for any other type (the
// trait is purely structural; the "must be an option" contract is enforced
// later, by run_set_checks).
bool
expand_option_default_wraps()
{
    constexpr bool ok =
        std::is_same<expand_option_t<option<os_key::a>>,
                     std::tuple<option<os_key::a>>>::value                       &&
        std::is_same<expand_option_t<option<os_key::a, int>>,
                     std::tuple<option<os_key::a, int>>>::value                  &&
        std::is_same<expand_option_t<os_not_option>,
                     std::tuple<os_not_option>>::value;

    static_assert(ok, "expand_option default: wrap entry in a single-element tuple");
    return ok;
}

// expand_option_multi_expander
//   an entry exposing ::expanded_t expands to that tuple verbatim - whether it
// carries several options or just one.
bool
expand_option_multi_expander()
{
    constexpr bool ok =
        std::is_same<expand_option_t<os_expander_bc>,
                     std::tuple<option<os_key::b>, option<os_key::c>>>::value    &&
        std::is_same<expand_option_t<os_expander_d>,
                     std::tuple<option<os_key::d>>>::value;

    static_assert(ok, "expand_option: ::expanded_t entry expands to that tuple verbatim");
    return ok;
}

// expand_option_passthrough_empty
//   a passthrough marker's empty ::expanded_t expands to the empty tuple - it
// contributes nothing to the flattened result.
bool
expand_option_passthrough_empty()
{
    constexpr bool ok =
        std::is_same<expand_option_t<os_passthrough>, std::tuple<>>::value;

    static_assert(ok, "expand_option: empty ::expanded_t -> tuple<> (contributes nothing)");
    return ok;
}

// flatten_tuples_concatenates
//   flatten_tuples_t is tuple_cat at the type level: several per-entry tuples
// (including an interspersed empty one) concatenate in order.
bool
flatten_tuples_concatenates()
{
    constexpr bool ok =
        std::is_same<
            flatten_tuples_t<std::tuple<int>, std::tuple<char, double>, std::tuple<>>,
            std::tuple<int, char, double>>::value                               &&
        std::is_same<
            flatten_tuples_t<std::tuple<int, char>>,
            std::tuple<int, char>>::value;

    static_assert(ok, "flatten_tuples_t: concatenate per-entry tuples in order");
    return ok;
}

// flatten_tuples_edge_cases
//   the degenerate inputs: no tuples at all, and a single empty tuple, both
// flatten to the empty tuple.
bool
flatten_tuples_edge_cases()
{
    constexpr bool ok =
        std::is_same<flatten_tuples_t<>, std::tuple<>>::value                   &&
        std::is_same<flatten_tuples_t<std::tuple<>>, std::tuple<>>::value;

    static_assert(ok, "flatten_tuples_t: nothing / single-empty -> tuple<>");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_set_expand_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "I+II. expansion + flattening";
    b.descriptor = "expand_option (default wrap / ::expanded_t / passthrough) and flatten_tuples_t";
    b.tests = {
        { "expand_option_default_wraps",
          "default expansion wraps an entry in a single-element tuple",
          &expand_option_default_wraps },
        { "expand_option_multi_expander",
          "::expanded_t entry expands to that tuple verbatim",
          &expand_option_multi_expander },
        { "expand_option_passthrough_empty",
          "empty ::expanded_t -> tuple<> (contributes nothing)",
          &expand_option_passthrough_empty },
        { "flatten_tuples_concatenates",
          "flatten_tuples_t concatenates per-entry tuples in order",
          &flatten_tuples_concatenates },
        { "flatten_tuples_edge_cases",
          "flatten_tuples_t nothing / single-empty -> tuple<>",
          &flatten_tuples_edge_cases },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
