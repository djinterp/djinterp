/*******************************************************************************
* djinterp [env]                                                    env_curses.c
*
* Implementation of curses library detection functions.
*
*
* path:      /src/djinterp/env/ui/env_curses.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2024.12.26
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/env/ui/env_curses.h"


// std
#include <stdio.h>
#include <string.h>

// Try to include curses if available
#if D_ENV_CURSES_AVAILABLE
    #if defined(NCURSES_VERSION)
        // curses
        #include <ncurses.h>
    #elif defined(PDC_VER_MAJOR) || defined(__PDCURSES__)
        // curses
        #include <curses.h>
    #else
        // curses
        #include <curses.h>
    #endif
#endif

// Static cache for runtime detection
static int g_curses_type_cache = -1;
static int g_curses_features_cache = -1;

// =============================================================================
// INTERNAL HELPER FUNCTIONS
// =============================================================================

// Try to initialize curses in a safe way for detection
static int try_init_curses(void) {
#if D_ENV_CURSES_AVAILABLE
    WINDOW* test_win = NULL;

    // Try to initialize
    test_win = initscr();
    if (test_win == NULL) {
        return 0;
    }

    // Clean up immediately
    endwin();
    return 1;
#else
    return 0;
#endif
}

// Detect curses features at runtime
static int detect_curses_features(void) {
#if D_ENV_CURSES_AVAILABLE
    int features = 0;

    // Initialize curses temporarily for feature detection
    WINDOW* test_win = initscr();
    if (test_win == NULL) {
        return 0;
    }

    // Check color support
    if (has_colors()) {
        features |= D_ENV_CURSES_FEAT_COLOR;
    }

    // Check for wide character support
    #if defined(NCURSES_WIDECHAR) || defined(PDC_WIDE)
        features |= D_ENV_CURSES_FEAT_WIDE;
    #endif

    // Check mouse support
    #if defined(NCURSES_MOUSE_VERSION) || defined(PDC_VER_MAJOR)
        features |= D_ENV_CURSES_FEAT_MOUSE;
    #endif

    // Check resize support
    #if defined(KEY_RESIZE)
        features |= D_ENV_CURSES_FEAT_RESIZE;
    #endif

    // Check for extended features (ncurses-specific)
    #if defined(NCURSES_VERSION)
        features |= D_ENV_CURSES_FEAT_EXTENDED;
    #endif

    endwin();
    return features;
#else
    return 0;
#endif
}

// =============================================================================
// PUBLIC API IMPLEMENTATION
// =============================================================================

int d_env_curses_detect(void) {
    // Return cached result if available
    if (g_curses_type_cache != -1) {
        return g_curses_type_cache;
    }

#if D_ENV_CURSES_AVAILABLE
    // Compile-time detection succeeded
    g_curses_type_cache = D_ENV_CURSES_TYPE;
    g_curses_features_cache = D_ENV_CURSES_FEATURES;
    return g_curses_type_cache;
#else
    // No compile-time detection - try runtime detection

    // Attempt to initialize curses
    if (!try_init_curses()) {
        g_curses_type_cache = D_ENV_CURSES_TYPE_NONE;
        g_curses_features_cache = 0;
        return D_ENV_CURSES_TYPE_NONE;
    }

    // Try to determine the curses type from platform
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        // On Windows, it's most likely PDCurses
        g_curses_type_cache = D_ENV_CURSES_TYPE_PDCURSES;
    #elif ( (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX) ||                            \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS) ||                            \
            (D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)) )
        // On Unix-like systems, likely ncurses
        g_curses_type_cache = D_ENV_CURSES_TYPE_NCURSES;
    #else
        // Unknown - could be System V curses
        g_curses_type_cache = D_ENV_CURSES_TYPE_UNKNOWN;
    #endif

    // Detect features
    g_curses_features_cache = detect_curses_features();

    return g_curses_type_cache;
#endif
}

const char* d_env_curses_get_name(void) {
    int curses_type = d_env_curses_detect();

    if (curses_type == D_ENV_CURSES_TYPE_NONE) {
        return "None";
    }

    #if D_ENV_CURSES_AVAILABLE
        return D_ENV_CURSES_NAME;
    #else
        // Runtime detection result
        if (curses_type & D_ENV_CURSES_TYPE_NCURSESW) {
            return "ncursesw";
        } else if (curses_type & D_ENV_CURSES_TYPE_NCURSES) {
            return "ncurses";
        } else if (curses_type & D_ENV_CURSES_TYPE_PDCURSESMOD) {
            return "PDCursesMod";
        } else if (curses_type & D_ENV_CURSES_TYPE_PDCURSES) {
            return "PDCurses";
        } else if (curses_type & D_ENV_CURSES_TYPE_BSD_CURSES) {
            return "BSD curses";
        } else if (curses_type & D_ENV_CURSES_TYPE_SYSV) {
            return "System V curses";
        } else {
            return "Unknown curses";
        }
    #endif
}

const char* d_env_curses_get_version(void) {
#if defined(NCURSES_VERSION)
    return NCURSES_VERSION;
#elif defined(PDC_VER_MAJOR)
    static char version_buf[32];
    snprintf(version_buf, sizeof(version_buf), "%d.%d",
             PDC_VER_MAJOR, PDC_VER_MINOR);
    return version_buf;
#elif defined(__PDCURSES__)
    return "PDCurses (version unknown)";
#else
    return "Unknown";
#endif
}

int d_env_curses_has_feature(int feature) {
    // Ensure detection has run
    if (g_curses_features_cache == -1) {
        d_env_curses_detect();
    }

    return (g_curses_features_cache & feature) != 0;
}

int d_env_curses_supports_color(void) {
    return d_env_curses_has_feature(D_ENV_CURSES_FEAT_COLOR);
}

int d_env_curses_supports_wide(void) {
    return d_env_curses_has_feature(D_ENV_CURSES_FEAT_WIDE);
}

int d_env_curses_supports_mouse(void) {
    return d_env_curses_has_feature(D_ENV_CURSES_FEAT_MOUSE);
}

void d_env_curses_print_info(void) {
    int curses_type = d_env_curses_detect();

    printf("Curses Library Information:\n");
    printf("==========================\n");

    if (curses_type == D_ENV_CURSES_TYPE_NONE) {
        printf("Status:          Not available\n");

        // Provide platform-specific guidance
        #if defined(D_ENV_CURSES_EXPECTED_PACKAGE)
            printf("Expected:        %s\n", D_ENV_CURSES_EXPECTED_PACKAGE);
        #endif

        #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
            printf("\nInstallation options for Windows:\n");
            printf("  - PDCurses:     https://pdcurses.org/\n");
            printf("  - PDCursesMod:  https://github.com/Bill-Gray/PDCursesMod\n");
            printf("  - ncurses:      Via WSL, Cygwin, or MSYS2\n");
        #elif (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)
            printf("\nInstallation commands for Linux:\n");
            printf("  Debian/Ubuntu:  sudo apt-get install libncurses-dev\n");
            printf("  Red Hat/Fedora: sudo dnf install ncurses-devel\n");
            printf("  Arch:           sudo pacman -S ncurses\n");
        #elif (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS)
            printf("\nInstallation options for macOS:\n");
            printf("  Homebrew:       brew install ncurses\n");
            printf("  MacPorts:       sudo port install ncurses\n");
            printf("  (ncurses is typically pre-installed on macOS)\n");
        #elif D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)
            printf("\nNote: ncurses is typically part of the base system on BSD.\n");
        #endif

        return;
    }

    printf("Status:          Available\n");
    printf("Library:         %s\n", d_env_curses_get_name());
    printf("Version:         %s\n", d_env_curses_get_version());
    printf("\nPlatform:        %s\n", D_ENV_OS_NAME);

    printf("\nFeatures:\n");
    printf("  Color support:       %s\n",
           d_env_curses_supports_color() ? "Yes" : "No");
    printf("  Wide char support:   %s\n",
           d_env_curses_supports_wide() ? "Yes" : "No");
    printf("  Mouse support:       %s\n",
           d_env_curses_supports_mouse() ? "Yes" : "No");
    printf("  Resize support:      %s\n",
           d_env_curses_has_feature(D_ENV_CURSES_FEAT_RESIZE) ? "Yes" : "No");
    printf("  Extended features:   %s\n",
           d_env_curses_has_feature(D_ENV_CURSES_FEAT_EXTENDED) ? "Yes" : "No");

    printf("\nDetection method:    ");
    #if D_ENV_CURSES_AVAILABLE
        printf("Compile-time\n");
    #else
        printf("Runtime\n");
    #endif
}
