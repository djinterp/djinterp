/******************************************************************************
* djinterp [core]                                                env_curses.h
* 
*   djinterp curses detection header:
* This header provides compile-time and runtime detection of curses libraries
* across different platforms:
*   - Linux: ncurses (standard), ncursesw (wide char)
*   - Windows: PDCurses, PDCursesMod, ncurses (via WSL/Cygwin/MSYS2)
*   - macOS/BSD: ncurses (system), ncursesw
*   - Other Unix: System V curses, ncurses
*
* The header detects:
*   - Curses library presence and type
*   - Wide character support
*   - Color support capabilities
*   - Extended features (mouse, resize, etc.)
* 
* Detection is performed at compile-time where possible, with runtime
* feature detection functions provided for dynamic capability checking.
*
* 
* path:      \inc\core\env\env_curses.h                                           
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2024.12.26
******************************************************************************/

#ifndef DJINTERP_ENV_CURSES_
#define DJINTERP_ENV_CURSES_ 1

#include "./env.h"

// =============================================================================
// I.   CURSES LIBRARY TYPE FLAGS
// =============================================================================

// Curses library type identifiers
#define D_ENV_CURSES_TYPE_NONE          0x00000000  // no curses available
#define D_ENV_CURSES_TYPE_NCURSES       0x00000001  // standard ncurses
#define D_ENV_CURSES_TYPE_NCURSESW      0x00000002  // wide-char ncurses
#define D_ENV_CURSES_TYPE_PDCURSES      0x00000004  // PDCurses (Windows)
#define D_ENV_CURSES_TYPE_PDCURSESMOD   0x00000008  // PDCursesMod (Windows)
#define D_ENV_CURSES_TYPE_SYSV          0x00000010  // system V curses
#define D_ENV_CURSES_TYPE_BSD_CURSES    0x00000020  // BSD curses
#define D_ENV_CURSES_TYPE_UNKNOWN       0x80000000  // unknown curses variant

// Curses feature flags
#define D_ENV_CURSES_FEAT_COLOR         0x00000100  // color support
#define D_ENV_CURSES_FEAT_WIDE          0x00000200  // wide character support
#define D_ENV_CURSES_FEAT_MOUSE         0x00000400  // mouse support
#define D_ENV_CURSES_FEAT_RESIZE        0x00000800  // window resize support
#define D_ENV_CURSES_FEAT_EXTENDED      0x00001000  // extended features

// =============================================================================
// II.  COMPILE-TIME CURSES DETECTION
// =============================================================================

// Default: assume no curses
#ifndef D_ENV_CURSES_TYPE
    #define D_ENV_CURSES_TYPE D_ENV_CURSES_TYPE_NONE
#endif

#ifndef D_ENV_CURSES_FEATURES
    #define D_ENV_CURSES_FEATURES 0
#endif

// attempt to detect curses at compile-time
#if defined(NCURSES_VERSION)
    // ncurses is present
    #undef  D_ENV_CURSES_TYPE
    
    #if ( defined(_XOPEN_SOURCE_EXTENDED) ||  \
          defined(NCURSES_WIDECHAR) )
        // Wide character support detected
        #define D_ENV_CURSES_TYPE D_ENV_CURSES_TYPE_NCURSESW
        #undef  D_ENV_CURSES_FEATURES
        #define D_ENV_CURSES_FEATURES (D_ENV_CURSES_FEAT_WIDE   |  \
                                       D_ENV_CURSES_FEAT_COLOR  |  \
                                       D_ENV_CURSES_FEAT_MOUSE  |  \
                                       D_ENV_CURSES_FEAT_RESIZE |  \
                                       D_ENV_CURSES_FEAT_EXTENDED)
    #else
        #define D_ENV_CURSES_TYPE D_ENV_CURSES_TYPE_NCURSES
        #undef  D_ENV_CURSES_FEATURES
        #define D_ENV_CURSES_FEATURES (D_ENV_CURSES_FEAT_COLOR  |  \
                                       D_ENV_CURSES_FEAT_MOUSE  |  \
                                       D_ENV_CURSES_FEAT_RESIZE |  \
                                       D_ENV_CURSES_FEAT_EXTENDED)
    #endif
    
    #define D_ENV_CURSES_NAME "ncurses"
    #define D_ENV_CURSES_AVAILABLE 1

#elif defined(PDC_VER_MAJOR)
    // PDCurses or PDCursesMod detected
    #undef  D_ENV_CURSES_TYPE
    
    #if defined(PDC_WIDE)
        #define D_ENV_CURSES_TYPE D_ENV_CURSES_TYPE_PDCURSESMOD
        #undef  D_ENV_CURSES_FEATURES
        #define D_ENV_CURSES_FEATURES (D_ENV_CURSES_FEAT_WIDE | \
                                       D_ENV_CURSES_FEAT_COLOR | \
                                       D_ENV_CURSES_FEAT_MOUSE | \
                                       D_ENV_CURSES_FEAT_RESIZE)
        #define D_ENV_CURSES_NAME "PDCursesMod"
    #else
        #define D_ENV_CURSES_TYPE D_ENV_CURSES_TYPE_PDCURSES
        #undef  D_ENV_CURSES_FEATURES
        #define D_ENV_CURSES_FEATURES (D_ENV_CURSES_FEAT_COLOR | \
                                       D_ENV_CURSES_FEAT_MOUSE | \
                                       D_ENV_CURSES_FEAT_RESIZE)
        #define D_ENV_CURSES_NAME "PDCurses"
    #endif
    
    #define D_ENV_CURSES_AVAILABLE 1

#elif defined(__PDCURSES__)
    // Older PDCurses version
    #undef  D_ENV_CURSES_TYPE
    #define D_ENV_CURSES_TYPE D_ENV_CURSES_TYPE_PDCURSES
    #undef  D_ENV_CURSES_FEATURES
    #define D_ENV_CURSES_FEATURES (D_ENV_CURSES_FEAT_COLOR | \
                                   D_ENV_CURSES_FEAT_MOUSE)
    #define D_ENV_CURSES_NAME "PDCurses"
    #define D_ENV_CURSES_AVAILABLE 1

#else
    // No compile-time detection - may still be available
    #define D_ENV_CURSES_NAME "Unknown"
    #define D_ENV_CURSES_AVAILABLE 0
#endif

// =============================================================================
// III. PLATFORM-SPECIFIC DETECTION GUIDANCE
// =============================================================================

// Linux: ncurses is standard
#if D_ENV_IS_OS_LINUX(D_ENV_OS_ID) && !D_ENV_CURSES_AVAILABLE
    // On Linux, curses is typically available as ncurses
    // Check for: libncurses-dev, libncursesw5-dev (Debian/Ubuntu)
    //            ncurses-devel (Red Hat/Fedora)
    #define D_ENV_CURSES_EXPECTED_PACKAGE "ncurses-dev or ncurses-devel"
    #define D_ENV_CURSES_EXPECTED_TYPE D_ENV_CURSES_TYPE_NCURSES
#endif

// Windows: requires external library
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) && !D_ENV_CURSES_AVAILABLE
    // On Windows, curses must be explicitly installed
    // Options: PDCurses, PDCursesMod, or ncurses via WSL/Cygwin/MSYS2
    #define D_ENV_CURSES_EXPECTED_PACKAGE "PDCurses, PDCursesMod, or ncurses (via WSL/Cygwin/MSYS2)"
    #define D_ENV_CURSES_EXPECTED_TYPE D_ENV_CURSES_TYPE_PDCURSES
#endif

// macOS/BSD: ncurses is typically available
#if (D_ENV_IS_OS_MACOS(D_ENV_OS_ID) || D_ENV_IS_OS_BSD(D_ENV_OS_ID)) && !D_ENV_CURSES_AVAILABLE
    // On macOS/BSD, ncurses is usually part of the base system
    #define D_ENV_CURSES_EXPECTED_PACKAGE "ncurses (system)"
    #define D_ENV_CURSES_EXPECTED_TYPE D_ENV_CURSES_TYPE_NCURSES
#endif

// Other Unix systems
#if D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_ID) && !D_ENV_CURSES_AVAILABLE
    // On other Unix systems, could be System V curses or ncurses
    #define D_ENV_CURSES_EXPECTED_PACKAGE "ncurses or system curses"
    #define D_ENV_CURSES_EXPECTED_TYPE D_ENV_CURSES_TYPE_SYSV
#endif

// =============================================================================
// IV.  RUNTIME DETECTION FUNCTIONS
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// d_env_curses_detect
//   function: attempts runtime detection of curses library
//   returns: curses type flags (D_ENV_CURSES_TYPE_*) or 0 if not found
//   
//   This function attempts to detect the presence and type of curses library
//   at runtime by checking for library symbols and features.
int d_env_curses_detect(void);

// d_env_curses_get_name
//   function: returns human-readable name of detected curses library
//   returns: string describing the curses variant, or "None" if not available
const char* d_env_curses_get_name(void);

// d_env_curses_get_version
//   function: attempts to get version information of curses library
//   returns: version string if available, or "Unknown" if not determinable
const char* d_env_curses_get_version(void);

// d_env_curses_has_feature
//   function: checks if a specific curses feature is available
//   params:
//     feature - D_ENV_CURSES_FEAT_* flag to check
//   returns: 1 if feature is available, 0 otherwise
int d_env_curses_has_feature(int feature);

// d_env_curses_supports_color
//   function: checks if the curses library supports colors
//   returns: 1 if color support is available, 0 otherwise
int d_env_curses_supports_color(void);

// d_env_curses_supports_wide
//   function: checks if the curses library supports wide characters
//   returns: 1 if wide character support is available, 0 otherwise
int d_env_curses_supports_wide(void);

// d_env_curses_supports_mouse
//   function: checks if the curses library supports mouse events
//   returns: 1 if mouse support is available, 0 otherwise
int d_env_curses_supports_mouse(void);

// d_env_curses_print_info
//   function: prints detailed information about detected curses library
//   This is useful for debugging and system capability reporting
void d_env_curses_print_info(void);

#ifdef __cplusplus
}
#endif

// =============================================================================
// V.  CONVENIENCE MACROS
// =============================================================================

// D_ENV_HAS_CURSES
//   macro: evaluates to 1 if any curses library is detected, 0 otherwise
#define D_ENV_HAS_CURSES() \
    (D_ENV_CURSES_AVAILABLE || (d_env_curses_detect() != D_ENV_CURSES_TYPE_NONE))

// D_ENV_HAS_NCURSES
//   macro: evaluates to 1 if ncurses (narrow or wide) is detected
#define D_ENV_HAS_NCURSES() \
    ((D_ENV_CURSES_TYPE & (D_ENV_CURSES_TYPE_NCURSES | D_ENV_CURSES_TYPE_NCURSESW)) != 0)

// D_ENV_HAS_PDCURSES
//   macro: evaluates to 1 if PDCurses variant is detected
#define D_ENV_HAS_PDCURSES() \
    ((D_ENV_CURSES_TYPE & (D_ENV_CURSES_TYPE_PDCURSES | D_ENV_CURSES_TYPE_PDCURSESMOD)) != 0)

// D_ENV_CURSES_IS_TYPE
//   macro: checks if detected curses matches a specific type
#define D_ENV_CURSES_IS_TYPE(TYPE) \
    ((D_ENV_CURSES_TYPE & (TYPE)) != 0)

// D_ENV_CURSES_HAS_FEAT
//   macro: checks if a specific feature is available at compile-time
#define D_ENV_CURSES_HAS_FEAT(FEAT) \
    ((D_ENV_CURSES_FEATURES & (FEAT)) != 0)

#endif  // DJINTERP_ENV_CURSES_
