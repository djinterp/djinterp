/*******************************************************************************
* djinterp [env]                                                        env_qt.h
*
* djinterp Qt environment detection.
*   Compile-time detection of the Qt framework across its major versions, Qt 1
* through Qt 6. It detects:
*     - Qt presence and exact version (major, minor, patch)
*     - the build configuration (static or shared, debug or release, namespace)
*     - module availability (Widgets, QML, Network, SQL, and more)
*     - platform integration (XCB, Wayland, Win32, Cocoa, EGLFS, and the
*       Qt 5 platform extras)
*     - OpenGL / rendering and core feature flags (accessibility, D-Bus,
*       concurrency, SSL, regular expressions)
*     - the C++ standard each Qt version requires (Qt 6 requires C++17)
*   Version detection reads QT_VERSION and QT_VERSION_STR, and module
* availability the build's QT_<MODULE>_LIB macros. From Qt 6.5, a C++ unit
* needs nothing first: this header includes <QtCore/qtversionchecks.h>
* itself (2.1.1; D_CFG_ENV_QT_PROBE_VERSION turns that off). Older Qt, and C,
* need a Qt header included before this one; without it, Qt reads as not
* detected:
*     #include <QtGlobal>     // or <QtCore/QtGlobal>
*     #include "./env_qt.h"
*   Naming: D_ENV_QT_<CATEGORY>_<FEATURE> is 1 if available, 0 otherwise.
*
*
* path:      /inc/djinterp/env/ui/env_qt.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.28
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VERSION CONSTANTS
    -----------------
    1.  Major versions
         1.  D_ENV_QT_VERSION_1
         2.  D_ENV_QT_VERSION_2
         3.  D_ENV_QT_VERSION_3
         4.  D_ENV_QT_VERSION_4
         5.  D_ENV_QT_VERSION_5
         6.  D_ENV_QT_VERSION_6
    2.  Qt 4 minor versions
         1.  D_ENV_QT_VERSION_4_6
         2.  D_ENV_QT_VERSION_4_7
         3.  D_ENV_QT_VERSION_4_8
    3.  Qt 5 minor versions
         1.  D_ENV_QT_VERSION_5_0
         2.  D_ENV_QT_VERSION_5_1
         3.  D_ENV_QT_VERSION_5_2
         4.  D_ENV_QT_VERSION_5_3
         5.  D_ENV_QT_VERSION_5_4
         6.  D_ENV_QT_VERSION_5_5
         7.  D_ENV_QT_VERSION_5_6
         8.  D_ENV_QT_VERSION_5_7
         9.  D_ENV_QT_VERSION_5_9
         10. D_ENV_QT_VERSION_5_10
         11. D_ENV_QT_VERSION_5_12
         12. D_ENV_QT_VERSION_5_15
    4.  Qt 6 minor versions
         1.  D_ENV_QT_VERSION_6_0
         2.  D_ENV_QT_VERSION_6_1
         3.  D_ENV_QT_VERSION_6_2
         4.  D_ENV_QT_VERSION_6_3
         5.  D_ENV_QT_VERSION_6_4
         6.  D_ENV_QT_VERSION_6_5
         7.  D_ENV_QT_VERSION_6_6
         8.  D_ENV_QT_VERSION_6_7
         9.  D_ENV_QT_VERSION_6_8
2.  DETECTION AND BUILD CONFIGURATION
    ---------------------------------
    1.  Presence and version
         1.  <QtCore/qtversionchecks.h>
         2.  D_ENV_QT_VER / D_ENV_QT_AVAILABLE
    2.  Series classification
         1.  D_ENV_QT_IS_QT6 / D_ENV_QT_SERIES_NAME
    3.  Static or shared linkage
         1.  D_ENV_QT_STATIC / D_ENV_QT_SHARED / D_ENV_QT_LINKAGE
    4.  Debug or release
         1.  D_ENV_QT_DEBUG / D_ENV_QT_RELEASE / D_ENV_QT_BUILD_MODE
    5.  Namespace
         1.  D_ENV_QT_NAMESPACED
3.  MODULES
    -------
    1.  Essential modules
         1.  D_ENV_QT_HAS_CORE
         2.  D_ENV_QT_HAS_GUI
         3.  D_ENV_QT_HAS_WIDGETS
         4.  D_ENV_QT_HAS_NETWORK
         5.  D_ENV_QT_HAS_SQL
         6.  D_ENV_QT_HAS_TEST
         7.  D_ENV_QT_HAS_CONCURRENT
         8.  D_ENV_QT_HAS_DBUS
    2.  QML and Quick
         1.  D_ENV_QT_HAS_QML
         2.  D_ENV_QT_HAS_QUICK
         3.  D_ENV_QT_HAS_QUICKCONTROLS2
         4.  D_ENV_QT_HAS_QUICK3D
         5.  D_ENV_QT_HAS_DECLARATIVE
    3.  Media and graphics
         1.  D_ENV_QT_HAS_MULTIMEDIA
         2.  D_ENV_QT_HAS_OPENGL
         3.  D_ENV_QT_HAS_SVG
         4.  D_ENV_QT_HAS_PRINTSUPPORT
    4.  Data and serialization
         1.  D_ENV_QT_HAS_XML
         2.  D_ENV_QT_HAS_XMLPATTERNS
         3.  D_ENV_QT_HAS_JSON
         4.  D_ENV_QT_HAS_CBOR
    5.  Web and connectivity
         1.  D_ENV_QT_HAS_WEBENGINE
         2.  D_ENV_QT_HAS_WEBKIT
         3.  D_ENV_QT_HAS_WEBSOCKETS
         4.  D_ENV_QT_HAS_WEBCHANNEL
         5.  D_ENV_QT_HAS_BLUETOOTH
         6.  D_ENV_QT_HAS_NFC
         7.  D_ENV_QT_HAS_SERIALPORT
         8.  D_ENV_QT_HAS_SERIALBUS
         9.  D_ENV_QT_HAS_MQTT
         10. D_ENV_QT_HAS_HTTPSERVER
         11. D_ENV_QT_HAS_GRPC
         12. D_ENV_QT_HAS_PROTOBUF
    6.  3D, charts, and visualization
         1.  D_ENV_QT_HAS_3D
         2.  D_ENV_QT_HAS_CHARTS
         3.  D_ENV_QT_HAS_DATAVISUALIZATION
         4.  D_ENV_QT_HAS_GRAPHS
         5.  D_ENV_QT_HAS_SCXML
         6.  D_ENV_QT_HAS_STATEMACHINE
    7.  Positioning, sensors, and input
         1.  D_ENV_QT_HAS_POSITIONING
         2.  D_ENV_QT_HAS_SENSORS
         3.  D_ENV_QT_HAS_GAMEPAD
4.  PLATFORM INTEGRATION
    --------------------
    1.  Platform backends
         1.  D_ENV_QT_PLATFORM_XCB
         2.  D_ENV_QT_PLATFORM_WAYLAND
         3.  D_ENV_QT_PLATFORM_WIN32
         4.  D_ENV_QT_PLATFORM_COCOA
         5.  D_ENV_QT_PLATFORM_IOS
         6.  D_ENV_QT_PLATFORM_ANDROID
         7.  D_ENV_QT_PLATFORM_WASM
         8.  D_ENV_QT_PLATFORM_EGLFS
         9.  D_ENV_QT_PLATFORM_INTEGRITY
         10. D_ENV_QT_PLATFORM_QNX
         11. D_ENV_QT_PLATFORM_VXWORKS
    2.  Platform extras (Qt 5 only; removed in Qt 6)
         1.  D_ENV_QT_HAS_X11EXTRAS
         2.  D_ENV_QT_HAS_WINEXTRAS
         3.  D_ENV_QT_HAS_MACEXTRAS
5.  OPENGL AND RENDERING
    --------------------
    1.  OpenGL and rendering
         1.  D_ENV_QT_OPENGL_ES
         2.  D_ENV_QT_OPENGL_DESKTOP
         3.  D_ENV_QT_OPENGL_DYNAMIC
         4.  D_ENV_QT_HAS_VULKAN
         5.  D_ENV_QT_HAS_RHI
6.  FEATURE FLAGS
    -------------
    1.  Accessibility
         1.  D_ENV_QT_HAS_ACCESSIBILITY
    2.  Internationalization
         1.  D_ENV_QT_HAS_TRANSLATION
         2.  D_ENV_QT_HAS_ICU
    3.  Threading and concurrency
         1.  D_ENV_QT_HAS_THREAD
         2.  D_ENV_QT_HAS_FUTURE
    4.  File system and I/O
         1.  D_ENV_QT_HAS_FILESYSTEMWATCHER
         2.  D_ENV_QT_HAS_PROCESS
         3.  D_ENV_QT_HAS_SHAREDMEMORY
    5.  SSL and cryptography
         1.  D_ENV_QT_HAS_SSL
         2.  D_ENV_QT_HAS_OPENSSL / D_ENV_QT_OPENSSL_LINKED
    6.  Regular expressions
         1.  D_ENV_QT_HAS_REGEXP
         2.  D_ENV_QT_HAS_REGULAREXPRESSION
7.  C++ STANDARD AND DEPRECATION
    ----------------------------
    1.  C++ standard interplay
         1.  D_ENV_QT_CPP_MINIMUM_MET
         2.  D_ENV_QT_HAS_CPP17_API
         3.  D_ENV_QT_HAS_CPP20_API
    2.  Deprecation and migration
         1.  D_ENV_QT_DEPRECATION_CUTOFF
         2.  D_ENV_QT_NO_DEPRECATED_WARNINGS
         3.  D_ENV_QT_HAS_QT5_COMPAT
8.  RUNTIME DETECTION
    -----------------
    1.  Runtime queries
9.  CONVENIENCE MACROS
    ------------------
    1.  Combined predicates
         1.  D_ENV_HAS_QT
         2.  D_ENV_QT_AT_LEAST
         3.  D_ENV_QT_AT_LEAST_HEX
         4.  D_ENV_QT_VERSION_CHECK
         5.  D_ENV_QT_IS_SERIES
         6.  D_ENV_QT_IS_LTS
         7.  D_ENV_QT_HAS_MODERN_CONNECT
         8.  D_ENV_QT_HAS_QPROPERTY
*/

#ifndef DJINTERP_ENV_UI_ENV_QT_H
#define DJINTERP_ENV_UI_ENV_QT_H 1

// djinterp
#include "../env.h"  // D_ENV_LANG_CPP_STANDARD, D_ENV_LANG_USING_CPP
#include "../../config/core/env/ui/cfg_env_qt.h"  // D_CFG_ENV_QT_PROBE_VERSION


//==============================================================================
// 1.  VERSION CONSTANTS
//==============================================================================


// 1.1    Major versions
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_QT_VERSION_1
//   constant: QT_VERSION value for Qt 1.x (earliest public release).
#define D_ENV_QT_VERSION_1              0x010000

// 1.1.2
// D_ENV_QT_VERSION_2
//   constant: QT_VERSION value for Qt 2.0.
#define D_ENV_QT_VERSION_2              0x020000

// 1.1.3
// D_ENV_QT_VERSION_3
//   constant: QT_VERSION value for Qt 3.0.
#define D_ENV_QT_VERSION_3              0x030000

// 1.1.4
// D_ENV_QT_VERSION_4
//   constant: QT_VERSION value for Qt 4.0.0.
#define D_ENV_QT_VERSION_4              0x040000

// 1.1.5
// D_ENV_QT_VERSION_5
//   constant: QT_VERSION value for Qt 5.0.0.
#define D_ENV_QT_VERSION_5              0x050000

// 1.1.6
// D_ENV_QT_VERSION_6
//   constant: QT_VERSION value for Qt 6.0.0.
#define D_ENV_QT_VERSION_6              0x060000

// 1.2    Qt 4 minor versions
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_QT_VERSION_4_6
//   constant: QT_VERSION for Qt 4.6.0 (animation framework, state machines).
#define D_ENV_QT_VERSION_4_6            0x040600

// 1.2.2
// D_ENV_QT_VERSION_4_7
//   constant: QT_VERSION for Qt 4.7.0 (QML/QtDeclarative introduced).
#define D_ENV_QT_VERSION_4_7            0x040700

// 1.2.3
// D_ENV_QT_VERSION_4_8
//   constant: QT_VERSION for Qt 4.8.0 (last Qt 4 feature release).
#define D_ENV_QT_VERSION_4_8            0x040800

// 1.3    Qt 5 minor versions
//------------------------------------------------------------------------------
// 1.3.1
// D_ENV_QT_VERSION_5_0
//   constant: QT_VERSION for Qt 5.0.0 (initial Qt 5 release).
#define D_ENV_QT_VERSION_5_0            0x050000

// 1.3.2
// D_ENV_QT_VERSION_5_1
//   constant: QT_VERSION for Qt 5.1.0 (Qt Sensors, Qt Serial Port).
#define D_ENV_QT_VERSION_5_1            0x050100

// 1.3.3
// D_ENV_QT_VERSION_5_2
//   constant: QT_VERSION for Qt 5.2.0 (Android/iOS fully supported).
#define D_ENV_QT_VERSION_5_2            0x050200

// 1.3.4
// D_ENV_QT_VERSION_5_3
//   constant: QT_VERSION for Qt 5.3.0 (Qt WebEngine preview).
#define D_ENV_QT_VERSION_5_3            0x050300

// 1.3.5
// D_ENV_QT_VERSION_5_4
//   constant: QT_VERSION for Qt 5.4.0 (Qt WebChannel, Qt WebEngine).
#define D_ENV_QT_VERSION_5_4            0x050400

// 1.3.6
// D_ENV_QT_VERSION_5_5
//   constant: QT_VERSION for Qt 5.5.0 (Qt 3D, Qt Canvas3D).
#define D_ENV_QT_VERSION_5_5            0x050500

// 1.3.7
// D_ENV_QT_VERSION_5_6
//   constant: QT_VERSION for Qt 5.6.0 (first LTS release).
#define D_ENV_QT_VERSION_5_6            0x050600

// 1.3.8
// D_ENV_QT_VERSION_5_7
//   constant: QT_VERSION for Qt 5.7.0 (C++11 required, Qt Gamepad).
#define D_ENV_QT_VERSION_5_7            0x050700

// 1.3.9
// D_ENV_QT_VERSION_5_9
//   constant: QT_VERSION for Qt 5.9.0 (LTS, Qt Lite / configure revamp).
#define D_ENV_QT_VERSION_5_9            0x050900

// 1.3.10
// D_ENV_QT_VERSION_5_10
//   constant: QT_VERSION for Qt 5.10.0 (Qt Virtual Keyboard, Qt Quick).
#define D_ENV_QT_VERSION_5_10           0x050A00

// 1.3.11
// D_ENV_QT_VERSION_5_12
//   constant: QT_VERSION for Qt 5.12.0 (LTS, Qt for Python, C++17 prep).
#define D_ENV_QT_VERSION_5_12           0x050C00

// 1.3.12
// D_ENV_QT_VERSION_5_15
//   constant: QT_VERSION for Qt 5.15.0 (LTS, final Qt 5 release, Qt 6
// migration bridge).
#define D_ENV_QT_VERSION_5_15           0x050F00

// 1.4    Qt 6 minor versions
//------------------------------------------------------------------------------
// 1.4.1
// D_ENV_QT_VERSION_6_0
//   constant: QT_VERSION for Qt 6.0.0 (C++17 required, CMake build system).
#define D_ENV_QT_VERSION_6_0            0x060000

// 1.4.2
// D_ENV_QT_VERSION_6_1
//   constant: QT_VERSION for Qt 6.1.0 (Qt Charts, Qt Data Visualization
// ported).
#define D_ENV_QT_VERSION_6_1            0x060100

// 1.4.3
// D_ENV_QT_VERSION_6_2
//   constant: QT_VERSION for Qt 6.2.0 (LTS, Qt Multimedia rewritten,
// Qt Connectivity restored).
#define D_ENV_QT_VERSION_6_2            0x060200

// 1.4.4
// D_ENV_QT_VERSION_6_3
//   constant: QT_VERSION for Qt 6.3.0 (Qt Language Server Protocol).
#define D_ENV_QT_VERSION_6_3            0x060300

// 1.4.5
// D_ENV_QT_VERSION_6_4
//   constant: QT_VERSION for Qt 6.4.0 (Qt HTTP Server, Qt Quick 3D
// Physics).
#define D_ENV_QT_VERSION_6_4            0x060400

// 1.4.6
// D_ENV_QT_VERSION_6_5
//   constant: QT_VERSION for Qt 6.5.0 (LTS, Qt Graphs introduced).
#define D_ENV_QT_VERSION_6_5            0x060500

// 1.4.7
// D_ENV_QT_VERSION_6_6
//   constant: QT_VERSION for Qt 6.6.0 (Qt Graphs 3D, Qt GRPC).
#define D_ENV_QT_VERSION_6_6            0x060600

// 1.4.8
// D_ENV_QT_VERSION_6_7
//   constant: QT_VERSION for Qt 6.7.0 (latest feature release).
#define D_ENV_QT_VERSION_6_7            0x060700

// 1.4.9
// D_ENV_QT_VERSION_6_8
//   constant: QT_VERSION for Qt 6.8.0 (LTS).
#define D_ENV_QT_VERSION_6_8            0x060800


//==============================================================================
// 2.  DETECTION AND BUILD CONFIGURATION
//==============================================================================


// 2.1    Presence and version
//------------------------------------------------------------------------------
// 2.1.1
// <QtCore/qtversionchecks.h>
//   include: Qt 6.5 and later define QT_VERSION in this small header, which
// brings in only Qt's configuration macros; earlier Qt has only the heavy
// qglobal.h, and Qt's headers are C++. So a C++ unit that has no QT_VERSION
// yet includes it here where it exists, unless D_CFG_ENV_QT_PROBE_VERSION is
// 0, and Qt's version no longer depends on include order there (decision 47
// of the register). Older Qt, and C, still need a Qt header first.
#if ( (D_CFG_IS_ON(D_CFG_ENV_QT_PROBE_VERSION)) &&                             \
      (D_ENV_LANG_USING_CPP)                    &&                             \
      (!defined(QT_VERSION))                    &&                             \
      (defined(__has_include)) )
    #if __has_include(<QtCore/qtversionchecks.h>)
        // qt
        #include <QtCore/qtversionchecks.h>  // QT_VERSION, from Qt 6.5
    #endif
#endif

// 2.1.2
// D_ENV_QT_VER / D_ENV_QT_AVAILABLE
//   feature: QT_VERSION's raw hex value, which <QtGlobal> and
// <QtCore/qglobal.h> define, or 0 without it, and D_ENV_QT_AVAILABLE, 1 when
// it is nonzero; the version is decomposed below. A Qt header must be included
// first; without one, Qt reads as absent. Every result in this header is
// guarded, so a cross-build or a test can pin it, and the grouped ones derive
// from the one they depend on: pinning D_ENV_QT_VER moves the series flags
// and the version parts with it (decision 48 of the register).
#ifndef D_ENV_QT_VER
    #if defined(QT_VERSION)
        #define D_ENV_QT_VER            QT_VERSION
    #else
        #define D_ENV_QT_VER            0
    #endif
#endif  // D_ENV_QT_VER

#ifndef D_ENV_QT_AVAILABLE
    #if (D_ENV_QT_VER > 0)
        #define D_ENV_QT_AVAILABLE      1
    #else
        #define D_ENV_QT_AVAILABLE      0
    #endif
#endif  // D_ENV_QT_AVAILABLE

// D_ENV_QT_VER_MAJOR
//   constant: major version extracted from D_ENV_QT_VER.
#ifndef D_ENV_QT_VER_MAJOR
    #define D_ENV_QT_VER_MAJOR          ((D_ENV_QT_VER >> 16) & 0xFF)
#endif  // D_ENV_QT_VER_MAJOR

// D_ENV_QT_VER_MINOR
//   constant: minor version extracted from D_ENV_QT_VER.
#ifndef D_ENV_QT_VER_MINOR
    #define D_ENV_QT_VER_MINOR          ((D_ENV_QT_VER >> 8) & 0xFF)
#endif  // D_ENV_QT_VER_MINOR

// D_ENV_QT_VER_PATCH
//   constant: patch version extracted from D_ENV_QT_VER.
#ifndef D_ENV_QT_VER_PATCH
    #define D_ENV_QT_VER_PATCH          (D_ENV_QT_VER & 0xFF)
#endif  // D_ENV_QT_VER_PATCH

// D_ENV_QT_VER_STR
//   constant: QT_VERSION_STR where Qt defines it, "Unknown" where Qt is there
// without it, "None" without Qt.
#ifndef D_ENV_QT_VER_STR
    #if defined(QT_VERSION_STR)
        #define D_ENV_QT_VER_STR        QT_VERSION_STR
    #elif D_ENV_QT_AVAILABLE
        #define D_ENV_QT_VER_STR        "Unknown"
    #else
        #define D_ENV_QT_VER_STR        "None"
    #endif
#endif  // D_ENV_QT_VER_STR
// 2.2    Series classification
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_QT_IS_QT6 / D_ENV_QT_SERIES_NAME
//   feature: which Qt series is detected -- D_ENV_QT_IS_QT6, _QT5, _QT4,
// _QT3, or _LEGACY for Qt 1 and 2 -- with D_ENV_QT_SERIES_NAME naming it.
#ifndef D_ENV_QT_IS_QT6
    #if (D_ENV_QT_VER >= D_ENV_QT_VERSION_6)
        #define D_ENV_QT_IS_QT6         1
    #else
        #define D_ENV_QT_IS_QT6         0
    #endif
#endif  // D_ENV_QT_IS_QT6

#ifndef D_ENV_QT_IS_QT5
    #if ( (D_ENV_QT_VER >= D_ENV_QT_VERSION_5) &&                              \
          (D_ENV_QT_VER <  D_ENV_QT_VERSION_6) )
        #define D_ENV_QT_IS_QT5         1
    #else
        #define D_ENV_QT_IS_QT5         0
    #endif
#endif  // D_ENV_QT_IS_QT5

#ifndef D_ENV_QT_IS_QT4
    #if ( (D_ENV_QT_VER >= D_ENV_QT_VERSION_4) &&                              \
          (D_ENV_QT_VER <  D_ENV_QT_VERSION_5) )
        #define D_ENV_QT_IS_QT4         1
    #else
        #define D_ENV_QT_IS_QT4         0
    #endif
#endif  // D_ENV_QT_IS_QT4

#ifndef D_ENV_QT_IS_QT3
    #if ( (D_ENV_QT_VER >= D_ENV_QT_VERSION_3) &&                              \
          (D_ENV_QT_VER <  D_ENV_QT_VERSION_4) )
        #define D_ENV_QT_IS_QT3         1
    #else
        #define D_ENV_QT_IS_QT3         0
    #endif
#endif  // D_ENV_QT_IS_QT3

#ifndef D_ENV_QT_IS_LEGACY
    #if ( (D_ENV_QT_AVAILABLE) &&                                              \
          (D_ENV_QT_VER < D_ENV_QT_VERSION_3) )
        #define D_ENV_QT_IS_LEGACY      1
    #else
        #define D_ENV_QT_IS_LEGACY      0
    #endif
#endif  // D_ENV_QT_IS_LEGACY

#ifndef D_ENV_QT_SERIES_NAME
    #if D_ENV_QT_IS_QT6
        #define D_ENV_QT_SERIES_NAME    "Qt 6"
    #elif D_ENV_QT_IS_QT5
        #define D_ENV_QT_SERIES_NAME    "Qt 5"
    #elif D_ENV_QT_IS_QT4
        #define D_ENV_QT_SERIES_NAME    "Qt 4"
    #elif D_ENV_QT_IS_QT3
        #define D_ENV_QT_SERIES_NAME    "Qt 3"
    #elif D_ENV_QT_IS_LEGACY
        #define D_ENV_QT_SERIES_NAME    "Qt (Legacy)"
    #else
        #define D_ENV_QT_SERIES_NAME    "None"
    #endif
#endif  // D_ENV_QT_SERIES_NAME
// 2.3    Static or shared linkage
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_QT_STATIC / D_ENV_QT_SHARED / D_ENV_QT_LINKAGE
//   feature: detect if Qt was built as a static library, with
// D_ENV_QT_LINKAGE naming the linkage.
// QT_STATIC is defined by Qt's build system for static builds.
#ifndef D_ENV_QT_STATIC
    #if defined(QT_STATIC)
        #define D_ENV_QT_STATIC         1
    #else
        #define D_ENV_QT_STATIC         0
    #endif
#endif  // D_ENV_QT_STATIC

// default assumption: shared (most common distribution)
#ifndef D_ENV_QT_SHARED
    #if (!D_ENV_QT_STATIC)
        #define D_ENV_QT_SHARED         1
    #else
        #define D_ENV_QT_SHARED         0
    #endif
#endif  // D_ENV_QT_SHARED

#ifndef D_ENV_QT_LINKAGE
    #if D_ENV_QT_STATIC
        #define D_ENV_QT_LINKAGE        "Static"
    #elif defined(QT_SHARED)
        #define D_ENV_QT_LINKAGE        "Shared"
    #else
        #define D_ENV_QT_LINKAGE        "Unknown (assuming Shared)"
    #endif
#endif  // D_ENV_QT_LINKAGE// 2.4    Debug or release
//------------------------------------------------------------------------------
// 2.4.1
// D_ENV_QT_DEBUG / D_ENV_QT_RELEASE / D_ENV_QT_BUILD_MODE
//   feature: the application's Qt build mode. qglobal.h defines QT_DEBUG
// unless the build defines QT_NO_DEBUG, as release builds do, so this describes
// how the application is being built, not how Qt itself was built.
#ifndef D_ENV_QT_DEBUG
    #if defined(QT_DEBUG)
        #define D_ENV_QT_DEBUG          1
    #else
        #define D_ENV_QT_DEBUG          0
    #endif
#endif  // D_ENV_QT_DEBUG

#ifndef D_ENV_QT_RELEASE
    #if ( (!D_ENV_QT_DEBUG) &&                                                 \
          (defined(QT_NO_DEBUG)) )
        #define D_ENV_QT_RELEASE        1
    #else
        #define D_ENV_QT_RELEASE        0
    #endif
#endif  // D_ENV_QT_RELEASE

#ifndef D_ENV_QT_BUILD_MODE
    #if D_ENV_QT_DEBUG
        #define D_ENV_QT_BUILD_MODE     "Debug"
    #elif D_ENV_QT_RELEASE
        #define D_ENV_QT_BUILD_MODE     "Release"
    #else
        #define D_ENV_QT_BUILD_MODE     "Unknown"
    #endif
#endif  // D_ENV_QT_BUILD_MODE// 2.5    Namespace
//------------------------------------------------------------------------------
// 2.5.1
// D_ENV_QT_NAMESPACED
//   feature: detect if Qt was built with a custom namespace
// (QT_NAMESPACE / QT_BEGIN_NAMESPACE).
#ifndef D_ENV_QT_NAMESPACED
    #if defined(QT_NAMESPACE)
        #define D_ENV_QT_NAMESPACED         1
    #else
        #define D_ENV_QT_NAMESPACED         0
    #endif
#endif  // D_ENV_QT_NAMESPACED


//==============================================================================
// 3.  MODULES
//==============================================================================
// Module detection reads the QT_<MODULE>_LIB macros, which the build system
// defines for each module the target links (qmake's QT += ..., or CMake's
// Qt6:: targets). A flag therefore means "this build links the module".


// 3.1    Essential modules
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_QT_HAS_CORE
//   feature: detect if QtCore module is available.
// QtCore is always present when Qt is detected.
#ifndef D_ENV_QT_HAS_CORE
    #if D_ENV_QT_AVAILABLE
        #define D_ENV_QT_HAS_CORE          1
    #else
        #define D_ENV_QT_HAS_CORE          0
    #endif
#endif  // D_ENV_QT_HAS_CORE

// 3.1.2
// D_ENV_QT_HAS_GUI
//   feature: detect if QtGui module is available: QT_GUI_LIB, which qmake and
// CMake's Qt targets define, Qt 4's qmake included, like every other module
// here. It used to read 1 for any Qt build without Qt 4's QT_NO_GUI, so a
// console build linking only QtCore reported a GUI (decision 49 of the
// register).
#ifndef D_ENV_QT_HAS_GUI
    #if defined(QT_GUI_LIB)
        #define D_ENV_QT_HAS_GUI           1
    #else
        #define D_ENV_QT_HAS_GUI           0
    #endif
#endif  // D_ENV_QT_HAS_GUI

// 3.1.3
// D_ENV_QT_HAS_WIDGETS
//   feature: detect if QtWidgets module is available (Qt 5+).
// in Qt 4, widgets lived inside QtGui.
#ifndef D_ENV_QT_HAS_WIDGETS
    #if defined(QT_WIDGETS_LIB)
        #define D_ENV_QT_HAS_WIDGETS       1
    #elif ( (D_ENV_QT_IS_QT4) &&                                               \
            (D_ENV_QT_HAS_GUI) )
        // Qt 4 widgets are part of QtGui
        #define D_ENV_QT_HAS_WIDGETS       1
    #else
        #define D_ENV_QT_HAS_WIDGETS       0
    #endif
#endif  // D_ENV_QT_HAS_WIDGETS

// 3.1.4
// D_ENV_QT_HAS_NETWORK
//   feature: detect if QtNetwork module is available.
#ifndef D_ENV_QT_HAS_NETWORK
    #if defined(QT_NETWORK_LIB)
        #define D_ENV_QT_HAS_NETWORK       1
    #else
        #define D_ENV_QT_HAS_NETWORK       0
    #endif
#endif  // D_ENV_QT_HAS_NETWORK

// 3.1.5
// D_ENV_QT_HAS_SQL
//   feature: detect if QtSql module is available.
#ifndef D_ENV_QT_HAS_SQL
    #if defined(QT_SQL_LIB)
        #define D_ENV_QT_HAS_SQL           1
    #else
        #define D_ENV_QT_HAS_SQL           0
    #endif
#endif  // D_ENV_QT_HAS_SQL

// 3.1.6
// D_ENV_QT_HAS_TEST
//   feature: detect if QtTest module is available.
#ifndef D_ENV_QT_HAS_TEST
    #if defined(QT_TEST_LIB)
        #define D_ENV_QT_HAS_TEST          1
    #else
        #define D_ENV_QT_HAS_TEST          0
    #endif
#endif  // D_ENV_QT_HAS_TEST

// 3.1.7
// D_ENV_QT_HAS_CONCURRENT
//   feature: detect if QtConcurrent module is available.
#ifndef D_ENV_QT_HAS_CONCURRENT
    #if defined(QT_CONCURRENT_LIB)
        #define D_ENV_QT_HAS_CONCURRENT    1
    #else
        #define D_ENV_QT_HAS_CONCURRENT    0
    #endif
#endif  // D_ENV_QT_HAS_CONCURRENT

// 3.1.8
// D_ENV_QT_HAS_DBUS
//   feature: detect if QtDBus module is available.
#ifndef D_ENV_QT_HAS_DBUS
    #if defined(QT_DBUS_LIB)
        #define D_ENV_QT_HAS_DBUS          1
    #else
        #define D_ENV_QT_HAS_DBUS          0
    #endif
#endif  // D_ENV_QT_HAS_DBUS

// 3.2    QML and Quick
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_QT_HAS_QML
//   feature: detect if QtQml module is available (Qt 5+).
#ifndef D_ENV_QT_HAS_QML
    #if defined(QT_QML_LIB)
        #define D_ENV_QT_HAS_QML           1
    #else
        #define D_ENV_QT_HAS_QML           0
    #endif
#endif  // D_ENV_QT_HAS_QML

// 3.2.2
// D_ENV_QT_HAS_QUICK
//   feature: detect if QtQuick module is available (Qt 5+).
#ifndef D_ENV_QT_HAS_QUICK
    #if defined(QT_QUICK_LIB)
        #define D_ENV_QT_HAS_QUICK         1
    #else
        #define D_ENV_QT_HAS_QUICK         0
    #endif
#endif  // D_ENV_QT_HAS_QUICK

// 3.2.3
// D_ENV_QT_HAS_QUICKCONTROLS2
//   feature: detect if Qt Quick Controls 2 module is available (Qt 5.7+).
// in Qt 6, this merged into QtQuick.
#ifndef D_ENV_QT_HAS_QUICKCONTROLS2
    #if defined(QT_QUICKCONTROLS2_LIB)
        #define D_ENV_QT_HAS_QUICKCONTROLS2 1
    #elif ( (D_ENV_QT_IS_QT6) &&                                               \
            (D_ENV_QT_HAS_QUICK) )
        // merged into QtQuick in Qt 6
        #define D_ENV_QT_HAS_QUICKCONTROLS2 1
    #else
        #define D_ENV_QT_HAS_QUICKCONTROLS2 0
    #endif
#endif  // D_ENV_QT_HAS_QUICKCONTROLS2

// 3.2.4
// D_ENV_QT_HAS_QUICK3D
//   feature: detect if Qt Quick 3D module is available (Qt 5.15+, Qt 6+).
#ifndef D_ENV_QT_HAS_QUICK3D
    #if defined(QT_QUICK3D_LIB)
        #define D_ENV_QT_HAS_QUICK3D       1
    #else
        #define D_ENV_QT_HAS_QUICK3D       0
    #endif
#endif  // D_ENV_QT_HAS_QUICK3D

// 3.2.5
// D_ENV_QT_HAS_DECLARATIVE
//   feature: detect if QtDeclarative (Qt Quick 1) is available (Qt 4.7+).
// deprecated in Qt 5, removed in Qt 6.
#ifndef D_ENV_QT_HAS_DECLARATIVE
    #if defined(QT_DECLARATIVE_LIB)
        #define D_ENV_QT_HAS_DECLARATIVE   1
    #else
        #define D_ENV_QT_HAS_DECLARATIVE   0
    #endif
#endif  // D_ENV_QT_HAS_DECLARATIVE

// 3.3    Media and graphics
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_QT_HAS_MULTIMEDIA
//   feature: detect if QtMultimedia module is available.
#ifndef D_ENV_QT_HAS_MULTIMEDIA
    #if defined(QT_MULTIMEDIA_LIB)
        #define D_ENV_QT_HAS_MULTIMEDIA    1
    #else
        #define D_ENV_QT_HAS_MULTIMEDIA    0
    #endif
#endif  // D_ENV_QT_HAS_MULTIMEDIA

// 3.3.2
// D_ENV_QT_HAS_OPENGL
//   feature: detect if QtOpenGL module is available.
#ifndef D_ENV_QT_HAS_OPENGL
    #if defined(QT_OPENGL_LIB)
        #define D_ENV_QT_HAS_OPENGL        1
    #elif ( (D_ENV_QT_AVAILABLE) &&                                            \
            (!defined(QT_NO_OPENGL)) )
        #define D_ENV_QT_HAS_OPENGL        1
    #else
        #define D_ENV_QT_HAS_OPENGL        0
    #endif
#endif  // D_ENV_QT_HAS_OPENGL

// 3.3.3
// D_ENV_QT_HAS_SVG
//   feature: detect if QtSvg module is available.
#ifndef D_ENV_QT_HAS_SVG
    #if defined(QT_SVG_LIB)
        #define D_ENV_QT_HAS_SVG           1
    #else
        #define D_ENV_QT_HAS_SVG           0
    #endif
#endif  // D_ENV_QT_HAS_SVG

// 3.3.4
// D_ENV_QT_HAS_PRINTSUPPORT
//   feature: detect if QtPrintSupport module is available (Qt 5+).
#ifndef D_ENV_QT_HAS_PRINTSUPPORT
    #if defined(QT_PRINTSUPPORT_LIB)
        #define D_ENV_QT_HAS_PRINTSUPPORT  1
    #else
        #define D_ENV_QT_HAS_PRINTSUPPORT  0
    #endif
#endif  // D_ENV_QT_HAS_PRINTSUPPORT

// 3.4    Data and serialization
//------------------------------------------------------------------------------
// 3.4.1
// D_ENV_QT_HAS_XML
//   feature: detect if QtXml module is available.
#ifndef D_ENV_QT_HAS_XML
    #if defined(QT_XML_LIB)
        #define D_ENV_QT_HAS_XML           1
    #else
        #define D_ENV_QT_HAS_XML           0
    #endif
#endif  // D_ENV_QT_HAS_XML

// 3.4.2
// D_ENV_QT_HAS_XMLPATTERNS
//   feature: detect if QtXmlPatterns module is available (Qt 4/5 only,
// removed in Qt 6).
#ifndef D_ENV_QT_HAS_XMLPATTERNS
    #if defined(QT_XMLPATTERNS_LIB)
        #define D_ENV_QT_HAS_XMLPATTERNS   1
    #else
        #define D_ENV_QT_HAS_XMLPATTERNS   0
    #endif
#endif  // D_ENV_QT_HAS_XMLPATTERNS

// 3.4.3
// D_ENV_QT_HAS_JSON
//   feature: detect if JSON support is available.
// JSON was added in Qt 5.0 as part of QtCore. always available in Qt 5+.
#ifndef D_ENV_QT_HAS_JSON
    #if ( (D_ENV_QT_IS_QT5) ||                                                 \
          (D_ENV_QT_IS_QT6) )
        #define D_ENV_QT_HAS_JSON          1
    #else
        #define D_ENV_QT_HAS_JSON          0
    #endif
#endif  // D_ENV_QT_HAS_JSON

// 3.4.4
// D_ENV_QT_HAS_CBOR
//   feature: detect if CBOR support is available (Qt 5.12+).
#ifndef D_ENV_QT_HAS_CBOR
    #if ( (D_ENV_QT_AVAILABLE) &&                                              \
          (D_ENV_QT_VER >= D_ENV_QT_VERSION_5_12) )
        #define D_ENV_QT_HAS_CBOR          1
    #else
        #define D_ENV_QT_HAS_CBOR          0
    #endif
#endif  // D_ENV_QT_HAS_CBOR

// 3.5    Web and connectivity
//------------------------------------------------------------------------------
// 3.5.1
// D_ENV_QT_HAS_WEBENGINE
//   feature: detect if Qt WebEngine module is available (Qt 5.4+).
#ifndef D_ENV_QT_HAS_WEBENGINE
    #if ( (defined(QT_WEBENGINE_LIB))     ||                                   \
          (defined(QT_WEBENGINECORE_LIB)) ||                                   \
          (defined(QT_WEBENGINEWIDGETS_LIB)) )
        #define D_ENV_QT_HAS_WEBENGINE     1
    #else
        #define D_ENV_QT_HAS_WEBENGINE     0
    #endif
#endif  // D_ENV_QT_HAS_WEBENGINE

// 3.5.2
// D_ENV_QT_HAS_WEBKIT
//   feature: detect if QtWebKit is available (deprecated in Qt 5,
// removed in Qt 6).
#ifndef D_ENV_QT_HAS_WEBKIT
    #if defined(QT_WEBKIT_LIB)
        #define D_ENV_QT_HAS_WEBKIT        1
    #else
        #define D_ENV_QT_HAS_WEBKIT        0
    #endif
#endif  // D_ENV_QT_HAS_WEBKIT

// 3.5.3
// D_ENV_QT_HAS_WEBSOCKETS
//   feature: detect if QtWebSockets module is available (Qt 5.3+).
#ifndef D_ENV_QT_HAS_WEBSOCKETS
    #if defined(QT_WEBSOCKETS_LIB)
        #define D_ENV_QT_HAS_WEBSOCKETS    1
    #else
        #define D_ENV_QT_HAS_WEBSOCKETS    0
    #endif
#endif  // D_ENV_QT_HAS_WEBSOCKETS

// 3.5.4
// D_ENV_QT_HAS_WEBCHANNEL
//   feature: detect if QtWebChannel module is available (Qt 5.4+).
#ifndef D_ENV_QT_HAS_WEBCHANNEL
    #if defined(QT_WEBCHANNEL_LIB)
        #define D_ENV_QT_HAS_WEBCHANNEL    1
    #else
        #define D_ENV_QT_HAS_WEBCHANNEL    0
    #endif
#endif  // D_ENV_QT_HAS_WEBCHANNEL

// 3.5.5
// D_ENV_QT_HAS_BLUETOOTH
//   feature: detect if QtBluetooth module is available.
#ifndef D_ENV_QT_HAS_BLUETOOTH
    #if defined(QT_BLUETOOTH_LIB)
        #define D_ENV_QT_HAS_BLUETOOTH     1
    #else
        #define D_ENV_QT_HAS_BLUETOOTH     0
    #endif
#endif  // D_ENV_QT_HAS_BLUETOOTH

// 3.5.6
// D_ENV_QT_HAS_NFC
//   feature: detect if QtNfc module is available.
#ifndef D_ENV_QT_HAS_NFC
    #if defined(QT_NFC_LIB)
        #define D_ENV_QT_HAS_NFC           1
    #else
        #define D_ENV_QT_HAS_NFC           0
    #endif
#endif  // D_ENV_QT_HAS_NFC

// 3.5.7
// D_ENV_QT_HAS_SERIALPORT
//   feature: detect if QtSerialPort module is available (Qt 5.1+).
#ifndef D_ENV_QT_HAS_SERIALPORT
    #if defined(QT_SERIALPORT_LIB)
        #define D_ENV_QT_HAS_SERIALPORT    1
    #else
        #define D_ENV_QT_HAS_SERIALPORT    0
    #endif
#endif  // D_ENV_QT_HAS_SERIALPORT

// 3.5.8
// D_ENV_QT_HAS_SERIALBUS
//   feature: detect if QtSerialBus module is available (Qt 5.6+).
#ifndef D_ENV_QT_HAS_SERIALBUS
    #if defined(QT_SERIALBUS_LIB)
        #define D_ENV_QT_HAS_SERIALBUS     1
    #else
        #define D_ENV_QT_HAS_SERIALBUS     0
    #endif
#endif  // D_ENV_QT_HAS_SERIALBUS

// 3.5.9
// D_ENV_QT_HAS_MQTT
//   feature: detect if QtMqtt module is available (Qt 5.10+).
#ifndef D_ENV_QT_HAS_MQTT
    #if defined(QT_MQTT_LIB)
        #define D_ENV_QT_HAS_MQTT          1
    #else
        #define D_ENV_QT_HAS_MQTT          0
    #endif
#endif  // D_ENV_QT_HAS_MQTT

// 3.5.10
// D_ENV_QT_HAS_HTTPSERVER
//   feature: detect if Qt HTTP Server module is available (Qt 6.4+).
#ifndef D_ENV_QT_HAS_HTTPSERVER
    #if defined(QT_HTTPSERVER_LIB)
        #define D_ENV_QT_HAS_HTTPSERVER    1
    #else
        #define D_ENV_QT_HAS_HTTPSERVER    0
    #endif
#endif  // D_ENV_QT_HAS_HTTPSERVER

// 3.5.11
// D_ENV_QT_HAS_GRPC
//   feature: detect if Qt GRPC module is available (Qt 6.5+).
#ifndef D_ENV_QT_HAS_GRPC
    #if defined(QT_GRPC_LIB)
        #define D_ENV_QT_HAS_GRPC          1
    #else
        #define D_ENV_QT_HAS_GRPC          0
    #endif
#endif  // D_ENV_QT_HAS_GRPC

// 3.5.12
// D_ENV_QT_HAS_PROTOBUF
//   feature: detect if Qt Protobuf module is available (Qt 6.5+).
#ifndef D_ENV_QT_HAS_PROTOBUF
    #if defined(QT_PROTOBUF_LIB)
        #define D_ENV_QT_HAS_PROTOBUF      1
    #else
        #define D_ENV_QT_HAS_PROTOBUF      0
    #endif
#endif  // D_ENV_QT_HAS_PROTOBUF

// 3.6    3D, charts, and visualization
//------------------------------------------------------------------------------
// 3.6.1
// D_ENV_QT_HAS_3D
//   feature: detect if Qt 3D module is available (Qt 5.5+).
#ifndef D_ENV_QT_HAS_3D
    #if ( (defined(QT_3DCORE_LIB))   ||                                        \
          (defined(QT_3DRENDER_LIB)) ||                                        \
          (defined(QT_3DINPUT_LIB)) )
        #define D_ENV_QT_HAS_3D            1
    #else
        #define D_ENV_QT_HAS_3D            0
    #endif
#endif  // D_ENV_QT_HAS_3D

// 3.6.2
// D_ENV_QT_HAS_CHARTS
//   feature: detect if QtCharts module is available (Qt 5.7+ / Qt 6.1+).
#ifndef D_ENV_QT_HAS_CHARTS
    #if defined(QT_CHARTS_LIB)
        #define D_ENV_QT_HAS_CHARTS        1
    #else
        #define D_ENV_QT_HAS_CHARTS        0
    #endif
#endif  // D_ENV_QT_HAS_CHARTS

// 3.6.3
// D_ENV_QT_HAS_DATAVISUALIZATION
//   feature: detect if Qt Data Visualization module is available.
#ifndef D_ENV_QT_HAS_DATAVISUALIZATION
    #if defined(QT_DATAVISUALIZATION_LIB)
        #define D_ENV_QT_HAS_DATAVISUALIZATION 1
    #else
        #define D_ENV_QT_HAS_DATAVISUALIZATION 0
    #endif
#endif  // D_ENV_QT_HAS_DATAVISUALIZATION

// 3.6.4
// D_ENV_QT_HAS_GRAPHS
//   feature: detect if Qt Graphs module is available (Qt 6.5+,
// replacement for Charts + Data Visualization).
#ifndef D_ENV_QT_HAS_GRAPHS
    #if defined(QT_GRAPHS_LIB)
        #define D_ENV_QT_HAS_GRAPHS        1
    #else
        #define D_ENV_QT_HAS_GRAPHS        0
    #endif
#endif  // D_ENV_QT_HAS_GRAPHS

// 3.6.5
// D_ENV_QT_HAS_SCXML
//   feature: detect if QtScxml module is available (Qt 5.7+).
#ifndef D_ENV_QT_HAS_SCXML
    #if defined(QT_SCXML_LIB)
        #define D_ENV_QT_HAS_SCXML         1
    #else
        #define D_ENV_QT_HAS_SCXML         0
    #endif
#endif  // D_ENV_QT_HAS_SCXML

// 3.6.6
// D_ENV_QT_HAS_STATEMACHINE
//   feature: detect if QtStateMachine module is available.
// in Qt 5, state machine was part of QtCore. in Qt 6, separate module.
#ifndef D_ENV_QT_HAS_STATEMACHINE
    #if defined(QT_STATEMACHINE_LIB)
        #define D_ENV_QT_HAS_STATEMACHINE  1
    #elif D_ENV_QT_IS_QT5
        // included in QtCore for Qt 5
        #define D_ENV_QT_HAS_STATEMACHINE  1
    #else
        #define D_ENV_QT_HAS_STATEMACHINE  0
    #endif
#endif  // D_ENV_QT_HAS_STATEMACHINE

// 3.7    Positioning, sensors, and input
//------------------------------------------------------------------------------
// 3.7.1
// D_ENV_QT_HAS_POSITIONING
//   feature: detect if QtPositioning module is available (Qt 5.2+).
#ifndef D_ENV_QT_HAS_POSITIONING
    #if defined(QT_POSITIONING_LIB)
        #define D_ENV_QT_HAS_POSITIONING   1
    #else
        #define D_ENV_QT_HAS_POSITIONING   0
    #endif
#endif  // D_ENV_QT_HAS_POSITIONING

// 3.7.2
// D_ENV_QT_HAS_SENSORS
//   feature: detect if QtSensors module is available (Qt 5.1+).
#ifndef D_ENV_QT_HAS_SENSORS
    #if defined(QT_SENSORS_LIB)
        #define D_ENV_QT_HAS_SENSORS       1
    #else
        #define D_ENV_QT_HAS_SENSORS       0
    #endif
#endif  // D_ENV_QT_HAS_SENSORS

// 3.7.3
// D_ENV_QT_HAS_GAMEPAD
//   feature: detect if QtGamepad module is available (Qt 5.7+, removed
// in Qt 6).
#ifndef D_ENV_QT_HAS_GAMEPAD
    #if defined(QT_GAMEPAD_LIB)
        #define D_ENV_QT_HAS_GAMEPAD       1
    #else
        #define D_ENV_QT_HAS_GAMEPAD       0
    #endif
#endif  // D_ENV_QT_HAS_GAMEPAD


//==============================================================================
// 4.  PLATFORM INTEGRATION
//==============================================================================


// 4.1    Platform backends
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_QT_PLATFORM_XCB
//   feature: detect if the XCB (X11) platform plugin is targeted.
#ifndef D_ENV_QT_PLATFORM_XCB
    #if defined(Q_OS_LINUX)
        #if ( (!defined(QT_NO_XCB)) &&                                         \
              (!defined(D_ENV_QT_FORCE_WAYLAND)) )
            #define D_ENV_QT_PLATFORM_XCB   1
        #else
            #define D_ENV_QT_PLATFORM_XCB   0
        #endif
    #else
        #define D_ENV_QT_PLATFORM_XCB       0
    #endif
#endif  // D_ENV_QT_PLATFORM_XCB

// 4.1.2
// D_ENV_QT_PLATFORM_WAYLAND
//   feature: detect if Wayland platform plugin support is available.
#ifndef D_ENV_QT_PLATFORM_WAYLAND
    #if defined(QT_WAYLAND_LIB)
        #define D_ENV_QT_PLATFORM_WAYLAND   1
    #elif ( (defined(Q_OS_LINUX)) &&                                           \
            (D_ENV_QT_AVAILABLE)  &&                                           \
            (D_ENV_QT_VER >= D_ENV_QT_VERSION_5_4) )
        // Wayland support available since Qt 5.4 on Linux
        #define D_ENV_QT_PLATFORM_WAYLAND   1
    #else
        #define D_ENV_QT_PLATFORM_WAYLAND   0
    #endif
#endif  // D_ENV_QT_PLATFORM_WAYLAND

// 4.1.3
// D_ENV_QT_PLATFORM_WIN32
//   feature: detect if the Windows platform plugin is targeted.
#ifndef D_ENV_QT_PLATFORM_WIN32
    #if defined(Q_OS_WIN)
        #define D_ENV_QT_PLATFORM_WIN32     1
    #else
        #define D_ENV_QT_PLATFORM_WIN32     0
    #endif
#endif  // D_ENV_QT_PLATFORM_WIN32

// 4.1.4
// D_ENV_QT_PLATFORM_COCOA
//   feature: detect if the Cocoa (macOS) platform plugin is targeted.
#ifndef D_ENV_QT_PLATFORM_COCOA
    #if defined(Q_OS_MACOS)
        #define D_ENV_QT_PLATFORM_COCOA     1
    #elif defined(Q_OS_MAC)
        // older macro used in Qt 4
        #define D_ENV_QT_PLATFORM_COCOA     1
    #else
        #define D_ENV_QT_PLATFORM_COCOA     0
    #endif
#endif  // D_ENV_QT_PLATFORM_COCOA

// 4.1.5
// D_ENV_QT_PLATFORM_IOS
//   feature: detect if the iOS platform plugin is targeted.
#ifndef D_ENV_QT_PLATFORM_IOS
    #if defined(Q_OS_IOS)
        #define D_ENV_QT_PLATFORM_IOS       1
    #else
        #define D_ENV_QT_PLATFORM_IOS       0
    #endif
#endif  // D_ENV_QT_PLATFORM_IOS

// 4.1.6
// D_ENV_QT_PLATFORM_ANDROID
//   feature: detect if the Android platform plugin is targeted.
#ifndef D_ENV_QT_PLATFORM_ANDROID
    #if defined(Q_OS_ANDROID)
        #define D_ENV_QT_PLATFORM_ANDROID   1
    #else
        #define D_ENV_QT_PLATFORM_ANDROID   0
    #endif
#endif  // D_ENV_QT_PLATFORM_ANDROID

// 4.1.7
// D_ENV_QT_PLATFORM_WASM
//   feature: detect if the WebAssembly platform is targeted (Qt 5.13+).
#ifndef D_ENV_QT_PLATFORM_WASM
    #if defined(Q_OS_WASM)
        #define D_ENV_QT_PLATFORM_WASM      1
    #elif defined(__EMSCRIPTEN__)
        #define D_ENV_QT_PLATFORM_WASM      1
    #else
        #define D_ENV_QT_PLATFORM_WASM      0
    #endif
#endif  // D_ENV_QT_PLATFORM_WASM

// 4.1.8
// D_ENV_QT_PLATFORM_EGLFS
//   feature: detect if the EGLFS platform plugin is targeted
// (embedded Linux without X11/Wayland).
#ifndef D_ENV_QT_PLATFORM_EGLFS
    #if defined(QT_EGLFS_LIB)
        #define D_ENV_QT_PLATFORM_EGLFS     1
    #else
        #define D_ENV_QT_PLATFORM_EGLFS     0
    #endif
#endif  // D_ENV_QT_PLATFORM_EGLFS

// 4.1.9
// D_ENV_QT_PLATFORM_INTEGRITY
//   feature: detect if targeting the INTEGRITY RTOS.
#ifndef D_ENV_QT_PLATFORM_INTEGRITY
    #if defined(Q_OS_INTEGRITY)
        #define D_ENV_QT_PLATFORM_INTEGRITY 1
    #else
        #define D_ENV_QT_PLATFORM_INTEGRITY 0
    #endif
#endif  // D_ENV_QT_PLATFORM_INTEGRITY

// 4.1.10
// D_ENV_QT_PLATFORM_QNX
//   feature: detect if targeting QNX.
#ifndef D_ENV_QT_PLATFORM_QNX
    #if defined(Q_OS_QNX)
        #define D_ENV_QT_PLATFORM_QNX       1
    #else
        #define D_ENV_QT_PLATFORM_QNX       0
    #endif
#endif  // D_ENV_QT_PLATFORM_QNX

// 4.1.11
// D_ENV_QT_PLATFORM_VXWORKS
//   feature: detect if targeting VxWorks.
#ifndef D_ENV_QT_PLATFORM_VXWORKS
    #if defined(Q_OS_VXWORKS)
        #define D_ENV_QT_PLATFORM_VXWORKS   1
    #else
        #define D_ENV_QT_PLATFORM_VXWORKS   0
    #endif
#endif  // D_ENV_QT_PLATFORM_VXWORKS

// 4.2    Platform extras (Qt 5 only; removed in Qt 6)
//------------------------------------------------------------------------------
// 4.2.1
// D_ENV_QT_HAS_X11EXTRAS
//   feature: detect if QtX11Extras module is available (Qt 5 only,
// replaced by QNativeInterface in Qt 6).
#ifndef D_ENV_QT_HAS_X11EXTRAS
    #if defined(QT_X11EXTRAS_LIB)
        #define D_ENV_QT_HAS_X11EXTRAS     1
    #else
        #define D_ENV_QT_HAS_X11EXTRAS     0
    #endif
#endif  // D_ENV_QT_HAS_X11EXTRAS

// 4.2.2
// D_ENV_QT_HAS_WINEXTRAS
//   feature: detect if QtWinExtras module is available (Qt 5 only,
// removed in Qt 6).
#ifndef D_ENV_QT_HAS_WINEXTRAS
    #if defined(QT_WINEXTRAS_LIB)
        #define D_ENV_QT_HAS_WINEXTRAS     1
    #else
        #define D_ENV_QT_HAS_WINEXTRAS     0
    #endif
#endif  // D_ENV_QT_HAS_WINEXTRAS

// 4.2.3
// D_ENV_QT_HAS_MACEXTRAS
//   feature: detect if QtMacExtras module is available (Qt 5 only,
// removed in Qt 6).
#ifndef D_ENV_QT_HAS_MACEXTRAS
    #if defined(QT_MACEXTRAS_LIB)
        #define D_ENV_QT_HAS_MACEXTRAS     1
    #else
        #define D_ENV_QT_HAS_MACEXTRAS     0
    #endif
#endif  // D_ENV_QT_HAS_MACEXTRAS


//==============================================================================
// 5.  OPENGL AND RENDERING
//==============================================================================


// 5.1    OpenGL and rendering
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_QT_OPENGL_ES
//   feature: detect if Qt is configured for OpenGL ES.
#ifndef D_ENV_QT_OPENGL_ES_VER
    #if defined(QT_OPENGL_ES_2)
        #define D_ENV_QT_OPENGL_ES_VER  2
    #elif defined(QT_OPENGL_ES_3)
        #define D_ENV_QT_OPENGL_ES_VER  3
    #elif defined(QT_OPENGL_ES_3_1)
        #define D_ENV_QT_OPENGL_ES_VER  31
    #elif defined(QT_OPENGL_ES_3_2)
        #define D_ENV_QT_OPENGL_ES_VER  32
    #elif defined(QT_OPENGL_ES)
        #define D_ENV_QT_OPENGL_ES_VER  1
    #else
        #define D_ENV_QT_OPENGL_ES_VER  0
    #endif
#endif  // D_ENV_QT_OPENGL_ES_VER

#ifndef D_ENV_QT_OPENGL_ES
    #if (D_ENV_QT_OPENGL_ES_VER > 0)
        #define D_ENV_QT_OPENGL_ES      1
    #else
        #define D_ENV_QT_OPENGL_ES      0
    #endif
#endif  // D_ENV_QT_OPENGL_ES
// 5.1.2
// D_ENV_QT_OPENGL_DESKTOP
//   feature: detect if Qt is configured for desktop OpenGL.
#ifndef D_ENV_QT_OPENGL_DESKTOP
    #if ( (D_ENV_QT_HAS_OPENGL) &&                                             \
          (!D_ENV_QT_OPENGL_ES) )
        #define D_ENV_QT_OPENGL_DESKTOP    1
    #else
        #define D_ENV_QT_OPENGL_DESKTOP    0
    #endif
#endif  // D_ENV_QT_OPENGL_DESKTOP

// 5.1.3
// D_ENV_QT_OPENGL_DYNAMIC
//   feature: detect if Qt uses dynamic OpenGL loading (Windows, Qt 5.4+).
// QT_OPENGL_DYNAMIC enables runtime switching between desktop and ANGLE.
#ifndef D_ENV_QT_OPENGL_DYNAMIC
    #if defined(QT_OPENGL_DYNAMIC)
        #define D_ENV_QT_OPENGL_DYNAMIC    1
    #else
        #define D_ENV_QT_OPENGL_DYNAMIC    0
    #endif
#endif  // D_ENV_QT_OPENGL_DYNAMIC

// 5.1.4
// D_ENV_QT_HAS_VULKAN
//   feature: detect if Qt Vulkan support is available (Qt 5.10+).
#ifndef D_ENV_QT_HAS_VULKAN
    #if ( (D_ENV_QT_AVAILABLE)     &&                                          \
          (!defined(QT_NO_VULKAN)) &&                                          \
          (D_ENV_QT_VER >= D_ENV_QT_VERSION_5_10) )
        #define D_ENV_QT_HAS_VULKAN        1
    #else
        #define D_ENV_QT_HAS_VULKAN        0
    #endif
#endif  // D_ENV_QT_HAS_VULKAN

// 5.1.5
// D_ENV_QT_HAS_RHI
//   feature: detect if Qt RHI (Rendering Hardware Interface) is available
// (Qt 6.0+). RHI abstracts Vulkan, Metal, D3D11, D3D12, and OpenGL.
#ifndef D_ENV_QT_HAS_RHI
    #if D_ENV_QT_IS_QT6
        #define D_ENV_QT_HAS_RHI           1
    #else
        #define D_ENV_QT_HAS_RHI           0
    #endif
#endif  // D_ENV_QT_HAS_RHI


//==============================================================================
// 6.  FEATURE FLAGS
//==============================================================================


// 6.1    Accessibility
//------------------------------------------------------------------------------
// 6.1.1
// D_ENV_QT_HAS_ACCESSIBILITY
//   feature: detect if accessibility support is enabled.
#ifndef D_ENV_QT_HAS_ACCESSIBILITY
    #if ( (D_ENV_QT_AVAILABLE) &&                                              \
          (!defined(QT_NO_ACCESSIBILITY)) )
        #define D_ENV_QT_HAS_ACCESSIBILITY 1
    #else
        #define D_ENV_QT_HAS_ACCESSIBILITY 0
    #endif
#endif  // D_ENV_QT_HAS_ACCESSIBILITY
// 6.2    Internationalization
//------------------------------------------------------------------------------
// 6.2.1
// D_ENV_QT_HAS_TRANSLATION
//   feature: detect if Qt translation/i18n support is available.
#ifndef D_ENV_QT_HAS_TRANSLATION
    #if ( (D_ENV_QT_AVAILABLE) &&                                              \
          (!defined(QT_NO_TRANSLATION)) )
        #define D_ENV_QT_HAS_TRANSLATION   1
    #else
        #define D_ENV_QT_HAS_TRANSLATION   0
    #endif
#endif  // D_ENV_QT_HAS_TRANSLATION

// 6.2.2
// D_ENV_QT_HAS_ICU
//   feature: detect if Qt was built with ICU support.
#ifndef D_ENV_QT_HAS_ICU
    #if defined(QT_USE_ICU)
        #define D_ENV_QT_HAS_ICU           1
    #else
        #define D_ENV_QT_HAS_ICU           0
    #endif
#endif  // D_ENV_QT_HAS_ICU
// 6.3    Threading and concurrency
//------------------------------------------------------------------------------
// 6.3.1
// D_ENV_QT_HAS_THREAD
//   feature: detect if Qt threading support is enabled.
#ifndef D_ENV_QT_HAS_THREAD
    #if ( (D_ENV_QT_AVAILABLE) &&                                              \
          (!defined(QT_NO_THREAD)) )
        #define D_ENV_QT_HAS_THREAD        1
    #else
        #define D_ENV_QT_HAS_THREAD        0
    #endif
#endif  // D_ENV_QT_HAS_THREAD

// 6.3.2
// D_ENV_QT_HAS_FUTURE
//   feature: detect if QFuture/QPromise are available (Qt 6 expanded).
#ifndef D_ENV_QT_HAS_FUTURE
    #if ( (D_ENV_QT_IS_QT5) ||                                                 \
          (D_ENV_QT_IS_QT6) )
        #define D_ENV_QT_HAS_FUTURE        1
    #else
        #define D_ENV_QT_HAS_FUTURE        0
    #endif
#endif  // D_ENV_QT_HAS_FUTURE
// 6.4    File system and I/O
//------------------------------------------------------------------------------
// 6.4.1
// D_ENV_QT_HAS_FILESYSTEMWATCHER
//   feature: detect if QFileSystemWatcher is available.
#ifndef D_ENV_QT_HAS_FILESYSTEMWATCHER
    #if ( (D_ENV_QT_AVAILABLE) &&                                              \
          (!defined(QT_NO_FILESYSTEMWATCHER)) )
        #define D_ENV_QT_HAS_FILESYSTEMWATCHER 1
    #else
        #define D_ENV_QT_HAS_FILESYSTEMWATCHER 0
    #endif
#endif  // D_ENV_QT_HAS_FILESYSTEMWATCHER

// 6.4.2
// D_ENV_QT_HAS_PROCESS
//   feature: detect if QProcess is available.
#ifndef D_ENV_QT_HAS_PROCESS
    #if ( (D_ENV_QT_AVAILABLE) &&                                              \
          (!defined(QT_NO_PROCESS)) )
        #define D_ENV_QT_HAS_PROCESS       1
    #else
        #define D_ENV_QT_HAS_PROCESS       0
    #endif
#endif  // D_ENV_QT_HAS_PROCESS

// 6.4.3
// D_ENV_QT_HAS_SHAREDMEMORY
//   feature: detect if QSharedMemory is available.
#ifndef D_ENV_QT_HAS_SHAREDMEMORY
    #if ( (D_ENV_QT_AVAILABLE) &&                                              \
          (!defined(QT_NO_SHAREDMEMORY)) )
        #define D_ENV_QT_HAS_SHAREDMEMORY  1
    #else
        #define D_ENV_QT_HAS_SHAREDMEMORY  0
    #endif
#endif  // D_ENV_QT_HAS_SHAREDMEMORY
// 6.5    SSL and cryptography
//------------------------------------------------------------------------------
// 6.5.1
// D_ENV_QT_HAS_SSL
//   feature: detect if SSL/TLS support is available in QtNetwork.
#ifndef D_ENV_QT_HAS_SSL
    #if ( (D_ENV_QT_HAS_NETWORK) &&                                            \
          (!defined(QT_NO_SSL)) )
        #define D_ENV_QT_HAS_SSL           1
    #else
        #define D_ENV_QT_HAS_SSL           0
    #endif
#endif  // D_ENV_QT_HAS_SSL

// 6.5.2
// D_ENV_QT_HAS_OPENSSL / D_ENV_QT_OPENSSL_LINKED
//   feature: detect if Qt was built against OpenSSL specifically, with
// D_ENV_QT_OPENSSL_LINKED set when it is linked at build time rather than
// loaded at runtime.
#ifndef D_ENV_QT_OPENSSL_LINKED
    #if defined(QT_LINKED_OPENSSL)
        #define D_ENV_QT_OPENSSL_LINKED 1
    #else
        #define D_ENV_QT_OPENSSL_LINKED 0
    #endif
#endif  // D_ENV_QT_OPENSSL_LINKED

#ifndef D_ENV_QT_HAS_OPENSSL
    #if ( (D_ENV_QT_OPENSSL_LINKED) ||                                         \
          (defined(QT_RUNTIME_OPENSSL)) )
        #define D_ENV_QT_HAS_OPENSSL    1
    #else
        #define D_ENV_QT_HAS_OPENSSL    0
    #endif
#endif  // D_ENV_QT_HAS_OPENSSL// 6.6    Regular expressions
//------------------------------------------------------------------------------
// 6.6.1
// D_ENV_QT_HAS_REGEXP
//   feature: detect if QRegExp is available (Qt 4/5, removed in Qt 6).
#ifndef D_ENV_QT_HAS_REGEXP
    #if ( (D_ENV_QT_IS_QT4 || D_ENV_QT_IS_QT5) &&                              \
          (!defined(QT_NO_REGEXP)) )
        #define D_ENV_QT_HAS_REGEXP        1
    #else
        #define D_ENV_QT_HAS_REGEXP        0
    #endif
#endif  // D_ENV_QT_HAS_REGEXP

// 6.6.2
// D_ENV_QT_HAS_REGULAREXPRESSION
//   feature: detect if QRegularExpression is available (Qt 5.0+).
// this is the PCRE2-based replacement for QRegExp.
#ifndef D_ENV_QT_HAS_REGULAREXPRESSION
    #if ( (D_ENV_QT_AVAILABLE)                 &&                              \
          (D_ENV_QT_VER >= D_ENV_QT_VERSION_5) &&                              \
          (!defined(QT_NO_REGULAREXPRESSION)) )
        #define D_ENV_QT_HAS_REGULAREXPRESSION 1
    #else
        #define D_ENV_QT_HAS_REGULAREXPRESSION 0
    #endif
#endif  // D_ENV_QT_HAS_REGULAREXPRESSION


//==============================================================================
// 7.  C++ STANDARD AND DEPRECATION
//==============================================================================


// 7.1    C++ standard interplay
//------------------------------------------------------------------------------
// 7.1.1
// D_ENV_QT_CPP_MINIMUM_MET
//   feature: evaluates to 1 if the current C++ standard meets the minimum
// required by the detected Qt version.
// Qt 6 requires C++17; Qt 5.7+ requires C++11; Qt 4 requires C++98.
#ifndef D_ENV_QT_CPP_MINIMUM_MET
    #if D_ENV_QT_IS_QT6
        #ifdef D_ENV_LANG_CPP_STANDARD
            #define D_ENV_QT_CPP_MINIMUM_MET                                   \
                (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP17)
        #else
            #define D_ENV_QT_CPP_MINIMUM_MET 0
        #endif  // D_ENV_LANG_CPP_STANDARD
    #elif ( (D_ENV_QT_IS_QT5) &&                                               \
            (D_ENV_QT_VER >= D_ENV_QT_VERSION_5_7) )
        #ifdef D_ENV_LANG_CPP_STANDARD
            #define D_ENV_QT_CPP_MINIMUM_MET                                   \
                (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP11)
        #else
            #define D_ENV_QT_CPP_MINIMUM_MET 0
        #endif  // D_ENV_LANG_CPP_STANDARD
    #elif ( (D_ENV_QT_IS_QT5) ||                                               \
            (D_ENV_QT_IS_QT4) )
        #ifdef D_ENV_LANG_CPP_STANDARD
            #define D_ENV_QT_CPP_MINIMUM_MET                                   \
                (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP98)
        #else
            #define D_ENV_QT_CPP_MINIMUM_MET 0
        #endif  // D_ENV_LANG_CPP_STANDARD
    #else
        #define D_ENV_QT_CPP_MINIMUM_MET    0
    #endif
#endif  // D_ENV_QT_CPP_MINIMUM_MET

// 7.1.2
// D_ENV_QT_HAS_CPP17_API
//   feature: evaluates to 1 if Qt C++17-era APIs are available.
// Qt 5.15+ began offering opt-in C++17 APIs; Qt 6 requires them.
#ifndef D_ENV_QT_HAS_CPP17_API
    #if ( (D_ENV_QT_IS_QT6) ||                                                 \
          (D_ENV_QT_IS_QT5 && (D_ENV_QT_VER >= D_ENV_QT_VERSION_5_15)) )
        #ifdef D_ENV_LANG_CPP_STANDARD
            #define D_ENV_QT_HAS_CPP17_API                                     \
                (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP17)
        #else
            #define D_ENV_QT_HAS_CPP17_API  0
        #endif  // D_ENV_LANG_CPP_STANDARD
    #else
        #define D_ENV_QT_HAS_CPP17_API      0
    #endif
#endif  // D_ENV_QT_HAS_CPP17_API

// 7.1.3
// D_ENV_QT_HAS_CPP20_API
//   feature: evaluates to 1 if Qt C++20-era APIs are available.
// Qt 6.4+ introduced opt-in C++20 features (QProperty improvements, etc.).
#ifndef D_ENV_QT_HAS_CPP20_API
    #if ( (D_ENV_QT_IS_QT6) &&                                                 \
          (D_ENV_QT_VER >= D_ENV_QT_VERSION_6_4) )
        #ifdef D_ENV_LANG_CPP_STANDARD
            #define D_ENV_QT_HAS_CPP20_API                                     \
                (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP20)
        #else
            #define D_ENV_QT_HAS_CPP20_API  0
        #endif  // D_ENV_LANG_CPP_STANDARD
    #else
        #define D_ENV_QT_HAS_CPP20_API      0
    #endif
#endif  // D_ENV_QT_HAS_CPP20_API

// 7.2    Deprecation and migration
//------------------------------------------------------------------------------
// 7.2.1
// D_ENV_QT_DEPRECATION_CUTOFF
//   feature: detect the Qt deprecation cutoff version if configured.
// QT_DISABLE_DEPRECATED_BEFORE hides APIs deprecated before that version.
#ifndef D_ENV_QT_DEPRECATION_CUTOFF
    #if defined(QT_DISABLE_DEPRECATED_BEFORE)
        #define D_ENV_QT_DEPRECATION_CUTOFF QT_DISABLE_DEPRECATED_BEFORE
    #else
        #define D_ENV_QT_DEPRECATION_CUTOFF 0
    #endif
#endif  // D_ENV_QT_DEPRECATION_CUTOFF

// 7.2.2
// D_ENV_QT_NO_DEPRECATED_WARNINGS
//   feature: detect if deprecated-API warnings have been silenced.
#ifndef D_ENV_QT_NO_DEPRECATED_WARNINGS
    #if defined(QT_NO_DEPRECATED_WARNINGS)
        #define D_ENV_QT_NO_DEPRECATED_WARNINGS 1
    #else
        #define D_ENV_QT_NO_DEPRECATED_WARNINGS 0
    #endif
#endif  // D_ENV_QT_NO_DEPRECATED_WARNINGS

// 7.2.3
// D_ENV_QT_HAS_QT5_COMPAT
//   feature: detect if the Qt5Compat module is available (Qt 6 only).
// provides classes removed from Qt 6 for migration purposes.
#ifndef D_ENV_QT_HAS_QT5_COMPAT
    #if defined(QT_CORE5COMPAT_LIB)
        #define D_ENV_QT_HAS_QT5_COMPAT    1
    #else
        #define D_ENV_QT_HAS_QT5_COMPAT    0
    #endif
#endif  // D_ENV_QT_HAS_QT5_COMPAT


//==============================================================================
// 8.  RUNTIME DETECTION
//==============================================================================
// Declared here and defined in the Qt implementation; unlike the macros
// above, they report on the Qt the program actually runs against.


// 8.1    Runtime queries
//------------------------------------------------------------------------------
//   C linkage for C++ callers. The env headers sit below djinterp.h, so the
// D_EXTERN_C_BEGIN / D_EXTERN_C_END pair is not available here.
#if D_ENV_LANG_USING_CPP
    extern "C" {
#endif

/**
 * @brief Returns the version of the Qt library loaded at runtime.
 *
 * @note It can differ from the compile-time version when the program is
 *       dynamically linked against another Qt build.
 *
 * @return qVersion()'s string, or "N/A" if Qt is not available at runtime.
 */
const char* d_env_qt_get_runtime_version(void);
/**
 * @brief Returns the Qt version the program was compiled against.
 *
 * @return QT_VERSION_STR, or "None" if Qt was not detected.
 */
const char* d_env_qt_get_compile_version(void);
/**
 * @brief Tests whether the runtime Qt version matches the compile-time one.
 *
 * @return `1` if the major and minor versions match, `0` otherwise.
 */
int         d_env_qt_runtime_matches_compile(void);
/**
 * @brief Tests at runtime whether a Qt module's shared library is loadable.
 *
 * @param[in] _module_name  the module's name, e.g. "QtWidgets" or "QtQml".
 * @return `1` if the module's library is loadable, `0` otherwise.
 */
int         d_env_qt_has_module(const char* _module_name);
/**
 * @brief Prints the detected Qt environment: version, modules, platform,
 *        build configuration, and feature flags.
 */
void        d_env_qt_print_info(void);

#if D_ENV_LANG_USING_CPP
    }
#endif


//==============================================================================
// 9.  CONVENIENCE MACROS
//==============================================================================


// 9.1    Combined predicates
//------------------------------------------------------------------------------
// 9.1.1
// D_ENV_HAS_QT
//   macro: evaluates to 1 if any version of Qt is detected, 0 otherwise.
#define D_ENV_HAS_QT()                                                         \
    (D_ENV_QT_AVAILABLE)

// 9.1.2
// D_ENV_QT_AT_LEAST
//   macro: evaluates to 1 if the detected Qt version is at least the
// specified major, minor, patch version.
#define D_ENV_QT_AT_LEAST(major, minor, patch)                                 \
    ( (D_ENV_QT_AVAILABLE) &&                                                  \
      (D_ENV_QT_VER >= D_ENV_QT_VERSION_CHECK(major, minor, patch)) )

// 9.1.3
// D_ENV_QT_AT_LEAST_HEX
//   macro: evaluates to 1 if the detected Qt version is at least the
// specified hex version constant (e.g. D_ENV_QT_VERSION_5_12).
#define D_ENV_QT_AT_LEAST_HEX(hex_version)                                     \
    ( (D_ENV_QT_AVAILABLE) &&                                                  \
      (D_ENV_QT_VER >= (hex_version)) )

// 9.1.4
// D_ENV_QT_VERSION_CHECK
//   macro: packs a version as QT_VERSION does, with every parameter
// parenthesized. This header used to define Qt's own QT_VERSION_CHECK when
// no Qt header had: a Qt header included afterwards redefined it with a
// warning (an error under -Werror), and with no Qt at all,
// #ifdef QT_VERSION_CHECK said Qt was there. It never defines Qt's name now
// (decision 46 of the register).
#define D_ENV_QT_VERSION_CHECK(major, minor, patch)                            \
    ( ((major) << 16) | ((minor) << 8) | (patch) )

// 9.1.5
// D_ENV_QT_IS_SERIES
//   macro: evaluates to 1 if the detected Qt major version matches.
#define D_ENV_QT_IS_SERIES(major)                                              \
    ( (D_ENV_QT_AVAILABLE) &&                                                  \
      (D_ENV_QT_VER_MAJOR == (major)) )

// 9.1.6
// D_ENV_QT_IS_LTS
//   macro: evaluates to 1 if the detected Qt version is a known LTS
// release (Qt 5.6, 5.9, 5.12, 5.15, 6.2, 6.5, 6.8).
#define D_ENV_QT_IS_LTS()                                                      \
    ( ( D_ENV_QT_IS_QT5 &&                                                     \
        ( (D_ENV_QT_VER_MINOR == 6)  ||                                        \
          (D_ENV_QT_VER_MINOR == 9)  ||                                        \
          (D_ENV_QT_VER_MINOR == 12) ||                                        \
          (D_ENV_QT_VER_MINOR == 15) ) )                                       \
      ||                                                                       \
      ( D_ENV_QT_IS_QT6 &&                                                     \
        ( (D_ENV_QT_VER_MINOR == 2)  ||                                        \
          (D_ENV_QT_VER_MINOR == 5)  ||                                        \
          (D_ENV_QT_VER_MINOR == 8) ) ) )

// 9.1.7
// D_ENV_QT_HAS_MODERN_CONNECT
//   macro: evaluates to 1 if the Qt 5+ type-safe signal/slot connect
// syntax is available.
#define D_ENV_QT_HAS_MODERN_CONNECT()                                          \
    ( (D_ENV_QT_IS_QT5) ||                                                     \
      (D_ENV_QT_IS_QT6) )

// 9.1.8
// D_ENV_QT_HAS_QPROPERTY
//   macro: evaluates to 1 if the new QProperty binding system is available
// (Qt 6.0+).
#define D_ENV_QT_HAS_QPROPERTY()                                               \
    ( D_ENV_QT_IS_QT6 )


#endif  // DJINTERP_ENV_UI_ENV_QT_H
