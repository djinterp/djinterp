/******************************************************************************
* djinterp [test]                                                test_common.h
*
*   Common types and declarations shared across the test framework.
*   Includes the discriminated union d_test_type for tree node types,
*   and the d_test_arg structure for configuration arguments.
*
*
* path:      \inc\test\test_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#ifndef DJINTERP_TEST_COMMON_
#define DJINTERP_TEST_COMMON_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\djinterp.h"
#include "..\container\map\min_enum_map.h"


#define D_KEYWORD_TEST_ASSERTION   assertion
#define D_KEYWORD_TEST_TEST_FN     test_fn
#define D_KEYWORD_TEST_TEST        test
#define D_KEYWORD_TEST_BLOCK       block
#define D_KEYWORD_TEST_MODULE      module

// D_TEST_PASS
//   definition: evaluates in an evaluation, assertion, test, etc. passing
// and having the desired outcome.
#define D_TEST_PASS D_SUCCESS

// D_TEST_FAIL
//   definition: evaluates in an evaluation, assertion, test, etc. failing
// to have the desired outcome.
#define D_TEST_FAIL D_FAILURE

// d_test_id
//   type: 
typedef unsigned long long d_test_id;

// fn_test
//   function pointer: function pointer for a boolean test that returns 
// D_TEST_PASS or D_TEST_FAIL.
typedef bool (*fn_test)();

// fn_stage
//   function pointer: function pointer for test lifecycle stage hooks.
// takes a test pointer and returns success/failure.
typedef bool (*fn_stage)(struct d_test*);


//
#if ( defined(D_EMOJIS) &&  \
      (D_EMOJIS == D_ENABLED) )
    #define D_EMOJI_CHECKMARK       "\xE2\x9C\x94"
    #define D_EMOJI_CROSS_MARK      "\xE2\x9D\x8C"
    #define D_EMOJI_PARTY_POPPER    "\xF0\x9F\x8E\x89"
    #define D_EMOJI_CLIPBOARD       "\xF0\x9F\x93\x8B"
    #define D_EMOJI_PAGE_FACING_UP  "\xF0\x9F\x93\x84"
    #define D_EMOJI_FILE_FOLDER     "\xF0\x9F\x93\x81"
    #define D_EMOJI_PACKAGE         "\xF0\x9F\x93\xA6"
    #define D_EMOJI_WARNING_SIGN    "\xE2\x9A\xA0"
    #define D_EMOJI_QUESTION_MARK   "\xE2\x9D\x93"

    // primary test symbols
    #define D_TEST_SYMBOL_PASS      D_EMOJI_CHECKMARK
    #define D_TEST_SYMBOL_FAIL      D_EMOJI_CROSS_MARK
    #define D_TEST_SYMBOL_SUCCESS   D_EMOJI_PARTY_POPPER
    #define D_TEST_SYMBOL_INFO      D_EMOJI_CLIPBOARD

    // tree structure symbols (emoji versions)
    #define D_TEST_SYMBOL_LEAF      D_EMOJI_PAGE_FACING_UP
    #define D_TEST_SYMBOL_INTERIOR  D_EMOJI_FILE_FOLDER  
    #define D_TEST_SYMBOL_MODULE    D_EMOJI_PACKAGE
    #define D_TEST_SYMBOL_WARNING   D_EMOJI_WARNING_SIGN
    #define D_TEST_SYMBOL_UNKNOWN   D_EMOJI_QUESTION_MARK

#else
    // fallback to ASCII-only symbols
    #define D_TEST_SYMBOL_PASS      "[PASS]"
    #define D_TEST_SYMBOL_FAIL      "[FAIL]"
    #define D_TEST_SYMBOL_SUCCESS   "[SUCCESS]"
    #define D_TEST_SYMBOL_INFO      "[INFO]"

    // ASCII tree structure symbols
    #define D_TEST_SYMBOL_LEAF      "[LEAF]"
    #define D_TEST_SYMBOL_INTERIOR  "[GROUP]"
    #define D_TEST_SYMBOL_MODULE    "[MODULE]"
    #define D_TEST_SYMBOL_WARNING   "[WARNING]"
    #define D_TEST_SYMBOL_UNKNOWN   "[UNKNOWN]"

#endif  // D_IS_ENABLED(D_EMOJIS)

// d_test_arg
//   struct: key-value pair for module/block/test configuration arguments.
// Used by D_TEST_ARGS marker and the *_new_args() constructor functions.
struct d_test_arg
{
    const char* key;
    void*       value;
};

// d_test_arg_list
//   struct: collection of test arguments.
struct d_test_arg_list
{
    size_t             count;
    struct d_test_arg* args;
};

// d_test_fn
//   struct: wrapper for a test function with optional arguments.
struct d_test_fn
{
    fn_test test_fn;
    size_t  count;
    void**  args;
};

// DTestStage
//   enum: lifecycle stages for test execution hooks.
enum DTestStage
{
    D_TEST_STAGE_SETUP      = 0,
    D_TEST_STAGE_TEAR_DOWN  = 1,
    D_TEST_STAGE_ON_SUCCESS = 2,
    D_TEST_STAGE_ON_FAILURE = 3,
    D_TEST_STAGE_BEFORE     = 4,
    D_TEST_STAGE_AFTER      = 5
};

// DTestTypeFlag
//   enum: discriminator for d_test_type union, identifying the type of
// test tree node.
enum DTestTypeFlag
{
    D_TEST_TYPE_UNKNOWN    = 0,
    D_TEST_TYPE_ASSERT     = 1,
    D_TEST_TYPE_TEST_FN    = 2,
    D_TEST_TYPE_TEST       = 3,
    D_TEST_TYPE_TEST_BLOCK = 4,
    D_TEST_TYPE_MODULE     = 5
};


#endif  // DJINTERP_TEST_COMMON_