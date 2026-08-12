/******************************************************************************
* djinterp [config/test]                                             cfg_test.h
*
*   The DTest config umbrella.  Includes the three per-module config headers
* and declares nothing of its own.
*
*   targets:  nothing directly
*   requires: cfg_test_object.h, cfg_test_metadata.h, cfg_test_event.h
*
*   A MODULE MUST NOT INCLUDE THIS FILE.  Each module includes the one config
* that owns its knobs -- test_event.h includes cfg_test_event.h and nothing
* else.  This umbrella exists for CONSUMERS: a build script, a preset, or a
* tool that wants every DTest knob resolved without naming three headers.
*
*   IT USED TO BE THE OPPOSITE, and that is why the warning is here rather than
* left implied.  Until rev13 this file held every knob for four modules, having
* absorbed cfg_test_common.h and cfg_test_object.h at revision.md §8 and
* cfg_test_event.h at rev11.  Each absorption was argued from "one file, one
* module" -- and the accumulation ended somewhere that rule forbids, because a
* file serving four modules is the sibling problem at a larger scale.  A module
* reaching for this umbrella instead of its own config would rebuild the same
* thing one include at a time.
*
*   WHAT MOVED OUT ENTIRELY.  The D_TEST_*_ID_TYPE roll-up aliases are gone.  A
* knob still originates in a config header; the typedef that turns it into a
* type now lives at the top of the module header that owns the type, so a
* reader looking for d_test_event_id finds it in test_event.h.  The alias layer
* only ever renamed one macro to another and put the answer in a third file.
*
* path:      /inc/djinterp/config/c/test/cfg_test.h
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_CONFIG_C_TEST
#define DJINTERP_CONFIG_C_TEST 1

#include "./cfg_test_object.h"
#include "./cfg_test_metadata.h"
#include "./cfg_test_event.h"

//   test_common has NO knobs and no config file, so there is nothing here for
// it.  Its only one, D_CFG_TEST_COMMON_SKIP_IS_FAILURE, was dropped in
// revision.md §3: the module became header-only, and a preprocessor-chosen
// rank table compiled per translation unit is a disagreement no linker can
// diagnose.

#endif  // DJINTERP_CONFIG_C_TEST
