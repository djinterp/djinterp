/******************************************************************************
* djinterp [test]                                                 test_stats.h
*
*   Aggregate statistics tracking for the DTest framework.
*   Provides a structure that holds counters for all test element types,
* along with depth tracking and timing information.
*
* This module provides:
* - d_test_statistics struct for aggregating counters across categories
* - Convenience macros for incrementing specific counter types
* - Utility functions for statistics manipulation
*
*
* path:      \inc\test\test_stats.h 
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.19
******************************************************************************/

#ifndef DJINTERP_TEST_STATISTICS_
#define DJINTERP_TEST_STATISTICS_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// d_test_statistics_flag
//   type: represents the type of a statistic value.
typedef uint8_t d_test_statistics_type;

// DTestStatType
//   enum: represents the type of statistic being tracked.
enum DTestStatType
{
    D_TEST_STAT_TYPE_UNKOWN = 0x00,
    D_TEST_STAT_TYPE_COUNT,
    D_TEST_STAT_TYPE_TIME
};

// DTestStatCountFlag
//   enum: flags for different count categories.
enum DTestStatCountFlag
{
    D_TEST_STAT_COUNT_FLAG_SKIPPED = 0x01,
    D_TEST_STAT_COUNT_FLAG_PASSED  = 0x02,
    D_TEST_STAT_COUNT_FLAG_FAILED  = 0x04,
    D_TEST_STAT_COUNT_FLAG_TOTAL   = 0x08
};

// d_test_stat_counter
//   struct: tracks counts for a single category of test elements (assertions,
// tests, blocks, or modules).
struct d_test_stat_counter
{
    size_t skipped;
    size_t passed;
    size_t failed;
    size_t run;
};

// d_test_statistics
//   struct: aggregates counters for all test element types plus timing and
// depth tracking information.
struct d_test_statistics
{
    struct d_test_stat_counter asserts;
    struct d_test_stat_counter test_fns;
    struct d_test_stat_counter tests;
    struct d_test_stat_counter blocks;
    struct d_test_stat_counter modules;

    // depth tracking
    size_t max_depth;
    size_t current_depth;

    // timing
    double total_time_ms;
    double start_time_ms;
    double end_time_ms;
};


/******************************************************************************
 * COUNTER INITIALIZER MACROS
 *****************************************************************************/

// D_STAT_COUNTER_INIT
//   macro: initializer for a d_test_stat_counter with all fields zeroed.
#define D_STAT_COUNTER_INIT  { 0, 0, 0, 0 }

// D_STATISTICS_INIT
//   macro: initializer for a d_test_statistics with all fields zeroed.
#define D_STATISTICS_INIT                                                    \
    {                                                                        \
        D_STAT_COUNTER_INIT, D_STAT_COUNTER_INIT, D_STAT_COUNTER_INIT,                      \
        D_STAT_COUNTER_INIT, D_STAT_COUNTER_INIT,                                      \
        0, 0, 0.0, 0.0, 0.0                                                  \
    }


/******************************************************************************
 * STANDALONE COUNTER MACROS (4 macros)
 *****************************************************************************/

// D_STAT_COUNTER_RESET
//   macro: resets a d_test_stat_counter to all zeros.
#define D_STAT_COUNTER_RESET(c)                                                   \
    memset((c), 0, sizeof(struct d_test_stat_counter))

// D_STAT_COUNTER_INC
//   macro: increments a specific field of a counter.
// Parameters:
//   c:     pointer to d_test_stat_counter
//   field: one of: skipped, passed, failed, run
#define D_STAT_COUNTER_INC(c, field)                                              \
    do { (c)->field++; } while(0)

// D_STAT_COUNTER_DEC
//   macro: decrements a specific field of a counter (with underflow protection).
// Parameters:
//   c:     pointer to d_test_stat_counter
//   field: one of: skipped, passed, failed, run
#define D_STAT_COUNTER_DEC(c, field)                                              \
    do {                                                                     \
        if ((c)->field > 0)                                                  \
        {                                                                    \
            (c)->field--;                                                    \
        }                                                                    \
    } while(0)

// D_STAT_COUNTER_SET
//   macro: sets a specific field of a counter to a value.
// Parameters:
//   c:     pointer to d_test_stat_counter
//   field: one of: skipped, passed, failed, run
//   val:   value to set
#define D_STAT_COUNTER_SET(c, field, val)                                         \
    do { (c)->field = (val); } while(0)

// D_STAT_COUNTER_GET
//   macro: gets a specific field of a counter.
// Parameters:
//   c:     pointer to d_test_stat_counter
//   field: one of: skipped, passed, failed, run
// Returns:
//   The value of the specified field.
#define D_STAT_COUNTER_GET(c, field)  ((c)->field)


/******************************************************************************
 * CONVENIENCE COUNTER MACROS (pass/fail/skip with run tracking)
 *****************************************************************************/

// D_STAT_COUNTER_INC_PASS
//   macro: increments pass and run counters.
#define D_STAT_COUNTER_INC_PASS(c)                                                \
    do { (c)->run++; (c)->passed++; } while(0)

// D_STAT_COUNTER_INC_FAIL
//   macro: increments fail and run counters.
#define D_STAT_COUNTER_INC_FAIL(c)                                                \
    do { (c)->run++; (c)->failed++; } while(0)

// D_STAT_COUNTER_INC_SKIP
//   macro: increments skip counter.
#define D_STAT_COUNTER_INC_SKIP(c)                                                \
    do { (c)->skipped++; } while(0)


/******************************************************************************
 * STATISTICS RESET MACRO
 *****************************************************************************/

// D_STATISTICS_RESET
//   macro: resets a d_test_statistics to all zeros.
#define D_STATISTICS_RESET(s)                                                \
    memset((s), 0, sizeof(struct d_test_statistics))


/******************************************************************************
 * STATISTICS COUNTER MACROS - ASSERTS (4 macros)
 *****************************************************************************/

// D_STAT_ASSERT_INC
//   macro: increments a specific field of the asserts counter.
#define D_STAT_ASSERT_INC(s, field)                                          \
    do { (s)->asserts.field++; } while(0)

// D_STAT_ASSERT_DEC
//   macro: decrements a specific field of the asserts counter.
#define D_STAT_ASSERT_DEC(s, field)                                          \
    do {                                                                     \
        if ((s)->asserts.field > 0)                                          \
        {                                                                    \
            (s)->asserts.field--;                                            \
        }                                                                    \
    } while(0)

// D_STAT_ASSERT_SET
//   macro: sets a specific field of the asserts counter.
#define D_STAT_ASSERT_SET(s, field, val)                                     \
    do { (s)->asserts.field = (val); } while(0)

// D_STAT_ASSERT_GET
//   macro: gets a specific field of the asserts counter.
#define D_STAT_ASSERT_GET(s, field)  ((s)->asserts.field)


/******************************************************************************
 * STATISTICS COUNTER MACROS - TEST_FNS (4 macros)
 *****************************************************************************/

// D_STAT_TEST_FN_INC
//   macro: increments a specific field of the test_fns counter.
#define D_STAT_TEST_FN_INC(s, field)                                         \
    do { (s)->test_fns.field++; } while(0)

// D_STAT_TEST_FN_DEC
//   macro: decrements a specific field of the test_fns counter.
#define D_STAT_TEST_FN_DEC(s, field)                                         \
    do {                                                                     \
        if ((s)->test_fns.field > 0)                                         \
        {                                                                    \
            (s)->test_fns.field--;                                           \
        }                                                                    \
    } while(0)

// D_STAT_TEST_FN_SET
//   macro: sets a specific field of the test_fns counter.
#define D_STAT_TEST_FN_SET(s, field, val)                                    \
    do { (s)->test_fns.field = (val); } while(0)

// D_STAT_TEST_FN_GET
//   macro: gets a specific field of the test_fns counter.
#define D_STAT_TEST_FN_GET(s, field)  ((s)->test_fns.field)


/******************************************************************************
 * STATISTICS COUNTER MACROS - TESTS (4 macros)
 *****************************************************************************/

// D_STAT_TEST_INC
//   macro: increments a specific field of the tests counter.
#define D_STAT_TEST_INC(s, field)                                            \
    do { (s)->tests.field++; } while(0)

// D_STAT_TEST_DEC
//   macro: decrements a specific field of the tests counter.
#define D_STAT_TEST_DEC(s, field)                                            \
    do {                                                                     \
        if ((s)->tests.field > 0)                                            \
        {                                                                    \
            (s)->tests.field--;                                              \
        }                                                                    \
    } while(0)

// D_STAT_TEST_SET
//   macro: sets a specific field of the tests counter.
#define D_STAT_TEST_SET(s, field, val)                                       \
    do { (s)->tests.field = (val); } while(0)

// D_STAT_TEST_GET
//   macro: gets a specific field of the tests counter.
#define D_STAT_TEST_GET(s, field)  ((s)->tests.field)


/******************************************************************************
 * STATISTICS COUNTER MACROS - BLOCKS (4 macros)
 *****************************************************************************/

// D_STAT_BLOCK_INC
//   macro: increments a specific field of the blocks counter.
#define D_STAT_BLOCK_INC(s, field)                                           \
    do { (s)->blocks.field++; } while(0)

// D_STAT_BLOCK_DEC
//   macro: decrements a specific field of the blocks counter.
#define D_STAT_BLOCK_DEC(s, field)                                           \
    do {                                                                     \
        if ((s)->blocks.field > 0)                                           \
        {                                                                    \
            (s)->blocks.field--;                                             \
        }                                                                    \
    } while(0)

// D_STAT_BLOCK_SET
//   macro: sets a specific field of the blocks counter.
#define D_STAT_BLOCK_SET(s, field, val)                                      \
    do { (s)->blocks.field = (val); } while(0)

// D_STAT_BLOCK_GET
//   macro: gets a specific field of the blocks counter.
#define D_STAT_BLOCK_GET(s, field)  ((s)->blocks.field)


/******************************************************************************
 * STATISTICS COUNTER MACROS - MODULES (4 macros)
 *****************************************************************************/

// D_STAT_MODULE_INC
//   macro: increments a specific field of the modules counter.
#define D_STAT_MODULE_INC(s, field)                                          \
    do { (s)->modules.field++; } while(0)

// D_STAT_MODULE_DEC
//   macro: decrements a specific field of the modules counter.
#define D_STAT_MODULE_DEC(s, field)                                          \
    do {                                                                     \
        if ((s)->modules.field > 0)                                          \
        {                                                                    \
            (s)->modules.field--;                                            \
        }                                                                    \
    } while(0)

// D_STAT_MODULE_SET
//   macro: sets a specific field of the modules counter.
#define D_STAT_MODULE_SET(s, field, val)                                     \
    do { (s)->modules.field = (val); } while(0)

// D_STAT_MODULE_GET
//   macro: gets a specific field of the modules counter.
#define D_STAT_MODULE_GET(s, field)  ((s)->modules.field)


/******************************************************************************
 * LEGACY CONVENIENCE MACROS (pass/fail/skip with run tracking)
 *****************************************************************************/

#define D_STAT_COUNTER_INC_ASSERT_PASS(s)                                         \
    do { (s)->asserts.run++;   (s)->asserts.passed++; } while(0)

#define D_STAT_COUNTER_INC_ASSERT_FAIL(s)                                         \
    do { (s)->asserts.run++;   (s)->asserts.failed++; } while(0)

#define D_STAT_COUNTER_INC_ASSERT_SKIP(s)                                         \
    do { (s)->asserts.skipped++; } while(0)

#define D_STAT_COUNTER_INC_TEST_FN_PASS(s)                                        \
    do { (s)->test_fns.run++;  (s)->test_fns.passed++; } while(0)

#define D_STAT_COUNTER_INC_TEST_FN_FAIL(s)                                        \
    do { (s)->test_fns.run++;  (s)->test_fns.failed++; } while(0)

#define D_STAT_COUNTER_INC_TEST_FN_SKIP(s)                                        \
    do { (s)->test_fns.skipped++; } while(0)

#define D_STAT_COUNTER_INC_TEST_PASS(s)                                           \
    do { (s)->tests.run++;     (s)->tests.passed++; } while(0)

#define D_STAT_COUNTER_INC_TEST_FAIL(s)                                           \
    do { (s)->tests.run++;     (s)->tests.failed++; } while(0)

#define D_STAT_COUNTER_INC_TEST_SKIP(s)                                           \
    do { (s)->tests.skipped++; } while(0)

#define D_STAT_COUNTER_INC_BLOCK_PASS(s)                                          \
    do { (s)->blocks.run++;    (s)->blocks.passed++; } while(0)

#define D_STAT_COUNTER_INC_BLOCK_FAIL(s)                                          \
    do { (s)->blocks.run++;    (s)->blocks.failed++; } while(0)

#define D_STAT_COUNTER_INC_BLOCK_SKIP(s)                                          \
    do { (s)->blocks.skipped++; } while(0)

#define D_STAT_COUNTER_INC_MODULE_PASS(s)                                         \
    do { (s)->modules.run++;   (s)->modules.passed++; } while(0)

#define D_STAT_COUNTER_INC_MODULE_FAIL(s)                                         \
    do { (s)->modules.run++;   (s)->modules.failed++; } while(0)

#define D_STAT_COUNTER_INC_MODULE_SKIP(s)                                         \
    do { (s)->modules.skipped++; } while(0)


/******************************************************************************
 * DEPTH TRACKING MACROS
 *****************************************************************************/

// D_STATISTICS_DEPTH_ENTER
//   macro: increments current depth and updates max depth if necessary.
#define D_STATISTICS_DEPTH_ENTER(s)                                          \
    do {                                                                     \
        (s)->current_depth++;                                                \
        if ((s)->current_depth > (s)->max_depth)                             \
        {                                                                    \
            (s)->max_depth = (s)->current_depth;                             \
        }                                                                    \
    } while(0)

// D_STATISTICS_DEPTH_EXIT
//   macro: decrements current depth.
#define D_STATISTICS_DEPTH_EXIT(s)                                           \
    do {                                                                     \
        if ((s)->current_depth > 0)                                          \
        {                                                                    \
            (s)->current_depth--;                                            \
        }                                                                    \
    } while(0)


/******************************************************************************
 * FUNCTION DECLARATIONS
 *****************************************************************************/

// counter utility functions
void   d_test_stat_counter_reset(struct d_test_stat_counter* _counter);
void   d_test_stat_counter_add(struct d_test_stat_counter* _dest,
                          const struct d_test_stat_counter* _src);
size_t d_test_stat_counter_total(const struct d_test_stat_counter* _counter);

// statistics utility functions
void   d_test_statistics_reset(struct d_test_statistics* _stats);
void   d_test_statistics_add(struct d_test_statistics* _dest,
                             const struct d_test_statistics* _src);
size_t d_test_statistics_total_run(const struct d_test_statistics* _stats);
size_t d_test_statistics_total_passed(const struct d_test_statistics* _stats);
size_t d_test_statistics_total_failed(const struct d_test_statistics* _stats);
size_t d_test_statistics_total_skipped(const struct d_test_statistics* _stats);
bool   d_test_statistics_all_passed(const struct d_test_statistics* _stats);

// timing functions
void   d_test_statistics_start_timer(struct d_test_statistics* _stats);
void   d_test_statistics_stop_timer(struct d_test_statistics* _stats);

// rate calculation functions
double d_test_stat_counter_pass_rate(const struct d_test_stat_counter* _counter);
double d_test_stat_counter_assert_rate(const struct d_test_statistics* _stats);


#endif // DJINTERP_TEST_STATISTICS_