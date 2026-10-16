/*******************************************************************************
* djinterp [test]                               option_factory_tests_carrier.cpp
*
*   Section I - key_carrier<auto>, the NTTP-to-type bridge.  Covers its two
* members (::value carrying the NTTP, ::value_type reporting decltype of it)
* across the NTTP kinds a key might take (enum class, int, char, bool,
* unsigned), the round-trip that ::value is itself a usable constant
* expression, per-value type distinctness, and its intended use as a typename
* slot.  All compile-time.
*
*
* path:      /tests/djinterp/core/option/option_factory_tests_carrier.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./option_factory_tests.hpp"


NS_DJINTERP
NS_TESTING


::djinterp::test::block_spec
option_factory_carrier_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "I.  key_carrier";
    block.descriptor = "NTTP-to-type bridge: ::value / ::value_type, round-trip, identity";

    // ---- ::value + ::value_type across NTTP kinds ------------------------

    block.tests.push_back({
        "carrier_enum",
        "enum-class key: value carried, value_type is the enum",
        []() -> bool
        {
            constexpr bool ok =
                ( ::djinterp::key_carrier<fkey::alpha>::value == fkey::alpha ) &&
                same_v< ::djinterp::key_carrier<fkey::alpha>::value_type, fkey >;
            static_assert(ok, "key_carrier<fkey::alpha>");
            return ok;
        } });

    block.tests.push_back({
        "carrier_int",
        "int key: value carried, value_type is int",
        []() -> bool
        {
            constexpr bool ok =
                ( ::djinterp::key_carrier<42>::value == 42 ) &&
                same_v< ::djinterp::key_carrier<42>::value_type, int >;
            static_assert(ok, "key_carrier<42>");
            return ok;
        } });

    block.tests.push_back({
        "carrier_char",
        "char key: value_type is char, not the promoted int",
        []() -> bool
        {
            constexpr bool ok =
                ( ::djinterp::key_carrier<'k'>::value == 'k' ) &&
                same_v< ::djinterp::key_carrier<'k'>::value_type, char >;
            static_assert(ok, "key_carrier<'k'>");
            return ok;
        } });

    block.tests.push_back({
        "carrier_bool",
        "bool key: value carried, value_type is bool",
        []() -> bool
        {
            constexpr bool ok =
                ( ::djinterp::key_carrier<true>::value == true ) &&
                same_v< ::djinterp::key_carrier<true>::value_type, bool >;
            static_assert(ok, "key_carrier<true>");
            return ok;
        } });

    block.tests.push_back({
        "carrier_unsigned",
        "unsigned key: value_type reflects the exact integral type",
        []() -> bool
        {
            constexpr bool ok =
                ( ::djinterp::key_carrier<7u>::value == 7u ) &&
                same_v< ::djinterp::key_carrier<7u>::value_type, unsigned > &&
                !same_v< ::djinterp::key_carrier<7u>::value_type, int >;
            static_assert(ok, "key_carrier<7u>");
            return ok;
        } });

    // ---- ::value is itself a usable constant expression ------------------

    block.tests.push_back({
        "carrier_value_round_trips_as_nttp",
        "::value can be fed straight back as an NTTP",
        []() -> bool
        {
            // option<key_carrier<k>::value> must name the very same type as option<k>
            constexpr bool ok = same_v<
                ::djinterp::option< ::djinterp::key_carrier<fkey::beta>::value >,
                ::djinterp::option<fkey::beta> >;
            static_assert(ok, "option<carrier::value> == option<key>");
            return ok;
        } });

    // ---- type identity ---------------------------------------------------

    block.tests.push_back({
        "carrier_distinct_values_distinct_types",
        "different carried values yield different types",
        []() -> bool
        {
            constexpr bool ok =
                !same_v< ::djinterp::key_carrier<fkey::alpha>,
                         ::djinterp::key_carrier<fkey::beta> > &&
                !same_v< ::djinterp::key_carrier<1>,
                         ::djinterp::key_carrier<2> >;
            static_assert(ok, "distinct values -> distinct carriers");
            return ok;
        } });

    block.tests.push_back({
        "carrier_equal_values_same_type",
        "the same carried value yields the same type",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::key_carrier<fkey::alpha>,
                ::djinterp::key_carrier<fkey::alpha> >;
            static_assert(ok, "equal values -> identical carrier");
            return ok;
        } });

    // ---- the reason it exists: a typename slot ---------------------------

    block.tests.push_back({
        "carrier_fills_a_typename_slot",
        "key_carrier<> occupies a template<typename...> position, keeping identity",
        []() -> bool
        {
            // used where only typenames are allowed; still distinguishable by key
            constexpr bool ok = !same_v<
                type_pack< ::djinterp::key_carrier<fkey::alpha>,
                           ::djinterp::key_carrier<fkey::beta> >,
                type_pack< ::djinterp::key_carrier<fkey::beta>,
                           ::djinterp::key_carrier<fkey::alpha> > >;
            static_assert(ok, "carriers as typenames retain per-key identity");
            return ok;
        } });

    return block;
}


NS_END  // testing
NS_END  // djinterp
