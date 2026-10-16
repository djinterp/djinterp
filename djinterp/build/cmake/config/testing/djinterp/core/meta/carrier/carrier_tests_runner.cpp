// djinterp
#include "carrier_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_CR_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_CR_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("carrier.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("carrier_tests.pdf");

    rb.module("carrier",
              "Type and value carriers");

    // type carrier
    D_CR_RUN(tests_carrier_type_t_carries_type);
    D_CR_RUN(tests_carrier_type_t_recover);
    D_CR_RUN(tests_carrier_type_t_recover_cvref);
    D_CR_RUN(tests_carrier_type_t_nested);
    D_CR_RUN(tests_carrier_type_c_object);

    // detection
    D_CR_RUN(tests_carrier_is_type_carrier);
    D_CR_RUN(tests_carrier_is_type_carrier_rejects);
    D_CR_RUN(tests_carrier_is_value_carrier);
    D_CR_RUN(tests_carrier_is_value_carrier_rejects);
    D_CR_RUN(tests_carrier_is_carrier_accepts_type);
    D_CR_RUN(tests_carrier_is_carrier_accepts_value);
    D_CR_RUN(tests_carrier_is_carrier_rejects);
    D_CR_RUN(tests_carrier_v_companions);

    // value carrier
    D_CR_RUN(tests_carrier_val_t_carries_value);
    D_CR_RUN(tests_carrier_val_object);
    D_CR_RUN(tests_carrier_value_recover);
    D_CR_RUN(tests_carrier_value_recover_cvref);
    D_CR_RUN(tests_carrier_value_runtime);

    // concept faces
    D_CR_RUN(tests_carrier_concept_type_carrier);
    D_CR_RUN(tests_carrier_concept_value_carrier);
    D_CR_RUN(tests_carrier_concept_carrier);
    D_CR_RUN(tests_carrier_concept_constrains);

    return rb.finish();
}

#undef D_CR_RUN
