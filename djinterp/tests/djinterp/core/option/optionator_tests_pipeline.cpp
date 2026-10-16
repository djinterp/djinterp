/*******************************************************************************
* djinterp [test]                                  optionator_tests_pipeline.cpp
*
*   Sections V + VI - assembly and execution.  optionator<...> is the empty
* entry point carrying the normalized environment as options_t; piping a source
* stage into it seeds an option_chain (producer + identity seed transform), and
* further pipes drive the five operator| overloads: appending a transform
* (through() - identity-replace on the first, compose on the rest), and the
* three terminals (into a supplied consumer, collect to a vector, or a
* configures<>-built configured sink).  These tests assemble real pipelines and
* RUN them, checking emitted values, transform ordering (compose is order-
* sensitive), the is_done short-circuit, re-runnability, the chain typedefs /
* accessors, and that each stage reads the specific carried environment.
* C++17-gated (optionator self-suppresses below it).
*
*
* path:      /tests/djinterp/core/option/optionator_tests_pipeline.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./optionator_tests.hpp"


NS_DJINTERP
NS_TESTING


::djinterp::test::block_spec
optionator_pipeline_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "V+VI.  assembly + operator| execution";
    block.descriptor = "optionator / option_chain / operator| end to end";

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    namespace osi = ::djinterp::internal;

    using ivec = std::vector<int>;

    // ---- optionator::options_t (V) ---------------------------------------

    block.tests.push_back({
        "optionator_options_t_single",
        "optionator normalizes a lone option_set to itself",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::optionator<env>::options_t, env >;
            static_assert(ok, "options_t of a single set");
            return ok;
        } });

    block.tests.push_back({
        "optionator_options_t_pack",
        "optionator composes a multi-surface pack",
        []() -> bool
        {
            using a = ::djinterp::option_set< ::djinterp::option<cfg::count,  env_val<1>> >;
            using b = ::djinterp::option_set< ::djinterp::option<cfg::factor, env_val<2>> >;
            constexpr bool ok = same_v<
                ::djinterp::optionator<a, b>::options_t,
                ::djinterp::compose_options_t<a, b> >;
            static_assert(ok, "options_t of a pack");
            return ok;
        } });

    // ---- seeding: operator|(optionator, pending_stage) [overload 1] ------

    block.tests.push_back({
        "seed_yields_option_chain",
        "piping a source into an optionator seeds an option_chain",
        []() -> bool
        {
            auto ch = ::djinterp::optionator<env>{} | stage<src_tag>();
            return ::djinterp::is_option_chain_v<decltype(ch)>;
        } });

    block.tests.push_back({
        "seed_chain_typedefs",
        "the seeded chain exposes options_t / producer_t / identity xform_t",
        []() -> bool
        {
            auto ch = ::djinterp::optionator<env>{} | stage<src_tag>();
            using chain = decltype(ch);
            constexpr bool ok =
                same_v< typename chain::options_t,  env >           &&
                same_v< typename chain::producer_t, test_producer > &&
                same_v< typename chain::xform_t,    osi::identity_xform >;
            static_assert(ok, "seeded chain typedefs");
            (void)ch;
            return ok;
        } });

    block.tests.push_back({
        "chain_accessors",
        "source() / transducer() expose the held producer and transform",
        []() -> bool
        {
            auto ch = ::djinterp::optionator<env>{} | stage<src_tag>();
            // source read count=5 from env; transform is the identity seed
            return ( ch.source().m_n == 5 &&
                     ::djinterp::is_transducer<
                         std::decay_t<decltype(ch.transducer())>>::value );
        } });

    // ---- collect: operator|(chain, collect_terminal) [overload 5] --------

    block.tests.push_back({
        "collect_no_transform",
        "a source then collect drains the source verbatim",
        []() -> bool
        {
            ivec v = ::djinterp::optionator<env>{}
                   | stage<src_tag>()
                   | collect<int>();
            return (v == ivec{1, 2, 3, 4, 5});
        } });

    // ---- one transform: through() identity branch + overload 2 -----------

    block.tests.push_back({
        "one_transform_scale",
        "a single transform maps every value (identity-replace branch)",
        []() -> bool
        {
            ivec v = ::djinterp::optionator<env>{}
                   | stage<src_tag>()      // 1..5
                   | stage<scale_tag>()    // * factor(10)
                   | collect<int>();
            return (v == ivec{10, 20, 30, 40, 50});
        } });

    // ---- two transforms: through() compose branch + ORDER ----------------

    block.tests.push_back({
        "two_transforms_order_sensitive",
        "composition is order-sensitive (over-then-scale != scale-then-over)",
        []() -> bool
        {
            // over(>3) BEFORE scale: keep 4,5 then *10 -> 40,50
            ivec a = ::djinterp::optionator<env>{}
                   | stage<src_tag>()
                   | stage<over_tag>()
                   | stage<scale_tag>()
                   | collect<int>();

            // scale BEFORE over(>3): *10 -> 10..50, all exceed 3 -> unchanged
            ivec b = ::djinterp::optionator<env>{}
                   | stage<src_tag>()
                   | stage<scale_tag>()
                   | stage<over_tag>()
                   | collect<int>();

            return (a == ivec{40, 50}) &&
                   (b == ivec{10, 20, 30, 40, 50}) &&
                   (a != b);
        } });

    // ---- into: operator|(chain, into_terminal) [overload 3] --------------

    block.tests.push_back({
        "into_supplied_consumer",
        "into(sink) drives the pipeline through a caller-supplied consumer",
        []() -> bool
        {
            ivec out;
            ::djinterp::optionator<env>{}
                | stage<src_tag>()
                | stage<scale_tag>()
                | ::djinterp::into([&out](const int& x) { out.push_back(x); });
            return (out == ivec{10, 20, 30, 40, 50});
        } });

    // ---- configured terminal: operator|(chain, pending_terminal) [ov. 4] -

    block.tests.push_back({
        "configured_terminal_sink",
        "a terminal<>() builds its consumer from the environment and drains",
        []() -> bool
        {
            ivec out;
            ::djinterp::optionator<env>{}
                | stage<src_tag>()               // 1..5
                | stage<over_tag>()              // keep > 3 -> 4,5
                | terminal<sink_tag>(&out);      // configured sink
            return (out == ivec{4, 5});
        } });

    // ---- drive short-circuit: take marks the state done ------------------

    block.tests.push_back({
        "short_circuit_take",
        "a bounding transform stops the drive before the source is exhausted",
        []() -> bool
        {
            // src emits 1..5 but take(2) marks done -> only 1,2 reach collect
            ivec v = ::djinterp::optionator<env>{}
                   | stage<src_tag>()
                   | stage<take_tag>(2)
                   | collect<int>();
            return (v == ivec{1, 2});
        } });

    // ---- re-runnability ---------------------------------------------------

    block.tests.push_back({
        "chain_is_rerunnable",
        "the same built chain can be collected repeatedly with equal results",
        []() -> bool
        {
            auto pipe = ::djinterp::optionator<env>{}
                      | stage<src_tag>()
                      | stage<scale_tag>();
            ivec a = pipe.collect<int>();
            ivec b = pipe.collect<int>();
            return (a == b) && (a == ivec{10, 20, 30, 40, 50});
        } });

    // ---- environment is the specific carried set -------------------------

    block.tests.push_back({
        "reads_the_specific_environment",
        "stages read config from THIS optionator's environment",
        []() -> bool
        {
            // a different environment (count=3, factor=100) drives a different run
            using env2 = ::djinterp::option_set<
                ::djinterp::option<cfg::count,  env_val<3>>,
                ::djinterp::option<cfg::factor, env_val<100>> >;

            ivec v = ::djinterp::optionator<env2>{}
                   | stage<src_tag>()      // count=3 -> 1,2,3
                   | stage<scale_tag>()    // factor=100 -> 100,200,300
                   | collect<int>();
            return (v == ivec{100, 200, 300});
        } });

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
