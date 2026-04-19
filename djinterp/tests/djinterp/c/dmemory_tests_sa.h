/******************************************************************************
* djinterp [test]                                            dmemory_tests_sa.h
*
*   Unit tests for the dmemory module (cross-platform memory operations).
*   Tests cover secure memory copying, duplication, setting, and boundary
* conditions for memory manipulation functions.
*
*
* path:      \inc\c\test\dmemory_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/

#ifndef DJINTERP_DMEMORY_TESTS_STANDALONE_
#define DJINTERP_DMEMORY_TESTS_STANDALONE_ 1

#include <limits.h>
#include "..\..\inc\c\djinterp.h"
#include "..\..\inc\c\dmemory.h"
#include "..\..\inc\c\dfile.h"
#include "..\..\inc\c\test\test_standalone.h"


/******************************************************************************
 * TEST CONFIGURATION
 *****************************************************************************/

// D_TESTS_MEMORY_SMALL_SIZE
//   constant: small buffer size for basic tests.
#define D_TESTS_MEMORY_SMALL_SIZE       16

// D_TESTS_MEMORY_MEDIUM_SIZE
//   constant: medium buffer size for standard tests.
#define D_TESTS_MEMORY_MEDIUM_SIZE      256

// D_TESTS_MEMORY_LARGE_SIZE
//   constant: large buffer size for performance tests.
#define D_TESTS_MEMORY_LARGE_SIZE       4096

// D_TESTS_MEMORY_PATTERN_A
//   constant: test pattern A for memory operations.
#define D_TESTS_MEMORY_PATTERN_A        0xAA

// D_TESTS_MEMORY_PATTERN_B
//   constant: test pattern B for memory operations.
#define D_TESTS_MEMORY_PATTERN_B        0xBB

// D_TESTS_MEMORY_PATTERN_ZERO
//   constant: zero pattern for memory clearing.
#define D_TESTS_MEMORY_PATTERN_ZERO     0x00

// D_TESTS_MEMORY_PATTERN_FF
//   constant: all-ones pattern for memory tests.
#define D_TESTS_MEMORY_PATTERN_FF       0xFF

// D_TESTS_MEMORY_GUARD_VALUE
//   constant: guard value for overflow detection.
#define D_TESTS_MEMORY_GUARD_VALUE      0xDEADBEEF

// D_TESTS_MEMORY_ALIGNMENT
//   constant: memory alignment for aligned tests.
#define D_TESTS_MEMORY_ALIGNMENT         16


/******************************************************************************
 * TEST UTILITY FUNCTIONS (IMPLEMENTED)
 *****************************************************************************/

bool d_tests_dmemory_setup(void);

bool d_tests_dmemory_teardown(void);

void d_tests_dmemory_fill_pattern(void*       _buffer,
                                  size_t      _size,
                                  const void* _pattern,
                                  size_t      _pattern_size);

bool d_tests_dmemory_verify_pattern(const void*   _buffer,
                                    size_t        _size,
                                    unsigned char _pattern);

// Note: d_tests_dmemory_compare_buffers is implemented (not d_tests_dmemory_compare)
bool d_tests_dmemory_compare_buffers(const void* _buf1,
                                     const void* _buf2,
                                     size_t      _size);

// The following utility functions are not implemented but could be added if needed:
// - d_tests_dmemory_is_aligned
// - d_tests_dmemory_create_guard_buffer  
// - d_tests_dmemory_check_guards


/******************************************************************************
 * MEMORY COPY TESTS
 *****************************************************************************/

struct d_test_object* d_tests_dmemory_memcpy(void);
struct d_test_object* d_tests_dmemory_memcpy_s(void);
struct d_test_object* d_tests_dmemory_copy_all(void);


/******************************************************************************
 * MEMORY DUPLICATION TESTS
 *****************************************************************************/

struct d_test_object* d_tests_dmemory_memdup_s(void);
struct d_test_object* d_tests_dmemory_duplication_all(void);


/******************************************************************************
 * MEMORY SET TESTS
 *****************************************************************************/

struct d_test_object* d_tests_dmemory_memset(void);
struct d_test_object* d_tests_dmemory_memset_s(void);
struct d_test_object* d_tests_dmemory_set_all(void);


/******************************************************************************
 * SPECIAL CONDITION TESTS
 *****************************************************************************/

struct d_test_object* d_tests_dmemory_null_params_all(void);
struct d_test_object* d_tests_dmemory_boundary_conditions_all(void);
struct d_test_object* d_tests_dmemory_alignment_all(void);
struct d_test_object* d_tests_dmemory_overlap_all(void);
struct d_test_object* d_tests_dmemory_performance_all(void);

/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

struct d_test_object* d_tests_dmemory_run_all(void);


#endif  // DJINTERP_DMEMORY_TESTS_STANDALONE_




