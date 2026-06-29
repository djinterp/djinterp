// djinterp
#include "test_kind_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_TK_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_TK_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("test_kind.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("test_kind_tests.pdf");

    rb.module("test_kind",
              "Test-kind records, the kind set, and resolved queries");

    // I. test kind record
    D_TK_RUN(tests_test_kind_aggregate);
    D_TK_RUN(tests_test_kind_members);
    D_TK_RUN(tests_test_kind_traits);
    D_TK_RUN(tests_test_kind_values);

    // II. factory function
    D_TK_RUN(tests_make_test_kind_basic);
    D_TK_RUN(tests_make_test_kind_default_arg);
    D_TK_RUN(tests_make_test_kind_constexpr);
    D_TK_RUN(tests_make_test_kind_noexcept);

    // III. test kind set -- construction / aliases
    D_TK_RUN(tests_set_aliases);
    D_TK_RUN(tests_set_default_ctor);
    D_TK_RUN(tests_set_copy_ctor);
    D_TK_RUN(tests_set_move_ctor);
    D_TK_RUN(tests_set_underlying);
    D_TK_RUN(tests_set_ctor_noexcept);

    // III. test kind set -- forwarded surface + iteration
    D_TK_RUN(tests_set_size_empty);
    D_TK_RUN(tests_set_clear);
    D_TK_RUN(tests_set_insert);
    D_TK_RUN(tests_set_erase);
    D_TK_RUN(tests_set_find);
    D_TK_RUN(tests_set_iteration);

    // III. test kind set -- contains + dispatch
    D_TK_RUN(tests_set_contains_native);
    D_TK_RUN(tests_set_contains_fallback);
    D_TK_RUN(tests_set_contains_detection);

    // IV. resolved queries
    D_TK_RUN(tests_find_kind);
    D_TK_RUN(tests_rank_of);
    D_TK_RUN(tests_is_leaf);
    D_TK_RUN(tests_is_interior);
    D_TK_RUN(tests_name_of);
    D_TK_RUN(tests_default_options);
    D_TK_RUN(tests_can_be_child_of);
    D_TK_RUN(tests_queries_compose);

    // V. structural detection
    D_TK_RUN(tests_is_test_kind_set);
    D_TK_RUN(tests_is_test_kind_set_cvref);
    D_TK_RUN(tests_is_test_kind_set_variable);

    return rb.finish();
}

#undef D_TK_RUN
