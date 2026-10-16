/*******************************************************************************
* djinterp [test]                                    optionator_tests_traits.cpp
*
*   Section VII - the public identification surface.  is_optionator recognizes
* an unsourced optionator<...> head; is_option_chain recognizes a sourced
* pipeline; both strip cv / reference via clean_t in their _v forms.  On C++20
* with concepts, Optionator / OptionChain / ConfigurableStage restate those
* verdicts as concepts.  These tests cover the positive and negative cases and
* the cv/ref stripping, with the concept checks nested behind the same double
* gate the header uses.  C++17-gated overall (optionator self-suppresses below).
*
*
* path:      /tests/djinterp/core/option/optionator_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./optionator_tests.hpp"


NS_DJINTERP
NS_TESTING


::djinterp::test::block_spec
optionator_traits_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "VII.  traits & concepts";
    block.descriptor = "is_optionator / is_option_chain / concepts";

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    // ---- is_optionator ----------------------------------------------------

    block.tests.push_back({
        "is_optionator_true",
        "an optionator<...> head is recognized (struct and _v forms)",
        []() -> bool
        {
            constexpr bool s = ::djinterp::is_optionator<
                ::djinterp::optionator<env>>::value;
            constexpr bool v = ::djinterp::is_optionator_v<
                ::djinterp::optionator<env>>;
            static_assert(s && v, "optionator is an optionator");
            return (s && v);
        } });

    block.tests.push_back({
        "is_optionator_false",
        "non-heads (a plain type, an option_set, a chain) are rejected",
        []() -> bool
        {
            auto ch = ::djinterp::optionator<env>{} | stage<src_tag>();
            constexpr bool a = ::djinterp::is_optionator_v<int>;
            constexpr bool b = ::djinterp::is_optionator_v<env>;   // an option_set
            bool           c = ::djinterp::is_optionator_v<decltype(ch)>;
            static_assert(!a && !b, "non-optionators rejected");
            return (!a && !b && !c);
        } });

    block.tests.push_back({
        "is_optionator_strips_cvref",
        "is_optionator_v cleans cv / reference before matching",
        []() -> bool
        {
            using opt = ::djinterp::optionator<env>;
            constexpr bool ok =
                ::djinterp::is_optionator_v<const opt>  &&
                ::djinterp::is_optionator_v<opt&>       &&
                ::djinterp::is_optionator_v<const opt&>;
            static_assert(ok, "cv/ref stripped");
            return ok;
        } });

    // ---- is_option_chain --------------------------------------------------

    block.tests.push_back({
        "is_option_chain_true",
        "a sourced pipeline is recognized (struct and _v forms)",
        []() -> bool
        {
            auto ch = ::djinterp::optionator<env>{} | stage<src_tag>();
            using chain = decltype(ch);
            constexpr bool s = ::djinterp::is_option_chain<chain>::value;
            constexpr bool v = ::djinterp::is_option_chain_v<chain>;
            static_assert(s && v, "chain is an option_chain");
            (void)ch;
            return (s && v);
        } });

    block.tests.push_back({
        "is_option_chain_false",
        "an optionator head and a plain type are rejected",
        []() -> bool
        {
            constexpr bool a = ::djinterp::is_option_chain_v<
                ::djinterp::optionator<env>>;
            constexpr bool b = ::djinterp::is_option_chain_v<int>;
            static_assert(!a && !b, "non-chains rejected");
            return (!a && !b);
        } });

    block.tests.push_back({
        "is_option_chain_strips_cvref",
        "is_option_chain_v cleans cv / reference before matching",
        []() -> bool
        {
            auto ch = ::djinterp::optionator<env>{} | stage<src_tag>();
            using chain = decltype(ch);
            constexpr bool ok =
                ::djinterp::is_option_chain_v<const chain>  &&
                ::djinterp::is_option_chain_v<chain&>       &&
                ::djinterp::is_option_chain_v<const chain&>;
            static_assert(ok, "cv/ref stripped");
            (void)ch;
            return ok;
        } });

#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // ---- concepts (C++20) -------------------------------------------------

    block.tests.push_back({
        "concept_optionator",
        "Optionator is satisfied by a head and by nothing else",
        []() -> bool
        {
            constexpr bool yes = ::djinterp::Optionator< ::djinterp::optionator<env> >;
            constexpr bool no  = ::djinterp::Optionator< int >;
            static_assert(yes && !no, "Optionator concept");
            return (yes && !no);
        } });

    block.tests.push_back({
        "concept_option_chain",
        "OptionChain is satisfied by a pipeline, not by a head",
        []() -> bool
        {
            auto ch = ::djinterp::optionator<env>{} | stage<src_tag>();
            constexpr bool yes = ::djinterp::OptionChain< decltype(ch) >;
            constexpr bool no  = ::djinterp::OptionChain< ::djinterp::optionator<env> >;
            static_assert(yes && !no, "OptionChain concept");
            (void)ch;
            return (yes && !no);
        } });

    block.tests.push_back({
        "concept_configurable_stage",
        "ConfigurableStage tracks adapter presence",
        []() -> bool
        {
            constexpr bool yes = ::djinterp::ConfigurableStage< src_tag >;
            constexpr bool no  = ::djinterp::ConfigurableStage< unconfigured_tag >;
            static_assert(yes && !no, "ConfigurableStage concept");
            return (yes && !no);
        } });

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
