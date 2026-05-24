/******************************************************************************
* djinterp [core]                                                 env_apple.h
*
*   djinterp Apple environment detection header:
* This header provides comprehensive, compile-time detection of the Apple
* compilation environment, covering both shared Apple platform features and
* macOS-specific capabilities. It detects:
*   - Apple platform identification (macOS, iOS, tvOS, watchOS, visionOS)
*   - Darwin kernel version and Mach subsystem availability
*   - macOS SDK and deployment target version detection
*   - Apple Clang / Xcode toolchain identification
*   - Objective-C and Swift interop availability
*   - Apple framework detection (Foundation, CoreFoundation, Security, etc.)
*   - macOS-specific APIs (Grand Central Dispatch, App Sandbox, Keychain)
*   - hardware features (Apple Silicon, Rosetta 2, Hypervisor.framework)
*   - POSIX and BSD layer characteristics on Darwin
*
* scope:
*   - shared Apple platform detection (all Apple OSes)
*   - macOS SDK version targeting (__MAC_OS_X_VERSION_MIN_REQUIRED)
*   - Darwin/XNU kernel feature availability
*   - Objective-C runtime and ARC detection
*   - framework availability via __has_include and SDK version gates
*   - macOS security (App Sandbox, Hardened Runtime, Gatekeeper, TCC)
*   - Apple Silicon and architecture-specific features
*   - macOS-specific POSIX/BSD extensions
*
* usage:
*   Included automatically by env.h when an Apple OS is detected:
*     #if D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)
*         #include ".\core\env\env_apple.h"
*     #endif
*   For iOS-specific detection, see env_ios.h.
*
* NAMING CONVENTION:
*   D_ENV_APPLE_[CATEGORY]_[FEATURE] - shared across Apple platforms
*   D_ENV_MACOS_[CATEGORY]_[FEATURE] - macOS-specific
*
*
* path:      \inc\core\env\env_apple.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.28
******************************************************************************/

#ifndef DJINTERP_ENV_APPLE_
#define DJINTERP_ENV_APPLE_ 1

#include "./env.h"


// =============================================================================
// I.   APPLE PLATFORM IDENTIFICATION
// =============================================================================

// Apple's TargetConditionals.h provides the canonical platform macros.
// env.h already includes it on __APPLE__; these refine the detection
// into the full Apple platform matrix.

// -----------------------------------------------------------------------------
// A.  platform flags
// -----------------------------------------------------------------------------

// D_ENV_APPLE_IS_APPLE
//   feature: detect if building on any Apple platform.
#if defined(__APPLE__)
    #define D_ENV_APPLE_IS_APPLE        1
#else
    #define D_ENV_APPLE_IS_APPLE        0
#endif

// D_ENV_APPLE_IS_MACOS
//   feature: detect if building for macOS.
#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
    #define D_ENV_APPLE_IS_MACOS        1
#elif ( defined(__APPLE__) &&                                                  \
        !defined(TARGET_OS_IPHONE) )
    // fallback for older SDKs
    #define D_ENV_APPLE_IS_MACOS        1
#elif ( defined(__APPLE__)        &&                                           \
        defined(TARGET_OS_IPHONE) &&                                           \
        !TARGET_OS_IPHONE )
    #define D_ENV_APPLE_IS_MACOS        1
#else
    #define D_ENV_APPLE_IS_MACOS        0
#endif

// D_ENV_APPLE_IS_IOS
//   feature: detect if building for iOS/iPadOS.
#if ( defined(TARGET_OS_IOS) && TARGET_OS_IOS )
    #define D_ENV_APPLE_IS_IOS          1
#elif ( defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE &&                       \
        !(defined(TARGET_OS_TV) && TARGET_OS_TV)      &&                       \
        !(defined(TARGET_OS_WATCH) && TARGET_OS_WATCH) )
    #define D_ENV_APPLE_IS_IOS          1
#else
    #define D_ENV_APPLE_IS_IOS          0
#endif

// D_ENV_APPLE_IS_TVOS
//   feature: detect if building for tvOS.
#if ( defined(TARGET_OS_TV) && TARGET_OS_TV )
    #define D_ENV_APPLE_IS_TVOS         1
#else
    #define D_ENV_APPLE_IS_TVOS         0
#endif

// D_ENV_APPLE_IS_WATCHOS
//   feature: detect if building for watchOS.
#if ( defined(TARGET_OS_WATCH) && TARGET_OS_WATCH )
    #define D_ENV_APPLE_IS_WATCHOS      1
#else
    #define D_ENV_APPLE_IS_WATCHOS      0
#endif

// D_ENV_APPLE_IS_VISIONOS
//   feature: detect if building for visionOS (Xcode 15+).
#if ( defined(TARGET_OS_VISION) && TARGET_OS_VISION )
    #define D_ENV_APPLE_IS_VISIONOS     1
#else
    #define D_ENV_APPLE_IS_VISIONOS     0
#endif

// D_ENV_APPLE_IS_MACCATALYST
//   feature: detect if building as Mac Catalyst (iPad app on macOS).
#if ( defined(TARGET_OS_MACCATALYST) && TARGET_OS_MACCATALYST )
    #define D_ENV_APPLE_IS_MACCATALYST  1
#else
    #define D_ENV_APPLE_IS_MACCATALYST  0
#endif

// D_ENV_APPLE_IS_SIMULATOR
//   feature: detect if building for the simulator (not device).
#if ( defined(TARGET_OS_SIMULATOR) && TARGET_OS_SIMULATOR )
    #define D_ENV_APPLE_IS_SIMULATOR    1
#elif ( defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR )
    // older SDK macro
    #define D_ENV_APPLE_IS_SIMULATOR    1
#else
    #define D_ENV_APPLE_IS_SIMULATOR    0
#endif

// D_ENV_APPLE_IS_DEVICE
//   feature: detect if building for a physical device (not simulator).
#if ( D_ENV_APPLE_IS_APPLE &&                                                 \
      !D_ENV_APPLE_IS_SIMULATOR )
    #define D_ENV_APPLE_IS_DEVICE       1
#else
    #define D_ENV_APPLE_IS_DEVICE       0
#endif


// -----------------------------------------------------------------------------
// B.  platform name
// -----------------------------------------------------------------------------

#if D_ENV_APPLE_IS_VISIONOS
    #define D_ENV_APPLE_PLATFORM_NAME   "visionOS"
#elif D_ENV_APPLE_IS_WATCHOS
    #define D_ENV_APPLE_PLATFORM_NAME   "watchOS"
#elif D_ENV_APPLE_IS_TVOS
    #define D_ENV_APPLE_PLATFORM_NAME   "tvOS"
#elif D_ENV_APPLE_IS_MACCATALYST
    #define D_ENV_APPLE_PLATFORM_NAME   "Mac Catalyst"
#elif D_ENV_APPLE_IS_IOS
    #define D_ENV_APPLE_PLATFORM_NAME   "iOS"
#elif D_ENV_APPLE_IS_MACOS
    #define D_ENV_APPLE_PLATFORM_NAME   "macOS"
#elif D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_PLATFORM_NAME   "Apple (Unknown)"
#else
    #define D_ENV_APPLE_PLATFORM_NAME   "None"
#endif


// =============================================================================
// II.  MACOS SDK AND DEPLOYMENT TARGET
// =============================================================================

// Apple SDKs use __MAC_OS_X_VERSION_MIN_REQUIRED and
// __MAC_OS_X_VERSION_MAX_ALLOWED (from <Availability.h>) for version
// gating. Values encode as MMmmPP (e.g. 101500 = 10.15.0, 130000 =
// 13.0.0). Starting with macOS 11, Apple shifted to MMmm00.

// -----------------------------------------------------------------------------
// A.  macOS version constants
// -----------------------------------------------------------------------------

// D_ENV_MACOS_VER_10_9
//   constant: macOS 10.9 Mavericks.
#define D_ENV_MACOS_VER_10_9            1090

// D_ENV_MACOS_VER_10_10
//   constant: macOS 10.10 Yosemite.
#define D_ENV_MACOS_VER_10_10           101000

// D_ENV_MACOS_VER_10_11
//   constant: macOS 10.11 El Capitan (SIP introduced).
#define D_ENV_MACOS_VER_10_11           101100

// D_ENV_MACOS_VER_10_12
//   constant: macOS 10.12 Sierra (Siri, APFS preview).
#define D_ENV_MACOS_VER_10_12           101200

// D_ENV_MACOS_VER_10_13
//   constant: macOS 10.13 High Sierra (APFS default, Metal 2).
#define D_ENV_MACOS_VER_10_13           101300

// D_ENV_MACOS_VER_10_14
//   constant: macOS 10.14 Mojave (Dark Mode, Hardened Runtime).
#define D_ENV_MACOS_VER_10_14           101400

// D_ENV_MACOS_VER_10_15
//   constant: macOS 10.15 Catalina (Catalyst, notarization required,
// 32-bit apps dropped).
#define D_ENV_MACOS_VER_10_15           101500

// D_ENV_MACOS_VER_11
//   constant: macOS 11.0 Big Sur (Apple Silicon, ARM64 native, new
// version numbering).
#define D_ENV_MACOS_VER_11              110000

// D_ENV_MACOS_VER_12
//   constant: macOS 12.0 Monterey (Shortcuts, SharePlay).
#define D_ENV_MACOS_VER_12              120000

// D_ENV_MACOS_VER_13
//   constant: macOS 13.0 Ventura (Stage Manager, Passkeys).
#define D_ENV_MACOS_VER_13              130000

// D_ENV_MACOS_VER_14
//   constant: macOS 14.0 Sonoma (game mode, desktop widgets).
#define D_ENV_MACOS_VER_14              140000

// D_ENV_MACOS_VER_15
//   constant: macOS 15.0 Sequoia (iPhone mirroring, window tiling).
#define D_ENV_MACOS_VER_15              150000

// D_ENV_MACOS_VER_16
//   constant: macOS 16.0 (development / future).
#define D_ENV_MACOS_VER_16              160000


// -----------------------------------------------------------------------------
// B.  detected macOS deployment target
// -----------------------------------------------------------------------------

#if D_ENV_APPLE_IS_MACOS || D_ENV_APPLE_IS_MACCATALYST
    #ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
        #define D_ENV_MACOS_DEPLOY_TARGET                                      \
            __MAC_OS_X_VERSION_MIN_REQUIRED
        #define D_ENV_MACOS_DEPLOY_DETECTED 1
    #else
        #define D_ENV_MACOS_DEPLOY_TARGET   0
        #define D_ENV_MACOS_DEPLOY_DETECTED 0
    #endif

    #ifdef __MAC_OS_X_VERSION_MAX_ALLOWED
        #define D_ENV_MACOS_SDK_VERSION                                        \
            __MAC_OS_X_VERSION_MAX_ALLOWED
        #define D_ENV_MACOS_SDK_DETECTED    1
    #else
        #define D_ENV_MACOS_SDK_VERSION     0
        #define D_ENV_MACOS_SDK_DETECTED    0
    #endif
#else
    #define D_ENV_MACOS_DEPLOY_TARGET       0
    #define D_ENV_MACOS_DEPLOY_DETECTED     0
    #define D_ENV_MACOS_SDK_VERSION         0
    #define D_ENV_MACOS_SDK_DETECTED        0
#endif

// D_ENV_MACOS_AT_LEAST
//   macro: evaluates to 1 if the macOS deployment target is at least
// the specified version constant.
#define D_ENV_MACOS_AT_LEAST(version)                                          \
    ( D_ENV_MACOS_DEPLOY_DETECTED &&                                           \
      (D_ENV_MACOS_DEPLOY_TARGET >= (version)) )

// D_ENV_MACOS_SDK_AT_LEAST
//   macro: evaluates to 1 if the macOS SDK version is at least the
// specified version constant.
#define D_ENV_MACOS_SDK_AT_LEAST(version)                                      \
    ( D_ENV_MACOS_SDK_DETECTED &&                                              \
      (D_ENV_MACOS_SDK_VERSION >= (version)) )

// D_ENV_MACOS_DEPLOY_NAME
//   macro: human-readable deployment target name.
#if D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_16)
    #define D_ENV_MACOS_DEPLOY_NAME     "macOS 16+"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_15)
    #define D_ENV_MACOS_DEPLOY_NAME     "macOS 15 Sequoia"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_14)
    #define D_ENV_MACOS_DEPLOY_NAME     "macOS 14 Sonoma"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_13)
    #define D_ENV_MACOS_DEPLOY_NAME     "macOS 13 Ventura"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_12)
    #define D_ENV_MACOS_DEPLOY_NAME     "macOS 12 Monterey"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_11)
    #define D_ENV_MACOS_DEPLOY_NAME     "macOS 11 Big Sur"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_15)
    #define D_ENV_MACOS_DEPLOY_NAME     "macOS 10.15 Catalina"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_14)
    #define D_ENV_MACOS_DEPLOY_NAME     "macOS 10.14 Mojave"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_13)
    #define D_ENV_MACOS_DEPLOY_NAME     "macOS 10.13 High Sierra"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_12)
    #define D_ENV_MACOS_DEPLOY_NAME     "macOS 10.12 Sierra"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_11)
    #define D_ENV_MACOS_DEPLOY_NAME     "OS X 10.11 El Capitan"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_10)
    #define D_ENV_MACOS_DEPLOY_NAME     "OS X 10.10 Yosemite"
#elif D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_9)
    #define D_ENV_MACOS_DEPLOY_NAME     "OS X 10.9 Mavericks"
#elif D_ENV_APPLE_IS_MACOS
    #define D_ENV_MACOS_DEPLOY_NAME     "macOS (Legacy)"
#else
    #define D_ENV_MACOS_DEPLOY_NAME     "N/A"
#endif


// =============================================================================
// III. DARWIN KERNEL AND XNU
// =============================================================================

// D_ENV_APPLE_HAS_DARWIN
//   feature: detect if the Darwin/XNU kernel is present.
// all Apple platforms run on Darwin.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_DARWIN      1
#else
    #define D_ENV_APPLE_HAS_DARWIN      0
#endif

// D_ENV_APPLE_DARWIN_VERSION
//   constant: __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ is the
// canonical deployment version. Darwin major version can be inferred
// (Darwin 20 = macOS 11, Darwin 21 = macOS 12, etc.).
#ifdef __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
    #define D_ENV_APPLE_DARWIN_DEPLOY                                          \
        __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#else
    #define D_ENV_APPLE_DARWIN_DEPLOY   0
#endif

// D_ENV_APPLE_HAS_MACH
//   feature: detect if Mach kernel primitives are available.
// Mach ports, Mach messages, Mach VM, etc.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_MACH        1
#else
    #define D_ENV_APPLE_HAS_MACH        0
#endif

// D_ENV_APPLE_HAS_KQUEUE
//   feature: detect if kqueue/kevent is available.
// Darwin inherits kqueue from its BSD layer.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_KQUEUE      1
#else
    #define D_ENV_APPLE_HAS_KQUEUE      0
#endif

// D_ENV_APPLE_HAS_POSIX
//   feature: Darwin is POSIX-compliant.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_POSIX       1
#else
    #define D_ENV_APPLE_HAS_POSIX       0
#endif


// =============================================================================
// IV.  APPLE CLANG AND TOOLCHAIN
// =============================================================================

// D_ENV_APPLE_IS_APPLE_CLANG
//   feature: detect if the compiler is Apple Clang (distinct from
// upstream LLVM Clang). Apple Clang uses its own version numbering.
#if ( defined(__apple_build_version__) ||                                      \
      defined(D_ENV_COMPILER_APPLE_CLANG) )
    #define D_ENV_APPLE_IS_APPLE_CLANG  1
#else
    #define D_ENV_APPLE_IS_APPLE_CLANG  0
#endif

// D_ENV_APPLE_CLANG_VERSION
//   constant: Apple Clang build version (__apple_build_version__).
// this is NOT the same as the upstream Clang version.
#ifdef __apple_build_version__
    #define D_ENV_APPLE_CLANG_BUILD_VER __apple_build_version__
#else
    #define D_ENV_APPLE_CLANG_BUILD_VER 0
#endif

// D_ENV_APPLE_HAS_BLOCKS
//   feature: detect if Blocks (Apple's closure extension) are
// available. blocks are supported by Apple Clang and upstream Clang.
#if defined(__BLOCKS__)
    #define D_ENV_APPLE_HAS_BLOCKS      1
#else
    #define D_ENV_APPLE_HAS_BLOCKS      0
#endif

// D_ENV_APPLE_HAS_MODULES
//   feature: detect if Clang modules (@import) are available.
#if ( defined(__has_feature) &&                                                \
      __has_feature(modules) )
    #define D_ENV_APPLE_HAS_MODULES     1
#else
    #define D_ENV_APPLE_HAS_MODULES     0
#endif


// =============================================================================
// V.   OBJECTIVE-C AND SWIFT INTEROP
// =============================================================================

// D_ENV_APPLE_HAS_OBJC
//   feature: detect if compiling in Objective-C or Objective-C++ mode.
#if ( defined(__OBJC__) ||                                                     \
      defined(__OBJC2__) )
    #define D_ENV_APPLE_HAS_OBJC        1
#else
    #define D_ENV_APPLE_HAS_OBJC        0
#endif

// D_ENV_APPLE_HAS_OBJC2
//   feature: detect if the modern Objective-C 2.0 runtime is in use.
// Objective-C 2.0 (non-fragile ABI) is standard on 64-bit Apple.
#if defined(__OBJC2__)
    #define D_ENV_APPLE_HAS_OBJC2       1
#else
    #define D_ENV_APPLE_HAS_OBJC2       0
#endif

// D_ENV_APPLE_HAS_ARC
//   feature: detect if Automatic Reference Counting (ARC) is enabled.
#if ( defined(__has_feature) &&                                                \
      __has_feature(objc_arc) )
    #define D_ENV_APPLE_HAS_ARC         1
#else
    #define D_ENV_APPLE_HAS_ARC         0
#endif

// D_ENV_APPLE_HAS_OBJC_WEAK
//   feature: detect if Objective-C weak references are available.
#if ( defined(__has_feature) &&                                                \
      __has_feature(objc_arc_weak) )
    #define D_ENV_APPLE_HAS_OBJC_WEAK   1
#else
    #define D_ENV_APPLE_HAS_OBJC_WEAK   0
#endif

// D_ENV_APPLE_HAS_SWIFT_BRIDGING
//   feature: detect if Swift-to-C/ObjC bridging is being used.
// SWIFT_PACKAGE is defined by SPM; SWIFT_MODULE_NAME by Xcode builds.
#if ( defined(SWIFT_PACKAGE)     ||                                            \
      defined(SWIFT_MODULE_NAME) )
    #define D_ENV_APPLE_HAS_SWIFT_BRIDGING 1
#else
    #define D_ENV_APPLE_HAS_SWIFT_BRIDGING 0
#endif

// D_ENV_APPLE_HAS_NONNULL
//   feature: detect if nullability annotations (_Nonnull, _Nullable)
// are available (Xcode 6.3+, Apple Clang 6.1+).
#if ( defined(__has_feature) &&                                                \
      __has_feature(nullability) )
    #define D_ENV_APPLE_HAS_NONNULL     1
#else
    #define D_ENV_APPLE_HAS_NONNULL     0
#endif


// =============================================================================
// VI.  APPLE FRAMEWORKS — SHARED
// =============================================================================

// these frameworks are available across multiple Apple platforms
// (macOS, iOS, tvOS, watchOS, visionOS).

// -----------------------------------------------------------------------------
// A.  core frameworks
// -----------------------------------------------------------------------------

// D_ENV_APPLE_HAS_FOUNDATION
//   feature: detect if Foundation.framework is available.
// Foundation is available on all Apple platforms.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_FOUNDATION  1
#else
    #define D_ENV_APPLE_HAS_FOUNDATION  0
#endif

// D_ENV_APPLE_HAS_COREFOUNDATION
//   feature: detect if CoreFoundation.framework is available.
// CoreFoundation is available on all Apple platforms and is the
// C-level counterpart to Foundation.
#define D_ENV_APPLE_HAS_COREFOUNDATION  D_ENV_APPLE_HAS_FOUNDATION

// D_ENV_APPLE_HAS_DISPATCH
//   feature: detect if libdispatch (Grand Central Dispatch) is
// available. GCD is available on all Apple platforms.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_DISPATCH    1
#else
    #define D_ENV_APPLE_HAS_DISPATCH    0
#endif

// D_ENV_APPLE_HAS_COREDATA
//   feature: detect if CoreData.framework is available.
#if ( D_ENV_APPLE_IS_MACOS   ||                                               \
      D_ENV_APPLE_IS_IOS     ||                                                \
      D_ENV_APPLE_IS_TVOS    ||                                                \
      D_ENV_APPLE_IS_WATCHOS )
    #define D_ENV_APPLE_HAS_COREDATA    1
#else
    #define D_ENV_APPLE_HAS_COREDATA    0
#endif

// D_ENV_APPLE_HAS_SWIFTDATA
//   feature: detect if SwiftData.framework is available (Xcode 15+,
// macOS 14+, iOS 17+).
#if D_ENV_MACOS_SDK_AT_LEAST(D_ENV_MACOS_VER_14)
    #define D_ENV_APPLE_HAS_SWIFTDATA   1
#else
    #define D_ENV_APPLE_HAS_SWIFTDATA   0
#endif

// D_ENV_APPLE_HAS_COMBINE
//   feature: detect if Combine.framework is available (macOS 10.15+,
// iOS 13+).
#if D_ENV_MACOS_SDK_AT_LEAST(D_ENV_MACOS_VER_10_15)
    #define D_ENV_APPLE_HAS_COMBINE     1
#elif D_ENV_APPLE_IS_IOS
    // iOS 13+ has Combine; SDK gate handled in env_ios.h
    #define D_ENV_APPLE_HAS_COMBINE     1
#else
    #define D_ENV_APPLE_HAS_COMBINE     0
#endif


// -----------------------------------------------------------------------------
// B.  security frameworks
// -----------------------------------------------------------------------------

// D_ENV_APPLE_HAS_SECURITY
//   feature: detect if Security.framework is available (Keychain,
// certificates, trust evaluation, code signing APIs).
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_SECURITY    1
#else
    #define D_ENV_APPLE_HAS_SECURITY    0
#endif

// D_ENV_APPLE_HAS_CRYPTOKIT
//   feature: detect if CryptoKit.framework is available
// (macOS 10.15+, iOS 13+).
#if D_ENV_MACOS_SDK_AT_LEAST(D_ENV_MACOS_VER_10_15)
    #define D_ENV_APPLE_HAS_CRYPTOKIT   1
#elif ( D_ENV_APPLE_IS_IOS   ||                                                \
        D_ENV_APPLE_IS_TVOS  ||                                                \
        D_ENV_APPLE_IS_WATCHOS )
    #define D_ENV_APPLE_HAS_CRYPTOKIT   1
#else
    #define D_ENV_APPLE_HAS_CRYPTOKIT   0
#endif

// D_ENV_APPLE_HAS_COMMONCRYPTO
//   feature: detect if CommonCrypto (CC_MD5, CC_SHA256, CCCrypt) is
// available. present on all Apple platforms.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_COMMONCRYPTO 1
#else
    #define D_ENV_APPLE_HAS_COMMONCRYPTO 0
#endif

// D_ENV_APPLE_HAS_SECURE_TRANSPORT
//   feature: detect if Secure Transport (legacy TLS) is available.
// deprecated in macOS 10.15 / iOS 13 in favor of Network.framework.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_SECURE_TRANSPORT 1
#else
    #define D_ENV_APPLE_HAS_SECURE_TRANSPORT 0
#endif


// -----------------------------------------------------------------------------
// C.  networking frameworks
// -----------------------------------------------------------------------------

// D_ENV_APPLE_HAS_CFNETWORK
//   feature: detect if CFNetwork.framework is available.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_CFNETWORK   1
#else
    #define D_ENV_APPLE_HAS_CFNETWORK   0
#endif

// D_ENV_APPLE_HAS_NETWORK_FRAMEWORK
//   feature: detect if Network.framework (modern networking, QUIC) is
// available (macOS 10.14+, iOS 12+).
#if D_ENV_MACOS_SDK_AT_LEAST(D_ENV_MACOS_VER_10_14)
    #define D_ENV_APPLE_HAS_NETWORK_FRAMEWORK 1
#elif D_ENV_APPLE_IS_IOS
    #define D_ENV_APPLE_HAS_NETWORK_FRAMEWORK 1
#else
    #define D_ENV_APPLE_HAS_NETWORK_FRAMEWORK 0
#endif

// D_ENV_APPLE_HAS_MULTIPEER
//   feature: detect if MultipeerConnectivity.framework is available.
#if ( D_ENV_APPLE_IS_MACOS ||                                                 \
      D_ENV_APPLE_IS_IOS   ||                                                  \
      D_ENV_APPLE_IS_TVOS )
    #define D_ENV_APPLE_HAS_MULTIPEER   1
#else
    #define D_ENV_APPLE_HAS_MULTIPEER   0
#endif


// -----------------------------------------------------------------------------
// D.  graphics and media frameworks
// -----------------------------------------------------------------------------

// D_ENV_APPLE_HAS_METAL
//   feature: detect if Metal.framework (GPU API) is available
// (macOS 10.11+, iOS 8+).
#if D_ENV_MACOS_SDK_AT_LEAST(D_ENV_MACOS_VER_10_11)
    #define D_ENV_APPLE_HAS_METAL       1
#elif ( D_ENV_APPLE_IS_IOS   ||                                                \
        D_ENV_APPLE_IS_TVOS  ||                                                \
        D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_APPLE_HAS_METAL       1
#else
    #define D_ENV_APPLE_HAS_METAL       0
#endif

// D_ENV_APPLE_HAS_METAL3
//   feature: detect if Metal 3 is available (macOS 13+, iOS 16+).
#if D_ENV_MACOS_SDK_AT_LEAST(D_ENV_MACOS_VER_13)
    #define D_ENV_APPLE_HAS_METAL3      1
#else
    #define D_ENV_APPLE_HAS_METAL3      0
#endif

// D_ENV_APPLE_HAS_COREGRAPHICS
//   feature: detect if CoreGraphics.framework (Quartz 2D) is available.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_COREGRAPHICS 1
#else
    #define D_ENV_APPLE_HAS_COREGRAPHICS 0
#endif

// D_ENV_APPLE_HAS_COREIMAGE
//   feature: detect if CoreImage.framework is available.
#if ( D_ENV_APPLE_IS_MACOS ||                                                 \
      D_ENV_APPLE_IS_IOS   ||                                                  \
      D_ENV_APPLE_IS_TVOS )
    #define D_ENV_APPLE_HAS_COREIMAGE   1
#else
    #define D_ENV_APPLE_HAS_COREIMAGE   0
#endif

// D_ENV_APPLE_HAS_COREANIMATION
//   feature: detect if CoreAnimation (QuartzCore.framework) is
// available.
#if ( D_ENV_APPLE_IS_MACOS ||                                                 \
      D_ENV_APPLE_IS_IOS   ||                                                  \
      D_ENV_APPLE_IS_TVOS )
    #define D_ENV_APPLE_HAS_COREANIMATION 1
#else
    #define D_ENV_APPLE_HAS_COREANIMATION 0
#endif

// D_ENV_APPLE_HAS_COREAUDIO
//   feature: detect if CoreAudio.framework is available.
#if ( D_ENV_APPLE_IS_MACOS ||                                                 \
      D_ENV_APPLE_IS_IOS   ||                                                  \
      D_ENV_APPLE_IS_TVOS  ||                                                  \
      D_ENV_APPLE_IS_WATCHOS )
    #define D_ENV_APPLE_HAS_COREAUDIO   1
#else
    #define D_ENV_APPLE_HAS_COREAUDIO   0
#endif

// D_ENV_APPLE_HAS_AVFOUNDATION
//   feature: detect if AVFoundation.framework is available.
#if ( D_ENV_APPLE_IS_MACOS ||                                                 \
      D_ENV_APPLE_IS_IOS   ||                                                  \
      D_ENV_APPLE_IS_TVOS  ||                                                  \
      D_ENV_APPLE_IS_WATCHOS ||                                                \
      D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_APPLE_HAS_AVFOUNDATION 1
#else
    #define D_ENV_APPLE_HAS_AVFOUNDATION 0
#endif

// D_ENV_APPLE_HAS_OPENGL
//   feature: detect if OpenGL (AppKit/NSOpenGL) is available.
// deprecated in macOS 10.14 in favor of Metal; still available.
// not available on iOS (which uses OpenGL ES, then Metal).
#if D_ENV_APPLE_IS_MACOS
    #define D_ENV_APPLE_HAS_OPENGL      1
#else
    #define D_ENV_APPLE_HAS_OPENGL      0
#endif

// D_ENV_APPLE_HAS_OPENGL_ES
//   feature: detect if OpenGL ES is available.
// deprecated in iOS 12 in favor of Metal; still available.
#if ( D_ENV_APPLE_IS_IOS  ||                                                   \
      D_ENV_APPLE_IS_TVOS )
    #define D_ENV_APPLE_HAS_OPENGL_ES   1
#else
    #define D_ENV_APPLE_HAS_OPENGL_ES   0
#endif

// D_ENV_APPLE_HAS_VULKAN
//   feature: detect if Vulkan (via MoltenVK) is available.
// MoltenVK translates Vulkan to Metal. not an Apple framework, but
// commonly used.
#if ( D_ENV_APPLE_HAS_METAL &&                                                \
      defined(VK_USE_PLATFORM_METAL_EXT) )
    #define D_ENV_APPLE_HAS_VULKAN      1
#else
    #define D_ENV_APPLE_HAS_VULKAN      0
#endif


// -----------------------------------------------------------------------------
// E.  UI frameworks
// -----------------------------------------------------------------------------

// D_ENV_APPLE_HAS_APPKIT
//   feature: detect if AppKit.framework (macOS UI) is available.
#if ( D_ENV_APPLE_IS_MACOS &&                                                 \
      !D_ENV_APPLE_IS_MACCATALYST )
    #define D_ENV_APPLE_HAS_APPKIT      1
#else
    #define D_ENV_APPLE_HAS_APPKIT      0
#endif

// D_ENV_APPLE_HAS_UIKIT
//   feature: detect if UIKit.framework (iOS/tvOS/Catalyst UI) is
// available.
#if ( D_ENV_APPLE_IS_IOS        ||                                             \
      D_ENV_APPLE_IS_TVOS       ||                                             \
      D_ENV_APPLE_IS_MACCATALYST ||                                            \
      D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_APPLE_HAS_UIKIT       1
#else
    #define D_ENV_APPLE_HAS_UIKIT       0
#endif

// D_ENV_APPLE_HAS_SWIFTUI
//   feature: detect if SwiftUI.framework is available
// (macOS 10.15+, iOS 13+).
#if D_ENV_MACOS_SDK_AT_LEAST(D_ENV_MACOS_VER_10_15)
    #define D_ENV_APPLE_HAS_SWIFTUI     1
#elif ( D_ENV_APPLE_IS_IOS      ||                                             \
        D_ENV_APPLE_IS_TVOS     ||                                             \
        D_ENV_APPLE_IS_WATCHOS  ||                                             \
        D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_APPLE_HAS_SWIFTUI     1
#else
    #define D_ENV_APPLE_HAS_SWIFTUI     0
#endif

// D_ENV_APPLE_HAS_WATCHKIT
//   feature: detect if WatchKit.framework is available.
#if D_ENV_APPLE_IS_WATCHOS
    #define D_ENV_APPLE_HAS_WATCHKIT    1
#else
    #define D_ENV_APPLE_HAS_WATCHKIT    0
#endif


// =============================================================================
// VII. MACOS-SPECIFIC FEATURES
// =============================================================================

// -----------------------------------------------------------------------------
// A.  App Sandbox and Hardened Runtime
// -----------------------------------------------------------------------------

// D_ENV_MACOS_HAS_APP_SANDBOX
//   feature: detect if App Sandbox entitlements are supported.
// App Sandbox is available since macOS 10.7; required for Mac App Store.
#if D_ENV_APPLE_IS_MACOS
    #define D_ENV_MACOS_HAS_APP_SANDBOX 1
#else
    #define D_ENV_MACOS_HAS_APP_SANDBOX 0
#endif

// D_ENV_MACOS_HAS_HARDENED_RUNTIME
//   feature: detect if the Hardened Runtime is supported.
// introduced in macOS 10.14; required for notarization.
#if D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_14)
    #define D_ENV_MACOS_HAS_HARDENED_RUNTIME 1
#elif ( D_ENV_APPLE_IS_MACOS &&                                                \
        !D_ENV_MACOS_DEPLOY_DETECTED )
    #define D_ENV_MACOS_HAS_HARDENED_RUNTIME 1
#else
    #define D_ENV_MACOS_HAS_HARDENED_RUNTIME 0
#endif

// D_ENV_MACOS_HAS_NOTARIZATION
//   feature: detect if notarization is expected to be required.
// Apple began requiring notarization for distribution in macOS 10.15.
#if D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_15)
    #define D_ENV_MACOS_HAS_NOTARIZATION 1
#elif ( D_ENV_APPLE_IS_MACOS &&                                                \
        !D_ENV_MACOS_DEPLOY_DETECTED )
    #define D_ENV_MACOS_HAS_NOTARIZATION 1
#else
    #define D_ENV_MACOS_HAS_NOTARIZATION 0
#endif

// D_ENV_MACOS_HAS_SIP
//   feature: detect if System Integrity Protection (SIP) is expected.
// SIP was introduced in macOS 10.11.
#if D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_11)
    #define D_ENV_MACOS_HAS_SIP         1
#elif ( D_ENV_APPLE_IS_MACOS &&                                                \
        !D_ENV_MACOS_DEPLOY_DETECTED )
    #define D_ENV_MACOS_HAS_SIP         1
#else
    #define D_ENV_MACOS_HAS_SIP         0
#endif

// D_ENV_MACOS_HAS_TCC
//   feature: detect if Transparency, Consent, and Control (TCC) is
// expected (privacy permission prompts). TCC matured in macOS 10.14+.
#if D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_14)
    #define D_ENV_MACOS_HAS_TCC         1
#elif ( D_ENV_APPLE_IS_MACOS &&                                                \
        !D_ENV_MACOS_DEPLOY_DETECTED )
    #define D_ENV_MACOS_HAS_TCC         1
#else
    #define D_ENV_MACOS_HAS_TCC         0
#endif


// -----------------------------------------------------------------------------
// B.  macOS system APIs
// -----------------------------------------------------------------------------

// D_ENV_MACOS_HAS_SERVICEMGMT
//   feature: detect if ServiceManagement.framework (login items, launch
// daemons) is available.
#if D_ENV_APPLE_IS_MACOS
    #define D_ENV_MACOS_HAS_SERVICEMGMT 1
#else
    #define D_ENV_MACOS_HAS_SERVICEMGMT 0
#endif

// D_ENV_MACOS_HAS_IOKIT
//   feature: detect if IOKit.framework (hardware/device access) is
// available.
#if D_ENV_APPLE_IS_MACOS
    #define D_ENV_MACOS_HAS_IOKIT       1
#else
    #define D_ENV_MACOS_HAS_IOKIT       0
#endif

// D_ENV_MACOS_HAS_DISKARBITER
//   feature: detect if DiskArbitration.framework is available.
#if D_ENV_APPLE_IS_MACOS
    #define D_ENV_MACOS_HAS_DISKARBITER 1
#else
    #define D_ENV_MACOS_HAS_DISKARBITER 0
#endif

// D_ENV_MACOS_HAS_SYSTEMCONFIG
//   feature: detect if SystemConfiguration.framework (network config,
// reachability) is available.
#if ( D_ENV_APPLE_IS_MACOS ||                                                 \
      D_ENV_APPLE_IS_IOS )
    #define D_ENV_MACOS_HAS_SYSTEMCONFIG 1
#else
    #define D_ENV_MACOS_HAS_SYSTEMCONFIG 0
#endif

// D_ENV_MACOS_HAS_ENDPOINTSECURITY
//   feature: detect if EndpointSecurity.framework is available
// (macOS 10.15+). used by security products and system extensions.
#if D_ENV_MACOS_SDK_AT_LEAST(D_ENV_MACOS_VER_10_15)
    #define D_ENV_MACOS_HAS_ENDPOINTSECURITY 1
#else
    #define D_ENV_MACOS_HAS_ENDPOINTSECURITY 0
#endif

// D_ENV_MACOS_HAS_HYPERVISOR
//   feature: detect if Hypervisor.framework (lightweight
// virtualization) is available (macOS 10.10+).
#if D_ENV_MACOS_SDK_AT_LEAST(D_ENV_MACOS_VER_10_10)
    #define D_ENV_MACOS_HAS_HYPERVISOR  1
#elif ( D_ENV_APPLE_IS_MACOS &&                                                \
        !D_ENV_MACOS_SDK_DETECTED )
    #define D_ENV_MACOS_HAS_HYPERVISOR  1
#else
    #define D_ENV_MACOS_HAS_HYPERVISOR  0
#endif

// D_ENV_MACOS_HAS_VIRTUALIZATION
//   feature: detect if Virtualization.framework is available
// (macOS 11+). high-level VM API for Linux and macOS guests.
#if D_ENV_MACOS_SDK_AT_LEAST(D_ENV_MACOS_VER_11)
    #define D_ENV_MACOS_HAS_VIRTUALIZATION 1
#else
    #define D_ENV_MACOS_HAS_VIRTUALIZATION 0
#endif


// =============================================================================
// VIII. HARDWARE DETECTION
// =============================================================================

// D_ENV_APPLE_IS_ARM64
//   feature: detect if building for Apple Silicon (ARM64).
#if ( defined(__arm64__)   ||                                                  \
      defined(__aarch64__) )
    #define D_ENV_APPLE_IS_ARM64        1
#else
    #define D_ENV_APPLE_IS_ARM64        0
#endif

// D_ENV_APPLE_IS_X86_64
//   feature: detect if building for Intel x86-64.
#if ( defined(__x86_64__) ||                                                   \
      defined(__x86_64) )
    #define D_ENV_APPLE_IS_X86_64       1
#else
    #define D_ENV_APPLE_IS_X86_64       0
#endif

// D_ENV_APPLE_IS_UNIVERSAL
//   feature: detect if building a Universal Binary (fat binary with
// multiple architectures).
// note: this is a heuristic. universal binaries are built by lipo
// after separate compilations; each slice compiles with one arch.
// this detects if the build system has indicated universal intent.
#if defined(D_CFG_APPLE_UNIVERSAL)
    #define D_ENV_APPLE_IS_UNIVERSAL    1
#else
    #define D_ENV_APPLE_IS_UNIVERSAL    0
#endif

// D_ENV_APPLE_MAYBE_ROSETTA
//   feature: detect if running under Rosetta 2 translation is
// plausible (x86-64 binary on macOS 11+ deployment target).
// actual Rosetta detection requires runtime sysctl queries.
#if ( D_ENV_APPLE_IS_X86_64 &&                                                \
      D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_11) )
    #define D_ENV_APPLE_MAYBE_ROSETTA   1
#else
    #define D_ENV_APPLE_MAYBE_ROSETTA   0
#endif

// D_ENV_APPLE_HAS_NEON
//   feature: detect if ARM NEON is available (always on Apple Silicon).
#if D_ENV_APPLE_IS_ARM64
    #define D_ENV_APPLE_HAS_NEON        1
#else
    #define D_ENV_APPLE_HAS_NEON        0
#endif

// D_ENV_APPLE_HAS_AMX
//   feature: detect if Apple's AMX (matrix coprocessor) is likely
// present. AMX is on all Apple Silicon Macs (M1+). no public header
// exists; detection is architecture-based.
#if ( D_ENV_APPLE_IS_ARM64 &&                                                 \
      D_ENV_APPLE_IS_MACOS )
    #define D_ENV_APPLE_HAS_AMX         1
#else
    #define D_ENV_APPLE_HAS_AMX         0
#endif


// =============================================================================
// IX.  APPLE BSD/POSIX EXTENSIONS
// =============================================================================

// Darwin inherits from FreeBSD and provides many BSD functions.

// D_ENV_APPLE_HAS_ARC4RANDOM
//   feature: arc4random() is available on all Apple platforms.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_ARC4RANDOM  1
#else
    #define D_ENV_APPLE_HAS_ARC4RANDOM  0
#endif

// D_ENV_APPLE_HAS_GETENTROPY
//   feature: detect if getentropy() is available (macOS 10.12+,
// iOS 10+).
#if D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_12)
    #define D_ENV_APPLE_HAS_GETENTROPY  1
#elif ( D_ENV_APPLE_IS_IOS      ||                                             \
        D_ENV_APPLE_IS_TVOS     ||                                             \
        D_ENV_APPLE_IS_WATCHOS  ||                                             \
        D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_APPLE_HAS_GETENTROPY  1
#elif ( D_ENV_APPLE_IS_MACOS &&                                                \
        !D_ENV_MACOS_DEPLOY_DETECTED )
    #define D_ENV_APPLE_HAS_GETENTROPY  1
#else
    #define D_ENV_APPLE_HAS_GETENTROPY  0
#endif

// D_ENV_APPLE_HAS_STRLCPY
//   feature: strlcpy/strlcat are available on all Apple platforms.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_STRLCPY     1
    #define D_ENV_APPLE_HAS_STRLCAT     1
#else
    #define D_ENV_APPLE_HAS_STRLCPY     0
    #define D_ENV_APPLE_HAS_STRLCAT     0
#endif

// D_ENV_APPLE_HAS_POSIX_MEMALIGN
//   feature: posix_memalign() is available on all Apple platforms.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_POSIX_MEMALIGN 1
#else
    #define D_ENV_APPLE_HAS_POSIX_MEMALIGN 0
#endif

// D_ENV_APPLE_HAS_PTHREAD
//   feature: POSIX threads are available on all Apple platforms.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_PTHREAD     1
#else
    #define D_ENV_APPLE_HAS_PTHREAD     0
#endif

// D_ENV_APPLE_HAS_PTHREAD_NP
//   feature: Apple non-portable pthread extensions
// (pthread_setname_np, pthread_threadid_np, etc.).
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_PTHREAD_NP  1
#else
    #define D_ENV_APPLE_HAS_PTHREAD_NP  0
#endif

// D_ENV_APPLE_HAS_SYSCTL
//   feature: sysctl/sysctlbyname are available on Apple platforms.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_SYSCTL      1
#else
    #define D_ENV_APPLE_HAS_SYSCTL      0
#endif

// D_ENV_APPLE_HAS_DYLD
//   feature: detect if dyld (dynamic linker) APIs are available
// (_dyld_image_count, dlopen, etc.).
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_DYLD        1
#else
    #define D_ENV_APPLE_HAS_DYLD        0
#endif


// =============================================================================
// X.   FILESYSTEM FEATURES
// =============================================================================

// D_ENV_APPLE_HAS_APFS
//   feature: detect if APFS is expected as the default filesystem.
// APFS became default in macOS 10.13 / iOS 10.3.
#if D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_10_13)
    #define D_ENV_APPLE_HAS_APFS        1
#elif ( D_ENV_APPLE_IS_IOS      ||                                             \
        D_ENV_APPLE_IS_TVOS     ||                                             \
        D_ENV_APPLE_IS_WATCHOS  ||                                             \
        D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_APPLE_HAS_APFS        1
#elif ( D_ENV_APPLE_IS_MACOS &&                                                \
        !D_ENV_MACOS_DEPLOY_DETECTED )
    #define D_ENV_APPLE_HAS_APFS        1
#else
    #define D_ENV_APPLE_HAS_APFS        0
#endif

// D_ENV_APPLE_HAS_FSEVENT
//   feature: detect if FSEvents (filesystem event monitoring) is
// available. macOS only (iOS uses different mechanisms).
#if D_ENV_APPLE_IS_MACOS
    #define D_ENV_APPLE_HAS_FSEVENT     1
#else
    #define D_ENV_APPLE_HAS_FSEVENT     0
#endif

// D_ENV_APPLE_HAS_XATTR
//   feature: detect if extended attributes (xattr) are available.
#if D_ENV_APPLE_IS_APPLE
    #define D_ENV_APPLE_HAS_XATTR       1
#else
    #define D_ENV_APPLE_HAS_XATTR       0
#endif

// D_ENV_APPLE_HAS_SPOTLIGHT
//   feature: detect if Spotlight (mdfind, MDQuery) APIs are available.
#if ( D_ENV_APPLE_IS_MACOS ||                                                 \
      D_ENV_APPLE_IS_IOS )
    #define D_ENV_APPLE_HAS_SPOTLIGHT   1
#else
    #define D_ENV_APPLE_HAS_SPOTLIGHT   0
#endif


// =============================================================================
// XI.  RUNTIME DETECTION FUNCTIONS
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// d_env_apple_get_os_version
//   function: returns the runtime OS version string.
//   returns: version string (e.g. "15.1.0" for macOS 15.1), or
// "Unknown" if unavailable.
const char* d_env_apple_get_os_version(void);

// d_env_apple_get_platform_name
//   function: returns the runtime platform name.
//   returns: "macOS", "iOS", "tvOS", "watchOS", "visionOS", or
// "Apple (Unknown)".
const char* d_env_apple_get_platform_name(void);

// d_env_apple_get_darwin_version
//   function: returns the Darwin/XNU kernel version string.
//   returns: uname release string, or "Unknown".
const char* d_env_apple_get_darwin_version(void);

// d_env_apple_is_rosetta
//   function: detects at runtime if running under Rosetta 2 translation.
//   returns: 1 if Rosetta 2 is active, 0 otherwise.
int d_env_apple_is_rosetta(void);

// d_env_apple_is_apple_silicon
//   function: detects at runtime if running on Apple Silicon hardware.
//   returns: 1 if Apple Silicon, 0 if Intel (even under Rosetta).
int d_env_apple_is_apple_silicon(void);

// d_env_apple_has_framework
//   function: attempts to detect a framework at runtime via dlopen.
//   params:
//     framework_name - framework name (e.g. "Metal", "Security")
//   returns: 1 if the framework is loadable, 0 otherwise.
int d_env_apple_has_framework(const char* framework_name);

// d_env_apple_print_info
//   function: prints detailed information about the detected Apple
// environment.
void d_env_apple_print_info(void);

#ifdef __cplusplus
}
#endif


// =============================================================================
// XII. CONVENIENCE MACROS
// =============================================================================

// D_ENV_APPLE_IS_DESKTOP
//   macro: evaluates to 1 if the platform is a desktop OS (macOS).
#define D_ENV_APPLE_IS_DESKTOP()                                               \
    ( D_ENV_APPLE_IS_MACOS &&                                                  \
      !D_ENV_APPLE_IS_MACCATALYST )

// D_ENV_APPLE_IS_MOBILE
//   macro: evaluates to 1 if the platform is a mobile/embedded Apple
// OS (iOS, tvOS, watchOS, visionOS).
#define D_ENV_APPLE_IS_MOBILE()                                                \
    ( D_ENV_APPLE_IS_IOS     ||                                                \
      D_ENV_APPLE_IS_TVOS    ||                                                \
      D_ENV_APPLE_IS_WATCHOS ||                                                \
      D_ENV_APPLE_IS_VISIONOS )

// D_ENV_APPLE_IS_MODERN
//   macro: evaluates to 1 if the deployment target is a modern Apple
// OS (macOS 11+ / Apple Silicon era, or any current mobile OS).
#define D_ENV_APPLE_IS_MODERN()                                                \
    ( D_ENV_MACOS_AT_LEAST(D_ENV_MACOS_VER_11) ||                              \
      D_ENV_APPLE_IS_IOS                        ||                              \
      D_ENV_APPLE_IS_TVOS                       ||                              \
      D_ENV_APPLE_IS_WATCHOS                    ||                              \
      D_ENV_APPLE_IS_VISIONOS )

// D_ENV_APPLE_HAS_SECURE_RANDOM
//   macro: evaluates to 1 if a strong secure random source is
// available (arc4random or getentropy).
#define D_ENV_APPLE_HAS_SECURE_RANDOM()                                        \
    ( D_ENV_APPLE_HAS_ARC4RANDOM ||                                            \
      D_ENV_APPLE_HAS_GETENTROPY )

// D_ENV_APPLE_HAS_GPU_API
//   macro: evaluates to 1 if a GPU API (Metal, OpenGL, or OpenGL ES)
// is available.
#define D_ENV_APPLE_HAS_GPU_API()                                              \
    ( D_ENV_APPLE_HAS_METAL     ||                                             \
      D_ENV_APPLE_HAS_OPENGL   ||                                              \
      D_ENV_APPLE_HAS_OPENGL_ES )

// D_ENV_MACOS_IS_HARDENED
//   macro: evaluates to 1 if macOS hardening features are all expected
// to be present (SIP + Hardened Runtime + notarization).
#define D_ENV_MACOS_IS_HARDENED()                                              \
    ( D_ENV_MACOS_HAS_SIP              &&                                      \
      D_ENV_MACOS_HAS_HARDENED_RUNTIME &&                                      \
      D_ENV_MACOS_HAS_NOTARIZATION )


#endif  // DJINTERP_ENV_APPLE_
