// djinterp
#include "test_common_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_TC_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_TC_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("test_common.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("test_common_tests.pdf");

    rb.module("test_common",
              "Foundational test types, status classification, and events");

    // I.   TEST TYPE IDENTIFICATION
    D_TC_RUN(tests_test_type_id);
    D_TC_RUN(tests_test_callable_id);
    D_TC_RUN(tests_k_no_callable);

    // II.  STATUS CLASSIFICATION
    D_TC_RUN(tests_test_status);

    // III. EVENT SYSTEM
    D_TC_RUN(tests_test_event_id);
    D_TC_RUN(tests_test_event);

    // IV.  CONSTEXPR SUPPORT MACROS
    D_TC_RUN(tests_d_test_constexpr);
    D_TC_RUN(tests_d_test_static_constexpr);

    return rb.finish();
}

#undef D_TC_RUN
