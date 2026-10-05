/*******************************************************************************
* djinterp [config]                                            cfg_test_common.h
*
*   Build-time configuration for the DTest vocabulary.
*
*   targets:  test/test_common_common.h -> D_INTERNAL_TEST_COMMON_*
*   requires: cfg_common.h
*
*   DELIBERATELY SMALL.  This module is a value set and a record; there is very
* little about it a build should be able to change, and a knob that lets one
* build disagree with another about what `failed` means would be a defect
* wearing the costume of a feature. The one knob here governs a POLICY -- how
* two statuses combine -- not a representation.
*
*
* path:      /inc/djinterp/config/test/cfg_test_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_CONFIG_TEST_CFG_TEST_COMMON_H
#define DJINTERP_CONFIG_TEST_CFG_TEST_COMMON_H 1

// djinterp
#include "../cfg_common.h"

// D_CFG_TEST_COMMON_SKIP_IS_FAILURE
//   brief: whether `skipped` counts as a failure when statuses combine.
// Default 0 -- a skip is an intentional non-evaluation, not a failure.
//   Set 1 for a build where an unrun test must not pass silently: CI for a
// release branch, typically, where "we skipped it" and "it works" should not
// look the same in a summary. It changes only d_test_status_worse_of, which is
// where a parent node's status is derived from its children -- so the effect is
// visible in a rolled-up result and nowhere else.
#ifndef D_CFG_TEST_COMMON_SKIP_IS_FAILURE
#   define D_CFG_TEST_COMMON_SKIP_IS_FAILURE 0
#endif

#if !D_CFG_IS_BOOL(D_CFG_TEST_COMMON_SKIP_IS_FAILURE)
#   error "D_CFG_TEST_COMMON_SKIP_IS_FAILURE must be 0 or 1"
#endif

// D_INTERNAL_TEST_COMMON_SKIP_IS_FAILURE
//   brief: the resolved policy, read by d_test_status_worse_of.
#if D_CFG_IS_ON(D_CFG_TEST_COMMON_SKIP_IS_FAILURE)
#   define D_INTERNAL_TEST_COMMON_SKIP_IS_FAILURE 1
#else
#   define D_INTERNAL_TEST_COMMON_SKIP_IS_FAILURE 0
#endif

#endif  // DJINTERP_CONFIG_TEST_CFG_TEST_COMMON_H
