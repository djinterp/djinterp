/******************************************************************************
* djinterp [test]                                               test_printer.h
*
*  
*
*
* 
* link:      TBA
* path:      \inc\test\test_printer.h                             
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.09
******************************************************************************/

#ifndef DJINTERP_TEST_PRINTER_
#define	DJINTERP_TEST_PRINTER_ 1

#include <stdint.h>
#include <stdlib.h>
#include "../../djinterp.h"
#include "../../dmemory.h"
#include "../../test/assert.h"
#include "../../test/test.h"
#include "../../test.h"


// D_TEST_PRINT_LINE_WIDTH
//   constant: standard line width for formatted output
#define D_TEST_PRINT_LINE_WIDTH 80

// D_TEST_PRINT_HEADER_CHAR
//   constant: character used for header separators
#define D_TEST_PRINT_HEADER_CHAR            '='

// D_TEST_PRINT_SUBHEADER_CHAR
//   constant: character used for subheader separators
#define D_TEST_PRINT_SUBHEADER_CHAR         '-' 

// D_TEST_PRINT_DEFAULT_FRAMEWORK_DESC
//   constant: default description for framework header
#define D_TEST_PRINT_DEFAULT_FRAMEWORK_DESC "Comprehensive Unit Test Suite"

// D_TEST_PRINT_DEFAULT_VERSION
//   constant: default version string
#define D_TEST_PRINT_DEFAULT_VERSION        "djinterp Core v1.0 (Development Build)"

// D_TEST_PRINT_INITIAL_BUFFER_SIZE
//   constant: initial buffer size for string generation
#define D_TEST_PRINT_INITIAL_BUFFER_SIZE 4096

//=============================================================================
// PRINT STYLE FLAGS
//=============================================================================

// DTestPrintStyle
//   enum: style flags for controlling output appearance
enum DTestPrintStyle
{
    D_TEST_PRINT_STYLE_PLAIN   = 0x00,  // no special formatting
    D_TEST_PRINT_STYLE_COMPACT = 0x01,  // minimal output
    D_TEST_PRINT_STYLE_VERBOSE = 0x02,  // detailed output
    D_TEST_PRINT_STYLE_COLOR   = 0x04   // use color codes (if supported)
};

//=============================================================================
// FILE OPERATION MODES
//=============================================================================

// DTestFileMode
//   enum: file operation modes
enum DTestFileMode
{
    D_TEST_FILE_WRITE,      // overwrite file
    D_TEST_FILE_APPEND,     // append to end of file
    D_TEST_FILE_PREPEND     // prepend to beginning of file
};

//=============================================================================
// STRING BUFFER HELPER (INTERNAL)
//=============================================================================

// d_test_string_buffer
//   struct: dynamic string buffer for building output
struct d_test_string_buffer
{
    char*  data;
    size_t size;
    size_t capacity;
};

// Internal buffer management functions
struct d_test_string_buffer* d_test_string_buffer_new(size_t _initial_capacity);
bool                         d_test_string_buffer_append(struct d_test_string_buffer* _buffer, 
                                                          const char* _str);
bool                         d_test_string_buffer_append_format(struct d_test_string_buffer* _buffer,
                                                                const char* _format, ...);
char*                        d_test_string_buffer_finalize(struct d_test_string_buffer* _buffer);
void                         d_test_string_buffer_free(struct d_test_string_buffer* _buffer);

//=============================================================================
// FRAMEWORK HEADER FUNCTIONS - PRINT TO STDOUT
//=============================================================================

void d_test_print_framework_header(const char* _framework_name,
                                   const char* _framework_desc);
void d_test_print_framework_header_custom(const char* _framework_name,
                                          const char* _framework_desc,
                                          const char* _version,
                                          const char* _test_framework,
                                          const char* _platform);
void d_test_print_testing_approach(void);

//=============================================================================
// FRAMEWORK HEADER FUNCTIONS - RETURN STRING
//=============================================================================

char* d_test_print_framework_header_to_string(const char* _framework_name,
                                               const char* _framework_desc);
char* d_test_print_framework_header_custom_to_string(const char* _framework_name,
                                                     const char* _framework_desc,
                                                     const char* _version,
                                                     const char* _test_framework,
                                                     const char* _platform);
char* d_test_print_testing_approach_to_string(void);

//=============================================================================
// MODULE HEADER FUNCTIONS - PRINT TO STDOUT
//=============================================================================

void d_test_print_module_header(const char* _module_name,
                                const char* _description);
void d_test_print_module_header_detailed(const char* _module_name,
                                         const char* _description,
                                         const char* _file_path,
                                         size_t      _test_count);

//=============================================================================
// MODULE HEADER FUNCTIONS - RETURN STRING
//=============================================================================

char* d_test_print_module_header_to_string(const char* _module_name,
                                            const char* _description);
char* d_test_print_module_header_detailed_to_string(const char* _module_name,
                                                     const char* _description,
                                                     const char* _file_path,
                                                     size_t      _test_count);

//=============================================================================
// PROGRESS AND STATUS FUNCTIONS - PRINT TO STDOUT
//=============================================================================

void d_test_print_test_start(const char* _test_name);
void d_test_print_test_result(const char* _test_name, bool _passed);
void d_test_print_progress(size_t _current, size_t _total);

//=============================================================================
// PROGRESS AND STATUS FUNCTIONS - RETURN STRING
//=============================================================================

char* d_test_print_test_start_to_string(const char* _test_name);
char* d_test_print_test_result_to_string(const char* _test_name, bool _passed);
char* d_test_print_progress_to_string(size_t _current, size_t _total);

//=============================================================================
// RESULT SUMMARY FUNCTIONS - PRINT TO STDOUT
//=============================================================================

void d_test_print_module_results(const char* _module_name,
                                 const struct d_test_counter* _module_counter,
                                 bool                         _module_result);
void d_test_print_comprehensive_results(const struct d_test_counter* _overall_counter,
                                        bool   _overall_result,
                                        size_t _modules_tested,
                                        size_t _modules_passed);

//=============================================================================
// RESULT SUMMARY FUNCTIONS - RETURN STRING
//=============================================================================

char* d_test_print_module_results_to_string(const char* _module_name,
                                             const struct d_test_counter* _module_counter,
                                             bool                         _module_result);
char* d_test_print_comprehensive_results_to_string(const struct d_test_counter* _overall_counter,
                                                    bool   _overall_result,
                                                    size_t _modules_tested,
                                                    size_t _modules_passed);

//=============================================================================
// STATISTICS AND ANALYSIS FUNCTIONS - PRINT TO STDOUT
//=============================================================================

void d_test_print_statistics_table(const struct d_test_counter* _counter);
void d_test_print_success_rate(size_t _passed, size_t _total, const char* _item_name);

//=============================================================================
// STATISTICS AND ANALYSIS FUNCTIONS - RETURN STRING
//=============================================================================

char* d_test_print_statistics_table_to_string(const struct d_test_counter* _counter);
char* d_test_print_success_rate_to_string(size_t _passed, size_t _total, const char* _item_name);

//=============================================================================
// SEPARATOR AND FORMATTING FUNCTIONS - PRINT TO STDOUT
//=============================================================================

void d_test_print_separator(char _char, size_t _width);
void d_test_print_section_header(const char* _title);
void d_test_print_subsection_header(const char* _title);

//=============================================================================
// SEPARATOR AND FORMATTING FUNCTIONS - RETURN STRING
//=============================================================================

char* d_test_print_separator_to_string(char _char, size_t _width);
char* d_test_print_section_header_to_string(const char* _title);
char* d_test_print_subsection_header_to_string(const char* _title);

//=============================================================================
// INFORMATION AND NOTES FUNCTIONS - PRINT TO STDOUT
//=============================================================================

void d_test_print_implementation_notes(void);
void d_test_print_custom_notes(const char* _title, 
                               size_t _note_count, 
                               const char** _notes);
void d_test_print_info_line(const char* _message);
void d_test_print_warning_line(const char* _message);
void d_test_print_error_line(const char* _message);

//=============================================================================
// INFORMATION AND NOTES FUNCTIONS - RETURN STRING
//=============================================================================

char* d_test_print_implementation_notes_to_string(void);
char* d_test_print_custom_notes_to_string(const char* _title, 
                                           size_t _note_count, 
                                           const char** _notes);
char* d_test_print_info_line_to_string(const char* _message);
char* d_test_print_warning_line_to_string(const char* _message);
char* d_test_print_error_line_to_string(const char* _message);

//=============================================================================
// SUMMARY AND FINAL MESSAGES - PRINT TO STDOUT
//=============================================================================

void d_test_print_final_summary(bool _passed, const char* _framework_name);
void d_test_print_recommendations(bool _overall_result);

//=============================================================================
// SUMMARY AND FINAL MESSAGES - RETURN STRING
//=============================================================================

char* d_test_print_final_summary_to_string(bool _passed, const char* _framework_name);
char* d_test_print_recommendations_to_string(bool _overall_result);

//=============================================================================
// FILE I/O FUNCTIONS
//=============================================================================

// d_test_write_to_file
//   Write string to file (overwrites existing content)
bool d_test_write_to_file(const char* _filename, 
                          const char* _content);

// d_test_append_to_file
//   Append string to end of file
bool d_test_append_to_file(const char* _filename, 
                           const char* _content);

// d_test_prepend_to_file
//   Prepend string to beginning of file
bool d_test_prepend_to_file(const char* _filename, 
                            const char* _content);

// d_test_write_to_file_mode
//   Write string to file using specified mode
bool d_test_write_to_file_mode(const char* _filename, 
                               const char* _content,
                               enum DTestFileMode _mode);

//=============================================================================
// COMPLETE REPORT GENERATION
//=============================================================================

// d_test_generate_full_report_to_string
//   Generate a complete test report as a string
char* d_test_generate_full_report_to_string(const char* _framework_name,
                                            const char* _framework_desc,
                                            const struct d_test_counter* _overall_counter,
                                            bool _overall_result,
                                            size_t _modules_tested,
                                            size_t _modules_passed);

// d_test_generate_full_report_to_file
//   Generate a complete test report and write to file
bool d_test_generate_full_report_to_file(const char*  _filename,
                                         const char* _framework_name,
                                         const char* _framework_desc,
                                         const struct d_test_counter* _overall_counter,
                                         bool _overall_result,
                                         size_t _modules_tested,
                                         size_t _modules_passed,
                                         enum DTestFileMode _mode);

//=============================================================================
// UTILITY MACROS
//=============================================================================

// D_TEST_PRINT_AND_FREE
//   Helper macro: print string and free it
#define D_TEST_PRINT_AND_FREE(str_func) \
    do { \
        char* _tmp_str = (str_func); \
        if (_tmp_str) { \
            printf("%s", _tmp_str); \
            free(_tmp_str); \
        } \
    } while(0)

// D_TEST_WRITE_AND_FREE
//   Helper macro: write string to file and free it
#define D_TEST_WRITE_AND_FREE(filename, str_func) \
    do { \
        char* _tmp_str = (str_func); \
        if (_tmp_str) { \
            d_test_write_to_file((filename), _tmp_str); \
            free(_tmp_str); \
        } \
    } while(0)

// D_TEST_APPEND_AND_FREE
//   Helper macro: append string to file and free it
#define D_TEST_APPEND_AND_FREE(filename, str_func) \
    do { \
        char* _tmp_str = (str_func); \
        if (_tmp_str) { \
            d_test_append_to_file((filename), _tmp_str); \
            free(_tmp_str); \
        } \
    } while(0)


#endif	// DJINTERP_TEST_PRINTER_