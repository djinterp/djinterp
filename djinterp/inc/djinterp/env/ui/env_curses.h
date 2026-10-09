/*******************************************************************************
* djinterp [env]                                                    env_curses.h
*
* djinterp curses environment detection.
*   Compile-time and runtime detection of curses libraries across platforms:
*     - Linux: ncurses (standard) and ncursesw (wide character)
*     - Windows: PDCurses, PDCursesMod, or ncurses (via WSL, Cygwin, MSYS2)
*     - macOS and the BSDs: ncurses (system) and ncursesw
*     - other Unix: System V curses or ncurses
*   It detects the library's presence and type, wide-character support, color
* support, and extended features such as mouse and resize handling. Detection
* is at compile time where possible, from the macros a curses header defines,
* with runtime functions for capabilities that only the running library can
* report.
*
*
* path:      /inc/djinterp/env/ui/env_curses.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.12.26
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPE AND FEATURE IDENTIFIERS
    ----------------------------
    1.  Library types
         1.  D_ENV_CURSES_TYPE_*
              1.  D_ENV_CURSES_TYPE_NONE
              2.  D_ENV_CURSES_TYPE_NCURSES
              3.  D_ENV_CURSES_TYPE_NCURSESW
              4.  D_ENV_CURSES_TYPE_PDCURSES
              5.  D_ENV_CURSES_TYPE_PDCURSESMOD
              6.  D_ENV_CURSES_TYPE_SYSV
              7.  D_ENV_CURSES_TYPE_BSD_CURSES
              8.  D_ENV_CURSES_TYPE_UNKNOWN
    2.  Feature flags
         1.  D_ENV_CURSES_FEAT_*
              1.  D_ENV_CURSES_FEAT_COLOR
              2.  D_ENV_CURSES_FEAT_WIDE
              3.  D_ENV_CURSES_FEAT_MOUSE
              4.  D_ENV_CURSES_FEAT_RESIZE
              5.  D_ENV_CURSES_FEAT_EXTENDED
2.  COMPILE-TIME DETECTION
    ----------------------
    1.  Library detection
         1.  D_ENV_CURSES_TYPE
         2.  D_ENV_CURSES_FEATURES
         3.  D_ENV_CURSES_NAME
         4.  D_ENV_CURSES_AVAILABLE
3.  PLATFORM GUIDANCE
    -----------------
    1.  Expected package
         1.  D_ENV_CURSES_EXPECTED_PACKAGE / D_ENV_CURSES_EXPECTED_TYPE
4.  RUNTIME DETECTION
    -----------------
    1.  Runtime queries
5.  CONVENIENCE MACROS
    ------------------
    1.  Combined predicates
         1.  D_ENV_HAS_CURSES
         2.  D_ENV_HAS_NCURSES
         3.  D_ENV_HAS_PDCURSES
         4.  D_ENV_CURSES_IS_TYPE
         5.  D_ENV_CURSES_HAS_FEAT
*/

#ifndef DJINTERP_ENV_UI_ENV_CURSES_H
#define DJINTERP_ENV_UI_ENV_CURSES_H 1

// djinterp
#include "../env.h"  // D_ENV_OS_ID, D_ENV_IS_OS_*, D_ENV_LANG_USING_CPP

//==============================================================================
// 1.  TYPE AND FEATURE IDENTIFIERS
//==============================================================================


// 1.1    Library types
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_CURSES_TYPE_*
//   constant: bit identifiers for D_ENV_CURSES_TYPE, one per curses family.

// 1.1.1.1
// D_ENV_CURSES_TYPE_NONE
//   constant: no curses available.
#define D_ENV_CURSES_TYPE_NONE        0x00000000

// 1.1.1.2
// D_ENV_CURSES_TYPE_NCURSES
//   constant: standard ncurses.
#define D_ENV_CURSES_TYPE_NCURSES     0x00000001

// 1.1.1.3
// D_ENV_CURSES_TYPE_NCURSESW
//   constant: wide-char ncurses.
#define D_ENV_CURSES_TYPE_NCURSESW    0x00000002

// 1.1.1.4
// D_ENV_CURSES_TYPE_PDCURSES
//   constant: PDCurses (Windows).
#define D_ENV_CURSES_TYPE_PDCURSES    0x00000004

// 1.1.1.5
// D_ENV_CURSES_TYPE_PDCURSESMOD
//   constant: PDCursesMod (Windows).
#define D_ENV_CURSES_TYPE_PDCURSESMOD 0x00000008

// 1.1.1.6
// D_ENV_CURSES_TYPE_SYSV
//   constant: system V curses.
#define D_ENV_CURSES_TYPE_SYSV        0x00000010

// 1.1.1.7
// D_ENV_CURSES_TYPE_BSD_CURSES
//   constant: BSD curses.
#define D_ENV_CURSES_TYPE_BSD_CURSES  0x00000020

// 1.1.1.8
// D_ENV_CURSES_TYPE_UNKNOWN
//   constant: unknown curses variant.
#define D_ENV_CURSES_TYPE_UNKNOWN     0x80000000

// 1.2    Feature flags
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_CURSES_FEAT_*
//   constant: bit flags for D_ENV_CURSES_FEATURES.

// 1.2.1.1
// D_ENV_CURSES_FEAT_COLOR
//   constant: color support.
#define D_ENV_CURSES_FEAT_COLOR       0x00000100

// 1.2.1.2
// D_ENV_CURSES_FEAT_WIDE
//   constant: wide character support.
#define D_ENV_CURSES_FEAT_WIDE        0x00000200

// 1.2.1.3
// D_ENV_CURSES_FEAT_MOUSE
//   constant: mouse support.
#define D_ENV_CURSES_FEAT_MOUSE       0x00000400

// 1.2.1.4
// D_ENV_CURSES_FEAT_RESIZE
//   constant: window resize support.
#define D_ENV_CURSES_FEAT_RESIZE      0x00000800

// 1.2.1.5
// D_ENV_CURSES_FEAT_EXTENDED
//   constant: extended features.
#define D_ENV_CURSES_FEAT_EXTENDED    0x00001000


//==============================================================================
// 2.  COMPILE-TIME DETECTION
//==============================================================================
// Reads the macros a curses header defines -- NCURSES_VERSION for ncurses,
// PDC_VER_MAJOR or __PDCURSES__ for PDCurses -- so a curses header must be
// included before this one; otherwise curses reads as not detected.


// 2.1    Library detection
//------------------------------------------------------------------------------
// Every result here is guarded, so a value the build or a test defines first
// stands, and each derives from the one before it: pinning D_ENV_CURSES_TYPE
// carries its features, name and availability with it (decision 52 of the
// register; detection used to #undef a pre-defined type and features, and
// the name and availability were unguarded).

// 2.1.1
// D_ENV_CURSES_TYPE
//   feature: the detected library, a D_ENV_CURSES_TYPE_* value: ncurses from
// NCURSES_VERSION, wide when NCURSES_WIDECHAR is 1 (#51); PDCursesMod from its
// own __PDCURSESMOD__ or a PDC_VER_MAJOR of 4 and up, and PDCurses from
// PDC_VER_MAJOR or __PDCURSES__ otherwise (#53); D_ENV_CURSES_TYPE_NONE with no
// curses header.
#ifndef D_ENV_CURSES_TYPE
    #if defined(NCURSES_VERSION)
        #if ( ( (defined(NCURSES_WIDECHAR)) &&                                 \
                (NCURSES_WIDECHAR) )                ||                         \
              ( (!defined(NCURSES_WIDECHAR)) &&                                \
                (defined(_XOPEN_SOURCE_EXTENDED)) ) )
            #define D_ENV_CURSES_TYPE D_ENV_CURSES_TYPE_NCURSESW
        #else
            #define D_ENV_CURSES_TYPE D_ENV_CURSES_TYPE_NCURSES
        #endif
    #elif ( (defined(__PDCURSESMOD__))     ||                                  \
            ( (defined(PDC_VER_MAJOR)) &&                                      \
              (PDC_VER_MAJOR >= 4) ) )
        #define D_ENV_CURSES_TYPE D_ENV_CURSES_TYPE_PDCURSESMOD
    #elif ( (defined(PDC_VER_MAJOR)) ||                                        \
            (defined(__PDCURSES__)) )
        #define D_ENV_CURSES_TYPE D_ENV_CURSES_TYPE_PDCURSES
    #else
        #define D_ENV_CURSES_TYPE D_ENV_CURSES_TYPE_NONE
    #endif
#endif  // D_ENV_CURSES_TYPE

// 2.1.2
// D_ENV_CURSES_FEATURES
//   feature: the D_ENV_CURSES_FEAT_* flags of the type: ncurses has color,
// mouse, resize and the extended API, and wide characters as ncursesw;
// PDCurses and PDCursesMod have color and mouse, resize from PDC_VER_MAJOR's
// versions on, and wide characters where built with PDC_WIDE; anything else,
// none.
#ifndef D_ENV_CURSES_FEATURES
    #if (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_NCURSESW)
        #define D_ENV_CURSES_FEATURES (D_ENV_CURSES_FEAT_WIDE     |            \
                                       D_ENV_CURSES_FEAT_COLOR    |            \
                                       D_ENV_CURSES_FEAT_MOUSE    |            \
                                       D_ENV_CURSES_FEAT_RESIZE   |            \
                                       D_ENV_CURSES_FEAT_EXTENDED)
    #elif (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_NCURSES)
        #define D_ENV_CURSES_FEATURES (D_ENV_CURSES_FEAT_COLOR    |            \
                                       D_ENV_CURSES_FEAT_MOUSE    |            \
                                       D_ENV_CURSES_FEAT_RESIZE   |            \
                                       D_ENV_CURSES_FEAT_EXTENDED)
    #elif ( ( (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_PDCURSES)    ||          \
              (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_PDCURSESMOD) ) &&        \
            (defined(PDC_VER_MAJOR))                                &&         \
            (defined(PDC_WIDE)) )
        #define D_ENV_CURSES_FEATURES (D_ENV_CURSES_FEAT_WIDE     |            \
                                       D_ENV_CURSES_FEAT_COLOR    |            \
                                       D_ENV_CURSES_FEAT_MOUSE    |            \
                                       D_ENV_CURSES_FEAT_RESIZE)
    #elif ( ( (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_PDCURSES)    ||          \
              (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_PDCURSESMOD) ) &&        \
            (defined(PDC_VER_MAJOR)) )
        #define D_ENV_CURSES_FEATURES (D_ENV_CURSES_FEAT_COLOR    |            \
                                       D_ENV_CURSES_FEAT_MOUSE    |            \
                                       D_ENV_CURSES_FEAT_RESIZE)
    #elif ( (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_PDCURSES) ||               \
            (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_PDCURSESMOD) )
        // a PDCurses older than PDC_VER_MAJOR
        #define D_ENV_CURSES_FEATURES (D_ENV_CURSES_FEAT_COLOR    |            \
                                       D_ENV_CURSES_FEAT_MOUSE)
    #else
        #define D_ENV_CURSES_FEATURES 0
    #endif
#endif  // D_ENV_CURSES_FEATURES

// 2.1.3
// D_ENV_CURSES_NAME
//   feature: the type's name, "Unknown" for D_ENV_CURSES_TYPE_NONE as before.
#ifndef D_ENV_CURSES_NAME
    #if ( (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_NCURSES) ||                  \
          (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_NCURSESW) )
        #define D_ENV_CURSES_NAME "ncurses"
    #elif (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_PDCURSESMOD)
        #define D_ENV_CURSES_NAME "PDCursesMod"
    #elif (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_PDCURSES)
        #define D_ENV_CURSES_NAME "PDCurses"
    #elif (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_SYSV)
        #define D_ENV_CURSES_NAME "System V curses"
    #elif (D_ENV_CURSES_TYPE == D_ENV_CURSES_TYPE_BSD_CURSES)
        #define D_ENV_CURSES_NAME "BSD curses"
    #else
        #define D_ENV_CURSES_NAME "Unknown"
    #endif
#endif  // D_ENV_CURSES_NAME

// 2.1.4
// D_ENV_CURSES_AVAILABLE
//   feature: 1 when a curses library was detected or pinned, 0 otherwise.
#ifndef D_ENV_CURSES_AVAILABLE
    #if (D_ENV_CURSES_TYPE != D_ENV_CURSES_TYPE_NONE)
        #define D_ENV_CURSES_AVAILABLE 1
    #else
        #define D_ENV_CURSES_AVAILABLE 0
    #endif
#endif  // D_ENV_CURSES_AVAILABLE

//==============================================================================
// 3.  PLATFORM GUIDANCE
//==============================================================================
// When no curses library was detected, names the package that usually
// provides one on the current platform.


// 3.1    Expected package
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_CURSES_EXPECTED_PACKAGE / D_ENV_CURSES_EXPECTED_TYPE
//   constant: where no curses was detected, the package that usually provides
// one and the D_ENV_CURSES_TYPE_* it would be; undefined where curses was
// detected, or on a platform with no guidance.
// D_INTERNAL_ENV_CURSES_PLATFORM
//   macro: which guidance applies where no curses was detected: 1, Linux; 2,
// Windows; 3, macOS and the BSDs; 4, other Unix; 0, none. One chain, so each
// platform gets exactly one answer: Linux, whose identifier sits in the Unix
// block, is matched before the generic Unix case.
#if D_ENV_CURSES_AVAILABLE
    #define D_INTERNAL_ENV_CURSES_PLATFORM 0
#elif D_ENV_IS_OS_LINUX(D_ENV_OS_ID)
    #define D_INTERNAL_ENV_CURSES_PLATFORM 1
#elif D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #define D_INTERNAL_ENV_CURSES_PLATFORM 2
#elif ( (D_ENV_IS_OS_MACOS(D_ENV_OS_ID)) ||                                    \
        (D_ENV_IS_OS_BSD(D_ENV_OS_ID)) )
    #define D_INTERNAL_ENV_CURSES_PLATFORM 3
#elif D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID)
    #define D_INTERNAL_ENV_CURSES_PLATFORM 4
#else
    #define D_INTERNAL_ENV_CURSES_PLATFORM 0
#endif

// each result is guarded, as section 2's are (decision 52 of the register)
#ifndef D_ENV_CURSES_EXPECTED_PACKAGE
    #if (D_INTERNAL_ENV_CURSES_PLATFORM == 1)
        // Linux: ncurses is standard. Debian / Ubuntu ship it as
        // libncurses-dev or libncursesw5-dev, and Red Hat / Fedora as
        // ncurses-devel.
        #define D_ENV_CURSES_EXPECTED_PACKAGE "ncurses-dev or ncurses-devel"
    #elif (D_INTERNAL_ENV_CURSES_PLATFORM == 2)
        // Windows: curses must be installed explicitly.
        #define D_ENV_CURSES_EXPECTED_PACKAGE                                  \
            "PDCurses, PDCursesMod, or ncurses (via WSL/Cygwin/MSYS2)"
    #elif (D_INTERNAL_ENV_CURSES_PLATFORM == 3)
        // macOS and the BSDs: ncurses is usually part of the base system.
        #define D_ENV_CURSES_EXPECTED_PACKAGE "ncurses (system)"
    #elif (D_INTERNAL_ENV_CURSES_PLATFORM == 4)
        // other Unix systems: System V curses or ncurses.
        #define D_ENV_CURSES_EXPECTED_PACKAGE "ncurses or system curses"
    #endif
#endif  // D_ENV_CURSES_EXPECTED_PACKAGE

#ifndef D_ENV_CURSES_EXPECTED_TYPE
    #if ( (D_INTERNAL_ENV_CURSES_PLATFORM == 1) ||                             \
          (D_INTERNAL_ENV_CURSES_PLATFORM == 3) )
        #define D_ENV_CURSES_EXPECTED_TYPE    D_ENV_CURSES_TYPE_NCURSES
    #elif (D_INTERNAL_ENV_CURSES_PLATFORM == 2)
        #define D_ENV_CURSES_EXPECTED_TYPE    D_ENV_CURSES_TYPE_PDCURSES
    #elif (D_INTERNAL_ENV_CURSES_PLATFORM == 4)
        #define D_ENV_CURSES_EXPECTED_TYPE    D_ENV_CURSES_TYPE_SYSV
    #endif
#endif  // D_ENV_CURSES_EXPECTED_TYPE


//==============================================================================
// 4.  RUNTIME DETECTION
//==============================================================================
// Declared here and defined in the curses implementation, for capabilities
// that compile-time detection cannot establish.


// 4.1    Runtime queries
//------------------------------------------------------------------------------
//   C linkage for C++ callers. The env headers sit below djinterp.h, so the
// D_EXTERN_C_BEGIN / D_EXTERN_C_END pair is not available here.
#if D_ENV_LANG_USING_CPP
    extern "C" {
#endif

/**
 * @brief Detects the curses library at runtime, by checking for library
 *        symbols and features.
 *
 * @return the D_ENV_CURSES_TYPE_* flags found, or `0` if none.
 */
int         d_env_curses_detect(void);
/**
 * @brief Returns the detected curses library's name.
 *
 * @return a description of the curses variant, or "None" if unavailable.
 */
const char* d_env_curses_get_name(void);
/**
 * @brief Returns the curses library's version.
 *
 * @return the version string, or "Unknown" if it cannot be determined.
 */
const char* d_env_curses_get_version(void);
/**
 * @brief Tests whether a curses feature is available.
 *
 * @param[in] _feature  the D_ENV_CURSES_FEAT_* flag to test.
 * @return `1` if the feature is available, `0` otherwise.
 */
int         d_env_curses_has_feature(int _feature);
/**
 * @brief Tests whether the curses library supports colors.
 *
 * @return `1` if color is supported, `0` otherwise.
 */
int         d_env_curses_supports_color(void);
/**
 * @brief Tests whether the curses library supports wide characters.
 *
 * @return `1` if wide characters are supported, `0` otherwise.
 */
int         d_env_curses_supports_wide(void);
/**
 * @brief Tests whether the curses library supports mouse events.
 *
 * @return `1` if mouse events are supported, `0` otherwise.
 */
int         d_env_curses_supports_mouse(void);
/**
 * @brief Prints detailed information about the detected curses library, for
 *        debugging and capability reporting.
 */
void        d_env_curses_print_info(void);

#if D_ENV_LANG_USING_CPP
    }
#endif


//==============================================================================
// 5.  CONVENIENCE MACROS
//==============================================================================


// 5.1    Combined predicates
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_HAS_CURSES
//   macro: 1 if a curses library is detected at compile time or at runtime,
// 0 otherwise. It calls d_env_curses_detect(), so it cannot be used in #if.
#define D_ENV_HAS_CURSES()                                                     \
    ( (D_ENV_CURSES_AVAILABLE) ||                                              \
      (d_env_curses_detect() != D_ENV_CURSES_TYPE_NONE) )

// 5.1.2
// D_ENV_HAS_NCURSES
//   macro: evaluates to 1 if ncurses (narrow or wide) is detected
#define D_ENV_HAS_NCURSES()                                                    \
    ((D_ENV_CURSES_TYPE & (D_ENV_CURSES_TYPE_NCURSES |                         \
                           D_ENV_CURSES_TYPE_NCURSESW)) != 0)

// 5.1.3
// D_ENV_HAS_PDCURSES
//   macro: evaluates to 1 if PDCurses variant is detected
#define D_ENV_HAS_PDCURSES()                                                   \
    ((D_ENV_CURSES_TYPE & (D_ENV_CURSES_TYPE_PDCURSES |                        \
                           D_ENV_CURSES_TYPE_PDCURSESMOD)) != 0)

// 5.1.4
// D_ENV_CURSES_IS_TYPE
//   macro: checks if detected curses matches a specific type
#define D_ENV_CURSES_IS_TYPE(TYPE)                                             \
    ((D_ENV_CURSES_TYPE & (TYPE)) != 0)

// 5.1.5
// D_ENV_CURSES_HAS_FEAT
//   macro: checks if a specific feature is available at compile-time
#define D_ENV_CURSES_HAS_FEAT(FEAT)                                            \
    ((D_ENV_CURSES_FEATURES & (FEAT)) != 0)

#endif  // DJINTERP_ENV_UI_ENV_CURSES_H
