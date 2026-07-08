/******************************************************************************
* djinterp [test]                          option_set_compare_tests_keylist.cpp
*
*   Sections I + II of the option_set_compare.hpp suite: the key-list layer.
*
*     key_list<_Keys...>            - a type-level pack of NTTP keys (::size).
*     option_set_keys / _t          - the key_list of a set, read from its FLAT
*                                     (post-expansion) view, in flat order.
*     internal::value_in_pack       - NTTP-level membership (K is one of Ks).
*     key_list_subset / _v          - every key of the lhs appears in the rhs.
*     key_list_equal / _v           - same key SET, order-insensitive (subset
*                                     both ways).
*
*   The order sensitivity is deliberate and tested on both sides: option_set_keys
* preserves flat order (so its result is compared with the exact key_list), while
* key_list_equal is order-INSENSITIVE (a reordering is still equal).  The empty
* cases pin the fold identities: an empty key_list is a subset of anything, and
* value_in_pack over an empty candidate pack is false.  value_in_pack is
* internal:: and is reached through a TU-local alias.
*
*
* path:      /tests/djinterp/core/option/option_set_compare_tests_keylist.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <tuple>
#include <type_traits>
// djinterp
#include "option_set_compare_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace  // internal-helper alias, local to this TU
{
    namespace ic = ::djinterp::internal;
}

// keylist_key_list_size
//   key_list reports its arity through ::size, including the empty pack.
bool
keylist_key_list_size()
{
    constexpr bool ok =
        (key_list<cmp_key::a, cmp_key::b, cmp_key::c>::size == 3u)              &&
        (key_list<cmp_key::a>::size == 1u)                                     &&
        (key_list<>::size == 0u);

    static_assert(ok, "key_list::size == number of keys (incl. empty)");
    return ok;
}

// keylist_option_set_keys
//   option_set_keys yields the flat-view key_list in flat order: for direct
// options, for the empty set, through a multi-expander, and independent of any
// option args (only keys are collected).
bool
keylist_option_set_keys()
{
    constexpr bool ok =
        std::is_same<option_set_keys_t<option_set<option<cmp_key::a>,
                                                  option<cmp_key::b>>>,
                     key_list<cmp_key::a, cmp_key::b>>::value                   &&
        std::is_same<option_set_keys_t<option_set<>>, key_list<>>::value       &&
        std::is_same<option_set_keys_t<option_set<cmp_expander_ab>>,
                     key_list<cmp_key::a, cmp_key::b>>::value                   &&
        std::is_same<option_set_keys_t<option_set<option<cmp_key::a>,
                                                  option<cmp_key::b, int>>>,
                     key_list<cmp_key::a, cmp_key::b>>::value;

    static_assert(ok, "option_set_keys: flat-order key_list (direct / empty / expander / args-ignored)");
    return ok;
}

// keylist_value_in_pack
//   value_in_pack is NTTP membership: true when the key equals one of the
// candidates, false otherwise, and false over an empty candidate pack.
bool
keylist_value_in_pack()
{
    constexpr bool ok =
        ic::value_in_pack<cmp_key::a, cmp_key::a, cmp_key::b>::value            &&
        !ic::value_in_pack<cmp_key::d, cmp_key::a, cmp_key::b>::value           &&
        !ic::value_in_pack<cmp_key::a>::value;

    static_assert(ok, "value_in_pack: NTTP membership; empty candidate pack -> false");
    return ok;
}

// keylist_subset
//   key_list_subset is true iff every lhs key appears in the rhs: a genuine
// subset holds, a stray key breaks it, the empty list is a subset of anything,
// and a non-empty list is not a subset of the empty list.
bool
keylist_subset()
{
    constexpr bool ok =
        key_list_subset_v<key_list<cmp_key::a>, key_list<cmp_key::a, cmp_key::b>>       &&
        !key_list_subset_v<key_list<cmp_key::a, cmp_key::c>, key_list<cmp_key::a, cmp_key::b>> &&
        key_list_subset_v<key_list<>, key_list<cmp_key::a, cmp_key::b>>                 &&
        !key_list_subset_v<key_list<cmp_key::a, cmp_key::b>, key_list<>>;

    static_assert(ok, "key_list_subset: every lhs key in rhs; empty is a subset of all, not conversely");
    return ok;
}

// keylist_equal
//   key_list_equal is order-insensitive set equality: a reordering is equal,
// a size/element difference is not, and two empty lists are equal.
bool
keylist_equal()
{
    constexpr bool ok =
        key_list_equal_v<key_list<cmp_key::a, cmp_key::b>, key_list<cmp_key::b, cmp_key::a>> &&
        !key_list_equal_v<key_list<cmp_key::a, cmp_key::b>, key_list<cmp_key::a>>            &&
        key_list_equal_v<key_list<>, key_list<>>;

    static_assert(ok, "key_list_equal: order-insensitive set equality (reorder equal; size differs not)");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_set_compare_keylist_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "I+II. key lists + operations";
    b.descriptor = "key_list / option_set_keys and value_in_pack / key_list_subset / key_list_equal";
    b.tests = {
        { "keylist_key_list_size",
          "key_list::size == number of keys (incl. empty)",
          &keylist_key_list_size },
        { "keylist_option_set_keys",
          "option_set_keys: flat-order key_list (direct / empty / expander / args-ignored)",
          &keylist_option_set_keys },
        { "keylist_value_in_pack",
          "value_in_pack: NTTP membership; empty pack -> false",
          &keylist_value_in_pack },
        { "keylist_subset",
          "key_list_subset: empty is subset of all, not conversely",
          &keylist_subset },
        { "keylist_equal",
          "key_list_equal: order-insensitive set equality",
          &keylist_equal },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
