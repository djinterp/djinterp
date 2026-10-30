/******************************************************************************
* djinterp [test]                              djinterp_header_tests_runner.cpp
*
*   Entry point for the DTest djinterp.hpp (framework headers) test suite.
* Registers itself with the global test_module_registry on static-init; runs
* every per-section group of the suite in order when the registered function
* is invoked (whether by single_module_main.cpp in the standalone exe or by
* aggregate_main.cpp in djinterp_tests.exe).
*
*   ARCHITECTURE NOTES:
*   The runner takes a test_handler reference (via test_runner_ctx).  All
* printer/handler setup, per-section timing / verdict tracking, master-
* template binding, and exit-code logic now live inside
* djinterp::test::test_runner_ctx -- this file just lists the sections and
* adapts the legacy bool()-returning test groups into the
* void(test_handler&) signature run() expects.
*
*   LEGACY ADAPTER LAYER:
*   The djinterp.hpp suite predates the void(test_handler&) section
* protocol used by dtuple / type_traits / array.  Each section is a
* parameterless `bool tests_*()` that performs all of its checks
* internally -- static_assert at translation time, plus an `ok = ok && ...`
* accumulator that mirrors the same checks at runtime, returning a single
* bool to summarize.
*   Bridging these into the new framework is a one-call wrapper: invoke
* the legacy bool group, then report the outcome through the test_handler
* as a single assertion.  The dj_record_section_result helper centralizes
* that call so if the project's record_assertion spelling differs from
* the one assumed below, there is exactly one line to update.
*
*   MULTI-MODULE PROTOCOL:
*   Thirteen sections are run, in the order they appear in
* djinterp_header_tests.hpp's TABLE OF CONTENTS:
*
*     CHAPTER I.   C++ KEYWORDS & NAMESPACE MACROS
*       1.  keyword_macros       - D_KEYWORD_* stringization
*       2.  namespace_macros     - NS_* open / NS_END close
*
*     CHAPTER II.  CONSTEXPR / NOEXCEPT SUPPORT
*       3.  constexpr_macros     - D_CONSTEXPR family, D_TESTING_CONSTEXPR
*       4.  noexcept_macro       - D_NOEXCEPT
*
*     CHAPTER III. CORE TYPE UTILITIES
*       5.  void_t               - ::void_t SFINAE sink
*       6.  abs_value            - integral-value abs trait
*       7.  abs_value_v          - variable-template form (C++14+)
*       8.  abs_value_to_size_t  - magnitude widened to std::size_t (C++14+)
*       9.  clean                - cv/ref stripper (clean / clean_t)
*       10. constexpr_swap       - move-only-safe swap
*       11. repeat               - tuple-of-N builder (repeat / repeat_t)
*       12. self_and_is_self     - the self marker + is_self detection
*       13. resolve_self         - self-rewriting specializations
*
*   To run only a subset of sections, comment out the corresponding
* _ctx.run(...) line below; everything else (totals, exit code) keys off
* the test_runner_ctx accumulators so it adapts.
*
*   VERDICT POLICY:
*   Each section maps to one recorded assertion, so DSessionResult.total
* equals the number of sections actually run and DSessionResult.failed
* equals the number that returned false.  test_runner_ctx::render_and_exit
* returns EXIT_FAILURE iff any section failed; static_assert violations
* would have already broken the build at translation time and are never
* observed at runtime.
*
*   PORTABILITY:
*   C++11 minimum.  The legacy tests already gate their C++14+ pieces
* (variable templates, relaxed constexpr) on D_ENV_CPP_FEATURE_* macros
* internally; this runner does not gate anything itself.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/djinterp_header_tests_runner.cpp
*                djinterp_header_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

// djinterp
#include <djinterp/core/djinterp.hpp>
#include <djinterp/cpp/modules/test/test_defaults.hpp>
#include <djinterp/cpp/modules/test/test_handler.hpp>
#include <djinterp/cpp/modules/test/test_options.hpp>
#include "../../../../../../tests/djinterp/core/djinterp_header_tests.hpp"


using djinterp::test::test_handler;
using djinterp::test::test_module_info;
using djinterp::test::test_runner_ctx;


// ---------------------------------------------------------------------------
//  legacy-bridge layer
//   Translates `bool tests_*()` into the void(test_handler&) signature the
// test_runner_ctx expects.  One assertion is recorded per section: the
// section's overall pass/fail.  Per-check granularity is already enforced
// at compile time by the static_asserts inside each section, so the
// runtime side does not need to subdivide further.
//
//   CONFIRM-THE-API NOTE:
//   The body of record_section_result_ assumes test_handler exposes
//
//     void record_assertion(bool _ok, const char* _name);
//
// which matches the comment "They record their assertions directly into
// the handler via record_assertion()" from the dtuple runner.  If the
// actual signature is different (e.g. takes file/line, or is named
// `record`/`note_assertion`, or is reached through a macro like
// D_TEST_CHECK), this is the one place to update.  Every adapter below
// routes through this single call.
// ---------------------------------------------------------------------------
namespace {

    inline
    void record_section_result_(
        test_handler& _h,
        const char*   _name,
        bool          _ok
    )
    {
        _h.record_assertion(_ok, _name);
    }


    // DJINTERP_HEADER_SECTION_ADAPTER
    //   macro: emits a static void adapt_<_section>(test_handler&) function
    // that invokes tests_<_section>() (from djinterp_header_tests.hpp) and
    // reports the outcome through record_section_result_ above.  Using
    // a macro keeps the adapter list legible -- 13 four-line functions
    // collapse to 13 single-line macro invocations.
    //
    //   The legacy tests live in namespace djinterp::test, so we resolve
    // their unqualified names by routing through a using-directive inside
    // each adapter body (rather than dragging djinterp::test into the
    // anonymous namespace at file scope, which would shadow names from
    // the framework).
    #define DJINTERP_HEADER_SECTION_ADAPTER(_section)                       \
        static void                                                         \
        adapt_##_section(test_handler& _h)                                  \
        {                                                                   \
            using namespace djinterp::test;                                 \
            record_section_result_(_h, #_section, tests_##_section());  \
        }                                                                   \
        /**/

    // -- chapter I: keywords & namespaces ------------------------------------
    DJINTERP_HEADER_SECTION_ADAPTER(keyword_macros)
    DJINTERP_HEADER_SECTION_ADAPTER(namespace_macros)

    // -- chapter II: constexpr / noexcept ------------------------------------
    DJINTERP_HEADER_SECTION_ADAPTER(constexpr_macros)
    DJINTERP_HEADER_SECTION_ADAPTER(noexcept_macro)

    // -- chapter III: core type utilities ------------------------------------
    DJINTERP_HEADER_SECTION_ADAPTER(void_t)
    DJINTERP_HEADER_SECTION_ADAPTER(abs_value)
    DJINTERP_HEADER_SECTION_ADAPTER(abs_value_v)
    DJINTERP_HEADER_SECTION_ADAPTER(abs_value_to_size_t)
    DJINTERP_HEADER_SECTION_ADAPTER(clean)
    DJINTERP_HEADER_SECTION_ADAPTER(constexpr_swap)
    DJINTERP_HEADER_SECTION_ADAPTER(repeat)
    DJINTERP_HEADER_SECTION_ADAPTER(self_and_is_self)
    DJINTERP_HEADER_SECTION_ADAPTER(resolve_self)

    #undef DJINTERP_HEADER_SECTION_ADAPTER

}  // anonymous namespace


// ---------------------------------------------------------------------------
//  module identity + run function + registration
// ---------------------------------------------------------------------------
namespace {

    // module identity bound into the master template when this module is
    // run as a standalone exe.  When run as part of the aggregate, the
    // aggregate's identity replaces this; the per-section banners still
    // come from run() calls below.
    const test_module_info djinterp_header_info =
    {
        /* short_name        */ "djinterp_header",
        /* display_name      */ "djinterp / core / djinterp.hpp",
        /* path              */ "/inc/djinterp/core/djinterp.hpp",
        /* description_short */ "djinterp.hpp suite",
        /* description_long  */
            "Tests for the djinterp framework header (djinterp.hpp): "
            "C++ keyword and namespace macros, constexpr / noexcept "
            "qualifier macros, and core type utilities (void_t, abs_value "
            "and friends, clean / clean_t, constexpr_swap, repeat / "
            "repeat_t, and the self / is_self / resolve_self family).  "
            "Each section is a legacy bool-returning group adapted into "
            "the framework's section protocol via a single recorded "
            "assertion per section."
    };


    // djinterp_header_run_all
    //   Section list - order matches djinterp_header_tests.hpp's TABLE OF
    // CONTENTS.  Comment a line to skip a section; everything else
    // (totals, exit code) keys off the accumulator inside the ctx so it
    // adapts.
    void djinterp_header_run_all(
        test_runner_ctx& _ctx
    )
    {
        // -- chapter I: keywords & namespaces ---------------------------
        _ctx.run("keyword_macros",      &adapt_keyword_macros);
        _ctx.run("namespace_macros",    &adapt_namespace_macros);

        // -- chapter II: constexpr / noexcept ---------------------------
        _ctx.run("constexpr_macros",    &adapt_constexpr_macros);
        _ctx.run("noexcept_macro",      &adapt_noexcept_macro);

        // -- chapter III: core type utilities ---------------------------
        _ctx.run("void_t",              &adapt_void_t);
        _ctx.run("abs_value",           &adapt_abs_value);
        _ctx.run("abs_value_v",         &adapt_abs_value_v);
        _ctx.run("abs_value_to_size_t", &adapt_abs_value_to_size_t);
        _ctx.run("clean",               &adapt_clean);
        _ctx.run("constexpr_swap",      &adapt_constexpr_swap);
        _ctx.run("repeat",              &adapt_repeat);
        _ctx.run("self_and_is_self",    &adapt_self_and_is_self);
        _ctx.run("resolve_self",        &adapt_resolve_self);
    }

}  // anonymous namespace


// registration: pulls this TU into the global registry on static-init.
// Visible from both single_module_main (standalone exe) and aggregate_main
// (djinterp_tests.exe) without either of them needing to know about this
// module specifically.
DJINTERP_REGISTER_TEST_MODULE(djinterp_header_info, &djinterp_header_run_all);
