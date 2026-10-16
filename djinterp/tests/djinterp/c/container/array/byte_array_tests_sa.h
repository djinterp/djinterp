/*******************************************************************************
* djinterp [test]                                          byte_array_tests_sa.h
*
*   Unit test declarations for `byte_array.h` module.
*   Provides comprehensive testing of all byte array macros including byte
*   extraction, byte expansion, 1D array builders, 2D array builders, and
*   jagged array builders.
*
*   NOTE: The byte_array macros have the following MSVC limitations:
*   - 1D arrays (D_BYTE_ARRAY*) require at least 2 elements
*   - 8-byte count macros (D_BYTE_ARRAY_8_*) skipped on MSVC (shift warnings)
*   - 2D arrays (D_BYTE_ARRAY_2D*) skipped on MSVC (preprocessor incompatibility)
*   - Jagged arrays (D_BYTE_ARRAY_JAGGED*) skipped on MSVC (preprocessor incompatibility)
*
*   The 2D and JAGGED macros use a GCC/Clang-specific preprocessor trick
*   that MSVC does not support (zero-argument macro detection via argument
*   count mismatch).
*
*
* path:      /tests/djinterp/c/container/array/byte_array_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.04
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_BYTE_ARRAY_SA_
#define DJINTERP_TESTS_BYTE_ARRAY_SA_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/container/array/byte_array.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/******************************************************************************
 * I. BYTE EXTRACTION MACRO TESTS
 *****************************************************************************/
bool d_tests_sa_byte_array_byte_0_of_1(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_bytes_of_2(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_bytes_of_4(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_bytes_of_8(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_byte_array_extraction_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. BYTE EXPANSION MACRO TESTS
 *****************************************************************************/
bool d_tests_sa_byte_array_bytes_1(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_bytes_2(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_bytes_4(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_bytes_8(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_bytes_n(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_byte_array_expansion_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. 1D ARRAY BUILDER TESTS
 *****************************************************************************/
bool d_tests_sa_byte_array_default(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_init(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_1_x(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_2_x(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_4_x(struct d_test_counter* _counter);
#if !defined(_MSC_VER)
bool d_tests_sa_byte_array_8_x(struct d_test_counter* _counter);
#endif
bool d_tests_sa_byte_array_custom(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_byte_array_1d_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. 2D ARRAY BUILDER TESTS (GCC/Clang only)
 *****************************************************************************/
#if !defined(_MSC_VER)
bool d_tests_sa_byte_array_2d_basic(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_2d_layout(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_2d_ex(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_byte_array_2d_all(struct d_test_counter* _counter);
#endif


/******************************************************************************
 * V. JAGGED ARRAY BUILDER TESTS (GCC/Clang only)
 *****************************************************************************/
#if !defined(_MSC_VER)
bool d_tests_sa_byte_array_jagged_basic(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_jagged_layout(struct d_test_counter* _counter);
bool d_tests_sa_byte_array_jagged_ex(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_byte_array_jagged_all(struct d_test_counter* _counter);
#endif


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_byte_array_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_BYTE_ARRAY_SA_
