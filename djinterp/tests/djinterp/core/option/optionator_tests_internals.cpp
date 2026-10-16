/*******************************************************************************
* djinterp [test]                                 optionator_tests_internals.cpp
*
*   Section IV - the pipeline's pure internals.  identity_xform is the seed
* transform a fresh chain carries: it is a genuine transducer (so operator|
* composition and the through() compose branch accept it) whose reducer is the
* downstream reducer unchanged.  normalize_options canonicalizes optionator's
* variadic argument into a single option_set: a lone option_set passes through
* untouched, while a multi-surface pack is delegated to compose_options_t.
* (drive_into_consumer - the loop that threads a producer through a transducer
* into a consumer - is exercised end to end by the pipeline section.)  C++17-gated.
*
*
* path:      /tests/djinterp/core/option/optionator_tests_internals.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./optionator_tests.hpp"


NS_DJINTERP
NS_TESTING


::djinterp::test::block_spec
optionator_internals_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "IV.  pipeline internals";
    block.descriptor = "identity_xform + normalize_options";

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    namespace osi = ::djinterp::internal;

    // ---- identity_xform ---------------------------------------------------

    block.tests.push_back({
        "identity_is_transducer",
        "the seed transform is a real transducer",
        []() -> bool
        {
            constexpr bool ok = ::djinterp::is_transducer<osi::identity_xform>::value;
            static_assert(ok, "identity_xform is a transducer");
            return ok;
        } });

    block.tests.push_back({
        "identity_returns_downstream_unchanged",
        "applying identity_xform yields the downstream reducer's own type",
        []() -> bool
        {
            auto reducer = [](auto&, const auto&) {};
            // operator()(R down) const -> R : the reducer type is preserved
            constexpr bool ok = same_v<
                decltype(osi::identity_xform{}(reducer)),
                decltype(reducer) >;
            static_assert(ok, "identity is type-preserving");
            (void)reducer;
            return ok;
        } });

    // ---- normalize_options ------------------------------------------------

    block.tests.push_back({
        "normalize_single_set_passthrough",
        "a lone option_set is carried through unchanged",
        []() -> bool
        {
            constexpr bool ok = same_v<
                typename osi::normalize_options<env>::type, env >;
            static_assert(ok, "single set passes through");
            return ok;
        } });

    block.tests.push_back({
        "normalize_single_set_generic",
        "the passthrough holds for any single option_set, not just env",
        []() -> bool
        {
            using one = ::djinterp::option_set<
                ::djinterp::option<cfg::count, env_val<9>> >;
            constexpr bool ok = same_v<
                typename osi::normalize_options<one>::type, one >;
            static_assert(ok, "generic single set passes through");
            return ok;
        } });

    block.tests.push_back({
        "normalize_pack_delegates_to_compose",
        "a multi-surface pack is composed, not passed through",
        []() -> bool
        {
            using a = ::djinterp::option_set< ::djinterp::option<cfg::count,  env_val<1>> >;
            using b = ::djinterp::option_set< ::djinterp::option<cfg::factor, env_val<2>> >;
            // two surfaces -> the primary template -> compose_options_t
            constexpr bool ok = same_v<
                typename osi::normalize_options<a, b>::type,
                ::djinterp::compose_options_t<a, b> >;
            static_assert(ok, "pack delegates to compose_options_t");
            return ok;
        } });

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
