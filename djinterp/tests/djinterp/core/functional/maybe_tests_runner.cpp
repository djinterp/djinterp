// djinterp [test] -- maybe.hpp test module runner / entry point
#include "./maybe_tests.hpp"

#include <cstdlib>


NS_DJINTERP
NS_TESTING


// maybe_module_info
//   constant: per-module identity bound at the module banner site and reused
// as the single-module session descriptor below.
const test::test_module_info maybe_module_info =
{
    "maybe",
    "djinterp / functional / maybe",
    "/inc/functional/maybe.hpp",
    "maybe suite",
    "optional value type: lifetime, monadic methods, equality, predicate / "
    "structural traits, factories, pipeline combinators, monad_traits, and "
    "free-function helpers"
};


/*
maybe_module_run_all
  Schedules every maybe.hpp test section against the shared engine, in the
  same order the sections appear in maybe.hpp's table of contents.
*/
void
maybe_module_run_all(
    test::test_runner_ctx& _ctx
)
{
    // I.  maybe primitive: lifetime + value access
    _ctx.run("construction",        &test_construction);
    _ctx.run("assignment",          &test_assignment);
    _ctx.run("observers",           &test_observers);
    _ctx.run("modifiers",           &test_modifiers);
    _ctx.run("lifetime",            &test_lifetime);

    // I.  maybe primitive: monadic / fluent methods
    _ctx.run("map",                 &test_map);
    _ctx.run("and_then",            &test_and_then);
    _ctx.run("or_else",             &test_or_else);
    _ctx.run("filter",              &test_filter);
    _ctx.run("match",               &test_match);

    // I.  equality operators
    _ctx.run("equality",            &test_equality);

    // II. predicate & structural traits
    _ctx.run("traits",              &test_traits);

    // III. factories
    _ctx.run("factories",           &test_factories);

    // IV. combinator factories / pipeline
    _ctx.run("combinators",         &test_combinators);

    // V.  monad_traits specialization
    _ctx.run("monad_traits",        &test_monad_traits);

    // VI. free-function helpers
    _ctx.run("zip_with",            &test_zip_with);
    _ctx.run("flatten",             &test_flatten);
    _ctx.run("collect",             &test_collect);

    return;
}


NS_END  // testing
NS_END  // djinterp


/*
main
  Runs the maybe.hpp suite as a single-module session.
*/
int
main()
{
    return djinterp::test::run_session(
        djinterp::test::session_descriptor{
            djinterp::testing::maybe_module_info.display_name,
            djinterp::testing::maybe_module_info.description_short,
            djinterp::testing::maybe_module_info.path,
            djinterp::testing::maybe_module_info.description_short,
            djinterp::testing::maybe_module_info.description_long
        },
        {
            { djinterp::testing::maybe_module_info,
              &djinterp::testing::maybe_module_run_all }
        });
}
