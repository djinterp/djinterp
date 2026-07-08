// djinterp [test] -- view.hpp test module runner / entry point
#include "./view_tests.hpp"

#include <cstdlib>


NS_DJINTERP
NS_TESTING


// view_module_info
//   constant: per-module identity bound at the module banner site and reused
// as the single-module session descriptor below.
const test::test_module_info view_module_info =
{
    "view",
    "djinterp / functional / view",
    "/inc/djinterp/core/functional/view.hpp",
    "view suite",
    "lazy pull-based views: SFINAE traits & concepts, fundamental views, "
    "source views, basic and combining adapters, the pipeline operators, and "
    "the terminal operators"
};


/*
view_module_run_all
  Schedules every view.hpp test section against the shared engine, in the same
  order the sections appear in view.hpp's table of contents.
*/
void
view_module_run_all(
    test::test_runner_ctx& _ctx
)
{
    // VIII / I. traits & concepts
    _ctx.run("core_traits",             &test_core_traits);
    _ctx.run("adapter_terminal_traits", &test_adapter_terminal_traits);

    // II. fundamental views
    _ctx.run("ref_view",                &test_ref_view);
    _ctx.run("owning_view",             &test_owning_view);
    _ctx.run("iterator_pair_view",      &test_iterator_pair_view);

    // III. source views
    _ctx.run("iota",                    &test_iota);
    _ctx.run("repeat",                  &test_repeat);
    _ctx.run("generate",                &test_generate);
    _ctx.run("empty",                   &test_empty);
    _ctx.run("single",                  &test_single);

    // IV. adapter views (basic)
    _ctx.run("transform",               &test_transform);
    _ctx.run("filter",                  &test_filter);
    _ctx.run("take",                    &test_take);
    _ctx.run("drop",                    &test_drop);
    _ctx.run("take_while",              &test_take_while);
    _ctx.run("drop_while",              &test_drop_while);

    // IV. adapter views (combining)
    _ctx.run("enumerate",               &test_enumerate);
    _ctx.run("zip",                     &test_zip);
    _ctx.run("concat",                  &test_concat);
    _ctx.run("reverse",                 &test_reverse);
    _ctx.run("chunk",                   &test_chunk);
    _ctx.run("stride",                  &test_stride);

    // VI. pipeline operators
    _ctx.run("pipeline_view_adapter",   &test_pipeline_view_adapter);
    _ctx.run("pipeline_container_lift", &test_pipeline_container_lift);
    _ctx.run("pipeline_chain",          &test_pipeline_chain);

    // VII. terminal operators
    _ctx.run("to_vector",               &test_to_vector);
    _ctx.run("to_container",            &test_to_container);
    _ctx.run("count",                   &test_count);
    _ctx.run("fold",                    &test_fold);
    _ctx.run("for_each",                &test_for_each);
    _ctx.run("any_all_none",            &test_any_all_none);

    return;
}


NS_END  // testing
NS_END  // djinterp


/*
main
  Runs the view.hpp suite as a single-module session.
*/
int
main()
{
    return djinterp::test::run_session(
        djinterp::test::session_descriptor{
            djinterp::testing::view_module_info.display_name,
            djinterp::testing::view_module_info.description_short,
            djinterp::testing::view_module_info.path,
            djinterp::testing::view_module_info.description_short,
            djinterp::testing::view_module_info.description_long
        },
        {
            { djinterp::testing::view_module_info,
              &djinterp::testing::view_module_run_all }
        });
}
