/******************************************************************************
* djinterp [test]                                             test_context.hpp
*
*   The focus type for the DTest document layer: a "fat" nullable context
* whose four parts (run / module / unit / check) name the four depths of a
* test report.  This is the _Ctx that binding_env<test_context> and
* section<test_context> are instantiated against; every projection in the
* baseline env reads the deepest part it needs and falls back to the empty
* fragment if that part is null - matching binding_env's lenient missing-key
* rule and text_template's lenient missing-source rule.
*
*   SCOPE BY FOCUS (NO SCOPE MECHANISM):
*   Keys live in one flat namespace.  "Scope" - a run-level title vs a
* module-level module_name - is simply which part of the focus the projection
* reads.  Iteration (per_module / per_unit_in_module / per_check_in_unit,
* defined in test_document.hpp) is a skeleton concern: the env never changes,
* only the focus does.  A projection evaluated against the wrong focus reads
* a null part and yields empty, so a check-level skeleton handed a module-
* level focus draws blank check fields rather than crashing.
*
*   INDICES (1-BASED FOR DISPLAY):
*   The three indices ride alongside the pointers so that "Module 3 of 5",
* "Check 1", etc. read straight off the ctx.  test_document's standard
* sections populate them when refocusing; outside iteration they are zero.
* They are 1-based at the point of display (a 1-based "module N" reads more
* naturally than a 0-based one); refocus closures install (i + 1) when
* refocusing for iteration i.
*
*   DTEST-AGNOSTIC ABOVE THIS LINE:
*   binding_env<test_context> and section<test_context> are still generic at
* the core layer - they see _Ctx as opaque.  The DTest-specific shape lives
* HERE; nothing in binding_env / section / template / document mentions a
* test_report.
*
*   PORTABILITY:
*   C++17 (the test_report this builds against is C++11, but the document
* stack from binding_env on up is C++17).  Self-suppresses below C++17,
* parallel to binding_env.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEST CONTEXT               (the focus struct)
* II.   FOCUS BUILDERS             (at_run / at_module / at_unit / at_check)
*
*
* path:      /inc/djinterp/test/test_context.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.27
******************************************************************************/

#ifndef DJINTERP_TEST_TEST_CONTEXT_
#define DJINTERP_TEST_TEST_CONTEXT_ 1

// std
#include <cstddef>
// djinterp
#include "../core/djinterp.hpp"   // NS_*, D_NODISCARD, language gates
#include "./test_report.hpp"      // test_report, report_module, report_unit,
                                  //   report_check (the focus pointees)


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   TEST CONTEXT                                         ///
///////////////////////////////////////////////////////////////////////////////

// test_context
//   struct: the focus the DTest document layer reads from.  Four nullable
// pointers (run / module_ / unit / check) name the four depths of a test
// report; three indices (module_index / unit_index / check_index) carry
// iteration position when the focus is being walked by a section.  Each part
// is optional - a projection that needs a deeper part than the focus
// currently carries reads null and yields the empty fragment, matching the
// rest of the document stack's lenient missing-source rule.
//
//   The trailing underscore on `module_` is a guard against the C++20 module
// keyword, which is context-sensitive but interacts poorly with some
// tooling; the field name everywhere else in the test layer follows suit.
struct test_context
{
    const test_report*   run          = nullptr;
    const report_module* module_      = nullptr;
    const report_unit*   unit         = nullptr;
    const report_check*  check        = nullptr;

    std::size_t          module_index = 0;
    std::size_t          unit_index   = 0;
    std::size_t          check_index  = 0;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  FOCUS BUILDERS                                       ///
///////////////////////////////////////////////////////////////////////////////
//   Convenience constructors that name common focus shapes.  The standard
// sections in test_document re-focus by hand (so they can install indices),
// but a caller who wants to address one specific node without iterating uses
// one of these.

// at_run
//   function: a focus carrying only a test_report.  The "outermost" focus,
// from which per_module iteration refocuses to each module.
D_NODISCARD inline test_context
at_run(
    const test_report* _run
)
{
    test_context _c;
    _c.run = _run;

    return _c;
}


// at_module
//   function: a focus carrying run + one module + the module's 1-based
// index.  The per_module section in test_document yields a refocused ctx
// of this shape per iteration.
D_NODISCARD inline test_context
at_module(
    const test_report*   _run,
    const report_module* _module,
    std::size_t          _index = 0
)
{
    test_context _c;
    _c.run          = _run;
    _c.module_      = _module;
    _c.module_index = _index;

    return _c;
}


// at_unit
//   function: a focus carrying run + module + one unit + the unit's
// 1-based index (plus the parent module's index, when known).
D_NODISCARD inline test_context
at_unit(
    const test_report*   _run,
    const report_module* _module,
    const report_unit*   _unit,
    std::size_t          _module_index = 0,
    std::size_t          _unit_index   = 0
)
{
    test_context _c;
    _c.run          = _run;
    _c.module_      = _module;
    _c.unit         = _unit;
    _c.module_index = _module_index;
    _c.unit_index   = _unit_index;

    return _c;
}


// at_check
//   function: a focus carrying every part populated, for addressing one
// specific assertion.
D_NODISCARD inline test_context
at_check(
    const test_report*   _run,
    const report_module* _module,
    const report_unit*   _unit,
    const report_check*  _check,
    std::size_t          _module_index = 0,
    std::size_t          _unit_index   = 0,
    std::size_t          _check_index  = 0
)
{
    test_context _c;
    _c.run          = _run;
    _c.module_      = _module;
    _c.unit         = _unit;
    _c.check        = _check;
    _c.module_index = _module_index;
    _c.unit_index   = _unit_index;
    _c.check_index  = _check_index;

    return _c;
}


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEST_TEST_CONTEXT_
