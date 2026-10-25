/*******************************************************************************
* djinterp [c]                                                 option_override.h
*
* The merge engine's front door. Include THIS. See option.h for the umbrella
* pattern and for the note on the provisional dispatch spelling.
*
* NO C FACE, AND THAT IS NOT AN OMISSION.
*   There is no `option_override_c.h`. A C face exists to add C ERGONOMICS on
* top of the core -- macros, _Generic dispatch, a literal-friendly spelling --
* and the merge engine has none to add: `d_option_set_override` takes three
* sets and a policy, and there is no sugar that makes that shorter without
* making it less clear. Shipping an empty `_c.h` for symmetry would be a file
* the boundary criterion says should not exist.
*   The C++ face is a different matter: it carries the type-level policies and
* the translation-time walk, which are real notation over the same core.
*
*
* path:      /inc/djinterp/c/option/option_override.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.23
*******************************************************************************/

#ifndef DJINTERP_C_OPTION_OPTION_OVERRIDE_H
#define DJINTERP_C_OPTION_OPTION_OVERRIDE_H 1

// djinterp
#include "./option_set.h"
#include "./option_override_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // RE_STD_HAS_INT64: its floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS --
// the header compiles to nothing (the owner's ruling of 2026.10.03 on round 3's
// question 1, (a)).
#if RE_STD_HAS_INT64

// the C++ face, from C++17, decision 3.6's floor for option; below it a C++
// caller has the C core
#if ( (D_ENV_LANG_USING_CPP) &&                                           \
      (D_ENV_LANG_IS_CPP17_OR_HIGHER) )
#   include "../../core/option/option_override.hpp"
#endif  // C++17 and up


#endif  // RE_STD_HAS_INT64

#endif  // DJINTERP_C_OPTION_OPTION_OVERRIDE_H
