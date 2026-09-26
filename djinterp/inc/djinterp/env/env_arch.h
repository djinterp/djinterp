/*******************************************************************************
* djinterp [env]                                                      env_arch.h
*
* djinterp CPU architecture detection.
*   Compile-time detection of the CPU architecture family, bit width, and
* endianness, exposing the D_ENV_ARCH_* interface and the family / bit-width /
* endianness helper flags.
*   Requires cfg_env.h (for the D_CFG_ENV_* switches). This header is an
* internal component of env.h and is #included by it; do not #include it
* directly.
*
* path:      /inc/djinterp/env/env_arch.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.03.27
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DEFINED CONSTANTS
    -----------------
    1.  Architecture types
         1.  Architecture type identifiers
              1.  D_ENV_ARCH_TYPE_X86
              2.  D_ENV_ARCH_TYPE_X64
              3.  D_ENV_ARCH_TYPE_ARM
              4.  D_ENV_ARCH_TYPE_ARM64
              5.  D_ENV_ARCH_TYPE_RISCV
              6.  D_ENV_ARCH_TYPE_POWERPC
              7.  D_ENV_ARCH_TYPE_MIPS
              8.  D_ENV_ARCH_TYPE_SPARC
              9.  D_ENV_ARCH_TYPE_S390
              10. D_ENV_ARCH_TYPE_IA64
              11. D_ENV_ARCH_TYPE_ALPHA
              12. D_ENV_ARCH_TYPE_UNKNOWN
    2.  Endianness
         1.  Endianness identifiers
              1.  D_ENV_ARCH_ENDIAN_UNKNOWN
              2.  D_ENV_ARCH_ENDIAN_LITTLE
              3.  D_ENV_ARCH_ENDIAN_BIG
2.  ARCHITECTURE DETECTION
    ----------------------
    1.  Automatic detection
         1.  Architecture cases
              1.  x86-64
              2.  x86
              3.  ARM64
              4.  ARM
              5.  RISC-V
              6.  PowerPC
              7.  MIPS
              8.  SPARC
              9.  IBM System/390
              10. Itanium (IA-64)
              11. Alpha
              12. Unknown architecture
    2.  Predefined detection
         1.  D_ENV_DETECTED_ARCH_* overrides
              1.  x86-64
              2.  x86
              3.  ARM64
              4.  ARM
              5.  RISC-V
              6.  PowerPC
              7.  MIPS
              8.  SPARC
              9.  IBM System/390
              10. Itanium (IA-64)
              11. Alpha
              12. Unknown architecture
3.  DERIVED ARCHITECTURE FLAGS
    --------------------------
    1.  Architecture families
         1.  D_ENV_ARCH_IS_X86_FAMILY
         2.  D_ENV_ARCH_IS_ARM_FAMILY
    2.  Architecture bit width
         1.  D_ENV_ARCH_IS_64BIT
         2.  D_ENV_ARCH_IS_32BIT
    3.  Architecture endianness
         1.  D_ENV_ARCH_IS_LITTLE_ENDIAN
         2.  D_ENV_ARCH_IS_BIG_ENDIAN
*/

#ifndef DJINTERP_ENV_ENV_ARCH_H
#define DJINTERP_ENV_ENV_ARCH_H 1


//==============================================================================
// 1.  DEFINED CONSTANTS
//==============================================================================
// Defines the stable identifiers used to classify architecture and endianness.


// 1.1    Architecture types
//------------------------------------------------------------------------------
// 1.1.1
// Architecture type identifiers

// 1.1.1.1
// D_ENV_ARCH_TYPE_X86
//   constant: architecture type identifier for 32-bit x86.
#define D_ENV_ARCH_TYPE_X86     0

// 1.1.1.2
// D_ENV_ARCH_TYPE_X64
//   constant: architecture type identifier for 64-bit x86.
#define D_ENV_ARCH_TYPE_X64     1

// 1.1.1.3
// D_ENV_ARCH_TYPE_ARM
//   constant: architecture type identifier for 32-bit ARM.
#define D_ENV_ARCH_TYPE_ARM     2

// 1.1.1.4
// D_ENV_ARCH_TYPE_ARM64
//   constant: architecture type identifier for 64-bit ARM.
#define D_ENV_ARCH_TYPE_ARM64   3

// 1.1.1.5
// D_ENV_ARCH_TYPE_RISCV
//   constant: architecture type identifier for RISC-V.
#define D_ENV_ARCH_TYPE_RISCV   4

// 1.1.1.6
// D_ENV_ARCH_TYPE_POWERPC
//   constant: architecture type identifier for PowerPC.
#define D_ENV_ARCH_TYPE_POWERPC 5

// 1.1.1.7
// D_ENV_ARCH_TYPE_MIPS
//   constant: architecture type identifier for MIPS.
#define D_ENV_ARCH_TYPE_MIPS    6

// 1.1.1.8
// D_ENV_ARCH_TYPE_SPARC
//   constant: architecture type identifier for SPARC.
#define D_ENV_ARCH_TYPE_SPARC   7

// 1.1.1.9
// D_ENV_ARCH_TYPE_S390
//   constant: architecture type identifier for IBM System/390.
#define D_ENV_ARCH_TYPE_S390    8

// 1.1.1.10
// D_ENV_ARCH_TYPE_IA64
//   constant: architecture type identifier for Itanium (IA-64).
#define D_ENV_ARCH_TYPE_IA64    9

// 1.1.1.11
// D_ENV_ARCH_TYPE_ALPHA
//   constant: architecture type identifier for Alpha.
#define D_ENV_ARCH_TYPE_ALPHA   10

// 1.1.1.12
// D_ENV_ARCH_TYPE_UNKNOWN
//   constant: architecture type identifier for an unknown architecture.
#define D_ENV_ARCH_TYPE_UNKNOWN 11

// 1.2    Endianness
//------------------------------------------------------------------------------
// 1.2.1
// Endianness identifiers

// 1.2.1.1
// D_ENV_ARCH_ENDIAN_UNKNOWN
//   constant: endianness identifier for an unknown byte order.
#define D_ENV_ARCH_ENDIAN_UNKNOWN 0

// 1.2.1.2
// D_ENV_ARCH_ENDIAN_LITTLE
//   constant: endianness identifier for little-endian byte order.
#define D_ENV_ARCH_ENDIAN_LITTLE  1

// 1.2.1.3
// D_ENV_ARCH_ENDIAN_BIG
//   constant: endianness identifier for big-endian byte order.
#define D_ENV_ARCH_ENDIAN_BIG     2


//==============================================================================
// 2.  ARCHITECTURE DETECTION
//==============================================================================
// Selects exactly one architecture, either from the compiler's predefined
// macros or, when automatic detection is disabled, from the
// D_ENV_DETECTED_ARCH_* overrides.


// 2.1    Automatic detection
//------------------------------------------------------------------------------
// 2.1.1
// Architecture cases

#if (D_CFG_ENV_ARCH_ENABLED)

    // 2.1.1.1
    // x86-64
    #if ( defined(_M_X64)     ||                                              \
          defined(__x86_64__) ||                                              \
          defined(__x86_64)   ||                                              \
          defined(__amd64__)  ||                                              \
          defined(__amd64) )
        // D_ENV_ARCH_X64
        //   macro: indicates that the detected architecture is x86-64.
        #define D_ENV_ARCH_X64    1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the detected architecture.
        #define D_ENV_ARCH_NAME   "x86-64"

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_X64

        // D_ENV_ARCH_BITS
        //   constant: native architecture width in bits.
        #define D_ENV_ARCH_BITS   64

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_LITTLE

    // 2.1.1.2
    // x86
    #elif ( defined(_M_IX86)   ||                                             \
            defined(__i386__)  ||                                             \
            defined(__i386)    ||                                             \
            defined(i386) )
        // D_ENV_ARCH_X86
        //   macro: indicates that the detected architecture is 32-bit x86.
        #define D_ENV_ARCH_X86     1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the detected architecture.
        #define D_ENV_ARCH_NAME    "x86"

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_X86

        // D_ENV_ARCH_BITS
        //   constant: native architecture width in bits.
        #define D_ENV_ARCH_BITS    32

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE

    // 2.1.1.3
    // ARM64
    #elif ( defined(_M_ARM64) ||                                              \
            defined(__aarch64__) )
        // D_ENV_ARCH_ARM64
        //   macro: indicates that the detected architecture is 64-bit ARM.
        #define D_ENV_ARCH_ARM64   1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the detected architecture.
        #define D_ENV_ARCH_NAME    "ARM64"

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_ARM64

        // D_ENV_ARCH_BITS
        //   constant: native architecture width in bits.
        #define D_ENV_ARCH_BITS    64

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE

    // 2.1.1.4
    // ARM
    #elif ( defined(_M_ARM)    ||                                             \
            defined(__arm__)   ||                                             \
            defined(__thumb__) )
        // D_ENV_ARCH_ARM
        //   macro: indicates that the detected architecture is 32-bit ARM.
        #define D_ENV_ARCH_ARM     1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the detected architecture.
        #define D_ENV_ARCH_NAME    "ARM"

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_ARM

        // D_ENV_ARCH_BITS
        //   constant: native architecture width in bits.
        #define D_ENV_ARCH_BITS    32

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE

    // 2.1.1.5
    // RISC-V
    #elif defined(__riscv)
        // D_ENV_ARCH_RISCV
        //   macro: indicates that the detected architecture is RISC-V.
        #define D_ENV_ARCH_RISCV 1

        #if ( defined(__riscv_xlen) &&                                        \
              (__riscv_xlen == 64) )
            // D_ENV_ARCH_RISCV64
            //   macro: indicates a 64-bit RISC-V target.
            #define D_ENV_ARCH_RISCV64 1

            // D_ENV_ARCH_NAME
            //   constant: human-readable name of the detected architecture.
            #define D_ENV_ARCH_NAME    "RISC-V 64"

            // D_ENV_ARCH_BITS
            //   constant: native architecture width in bits.
            #define D_ENV_ARCH_BITS    64
        #else
            // D_ENV_ARCH_RISCV32
            //   macro: indicates a 32-bit RISC-V target.
            #define D_ENV_ARCH_RISCV32 1

            // D_ENV_ARCH_NAME
            //   constant: human-readable name of the detected architecture.
            #define D_ENV_ARCH_NAME    "RISC-V 32"

            // D_ENV_ARCH_BITS
            //   constant: native architecture width in bits.
            #define D_ENV_ARCH_BITS    32
        #endif

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_RISCV

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_LITTLE

    // 2.1.1.6
    // PowerPC
    #elif ( defined(__powerpc__)   ||                                        \
            defined(__powerpc64__) ||                                        \
            defined(__PPC__)       ||                                        \
            defined(__PPC64__) )
        // D_ENV_ARCH_POWERPC
        //   macro: indicates that the detected architecture is PowerPC.
        #define D_ENV_ARCH_POWERPC 1

        #if defined(__powerpc64__) || defined(__PPC64__)
            // D_ENV_ARCH_POWERPC64
            //   macro: indicates a 64-bit PowerPC target.
            #define D_ENV_ARCH_POWERPC64 1

            // D_ENV_ARCH_NAME
            //   constant: human-readable name of the detected architecture.
            #define D_ENV_ARCH_NAME      "PowerPC 64"

            // D_ENV_ARCH_BITS
            //   constant: native architecture width in bits.
            #define D_ENV_ARCH_BITS      64
        #else
            // D_ENV_ARCH_POWERPC32
            //   macro: indicates a 32-bit PowerPC target.
            #define D_ENV_ARCH_POWERPC32 1

            // D_ENV_ARCH_NAME
            //   constant: human-readable name of the detected architecture.
            #define D_ENV_ARCH_NAME      "PowerPC 32"

            // D_ENV_ARCH_BITS
            //   constant: native architecture width in bits.
            #define D_ENV_ARCH_BITS      32
        #endif

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_POWERPC

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_BIG

    // 2.1.1.7
    // MIPS
    #elif ( defined(__mips__) ||                                               \
            defined(__mips)   ||                                               \
            defined(__MIPS__) )
        // D_ENV_ARCH_MIPS
        //   macro: indicates that the detected architecture is MIPS.
        #define D_ENV_ARCH_MIPS 1

        #if ( defined(_MIPS_ARCH_MIPS64) ||                                    \
              defined(__mips64) )
            // D_ENV_ARCH_MIPS64
            //   macro: indicates a 64-bit MIPS target.
            #define D_ENV_ARCH_MIPS64 1

            // D_ENV_ARCH_NAME
            //   constant: human-readable name of the detected architecture.
            #define D_ENV_ARCH_NAME   "MIPS 64"

            // D_ENV_ARCH_BITS
            //   constant: native architecture width in bits.
            #define D_ENV_ARCH_BITS   64
        #else
            // D_ENV_ARCH_MIPS32
            //   macro: indicates a 32-bit MIPS target.
            #define D_ENV_ARCH_MIPS32 1

            // D_ENV_ARCH_NAME
            //   constant: human-readable name of the detected architecture.
            #define D_ENV_ARCH_NAME   "MIPS 32"

            // D_ENV_ARCH_BITS
            //   constant: native architecture width in bits.
            #define D_ENV_ARCH_BITS   32
        #endif

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_MIPS

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_BIG

    // 2.1.1.8
    // SPARC
    #elif ( defined(__sparc__) ||                                              \
            defined(__sparc) )
        // D_ENV_ARCH_SPARC
        //   macro: indicates that the detected architecture is SPARC.
        #define D_ENV_ARCH_SPARC 1

        #if ( defined(__sparc64__) ||                                          \
              defined(__sparcv9) )
            // D_ENV_ARCH_SPARC64
            //   macro: indicates a 64-bit SPARC target.
            #define D_ENV_ARCH_SPARC64 1

            // D_ENV_ARCH_NAME
            //   constant: human-readable name of the detected architecture.
            #define D_ENV_ARCH_NAME    "SPARC 64"

            // D_ENV_ARCH_BITS
            //   constant: native architecture width in bits.
            #define D_ENV_ARCH_BITS    64
        #else
            // D_ENV_ARCH_SPARC32
            //   macro: indicates a 32-bit SPARC target.
            #define D_ENV_ARCH_SPARC32 1

            // D_ENV_ARCH_NAME
            //   constant: human-readable name of the detected architecture.
            #define D_ENV_ARCH_NAME    "SPARC 32"

            // D_ENV_ARCH_BITS
            //   constant: native architecture width in bits.
            #define D_ENV_ARCH_BITS    32
        #endif

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_SPARC

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_BIG

    // 2.1.1.9
    // IBM System/390
    #elif ( defined(__s390__) ||                                               \
            defined(__s390x__) )
        // D_ENV_ARCH_S390
        //   macro: indicates that the detected architecture is System/390.
        #define D_ENV_ARCH_S390 1

        #if defined(__s390x__)
            // D_ENV_ARCH_S390X
            //   macro: indicates a 64-bit System/390 target.
            #define D_ENV_ARCH_S390X 1

            // D_ENV_ARCH_NAME
            //   constant: human-readable name of the detected architecture.
            #define D_ENV_ARCH_NAME  "system/390 64"

            // D_ENV_ARCH_BITS
            //   constant: native architecture width in bits.
            #define D_ENV_ARCH_BITS  64
        #else
            // D_ENV_ARCH_NAME
            //   constant: human-readable name of the detected architecture.
            #define D_ENV_ARCH_NAME "system/390 32"

            // D_ENV_ARCH_BITS
            //   constant: native architecture width in bits.
            #define D_ENV_ARCH_BITS 32
        #endif  // __s390x__

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_S390

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_BIG

    // 2.1.1.10
    // Itanium (IA-64)
    #elif ( defined(__ia64__) ||                                              \
            defined(_IA64)    ||                                              \
            defined(__IA64__) ||                                              \
            defined(_M_IA64) )
        // D_ENV_ARCH_IA64
        //   macro: indicates that the detected architecture is Itanium.
        #define D_ENV_ARCH_IA64   1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the detected architecture.
        #define D_ENV_ARCH_NAME   "Itanium"

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_IA64

        // D_ENV_ARCH_BITS
        //   constant: native architecture width in bits.
        #define D_ENV_ARCH_BITS   64

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_LITTLE

    // 2.1.1.11
    // Alpha
    #elif ( defined(__alpha__) ||                                             \
            defined(__alpha)   ||                                             \
            defined(_M_ALPHA) )
        // D_ENV_ARCH_ALPHA
        //   macro: indicates that the detected architecture is Alpha.
        #define D_ENV_ARCH_ALPHA  1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the detected architecture.
        #define D_ENV_ARCH_NAME   "Alpha"

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_ALPHA

        // D_ENV_ARCH_BITS
        //   constant: native architecture width in bits.
        #define D_ENV_ARCH_BITS   64

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_LITTLE

    // 2.1.1.12
    // Unknown architecture
    #else
        // D_ENV_ARCH_UNKNOWN
        //   macro: indicates that no supported architecture was detected.
        #define D_ENV_ARCH_UNKNOWN 1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the detected architecture.
        #define D_ENV_ARCH_NAME    "Unknown"

        // D_ENV_ARCH_TYPE
        //   constant: architecture type identifier for the detected target.
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_UNKNOWN

        // D_ENV_ARCH_BITS
        //   constant: native architecture width in bits; 0 when unknown.
        #define D_ENV_ARCH_BITS    0

        // D_ENV_ARCH_ENDIAN
        //   constant: byte-order identifier for the detected architecture.
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_UNKNOWN
    #endif

#else

// 2.2    Predefined detection
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_DETECTED_ARCH_* overrides

    // use pre-defined detection variables when automatic detection is disabled
    // 2.2.1.1
    // x86-64
    #ifdef D_ENV_DETECTED_ARCH_X64
        // D_ENV_ARCH_X64
        //   macro: indicates a configured x86-64 target.
        #define D_ENV_ARCH_X64    1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME   "x86-64"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_X64

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits.
        #define D_ENV_ARCH_BITS   64

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_LITTLE

    // 2.2.1.2
    // x86
    #elif defined(D_ENV_DETECTED_ARCH_X86)
        // D_ENV_ARCH_X86
        //   macro: indicates a configured 32-bit x86 target.
        #define D_ENV_ARCH_X86    1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME   "x86"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_X86

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits.
        #define D_ENV_ARCH_BITS   32

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_LITTLE

    // 2.2.1.3
    // ARM64
    #elif defined(D_ENV_DETECTED_ARCH_ARM64)
        // D_ENV_ARCH_ARM64
        //   macro: indicates a configured 64-bit ARM target.
        #define D_ENV_ARCH_ARM64  1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME   "ARM64"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_ARM64

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits.
        #define D_ENV_ARCH_BITS   64

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_LITTLE

    // 2.2.1.4
    // ARM
    #elif defined(D_ENV_DETECTED_ARCH_ARM)
        // D_ENV_ARCH_ARM
        //   macro: indicates a configured 32-bit ARM target.
        #define D_ENV_ARCH_ARM    1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME   "ARM"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_ARM

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits.
        #define D_ENV_ARCH_BITS   32

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_LITTLE

    // 2.2.1.5
    // RISC-V
    #elif defined(D_ENV_DETECTED_ARCH_RISCV)
        // D_ENV_ARCH_RISCV
        //   macro: indicates a configured RISC-V target.
        #define D_ENV_ARCH_RISCV  1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME   "RISC-V"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE   D_ENV_ARCH_TYPE_RISCV

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits.
        #define D_ENV_ARCH_BITS   64

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN D_ENV_ARCH_ENDIAN_LITTLE

    // 2.2.1.6
    // PowerPC
    #elif defined(D_ENV_DETECTED_ARCH_POWERPC)
        // D_ENV_ARCH_POWERPC
        //   macro: indicates a configured PowerPC target.
        #define D_ENV_ARCH_POWERPC 1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME    "PowerPC"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_POWERPC

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits.
        #define D_ENV_ARCH_BITS    64

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_BIG

    // 2.2.1.7
    // MIPS
    #elif defined(D_ENV_DETECTED_ARCH_MIPS)
        // D_ENV_ARCH_MIPS
        //   macro: indicates a configured MIPS target.
        #define D_ENV_ARCH_MIPS    1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME    "MIPS"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_MIPS

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits.
        #define D_ENV_ARCH_BITS    64

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_BIG

    // 2.2.1.8
    // SPARC
    #elif defined(D_ENV_DETECTED_ARCH_SPARC)
        // D_ENV_ARCH_SPARC
        //   macro: indicates a configured SPARC target.
        #define D_ENV_ARCH_SPARC   1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME    "SPARC"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_SPARC

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits.
        #define D_ENV_ARCH_BITS    64

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_BIG

    // 2.2.1.9
    // IBM System/390
    #elif defined(D_ENV_DETECTED_ARCH_S390)
        // D_ENV_ARCH_S390
        //   macro: indicates a configured IBM System/390 target.
        #define D_ENV_ARCH_S390    1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME    "system/390"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_S390

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits.
        #define D_ENV_ARCH_BITS    64

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_BIG

    // 2.2.1.10
    // Itanium (IA-64)
    #elif defined(D_ENV_DETECTED_ARCH_IA64)
        // D_ENV_ARCH_IA64
        //   macro: indicates a configured Itanium target.
        #define D_ENV_ARCH_IA64    1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME    "Itanium"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_IA64

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits.
        #define D_ENV_ARCH_BITS    64

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE

    // 2.2.1.11
    // Alpha
    #elif defined(D_ENV_DETECTED_ARCH_ALPHA)
        // D_ENV_ARCH_ALPHA
        //   macro: indicates a configured Alpha target.
        #define D_ENV_ARCH_ALPHA   1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME    "Alpha"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_ALPHA

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits.
        #define D_ENV_ARCH_BITS    64

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_LITTLE

    // 2.2.1.12
    // Unknown architecture
    #elif defined(D_ENV_DETECTED_ARCH_UNKNOWN)
        // D_ENV_ARCH_UNKNOWN
        //   macro: indicates a configured unknown-architecture target.
        #define D_ENV_ARCH_UNKNOWN 1

        // D_ENV_ARCH_NAME
        //   constant: human-readable name of the configured architecture.
        #define D_ENV_ARCH_NAME    "Unknown"

        // D_ENV_ARCH_TYPE
        //   constant: configured architecture type identifier.
        #define D_ENV_ARCH_TYPE    D_ENV_ARCH_TYPE_UNKNOWN

        // D_ENV_ARCH_BITS
        //   constant: configured architecture width in bits; 0 when unknown.
        #define D_ENV_ARCH_BITS    0

        // D_ENV_ARCH_ENDIAN
        //   constant: configured architecture byte order.
        #define D_ENV_ARCH_ENDIAN  D_ENV_ARCH_ENDIAN_UNKNOWN
    #endif  // D_ENV_DETECTED_ARCH_X64
#endif  // D_CFG_ENV_ARCH_ENABLED


//==============================================================================
// 3.  DERIVED ARCHITECTURE FLAGS
//==============================================================================
// Derives family, width, and endianness predicates from the selected
// architecture descriptors.


// 3.1    Architecture families
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_ARCH_IS_X86_FAMILY
//   macro: 1 for x86 or x86-64 targets; otherwise 0.
#if ( defined(D_ENV_ARCH_X86) ||                                              \
      defined(D_ENV_ARCH_X64) )
    #define D_ENV_ARCH_IS_X86_FAMILY 1
#else
    #define D_ENV_ARCH_IS_X86_FAMILY 0
#endif  // defined(D_ENV_ARCH_X86) || defined(D_ENV_ARCH_X64)

// 3.1.2
// D_ENV_ARCH_IS_ARM_FAMILY
//   macro: 1 for ARM or ARM64 targets; otherwise 0.
#if ( defined(D_ENV_ARCH_ARM) ||                                              \
      defined(D_ENV_ARCH_ARM64) )
    #define D_ENV_ARCH_IS_ARM_FAMILY 1
#else
    #define D_ENV_ARCH_IS_ARM_FAMILY 0
#endif  // defined(D_ENV_ARCH_ARM) || defined(D_ENV_ARCH_ARM64)

// 3.2    Architecture bit width
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_ARCH_IS_64BIT
//   macro: 1 when the selected architecture width is 64 bits; otherwise 0.
#if (D_ENV_ARCH_BITS == 64)
    #define D_ENV_ARCH_IS_64BIT 1
#else
    #define D_ENV_ARCH_IS_64BIT 0
#endif  // D_ENV_ARCH_BITS == 64

// 3.2.2
// D_ENV_ARCH_IS_32BIT
//   macro: 1 when the selected architecture width is 32 bits; otherwise 0.
#if (D_ENV_ARCH_BITS == 32)
    #define D_ENV_ARCH_IS_32BIT 1
#else
    #define D_ENV_ARCH_IS_32BIT 0
#endif  // D_ENV_ARCH_BITS == 32

// 3.3    Architecture endianness
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_ARCH_IS_LITTLE_ENDIAN
//   macro: 1 when the selected architecture is little-endian; otherwise 0.
#if (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE)
    #define D_ENV_ARCH_IS_LITTLE_ENDIAN 1
#else
    #define D_ENV_ARCH_IS_LITTLE_ENDIAN 0
#endif  // D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE

// 3.3.2
// D_ENV_ARCH_IS_BIG_ENDIAN
//   macro: 1 when the selected architecture is big-endian; otherwise 0.
#if (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG)
    #define D_ENV_ARCH_IS_BIG_ENDIAN 1
#else
    #define D_ENV_ARCH_IS_BIG_ENDIAN 0
#endif  // D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG


#endif  // DJINTERP_ENV_ENV_ARCH_H
