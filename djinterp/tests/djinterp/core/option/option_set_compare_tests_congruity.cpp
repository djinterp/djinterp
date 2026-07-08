/******************************************************************************
* djinterp [test]                        option_set_compare_tests_congruity.cpp
*
*   Section III of the option_set_compare.hpp suite: the two congruity traits.
*
*     option_set_key_congruent / _v   - same key SET (order-insensitive).
*     option_set_type_congruent / _v  - STRONGER: key-congruent AND the option
*                                       at each shared key is the exact same
*                                       type (args pack included).
*
*   Both are flat-view aware, so a set written with a multi-expander is congruent
* to the equivalent set of direct options.  The headline relationship is that
* type-congruent strictly refines key-congruent: two sets can share a key set
* yet differ in the option type at some key (a unary option vs the same key with
* an args pack), which is key-congruent but NOT type-congruent - pinned directly
* by the last test.
*
*
* path:      /tests/djinterp/core/option/option_set_compare_tests_congruity.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "option_set_compare_tests.hpp"


NS_DJINTERP
NS_TESTING


// congruity_key
//   option_set_key_congruent is order-insensitive set equality of keys: a
// reordering is congruent, a differing key is not, the flat view of an expander
// matches the equivalent direct set, and two empty sets are congruent.
bool
congruity_key()
{
    constexpr bool ok =
        option_set_key_congruent_v<option_set<option<cmp_key::a>, option<cmp_key::b>>,
                                   option_set<option<cmp_key::b>, option<cmp_key::a>>>  &&
        !option_set_key_congruent_v<option_set<option<cmp_key::a>, option<cmp_key::b>>,
                                    option_set<option<cmp_key::a>, option<cmp_key::c>>> &&
        option_set_key_congruent_v<option_set<cmp_expander_ab>,
                                   option_set<option<cmp_key::a>, option<cmp_key::b>>>  &&
        option_set_key_congruent_v<option_set<>, option_set<>>;

    static_assert(ok, "option_set_key_congruent: same key set (reorder / expander congruent; empty congruent)");
    return ok;
}

// congruity_type
//   option_set_type_congruent additionally requires identical option types at
// each key: a reordering with identical options is congruent, a differing
// option type at a key is not, identical args-carrying options are congruent,
// and a differing key set fails outright (there is nothing to match).
bool
congruity_type()
{
    constexpr bool ok =
        option_set_type_congruent_v<option_set<option<cmp_key::a>, option<cmp_key::b>>,
                                    option_set<option<cmp_key::b>, option<cmp_key::a>>>       &&
        !option_set_type_congruent_v<option_set<option<cmp_key::a>, option<cmp_key::b>>,
                                     option_set<option<cmp_key::a, int>, option<cmp_key::b>>> &&
        option_set_type_congruent_v<option_set<option<cmp_key::a, int>>,
                                    option_set<option<cmp_key::a, int>>>                      &&
        !option_set_type_congruent_v<option_set<option<cmp_key::a>>,
                                     option_set<option<cmp_key::b>>>;

    static_assert(ok, "option_set_type_congruent: identical option type at each key");
    return ok;
}

// congruity_type_stronger_than_key
//   the defining relationship: a pair that shares a key set but differs in the
// option type at that key is key-congruent yet NOT type-congruent.
bool
congruity_type_stronger_than_key()
{
    using unary = option_set<option<cmp_key::a>>;
    using typed = option_set<option<cmp_key::a, int>>;

    constexpr bool ok =
        option_set_key_congruent_v<unary, typed>                               &&
        !option_set_type_congruent_v<unary, typed>;

    static_assert(ok, "type-congruent strictly refines key-congruent (same key, different option type)");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_set_compare_congruity_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "III. congruity";
    b.descriptor = "option_set_key_congruent and the stronger option_set_type_congruent";
    b.tests = {
        { "congruity_key",
          "key congruity: same key set, order-insensitive, flat-view aware",
          &congruity_key },
        { "congruity_type",
          "type congruity: identical option type at each key",
          &congruity_type },
        { "congruity_type_stronger_than_key",
          "type-congruent strictly refines key-congruent",
          &congruity_type_stronger_than_key },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
