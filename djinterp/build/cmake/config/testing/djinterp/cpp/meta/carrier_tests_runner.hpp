#ifndef DJINTERP_TESTS_CARRIER_RUNNER_
#define DJINTERP_TESTS_CARRIER_RUNNER_ 1

// djinterp
#include "carrier_tests.hpp"                        // ::djinterp::testing::tests_carrier_*
#include "djinterp/test/test_report_runner.hpp"     // report_builder


// register_carrier
//   opens the "carrier" module on _rb and runs every carrier unit test.  This
// is the ONE place the carrier test list lives: carrier_tests_runner.cpp drives
// it into a single PDF, dtest_tests_runner.cpp drives it (with every other
// component) into a shared 7z.  Each other component has the identical shape -
// a register_<component>(report_builder&) that is exactly the body its
// standalone *_tests_runner.cpp used to carry inline.
inline
void register_carrier(
    ::djinterp::test::report_builder& rb
)
{
    rb.module("carrier", "Type and value carriers");

    #define D_CR_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)

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

    #undef D_CR_RUN

    return;
}


#endif  // DJINTERP_TESTS_CARRIER_RUNNER_
