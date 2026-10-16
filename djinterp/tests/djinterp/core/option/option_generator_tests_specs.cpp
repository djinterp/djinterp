/*******************************************************************************
* djinterp [test]                               option_generator_tests_specs.cpp
*
*   Section I - the spec carriers.  Each parsed token becomes one spec that
* knows the option<> it emits (::option_t) and the runtime value that seeds its
* slot (::initial()).  Covers kv_spec (valued: option<key, field<T>> seeded
* with the value, T taken through remove_cvref - int, char, and a class-type
* fixed_string NTTP), k_spec (presence-only: unary_option<key> seeded with
* unit{}), and first_arg_type (the inferred key type = decay of the first
* token).  C++20-gated (the whole header self-suppresses below it).
*
*
* path:      /tests/djinterp/core/option/option_generator_tests_specs.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./option_generator_tests.hpp"


NS_DJINTERP
NS_TESTING


::djinterp::test::block_spec
option_generator_specs_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "I.  spec carriers";
    block.descriptor = "kv_spec / k_spec option_t + initial(), first_arg_type";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    namespace osi = ::djinterp::internal;

    // ---- kv_spec::option_t : option<key, field<decayed value type>> ------

    block.tests.push_back({
        "kv_spec_option_t_int",
        "an int value yields option<key, field<int>>",
        []() -> bool
        {
            constexpr bool ok = same_v<
                osi::kv_spec<setting::width, 1024>::option_t,
                ::djinterp::option<setting::width, ::djinterp::field<int>> >;
            static_assert(ok, "kv_spec<width,1024>::option_t");
            return ok;
        } });

    block.tests.push_back({
        "kv_spec_option_t_char",
        "a char value yields option<key, field<char>>",
        []() -> bool
        {
            constexpr bool ok = same_v<
                osi::kv_spec<setting::width, 'c'>::option_t,
                ::djinterp::option<setting::width, ::djinterp::field<char>> >;
            static_assert(ok, "kv_spec<width,'c'>::option_t");
            return ok;
        } });

    block.tests.push_back({
        "kv_spec_option_t_string",
        "a class-type NTTP (fixed_string) yields field<fixed_string<N>>",
        []() -> bool
        {
            constexpr bool ok = same_v<
                osi::kv_spec<setting::name, ::djinterp::fixed_string("box")>::option_t,
                ::djinterp::option<setting::name,
                    ::djinterp::field< ::djinterp::fixed_string<4> >> >;
            static_assert(ok, "kv_spec<name,\"box\">::option_t");
            return ok;
        } });

    // ---- kv_spec::initial() : the seed value, decayed ---------------------

    block.tests.push_back({
        "kv_spec_initial_int_value",
        "initial() returns the bound int value",
        []() -> bool
        {
            constexpr int v = osi::kv_spec<setting::width, 1024>::initial();
            static_assert(v == 1024, "kv_spec<width,1024>::initial() == 1024");
            return (v == 1024);
        } });

    block.tests.push_back({
        "kv_spec_initial_int_type",
        "initial()'s type matches the field's element type",
        []() -> bool
        {
            constexpr bool ok = same_v<
                decltype(osi::kv_spec<setting::width, 1024>::initial()), int >;
            static_assert(ok, "decltype(initial()) == int");
            return ok;
        } });

    block.tests.push_back({
        "kv_spec_initial_string",
        "initial() carries the string payload",
        []() -> bool
        {
            constexpr bool ok =
                ( osi::kv_spec<setting::name, ::djinterp::fixed_string("box")>::initial().view()
                  == std::string_view("box") ) &&
                same_v< decltype(osi::kv_spec<setting::name,
                            ::djinterp::fixed_string("box")>::initial()),
                        ::djinterp::fixed_string<4> >;
            static_assert(ok, "kv_spec<name,\"box\">::initial()");
            return ok;
        } });

    // ---- k_spec : presence-only carrier ----------------------------------

    block.tests.push_back({
        "k_spec_option_t",
        "a lone key yields unary_option<key> (option<key, field<unit>>)",
        []() -> bool
        {
            constexpr bool ok =
                same_v< osi::k_spec<setting::visible>::option_t,
                        ::djinterp::unary_option<setting::visible> > &&
                same_v< osi::k_spec<setting::visible>::option_t,
                        ::djinterp::option<setting::visible,
                            ::djinterp::field< ::djinterp::unit >> >;
            static_assert(ok, "k_spec<visible>::option_t");
            return ok;
        } });

    block.tests.push_back({
        "k_spec_initial_is_unit",
        "initial() of a presence-only key is unit",
        []() -> bool
        {
            constexpr bool ok = same_v<
                decltype(osi::k_spec<setting::visible>::initial()),
                ::djinterp::unit >;
            static_assert(ok, "decltype(k_spec::initial()) == unit");
            return ok;
        } });

    // ---- first_arg_type : the inferred key type --------------------------

    block.tests.push_back({
        "first_arg_type_basic",
        "the key type is the decayed type of the first token",
        []() -> bool
        {
            constexpr bool ok = same_v<
                osi::first_arg_type<setting::width, 1024, setting::height>::type,
                setting >;
            static_assert(ok, "first_arg_type<width,1024,height> == setting");
            return ok;
        } });

    block.tests.push_back({
        "first_arg_type_single",
        "a single-token stream still infers its key type",
        []() -> bool
        {
            constexpr bool ok = same_v<
                osi::first_arg_type<setting::visible>::type, setting >;
            static_assert(ok, "first_arg_type<visible> == setting");
            return ok;
        } });

    block.tests.push_back({
        "first_arg_type_keys_off_first_only",
        "only the first token decides the key type; later tokens are ignored",
        []() -> bool
        {
            // trailing tokens are various non-key types - none affects the result
            constexpr bool ok = same_v<
                osi::first_arg_type<setting::width, 'c', 3, true>::type, setting >;
            static_assert(ok, "first_arg_type keys off _First only");
            return ok;
        } });

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
