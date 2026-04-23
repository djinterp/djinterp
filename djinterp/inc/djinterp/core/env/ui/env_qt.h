/******************************************************************************
* djinterp [core]                                                     env_qt.h
*
*   djinterp Qt environment detection header:
* This header provides comprehensive, compile-time detection of the Qt
* framework across all major versions (Qt 1 through Qt 6). It detects:
*   - Qt presence and exact version (major, minor, patch)
*   - Qt edition and licensing (Open Source, Commercial)
*   - Core module availability (Widgets, QML, Network, SQL, etc.)
*   - Platform integration modules (X11, Wayland, WinExtras, MacExtras)
*   - Build configuration (static vs shared, debug vs release, namespace)
*   - Feature flags (accessibility, OpenGL, D-Bus, concurrent, etc.)
*   - C++ standard interplay (Qt 6 requires C++17, etc.)
*
* scope:
*   - Qt version detection from QT_VERSION and QT_VERSION_STR
*   - module availability via QT_CONFIG and individual module macros
*   - platform backend detection (XCB, Wayland, Win32, Cocoa, EGLFS)
*   - Qt build properties (shared/static, namespaced, feature flags)
*   - cross-referencing Qt version requirements against C++ standard
*
* usage:
*   Include after env.h. Qt headers (QtGlobal / QtVersionNumber) must be
*   included before this header for compile-time detection to work:
*     #include <QtGlobal>          // or <QtCore/QtGlobal>
*     #include "./env_qt.h"
*   Without a prior Qt include, the header falls back to "not detected."
*
* NAMING CONVENTION:
*   D_ENV_QT_[CATEGORY]_[FEATURE] - 1 if available, 0 otherwise
*
*
* path:      /inc/base/env/ui/env_qt.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.28
******************************************************************************/

#ifndef DJINTERP_ENV_QT_
#define DJINTERP_ENV_QT_ 1

#include "./env.h"


//////////////////////////////////////////////////////////////////////////////
// I.   QT VERSION CONSTANTS
////////////////////////////////////////////////////////////////////////////////

// A.  major version constants

// D_ENV_QT_VERSION_1
//   constant: QT_VERSION value for Qt 1.x (earliest public release).
#define D_ENV_QT_VERSION_1              0x010000

// D_ENV_QT_VERSION_2
//   constant: QT_VERSION value for Qt 2.0.
#define D_ENV_QT_VERSION_2              0x020000

// D_ENV_QT_VERSION_3
//   constant: QT_VERSION value for Qt 3.0.
#define D_ENV_QT_VERSION_3              0x030000

// D_ENV_QT_VERSION_4
//   constant: QT_VERSION value for Qt 4.0.0.
#define D_ENV_QT_VERSION_4              0x040000

// D_ENV_QT_VERSION_5
//   constant: QT_VERSION value for Qt 5.0.0.
#define D_ENV_QT_VERSION_5              0x050000

// D_ENV_QT_VERSION_6
//   constant: QT_VERSION value for Qt 6.0.0.
#define D_ENV_QT_VERSION_6              0x060000


// B.  notable minor version constants (Qt 4)

// D_ENV_QT_VERSION_4_6
//   constant: QT_VERSION for Qt 4.6.0 (animation framework, state machines).
#define D_ENV_QT_VERSION_4_6            0x040600

// D_ENV_QT_VERSION_4_7
//   constant: QT_VERSION for Qt 4.7.0 (QML/QtDeclarative introduced).
#define D_ENV_QT_VERSION_4_7            0x040700

// D_ENV_QT_VERSION_4_8
//   constant: QT_VERSION for Qt 4.8.0 (last Qt 4 feature release).
#define D_ENV_QT_VERSION_4_8            0x040800


// C.  notable minor version constants (Qt 5)

// D_ENV_QT_VERSION_5_0
//   constant: QT_VERSION for Qt 5.0.0 (initial Qt 5 release).
#define D_ENV_QT_VERSION_5_0            0x050000

// D_ENV_QT_VERSION_5_1
//   constant: QT_VERSION for Qt 5.1.0 (Qt Sensors, Qt Serial Port).
#define D_ENV_QT_VERSION_5_1            0x050100

// D_ENV_QT_VERSION_5_2
//   constant: QT_VERSION for Qt 5.2.0 (Android/iOS fully supported).
#define D_ENV_QT_VERSION_5_2            0x050200

// D_ENV_QT_VERSION_5_3
//   constant: QT_VERSION for Qt 5.3.0 (Qt WebEngine preview).
#define D_ENV_QT_VERSION_5_3            0x050300

// D_ENV_QT_VERSION_5_4
//   constant: QT_VERSION for Qt 5.4.0 (Qt WebChannel, Qt WebEngine).
#define D_ENV_QT_VERSION_5_4            0x050400

// D_ENV_QT_VERSION_5_5
//   constant: QT_VERSION for Qt 5.5.0 (Qt 3D, Qt Canvas3D).
#define D_ENV_QT_VERSION_5_5            0x050500

// D_ENV_QT_VERSION_5_6
//   constant: QT_VERSION for Qt 5.6.0 (first LTS release).
#define D_ENV_QT_VERSION_5_6            0x050600

// D_ENV_QT_VERSION_5_7
//   constant: QT_VERSION for Qt 5.7.0 (C++11 required, Qt Gamepad).
#define D_ENV_QT_VERSION_5_7            0x050700

// D_ENV_QT_VERSION_5_9
//   constant: QT_VERSION for Qt 5.9.0 (LTS, Qt Lite / configure revamp).
#define D_ENV_QT_VERSION_5_9            0x050900

// D_ENV_QT_VERSION_5_10
//   constant: QT_VERSION for Qt 5.10.0 (Qt Virtual Keyboard, Qt Quick).
#define D_ENV_QT_VERSION_5_10           0x050A00

// D_ENV_QT_VERSION_5_12
//   constant: QT_VERSION for Qt 5.12.0 (LTS, Qt for Python, C++17 prep).
#define D_ENV_QT_VERSION_5_12           0x050C00

// D_ENV_QT_VERSION_5_15
//   constant: QT_VERSION for Qt 5.15.0 (LTS, final Qt 5 release, Qt 6
// migration bridge).
#define D_ENV_QT_VERSION_5_15           0x050F00


// D.  notable minor version constants (Qt 6)

// D_ENV_QT_VERSION_6_0
//   constant: QT_VERSION for Qt 6.0.0 (C++17 required, CMake build system).
#define D_ENV_QT_VERSION_6_0            0x060000

// D_ENV_QT_VERSION_6_1
//   constant: QT_VERSION for Qt 6.1.0 (Qt Charts, Qt Data Visualization
// ported).
#define D_ENV_QT_VERSION_6_1            0x060100

// D_ENV_QT_VERSION_6_2
//   constant: QT_VERSION for Qt 6.2.0 (LTS, Qt Multimedia rewritten,
// Qt Connectivity restored).
#define D_ENV_QT_VERSION_6_2            0x060200

// D_ENV_QT_VERSION_6_3
//   constant: QT_VERSION for Qt 6.3.0 (Qt Language Server Protocol).
#define D_ENV_QT_VERSION_6_3            0x060300

// D_ENV_QT_VERSION_6_4
//   constant: QT_VERSION for Qt 6.4.0 (Qt HTTP Server, Qt Quick 3D
// Physics).
#define D_ENV_QT_VERSION_6_4            0x060400

// D_ENV_QT_VERSION_6_5
//   constant: QT_VERSION for Qt 6.5.0 (LTS, Qt Graphs introduced).
#define D_ENV_QT_VERSION_6_5            0x060500

// D_ENV_QT_VERSION_6_6
//   constant: QT_VERSION for Qt 6.6.0 (Qt Graphs 3D, Qt GRPC).
#define D_ENV_QT_VERSION_6_6            0x060600

// D_ENV_QT_VERSION_6_7
//   constant: QT_VERSION for Qt 6.7.0 (latest feature release).
#define D_ENV_QT_VERSION_6_7            0x060700

// D_ENV_QT_VERSION_6_8
//   constant: QT_VERSION for Qt 6.8.0 (LTS).
#define D_ENV_QT_VERSION_6_8            0x060800


////////////////////////////////////////////////////////////////////////////////
// II.  COMPILE-TIME QT DETECTION
////////////////////////////////////////////////////////////////////////////////

// A.  presence and version extraction

// detect Qt via QT_VERSION (defined by <QtGlobal> or <QtCore/qglobal.h>)
#if defined(QT_VERSION)
    #define D_ENV_QT_AVAILABLE          1

    // D_ENV_QT_VER
    //   constant: raw QT_VERSION hex value.
    #define D_ENV_QT_VER                QT_VERSION

    // D_ENV_QT_VER_MAJOR
    //   constant: major version extracted from QT_VERSION.
    #define D_ENV_QT_VER_MAJOR          ((QT_VERSION >> 16) & 0xFF)

    // D_ENV_QT_VER_MINOR
    //   constant: minor version extracted from QT_VERSION.
    #define D_ENV_QT_VER_MINOR          ((QT_VERSION >> 8) & 0xFF)

    // D_ENV_QT_VER_PATCH
    //   constant: patch version extracted from QT_VERSION.
    #define D_ENV_QT_VER_PATCH          (QT_VERSION & 0xFF)

    // D_ENV_QT_VER_STR
    //   constant: version string if QT_VERSION_STR is defined.
    #ifdef QT_VERSION_STR
        #define D_ENV_QT_VER_STR        QT_VERSION_STR
    #else
        #define D_ENV_QT_VER_STR        "Unknown"
    #endif

#else
    #define D_ENV_QT_AVAILABLE          0
    #define D_ENV_QT_VER                0
    #define D_ENV_QT_VER_MAJOR          0
    #define D_ENV_QT_VER_MINOR          0
    #define D_ENV_QT_VER_PATCH          0
    #define D_ENV_QT_VER_STR            "None"
#endif


// B.  major version classification

#if D_ENV_QT_AVAILABLE

    #if (D_ENV_QT_VER >= D_ENV_QT_VERSION_6)
        #define D_ENV_QT_IS_QT6         1
        #define D_ENV_QT_IS_QT5         0
        #define D_ENV_QT_IS_QT4         0
        #define D_ENV_QT_IS_QT3         0
        #define D_ENV_QT_IS_LEGACY      0
        #define D_ENV_QT_SERIES_NAME    "Qt 6"
    #elif (D_ENV_QT_VER >= D_ENV_QT_VERSION_5)
        #define D_ENV_QT_IS_QT6         0
        #define D_ENV_QT_IS_QT5         1
        #define D_ENV_QT_IS_QT4         0
        #define D_ENV_QT_IS_QT3         0
        #define D_ENV_QT_IS_LEGACY      0
        #define D_ENV_QT_SERIES_NAME    "Qt 5"
    #elif (D_ENV_QT_VER >= D_ENV_QT_VERSION_4)
        #define D_ENV_QT_IS_QT6         0
        #define D_ENV_QT_IS_QT5         0
        #define D_ENV_QT_IS_QT4         1
        #define D_ENV_QT_IS_QT3         0
        #define D_ENV_QT_IS_LEGACY      0
        #define D_ENV_QT_SERIES_NAME    "Qt 4"
    #elif (D_ENV_QT_VER >= D_ENV_QT_VERSION_3)
        #define D_ENV_QT_IS_QT6         0
        #define D_ENV_QT_IS_QT5         0
        #define D_ENV_QT_IS_QT4         0
        #define D_ENV_QT_IS_QT3         1
        #define D_ENV_QT_IS_LEGACY      0
        #define D_ENV_QT_SERIES_NAME    "Qt 3"
    #else
        // Qt 1.x or Qt 2.x
        #define D_ENV_QT_IS_QT6         0
        #define D_ENV_QT_IS_QT5         0
        #define D_ENV_QT_IS_QT4         0
        #define D_ENV_QT_IS_QT3         0
        #define D_ENV_QT_IS_LEGACY      1
        #define D_ENV_QT_SERIES_NAME    "Qt (Legacy)"
    #endif

#else
    #define D_ENV_QT_IS_QT6             0
    #define D_ENV_QT_IS_QT5             0
    #define D_ENV_QT_IS_QT4             0
    #define D_ENV_QT_IS_QT3             0
    #define D_ENV_QT_IS_LEGACY          0
    #define D_ENV_QT_SERIES_NAME        "None"
#endif


////////////////////////////////////////////////////////////////////////////////
// III. QT BUILD CONFIGURATION
////////////////////////////////////////////////////////////////////////////////

// A.  static vs shared linkage

// D_ENV_QT_STATIC
//   feature: detect if Qt was built as a static library.
// QT_STATIC is defined by Qt's build system for static builds.
#if defined(QT_STATIC)
    #define D_ENV_QT_STATIC             1
    #define D_ENV_QT_SHARED             0
    #define D_ENV_QT_LINKAGE            "Static"
#elif defined(QT_SHARED)
    #define D_ENV_QT_STATIC             0
    #define D_ENV_QT_SHARED             1
    #define D_ENV_QT_LINKAGE            "Shared"
#else
    // default assumption: shared (most common distribution)
    #define D_ENV_QT_STATIC             0
    #define D_ENV_QT_SHARED             1
    #define D_ENV_QT_LINKAGE            "Unknown (assuming Shared)"
#endif

// B.  debug vs release

// D_ENV_QT_DEBUG
//   feature: detect if Qt was built in debug mode.
#if defined(QT_DEBUG)
    #define D_ENV_QT_DEBUG              1
    #define D_ENV_QT_RELEASE            0
    #define D_ENV_QT_BUILD_MODE         "Debug"
#elif defined(QT_NO_DEBUG)
    #define D_ENV_QT_DEBUG              0
    #define D_ENV_QT_RELEASE            1
    #define D_ENV_QT_BUILD_MODE         "Release"
#else
    #define D_ENV_QT_DEBUG              0
    #define D_ENV_QT_RELEASE            0
    #define D_ENV_QT_BUILD_MODE         "Unknown"
#endif

// C.  namespace

// D_ENV_QT_NAMESPACED
//   feature: detect if Qt was built with a custom namespace
// (QT_NAMESPACE / QT_BEGIN_NAMESPACE).
#if defined(QT_NAMESPACE)
    #define D_ENV_QT_NAMESPACED         1
#else
    #define D_ENV_QT_NAMESPACED         0
#endif


////////////////////////////////////////////////////////////////////////////////
// IV.  CORE MODULE DETECTION
////////////////////////////////////////////////////////////////////////////////

// module detection via QT_MODULE_* or module-specific macros defined when
// the corresponding Qt module header is included.

// A.  essential modules (all Qt 5/6 installations)

// D_ENV_QT_HAS_CORE
//   feature: detect if QtCore module is available.
// QtCore is always present when Qt is detected.
#if D_ENV_QT_AVAILABLE
    #define D_ENV_QT_HAS_CORE          1
#else
    #define D_ENV_QT_HAS_CORE          0
#endif

// D_ENV_QT_HAS_GUI
//   feature: detect if QtGui module is available.
#if defined(QT_GUI_LIB)
    #define D_ENV_QT_HAS_GUI           1
#elif D_ENV_QT_AVAILABLE && !defined(QT_NO_GUI)
    #define D_ENV_QT_HAS_GUI           1
#else
    #define D_ENV_QT_HAS_GUI           0
#endif

// D_ENV_QT_HAS_WIDGETS
//   feature: detect if QtWidgets module is available (Qt 5+).
// in Qt 4, widgets lived inside QtGui.
#if defined(QT_WIDGETS_LIB)
    #define D_ENV_QT_HAS_WIDGETS       1
#elif ( D_ENV_QT_IS_QT4 &&                                                    \
        D_ENV_QT_HAS_GUI )
    // Qt 4 widgets are part of QtGui
    #define D_ENV_QT_HAS_WIDGETS       1
#else
    #define D_ENV_QT_HAS_WIDGETS       0
#endif

// D_ENV_QT_HAS_NETWORK
//   feature: detect if QtNetwork module is available.
#if defined(QT_NETWORK_LIB)
    #define D_ENV_QT_HAS_NETWORK       1
#else
    #define D_ENV_QT_HAS_NETWORK       0
#endif

// D_ENV_QT_HAS_SQL
//   feature: detect if QtSql module is available.
#if defined(QT_SQL_LIB)
    #define D_ENV_QT_HAS_SQL           1
#else
    #define D_ENV_QT_HAS_SQL           0
#endif

// D_ENV_QT_HAS_TEST
//   feature: detect if QtTest module is available.
#if defined(QT_TEST_LIB)
    #define D_ENV_QT_HAS_TEST          1
#else
    #define D_ENV_QT_HAS_TEST          0
#endif

// D_ENV_QT_HAS_CONCURRENT
//   feature: detect if QtConcurrent module is available.
#if defined(QT_CONCURRENT_LIB)
    #define D_ENV_QT_HAS_CONCURRENT    1
#else
    #define D_ENV_QT_HAS_CONCURRENT    0
#endif

// D_ENV_QT_HAS_DBUS
//   feature: detect if QtDBus module is available.
#if defined(QT_DBUS_LIB)
    #define D_ENV_QT_HAS_DBUS          1
#else
    #define D_ENV_QT_HAS_DBUS          0
#endif


// B.  QML and Quick modules

// D_ENV_QT_HAS_QML
//   feature: detect if QtQml module is available (Qt 5+).
#if defined(QT_QML_LIB)
    #define D_ENV_QT_HAS_QML           1
#else
    #define D_ENV_QT_HAS_QML           0
#endif

// D_ENV_QT_HAS_QUICK
//   feature: detect if QtQuick module is available (Qt 5+).
#if defined(QT_QUICK_LIB)
    #define D_ENV_QT_HAS_QUICK         1
#else
    #define D_ENV_QT_HAS_QUICK         0
#endif

// D_ENV_QT_HAS_QUICKCONTROLS2
//   feature: detect if Qt Quick Controls 2 module is available (Qt 5.7+).
// in Qt 6, this merged into QtQuick.
#if defined(QT_QUICKCONTROLS2_LIB)
    #define D_ENV_QT_HAS_QUICKCONTROLS2 1
#elif ( D_ENV_QT_IS_QT6 &&                                                    \
        D_ENV_QT_HAS_QUICK )
    // merged into QtQuick in Qt 6
    #define D_ENV_QT_HAS_QUICKCONTROLS2 1
#else
    #define D_ENV_QT_HAS_QUICKCONTROLS2 0
#endif

// D_ENV_QT_HAS_QUICK3D
//   feature: detect if Qt Quick 3D module is available (Qt 5.15+, Qt 6+).
#if defined(QT_QUICK3D_LIB)
    #define D_ENV_QT_HAS_QUICK3D       1
#else
    #define D_ENV_QT_HAS_QUICK3D       0
#endif

// D_ENV_QT_HAS_DECLARATIVE
//   feature: detect if QtDeclarative (Qt Quick 1) is available (Qt 4.7+).
// deprecated in Qt 5, removed in Qt 6.
#if defined(QT_DECLARATIVE_LIB)
    #define D_ENV_QT_HAS_DECLARATIVE   1
#else
    #define D_ENV_QT_HAS_DECLARATIVE   0
#endif


// C.  media and graphics modules

// D_ENV_QT_HAS_MULTIMEDIA
//   feature: detect if QtMultimedia module is available.
#if defined(QT_MULTIMEDIA_LIB)
    #define D_ENV_QT_HAS_MULTIMEDIA    1
#else
    #define D_ENV_QT_HAS_MULTIMEDIA    0
#endif

// D_ENV_QT_HAS_OPENGL
//   feature: detect if QtOpenGL module is available.
#if defined(QT_OPENGL_LIB)
    #define D_ENV_QT_HAS_OPENGL        1
#elif ( D_ENV_QT_AVAILABLE &&                                                 \
        !defined(QT_NO_OPENGL) )
    #define D_ENV_QT_HAS_OPENGL        1
#else
    #define D_ENV_QT_HAS_OPENGL        0
#endif

// D_ENV_QT_HAS_SVG
//   feature: detect if QtSvg module is available.
#if defined(QT_SVG_LIB)
    #define D_ENV_QT_HAS_SVG           1
#else
    #define D_ENV_QT_HAS_SVG           0
#endif

// D_ENV_QT_HAS_PRINTSUPPORT
//   feature: detect if QtPrintSupport module is available (Qt 5+).
#if defined(QT_PRINTSUPPORT_LIB)
    #define D_ENV_QT_HAS_PRINTSUPPORT  1
#else
    #define D_ENV_QT_HAS_PRINTSUPPORT  0
#endif


// D.  data and serialization modules

// D_ENV_QT_HAS_XML
//   feature: detect if QtXml module is available.
#if defined(QT_XML_LIB)
    #define D_ENV_QT_HAS_XML           1
#else
    #define D_ENV_QT_HAS_XML           0
#endif

// D_ENV_QT_HAS_XMLPATTERNS
//   feature: detect if QtXmlPatterns module is available (Qt 4/5 only,
// removed in Qt 6).
#if defined(QT_XMLPATTERNS_LIB)
    #define D_ENV_QT_HAS_XMLPATTERNS   1
#else
    #define D_ENV_QT_HAS_XMLPATTERNS   0
#endif

// D_ENV_QT_HAS_JSON
//   feature: detect if JSON support is available.
// JSON was added in Qt 5.0 as part of QtCore. always available in Qt 5+.
#if ( D_ENV_QT_IS_QT5 ||                                                      \
      D_ENV_QT_IS_QT6 )
    #define D_ENV_QT_HAS_JSON          1
#else
    #define D_ENV_QT_HAS_JSON          0
#endif

// D_ENV_QT_HAS_CBOR
//   feature: detect if CBOR support is available (Qt 5.12+).
#if ( D_ENV_QT_AVAILABLE &&                                                   \
      (D_ENV_QT_VER >= D_ENV_QT_VERSION_5_12) )
    #define D_ENV_QT_HAS_CBOR          1
#else
    #define D_ENV_QT_HAS_CBOR          0
#endif


// E.  web and connectivity modules

// D_ENV_QT_HAS_WEBENGINE
//   feature: detect if Qt WebEngine module is available (Qt 5.4+).
#if ( defined(QT_WEBENGINE_LIB)      ||                                       \
      defined(QT_WEBENGINECORE_LIB)   ||                                      \
      defined(QT_WEBENGINEWIDGETS_LIB) )
    #define D_ENV_QT_HAS_WEBENGINE     1
#else
    #define D_ENV_QT_HAS_WEBENGINE     0
#endif

// D_ENV_QT_HAS_WEBKIT
//   feature: detect if QtWebKit is available (deprecated in Qt 5,
// removed in Qt 6).
#if defined(QT_WEBKIT_LIB)
    #define D_ENV_QT_HAS_WEBKIT        1
#else
    #define D_ENV_QT_HAS_WEBKIT        0
#endif

// D_ENV_QT_HAS_WEBSOCKETS
//   feature: detect if QtWebSockets module is available (Qt 5.3+).
#if defined(QT_WEBSOCKETS_LIB)
    #define D_ENV_QT_HAS_WEBSOCKETS    1
#else
    #define D_ENV_QT_HAS_WEBSOCKETS    0
#endif

// D_ENV_QT_HAS_WEBCHANNEL
//   feature: detect if QtWebChannel module is available (Qt 5.4+).
#if defined(QT_WEBCHANNEL_LIB)
    #define D_ENV_QT_HAS_WEBCHANNEL    1
#else
    #define D_ENV_QT_HAS_WEBCHANNEL    0
#endif

// D_ENV_QT_HAS_BLUETOOTH
//   feature: detect if QtBluetooth module is available.
#if defined(QT_BLUETOOTH_LIB)
    #define D_ENV_QT_HAS_BLUETOOTH     1
#else
    #define D_ENV_QT_HAS_BLUETOOTH     0
#endif

// D_ENV_QT_HAS_NFC
//   feature: detect if QtNfc module is available.
#if defined(QT_NFC_LIB)
    #define D_ENV_QT_HAS_NFC           1
#else
    #define D_ENV_QT_HAS_NFC           0
#endif

// D_ENV_QT_HAS_SERIALPORT
//   feature: detect if QtSerialPort module is available (Qt 5.1+).
#if defined(QT_SERIALPORT_LIB)
    #define D_ENV_QT_HAS_SERIALPORT    1
#else
    #define D_ENV_QT_HAS_SERIALPORT    0
#endif

// D_ENV_QT_HAS_SERIALBUS
//   feature: detect if QtSerialBus module is available (Qt 5.6+).
#if defined(QT_SERIALBUS_LIB)
    #define D_ENV_QT_HAS_SERIALBUS     1
#else
    #define D_ENV_QT_HAS_SERIALBUS     0
#endif

// D_ENV_QT_HAS_MQTT
//   feature: detect if QtMqtt module is available (Qt 5.10+).
#if defined(QT_MQTT_LIB)
    #define D_ENV_QT_HAS_MQTT          1
#else
    #define D_ENV_QT_HAS_MQTT          0
#endif

// D_ENV_QT_HAS_HTTPSERVER
//   feature: detect if Qt HTTP Server module is available (Qt 6.4+).
#if defined(QT_HTTPSERVER_LIB)
    #define D_ENV_QT_HAS_HTTPSERVER    1
#else
    #define D_ENV_QT_HAS_HTTPSERVER    0
#endif

// D_ENV_QT_HAS_GRPC
//   feature: detect if Qt GRPC module is available (Qt 6.5+).
#if defined(QT_GRPC_LIB)
    #define D_ENV_QT_HAS_GRPC          1
#else
    #define D_ENV_QT_HAS_GRPC          0
#endif

// D_ENV_QT_HAS_PROTOBUF
//   feature: detect if Qt Protobuf module is available (Qt 6.5+).
#if defined(QT_PROTOBUF_LIB)
    #define D_ENV_QT_HAS_PROTOBUF      1
#else
    #define D_ENV_QT_HAS_PROTOBUF      0
#endif


// F.  3D, charts, and visualization modules

// D_ENV_QT_HAS_3D
//   feature: detect if Qt 3D module is available (Qt 5.5+).
#if ( defined(QT_3DCORE_LIB)    ||                                            \
      defined(QT_3DRENDER_LIB)  ||                                            \
      defined(QT_3DINPUT_LIB) )
    #define D_ENV_QT_HAS_3D            1
#else
    #define D_ENV_QT_HAS_3D            0
#endif

// D_ENV_QT_HAS_CHARTS
//   feature: detect if QtCharts module is available (Qt 5.7+ / Qt 6.1+).
#if defined(QT_CHARTS_LIB)
    #define D_ENV_QT_HAS_CHARTS        1
#else
    #define D_ENV_QT_HAS_CHARTS        0
#endif

// D_ENV_QT_HAS_DATAVISUALIZATION
//   feature: detect if Qt Data Visualization module is available.
#if defined(QT_DATAVISUALIZATION_LIB)
    #define D_ENV_QT_HAS_DATAVISUALIZATION 1
#else
    #define D_ENV_QT_HAS_DATAVISUALIZATION 0
#endif

// D_ENV_QT_HAS_GRAPHS
//   feature: detect if Qt Graphs module is available (Qt 6.5+,
// replacement for Charts + Data Visualization).
#if defined(QT_GRAPHS_LIB)
    #define D_ENV_QT_HAS_GRAPHS        1
#else
    #define D_ENV_QT_HAS_GRAPHS        0
#endif

// D_ENV_QT_HAS_SCXML
//   feature: detect if QtScxml module is available (Qt 5.7+).
#if defined(QT_SCXML_LIB)
    #define D_ENV_QT_HAS_SCXML         1
#else
    #define D_ENV_QT_HAS_SCXML         0
#endif

// D_ENV_QT_HAS_STATEMACHINE
//   feature: detect if QtStateMachine module is available.
// in Qt 5, state machine was part of QtCore. in Qt 6, separate module.
#if defined(QT_STATEMACHINE_LIB)
    #define D_ENV_QT_HAS_STATEMACHINE  1
#elif D_ENV_QT_IS_QT5
    // included in QtCore for Qt 5
    #define D_ENV_QT_HAS_STATEMACHINE  1
#else
    #define D_ENV_QT_HAS_STATEMACHINE  0
#endif


// G.  positioning, sensors, and input

// D_ENV_QT_HAS_POSITIONING
//   feature: detect if QtPositioning module is available (Qt 5.2+).
#if defined(QT_POSITIONING_LIB)
    #define D_ENV_QT_HAS_POSITIONING   1
#else
    #define D_ENV_QT_HAS_POSITIONING   0
#endif

// D_ENV_QT_HAS_SENSORS
//   feature: detect if QtSensors module is available (Qt 5.1+).
#if defined(QT_SENSORS_LIB)
    #define D_ENV_QT_HAS_SENSORS       1
#else
    #define D_ENV_QT_HAS_SENSORS       0
#endif

// D_ENV_QT_HAS_GAMEPAD
//   feature: detect if QtGamepad module is available (Qt 5.7+, removed
// in Qt 6).
#if defined(QT_GAMEPAD_LIB)
    #define D_ENV_QT_HAS_GAMEPAD       1
#else
    #define D_ENV_QT_HAS_GAMEPAD       0
#endif


////////////////////////////////////////////////////////////////////////////////
// V.   PLATFORM INTEGRATION
////////////////////////////////////////////////////////////////////////////////

// A.  platform backend detection

// D_ENV_QT_PLATFORM_XCB
//   feature: detect if the XCB (X11) platform plugin is targeted.
#if defined(Q_OS_LINUX)
    #if ( !defined(QT_NO_XCB) &&                                              \
          !defined(D_ENV_QT_FORCE_WAYLAND) )
        #define D_ENV_QT_PLATFORM_XCB   1
    #else
        #define D_ENV_QT_PLATFORM_XCB   0
    #endif
#else
    #define D_ENV_QT_PLATFORM_XCB       0
#endif

// D_ENV_QT_PLATFORM_WAYLAND
//   feature: detect if Wayland platform plugin support is available.
#if defined(QT_WAYLAND_LIB)
    #define D_ENV_QT_PLATFORM_WAYLAND   1
#elif ( defined(Q_OS_LINUX) &&                                                \
        D_ENV_QT_AVAILABLE  &&                                                \
        (D_ENV_QT_VER >= D_ENV_QT_VERSION_5_4) )
    // Wayland support available since Qt 5.4 on Linux
    #define D_ENV_QT_PLATFORM_WAYLAND   1
#else
    #define D_ENV_QT_PLATFORM_WAYLAND   0
#endif

// D_ENV_QT_PLATFORM_WIN32
//   feature: detect if the Windows platform plugin is targeted.
#if defined(Q_OS_WIN)
    #define D_ENV_QT_PLATFORM_WIN32     1
#else
    #define D_ENV_QT_PLATFORM_WIN32     0
#endif

// D_ENV_QT_PLATFORM_COCOA
//   feature: detect if the Cocoa (macOS) platform plugin is targeted.
#if defined(Q_OS_MACOS)
    #define D_ENV_QT_PLATFORM_COCOA     1
#elif defined(Q_OS_MAC)
    // older macro used in Qt 4
    #define D_ENV_QT_PLATFORM_COCOA     1
#else
    #define D_ENV_QT_PLATFORM_COCOA     0
#endif

// D_ENV_QT_PLATFORM_IOS
//   feature: detect if the iOS platform plugin is targeted.
#if defined(Q_OS_IOS)
    #define D_ENV_QT_PLATFORM_IOS       1
#else
    #define D_ENV_QT_PLATFORM_IOS       0
#endif

// D_ENV_QT_PLATFORM_ANDROID
//   feature: detect if the Android platform plugin is targeted.
#if defined(Q_OS_ANDROID)
    #define D_ENV_QT_PLATFORM_ANDROID   1
#else
    #define D_ENV_QT_PLATFORM_ANDROID   0
#endif

// D_ENV_QT_PLATFORM_WASM
//   feature: detect if the WebAssembly platform is targeted (Qt 5.13+).
#if defined(Q_OS_WASM)
    #define D_ENV_QT_PLATFORM_WASM      1
#elif defined(__EMSCRIPTEN__)
    #define D_ENV_QT_PLATFORM_WASM      1
#else
    #define D_ENV_QT_PLATFORM_WASM      0
#endif

// D_ENV_QT_PLATFORM_EGLFS
//   feature: detect if the EGLFS platform plugin is targeted
// (embedded Linux without X11/Wayland).
#if defined(QT_EGLFS_LIB)
    #define D_ENV_QT_PLATFORM_EGLFS     1
#else
    #define D_ENV_QT_PLATFORM_EGLFS     0
#endif

// D_ENV_QT_PLATFORM_INTEGRITY
//   feature: detect if targeting the INTEGRITY RTOS.
#if defined(Q_OS_INTEGRITY)
    #define D_ENV_QT_PLATFORM_INTEGRITY 1
#else
    #define D_ENV_QT_PLATFORM_INTEGRITY 0
#endif

// D_ENV_QT_PLATFORM_QNX
//   feature: detect if targeting QNX.
#if defined(Q_OS_QNX)
    #define D_ENV_QT_PLATFORM_QNX       1
#else
    #define D_ENV_QT_PLATFORM_QNX       0
#endif

// D_ENV_QT_PLATFORM_VXWORKS
//   feature: detect if targeting VxWorks.
#if defined(Q_OS_VXWORKS)
    #define D_ENV_QT_PLATFORM_VXWORKS   1
#else
    #define D_ENV_QT_PLATFORM_VXWORKS   0
#endif


// B.  platform extras modules (Qt 5 only; removed in Qt 6)

// D_ENV_QT_HAS_X11EXTRAS
//   feature: detect if QtX11Extras module is available (Qt 5 only,
// replaced by QNativeInterface in Qt 6).
#if defined(QT_X11EXTRAS_LIB)
    #define D_ENV_QT_HAS_X11EXTRAS     1
#else
    #define D_ENV_QT_HAS_X11EXTRAS     0
#endif

// D_ENV_QT_HAS_WINEXTRAS
//   feature: detect if QtWinExtras module is available (Qt 5 only,
// removed in Qt 6).
#if defined(QT_WINEXTRAS_LIB)
    #define D_ENV_QT_HAS_WINEXTRAS     1
#else
    #define D_ENV_QT_HAS_WINEXTRAS     0
#endif

// D_ENV_QT_HAS_MACEXTRAS
//   feature: detect if QtMacExtras module is available (Qt 5 only,
// removed in Qt 6).
#if defined(QT_MACEXTRAS_LIB)
    #define D_ENV_QT_HAS_MACEXTRAS     1
#else
    #define D_ENV_QT_HAS_MACEXTRAS     0
#endif


////////////////////////////////////////////////////////////////////////////////
// VI.  OPENGL AND RENDERING
////////////////////////////////////////////////////////////////////////////////

// D_ENV_QT_OPENGL_ES
//   feature: detect if Qt is configured for OpenGL ES.
#if defined(QT_OPENGL_ES_2)
    #define D_ENV_QT_OPENGL_ES         1
    #define D_ENV_QT_OPENGL_ES_VER     2
#elif defined(QT_OPENGL_ES_3)
    #define D_ENV_QT_OPENGL_ES         1
    #define D_ENV_QT_OPENGL_ES_VER     3
#elif defined(QT_OPENGL_ES_3_1)
    #define D_ENV_QT_OPENGL_ES         1
    #define D_ENV_QT_OPENGL_ES_VER     31
#elif defined(QT_OPENGL_ES_3_2)
    #define D_ENV_QT_OPENGL_ES         1
    #define D_ENV_QT_OPENGL_ES_VER     32
#elif defined(QT_OPENGL_ES)
    #define D_ENV_QT_OPENGL_ES         1
    #define D_ENV_QT_OPENGL_ES_VER     1
#else
    #define D_ENV_QT_OPENGL_ES         0
    #define D_ENV_QT_OPENGL_ES_VER     0
#endif

// D_ENV_QT_OPENGL_DESKTOP
//   feature: detect if Qt is configured for desktop OpenGL.
#if ( D_ENV_QT_HAS_OPENGL &&                                                  \
      !D_ENV_QT_OPENGL_ES )
    #define D_ENV_QT_OPENGL_DESKTOP    1
#else
    #define D_ENV_QT_OPENGL_DESKTOP    0
#endif

// D_ENV_QT_OPENGL_DYNAMIC
//   feature: detect if Qt uses dynamic OpenGL loading (Windows, Qt 5.4+).
// QT_OPENGL_DYNAMIC enables runtime switching between desktop and ANGLE.
#if defined(QT_OPENGL_DYNAMIC)
    #define D_ENV_QT_OPENGL_DYNAMIC    1
#else
    #define D_ENV_QT_OPENGL_DYNAMIC    0
#endif

// D_ENV_QT_HAS_VULKAN
//   feature: detect if Qt Vulkan support is available (Qt 5.10+).
#if ( D_ENV_QT_AVAILABLE &&                                                   \
      !defined(QT_NO_VULKAN) &&                                               \
      (D_ENV_QT_VER >= D_ENV_QT_VERSION_5_10) )
    #define D_ENV_QT_HAS_VULKAN        1
#else
    #define D_ENV_QT_HAS_VULKAN        0
#endif

// D_ENV_QT_HAS_RHI
//   feature: detect if Qt RHI (Rendering Hardware Interface) is available
// (Qt 6.0+). RHI abstracts Vulkan, Metal, D3D11, D3D12, and OpenGL.
#if D_ENV_QT_IS_QT6
    #define D_ENV_QT_HAS_RHI           1
#else
    #define D_ENV_QT_HAS_RHI           0
#endif


////////////////////////////////////////////////////////////////////////////////
// VII. CORE FEATURE FLAGS
////////////////////////////////////////////////////////////////////////////////

// A.  accessibility

// D_ENV_QT_HAS_ACCESSIBILITY
//   feature: detect if accessibility support is enabled.
#if ( D_ENV_QT_AVAILABLE &&                                                   \
      !defined(QT_NO_ACCESSIBILITY) )
    #define D_ENV_QT_HAS_ACCESSIBILITY 1
#else
    #define D_ENV_QT_HAS_ACCESSIBILITY 0
#endif

// B.  internationalization

// D_ENV_QT_HAS_TRANSLATION
//   feature: detect if Qt translation/i18n support is available.
#if ( D_ENV_QT_AVAILABLE &&                                                   \
      !defined(QT_NO_TRANSLATION) )
    #define D_ENV_QT_HAS_TRANSLATION   1
#else
    #define D_ENV_QT_HAS_TRANSLATION   0
#endif

// D_ENV_QT_HAS_ICU
//   feature: detect if Qt was built with ICU support.
#if defined(QT_USE_ICU)
    #define D_ENV_QT_HAS_ICU           1
#else
    #define D_ENV_QT_HAS_ICU           0
#endif

// C.  threading and concurrency

// D_ENV_QT_HAS_THREAD
//   feature: detect if Qt threading support is enabled.
#if ( D_ENV_QT_AVAILABLE &&                                                   \
      !defined(QT_NO_THREAD) )
    #define D_ENV_QT_HAS_THREAD        1
#else
    #define D_ENV_QT_HAS_THREAD        0
#endif

// D_ENV_QT_HAS_FUTURE
//   feature: detect if QFuture/QPromise are available (Qt 6 expanded).
#if ( D_ENV_QT_IS_QT5 ||                                                      \
      D_ENV_QT_IS_QT6 )
    #define D_ENV_QT_HAS_FUTURE        1
#else
    #define D_ENV_QT_HAS_FUTURE        0
#endif

// D.  file system and I/O

// D_ENV_QT_HAS_FILESYSTEMWATCHER
//   feature: detect if QFileSystemWatcher is available.
#if ( D_ENV_QT_AVAILABLE &&                                                   \
      !defined(QT_NO_FILESYSTEMWATCHER) )
    #define D_ENV_QT_HAS_FILESYSTEMWATCHER 1
#else
    #define D_ENV_QT_HAS_FILESYSTEMWATCHER 0
#endif

// D_ENV_QT_HAS_PROCESS
//   feature: detect if QProcess is available.
#if ( D_ENV_QT_AVAILABLE &&                                                   \
      !defined(QT_NO_PROCESS) )
    #define D_ENV_QT_HAS_PROCESS       1
#else
    #define D_ENV_QT_HAS_PROCESS       0
#endif

// D_ENV_QT_HAS_SHAREDMEMORY
//   feature: detect if QSharedMemory is available.
#if ( D_ENV_QT_AVAILABLE &&                                                   \
      !defined(QT_NO_SHAREDMEMORY) )
    #define D_ENV_QT_HAS_SHAREDMEMORY  1
#else
    #define D_ENV_QT_HAS_SHAREDMEMORY  0
#endif

// E.  SSL and cryptography

// D_ENV_QT_HAS_SSL
//   feature: detect if SSL/TLS support is available in QtNetwork.
#if ( D_ENV_QT_HAS_NETWORK &&                                                 \
      !defined(QT_NO_SSL) )
    #define D_ENV_QT_HAS_SSL           1
#else
    #define D_ENV_QT_HAS_SSL           0
#endif

// D_ENV_QT_HAS_OPENSSL
//   feature: detect if Qt was built against OpenSSL specifically.
#if defined(QT_LINKED_OPENSSL)
    #define D_ENV_QT_HAS_OPENSSL       1
    #define D_ENV_QT_OPENSSL_LINKED    1
#elif defined(QT_RUNTIME_OPENSSL)
    #define D_ENV_QT_HAS_OPENSSL       1
    #define D_ENV_QT_OPENSSL_LINKED    0
#else
    #define D_ENV_QT_HAS_OPENSSL       0
    #define D_ENV_QT_OPENSSL_LINKED    0
#endif

// F.  regular expressions

// D_ENV_QT_HAS_REGEXP
//   feature: detect if QRegExp is available (Qt 4/5, removed in Qt 6).
#if ( (D_ENV_QT_IS_QT4 || D_ENV_QT_IS_QT5) &&                                 \
      !defined(QT_NO_REGEXP) )
    #define D_ENV_QT_HAS_REGEXP        1
#else
    #define D_ENV_QT_HAS_REGEXP        0
#endif

// D_ENV_QT_HAS_REGULAREXPRESSION
//   feature: detect if QRegularExpression is available (Qt 5.0+).
// this is the PCRE2-based replacement for QRegExp.
#if ( D_ENV_QT_AVAILABLE                    &&                                \
      (D_ENV_QT_VER >= D_ENV_QT_VERSION_5)  &&                                \
      !defined(QT_NO_REGULAREXPRESSION) )
    #define D_ENV_QT_HAS_REGULAREXPRESSION 1
#else
    #define D_ENV_QT_HAS_REGULAREXPRESSION 0
#endif


////////////////////////////////////////////////////////////////////////////////
// VIII. C++ STANDARD INTERPLAY
////////////////////////////////////////////////////////////////////////////////

// D_ENV_QT_CPP_MINIMUM_MET
//   feature: evaluates to 1 if the current C++ standard meets the minimum
// required by the detected Qt version.
// Qt 6 requires C++17; Qt 5.7+ requires C++11; Qt 4 requires C++98.
#if D_ENV_QT_IS_QT6
    #ifdef D_ENV_LANG_CPP_STANDARD
        #define D_ENV_QT_CPP_MINIMUM_MET                                      \
            (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP17)
    #else
        #define D_ENV_QT_CPP_MINIMUM_MET 0
    #endif
#elif ( D_ENV_QT_IS_QT5 &&                                                    \
        (D_ENV_QT_VER >= D_ENV_QT_VERSION_5_7) )
    #ifdef D_ENV_LANG_CPP_STANDARD
        #define D_ENV_QT_CPP_MINIMUM_MET                                      \
            (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP11)
    #else
        #define D_ENV_QT_CPP_MINIMUM_MET 0
    #endif
#elif ( D_ENV_QT_IS_QT5 ||                                                    \
        D_ENV_QT_IS_QT4 )
    #ifdef D_ENV_LANG_CPP_STANDARD
        #define D_ENV_QT_CPP_MINIMUM_MET                                      \
            (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP98)
    #else
        #define D_ENV_QT_CPP_MINIMUM_MET 0
    #endif
#else
    #define D_ENV_QT_CPP_MINIMUM_MET    0
#endif

// D_ENV_QT_HAS_CPP17_API
//   feature: evaluates to 1 if Qt C++17-era APIs are available.
// Qt 5.15+ began offering opt-in C++17 APIs; Qt 6 requires them.
#if ( D_ENV_QT_IS_QT6                                                ||       \
      (D_ENV_QT_IS_QT5 && (D_ENV_QT_VER >= D_ENV_QT_VERSION_5_15)) )
    #ifdef D_ENV_LANG_CPP_STANDARD
        #define D_ENV_QT_HAS_CPP17_API                                        \
            (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP17)
    #else
        #define D_ENV_QT_HAS_CPP17_API  0
    #endif
#else
    #define D_ENV_QT_HAS_CPP17_API      0
#endif

// D_ENV_QT_HAS_CPP20_API
//   feature: evaluates to 1 if Qt C++20-era APIs are available.
// Qt 6.4+ introduced opt-in C++20 features (QProperty improvements, etc.).
#if ( D_ENV_QT_IS_QT6 &&                                                      \
      (D_ENV_QT_VER >= D_ENV_QT_VERSION_6_4) )
    #ifdef D_ENV_LANG_CPP_STANDARD
        #define D_ENV_QT_HAS_CPP20_API                                        \
            (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP20)
    #else
        #define D_ENV_QT_HAS_CPP20_API  0
    #endif
#else
    #define D_ENV_QT_HAS_CPP20_API      0
#endif


////////////////////////////////////////////////////////////////////////////////
// IX.  DEPRECATION AND MIGRATION
////////////////////////////////////////////////////////////////////////////////

// D_ENV_QT_DISABLE_DEPRECATED_BEFORE
//   feature: detect the Qt deprecation cutoff version if configured.
// QT_DISABLE_DEPRECATED_BEFORE hides APIs deprecated before that version.
#if defined(QT_DISABLE_DEPRECATED_BEFORE)
    #define D_ENV_QT_DEPRECATION_CUTOFF QT_DISABLE_DEPRECATED_BEFORE
#else
    #define D_ENV_QT_DEPRECATION_CUTOFF 0
#endif

// D_ENV_QT_NO_DEPRECATED_WARNINGS
//   feature: detect if deprecated-API warnings have been silenced.
#if defined(QT_NO_DEPRECATED_WARNINGS)
    #define D_ENV_QT_NO_DEPRECATED_WARNINGS 1
#else
    #define D_ENV_QT_NO_DEPRECATED_WARNINGS 0
#endif

// D_ENV_QT_HAS_QT5_COMPAT
//   feature: detect if the Qt5Compat module is available (Qt 6 only).
// provides classes removed from Qt 6 for migration purposes.
#if defined(QT_CORE5COMPAT_LIB)
    #define D_ENV_QT_HAS_QT5_COMPAT    1
#else
    #define D_ENV_QT_HAS_QT5_COMPAT    0
#endif


////////////////////////////////////////////////////////////////////////////////
// X.   RUNTIME DETECTION FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

// d_env_qt_get_runtime_version
//   function: returns the runtime Qt version string.
// note: this may differ from compile-time if dynamically linked against
// a different Qt version than what was used during compilation.
//   returns: version string from qVersion(), or "N/A" if Qt is not
// available at runtime.
const char* d_env_qt_get_runtime_version(void);

// d_env_qt_get_compile_version
//   function: returns the compile-time Qt version string.
//   returns: QT_VERSION_STR, or "None" if Qt was not detected.
const char* d_env_qt_get_compile_version(void);

// d_env_qt_runtime_matches_compile
//   function: checks if runtime Qt version matches compile-time version.
//   returns: 1 if major and minor versions match, 0 otherwise.
int d_env_qt_runtime_matches_compile(void);

// d_env_qt_has_module
//   function: attempts runtime detection of a Qt module by name.
//   params:
//     module_name - module name (e.g. "QtWidgets", "QtQml")
//   returns: 1 if the module's shared library is loadable, 0 otherwise.
int d_env_qt_has_module(const char* module_name);

// d_env_qt_print_info
//   function: prints detailed information about detected Qt environment.
// includes version, modules, platform, build configuration, and feature
// flags. useful for debugging and system capability reporting.
void d_env_qt_print_info(void);

#ifdef __cplusplus
}
#endif


////////////////////////////////////////////////////////////////////////////////
// XI.  CONVENIENCE MACROS
////////////////////////////////////////////////////////////////////////////////

// D_ENV_HAS_QT
//   macro: evaluates to 1 if any version of Qt is detected, 0 otherwise.
#define D_ENV_HAS_QT()                                                        \
    (D_ENV_QT_AVAILABLE)

// D_ENV_QT_AT_LEAST
//   macro: evaluates to 1 if the detected Qt version is at least the
// specified major, minor, patch version.
#define D_ENV_QT_AT_LEAST(major, minor, patch)                                \
    ( D_ENV_QT_AVAILABLE &&                                                   \
      (D_ENV_QT_VER >= QT_VERSION_CHECK(major, minor, patch)) )

// D_ENV_QT_AT_LEAST_HEX
//   macro: evaluates to 1 if the detected Qt version is at least the
// specified hex version constant (e.g. D_ENV_QT_VERSION_5_12).
#define D_ENV_QT_AT_LEAST_HEX(hex_version)                                    \
    ( D_ENV_QT_AVAILABLE &&                                                   \
      (D_ENV_QT_VER >= (hex_version)) )

// D_ENV_QT_VERSION_CHECK_COMPAT
//   macro: compatibility shim for QT_VERSION_CHECK on pre-Qt environments.
// if QT_VERSION_CHECK is not defined (no Qt headers), provide an equivalent.
#ifndef QT_VERSION_CHECK
    #define QT_VERSION_CHECK(major, minor, patch)                             \
        ((major << 16) | (minor << 8) | (patch))
#endif

// D_ENV_QT_IS_SERIES
//   macro: evaluates to 1 if the detected Qt major version matches.
#define D_ENV_QT_IS_SERIES(major)                                             \
    ( D_ENV_QT_AVAILABLE &&                                                   \
      (D_ENV_QT_VER_MAJOR == (major)) )

// D_ENV_QT_IS_LTS
//   macro: evaluates to 1 if the detected Qt version is a known LTS
// release (Qt 5.6, 5.9, 5.12, 5.15, 6.2, 6.5, 6.8).
#define D_ENV_QT_IS_LTS()                                                     \
    ( ( D_ENV_QT_IS_QT5 &&                                                    \
        ( (D_ENV_QT_VER_MINOR == 6)  ||                                       \
          (D_ENV_QT_VER_MINOR == 9)  ||                                       \
          (D_ENV_QT_VER_MINOR == 12) ||                                       \
          (D_ENV_QT_VER_MINOR == 15) ) )                                      \
      ||                                                                      \
      ( D_ENV_QT_IS_QT6 &&                                                    \
        ( (D_ENV_QT_VER_MINOR == 2)  ||                                       \
          (D_ENV_QT_VER_MINOR == 5)  ||                                       \
          (D_ENV_QT_VER_MINOR == 8) ) ) )

// D_ENV_QT_HAS_MODERN_CONNECT
//   macro: evaluates to 1 if the Qt 5+ type-safe signal/slot connect
// syntax is available.
#define D_ENV_QT_HAS_MODERN_CONNECT()                                         \
    ( D_ENV_QT_IS_QT5 ||                                                      \
      D_ENV_QT_IS_QT6 )

// D_ENV_QT_HAS_QPROPERTY
//   macro: evaluates to 1 if the new QProperty binding system is available
// (Qt 6.0+).
#define D_ENV_QT_HAS_QPROPERTY()                                              \
    ( D_ENV_QT_IS_QT6 )


#endif  // DJINTERP_ENV_QT_
