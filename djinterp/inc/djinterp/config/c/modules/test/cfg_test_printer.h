/*******************************************************************************
* djinterp [config]                                           cfg_test_printer.h
*
*   Build-time configuration for the test printer: which of its optional parts
* are compiled, and the symbols it prints for a result.
*
*   targets:  c/modules/test/test_printer.c -> D_CFG_TEST_ENABLE_CLI,
*                                              D_TEST_SYMBOL_*
*             c/modules/test/test_module.c,
*             c/modules/test/test_standalone.c -> D_TEST_SYMBOL_*
*   requires: cfg_common.h (helpers, D_CFG_TESTING); djinterp.h (D_ENABLED)
*
*   ONE HOME FOR WHAT WAS TWO.  This file replaces config/core/test/cfg_test.h
* and config/c/modules/test/test_config.h, which held the same knobs and
* symbols word for word under two names, neither of them the cfg_ name of the
* module they serve. The README's rule is one canonical default per knob.
*
*
* path:      /inc/djinterp/config/c/modules/test/cfg_test_printer.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.31
*                                                            revised: 2026.10.31
*******************************************************************************/

#ifndef DJINTERP_CONFIG_C_TEST_CFG_TEST_PRINTER_H
#define DJINTERP_CONFIG_C_TEST_CFG_TEST_PRINTER_H 1

// djinterp
#include "../../../cfg_common.h"      // D_CFG_* helpers, user overrides
#include "../../../../c/djinterp.h"  // framework root


//==============================================================================
// 1.  KNOBS
//==============================================================================

// D_CFG_TEST_ENABLE_CLI
//   knob: compile the printer's command-line option functions
// (d_test_printer_options_parse and its kin) (1, the default), or leave them
// out (0) for a runner that takes no arguments.
#ifndef D_CFG_TEST_ENABLE_CLI
    #define D_CFG_TEST_ENABLE_CLI     1
#endif  // D_CFG_TEST_ENABLE_CLI

// D_CFG_TEST_ENABLE_FILE_IO
//   knob: reserved for the test framework's file output (1, the default).
// Nothing in this tree reads it yet.
#ifndef D_CFG_TEST_ENABLE_FILE_IO
    #define D_CFG_TEST_ENABLE_FILE_IO 1
#endif  // D_CFG_TEST_ENABLE_FILE_IO

// D_CFG_TEST_ENABLE_TIME
//   knob: reserved for the test framework's timing (1, the default). Nothing
// in this tree reads it yet.
#ifndef D_CFG_TEST_ENABLE_TIME
    #define D_CFG_TEST_ENABLE_TIME    1
#endif  // D_CFG_TEST_ENABLE_TIME

#if !D_CFG_IS_BOOL(D_CFG_TEST_ENABLE_CLI)
    #error "D_CFG_TEST_ENABLE_CLI must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TEST_ENABLE_FILE_IO)
    #error "D_CFG_TEST_ENABLE_FILE_IO must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TEST_ENABLE_TIME)
    #error "D_CFG_TEST_ENABLE_TIME must be 0 or 1"
#endif


//==============================================================================
// 2.  RESOLVED VALUES
//==============================================================================

// D_EMOJI_*, D_TEST_SYMBOL_*
//   constants: what the printer writes for a result and for each kind of tree
// node. Emoji where the build defines D_EMOJIS as D_ENABLED; plain ASCII
// otherwise, which is also what every terminal can show.
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

#endif  // D_EMOJIS


#endif  // DJINTERP_CONFIG_C_TEST_CFG_TEST_PRINTER_H
