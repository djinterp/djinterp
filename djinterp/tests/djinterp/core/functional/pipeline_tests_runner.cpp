// djinterp [test] -- pipeline.hpp test module runner / entry point
#include "./pipeline_tests.hpp"

#include <cstdlib>


NS_DJINTERP
NS_TESTING


// pipeline_module_info
//   constant: per-module identity bound at the module banner site and reused
// as the single-module session descriptor below.
const test::test_module_info pipeline_module_info =
{
    "pipeline",
    "djinterp / functional / pipeline",
    "/inc/djinterp/core/functional/pipeline.hpp",
    "pipeline suite",
    "typed function_pipeline: creation, transforming / slicing / aggregating "
    "/ query operations, accessors and iteration, error propagation, the "
    "SFINAE traits, and the convenience factory"
};


/*
pipeline_module_run_all
  Schedules every pipeline.hpp test section against the shared engine, in the
  order the sections appear in the header.
*/
void
pipeline_module_run_all(
    test::test_runner_ctx& _ctx
)
{
    // i. creation
    _ctx.run("creation",            &test_creation);

    // ii. transforming operations
    _ctx.run("map",                 &test_map);
    _ctx.run("filter",              &test_filter);
    _ctx.run("distinct",            &test_distinct);
    _ctx.run("flat_map",            &test_flat_map);

    // ii. slicing / reordering
    _ctx.run("take",                &test_take);
    _ctx.run("skip",                &test_skip);
    _ctx.run("slice",               &test_slice);
    _ctx.run("reorder",             &test_reorder);

    // ii / iii. aggregating operations
    _ctx.run("fold",                &test_fold);
    _ctx.run("reduce",              &test_reduce);
    _ctx.run("group_by",            &test_group_by);
    _ctx.run("partition",           &test_partition);
    _ctx.run("zip_with",            &test_zip_with);

    // iii. query operations
    _ctx.run("query",               &test_query);
    _ctx.run("for_each",            &test_for_each);

    // iv. accessors / iteration
    _ctx.run("accessors",           &test_accessors);
    _ctx.run("iteration",           &test_iteration);

    // error propagation
    _ctx.run("error_propagation",   &test_error_propagation);

    // III. SFINAE traits & concepts
    _ctx.run("traits",              &test_traits);

    // II. convenience factory
    _ctx.run("factory",             &test_factory);

    return;
}


NS_END  // testing
NS_END  // djinterp


/*
main
  Runs the pipeline.hpp suite as a single-module session.
*/
int
main()
{
    return djinterp::test::run_session(
        djinterp::test::session_descriptor{
            djinterp::testing::pipeline_module_info.display_name,
            djinterp::testing::pipeline_module_info.description_short,
            djinterp::testing::pipeline_module_info.path,
            djinterp::testing::pipeline_module_info.description_short,
            djinterp::testing::pipeline_module_info.description_long
        },
        {
            { djinterp::testing::pipeline_module_info,
              &djinterp::testing::pipeline_module_run_all }
        });
}
