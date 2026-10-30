// djinterp
#include "carrier_tests_runner.hpp"    // register_carrier (shared with dtest)


// SINGLE-DOCUMENT CASE
//   The standalone per-component runner is now a thin shell: build a
// report_builder, opt into one PDF with use_pdf(), register this component's
// module, finish.  use_pdf() is the single-document knob - omit it for a
// console-only run.  Every other *_tests_runner.cpp collapses to this same
// four-line shape around its own register_<component>().
int main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("carrier.hpp unit tests");
    rb.use_pdf("carrier_tests.pdf");

    register_carrier(rb);

    return rb.finish();
}
