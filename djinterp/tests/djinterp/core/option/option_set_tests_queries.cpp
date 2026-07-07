/******************************************************************************
* djinterp [test]                                 option_set_tests_queries.cpp
*
*   Section VI of the option_set.hpp suite: the trait machinery that queries an
* already-instantiated option_set<>.
*
*     is_option_set / is_option_set_v   - is _Type some option_set<...>?  The _v
*         form cleans cv / ref first.
*     option_set_contains / _v          - does the set have an option with key
*         _Key?  Walks the flat (post-expansion) tuple.
*     option_set_find / _t              - the option with key _Key, else
*         lookup_not_found; also ::found and ::index (lookup_npos on a miss).
*     option_set_key_type / _t          - (C++20 concepts) the head option's
*         key_type of a non-empty set.
*
*   Because contains and find walk the FLAT tuple, membership reflects
* expansion: a key contributed by a multi-expander is found even though it was
* never written directly.  option_set_key_type uses a requires-clause and so is
* gated on C++20 concepts; the rest of this section is available at the base
* standard.
*
*
* path:      /tests/djinterp/core/option/option_set_tests_queries.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "option_set_tests.hpp"


NS_DJINTERP
NS_TESTING


// queries_is_option_set
//   is_option_set is true only for an option_set<...> specialization - not for
// an unrelated type, and not for a bare option (an option is not a set).
bool
queries_is_option_set()
{
    constexpr bool ok =
        is_option_set_v<option_set<option<os_key::a>>>                          &&
        is_option_set_v<option_set<>>                                          &&
        !is_option_set_v<int>                                                   &&
        !is_option_set_v<option<os_key::a>>;

    static_assert(ok, "is_option_set: true only for option_set<...> (an option is not a set)");
    return ok;
}

// queries_is_option_set_cleans_cvref
//   the _v form strips cv / ref before testing, so a reference or cv-qualified
// option_set still reports true.
bool
queries_is_option_set_cleans_cvref()
{
    constexpr bool ok =
        is_option_set_v<const option_set<option<os_key::a>>&>                   &&
        is_option_set_v<option_set<option<os_key::a>>&&>                        &&
        is_option_set_v<volatile option_set<option<os_key::a>>>;

    static_assert(ok, "is_option_set_v: cleans cv / ref before testing");
    return ok;
}

// queries_option_set_contains
//   option_set_contains is true for a declared key and false for an absent one.
bool
queries_option_set_contains()
{
    using set = option_set<option<os_key::a>, option<os_key::b, int>>;

    constexpr bool ok =
        option_set_contains_v<set, os_key::a>                                   &&
        option_set_contains_v<set, os_key::b>                                   &&
        !option_set_contains_v<set, os_key::c>;

    static_assert(ok, "option_set_contains: true for declared keys, false otherwise");
    return ok;
}

// queries_option_set_find_hit
//   option_set_find on a present key yields that exact option, reports found,
// and gives its slot index.
bool
queries_option_set_find_hit()
{
    using set = option_set<option<os_key::a>, option<os_key::b, int>>;

    constexpr bool ok =
        std::is_same<option_set_find_t<set, os_key::b>,
                     option<os_key::b, int>>::value                             &&
        option_set_find<set, os_key::b>::found                                  &&
        (option_set_find<set, os_key::b>::index == 1u);

    static_assert(ok, "option_set_find hit: exact option, found == true, correct index");
    return ok;
}

// queries_option_set_find_miss
//   option_set_find on an absent key yields lookup_not_found, reports not
// found, and gives the lookup_npos index sentinel.
bool
queries_option_set_find_miss()
{
    using set = option_set<option<os_key::a>>;

    constexpr bool ok =
        std::is_same<option_set_find_t<set, os_key::c>, lookup_not_found>::value &&
        !option_set_find<set, os_key::c>::found                                 &&
        (option_set_find<set, os_key::c>::index == lookup_npos);

    static_assert(ok, "option_set_find miss: lookup_not_found, found == false, index == lookup_npos");
    return ok;
}

// queries_find_and_contains_via_expansion
//   contains and find walk the flat tuple, so a key contributed by a multi-
// expander is visible: found for its inner keys, absent for a key that never
// appears, and find returns the expanded option.
bool
queries_find_and_contains_via_expansion()
{
    using set = option_set<os_expander_bc>;   // -> option<b>, option<c>

    constexpr bool ok =
        option_set_contains_v<set, os_key::b>                                   &&
        option_set_contains_v<set, os_key::c>                                   &&
        !option_set_contains_v<set, os_key::a>                                  &&
        std::is_same<option_set_find_t<set, os_key::c>,
                     option<os_key::c>>::value;

    static_assert(ok, "contains / find see expander-contributed keys (walk the flat tuple)");
    return ok;
}


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
// queries_option_set_key_type
//   option_set_key_type extracts the (uniform) key_type of a non-empty set
// from its head option.
bool
queries_option_set_key_type()
{
    constexpr bool ok =
        std::is_same<
            option_set_key_type_t<option_set<option<os_key::a>,
                                             option<os_key::b>>>,
            os_key>::value;

    static_assert(ok, "option_set_key_type: head option's key_type of a non-empty set");
    return ok;
}
#endif  // C++20 concepts


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_set_queries_block()
{
    ::djinterp::test::block_spec b;
    b.name = "VI. queries";
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.descriptor = "is_option_set / option_set_contains / option_set_find / option_set_key_type";
#else
    b.descriptor = "is_option_set / option_set_contains / option_set_find "
                   "(option_set_key_type skipped: needs C++20 concepts)";
#endif
    b.tests = {
        { "queries_is_option_set",
          "is_option_set true only for option_set<...>",
          &queries_is_option_set },
        { "queries_is_option_set_cleans_cvref",
          "is_option_set_v cleans cv / ref",
          &queries_is_option_set_cleans_cvref },
        { "queries_option_set_contains",
          "option_set_contains: declared keys true, otherwise false",
          &queries_option_set_contains },
        { "queries_option_set_find_hit",
          "option_set_find hit: exact option, found, correct index",
          &queries_option_set_find_hit },
        { "queries_option_set_find_miss",
          "option_set_find miss: lookup_not_found, not found, lookup_npos",
          &queries_option_set_find_miss },
        { "queries_find_and_contains_via_expansion",
          "contains / find see expander-contributed keys",
          &queries_find_and_contains_via_expansion },
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
        { "queries_option_set_key_type",
          "option_set_key_type: head option's key_type of a non-empty set",
          &queries_option_set_key_type },
#endif
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
