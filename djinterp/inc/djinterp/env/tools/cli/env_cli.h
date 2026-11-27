/*******************************************************************************
* djinterp [c][env]                                                    env_cli.h
*
* Environment detection for the command-line kit.
*   What the platform decides for a program built with the kit: how a
* loadable module is opened and what its file name ends in, how the running
* program finds its own file, how it asks whether a stream is a terminal and
* writes to one, what separates the directories of a search path, and how
* it asks whether it runs with privileges its caller lacks.
*   It includes no system header. Each fact is derived from env.h's OS
* classification, is #ifndef-guarded so a build may define it first, and is
* read only by cfg_cli.h and the kit's sources: the system headers the facts
* call for are included by one source, cli_system.c, and by no header.
*
* HOW TO USE
* ----------
*   Nothing to call, and no program includes it: cfg_cli.h does, and the kit's
* sources read its facts. D_ENV_CLI_MODULES and D_ENV_CLI_MODULE_SUFFIX say how
* a module is loaded and named; D_ENV_CLI_EXECUTABLE and
* D_ENV_CLI_EXECUTABLE_LINK, how the program finds its own file;
* D_ENV_CLI_CONSOLE, how a stream is asked whether it is a terminal;
* D_ENV_CLI_PATH_LIST_SEPARATOR, D_ENV_CLI_PATH_SEPARATOR and
* D_ENV_CLI_HELP_SLASH, the separators; and D_ENV_CLI_PRIVILEGE, how a run is
* asked whether it has privileges its caller lacks.
*
* OPTIONS
* -------
*   Each fact's values are its identifiers: D_ENV_CLI_MODULES_NONE, _DLOPEN or
* _WINDOWS; D_ENV_CLI_EXECUTABLE_ARGV, _PROC, _WINDOWS, _MACOS, _SYSCTL or
* _SOLARIS; D_ENV_CLI_CONSOLE_NONE, _POSIX or _WINDOWS;
* D_ENV_CLI_PRIVILEGE_NONE, _AUXVAL, _ISSETUGID or _IDS.
*
* CONFIGURATION
* -------------
*   Define any fact before this header is read, with -D or in a prefix header,
* and the kit takes it as given: D_ENV_CLI_MODULES as D_ENV_CLI_MODULES_NONE
* to build without loadable modules, say, or the facts of a platform env.h
* does not classify.
* Otherwise each is derived from env.h's: the privilege check is AUXVAL on Linux
* and Android (IDS with a glibc before 2.16), ISSETUGID on the BSDs and Apple's
* systems, IDS on any other POSIX system, and NONE on Windows and elsewhere.
*
*
* path:      /inc/djinterp/env/tools/cli/env_cli.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.11.25
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
         2.  D_ENV_CLI_PATH_SEPARATOR
         3.  D_ENV_CLI_HELP_SLASH
    4.  Privileges
         1.  Privilege identifiers
              1.  D_ENV_CLI_PRIVILEGE_NONE
              2.  D_ENV_CLI_PRIVILEGE_AUXVAL
              3.  D_ENV_CLI_PRIVILEGE_ISSETUGID
              4.  D_ENV_CLI_PRIVILEGE_IDS
         2.  D_ENV_CLI_PRIVILEGE
*/

#ifndef DJINTERP_ENV_TOOLS_CLI_ENV_CLI_H
#define DJINTERP_ENV_TOOLS_CLI_ENV_CLI_H 1

// djinterp
#include "../../env.h"  // D_ENV_OS_ID, D_ENV_IS_OS_*, D_ENV_OS_FLAG_*


//==============================================================================
// 1.  MODULES
//==============================================================================
// How a program loads the module of a command it does not carry itself --
// its window, as a plugin (GRAMMAR.html, section 7).


// 1.1    Loading
//------------------------------------------------------------------------------
// 1.1.1
// Module identifiers
//   constant: the ways a module is opened, for D_ENV_CLI_MODULES.

// 1.1.1.1
// D_ENV_CLI_MODULES_NONE
//   constant: none - the platform has no loadable modules the kit knows; a
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
//   constant: from `argv[0]` - made absolute where it holds a separator,
// and looked for along PATH where it does not - the fallback where the
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
// D_ENV_CLI_PATH_SEPARATOR
//   detection: what the kit joins a directory and a name with: '\\' on
// Windows, where LoadLibraryExW, which loads a module, asks that a path it is
// given use backslashes, not forward slashes; '/' elsewhere. Either is read
// as a separator on Windows.
#ifndef D_ENV_CLI_PATH_SEPARATOR
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_CLI_PATH_SEPARATOR '\\'
    #else
        #define D_ENV_CLI_PATH_SEPARATOR '/'
    #endif
#endif  // D_ENV_CLI_PATH_SEPARATOR

// 2.3.3
// D_ENV_CLI_HELP_SLASH
//   detection: 1 where `/?` is a request for help, as Windows users type it
// (GRAMMAR.html, R10); 0 elsewhere, where it is a path.
#ifndef D_ENV_CLI_HELP_SLASH
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_CLI_HELP_SLASH 1
    #else
        #define D_ENV_CLI_HELP_SLASH 0
    #endif
#endif  // D_ENV_CLI_HELP_SLASH

// 2.4    Privileges
//------------------------------------------------------------------------------
// 2.4.1
// Privilege identifiers
//   constant: the ways a program asks whether it runs with privileges its
// caller lacks - set-user-ID, set-group-ID, or capabilities given to the
// file - for D_ENV_CLI_PRIVILEGE (cfg_cli.h, 1.6).

// 2.4.1.1
// D_ENV_CLI_PRIVILEGE_NONE
//   constant: no way to ask, or nothing to ask: no run is privileged. So on
// Windows, where a process's elevation is never inherited from its caller.
#define D_ENV_CLI_PRIVILEGE_NONE      0

// 2.4.1.2
// D_ENV_CLI_PRIVILEGE_AUXVAL
//   constant: getauxval(AT_SECURE), from <sys/auxv.h>: set for set-user-ID,
// set-group-ID and capabilities alike. Linux, where glibc (2.16 and later,
// D_ENV_LINUX_GLIBC_HAS_GETAUXVAL), musl or Android's C library has it;
// with an older glibc the source falls back to D_ENV_CLI_PRIVILEGE_IDS.
#define D_ENV_CLI_PRIVILEGE_AUXVAL    1

// 2.4.1.3
// D_ENV_CLI_PRIVILEGE_ISSETUGID
//   constant: issetugid(), from <unistd.h>: the BSDs and Apple's systems
// (D_ENV_BSD_HAS_ISSETUGID, D_ENV_APPLE_HAS_ISSETUGID).
#define D_ENV_CLI_PRIVILEGE_ISSETUGID 2

// 2.4.1.4
// D_ENV_CLI_PRIVILEGE_IDS
//   constant: the real user and group against the effective ones, from
// <unistd.h>: any other POSIX system. Capabilities are not seen.
#define D_ENV_CLI_PRIVILEGE_IDS       3

// 2.4.2
// D_ENV_CLI_PRIVILEGE
//   detection: how a privileged run is known here.
#ifndef D_ENV_CLI_PRIVILEGE
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_CLI_PRIVILEGE D_ENV_CLI_PRIVILEGE_NONE
    #elif ( (D_ENV_IS_OS_LINUX(D_ENV_OS_ID)) ||                               \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_ANDROID) )
        #define D_ENV_CLI_PRIVILEGE D_ENV_CLI_PRIVILEGE_AUXVAL
    #elif ( (D_ENV_IS_OS_BSD(D_ENV_OS_ID)) ||                                 \
            (D_ENV_IS_OS_MACOS(D_ENV_OS_ID)) ||                               \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_IOS) )
        #define D_ENV_CLI_PRIVILEGE D_ENV_CLI_PRIVILEGE_ISSETUGID
    #elif D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_ID)
        #define D_ENV_CLI_PRIVILEGE D_ENV_CLI_PRIVILEGE_IDS
    #else
        #define D_ENV_CLI_PRIVILEGE D_ENV_CLI_PRIVILEGE_NONE
    #endif
#endif  // D_ENV_CLI_PRIVILEGE


#endif  // DJINTERP_ENV_TOOLS_CLI_ENV_CLI_H
