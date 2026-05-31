// djinterp [test] -- curry.hpp test module runner / entry point
#include "./curry_tests.hpp"

#include <cstdlib>


NS_DJINTERP
NS_TESTING


// curry_module_info
//   constant: per-module identity bound at the module banner site and reused
// as the single-module session descriptor below.
const test::test_module_info curry_module_info =
{
    "curry",
    "djinterp / core / functional / curry",
    "/inc/djinterp/core/functional/curry.hpp",
    "curry suite",
    "currying, uncurrying, argument transforms, constant-valued combinators, "
    "and predicate traits"
};


/*
curry_module_run_all
  Schedules every curry.hpp test section against the shared engine, in the
  same order the sections appear in curry.hpp's table of contents.
*/
void
curry_module_run_all(
    test::test_runner_ctx& _ctx
)
{
    _ctx.run("internal_machinery",      &test_internal_machinery);
    _ctx.run("predicate_traits",        &test_predicate_traits);
    _ctx.run("curry_factory",           &test_curry_factory);
    _ctx.run("curry_n_factory",         &test_curry_n_factory);
    _ctx.run("uncurry",                 &test_uncurry);
    _ctx.run("flip",                    &test_flip);
    _ctx.run("identity",                &test_identity);
    _ctx.run("always_constant",         &test_always_constant);
    _ctx.run("never",                   &test_never);

    return;
}


NS_END  // testing
NS_END  // djinterp


/*
main
  Runs the curry.hpp suite as a single-module session.
*/
int
main()
{
    return djinterp::test::run_session(
        djinterp::test::session_descriptor{
            djinterp::testing::curry_module_info.display_name,
            djinterp::testing::curry_module_info.description_short,
            djinterp::testing::curry_module_info.path,
            djinterp::testing::curry_module_info.description_short,
            djinterp::testing::curry_module_info.description_long
        },
        {
            { djinterp::testing::curry_module_info,
              &djinterp::testing::curry_module_run_all }
        });
}
