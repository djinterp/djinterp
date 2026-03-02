#include "../../../inc/c/test/test_printer.h"


//=============================================================================
// FRAMEWORK HEADER FUNCTIONS
//=============================================================================

/*
d_test_print_framework_header
  Prints standardized header for the test framework with version info,
  platform details, and testing philosophy.

Parameter(s):
  _framework_name: name of the framework being tested
  _framework_desc: description of the test suite (NULL uses default)
Return:
  none
*/
void
d_test_print_framework_header
(
    const char* _framework_name,
    const char* _framework_desc
)
{
    const char* desc = (_framework_desc) ? _framework_desc :
                                           D_TEST_PRINT_DEFAULT_FRAMEWORK_DESC;

    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("%s - %s\n", _framework_name, desc);
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("Framework Version: %s\n", D_TEST_PRINT_DEFAULT_VERSION);
    printf("Test Framework:    Standalone Testing (test_standalone.h)\n");
    printf("Target Platform:   Cross-Platform C99/C11\n");
    printf("Test Philosophy:   \"dTest -- for those who DETEST testing\"\n\n");
    
    d_test_print_testing_approach();
}

/*
d_test_print_framework_header_custom
  Prints customized framework header with user-specified version and details.

Parameter(s):
  _framework_name: name of the framework
  _framework_desc: description of the test suite
  _version: version string
  _test_framework: name of testing framework being used
  _platform: target platform description
Return:
  none
*/
void
d_test_print_framework_header_custom
(
    const char* _framework_name,
    const char* _framework_desc,
    const char* _version,
    const char* _test_framework,
    const char* _platform
)
{
    const char* desc = (_framework_desc) ? _framework_desc :
                                           D_TEST_PRINT_DEFAULT_FRAMEWORK_DESC;
    const char* ver = (_version) ? _version : D_TEST_PRINT_DEFAULT_VERSION;
    const char* test_fw = (_test_framework) ? _test_framework : 
                                              "Standalone Testing (test_standalone.h)";
    const char* plat = (_platform) ? _platform : "Cross-Platform C99/C11";

    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("%s - %s\n", _framework_name, desc);
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("Framework Version: %s\n", ver);
    printf("Test Framework:    %s\n", test_fw);
    printf("Target Platform:   %s\n", plat);
    printf("Test Philosophy:   \"dTest -- for those who DETEST testing\"\n\n");
    
    d_test_print_testing_approach();
}

/*
d_test_print_testing_approach
  Prints the testing approach philosophy and methodology section.

Parameter(s):
  none
Return:
  none
*/
void
d_test_print_testing_approach
(
    void
)
{
    printf("TESTING APPROACH:\n");
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("%s Bootstrap Testing: Using standalone framework to test core modules\n", 
           TEST_INFO_SYMBOL);
    printf("%s Meta-Testing: Testing the testing framework itself (d_test)\n", 
           TEST_INFO_SYMBOL);
    printf("%s Foundation First: Core data structures before higher-level modules\n", 
           TEST_INFO_SYMBOL);
    printf("%s Comprehensive: Edge cases, memory management, and error conditions\n", 
           TEST_INFO_SYMBOL);
    printf("%s Production Ready: Tests designed for CI/CD integration\n", 
           TEST_INFO_SYMBOL);
    printf("\n");
}

//=============================================================================
// MODULE HEADER FUNCTIONS
//=============================================================================

/*
d_test_print_module_header
  Prints standardized header for individual module test sections.

Parameter(s):
  _module_name: name of the module being tested
  _description: brief description of the module's purpose
Return:
  none
*/
void
d_test_print_module_header
(
    const char* _module_name,
    const char* _description
)
{
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("TESTING MODULE: %s\n", _module_name);
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("Description: %s\n", _description);
    printf("Starting module test suite...\n");
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
}

/*
d_test_print_module_header_detailed
  Prints detailed module header with additional information.

Parameter(s):
  _module_name: name of the module
  _description: module description
  _file_path: path to module source file
  _test_count: number of unit tests to be run
Return:
  none
*/
void
d_test_print_module_header_detailed
(
    const char* _module_name,
    const char* _description,
    const char* _file_path,
    size_t      _test_count
)
{
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("TESTING MODULE: %s\n", _module_name);
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("Description: %s\n", _description);
    if (_file_path)
    {
        printf("Source File: %s\n", _file_path);
    }
    printf("Test Count:  %zu unit tests\n", _test_count);
    printf("Starting module test suite...\n");
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
}

//=============================================================================
// PROGRESS AND STATUS FUNCTIONS
//=============================================================================

/*
d_test_print_test_start
  Prints a message indicating a specific test is starting.

Parameter(s):
  _test_name: name of the test being started
Return:
  none
*/
void
d_test_print_test_start
(
    const char* _test_name
)
{
    printf("%sRunning: %s...\n", D_INDENT, _test_name);
}

/*
d_test_print_test_result
  Prints the result of a completed test.

Parameter(s):
  _test_name: name of the completed test
  _passed: whether the test passed
Return:
  none
*/
void
d_test_print_test_result
(
    const char* _test_name,
    bool        _passed
)
{
    if (_passed)
    {
        printf("%s%s %s: PASSED\n", D_INDENT, TEST_PASS_SYMBOL, _test_name);
    }
    else
    {
        printf("%s%s %s: FAILED\n", D_INDENT, TEST_FAIL_SYMBOL, _test_name);
    }
}

/*
d_test_print_progress
  Prints progress indicator showing current position in test suite.

Parameter(s):
  _current: current test number
  _total:   total number of tests
Return:
  none
*/
void
d_test_print_progress
(
    size_t _current,
    size_t _total
)
{
    double percent = (_total > 0) ? ((double)_current / (double)_total * 100.0) : 0.0;
    printf("%sProgress: [%zu/%zu] (%.1f%%)\n", D_INDENT, _current, _total, percent);
}

//=============================================================================
// RESULT SUMMARY FUNCTIONS
//=============================================================================

/*
d_test_print_module_results
  Prints standardized results summary for a completed module test suite.

Parameter(s):
  _module_name:    name of the tested module
  _module_counter: test counter containing module-specific results
  _module_result:  boolean indicating overall module test success
Return:
  none
*/
void
d_test_print_module_results
(
    const char*                  _module_name,
    const struct d_test_counter* _module_counter,
    bool                         _module_result
)
{
    printf("\n");
    d_test_print_separator(D_TEST_PRINT_SUBHEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("MODULE RESULTS: %s\n", _module_name);
    d_test_print_separator(D_TEST_PRINT_SUBHEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    
    // Assertion statistics
    printf("Assertions: %zu/%zu passed (%.2f%%)\n",
        _module_counter->assertions_passed, 
        _module_counter->assertions_total,
        _module_counter->assertions_total > 0 ? 
            (double)_module_counter->assertions_passed / 
            (double)_module_counter->assertions_total * 100.0 : 0.0);
    
    // Unit test statistics
    printf("Unit Tests: %zu/%zu passed (%.2f%%)\n",
        _module_counter->tests_passed,
        _module_counter->tests_run,
        _module_counter->tests_run > 0 ? 
            (double)_module_counter->tests_passed / 
            (double)_module_counter->tests_run * 100.0 : 0.0);

    // Overall status
    if (_module_result)
    {
        printf("Status: %s %s MODULE PASSED\n", TEST_SUCCESS_SYMBOL, _module_name);
    }
    else
    {
        printf("Status: %s %s MODULE FAILED\n", TEST_FAIL_SYMBOL, _module_name);
    }
    d_test_print_separator(D_TEST_PRINT_SUBHEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("\n");
}

/*
d_test_print_comprehensive_results
  Prints final comprehensive test results with detailed statistics.

Parameter(s):
  _overall_counter: cumulative results from all modules
  _overall_result:  whether all tests passed
  _modules_tested:  number of modules included in test run
  _modules_passed:  number of modules that passed all tests
Return:
  none
*/
void
d_test_print_comprehensive_results
(
    const struct d_test_counter* _overall_counter,
    bool                         _overall_result,
    size_t                       _modules_tested,
    size_t                       _modules_passed
)
{
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("COMPREHENSIVE TEST RESULTS\n");
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    
    // Module-level results
    printf("MODULE SUMMARY:\n");
    printf("  Modules Tested: %zu\n", _modules_tested);
    printf("  Modules Passed: %zu\n", _modules_passed);
    printf("  Module Success Rate: %.2f%%\n",
        _modules_tested > 0 ? 
            (double)_modules_passed / (double)_modules_tested * 100.0 : 0.0);
    printf("\n");
    
    // Assertion-level results
    printf("ASSERTION SUMMARY:\n");
    printf("  Total Assertions: %zu\n", _overall_counter->assertions_total);
    printf("  Assertions Passed: %zu\n", _overall_counter->assertions_passed);
    printf("  Assertions Failed: %zu\n", 
        _overall_counter->assertions_total - _overall_counter->assertions_passed);
    printf("  Assertion Success Rate: %.2f%%\n",
        _overall_counter->assertions_total > 0 ? 
            (double)_overall_counter->assertions_passed / 
            (double)_overall_counter->assertions_total * 100.0 : 0.0);
    printf("\n");
    
    // Unit test-level results
    printf("UNIT TEST SUMMARY:\n");
    printf("  Total Unit Tests: %zu\n", _overall_counter->tests_run);
    printf("  Unit Tests Passed: %zu\n", _overall_counter->tests_passed);
    printf("  Unit Tests Failed: %zu\n", 
        _overall_counter->tests_run - _overall_counter->tests_passed);
    printf("  Unit Test Success Rate: %.2f%%\n",
        _overall_counter->tests_run > 0 ? 
            (double)_overall_counter->tests_passed / 
            (double)_overall_counter->tests_run * 100.0 : 0.0);
    printf("\n");
    
    // Overall assessment
    printf("OVERALL ASSESSMENT:\n");
    if (_overall_result && 
        _overall_counter->assertions_passed == _overall_counter->assertions_total)
    {
        printf("  %s ALL TESTS PASSED SUCCESSFULLY!\n", TEST_SUCCESS_SYMBOL);
        printf("  %s djinterp core framework is ready for development\n", TEST_SUCCESS_SYMBOL);
        printf("  %s All tested modules meet quality standards\n", TEST_SUCCESS_SYMBOL);
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
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
}

//=============================================================================
// STATISTICS AND ANALYSIS FUNCTIONS
//=============================================================================

/*
d_test_print_statistics_table
  Prints detailed statistics in table format.

Parameter(s):
  _counter: test counter containing statistics
Return:
  none
*/
void
d_test_print_statistics_table
(
    const struct d_test_counter* _counter
)
{
    printf("\n");
    printf("STATISTICS TABLE:\n");
    d_test_print_separator(D_TEST_PRINT_SUBHEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("| %-30s | %10s | %10s | %10s |\n", 
           "Category", "Total", "Passed", "Failed");
    d_test_print_separator(D_TEST_PRINT_SUBHEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("| %-30s | %10zu | %10zu | %10zu |\n",
           "Assertions",
           _counter->assertions_total,
           _counter->assertions_passed,
           _counter->assertions_total - _counter->assertions_passed);
    printf("| %-30s | %10zu | %10zu | %10zu |\n",
           "Unit Tests",
           _counter->tests_run,
           _counter->tests_passed,
           _counter->tests_run - _counter->tests_passed);
    d_test_print_separator(D_TEST_PRINT_SUBHEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("\n");
}

/*
d_test_print_success_rate
  Prints success rate analysis with visual indicators.

Parameter(s):
  _passed: number of items that passed
  _total: total number of items
  _item_name: name of items being measured (e.g., "Tests", "Assertions")
Return:
  none
*/
void
d_test_print_success_rate
(
    size_t      _passed,
    size_t      _total,
    const char* _item_name
)
{
    double rate = (_total > 0) ? ((double)_passed / (double)_total * 100.0) : 
                                 0.0;

    const char* symbol = (rate >= 100.0) ? TEST_SUCCESS_SYMBOL : 
                         (rate >= 50.0)  ? TEST_INFO_SYMBOL : 
                                           TEST_FAIL_SYMBOL;
    
    printf("%s %s Success Rate: %.2f%% (%zu/%zu)\n", symbol, 
                                                     _item_name, 
                                                     rate, 
                                                     _passed, 
                                                     _total);
}

//=============================================================================
// SEPARATOR AND FORMATTING FUNCTIONS
//=============================================================================

/*
d_test_print_separator
  Prints a horizontal separator line.

Parameter(s):
  _char: character to use for separator
  _width: width of separator line (0 uses default)
Return:
  none
*/
void
d_test_print_separator
(
    char   _char,
    size_t _width
)
{
    size_t width = (_width > 0) ? _width : D_TEST_PRINT_LINE_WIDTH;
    
    for (size_t i = 0; i < width; i++)
    {
        putchar(_char);
    }
    putchar('\n');
}

/*
d_test_print_section_header
  Prints a formatted section header with title.

Parameter(s):
  _title: section title text
Return:
  none
*/
void
d_test_print_section_header
(
    const char* _title
)
{
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("%s\n", _title);
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
}

/*
d_test_print_subsection_header
  Prints a formatted subsection header with title.

Parameter(s):
  _title: subsection title text
Return:
  none
*/
void
d_test_print_subsection_header
(
    const char* _title
)
{
    d_test_print_separator(D_TEST_PRINT_SUBHEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("%s\n", _title);
    d_test_print_separator(D_TEST_PRINT_SUBHEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
}

//=============================================================================
// INFORMATION AND NOTES FUNCTIONS
//=============================================================================

/*
d_test_print_implementation_notes
  Prints implementation notes, known issues, and developer guidelines.

Parameter(s):
  none
Return:
  none
*/
void
d_test_print_implementation_notes
(
    void
)
{
    printf("\n");
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    printf("IMPLEMENTATION NOTES & RECOMMENDATIONS\n");
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    
    printf("CURRENT STATUS:\n");
    printf("  %s d_test framework core functionality tested\n", TEST_INFO_SYMBOL);
    printf("  %s Core data structures verified\n", TEST_INFO_SYMBOL);
    printf("  %s Memory management patterns established\n", TEST_INFO_SYMBOL);
    printf("  %s Error handling conventions defined\n", TEST_INFO_SYMBOL);
    printf("  %s Cross-platform compatibility addressed\n", TEST_INFO_SYMBOL);
    printf("\n");
    
    printf("KNOWN ISSUES:\n");
    printf("  %s Some advanced features have incomplete implementations\n", TEST_INFO_SYMBOL);
    printf("  %s Complex configuration merging needs validation\n", TEST_INFO_SYMBOL);
    printf("  %s Thread safety not yet tested (single-threaded design assumed)\n", 
           TEST_INFO_SYMBOL);
    printf("\n");
    
    printf("NEXT STEPS:\n");
    printf("  %s Complete remaining module implementations\n", TEST_INFO_SYMBOL);
    printf("  %s Add integration test scenarios\n", TEST_INFO_SYMBOL);
    printf("  %s Create performance benchmarking\n", TEST_INFO_SYMBOL);
    printf("  %s Expand test coverage to edge cases\n", TEST_INFO_SYMBOL);
    printf("\n");
    
    printf("DEVELOPER GUIDELINES:\n");
    printf("  %s Always run full test suite before commits\n", TEST_INFO_SYMBOL);
    printf("  %s Add unit tests for any new functions\n", TEST_INFO_SYMBOL);
    printf("  %s Test edge cases and error conditions thoroughly\n", TEST_INFO_SYMBOL);
    printf("  %s Verify memory cleanup in all code paths\n", TEST_INFO_SYMBOL);
    printf("  %s Maintain consistent coding standards\n", TEST_INFO_SYMBOL);
    
    d_test_print_separator(D_TEST_PRINT_HEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
}

/*
d_test_print_custom_notes
  Prints custom notes section with provided content.

Parameter(s):
  _title: title for notes section
  _note_count: number of notes to print
  _notes: array of note strings
Return:
  none
*/
void
d_test_print_custom_notes
(
    const char*  _title,
    size_t       _note_count,
    const char** _notes
)
{
    if (!_title || !_notes || _note_count == 0)
    {
        return;
    }
    
    printf("\n%s:\n", _title);
    for (size_t i = 0; i < _note_count; i++)
    {
        if (_notes[i])
        {
            printf("  %s %s\n", TEST_INFO_SYMBOL, _notes[i]);
        }
    }
    printf("\n");
}

/*
d_test_print_info_line
  Prints a single information line with icon.

Parameter(s):
  _message: information message to print
Return:
  none
*/
void
d_test_print_info_line
(
    const char* _message
)
{
    if (_message)
    {
        printf("%s %s\n", TEST_INFO_SYMBOL, _message);
    }
}

/*
d_test_print_warning_line
  Prints a warning message line.

Parameter(s):
  _message: warning message to print
Return:
  none
*/
void
d_test_print_warning_line
(
    const char* _message
)
{
    if (_message)
    {
        printf("%s WARNING: %s\n", TEST_INFO_SYMBOL, _message);
    }
}

/*
d_test_print_error_line
  Prints an error message line.

Parameter(s):
  _message: error message to print
Return:
  none
*/
void
d_test_print_error_line
(
    const char* _message
)
{
    if (_message)
    {
        printf("%s ERROR: %s\n", TEST_FAIL_SYMBOL, _message);
    }
}

//=============================================================================
// SUMMARY AND FINAL MESSAGES
//=============================================================================

/*
d_test_print_final_summary
  Prints final summary message with overall assessment.

Parameter(s):
  _passed: whether all tests passed
  _framework_name: name of framework being tested
Return:
  none
*/
void
d_test_print_final_summary
(
    bool        _passed,
    const char* _framework_name
)
{
    printf("\n");

    if (_passed)
    {
        printf("%s%s %s Framework Test Suite COMPLETED SUCCESSFULLY\n", 
            D_INDENT, TEST_SUCCESS_SYMBOL, _framework_name);
        printf("%s Ready for development and integration\n", D_INDENT);
    }
    else
    {
        printf("%s%s %s Framework Test Suite COMPLETED WITH FAILURES\n", 
            D_INDENT, TEST_FAIL_SYMBOL, _framework_name);
        printf("%s Review failures before proceeding with development\n", D_INDENT);
    }

    printf("\n");
}

/*
d_test_print_recommendations
  Prints next steps and recommendations based on test results.

Parameter(s):
  _overall_result: whether all tests passed
Return:
  none
*/
void
d_test_print_recommendations
(
    bool _overall_result
)
{
    printf("\nRECOMMENDATIONS:\n");
    d_test_print_separator(D_TEST_PRINT_SUBHEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);
    
    if (_overall_result)
    {
        printf("%s%s Proceed with next development phase\n",        D_INDENT, TEST_SUCCESS_SYMBOL);
        printf("%s%s Consider expanding test coverage\n",           D_INDENT, TEST_INFO_SYMBOL);
        printf("%s%s Review code for optimization opportunities\n", D_INDENT, TEST_INFO_SYMBOL);
        printf("%s%s Document any new patterns or conventions\n",   D_INDENT, TEST_INFO_SYMBOL);
    }
    else
    {
        printf("%s%s Fix all failing tests before proceeding\n",         D_INDENT, TEST_FAIL_SYMBOL);
        printf("%s%s Review error messages for root causes\n",           D_INDENT, TEST_FAIL_SYMBOL);
        printf("%s%s Check for memory leaks with valgrind/sanitizers\n", D_INDENT, TEST_INFO_SYMBOL);
        printf("%s%s Verify assumptions about edge cases\n",             D_INDENT, TEST_INFO_SYMBOL);
        printf("%s%s Consider refactoring problematic areas\n",          D_INDENT, TEST_INFO_SYMBOL);
    }
    
    d_test_print_separator(D_TEST_PRINT_SUBHEADER_CHAR, D_TEST_PRINT_LINE_WIDTH);

    printf("\n");
}