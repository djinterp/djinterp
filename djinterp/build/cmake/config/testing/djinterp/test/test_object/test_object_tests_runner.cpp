// djinterp
#include "../../../../../../../tests/djinterp/test/test_object_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_TO_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_TO_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("test_object.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("test_object_tests.pdf");

    rb.module("test_object",
              "Unified test object: type identity, result, status, and metadata");

    // test_metadata
    D_TO_RUN(tests_test_metadata_types);
    D_TO_RUN(tests_test_metadata_set);
    D_TO_RUN(tests_test_metadata_get);
    D_TO_RUN(tests_test_metadata_contains);

    // aliases / status constants / storage / traits
    D_TO_RUN(tests_object_aliases);
    D_TO_RUN(tests_object_status_constants);
    D_TO_RUN(tests_object_storage);
    D_TO_RUN(tests_object_traits);

    // construction
    D_TO_RUN(tests_object_ctor_default);
    D_TO_RUN(tests_object_ctor_type_id);
    D_TO_RUN(tests_object_ctor_type_result);
    D_TO_RUN(tests_object_ctor_metadata);
    D_TO_RUN(tests_object_ctor_noexcept);

    // read-only query surface
    D_TO_RUN(tests_object_bool_conversion);
    D_TO_RUN(tests_object_status_result);
    D_TO_RUN(tests_object_passed_failed);
    D_TO_RUN(tests_object_type_id);
    D_TO_RUN(tests_object_callable_query);

    // mutation
    D_TO_RUN(tests_object_evaluate);
    D_TO_RUN(tests_object_skip);
    D_TO_RUN(tests_object_set_status);
    D_TO_RUN(tests_object_set_type_id);
    D_TO_RUN(tests_object_set_callable_id);
    D_TO_RUN(tests_object_mutation_sequence);

    // metadata access
    D_TO_RUN(tests_object_metadata_accessor);
    D_TO_RUN(tests_object_set_metadata_copy);
    D_TO_RUN(tests_object_set_metadata_move);
    D_TO_RUN(tests_object_metadata_noexcept);

    // convenience aliases
    D_TO_RUN(tests_basic_test);
    D_TO_RUN(tests_tagged_test);

    // factory functions
    D_TO_RUN(tests_make_test);
    D_TO_RUN(tests_make_interior);
    D_TO_RUN(tests_make_interior_named);

    return rb.finish();
}

#undef D_TO_RUN
