/******************************************************************************
* djinterp [test]                                            dio_tests_sa.h
*
*   Unit test declarations for `dio.h` module.
*   Provides comprehensive testing of all d_io functions including formatted
* input/output (secure variants), character and string I/O, large file stream
* positioning, and error handling.
*
*
* path:      \tests\c\core\dio_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.16
******************************************************************************/

#ifndef DJINTERP_TESTS_DIO_SA_
#define DJINTERP_TESTS_DIO_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "..\inc\test\test_standalone.h"
#include "..\inc\dio.h"
#include "..\inc\string_fn.h"
#include "..\inc\dstring.h"


/******************************************************************************
 * I. FORMATTED INPUT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_dio_sscanf(struct d_test_counter* _counter);
bool d_tests_sa_dio_sscanf_s(struct d_test_counter* _counter);
bool d_tests_sa_dio_vsscanf(struct d_test_counter* _counter);
bool d_tests_sa_dio_vsscanf_s(struct d_test_counter* _counter);
bool d_tests_sa_dio_fscanf(struct d_test_counter* _counter);
bool d_tests_sa_dio_fscanf_s(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_dio_formatted_input_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. FORMATTED OUTPUT FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_dio_sprintf_s(struct d_test_counter* _counter);
bool d_tests_sa_dio_vsprintf_s(struct d_test_counter* _counter);
bool d_tests_sa_dio_snprintf(struct d_test_counter* _counter);
bool d_tests_sa_dio_vsnprintf(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_dio_formatted_output_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. CHARACTER AND STRING I/O FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_dio_gets_s(struct d_test_counter* _counter);
bool d_tests_sa_dio_fputs(struct d_test_counter* _counter);
bool d_tests_sa_dio_fgets(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_dio_char_string_io_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. LARGE FILE STREAM POSITIONING FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_dio_fgetpos(struct d_test_counter* _counter);
bool d_tests_sa_dio_fsetpos(struct d_test_counter* _counter);
bool d_tests_sa_dio_rewind(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_dio_file_positioning_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. ERROR HANDLING FUNCTION TESTS
 *****************************************************************************/
bool d_tests_sa_dio_perror(struct d_test_counter* _counter);
bool d_tests_sa_dio_feof(struct d_test_counter* _counter);
bool d_tests_sa_dio_ferror(struct d_test_counter* _counter);
bool d_tests_sa_dio_clearerr(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_dio_error_handling_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_dio_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_DIO_SA_
