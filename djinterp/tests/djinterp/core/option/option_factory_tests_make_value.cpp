/*******************************************************************************
* djinterp [test]                            option_factory_tests_make_value.cpp
*
*   Section III - make_option(), the runtime option factory.  It returns a
* value-initialized option<_Key, _Args...>, so the tests pin down its return
* type (via decltype, and cross-checked against make_option_t), its noexcept
* guarantee, its advertised constexpr usability, and a genuine runtime
* construct-and-read - across arities, key types, and arg order.
*
*
* path:      /tests/djinterp/core/option/option_factory_tests_make_value.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./option_factory_tests.hpp"


NS_DJINTERP
NS_TESTING


::djinterp::test::block_spec
option_factory_make_value_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "III.  make_option";
    block.descriptor = "runtime factory: return type, noexcept, constexpr + runtime use";

    // ---- return type, by arity -------------------------------------------

    block.tests.push_back({
        "make_value_return_no_args",
        "no args -> returns the unary option",
        []() -> bool
        {
            constexpr bool ok = same_v<
                decltype(::djinterp::make_option<fkey::alpha>()),
                ::djinterp::option<fkey::alpha> >;
            static_assert(ok, "decltype == option<k>");
            return ok;
        } });

    block.tests.push_back({
        "make_value_return_args",
        "with args -> returns the args option in order",
        []() -> bool
        {
            constexpr bool ok = same_v<
                decltype(::djinterp::make_option<fkey::alpha, arg_a, arg_b>()),
                ::djinterp::option<fkey::alpha, arg_a, arg_b> >;
            static_assert(ok, "decltype == option<k,a,b>");
            return ok;
        } });

    block.tests.push_back({
        "make_value_matches_make_type",
        "the runtime and compile-time factories agree on the type",
        []() -> bool
        {
            constexpr bool ok = same_v<
                decltype(::djinterp::make_option<fkey::alpha, arg_a>()),
                ::djinterp::make_option_t<fkey::alpha, arg_a> >;
            static_assert(ok, "decltype(make_option) == make_option_t");
            return ok;
        } });

    block.tests.push_back({
        "make_value_arg_order_preserved",
        "arg order is significant in the returned type",
        []() -> bool
        {
            constexpr bool ok = !same_v<
                decltype(::djinterp::make_option<fkey::alpha, arg_a, arg_b>()),
                ::djinterp::option<fkey::alpha, arg_b, arg_a> >;
            static_assert(ok, "returned <...,a,b> != option<...,b,a>");
            return ok;
        } });

    block.tests.push_back({
        "make_value_non_enum_keys",
        "int and char keys return the matching option",
        []() -> bool
        {
            constexpr bool int_ok = same_v<
                decltype(::djinterp::make_option<42>()),
                ::djinterp::option<42> >;
            constexpr bool char_ok = same_v<
                decltype(::djinterp::make_option<'c', arg_a>()),
                ::djinterp::option<'c', arg_a> >;
            static_assert(int_ok && char_ok, "non-enum key return types");
            return (int_ok && char_ok);
        } });

    // ---- noexcept --------------------------------------------------------

    block.tests.push_back({
        "make_value_is_noexcept",
        "the factory is declared noexcept",
        []() -> bool
        {
            constexpr bool ok = noexcept(::djinterp::make_option<fkey::alpha>());
            static_assert(ok, "make_option<...>() is noexcept");
            return ok;
        } });

    // ---- return-type surface reaches through to option<> -----------------

    block.tests.push_back({
        "make_value_return_key_surface",
        "the returned type reports the key and its type",
        []() -> bool
        {
            constexpr bool ok =
                ( decltype(::djinterp::make_option<fkey::alpha>())::key == fkey::alpha ) &&
                same_v< decltype(::djinterp::make_option<fkey::alpha>())::key_type, fkey >;
            static_assert(ok, "returned ::key / ::key_type");
            return ok;
        } });

    block.tests.push_back({
        "make_value_return_arg_count",
        "the returned args type reports the arg count",
        []() -> bool
        {
            constexpr bool ok =
                decltype(::djinterp::make_option<fkey::alpha, arg_a, arg_b>())::arg_count == 2u;
            static_assert(ok, "returned ::arg_count == 2");
            return ok;
        } });

    // ---- advertised constexpr usage --------------------------------------

    block.tests.push_back({
        "make_value_usable_in_constexpr",
        "make_option can initialize a constexpr object (the documented use)",
        []() -> bool
        {
            constexpr auto opt = ::djinterp::make_option<fkey::gamma, arg_a>();
            constexpr bool ok =
                ( opt.key == fkey::gamma ) &&
                same_v< std::remove_const_t<decltype(opt)>,
                        ::djinterp::option<fkey::gamma, arg_a> >;
            static_assert(ok, "constexpr auto opt = make_option<...>()");
            return ok;
        } });

    // ---- genuine runtime construction ------------------------------------

    block.tests.push_back({
        "make_value_runtime_construct",
        "calling the factory at runtime yields a usable option value",
        []() -> bool
        {
            auto opt = ::djinterp::make_option<fkey::delta, arg_a>();
            return ( opt.key == fkey::delta ) &&
                   ( opt.arg_count == 1u )    &&
                   ( opt.has_args == true );
        } });

    return block;
}


NS_END  // testing
NS_END  // djinterp
