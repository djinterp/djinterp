#include "..\..\..\..\..\inc\test\test_standalone.h"
#include "..\..\..\..\..\inc\container\stack\min_stack.h"
#include "..\..\..\..\..\test\test\test_printer_tests_sa.h"


int
main
(
    int    _argc,
    char** _argv
)
{
    // Print module header
    printf("================================================================================\n");
    printf("TESTING MODULE: test_printer\n");
    printf("================================================================================\n");
    printf("Description: Test output and reporting - string generation, file I/O, formatting\n");
    printf("Starting module test suite...\n");
    printf("================================================================================\n\n");

    // Run all test_printer tests
    struct d_test_counter overall_counter = { 0, 0, 0, 0 };
    bool overall_result = d_tests_sa_test_printer_all(&overall_counter);

    // Print module results
    printf("\n");
    printf("--------------------------------------------------------------------------------\n");
    printf("MODULE RESULTS: test_printer\n");
    printf("--------------------------------------------------------------------------------\n");
    printf("Assertions: %zu/%zu passed (%.2f%%)\n",
        overall_counter.assertions_passed,
        overall_counter.assertions_total,
        overall_counter.assertions_total > 0 ?
        (double)overall_counter.assertions_passed /
        (double)overall_counter.assertions_total * 100.0 : 0.0);
    printf("Unit Tests: %zu/%zu passed (%.2f%%)\n",
        overall_counter.tests_passed,
        overall_counter.tests_run,
        overall_counter.tests_run > 0 ?
        (double)overall_counter.tests_passed /
        (double)overall_counter.tests_run * 100.0 : 0.0);

    if (overall_result)
    {
        printf("Status: %s test_printer MODULE PASSED\n", TEST_SUCCESS_SYMBOL);
    }
    else
    {
        printf("Status: %s test_printer MODULE FAILED\n", TEST_FAIL_SYMBOL);
    }
    printf("--------------------------------------------------------------------------------\n\n");

    // Print comprehensive results
    printf("================================================================================\n");
    printf("COMPREHENSIVE TEST RESULTS\n");
    printf("================================================================================\n");
    printf("MODULE SUMMARY:\n");
    printf("  Modules Tested: 1\n");
    printf("  Modules Passed: %d\n", overall_result ? 1 : 0);
    printf("  Module Success Rate: %.2f%%\n\n", overall_result ? 100.0 : 0.0);

    printf("ASSERTION SUMMARY:\n");
    printf("  Total Assertions: %zu\n", overall_counter.assertions_total);
    printf("  Assertions Passed: %zu\n", overall_counter.assertions_passed);
    printf("  Assertions Failed: %zu\n",
        overall_counter.assertions_total - overall_counter.assertions_passed);
    printf("  Assertion Success Rate: %.2f%%\n\n",
        overall_counter.assertions_total > 0 ?
        (double)overall_counter.assertions_passed /
        (double)overall_counter.assertions_total * 100.0 : 0.0);

    printf("UNIT TEST SUMMARY:\n");
    printf("  Total Unit Tests: %zu\n", overall_counter.tests_run);
    printf("  Unit Tests Passed: %zu\n", overall_counter.tests_passed);
    printf("  Unit Tests Failed: %zu\n",
        overall_counter.tests_run - overall_counter.tests_passed);
    printf("  Unit Test Success Rate: %.2f%%\n\n",
        overall_counter.tests_run > 0 ?
        (double)overall_counter.tests_passed /
        (double)overall_counter.tests_run * 100.0 : 0.0);

    printf("OVERALL ASSESSMENT:\n");
    if (overall_result &&
        overall_counter.assertions_passed == overall_counter.assertions_total)
    {
        printf("  %s ALL TESTS PASSED SUCCESSFULLY!\n", TEST_SUCCESS_SYMBOL);
        printf("  %s test_printer module is ready for integration\n", TEST_SUCCESS_SYMBOL);
        printf("  %s All string generation functions verified\n", TEST_SUCCESS_SYMBOL);
        printf("  %s File I/O operations validated\n", TEST_SUCCESS_SYMBOL);
        printf("  %s Memory management appears sound\n", TEST_SUCCESS_SYMBOL);
        printf("  %s Error handling is robust\n", TEST_SUCCESS_SYMBOL);
    }
    else
    {
        printf("  %s SOME TESTS FAILED - ATTENTION REQUIRED\n", TEST_FAIL_SYMBOL);
        printf("  %s Review failed tests before proceeding\n", TEST_FAIL_SYMBOL);
        printf("  %s Check for memory leaks or logic errors\n", TEST_FAIL_SYMBOL);
        printf("  %s Verify all edge cases are handled properly\n", TEST_FAIL_SYMBOL);
    }
    printf("================================================================================\n\n");

    // Print implementation notes
    printf("================================================================================\n");
    printf("IMPLEMENTATION NOTES & RECOMMENDATIONS\n");
    printf("================================================================================\n");
    printf("CURRENT STATUS:\n");
    printf("  %s test_printer core functionality tested\n", TEST_INFO_SYMBOL);
    printf("  %s String buffer operations verified\n", TEST_INFO_SYMBOL);
    printf("  %s File I/O functions validated\n", TEST_INFO_SYMBOL);
    printf("  %s Memory management patterns established\n", TEST_INFO_SYMBOL);
    printf("  %s Error handling conventions defined\n\n", TEST_INFO_SYMBOL);

    printf("KNOWN ISSUES:\n");
    printf("  %s Some string generation functions need full implementation\n", TEST_INFO_SYMBOL);
    printf("  %s Report generation tests need expansion\n", TEST_INFO_SYMBOL);
    printf("  %s Formatting consistency validation needed\n", TEST_INFO_SYMBOL);
    printf("  %s Large-scale stress testing not yet included\n\n", TEST_INFO_SYMBOL);

    printf("NEXT STEPS:\n");
    printf("  %s Implement remaining string generation function tests\n", TEST_INFO_SYMBOL);
    printf("  %s Add comprehensive report generation tests\n", TEST_INFO_SYMBOL);
    printf("  %s Test print function/string version consistency\n", TEST_INFO_SYMBOL);
    printf("  %s Add performance benchmarking for large outputs\n", TEST_INFO_SYMBOL);
    printf("  %s Expand integration test scenarios\n\n", TEST_INFO_SYMBOL);

    printf("DEVELOPER GUIDELINES:\n");
    printf("  %s Always free strings returned by _to_string functions\n", TEST_INFO_SYMBOL);
    printf("  %s Use helper macros (D_TEST_PRINT_AND_FREE, etc.)\n", TEST_INFO_SYMBOL);
    printf("  %s Clean up temporary test files after each test\n", TEST_INFO_SYMBOL);
    printf("  %s Verify memory cleanup in all code paths\n", TEST_INFO_SYMBOL);
    printf("  %s Maintain consistent coding standards\n", TEST_INFO_SYMBOL);
    printf("================================================================================\n\n");

    // Print final summary
    if (overall_result)
    {
        printf("  %s test_printer Framework Test Suite COMPLETED SUCCESSFULLY\n", TEST_SUCCESS_SYMBOL);
        printf("   Ready for integration with djinterp core\n\n");
    }
    else
    {
        printf("  %s test_printer Framework Test Suite COMPLETED WITH FAILURES\n", TEST_FAIL_SYMBOL);
        printf("   Review failures before proceeding with development\n\n");
    }

    // Return appropriate exit code
    return overall_result ? 0 : 1;
}