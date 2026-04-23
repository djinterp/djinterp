/******************************************************************************
* djinterp [core]                                                env_windows.h
*
* djinterp Windows environment detection header:
*   This header provides comprehensive, compile-time detection of the Windows
* compilation environment. It detects Windows version targeting, SDK version,
* subsystem type, Unicode configuration, CRT features, COM/WinRT/UWP
* availability, security APIs, and Windows-specific runtime characteristics.
*
* scope:
*   - Windows version targeting (NTDDI_VERSION, _WIN32_WINNT)
*   - Windows SDK version detection
*   - Win32/Win64 and WoW64 detection
*   - Unicode vs. ANSI build configuration
*   - Windows subsystem detection (console, GUI, driver)
*   - CRT and UCRT feature availability
*   - COM, OLE, and WinRT/UWP support
*   - security API availability (CNG, DPAPI, SSPI)
*   - Windows-specific header availability
*
* usage:
*   Included automatically by env.h when a Windows OS is detected:
*     #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
*         #include ".\core\env\win_env.h"
*     #endif
*
* NAMING CONVENTION:
*   D_ENV_WIN_[CATEGORY]_[FEATURE] - 1 if available, 0 otherwise
*
* 
* path:      /inc/c/core/env/env_windows.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_WINDOWS_
#define DJINTERP_ENVIRONMENT_WINDOWS_ 1


// =============================================================================
// I.   WINDOWS VERSION TARGETING
// =============================================================================

// -----------------------------------------------------------------------------
// A.  _WIN32_WINNT version constants
// -----------------------------------------------------------------------------

// D_ENV_WIN_WINNT_NT4
//   constant: _WIN32_WINNT value for Windows NT 4.0.
#define D_ENV_WIN_WINNT_NT4             0x0400

// D_ENV_WIN_WINNT_2000
//   constant: _WIN32_WINNT value for Windows 2000.
#define D_ENV_WIN_WINNT_2000            0x0500

// D_ENV_WIN_WINNT_XP
//   constant: _WIN32_WINNT value for Windows XP.
#define D_ENV_WIN_WINNT_XP              0x0501

// D_ENV_WIN_WINNT_WS03
//   constant: _WIN32_WINNT value for Windows Server 2003.
#define D_ENV_WIN_WINNT_WS03            0x0502

// D_ENV_WIN_WINNT_VISTA
//   constant: _WIN32_WINNT value for Windows Vista / Server 2008.
#define D_ENV_WIN_WINNT_VISTA           0x0600

// D_ENV_WIN_WINNT_WIN7
//   constant: _WIN32_WINNT value for Windows 7 / Server 2008 R2.
#define D_ENV_WIN_WINNT_WIN7            0x0601

// D_ENV_WIN_WINNT_WIN8
//   constant: _WIN32_WINNT value for Windows 8 / Server 2012.
#define D_ENV_WIN_WINNT_WIN8            0x0602

// D_ENV_WIN_WINNT_WINBLUE
//   constant: _WIN32_WINNT value for Windows 8.1 / Server 2012 R2.
#define D_ENV_WIN_WINNT_WINBLUE         0x0603

// D_ENV_WIN_WINNT_WIN10
//   constant: _WIN32_WINNT value for Windows 10 / Server 2016+.
#define D_ENV_WIN_WINNT_WIN10           0x0A00


// -----------------------------------------------------------------------------
// B.  NTDDI version constants
// -----------------------------------------------------------------------------

// D_ENV_WIN_NTDDI_WIN2K
//   constant: NTDDI_VERSION value for Windows 2000.
#define D_ENV_WIN_NTDDI_WIN2K           0x05000000

// D_ENV_WIN_NTDDI_WINXP
//   constant: NTDDI_VERSION value for Windows XP.
#define D_ENV_WIN_NTDDI_WINXP           0x05010000

// D_ENV_WIN_NTDDI_WS03
//   constant: NTDDI_VERSION value for Windows Server 2003.
#define D_ENV_WIN_NTDDI_WS03            0x05020000

// D_ENV_WIN_NTDDI_VISTA
//   constant: NTDDI_VERSION value for Windows Vista.
#define D_ENV_WIN_NTDDI_VISTA           0x06000000

// D_ENV_WIN_NTDDI_VISTASP1
//   constant: NTDDI_VERSION value for Windows Vista SP1.
#define D_ENV_WIN_NTDDI_VISTASP1        0x06000100

// D_ENV_WIN_NTDDI_WIN7
//   constant: NTDDI_VERSION value for Windows 7.
#define D_ENV_WIN_NTDDI_WIN7            0x06010000

// D_ENV_WIN_NTDDI_WIN8
//   constant: NTDDI_VERSION value for Windows 8.
#define D_ENV_WIN_NTDDI_WIN8            0x06020000

// D_ENV_WIN_NTDDI_WINBLUE
//   constant: NTDDI_VERSION value for Windows 8.1.
#define D_ENV_WIN_NTDDI_WINBLUE         0x06030000

// D_ENV_WIN_NTDDI_WIN10
//   constant: NTDDI_VERSION value for Windows 10 (initial release).
#define D_ENV_WIN_NTDDI_WIN10           0x0A000000

// D_ENV_WIN_NTDDI_WIN10_TH2
//   constant: NTDDI_VERSION value for Windows 10 version 1511.
#define D_ENV_WIN_NTDDI_WIN10_TH2       0x0A000001

// D_ENV_WIN_NTDDI_WIN10_RS1
//   constant: NTDDI_VERSION value for Windows 10 version 1607.
#define D_ENV_WIN_NTDDI_WIN10_RS1       0x0A000002

// D_ENV_WIN_NTDDI_WIN10_RS2
//   constant: NTDDI_VERSION value for Windows 10 version 1703.
#define D_ENV_WIN_NTDDI_WIN10_RS2       0x0A000003

// D_ENV_WIN_NTDDI_WIN10_RS3
//   constant: NTDDI_VERSION value for Windows 10 version 1709.
#define D_ENV_WIN_NTDDI_WIN10_RS3       0x0A000004

// D_ENV_WIN_NTDDI_WIN10_RS4
//   constant: NTDDI_VERSION value for Windows 10 version 1803.
#define D_ENV_WIN_NTDDI_WIN10_RS4       0x0A000005

// D_ENV_WIN_NTDDI_WIN10_RS5
//   constant: NTDDI_VERSION value for Windows 10 version 1809.
#define D_ENV_WIN_NTDDI_WIN10_RS5       0x0A000006

// D_ENV_WIN_NTDDI_WIN10_19H1
//   constant: NTDDI_VERSION value for Windows 10 version 1903.
#define D_ENV_WIN_NTDDI_WIN10_19H1      0x0A000007

// D_ENV_WIN_NTDDI_WIN10_VB
//   constant: NTDDI_VERSION value for Windows 10 version 2004.
#define D_ENV_WIN_NTDDI_WIN10_VB        0x0A000008

// D_ENV_WIN_NTDDI_WIN10_MN
//   constant: NTDDI_VERSION value for Windows 10 version 20H2.
#define D_ENV_WIN_NTDDI_WIN10_MN        0x0A000009

// D_ENV_WIN_NTDDI_WIN10_FE
//   constant: NTDDI_VERSION value for Windows 10 version 21H1.
#define D_ENV_WIN_NTDDI_WIN10_FE        0x0A00000A

// D_ENV_WIN_NTDDI_WIN10_CO
//   constant: NTDDI_VERSION value for Windows 10 version 21H2.
#define D_ENV_WIN_NTDDI_WIN10_CO        0x0A00000B

// D_ENV_WIN_NTDDI_WIN10_NI
//   constant: NTDDI_VERSION value for Windows 10 version 22H2.
#define D_ENV_WIN_NTDDI_WIN10_NI        0x0A00000C

// D_ENV_WIN_NTDDI_WIN11_ZN
//   constant: NTDDI_VERSION value for Windows 11 (initial release, 21H2).
#define D_ENV_WIN_NTDDI_WIN11_ZN        0x0A00000D

// D_ENV_WIN_NTDDI_WIN11_GA
//   constant: NTDDI_VERSION value for Windows 11 version 22H2.
#define D_ENV_WIN_NTDDI_WIN11_GA        0x0A00000E

// D_ENV_WIN_NTDDI_WIN11_GE
//   constant: NTDDI_VERSION value for Windows 11 version 23H2.
#define D_ENV_WIN_NTDDI_WIN11_GE        0x0A00000F


// -----------------------------------------------------------------------------
// C.  detected target version
// -----------------------------------------------------------------------------

// detect the targeted Windows version from _WIN32_WINNT
#ifdef _WIN32_WINNT
    #define D_ENV_WIN_TARGET_WINNT _WIN32_WINNT

    #if (_WIN32_WINNT >= D_ENV_WIN_WINNT_WIN10)
        #define D_ENV_WIN_TARGET_NAME  "Windows 10+"
    #elif (_WIN32_WINNT >= D_ENV_WIN_WINNT_WINBLUE)
        #define D_ENV_WIN_TARGET_NAME  "Windows 8.1"
    #elif (_WIN32_WINNT >= D_ENV_WIN_WINNT_WIN8)
        #define D_ENV_WIN_TARGET_NAME  "Windows 8"
    #elif (_WIN32_WINNT >= D_ENV_WIN_WINNT_WIN7)
        #define D_ENV_WIN_TARGET_NAME  "Windows 7"
    #elif (_WIN32_WINNT >= D_ENV_WIN_WINNT_VISTA)
        #define D_ENV_WIN_TARGET_NAME  "Windows Vista"
    #elif (_WIN32_WINNT >= D_ENV_WIN_WINNT_WS03)
        #define D_ENV_WIN_TARGET_NAME  "Windows Server 2003"
    #elif (_WIN32_WINNT >= D_ENV_WIN_WINNT_XP)
        #define D_ENV_WIN_TARGET_NAME  "Windows XP"
    #elif (_WIN32_WINNT >= D_ENV_WIN_WINNT_2000)
        #define D_ENV_WIN_TARGET_NAME  "Windows 2000"
    #else
        #define D_ENV_WIN_TARGET_NAME  "Windows (Legacy)"
    #endif
#else
    #define D_ENV_WIN_TARGET_WINNT     0
    #define D_ENV_WIN_TARGET_NAME      "Windows (Unspecified)"
#endif

// detect the targeted NTDDI version
#ifdef NTDDI_VERSION
    #define D_ENV_WIN_TARGET_NTDDI NTDDI_VERSION
#else
    #define D_ENV_WIN_TARGET_NTDDI     0
#endif

// D_ENV_WIN_TARGET_AT_LEAST
//   macro: evaluates to 1 if the targeted _WIN32_WINNT version is at least
// the specified version.
#define D_ENV_WIN_TARGET_AT_LEAST(version)                                    \
    ( D_ENV_WIN_TARGET_WINNT >= (version) )

// D_ENV_WIN_NTDDI_AT_LEAST
//   macro: evaluates to 1 if the targeted NTDDI_VERSION is at least the
// specified version.
#define D_ENV_WIN_NTDDI_AT_LEAST(version)                                     \
    ( D_ENV_WIN_TARGET_NTDDI >= (version) )


// =============================================================================
// II.  WINDOWS SDK DETECTION
// =============================================================================

// D_ENV_WIN_SDK_VERSION
//   feature: detect the Windows SDK version from WINVER and related macros.

// detect primary SDK version indicator
#ifdef WINVER
    #define D_ENV_WIN_SDK_WINVER WINVER
#else
    #define D_ENV_WIN_SDK_WINVER 0
#endif

// detect Windows SDK from the UCRT version header
#ifdef _UCRT
    #define D_ENV_WIN_HAS_UCRT 1
#else
    #define D_ENV_WIN_HAS_UCRT 0
#endif

// detect _MSC_FULL_VER for precise MSVC/SDK pairing
#ifdef _MSC_FULL_VER
    #define D_ENV_WIN_MSVC_FULL_VER _MSC_FULL_VER
#else
    #define D_ENV_WIN_MSVC_FULL_VER 0
#endif

// detect _MSC_BUILD for incremental build number
#ifdef _MSC_BUILD
    #define D_ENV_WIN_MSVC_BUILD _MSC_BUILD
#else
    #define D_ENV_WIN_MSVC_BUILD 0
#endif


// =============================================================================
// III. PLATFORM AND BITNESS
// =============================================================================

// D_ENV_WIN_IS_64BIT
//   feature: detect if compiling for 64-bit Windows.
#if defined(_WIN64)
    #define D_ENV_WIN_IS_64BIT  1
    #define D_ENV_WIN_IS_32BIT  0
#elif defined(_WIN32)
    #define D_ENV_WIN_IS_64BIT  0
    #define D_ENV_WIN_IS_32BIT  1
#else
    #define D_ENV_WIN_IS_64BIT  0
    #define D_ENV_WIN_IS_32BIT  0
#endif

// D_ENV_WIN_IS_WOW64
//   feature: detect if this is a 32-bit process on 64-bit Windows.
// note: compile-time heuristic only — true WoW64 detection requires
// runtime checks via IsWow64Process(). this detects the typical scenario
// of targeting x86 while the SDK indicates 64-bit awareness.
#if ( defined(_WIN32)  &&                                                     \
      !defined(_WIN64) &&                                                     \
      defined(_M_IX86) )
    #define D_ENV_WIN_MAYBE_WOW64 1
#else
    #define D_ENV_WIN_MAYBE_WOW64 0
#endif

// D_ENV_WIN_IS_ARM
//   feature: detect if targeting Windows on ARM.
#if ( defined(_M_ARM)   ||                                                    \
      defined(_M_ARM64) )
    #define D_ENV_WIN_IS_ARM    1
#else
    #define D_ENV_WIN_IS_ARM    0
#endif

// D_ENV_WIN_IS_ARM64
//   feature: detect if targeting Windows on ARM64 specifically.
#ifdef _M_ARM64
    #define D_ENV_WIN_IS_ARM64  1
#else
    #define D_ENV_WIN_IS_ARM64  0
#endif


// =============================================================================
// IV.  UNICODE / ANSI CONFIGURATION
// =============================================================================

// D_ENV_WIN_UNICODE
//   feature: detect if building in Unicode mode (UNICODE / _UNICODE).
#if ( defined(UNICODE) ||                                                     \
      defined(_UNICODE) )
    #define D_ENV_WIN_UNICODE       1
    #define D_ENV_WIN_CHAR_MODE     "Unicode"
#else
    #define D_ENV_WIN_UNICODE       0
    #define D_ENV_WIN_CHAR_MODE     "ANSI"
#endif

// D_ENV_WIN_MBCS
//   feature: detect if multi-byte character set mode is active.
#ifdef _MBCS
    #define D_ENV_WIN_MBCS          1
#else
    #define D_ENV_WIN_MBCS          0
#endif


// =============================================================================
// V.   WINDOWS SUBSYSTEM DETECTION
// =============================================================================

// D_ENV_WIN_SUBSYSTEM_CONSOLE
//   feature: detect console subsystem (heuristic — _CONSOLE is typically
// defined by MSVC project settings or build system macros).
#ifdef _CONSOLE
    #define D_ENV_WIN_SUBSYSTEM_CONSOLE 1
#else
    #define D_ENV_WIN_SUBSYSTEM_CONSOLE 0
#endif

// D_ENV_WIN_SUBSYSTEM_WINDOWS
//   feature: detect GUI (Windows) subsystem.
#ifdef _WINDOWS
    #define D_ENV_WIN_SUBSYSTEM_WINDOWS 1
#else
    #define D_ENV_WIN_SUBSYSTEM_WINDOWS 0
#endif

// D_ENV_WIN_IS_DLL
//   feature: detect if building a DLL.
#if ( defined(_WINDLL) ||                                                     \
      defined(_USRDLL) )
    #define D_ENV_WIN_IS_DLL        1
#else
    #define D_ENV_WIN_IS_DLL        0
#endif

// D_ENV_WIN_IS_DRIVER
//   feature: detect if building a kernel-mode driver (WDK/DDK).
#if ( defined(NTDDI_VERSION) &&                                               \
      defined(_KERNEL_MODE) )
    #define D_ENV_WIN_IS_DRIVER     1
#elif defined(_WDMDDK_)
    #define D_ENV_WIN_IS_DRIVER     1
#else
    #define D_ENV_WIN_IS_DRIVER     0
#endif


// =============================================================================
// VI.  CRT AND RUNTIME FEATURES
// =============================================================================

// D_ENV_WIN_HAS_CRT_SECURE
//   feature: detect if the secure CRT functions are available (MSVC).
// the secure CRT (_s suffix functions) was introduced in MSVC 8.0
// (Visual Studio 2005), corresponding to _MSC_VER >= 1400.
#ifdef _MSC_VER
    #if (_MSC_VER >= 1400)
        #define D_ENV_WIN_HAS_CRT_SECURE 1
    #else
        #define D_ENV_WIN_HAS_CRT_SECURE 0
    #endif
#else
    #define D_ENV_WIN_HAS_CRT_SECURE 0
#endif

// D_ENV_WIN_HAS_CRT_DBG
//   feature: detect if CRT debug features are available.
#if ( defined(_DEBUG) &&                                                      \
      defined(_MSC_VER) )
    #define D_ENV_WIN_HAS_CRT_DBG   1
#else
    #define D_ENV_WIN_HAS_CRT_DBG   0
#endif

// D_ENV_WIN_STATIC_CRT
//   feature: detect if statically linking against the CRT.
#if ( defined(_MT) &&                                                         \
      !defined(_DLL) )
    #define D_ENV_WIN_STATIC_CRT    1
    #define D_ENV_WIN_DYNAMIC_CRT   0
#elif defined(_DLL)
    #define D_ENV_WIN_STATIC_CRT    0
    #define D_ENV_WIN_DYNAMIC_CRT   1
#else
    #define D_ENV_WIN_STATIC_CRT    0
    #define D_ENV_WIN_DYNAMIC_CRT   0
#endif

// D_ENV_WIN_HAS_ITERATOR_DEBUG
//   feature: detect if MSVC iterator debugging is enabled.
// note: _ITERATOR_DEBUG_LEVEL == 2 is the default for debug builds.
#ifdef _ITERATOR_DEBUG_LEVEL
    #if (_ITERATOR_DEBUG_LEVEL >= 1)
        #define D_ENV_WIN_HAS_ITERATOR_DEBUG 1
    #else
        #define D_ENV_WIN_HAS_ITERATOR_DEBUG 0
    #endif
#else
    #define D_ENV_WIN_HAS_ITERATOR_DEBUG 0
#endif

// D_ENV_WIN_HAS_SEH
//   feature: detect if Structured Exception Handling is available.
// SEH is always available with MSVC; other compilers on Windows may
// support it via extensions.
#ifdef _MSC_VER
    #define D_ENV_WIN_HAS_SEH       1
#elif defined(__SEH__)
    #define D_ENV_WIN_HAS_SEH       1
#else
    #define D_ENV_WIN_HAS_SEH       0
#endif

// D_ENV_WIN_HAS_DECLSPEC
//   feature: detect if __declspec is available.
#if ( defined(_MSC_VER)    ||                                                 \
      defined(__MINGW32__) ||                                                 \
      defined(__MINGW64__) )
    #define D_ENV_WIN_HAS_DECLSPEC  1
#else
    #define D_ENV_WIN_HAS_DECLSPEC  0
#endif

// D_ENV_WIN_HAS_SAL
//   feature: detect if SAL (Source Annotation Language) is available.
// SAL annotations are available in MSVC with sal.h.
#ifdef _MSC_VER
    #define D_ENV_WIN_HAS_SAL       1
#else
    #define D_ENV_WIN_HAS_SAL       0
#endif


// =============================================================================
// VII. COM, OLE, AND WINRT
// =============================================================================

// D_ENV_WIN_HAS_COM
//   feature: detect if COM (Component Object Model) support is available.
// COM is available on all Windows versions since Windows 95/NT 3.51.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_NT4)
    #define D_ENV_WIN_HAS_COM       1
#else
    // assume available on any detected Windows target
    #define D_ENV_WIN_HAS_COM       1
#endif

// D_ENV_WIN_HAS_OLE
//   feature: detect if OLE (Object Linking and Embedding) is available.
#define D_ENV_WIN_HAS_OLE           D_ENV_WIN_HAS_COM

// D_ENV_WIN_HAS_DCOM
//   feature: detect if DCOM (Distributed COM) is available.
// DCOM requires Windows NT 4.0 SP2+ or Windows 2000+.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_2000)
    #define D_ENV_WIN_HAS_DCOM      1
#else
    #define D_ENV_WIN_HAS_DCOM      0
#endif

// D_ENV_WIN_HAS_WINRT
//   feature: detect if WinRT (Windows Runtime) APIs are available.
// WinRT requires Windows 8+ and a C++/CX or C++/WinRT capable compiler.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_WIN8)
    #define D_ENV_WIN_HAS_WINRT     1
#else
    #define D_ENV_WIN_HAS_WINRT     0
#endif

// D_ENV_WIN_HAS_CPPWINRT
//   feature: detect if C++/WinRT headers are available.
#if ( defined(__cpp_lib_coroutine) ||                                         \
      defined(WINRT_BASE_H) )
    #define D_ENV_WIN_HAS_CPPWINRT  1
#else
    #define D_ENV_WIN_HAS_CPPWINRT  0
#endif

// D_ENV_WIN_HAS_UWP
//   feature: detect if compiling for UWP (Universal Windows Platform).
#if ( defined(WINAPI_FAMILY) &&                                               \
      (WINAPI_FAMILY == WINAPI_FAMILY_APP) )
    #define D_ENV_WIN_HAS_UWP       1
#else
    #define D_ENV_WIN_HAS_UWP       0
#endif

// D_ENV_WIN_HAS_DESKTOP
//   feature: detect if compiling for the desktop API partition.
#if defined(WINAPI_FAMILY)
    #if ( (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) || \
          (WINAPI_FAMILY == WINAPI_FAMILY_PC_APP) )
        #define D_ENV_WIN_HAS_DESKTOP 1
    #else
        #define D_ENV_WIN_HAS_DESKTOP 0
    #endif
#else
    // when WINAPI_FAMILY is not defined, assume full desktop access
    #define D_ENV_WIN_HAS_DESKTOP   1
#endif


// =============================================================================
// VIII. SECURITY APIS
// =============================================================================

// D_ENV_WIN_HAS_CNG
//   feature: detect if CNG (Cryptography Next Generation) is available.
// CNG was introduced in Windows Vista (bcrypt.h, ncrypt.h).
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_VISTA)
    #define D_ENV_WIN_HAS_CNG       1
#else
    #define D_ENV_WIN_HAS_CNG       0
#endif

// D_ENV_WIN_HAS_CRYPTOAPI
//   feature: detect if the legacy CryptoAPI (wincrypt.h) is available.
// CryptoAPI has been available since Windows NT 4.0.
#define D_ENV_WIN_HAS_CRYPTOAPI     1

// D_ENV_WIN_HAS_DPAPI
//   feature: detect if DPAPI (Data Protection API) is available.
// DPAPI requires Windows 2000+.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_2000)
    #define D_ENV_WIN_HAS_DPAPI     1
#else
    #define D_ENV_WIN_HAS_DPAPI     0
#endif

// D_ENV_WIN_HAS_SSPI
//   feature: detect if SSPI (Security Support Provider Interface) is
// available. SSPI requires Windows 2000+.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_2000)
    #define D_ENV_WIN_HAS_SSPI      1
#else
    #define D_ENV_WIN_HAS_SSPI      0
#endif

// D_ENV_WIN_HAS_RTLGENRANDOM
//   feature: detect if RtlGenRandom (SystemFunction036) is available.
// available since Windows XP.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_XP)
    #define D_ENV_WIN_HAS_RTLGENRANDOM 1
#else
    #define D_ENV_WIN_HAS_RTLGENRANDOM 0
#endif

// D_ENV_WIN_HAS_BCRYPTRNG
//   feature: detect if BCryptGenRandom (CNG) is available for secure
// random number generation. requires Vista+.
#define D_ENV_WIN_HAS_BCRYPTRNG     D_ENV_WIN_HAS_CNG


// =============================================================================
// IX.  WINDOWS API FEATURES
// =============================================================================

// D_ENV_WIN_HAS_CREATEFILE2
//   feature: detect if CreateFile2 is available (Windows 8+).
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_WIN8)
    #define D_ENV_WIN_HAS_CREATEFILE2 1
#else
    #define D_ENV_WIN_HAS_CREATEFILE2 0
#endif

// D_ENV_WIN_HAS_THREADPOOL
//   feature: detect if the Vista+ thread pool API is available.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_VISTA)
    #define D_ENV_WIN_HAS_THREADPOOL 1
#else
    #define D_ENV_WIN_HAS_THREADPOOL 0
#endif

// D_ENV_WIN_HAS_CONDITION_VARIABLE
//   feature: detect if CONDITION_VARIABLE (Vista+) is available.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_VISTA)
    #define D_ENV_WIN_HAS_CONDITION_VARIABLE 1
#else
    #define D_ENV_WIN_HAS_CONDITION_VARIABLE 0
#endif

// D_ENV_WIN_HAS_SRW_LOCK
//   feature: detect if SRWLOCK (Slim Reader/Writer Lock, Vista+) is
// available.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_VISTA)
    #define D_ENV_WIN_HAS_SRW_LOCK  1
#else
    #define D_ENV_WIN_HAS_SRW_LOCK  0
#endif

// D_ENV_WIN_HAS_INIT_ONCE
//   feature: detect if InitOnceExecuteOnce (Vista+) is available.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_VISTA)
    #define D_ENV_WIN_HAS_INIT_ONCE 1
#else
    #define D_ENV_WIN_HAS_INIT_ONCE 0
#endif

// D_ENV_WIN_HAS_VIRTUAL_ALLOC_2
//   feature: detect if VirtualAlloc2 (Win10 1803+) is available.
#if D_ENV_WIN_NTDDI_AT_LEAST(D_ENV_WIN_NTDDI_WIN10_RS4)
    #define D_ENV_WIN_HAS_VIRTUAL_ALLOC_2 1
#else
    #define D_ENV_WIN_HAS_VIRTUAL_ALLOC_2 0
#endif

// D_ENV_WIN_HAS_PREFETCH_VIRTUAL_MEMORY
//   feature: detect if PrefetchVirtualMemory (Win8+) is available.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_WIN8)
    #define D_ENV_WIN_HAS_PREFETCH_VIRTUAL_MEMORY 1
#else
    #define D_ENV_WIN_HAS_PREFETCH_VIRTUAL_MEMORY 0
#endif

// D_ENV_WIN_HAS_FIBERS
//   feature: detect if the fiber API (ConvertThreadToFiber, etc.) is
// available. fibers have been available since Windows XP.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_XP)
    #define D_ENV_WIN_HAS_FIBERS    1
#else
    #define D_ENV_WIN_HAS_FIBERS    0
#endif

// D_ENV_WIN_HAS_VECTORED_EXCEPTION
//   feature: detect if Vectored Exception Handling (VEH) is available.
// VEH was introduced in Windows XP.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_XP)
    #define D_ENV_WIN_HAS_VECTORED_EXCEPTION 1
#else
    #define D_ENV_WIN_HAS_VECTORED_EXCEPTION 0
#endif

// D_ENV_WIN_HAS_GETPROCESSMEMORYINFO
//   feature: detect if GetProcessMemoryInfo (psapi.h) is available.
#if D_ENV_WIN_TARGET_AT_LEAST(D_ENV_WIN_WINNT_2000)
    #define D_ENV_WIN_HAS_GETPROCESSMEMORYINFO 1
#else
    #define D_ENV_WIN_HAS_GETPROCESSMEMORYINFO 0
#endif


// =============================================================================
// X.   WINDOWS HEADERS AVAILABILITY
// =============================================================================

// D_ENV_WIN_HAS_WINDOWS_H
//   feature: windows.h is available on all Windows targets.
#define D_ENV_WIN_HAS_WINDOWS_H     1

// D_ENV_WIN_HAS_WINSOCK2_H
//   feature: detect if winsock2.h is available (desktop only).
#if D_ENV_WIN_HAS_DESKTOP
    #define D_ENV_WIN_HAS_WINSOCK2_H 1
#else
    #define D_ENV_WIN_HAS_WINSOCK2_H 0
#endif

// D_ENV_WIN_HAS_WS2TCPIP_H
//   feature: detect if ws2tcpip.h is available (modern socket extensions).
#define D_ENV_WIN_HAS_WS2TCPIP_H    D_ENV_WIN_HAS_WINSOCK2_H

// D_ENV_WIN_HAS_SHLWAPI_H
//   feature: detect if shlwapi.h (Shell Light-Weight Utility) is available.
#if D_ENV_WIN_HAS_DESKTOP
    #define D_ENV_WIN_HAS_SHLWAPI_H 1
#else
    #define D_ENV_WIN_HAS_SHLWAPI_H 0
#endif

// D_ENV_WIN_HAS_DBGHELP_H
//   feature: detect if dbghelp.h is available (debug/symbol APIs).
#if D_ENV_WIN_HAS_DESKTOP
    #define D_ENV_WIN_HAS_DBGHELP_H 1
#else
    #define D_ENV_WIN_HAS_DBGHELP_H 0
#endif

// D_ENV_WIN_HAS_PSAPI_H
//   feature: detect if psapi.h (Process Status API) is available.
#if D_ENV_WIN_HAS_DESKTOP
    #define D_ENV_WIN_HAS_PSAPI_H   1
#else
    #define D_ENV_WIN_HAS_PSAPI_H   0
#endif

// D_ENV_WIN_HAS_SHELLAPI_H
//   feature: detect if shellapi.h is available (Shell functions).
#if D_ENV_WIN_HAS_DESKTOP
    #define D_ENV_WIN_HAS_SHELLAPI_H 1
#else
    #define D_ENV_WIN_HAS_SHELLAPI_H 0
#endif

// D_ENV_WIN_HAS_IO_H
//   feature: detect if io.h is available (low-level I/O, MSVC).
#ifdef _MSC_VER
    #define D_ENV_WIN_HAS_IO_H      1
#elif ( defined(__MINGW32__) ||                                               \
        defined(__MINGW64__) )
    #define D_ENV_WIN_HAS_IO_H      1
#else
    #define D_ENV_WIN_HAS_IO_H      0
#endif

// D_ENV_WIN_HAS_DIRECT_H
//   feature: detect if direct.h is available (directory functions, MSVC).
#ifdef _MSC_VER
    #define D_ENV_WIN_HAS_DIRECT_H  1
#elif ( defined(__MINGW32__) ||                                               \
        defined(__MINGW64__) )
    #define D_ENV_WIN_HAS_DIRECT_H  1
#else
    #define D_ENV_WIN_HAS_DIRECT_H  0
#endif

// D_ENV_WIN_HAS_PROCESS_H
//   feature: detect if process.h is available (_beginthread, _spawnl).
#ifdef _MSC_VER
    #define D_ENV_WIN_HAS_PROCESS_H 1
#elif ( defined(__MINGW32__) ||                                               \
        defined(__MINGW64__) )
    #define D_ENV_WIN_HAS_PROCESS_H 1
#else
    #define D_ENV_WIN_HAS_PROCESS_H 0
#endif


// =============================================================================
// XI.  COMPILER-SPECIFIC WINDOWS FEATURES
// =============================================================================

// D_ENV_WIN_HAS_INTRIN_H
//   feature: detect if intrin.h (compiler intrinsics) is available.
#ifdef _MSC_VER
    #define D_ENV_WIN_HAS_INTRIN_H  1
#else
    #define D_ENV_WIN_HAS_INTRIN_H  0
#endif

// D_ENV_WIN_HAS_BITSCANFORWARD
//   feature: detect if _BitScanForward / _BitScanReverse intrinsics are
// available (MSVC).
#ifdef _MSC_VER
    #define D_ENV_WIN_HAS_BITSCANFORWARD 1
#else
    #define D_ENV_WIN_HAS_BITSCANFORWARD 0
#endif

// D_ENV_WIN_HAS_BYTESWAP
//   feature: detect if _byteswap_ushort/ulong/uint64 intrinsics are
// available (MSVC).
#ifdef _MSC_VER
    #define D_ENV_WIN_HAS_BYTESWAP  1
#else
    #define D_ENV_WIN_HAS_BYTESWAP  0
#endif

// D_ENV_WIN_HAS_POPCNT
//   feature: detect if __popcnt / __popcnt64 intrinsics are available.
#ifdef _MSC_VER
    #if ( defined(__AVX__) ||                                                 \
          defined(__POPCNT__) )
        #define D_ENV_WIN_HAS_POPCNT 1
    #else
        #define D_ENV_WIN_HAS_POPCNT 0
    #endif
#else
    #define D_ENV_WIN_HAS_POPCNT    0
#endif

// D_ENV_WIN_HAS_LZCNT
//   feature: detect if __lzcnt / __lzcnt64 intrinsics are available.
#ifdef _MSC_VER
    #if defined(__LZCNT__)
        #define D_ENV_WIN_HAS_LZCNT 1
    #else
        #define D_ENV_WIN_HAS_LZCNT 0
    #endif
#else
    #define D_ENV_WIN_HAS_LZCNT     0
#endif


// =============================================================================
// XII. MINGW DETECTION
// =============================================================================

// D_ENV_WIN_IS_MINGW
//   feature: detect if building with MinGW (GNU toolchain on Windows).
#if ( defined(__MINGW32__) ||                                                 \
      defined(__MINGW64__) )
    #define D_ENV_WIN_IS_MINGW      1
#else
    #define D_ENV_WIN_IS_MINGW      0
#endif

// D_ENV_WIN_IS_MINGW64
//   feature: detect if building with MinGW-w64 specifically.
#ifdef __MINGW64__
    #define D_ENV_WIN_IS_MINGW64    1
#else
    #define D_ENV_WIN_IS_MINGW64    0
#endif

// D_ENV_WIN_IS_CYGWIN
//   feature: detect if building under Cygwin.
#ifdef __CYGWIN__
    #define D_ENV_WIN_IS_CYGWIN     1
#else
    #define D_ENV_WIN_IS_CYGWIN     0
#endif

// D_ENV_WIN_IS_MSVC_NATIVE
//   feature: detect if building with native MSVC (not MinGW, not Cygwin).
#if ( defined(_MSC_VER)       &&                                              \
      !defined(__MINGW32__)   &&                                              \
      !defined(__MINGW64__)   &&                                              \
      !defined(__CYGWIN__) )
    #define D_ENV_WIN_IS_MSVC_NATIVE 1
#else
    #define D_ENV_WIN_IS_MSVC_NATIVE 0
#endif


// =============================================================================
// XIII. LEAN AND MEAN / EXCLUSION MACROS
// =============================================================================

// D_ENV_WIN_LEAN_AND_MEAN
//   feature: detect if WIN32_LEAN_AND_MEAN is active, which excludes
// rarely-used headers from windows.h.
#ifdef WIN32_LEAN_AND_MEAN
    #define D_ENV_WIN_LEAN_AND_MEAN 1
#else
    #define D_ENV_WIN_LEAN_AND_MEAN 0
#endif

// D_ENV_WIN_STRICT
//   feature: detect if STRICT type checking is enabled.
#ifdef STRICT
    #define D_ENV_WIN_STRICT        1
#else
    #define D_ENV_WIN_STRICT        0
#endif

// D_ENV_WIN_NOMINMAX
//   feature: detect if NOMINMAX is defined (prevents windows.h from
// defining min/max macros).
#ifdef NOMINMAX
    #define D_ENV_WIN_NOMINMAX      1
#else
    #define D_ENV_WIN_NOMINMAX      0
#endif


#endif  // DJINTERP_ENVIRONMENT_WINDOWS_
