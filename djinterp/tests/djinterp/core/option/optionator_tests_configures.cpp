/*******************************************************************************
* djinterp [test]                                optionator_tests_configures.cpp
*
*   Section I - the Pattern-C adapter trait.  configures<_Tag> is the opt-in
* point: its primary is left undefined so an unconfigured tag is a hard, named
* error, and is_configurable_stage<_Tag> reports whether an adapter exists by
* detecting whether configures<_Tag> is a COMPLETE type.  These tests cover
* both verdicts across the suite's tags.  C++17-gated (optionator self-
* suppresses below it).
*
*
* path:      /tests/djinterp/core/option/optionator_tests_configures.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./optionator_tests.hpp"


NS_DJINTERP
NS_TESTING


::djinterp::test::block_spec
optionator_configures_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "I.  configures";
    block.descriptor = "is_configurable_stage: adapter presence via completeness";

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    block.tests.push_back({
        "configurable_source",
        "a tag with a source adapter is configurable",
        []() -> bool
        {
            constexpr bool ok = ::djinterp::is_configurable_stage<src_tag>::value;
            static_assert(ok, "src_tag is configurable");
            return ok;
        } });

    block.tests.push_back({
        "configurable_transforms_and_terminal",
        "transform and terminal tags with adapters are configurable",
        []() -> bool
        {
            constexpr bool ok =
                ::djinterp::is_configurable_stage<scale_tag>::value &&
                ::djinterp::is_configurable_stage<over_tag>::value  &&
                ::djinterp::is_configurable_stage<sink_tag>::value;
            static_assert(ok, "scale/over/sink configurable");
            return ok;
        } });

    block.tests.push_back({
        "configurable_runtime_source",
        "the runtime-argument source tag is configurable too",
        []() -> bool
        {
            constexpr bool ok = ::djinterp::is_configurable_stage<count_tag>::value;
            static_assert(ok, "count_tag is configurable");
            return ok;
        } });

    block.tests.push_back({
        "not_configurable_without_adapter",
        "a tag with no configures<> specialization is NOT configurable",
        []() -> bool
        {
            constexpr bool ok = !::djinterp::is_configurable_stage<unconfigured_tag>::value;
            static_assert(ok, "unconfigured_tag is not configurable");
            return ok;
        } });

    block.tests.push_back({
        "configurable_is_bool_trait",
        "the trait yields a usable compile-time bool either way",
        []() -> bool
        {
            // both verdicts are plain bool constants (true_type / false_type)
            constexpr bool yes = ::djinterp::is_configurable_stage<src_tag>::value;
            constexpr bool no  = ::djinterp::is_configurable_stage<unconfigured_tag>::value;
            static_assert(yes && !no, "trait is a bool constant");
            return (yes && !no);
        } });

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
