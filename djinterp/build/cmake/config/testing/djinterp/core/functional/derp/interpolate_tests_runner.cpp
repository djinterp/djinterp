/******************************************************************************
* djinterp [test]                                    interpolate_tests_runner.cpp
*
*   Entry point for the interpolate.hpp unit suite.  Assembles the thirteen
* section blocks - each contributed by its own translation unit through the
* block-provider declared in interpolate_tests.hpp - into a single module_spec
* and hands it to the framework runner.
*
*   Sections:
*     I.    scan-event vocabulary       (interpolate_tests_piece.cpp)
*     II.i  brace_scanner               (interpolate_tests_brace.cpp)
*     II.ii sigil_scanner               (interpolate_tests_sigil.cpp)
*     II.iii replay_scanner             (interpolate_tests_replay.cpp)
*     III.i resolution + or_else        (interpolate_tests_resolution.cpp)
*     III.  leaf resolvers              (interpolate_tests_resolvers.cpp)
*     III.  chain / when composition    (interpolate_tests_composition.cpp)
*     IV.   interp_string_sink          (interpolate_tests_sink.cpp)
*     V.    the fold engine             (interpolate_tests_engine.cpp)
*     VI.   recursive expansion         (interpolate_tests_recursive.cpp)
*     VII.  the lazy interpolation      (interpolate_tests_builder.cpp)
*     VIII. prepared_interpolation      (interpolate_tests_prepared.cpp)
*     IX.   concepts (C++20)            (interpolate_tests_concepts.cpp)
*
*   interpolate.hpp requires C++17 (std::string_view backs the piece views) and
* self-suppresses below it; its concepts are C++20-only.  Below a given tier the
* corresponding blocks are emitted empty, so this runner links and reports a
* clean run at any standard level rather than failing to build.  run_module
* returns 0 iff every executed test passed, which becomes the process exit
* status for CI.
*
*   run_module also accepts an optional trailing options argument (the shared
* module-report options surface, e.g. to emit a PDF report); it is omitted here
* so the suite depends only on the default console reporter.
*
* path:      /build/cmake/config/testing/djinterp/core/functional/interpolate/interpolate_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// djinterp -- the suite header lives in the tests tree; this relative depth
// mirrors the per-module runner location under the build/config testing tree.
#include "../../../../../../../../tests/djinterp/core/functional/interpolate_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "interpolate.hpp";
    mod.descriptor =
        "scan-event vocabulary, the brace / sigil / replay scanners, "
        "resolution and the empty / map / lookup / chain / when resolvers, the "
        "string sink, the interpolate_into fold, recursive expansion, the lazy "
        "interpolation builder, prepared templates, and the C++20 concepts";

    mod.blocks.push_back(tt::piece_block());        // I.
    mod.blocks.push_back(tt::brace_block());        // II.i
    mod.blocks.push_back(tt::sigil_block());        // II.ii
    mod.blocks.push_back(tt::replay_block());       // II.iii
    mod.blocks.push_back(tt::resolution_block());   // III.i
    mod.blocks.push_back(tt::resolvers_block());    // III.
    mod.blocks.push_back(tt::composition_block());  // III.
    mod.blocks.push_back(tt::sink_block());         // IV.
    mod.blocks.push_back(tt::engine_block());       // V.
    mod.blocks.push_back(tt::recursive_block());    // VI.
    mod.blocks.push_back(tt::builder_block());      // VII.
    mod.blocks.push_back(tt::prepared_block());     // VIII.
    mod.blocks.push_back(tt::concepts_block());     // IX.

    return dt::run_module(
        mod,
        "djinterp interpolate.hpp unit tests",
        "scanners + resolvers + sink + fold + recursion + builder + prepared + concepts");
}
