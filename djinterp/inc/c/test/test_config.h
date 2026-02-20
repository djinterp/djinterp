/******************************************************************************
* djinterp [test]                                                test_config.h
*
*   Configuration structures and flags for the DTest testing framework.
* Provides hierarchical flag system for controlling test output, counting,
* and behavior settings.
*
* path:      \inc\test\test_config.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.22
******************************************************************************/

#ifndef DJINTERP_TEST_CONFIG_
#define DJINTERP_TEST_CONFIG_ 1

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "..\djinterp.h"
#include "..\container\map\min_enum_map.h"


/******************************************************************************
 * FLAG MANIPULATION MACROS
 *****************************************************************************/

 // D_TEST_SETTINGS_SHIFT
 //   constant: number of bits to shift settings flags into position
#define D_TEST_SETTINGS_SHIFT 16

// D_TEST_SETTINGS_TO_FLAGS
//   macro: shifts settings flags from bits 0-3 to bits 16-19
#define D_TEST_SETTINGS_TO_FLAGS(settings) \
    ((uint32_t)(settings) << D_TEST_SETTINGS_SHIFT)

// D_TEST_FLAGS_TO_SETTINGS
//   macro: extracts and shifts settings flags from bits 16-19 to bits 0-3
#define D_TEST_FLAGS_TO_SETTINGS(flags) \
    (((flags) & D_TEST_MASK_SETTINGS_FLAGS) >> D_TEST_SETTINGS_SHIFT)


/******************************************************************************
 * MASK DEFINITIONS - Extract specific bit ranges
 *****************************************************************************/

 // D_TEST_MASK_MESSAGE_FLAGS
 //   constant: extract only message flags (bits 0-15)
#define D_TEST_MASK_MESSAGE_FLAGS   0x0000FFFF

// D_TEST_MASK_SETTINGS_FLAGS
//   constant: extract only settings flags (bits 16-31)
#define D_TEST_MASK_SETTINGS_FLAGS  0xFFFF0000

// D_TEST_MASK_COUNTER_FLAGS
//   constant: extract only counter flags (bits 0-7)
#define D_TEST_MASK_COUNTER_FLAGS   0x000000FF

// D_TEST_MASK_PRINT_FLAGS
//   constant: extract only print flags (bits 8-15)
#define D_TEST_MASK_PRINT_FLAGS     0x0000FF00

// D_TEST_MASK_STACK_FLAGS
//   constant: extract only stack push flags (bits 16-19)
#define D_TEST_MASK_STACK_FLAGS     0x000F0000


/******************************************************************************
 * CONFIGURATION ENUMS AND FLAGS
 *****************************************************************************/

 // DTestMessageFlag
 //   enum: message output flags (counting and printing)
 // All flags pack into a single uint32_t:
 //   bits 0-15  (0x0000FFFF): Message flags (count/print)
 //   bits 16-31 (0xFFFF0000): Settings flags (stack push, etc.)
 // Message Flags Layout (bits 0-15):
 //   Bits 0-7   (0x000000FF): Counter flags
 //   Bits 8-15  (0x0000FF00): Print flags
 // Settings Flags Layout (bits 16-31):
 //   Bits 16-19 (0x000F0000): Stack push flags
 //   Bits 20-31 (0xFFF00000): Reserved for future use
enum DTestMessageFlag
{
    // COUNTING flags (bits 0-7)
    D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL = 0x01 << 0,  // bit 0
    D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS = 0x02 << 0,  // bit 1
    D_TEST_MSG_FLAG_COUNT_TESTS_FAIL   = 0x04 << 0,  // bit 2
    D_TEST_MSG_FLAG_COUNT_TESTS_PASS   = 0x08 << 0,  // bit 3
    D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL  = 0x10 << 0,  // bit 4
    D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS  = 0x20 << 0,  // bit 5
    D_TEST_MSG_FLAG_COUNT_MODULES_FAIL = 0x40 << 0,  // bit 6
    D_TEST_MSG_FLAG_COUNT_MODULES_PASS = 0x80 << 0,  // bit 7
    // PRINT flags (bits 8-15)
    D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL = 0x01 << 8,  // bit 8
    D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS = 0x02 << 8,  // bit 9
    D_TEST_MSG_FLAG_PRINT_TESTS_FAIL   = 0x04 << 8,  // bit 10
    D_TEST_MSG_FLAG_PRINT_TESTS_PASS   = 0x08 << 8,  // bit 11
    D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL  = 0x10 << 8,  // bit 12
    D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS  = 0x20 << 8,  // bit 13
    D_TEST_MSG_FLAG_PRINT_MODULES_FAIL = 0x40 << 8,  // bit 14
    D_TEST_MSG_FLAG_PRINT_MODULES_PASS = 0x80 << 8   // bit 15
};

// DTestSettingsFlag
//   enum: behavior settings flags - uses bits 16-31
enum DTestSettingsFlag
{
    D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL    = 0x01,
    D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS    = 0x02,
    D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING = 0x04,
    D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO    = 0x08
    // bits 20-31 reserved for future settings flags
};

// DTestConfigKey
//   enum: keys for the configuration settings map
enum DTestConfigKey
{
    D_TEST_CONFIG_ENABLED,          // bool*       - whether custom config is enabled
    D_TEST_CONFIG_INDENT_MAX_LEVEL, // uint16_t    - maximum level of indentation
    D_TEST_CONFIG_INDENT_STR,       // const char* - indentation string for output
    D_TEST_CONFIG_MAX_FAILURES,     // size_t      - maximum failures before stopping
    D_TEST_CONFIG_MESSAGE_FLAGS,    // uint32_t    - packed message/settings flags
    D_TEST_CONFIG_NAME,             // const char* - test name
    D_TEST_CONFIG_PRIORITY,         // int32_t     - test priority level
    D_TEST_CONFIG_SKIP,             // bool*       - whether test is skipped
    D_TEST_CONFIG_TIMEOUT_MS        // size_t      - test timeout in milliseconds
};

// DTestMetadataFlag
//   enum: common enum types for test tree metadata
enum DTestMetadataFlag
{
    D_TEST_METADATA_UNKNOWN = 0,
    D_TEST_METADATA_AUTHORS,            // char*  - author(s)' name(s)
    D_TEST_METADATA_CATEGORY,           // char*  - test category
    D_TEST_METADATA_CUSTOM_DATA,        // void*  - arbitrary custom data
    D_TEST_METADATA_DATE_CREATED,       // char*  - creation date
    D_TEST_METADATA_DATE_MODIFIED,      // char*  - modification date
    D_TEST_METADATA_DESCRIPTION,        // char*  - node description
    D_TEST_METADATA_DEPENDENCIES,       // char** - array of dependencies
    D_TEST_METADATA_FRAMEWORK_NAME,     // char*  - name of framework
    D_TEST_METADATA_MODULE_NAME,        // char*  - name of module
    D_TEST_METADATA_SUBMODULE_NAME,     // char*  - name of submodule
    D_TEST_METADATA_NAME,               // char*  - name of test
    D_TEST_METADATA_NOTES,              // char** - note(s)
    D_TEST_METADATA_SKIP_REASON,        // char*  - reason for skipping
    D_TEST_METADATA_TAGS,               // char** - array of tags
    D_TEST_METADATA_TIMESTAMP_FORMAT,   // char*  - timestamp
    D_TEST_METADATA_VERSION,            // void*  - version
    D_TEST_METADATA_VERSION_STRING      // char*  - version string
};

// DTestEvent
//   enum: test lifecycle events for callbacks and logging
enum DTestEvent
{
    D_TEST_EVENT_SETUP,             // test setup phase
    D_TEST_EVENT_START,             // test execution started
    D_TEST_EVENT_SUCCESS,           // test completed successfully
    D_TEST_EVENT_FAILURE,           // test failed
    D_TEST_EVENT_END,               // test execution ended
    D_TEST_EVENT_TEAR_DOWN          // test cleanup phase
};

// D_TEST_CONFIG_INDENT
//   alias: legacy alias for D_TEST_CONFIG_INDENT_STR
#define D_TEST_CONFIG_INDENT D_TEST_CONFIG_INDENT_STR


/******************************************************************************
 * MESSAGE FLAG COMBINATIONS
 *****************************************************************************/

 // D_TEST_MSG_COUNT_FAIL_ALL
 //   constant: count ALL failures (all counter fail flags)
#define D_TEST_MSG_COUNT_FAIL_ALL               \
    (D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |       \
     D_TEST_MSG_FLAG_COUNT_TESTS_FAIL   |       \
     D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL  |       \
     D_TEST_MSG_FLAG_COUNT_MODULES_FAIL)

// D_TEST_MSG_COUNT_PASS_ALL
//   constant: count all passes (all counter pass flags)
#define D_TEST_MSG_COUNT_PASS_ALL               \
    (D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS |       \
     D_TEST_MSG_FLAG_COUNT_TESTS_PASS   |       \
     D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS  |       \
     D_TEST_MSG_FLAG_COUNT_MODULES_PASS)

// D_TEST_MSG_COUNT_ALL
//   constant: count everything (all counter flags)
#define D_TEST_MSG_COUNT_ALL                    \
    (D_TEST_MSG_COUNT_FAIL_ALL |                \
     D_TEST_MSG_COUNT_PASS_ALL)

// D_TEST_MSG_PRINT_FAIL_ALL
//   constant: print all failures (all print fail flags)
#define D_TEST_MSG_PRINT_FAIL_ALL               \
    (D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL |       \
     D_TEST_MSG_FLAG_PRINT_TESTS_FAIL   |       \
     D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL  |       \
     D_TEST_MSG_FLAG_PRINT_MODULES_FAIL)

// D_TEST_MSG_PRINT_PASS_ALL
//   constant: print all passes (all print pass flags)
#define D_TEST_MSG_PRINT_PASS_ALL               \
    (D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS |       \
     D_TEST_MSG_FLAG_PRINT_TESTS_PASS   |       \
     D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS  |       \
     D_TEST_MSG_FLAG_PRINT_MODULES_PASS)

// D_TEST_MSG_PRINT_ALL
//   constant: print everything (all print flags)
#define D_TEST_MSG_PRINT_ALL                    \
    (D_TEST_MSG_PRINT_FAIL_ALL |                \
     D_TEST_MSG_PRINT_PASS_ALL)

// D_TEST_MSG_ALL
//   constant: all message flags (counter + print)
#define D_TEST_MSG_ALL                          \
    (D_TEST_MSG_COUNT_ALL |                     \
     D_TEST_MSG_PRINT_ALL)


/******************************************************************************
 * CATEGORY-SPECIFIC MESSAGE COMBINATIONS
 *****************************************************************************/

 // assertions - count and print
#define D_TEST_MSG_ASSERTS_FAIL_ONLY            \
    (D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |       \
     D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL)

#define D_TEST_MSG_ASSERTS_PASS_ONLY            \
    (D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS |       \
     D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS)

#define D_TEST_MSG_ASSERTS_ALL                  \
    (D_TEST_MSG_ASSERTS_FAIL_ONLY |             \
     D_TEST_MSG_ASSERTS_PASS_ONLY)

// tests - count and print
#define D_TEST_MSG_TESTS_FAIL_ONLY              \
    (D_TEST_MSG_FLAG_COUNT_TESTS_FAIL |         \
     D_TEST_MSG_FLAG_PRINT_TESTS_FAIL)

#define D_TEST_MSG_TESTS_PASS_ONLY              \
    (D_TEST_MSG_FLAG_COUNT_TESTS_PASS |         \
     D_TEST_MSG_FLAG_PRINT_TESTS_PASS)

#define D_TEST_MSG_TESTS_ALL                    \
    (D_TEST_MSG_TESTS_FAIL_ONLY |               \
     D_TEST_MSG_TESTS_PASS_ONLY)

// test blocks - count and print
#define D_TEST_MSG_BLOCKS_FAIL_ONLY             \
    (D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL |        \
     D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL)

#define D_TEST_MSG_BLOCKS_PASS_ONLY             \
    (D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS |        \
     D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS)

#define D_TEST_MSG_BLOCKS_ALL                   \
    (D_TEST_MSG_BLOCKS_FAIL_ONLY |              \
     D_TEST_MSG_BLOCKS_PASS_ONLY)

// modules - count and print
#define D_TEST_MSG_MODULES_FAIL_ONLY            \
    (D_TEST_MSG_FLAG_COUNT_MODULES_FAIL |       \
     D_TEST_MSG_FLAG_PRINT_MODULES_FAIL)

#define D_TEST_MSG_MODULES_PASS_ONLY            \
    (D_TEST_MSG_FLAG_COUNT_MODULES_PASS |       \
     D_TEST_MSG_FLAG_PRINT_MODULES_PASS)

#define D_TEST_MSG_MODULES_ALL                  \
    (D_TEST_MSG_MODULES_FAIL_ONLY |             \
     D_TEST_MSG_MODULES_PASS_ONLY)


/******************************************************************************
 * SETTINGS FLAG COMBINATIONS
 *****************************************************************************/

 // D_TEST_SETTINGS_STACK_PUSH_ALL
 //   constant: stack push all message types
#define D_TEST_SETTINGS_STACK_PUSH_ALL                  \
    D_TEST_SETTINGS_TO_FLAGS(                           \
        D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL    |       \
        D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS    |       \
        D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING |       \
        D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO)


/******************************************************************************
 * MODE DEFINITIONS - Predefined flag combinations
 *****************************************************************************/

 // D_TEST_MODE_SILENT
 //   constant: no output, no counting, nothing
#define D_TEST_MODE_SILENT  0x00000000

// D_TEST_MODE_MINIMAL
//   constant: count failures, print test failures only
#define D_TEST_MODE_MINIMAL                     \
    (D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |       \
     D_TEST_MSG_FLAG_COUNT_TESTS_FAIL   |       \
     D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL  |       \
     D_TEST_MSG_FLAG_COUNT_MODULES_FAIL |       \
     D_TEST_MSG_FLAG_PRINT_TESTS_FAIL)

// D_TEST_MODE_NORMAL
//   constant: count everything, print failures
#define D_TEST_MODE_NORMAL                      \
    (D_TEST_MSG_COUNT_ALL |                     \
     D_TEST_MSG_PRINT_FAIL_ALL)

// D_TEST_MODE_VERBOSE
//   constant: all message flags set (count and print everything)
#define D_TEST_MODE_VERBOSE  D_TEST_MSG_ALL


/******************************************************************************
 * CONFIGURATION PRESETS
 *****************************************************************************/

#define D_TEST_CONFIG_PRESET_SILENT   D_TEST_MODE_SILENT
#define D_TEST_CONFIG_PRESET_MINIMAL  D_TEST_MODE_MINIMAL
#define D_TEST_CONFIG_PRESET_NORMAL   D_TEST_MODE_NORMAL
#define D_TEST_CONFIG_PRESET_VERBOSE  D_TEST_MODE_VERBOSE


 /******************************************************************************
  * UTILITY MACROS - Flag manipulation and checking
  *****************************************************************************/

  // D_TEST_HAS_FLAG
  //   macro: check if specific flag is set (all bits of flag must match)
#define D_TEST_HAS_FLAG(test_config, flag_mask) \
    (((test_config)->flags & (flag_mask)) == (flag_mask))

// D_TEST_HAS_ANY_FLAG
//   macro: check if any of the flags are set
#define D_TEST_HAS_ANY_FLAG(test_config, flag_mask) \
    (((test_config)->flags & (flag_mask)) != 0)

// D_TEST_GET_MESSAGE_FLAGS
//   macro: extract just the message flags (bits 0-15)
#define D_TEST_GET_MESSAGE_FLAGS(test_config) \
    ((test_config)->flags & D_TEST_MASK_MESSAGE_FLAGS)

// D_TEST_GET_SETTINGS_FLAGS
//   macro: extract just the settings flags (bits 16-31)
#define D_TEST_GET_SETTINGS_FLAGS(test_config) \
    ((test_config)->flags & D_TEST_MASK_SETTINGS_FLAGS)

// D_TEST_IS_SILENT
//   macro: check if in silent mode (no message flags set)
#define D_TEST_IS_SILENT(test_config) \
    (D_TEST_GET_MESSAGE_FLAGS(test_config) == 0)

// D_TEST_IS_VERBOSE
//   macro: check if in verbose mode (all message flags set)
#define D_TEST_IS_VERBOSE(test_config) \
    ((D_TEST_GET_MESSAGE_FLAGS(test_config) & D_TEST_MSG_ALL) == D_TEST_MSG_ALL)

// D_TEST_IS_MODE
//   macro: check if mode matches exactly
#define D_TEST_IS_MODE(test_config, mode) \
    (D_TEST_GET_MESSAGE_FLAGS(test_config) == (mode))


/******************************************************************************
 * SEMANTIC CHECKS - Message flags
 *****************************************************************************/

 // counter semantic checks
#define D_TEST_SHOULD_COUNT_FAILURES(config)  \
    D_TEST_HAS_ANY_FLAG(config, D_TEST_MSG_COUNT_FAIL_ALL)

#define D_TEST_SHOULD_COUNT_PASSES(config)  \
    D_TEST_HAS_ANY_FLAG(config, D_TEST_MSG_COUNT_PASS_ALL)

#define D_TEST_SHOULD_COUNT_ASSERTS_FAIL(config)  \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL)

#define D_TEST_SHOULD_COUNT_ASSERTS_PASS(config)  \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_ASSERTS_PASS)

#define D_TEST_SHOULD_COUNT_TESTS_FAIL(config)  \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_TESTS_FAIL)

#define D_TEST_SHOULD_COUNT_TESTS_PASS(config)  \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_TESTS_PASS)

#define D_TEST_SHOULD_COUNT_BLOCKS_FAIL(config)  \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL)

#define D_TEST_SHOULD_COUNT_BLOCKS_PASS(config)  \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_BLOCKS_PASS)

#define D_TEST_SHOULD_COUNT_MODULES_FAIL(config) \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_MODULES_FAIL)

#define D_TEST_SHOULD_COUNT_MODULES_PASS(config) \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_MODULES_PASS)

// print semantic checks
#define D_TEST_SHOULD_PRINT_FAILURES(config) \
    D_TEST_HAS_ANY_FLAG(config, D_TEST_MSG_PRINT_FAIL_ALL)

#define D_TEST_SHOULD_PRINT_PASSES(config) \
    D_TEST_HAS_ANY_FLAG(config, D_TEST_MSG_PRINT_PASS_ALL)

#define D_TEST_SHOULD_PRINT_ASSERTS_FAIL(config) \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_PRINT_ASSERTS_FAIL)

#define D_TEST_SHOULD_PRINT_ASSERTS_PASS(config) \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS)

#define D_TEST_SHOULD_PRINT_TESTS_FAIL(config) \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_PRINT_TESTS_FAIL)

#define D_TEST_SHOULD_PRINT_TESTS_PASS(config) \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_PRINT_TESTS_PASS)

#define D_TEST_SHOULD_PRINT_BLOCKS_FAIL(config) \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_PRINT_BLOCKS_FAIL)

#define D_TEST_SHOULD_PRINT_BLOCKS_PASS(config) \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_PRINT_BLOCKS_PASS)

#define D_TEST_SHOULD_PRINT_MODULES_FAIL(config) \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_PRINT_MODULES_FAIL)

#define D_TEST_SHOULD_PRINT_MODULES_PASS(config) \
    D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_PRINT_MODULES_PASS)


/******************************************************************************
 * SEMANTIC CHECKS - Settings flags
 *****************************************************************************/

#define D_TEST_SHOULD_PUSH_FAILURES(test_config) \
    D_TEST_HAS_FLAG(test_config, D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_FAIL))

#define D_TEST_SHOULD_PUSH_PASSES(test_config) \
    D_TEST_HAS_FLAG(test_config, D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_PASS))

#define D_TEST_SHOULD_PUSH_WARNINGS(test_config) \
    D_TEST_HAS_FLAG(test_config, D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_WARNING))

#define D_TEST_SHOULD_PUSH_INFO(test_config) \
    D_TEST_HAS_FLAG(test_config, D_TEST_SETTINGS_TO_FLAGS(D_TEST_SETTINGS_FLAG_STACK_PUSH_INFO))

 // legacy aliases for test compatibility
#define D_TEST_SHOULD_STACK_PUSH_FAIL(config)    D_TEST_SHOULD_PUSH_FAILURES(config)
#define D_TEST_SHOULD_STACK_PUSH_PASS(config)    D_TEST_SHOULD_PUSH_PASSES(config)
#define D_TEST_SHOULD_STACK_PUSH_WARNING(config) D_TEST_SHOULD_PUSH_WARNINGS(config)
#define D_TEST_SHOULD_STACK_PUSH_INFO(config)    D_TEST_SHOULD_PUSH_INFO(config)


/******************************************************************************
 * DEFAULT VALUES
 *****************************************************************************/

#define D_TEST_DEFAULT_INDENT       "  "
#define D_TEST_DEFAULT_MAX_INDENT   ((uint16_t)10)
#define D_TEST_DEFAULT_MAX_FAILURES ((size_t)0)
#define D_TEST_DEFAULT_TIMEOUT      ((size_t)1000)


/******************************************************************************
* CORE STRUCTURES
*****************************************************************************/

// d_test_config
//   struct: configuration settings for individual customizations of 
// `d_test_element`s.
struct d_test_config
{
    uint32_t               flags;        // packed message and settings flags
    struct d_min_enum_map* settings;     // key-value configuration map
    struct d_min_enum_map* stage_hooks;  // stage hooks
};


/******************************************************************************
 * FUNCTION DECLARATIONS
 *****************************************************************************/

// creation/destruction
struct d_test_config* d_test_config_new(uint32_t _flags);
struct d_test_config* d_test_config_new_preset(uint32_t _preset);
struct d_test_config* d_test_config_new_copy(const struct d_test_config* _other);

// config value accessors
bool        d_test_config_get_bool(const struct d_test_config* _config, uint32_t _key);
size_t      d_test_config_get_size_t(const struct d_test_config* _config, uint32_t _key);
int32_t     d_test_config_get_int32(const struct d_test_config* _config, uint32_t _key);
uint32_t    d_test_config_get_uint32(const struct d_test_config* _config, uint32_t _key);
const char* d_test_config_get_string(const struct d_test_config* _config, uint32_t _key);
void*       d_test_config_get_ptr(const struct d_test_config* _config, uint32_t _key);

// config value setters
bool d_test_config_set_bool(struct d_test_config* _config, uint32_t _key, bool _value);
bool d_test_config_set_size_t(struct d_test_config* _config, uint32_t _key, size_t _value);
bool d_test_config_set_int32(struct d_test_config* _config, uint32_t _key, int32_t _value);
bool d_test_config_set_uint32(struct d_test_config* _config, uint32_t _key, uint32_t _value);
bool d_test_config_set_string(struct d_test_config* _config, uint32_t _key, const char* _value);
bool d_test_config_set_ptr(struct d_test_config* _config, uint32_t _key, void* _value);

// key lookup from string
enum DTestConfigKey d_test_config_key_from_string(const char* _key);

void                d_test_config_free(struct d_test_config* _config);


#endif  // DJINTERP_TEST_CONFIG_