/*******************************************************************************
* djinterp [test]                                                dmemory_tests.h
*
*   Unit tests for the dmemory module (cross-platform memory operations).
*   Tests cover secure memory copying, duplication, setting, and boundary
* conditions for memory manipulation functions.
*
*
* path:      /tests/djinterp/c/memory/dmemory_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_MEMORY_DMEMORY_TESTS_H
#define DJINTERP_TESTS_C_MEMORY_DMEMORY_TESTS_H 1

// std
#include <limits.h>
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"
#include "../../../../inc/djinterp/c/memory/dmemory.h"
#include "../../../../inc/djinterp/c/filesys/dfile.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


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

void d_tests_dmemory_fill_pattern(
    void*       _buffer,
    size_t      _size,
    const void* _pattern,
    size_t      _pattern_size
);

bool d_tests_dmemory_verify_pattern(
    const void*   _buffer,
    size_t        _size,
    unsigned char _pattern
);

// Note: d_tests_dmemory_compare_buffers is implemented (not d_tests_dmemory_compare)
bool d_tests_dmemory_compare_buffers(
    const void* _buf1,
    const void* _buf2,
    size_t      _size
);

// The following utility functions are not implemented but could be added if needed:
// - d_tests_dmemory_is_aligned
// - d_tests_dmemory_create_guard_buffer
// - d_tests_dmemory_check_guards


/******************************************************************************
 * MEMORY COPY TESTS
 *****************************************************************************/

bool d_tests_dmemory_memcpy(struct dj_test_suite* _suite);
bool d_tests_dmemory_memcpy_s(struct dj_test_suite* _suite);
bool d_tests_dmemory_copy_all(struct dj_test_suite* _suite);


/******************************************************************************
 * MEMORY DUPLICATION TESTS
 *****************************************************************************/

bool d_tests_dmemory_memdup_s(struct dj_test_suite* _suite);
bool d_tests_dmemory_duplication_all(struct dj_test_suite* _suite);


/******************************************************************************
 * MEMORY SET TESTS
 *****************************************************************************/

bool d_tests_dmemory_memset(struct dj_test_suite* _suite);
bool d_tests_dmemory_memset_s(struct dj_test_suite* _suite);
bool d_tests_dmemory_set_all(struct dj_test_suite* _suite);


/******************************************************************************
 * SPECIAL CONDITION TESTS
 *****************************************************************************/

bool d_tests_dmemory_null_params_all(struct dj_test_suite* _suite);
bool d_tests_dmemory_boundary_conditions_all(struct dj_test_suite* _suite);
bool d_tests_dmemory_alignment_all(struct dj_test_suite* _suite);
bool d_tests_dmemory_overlap_all(struct dj_test_suite* _suite);
bool d_tests_dmemory_performance_all(struct dj_test_suite* _suite);

/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

bool d_tests_dmemory_run_all(struct dj_test_suite* _suite);

// the entry points the runner calls
bool d_tests_dmemory_all(struct dj_test_suite* _suite);


#endif  // DJINTERP_TESTS_C_MEMORY_DMEMORY_TESTS_H
