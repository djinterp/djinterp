/******************************************************************************
* djinterp [test]                                        option_tests_core.cpp
*
*   Section II of the option.hpp suite: the core option<> type.
*
*     option<_Key, _Args...>
*
*   Two partial specializations of an only-declared primary:
*     - unary form  option<_Key>            : key_type, key, has_args=false,
*                                             arg_count=0  (NO args_type).
*     - args  form  option<_Key,_F,_Rest...>: key_type, args_type=tuple<...>,
*                                             key, has_args=true, arg_count>=1.
*
*   Coverage walks every member of both forms, the full member SURFACE of each
* (including the deliberate absence of args_type on the unary form), the key
* NTTP across its representable kinds (enum / scoped enum / int / char / bool /
* unsigned / long / nullptr / pointer), and the args pack's exact,
* order-preserving, non-flattening, non-deduplicating storage - plus type
* identity (same key+args => same type; any difference => a different type).
*
*
* path:      /tests/djinterp/core/option/option_tests_core.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "option_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// unary form  option<_Key>
// ===========================================================================

// core_unary_members
//   the four members of a unary option carry the documented values:
// key_type == decltype(_Key), key == _Key, has_args == false, arg_count == 0.
bool
core_unary_members()
{
    using opt = option<opt_key::alpha>;

    constexpr bool ok =
        std::is_same<opt::key_type, opt_key>::value  &&
        (opt::key == opt_key::alpha)                 &&
        (opt::has_args == false)                     &&
        (opt::arg_count == 0);

    static_assert(ok, "unary option<K> member values");
    return ok;
}

// core_unary_member_surface
//   a unary option exposes key_type / key / has_args / arg_count but NOT
// args_type - the unary specialization deliberately omits it.
bool
core_unary_member_surface()
{
    using opt = option<opt_key::alpha>;

    constexpr bool ok =
         has_key_type_member<opt>::value   &&
         has_key_value_member<opt>::value  &&
         has_has_args_member<opt>::value   &&
         has_arg_count_member<opt>::value  &&
        !has_args_type_member<opt>::value;   // the defining absence

    static_assert(ok, "unary option<K> surface: no args_type");
    return ok;
}


// ===========================================================================
// args form  option<_Key, _First, _Rest...>
// ===========================================================================

// core_args_single_members
//   a one-arg option selects the args specialization: has_args == true,
// arg_count == 1, args_type == std::tuple<arg_a>, key/key_type intact.
bool
core_args_single_members()
{
    using opt = option<opt_key::beta, arg_a>;

    constexpr bool ok =
        std::is_same<opt::key_type, opt_key>::value          &&
        (opt::key == opt_key::beta)                          &&
        (opt::has_args == true)                              &&
        (opt::arg_count == 1)                                &&
        std::is_same<opt::args_type, std::tuple<arg_a>>::value;

    static_assert(ok, "single-arg option<K,A> member values");
    return ok;
}

// core_args_single_member_surface
//   the args form adds args_type to the full surface (all five members
// present).
bool
core_args_single_member_surface()
{
    using opt = option<opt_key::beta, arg_a>;

    constexpr bool ok =
        has_key_type_member<opt>::value   &&
        has_args_type_member<opt>::value  &&   // present on the args form
        has_key_value_member<opt>::value  &&
        has_has_args_member<opt>::value   &&
        has_arg_count_member<opt>::value;

    static_assert(ok, "args option<K,A> surface: args_type present");
    return ok;
}

// core_args_multi_counts
//   arg_count == (sizeof...(_Rest) + 1) and args_type is the exact,
// order-preserving tuple for two- and three-arg options.
bool
core_args_multi_counts()
{
    using o2 = option<opt_key::alpha, arg_a, arg_b>;
    using o3 = option<opt_key::alpha, arg_a, arg_b, arg_c>;

    constexpr bool ok =
        (o2::arg_count == 2)                                              &&
        std::is_same<o2::args_type, std::tuple<arg_a, arg_b>>::value      &&
        (o3::arg_count == 3)                                              &&
        std::is_same<o3::args_type, std::tuple<arg_a, arg_b, arg_c>>::value;

    static_assert(ok, "multi-arg count + ordered args_type");
    return ok;
}

// core_args_many
//   a large, mixed pack survives verbatim - the args are opaque, so tags and
// fundamentals coexist in order with the correct arity.
bool
core_args_many()
{
    using big = option<opt_key::gamma, arg_a, arg_b, arg_c, int, char, bool, long, double>;

    constexpr bool ok =
        (big::arg_count == 8)                                            &&
        std::is_same<
            big::args_type,
            std::tuple<arg_a, arg_b, arg_c, int, char, bool, long, double>
        >::value;

    static_assert(ok, "eight-arg option: arity + ordered mixed args");
    return ok;
}

// core_arg_count_matches_tuple_size
//   arg_count is exactly std::tuple_size_v<args_type> across arities - the
// scalar count and the tuple width never disagree.
bool
core_arg_count_matches_tuple_size()
{
    using o1 = option<opt_key::alpha, arg_a>;
    using o2 = option<opt_key::alpha, arg_a, arg_b>;
    using o5 = option<opt_key::alpha, int, int, int, int, int>;

    constexpr bool ok =
        (o1::arg_count == std::tuple_size<o1::args_type>::value)  &&
        (o2::arg_count == std::tuple_size<o2::args_type>::value)  &&
        (o5::arg_count == std::tuple_size<o5::args_type>::value)  &&
        (o5::arg_count == 5);

    static_assert(ok, "arg_count == tuple_size(args_type)");
    return ok;
}


// ===========================================================================
// key NTTP variety   (key_type == decltype(_Key))
// ===========================================================================

// core_key_type_variety
//   key_type is decltype(_Key) for every representable non-type key kind:
// scoped/unscoped enum, int, char, bool, unsigned, long.
bool
core_key_type_variety()
{
    constexpr bool ok =
        std::is_same<option<opt_key::alpha>::key_type,   opt_key>::value       &&
        std::is_same<option<opt_key2::red>::key_type,    opt_key2>::value      &&
        std::is_same<option<opk_one>::key_type,          opt_plain_key>::value &&
        std::is_same<option<42>::key_type,               int>::value          &&
        std::is_same<option<'x'>::key_type,              char>::value          &&
        std::is_same<option<true>::key_type,             bool>::value          &&
        std::is_same<option<7u>::key_type,               unsigned>::value      &&
        std::is_same<option<9L>::key_type,               long>::value;

    static_assert(ok, "key_type == decltype(_Key) across NTTP kinds");
    return ok;
}

// core_key_value_variety
//   key holds the exact NTTP it was given, and distinct enumerators of one
// enum produce options with distinct key values.
bool
core_key_value_variety()
{
    constexpr bool ok =
        (option<opt_key::alpha>::key == opt_key::alpha)  &&
        (option<opt_key::beta>::key  == opt_key::beta)   &&
        (option<42>::key             == 42)              &&
        (option<'x'>::key            == 'x')             &&
        (option<true>::key           == true)            &&
        (option<opt_key::alpha>::key != opt_key::beta);

    static_assert(ok, "key preserves the exact NTTP value");
    return ok;
}

// core_key_nullptr
//   a std::nullptr_t key is representable: key_type is nullptr_t, the option
// is unary, and the stored key compares equal to nullptr.
bool
core_key_nullptr()
{
    using opt = option<nullptr>;

    constexpr bool ok =
        std::is_same<opt::key_type, std::nullptr_t>::value  &&
        (opt::has_args == false)                            &&
        (opt::arg_count == 0)                               &&
        (opt::key == nullptr);

    static_assert(ok, "nullptr_t key: unary, key_type == nullptr_t");
    return ok;
}

// core_key_pointer
//   a pointer-valued key (address of a linkage-bearing constexpr object) is
// representable: key_type is the pointer type and key is that address.
bool
core_key_pointer()
{
    using opt = option<&opt_nttp_object>;

    constexpr bool ok =
        std::is_same<opt::key_type, const int*>::value  &&
        (opt::key == &opt_nttp_object)                  &&
        (opt::has_args == false);

    static_assert(ok, "pointer key: key_type == const int*, key == &object");
    return ok;
}


// ===========================================================================
// args storage semantics   (verbatim, ordered, no flatten, no dedup)
// ===========================================================================

// core_args_not_flattened
//   a single std::tuple arg is stored AS a nested tuple, not flattened:
// option<K, tuple<int,char>> has arg_count 1 and args_type
// tuple<tuple<int,char>>.
bool
core_args_not_flattened()
{
    using opt = option<opt_key::alpha, std::tuple<int, char>>;

    constexpr bool ok =
        (opt::arg_count == 1)                                                    &&
        std::is_same<opt::args_type, std::tuple<std::tuple<int, char>>>::value   &&
        !std::is_same<opt::args_type, std::tuple<int, char>>::value;

    static_assert(ok, "a tuple arg is nested, never flattened");
    return ok;
}

// core_args_duplicates_preserved
//   repeated arg types are kept as-is (no deduplication): three arg_a slots
// give arg_count 3 and tuple<arg_a,arg_a,arg_a>.
bool
core_args_duplicates_preserved()
{
    using opt = option<opt_key::alpha, arg_a, arg_a, arg_a>;

    constexpr bool ok =
        (opt::arg_count == 3)                                                    &&
        std::is_same<opt::args_type, std::tuple<arg_a, arg_a, arg_a>>::value;

    static_assert(ok, "duplicate args are preserved, not deduplicated");
    return ok;
}

// core_args_ref_cv_ptr
//   reference, cv-qualified, and pointer arg types are stored verbatim - the
// args pack is fully opaque and applies no decay.
bool
core_args_ref_cv_ptr()
{
    constexpr bool ok =
        std::is_same<option<opt_key::alpha, int&>::args_type,
                     std::tuple<int&>>::value                                    &&
        std::is_same<option<opt_key::alpha, const int>::args_type,
                     std::tuple<const int>>::value                               &&
        std::is_same<option<opt_key::alpha, int*>::args_type,
                     std::tuple<int*>>::value                                    &&
        std::is_same<option<opt_key::alpha, const arg_a&>::args_type,
                     std::tuple<const arg_a&>>::value;

    static_assert(ok, "reference / cv / pointer args stored verbatim (no decay)");
    return ok;
}


// ===========================================================================
// type identity
// ===========================================================================

// core_type_identity_same
//   identical key + identical args yield the identical type (options are
// value-less type tags, so type equality is the identity).
bool
core_type_identity_same()
{
    constexpr bool ok =
        std::is_same<option<opt_key::alpha>, option<opt_key::alpha>>::value          &&
        std::is_same<option<opt_key::alpha, arg_a>,
                     option<opt_key::alpha, arg_a>>::value                           &&
        std::is_same<option<opt_key::alpha, arg_a, arg_b>,
                     option<opt_key::alpha, arg_a, arg_b>>::value;

    static_assert(ok, "same key + same args => same type");
    return ok;
}

// core_type_identity_distinct
//   any difference - the key, the arg set, arg ORDER, or unary-vs-args -
// produces a different type.
bool
core_type_identity_distinct()
{
    constexpr bool ok =
        !std::is_same<option<opt_key::alpha>, option<opt_key::beta>>::value          &&  // key
        !std::is_same<option<opt_key::alpha, arg_a>,
                      option<opt_key::alpha, arg_b>>::value                          &&  // arg set
        !std::is_same<option<opt_key::alpha, arg_a, arg_b>,
                      option<opt_key::alpha, arg_b, arg_a>>::value                   &&  // arg order
        !std::is_same<option<opt_key::alpha, arg_a>,
                      option<opt_key::alpha>>::value                                 &&  // args vs unary
        !std::is_same<option<opt_key::alpha>, option<opt_key2::red>>::value;             // key type

    static_assert(ok, "any key/arg/order difference => distinct type");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_core_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "core option<>";
    b.descriptor = "the option<_Key, _Args...> aggregate: unary + args specializations";
    b.tests      = {
        { "core_unary_members",
          "unary option<K>: key_type/key/has_args=false/arg_count=0",
          &core_unary_members },
        { "core_unary_member_surface",
          "unary option<K> exposes every member except args_type",
          &core_unary_member_surface },
        { "core_args_single_members",
          "option<K,A>: has_args=true, arg_count=1, args_type=tuple<A>",
          &core_args_single_members },
        { "core_args_single_member_surface",
          "args option<K,A> exposes args_type plus the shared members",
          &core_args_single_member_surface },
        { "core_args_multi_counts",
          "arg_count and ordered args_type for 2- and 3-arg options",
          &core_args_multi_counts },
        { "core_args_many",
          "an 8-arg mixed pack keeps its arity and order verbatim",
          &core_args_many },
        { "core_arg_count_matches_tuple_size",
          "arg_count always equals tuple_size(args_type)",
          &core_arg_count_matches_tuple_size },
        { "core_key_type_variety",
          "key_type == decltype(_Key) for enum/int/char/bool/unsigned/long",
          &core_key_type_variety },
        { "core_key_value_variety",
          "key stores the exact NTTP value; distinct enumerators differ",
          &core_key_value_variety },
        { "core_key_nullptr",
          "a std::nullptr_t key is a valid unary key",
          &core_key_nullptr },
        { "core_key_pointer",
          "a pointer-valued key carries the pointer type and address",
          &core_key_pointer },
        { "core_args_not_flattened",
          "a std::tuple arg is nested, not flattened",
          &core_args_not_flattened },
        { "core_args_duplicates_preserved",
          "repeated arg types are kept, not deduplicated",
          &core_args_duplicates_preserved },
        { "core_args_ref_cv_ptr",
          "reference/cv/pointer args are stored verbatim (no decay)",
          &core_args_ref_cv_ptr },
        { "core_type_identity_same",
          "same key + same args => the same type",
          &core_type_identity_same },
        { "core_type_identity_distinct",
          "any key/arg/order difference => a distinct type",
          &core_type_identity_distinct },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
