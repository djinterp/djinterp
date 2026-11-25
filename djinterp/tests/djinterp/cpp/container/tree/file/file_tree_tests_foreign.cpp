/*******************************************************************************
* djinterp [cpp][container]                          file_tree_tests_foreign.cpp
*
*   The apple, ios and windows scanners, off their platforms. On a host that
* is not Windows this unit allows every foreign backend before including the
* umbrella, so each compiles here and its portable path runs; its native
* code compiles on its own platform, and Windows's under the leaf's MinGW-w64
* check. On Windows the POSIX scanners cannot compile, and nothing here runs.
*
*
* path:      /tests/djinterp/cpp/container/tree/file/file_tree_tests_foreign.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// djinterp
#include <djinterp/env/env.h>  // D_ENV_OS_ID, before the knobs it decides
#if !D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #define D_CFG_FILESYS_ALLOW_FOREIGN 1
#endif
#include "file_tree_tests.hpp"  // D_FT_CHECK, DFtScratch, the declarations
#include <djinterp/cpp/container/tree/file/file_tree.hpp>  // under test

// std
#include <type_traits>  // std::is_same


NS_DJINTERP
NS_TESTING

/*
tests_file_tree_foreign_selectors
  each operating system selects its scanner
  Tests the following:
  - apple, ios, windows, windows10 and windows11 select their scanners
  - windows's scanner is the basic scanner without the large fetch, and
    windows10's and windows11's the one with it
  - none selects the null scanner
*/
bool tests_file_tree_foreign_selectors()
{
#if D_FILESYS_ENABLE_APPLE && D_FILESYS_ENABLE_IOS && D_FILESYS_ENABLE_WINDOWS
    D_FT_CHECK(std::is_same<os_scanner<DOperatingSystem::apple>::type,
                            DAppleScanner>::value);
    D_FT_CHECK(std::is_same<os_scanner<DOperatingSystem::ios>::type,
                            DIosScanner>::value);
    D_FT_CHECK(std::is_same<os_scanner<DOperatingSystem::windows>::type,
                            DBasicWindowsScanner<false>>::value);
    D_FT_CHECK(std::is_same<os_scanner<DOperatingSystem::windows10>::type,
                            DBasicWindowsScanner<true>>::value);
    D_FT_CHECK(std::is_same<os_scanner<DOperatingSystem::windows11>::type,
                            DWindows10Scanner>::value);
#endif
    D_FT_CHECK(std::is_same<os_scanner<DOperatingSystem::none>::type,
                            DNullScanner>::value);

    return true;
}

/*
tests_file_tree_foreign_off_platform
  the foreign scanners' portable paths, run here
  Tests the following:
  - apple's scanner, without getattrlistbulk, falls back to the bsd scan
    and gives the native tree; ios's delegates to it and gives the same
  - windows's scanner, off Windows, is a stub: the root alone
*/
bool tests_file_tree_foreign_off_platform()
{
#if !D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) && !defined(__APPLE__)
    DFtScratch                         s("foreign");
    DFileTreeDefault                   native;
    DFileTree<DOperatingSystem::apple> apple;
    DFileTree<DOperatingSystem::ios>   ios;
    DFileTree<DOperatingSystem::windows10> windows;

    D_FT_CHECK(s.ok);

    native.scan(s.path());
    apple.scan(s.path());
    ios.scan(s.path());
    windows.scan(s.path());

    D_FT_CHECK(apple.size() == native.size());
    D_FT_CHECK(ios.size() == native.size());
    D_FT_CHECK(apple.resolve("sub/deeper/c") != null_file_node);
    D_FT_CHECK(apple[apple.resolve("a.txt")].data.size == 5);
    D_FT_CHECK(windows.size() == 1);
#endif

    return true;
}

NS_END  // testing
NS_END  // djinterp
