/*******************************************************************************
* djinterp [env]                                                       env_cli.h
*
* Environment detection for the command-line kit.
*   What the platform decides for a program built with the kit: how a
* loadable module is opened and what its file name ends in, how the running
* program finds its own file, how it asks whether a stream is a terminal and
* writes to one, and what separates the directories of a search path.
*   It includes no system header. Each fact is derived from env.h's OS
* classification, is #ifndef-guarded so a build may define it first, and is
* read only by cfg_cli.h and the kit's sources: the system headers the facts
* call for are included by one source, cli_system.c, and by no header.
*
*
* path:      /inc/djinterp/env/parse/cli/env_cli.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  MODULES
    -------
    1.  Loading
         1.  Module identifiers
              1.  D_ENV_CLI_MODULES_NONE
              2.  D_ENV_CLI_MODULES_DLOPEN
              3.  D_ENV_CLI_MODULES_WINDOWS
         2.  D_ENV_CLI_MODULES
    2.  Names
         1.  D_ENV_CLI_MODULE_SUFFIX
2.  THE PROCESS
    -----------
    1.  The program's own file
         1.  Executable identifiers
              1.  D_ENV_CLI_EXECUTABLE_ARGV
              2.  D_ENV_CLI_EXECUTABLE_PROC
              3.  D_ENV_CLI_EXECUTABLE_WINDOWS
              4.  D_ENV_CLI_EXECUTABLE_MACOS
              5.  D_ENV_CLI_EXECUTABLE_SYSCTL
              6.  D_ENV_CLI_EXECUTABLE_SOLARIS
         2.  D_ENV_CLI_EXECUTABLE
         3.  D_ENV_CLI_EXECUTABLE_LINK
    2.  Terminals
         1.  Console identifiers
              1.  D_ENV_CLI_CONSOLE_NONE
              2.  D_ENV_CLI_CONSOLE_POSIX
              3.  D_ENV_CLI_CONSOLE_WINDOWS
         2.  D_ENV_CLI_CONSOLE
    3.  Search paths
         1.  D_ENV_CLI_PATH_LIST_SEPARATOR
         2.  D_ENV_CLI_HELP_SLASH
*/

#ifndef DJINTERP_ENV_PARSE_CLI_ENV_CLI_H
#define DJINTERP_ENV_PARSE_CLI_ENV_CLI_H 1

// djinterp
#include "../../env.h"  // D_ENV_OS_ID, D_ENV_IS_OS_*, D_ENV_OS_FLAG_*


//==============================================================================
// 1.  MODULES
//==============================================================================
// How a program loads the module of a command it does not carry itself --
// its window, as a plugin (GRAMMAR.md, section 7).


// 1.1    Loading
//------------------------------------------------------------------------------
// 1.1.1
// Module identifiers
//   constant: the ways a module is opened, for D_ENV_CLI_MODULES.

// 1.1.1.1
// D_ENV_CLI_MODULES_NONE
//   constant: none -- the platform has no loadable modules the kit knows; a
// plugin command reports that it cannot load one.
#define D_ENV_CLI_MODULES_NONE    0

// 1.1.1.2
// D_ENV_CLI_MODULES_DLOPEN
//   constant: POSIX's dlopen, dlsym and dlclose, from <dlfcn.h>.
#define D_ENV_CLI_MODULES_DLOPEN  1

// 1.1.1.3
// D_ENV_CLI_MODULES_WINDOWS
//   constant: LoadLibraryExW, GetProcAddress and FreeLibrary.
#define D_ENV_CLI_MODULES_WINDOWS 2

// 1.1.2
// D_ENV_CLI_MODULES
//   detection: how modules are opened here: Windows' own calls on Windows,
// dlopen on Linux, Android, macOS, the BSDs and Solaris, and none elsewhere.
#ifndef D_ENV_CLI_MODULES
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_CLI_MODULES D_ENV_CLI_MODULES_WINDOWS
    #elif D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_ID)
        #define D_ENV_CLI_MODULES D_ENV_CLI_MODULES_DLOPEN
    #else
        #define D_ENV_CLI_MODULES D_ENV_CLI_MODULES_NONE
    #endif
#endif  // D_ENV_CLI_MODULES

// 1.2    Names
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_CLI_MODULE_SUFFIX
//   detection: what a loadable module's file name ends in, as CMake names a
// MODULE library: ".dll" on Windows and ".so" everywhere else, macOS
// included.
#ifndef D_ENV_CLI_MODULE_SUFFIX
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_CLI_MODULE_SUFFIX ".dll"
    #else
        #define D_ENV_CLI_MODULE_SUFFIX ".so"
    #endif
#endif  // D_ENV_CLI_MODULE_SUFFIX


//==============================================================================
// 2.  THE PROCESS
//==============================================================================


// 2.1    The program's own file
//------------------------------------------------------------------------------
// 2.1.1
// Executable identifiers
//   constant: the ways a program finds the file it was run from, for
// D_ENV_CLI_EXECUTABLE.

// 2.1.1.1
// D_ENV_CLI_EXECUTABLE_ARGV
//   constant: from `argv[0]` -- made absolute where it holds a separator,
// and looked for along PATH where it does not -- the fallback where the
// system offers nothing better, OpenBSD's case.
#define D_ENV_CLI_EXECUTABLE_ARGV    0

// 2.1.1.2
// D_ENV_CLI_EXECUTABLE_PROC
//   constant: the link the process file system keeps to it,
// D_ENV_CLI_EXECUTABLE_LINK, read with readlink.
#define D_ENV_CLI_EXECUTABLE_PROC    1

// 2.1.1.3
// D_ENV_CLI_EXECUTABLE_WINDOWS
//   constant: GetModuleFileNameW for the process's own module.
#define D_ENV_CLI_EXECUTABLE_WINDOWS 2

// 2.1.1.4
// D_ENV_CLI_EXECUTABLE_MACOS
//   constant: _NSGetExecutablePath, from <mach-o/dyld.h>, made canonical.
#define D_ENV_CLI_EXECUTABLE_MACOS   3

// 2.1.1.5
// D_ENV_CLI_EXECUTABLE_SYSCTL
//   constant: the KERN_PROC_PATHNAME request to sysctl, of FreeBSD and
// DragonFly BSD.
#define D_ENV_CLI_EXECUTABLE_SYSCTL  4

// 2.1.1.6
// D_ENV_CLI_EXECUTABLE_SOLARIS
//   constant: getexecname, made absolute against the directory the process
// started in.
#define D_ENV_CLI_EXECUTABLE_SOLARIS 5

// 2.1.2
// D_ENV_CLI_EXECUTABLE
//   detection: how the running program finds its own file here.
#ifndef D_ENV_CLI_EXECUTABLE
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_CLI_EXECUTABLE D_ENV_CLI_EXECUTABLE_WINDOWS
    #elif ( (D_ENV_IS_OS_LINUX(D_ENV_OS_ID))           ||                     \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_ANDROID)     ||                     \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_NET) )
        #define D_ENV_CLI_EXECUTABLE D_ENV_CLI_EXECUTABLE_PROC
    #elif D_ENV_IS_OS_MACOS(D_ENV_OS_ID)
        #define D_ENV_CLI_EXECUTABLE D_ENV_CLI_EXECUTABLE_MACOS
    #elif ( (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_FREE)     ||                     \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_DRAGONFLY) )
        #define D_ENV_CLI_EXECUTABLE D_ENV_CLI_EXECUTABLE_SYSCTL
    #elif (D_ENV_OS_ID == D_ENV_OS_FLAG_SOLARIS)
        #define D_ENV_CLI_EXECUTABLE D_ENV_CLI_EXECUTABLE_SOLARIS
    #else
        #define D_ENV_CLI_EXECUTABLE D_ENV_CLI_EXECUTABLE_ARGV
    #endif
#endif  // D_ENV_CLI_EXECUTABLE

// 2.1.3
// D_ENV_CLI_EXECUTABLE_LINK
//   detection: the link D_ENV_CLI_EXECUTABLE_PROC reads: Linux's and
// Android's /proc/self/exe, and NetBSD's /proc/curproc/exe.
#ifndef D_ENV_CLI_EXECUTABLE_LINK
    #if (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_NET)
        #define D_ENV_CLI_EXECUTABLE_LINK "/proc/curproc/exe"
    #else
        #define D_ENV_CLI_EXECUTABLE_LINK "/proc/self/exe"
    #endif
#endif  // D_ENV_CLI_EXECUTABLE_LINK

// 2.2    Terminals
//------------------------------------------------------------------------------
// 2.2.1
// Console identifiers
//   constant: the ways a stream is asked whether it is a terminal, and how a
// terminal is written to, for D_ENV_CLI_CONSOLE.

// 2.2.1.1
// D_ENV_CLI_CONSOLE_NONE
//   constant: no way to ask: no stream is a terminal, and none is coloured
// unless asked.
#define D_ENV_CLI_CONSOLE_NONE    0

// 2.2.1.2
// D_ENV_CLI_CONSOLE_POSIX
//   constant: isatty, from <unistd.h>; a terminal takes UTF-8 and ANSI
// sequences as they are.
#define D_ENV_CLI_CONSOLE_POSIX   1

// 2.2.1.3
// D_ENV_CLI_CONSOLE_WINDOWS
//   constant: the console API: GetConsoleMode says whether a handle is a
// console, SetConsoleMode turns on its ANSI sequences, and WriteConsoleW
// writes it text whatever its code page.
#define D_ENV_CLI_CONSOLE_WINDOWS 2

// 2.2.2
// D_ENV_CLI_CONSOLE
//   detection: how terminals are asked about and written to here.
#ifndef D_ENV_CLI_CONSOLE
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_CLI_CONSOLE D_ENV_CLI_CONSOLE_WINDOWS
    #elif D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_ID)
        #define D_ENV_CLI_CONSOLE D_ENV_CLI_CONSOLE_POSIX
    #else
        #define D_ENV_CLI_CONSOLE D_ENV_CLI_CONSOLE_NONE
    #endif
#endif  // D_ENV_CLI_CONSOLE

// 2.3    Search paths
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_CLI_PATH_LIST_SEPARATOR
//   detection: what separates the directories of a search path, as PATH
// writes them: ';' on Windows and ':' elsewhere.
#ifndef D_ENV_CLI_PATH_LIST_SEPARATOR
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_CLI_PATH_LIST_SEPARATOR ';'
    #else
        #define D_ENV_CLI_PATH_LIST_SEPARATOR ':'
    #endif
#endif  // D_ENV_CLI_PATH_LIST_SEPARATOR

// 2.3.2
// D_ENV_CLI_HELP_SLASH
//   detection: 1 where `/?` is a request for help, as Windows users type it
// (GRAMMAR.md, R10); 0 elsewhere, where it is a path.
#ifndef D_ENV_CLI_HELP_SLASH
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_CLI_HELP_SLASH 1
    #else
        #define D_ENV_CLI_HELP_SLASH 0
    #endif
#endif  // D_ENV_CLI_HELP_SLASH


#endif  // DJINTERP_ENV_PARSE_CLI_ENV_CLI_H
