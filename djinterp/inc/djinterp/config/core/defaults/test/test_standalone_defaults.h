/******************************************************************************
* djinterp [test]                                   test_standalone_defaults.h
*
*   Standalone test framework with simple tree structure for tests and 
* assertions. Supports nested test blocks/groups, template-based output,
* and unified test runner with chainable module execution.
*
* 
* path:      \inc\test\test_standalone.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.08
******************************************************************************/

#ifndef DJINTERP_C_DEFAULTS_TEST_STANDALONE_
#define DJINTERP_C_DEFAULTS_TEST_STANDALONE_ 1

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../djinterp.h"
#include "../../../container/map/map.h"
#include "../../../container/map/enum_map_entry.h"
#include "../../../container/map/min_enum_map.h"
#include "../../config/test/test_config.h"


#if defined(D_CFG_TEST_USING_DTIME)
    
#endif 


// DTestDefaultOption
//   enum:
enum DTestDefaultOption
{
    D_TEST_DEFAULT_OPTION_NONE                    = 0x00,
    D_TEST_DEFAULT_OPTION_OUTPUT_MODE             = 0x01,    
    D_TEST_DEFAULT_OPTION_METADATA                = 0x02,   // struct d_min_enum_map*
    D_TEST_DEFAULT_OPTION_COUNTERS                = 0x03,   // array
    D_TEST_DEFAULT_OPTION_TEST_PRINTER            = 0x04,
    D_TEST_DEFAULT_OPTION_MAX_FAILURES            = 0x05,


    D_TEST_DEFAULT_OPTION_USER                    = 0xFF << 1


    // --- general keys (1 - 10) ---
    D_TEST_DEFAULT_OPTION_FAILURES                = 0x01,   // d_test_standalone_failure_list*
    D_TEST_DEFAULT_OPTION_ASSERTION_NUMBER        = 0x02,   // size_t via intptr_t
    D_TEST_DEFAULT_OPTION_TEST_NUMBER             = 0x03,   // size_t via intptr_t
    D_TEST_DEFAULT_OPTION_OUTPUT_FILE             = 0x04,   // const char* (filepath)
    D_TEST_DEFAULT_OPTION_CONTEXT                 = 0x05,   // void* (opaque user data)
    D_TEST_DEFAULT_OPTION_MAX_MODULES             = 0x07,   // intptr_t (D_TEST_DEFAULT_OPTION_NO_LIMIT)
    // --- counter keys (11 - 12) ---
    D_TEST_DEFAULT_OPTION_ASSERTION_COUNTER       = 0x10,  // d_test_counter*
    D_TEST_DEFAULT_OPTION_TEST_COUNTER            = 0x11,  // d_test_counter*
    // --- option keys (21 - 30) ---
    //   stored as D_TEST_DEFAULT_OPTION_TRUE / D_TEST_DEFAULT_OPTION_FALSE.
    D_TEST_DEFAULT_OPTION_OPT_NUMBER_ASSERTIONS   = 0x21,  // bool via intptr_t
    D_TEST_DEFAULT_OPTION_OPT_NUMBER_TESTS        = 0x22,  // bool via intptr_t
    D_TEST_DEFAULT_OPTION_OPT_GLOBAL_NUMBERING    = 0x23,  // bool via intptr_t
    D_TEST_DEFAULT_OPTION_OPT_SHOW_INFO           = 0x24,  // bool via intptr_t
    D_TEST_DEFAULT_OPTION_OPT_SHOW_MODULE_FOOTER  = 0x25,  // bool via intptr_t
    D_TEST_DEFAULT_OPTION_OPT_LIST_FAILURES       = 0x26,  // bool via intptr_t
    D_TEST_DEFAULT_OPTION_OPT_WAIT_FOR_INPUT      = 0x27,  // bool via intptr_t
    D_TEST_DEFAULT_OPTION_OPT_SHOW_NOTES          = 0x28,  // bool via intptr_t
    // --- result keys (31 - 40) ---
    D_TEST_DEFAULT_OPTION_ELAPSED_TIME            = 0x31,  // double* (owned)
    D_TEST_DEFAULT_OPTION_PASSED                  = 0x32,  // bool via intptr_t
    D_TEST_DEFAULT_OPTION_MODULES_TOTAL           = 0x33,  // size_t via intptr_t
    D_TEST_DEFAULT_OPTION_MODULES_PASSED          = 0x34,  // size_t via intptr_t
    D_TEST_DEFAULT_OPTION_MODULE_RESULTS          = 0x35,  // D_TEST_DEFAULT_OPTION_list** (array, owned)
    D_TEST_DEFAULT_OPTION_MODULE_RESULT_COUNT     = 0x36,  // size_t via intptr_t
    // --- note keys (41 - 42) ---
    D_TEST_DEFAULT_OPTION_NOTES                   = 0x41,  // const d_test_note_section*
    D_TEST_DEFAULT_OPTION_NOTE_COUNT              = 0x42,  // size_t via intptr_t
    // --- tree/nesting keys (51 - 56) ---
    D_TEST_DEFAULT_OPTION_CHILDREN                = 0x51,  // d_test_object**
    D_TEST_DEFAULT_OPTION_CHILD_COUNT             = 0x52,  // size_t via intptr_t
    D_TEST_DEFAULT_OPTION_SUB_RUNNER              = 0x53,  // d_test_standalone_runner*
    D_TEST_DEFAULT_OPTION_PARENT_RUNNER           = 0x54,  // d_test_standalone_runner*
    D_TEST_DEFAULT_OPTION_DEPTH                   = 0x55,  // size_t via intptr_t
    D_TEST_DEFAULT_OPTION_RUNNER_ID               = 0x56,  // size_t via intptr_t
    // --- module registration keys (61 - 63) ---
    //   replaces inline module array fields on the runner.
    //   module_entry array is owned; freed during cleanup.
    D_TEST_DEFAULT_OPTION_MODULES                 = 0x61,  // d_test_standalone_module_entry*
    D_TEST_DEFAULT_OPTION_MODULE_COUNT            = 0x62,  // size_t via intptr_t
    D_TEST_DEFAULT_OPTION_MODULE_CAPACITY         = 0x63,  // size_t via intptr_t
    // --- result backing storage keys (71 - 73) ---
    //   per-module counter/time arrays allocated during execute.
    //   owned by the runner; freed during cleanup.
    D_TEST_DEFAULT_OPTION_RESULT_ASSERTION_CTRS   = 0x71,  // d_test_counter* (array, owned)
    D_TEST_DEFAULT_OPTION_RESULT_TEST_CTRS        = 0x72,  // d_test_counter* (array, owned)
    D_TEST_DEFAULT_OPTION_RESULT_ELAPSED_TIMES    = 0x73,  // double* (array, owned)

    // --- user keys ---
};


struct d_min_enum_map* d_defaults_test_generate_options(void);


#endif  // DJINTERP_C_DEFAULTS_TEST_STANDALONE_
