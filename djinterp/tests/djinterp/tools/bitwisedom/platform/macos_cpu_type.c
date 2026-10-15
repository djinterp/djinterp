/*******************************************************************************
* bitwisedom [tests]                                            macos_cpu_type.c
*
* The CPU-type example against mach/machine.h: the ABI bits, every
* type and the subtypes, compared with the platform's own at compile
* time -- CPU_TYPE_ANY as the negative number it is. Built where the
* build targets macOS.
*
* path:      /tests/djinterp/tools/bitwisedom/platform/macos_cpu_type.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.10
*******************************************************************************/

#include <stdint.h>              // uint32_t
#include <mach/machine.h>        // CPU_TYPE_ARM64, ...
#include "macos_cpu_type.h"      // the example, as bitwisedom-gen wrote it


// SAME
//   ours is theirs, compared as the 32 unsigned bits both are.
#define SAME(_ours, _theirs)                                                   \
    _Static_assert((uint32_t)(_ours) == (uint32_t)(_theirs),                   \
                   #_ours " is " #_theirs)

// SAME_SIGNED
//   ours is theirs, compared as the signed numbers both are.
#define SAME_SIGNED(_ours, _theirs)                                            \
    _Static_assert((long long)(_ours) == (long long)(_theirs),                \
                   #_ours " is " #_theirs)


// the fields: the family is every bit the ABI's mask is not
SAME(MACH_CPU_ARCH_MASK, CPU_ARCH_MASK);
SAME(MACH_CPU_FAMILY, ~CPU_ARCH_MASK);
SAME(MACH_CPU_ARCH_ABI64, CPU_ARCH_ABI64);
#if defined(CPU_ARCH_ABI64_32)
SAME(MACH_CPU_ARCH_ABI64_32, CPU_ARCH_ABI64_32);
#endif

// the types: a signed int, CPU_TYPE_ANY negative
SAME_SIGNED(MACH_CPU_TYPE_ANY, CPU_TYPE_ANY);
SAME(MACH_CPU_TYPE_VAX, CPU_TYPE_VAX);
SAME(MACH_CPU_TYPE_MC680x0, CPU_TYPE_MC680x0);
SAME(MACH_CPU_TYPE_X86, CPU_TYPE_X86);
SAME(MACH_CPU_TYPE_I386, CPU_TYPE_I386);
SAME(MACH_CPU_TYPE_X86_64, CPU_TYPE_X86_64);
SAME(MACH_CPU_TYPE_MC98000, CPU_TYPE_MC98000);
SAME(MACH_CPU_TYPE_HPPA, CPU_TYPE_HPPA);
SAME(MACH_CPU_TYPE_ARM, CPU_TYPE_ARM);
SAME(MACH_CPU_TYPE_ARM64, CPU_TYPE_ARM64);
SAME(MACH_CPU_TYPE_MC88000, CPU_TYPE_MC88000);
SAME(MACH_CPU_TYPE_SPARC, CPU_TYPE_SPARC);
SAME(MACH_CPU_TYPE_I860, CPU_TYPE_I860);
SAME(MACH_CPU_TYPE_POWERPC, CPU_TYPE_POWERPC);
SAME(MACH_CPU_TYPE_POWERPC64, CPU_TYPE_POWERPC64);
#if defined(CPU_TYPE_ARM64_32)
SAME(MACH_CPU_TYPE_ARM64_32, CPU_TYPE_ARM64_32);
#endif

// the subtypes
SAME(MACH_CPU_SUBTYPE_MASK, CPU_SUBTYPE_MASK);
SAME(MACH_CPU_SUBTYPE_LIB64, CPU_SUBTYPE_LIB64);
SAME(MACH_CPU_SUBTYPE_X86_64_ALL, CPU_SUBTYPE_X86_64_ALL);
SAME(MACH_CPU_SUBTYPE_X86_64_H, CPU_SUBTYPE_X86_64_H);
SAME(MACH_CPU_SUBTYPE_ARM64_ALL, CPU_SUBTYPE_ARM64_ALL);
#if defined(CPU_SUBTYPE_ARM64E)
SAME(MACH_CPU_SUBTYPE_ARM64E, CPU_SUBTYPE_ARM64E);
#endif
#if defined(CPU_SUBTYPE_PTRAUTH_ABI)
SAME(MACH_CPU_SUBTYPE_PTRAUTH_ABI, CPU_SUBTYPE_PTRAUTH_ABI);
#endif
