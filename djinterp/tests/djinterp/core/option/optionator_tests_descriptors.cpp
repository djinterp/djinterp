/*******************************************************************************
* djinterp [test]                               optionator_tests_descriptors.cpp
*
*   Sections II + III - the unconfigured descriptor + factory layer.  stage()
* builds a pending_stage carrying a tag and decayed call-site args; into() /
* collect() name concrete sink terminals; terminal() builds a pending_terminal
* whose configures<> adapter reads the environment.  These tests pin the
* factory return types, the decayed argument capture, the tag_type surface, and
* the build() step that turns a descriptor into its materialized stage/consumer
* by invoking configures<_Tag>::apply against a carried option_set.  C++17-gated.
*
*
* path:      /tests/djinterp/core/option/optionator_tests_descriptors.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./optionator_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// probe_sink
//   a named consumer, so into()'s wrapped-sink type is nameable for identity.
struct probe_sink
{
    void operator()(const int&) const
    {}
};

// drain
//   pull a (copied) producer to exhaustion into a vector, for asserting what a
// built source emits.
inline std::vector<int>
drain(test_producer _p)
{
    std::vector<int> out;
    while (true)
    {
        auto step = _p();
        if (!step.has_value)
        {
            break;
        }
        out.push_back(step.value);
    }
    return out;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


::djinterp::test::block_spec
optionator_descriptors_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "II+III.  descriptors + factories";
    block.descriptor = "stage / into / collect / terminal return types + build()";

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    namespace osi = ::djinterp::internal;

    // ---- stage() / pending_stage -----------------------------------------

    block.tests.push_back({
        "stage_return_type",
        "stage<tag>() yields a pending_stage for that tag",
        []() -> bool
        {
            constexpr bool ok = same_v<
                decltype(stage<src_tag>()),
                osi::pending_stage<src_tag> >;
            static_assert(ok, "stage<src_tag>() type");
            return ok;
        } });

    block.tests.push_back({
        "stage_decays_args",
        "call-site args are captured by decayed value type",
        []() -> bool
        {
            int        i = 1;
            const char c = 'c';
            // lvalues of int and const char -> int, char
            constexpr bool ok = same_v<
                decltype(stage<unconfigured_tag>(i, c)),
                osi::pending_stage<unconfigured_tag, int, char> >;
            static_assert(ok, "stage decays its args");
            (void)i;
            (void)c;
            return ok;
        } });

    block.tests.push_back({
        "stage_tag_type",
        "pending_stage exposes its tag as tag_type",
        []() -> bool
        {
            constexpr bool ok = same_v<
                decltype(stage<src_tag>(1, 2.0))::tag_type, src_tag >;
            static_assert(ok, "pending_stage::tag_type");
            return ok;
        } });

    block.tests.push_back({
        "stage_build_reads_environment",
        "build<env>() configures the stage against the carried option_set",
        []() -> bool
        {
            // src adapter reads count=5 from env -> a producer emitting 1..5
            auto produced = stage<src_tag>().build<env>();
            return ( ::djinterp::is_producer<decltype(produced)>::value &&
                     drain(produced) == std::vector<int>{1, 2, 3, 4, 5} );
        } });

    block.tests.push_back({
        "stage_build_forwards_runtime_arg",
        "build() forwards the captured runtime args into apply()",
        []() -> bool
        {
            // count adapter reads a runtime 3 -> a producer emitting 1..3
            auto produced = stage<count_tag>(3).build<env>();
            return ( ::djinterp::is_producer<decltype(produced)>::value &&
                     drain(produced) == std::vector<int>{1, 2, 3} );
        } });

    // ---- into() / into_terminal ------------------------------------------

    block.tests.push_back({
        "into_return_type",
        "into(sink) wraps the decayed sink in an into_terminal",
        []() -> bool
        {
            constexpr bool ok = same_v<
                decltype(into(probe_sink{})),
                osi::into_terminal<probe_sink> >;
            static_assert(ok, "into() type");
            return ok;
        } });

    block.tests.push_back({
        "into_holds_sink",
        "the terminal holds the sink it was given",
        []() -> bool
        {
            probe_sink s;
            auto t = into(s);
            // sink() returns a reference of the stored sink type
            return same_v< std::decay_t<decltype(t.sink())>, probe_sink >;
        } });

    // ---- collect() / collect_terminal ------------------------------------

    block.tests.push_back({
        "collect_return_type",
        "collect<Out>() yields a collect_terminal<Out>",
        []() -> bool
        {
            constexpr bool ok = same_v<
                decltype(collect<int>()),
                osi::collect_terminal<int> >;
            static_assert(ok, "collect() type");
            return ok;
        } });

    block.tests.push_back({
        "collect_value_type",
        "collect_terminal carries its element type as value_type",
        []() -> bool
        {
            constexpr bool ok =
                same_v< decltype(collect<int>())::value_type, int > &&
                same_v< decltype(collect<double>())::value_type, double >;
            static_assert(ok, "collect_terminal::value_type");
            return ok;
        } });

    // ---- terminal() / pending_terminal -----------------------------------

    block.tests.push_back({
        "terminal_return_type",
        "terminal<tag>(args) yields a pending_terminal with decayed args",
        []() -> bool
        {
            constexpr bool ok = same_v<
                decltype(terminal<sink_tag>(static_cast<std::vector<int>*>(nullptr))),
                osi::pending_terminal<sink_tag, std::vector<int>*> >;
            static_assert(ok, "terminal() type");
            return ok;
        } });

    block.tests.push_back({
        "terminal_tag_type",
        "pending_terminal exposes its tag as tag_type",
        []() -> bool
        {
            constexpr bool ok = same_v<
                decltype(terminal<sink_tag>(
                    static_cast<std::vector<int>*>(nullptr)))::tag_type,
                sink_tag >;
            static_assert(ok, "pending_terminal::tag_type");
            return ok;
        } });

    block.tests.push_back({
        "terminal_build_makes_consumer",
        "build<env>() forwards the runtime arg and returns the configured consumer",
        []() -> bool
        {
            std::vector<int> out;
            auto sink = terminal<sink_tag>(&out).build<env>();
            sink(42);
            sink(7);
            return (out == std::vector<int>{42, 7});
        } });

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
