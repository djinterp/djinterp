/******************************************************************************
* djinterp [test]                                                test_config.h
*
* Feature configuration for the djinterp DTest module.
*
*
* path:      /inc/c/core/config/test/test_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.27
******************************************************************************/

#ifndef DJINTERP_C_CONFIG_TEST_
#define DJINTERP_C_CONFIG_TEST_ 1

#include "../../../djinterp.h"
#include "../../../dconfig.h"


#ifndef D_CFG_TEST_ENABLE_CLI
    #define D_CFG_TEST_ENABLE_CLI     1
#endif  // D_CFG_TEST_ENABLE_CLI

#ifndef D_CFG_TEST_ENABLE_FILE_IO
    #define D_CFG_TEST_ENABLE_FILE_IO 1
#endif  // D_CFG_TEST_ENABLE_FILE_IO

#ifndef D_CFG_TEST_ENABLE_TIME
    #define D_CFG_TEST_ENABLE_TIME    1
#endif  // D_CFG_TEST_ENABLE_TIME

#if ( defined(D_EMOJIS) &&                                                  \
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


#endif  // DJINTERP_C_CONFIG_TEST_
