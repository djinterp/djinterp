/*******************************************************************************
* djinterp [env]                                                       env_ios.h
*
* djinterp iOS environment detection.
*   Compile-time detection of the mobile Apple compilation environment: iOS,
* iPadOS, tvOS, watchOS, and visionOS. It detects:
*     - iOS SDK and deployment target versions
*     - tvOS, watchOS, and visionOS SDK versions
*     - UIKit feature availability across versions
*     - device capability indicators (camera, GPS, haptics, LiDAR)
*     - iOS-specific frameworks (ARKit, CoreMotion, HealthKit, etc.)
*     - App Store and distribution constraints
*     - extension and widget support
*     - multitasking and windowing capabilities
*   Versions come from <Availability.h>'s __IPHONE_OS_VERSION_MIN_REQUIRED,
* __TV_OS_VERSION_MIN_REQUIRED, __WATCH_OS_VERSION_MIN_REQUIRED, and
* __VISION_OS_VERSION_MIN_REQUIRED families; see section 1.
*   It includes env_apple.h for the features shared across Apple platforms.
* Include it where mobile Apple detail is needed; from env.h's directory, for
* example:
*     #if (D_ENV_OS_ID == D_ENV_OS_FLAG_IOS)
*         #include "./os/env_ios.h"
*     #endif
*   Naming: D_ENV_IOS_, D_ENV_TVOS_, D_ENV_WATCHOS_, and D_ENV_VISOS_ prefixes
* are iOS / iPadOS-, tvOS-, watchOS-, and visionOS-specific; D_ENV_MOBILE_ is
* shared across the mobile Apple platforms.
*
* path:      /inc/djinterp/env/os/env_ios.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.28
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  IOS DEPLOYMENT TARGET
    ---------------------
    1.  iOS version constants
         1.  D_ENV_IOS_VER_11
         2.  D_ENV_IOS_VER_12
         3.  D_ENV_IOS_VER_13
         4.  D_ENV_IOS_VER_14
         5.  D_ENV_IOS_VER_15
         6.  D_ENV_IOS_VER_16
         7.  D_ENV_IOS_VER_17
         8.  D_ENV_IOS_VER_18
         9.  D_ENV_IOS_VER_19
    2.  Detected iOS deployment target and SDK
         1.  D_ENV_IOS_DEPLOY_TARGET / D_ENV_IOS_SDK_VERSION
         2.  D_ENV_IOS_AT_LEAST
         3.  D_ENV_IOS_SDK_AT_LEAST
         4.  D_ENV_IOS_DEPLOY_NAME
2.  TVOS, WATCHOS, AND VISIONOS TARGETS
    -----------------------------------
    1.  tvOS version constants
         1.  D_ENV_TVOS_VER_12
         2.  D_ENV_TVOS_VER_13
         3.  D_ENV_TVOS_VER_14
         4.  D_ENV_TVOS_VER_15
         5.  D_ENV_TVOS_VER_16
         6.  D_ENV_TVOS_VER_17
         7.  D_ENV_TVOS_VER_18
    2.  Detected tvOS deployment target and SDK
         1.  D_ENV_TVOS_DEPLOY_TARGET / D_ENV_TVOS_SDK_VERSION
         2.  D_ENV_TVOS_AT_LEAST
    3.  watchOS version constants
         1.  D_ENV_WATCHOS_VER_5
         2.  D_ENV_WATCHOS_VER_6
         3.  D_ENV_WATCHOS_VER_7
         4.  D_ENV_WATCHOS_VER_8
         5.  D_ENV_WATCHOS_VER_9
         6.  D_ENV_WATCHOS_VER_10
         7.  D_ENV_WATCHOS_VER_11
    4.  Detected watchOS deployment target and SDK
         1.  D_ENV_WATCHOS_DEPLOY_TARGET / D_ENV_WATCHOS_SDK_VERSION
         2.  D_ENV_WATCHOS_AT_LEAST
    5.  visionOS version constants
         1.  D_ENV_VISOS_VER_1
         2.  D_ENV_VISOS_VER_2
    6.  Detected visionOS deployment target
         1.  D_ENV_VISOS_DEPLOY_TARGET / D_ENV_VISOS_DEPLOY_DETECTED
         2.  D_ENV_VISOS_AT_LEAST
3.  IOS AND IPADOS FRAMEWORKS
    -------------------------
    1.  Augmented reality
         1.  D_ENV_IOS_HAS_ARKIT
         2.  D_ENV_IOS_HAS_ARKIT_5
         3.  D_ENV_IOS_HAS_REALITYKIT
    2.  Machine learning
         1.  D_ENV_IOS_HAS_COREML
         2.  D_ENV_IOS_HAS_VISION
         3.  D_ENV_IOS_HAS_NATURALLANGUAGE
    3.  Health and motion
         1.  D_ENV_IOS_HAS_HEALTHKIT
         2.  D_ENV_IOS_HAS_COREMOTION
         3.  D_ENV_IOS_HAS_CORELOCATION
    4.  Notifications and communication
         1.  D_ENV_IOS_HAS_USERNOTIFICATIONS
         2.  D_ENV_IOS_HAS_CALLKIT
         3.  D_ENV_IOS_HAS_MESSAGEUI
    5.  Maps and navigation
         1.  D_ENV_IOS_HAS_MAPKIT
4.  EXTENSIONS AND COMMERCE
    -----------------------
    1.  Widgets, App Clips, and extensions
         1.  D_ENV_IOS_HAS_WIDGETKIT
         2.  D_ENV_IOS_HAS_INTERACTIVE_WIDGETS
         3.  D_ENV_IOS_HAS_APPCLIP
         4.  D_ENV_IOS_HAS_ACTIVITYKIT
         5.  D_ENV_IOS_HAS_TIPKIT
         6.  D_ENV_IOS_HAS_APP_INTENTS
         7.  D_ENV_IOS_IS_APP_EXTENSION
    2.  StoreKit and in-app purchase
         1.  D_ENV_IOS_HAS_STOREKIT
         2.  D_ENV_IOS_HAS_STOREKIT2
5.  DEVICE AND WINDOWING
    --------------------
    1.  Device capabilities
         1.  D_ENV_IOS_HAS_CAMERA_API
         2.  D_ENV_IOS_HAS_HAPTICS_API
         3.  D_ENV_IOS_HAS_LIDAR_API
         4.  D_ENV_IOS_HAS_NFC_API
         5.  D_ENV_IOS_HAS_FACEID_API
         6.  D_ENV_IOS_HAS_BLUETOOTH_API
         7.  D_ENV_IOS_HAS_UWB_API
    2.  Multitasking and windowing
         1.  D_ENV_IOS_HAS_MULTITASKING
         2.  D_ENV_IOS_HAS_SCENES
         3.  D_ENV_IOS_HAS_STAGE_MANAGER
         4.  D_ENV_IOS_HAS_EXTERNAL_DISPLAY
6.  VISIONOS AND DISTRIBUTION
    -------------------------
    1.  visionOS-specific features
         1.  D_ENV_VISOS_HAS_REALITYKIT
         2.  D_ENV_VISOS_HAS_ARKIT_SPATIAL
         3.  D_ENV_VISOS_HAS_COMPOSITOR
         4.  D_ENV_VISOS_HAS_VOLUMES
         5.  D_ENV_VISOS_HAS_IMMERSIVE_SPACE
    2.  App Store and distribution
         1.  D_ENV_MOBILE_IS_APP_STORE_BUILD
         2.  D_ENV_MOBILE_HAS_TESTFLIGHT
         3.  D_ENV_MOBILE_HAS_ENTERPRISE_DIST
         4.  D_ENV_MOBILE_NO_JIT
         5.  D_ENV_MOBILE_NO_DLOPEN
         6.  D_ENV_MOBILE_NO_FORK
7.  RUNTIME DETECTION AND CONVENIENCE
    ---------------------------------
    1.  Runtime queries
    2.  Combined predicates
         1.  D_ENV_IOS_IS_MODERN
         2.  D_ENV_IOS_HAS_SWIFTUI_FULL
         3.  D_ENV_IOS_HAS_MODERN_CONCURRENCY
         4.  D_ENV_MOBILE_IS_SANDBOXED
         5.  D_ENV_MOBILE_IS_CONSTRAINED
         6.  D_ENV_IOS_HAS_SPATIAL_COMPUTING
*/

#ifndef DJINTERP_ENV_OS_ENV_IOS_H
#define DJINTERP_ENV_OS_ENV_IOS_H 1

// djinterp
#include "./env_apple.h"  // D_ENV_APPLE_IS_*, D_ENV_LANG_USING_CPP (via env.h)


//==============================================================================
// 1.  IOS DEPLOYMENT TARGET
//==============================================================================
// iOS gates versions with <Availability.h>'s __IPHONE_OS_VERSION_MIN_REQUIRED
// and __IPHONE_OS_VERSION_MAX_ALLOWED, encoded MMmm00 (160000 is iOS 16.0).
// This header, like env_apple.h, does not include <Availability.h>: unless it
// was included first, every deployment and SDK version here and in section 2
// reads 0.


// 1.1    iOS version constants
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_IOS_VER_11
//   constant: iOS 11.0 (ARKit, Core ML, drag-and-drop on iPad).
#define D_ENV_IOS_VER_11                110000

// 1.1.2
// D_ENV_IOS_VER_12
//   constant: iOS 12.0 (Siri Shortcuts, grouped notifications, ARKit 2).
#define D_ENV_IOS_VER_12                120000

// 1.1.3
// D_ENV_IOS_VER_13
//   constant: iOS 13.0 (Dark Mode, SwiftUI, Combine, iPadOS split,
// Sign in with Apple, SF Symbols).
#define D_ENV_IOS_VER_13                130000

// 1.1.4
// D_ENV_IOS_VER_14
//   constant: iOS 14.0 (WidgetKit, App Clips, App Library, compact UI).
#define D_ENV_IOS_VER_14                140000

// 1.1.5
// D_ENV_IOS_VER_15
//   constant: iOS 15.0 (Focus, SharePlay, async/await in system
// frameworks, StoreKit 2).
#define D_ENV_IOS_VER_15                150000

// 1.1.6
// D_ENV_IOS_VER_16
//   constant: iOS 16.0 (Lock Screen widgets, ActivityKit,
// SwiftUI NavigationStack, Passkeys).
#define D_ENV_IOS_VER_16                160000

// 1.1.7
// D_ENV_IOS_VER_17
//   constant: iOS 17.0 (StandBy, interactive widgets, SwiftData,
// TipKit, Observation framework).
#define D_ENV_IOS_VER_17                170000

// 1.1.8
// D_ENV_IOS_VER_18
//   constant: iOS 18.0 (home screen customization, RCS, Apple
// Intelligence).
#define D_ENV_IOS_VER_18                180000

// 1.1.9
// D_ENV_IOS_VER_19
//   constant: iOS 19.0 (development / future).
#define D_ENV_IOS_VER_19                190000

// 1.2    Detected iOS deployment target and SDK
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_IOS_DEPLOY_TARGET / D_ENV_IOS_SDK_VERSION
//   constant: __IPHONE_OS_VERSION_MIN_REQUIRED and _MAX_ALLOWED, with
// D_ENV_IOS_DEPLOY_DETECTED and D_ENV_IOS_SDK_DETECTED set to 1 when each is
// known; 0 off iOS.
#if D_ENV_APPLE_IS_IOS
    #ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
        #define D_ENV_IOS_DEPLOY_TARGET                                        \
            __IPHONE_OS_VERSION_MIN_REQUIRED
        #define D_ENV_IOS_DEPLOY_DETECTED   1
    #else
        #define D_ENV_IOS_DEPLOY_TARGET     0
        #define D_ENV_IOS_DEPLOY_DETECTED   0
    #endif  // __IPHONE_OS_VERSION_MIN_REQUIRED

    #ifdef __IPHONE_OS_VERSION_MAX_ALLOWED
        #define D_ENV_IOS_SDK_VERSION                                          \
            __IPHONE_OS_VERSION_MAX_ALLOWED
        #define D_ENV_IOS_SDK_DETECTED      1
    #else
        #define D_ENV_IOS_SDK_VERSION       0
        #define D_ENV_IOS_SDK_DETECTED      0
    #endif  // __IPHONE_OS_VERSION_MAX_ALLOWED
#else
    #define D_ENV_IOS_DEPLOY_TARGET         0
    #define D_ENV_IOS_DEPLOY_DETECTED       0
    #define D_ENV_IOS_SDK_VERSION           0
    #define D_ENV_IOS_SDK_DETECTED          0
#endif

// 1.2.2
// D_ENV_IOS_AT_LEAST
//   macro: evaluates to 1 if the iOS deployment target is at least
// the specified version.
#define D_ENV_IOS_AT_LEAST(version)                                            \
    ( D_ENV_IOS_DEPLOY_DETECTED &&                                             \
      (D_ENV_IOS_DEPLOY_TARGET >= (version)) )

// 1.2.3
// D_ENV_IOS_SDK_AT_LEAST
//   macro: evaluates to 1 if the iOS SDK version is at least the
// specified version.
#define D_ENV_IOS_SDK_AT_LEAST(version)                                        \
    ( D_ENV_IOS_SDK_DETECTED &&                                                \
      (D_ENV_IOS_SDK_VERSION >= (version)) )

// 1.2.4
// D_ENV_IOS_DEPLOY_NAME
//   macro: human-readable deployment target name.
#if D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_19)
    #define D_ENV_IOS_DEPLOY_NAME       "iOS 19+"
#elif D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_18)
    #define D_ENV_IOS_DEPLOY_NAME       "iOS 18"
#elif D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_17)
    #define D_ENV_IOS_DEPLOY_NAME       "iOS 17"
#elif D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_16)
    #define D_ENV_IOS_DEPLOY_NAME       "iOS 16"
#elif D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_15)
    #define D_ENV_IOS_DEPLOY_NAME       "iOS 15"
#elif D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_14)
    #define D_ENV_IOS_DEPLOY_NAME       "iOS 14"
#elif D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_13)
    #define D_ENV_IOS_DEPLOY_NAME       "iOS 13"
#elif D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_12)
    #define D_ENV_IOS_DEPLOY_NAME       "iOS 12"
#elif D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_11)
    #define D_ENV_IOS_DEPLOY_NAME       "iOS 11"
#elif D_ENV_APPLE_IS_IOS
    #define D_ENV_IOS_DEPLOY_NAME       "iOS (Legacy)"
#else
    #define D_ENV_IOS_DEPLOY_NAME       "N/A"
#endif


//==============================================================================
// 2.  TVOS, WATCHOS, AND VISIONOS TARGETS
//==============================================================================


// 2.1    tvOS version constants
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_TVOS_VER_12
//   constant: tvOS 12.0.
#define D_ENV_TVOS_VER_12               120000

// 2.1.2
// D_ENV_TVOS_VER_13
//   constant: tvOS 13.0 (multi-user, SwiftUI).
#define D_ENV_TVOS_VER_13               130000

// 2.1.3
// D_ENV_TVOS_VER_14
//   constant: tvOS 14.0.
#define D_ENV_TVOS_VER_14               140000

// 2.1.4
// D_ENV_TVOS_VER_15
//   constant: tvOS 15.0 (SharePlay).
#define D_ENV_TVOS_VER_15               150000

// 2.1.5
// D_ENV_TVOS_VER_16
//   constant: tvOS 16.0 (HDR10+ support).
#define D_ENV_TVOS_VER_16               160000

// 2.1.6
// D_ENV_TVOS_VER_17
//   constant: tvOS 17.0 (FaceTime on Apple TV).
#define D_ENV_TVOS_VER_17               170000

// 2.1.7
// D_ENV_TVOS_VER_18
//   constant: tvOS 18.0.
#define D_ENV_TVOS_VER_18               180000

// 2.2    Detected tvOS deployment target and SDK
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_TVOS_DEPLOY_TARGET / D_ENV_TVOS_SDK_VERSION
//   constant: __TV_OS_VERSION_MIN_REQUIRED and _MAX_ALLOWED, with
// D_ENV_TVOS_DEPLOY_DETECTED and D_ENV_TVOS_SDK_DETECTED set to 1 when each is
// known; 0 off tvOS.
#if D_ENV_APPLE_IS_TVOS
    #ifdef __TV_OS_VERSION_MIN_REQUIRED
        #define D_ENV_TVOS_DEPLOY_TARGET                                       \
            __TV_OS_VERSION_MIN_REQUIRED
        #define D_ENV_TVOS_DEPLOY_DETECTED  1
    #else
        #define D_ENV_TVOS_DEPLOY_TARGET    0
        #define D_ENV_TVOS_DEPLOY_DETECTED  0
    #endif  // __TV_OS_VERSION_MIN_REQUIRED

    #ifdef __TV_OS_VERSION_MAX_ALLOWED
        #define D_ENV_TVOS_SDK_VERSION                                         \
            __TV_OS_VERSION_MAX_ALLOWED
        #define D_ENV_TVOS_SDK_DETECTED     1
    #else
        #define D_ENV_TVOS_SDK_VERSION      0
        #define D_ENV_TVOS_SDK_DETECTED     0
    #endif  // __TV_OS_VERSION_MAX_ALLOWED
#else
    #define D_ENV_TVOS_DEPLOY_TARGET        0
    #define D_ENV_TVOS_DEPLOY_DETECTED      0
    #define D_ENV_TVOS_SDK_VERSION          0
    #define D_ENV_TVOS_SDK_DETECTED         0
#endif

// 2.2.2
// D_ENV_TVOS_AT_LEAST
//   macro: evaluates to 1 if the tvOS deployment target is at least
// the specified version.
#define D_ENV_TVOS_AT_LEAST(version)                                           \
    ( D_ENV_TVOS_DEPLOY_DETECTED &&                                            \
      (D_ENV_TVOS_DEPLOY_TARGET >= (version)) )

// 2.3    watchOS version constants
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_WATCHOS_VER_5
//   constant: watchOS 5.0 (WebKit, background audio).
#define D_ENV_WATCHOS_VER_5             50000

// 2.3.2
// D_ENV_WATCHOS_VER_6
//   constant: watchOS 6.0 (independent apps, SwiftUI, App Store).
#define D_ENV_WATCHOS_VER_6             60000

// 2.3.3
// D_ENV_WATCHOS_VER_7
//   constant: watchOS 7.0 (face sharing, sleep tracking).
#define D_ENV_WATCHOS_VER_7             70000

// 2.3.4
// D_ENV_WATCHOS_VER_8
//   constant: watchOS 8.0 (always-on display API, async/await).
#define D_ENV_WATCHOS_VER_8             80000

// 2.3.5
// D_ENV_WATCHOS_VER_9
//   constant: watchOS 9.0 (CallKit, workout API).
#define D_ENV_WATCHOS_VER_9             90000

// 2.3.6
// D_ENV_WATCHOS_VER_10
//   constant: watchOS 10.0 (WidgetKit, redesigned UI).
#define D_ENV_WATCHOS_VER_10           100000

// 2.3.7
// D_ENV_WATCHOS_VER_11
//   constant: watchOS 11.0 (Smart Stack, training load).
#define D_ENV_WATCHOS_VER_11           110000

// 2.4    Detected watchOS deployment target and SDK
//------------------------------------------------------------------------------
// 2.4.1
// D_ENV_WATCHOS_DEPLOY_TARGET / D_ENV_WATCHOS_SDK_VERSION
//   constant: __WATCH_OS_VERSION_MIN_REQUIRED and _MAX_ALLOWED, with
// D_ENV_WATCHOS_DEPLOY_DETECTED and D_ENV_WATCHOS_SDK_DETECTED set to 1 when
// each is known; 0 off watchOS.
#if D_ENV_APPLE_IS_WATCHOS
    #ifdef __WATCH_OS_VERSION_MIN_REQUIRED
        #define D_ENV_WATCHOS_DEPLOY_TARGET                                    \
            __WATCH_OS_VERSION_MIN_REQUIRED
        #define D_ENV_WATCHOS_DEPLOY_DETECTED 1
    #else
        #define D_ENV_WATCHOS_DEPLOY_TARGET 0
        #define D_ENV_WATCHOS_DEPLOY_DETECTED 0
    #endif  // __WATCH_OS_VERSION_MIN_REQUIRED

    #ifdef __WATCH_OS_VERSION_MAX_ALLOWED
        #define D_ENV_WATCHOS_SDK_VERSION                                      \
            __WATCH_OS_VERSION_MAX_ALLOWED
        #define D_ENV_WATCHOS_SDK_DETECTED  1
    #else
        #define D_ENV_WATCHOS_SDK_VERSION   0
        #define D_ENV_WATCHOS_SDK_DETECTED  0
    #endif  // __WATCH_OS_VERSION_MAX_ALLOWED
#else
    #define D_ENV_WATCHOS_DEPLOY_TARGET     0
    #define D_ENV_WATCHOS_DEPLOY_DETECTED   0
    #define D_ENV_WATCHOS_SDK_VERSION       0
    #define D_ENV_WATCHOS_SDK_DETECTED      0
#endif

// 2.4.2
// D_ENV_WATCHOS_AT_LEAST
//   macro: evaluates to 1 if the watchOS deployment target is at least
// the specified version.
#define D_ENV_WATCHOS_AT_LEAST(version)                                        \
    ( D_ENV_WATCHOS_DEPLOY_DETECTED &&                                         \
      (D_ENV_WATCHOS_DEPLOY_TARGET >= (version)) )

// 2.5    visionOS version constants
//------------------------------------------------------------------------------
// 2.5.1
// D_ENV_VISOS_VER_1
//   constant: visionOS 1.0 (initial release, Apple Vision Pro).
#define D_ENV_VISOS_VER_1               10000

// 2.5.2
// D_ENV_VISOS_VER_2
//   constant: visionOS 2.0.
#define D_ENV_VISOS_VER_2               20000

// 2.6    Detected visionOS deployment target
//------------------------------------------------------------------------------
// 2.6.1
// D_ENV_VISOS_DEPLOY_TARGET / D_ENV_VISOS_DEPLOY_DETECTED
//   constant: __VISION_OS_VERSION_MIN_REQUIRED, with
// D_ENV_VISOS_DEPLOY_DETECTED set to 1 when it is known; 0 off visionOS.
#if D_ENV_APPLE_IS_VISIONOS
    #ifdef __VISION_OS_VERSION_MIN_REQUIRED
        #define D_ENV_VISOS_DEPLOY_TARGET                                      \
            __VISION_OS_VERSION_MIN_REQUIRED
        #define D_ENV_VISOS_DEPLOY_DETECTED 1
    #else
        #define D_ENV_VISOS_DEPLOY_TARGET   0
        #define D_ENV_VISOS_DEPLOY_DETECTED 0
    #endif  // __VISION_OS_VERSION_MIN_REQUIRED
#else
    #define D_ENV_VISOS_DEPLOY_TARGET       0
    #define D_ENV_VISOS_DEPLOY_DETECTED     0
#endif

// 2.6.2
// D_ENV_VISOS_AT_LEAST
//   macro: evaluates to 1 if the visionOS deployment target is at
// least the specified version.
#define D_ENV_VISOS_AT_LEAST(version)                                          \
    ( D_ENV_VISOS_DEPLOY_DETECTED &&                                           \
      (D_ENV_VISOS_DEPLOY_TARGET >= (version)) )


//==============================================================================
// 3.  IOS AND IPADOS FRAMEWORKS
//==============================================================================


// 3.1    Augmented reality
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_IOS_HAS_ARKIT
//   feature: detect if ARKit.framework is available (iOS 11+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_11)
    #define D_ENV_IOS_HAS_ARKIT         1
#elif ( D_ENV_APPLE_IS_IOS &&                                                  \
        !D_ENV_IOS_SDK_DETECTED )
    #define D_ENV_IOS_HAS_ARKIT         1
#else
    #define D_ENV_IOS_HAS_ARKIT         0
#endif

// 3.1.2
// D_ENV_IOS_HAS_ARKIT_5
//   feature: detect if ARKit 5 features are available (iOS 15+,
// location anchors, improved face tracking).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_15)
    #define D_ENV_IOS_HAS_ARKIT_5       1
#else
    #define D_ENV_IOS_HAS_ARKIT_5       0
#endif

// 3.1.3
// D_ENV_IOS_HAS_REALITYKIT
//   feature: detect if RealityKit.framework is available (iOS 13+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_13)
    #define D_ENV_IOS_HAS_REALITYKIT    1
#elif ( D_ENV_APPLE_IS_IOS &&                                                  \
        !D_ENV_IOS_SDK_DETECTED )
    #define D_ENV_IOS_HAS_REALITYKIT    1
#elif D_ENV_APPLE_IS_VISIONOS
    #define D_ENV_IOS_HAS_REALITYKIT    1
#else
    #define D_ENV_IOS_HAS_REALITYKIT    0
#endif

// 3.2    Machine learning
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_IOS_HAS_COREML
//   feature: detect if CoreML.framework is available (iOS 11+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_11)
    #define D_ENV_IOS_HAS_COREML        1
#elif ( D_ENV_APPLE_IS_IOS &&                                                  \
        !D_ENV_IOS_SDK_DETECTED )
    #define D_ENV_IOS_HAS_COREML        1
#elif D_ENV_APPLE_IS_MACOS
    // Core ML is also on macOS 10.13+
    #define D_ENV_IOS_HAS_COREML        1
#else
    #define D_ENV_IOS_HAS_COREML        0
#endif

// 3.2.2
// D_ENV_IOS_HAS_VISION
//   feature: detect if Vision.framework (image analysis, object
// detection) is available (iOS 11+).
#define D_ENV_IOS_HAS_VISION            D_ENV_IOS_HAS_COREML

// 3.2.3
// D_ENV_IOS_HAS_NATURALLANGUAGE
//   feature: detect if NaturalLanguage.framework is available
// (iOS 12+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_12)
    #define D_ENV_IOS_HAS_NATURALLANGUAGE 1
#else
    #define D_ENV_IOS_HAS_NATURALLANGUAGE 0
#endif

// 3.3    Health and motion
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_IOS_HAS_HEALTHKIT
//   feature: detect if HealthKit.framework is available (iOS 8+).
// not available on iPad, tvOS, or macOS.
#if ( D_ENV_APPLE_IS_IOS &&                                                   \
      !D_ENV_APPLE_IS_SIMULATOR )
    #define D_ENV_IOS_HAS_HEALTHKIT     1
#elif D_ENV_APPLE_IS_WATCHOS
    #define D_ENV_IOS_HAS_HEALTHKIT     1
#else
    #define D_ENV_IOS_HAS_HEALTHKIT     0
#endif

// 3.3.2
// D_ENV_IOS_HAS_COREMOTION
//   feature: detect if CoreMotion.framework (accelerometer, gyroscope,
// pedometer) is available.
#if ( D_ENV_APPLE_IS_IOS    ||                                                 \
      D_ENV_APPLE_IS_WATCHOS )
    #define D_ENV_IOS_HAS_COREMOTION    1
#else
    #define D_ENV_IOS_HAS_COREMOTION    0
#endif

// 3.3.3
// D_ENV_IOS_HAS_CORELOCATION
//   feature: detect if CoreLocation.framework is available.
#if ( D_ENV_APPLE_IS_IOS    ||                                                 \
      D_ENV_APPLE_IS_WATCHOS ||                                                \
      D_ENV_APPLE_IS_MACOS  ||                                                 \
      D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_IOS_HAS_CORELOCATION  1
#else
    #define D_ENV_IOS_HAS_CORELOCATION  0
#endif

// 3.4    Notifications and communication
//------------------------------------------------------------------------------
// 3.4.1
// D_ENV_IOS_HAS_USERNOTIFICATIONS
//   feature: detect if UserNotifications.framework is available
// (iOS 10+).
#if ( D_ENV_APPLE_IS_IOS     ||                                                \
      D_ENV_APPLE_IS_WATCHOS  ||                                               \
      D_ENV_APPLE_IS_TVOS     ||                                               \
      D_ENV_APPLE_IS_MACOS )
    #define D_ENV_IOS_HAS_USERNOTIFICATIONS 1
#else
    #define D_ENV_IOS_HAS_USERNOTIFICATIONS 0
#endif

// 3.4.2
// D_ENV_IOS_HAS_CALLKIT
//   feature: detect if CallKit.framework is available (iOS 10+).
#if D_ENV_APPLE_IS_IOS
    #define D_ENV_IOS_HAS_CALLKIT       1
#elif D_ENV_WATCHOS_AT_LEAST(D_ENV_WATCHOS_VER_9)
    #define D_ENV_IOS_HAS_CALLKIT       1
#else
    #define D_ENV_IOS_HAS_CALLKIT       0
#endif

// 3.4.3
// D_ENV_IOS_HAS_MESSAGEUI
//   feature: detect if MessageUI.framework (in-app email/SMS) is
// available.
#if D_ENV_APPLE_IS_IOS
    #define D_ENV_IOS_HAS_MESSAGEUI     1
#else
    #define D_ENV_IOS_HAS_MESSAGEUI     0
#endif

// 3.5    Maps and navigation
//------------------------------------------------------------------------------
// 3.5.1
// D_ENV_IOS_HAS_MAPKIT
//   feature: detect if MapKit.framework is available.
#if ( D_ENV_APPLE_IS_IOS   ||                                                  \
      D_ENV_APPLE_IS_MACOS ||                                                  \
      D_ENV_APPLE_IS_TVOS  ||                                                  \
      D_ENV_APPLE_IS_WATCHOS )
    #define D_ENV_IOS_HAS_MAPKIT        1
#else
    #define D_ENV_IOS_HAS_MAPKIT        0
#endif


//==============================================================================
// 4.  EXTENSIONS AND COMMERCE
//==============================================================================


// 4.1    Widgets, App Clips, and extensions
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_IOS_HAS_WIDGETKIT
//   feature: detect if WidgetKit.framework is available (iOS 14+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_14)
    #define D_ENV_IOS_HAS_WIDGETKIT     1
#elif D_ENV_WATCHOS_AT_LEAST(D_ENV_WATCHOS_VER_10)
    #define D_ENV_IOS_HAS_WIDGETKIT     1
#else
    #define D_ENV_IOS_HAS_WIDGETKIT     0
#endif

// 4.1.2
// D_ENV_IOS_HAS_INTERACTIVE_WIDGETS
//   feature: detect if interactive widgets (Button/Toggle in widgets)
// are available (iOS 17+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_17)
    #define D_ENV_IOS_HAS_INTERACTIVE_WIDGETS 1
#else
    #define D_ENV_IOS_HAS_INTERACTIVE_WIDGETS 0
#endif

// 4.1.3
// D_ENV_IOS_HAS_APPCLIP
//   feature: detect if App Clips are available (iOS 14+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_14)
    #define D_ENV_IOS_HAS_APPCLIP       1
#else
    #define D_ENV_IOS_HAS_APPCLIP       0
#endif

// 4.1.4
// D_ENV_IOS_HAS_ACTIVITYKIT
//   feature: detect if ActivityKit (Live Activities) is available
// (iOS 16.1+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_16)
    #define D_ENV_IOS_HAS_ACTIVITYKIT   1
#else
    #define D_ENV_IOS_HAS_ACTIVITYKIT   0
#endif

// 4.1.5
// D_ENV_IOS_HAS_TIPKIT
//   feature: detect if TipKit.framework is available (iOS 17+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_17)
    #define D_ENV_IOS_HAS_TIPKIT        1
#else
    #define D_ENV_IOS_HAS_TIPKIT        0
#endif

// 4.1.6
// D_ENV_IOS_HAS_APP_INTENTS
//   feature: detect if AppIntents.framework (Siri intents, Shortcuts)
// is available (iOS 16+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_16)
    #define D_ENV_IOS_HAS_APP_INTENTS   1
#else
    #define D_ENV_IOS_HAS_APP_INTENTS   0
#endif

// 4.1.7
// D_ENV_IOS_IS_APP_EXTENSION
//   feature: detect if building an app extension (Today, Share, etc.)
// rather than a main app. extensions cannot use certain APIs.
#if defined(NS_EXTENSION_UNAVAILABLE)
    #define D_ENV_IOS_IS_APP_EXTENSION  1
#elif defined(NS_EXTENSION_UNAVAILABLE_IOS)
    #define D_ENV_IOS_IS_APP_EXTENSION  1
#else
    #define D_ENV_IOS_IS_APP_EXTENSION  0
#endif

// 4.2    StoreKit and in-app purchase
//------------------------------------------------------------------------------
// 4.2.1
// D_ENV_IOS_HAS_STOREKIT
//   feature: detect if StoreKit.framework (IAP) is available.
#if ( D_ENV_APPLE_IS_IOS   ||                                                  \
      D_ENV_APPLE_IS_MACOS ||                                                  \
      D_ENV_APPLE_IS_TVOS  ||                                                  \
      D_ENV_APPLE_IS_WATCHOS ||                                                \
      D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_IOS_HAS_STOREKIT      1
#else
    #define D_ENV_IOS_HAS_STOREKIT      0
#endif

// 4.2.2
// D_ENV_IOS_HAS_STOREKIT2
//   feature: detect if StoreKit 2 (modern async/await API) is
// available (iOS 15+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_15)
    #define D_ENV_IOS_HAS_STOREKIT2     1
#elif D_ENV_MACOS_SDK_AT_LEAST(D_ENV_MACOS_VER_12)
    #define D_ENV_IOS_HAS_STOREKIT2     1
#else
    #define D_ENV_IOS_HAS_STOREKIT2     0
#endif


//==============================================================================
// 5.  DEVICE AND WINDOWING
//==============================================================================


// 5.1    Device capabilities
//------------------------------------------------------------------------------
//   compile-time heuristics only: these report whether the APIs for querying
// a capability exist, while the capability itself needs a runtime check
// through UIDevice, AVCaptureDevice, and the like.
// 5.1.1
// D_ENV_IOS_HAS_CAMERA_API
//   feature: detect if camera capture APIs are available.
// AVFoundation camera APIs are on iOS and macOS; not on watchOS/tvOS.
#if ( D_ENV_APPLE_IS_IOS  ||                                                   \
      D_ENV_APPLE_IS_MACOS )
    #define D_ENV_IOS_HAS_CAMERA_API    1
#else
    #define D_ENV_IOS_HAS_CAMERA_API    0
#endif

// 5.1.2
// D_ENV_IOS_HAS_HAPTICS_API
//   feature: detect if Core Haptics / UIFeedbackGenerator APIs are
// available (iOS 10+ for UIFeedbackGenerator, iOS 13+ for
// CoreHaptics).
#if D_ENV_APPLE_IS_IOS
    #define D_ENV_IOS_HAS_HAPTICS_API   1
#else
    #define D_ENV_IOS_HAS_HAPTICS_API   0
#endif

// 5.1.3
// D_ENV_IOS_HAS_LIDAR_API
//   feature: detect if LiDAR depth APIs (ARKit depth) are available.
// requires iOS 14+ SDK and ARKit 4.
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_14)
    #define D_ENV_IOS_HAS_LIDAR_API     1
#else
    #define D_ENV_IOS_HAS_LIDAR_API     0
#endif

// 5.1.4
// D_ENV_IOS_HAS_NFC_API
//   feature: detect if Core NFC framework is available (iOS 11+).
// not available on iPad, simulator, or tvOS.
#if ( D_ENV_APPLE_IS_IOS      &&                                               \
      !D_ENV_APPLE_IS_SIMULATOR &&                                             \
      D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_11) )
    #define D_ENV_IOS_HAS_NFC_API       1
#elif ( D_ENV_APPLE_IS_IOS &&                                                  \
        !D_ENV_IOS_SDK_DETECTED )
    #define D_ENV_IOS_HAS_NFC_API       1
#else
    #define D_ENV_IOS_HAS_NFC_API       0
#endif

// 5.1.5
// D_ENV_IOS_HAS_FACEID_API
//   feature: detect if Face ID / biometric authentication APIs
// (LocalAuthentication) are available (iOS 11+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_11)
    #define D_ENV_IOS_HAS_FACEID_API    1
#elif ( D_ENV_APPLE_IS_IOS &&                                                  \
        !D_ENV_IOS_SDK_DETECTED )
    #define D_ENV_IOS_HAS_FACEID_API    1
#else
    #define D_ENV_IOS_HAS_FACEID_API    0
#endif

// 5.1.6
// D_ENV_IOS_HAS_BLUETOOTH_API
//   feature: detect if CoreBluetooth.framework is available.
#if ( D_ENV_APPLE_IS_IOS    ||                                                 \
      D_ENV_APPLE_IS_MACOS   ||                                                \
      D_ENV_APPLE_IS_WATCHOS ||                                                \
      D_ENV_APPLE_IS_TVOS )
    #define D_ENV_IOS_HAS_BLUETOOTH_API 1
#else
    #define D_ENV_IOS_HAS_BLUETOOTH_API 0
#endif

// 5.1.7
// D_ENV_IOS_HAS_UWB_API
//   feature: detect if Nearby Interaction (UWB) framework is available
// (iOS 14+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_14)
    #define D_ENV_IOS_HAS_UWB_API       1
#else
    #define D_ENV_IOS_HAS_UWB_API       0
#endif

// 5.2    Multitasking and windowing
//------------------------------------------------------------------------------
// 5.2.1
// D_ENV_IOS_HAS_MULTITASKING
//   feature: detect if iPad multitasking (Split View, Slide Over) is
// available. requires iOS 13+ on iPad.
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_13)
    #define D_ENV_IOS_HAS_MULTITASKING  1
#else
    #define D_ENV_IOS_HAS_MULTITASKING  0
#endif

// 5.2.2
// D_ENV_IOS_HAS_SCENES
//   feature: detect if UIScene (multi-window) API is available
// (iOS 13+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_13)
    #define D_ENV_IOS_HAS_SCENES        1
#else
    #define D_ENV_IOS_HAS_SCENES        0
#endif

// 5.2.3
// D_ENV_IOS_HAS_STAGE_MANAGER
//   feature: detect if Stage Manager API support is available
// (iPadOS 16+).
#if D_ENV_IOS_SDK_AT_LEAST(D_ENV_IOS_VER_16)
    #define D_ENV_IOS_HAS_STAGE_MANAGER 1
#else
    #define D_ENV_IOS_HAS_STAGE_MANAGER 0
#endif

// 5.2.4
// D_ENV_IOS_HAS_EXTERNAL_DISPLAY
//   feature: detect if external display / screen mirroring APIs are
// available. UIScreen is available on all iOS versions; UIScene-based
// multi-display requires iOS 13+.
#if D_ENV_APPLE_IS_IOS
    #define D_ENV_IOS_HAS_EXTERNAL_DISPLAY 1
#elif D_ENV_APPLE_IS_TVOS
    #define D_ENV_IOS_HAS_EXTERNAL_DISPLAY 1
#else
    #define D_ENV_IOS_HAS_EXTERNAL_DISPLAY 0
#endif


//==============================================================================
// 6.  VISIONOS AND DISTRIBUTION
//==============================================================================


// 6.1    visionOS-specific features
//------------------------------------------------------------------------------
// 6.1.1
// D_ENV_VISOS_HAS_REALITYKIT
//   feature: detect if RealityKit (spatial computing) is available.
// RealityKit is the core 3D framework on visionOS.
#if D_ENV_APPLE_IS_VISIONOS
    #define D_ENV_VISOS_HAS_REALITYKIT  1
#else
    #define D_ENV_VISOS_HAS_REALITYKIT  0
#endif

// 6.1.2
// D_ENV_VISOS_HAS_ARKIT_SPATIAL
//   feature: detect if spatial ARKit features (hand tracking, scene
// understanding) are available (visionOS 1+).
#if D_ENV_APPLE_IS_VISIONOS
    #define D_ENV_VISOS_HAS_ARKIT_SPATIAL 1
#else
    #define D_ENV_VISOS_HAS_ARKIT_SPATIAL 0
#endif

// 6.1.3
// D_ENV_VISOS_HAS_COMPOSITOR
//   feature: detect if CompositorServices (custom Metal rendering in
// shared space) is available (visionOS 1+).
#if D_ENV_APPLE_IS_VISIONOS
    #define D_ENV_VISOS_HAS_COMPOSITOR  1
#else
    #define D_ENV_VISOS_HAS_COMPOSITOR  0
#endif

// 6.1.4
// D_ENV_VISOS_HAS_VOLUMES
//   feature: detect if volumetric window style is available
// (visionOS 1+).
#if D_ENV_APPLE_IS_VISIONOS
    #define D_ENV_VISOS_HAS_VOLUMES     1
#else
    #define D_ENV_VISOS_HAS_VOLUMES     0
#endif

// 6.1.5
// D_ENV_VISOS_HAS_IMMERSIVE_SPACE
//   feature: detect if immersive space (full immersion) is available
// (visionOS 1+).
#if D_ENV_APPLE_IS_VISIONOS
    #define D_ENV_VISOS_HAS_IMMERSIVE_SPACE 1
#else
    #define D_ENV_VISOS_HAS_IMMERSIVE_SPACE 0
#endif

// 6.2    App Store and distribution
//------------------------------------------------------------------------------
// 6.2.1
// D_ENV_MOBILE_IS_APP_STORE_BUILD
//   feature: detect if building for App Store distribution.
// NDEBUG + no simulator + not ad-hoc is a heuristic.
#if ( defined(NDEBUG)            &&                                            \
      !D_ENV_APPLE_IS_SIMULATOR  &&                                            \
      D_ENV_APPLE_IS_DEVICE )
    #define D_ENV_MOBILE_IS_APP_STORE_BUILD 1
#else
    #define D_ENV_MOBILE_IS_APP_STORE_BUILD 0
#endif

// 6.2.2
// D_ENV_MOBILE_HAS_TESTFLIGHT
//   feature: detect if TestFlight APIs (StoreKit receipt checking) are
// available. TestFlight is available on iOS 8+.
#if ( D_ENV_APPLE_IS_IOS   ||                                                  \
      D_ENV_APPLE_IS_TVOS  ||                                                  \
      D_ENV_APPLE_IS_WATCHOS ||                                                \
      D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_MOBILE_HAS_TESTFLIGHT 1
#else
    #define D_ENV_MOBILE_HAS_TESTFLIGHT 0
#endif

// 6.2.3
// D_ENV_MOBILE_HAS_ENTERPRISE_DIST
//   feature: detect if enterprise distribution (ad-hoc / in-house) is
// possible. always possible on iOS; not on tvOS or watchOS.
#if D_ENV_APPLE_IS_IOS
    #define D_ENV_MOBILE_HAS_ENTERPRISE_DIST 1
#else
    #define D_ENV_MOBILE_HAS_ENTERPRISE_DIST 0
#endif

// 6.2.4
// D_ENV_MOBILE_NO_JIT
//   feature: detect if JIT compilation is prohibited.
// App Store apps on iOS cannot use JIT (W^X policy) unless the app
// uses a JIT entitlement (which is not generally available).
#if ( D_ENV_APPLE_IS_IOS    ||                                                 \
      D_ENV_APPLE_IS_TVOS   ||                                                 \
      D_ENV_APPLE_IS_WATCHOS ||                                                \
      D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_MOBILE_NO_JIT         1
#else
    #define D_ENV_MOBILE_NO_JIT         0
#endif

// 6.2.5
// D_ENV_MOBILE_NO_DLOPEN
//   feature: detect if dlopen of third-party dynamic libraries is
// prohibited. App Store policy forbids loading non-system dylibs.
#if ( D_ENV_APPLE_IS_IOS    ||                                                 \
      D_ENV_APPLE_IS_TVOS   ||                                                 \
      D_ENV_APPLE_IS_WATCHOS ||                                                \
      D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_MOBILE_NO_DLOPEN      1
#else
    #define D_ENV_MOBILE_NO_DLOPEN      0
#endif

// 6.2.6
// D_ENV_MOBILE_NO_FORK
//   feature: detect if fork() is prohibited.
// iOS does not allow fork/exec; this is enforced by the sandbox.
#if ( D_ENV_APPLE_IS_IOS    ||                                                 \
      D_ENV_APPLE_IS_TVOS   ||                                                 \
      D_ENV_APPLE_IS_WATCHOS ||                                                \
      D_ENV_APPLE_IS_VISIONOS )
    #define D_ENV_MOBILE_NO_FORK        1
#else
    #define D_ENV_MOBILE_NO_FORK        0
#endif


//==============================================================================
// 7.  RUNTIME DETECTION AND CONVENIENCE
//==============================================================================


// 7.1    Runtime queries
//------------------------------------------------------------------------------
//   C linkage for C++ callers. The env headers sit below djinterp.h, so the
// D_EXTERN_C_BEGIN / D_EXTERN_C_END pair is not available here.
#if D_ENV_LANG_USING_CPP
    extern "C" {
#endif

/**
 * @brief Returns the running iOS, tvOS, watchOS, or visionOS version string.
 *
 * @return the version (e.g. "17.4.1"), or "Unknown".
 */
const char* d_env_ios_get_os_version(void);
/**
 * @brief Returns the device's model identifier at runtime (e.g.
 *        "iPhone16,1", "iPad14,1", "Watch7,2").
 *
 * @return sysctl hw.machine's string, or "Unknown".
 */
const char* d_env_ios_get_device_model(void);
/**
 * @brief Tests at runtime whether the device is an iPad.
 *
 * @return `1` on an iPad, `0` otherwise.
 */
int         d_env_ios_is_ipad(void);
/**
 * @brief Tests at runtime whether the device has a notch or Dynamic Island,
 *        judged from its safe-area insets.
 *
 * @return `1` if it has either, `0` otherwise.
 */
int         d_env_ios_has_notch_or_island(void);
/**
 * @brief Tests at runtime whether the device supports ARKit, via
 *        ARWorldTrackingConfiguration.isSupported.
 *
 * @return `1` if AR is supported, `0` otherwise.
 */
int         d_env_ios_supports_ar(void);
/**
 * @brief Prints detailed information about the detected iOS environment: OS
 *        version, device model, SDK, and available frameworks.
 */
void        d_env_ios_print_info(void);

#if D_ENV_LANG_USING_CPP
    }
#endif

// 7.2    Combined predicates
//------------------------------------------------------------------------------
// 7.2.1
// D_ENV_IOS_IS_MODERN
//   macro: evaluates to 1 if the iOS deployment target is modern
// (iOS 15+, with async/await, StoreKit 2, etc.).
#define D_ENV_IOS_IS_MODERN()                                                  \
    ( D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_15) )

// 7.2.2
// D_ENV_IOS_HAS_SWIFTUI_FULL
//   macro: evaluates to 1 if a mature SwiftUI is available (iOS 15+
// with NavigationStack-era improvements in iOS 16+).
#define D_ENV_IOS_HAS_SWIFTUI_FULL()                                           \
    ( D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_16) )

// 7.2.3
// D_ENV_IOS_HAS_MODERN_CONCURRENCY
//   macro: evaluates to 1 if Swift concurrency (async/await, actors)
// is available in system frameworks (iOS 15+).
#define D_ENV_IOS_HAS_MODERN_CONCURRENCY()                                     \
    ( D_ENV_IOS_AT_LEAST(D_ENV_IOS_VER_15)   ||                                \
      D_ENV_TVOS_AT_LEAST(D_ENV_TVOS_VER_15) ||                                \
      D_ENV_WATCHOS_AT_LEAST(D_ENV_WATCHOS_VER_8) )

// 7.2.4
// D_ENV_MOBILE_IS_SANDBOXED
//   macro: evaluates to 1 if the platform enforces mandatory app
// sandboxing (all mobile Apple platforms).
#define D_ENV_MOBILE_IS_SANDBOXED()                                            \
    ( D_ENV_APPLE_IS_IOS     ||                                                \
      D_ENV_APPLE_IS_TVOS    ||                                                \
      D_ENV_APPLE_IS_WATCHOS ||                                                \
      D_ENV_APPLE_IS_VISIONOS )

// 7.2.5
// D_ENV_MOBILE_IS_CONSTRAINED
//   macro: evaluates to 1 if the platform has significant runtime
// constraints (no JIT, no fork, no dlopen).
#define D_ENV_MOBILE_IS_CONSTRAINED()                                          \
    ( D_ENV_MOBILE_NO_JIT    &&                                                \
      D_ENV_MOBILE_NO_FORK   &&                                                \
      D_ENV_MOBILE_NO_DLOPEN )

// 7.2.6
// D_ENV_IOS_HAS_SPATIAL_COMPUTING
//   macro: evaluates to 1 if spatial computing features (RealityKit,
// hand tracking, volumes, immersive spaces) are available.
#define D_ENV_IOS_HAS_SPATIAL_COMPUTING()                                      \
    ( D_ENV_APPLE_IS_VISIONOS )


#endif  // DJINTERP_ENV_OS_ENV_IOS_H
