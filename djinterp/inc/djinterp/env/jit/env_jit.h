/*******************************************************************************
* djinterp [env]                                                       env_jit.h
*
* djinterp JIT code-generation environment detection.
*   Compile-time detection of the facilities a just-in-time code generator
* needs, expressed through the unified D_ENV_JIT_* interface. Like env_net.h,
* it is deliberately agnostic of any particular backend, here any particular
* CPU: it describes the environment an encoder backend requires. The runtime
* machinery that acts on these answers -- the code buffer, the cache-flush
* operation, and the per-architecture instruction encoders -- lives in the
* jit/ modules (jit.h and the jit_* encoders). This header contains detection
* only. It covers:
*     - the executable-memory backend (mmap, VirtualAlloc, MAP_JIT)     [1]
*     - platform JIT policy: prohibition and W^X enforcement            [2]
*     - instruction-cache coherency                                     [3]
*     - the target architecture and encoder availability                [4]
*     - capability summaries (CAN_ALLOCATE_EXEC, AVAILABLE)             [5]
*   It derives every answer from the base D_ENV_* families env.h establishes
* (D_ENV_ARCH_*, D_ENV_OS_*, D_ENV_C_HAS_MMAP, and the D_ENV_COMPILER_*
* identity), plus the mobile D_ENV_MOBILE_NO_JIT policy flag when env_ios.h
* has defined it; it does no header probing of its own.
*   Naming: D_ENV_JIT_HAS_<FEATURE> is 1 if available, 0 otherwise;
* D_ENV_JIT_<FEATURE> is a non-boolean detected value or identifier.
*   Every flag is #ifndef-guarded, so a project may pre-define any D_ENV_JIT_*
* macro before inclusion to override detection, for example to force a backend
* or to simulate a target during testing.
*   It requires env.h, for the D_ENV_ARCH_*, D_ENV_OS_*, D_ENV_C_HAS_*, and
* D_ENV_COMPILER_* families it reads, and includes it itself. Like env_net.h,
* it is an opt-in module and may be included directly.
*
*
* path:      /inc/djinterp/env/jit/env_jit.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.16
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  EXECUTABLE-MEMORY BACKEND
    -------------------------
    1.  Backend identifiers
         1.  D_ENV_JIT_BACKEND_*
              1.  D_ENV_JIT_BACKEND_NONE
              2.  D_ENV_JIT_BACKEND_POSIX_MMAP
              3.  D_ENV_JIT_BACKEND_WIN32_VIRTUALALLOC
              4.  D_ENV_JIT_BACKEND_APPLE_MAP_JIT
    2.  Backend detection
         1.  D_ENV_JIT_IS_APPLE_FAMILY
         2.  D_ENV_JIT_HAS_POSIX_MMAP
         3.  D_ENV_JIT_HAS_WIN32_VIRTUALALLOC
         4.  D_ENV_JIT_HAS_APPLE_MAP_JIT
         5.  D_ENV_JIT_HAS_EXEC_MEM
    3.  Backend selection
         1.  D_ENV_JIT_BACKEND
         2.  D_ENV_JIT_BACKEND_NAME
2.  PLATFORM JIT POLICY
    -------------------
    1.  Prohibition and W^X
         1.  D_ENV_JIT_PROHIBITED
         2.  D_ENV_JIT_REQUIRES_WX
         3.  D_ENV_JIT_HAS_APPLE_WX_TOGGLE
3.  INSTRUCTION-CACHE COHERENCY
    ---------------------------
    1.  Cache flushing
         1.  D_ENV_JIT_NEEDS_ICACHE_FLUSH
         2.  D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE
4.  TARGET ARCHITECTURE AND ENCODERS
    --------------------------------
    1.  Target architecture
         1.  D_ENV_JIT_TARGET_X64
         2.  D_ENV_JIT_TARGET_X86
         3.  D_ENV_JIT_TARGET_ARM64
         4.  D_ENV_JIT_TARGET_ARM
         5.  D_ENV_JIT_TARGET_RISCV32
         6.  D_ENV_JIT_TARGET_RISCV64
         7.  D_ENV_JIT_TARGET_MIPS32
         8.  D_ENV_JIT_TARGET_MIPS64
         9.  D_ENV_JIT_TARGET_POWERPC32
         10. D_ENV_JIT_TARGET_POWERPC64
         11. D_ENV_JIT_TARGET_SPARC32
         12. D_ENV_JIT_TARGET_SPARC64
         13. D_ENV_JIT_TARGET_S390X
    2.  Encoder availability
         1.  D_ENV_JIT_HAS_ENCODER
5.  CAPABILITY SUMMARY
    ------------------
    1.  Rolled-up capabilities
         1.  D_ENV_JIT_CAN_ALLOCATE_EXEC
         2.  D_ENV_JIT_AVAILABLE
*/

#ifndef DJINTERP_ENV_JIT_ENV_JIT_H
#define DJINTERP_ENV_JIT_ENV_JIT_H 1

// djinterp
#include "../env.h"  // D_ENV_ARCH_*, D_ENV_OS_*, D_ENV_COMPILER_*,
                     // D_ENV_C_HAS_MMAP


//==============================================================================
// 1.  EXECUTABLE-MEMORY BACKEND
//==============================================================================
// Every JIT needs memory it can both write and later execute. The three
// portable ways to obtain it are the POSIX mmap + mprotect pair, the Windows
// VirtualAlloc + VirtualProtect pair, and Apple's mmap(MAP_JIT) paired with
// per-thread pthread_jit_write_protect_np(). Each backend flag is derived
// from the OS/runtime classification in env.h; the selector then picks one.


// 1.1    Backend identifiers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_JIT_BACKEND_*
//   constant: identifiers for D_ENV_JIT_BACKEND.

// 1.1.1.1
// D_ENV_JIT_BACKEND_NONE
//   constant: identifies no executable-memory backend.
#define D_ENV_JIT_BACKEND_NONE               0

// 1.1.1.2
// D_ENV_JIT_BACKEND_POSIX_MMAP
//   constant: identifies the POSIX mmap + mprotect pair.
#define D_ENV_JIT_BACKEND_POSIX_MMAP         1

// 1.1.1.3
// D_ENV_JIT_BACKEND_WIN32_VIRTUALALLOC
//   constant: identifies the Windows VirtualAlloc + VirtualProtect pair.
#define D_ENV_JIT_BACKEND_WIN32_VIRTUALALLOC 2

// 1.1.1.4
// D_ENV_JIT_BACKEND_APPLE_MAP_JIT
//   constant: identifies Apple's mmap(MAP_JIT) with
// pthread_jit_write_protect_np.
#define D_ENV_JIT_BACKEND_APPLE_MAP_JIT      3

// 1.2    Backend detection
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_JIT_IS_APPLE_FAMILY
//   macro: internal helper. 1 when the detected OS is an Apple platform
// (macOS, iOS/iPadOS, or the bare Apple flag), which route executable memory
// through MAP_JIT rather than plain mmap under the hardened runtime.
#ifndef D_ENV_JIT_IS_APPLE_FAMILY
    #define D_ENV_JIT_IS_APPLE_FAMILY                                          \
        ( ((D_ENV_OS_ID) == D_ENV_OS_FLAG_MACOS) ||                            \
          ((D_ENV_OS_ID) == D_ENV_OS_FLAG_APPLE) ||                            \
          ((D_ENV_OS_ID) == D_ENV_OS_FLAG_IOS) )
#endif  // D_ENV_JIT_IS_APPLE_FAMILY

// 1.2.2
// D_ENV_JIT_HAS_POSIX_MMAP
//   feature: the POSIX mmap + mprotect path for executable memory is
// available (an alias of the C-runtime mmap flag from env_c_lib.h).
#ifndef D_ENV_JIT_HAS_POSIX_MMAP
    #if D_ENV_C_HAS_MMAP
        #define D_ENV_JIT_HAS_POSIX_MMAP 1
    #else
        #define D_ENV_JIT_HAS_POSIX_MMAP 0
    #endif
#endif  // D_ENV_JIT_HAS_POSIX_MMAP

// 1.2.3
// D_ENV_JIT_HAS_WIN32_VIRTUALALLOC
//   feature: the Windows VirtualAlloc + VirtualProtect path is available.
#ifndef D_ENV_JIT_HAS_WIN32_VIRTUALALLOC
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_JIT_HAS_WIN32_VIRTUALALLOC 1
    #else
        #define D_ENV_JIT_HAS_WIN32_VIRTUALALLOC 0
    #endif
#endif  // D_ENV_JIT_HAS_WIN32_VIRTUALALLOC

// 1.2.4
// D_ENV_JIT_HAS_APPLE_MAP_JIT
//   feature: the Apple mmap(MAP_JIT) path is available (Apple-family target).
#ifndef D_ENV_JIT_HAS_APPLE_MAP_JIT
    #if D_ENV_JIT_IS_APPLE_FAMILY
        #define D_ENV_JIT_HAS_APPLE_MAP_JIT 1
    #else
        #define D_ENV_JIT_HAS_APPLE_MAP_JIT 0
    #endif
#endif  // D_ENV_JIT_HAS_APPLE_MAP_JIT

// 1.2.5
// D_ENV_JIT_HAS_EXEC_MEM
//   feature: 1 if ANY executable-memory backend is available.
#ifndef D_ENV_JIT_HAS_EXEC_MEM
    #if ( (D_ENV_JIT_HAS_POSIX_MMAP)         ||                                \
          (D_ENV_JIT_HAS_WIN32_VIRTUALALLOC) ||                                \
          (D_ENV_JIT_HAS_APPLE_MAP_JIT) )
        #define D_ENV_JIT_HAS_EXEC_MEM 1
    #else
        #define D_ENV_JIT_HAS_EXEC_MEM 0
    #endif
#endif  // D_ENV_JIT_HAS_EXEC_MEM

// 1.3    Backend selection
//------------------------------------------------------------------------------
// 1.3.1
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
#endif  // D_ENV_JIT_BACKEND

// 1.3.2
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
#endif  // D_ENV_JIT_BACKEND_NAME


//==============================================================================
// 2.  PLATFORM JIT POLICY
//==============================================================================
// Some platforms forbid JIT outright for ordinary applications, and some
// enforce W^X: a page may be writable or executable but never both at once,
// so generated code must be written through a read-write mapping and then
// flipped to read-execute before it is called. These flags surface both
// constraints so a higher layer can refuse or adapt at compile time.


// 2.1    Prohibition and W^X
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_JIT_PROHIBITED
//   feature: 1 when the platform forbids JIT for general apps. Consumes the
// mobile-Apple policy flag D_ENV_MOBILE_NO_JIT when env_ios.h has established
// it (iOS / tvOS / watchOS / visionOS App Store rules); degrades to 0 on
// platforms where no such prohibition is known.
#ifndef D_ENV_JIT_PROHIBITED
    #if ( (defined(D_ENV_MOBILE_NO_JIT)) &&                                    \
          (D_ENV_MOBILE_NO_JIT) )
        #define D_ENV_JIT_PROHIBITED 1
    #else
        #define D_ENV_JIT_PROHIBITED 0
    #endif
#endif  // D_ENV_JIT_PROHIBITED

// 2.1.2
// D_ENV_JIT_REQUIRES_WX
//   feature: 1 when the platform mandates W^X, so RWX pages are unavailable
// and the write-then-protect dance is required rather than optional. True on
// the Apple family (MAP_JIT) and on OpenBSD (whose mmap refuses PROT_WRITE |
// PROT_EXEC). Note this is a hard requirement flag, not a recommendation:
// writing through RW then flipping to RX is good practice everywhere.
#ifndef D_ENV_JIT_REQUIRES_WX
    #if ( (D_ENV_JIT_IS_APPLE_FAMILY) ||                                       \
          ((D_ENV_OS_ID) == D_ENV_OS_FLAG_BSD_OPEN) )
        #define D_ENV_JIT_REQUIRES_WX 1
    #else
        #define D_ENV_JIT_REQUIRES_WX 0
    #endif
#endif  // D_ENV_JIT_REQUIRES_WX

// 2.1.3
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
#endif  // D_ENV_JIT_HAS_APPLE_WX_TOGGLE


//==============================================================================
// 3.  INSTRUCTION-CACHE COHERENCY
//==============================================================================
// On architectures whose instruction and data caches are not coherent,
// freshly written code must be flushed before it is executed. These two flags
// detect whether that is necessary and whether the compiler offers a builtin
// to do it. The flush OPERATION itself (D_JIT_CLEAR_CACHE) lives in jit.h,
// with the code that emits; keeping it out of this header preserves the
// detection-only contract.


// 3.1    Cache flushing
//------------------------------------------------------------------------------
// 3.1.1
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
#endif  // D_ENV_JIT_NEEDS_ICACHE_FLUSH

// 3.1.2
// D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE
//   feature: 1 when the compiler provides __builtin___clear_cache (GCC and
// Clang). MSVC has no equivalent builtin; callers there use
// FlushInstructionCache from the Windows API instead.
#ifndef D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE
    #if ( (defined(D_ENV_COMPILER_GCC)) ||                                     \
          (defined(D_ENV_COMPILER_CLANG)) )
        #define D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE 1
    #else
        #define D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE 0
    #endif
#endif  // D_ENV_JIT_HAS_BUILTIN_CLEAR_CACHE


//==============================================================================
// 4.  TARGET ARCHITECTURE AND ENCODERS
//==============================================================================
// Which per-architecture opcode table applies to the current target, and
// whether djinterp actually ships one for it. The TARGET_* flags mirror the
// D_ENV_ARCH_* family; HAS_ENCODER is 1 where a jit/ encoder module exists.
// Encoders exist for x86-64 (jit_x64.h), x86 (jit_x86.h), AArch64
// (jit_arm64.h), 32-bit ARM (jit_arm.h), and the RISC-V, MIPS, PowerPC,
// SPARC, and s390x families (jit_riscv, jit_mips, jit_ppc, jit_sparc, and
// jit_s390x, each serving both widths where the family has two).


// 4.1    Target architecture
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_JIT_TARGET_X64
//   feature: 1 when generating for x86-64 (alias of D_ENV_ARCH_X64).
#ifndef D_ENV_JIT_TARGET_X64
    #if defined(D_ENV_ARCH_X64)
        #define D_ENV_JIT_TARGET_X64 1
    #else
        #define D_ENV_JIT_TARGET_X64 0
    #endif
#endif  // D_ENV_JIT_TARGET_X64

// 4.1.2
// D_ENV_JIT_TARGET_X86
//   feature: 1 when generating for 32-bit x86 (alias of D_ENV_ARCH_X86).
#ifndef D_ENV_JIT_TARGET_X86
    #if defined(D_ENV_ARCH_X86)
        #define D_ENV_JIT_TARGET_X86 1
    #else
        #define D_ENV_JIT_TARGET_X86 0
    #endif
#endif  // D_ENV_JIT_TARGET_X86

// 4.1.3
// D_ENV_JIT_TARGET_ARM64
//   feature: 1 when generating for AArch64 (alias of D_ENV_ARCH_ARM64).
#ifndef D_ENV_JIT_TARGET_ARM64
    #if defined(D_ENV_ARCH_ARM64)
        #define D_ENV_JIT_TARGET_ARM64 1
    #else
        #define D_ENV_JIT_TARGET_ARM64 0
    #endif
#endif  // D_ENV_JIT_TARGET_ARM64

// 4.1.4
// D_ENV_JIT_TARGET_ARM
//   feature: 1 when generating for 32-bit ARM / A32 (alias of D_ENV_ARCH_ARM).
#ifndef D_ENV_JIT_TARGET_ARM
    #if defined(D_ENV_ARCH_ARM)
        #define D_ENV_JIT_TARGET_ARM 1
    #else
        #define D_ENV_JIT_TARGET_ARM 0
    #endif
#endif  // D_ENV_JIT_TARGET_ARM

// 4.1.5
// D_ENV_JIT_TARGET_RISCV32
//   feature: 1 when generating for RV32I (alias of D_ENV_ARCH_RISCV32). The
// jit_riscv encoder serves both RV32 and RV64.
#ifndef D_ENV_JIT_TARGET_RISCV32
    #if defined(D_ENV_ARCH_RISCV32)
        #define D_ENV_JIT_TARGET_RISCV32 1
    #else
        #define D_ENV_JIT_TARGET_RISCV32 0
    #endif
#endif  // D_ENV_JIT_TARGET_RISCV32

// 4.1.6
// D_ENV_JIT_TARGET_RISCV64
//   feature: 1 when generating for RV64I (alias of D_ENV_ARCH_RISCV64).
#ifndef D_ENV_JIT_TARGET_RISCV64
    #if defined(D_ENV_ARCH_RISCV64)
        #define D_ENV_JIT_TARGET_RISCV64 1
    #else
        #define D_ENV_JIT_TARGET_RISCV64 0
    #endif
#endif  // D_ENV_JIT_TARGET_RISCV64

// 4.1.7
// D_ENV_JIT_TARGET_MIPS32
//   feature: 1 when generating for MIPS32 (alias of D_ENV_ARCH_MIPS32). The
// jit_mips encoder serves both MIPS32 and MIPS64.
#ifndef D_ENV_JIT_TARGET_MIPS32
    #if defined(D_ENV_ARCH_MIPS32)
        #define D_ENV_JIT_TARGET_MIPS32 1
    #else
        #define D_ENV_JIT_TARGET_MIPS32 0
    #endif
#endif  // D_ENV_JIT_TARGET_MIPS32

// 4.1.8
// D_ENV_JIT_TARGET_MIPS64
//   feature: 1 when generating for MIPS64 (alias of D_ENV_ARCH_MIPS64).
#ifndef D_ENV_JIT_TARGET_MIPS64
    #if defined(D_ENV_ARCH_MIPS64)
        #define D_ENV_JIT_TARGET_MIPS64 1
    #else
        #define D_ENV_JIT_TARGET_MIPS64 0
    #endif
#endif  // D_ENV_JIT_TARGET_MIPS64

// 4.1.9
// D_ENV_JIT_TARGET_POWERPC32
//   feature: 1 when generating for 32-bit PowerPC (alias of
// D_ENV_ARCH_POWERPC32). The jit_ppc encoder serves both PPC32 and PPC64.
#ifndef D_ENV_JIT_TARGET_POWERPC32
    #if defined(D_ENV_ARCH_POWERPC32)
        #define D_ENV_JIT_TARGET_POWERPC32 1
    #else
        #define D_ENV_JIT_TARGET_POWERPC32 0
    #endif
#endif  // D_ENV_JIT_TARGET_POWERPC32

// 4.1.10
// D_ENV_JIT_TARGET_POWERPC64
//   feature: 1 when generating for 64-bit PowerPC (alias of
// D_ENV_ARCH_POWERPC64).
#ifndef D_ENV_JIT_TARGET_POWERPC64
    #if defined(D_ENV_ARCH_POWERPC64)
        #define D_ENV_JIT_TARGET_POWERPC64 1
    #else
        #define D_ENV_JIT_TARGET_POWERPC64 0
    #endif
#endif  // D_ENV_JIT_TARGET_POWERPC64

// 4.1.11
// D_ENV_JIT_TARGET_SPARC32
//   feature: 1 when generating for 32-bit SPARC / SPARC V8 (alias of
// D_ENV_ARCH_SPARC32). The jit_sparc encoder serves both V8 and V9.
#ifndef D_ENV_JIT_TARGET_SPARC32
    #if defined(D_ENV_ARCH_SPARC32)
        #define D_ENV_JIT_TARGET_SPARC32 1
    #else
        #define D_ENV_JIT_TARGET_SPARC32 0
    #endif
#endif  // D_ENV_JIT_TARGET_SPARC32

// 4.1.12
// D_ENV_JIT_TARGET_SPARC64
//   feature: 1 when generating for 64-bit SPARC / SPARC V9 (alias of
// D_ENV_ARCH_SPARC64).
#ifndef D_ENV_JIT_TARGET_SPARC64
    #if defined(D_ENV_ARCH_SPARC64)
        #define D_ENV_JIT_TARGET_SPARC64 1
    #else
        #define D_ENV_JIT_TARGET_SPARC64 0
    #endif
#endif  // D_ENV_JIT_TARGET_SPARC64

// 4.1.13
// D_ENV_JIT_TARGET_S390X
//   feature: 1 when generating for 64-bit IBM z/Architecture (alias of
// D_ENV_ARCH_S390X). The jit_s390x encoder targets z/Architecture; the
// 31-bit ESA/390 subset (D_ENV_ARCH_S390 without S390X) is not covered.
#ifndef D_ENV_JIT_TARGET_S390X
    #if defined(D_ENV_ARCH_S390X)
        #define D_ENV_JIT_TARGET_S390X 1
    #else
        #define D_ENV_JIT_TARGET_S390X 0
    #endif
#endif  // D_ENV_JIT_TARGET_S390X

// 4.2    Encoder availability
//------------------------------------------------------------------------------
// 4.2.1
// D_ENV_JIT_HAS_ENCODER
//   feature: 1 when a djinterp instruction-encoding module is available for
// the target (x86-64/x86, ARM64/ARM, RISC-V, MIPS, PowerPC, SPARC, s390x).
#ifndef D_ENV_JIT_HAS_ENCODER
    #if ( (D_ENV_JIT_TARGET_X64)       ||                                      \
          (D_ENV_JIT_TARGET_X86)       ||                                      \
          (D_ENV_JIT_TARGET_ARM64)     ||                                      \
          (D_ENV_JIT_TARGET_ARM)       ||                                      \
          (D_ENV_JIT_TARGET_RISCV32)   ||                                      \
          (D_ENV_JIT_TARGET_RISCV64)   ||                                      \
          (D_ENV_JIT_TARGET_MIPS32)    ||                                      \
          (D_ENV_JIT_TARGET_MIPS64)    ||                                      \
          (D_ENV_JIT_TARGET_POWERPC32) ||                                      \
          (D_ENV_JIT_TARGET_POWERPC64) ||                                      \
          (D_ENV_JIT_TARGET_SPARC32)   ||                                      \
          (D_ENV_JIT_TARGET_SPARC64)   ||                                      \
          (D_ENV_JIT_TARGET_S390X) )
        #define D_ENV_JIT_HAS_ENCODER 1
    #else
        #define D_ENV_JIT_HAS_ENCODER 0
    #endif
#endif  // D_ENV_JIT_HAS_ENCODER


//==============================================================================
// 5.  CAPABILITY SUMMARY
//==============================================================================
// The two headline questions a higher layer asks the environment.


// 5.1    Rolled-up capabilities
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_JIT_CAN_ALLOCATE_EXEC
//   feature: 1 if the platform can obtain executable memory at all -- some
// backend exists and JIT is not prohibited. The baseline gate for any code
// generation.
#ifndef D_ENV_JIT_CAN_ALLOCATE_EXEC
    #if ( (D_ENV_JIT_HAS_EXEC_MEM) &&                                          \
          (!D_ENV_JIT_PROHIBITED) )
        #define D_ENV_JIT_CAN_ALLOCATE_EXEC 1
    #else
        #define D_ENV_JIT_CAN_ALLOCATE_EXEC 0
    #endif
#endif  // D_ENV_JIT_CAN_ALLOCATE_EXEC

// 5.1.2
// D_ENV_JIT_AVAILABLE
//   feature: 1 if djinterp can both obtain executable memory and emit code
// for this target -- CAN_ALLOCATE_EXEC and an encoder module are both present.
#ifndef D_ENV_JIT_AVAILABLE
    #if ( (D_ENV_JIT_CAN_ALLOCATE_EXEC) &&                                     \
          (D_ENV_JIT_HAS_ENCODER) )
        #define D_ENV_JIT_AVAILABLE 1
    #else
        #define D_ENV_JIT_AVAILABLE 0
    #endif
#endif  // D_ENV_JIT_AVAILABLE


#endif  // DJINTERP_ENV_JIT_ENV_JIT_H
