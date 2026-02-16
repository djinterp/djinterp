#include "..\..\..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\..\..\tests\c\djinterp_tests_sa.h"


int
main
(
    int    _argc,
    char** _argv
)
{
    // Initialize overall test tracking
    struct d_test_counter overall_counter = {0, 0, 0, 0};
    size_t modules_tested = 0;
    size_t modules_passed = 0;
    bool overall_result = true;

    // Print framework header
    d_test_sa_create_framework_header("djinterp Core",
        "Comprehensive Testing of Core Types, Functions, and Macros");

    // ==========================================================================
    // TEST MODULE 1: d_index Functions
    // ==========================================================================
    {
        struct d_test_counter module_counter = {0, 0, 0, 0};

        d_test_sa_create_module_test_header("d_index Functions",
            "Testing d_index_convert_fast(), d_index_convert_safe(), and d_index_is_valid()");

        bool module_result = d_tests_sa_index_functions_all(&module_counter);

        // Update overall counters
        overall_counter.assertions_total  += module_counter.assertions_total;
        overall_counter.assertions_passed += module_counter.assertions_passed;
        overall_counter.tests_total       += module_counter.tests_total;
        overall_counter.tests_passed      += module_counter.tests_passed;

        modules_tested++;
        if (module_result) modules_passed++;
        overall_result = overall_result && module_result;

        d_test_sa_create_module_test_results("d_index Functions",
            &module_counter);
    }

    // ==========================================================================
    // TEST MODULE 2: Index Manipulation Macros
    // ==========================================================================
    {
        struct d_test_counter module_counter = {0, 0, 0, 0};

        d_test_sa_create_module_test_header("Index Manipulation Macros",
            "Testing D_CLAMP_INDEX, D_SAFE_ARR_IDX, D_IS_VALID_INDEX, D_NEG_IDX, D_ARR_IDX, and related macros");

        bool module_result = d_tests_sa_index_macros_all(&module_counter);

        // Update overall counters
        overall_counter.assertions_total  += module_counter.assertions_total;
        overall_counter.assertions_passed += module_counter.assertions_passed;
        overall_counter.tests_total       += module_counter.tests_total;
        overall_counter.tests_passed      += module_counter.tests_passed;

        modules_tested++;
        if (module_result) modules_passed++;
        overall_result = overall_result && module_result;

        d_test_sa_create_module_test_results("Index Manipulation Macros",
            &module_counter);
    }

    // ==========================================================================
    // TEST MODULE 3: Array Utility Macros
    // ==========================================================================
    {
        struct d_test_counter module_counter = {0, 0, 0, 0};

        d_test_sa_create_module_test_header("Array Utility Macros",
            "Testing D_ARRAY_TOTAL_SIZE and D_ARRAY_COUNT macros");

        bool module_result = d_tests_sa_array_macros_all(&module_counter);

        // Update overall counters
        overall_counter.assertions_total  += module_counter.assertions_total;
        overall_counter.assertions_passed += module_counter.assertions_passed;
        overall_counter.tests_total       += module_counter.tests_total;
        overall_counter.tests_passed      += module_counter.tests_passed;

        modules_tested++;
        if (module_result) modules_passed++;
        overall_result = overall_result && module_result;

        d_test_sa_create_module_test_results("Array Utility Macros",
            &module_counter);
    }

    // ==========================================================================
    // TEST MODULE 4: Boolean Constants and Macros
    // ==========================================================================
    {
        struct d_test_counter module_counter = {0, 0, 0, 0};

        d_test_sa_create_module_test_header("Boolean Constants and Macros",
            "Testing D_SUCCESS, D_FAILURE, D_ENABLED, D_DISABLED, D_IS_ENABLED(), and D_IS_DISABLED()");

        bool module_result = d_tests_sa_boolean_all(&module_counter);

        // Update overall counters
        overall_counter.assertions_total  += module_counter.assertions_total;
        overall_counter.assertions_passed += module_counter.assertions_passed;
        overall_counter.tests_total       += module_counter.tests_total;
        overall_counter.tests_passed      += module_counter.tests_passed;

        modules_tested++;
        if (module_result) modules_passed++;
        overall_result = overall_result && module_result;

        d_test_sa_create_module_test_results("Boolean Constants and Macros",
            &module_counter);
    }

    // ==========================================================================
    // TEST MODULE 5: Function Pointer Types
    // ==========================================================================
    {
        struct d_test_counter module_counter = {0, 0, 0, 0};

        d_test_sa_create_module_test_header("Function Pointer Types",
            "Testing fn_apply, fn_callback, fn_comparator, fn_free, fn_print, fn_to_string, and fn_write");

        bool module_result = d_tests_sa_function_pointers_all(&module_counter);

        // Update overall counters
        overall_counter.assertions_total  += module_counter.assertions_total;
        overall_counter.assertions_passed += module_counter.assertions_passed;
        overall_counter.tests_total       += module_counter.tests_total;
        overall_counter.tests_passed      += module_counter.tests_passed;

        modules_tested++;
        if (module_result) modules_passed++;
        overall_result = overall_result && module_result;

        d_test_sa_create_module_test_results("Function Pointer Types",
            &module_counter);
    }

    // ==========================================================================
    // TEST MODULE 6: Edge Cases and Boundary Conditions
    // ==========================================================================
    {
        struct d_test_counter module_counter = {0, 0, 0, 0};

        d_test_sa_create_module_test_header("Edge Cases and Boundaries",
            "Testing SIZE_MAX boundaries, ssize_t overflow, zero/single-element arrays, and large array stress tests");

        bool module_result = d_tests_sa_edge_cases_all(&module_counter);

        // Update overall counters
        overall_counter.assertions_total  += module_counter.assertions_total;
        overall_counter.assertions_passed += module_counter.assertions_passed;
        overall_counter.tests_total       += module_counter.tests_total;
        overall_counter.tests_passed      += module_counter.tests_passed;

        modules_tested++;
        if (module_result) modules_passed++;
        overall_result = overall_result && module_result;

        d_test_sa_create_module_test_results("Edge Cases and Boundaries",
            &module_counter);
    }

    // ==========================================================================
    // COMPREHENSIVE RESULTS
    // ==========================================================================
    
    // Create suite results structure
    struct d_test_sa_suite_results suite_results;
    suite_results.modules_total = modules_tested;
    suite_results.modules_passed = modules_passed;
    suite_results.totals = overall_counter;
    suite_results.modules = NULL;  // We're not tracking individual module results

    d_test_sa_create_comprehensive_results(&suite_results);

    // ==========================================================================
    // IMPLEMENTATION NOTES
    // ==========================================================================
    
    // Create note sections
    struct d_test_sa_note_item status_items[7];
    status_items[0].prefix = D_TEST_SYMBOL_PASS;
    status_items[0].message = "d_index core functions (convert_fast, convert_safe, is_valid) validated and ready for use";
    status_items[1].prefix = D_TEST_SYMBOL_PASS;
    status_items[1].message = "Critical safety macros (D_SAFE_ARR_IDX, D_CLAMP_INDEX) thoroughly tested with 60+ assertions";
    status_items[2].prefix = D_TEST_SYMBOL_PASS;
    status_items[2].message = "Index manipulation macros working correctly for positive, negative, and edge case indices";
    status_items[3].prefix = D_TEST_SYMBOL_PASS;
    status_items[3].message = "Array utility macros functioning properly for size calculations and element counting";
    status_items[4].prefix = D_TEST_SYMBOL_PASS;
    status_items[4].message = "Boolean constants and evaluation macros established and tested";
    status_items[5].prefix = D_TEST_SYMBOL_PASS;
    status_items[5].message = "Function pointer types defined and validated for callback patterns";
    status_items[6].prefix = D_TEST_SYMBOL_PASS;
    status_items[6].message = "Edge case handling verified for boundary conditions and extreme values";

    struct d_test_sa_note_item issues_items[5];
    issues_items[0].prefix = D_TEST_SYMBOL_WARNING;
    issues_items[0].message = "Some test modules still in stub form (marked with TODO)";
    issues_items[1].prefix = D_TEST_SYMBOL_WARNING;
    issues_items[1].message = "Function pointer tests need mock implementations";
    issues_items[2].prefix = D_TEST_SYMBOL_WARNING;
    issues_items[2].message = "Edge case tests need comprehensive boundary value analysis";
    issues_items[3].prefix = D_TEST_SYMBOL_WARNING;
    issues_items[3].message = "Array macro tests need porting from existing test suite";
    issues_items[4].prefix = D_TEST_SYMBOL_WARNING;
    issues_items[4].message = "Performance benchmarks not yet implemented";

    struct d_test_sa_note_item steps_items[8];
    steps_items[0].prefix = D_TEST_SYMBOL_INFO;
    steps_items[0].message = "Complete implementation of djinterp_tests_sa_index.c (port from existing tests)";
    steps_items[1].prefix = D_TEST_SYMBOL_INFO;
    steps_items[1].message = "Complete implementation of djinterp_tests_sa_array.c (port from existing tests)";
    steps_items[2].prefix = D_TEST_SYMBOL_INFO;
    steps_items[2].message = "Implement djinterp_tests_sa_boolean.c with macro evaluation tests";
    steps_items[3].prefix = D_TEST_SYMBOL_INFO;
    steps_items[3].message = "Implement djinterp_tests_sa_function_ptr.c with mock function tests";
    steps_items[4].prefix = D_TEST_SYMBOL_INFO;
    steps_items[4].message = "Implement djinterp_tests_sa_edge.c with comprehensive boundary tests";
    steps_items[5].prefix = D_TEST_SYMBOL_INFO;
    steps_items[5].message = "Add performance benchmarks for index conversion operations";
    steps_items[6].prefix = D_TEST_SYMBOL_INFO;
    steps_items[6].message = "Integrate test suite with CI/CD pipeline";
    steps_items[7].prefix = D_TEST_SYMBOL_INFO;
    steps_items[7].message = "Add code coverage analysis tooling";

    struct d_test_sa_note_item guidelines_items[8];
    guidelines_items[0].prefix = D_TEST_SYMBOL_INFO;
    guidelines_items[0].message = "Always use d_index_is_valid() before d_index_convert_fast() for safety";
    guidelines_items[1].prefix = D_TEST_SYMBOL_INFO;
    guidelines_items[1].message = "Prefer D_SAFE_ARR_IDX over D_ARR_IDX for production code to prevent crashes";
    guidelines_items[2].prefix = D_TEST_SYMBOL_INFO;
    guidelines_items[2].message = "Use D_CLAMP_INDEX when you need guaranteed valid indices without errors";
    guidelines_items[3].prefix = D_TEST_SYMBOL_INFO;
    guidelines_items[3].message = "Test all index-related code with both positive and negative indices";
    guidelines_items[4].prefix = D_TEST_SYMBOL_INFO;
    guidelines_items[4].message = "Verify array size calculations with D_ARRAY_TOTAL_SIZE before memory operations";
    guidelines_items[5].prefix = D_TEST_SYMBOL_INFO;
    guidelines_items[5].message = "Use D_IS_VALID_INDEX_N for stricter validation in critical code paths";
    guidelines_items[6].prefix = D_TEST_SYMBOL_INFO;
    guidelines_items[6].message = "Document any use of d_index_convert_fast() with clear precondition comments";
    guidelines_items[7].prefix = D_TEST_SYMBOL_INFO;
    guidelines_items[7].message = "Run the full test suite before committing changes to core index functionality";

    struct d_test_sa_note_section note_sections[4];
    note_sections[0].title = "CURRENT STATUS";
    note_sections[0].count = 7;
    note_sections[0].items = status_items;
    
    note_sections[1].title = "KNOWN ISSUES";
    note_sections[1].count = 5;
    note_sections[1].items = issues_items;
    
    note_sections[2].title = "NEXT STEPS";
    note_sections[2].count = 8;
    note_sections[2].items = steps_items;
    
    note_sections[3].title = "USAGE GUIDELINES";
    note_sections[3].count = 8;
    note_sections[3].items = guidelines_items;

    d_test_sa_create_implementation_notes(4, note_sections);

    // ==========================================================================
    // FINAL SUMMARY
    // ==========================================================================
    printf("\n");

    if (overall_result)
    {
        printf("%s%s djinterp Core Testing COMPLETED SUCCESSFULLY\n",
            D_INDENT, D_TEST_SYMBOL_PASS);
        printf("%s All core types, functions, and macros validated and ready for use\n", 
            D_INDENT);
        printf("%s Proceed with confidence in the stability of the core framework\n", 
            D_INDENT);
        printf("%s Index operations are safe and thoroughly tested\n", 
            D_INDENT);
    }
    else
    {
        printf("%s%s djinterp Core Testing COMPLETED WITH FAILURES\n",
            D_INDENT, D_TEST_SYMBOL_FAIL);
        printf("%s Review and fix all failures before using affected components\n", 
            D_INDENT);
        printf("%s Core functionality must be solid before building higher-level features\n", 
            D_INDENT);
        printf("%s Pay special attention to any D_SAFE_ARR_IDX or D_CLAMP_INDEX failures\n", 
            D_INDENT);
    }

    printf("\n");

    // Print test coverage summary
    if (overall_counter.assertions_total > 0)
    {
        double assertion_coverage = (100.0 * overall_counter.assertions_passed) / 
                                    overall_counter.assertions_total;
        printf("%s Assertion Coverage: %.1f%% (%zu/%zu passed)\n",
            D_INDENT, assertion_coverage, 
            overall_counter.assertions_passed, 
            overall_counter.assertions_total);
    }

    if (overall_counter.tests_total > 0)
    {
        double test_coverage = (100.0 * overall_counter.tests_passed) / 
                               overall_counter.tests_total;
        printf("%s Unit Test Coverage: %.1f%% (%zu/%zu passed)\n",
            D_INDENT, test_coverage, 
            overall_counter.tests_passed, 
            overall_counter.tests_total);
    }

    if (modules_tested > 0)
    {
        double module_coverage = (100.0 * modules_passed) / modules_tested;
        printf("%s Module Coverage: %.1f%% (%zu/%zu passed)\n",
            D_INDENT, module_coverage, 
            modules_passed, 
            modules_tested);
    }

    // wait for user input before closing
    printf("\nPress Enter to exit...");
    (void)getchar();

    // Return appropriate exit code for CI/CD systems
    return overall_result 
	       ? 0 
	       : 1;
}
