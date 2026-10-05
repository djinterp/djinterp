/*******************************************************************************
* djinterp [config]                                             dtest_defaults.h
*
*   Standalone test framework with simple tree structure for tests and
* assertions. Supports nested test blocks/groups, template-based output,
* and unified test runner with chainable module execution.
*
*
* path:      /inc/djinterp/config/core/defaults/test/dtest_defaults.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.08
*                                                            revised: 2026.09.29
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_DEFAULTS_TEST_DTEST_DEFAULTS_H
#define DJINTERP_CONFIG_CORE_DEFAULTS_TEST_DTEST_DEFAULTS_H 1

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../../c/djinterp.h"
#include "../../../../c/memory/dmemory.h"
#include "../../../../c/fs/dfile.h"
#include "../../../../c/dstring.h"
#include "../../../../c/dtime.h"
#include "../../../../test/c/test_common.h"
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's
                                                    // floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)






#endif  // defined(INT64_MAX)

#endif  // DJINTERP_CONFIG_CORE_DEFAULTS_TEST_DTEST_DEFAULTS_H
