/*******************************************************************************
* djinterp [test]                            option_generator_tests_frontend.cpp
*
*   Section IV - the public front-end.  option_generator parses a flat stream
* and exposes the key type, the spec list, the populated option_set type, and
* a constructed instance; option_set_t is the type face and make_option_set the
* value face.  These tests run the whole pipeline end to end: the emitted type
* (identity, size, is_option_set, contains, find, values_type) and the built
* instance (typed get<> values, a fixed_string, mutability via set<>, and the
* three unary shapes - trailing, key-before-key, and a lone key).  C++20-gated.
*
*
* path:      /tests/djinterp/core/option/option_generator_tests_frontend.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./option_generator_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// gen_full
//   the running example from the header: two valued ints, a unary flag, and a
// fixed_string value - the canonical generated set the type-level tests probe.
using gen_full = ::djinterp::option_generator<
    setting::width,  1024,
    setting::height, 768,
    setting::visible,
    setting::name,   ::djinterp::fixed_string("box") >;

// full_set
//   the option_set type gen_full produces, spelled out once for identity tests.
using full_set = ::djinterp::option_set<
    ::djinterp::option<setting::width,  ::djinterp::field<int>>,
    ::djinterp::option<setting::height, ::djinterp::field<int>>,
    ::djinterp::unary_option<setting::visible>,
    ::djinterp::option<setting::name,   ::djinterp::field< ::djinterp::fixed_string<4> >> >;
#endif


::djinterp::test::block_spec
option_generator_frontend_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "IV.  public front-end";
    block.descriptor = "option_generator / option_set_t / make_option_set end-to-end";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // ---- option_generator : exposed type-level products ------------------

    block.tests.push_back({
        "front_key_type_inferred",
        "the key type is inferred from the first token",
        []() -> bool
        {
            constexpr bool ok = same_v< gen_full::key_type, setting >;
            static_assert(ok, "gen::key_type == setting");
            return ok;
        } });

    block.tests.push_back({
        "front_emitted_type",
        "the generator emits the fully spelled-out option_set",
        []() -> bool
        {
            constexpr bool ok = same_v< gen_full::type, full_set >;
            static_assert(ok, "gen::type == full_set");
            return ok;
        } });

    block.tests.push_back({
        "front_specs_exposed",
        "the generator exposes the parsed spec list",
        []() -> bool
        {
            namespace osi = ::djinterp::internal;
            constexpr bool ok = same_v<
                ::djinterp::option_generator<setting::width, 1024, setting::visible>::specs,
                std::tuple< osi::kv_spec<setting::width, 1024>,
                            osi::k_spec<setting::visible> > >;
            static_assert(ok, "gen::specs");
            return ok;
        } });

    block.tests.push_back({
        "front_option_set_t_is_generator_type",
        "option_set_t is the generator's ::type face",
        []() -> bool
        {
            constexpr bool ok =
                same_v< ::djinterp::option_set_t<setting::width, 1024, setting::visible>,
                        ::djinterp::option_set<
                            ::djinterp::option<setting::width, ::djinterp::field<int>>,
                            ::djinterp::unary_option<setting::visible> > > &&
                same_v< ::djinterp::option_set_t<setting::width, 1024, setting::visible>,
                        ::djinterp::option_generator<setting::width, 1024, setting::visible>::type >;
            static_assert(ok, "option_set_t == generator::type");
            return ok;
        } });

    block.tests.push_back({
        "front_is_option_set_and_size",
        "the result is an option_set of the right size",
        []() -> bool
        {
            constexpr bool ok =
                ::djinterp::is_option_set_v<gen_full::type> &&
                ( gen_full::type::size == 4u );
            static_assert(ok, "is_option_set + size == 4");
            return ok;
        } });

    block.tests.push_back({
        "front_contains_query",
        "the set contains its keys and only its keys",
        []() -> bool
        {
            constexpr bool ok =
                ::djinterp::option_set_contains_v<gen_full::type, setting::visible> &&
                ::djinterp::option_set_contains_v<gen_full::type, setting::width>   &&
                !::djinterp::option_set_contains_v<gen_full::type, setting::depth>;
            static_assert(ok, "contains visible/width, not depth");
            return ok;
        } });

    block.tests.push_back({
        "front_find_valued_and_unary",
        "find recovers the emitted option for valued and unary keys",
        []() -> bool
        {
            constexpr bool ok =
                same_v< ::djinterp::option_set_find_t<gen_full::type, setting::width>,
                        ::djinterp::option<setting::width, ::djinterp::field<int>> > &&
                same_v< ::djinterp::option_set_find_t<gen_full::type, setting::visible>,
                        ::djinterp::unary_option<setting::visible> >;
            static_assert(ok, "find width (valued) / visible (unary)");
            return ok;
        } });

    block.tests.push_back({
        "front_values_type",
        "the slot tuple mirrors the field types in order",
        []() -> bool
        {
            constexpr bool ok = same_v<
                gen_full::type::values_type,
                std::tuple<int, int, ::djinterp::unit, ::djinterp::fixed_string<4>> >;
            static_assert(ok, "values_type == <int,int,unit,fixed_string<4>>");
            return ok;
        } });

    // ---- make_option_set : the constructed instance ----------------------

    block.tests.push_back({
        "front_make_typed_values",
        "get<> returns each key's seeded value at its exact type",
        []() -> bool
        {
            constexpr auto cfg = ::djinterp::make_option_set<
                setting::width,  1024,
                setting::height, 768,
                setting::visible,
                setting::name,   ::djinterp::fixed_string("box") >();
            constexpr bool ok =
                ( cfg.get<setting::width>()  == 1024 ) &&
                ( cfg.get<setting::height>() == 768 )  &&
                same_v< std::remove_cvref_t<decltype(cfg.get<setting::width>())>, int >;
            static_assert(ok, "make: typed int values");
            return ok;
        } });

    block.tests.push_back({
        "front_make_string_value",
        "a fixed_string value survives to get<>",
        []() -> bool
        {
            constexpr auto cfg = ::djinterp::make_option_set<
                setting::name, ::djinterp::fixed_string("box") >();
            constexpr bool ok =
                ( cfg.get<setting::name>().view() == std::string_view("box") );
            static_assert(ok, "make: string value");
            return ok;
        } });

    block.tests.push_back({
        "front_make_contains",
        "contains<> reflects presence, including the unary flag",
        []() -> bool
        {
            constexpr auto cfg = ::djinterp::make_option_set<
                setting::width, 1024, setting::visible >();
            constexpr bool ok =
                cfg.contains<setting::visible>() &&
                cfg.contains<setting::width>()   &&
                !cfg.contains<setting::depth>();
            static_assert(ok, "make: contains visible/width, not depth");
            return ok;
        } });

    block.tests.push_back({
        "front_make_mutable",
        "the built set is mutable through set<> at runtime",
        []() -> bool
        {
            auto cfg = ::djinterp::make_option_set<
                setting::width, 1024, setting::height, 768 >();
            cfg.set<setting::width>(1280);
            return ( cfg.get<setting::width>()  == 1280 ) &&
                   ( cfg.get<setting::height>() == 768 );
        } });

    // ---- the three unary shapes, end to end ------------------------------

    block.tests.push_back({
        "front_make_trailing_unary",
        "a trailing lone key becomes a flag alongside a valued key",
        []() -> bool
        {
            constexpr auto cfg = ::djinterp::make_option_set<
                setting::width, 1024, setting::visible >();
            constexpr bool ok =
                ( cfg.get<setting::width>() == 1024 ) &&
                cfg.contains<setting::visible>()      &&
                ( decltype(cfg)::size == 2u );
            static_assert(ok, "trailing unary shape");
            return ok;
        } });

    block.tests.push_back({
        "front_make_key_before_key_unary",
        "a key immediately before another key becomes a flag",
        []() -> bool
        {
            constexpr auto cfg = ::djinterp::make_option_set<
                setting::visible, setting::width, 1024 >();
            constexpr bool ok =
                cfg.contains<setting::visible>() &&
                ( cfg.get<setting::width>() == 1024 );
            static_assert(ok, "key-before-key unary shape");
            return ok;
        } });

    block.tests.push_back({
        "front_make_single_unary",
        "a one-key stream builds a single-flag set",
        []() -> bool
        {
            constexpr auto cfg = ::djinterp::make_option_set<setting::visible>();
            constexpr bool ok =
                cfg.contains<setting::visible>() &&
                !cfg.contains<setting::width>()  &&
                ( decltype(cfg)::size == 1u );
            static_assert(ok, "single unary shape");
            return ok;
        } });

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
