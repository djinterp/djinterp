/*******************************************************************************
* bitwisedom [tests]                                        freebsd_file_flags.c
*
* The file-flags example against sys/stat.h: both halves and every
* flag, compared with the platform's own at compile time. Built where
* the build targets FreeBSD.
*
* path:      /tests/djinterp/tools/bitwisedom/platform/freebsd_file_flags.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.10
*******************************************************************************/

#include <stdint.h>              // uint32_t
#include <sys/stat.h>            // UF_NODUMP, ...
#include "freebsd_file_flags.h"  // the example, as bitwisedom-gen wrote it


// SAME
//   ours is theirs, compared as the 32 unsigned bits both are.
#define SAME(_ours, _theirs)                                                   \
    _Static_assert((uint32_t)(_ours) == (uint32_t)(_theirs),                   \
                   #_ours " is " #_theirs)


// the halves
SAME(FBSD_UF_SETTABLE, UF_SETTABLE);
SAME(FBSD_SF_SETTABLE, SF_SETTABLE);

// the owner's flags; the Windows-compatible ones arrived in FreeBSD 10,
// UF_NOCACHE later
SAME(FBSD_UF_NODUMP, UF_NODUMP);
SAME(FBSD_UF_IMMUTABLE, UF_IMMUTABLE);
SAME(FBSD_UF_APPEND, UF_APPEND);
SAME(FBSD_UF_OPAQUE, UF_OPAQUE);
SAME(FBSD_UF_NOUNLINK, UF_NOUNLINK);
#if defined(UF_SYSTEM)
SAME(FBSD_UF_SYSTEM, UF_SYSTEM);
#endif
#if defined(UF_SPARSE)
SAME(FBSD_UF_SPARSE, UF_SPARSE);
#endif
#if defined(UF_OFFLINE)
SAME(FBSD_UF_OFFLINE, UF_OFFLINE);
#endif
#if defined(UF_REPARSE)
SAME(FBSD_UF_REPARSE, UF_REPARSE);
#endif
#if defined(UF_ARCHIVE)
SAME(FBSD_UF_ARCHIVE, UF_ARCHIVE);
#endif
#if defined(UF_READONLY)
SAME(FBSD_UF_READONLY, UF_READONLY);
#endif
#if defined(UF_NOCACHE)
SAME(FBSD_UF_NOCACHE, UF_NOCACHE);
#endif
#if defined(UF_HIDDEN)
SAME(FBSD_UF_HIDDEN, UF_HIDDEN);
#endif

// the superuser's
SAME(FBSD_SF_ARCHIVED, SF_ARCHIVED);
SAME(FBSD_SF_IMMUTABLE, SF_IMMUTABLE);
SAME(FBSD_SF_APPEND, SF_APPEND);
SAME(FBSD_SF_NOUNLINK, SF_NOUNLINK);
SAME(FBSD_SF_SNAPSHOT, SF_SNAPSHOT);
