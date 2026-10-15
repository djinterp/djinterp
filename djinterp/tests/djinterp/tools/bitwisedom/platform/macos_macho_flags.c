/*******************************************************************************
* bitwisedom [tests]                                         macos_macho_flags.c
*
* The Mach-O example against mach-o/loader.h: every header flag,
* compared with the platform's own at compile time. Built where the
* build targets macOS.
*
* path:      /tests/djinterp/tools/bitwisedom/platform/macos_macho_flags.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.10
*******************************************************************************/

#include <stdint.h>              // uint32_t
#include <mach-o/loader.h>       // MH_NOUNDEFS, ...
#include "macos_macho_flags.h"   // the example, as bitwisedom-gen wrote it


// SAME
//   ours is theirs, compared as the 32 unsigned bits both are.
#define SAME(_ours, _theirs)                                                   \
    _Static_assert((uint32_t)(_ours) == (uint32_t)(_theirs),                   \
                   #_ours " is " #_theirs)


// every flag; the newest only where the SDK has them
SAME(MACHO_MH_NOUNDEFS, MH_NOUNDEFS);
SAME(MACHO_MH_INCRLINK, MH_INCRLINK);
SAME(MACHO_MH_DYLDLINK, MH_DYLDLINK);
SAME(MACHO_MH_BINDATLOAD, MH_BINDATLOAD);
SAME(MACHO_MH_PREBOUND, MH_PREBOUND);
SAME(MACHO_MH_SPLIT_SEGS, MH_SPLIT_SEGS);
SAME(MACHO_MH_LAZY_INIT, MH_LAZY_INIT);
SAME(MACHO_MH_TWOLEVEL, MH_TWOLEVEL);
SAME(MACHO_MH_FORCE_FLAT, MH_FORCE_FLAT);
SAME(MACHO_MH_NOMULTIDEFS, MH_NOMULTIDEFS);
SAME(MACHO_MH_NOFIXPREBINDING, MH_NOFIXPREBINDING);
SAME(MACHO_MH_PREBINDABLE, MH_PREBINDABLE);
SAME(MACHO_MH_ALLMODSBOUND, MH_ALLMODSBOUND);
SAME(MACHO_MH_SUBSECTIONS_VIA_SYMBOLS, MH_SUBSECTIONS_VIA_SYMBOLS);
SAME(MACHO_MH_CANONICAL, MH_CANONICAL);
SAME(MACHO_MH_WEAK_DEFINES, MH_WEAK_DEFINES);
SAME(MACHO_MH_BINDS_TO_WEAK, MH_BINDS_TO_WEAK);
SAME(MACHO_MH_ALLOW_STACK_EXECUTION, MH_ALLOW_STACK_EXECUTION);
SAME(MACHO_MH_ROOT_SAFE, MH_ROOT_SAFE);
SAME(MACHO_MH_SETUID_SAFE, MH_SETUID_SAFE);
SAME(MACHO_MH_NO_REEXPORTED_DYLIBS, MH_NO_REEXPORTED_DYLIBS);
SAME(MACHO_MH_PIE, MH_PIE);
SAME(MACHO_MH_DEAD_STRIPPABLE_DYLIB, MH_DEAD_STRIPPABLE_DYLIB);
SAME(MACHO_MH_HAS_TLV_DESCRIPTORS, MH_HAS_TLV_DESCRIPTORS);
SAME(MACHO_MH_NO_HEAP_EXECUTION, MH_NO_HEAP_EXECUTION);
#if defined(MH_APP_EXTENSION_SAFE)
SAME(MACHO_MH_APP_EXTENSION_SAFE, MH_APP_EXTENSION_SAFE);
#endif
#if defined(MH_NLIST_OUTOFSYNC_WITH_DYLDINFO)
SAME(MACHO_MH_NLIST_OUTOFSYNC_WITH_DYLDINFO, MH_NLIST_OUTOFSYNC_WITH_DYLDINFO);
#endif
#if defined(MH_SIM_SUPPORT)
SAME(MACHO_MH_SIM_SUPPORT, MH_SIM_SUPPORT);
#endif
#if defined(MH_DYLIB_IN_CACHE)
SAME(MACHO_MH_DYLIB_IN_CACHE, MH_DYLIB_IN_CACHE);
#endif

// the typical files
SAME(MACHO_EXECUTABLE, MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE);
SAME(MACHO_DYLIB,
     MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_NO_REEXPORTED_DYLIBS);
