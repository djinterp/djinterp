/******************************************************************************
* djinterp [test]                                  option_set_tests_values.cpp
*
*   Section V of the option_set.hpp suite: the field marker and the value-
* carrying face.
*
*   TYPE-LEVEL (available at the header's base standard):
*     field<T>                 - marker whose ::type is T.
*     unit                     - the empty slot type.
*     unary_option<K>          - sugar for option<K, field<unit>>.
*     internal::option_field   - an option's bound runtime type: field<T> -> T,
*                                otherwise unit (unary, val_t<> schema, or any
*                                non-field first arg).
*     internal::store_values   - tuple<option...> -> tuple<field-type...>.
*     internal::os_slot        - key -> slot index over the flat tuple.
*
*   VALUE FACE (C++20 only - the values constructor is requires-clause guarded):
*     values_type              - one slot per option (all-unit for a pure schema).
*     the values constructor    - seeds fields in slot order.
*     get<K>() / set<K>()      - key-addressed field access.
*     contains<K>()            - compile-time membership on the instance.
*   These runtime facets are evaluated inside a constexpr immediately-invoked
* lambda, so a construct/get/set regression fails the build at the assertion.
*
*   The get/set static_asserts that reject an absent key or a unit slot are
* hard errors by design and are exercised out of suite; here contains<>() shows
* the false case (no error), and the value tests use genuine field slots.
*
*   internal:: pieces are reached through a TU-local alias.
*
*
* path:      /tests/djinterp/core/option/option_set_tests_values.cpp
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


namespace  // internal-helper alias, local to this TU
{
    namespace ic = ::djinterp::internal;
}

// ---------------------------------------------------------------------------
// type-level (base standard)
// ---------------------------------------------------------------------------

// values_field_type
//   field<T> simply carries T as ::type.
bool
values_field_type()
{
    constexpr bool ok =
        std::is_same<field<int>::type, int>::value                             &&
        std::is_same<field<os_not_option>::type, os_not_option>::value;

    static_assert(ok, "field<T>::type == T");
    return ok;
}

// values_unary_option_alias
//   unary_option<K> is exactly option<K, field<unit>> - a presence-only key
// whose slot is empty.
bool
values_unary_option_alias()
{
    constexpr bool ok =
        std::is_same<unary_option<os_key::a>,
                     option<os_key::a, field<unit>>>::value;

    static_assert(ok, "unary_option<K> == option<K, field<unit>>");
    return ok;
}

// values_option_field_cases
//   option_field reads an option's bound runtime type: field<T> -> T; unit for
// a unary option, for a non-field first arg, and for field<unit>; and it looks
// only at the FIRST arg (trailing args are ignored).
bool
values_option_field_cases()
{
    constexpr bool ok =
        std::is_same<option_field_t<option<os_key::a, field<int>>>, int>::value          &&
        std::is_same<option_field_t<option<os_key::a>>, unit>::value                     &&
        std::is_same<option_field_t<option<os_key::a, int>>, unit>::value                &&
        std::is_same<option_field_t<unary_option<os_key::a>>, unit>::value               &&
        std::is_same<option_field_t<option<os_key::a, field<double>, int>>, double>::value;

    static_assert(ok, "option_field: field<T>->T; unit for unary / non-field / field<unit>; first arg only");
    return ok;
}

// values_store_values
//   store_values maps a flat option tuple to its per-slot field types (unit
// where there is no field); the empty tuple maps to the empty tuple.
bool
values_store_values()
{
    constexpr bool ok =
        std::is_same<
            ic::store_values<std::tuple<option<os_key::a, field<int>>,
                                        option<os_key::b>>>::type,
            std::tuple<int, unit>>::value                                       &&
        std::is_same<ic::store_values<std::tuple<>>::type, std::tuple<>>::value;

    static_assert(ok, "store_values: tuple<option...> -> tuple<field-type...> (unit for no field)");
    return ok;
}

// values_os_slot
//   os_slot resolves a key to a slot index over the flat tuple: found with the
// right index for present keys, not-found for an absent key.
bool
values_os_slot()
{
    constexpr bool ok =
        ic::os_slot<os_key::a,
                    std::tuple<option<os_key::a, field<int>>,
                               option<os_key::b>>>::found                       &&
        (ic::os_slot<os_key::b,
                     std::tuple<option<os_key::a>,
                                option<os_key::b>>>::index == 1u)               &&
        !ic::os_slot<os_key::c,
                     std::tuple<option<os_key::a>,
                                option<os_key::b>>>::found;

    static_assert(ok, "os_slot: key -> slot index over the flat tuple (found + index / not-found)");
    return ok;
}


// ---------------------------------------------------------------------------
// value face (C++20)
// ---------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// values_values_type
//   values_type is one slot per option: the field types for field options, and
// all-unit for a pure (fieldless) schema.
bool
values_values_type()
{
    constexpr bool ok =
        std::is_same<
            option_set<option<os_key::a, field<int>>,
                       option<os_key::b, field<double>>>::values_type,
            std::tuple<int, double>>::value                                     &&
        std::is_same<
            option_set<option<os_key::a>, option<os_key::b>>::values_type,
            std::tuple<unit, unit>>::value;

    static_assert(ok, "values_type: field types per slot; all-unit for a pure schema");
    return ok;
}

// values_roundtrip_get_set
//   construct with seed values, read them back by key, mutate one with set,
// read it again - all in a constant-evaluated context.
bool
values_roundtrip_get_set()
{
    constexpr bool ok = []
    {
        option_set<option<os_key::a, field<int>>,
                   option<os_key::b, field<double>>> s{ 7, 2.5 };

        bool r = (s.get<os_key::a>() == 7) &&
                 (s.get<os_key::b>() == 2.5);

        s.set<os_key::a>(9);
        r = r && (s.get<os_key::a>() == 9);

        // the untouched slot is unchanged
        r = r && (s.get<os_key::b>() == 2.5);
        return r;
    }();

    static_assert(ok, "construct / get / set roundtrip by key (constexpr)");
    return ok;
}

// values_default_value_initialized
//   the default constructor value-initializes every slot (an int field becomes
// 0), and values() exposes that slot tuple.
bool
values_default_value_initialized()
{
    constexpr bool ok = []
    {
        option_set<option<os_key::a, field<int>>> s{};
        return (s.get<os_key::a>() == 0) &&
               (std::get<0>(s.values()) == 0);
    }();

    static_assert(ok, "default ctor value-initializes slots; values() exposes them");
    return ok;
}

// values_member_contains
//   the instance-level contains<K>() reports membership at compile time: true
// for a declared key, false for an absent one.
bool
values_member_contains()
{
    using set = option_set<option<os_key::a, field<int>>, option<os_key::b>>;

    constexpr bool ok =
        set::contains<os_key::a>()                                             &&
        set::contains<os_key::b>()                                             &&
        !set::contains<os_key::c>();

    static_assert(ok, "member contains<K>(): true for declared keys, false otherwise");
    return ok;
}

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_set_values_block()
{
    ::djinterp::test::block_spec b;
    b.name = "V. field marker + value face";
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    b.descriptor = "field / unit / unary_option / option_field / store_values / os_slot + value face";
#else
    b.descriptor = "field / unit / unary_option / option_field / store_values / os_slot "
                   "(value face skipped: needs C++20)";
#endif
    b.tests = {
        { "values_field_type",
          "field<T>::type == T",
          &values_field_type },
        { "values_unary_option_alias",
          "unary_option<K> == option<K, field<unit>>",
          &values_unary_option_alias },
        { "values_option_field_cases",
          "option_field: field<T>->T; unit otherwise; first arg only",
          &values_option_field_cases },
        { "values_store_values",
          "store_values: tuple<option...> -> tuple<field-type...>",
          &values_store_values },
        { "values_os_slot",
          "os_slot: key -> slot index over the flat tuple",
          &values_os_slot },
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
        { "values_values_type",
          "values_type: field types per slot; all-unit for a pure schema",
          &values_values_type },
        { "values_roundtrip_get_set",
          "construct / get / set roundtrip by key (constexpr)",
          &values_roundtrip_get_set },
        { "values_default_value_initialized",
          "default ctor value-initializes slots; values() exposes them",
          &values_default_value_initialized },
        { "values_member_contains",
          "member contains<K>(): true for declared keys, false otherwise",
          &values_member_contains },
#endif
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
