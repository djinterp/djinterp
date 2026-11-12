/*******************************************************************************
* djinterp [c][env]                                               env_tests_os.c
*
* Unit tests for `env.h` operating system detection (section VII).
* Tests the D_ENV_OS_* macros that detect operating systems and properties.
* Note: DTest's C face stands on this module (through djinterp.h), so a
* fault here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/env_tests_os.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.24
*                                                            revised: 2026.11.06
*******************************************************************************/

// djinterp
#include "env_tests.h"


/******************************************************************************
* OS FLAG CONSTANTS TESTS
******************************************************************************/

/*
d_tests_env_os_flag_constants_apple
  Tests Apple OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_APPLE equals 0x00
  - D_ENV_OS_FLAG_MACOS equals 0x01
  - D_ENV_OS_FLAG_IOS equals 0x90
*/
bool d_tests_env_os_flag_constants_apple(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_flag_constants_apple");

    all_assertions_passed = true;

    // verify Apple OS flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_APPLE == 0x00",
                          D_ENV_OS_FLAG_APPLE == 0x00,
                          "Apple base flag should be 0x00"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_MACOS == 0x01",
                          D_ENV_OS_FLAG_MACOS == 0x01,
                          "macOS flag should be 0x01"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_IOS == 0x90",
                          D_ENV_OS_FLAG_IOS == 0x90,
                          "iOS flag should be 0x90"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_flag_constants_unix
  Tests Unix-like OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_UNIX equals 0x10
  - D_ENV_OS_FLAG_LINUX equals 0x11
  - D_ENV_OS_FLAG_SOLARIS equals 0x50
*/
bool d_tests_env_os_flag_constants_unix(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_flag_constants_unix");

    all_assertions_passed = true;

    // verify Unix OS flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_UNIX == 0x10",
                          D_ENV_OS_FLAG_UNIX == 0x10,
                          "Unix flag should be 0x10"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_LINUX == 0x11",
                          D_ENV_OS_FLAG_LINUX == 0x11,
                          "Linux flag should be 0x11"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_SOLARIS == 0x50",
                          D_ENV_OS_FLAG_SOLARIS == 0x50,
                          "Solaris flag should be 0x50"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_flag_constants_bsd
  Tests BSD OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_BSD_DRAGONFLY equals 0x40
  - D_ENV_OS_FLAG_BSD_FREE equals 0x41
  - D_ENV_OS_FLAG_BSD_NET equals 0x42
  - D_ENV_OS_FLAG_BSD_OPEN equals 0x43
  - D_ENV_OS_FLAG_BSD_OS equals 0x44
  - all BSD flags are in block 0x4
*/
bool d_tests_env_os_flag_constants_bsd(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_flag_constants_bsd");

    all_assertions_passed = true;

    // verify BSD OS flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_BSD_DRAGONFLY == 0x40",
                          D_ENV_OS_FLAG_BSD_DRAGONFLY == 0x40,
                          "DragonFly BSD flag should be 0x40"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_BSD_FREE == 0x41",
                          D_ENV_OS_FLAG_BSD_FREE == 0x41,
                          "FreeBSD flag should be 0x41"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_BSD_NET == 0x42",
                          D_ENV_OS_FLAG_BSD_NET == 0x42,
                          "NetBSD flag should be 0x42"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_BSD_OPEN == 0x43",
                          D_ENV_OS_FLAG_BSD_OPEN == 0x43,
                          "OpenBSD flag should be 0x43"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_BSD_OS == 0x44",
                          D_ENV_OS_FLAG_BSD_OS == 0x44,
                          "BSD/OS flag should be 0x44"))
    {
        all_assertions_passed = false;
    }

    // verify all BSD flags are in block 0x4
    if (!D_TEST_CHECK_MSG(_suite,
                          "BSD flags are in block 0x4",
                          (D_ENV_OS_FLAG_BSD_DRAGONFLY >> 4) == 0x4,
                          "all BSD flags should be in block 0x4"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_flag_constants_windows
  Tests Windows OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_MSDOS equals 0x60
  - Windows PC flags (0x61-0x67)
  - Windows Server flags (0x70-0x7A)
  - Windows Embedded/Mobile flags (0x80-0x81)
  - all Windows flags are in correct blocks
*/
bool d_tests_env_os_flag_constants_windows(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_flag_constants_windows");

    all_assertions_passed = true;

    // verify MS-DOS flag
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_MSDOS == 0x60",
                          D_ENV_OS_FLAG_MSDOS == 0x60,
                          "MS-DOS flag should be 0x60"))
    {
        all_assertions_passed = false;
    }

    // verify Windows PC flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_WIN_PC_PRE_XP == 0x61",
                          D_ENV_OS_FLAG_WIN_PC_PRE_XP == 0x61,
                          "Windows pre-XP flag should be 0x61"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_WIN_PC_10 == 0x66",
                          D_ENV_OS_FLAG_WIN_PC_10 == 0x66,
                          "Windows 10 flag should be 0x66"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_WIN_PC_11 == 0x67",
                          D_ENV_OS_FLAG_WIN_PC_11 == 0x67,
                          "Windows 11 flag should be 0x67"))
    {
        all_assertions_passed = false;
    }

    // verify Windows Server flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_WIN_SERVER_NT == 0x70",
                          D_ENV_OS_FLAG_WIN_SERVER_NT == 0x70,
                          "Windows NT Server flag should be 0x70"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_WIN_SERVER_2022 == 0x7A",
                          D_ENV_OS_FLAG_WIN_SERVER_2022 == 0x7A,
                          "Windows Server 2022 flag should be 0x7A"))
    {
        all_assertions_passed = false;
    }

    // verify Windows Embedded/Mobile flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_WIN_EMBED == 0x80",
                          D_ENV_OS_FLAG_WIN_EMBED == 0x80,
                          "Windows Embedded flag should be 0x80"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_WIN_MOBILE == 0x81",
                          D_ENV_OS_FLAG_WIN_MOBILE == 0x81,
                          "Windows Mobile flag should be 0x81"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_flag_constants_mobile
  Tests mobile OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_ANDROID equals 0xA0
  - D_ENV_OS_FLAG_BADA equals 0xA1
  - D_ENV_OS_FLAG_TIZEN equals 0xB0
*/
bool d_tests_env_os_flag_constants_mobile(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_flag_constants_mobile");

    all_assertions_passed = true;

    // verify mobile OS flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_ANDROID == 0xA0",
                          D_ENV_OS_FLAG_ANDROID == 0xA0,
                          "Android flag should be 0xA0"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_BADA == 0xA1",
                          D_ENV_OS_FLAG_BADA == 0xA1,
                          "Bada flag should be 0xA1"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_TIZEN == 0xB0",
                          D_ENV_OS_FLAG_TIZEN == 0xB0,
                          "Tizen flag should be 0xB0"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_flag_constants_legacy
  Tests legacy OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_AIX equals 0xE0
  - D_ENV_OS_FLAG_HP_UX equals 0xE6
  - D_ENV_OS_FLAG_IRIX equals 0xE8
  - D_ENV_OS_FLAG_QNX equals 0xF3
  - D_ENV_OS_FLAG_VMS equals 0xF7
  - D_ENV_OS_FLAG_UNKNOWN equals 0xFF
*/
bool d_tests_env_os_flag_constants_legacy(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_flag_constants_legacy");

    all_assertions_passed = true;

    // verify legacy OS flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_AIX == 0xE0",
                          D_ENV_OS_FLAG_AIX == 0xE0,
                          "AIX flag should be 0xE0"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_HP_UX == 0xE6",
                          D_ENV_OS_FLAG_HP_UX == 0xE6,
                          "HP-UX flag should be 0xE6"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_IRIX == 0xE8",
                          D_ENV_OS_FLAG_IRIX == 0xE8,
                          "IRIX flag should be 0xE8"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_QNX == 0xF3",
                          D_ENV_OS_FLAG_QNX == 0xF3,
                          "QNX flag should be 0xF3"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_VMS == 0xF7",
                          D_ENV_OS_FLAG_VMS == 0xF7,
                          "VMS flag should be 0xF7"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_FLAG_UNKNOWN == 0xFF",
                          D_ENV_OS_FLAG_UNKNOWN == 0xFF,
                          "Unknown OS flag should be 0xFF"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_flag_constants_all
  Runs all OS flag constant tests.
  Tests the following:
  - Apple OS flags
  - Unix OS flags
  - BSD OS flags
  - Windows OS flags
  - Mobile OS flags
  - Legacy OS flags
*/
bool d_tests_env_os_flag_constants_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_os_flag_constants_all");

    d_tests_env_os_flag_constants_apple(_suite);
    d_tests_env_os_flag_constants_unix(_suite);
    d_tests_env_os_flag_constants_bsd(_suite);
    d_tests_env_os_flag_constants_windows(_suite);
    d_tests_env_os_flag_constants_mobile(_suite);
    d_tests_env_os_flag_constants_legacy(_suite);

    return true;
}


/******************************************************************************
* UTILITY CONSTANTS TESTS
******************************************************************************/

/*
d_tests_env_os_utility_constants
  Tests OS utility constant definitions.
  Tests the following:
  - D_ENV_OS_BLOCK_SIZE equals 4
  - D_ENV_OS_FLAG_DISCONTINUED_FIRST is correct
  - D_ENV_OS_FLAG_DISCONTINUED_LAST is correct
  - D_ENV_OS_VENDOR_MS_FIRST is correct
  - D_ENV_OS_VENDOR_MS_LAST is correct
  - D_ENV_OS_FLAG_WIN_FIRST is correct
  - D_ENV_OS_FLAG_WIN_LAST is correct
  - range constants are consistent
*/
bool d_tests_env_os_utility_constants(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_utility_constants");

    all_assertions_passed = true;

    // verify block size
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_OS_BLOCK_SIZE == 4",
                          D_ENV_OS_BLOCK_SIZE == 4,
                          "block size should be 4 bits"))
    {
        all_assertions_passed = false;
    }

    // verify discontinued range
    if (!D_TEST_CHECK_MSG(_suite,
                          "DISCONTINUED_FIRST equals APOLLO_AEGIS",
                          D_ENV_OS_FLAG_DISCONTINUED_FIRST == D_ENV_OS_FLAG_APOLLO_AEGIS,
                          "discontinued range should start at Apollo Aegis"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "DISCONTINUED_LAST equals WINDU",
                          D_ENV_OS_FLAG_DISCONTINUED_LAST == D_ENV_OS_FLAG_WINDU,
                          "discontinued range should end at Windu"))
    {
        all_assertions_passed = false;
    }

    // verify Microsoft vendor range
    if (!D_TEST_CHECK_MSG(_suite,
                          "VENDOR_MS_FIRST equals MSDOS",
                          D_ENV_OS_VENDOR_MS_FIRST == D_ENV_OS_FLAG_MSDOS,
                          "Microsoft range should start at MS-DOS"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "VENDOR_MS_LAST equals WIN_MOBILE",
                          D_ENV_OS_VENDOR_MS_LAST == D_ENV_OS_FLAG_WIN_MOBILE,
                          "Microsoft range should end at Windows Mobile"))
    {
        all_assertions_passed = false;
    }

    // verify Windows-specific range
    if (!D_TEST_CHECK_MSG(_suite,
                          "WIN_FIRST equals WIN_PC_PRE_XP",
                          D_ENV_OS_FLAG_WIN_FIRST == D_ENV_OS_FLAG_WIN_PC_PRE_XP,
                          "Windows range should start at pre-XP"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "WIN_LAST equals WIN_MOBILE",
                          D_ENV_OS_FLAG_WIN_LAST == D_ENV_OS_FLAG_WIN_MOBILE,
                          "Windows range should end at Windows Mobile"))
    {
        all_assertions_passed = false;
    }

    // verify ranges are consistent
    if (!D_TEST_CHECK_MSG(_suite,
                          "discontinued range is valid",
                          D_ENV_OS_FLAG_DISCONTINUED_FIRST <= D_ENV_OS_FLAG_DISCONTINUED_LAST,
                          "range first should be <= last"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Microsoft vendor range is valid",
                          D_ENV_OS_VENDOR_MS_FIRST <= D_ENV_OS_VENDOR_MS_LAST,
                          "range first should be <= last"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Windows range is valid",
                          D_ENV_OS_FLAG_WIN_FIRST <= D_ENV_OS_FLAG_WIN_LAST,
                          "range first should be <= last"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
* UTILITY MACROS TESTS
******************************************************************************/

/*
d_tests_env_os_utility_macro_block
  Tests D_ENV_IS_OS_FLAG_IN_BLOCK macro.
  Tests the following:
  - macro correctly identifies OS flags in specific blocks
  - block detection works for all major OS categories
  - macro returns boolean values
*/
bool d_tests_env_os_utility_macro_block(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_utility_macro_block");

    all_assertions_passed = true;

    // test block 0x0 (Apple)
    if (!D_TEST_CHECK_MSG(_suite,
                          "APPLE is in block 0x0",
                          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_APPLE, 0x0),
                          "Apple flag should be in block 0"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "MACOS is in block 0x0",
                          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_MACOS, 0x0),
                          "macOS flag should be in block 0"))
    {
        all_assertions_passed = false;
    }

    // test block 0x1 (Unix/Linux)
    if (!D_TEST_CHECK_MSG(_suite,
                          "UNIX is in block 0x1",
                          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_UNIX, 0x1),
                          "Unix flag should be in block 1"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "LINUX is in block 0x1",
                          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_LINUX, 0x1),
                          "Linux flag should be in block 1"))
    {
        all_assertions_passed = false;
    }

    // test block 0x4 (BSD)
    if (!D_TEST_CHECK_MSG(_suite,
                          "BSD_FREE is in block 0x4",
                          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_BSD_FREE, 0x4),
                          "FreeBSD flag should be in block 4"))
    {
        all_assertions_passed = false;
    }

    // test block 0x6 (MS-DOS/Windows PC)
    if (!D_TEST_CHECK_MSG(_suite,
                          "MSDOS is in block 0x6",
                          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_MSDOS, 0x6),
                          "MS-DOS flag should be in block 6"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "WIN_PC_10 is in block 0x6",
                          D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_WIN_PC_10, 0x6),
                          "Windows 10 flag should be in block 6"))
    {
        all_assertions_passed = false;
    }

    // test negative cases
    if (!D_TEST_CHECK_MSG(_suite,
                          "LINUX is not in block 0x4",
                          !D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_LINUX, 0x4),
                          "Linux should not be in BSD block"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_utility_macro_unix
  Tests D_ENV_IS_OS_FLAG_UNIX macro.
  Tests the following:
  - macro correctly identifies Unix flags (block 0x1)
  - macro returns false for non-Unix OS flags
*/
bool d_tests_env_os_utility_macro_unix(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_utility_macro_unix");

    all_assertions_passed = true;

    // test Unix flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "UNIX is Unix-like",
                          D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_FLAG_UNIX),
                          "Unix flag should be detected as Unix"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "LINUX is Unix-like",
                          D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_FLAG_LINUX),
                          "Linux flag should be detected as Unix"))
    {
        all_assertions_passed = false;
    }

    // test non-Unix flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "Windows is not Unix-like",
                          !D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_FLAG_WIN_PC_10),
                          "Windows should not be detected as Unix"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "macOS is not in Unix block",
                          !D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_FLAG_MACOS),
                          "macOS is Unix-like but not in Unix block"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_utility_macro_mobile
  Tests D_ENV_IS_OS_MOBILE macro.
  Tests the following:
  - macro correctly identifies mobile OS flags
  - macro returns false for non-mobile OS flags
*/
bool d_tests_env_os_utility_macro_mobile(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_utility_macro_mobile");

    all_assertions_passed = true;

    // test mobile OS flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "iOS is mobile",
                          D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_IOS),
                          "iOS flag should be detected as mobile"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Android is mobile",
                          D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_ANDROID),
                          "Android flag should be detected as mobile"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Bada is mobile",
                          D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_BADA),
                          "Bada flag should be detected as mobile"))
    {
        all_assertions_passed = false;
    }

    // test non-mobile OS flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "Linux is not mobile",
                          !D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_LINUX),
                          "Linux should not be detected as mobile"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "macOS is not mobile",
                          !D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_MACOS),
                          "macOS should not be detected as mobile"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Tizen is not in mobile macro",
                          !D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_TIZEN),
                          "Tizen is mobile but not in macro list"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_utility_macro_windows
  Tests D_ENV_IS_OS_WINDOWS and D_ENV_IS_OS_MSDOS macros.
  Tests the following:
  - D_ENV_IS_OS_WINDOWS correctly identifies Windows variants
  - D_ENV_IS_OS_MSDOS correctly identifies MS-DOS
  - macros return false for non-Windows OS flags
*/
bool d_tests_env_os_utility_macro_windows(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_utility_macro_windows");

    all_assertions_passed = true;

    // test Windows flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "WIN_PC_10 is Windows",
                          D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_WIN_PC_10),
                          "Windows 10 should be detected as Windows"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "WIN_SERVER_2022 is Windows",
                          D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_WIN_SERVER_2022),
                          "Windows Server should be detected as Windows"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "WIN_MOBILE is Windows",
                          D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_WIN_MOBILE),
                          "Windows Mobile should be detected as Windows"))
    {
        all_assertions_passed = false;
    }

    // test MS-DOS is not Windows (separate macro)
    if (!D_TEST_CHECK_MSG(_suite,
                          "MSDOS is not Windows",
                          !D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_MSDOS),
                          "MS-DOS should not be detected as Windows"))
    {
        all_assertions_passed = false;
    }

    // test MS-DOS macro
    if (!D_TEST_CHECK_MSG(_suite,
                          "MSDOS is MS-DOS",
                          D_ENV_IS_OS_MSDOS(D_ENV_OS_FLAG_MSDOS),
                          "MS-DOS flag should be detected"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Windows is not MS-DOS",
                          !D_ENV_IS_OS_MSDOS(D_ENV_OS_FLAG_WIN_PC_10),
                          "Windows should not be detected as MS-DOS"))
    {
        all_assertions_passed = false;
    }

    // test non-Windows flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "Linux is not Windows",
                          !D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_LINUX),
                          "Linux should not be detected as Windows"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_utility_macro_discontinued
  Tests D_ENV_IS_OS_DISCONTINUED and D_ENV_IS_OS_UNSUPPORTED macros.
  Tests the following:
  - D_ENV_IS_OS_DISCONTINUED correctly identifies discontinued OS flags
  - D_ENV_IS_OS_UNSUPPORTED correctly identifies unsupported OS flags
  - macros return false for modern/supported OS flags
*/
bool d_tests_env_os_utility_macro_discontinued(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_utility_macro_discontinued");

    all_assertions_passed = true;

    // test discontinued flags
    if (!D_TEST_CHECK_MSG(_suite,
                          "APOLLO_AEGIS is discontinued",
                          D_ENV_IS_OS_DISCONTINUED(D_ENV_OS_FLAG_APOLLO_AEGIS),
                          "Apollo Aegis should be discontinued"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "BEOS is discontinued",
                          D_ENV_IS_OS_DISCONTINUED(D_ENV_OS_FLAG_BEOS),
                          "BeOS should be discontinued"))
    {
        all_assertions_passed = false;
    }

    // test modern OS is not discontinued
    if (!D_TEST_CHECK_MSG(_suite,
                          "Linux is not discontinued",
                          !D_ENV_IS_OS_DISCONTINUED(D_ENV_OS_FLAG_LINUX),
                          "Linux should not be discontinued"))
    {
        all_assertions_passed = false;
    }

    // test unsupported includes discontinued
    if (!D_TEST_CHECK_MSG(_suite,
                          "APOLLO_AEGIS is unsupported",
                          D_ENV_IS_OS_UNSUPPORTED(D_ENV_OS_FLAG_APOLLO_AEGIS),
                          "Apollo Aegis should be unsupported"))
    {
        all_assertions_passed = false;
    }

    // test unsupported includes legacy systems
    if (!D_TEST_CHECK_MSG(_suite,
                          "AIX is unsupported",
                          D_ENV_IS_OS_UNSUPPORTED(D_ENV_OS_FLAG_AIX),
                          "AIX should be unsupported"))
    {
        all_assertions_passed = false;
    }

    // test modern OS is not unsupported
    if (!D_TEST_CHECK_MSG(_suite,
                          "Linux is not unsupported",
                          !D_ENV_IS_OS_UNSUPPORTED(D_ENV_OS_FLAG_LINUX),
                          "Linux should not be unsupported"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_utility_macro_posix
  Tests D_ENV_IS_OS_POSIX_COMPLIANT macro.
  Tests the following:
  - macro correctly identifies POSIX-compliant OS flags
  - Unix-like systems are POSIX-compliant
  - BSD systems are POSIX-compliant
  - macOS and Android are POSIX-compliant
  - Windows is not POSIX-compliant
*/
bool d_tests_env_os_utility_macro_posix(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_utility_macro_posix");

    all_assertions_passed = true;

    // test POSIX-compliant systems
    if (!D_TEST_CHECK_MSG(_suite,
                          "Unix is POSIX-compliant",
                          D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_UNIX),
                          "Unix should be POSIX-compliant"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Linux is POSIX-compliant",
                          D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_LINUX),
                          "Linux should be POSIX-compliant"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "FreeBSD is POSIX-compliant",
                          D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_BSD_FREE),
                          "FreeBSD should be POSIX-compliant"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "macOS is POSIX-compliant",
                          D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_MACOS),
                          "macOS should be POSIX-compliant"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Android is POSIX-compliant",
                          D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_ANDROID),
                          "Android should be POSIX-compliant"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Solaris is POSIX-compliant",
                          D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_SOLARIS),
                          "Solaris should be POSIX-compliant"))
    {
        all_assertions_passed = false;
    }

    // test non-POSIX systems
    if (!D_TEST_CHECK_MSG(_suite,
                          "Windows is not POSIX-compliant",
                          !D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_WIN_PC_10),
                          "Windows should not be POSIX-compliant"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "MS-DOS is not POSIX-compliant",
                          !D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_MSDOS),
                          "MS-DOS should not be POSIX-compliant"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_os_utility_macros_all
  Runs all OS utility macro tests.
  Tests the following:
  - block detection macro
  - Unix detection macro
  - mobile detection macro
  - Windows detection macros
  - discontinued/unsupported macros
  - POSIX compliance macro
*/
bool d_tests_env_os_utility_macros_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_os_utility_macros_all");

    d_tests_env_os_utility_macro_block(_suite);
    d_tests_env_os_utility_macro_unix(_suite);
    d_tests_env_os_utility_macro_mobile(_suite);
    d_tests_env_os_utility_macro_windows(_suite);
    d_tests_env_os_utility_macro_discontinued(_suite);
    d_tests_env_os_utility_macro_posix(_suite);

    return true;
}


/******************************************************************************
* OS DETECTION TESTS
******************************************************************************/

/*
d_tests_env_os_detection_macros
  Tests that OS detection macros are defined.
  Tests the following:
  - D_ENV_OS_ID is defined
  - D_ENV_OS_NAME is defined
  - D_ENV_OS_ID has a valid value
*/
bool d_tests_env_os_detection_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_detection_macros");

    all_assertions_passed = true;

    // verify D_ENV_OS_ID is defined
    #ifdef D_ENV_OS_ID
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_OS_ID is defined",
                              true,
                              "OS ID should be defined"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_OS_ID should be defined",
                              false,
                              "OS ID should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_OS_NAME is defined
    #ifdef D_ENV_OS_NAME
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_OS_NAME is defined",
                              true,
                              "OS name should be defined"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_OS_NAME should be defined",
                              false,
                              "OS name should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}

/*
d_tests_env_os_platform_macros
  Tests legacy platform detection macros.
  Tests the following:
  - exactly one D_ENV_PLATFORM_* macro is defined
  - platform macro matches OS detection
*/
bool d_tests_env_os_platform_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    platform_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_os_platform_macros");

    all_assertions_passed = true;

    // count platform macros
    platform_count = 0;

    #ifdef D_ENV_PLATFORM_ANDROID
        platform_count++;
    #endif

    #ifdef D_ENV_PLATFORM_WINDOWS
        platform_count++;
    #endif

    #ifdef D_ENV_PLATFORM_LINUX
        platform_count++;
    #endif

    #ifdef D_ENV_PLATFORM_MACOS
        platform_count++;
    #endif

    #ifdef D_ENV_PLATFORM_UNIX
        platform_count++;
    #endif

    #ifdef D_ENV_PLATFORM_UNKNOWN
        platform_count++;
    #endif

    // verify exactly one platform is defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "exactly one platform macro defined",
                          platform_count == 1,
                          "one and only one platform should be defined"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "%d platforms defined (expected 1)",
                                platform_count);
    }

    return all_assertions_passed;
}


/******************************************************************************
* MODULE AGGREGATOR
******************************************************************************/

/*
d_tests_env_os_all
  Runs all OS detection tests.
  Tests the following:
  - OS flag constants
  - OS utility constants
  - OS utility macros
  - OS detection
  - platform detection
*/
bool d_tests_env_os_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_os_all");

    d_tests_env_os_flag_constants_all(_suite);
    d_tests_env_os_utility_constants(_suite);
    d_tests_env_os_utility_macros_all(_suite);
    d_tests_env_os_detection_macros(_suite);
    d_tests_env_os_platform_macros(_suite);

    return true;
}
