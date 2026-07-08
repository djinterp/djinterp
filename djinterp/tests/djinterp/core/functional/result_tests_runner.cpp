// djinterp [test] -- result.hpp test module runner / entry point
#include "./result_tests.hpp"

#include <cstdlib>


NS_DJINTERP
NS_TESTING


// result_module_info
//   constant: per-module identity bound at the module banner site and reused
// as the single-module session descriptor below.
const test::test_module_info result_module_info =
{
    "result",
    "djinterp / functional / result",
    "/inc/djinterp/core/functional/result.hpp",
    "result suite",
    "success-or-error type: lifetime, transform methods, equality, SFINAE "
    "structural traits, factories, pipeline combinators, maybe conversions, "
    "monad_traits, and free-function helpers"
};


/*
result_module_run_all
  Schedules every result.hpp test section against the shared engine, in the
  same order the sections appear in result.hpp's table of contents.
*/
void
result_module_run_all(
    test::test_runner_ctx& _ctx
)
{
    // I.  result primitive: lifetime + value/error access
    _ctx.run("construction",        &test_construction);
    _ctx.run("assignment",          &test_assignment);
    _ctx.run("observers",           &test_observers);
    _ctx.run("lifetime",            &test_lifetime);

    // I.  result primitive: transform methods
    _ctx.run("map",                 &test_map);
    _ctx.run("map_err",             &test_map_err);
    _ctx.run("and_then",            &test_and_then);
    _ctx.run("or_else",             &test_or_else);
    _ctx.run("match",               &test_match);

    // I.  equality operators
    _ctx.run("equality",            &test_equality);

    // II. result SFINAE structural traits & concepts
    _ctx.run("traits",              &test_traits);

    // III. factories
    _ctx.run("factories",           &test_factories);

    // IV. combinator factories / pipeline
    _ctx.run("combinators",         &test_combinators);

    // I/VI. maybe conversions (ok()/err()/to_maybe)
    _ctx.run("conversions",         &test_conversions);

    // V.  monad_traits specialization
    _ctx.run("monad_traits",        &test_monad_traits);

    // VI. free-function helpers
    _ctx.run("collect",             &test_collect);
    _ctx.run("combine",             &test_combine);

    return;
}


NS_END  // testing
NS_END  // djinterp


/*
main
  Runs the result.hpp suite as a single-module session.
*/
int
main()
{
    return djinterp::test::run_session(
        djinterp::test::session_descriptor{
            djinterp::testing::result_module_info.display_name,
            djinterp::testing::result_module_info.description_short,
            djinterp::testing::result_module_info.path,
            djinterp::testing::result_module_info.description_short,
            djinterp::testing::result_module_info.description_long
        },
        {
            { djinterp::testing::result_module_info,
              &djinterp::testing::result_module_run_all }
        });
}
