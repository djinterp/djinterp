/******************************************************************************
* djinterp [test]                                    option_set_tests_core.cpp
*
*   Section IV of the option_set.hpp suite: the option_set pack surface that
* option_set inherits from internal::option_set_pack.
*
*     ::size            - number of options AFTER expansion.
*     ::empty           - size == 0.
*     ::flat_options_t  - the normalized std::tuple<option<>...>.
*     option_at<_I>     - positional access into the flat list.
*
*   These are validated through the public option_set<> (which derives the pack
* and, at C++20, adds the value face tested in the values section).  The focus
* here is the normalized shape: empty sets, single and multi entries with order
* preserved, entries that multi-expand through ::expanded_t, a mix of direct
* and expanding entries, and passthrough markers that contribute nothing.  The
* result tuple's identity is checked exactly with std::is_same, which pins both
* contents and order.
*
*
* path:      /tests/djinterp/core/option/option_set_tests_core.cpp
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


// core_empty_set
//   the empty set: size 0, empty true, flat tuple empty.
bool
core_empty_set()
{
    constexpr bool ok =
        (option_set<>::size == 0)                                               &&
        option_set<>::empty                                                     &&
        std::is_same<option_set<>::flat_options_t, std::tuple<>>::value;

    static_assert(ok, "option_set<>: size 0, empty, flat_options_t == tuple<>");
    return ok;
}

// core_single_option
//   a one-option set: size 1, not empty, flat tuple is the single option,
// option_at<0> is that option.
bool
core_single_option()
{
    using set = option_set<option<os_key::a, int>>;

    constexpr bool ok =
        (set::size == 1)                                                        &&
        !set::empty                                                             &&
        std::is_same<set::flat_options_t, std::tuple<option<os_key::a, int>>>::value &&
        std::is_same<set::option_at<0>, option<os_key::a, int>>::value;

    static_assert(ok, "single option: size 1, flat tuple + option_at<0> are that option");
    return ok;
}

// core_multi_preserves_order
//   several distinct-keyed options keep declaration order in the flat tuple
// and under option_at.
bool
core_multi_preserves_order()
{
    using set = option_set<option<os_key::a>,
                           option<os_key::b, int>,
                           option<os_key::c>>;

    constexpr bool ok =
        (set::size == 3)                                                        &&
        std::is_same<set::flat_options_t,
                     std::tuple<option<os_key::a>,
                                option<os_key::b, int>,
                                option<os_key::c>>>::value                      &&
        std::is_same<set::option_at<0>, option<os_key::a>>::value              &&
        std::is_same<set::option_at<1>, option<os_key::b, int>>::value         &&
        std::is_same<set::option_at<2>, option<os_key::c>>::value;

    static_assert(ok, "multiple options: declaration order preserved (flat + option_at)");
    return ok;
}

// core_expander_flattens
//   a lone multi-expander contributes its inner options, flattened: the set's
// size and flat tuple are those inner options.
bool
core_expander_flattens()
{
    using set = option_set<os_expander_bc>;   // -> option<b>, option<c>

    constexpr bool ok =
        (set::size == 2)                                                        &&
        std::is_same<set::flat_options_t,
                     std::tuple<option<os_key::b>, option<os_key::c>>>::value   &&
        std::is_same<set::option_at<0>, option<os_key::b>>::value              &&
        std::is_same<set::option_at<1>, option<os_key::c>>::value;

    static_assert(ok, "multi-expander: inner options flattened into the set");
    return ok;
}

// core_mixed_direct_and_expander
//   a direct option followed by a multi-expander: the flat tuple is the direct
// option then the expander's inner options, in order.
bool
core_mixed_direct_and_expander()
{
    using set = option_set<option<os_key::a>, os_expander_bc>;

    constexpr bool ok =
        (set::size == 3)                                                        &&
        std::is_same<set::flat_options_t,
                     std::tuple<option<os_key::a>,
                                option<os_key::b>,
                                option<os_key::c>>>::value;

    static_assert(ok, "direct + expander: direct option then expanded options, in order");
    return ok;
}

// core_passthrough_contributes_nothing
//   passthrough markers (empty ::expanded_t) drop out entirely: mixed with a
// real option they leave just that option; a set of only passthroughs is empty.
bool
core_passthrough_contributes_nothing()
{
    using mixed    = option_set<option<os_key::a>, os_passthrough>;
    using all_pass = option_set<os_passthrough, os_passthrough>;

    constexpr bool ok =
        (mixed::size == 1)                                                      &&
        std::is_same<mixed::flat_options_t,
                     std::tuple<option<os_key::a>>>::value                      &&
        all_pass::empty                                                         &&
        std::is_same<all_pass::flat_options_t, std::tuple<>>::value;

    static_assert(ok, "passthrough markers contribute nothing (drop from the flat tuple)");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_set_core_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "IV. option_set pack surface";
    b.descriptor = "size / empty / flat_options_t / option_at, with expansion and passthrough";
    b.tests = {
        { "core_empty_set",
          "option_set<> : size 0, empty, flat_options_t == tuple<>",
          &core_empty_set },
        { "core_single_option",
          "single option: size 1, flat tuple + option_at<0> are that option",
          &core_single_option },
        { "core_multi_preserves_order",
          "multiple options: declaration order preserved",
          &core_multi_preserves_order },
        { "core_expander_flattens",
          "multi-expander: inner options flattened into the set",
          &core_expander_flattens },
        { "core_mixed_direct_and_expander",
          "direct + expander: options in order",
          &core_mixed_direct_and_expander },
        { "core_passthrough_contributes_nothing",
          "passthrough markers drop from the flat tuple",
          &core_passthrough_contributes_nothing },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
