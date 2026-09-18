/******************************************************************************
* djinterp [core]                                                    env_jit.h
*
* djinterp JIT code-generation environment detection (root / arch-agnostic):
*   Compile-time detection of the facilities a just-in-time code generator
* needs, expressed through the unified D_ENV_JIT_* interface. Like env_web.h,
* this header is deliberately agnostic of any particular CPU: it describes the
* environment an encoder backend requires. The runtime machinery that acts on
* these answers -- the code buffer, the cache-flush operation, and the
* per-architecture instruction encoders -- lives in the jit/ modules (jit.h,
* jit_x64.h, jit_x86.h). This header contains DETECTION ONLY.
*
*   - executable-memory backend (mmap / VirtualAlloc / MAP_JIT)            [A]
*   - platform JIT policy: prohibition + W^X enforcement                   [B]
*   - instruction-cache coherency detection                                [C]
*   - target architecture + encoder-table availability                     [D]
*   - capability summaries (CAN_ALLOCATE_EXEC / AVAILABLE)                 [E]
*
*   This header derives every answer from the base D_ENV_* families already
* established by env.h (D_ENV_ARCH_*, D_ENV_OS_*, D_ENV_C_HAS_MMAP, the mobile
* D_ENV_MOBILE_NO_JIT policy flag, and the D_ENV_COMPILER_* identity); it does
* no header probing of its own and introduces no hard dependency.
*
*   NAMING CONVENTION:
*     D_ENV_JIT_HAS_[FEATURE] - 1 if available, 0 otherwise
*     D_ENV_JIT_[FEATURE]     - non-boolean detected value / identifier
*
*   Every flag is guarded with `#ifndef`, so a project may pre-define any
* D_ENV_JIT_* macro before inclusion to override detection (e.g. to force a
* backend, or to simulate a target during testing).
*
*   Requires:  env.h (for the D_ENV_ARCH_*, D_ENV_OS_*, D_ENV_C_HAS_*, and
*              D_ENV_COMPILER_* families it consumes). Like env_web.h this is
*              an OPT-IN module: it #includes env.h itself (guarded, a cheap
*              skip when already resolved) and MAY be included directly.
*
* path:      /inc/djinterp/env/jit/env_jit.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

#ifndef DJINTERP_ENV_JIT_
#define DJINTERP_ENV_JIT_ 1

// djinterp
// djinterp -- core environment detection (base D_ENV_* families)
#include "../env.h"


// ===========================================================================
// A.   EXECUTABLE-MEMORY BACKEND
// ===========================================================================
//   Every JIT needs memory it can both write and later execute. The three
// portable ways to obtain it are the POSIX mmap + mprotect pair, the Windows
// VirtualAlloc + VirtualProtect pair, and Apple's mmap(MAP_JIT) paired with
// per-thread pthread_jit_write_protect_np(). Each backend flag is derived
// from the OS/runtime classification in env.h; the selector then picks one.

// executable-memory backend identifiers
#define D_ENV_JIT_BACKEND_NONE                0
#define D_ENV_JIT_BACKEND_POSIX_MMAP          1
#define D_ENV_JIT_BACKEND_WIN32_VIRTUALALLOC  2
#define D_ENV_JIT_BACKEND_APPLE_MAP_JIT       3

// D_ENV_JIT_IS_APPLE_FAMILY
//   macro: internal helper. 1 when the detected OS is an Apple platform
// (macOS, iOS/iPadOS, or the bare Apple flag), which route executable memory
// through MAP_JIT rather than plain mmap under the hardened runtime.
#ifndef D_ENV_JIT_IS_APPLE_FAMILY
    #define D_ENV_JIT_IS_APPLE_FAMILY                                        \
        ( ((D_ENV_OS_ID) == D_ENV_OS_FLAG_MACOS) ||                          \
          ((D_ENV_OS_ID) == D_ENV_OS_FLAG_APPLE) ||                          \
          ((D_ENV_OS_ID) == D_ENV_OS_FLAG_IOS) )
#endif

// D_ENV_JIT_HAS_POSIX_MMAP
//   feature: the POSIX mmap + mprotect path for executable memory is
// available (an alias of the C-runtime mmap flag from env_c_lib.h).
#ifndef D_ENV_JIT_HAS_POSIX_MMAP
    #if D_ENV_C_HAS_MMAP
        #define D_ENV_JIT_HAS_POSIX_MMAP 1
    #else
        #define D_ENV_JIT_HAS_POSIX_MMAP 0
    #endif
#endif

// D_ENV_JIT_HAS_WIN32_VIRTUALALLOC
//   feature: the Windows VirtualAlloc + VirtualProtect path is available.
#ifndef D_ENV_JIT_HAS_WIN32_VIRTUALALLOC
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_JIT_HAS_WIN32_VIRTUALALLOC 1
    #else
        #define D_ENV_JIT_HAS_WIN32_VIRTUALALLOC 0
    #endif
#endif

// D_ENV_JIT_HAS_APPLE_MAP_JIT
//   feature: the Apple mmap(MAP_JIT) path is available (Apple-family target).
#ifndef D_ENV_JIT_HAS_APPLE_MAP_JIT
    #if D_ENV_JIT_IS_APPLE_FAMILY
        #define D_ENV_JIT_HAS_APPLE_MAP_JIT 1
    #else
        #define D_ENV_JIT_HAS_APPLE_MAP_JIT 0
    #endif
#endif

// D_ENV_JIT_HAS_EXEC_MEM
//   feature: 1 if ANY executable-memory backend is available.
#ifndef D_ENV_JIT_HAS_EXEC_MEM
    #if ( D_ENV_JIT_HAS_POSIX_MMAP        ||                                 \
          D_ENV_JIT_HAS_WIN32_VIRTUALALLOC ||                                \
          D_ENV_JIT_HAS_APPLE_MAP_JIT )
        #define D_ENV_JIT_HAS_EXEC_MEM 1
    #else
        #define D_ENV_JIT_HAS_EXEC_MEM 0
    #endif
#endif

// D_ENV_JIT_BACKEND
//   feature: the selected executable-memory backend identifier. Apple's
// MAP_JIT path wins on Apple targets (it is mandatory under the hardened
// runtime), then Windows, then the generic POSIX mmap path.
#ifndef D_ENV_JIT_BACKEND
    #if D_ENV_JIT_HAS_APPLE_MAP_JIT
        #define D_ENV_JIT_BACKEND D_ENV_JIT_BACKEND_APPLE_MAP_JIT
    #elif D_ENV_JIT_HAS_WIN32_VIRTUALALLOC
        #define D_ENV_JIT_BACKEND D_ENV_JIT_BACKEND_WIN32_VIRTUALALLOC
    #elif D_ENV_JIT_HAS_POSIX_MMAP
        #define D_ENV_JIT_BACKEND D_ENV_JIT_BACKEND_POSIX_MMAP
    #else
        #define D_ENV_JIT_BACKEND D_ENV_JIT_BACKEND_NONE
    #endif
#endif

// D_ENV_JIT_BACKEND_NAME
//   feature: human-readable name of the selected backend.
#ifndef D_ENV_JIT_BACKEND_NAME
    #if (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_APPLE_MAP_JIT)
        #define D_ENV_JIT_BACKEND_NAME "mmap(MAP_JIT)"
    #elif (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_WIN32_VIRTUALALLOC)
        #define D_ENV_JIT_BACKEND_NAME "VirtualAlloc"
    #elif (D_ENV_JIT_BACKEND == D_ENV_JIT_BACKEND_POSIX_MMAP)
        #define D_ENV_JIT_BACKEND_NAME "mmap"
    #else
        #define D_ENV_JIT_BACKEND_NAME "none"
    #endif
#endif


// ===========================================================================
// B.   PLATFORM JIT POLICY
// ===========================================================================
//   Some platforms forbid JIT outright for ordinary applications, and some
// enforce W^X: a page may be writable or executable but never both at once,
// so generated code must be written through a read-write mapping and then
// flipped to read-execute before it is called. These flags surface both
// constraints so a higher layer can refuse or adapt at compile time.

// D_ENV_JIT_PROHIBITED
//   feature: 1 when the platform forbids JIT for general apps. Consumes the
// mobile-Apple policy flag D_ENV_MOBILE_NO_JIT when env_ios.h has established
// it (iOS / tvOS / watchOS / visionOS App Store rules); degrades to 0 on
// platforms where no such prohibition is known.
#ifndef D_ENV_JIT_PROHIBITED
    #if ( defined(D_ENV_MOBILE_NO_JIT) && D_ENV_MOBILE_NO_JIT )
        #define D_ENV_JIT_PROHIBITED 1
    #else
        #define D_ENV_JIT_PROHIBITED 0
    #endif
#endif

// D_ENV_JIT_REQUIRES_WX
//   feature: 1 when the platform mandates W^X, so RWX pages are unavailable
// and the write-then-protect dance is required rather than optional. True on
// the Apple family (MAP_JIT) and on OpenBSD (whose mmap refuses PROT_WRITE |
// PROT_EXEC). Note this is a hard requirement flag, not a recommendation:
// writing through RW then flipping to RX is good practice everywhere.
#ifndef D_ENV_JIT_REQUIRES_WX
    #if ( D_ENV_JIT_IS_APPLE_FAMILY ||                                       \
          ((D_ENV_OS_ID) == D_ENV_OS_FLAG_BSD_OPEN) )
        #define D_ENV_JIT_REQUIRES_WX 1
    #else
        #define D_ENV_JIT_REQUIRES_WX 0
    #endif
#endif

// D_ENV_JIT_HAS_APPLE_WX_TOGGLE
//   feature: 1 when the Apple per-thread write-protect toggle
// (pthread_jit_write_protect_np) is the mechanism used to switch a MAP_JIT
// region between writable and executable. Apple-family targets only.
#ifndef D_ENV_JIT_HAS_APPLE_WX_TOGGLE
    #if D_ENV_JIT_IS_APPLE_FAMILY
        #define D_ENV_JIT_HAS_APPLE_WX_TOGGLE 1
    #else
        #define D_ENV_JIT_HAS_APPLE_WX_TOGGLE 0
    #endif
#endif


// ===========================================================================
// C.   INSTRUCTION-CACHE COHERENCY
// ===========================================================================
//   On architectures whose instruction and data caches are not coherent,
// freshly written code must be flushed before it is executed. These two flags
// detect whether that is necessary and whether the compiler offers a builtin
// to do it. The flush OPERATION itself (D_JIT_CLEAR_CACHE) lives in jit.h,
// with the code that emits; keeping it out of this header preserves the
// detection-only contract.

// D_ENV_JIT_NEEDS_ICACHE_FLUSH
//   feature: 1 when generated code must be explicitly flushed to the
// instruction stream before execution. 0 on the x86 family (coherent caches);
// 1 otherwise, which conservatively covers ARM, ARM64, RISC-V, PowerPC, MIPS,
// SPARC, and any unrecognised target (the flush is harmless where unneeded).
#ifndef D_ENV_JIT_NEEDS_ICACHE_FLUSH
    #if D_ENV_ARCH_IS_X86_FAMILY
        #define D_ENV_JIT_NEEDS_ICACHE_FLUSH 0
    #else
        #define D_ENV_JIT_NEEDS_ICACHE_FLUSH 1
    #endif
#endif

// D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE
//   feature: 1 when the compiler provides __builtin___clear_cache (GCC and
// Clang). MSVC has no equivalent builtin; callers there use FlushInstruction-
// Cache from the Windows API instead.
#ifndef D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE
    #if ( defined(D_ENV_COMPILER_GCC) ||                                     \
          defined(D_ENV_COMPILER_CLANG) )
        #define D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE 1
    #else
        #define D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE 0
    #endif
#endif


// ===========================================================================
// D.   TARGET ARCHITECTURE + ENCODER AVAILABILITY
// ===========================================================================
//   Which per-architecture opcode table applies to the current target, and
// whether djinterp actually ships one for it. The TARGET_* flags mirror the
// D_ENV_ARCH_* family; HAS_ENCODER is 1 where a jit/ encoder module exists.
// x86-64 (jit_x64.h), x86 (jit_x86.h), AArch64 (jit_arm64.h) and 32-bit ARM
// (jit_arm.h) all qualify.

// D_ENV_JIT_TARGET_X64
//   feature: 1 when generating for x86-64 (alias of D_ENV_ARCH_X64).
#ifndef D_ENV_JIT_TARGET_X64
    #if defined(D_ENV_ARCH_X64)
        #define D_ENV_JIT_TARGET_X64 1
    #else
        #define D_ENV_JIT_TARGET_X64 0
    #endif
#endif

// D_ENV_JIT_TARGET_X86
//   feature: 1 when generating for 32-bit x86 (alias of D_ENV_ARCH_X86).
#ifndef D_ENV_JIT_TARGET_X86
    #if defined(D_ENV_ARCH_X86)
        #define D_ENV_JIT_TARGET_X86 1
    #else
        #define D_ENV_JIT_TARGET_X86 0
    #endif
#endif

// D_ENV_JIT_TARGET_ARM64
//   feature: 1 when generating for AArch64 (alias of D_ENV_ARCH_ARM64).
#ifndef D_ENV_JIT_TARGET_ARM64
    #if defined(D_ENV_ARCH_ARM64)
        #define D_ENV_JIT_TARGET_ARM64 1
    #else
        #define D_ENV_JIT_TARGET_ARM64 0
    #endif
#endif

// D_ENV_JIT_TARGET_ARM
//   feature: 1 when generating for 32-bit ARM / A32 (alias of D_ENV_ARCH_ARM).
#ifndef D_ENV_JIT_TARGET_ARM
    #if defined(D_ENV_ARCH_ARM)
        #define D_ENV_JIT_TARGET_ARM 1
    #else
        #define D_ENV_JIT_TARGET_ARM 0
    #endif
#endif

// D_ENV_JIT_HAS_ENCODER
//   feature: 1 when a djinterp instruction-encoding module is available for
// the current target (x86-64, 32-bit x86, AArch64, or 32-bit ARM).
#ifndef D_ENV_JIT_HAS_ENCODER
    #if ( D_ENV_JIT_TARGET_X64   ||                                          \
          D_ENV_JIT_TARGET_X86   ||                                          \
          D_ENV_JIT_TARGET_ARM64 ||                                          \
          D_ENV_JIT_TARGET_ARM )
        #define D_ENV_JIT_HAS_ENCODER 1
    #else
        #define D_ENV_JIT_HAS_ENCODER 0
    #endif
#endif


// ===========================================================================
// E.   CAPABILITY SUMMARY
// ===========================================================================
//   The two headline questions a higher layer asks the environment.

// D_ENV_JIT_CAN_ALLOCATE_EXEC
//   feature: 1 if the platform can obtain executable memory at all -- some
// backend exists and JIT is not prohibited. The baseline gate for any code
// generation.
#ifndef D_ENV_JIT_CAN_ALLOCATE_EXEC
    #if ( D_ENV_JIT_HAS_EXEC_MEM &&                                          \
          !D_ENV_JIT_PROHIBITED )
        #define D_ENV_JIT_CAN_ALLOCATE_EXEC 1
    #else
        #define D_ENV_JIT_CAN_ALLOCATE_EXEC 0
    #endif
#endif

// D_ENV_JIT_AVAILABLE
//   feature: 1 if djinterp can both obtain executable memory and emit code
// for this target -- CAN_ALLOCATE_EXEC and an encoder module are both present.
#ifndef D_ENV_JIT_AVAILABLE
    #if ( D_ENV_JIT_CAN_ALLOCATE_EXEC &&                                     \
          D_ENV_JIT_HAS_ENCODER )
        #define D_ENV_JIT_AVAILABLE 1
    #else
        #define D_ENV_JIT_AVAILABLE 0
    #endif
#endif


#endif  // DJINTERP_ENV_JIT_
