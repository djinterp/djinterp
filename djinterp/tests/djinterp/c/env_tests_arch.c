/*******************************************************************************
* djinterp [test]                                               env_tests_arch.c
*
* Unit tests for `env.h` architecture detection (section VI).
* Tests the D_ENV_ARCH_* macros that detect CPU architectures and properties.
* Note: DTest's C face stands on this module (through djinterp.h), so a
* fault here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/env_tests_arch.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.24
*                                                            revised: 2026.11.06
*******************************************************************************/

// djinterp
#include "env_tests.h"


/******************************************************************************
* ARCHITECTURE TYPE CONSTANTS TESTS
******************************************************************************/

/*
d_tests_env_arch_type_constants
  Tests architecture type constant definitions.
  Tests the following:
  - D_ENV_ARCH_TYPE_X86 equals 0
  - D_ENV_ARCH_TYPE_X64 equals 1
  - D_ENV_ARCH_TYPE_ARM equals 2
  - D_ENV_ARCH_TYPE_ARM64 equals 3
  - D_ENV_ARCH_TYPE_RISCV equals 4
  - D_ENV_ARCH_TYPE_POWERPC equals 5
  - D_ENV_ARCH_TYPE_MIPS equals 6
  - D_ENV_ARCH_TYPE_SPARC equals 7
  - D_ENV_ARCH_TYPE_S390 equals 8
  - D_ENV_ARCH_TYPE_IA64 equals 9
  - D_ENV_ARCH_TYPE_ALPHA equals 10
  - D_ENV_ARCH_TYPE_UNKNOWN equals 11
  - all constants are unique
*/
bool d_tests_env_arch_type_constants(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    type_values[12];
    int    i;
    int    j;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_arch_type_constants");

    all_assertions_passed = true;

    // verify each type constant has correct value
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_X86 == 0",
                          D_ENV_ARCH_TYPE_X86 == 0,
                          "x86 type should be 0"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_X64 == 1",
                          D_ENV_ARCH_TYPE_X64 == 1,
                          "x64 type should be 1"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_ARM == 2",
                          D_ENV_ARCH_TYPE_ARM == 2,
                          "ARM type should be 2"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_ARM64 == 3",
                          D_ENV_ARCH_TYPE_ARM64 == 3,
                          "ARM64 type should be 3"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_RISCV == 4",
                          D_ENV_ARCH_TYPE_RISCV == 4,
                          "RISC-V type should be 4"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_POWERPC == 5",
                          D_ENV_ARCH_TYPE_POWERPC == 5,
                          "PowerPC type should be 5"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_MIPS == 6",
                          D_ENV_ARCH_TYPE_MIPS == 6,
                          "MIPS type should be 6"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_SPARC == 7",
                          D_ENV_ARCH_TYPE_SPARC == 7,
                          "SPARC type should be 7"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_S390 == 8",
                          D_ENV_ARCH_TYPE_S390 == 8,
                          "S390 type should be 8"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_IA64 == 9",
                          D_ENV_ARCH_TYPE_IA64 == 9,
                          "IA64 type should be 9"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_ALPHA == 10",
                          D_ENV_ARCH_TYPE_ALPHA == 10,
                          "Alpha type should be 10"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE_UNKNOWN == 11",
                          D_ENV_ARCH_TYPE_UNKNOWN == 11,
                          "Unknown type should be 11"))
    {
        all_assertions_passed = false;
    }

    // collect all type values for uniqueness check
    type_values[0]  = D_ENV_ARCH_TYPE_X86;
    type_values[1]  = D_ENV_ARCH_TYPE_X64;
    type_values[2]  = D_ENV_ARCH_TYPE_ARM;
    type_values[3]  = D_ENV_ARCH_TYPE_ARM64;
    type_values[4]  = D_ENV_ARCH_TYPE_RISCV;
    type_values[5]  = D_ENV_ARCH_TYPE_POWERPC;
    type_values[6]  = D_ENV_ARCH_TYPE_MIPS;
    type_values[7]  = D_ENV_ARCH_TYPE_SPARC;
    type_values[8]  = D_ENV_ARCH_TYPE_S390;
    type_values[9]  = D_ENV_ARCH_TYPE_IA64;
    type_values[10] = D_ENV_ARCH_TYPE_ALPHA;
    type_values[11] = D_ENV_ARCH_TYPE_UNKNOWN;

    // verify all type constants are unique
    for (i = 0; i < 12; i++)
    {
        for (j = i + 1; j < 12; j++)
        {
            if (type_values[i] == type_values[j])
            {
                if (!D_TEST_CHECK_MSG(_suite,
                                      "architecture type constants are unique",
                                      false,
                                      "duplicate type constant value detected"))
                {
                    all_assertions_passed = false;
                }

                break;
            }
        }
    }

    return all_assertions_passed;
}


/******************************************************************************
* ENDIANNESS CONSTANTS TESTS
******************************************************************************/

/*
d_tests_env_arch_endian_constants
  Tests endianness constant definitions.
  Tests the following:
  - D_ENV_ARCH_ENDIAN_UNKNOWN equals 0
  - D_ENV_ARCH_ENDIAN_LITTLE equals 1
  - D_ENV_ARCH_ENDIAN_BIG equals 2
  - all constants are unique
*/
bool d_tests_env_arch_endian_constants(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_arch_endian_constants");

    all_assertions_passed = true;

    // verify each endianness constant has correct value
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_ENDIAN_UNKNOWN == 0",
                          D_ENV_ARCH_ENDIAN_UNKNOWN == 0,
                          "unknown endianness should be 0"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_ENDIAN_LITTLE == 1",
                          D_ENV_ARCH_ENDIAN_LITTLE == 1,
                          "little endian should be 1"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_ENDIAN_BIG == 2",
                          D_ENV_ARCH_ENDIAN_BIG == 2,
                          "big endian should be 2"))
    {
        all_assertions_passed = false;
    }

    // verify all constants are unique
    if (!D_TEST_CHECK_MSG(_suite,
                          "all endianness constants are unique",
                          (D_ENV_ARCH_ENDIAN_UNKNOWN != D_ENV_ARCH_ENDIAN_LITTLE) && (D_ENV_ARCH_ENDIAN_UNKNOWN != D_ENV_ARCH_ENDIAN_BIG) && (D_ENV_ARCH_ENDIAN_LITTLE != D_ENV_ARCH_ENDIAN_BIG),
                          "no two constants should have same value"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
* ARCHITECTURE DETECTION TESTS
******************************************************************************/

/*
d_tests_env_arch_detection_macros
  Tests that architecture detection macros are defined.
  Tests the following:
  - exactly one architecture family macro is defined
  - D_ENV_ARCH_NAME is defined
  - D_ENV_ARCH_TYPE is defined
  - D_ENV_ARCH_BITS is defined
  - D_ENV_ARCH_ENDIAN is defined
*/
bool d_tests_env_arch_detection_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    arch_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_arch_detection_macros");

    all_assertions_passed = true;

    // count how many architecture-specific macros are defined
    arch_count = 0;

    #ifdef D_ENV_ARCH_X86
        arch_count++;
    #endif

    #ifdef D_ENV_ARCH_X64
        arch_count++;
    #endif

    #ifdef D_ENV_ARCH_ARM
        arch_count++;
    #endif

    #ifdef D_ENV_ARCH_ARM64
        arch_count++;
    #endif

    #ifdef D_ENV_ARCH_RISCV
        arch_count++;
    #endif

    #ifdef D_ENV_ARCH_POWERPC
        arch_count++;
    #endif

    #ifdef D_ENV_ARCH_MIPS
        arch_count++;
    #endif

    #ifdef D_ENV_ARCH_SPARC
        arch_count++;
    #endif

    #ifdef D_ENV_ARCH_S390
        arch_count++;
    #endif

    #ifdef D_ENV_ARCH_IA64
        arch_count++;
    #endif

    #ifdef D_ENV_ARCH_ALPHA
        arch_count++;
    #endif

    #ifdef D_ENV_ARCH_UNKNOWN
        arch_count++;
    #endif

    // verify exactly one architecture is detected
    if (!D_TEST_CHECK_MSG(_suite,
                          "exactly one architecture detected",
                          arch_count == 1,
                          "one and only one architecture should be defined"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite,
                                "%d architectures detected (expected 1)",
                                arch_count);
    }

    // verify D_ENV_ARCH_NAME is defined
    #ifdef D_ENV_ARCH_NAME
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_NAME is defined",
                              true,
                              "architecture name should be defined"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_NAME should be defined",
                              false,
                              "architecture name should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_TYPE is defined
    #ifdef D_ENV_ARCH_TYPE
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_TYPE is defined",
                              true,
                              "architecture type should be defined"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_TYPE should be defined",
                              false,
                              "architecture type should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_BITS is defined
    #ifdef D_ENV_ARCH_BITS
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_BITS is defined",
                              true,
                              "architecture bit width should be defined"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_BITS should be defined",
                              false,
                              "architecture bit width should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_ENDIAN is defined
    #ifdef D_ENV_ARCH_ENDIAN
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_ENDIAN is defined",
                              true,
                              "architecture endianness should be defined"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_ENDIAN should be defined",
                              false,
                              "architecture endianness should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}

/*
d_tests_env_arch_property_values
  Tests that architecture property values are valid.
  Tests the following:
  - D_ENV_ARCH_TYPE is within valid range (0-11)
  - D_ENV_ARCH_BITS is valid (0, 32, or 64)
  - D_ENV_ARCH_ENDIAN is within valid range (0-2)
  - architecture type matches detected architecture
*/
bool d_tests_env_arch_property_values(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_arch_property_values");

    all_assertions_passed = true;

    // verify D_ENV_ARCH_TYPE is in valid range
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_TYPE is in valid range (0-11)",
                          (D_ENV_ARCH_TYPE >= D_ENV_ARCH_TYPE_X86) && (D_ENV_ARCH_TYPE <= D_ENV_ARCH_TYPE_UNKNOWN),
                          "architecture type must be valid constant"))
    {
        all_assertions_passed = false;
    }

    // verify D_ENV_ARCH_BITS is valid
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_BITS is valid (0, 32, or 64)",
                          (D_ENV_ARCH_BITS == 0) || (D_ENV_ARCH_BITS == 32) || (D_ENV_ARCH_BITS == 64),
                          "bit width must be 0 (unknown), 32, or 64"))
    {
        all_assertions_passed = false;
    }

    // verify D_ENV_ARCH_ENDIAN is in valid range
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_ARCH_ENDIAN is in valid range (0-2)",
                          (D_ENV_ARCH_ENDIAN >= D_ENV_ARCH_ENDIAN_UNKNOWN) && (D_ENV_ARCH_ENDIAN <= D_ENV_ARCH_ENDIAN_BIG),
                          "endianness must be valid constant"))
    {
        all_assertions_passed = false;
    }

    // verify architecture type matches detected architecture
    #ifdef D_ENV_ARCH_X86
        if (!D_TEST_CHECK_MSG(_suite,
                              "x86 architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X86,
                              "D_ENV_ARCH_TYPE should match x86"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_X64
        if (!D_TEST_CHECK_MSG(_suite,
                              "x64 architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64,
                              "D_ENV_ARCH_TYPE should match x64"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ARM
        if (!D_TEST_CHECK_MSG(_suite,
                              "ARM architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_ARM,
                              "D_ENV_ARCH_TYPE should match ARM"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ARM64
        if (!D_TEST_CHECK_MSG(_suite,
                              "ARM64 architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_ARM64,
                              "D_ENV_ARCH_TYPE should match ARM64"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_RISCV
        if (!D_TEST_CHECK_MSG(_suite,
                              "RISC-V architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_RISCV,
                              "D_ENV_ARCH_TYPE should match RISC-V"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_POWERPC
        if (!D_TEST_CHECK_MSG(_suite,
                              "PowerPC architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_POWERPC,
                              "D_ENV_ARCH_TYPE should match PowerPC"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_MIPS
        if (!D_TEST_CHECK_MSG(_suite,
                              "MIPS architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_MIPS,
                              "D_ENV_ARCH_TYPE should match MIPS"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_SPARC
        if (!D_TEST_CHECK_MSG(_suite,
                              "SPARC architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_SPARC,
                              "D_ENV_ARCH_TYPE should match SPARC"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_S390
        if (!D_TEST_CHECK_MSG(_suite,
                              "S390 architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_S390,
                              "D_ENV_ARCH_TYPE should match S390"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_IA64
        if (!D_TEST_CHECK_MSG(_suite,
                              "IA64 architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_IA64,
                              "D_ENV_ARCH_TYPE should match IA64"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ALPHA
        if (!D_TEST_CHECK_MSG(_suite,
                              "Alpha architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_ALPHA,
                              "D_ENV_ARCH_TYPE should match Alpha"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_UNKNOWN
        if (!D_TEST_CHECK_MSG(_suite,
                              "Unknown architecture has correct type",
                              D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_UNKNOWN,
                              "D_ENV_ARCH_TYPE should match Unknown"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}

/*
d_tests_env_arch_detection_all
  Runs all architecture detection tests.
  Tests the following:
  - architecture detection macros are defined
  - architecture property values are valid
*/
bool d_tests_env_arch_detection_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_arch_detection_all");

    d_tests_env_arch_detection_macros(_suite);
    d_tests_env_arch_property_values(_suite);

    return true;
}


/******************************************************************************
* ARCHITECTURE FAMILY MACROS TESTS
******************************************************************************/

/*
d_tests_env_arch_family_macros
  Tests architecture family classification macros.
  Tests the following:
  - D_ENV_ARCH_IS_X86_FAMILY is defined
  - D_ENV_ARCH_IS_ARM_FAMILY is defined
  - family macros have correct boolean values (0 or 1)
  - x86 and x64 are correctly classified as x86 family
  - ARM and ARM64 are correctly classified as ARM family
*/
bool d_tests_env_arch_family_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_arch_family_macros");

    all_assertions_passed = true;

    // verify D_ENV_ARCH_IS_X86_FAMILY is defined
    #ifdef D_ENV_ARCH_IS_X86_FAMILY
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_X86_FAMILY is defined",
                              true,
                              "x86 family macro should be defined"))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_X86_FAMILY is boolean (0 or 1)",
                              (D_ENV_ARCH_IS_X86_FAMILY == 0) || (D_ENV_ARCH_IS_X86_FAMILY == 1),
                              "family macro should be boolean"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_X86_FAMILY should be defined",
                              false,
                              "x86 family macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_IS_ARM_FAMILY is defined
    #ifdef D_ENV_ARCH_IS_ARM_FAMILY
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_ARM_FAMILY is defined",
                              true,
                              "ARM family macro should be defined"))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_ARM_FAMILY is boolean (0 or 1)",
                              (D_ENV_ARCH_IS_ARM_FAMILY == 0) || (D_ENV_ARCH_IS_ARM_FAMILY == 1),
                              "family macro should be boolean"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_ARM_FAMILY should be defined",
                              false,
                              "ARM family macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify x86 and x64 are classified as x86 family
    #if ( defined(D_ENV_ARCH_X86) ||  \
          defined(D_ENV_ARCH_X64) )
        if (!D_TEST_CHECK_MSG(_suite,
                              "x86/x64 classified as x86 family",
                              D_ENV_ARCH_IS_X86_FAMILY == 1,
                              "x86 and x64 should be x86 family"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "non-x86 architectures not classified as x86 family",
                              D_ENV_ARCH_IS_X86_FAMILY == 0,
                              "other architectures should not be x86 family"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify ARM and ARM64 are classified as ARM family
    #if ( defined(D_ENV_ARCH_ARM) ||  \
          defined(D_ENV_ARCH_ARM64) )
        if (!D_TEST_CHECK_MSG(_suite,
                              "ARM/ARM64 classified as ARM family",
                              D_ENV_ARCH_IS_ARM_FAMILY == 1,
                              "ARM and ARM64 should be ARM family"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "non-ARM architectures not classified as ARM family",
                              D_ENV_ARCH_IS_ARM_FAMILY == 0,
                              "other architectures should not be ARM family"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* BIT WIDTH MACROS TESTS
******************************************************************************/

/*
d_tests_env_arch_bitwidth_macros
  Tests bit width classification macros.
  Tests the following:
  - D_ENV_ARCH_IS_64BIT is defined
  - D_ENV_ARCH_IS_32BIT is defined
  - bit width macros have correct boolean values (0 or 1)
  - macros correctly reflect D_ENV_ARCH_BITS value
  - at most one bit width is active
*/
bool d_tests_env_arch_bitwidth_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_arch_bitwidth_macros");

    all_assertions_passed = true;

    // verify D_ENV_ARCH_IS_64BIT is defined
    #ifdef D_ENV_ARCH_IS_64BIT
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_64BIT is defined",
                              true,
                              "64-bit macro should be defined"))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_64BIT is boolean (0 or 1)",
                              (D_ENV_ARCH_IS_64BIT == 0) || (D_ENV_ARCH_IS_64BIT == 1),
                              "bit width macro should be boolean"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_64BIT should be defined",
                              false,
                              "64-bit macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_IS_32BIT is defined
    #ifdef D_ENV_ARCH_IS_32BIT
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_32BIT is defined",
                              true,
                              "32-bit macro should be defined"))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_32BIT is boolean (0 or 1)",
                              (D_ENV_ARCH_IS_32BIT == 0) || (D_ENV_ARCH_IS_32BIT == 1),
                              "bit width macro should be boolean"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_32BIT should be defined",
                              false,
                              "32-bit macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify macros reflect D_ENV_ARCH_BITS
    if (D_ENV_ARCH_BITS == 64)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "64-bit architecture correctly detected",
                              D_ENV_ARCH_IS_64BIT == 1,
                              "IS_64BIT should be 1 when BITS is 64"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_32BIT is 0 for 64-bit architecture",
                              D_ENV_ARCH_IS_32BIT == 0,
                              "IS_32BIT should be 0 when BITS is 64"))
        {
            all_assertions_passed = false;
        }
    }
    else if (D_ENV_ARCH_BITS == 32)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "32-bit architecture correctly detected",
                              D_ENV_ARCH_IS_32BIT == 1,
                              "IS_32BIT should be 1 when BITS is 32"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_64BIT is 0 for 32-bit architecture",
                              D_ENV_ARCH_IS_64BIT == 0,
                              "IS_64BIT should be 0 when BITS is 32"))
        {
            all_assertions_passed = false;
        }
    }
    else
    {
        // unknown bit width (0)
        if (!D_TEST_CHECK_MSG(_suite,
                              "both bit width macros are 0 for unknown",
                              (D_ENV_ARCH_IS_64BIT == 0) && (D_ENV_ARCH_IS_32BIT == 0),
                              "both should be 0 when BITS is 0"))
        {
            all_assertions_passed = false;
        }
    }

    // verify at most one bit width is active
    if (!D_TEST_CHECK_MSG(_suite,
                          "at most one bit width is active",
                          (D_ENV_ARCH_IS_64BIT + D_ENV_ARCH_IS_32BIT) <= 1,
                          "cannot be both 32-bit and 64-bit"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
* ENDIANNESS MACROS TESTS
******************************************************************************/

/*
d_tests_env_arch_endianness_macros
  Tests endianness classification macros.
  Tests the following:
  - D_ENV_ARCH_IS_LITTLE_ENDIAN is defined
  - D_ENV_ARCH_IS_BIG_ENDIAN is defined
  - endianness macros have correct boolean values (0 or 1)
  - macros correctly reflect D_ENV_ARCH_ENDIAN value
  - at most one endianness is active
*/
bool d_tests_env_arch_endianness_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_arch_endianness_macros");

    all_assertions_passed = true;

    // verify D_ENV_ARCH_IS_LITTLE_ENDIAN is defined
    #ifdef D_ENV_ARCH_IS_LITTLE_ENDIAN
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_LITTLE_ENDIAN is defined",
                              true,
                              "little endian macro should be defined"))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_LITTLE_ENDIAN is boolean (0 or 1)",
                              (D_ENV_ARCH_IS_LITTLE_ENDIAN == 0) || (D_ENV_ARCH_IS_LITTLE_ENDIAN == 1),
                              "endianness macro should be boolean"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_LITTLE_ENDIAN should be defined",
                              false,
                              "little endian macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_IS_BIG_ENDIAN is defined
    #ifdef D_ENV_ARCH_IS_BIG_ENDIAN
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_BIG_ENDIAN is defined",
                              true,
                              "big endian macro should be defined"))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_BIG_ENDIAN is boolean (0 or 1)",
                              (D_ENV_ARCH_IS_BIG_ENDIAN == 0) || (D_ENV_ARCH_IS_BIG_ENDIAN == 1),
                              "endianness macro should be boolean"))
        {
            all_assertions_passed = false;
        }

    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_ARCH_IS_BIG_ENDIAN should be defined",
                              false,
                              "big endian macro should always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify macros reflect D_ENV_ARCH_ENDIAN
    if (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "little endian architecture correctly detected",
                              D_ENV_ARCH_IS_LITTLE_ENDIAN == 1,
                              "IS_LITTLE_ENDIAN should be 1"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_BIG_ENDIAN is 0 for little endian",
                              D_ENV_ARCH_IS_BIG_ENDIAN == 0,
                              "IS_BIG_ENDIAN should be 0"))
        {
            all_assertions_passed = false;
        }
    }
    else if (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "big endian architecture correctly detected",
                              D_ENV_ARCH_IS_BIG_ENDIAN == 1,
                              "IS_BIG_ENDIAN should be 1"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_LITTLE_ENDIAN is 0 for big endian",
                              D_ENV_ARCH_IS_LITTLE_ENDIAN == 0,
                              "IS_LITTLE_ENDIAN should be 0"))
        {
            all_assertions_passed = false;
        }
    }
    else
    {
        // unknown endianness
        if (!D_TEST_CHECK_MSG(_suite,
                              "both endianness macros are 0 for unknown",
                              (D_ENV_ARCH_IS_LITTLE_ENDIAN == 0) && (D_ENV_ARCH_IS_BIG_ENDIAN == 0),
                              "both should be 0 for unknown endianness"))
        {
            all_assertions_passed = false;
        }
    }

    // verify at most one endianness is active
    if (!D_TEST_CHECK_MSG(_suite,
                          "at most one endianness is active",
                          (D_ENV_ARCH_IS_LITTLE_ENDIAN + D_ENV_ARCH_IS_BIG_ENDIAN) <= 1,
                          "cannot be both little and big endian"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_env_arch_utility_macros_all
  Runs all architecture utility macro tests.
  Tests the following:
  - architecture family macros
  - bit width macros
  - endianness macros
*/
bool d_tests_env_arch_utility_macros_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_arch_utility_macros_all");

    d_tests_env_arch_family_macros(_suite);
    d_tests_env_arch_bitwidth_macros(_suite);
    d_tests_env_arch_endianness_macros(_suite);

    return true;
}


/******************************************************************************
* ARCHITECTURE CONSISTENCY TESTS
******************************************************************************/

/*
d_tests_env_arch_consistency_check
  Tests consistency between architecture properties.
  Tests the following:
  - known architectures have non-zero bit width
  - known little-endian architectures are correctly classified
  - known big-endian architectures are correctly classified
  - 32-bit variants have correct bit width
  - 64-bit variants have correct bit width
*/
bool d_tests_env_arch_consistency_check(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_arch_consistency_check");

    all_assertions_passed = true;

    // verify known architectures (not unknown) have non-zero bit width
    #ifndef D_ENV_ARCH_UNKNOWN
        if (!D_TEST_CHECK_MSG(_suite,
                              "known architecture has non-zero bit width",
                              D_ENV_ARCH_BITS != 0,
                              "BITS should be 32 or 64 for known architectures"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify x86 family is little endian
    #if ( defined(D_ENV_ARCH_X86) ||  \
          defined(D_ENV_ARCH_X64) )
        if (!D_TEST_CHECK_MSG(_suite,
                              "x86 family is little endian",
                              D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE,
                              "x86 and x64 should be little endian"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify ARM family is little endian
    #if ( defined(D_ENV_ARCH_ARM) ||  \
          defined(D_ENV_ARCH_ARM64) )
        if (!D_TEST_CHECK_MSG(_suite,
                              "ARM family is little endian",
                              D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE,
                              "ARM and ARM64 should be little endian"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify RISC-V is little endian
    #ifdef D_ENV_ARCH_RISCV
        if (!D_TEST_CHECK_MSG(_suite,
                              "RISC-V is little endian",
                              D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE,
                              "RISC-V should be little endian"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify PowerPC is big endian
    #ifdef D_ENV_ARCH_POWERPC
        if (!D_TEST_CHECK_MSG(_suite,
                              "PowerPC is big endian",
                              D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG,
                              "PowerPC should be big endian"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify MIPS is big endian
    #ifdef D_ENV_ARCH_MIPS
        if (!D_TEST_CHECK_MSG(_suite,
                              "MIPS is big endian",
                              D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG,
                              "MIPS should be big endian"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify SPARC is big endian
    #ifdef D_ENV_ARCH_SPARC
        if (!D_TEST_CHECK_MSG(_suite,
                              "SPARC is big endian",
                              D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG,
                              "SPARC should be big endian"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify S390 is big endian
    #ifdef D_ENV_ARCH_S390
        if (!D_TEST_CHECK_MSG(_suite,
                              "S390 is big endian",
                              D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG,
                              "S390 should be big endian"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify IA64 is little endian
    #ifdef D_ENV_ARCH_IA64
        if (!D_TEST_CHECK_MSG(_suite,
                              "IA64 is little endian",
                              D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE,
                              "Itanium should be little endian"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify Alpha is little endian
    #ifdef D_ENV_ARCH_ALPHA
        if (!D_TEST_CHECK_MSG(_suite,
                              "Alpha is little endian",
                              D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE,
                              "Alpha should be little endian"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify 32-bit architectures
    #ifdef D_ENV_ARCH_X86
        if (!D_TEST_CHECK_MSG(_suite,
                              "x86 is 32-bit",
                              D_ENV_ARCH_BITS == 32,
                              "x86 should be 32-bit"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ARM
        if (!D_TEST_CHECK_MSG(_suite,
                              "ARM is 32-bit",
                              D_ENV_ARCH_BITS == 32,
                              "ARM should be 32-bit"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify 64-bit architectures
    #ifdef D_ENV_ARCH_X64
        if (!D_TEST_CHECK_MSG(_suite,
                              "x64 is 64-bit",
                              D_ENV_ARCH_BITS == 64,
                              "x64 should be 64-bit"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ARM64
        if (!D_TEST_CHECK_MSG(_suite,
                              "ARM64 is 64-bit",
                              D_ENV_ARCH_BITS == 64,
                              "ARM64 should be 64-bit"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_IA64
        if (!D_TEST_CHECK_MSG(_suite,
                              "IA64 is 64-bit",
                              D_ENV_ARCH_BITS == 64,
                              "Itanium should be 64-bit"))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ALPHA
        if (!D_TEST_CHECK_MSG(_suite,
                              "Alpha is 64-bit",
                              D_ENV_ARCH_BITS == 64,
                              "Alpha should be 64-bit"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* MODULE AGGREGATOR
******************************************************************************/

/*
d_tests_env_arch_all
  Runs all architecture detection tests.
  Tests the following:
  - architecture type constants
  - endianness constants
  - architecture detection
  - architecture utility macros
  - architecture consistency
*/
bool d_tests_env_arch_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_arch_all");

    d_tests_env_arch_type_constants(_suite);
    d_tests_env_arch_endian_constants(_suite);
    d_tests_env_arch_detection_all(_suite);
    d_tests_env_arch_utility_macros_all(_suite);
    d_tests_env_arch_consistency_check(_suite);

    return true;
}
