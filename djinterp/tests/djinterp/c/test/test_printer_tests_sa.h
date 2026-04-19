/******************************************************************************
* djinterp [test]                                              test_tests_sa.h
*
* This is a test file for `test.h` unit tests.
* For the file itself, go to `\inc\test\test.h`.
* Note: this module is required to build DTest, so it uses `test_standalone.h`,
* rather than DTest for unit testing. Any modules that are not dependencies of
* DTest should use DTest for unit tests.
*
*
* link:      TBA
* file:      \test\test\test_tests_standalone.h
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.09.26
******************************************************************************/

#ifndef DJINTERP_TESTING_TEST_
#define	DJINTERP_TESTING_TEST_ 1

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../inc/c/djinterp.h"
#include "../../../inc/c/test/assert.h"
#include "../../../inc/c/test/test.h"
#include "../../../inc/c/test/test_standalone.h"


// Helper structure for managing temporary test files
struct d_test_temp_file
{
    char filename[256];
    bool created;
};

// Create a temporary test file with unique name
struct d_test_temp_file* d_test_create_temp_file(const char* _prefix);

// Read entire contents of a file into a dynamically allocated string
char* d_test_read_file_contents(const char* _filename);

// Clean up and delete temporary file
void d_test_cleanup_temp_file(struct d_test_temp_file* _temp_file);

//=============================================================================
// STRING BUFFER TESTS
//=============================================================================

// tests d_test_string_buffer_new with various initial capacities
bool d_tests_sa_test_string_buffer_new(struct d_test_counter* _test_info);

// tests d_test_string_buffer_append with various string sizes
bool d_tests_sa_test_string_buffer_append(struct d_test_counter* _test_info);

// tests d_test_string_buffer_append_format with various format strings
bool d_tests_sa_test_string_buffer_append_format(struct d_test_counter* _test_info);

// tests d_test_string_buffer_finalize and proper memory transfer
bool d_tests_sa_test_string_buffer_finalize(struct d_test_counter* _test_info);

// tests d_test_string_buffer_free with valid and NULL parameters
bool d_tests_sa_test_string_buffer_free(struct d_test_counter* _test_info);

// tests buffer auto-resize behavior with large strings
bool d_tests_sa_test_string_buffer_resize(struct d_test_counter* _test_info);

//=============================================================================
// BASIC STRING GENERATION TESTS
//=============================================================================

// tests d_test_print_separator_to_string with various characters and widths
bool d_tests_sa_test_separator_to_string(struct d_test_counter* _test_info);

// tests d_test_print_section_header_to_string formatting
bool d_tests_sa_test_section_header_to_string(struct d_test_counter* _test_info);

// tests d_test_print_subsection_header_to_string formatting
bool d_tests_sa_test_subsection_header_to_string(struct d_test_counter* _test_info);

// tests d_test_print_info_line_to_string with various messages
bool d_tests_sa_test_info_line_to_string(struct d_test_counter* _test_info);

// tests d_test_print_warning_line_to_string formatting
bool d_tests_sa_test_warning_line_to_string(struct d_test_counter* _test_info);

// tests d_test_print_error_line_to_string formatting
bool d_tests_sa_test_error_line_to_string(struct d_test_counter* _test_info);

//=============================================================================
// FRAMEWORK HEADER TESTS
//=============================================================================

// tests d_test_print_framework_header_to_string basic functionality
bool d_tests_sa_test_framework_header_to_string(struct d_test_counter* _test_info);

// tests d_test_print_framework_header_custom_to_string with custom parameters
bool d_tests_sa_test_framework_header_custom_to_string(struct d_test_counter* _test_info);

// tests d_test_print_testing_approach_to_string content
bool d_tests_sa_test_testing_approach_to_string(struct d_test_counter* _test_info);

//=============================================================================
// MODULE HEADER TESTS
//=============================================================================

// tests d_test_print_module_header_to_string basic functionality
bool d_tests_sa_test_module_header_to_string(struct d_test_counter* _test_info);

// tests d_test_print_module_header_detailed_to_string with all parameters
bool d_tests_sa_test_module_header_detailed_to_string(struct d_test_counter* _test_info);

//=============================================================================
// PROGRESS AND STATUS TESTS
//=============================================================================

// tests d_test_print_test_start_to_string formatting
bool d_tests_sa_test_test_start_to_string(struct d_test_counter* _test_info);

// tests d_test_print_test_result_to_string for pass and fail cases
bool d_tests_sa_test_test_result_to_string(struct d_test_counter* _test_info);

// tests d_test_print_progress_to_string with various values
bool d_tests_sa_test_progress_to_string(struct d_test_counter* _test_info);

//=============================================================================
// RESULT SUMMARY TESTS
//=============================================================================

// tests d_test_print_module_results_to_string with various counters
bool d_tests_sa_test_module_results_to_string(struct d_test_counter* _test_info);

// tests d_test_print_comprehensive_results_to_string formatting
bool d_tests_sa_test_comprehensive_results_to_string(struct d_test_counter* _test_info);

// tests d_test_print_statistics_table_to_string layout
bool d_tests_sa_test_statistics_table_to_string(struct d_test_counter* _test_info);

// tests d_test_print_success_rate_to_string calculations
bool d_tests_sa_test_success_rate_to_string(struct d_test_counter* _test_info);

//=============================================================================
// NOTES AND CUSTOM CONTENT TESTS
//=============================================================================

// tests d_test_print_implementation_notes_to_string content
bool d_tests_sa_test_implementation_notes_to_string(struct d_test_counter* _test_info);

// tests d_test_print_custom_notes_to_string with various note arrays
bool d_tests_sa_test_custom_notes_to_string(struct d_test_counter* _test_info);

//=============================================================================
// FINAL SUMMARY TESTS
//=============================================================================

// tests d_test_print_final_summary_to_string for pass and fail cases
bool d_tests_sa_test_final_summary_to_string(struct d_test_counter* _test_info);

// tests d_test_print_recommendations_to_string content
bool d_tests_sa_test_recommendations_to_string(struct d_test_counter* _test_info);

//=============================================================================
// FILE I/O TESTS
//=============================================================================

// tests d_test_write_to_file basic functionality
bool d_tests_sa_test_write_to_file(struct d_test_counter* _test_info);

// tests d_test_append_to_file behavior
bool d_tests_sa_test_append_to_file(struct d_test_counter* _test_info);

// tests d_test_prepend_to_file functionality
bool d_tests_sa_test_prepend_to_file(struct d_test_counter* _test_info);

// tests d_test_write_to_file_mode with all modes
bool d_tests_sa_test_write_to_file_mode(struct d_test_counter* _test_info);

// tests file I/O with empty files
bool d_tests_sa_test_file_io_empty_content(struct d_test_counter* _test_info);

// tests file I/O with large content
bool d_tests_sa_test_file_io_large_content(struct d_test_counter* _test_info);

// tests file I/O error handling (NULL parameters, invalid paths)
bool d_tests_sa_test_file_io_error_handling(struct d_test_counter* _test_info);

//=============================================================================
// REPORT GENERATION TESTS
//=============================================================================

// tests d_test_generate_full_report_to_string completeness
bool d_tests_sa_test_generate_full_report_to_string(struct d_test_counter* _test_info);

// tests d_test_generate_full_report_to_file with all modes
bool d_tests_sa_test_generate_full_report_to_file(struct d_test_counter* _test_info);

//=============================================================================
// PRINT FUNCTION TESTS (stdout versions)
//=============================================================================

// tests that print functions properly call string versions
bool d_tests_sa_test_print_functions_call_string_versions(struct d_test_counter* _test_info);

//=============================================================================
// EDGE CASE AND ERROR HANDLING TESTS
//=============================================================================

// tests NULL parameter handling for all string functions
bool d_tests_sa_test_null_parameter_handling(struct d_test_counter* _test_info);

// tests empty string handling
bool d_tests_sa_test_empty_string_handling(struct d_test_counter* _test_info);

// tests very long string handling (buffer boundaries)
bool d_tests_sa_test_long_string_handling(struct d_test_counter* _test_info);

// tests special character handling in strings
bool d_tests_sa_test_special_character_handling(struct d_test_counter* _test_info);

// tests memory allocation failure scenarios (where testable)
bool d_tests_sa_test_memory_allocation_failures(struct d_test_counter* _test_info);

//=============================================================================
// INTEGRATION TESTS
//=============================================================================

// tests building complex reports piece by piece
bool d_tests_sa_test_complex_report_building(struct d_test_counter* _test_info);

// tests interleaving file operations (write, append, prepend)
bool d_tests_sa_test_interleaved_file_operations(struct d_test_counter* _test_info);

// tests generating multiple reports to different files
bool d_tests_sa_test_multiple_concurrent_reports(struct d_test_counter* _test_info);

// tests consistency between print and string versions
bool d_tests_sa_test_print_string_consistency(struct d_test_counter* _test_info);

//=============================================================================
// FORMATTING AND CONTENT VALIDATION TESTS
//=============================================================================

// tests proper line width formatting
bool d_tests_sa_test_line_width_formatting(struct d_test_counter* _test_info);

// tests separator character consistency
bool d_tests_sa_test_separator_character_consistency(struct d_test_counter* _test_info);

// tests indentation consistency
bool d_tests_sa_test_indentation_consistency(struct d_test_counter* _test_info);

// tests symbol usage (PASS, FAIL, INFO, etc.)
bool d_tests_sa_test_symbol_usage(struct d_test_counter* _test_info);

// tests percentage calculations in statistics
bool d_tests_sa_test_percentage_calculations(struct d_test_counter* _test_info);

//=============================================================================
// MEMORY MANAGEMENT TESTS
//=============================================================================

// tests proper memory cleanup in all string-returning functions
bool d_tests_sa_test_string_function_memory_cleanup(struct d_test_counter* _test_info);

// tests for memory leaks in buffer operations
bool d_tests_sa_test_buffer_memory_leaks(struct d_test_counter* _test_info);

// tests repeated allocation/deallocation
bool d_tests_sa_test_repeated_allocations(struct d_test_counter* _test_info);

//=============================================================================
// COMPREHENSIVE TEST SUITE
//=============================================================================

// Runs all test_printer unit tests
bool d_tests_sa_test_printer_all(struct d_test_counter* _test_info);


#endif	// DJINTERP_TESTING_TEST_