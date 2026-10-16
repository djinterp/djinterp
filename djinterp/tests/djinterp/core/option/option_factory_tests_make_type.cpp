/*******************************************************************************
* djinterp [test]                             option_factory_tests_make_type.cpp
*
*   Section II - make_option_t, the compile-time option factory.  It is a thin
* alias for option<_Key, _Args...>, so the tests confirm it forwards the key
* and the arg pack verbatim - identity, order, and count - all the way through
* to option<>'s observable surface (::key, ::key_type, ::has_args, ::arg_count,
* ::args_type), including the empty-pack (unary) edge and non-enum key types.
* All compile-time.
*
*
* path:      /tests/djinterp/core/option/option_factory_tests_make_type.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./option_factory_tests.hpp"


NS_DJINTERP
NS_TESTING


::djinterp::test::block_spec
option_factory_make_type_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "II.  make_option_t";
    block.descriptor = "compile-time factory: option<_Key, _Args...> forwarding";

    // ---- forwarding to option<>, by arity --------------------------------

    block.tests.push_back({
        "make_type_no_args",
        "empty arg pack -> the unary option form",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::make_option_t<fkey::alpha>,
                ::djinterp::option<fkey::alpha> >;
            static_assert(ok, "make_option_t<k> == option<k>");
            return ok;
        } });

    block.tests.push_back({
        "make_type_one_arg",
        "a single arg is forwarded as-is",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::make_option_t<fkey::alpha, arg_a>,
                ::djinterp::option<fkey::alpha, arg_a> >;
            static_assert(ok, "make_option_t<k,a> == option<k,a>");
            return ok;
        } });

    block.tests.push_back({
        "make_type_many_args",
        "a multi-arg pack is forwarded in full",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::make_option_t<fkey::alpha, arg_a, arg_b, arg_c>,
                ::djinterp::option<fkey::alpha, arg_a, arg_b, arg_c> >;
            static_assert(ok, "make_option_t<k,a,b,c> == option<k,a,b,c>");
            return ok;
        } });

    block.tests.push_back({
        "make_type_arg_order_preserved",
        "arg order is significant and preserved",
        []() -> bool
        {
            constexpr bool ok = !same_v<
                ::djinterp::make_option_t<fkey::alpha, arg_a, arg_b>,
                ::djinterp::option<fkey::alpha, arg_b, arg_a> >;
            static_assert(ok, "<...,a,b> != option<...,b,a>");
            return ok;
        } });

    block.tests.push_back({
        "make_type_non_enum_keys",
        "int and char keys forward just like enum keys",
        []() -> bool
        {
            constexpr bool int_ok = same_v<
                ::djinterp::make_option_t<42, arg_a>,
                ::djinterp::option<42, arg_a> >;
            constexpr bool char_ok = same_v<
                ::djinterp::make_option_t<'c'>,
                ::djinterp::option<'c'> >;
            static_assert(int_ok && char_ok, "non-enum key forwarding");
            return (int_ok && char_ok);
        } });

    // ---- forwarding reaches option<>'s observable surface ----------------

    block.tests.push_back({
        "make_type_key_surface",
        "the produced option reports the key and its type",
        []() -> bool
        {
            constexpr bool ok =
                ( ::djinterp::make_option_t<fkey::alpha>::key == fkey::alpha ) &&
                same_v< ::djinterp::make_option_t<fkey::alpha>::key_type, fkey >;
            static_assert(ok, "::key / ::key_type");
            return ok;
        } });

    block.tests.push_back({
        "make_type_unary_flags",
        "the unary form reports no args",
        []() -> bool
        {
            constexpr bool ok =
                ( ::djinterp::make_option_t<fkey::alpha>::has_args == false ) &&
                ( ::djinterp::make_option_t<fkey::alpha>::arg_count == 0u );
            static_assert(ok, "unary: has_args=false, arg_count=0");
            return ok;
        } });

    block.tests.push_back({
        "make_type_args_flags",
        "the args form reports the right count",
        []() -> bool
        {
            constexpr bool ok =
                ( ::djinterp::make_option_t<fkey::alpha, arg_a, arg_b>::has_args == true ) &&
                ( ::djinterp::make_option_t<fkey::alpha, arg_a, arg_b>::arg_count == 2u );
            static_assert(ok, "args: has_args=true, arg_count=2");
            return ok;
        } });

    block.tests.push_back({
        "make_type_args_type_tuple",
        "args surface as std::tuple<...> in order",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::make_option_t<fkey::alpha, arg_a, arg_b>::args_type,
                std::tuple<arg_a, arg_b> >;
            static_assert(ok, "args_type == tuple<arg_a, arg_b>");
            return ok;
        } });

    // ---- args are uninterpreted typenames --------------------------------

    block.tests.push_back({
        "make_type_args_are_opaque",
        "even a key_carrier is just an arg - nothing inspects it",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::make_option_t<fkey::alpha, ::djinterp::key_carrier<fkey::beta>>,
                ::djinterp::option<fkey::alpha, ::djinterp::key_carrier<fkey::beta>> >;
            static_assert(ok, "arbitrary types pass through untouched");
            return ok;
        } });

    return block;
}


NS_END  // testing
NS_END  // djinterp
