/******************************************************************************
* djinterp [test]                                            env_tests_sa_os.c
*
* Unit tests for `env.h` operating system detection (section VII).
* Tests the D_ENV_OS_* macros that detect operating systems and properties.
* Note: this module is required to build DTest, so it uses `test_standalone.h`.
*
* path:      \test\env_tests_sa_os.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.24
******************************************************************************/

#include "env_tests_sa.h"


/******************************************************************************
* OS FLAG CONSTANTS TESTS
******************************************************************************/

/*
d_tests_sa_env_os_flag_constants_apple
  Tests Apple OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_APPLE equals 0x00
  - D_ENV_OS_FLAG_MACOS equals 0x01
  - D_ENV_OS_FLAG_IOS equals 0x90
*/
bool
d_tests_sa_env_os_flag_constants_apple
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Apple OS Flag Constants ---\n", D_INDENT);

    // verify Apple OS flags
    if (!d_assert_standalone(D_ENV_OS_FLAG_APPLE == 0x00,
                             "D_ENV_OS_FLAG_APPLE == 0x00",
                             "Apple base flag should be 0x00",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_MACOS == 0x01,
                             "D_ENV_OS_FLAG_MACOS == 0x01",
                             "macOS flag should be 0x01",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_IOS == 0x90,
                             "D_ENV_OS_FLAG_IOS == 0x90",
                             "iOS flag should be 0x90",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Apple flags: APPLE=0x%02X MACOS=0x%02X IOS=0x%02X\n",
           D_INDENT,
           D_ENV_OS_FLAG_APPLE,
           D_ENV_OS_FLAG_MACOS,
           D_ENV_OS_FLAG_IOS);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Apple OS flag constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Apple OS flag constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_flag_constants_unix
  Tests Unix-like OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_UNIX equals 0x10
  - D_ENV_OS_FLAG_LINUX equals 0x11
  - D_ENV_OS_FLAG_SOLARIS equals 0x50
*/
bool
d_tests_sa_env_os_flag_constants_unix
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Unix OS Flag Constants ---\n", D_INDENT);

    // verify Unix OS flags
    if (!d_assert_standalone(D_ENV_OS_FLAG_UNIX == 0x10,
                             "D_ENV_OS_FLAG_UNIX == 0x10",
                             "Unix flag should be 0x10",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_LINUX == 0x11,
                             "D_ENV_OS_FLAG_LINUX == 0x11",
                             "Linux flag should be 0x11",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_SOLARIS == 0x50,
                             "D_ENV_OS_FLAG_SOLARIS == 0x50",
                             "Solaris flag should be 0x50",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Unix flags: UNIX=0x%02X LINUX=0x%02X SOLARIS=0x%02X\n",
           D_INDENT,
           D_ENV_OS_FLAG_UNIX,
           D_ENV_OS_FLAG_LINUX,
           D_ENV_OS_FLAG_SOLARIS);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Unix OS flag constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Unix OS flag constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_flag_constants_bsd
  Tests BSD OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_BSD_DRAGONFLY equals 0x40
  - D_ENV_OS_FLAG_BSD_FREE equals 0x41
  - D_ENV_OS_FLAG_BSD_NET equals 0x42
  - D_ENV_OS_FLAG_BSD_OPEN equals 0x43
  - D_ENV_OS_FLAG_BSD_OS equals 0x44
  - all BSD flags are in block 0x4
*/
bool
d_tests_sa_env_os_flag_constants_bsd
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing BSD OS Flag Constants ---\n", D_INDENT);

    // verify BSD OS flags
    if (!d_assert_standalone(D_ENV_OS_FLAG_BSD_DRAGONFLY == 0x40,
                             "D_ENV_OS_FLAG_BSD_DRAGONFLY == 0x40",
                             "DragonFly BSD flag should be 0x40",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_BSD_FREE == 0x41,
                             "D_ENV_OS_FLAG_BSD_FREE == 0x41",
                             "FreeBSD flag should be 0x41",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_BSD_NET == 0x42,
                             "D_ENV_OS_FLAG_BSD_NET == 0x42",
                             "NetBSD flag should be 0x42",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_BSD_OPEN == 0x43,
                             "D_ENV_OS_FLAG_BSD_OPEN == 0x43",
                             "OpenBSD flag should be 0x43",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_BSD_OS == 0x44,
                             "D_ENV_OS_FLAG_BSD_OS == 0x44",
                             "BSD/OS flag should be 0x44",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify all BSD flags are in block 0x4
    if (!d_assert_standalone((D_ENV_OS_FLAG_BSD_DRAGONFLY >> 4) == 0x4,
                             "BSD flags are in block 0x4",
                             "all BSD flags should be in block 0x4",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    BSD flags: Dragon=0x%02X Free=0x%02X Net=0x%02X Open=0x%02X OS=0x%02X\n",
           D_INDENT,
           D_ENV_OS_FLAG_BSD_DRAGONFLY,
           D_ENV_OS_FLAG_BSD_FREE,
           D_ENV_OS_FLAG_BSD_NET,
           D_ENV_OS_FLAG_BSD_OPEN,
           D_ENV_OS_FLAG_BSD_OS);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] BSD OS flag constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] BSD OS flag constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_flag_constants_windows
  Tests Windows OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_MSDOS equals 0x60
  - Windows PC flags (0x61-0x67)
  - Windows Server flags (0x70-0x7A)
  - Windows Embedded/Mobile flags (0x80-0x81)
  - all Windows flags are in correct blocks
*/
bool
d_tests_sa_env_os_flag_constants_windows
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Windows OS Flag Constants ---\n", D_INDENT);

    // verify MS-DOS flag
    if (!d_assert_standalone(D_ENV_OS_FLAG_MSDOS == 0x60,
                             "D_ENV_OS_FLAG_MSDOS == 0x60",
                             "MS-DOS flag should be 0x60",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify Windows PC flags
    if (!d_assert_standalone(D_ENV_OS_FLAG_WIN_PC_PRE_XP == 0x61,
                             "D_ENV_OS_FLAG_WIN_PC_PRE_XP == 0x61",
                             "Windows pre-XP flag should be 0x61",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_WIN_PC_10 == 0x66,
                             "D_ENV_OS_FLAG_WIN_PC_10 == 0x66",
                             "Windows 10 flag should be 0x66",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_WIN_PC_11 == 0x67,
                             "D_ENV_OS_FLAG_WIN_PC_11 == 0x67",
                             "Windows 11 flag should be 0x67",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify Windows Server flags
    if (!d_assert_standalone(D_ENV_OS_FLAG_WIN_SERVER_NT == 0x70,
                             "D_ENV_OS_FLAG_WIN_SERVER_NT == 0x70",
                             "Windows NT Server flag should be 0x70",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_WIN_SERVER_2022 == 0x7A,
                             "D_ENV_OS_FLAG_WIN_SERVER_2022 == 0x7A",
                             "Windows Server 2022 flag should be 0x7A",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify Windows Embedded/Mobile flags
    if (!d_assert_standalone(D_ENV_OS_FLAG_WIN_EMBED == 0x80,
                             "D_ENV_OS_FLAG_WIN_EMBED == 0x80",
                             "Windows Embedded flag should be 0x80",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_WIN_MOBILE == 0x81,
                             "D_ENV_OS_FLAG_WIN_MOBILE == 0x81",
                             "Windows Mobile flag should be 0x81",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Windows flags: DOS=0x%02X PC_10=0x%02X PC_11=0x%02X\n",
           D_INDENT,
           D_ENV_OS_FLAG_MSDOS,
           D_ENV_OS_FLAG_WIN_PC_10,
           D_ENV_OS_FLAG_WIN_PC_11);

    printf("%s    Windows Server: NT=0x%02X 2022=0x%02X\n",
           D_INDENT,
           D_ENV_OS_FLAG_WIN_SERVER_NT,
           D_ENV_OS_FLAG_WIN_SERVER_2022);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Windows OS flag constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Windows OS flag constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_flag_constants_mobile
  Tests mobile OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_ANDROID equals 0xA0
  - D_ENV_OS_FLAG_BADA equals 0xA1
  - D_ENV_OS_FLAG_TIZEN equals 0xB0
*/
bool
d_tests_sa_env_os_flag_constants_mobile
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Mobile OS Flag Constants ---\n", D_INDENT);

    // verify mobile OS flags
    if (!d_assert_standalone(D_ENV_OS_FLAG_ANDROID == 0xA0,
                             "D_ENV_OS_FLAG_ANDROID == 0xA0",
                             "Android flag should be 0xA0",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_BADA == 0xA1,
                             "D_ENV_OS_FLAG_BADA == 0xA1",
                             "Bada flag should be 0xA1",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_TIZEN == 0xB0,
                             "D_ENV_OS_FLAG_TIZEN == 0xB0",
                             "Tizen flag should be 0xB0",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Mobile flags: ANDROID=0x%02X BADA=0x%02X TIZEN=0x%02X\n",
           D_INDENT,
           D_ENV_OS_FLAG_ANDROID,
           D_ENV_OS_FLAG_BADA,
           D_ENV_OS_FLAG_TIZEN);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Mobile OS flag constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Mobile OS flag constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_flag_constants_legacy
  Tests legacy OS flag constant definitions.
  Tests the following:
  - D_ENV_OS_FLAG_AIX equals 0xE0
  - D_ENV_OS_FLAG_HP_UX equals 0xE6
  - D_ENV_OS_FLAG_IRIX equals 0xE8
  - D_ENV_OS_FLAG_QNX equals 0xF3
  - D_ENV_OS_FLAG_VMS equals 0xF7
  - D_ENV_OS_FLAG_UNKNOWN equals 0xFF
*/
bool
d_tests_sa_env_os_flag_constants_legacy
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Legacy OS Flag Constants ---\n", D_INDENT);

    // verify legacy OS flags
    if (!d_assert_standalone(D_ENV_OS_FLAG_AIX == 0xE0,
                             "D_ENV_OS_FLAG_AIX == 0xE0",
                             "AIX flag should be 0xE0",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_HP_UX == 0xE6,
                             "D_ENV_OS_FLAG_HP_UX == 0xE6",
                             "HP-UX flag should be 0xE6",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_IRIX == 0xE8,
                             "D_ENV_OS_FLAG_IRIX == 0xE8",
                             "IRIX flag should be 0xE8",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_QNX == 0xF3,
                             "D_ENV_OS_FLAG_QNX == 0xF3",
                             "QNX flag should be 0xF3",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_VMS == 0xF7,
                             "D_ENV_OS_FLAG_VMS == 0xF7",
                             "VMS flag should be 0xF7",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_UNKNOWN == 0xFF,
                             "D_ENV_OS_FLAG_UNKNOWN == 0xFF",
                             "Unknown OS flag should be 0xFF",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Legacy flags: AIX=0x%02X HP_UX=0x%02X IRIX=0x%02X\n",
           D_INDENT,
           D_ENV_OS_FLAG_AIX,
           D_ENV_OS_FLAG_HP_UX,
           D_ENV_OS_FLAG_IRIX);

    printf("%s    Legacy flags: QNX=0x%02X VMS=0x%02X UNKNOWN=0x%02X\n",
           D_INDENT,
           D_ENV_OS_FLAG_QNX,
           D_ENV_OS_FLAG_VMS,
           D_ENV_OS_FLAG_UNKNOWN);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Legacy OS flag constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Legacy OS flag constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_flag_constants_all
  Runs all OS flag constant tests.
  Tests the following:
  - Apple OS flags
  - Unix OS flags
  - BSD OS flags
  - Windows OS flags
  - Mobile OS flags
  - Legacy OS flags
*/
bool
d_tests_sa_env_os_flag_constants_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter subsection_counter;
    bool apple_result;
    bool unix_result;
    bool bsd_result;
    bool windows_result;
    bool mobile_result;
    bool legacy_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    subsection_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[SUBSECTION] Testing OS Flag Constants\n");
    printf("----------------------------------------"
           "----------------------------------------\n");

    apple_result   = d_tests_sa_env_os_flag_constants_apple(&subsection_counter);
    unix_result    = d_tests_sa_env_os_flag_constants_unix(&subsection_counter);
    bsd_result     = d_tests_sa_env_os_flag_constants_bsd(&subsection_counter);
    windows_result = d_tests_sa_env_os_flag_constants_windows(&subsection_counter);
    mobile_result  = d_tests_sa_env_os_flag_constants_mobile(&subsection_counter);
    legacy_result  = d_tests_sa_env_os_flag_constants_legacy(&subsection_counter);

    // update totals
    _test_info->assertions_total  += subsection_counter.assertions_total;
    _test_info->assertions_passed += subsection_counter.assertions_passed;
    _test_info->tests_total         += subsection_counter.tests_total;
    _test_info->tests_passed      += subsection_counter.tests_passed;

    overall_result = ( apple_result   &&
                       unix_result    &&
                       bsd_result     &&
                       windows_result &&
                       mobile_result  &&
                       legacy_result );

    if (overall_result)
    {
        printf("[PASS] OS Flag Constants: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }
    else
    {
        printf("[FAIL] OS Flag Constants: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }

    return overall_result;
}


/******************************************************************************
* UTILITY CONSTANTS TESTS
******************************************************************************/

/*
d_tests_sa_env_os_utility_constants
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
bool
d_tests_sa_env_os_utility_constants
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing OS Utility Constants ---\n", D_INDENT);

    // verify block size
    if (!d_assert_standalone(D_ENV_OS_BLOCK_SIZE == 4,
                             "D_ENV_OS_BLOCK_SIZE == 4",
                             "block size should be 4 bits",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify discontinued range
    if (!d_assert_standalone(D_ENV_OS_FLAG_DISCONTINUED_FIRST == D_ENV_OS_FLAG_APOLLO_AEGIS,
                             "DISCONTINUED_FIRST equals APOLLO_AEGIS",
                             "discontinued range should start at Apollo Aegis",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_DISCONTINUED_LAST == D_ENV_OS_FLAG_WINDU,
                             "DISCONTINUED_LAST equals WINDU",
                             "discontinued range should end at Windu",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify Microsoft vendor range
    if (!d_assert_standalone(D_ENV_OS_VENDOR_MS_FIRST == D_ENV_OS_FLAG_MSDOS,
                             "VENDOR_MS_FIRST equals MSDOS",
                             "Microsoft range should start at MS-DOS",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_VENDOR_MS_LAST == D_ENV_OS_FLAG_WIN_MOBILE,
                             "VENDOR_MS_LAST equals WIN_MOBILE",
                             "Microsoft range should end at Windows Mobile",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify Windows-specific range
    if (!d_assert_standalone(D_ENV_OS_FLAG_WIN_FIRST == D_ENV_OS_FLAG_WIN_PC_PRE_XP,
                             "WIN_FIRST equals WIN_PC_PRE_XP",
                             "Windows range should start at pre-XP",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_WIN_LAST == D_ENV_OS_FLAG_WIN_MOBILE,
                             "WIN_LAST equals WIN_MOBILE",
                             "Windows range should end at Windows Mobile",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify ranges are consistent
    if (!d_assert_standalone(D_ENV_OS_FLAG_DISCONTINUED_FIRST <= D_ENV_OS_FLAG_DISCONTINUED_LAST,
                             "discontinued range is valid",
                             "range first should be <= last",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_VENDOR_MS_FIRST <= D_ENV_OS_VENDOR_MS_LAST,
                             "Microsoft vendor range is valid",
                             "range first should be <= last",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_OS_FLAG_WIN_FIRST <= D_ENV_OS_FLAG_WIN_LAST,
                             "Windows range is valid",
                             "range first should be <= last",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Block size: %d\n", D_INDENT, D_ENV_OS_BLOCK_SIZE);
    printf("%s    Discontinued range: 0x%02X-0x%02X\n",
           D_INDENT,
           D_ENV_OS_FLAG_DISCONTINUED_FIRST,
           D_ENV_OS_FLAG_DISCONTINUED_LAST);
    printf("%s    Windows range: 0x%02X-0x%02X\n",
           D_INDENT,
           D_ENV_OS_FLAG_WIN_FIRST,
           D_ENV_OS_FLAG_WIN_LAST);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] OS utility constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] OS utility constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* UTILITY MACROS TESTS
******************************************************************************/

/*
d_tests_sa_env_os_utility_macro_block
  Tests D_ENV_IS_OS_FLAG_IN_BLOCK macro.
  Tests the following:
  - macro correctly identifies OS flags in specific blocks
  - block detection works for all major OS categories
  - macro returns boolean values
*/
bool
d_tests_sa_env_os_utility_macro_block
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_ENV_IS_OS_FLAG_IN_BLOCK Macro ---\n", D_INDENT);

    // test block 0x0 (Apple)
    if (!d_assert_standalone(D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_APPLE, 0x0),
                             "APPLE is in block 0x0",
                             "Apple flag should be in block 0",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_MACOS, 0x0),
                             "MACOS is in block 0x0",
                             "macOS flag should be in block 0",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test block 0x1 (Unix/Linux)
    if (!d_assert_standalone(D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_UNIX, 0x1),
                             "UNIX is in block 0x1",
                             "Unix flag should be in block 1",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_LINUX, 0x1),
                             "LINUX is in block 0x1",
                             "Linux flag should be in block 1",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test block 0x4 (BSD)
    if (!d_assert_standalone(D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_BSD_FREE, 0x4),
                             "BSD_FREE is in block 0x4",
                             "FreeBSD flag should be in block 4",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test block 0x6 (MS-DOS/Windows PC)
    if (!d_assert_standalone(D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_MSDOS, 0x6),
                             "MSDOS is in block 0x6",
                             "MS-DOS flag should be in block 6",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_WIN_PC_10, 0x6),
                             "WIN_PC_10 is in block 0x6",
                             "Windows 10 flag should be in block 6",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test negative cases
    if (!d_assert_standalone(!D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_FLAG_LINUX, 0x4),
                             "LINUX is not in block 0x4",
                             "Linux should not be in BSD block",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Block detection working correctly\n", D_INDENT);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Block detection macro test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Block detection macro test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_utility_macro_unix
  Tests D_ENV_IS_OS_FLAG_UNIX macro.
  Tests the following:
  - macro correctly identifies Unix flags (block 0x1)
  - macro returns false for non-Unix OS flags
*/
bool
d_tests_sa_env_os_utility_macro_unix
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_ENV_IS_OS_FLAG_UNIX Macro ---\n", D_INDENT);

    // test Unix flags
    if (!d_assert_standalone(D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_FLAG_UNIX),
                             "UNIX is Unix-like",
                             "Unix flag should be detected as Unix",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_FLAG_LINUX),
                             "LINUX is Unix-like",
                             "Linux flag should be detected as Unix",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test non-Unix flags
    if (!d_assert_standalone(!D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_FLAG_WIN_PC_10),
                             "Windows is not Unix-like",
                             "Windows should not be detected as Unix",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(!D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_FLAG_MACOS),
                             "macOS is not in Unix block",
                             "macOS is Unix-like but not in Unix block",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Unix detection: UNIX=%d LINUX=%d WIN=%d\n",
           D_INDENT,
           D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_FLAG_UNIX),
           D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_FLAG_LINUX),
           D_ENV_IS_OS_FLAG_UNIX(D_ENV_OS_FLAG_WIN_PC_10));

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Unix detection macro test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Unix detection macro test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_utility_macro_mobile
  Tests D_ENV_IS_OS_MOBILE macro.
  Tests the following:
  - macro correctly identifies mobile OS flags
  - macro returns false for non-mobile OS flags
*/
bool
d_tests_sa_env_os_utility_macro_mobile
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_ENV_IS_OS_MOBILE Macro ---\n", D_INDENT);

    // test mobile OS flags
    if (!d_assert_standalone(D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_IOS),
                             "iOS is mobile",
                             "iOS flag should be detected as mobile",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_ANDROID),
                             "Android is mobile",
                             "Android flag should be detected as mobile",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_BADA),
                             "Bada is mobile",
                             "Bada flag should be detected as mobile",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test non-mobile OS flags
    if (!d_assert_standalone(!D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_LINUX),
                             "Linux is not mobile",
                             "Linux should not be detected as mobile",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(!D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_MACOS),
                             "macOS is not mobile",
                             "macOS should not be detected as mobile",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(!D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_TIZEN),
                             "Tizen is not in mobile macro",
                             "Tizen is mobile but not in macro list",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Mobile detection: iOS=%d Android=%d Linux=%d\n",
           D_INDENT,
           D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_IOS),
           D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_ANDROID),
           D_ENV_IS_OS_MOBILE(D_ENV_OS_FLAG_LINUX));

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Mobile detection macro test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Mobile detection macro test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_utility_macro_windows
  Tests D_ENV_IS_OS_WINDOWS and D_ENV_IS_OS_MSDOS macros.
  Tests the following:
  - D_ENV_IS_OS_WINDOWS correctly identifies Windows variants
  - D_ENV_IS_OS_MSDOS correctly identifies MS-DOS
  - macros return false for non-Windows OS flags
*/
bool
d_tests_sa_env_os_utility_macro_windows
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Windows Detection Macros ---\n", D_INDENT);

    // test Windows flags
    if (!d_assert_standalone(D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_WIN_PC_10),
                             "WIN_PC_10 is Windows",
                             "Windows 10 should be detected as Windows",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_WIN_SERVER_2022),
                             "WIN_SERVER_2022 is Windows",
                             "Windows Server should be detected as Windows",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_WIN_MOBILE),
                             "WIN_MOBILE is Windows",
                             "Windows Mobile should be detected as Windows",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test MS-DOS is not Windows (separate macro)
    if (!d_assert_standalone(!D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_MSDOS),
                             "MSDOS is not Windows",
                             "MS-DOS should not be detected as Windows",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test MS-DOS macro
    if (!d_assert_standalone(D_ENV_IS_OS_MSDOS(D_ENV_OS_FLAG_MSDOS),
                             "MSDOS is MS-DOS",
                             "MS-DOS flag should be detected",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(!D_ENV_IS_OS_MSDOS(D_ENV_OS_FLAG_WIN_PC_10),
                             "Windows is not MS-DOS",
                             "Windows should not be detected as MS-DOS",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test non-Windows flags
    if (!d_assert_standalone(!D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_LINUX),
                             "Linux is not Windows",
                             "Linux should not be detected as Windows",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Windows detection: Win10=%d Server=%d Linux=%d\n",
           D_INDENT,
           D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_WIN_PC_10),
           D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_WIN_SERVER_2022),
           D_ENV_IS_OS_WINDOWS(D_ENV_OS_FLAG_LINUX));

    printf("%s    MS-DOS detection: DOS=%d Win10=%d\n",
           D_INDENT,
           D_ENV_IS_OS_MSDOS(D_ENV_OS_FLAG_MSDOS),
           D_ENV_IS_OS_MSDOS(D_ENV_OS_FLAG_WIN_PC_10));

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Windows detection macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Windows detection macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_utility_macro_discontinued
  Tests D_ENV_IS_OS_DISCONTINUED and D_ENV_IS_OS_UNSUPPORTED macros.
  Tests the following:
  - D_ENV_IS_OS_DISCONTINUED correctly identifies discontinued OS flags
  - D_ENV_IS_OS_UNSUPPORTED correctly identifies unsupported OS flags
  - macros return false for modern/supported OS flags
*/
bool
d_tests_sa_env_os_utility_macro_discontinued
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Discontinued/Unsupported Macros ---\n", D_INDENT);

    // test discontinued flags
    if (!d_assert_standalone(D_ENV_IS_OS_DISCONTINUED(D_ENV_OS_FLAG_APOLLO_AEGIS),
                             "APOLLO_AEGIS is discontinued",
                             "Apollo Aegis should be discontinued",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_DISCONTINUED(D_ENV_OS_FLAG_BEOS),
                             "BEOS is discontinued",
                             "BeOS should be discontinued",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test modern OS is not discontinued
    if (!d_assert_standalone(!D_ENV_IS_OS_DISCONTINUED(D_ENV_OS_FLAG_LINUX),
                             "Linux is not discontinued",
                             "Linux should not be discontinued",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test unsupported includes discontinued
    if (!d_assert_standalone(D_ENV_IS_OS_UNSUPPORTED(D_ENV_OS_FLAG_APOLLO_AEGIS),
                             "APOLLO_AEGIS is unsupported",
                             "Apollo Aegis should be unsupported",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test unsupported includes legacy systems
    if (!d_assert_standalone(D_ENV_IS_OS_UNSUPPORTED(D_ENV_OS_FLAG_AIX),
                             "AIX is unsupported",
                             "AIX should be unsupported",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test modern OS is not unsupported
    if (!d_assert_standalone(!D_ENV_IS_OS_UNSUPPORTED(D_ENV_OS_FLAG_LINUX),
                             "Linux is not unsupported",
                             "Linux should not be unsupported",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Discontinued: BeOS=%d Linux=%d\n",
           D_INDENT,
           D_ENV_IS_OS_DISCONTINUED(D_ENV_OS_FLAG_BEOS),
           D_ENV_IS_OS_DISCONTINUED(D_ENV_OS_FLAG_LINUX));

    printf("%s    Unsupported: AIX=%d Linux=%d\n",
           D_INDENT,
           D_ENV_IS_OS_UNSUPPORTED(D_ENV_OS_FLAG_AIX),
           D_ENV_IS_OS_UNSUPPORTED(D_ENV_OS_FLAG_LINUX));

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Discontinued/unsupported macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Discontinued/unsupported macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_utility_macro_posix
  Tests D_ENV_IS_OS_POSIX_COMPLIANT macro.
  Tests the following:
  - macro correctly identifies POSIX-compliant OS flags
  - Unix-like systems are POSIX-compliant
  - BSD systems are POSIX-compliant
  - macOS and Android are POSIX-compliant
  - Windows is not POSIX-compliant
*/
bool
d_tests_sa_env_os_utility_macro_posix
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_ENV_IS_OS_POSIX_COMPLIANT Macro ---\n", D_INDENT);

    // test POSIX-compliant systems
    if (!d_assert_standalone(D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_UNIX),
                             "Unix is POSIX-compliant",
                             "Unix should be POSIX-compliant",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_LINUX),
                             "Linux is POSIX-compliant",
                             "Linux should be POSIX-compliant",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_BSD_FREE),
                             "FreeBSD is POSIX-compliant",
                             "FreeBSD should be POSIX-compliant",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_MACOS),
                             "macOS is POSIX-compliant",
                             "macOS should be POSIX-compliant",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_ANDROID),
                             "Android is POSIX-compliant",
                             "Android should be POSIX-compliant",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_SOLARIS),
                             "Solaris is POSIX-compliant",
                             "Solaris should be POSIX-compliant",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test non-POSIX systems
    if (!d_assert_standalone(!D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_WIN_PC_10),
                             "Windows is not POSIX-compliant",
                             "Windows should not be POSIX-compliant",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(!D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_MSDOS),
                             "MS-DOS is not POSIX-compliant",
                             "MS-DOS should not be POSIX-compliant",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    POSIX: Unix=%d Linux=%d BSD=%d macOS=%d Android=%d Win=%d\n",
           D_INDENT,
           D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_UNIX),
           D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_LINUX),
           D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_BSD_FREE),
           D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_MACOS),
           D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_ANDROID),
           D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_FLAG_WIN_PC_10));

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] POSIX compliance macro test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] POSIX compliance macro test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_utility_macros_all
  Runs all OS utility macro tests.
  Tests the following:
  - block detection macro
  - Unix detection macro
  - mobile detection macro
  - Windows detection macros
  - discontinued/unsupported macros
  - POSIX compliance macro
*/
bool
d_tests_sa_env_os_utility_macros_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter subsection_counter;
    bool block_result;
    bool unix_result;
    bool mobile_result;
    bool windows_result;
    bool discontinued_result;
    bool posix_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    subsection_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[SUBSECTION] Testing OS Utility Macros\n");
    printf("----------------------------------------"
           "----------------------------------------\n");

    block_result        = d_tests_sa_env_os_utility_macro_block(&subsection_counter);
    unix_result         = d_tests_sa_env_os_utility_macro_unix(&subsection_counter);
    mobile_result       = d_tests_sa_env_os_utility_macro_mobile(&subsection_counter);
    windows_result      = d_tests_sa_env_os_utility_macro_windows(&subsection_counter);
    discontinued_result = d_tests_sa_env_os_utility_macro_discontinued(&subsection_counter);
    posix_result        = d_tests_sa_env_os_utility_macro_posix(&subsection_counter);

    // update totals
    _test_info->assertions_total  += subsection_counter.assertions_total;
    _test_info->assertions_passed += subsection_counter.assertions_passed;
    _test_info->tests_total         += subsection_counter.tests_total;
    _test_info->tests_passed      += subsection_counter.tests_passed;

    overall_result = ( block_result        &&
                       unix_result         &&
                       mobile_result       &&
                       windows_result      &&
                       discontinued_result &&
                       posix_result );

    if (overall_result)
    {
        printf("[PASS] OS Utility Macros: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }
    else
    {
        printf("[FAIL] OS Utility Macros: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }

    return overall_result;
}


/******************************************************************************
* OS DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_os_detection_macros
  Tests that OS detection macros are defined.
  Tests the following:
  - D_ENV_OS_ID is defined
  - D_ENV_OS_NAME is defined
  - D_ENV_OS_ID has a valid value
*/
bool
d_tests_sa_env_os_detection_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing OS Detection Macros ---\n", D_INDENT);

    // verify D_ENV_OS_ID is defined
    #ifdef D_ENV_OS_ID
        if (!d_assert_standalone(true,
                                 "D_ENV_OS_ID is defined",
                                 "OS ID should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_OS_ID = 0x%02X\n", D_INDENT, D_ENV_OS_ID);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_OS_ID should be defined",
                                 "OS ID should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_OS_NAME is defined
    #ifdef D_ENV_OS_NAME
        if (!d_assert_standalone(true,
                                 "D_ENV_OS_NAME is defined",
                                 "OS name should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_OS_NAME = \"%s\"\n", D_INDENT, D_ENV_OS_NAME);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_OS_NAME should be defined",
                                 "OS name should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] OS detection macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] OS detection macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_os_platform_macros
  Tests legacy platform detection macros.
  Tests the following:
  - exactly one D_ENV_PLATFORM_* macro is defined
  - platform macro matches OS detection
*/
bool
d_tests_sa_env_os_platform_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    platform_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Platform Detection Macros ---\n", D_INDENT);

    // count platform macros
    platform_count = 0;

    #ifdef D_ENV_PLATFORM_ANDROID
        platform_count++;
        printf("%s    D_ENV_PLATFORM_ANDROID is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_PLATFORM_WINDOWS
        platform_count++;
        printf("%s    D_ENV_PLATFORM_WINDOWS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_PLATFORM_LINUX
        platform_count++;
        printf("%s    D_ENV_PLATFORM_LINUX is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_PLATFORM_MACOS
        platform_count++;
        printf("%s    D_ENV_PLATFORM_MACOS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_PLATFORM_UNIX
        platform_count++;
        printf("%s    D_ENV_PLATFORM_UNIX is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_PLATFORM_UNKNOWN
        platform_count++;
        printf("%s    D_ENV_PLATFORM_UNKNOWN is defined\n", D_INDENT);
    #endif

    // verify exactly one platform is defined
    if (!d_assert_standalone(platform_count == 1,
                             "exactly one platform macro defined",
                             "one and only one platform should be defined",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    ERROR: %d platforms defined (expected 1)\n",
               D_INDENT,
               platform_count);
    }

    #ifdef D_ENV_PLATFORM_NAME
        printf("%s    D_ENV_PLATFORM_NAME = \"%s\"\n",
               D_INDENT,
               D_ENV_PLATFORM_NAME);
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Platform detection macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Platform detection macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* MODULE AGGREGATOR
******************************************************************************/

/*
d_tests_sa_env_os_all
  Runs all OS detection tests.
  Tests the following:
  - OS flag constants
  - OS utility constants
  - OS utility macros
  - OS detection
  - platform detection
*/
bool
d_tests_sa_env_os_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool flag_constants_result;
    bool utility_constants_result;
    bool utility_macros_result;
    bool detection_result;
    bool platform_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[MODULE] Testing Operating System Detection\n");
    printf("========================================="
           "=======================================\n");

    flag_constants_result    = d_tests_sa_env_os_flag_constants_all(&module_counter);
    utility_constants_result = d_tests_sa_env_os_utility_constants(&module_counter);
    utility_macros_result    = d_tests_sa_env_os_utility_macros_all(&module_counter);
    detection_result         = d_tests_sa_env_os_detection_macros(&module_counter);
    platform_result          = d_tests_sa_env_os_platform_macros(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total         += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( flag_constants_result    &&
                       utility_constants_result &&
                       utility_macros_result    &&
                       detection_result         &&
                       platform_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] OS Detection Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] OS Detection Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Flag Constants:     %s\n",
               flag_constants_result ? "PASSED" : "FAILED");
        printf("  - Utility Constants:  %s\n",
               utility_constants_result ? "PASSED" : "FAILED");
        printf("  - Utility Macros:     %s\n",
               utility_macros_result ? "PASSED" : "FAILED");
        printf("  - OS Detection:       %s\n",
               detection_result ? "PASSED" : "FAILED");
        printf("  - Platform Detection: %s\n",
               platform_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
