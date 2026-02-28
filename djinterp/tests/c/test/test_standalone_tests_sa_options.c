#include "./test_standalone_tests_sa.h"


/******************************************************************************
 * XIII. CLI OPTIONS TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_options_struct
  Tests the d_test_sa_options structure.
  Tests the following:
  - Structure has expected members
  - All boolean members are accessible
  - output_file member is accessible
*/
bool
d_tests_sa_standalone_options_struct
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_test_sa_options   opts;

    result = true;

    // test 1: number_assertions member is accessible
    opts.number_assertions = true;

    result = d_assert_standalone(
        opts.number_assertions == true,
        "options_struct_num_assertions",
        "number_assertions member should be accessible",
        _counter) && result;

    // test 2: number_tests member is accessible
    opts.number_tests = true;

    result = d_assert_standalone(
        opts.number_tests == true,
        "options_struct_num_tests",
        "number_tests member should be accessible",
        _counter) && result;

    // test 3: global_numbering member is accessible
    opts.global_numbering = true;

    result = d_assert_standalone(
        opts.global_numbering == true,
        "options_struct_global_numbering",
        "global_numbering member should be accessible",
        _counter) && result;

    // test 4: show_info member is accessible
    opts.show_info = false;

    result = d_assert_standalone(
        opts.show_info == false,
        "options_struct_show_info",
        "show_info member should be accessible",
        _counter) && result;

    // test 5: show_module_footer member is accessible
    opts.show_module_footer = false;

    result = d_assert_standalone(
        opts.show_module_footer == false,
        "options_struct_show_footer",
        "show_module_footer member should be accessible",
        _counter) && result;

    // test 6: list_failures member is accessible
    opts.list_failures = true;

    result = d_assert_standalone(
        opts.list_failures == true,
        "options_struct_list_failures",
        "list_failures member should be accessible",
        _counter) && result;

    // test 7: output_file member is accessible
    opts.output_file = "/tmp/test.txt";

    result = d_assert_standalone(
        opts.output_file != NULL,
        "options_struct_output_file",
        "output_file member should be accessible",
        _counter) && result;

    // test 8: output_file can be NULL
    opts.output_file = NULL;

    result = d_assert_standalone(
        opts.output_file == NULL,
        "options_struct_output_file_null",
        "output_file should be assignable to NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_options_init
  Tests the d_test_sa_options_init function.
  Tests the following:
  - NULL options is handled safely
  - All numbering flags default to false
  - show_info defaults to true
  - show_module_footer defaults to true
  - list_failures defaults to false
  - output_file defaults to NULL
*/
bool
d_tests_sa_standalone_options_init
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_test_sa_options opts;

    result = true;

    // test 1: NULL options is handled safely
    d_test_sa_options_init(NULL);

    result = d_assert_standalone(
        true,
        "options_init_null_safe",
        "d_test_sa_options_init(NULL) should not crash",
        _counter) && result;

    // test 2: initialize and verify defaults
    d_test_sa_options_init(&opts);

    result = d_assert_standalone(
        opts.number_assertions == false,
        "options_init_num_assertions_false",
        "number_assertions should default to false",
        _counter) && result;

    result = d_assert_standalone(
        opts.number_tests == false,
        "options_init_num_tests_false",
        "number_tests should default to false",
        _counter) && result;

    result = d_assert_standalone(
        opts.global_numbering == false,
        "options_init_global_num_false",
        "global_numbering should default to false",
        _counter) && result;

    result = d_assert_standalone(
        opts.show_info == true,
        "options_init_show_info_true",
        "show_info should default to true",
        _counter) && result;

    result = d_assert_standalone(
        opts.show_module_footer == true,
        "options_init_show_footer_true",
        "show_module_footer should default to true",
        _counter) && result;

    result = d_assert_standalone(
        opts.list_failures == false,
        "options_init_list_failures_false",
        "list_failures should default to false",
        _counter) && result;

    result = d_assert_standalone(
        opts.output_file == NULL,
        "options_init_output_file_null",
        "output_file should default to NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_options_parse
  Tests the d_test_sa_options_parse function.
  Tests the following:
  - NULL options is handled safely
  - NULL argv is handled safely
  - -na flag sets number_assertions
  - -nt flag sets number_tests
  - -gn flag sets global_numbering
  - -ni flag clears show_info
  - -nf flag clears show_module_footer
  - -lf flag sets list_failures
  - -o flag sets output_file
  - Unknown flag is ignored (returns true)
  - No arguments returns true
*/
bool
d_tests_sa_standalone_options_parse
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    bool                     parse_result;
    struct d_test_sa_options opts;

    result = true;

    // test 1: NULL options is handled safely
    parse_result = d_test_sa_options_parse(NULL, 0, NULL);

    result = d_assert_standalone(
        parse_result == true,
        "options_parse_null_opts",
        "NULL options should return true",
        _counter) && result;

    // test 2: NULL argv is handled safely
    d_test_sa_options_init(&opts);
    parse_result = d_test_sa_options_parse(&opts, 2, NULL);

    result = d_assert_standalone(
        parse_result == true,
        "options_parse_null_argv",
        "NULL argv should return true",
        _counter) && result;

    // test 3: -na flag sets number_assertions
    {
        char* argv_na[] = { "prog", "-na" };

        d_test_sa_options_init(&opts);
        d_test_sa_options_parse(&opts, 2, argv_na);

        result = d_assert_standalone(
            opts.number_assertions == true,
            "options_parse_na_flag",
            "-na should set number_assertions to true",
            _counter) && result;
    }

    // test 4: -nt flag sets number_tests
    {
        char* argv_nt[] = { "prog", "-nt" };

        d_test_sa_options_init(&opts);
        d_test_sa_options_parse(&opts, 2, argv_nt);

        result = d_assert_standalone(
            opts.number_tests == true,
            "options_parse_nt_flag",
            "-nt should set number_tests to true",
            _counter) && result;
    }

    // test 5: -gn flag sets global_numbering
    {
        char* argv_gn[] = { "prog", "-gn" };

        d_test_sa_options_init(&opts);
        d_test_sa_options_parse(&opts, 2, argv_gn);

        result = d_assert_standalone(
            opts.global_numbering == true,
            "options_parse_gn_flag",
            "-gn should set global_numbering to true",
            _counter) && result;
    }

    // test 6: -ni flag clears show_info
    {
        char* argv_ni[] = { "prog", "-ni" };

        d_test_sa_options_init(&opts);
        d_test_sa_options_parse(&opts, 2, argv_ni);

        result = d_assert_standalone(
            opts.show_info == false,
            "options_parse_ni_flag",
            "-ni should set show_info to false",
            _counter) && result;
    }

    // test 7: -nf flag clears show_module_footer
    {
        char* argv_nf[] = { "prog", "-nf" };

        d_test_sa_options_init(&opts);
        d_test_sa_options_parse(&opts, 2, argv_nf);

        result = d_assert_standalone(
            opts.show_module_footer == false,
            "options_parse_nf_flag",
            "-nf should set show_module_footer to false",
            _counter) && result;
    }

    // test 8: -lf flag sets list_failures
    {
        char* argv_lf[] = { "prog", "-lf" };

        d_test_sa_options_init(&opts);
        d_test_sa_options_parse(&opts, 2, argv_lf);

        result = d_assert_standalone(
            opts.list_failures == true,
            "options_parse_lf_flag",
            "-lf should set list_failures to true",
            _counter) && result;
    }

    // test 9: -o flag sets output_file
    {
        char* argv_o[] = { "prog", "-o", "/tmp/out.txt" };

        d_test_sa_options_init(&opts);
        d_test_sa_options_parse(&opts, 3, argv_o);

        result = d_assert_standalone(
            opts.output_file != NULL,
            "options_parse_o_flag",
            "-o should set output_file",
            _counter) && result;

        result = d_assert_standalone(
            strcmp(opts.output_file, "/tmp/out.txt") == 0,
            "options_parse_o_flag_value",
            "-o should set output_file to specified path",
            _counter) && result;
    }

    // test 10: -o without argument returns false
    {
        char* argv_o_no_arg[] = { "prog", "-o" };

        d_test_sa_options_init(&opts);
        parse_result = d_test_sa_options_parse(&opts, 2,
                                               argv_o_no_arg);

        result = d_assert_standalone(
            parse_result == false,
            "options_parse_o_no_arg",
            "-o without argument should return false",
            _counter) && result;
    }

    // test 11: multiple flags work together
    {
        char* argv_multi[] = { "prog", "-na", "-nt", "-lf" };

        d_test_sa_options_init(&opts);
        d_test_sa_options_parse(&opts, 4, argv_multi);

        result = d_assert_standalone(
            (opts.number_assertions == true) &&
            (opts.number_tests == true)      &&
            (opts.list_failures == true),
            "options_parse_multiple_flags",
            "Multiple flags should all be set",
            _counter) && result;
    }

    // test 12: no arguments returns true
    {
        char* argv_empty[] = { "prog" };

        d_test_sa_options_init(&opts);
        parse_result = d_test_sa_options_parse(&opts, 1,
                                               argv_empty);

        result = d_assert_standalone(
            parse_result == true,
            "options_parse_no_args",
            "No arguments should return true",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_standalone_options_print_usage
  Tests the d_test_sa_options_print_usage function.
  Tests the following:
  - NULL program_name is handled safely
  - Valid program_name does not crash
*/
bool
d_tests_sa_standalone_options_print_usage
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: NULL program_name is handled safely
    d_test_sa_options_print_usage(NULL);

    result = d_assert_standalone(
        true,
        "options_print_usage_null_safe",
        "print_usage(NULL) should not crash",
        _counter) && result;

    // test 2: valid program_name does not crash
    d_test_sa_options_print_usage("test_program");

    result = d_assert_standalone(
        true,
        "options_print_usage_valid",
        "print_usage with valid name should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_options_all
  Aggregation function that runs all CLI options tests.
*/
bool
d_tests_sa_standalone_options_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] CLI Options\n");
    printf("  ----------------------\n");

    result = d_tests_sa_standalone_options_struct(_counter) && result;
    result = d_tests_sa_standalone_options_init(_counter) && result;
    result = d_tests_sa_standalone_options_parse(_counter) && result;
    result = d_tests_sa_standalone_options_print_usage(_counter) && result;

    return result;
}
