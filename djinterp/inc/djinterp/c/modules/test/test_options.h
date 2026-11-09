/*******************************************************************************
* djinterp [c/test]                                               test_options.h
*
* The C test framework's option set: packed message and settings flags, and a
* map of overrides for the registry's config keys.
*   The flags are one uint32_t. Message flags, what is counted and what is
* printed, take bits 0-15; settings flags, what is pushed onto the stack, start
* at bit 16. The macros build, mask and test them. The functions create an
* option set, read a config key through it (its override, else the registry's
* default, test_cvar.h) and override one.
*
*
* path:      /inc/djinterp/c/modules/test/test_options.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.10.22
*                                                            revised: 2026.11.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FLAG MANIPULATION MACROS
    ------------------------
    1.  Settings shift
         1.  D_TEST_SETTINGS_SHIFT
         2.  D_TEST_SETTINGS_TO_FLAGS
         3.  D_TEST_FLAGS_TO_SETTINGS
2.  MASK DEFINITIONS
    ----------------
    1.  Bit-range masks
         1.  D_TEST_MASK_MESSAGE_FLAGS
         2.  D_TEST_MASK_SETTINGS_FLAGS
         3.  D_TEST_MASK_COUNTER_FLAGS
         4.  D_TEST_MASK_PRINT_FLAGS
         5.  D_TEST_MASK_STACK_FLAGS
3.  CONFIGURATION ENUMS AND FLAGS
    -----------------------------
    1.  Flags
         1.  DTestMessageFlag
         2.  DTestSettingsFlag
    2.  Events
         1.  DTestEvent
    3.  Aliases
         1.  D_TEST_OPTIONS_INDENT
4.  MESSAGE FLAG COMBINATIONS
    -------------------------
    1.  Counting
         1.  D_TEST_MSG_COUNT_FAIL_ALL
         2.  D_TEST_MSG_COUNT_PASS_ALL
         3.  D_TEST_MSG_COUNT_ALL
    2.  Printing
         1.  D_TEST_MSG_PRINT_FAIL_ALL
         2.  D_TEST_MSG_PRINT_PASS_ALL
         3.  D_TEST_MSG_PRINT_ALL
    3.  Everything
         1.  D_TEST_MSG_ALL
5.  CATEGORY-SPECIFIC MESSAGE COMBINATIONS
    --------------------------------------
    1.  Assertions
         1.  D_TEST_MSG_ASSERTS_FAIL_ONLY
         2.  D_TEST_MSG_ASSERTS_PASS_ONLY
         3.  D_TEST_MSG_ASSERTS_ALL
    2.  Tests
         1.  D_TEST_MSG_TESTS_FAIL_ONLY
         2.  D_TEST_MSG_TESTS_PASS_ONLY
         3.  D_TEST_MSG_TESTS_ALL
    3.  Blocks
         1.  D_TEST_MSG_BLOCKS_FAIL_ONLY
         2.  D_TEST_MSG_BLOCKS_PASS_ONLY
         3.  D_TEST_MSG_BLOCKS_ALL
    4.  Modules
         1.  D_TEST_MSG_MODULES_FAIL_ONLY
         2.  D_TEST_MSG_MODULES_PASS_ONLY
         3.  D_TEST_MSG_MODULES_ALL
6.  SETTINGS FLAG COMBINATIONS
    --------------------------
    1.  Stack push
         1.  D_TEST_SETTINGS_STACK_PUSH_ALL
7.  MODE DEFINITIONS
    ----------------
    1.  Modes
         1.  D_TEST_MODE_SILENT
         2.  D_TEST_MODE_MINIMAL
         3.  D_TEST_MODE_NORMAL
         4.  D_TEST_MODE_VERBOSE
8.  CONFIGURATION PRESETS
    ---------------------
    1.  Presets
         1.  D_TEST_OPTIONS_PRESET_SILENT
         2.  D_TEST_OPTIONS_PRESET_MINIMAL
         3.  D_TEST_OPTIONS_PRESET_NORMAL
         4.  D_TEST_OPTIONS_PRESET_VERBOSE
9.  UTILITY MACROS
    --------------
    1.  Flag checks
         1.  D_TEST_HAS_FLAG
         2.  D_TEST_HAS_ANY_FLAG
    2.  Flag extraction
         1.  D_TEST_GET_MESSAGE_FLAGS
         2.  D_TEST_GET_SETTINGS_FLAGS
    3.  Mode checks
         1.  D_TEST_IS_SILENT
         2.  D_TEST_IS_VERBOSE
         3.  D_TEST_IS_MODE
10. SEMANTIC CHECKS - MESSAGE FLAGS
    -------------------------------
    1.  Counting
         1.  D_TEST_SHOULD_COUNT_FAILURES
         2.  D_TEST_SHOULD_COUNT_PASSES
         3.  D_TEST_SHOULD_COUNT_ASSERTS_FAIL
         4.  D_TEST_SHOULD_COUNT_ASSERTS_PASS
         5.  D_TEST_SHOULD_COUNT_TESTS_FAIL
         6.  D_TEST_SHOULD_COUNT_TESTS_PASS
         7.  D_TEST_SHOULD_COUNT_BLOCKS_FAIL
         8.  D_TEST_SHOULD_COUNT_BLOCKS_PASS
         9.  D_TEST_SHOULD_COUNT_MODULES_FAIL
         10. D_TEST_SHOULD_COUNT_MODULES_PASS
    2.  Printing
         1.  D_TEST_SHOULD_PRINT_FAILURES
         2.  D_TEST_SHOULD_PRINT_PASSES
         3.  D_TEST_SHOULD_PRINT_ASSERTS_FAIL
         4.  D_TEST_SHOULD_PRINT_ASSERTS_PASS
         5.  D_TEST_SHOULD_PRINT_TESTS_FAIL
         6.  D_TEST_SHOULD_PRINT_TESTS_PASS
         7.  D_TEST_SHOULD_PRINT_BLOCKS_FAIL
         8.  D_TEST_SHOULD_PRINT_BLOCKS_PASS
         9.  D_TEST_SHOULD_PRINT_MODULES_FAIL
         10. D_TEST_SHOULD_PRINT_MODULES_PASS
11. SEMANTIC CHECKS - SETTINGS FLAGS
    --------------------------------
    1.  Stack push
         1.  D_TEST_SHOULD_PUSH_FAILURES
         2.  D_TEST_SHOULD_PUSH_PASSES
         3.  D_TEST_SHOULD_PUSH_WARNINGS
         4.  D_TEST_SHOULD_PUSH_INFO
12. CORE STRUCTURES
    ---------------
    1.  Options
         1.  dj_test_options
13. FUNCTION DECLARATIONS
    ---------------------
    1.  Creation and destruction
    2.  Getters
    3.  Setters
    4.  Key lookup
*/

#ifndef DJINTERP_C_TEST_OPTIONS_H
#define DJINTERP_C_TEST_OPTIONS_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"                    // framework root
#include "../../container/map/min_enum_map.h"  // dj_min_enum_map
#include "./test_cvar.h"                       // DTestConfigKey, the registry
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int32_t, uint32_t


//==============================================================================
// 1.  FLAG MANIPULATION MACROS
//==============================================================================
// Settings flags are declared from bit 0 and stored from bit 16.


// 1.1    Settings shift
//------------------------------------------------------------------------------
// 1.1.1
// D_TEST_SETTINGS_SHIFT
//   constant: the number of bits settings flags are shifted by when stored.
#define D_TEST_SETTINGS_SHIFT 16

// 1.1.2
// D_TEST_SETTINGS_TO_FLAGS
//   macro: shifts settings flags from bits 0-3 to bits 16-19.
#define D_TEST_SETTINGS_TO_FLAGS(_settings)                                 \
    ( (uint32_t)(_settings) << D_TEST_SETTINGS_SHIFT )

// 1.1.3
// D_TEST_FLAGS_TO_SETTINGS
//   macro: extracts settings flags from bits 16-19 and shifts them to bits
// 0-3.
#define D_TEST_FLAGS_TO_SETTINGS(_flags)                                    \
    ( ((_flags) & D_TEST_MASK_SETTINGS_FLAGS) >> D_TEST_SETTINGS_SHIFT )


//==============================================================================
// 2.  MASK DEFINITIONS
//==============================================================================
// Each mask extracts one bit range of the packed flags.


// 2.1    Bit-range masks
//------------------------------------------------------------------------------
// 2.1.1
// D_TEST_MASK_MESSAGE_FLAGS
//   constant: extracts only the message flags (bits 0-15).
#define D_TEST_MASK_MESSAGE_FLAGS   0x0000FFFF

// 2.1.2
// D_TEST_MASK_SETTINGS_FLAGS
//   constant: extracts only the settings flags (bits 16-31).
#define D_TEST_MASK_SETTINGS_FLAGS  0xFFFF0000

// 2.1.3
// D_TEST_MASK_COUNTER_FLAGS
//   constant: extracts only the counter flags (bits 0-7).
#define D_TEST_MASK_COUNTER_FLAGS   0x000000FF

// 2.1.4
// D_TEST_MASK_PRINT_FLAGS
//   constant: extracts only the print flags (bits 8-15).
#define D_TEST_MASK_PRINT_FLAGS     0x0000FF00

// 2.1.5
// D_TEST_MASK_STACK_FLAGS
//   constant: extracts only the stack push flags (bits 16-19).
#define D_TEST_MASK_STACK_FLAGS     0x000F0000


//==============================================================================
// 3.  CONFIGURATION ENUMS AND FLAGS
//==============================================================================


// 3.1    Flags
//------------------------------------------------------------------------------
// 3.1.1
// DTestMessageFlag
//   enum: message output flags, for counting and printing.
// All flags pack into a single uint32_t:
//   bits 0-15  (0x0000FFFF): message flags (count and print)
//   bits 16-31 (0xFFFF0000): settings flags (stack push, etc.)
// Message flags layout (bits 0-15):
//   bits 0-7   (0x000000FF): counter flags
//   bits 8-15  (0x0000FF00): print flags
// Settings flags layout (bits 16-31):
//   bits 16-19 (0x000F0000): stack push flags
//   bits 20-31 (0xFFF00000): reserved for future use
enum DTestMessageFlag
{
    // counting flags (bits 0-7)
    D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL = 0x01 << 0,  // bit 0
    D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS = 0x02 << 0,  // bit 1
    D_TEST_MSG_FLAG_COUNT_TESTS_FAIL   = 0x04 << 0,  // bit 2
    D_TEST_MSG_FLAG_COUNT_TESTS_PASS   = 0x08 << 0,  // bit 3
    D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL  = 0x10 << 0,  // bit 4
    D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS  = 0x20 << 0,  // bit 5
    D_TEST_MSG_FLAG_COUNT_MODULES_FAIL = 0x40 << 0,  // bit 6
    D_TEST_MSG_FLAG_COUNT_MODULES_PASS = 0x80 << 0,  // bit 7
    // print flags (bits 8-15)
    D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL = 0x01 << 8,  // bit 8
    D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS = 0x02 << 8,  // bit 9
    D_TEST_MSG_FLAG_PRINT_TESTS_FAIL   = 0x04 << 8,  // bit 10
    D_TEST_MSG_FLAG_PRINT_TESTS_PASS   = 0x08 << 8,  // bit 11
    D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL  = 0x10 << 8,  // bit 12
    D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS  = 0x20 << 8,  // bit 13
    D_TEST_MSG_FLAG_PRINT_MODULES_FAIL = 0x40 << 8,  // bit 14
    D_TEST_MSG_FLAG_PRINT_MODULES_PASS = 0x80 << 8   // bit 15
};

// 3.1.2
// DTestSettingsFlag
//   enum: behaviour settings flags, declared from bit 0 and stored in bits
// 16-19 through D_TEST_SETTINGS_TO_FLAGS.
enum DTestSettingsFlag
{
    D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL    = 0x01,
    D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS    = 0x02,
    D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING = 0x04,
    D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO    = 0x08
    // bits 20-31 reserved for future settings flags
};

// 3.2    Events
//------------------------------------------------------------------------------
// 3.2.1
// DTestEvent
//   enum: test lifecycle events, for callbacks and logging.
enum DTestEvent
{
    D_TEST_EVENT_SETUP,             // test setup phase
    D_TEST_EVENT_START,             // test execution started
    D_TEST_EVENT_SUCCESS,           // test completed successfully
    D_TEST_EVENT_FAILURE,           // test failed
    D_TEST_EVENT_END,               // test execution ended
    D_TEST_EVENT_TEAR_DOWN          // test cleanup phase
};

// 3.3    Aliases
//------------------------------------------------------------------------------
// 3.3.1
// D_TEST_OPTIONS_INDENT
//   alias: the legacy name of D_TEST_OPTIONS_INDENT_STR.
#define D_TEST_OPTIONS_INDENT D_TEST_OPTIONS_INDENT_STR


//==============================================================================
// 4.  MESSAGE FLAG COMBINATIONS
//==============================================================================


// 4.1    Counting
//------------------------------------------------------------------------------
// 4.1.1
// D_TEST_MSG_COUNT_FAIL_ALL
//   constant: counts all failures (every counter fail flag).
#define D_TEST_MSG_COUNT_FAIL_ALL                                           \
    (D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |                                   \
     D_TEST_MSG_FLAG_COUNT_TESTS_FAIL   |                                   \
     D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL  |                                   \
     D_TEST_MSG_FLAG_COUNT_MODULES_FAIL)

// 4.1.2
// D_TEST_MSG_COUNT_PASS_ALL
//   constant: counts all passes (every counter pass flag).
#define D_TEST_MSG_COUNT_PASS_ALL                                           \
    (D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS |                                   \
     D_TEST_MSG_FLAG_COUNT_TESTS_PASS   |                                   \
     D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS  |                                   \
     D_TEST_MSG_FLAG_COUNT_MODULES_PASS)

// 4.1.3
// D_TEST_MSG_COUNT_ALL
//   constant: counts everything (every counter flag).
#define D_TEST_MSG_COUNT_ALL                                                \
    (D_TEST_MSG_COUNT_FAIL_ALL |                                            \
     D_TEST_MSG_COUNT_PASS_ALL)

// 4.2    Printing
//------------------------------------------------------------------------------
// 4.2.1
// D_TEST_MSG_PRINT_FAIL_ALL
//   constant: prints all failures (every print fail flag).
#define D_TEST_MSG_PRINT_FAIL_ALL                                           \
    (D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL |                                   \
     D_TEST_MSG_FLAG_PRINT_TESTS_FAIL   |                                   \
     D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL  |                                   \
     D_TEST_MSG_FLAG_PRINT_MODULES_FAIL)

// 4.2.2
// D_TEST_MSG_PRINT_PASS_ALL
//   constant: prints all passes (every print pass flag).
#define D_TEST_MSG_PRINT_PASS_ALL                                           \
    (D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS |                                   \
     D_TEST_MSG_FLAG_PRINT_TESTS_PASS   |                                   \
     D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS  |                                   \
     D_TEST_MSG_FLAG_PRINT_MODULES_PASS)

// 4.2.3
// D_TEST_MSG_PRINT_ALL
//   constant: prints everything (every print flag).
#define D_TEST_MSG_PRINT_ALL                                                \
    (D_TEST_MSG_PRINT_FAIL_ALL |                                            \
     D_TEST_MSG_PRINT_PASS_ALL)

// 4.3    Everything
//------------------------------------------------------------------------------
// 4.3.1
// D_TEST_MSG_ALL
//   constant: every message flag, counter and print.
#define D_TEST_MSG_ALL                                                      \
    (D_TEST_MSG_COUNT_ALL |                                                 \
     D_TEST_MSG_PRINT_ALL)


//==============================================================================
// 5.  CATEGORY-SPECIFIC MESSAGE COMBINATIONS
//==============================================================================
// Each category's failures, passes or both, counted and printed.


// 5.1    Assertions
//------------------------------------------------------------------------------
// 5.1.1
// D_TEST_MSG_ASSERTS_FAIL_ONLY
//   constant: counts and prints failed assertions.
#define D_TEST_MSG_ASSERTS_FAIL_ONLY                                        \
    (D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |                                   \
     D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL)

// 5.1.2
// D_TEST_MSG_ASSERTS_PASS_ONLY
//   constant: counts and prints passed assertions.
#define D_TEST_MSG_ASSERTS_PASS_ONLY                                        \
    (D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS |                                   \
     D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS)

// 5.1.3
// D_TEST_MSG_ASSERTS_ALL
//   constant: counts and prints every assertion.
#define D_TEST_MSG_ASSERTS_ALL                                              \
    (D_TEST_MSG_ASSERTS_FAIL_ONLY |                                         \
     D_TEST_MSG_ASSERTS_PASS_ONLY)

// 5.2    Tests
//------------------------------------------------------------------------------
// 5.2.1
// D_TEST_MSG_TESTS_FAIL_ONLY
//   constant: counts and prints failed tests.
#define D_TEST_MSG_TESTS_FAIL_ONLY                                          \
    (D_TEST_MSG_FLAG_COUNT_TESTS_FAIL |                                     \
     D_TEST_MSG_FLAG_PRINT_TESTS_FAIL)

// 5.2.2
// D_TEST_MSG_TESTS_PASS_ONLY
//   constant: counts and prints passed tests.
#define D_TEST_MSG_TESTS_PASS_ONLY                                          \
    (D_TEST_MSG_FLAG_COUNT_TESTS_PASS |                                     \
     D_TEST_MSG_FLAG_PRINT_TESTS_PASS)

// 5.2.3
// D_TEST_MSG_TESTS_ALL
//   constant: counts and prints every test.
#define D_TEST_MSG_TESTS_ALL                                                \
    (D_TEST_MSG_TESTS_FAIL_ONLY |                                           \
     D_TEST_MSG_TESTS_PASS_ONLY)

// 5.3    Blocks
//------------------------------------------------------------------------------
// 5.3.1
// D_TEST_MSG_BLOCKS_FAIL_ONLY
//   constant: counts and prints failed test blocks.
#define D_TEST_MSG_BLOCKS_FAIL_ONLY                                         \
    (D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL |                                    \
     D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL)

// 5.3.2
// D_TEST_MSG_BLOCKS_PASS_ONLY
//   constant: counts and prints passed test blocks.
#define D_TEST_MSG_BLOCKS_PASS_ONLY                                         \
    (D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS |                                    \
     D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS)

// 5.3.3
// D_TEST_MSG_BLOCKS_ALL
//   constant: counts and prints every test block.
#define D_TEST_MSG_BLOCKS_ALL                                               \
    (D_TEST_MSG_BLOCKS_FAIL_ONLY |                                          \
     D_TEST_MSG_BLOCKS_PASS_ONLY)

// 5.4    Modules
//------------------------------------------------------------------------------
// 5.4.1
// D_TEST_MSG_MODULES_FAIL_ONLY
//   constant: counts and prints failed modules.
#define D_TEST_MSG_MODULES_FAIL_ONLY                                        \
    (D_TEST_MSG_FLAG_COUNT_MODULES_FAIL |                                   \
     D_TEST_MSG_FLAG_PRINT_MODULES_FAIL)

// 5.4.2
// D_TEST_MSG_MODULES_PASS_ONLY
//   constant: counts and prints passed modules.
#define D_TEST_MSG_MODULES_PASS_ONLY                                        \
    (D_TEST_MSG_FLAG_COUNT_MODULES_PASS |                                   \
     D_TEST_MSG_FLAG_PRINT_MODULES_PASS)

// 5.4.3
// D_TEST_MSG_MODULES_ALL
//   constant: counts and prints every module.
#define D_TEST_MSG_MODULES_ALL                                              \
    (D_TEST_MSG_MODULES_FAIL_ONLY |                                         \
     D_TEST_MSG_MODULES_PASS_ONLY)


//==============================================================================
// 6.  SETTINGS FLAG COMBINATIONS
//==============================================================================


// 6.1    Stack push
//------------------------------------------------------------------------------
// 6.1.1
// D_TEST_SETTINGS_STACK_PUSH_ALL
//   constant: pushes every message type onto the stack, as stored flags.
#define D_TEST_SETTINGS_STACK_PUSH_ALL                                      \
    D_TEST_SETTINGS_TO_FLAGS(                                               \
        D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL    |                           \
        D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS    |                           \
        D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING |                           \
        D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO)


//==============================================================================
// 7.  MODE DEFINITIONS
//==============================================================================
// Predefined combinations of the message flags.


// 7.1    Modes
//------------------------------------------------------------------------------
// 7.1.1
// D_TEST_MODE_SILENT
//   constant: no output and no counting.
#define D_TEST_MODE_SILENT  0x00000000

// 7.1.2
// D_TEST_MODE_MINIMAL
//   constant: counts failures, and prints failed tests only.
#define D_TEST_MODE_MINIMAL                                                 \
    (D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |                                   \
     D_TEST_MSG_FLAG_COUNT_TESTS_FAIL   |                                   \
     D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL  |                                   \
     D_TEST_MSG_FLAG_COUNT_MODULES_FAIL |                                   \
     D_TEST_MSG_FLAG_PRINT_TESTS_FAIL)

// 7.1.3
// D_TEST_MODE_NORMAL
//   constant: counts everything, and prints failures.
#define D_TEST_MODE_NORMAL                                                  \
    (D_TEST_MSG_COUNT_ALL |                                                 \
     D_TEST_MSG_PRINT_FAIL_ALL)

// 7.1.4
// D_TEST_MODE_VERBOSE
//   constant: every message flag: counts and prints everything.
#define D_TEST_MODE_VERBOSE  D_TEST_MSG_ALL


//==============================================================================
// 8.  CONFIGURATION PRESETS
//==============================================================================
// The presets d_test_options_new_preset takes: the modes, by another name.


// 8.1    Presets
//------------------------------------------------------------------------------
// 8.1.1
// D_TEST_OPTIONS_PRESET_SILENT
//   constant: D_TEST_MODE_SILENT.
#define D_TEST_OPTIONS_PRESET_SILENT   D_TEST_MODE_SILENT

// 8.1.2
// D_TEST_OPTIONS_PRESET_MINIMAL
//   constant: D_TEST_MODE_MINIMAL.
#define D_TEST_OPTIONS_PRESET_MINIMAL  D_TEST_MODE_MINIMAL

// 8.1.3
// D_TEST_OPTIONS_PRESET_NORMAL
//   constant: D_TEST_MODE_NORMAL.
#define D_TEST_OPTIONS_PRESET_NORMAL   D_TEST_MODE_NORMAL

// 8.1.4
// D_TEST_OPTIONS_PRESET_VERBOSE
//   constant: D_TEST_MODE_VERBOSE.
#define D_TEST_OPTIONS_PRESET_VERBOSE  D_TEST_MODE_VERBOSE


//==============================================================================
// 9.  UTILITY MACROS
//==============================================================================
// Flag manipulation and checking, on a pointer to a dj_test_options.


// 9.1    Flag checks
//------------------------------------------------------------------------------
// 9.1.1
// D_TEST_HAS_FLAG
//   macro: whether a flag is set; every bit of `flag_mask` must be.
#define D_TEST_HAS_FLAG(_test_options, flag_mask)                           \
    (((_test_options)->flags & (flag_mask)) == (flag_mask))

// 9.1.2
// D_TEST_HAS_ANY_FLAG
//   macro: whether any of the flags is set.
#define D_TEST_HAS_ANY_FLAG(_test_options, flag_mask)                       \
    (((_test_options)->flags & (flag_mask)) != 0)

// 9.2    Flag extraction
//------------------------------------------------------------------------------
// 9.2.1
// D_TEST_GET_MESSAGE_FLAGS
//   macro: extracts just the message flags (bits 0-15).
#define D_TEST_GET_MESSAGE_FLAGS(_test_options)                             \
    ((_test_options)->flags & D_TEST_MASK_MESSAGE_FLAGS)

// 9.2.2
// D_TEST_GET_SETTINGS_FLAGS
//   macro: extracts just the settings flags (bits 16-31).
#define D_TEST_GET_SETTINGS_FLAGS(_test_options)                            \
    ((_test_options)->flags & D_TEST_MASK_SETTINGS_FLAGS)

// 9.3    Mode checks
//------------------------------------------------------------------------------
// 9.3.1
// D_TEST_IS_SILENT
//   macro: whether the options are in silent mode (no message flag set).
#define D_TEST_IS_SILENT(_test_options)                                     \
    (D_TEST_GET_MESSAGE_FLAGS(_test_options) == 0)

// 9.3.2
// D_TEST_IS_VERBOSE
//   macro: whether the options are in verbose mode (every message flag set).
#define D_TEST_IS_VERBOSE(_test_options)                                    \
    ( (D_TEST_GET_MESSAGE_FLAGS(_test_options) & D_TEST_MSG_ALL)            \
      == D_TEST_MSG_ALL)

// 9.3.3
// D_TEST_IS_MODE
//   macro: whether the message flags are exactly `mode`.
#define D_TEST_IS_MODE(_test_options, mode)                                 \
    (D_TEST_GET_MESSAGE_FLAGS(_test_options) == (mode))


//==============================================================================
// 10. SEMANTIC CHECKS - MESSAGE FLAGS
//==============================================================================
// What a set of options counts and prints, by name rather than by flag.


// 10.1   Counting
//------------------------------------------------------------------------------
// 10.1.1
// D_TEST_SHOULD_COUNT_FAILURES
//   macro: whether any failure is counted.
#define D_TEST_SHOULD_COUNT_FAILURES(_options)                              \
    D_TEST_HAS_ANY_FLAG(_options, D_TEST_MSG_COUNT_FAIL_ALL)

// 10.1.2
// D_TEST_SHOULD_COUNT_PASSES
//   macro: whether any pass is counted.
#define D_TEST_SHOULD_COUNT_PASSES(_options)                                \
    D_TEST_HAS_ANY_FLAG(_options, D_TEST_MSG_COUNT_PASS_ALL)

// 10.1.3
// D_TEST_SHOULD_COUNT_ASSERTS_FAIL
//   macro: whether failed assertions are counted.
#define D_TEST_SHOULD_COUNT_ASSERTS_FAIL(_options)                          \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL)

// 10.1.4
// D_TEST_SHOULD_COUNT_ASSERTS_PASS
//   macro: whether passed assertions are counted.
#define D_TEST_SHOULD_COUNT_ASSERTS_PASS(_options)                          \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS)

// 10.1.5
// D_TEST_SHOULD_COUNT_TESTS_FAIL
//   macro: whether failed tests are counted.
#define D_TEST_SHOULD_COUNT_TESTS_FAIL(_options)                            \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_COUNT_TESTS_FAIL)

// 10.1.6
// D_TEST_SHOULD_COUNT_TESTS_PASS
//   macro: whether passed tests are counted.
#define D_TEST_SHOULD_COUNT_TESTS_PASS(_options)                            \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_COUNT_TESTS_PASS)

// 10.1.7
// D_TEST_SHOULD_COUNT_BLOCKS_FAIL
//   macro: whether failed test blocks are counted.
#define D_TEST_SHOULD_COUNT_BLOCKS_FAIL(_options)                           \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL)

// 10.1.8
// D_TEST_SHOULD_COUNT_BLOCKS_PASS
//   macro: whether passed test blocks are counted.
#define D_TEST_SHOULD_COUNT_BLOCKS_PASS(_options)                           \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS)

// 10.1.9
// D_TEST_SHOULD_COUNT_MODULES_FAIL
//   macro: whether failed modules are counted.
#define D_TEST_SHOULD_COUNT_MODULES_FAIL(_options)                          \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_COUNT_MODULES_FAIL)

// 10.1.10
// D_TEST_SHOULD_COUNT_MODULES_PASS
//   macro: whether passed modules are counted.
#define D_TEST_SHOULD_COUNT_MODULES_PASS(_options)                          \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_COUNT_MODULES_PASS)

// 10.2   Printing
//------------------------------------------------------------------------------
// 10.2.1
// D_TEST_SHOULD_PRINT_FAILURES
//   macro: whether any failure is printed.
#define D_TEST_SHOULD_PRINT_FAILURES(_options)                              \
    D_TEST_HAS_ANY_FLAG(_options, D_TEST_MSG_PRINT_FAIL_ALL)

// 10.2.2
// D_TEST_SHOULD_PRINT_PASSES
//   macro: whether any pass is printed.
#define D_TEST_SHOULD_PRINT_PASSES(_options)                                \
    D_TEST_HAS_ANY_FLAG(_options, D_TEST_MSG_PRINT_PASS_ALL)

// 10.2.3
// D_TEST_SHOULD_PRINT_ASSERTS_FAIL
//   macro: whether failed assertions are printed.
#define D_TEST_SHOULD_PRINT_ASSERTS_FAIL(_options)                          \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL)

// 10.2.4
// D_TEST_SHOULD_PRINT_ASSERTS_PASS
//   macro: whether passed assertions are printed.
#define D_TEST_SHOULD_PRINT_ASSERTS_PASS(_options)                          \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS)

// 10.2.5
// D_TEST_SHOULD_PRINT_TESTS_FAIL
//   macro: whether failed tests are printed.
#define D_TEST_SHOULD_PRINT_TESTS_FAIL(_options)                            \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_PRINT_TESTS_FAIL)

// 10.2.6
// D_TEST_SHOULD_PRINT_TESTS_PASS
//   macro: whether passed tests are printed.
#define D_TEST_SHOULD_PRINT_TESTS_PASS(_options)                            \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_PRINT_TESTS_PASS)

// 10.2.7
// D_TEST_SHOULD_PRINT_BLOCKS_FAIL
//   macro: whether failed test blocks are printed.
#define D_TEST_SHOULD_PRINT_BLOCKS_FAIL(_options)                           \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL)

// 10.2.8
// D_TEST_SHOULD_PRINT_BLOCKS_PASS
//   macro: whether passed test blocks are printed.
#define D_TEST_SHOULD_PRINT_BLOCKS_PASS(_options)                           \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS)

// 10.2.9
// D_TEST_SHOULD_PRINT_MODULES_FAIL
//   macro: whether failed modules are printed.
#define D_TEST_SHOULD_PRINT_MODULES_FAIL(_options)                          \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_PRINT_MODULES_FAIL)

// 10.2.10
// D_TEST_SHOULD_PRINT_MODULES_PASS
//   macro: whether passed modules are printed.
#define D_TEST_SHOULD_PRINT_MODULES_PASS(_options)                          \
    D_TEST_HAS_FLAG(_options, D_TEST_MSG_FLAG_PRINT_MODULES_PASS)


//==============================================================================
// 11. SEMANTIC CHECKS - SETTINGS FLAGS
//==============================================================================
// What a set of options pushes onto the stack, by name rather than by flag.


// 11.1   Stack push
//------------------------------------------------------------------------------
// 11.1.1
// D_TEST_SHOULD_PUSH_FAILURES
//   macro: whether failures are pushed onto the stack.
#define D_TEST_SHOULD_PUSH_FAILURES(_test_config)                           \
    ( D_TEST_HAS_FLAG(_test_config,                                         \
                      D_TEST_SETTINGS_TO_FLAGS(                             \
                          D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL)) )

// 11.1.2
// D_TEST_SHOULD_PUSH_PASSES
//   macro: whether passes are pushed onto the stack.
#define D_TEST_SHOULD_PUSH_PASSES(_test_config)                             \
    ( D_TEST_HAS_FLAG(_test_config,                                         \
                      D_TEST_SETTINGS_TO_FLAGS(                             \
                          D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS)) )

// 11.1.3
// D_TEST_SHOULD_PUSH_WARNINGS
//   macro: whether warnings are pushed onto the stack.
#define D_TEST_SHOULD_PUSH_WARNINGS(_test_config)                           \
    ( D_TEST_HAS_FLAG(_test_config,                                         \
                      D_TEST_SETTINGS_TO_FLAGS(                             \
                          D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING)) )

// 11.1.4
// D_TEST_SHOULD_PUSH_INFO
//   macro: whether information messages are pushed onto the stack.
#define D_TEST_SHOULD_PUSH_INFO(_test_config)                               \
    ( D_TEST_HAS_FLAG(_test_config,                                         \
                      D_TEST_SETTINGS_TO_FLAGS(                             \
                          D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO)) )


//==============================================================================
// 12. CORE STRUCTURES
//==============================================================================


// 12.1   Options
//------------------------------------------------------------------------------
// 12.1.1
// dj_test_options
//   struct: an option set: the packed flags, the overrides of the registry's
// config keys, keyed by DTestConfigKey, and stage hooks.
struct dj_test_options
{
    uint32_t                flags;        // packed message and settings flags
    struct dj_min_enum_map* settings;     // config key overrides
    struct dj_min_enum_map* stage_hooks;  // stage hooks
};


//==============================================================================
// 13. FUNCTION DECLARATIONS
//==============================================================================


// 13.1   Creation and destruction
//------------------------------------------------------------------------------
/**
 * @brief Creates an option set with the flags given and no overrides.
 *
 * @param[in] _flags  the packed message and settings flags.
 * @return the option set, or `NULL` if allocation failed.
 * @post The option set belongs to the caller, who releases it with
 *       d_test_options_free.
 */
struct dj_test_options*
     d_test_options_new(uint32_t _flags);

/**
 * @brief Creates an option set from a preset; d_test_options_new by another
 *        name.
 *
 * @param[in] _preset  a D_TEST_OPTIONS_PRESET_*, or any packed flags.
 * @return the option set, or `NULL` if allocation failed.
 * @post The option set belongs to the caller, who releases it with
 *       d_test_options_free.
 */
struct dj_test_options*
     d_test_options_new_preset(uint32_t _preset);

/**
 * @brief Copies an option set: its flags, its overrides and its stage hooks.
 *
 * @param[in] _other  the option set to copy; may be `NULL`.
 * @return the copy, or `NULL` if `_other` is `NULL` or allocation failed.
 * @post The copy belongs to the caller, who releases it with
 *       d_test_options_free.
 */
struct dj_test_options*
     d_test_options_new_copy(const struct dj_test_options* _other);

/**
 * @brief Releases an option set, its overrides and its stage hooks.
 *
 * @param[in] _config  the option set; may be `NULL`.
 * @post `_config` is invalid.
 */
void d_test_options_free(struct dj_test_options* _config);

// 13.2   Getters
//------------------------------------------------------------------------------
// each returns `_key`'s override in `_config`, else the registry's default; a
// NULL `_config` reads the defaults. A key that is not a config key reads 0,
// false or NULL, as does one of a type the getter does not take: get_size_t
// and get_uint32 take size_t, uint16_t and uint32_t keys, get_int32 and
// get_string their own type's only, and get_bool and get_ptr any key.
bool        d_test_options_get_bool(const struct dj_test_options* _config,
                                    uint32_t                      _key);
size_t      d_test_options_get_size_t(const struct dj_test_options* _config,
                                      uint32_t                      _key);
int32_t     d_test_options_get_int32(const struct dj_test_options* _config,
                                     uint32_t                      _key);
uint32_t    d_test_options_get_uint32(const struct dj_test_options* _config,
                                      uint32_t                      _key);
const char* d_test_options_get_string(const struct dj_test_options* _config,
                                      uint32_t                      _key);
void*       d_test_options_get_ptr(const struct dj_test_options* _config,
                                   uint32_t                      _key);

// 13.3   Setters
//------------------------------------------------------------------------------
/**
 * @brief Overrides a bool config key.
 *
 * @param[in,out] _config  the option set.
 * @param[in]     _key     the config key, a DTestConfigKey.
 * @param[in]     _value   the value.
 * @return `true` if set; `false` if `_config` is `NULL` or `_key` is not a
 *         config key of type bool.
 */
bool d_test_options_set_bool(struct dj_test_options* _config,
                             uint32_t                _key,
                             bool                    _value);

/**
 * @brief Overrides a size_t, uint16_t or uint32_t config key.
 *
 * @param[in,out] _config  the option set.
 * @param[in]     _key     the config key, a DTestConfigKey.
 * @param[in]     _value   the value.
 * @return `true` if set; `false` if `_config` is `NULL` or `_key` is not a
 *         config key of one of those types.
 */
bool d_test_options_set_size_t(struct dj_test_options* _config,
                               uint32_t                _key,
                               size_t                  _value);

/**
 * @brief Overrides an int32_t config key.
 *
 * @param[in,out] _config  the option set.
 * @param[in]     _key     the config key, a DTestConfigKey.
 * @param[in]     _value   the value.
 * @return `true` if set; `false` if `_config` is `NULL` or `_key` is not a
 *         config key of type int32_t.
 */
bool d_test_options_set_int32(struct dj_test_options* _config,
                              uint32_t                _key,
                              int32_t                 _value);

/**
 * @brief Overrides a uint32_t, uint16_t or size_t config key.
 *
 * @note D_TEST_OPTIONS_MESSAGE_FLAGS is not an override: it sets
 *       `_config->flags`.
 *
 * @param[in,out] _config  the option set.
 * @param[in]     _key     the config key, a DTestConfigKey.
 * @param[in]     _value   the value.
 * @return `true` if set; `false` if `_config` is `NULL` or `_key` is not a
 *         config key of one of those types.
 */
bool d_test_options_set_uint32(struct dj_test_options* _config,
                               uint32_t                _key,
                               uint32_t                _value);

/**
 * @brief Overrides a string config key.
 *
 * @param[in,out] _config  the option set.
 * @param[in]     _key     the config key, a DTestConfigKey.
 * @param[in]     _value   the string; not copied, so it must outlive its use.
 * @return `true` if set; `false` if `_config` is `NULL` or `_key` is not a
 *         config key of type string.
 */
bool d_test_options_set_string(struct dj_test_options* _config,
                               uint32_t                _key,
                               const char*             _value);

/**
 * @brief Overrides any config key with a pointer, whatever the key's type.
 *
 * @param[in,out] _config  the option set.
 * @param[in]     _key     the config key, a DTestConfigKey.
 * @param[in]     _value   the pointer; not owned by the option set.
 * @return `true` if set; `false` if `_config` is `NULL` or `_key` is not a
 *         config key.
 */
bool d_test_options_set_ptr(struct dj_test_options* _config,
                            uint32_t                _key,
                            void*                   _value);

// 13.4   Key lookup
//------------------------------------------------------------------------------
/**
 * @brief Finds a config key by its name or an alias of it.
 *
 * @param[in] _key  the key's name; may be `NULL`.
 * @return the key, or UINT32_MAX, cast to the enum, if `_key` is `NULL`,
 *         unknown, or names a metadata entry.
 */
enum DTestConfigKey d_test_options_key_from_string(const char* _key);


#endif  // DJINTERP_C_TEST_OPTIONS_H
