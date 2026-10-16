/*******************************************************************************
* djinterp [test]                                option_generator_tests_emit.cpp
*
*   Section III - the emitter.  emit_set maps a spec tuple to the populated
* option_set: ::type is option_set< each spec's option_t... > and make()
* constructs it from the specs' initial() values in slot order.  These tests
* pin the type mapping (valued, unary, mixed, and the empty edge) and verify
* make() seeds each key's slot with the right value (ints and a fixed_string),
* addressed back through get<> / contains<>.  C++20-gated.
*
*
* path:      /tests/djinterp/core/option/option_generator_tests_emit.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./option_generator_tests.hpp"


NS_DJINTERP
NS_TESTING


::djinterp::test::block_spec
option_generator_emit_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "III.  emitter";
    block.descriptor = "emit_set: specs -> option_set type + value-seeding make()";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    namespace osi = ::djinterp::internal;

    // spec-tuple fixtures (what the parser hands the emitter)
    using specs_two = std::tuple<
        osi::kv_spec<setting::width, 1024>,
        osi::kv_spec<setting::height, 768> >;
    using specs_mixed = std::tuple<
        osi::kv_spec<setting::width, 1024>,
        osi::k_spec<setting::visible>,
        osi::kv_spec<setting::name, ::djinterp::fixed_string("box")> >;
    using specs_unary = std::tuple<
        osi::k_spec<setting::visible>,
        osi::k_spec<setting::mode> >;
    using specs_seed = std::tuple<
        osi::kv_spec<setting::width, 10>,
        osi::k_spec<setting::visible>,
        osi::kv_spec<setting::height, 20> >;
    using specs_str = std::tuple<
        osi::kv_spec<setting::name, ::djinterp::fixed_string("box")> >;

    // ---- ::type : specs mapped to option_set< option_t... > --------------

    block.tests.push_back({
        "emit_type_valued",
        "valued specs map to option<key, field<T>> options",
        []() -> bool
        {
            constexpr bool ok = same_v<
                osi::emit_set<specs_two>::type,
                ::djinterp::option_set<
                    ::djinterp::option<setting::width,  ::djinterp::field<int>>,
                    ::djinterp::option<setting::height, ::djinterp::field<int>> > >;
            static_assert(ok, "emit valued type");
            return ok;
        } });

    block.tests.push_back({
        "emit_type_mixed",
        "a mix of valued and unary specs maps in order",
        []() -> bool
        {
            constexpr bool ok = same_v<
                osi::emit_set<specs_mixed>::type,
                ::djinterp::option_set<
                    ::djinterp::option<setting::width, ::djinterp::field<int>>,
                    ::djinterp::unary_option<setting::visible>,
                    ::djinterp::option<setting::name,
                        ::djinterp::field< ::djinterp::fixed_string<4> >> > >;
            static_assert(ok, "emit mixed type");
            return ok;
        } });

    block.tests.push_back({
        "emit_type_all_unary",
        "unary specs map to unary_option slots",
        []() -> bool
        {
            constexpr bool ok = same_v<
                osi::emit_set<specs_unary>::type,
                ::djinterp::option_set<
                    ::djinterp::unary_option<setting::visible>,
                    ::djinterp::unary_option<setting::mode> > >;
            static_assert(ok, "emit all-unary type");
            return ok;
        } });

    block.tests.push_back({
        "emit_type_empty",
        "the empty spec tuple maps to the empty option_set",
        []() -> bool
        {
            constexpr bool ok = same_v<
                osi::emit_set<std::tuple<>>::type,
                ::djinterp::option_set<> >;
            static_assert(ok, "emit empty type");
            return ok;
        } });

    // ---- values_type : one slot per option -------------------------------

    block.tests.push_back({
        "emit_values_type_slots",
        "the mixed set's slot tuple is <int, unit, fixed_string<4>>",
        []() -> bool
        {
            constexpr bool ok = same_v<
                osi::emit_set<specs_mixed>::type::values_type,
                std::tuple<int, ::djinterp::unit, ::djinterp::fixed_string<4>> >;
            static_assert(ok, "emit values_type");
            return ok;
        } });

    // ---- make() : seed each slot in order --------------------------------

    block.tests.push_back({
        "emit_make_seeds_values",
        "make() seeds valued slots and leaves unary slots as presence",
        []() -> bool
        {
            constexpr auto s = osi::emit_set<specs_seed>::make();
            constexpr bool ok =
                ( s.get<setting::width>()  == 10 ) &&
                ( s.get<setting::height>() == 20 ) &&
                s.contains<setting::visible>();
            static_assert(ok, "emit make seeds width/height, visible present");
            return ok;
        } });

    block.tests.push_back({
        "emit_make_slot_order",
        "each key resolves to its own seeded value (order preserved)",
        []() -> bool
        {
            // distinct values so a mis-ordered seeding would be caught
            using specs_order = std::tuple<
                osi::kv_spec<setting::width, 111>,
                osi::kv_spec<setting::height, 222>,
                osi::kv_spec<setting::depth, 333> >;
            constexpr auto s = osi::emit_set<specs_order>::make();
            constexpr bool ok =
                ( s.get<setting::width>()  == 111 ) &&
                ( s.get<setting::height>() == 222 ) &&
                ( s.get<setting::depth>()  == 333 );
            static_assert(ok, "emit make preserves slot order");
            return ok;
        } });

    block.tests.push_back({
        "emit_make_string_value",
        "make() seeds a fixed_string slot with its payload",
        []() -> bool
        {
            constexpr auto s = osi::emit_set<specs_str>::make();
            constexpr bool ok =
                ( s.get<setting::name>().view() == std::string_view("box") );
            static_assert(ok, "emit make string slot");
            return ok;
        } });

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
