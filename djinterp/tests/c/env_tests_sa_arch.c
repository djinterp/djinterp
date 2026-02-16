/******************************************************************************
* djinterp [test]                                          env_tests_sa_arch.c
*
* Unit tests for `env.h` architecture detection (section VI).
* Tests the D_ENV_ARCH_* macros that detect CPU architectures and properties.
* Note: this module is required to build DTest, so it uses `test_standalone.h`.
*
* path:      \test\env_tests_sa_arch.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.24
******************************************************************************/

#include "env_tests_sa.h"


/******************************************************************************
* ARCHITECTURE TYPE CONSTANTS TESTS
******************************************************************************/

/*
d_tests_sa_env_arch_type_constants
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
bool
d_tests_sa_env_arch_type_constants
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    type_values[12];
    int    i;
    int    j;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Architecture Type Constants ---\n", D_INDENT);

    // verify each type constant has correct value
    if (!d_assert_standalone(D_ENV_ARCH_TYPE_X86 == 0,
                             "D_ENV_ARCH_TYPE_X86 == 0",
                             "x86 type should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_TYPE_X64 == 1,
                             "D_ENV_ARCH_TYPE_X64 == 1",
                             "x64 type should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_TYPE_ARM == 2,
                             "D_ENV_ARCH_TYPE_ARM == 2",
                             "ARM type should be 2",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_TYPE_ARM64 == 3,
                             "D_ENV_ARCH_TYPE_ARM64 == 3",
                             "ARM64 type should be 3",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_TYPE_RISCV == 4,
                             "D_ENV_ARCH_TYPE_RISCV == 4",
                             "RISC-V type should be 4",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_TYPE_POWERPC == 5,
                             "D_ENV_ARCH_TYPE_POWERPC == 5",
                             "PowerPC type should be 5",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_TYPE_MIPS == 6,
                             "D_ENV_ARCH_TYPE_MIPS == 6",
                             "MIPS type should be 6",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_TYPE_SPARC == 7,
                             "D_ENV_ARCH_TYPE_SPARC == 7",
                             "SPARC type should be 7",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_TYPE_S390 == 8,
                             "D_ENV_ARCH_TYPE_S390 == 8",
                             "S390 type should be 8",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_TYPE_IA64 == 9,
                             "D_ENV_ARCH_TYPE_IA64 == 9",
                             "IA64 type should be 9",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_TYPE_ALPHA == 10,
                             "D_ENV_ARCH_TYPE_ALPHA == 10",
                             "Alpha type should be 10",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_TYPE_UNKNOWN == 11,
                             "D_ENV_ARCH_TYPE_UNKNOWN == 11",
                             "Unknown type should be 11",
                             _test_info))
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
                if (!d_assert_standalone(false,
                                         "architecture type constants are unique",
                                         "duplicate type constant value detected",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }

                break;
            }
        }
    }

    printf("%s    Type constants: X86=%d X64=%d ARM=%d ARM64=%d RISCV=%d\n",
           D_INDENT,
           D_ENV_ARCH_TYPE_X86,
           D_ENV_ARCH_TYPE_X64,
           D_ENV_ARCH_TYPE_ARM,
           D_ENV_ARCH_TYPE_ARM64,
           D_ENV_ARCH_TYPE_RISCV);

    printf("%s    Type constants: PPC=%d MIPS=%d SPARC=%d S390=%d IA64=%d Alpha=%d Unknown=%d\n",
           D_INDENT,
           D_ENV_ARCH_TYPE_POWERPC,
           D_ENV_ARCH_TYPE_MIPS,
           D_ENV_ARCH_TYPE_SPARC,
           D_ENV_ARCH_TYPE_S390,
           D_ENV_ARCH_TYPE_IA64,
           D_ENV_ARCH_TYPE_ALPHA,
           D_ENV_ARCH_TYPE_UNKNOWN);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Architecture type constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Architecture type constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* ENDIANNESS CONSTANTS TESTS
******************************************************************************/

/*
d_tests_sa_env_arch_endian_constants
  Tests endianness constant definitions.
  Tests the following:
  - D_ENV_ARCH_ENDIAN_UNKNOWN equals 0
  - D_ENV_ARCH_ENDIAN_LITTLE equals 1
  - D_ENV_ARCH_ENDIAN_BIG equals 2
  - all constants are unique
*/
bool
d_tests_sa_env_arch_endian_constants
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

    printf("%s--- Testing Endianness Constants ---\n", D_INDENT);

    // verify each endianness constant has correct value
    if (!d_assert_standalone(D_ENV_ARCH_ENDIAN_UNKNOWN == 0,
                             "D_ENV_ARCH_ENDIAN_UNKNOWN == 0",
                             "unknown endianness should be 0",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_ENDIAN_LITTLE == 1,
                             "D_ENV_ARCH_ENDIAN_LITTLE == 1",
                             "little endian should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_ARCH_ENDIAN_BIG == 2,
                             "D_ENV_ARCH_ENDIAN_BIG == 2",
                             "big endian should be 2",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify all constants are unique
    if (!d_assert_standalone((D_ENV_ARCH_ENDIAN_UNKNOWN != D_ENV_ARCH_ENDIAN_LITTLE) &&
                             (D_ENV_ARCH_ENDIAN_UNKNOWN != D_ENV_ARCH_ENDIAN_BIG) &&
                             (D_ENV_ARCH_ENDIAN_LITTLE != D_ENV_ARCH_ENDIAN_BIG),
                             "all endianness constants are unique",
                             "no two constants should have same value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Endian constants: UNKNOWN=%d LITTLE=%d BIG=%d\n",
           D_INDENT,
           D_ENV_ARCH_ENDIAN_UNKNOWN,
           D_ENV_ARCH_ENDIAN_LITTLE,
           D_ENV_ARCH_ENDIAN_BIG);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Endianness constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Endianness constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* ARCHITECTURE DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_arch_detection_macros
  Tests that architecture detection macros are defined.
  Tests the following:
  - exactly one architecture family macro is defined
  - D_ENV_ARCH_NAME is defined
  - D_ENV_ARCH_TYPE is defined
  - D_ENV_ARCH_BITS is defined
  - D_ENV_ARCH_ENDIAN is defined
*/
bool
d_tests_sa_env_arch_detection_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    arch_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Architecture Detection Macros ---\n", D_INDENT);

    // count how many architecture-specific macros are defined
    arch_count = 0;

    #ifdef D_ENV_ARCH_X86
        arch_count++;
        printf("%s    D_ENV_ARCH_X86 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_ARCH_X64
        arch_count++;
        printf("%s    D_ENV_ARCH_X64 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_ARCH_ARM
        arch_count++;
        printf("%s    D_ENV_ARCH_ARM is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_ARCH_ARM64
        arch_count++;
        printf("%s    D_ENV_ARCH_ARM64 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_ARCH_RISCV
        arch_count++;
        printf("%s    D_ENV_ARCH_RISCV is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_ARCH_POWERPC
        arch_count++;
        printf("%s    D_ENV_ARCH_POWERPC is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_ARCH_MIPS
        arch_count++;
        printf("%s    D_ENV_ARCH_MIPS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_ARCH_SPARC
        arch_count++;
        printf("%s    D_ENV_ARCH_SPARC is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_ARCH_S390
        arch_count++;
        printf("%s    D_ENV_ARCH_S390 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_ARCH_IA64
        arch_count++;
        printf("%s    D_ENV_ARCH_IA64 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_ARCH_ALPHA
        arch_count++;
        printf("%s    D_ENV_ARCH_ALPHA is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_ARCH_UNKNOWN
        arch_count++;
        printf("%s    D_ENV_ARCH_UNKNOWN is defined\n", D_INDENT);
    #endif

    // verify exactly one architecture is detected
    if (!d_assert_standalone(arch_count == 1,
                             "exactly one architecture detected",
                             "one and only one architecture should be defined",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    ERROR: %d architectures detected (expected 1)\n",
               D_INDENT,
               arch_count);
    }

    // verify D_ENV_ARCH_NAME is defined
    #ifdef D_ENV_ARCH_NAME
        if (!d_assert_standalone(true,
                                 "D_ENV_ARCH_NAME is defined",
                                 "architecture name should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    Architecture name: %s\n", D_INDENT, D_ENV_ARCH_NAME);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_ARCH_NAME should be defined",
                                 "architecture name should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_TYPE is defined
    #ifdef D_ENV_ARCH_TYPE
        if (!d_assert_standalone(true,
                                 "D_ENV_ARCH_TYPE is defined",
                                 "architecture type should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    Architecture type: %d\n", D_INDENT, D_ENV_ARCH_TYPE);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_ARCH_TYPE should be defined",
                                 "architecture type should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_BITS is defined
    #ifdef D_ENV_ARCH_BITS
        if (!d_assert_standalone(true,
                                 "D_ENV_ARCH_BITS is defined",
                                 "architecture bit width should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    Architecture bits: %d\n", D_INDENT, D_ENV_ARCH_BITS);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_ARCH_BITS should be defined",
                                 "architecture bit width should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_ENDIAN is defined
    #ifdef D_ENV_ARCH_ENDIAN
        if (!d_assert_standalone(true,
                                 "D_ENV_ARCH_ENDIAN is defined",
                                 "architecture endianness should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    Architecture endianness: %d\n", D_INDENT, D_ENV_ARCH_ENDIAN);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_ARCH_ENDIAN should be defined",
                                 "architecture endianness should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Architecture detection macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Architecture detection macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_arch_property_values
  Tests that architecture property values are valid.
  Tests the following:
  - D_ENV_ARCH_TYPE is within valid range (0-11)
  - D_ENV_ARCH_BITS is valid (0, 32, or 64)
  - D_ENV_ARCH_ENDIAN is within valid range (0-2)
  - architecture type matches detected architecture
*/
bool
d_tests_sa_env_arch_property_values
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

    printf("%s--- Testing Architecture Property Values ---\n", D_INDENT);

    // verify D_ENV_ARCH_TYPE is in valid range
    if (!d_assert_standalone((D_ENV_ARCH_TYPE >= D_ENV_ARCH_TYPE_X86) &&
                             (D_ENV_ARCH_TYPE <= D_ENV_ARCH_TYPE_UNKNOWN),
                             "D_ENV_ARCH_TYPE is in valid range (0-11)",
                             "architecture type must be valid constant",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_ENV_ARCH_BITS is valid
    if (!d_assert_standalone((D_ENV_ARCH_BITS == 0) ||
                             (D_ENV_ARCH_BITS == 32) ||
                             (D_ENV_ARCH_BITS == 64),
                             "D_ENV_ARCH_BITS is valid (0, 32, or 64)",
                             "bit width must be 0 (unknown), 32, or 64",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_ENV_ARCH_ENDIAN is in valid range
    if (!d_assert_standalone((D_ENV_ARCH_ENDIAN >= D_ENV_ARCH_ENDIAN_UNKNOWN) &&
                             (D_ENV_ARCH_ENDIAN <= D_ENV_ARCH_ENDIAN_BIG),
                             "D_ENV_ARCH_ENDIAN is in valid range (0-2)",
                             "endianness must be valid constant",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify architecture type matches detected architecture
    #ifdef D_ENV_ARCH_X86
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X86,
                                 "x86 architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match x86",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_X64
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_X64,
                                 "x64 architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match x64",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ARM
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_ARM,
                                 "ARM architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match ARM",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ARM64
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_ARM64,
                                 "ARM64 architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match ARM64",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_RISCV
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_RISCV,
                                 "RISC-V architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match RISC-V",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_POWERPC
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_POWERPC,
                                 "PowerPC architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match PowerPC",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_MIPS
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_MIPS,
                                 "MIPS architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match MIPS",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_SPARC
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_SPARC,
                                 "SPARC architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match SPARC",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_S390
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_S390,
                                 "S390 architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match S390",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_IA64
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_IA64,
                                 "IA64 architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match IA64",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ALPHA
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_ALPHA,
                                 "Alpha architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match Alpha",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_UNKNOWN
        if (!d_assert_standalone(D_ENV_ARCH_TYPE == D_ENV_ARCH_TYPE_UNKNOWN,
                                 "Unknown architecture has correct type",
                                 "D_ENV_ARCH_TYPE should match Unknown",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    printf("%s    Properties: type=%d bits=%d endian=%d\n",
           D_INDENT,
           D_ENV_ARCH_TYPE,
           D_ENV_ARCH_BITS,
           D_ENV_ARCH_ENDIAN);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Architecture property values test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Architecture property values test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_arch_detection_all
  Runs all architecture detection tests.
  Tests the following:
  - architecture detection macros are defined
  - architecture property values are valid
*/
bool
d_tests_sa_env_arch_detection_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter subsection_counter;
    bool macros_result;
    bool properties_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    subsection_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[SUBSECTION] Testing Architecture Detection\n");
    printf("----------------------------------------"
           "----------------------------------------\n");

    macros_result     = d_tests_sa_env_arch_detection_macros(&subsection_counter);
    properties_result = d_tests_sa_env_arch_property_values(&subsection_counter);

    // update totals
    _test_info->assertions_total  += subsection_counter.assertions_total;
    _test_info->assertions_passed += subsection_counter.assertions_passed;
    _test_info->tests_total         += subsection_counter.tests_total;
    _test_info->tests_passed      += subsection_counter.tests_passed;

    overall_result = (macros_result && properties_result);

    if (overall_result)
    {
        printf("[PASS] Architecture Detection: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Architecture Detection: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }

    return overall_result;
}


/******************************************************************************
* ARCHITECTURE FAMILY MACROS TESTS
******************************************************************************/

/*
d_tests_sa_env_arch_family_macros
  Tests architecture family classification macros.
  Tests the following:
  - D_ENV_ARCH_IS_X86_FAMILY is defined
  - D_ENV_ARCH_IS_ARM_FAMILY is defined
  - family macros have correct boolean values (0 or 1)
  - x86 and x64 are correctly classified as x86 family
  - ARM and ARM64 are correctly classified as ARM family
*/
bool
d_tests_sa_env_arch_family_macros
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

    printf("%s--- Testing Architecture Family Macros ---\n", D_INDENT);

    // verify D_ENV_ARCH_IS_X86_FAMILY is defined
    #ifdef D_ENV_ARCH_IS_X86_FAMILY
        if (!d_assert_standalone(true,
                                 "D_ENV_ARCH_IS_X86_FAMILY is defined",
                                 "x86 family macro should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!d_assert_standalone((D_ENV_ARCH_IS_X86_FAMILY == 0) ||
                                 (D_ENV_ARCH_IS_X86_FAMILY == 1),
                                 "D_ENV_ARCH_IS_X86_FAMILY is boolean (0 or 1)",
                                 "family macro should be boolean",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_ARCH_IS_X86_FAMILY = %d\n",
               D_INDENT,
               D_ENV_ARCH_IS_X86_FAMILY);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_ARCH_IS_X86_FAMILY should be defined",
                                 "x86 family macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_IS_ARM_FAMILY is defined
    #ifdef D_ENV_ARCH_IS_ARM_FAMILY
        if (!d_assert_standalone(true,
                                 "D_ENV_ARCH_IS_ARM_FAMILY is defined",
                                 "ARM family macro should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!d_assert_standalone((D_ENV_ARCH_IS_ARM_FAMILY == 0) ||
                                 (D_ENV_ARCH_IS_ARM_FAMILY == 1),
                                 "D_ENV_ARCH_IS_ARM_FAMILY is boolean (0 or 1)",
                                 "family macro should be boolean",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_ARCH_IS_ARM_FAMILY = %d\n",
               D_INDENT,
               D_ENV_ARCH_IS_ARM_FAMILY);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_ARCH_IS_ARM_FAMILY should be defined",
                                 "ARM family macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify x86 and x64 are classified as x86 family
    #if ( defined(D_ENV_ARCH_X86) ||  \
          defined(D_ENV_ARCH_X64) )
        if (!d_assert_standalone(D_ENV_ARCH_IS_X86_FAMILY == 1,
                                 "x86/x64 classified as x86 family",
                                 "x86 and x64 should be x86 family",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(D_ENV_ARCH_IS_X86_FAMILY == 0,
                                 "non-x86 architectures not classified as x86 family",
                                 "other architectures should not be x86 family",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify ARM and ARM64 are classified as ARM family
    #if ( defined(D_ENV_ARCH_ARM) ||  \
          defined(D_ENV_ARCH_ARM64) )
        if (!d_assert_standalone(D_ENV_ARCH_IS_ARM_FAMILY == 1,
                                 "ARM/ARM64 classified as ARM family",
                                 "ARM and ARM64 should be ARM family",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(D_ENV_ARCH_IS_ARM_FAMILY == 0,
                                 "non-ARM architectures not classified as ARM family",
                                 "other architectures should not be ARM family",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Architecture family macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Architecture family macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* BIT WIDTH MACROS TESTS
******************************************************************************/

/*
d_tests_sa_env_arch_bitwidth_macros
  Tests bit width classification macros.
  Tests the following:
  - D_ENV_ARCH_IS_64BIT is defined
  - D_ENV_ARCH_IS_32BIT is defined
  - bit width macros have correct boolean values (0 or 1)
  - macros correctly reflect D_ENV_ARCH_BITS value
  - at most one bit width is active
*/
bool
d_tests_sa_env_arch_bitwidth_macros
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

    printf("%s--- Testing Bit Width Macros ---\n", D_INDENT);

    // verify D_ENV_ARCH_IS_64BIT is defined
    #ifdef D_ENV_ARCH_IS_64BIT
        if (!d_assert_standalone(true,
                                 "D_ENV_ARCH_IS_64BIT is defined",
                                 "64-bit macro should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!d_assert_standalone((D_ENV_ARCH_IS_64BIT == 0) ||
                                 (D_ENV_ARCH_IS_64BIT == 1),
                                 "D_ENV_ARCH_IS_64BIT is boolean (0 or 1)",
                                 "bit width macro should be boolean",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_ARCH_IS_64BIT = %d\n",
               D_INDENT,
               D_ENV_ARCH_IS_64BIT);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_ARCH_IS_64BIT should be defined",
                                 "64-bit macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_IS_32BIT is defined
    #ifdef D_ENV_ARCH_IS_32BIT
        if (!d_assert_standalone(true,
                                 "D_ENV_ARCH_IS_32BIT is defined",
                                 "32-bit macro should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!d_assert_standalone((D_ENV_ARCH_IS_32BIT == 0) ||
                                 (D_ENV_ARCH_IS_32BIT == 1),
                                 "D_ENV_ARCH_IS_32BIT is boolean (0 or 1)",
                                 "bit width macro should be boolean",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_ARCH_IS_32BIT = %d\n",
               D_INDENT,
               D_ENV_ARCH_IS_32BIT);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_ARCH_IS_32BIT should be defined",
                                 "32-bit macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify macros reflect D_ENV_ARCH_BITS
    if (D_ENV_ARCH_BITS == 64)
    {
        if (!d_assert_standalone(D_ENV_ARCH_IS_64BIT == 1,
                                 "64-bit architecture correctly detected",
                                 "IS_64BIT should be 1 when BITS is 64",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_ARCH_IS_32BIT == 0,
                                 "IS_32BIT is 0 for 64-bit architecture",
                                 "IS_32BIT should be 0 when BITS is 64",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }
    else if (D_ENV_ARCH_BITS == 32)
    {
        if (!d_assert_standalone(D_ENV_ARCH_IS_32BIT == 1,
                                 "32-bit architecture correctly detected",
                                 "IS_32BIT should be 1 when BITS is 32",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_ARCH_IS_64BIT == 0,
                                 "IS_64BIT is 0 for 32-bit architecture",
                                 "IS_64BIT should be 0 when BITS is 32",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }
    else
    {
        // unknown bit width (0)
        if (!d_assert_standalone((D_ENV_ARCH_IS_64BIT == 0) &&
                                 (D_ENV_ARCH_IS_32BIT == 0),
                                 "both bit width macros are 0 for unknown",
                                 "both should be 0 when BITS is 0",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // verify at most one bit width is active
    if (!d_assert_standalone((D_ENV_ARCH_IS_64BIT + D_ENV_ARCH_IS_32BIT) <= 1,
                             "at most one bit width is active",
                             "cannot be both 32-bit and 64-bit",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Bit width macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Bit width macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* ENDIANNESS MACROS TESTS
******************************************************************************/

/*
d_tests_sa_env_arch_endianness_macros
  Tests endianness classification macros.
  Tests the following:
  - D_ENV_ARCH_IS_LITTLE_ENDIAN is defined
  - D_ENV_ARCH_IS_BIG_ENDIAN is defined
  - endianness macros have correct boolean values (0 or 1)
  - macros correctly reflect D_ENV_ARCH_ENDIAN value
  - at most one endianness is active
*/
bool
d_tests_sa_env_arch_endianness_macros
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

    printf("%s--- Testing Endianness Macros ---\n", D_INDENT);

    // verify D_ENV_ARCH_IS_LITTLE_ENDIAN is defined
    #ifdef D_ENV_ARCH_IS_LITTLE_ENDIAN
        if (!d_assert_standalone(true,
                                 "D_ENV_ARCH_IS_LITTLE_ENDIAN is defined",
                                 "little endian macro should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!d_assert_standalone((D_ENV_ARCH_IS_LITTLE_ENDIAN == 0) ||
                                 (D_ENV_ARCH_IS_LITTLE_ENDIAN == 1),
                                 "D_ENV_ARCH_IS_LITTLE_ENDIAN is boolean (0 or 1)",
                                 "endianness macro should be boolean",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_ARCH_IS_LITTLE_ENDIAN = %d\n",
               D_INDENT,
               D_ENV_ARCH_IS_LITTLE_ENDIAN);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_ARCH_IS_LITTLE_ENDIAN should be defined",
                                 "little endian macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_ARCH_IS_BIG_ENDIAN is defined
    #ifdef D_ENV_ARCH_IS_BIG_ENDIAN
        if (!d_assert_standalone(true,
                                 "D_ENV_ARCH_IS_BIG_ENDIAN is defined",
                                 "big endian macro should be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify it's boolean
        if (!d_assert_standalone((D_ENV_ARCH_IS_BIG_ENDIAN == 0) ||
                                 (D_ENV_ARCH_IS_BIG_ENDIAN == 1),
                                 "D_ENV_ARCH_IS_BIG_ENDIAN is boolean (0 or 1)",
                                 "endianness macro should be boolean",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        printf("%s    D_ENV_ARCH_IS_BIG_ENDIAN = %d\n",
               D_INDENT,
               D_ENV_ARCH_IS_BIG_ENDIAN);
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_ARCH_IS_BIG_ENDIAN should be defined",
                                 "big endian macro should always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify macros reflect D_ENV_ARCH_ENDIAN
    if (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE)
    {
        if (!d_assert_standalone(D_ENV_ARCH_IS_LITTLE_ENDIAN == 1,
                                 "little endian architecture correctly detected",
                                 "IS_LITTLE_ENDIAN should be 1",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_ARCH_IS_BIG_ENDIAN == 0,
                                 "IS_BIG_ENDIAN is 0 for little endian",
                                 "IS_BIG_ENDIAN should be 0",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }
    else if (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG)
    {
        if (!d_assert_standalone(D_ENV_ARCH_IS_BIG_ENDIAN == 1,
                                 "big endian architecture correctly detected",
                                 "IS_BIG_ENDIAN should be 1",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone(D_ENV_ARCH_IS_LITTLE_ENDIAN == 0,
                                 "IS_LITTLE_ENDIAN is 0 for big endian",
                                 "IS_LITTLE_ENDIAN should be 0",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }
    else
    {
        // unknown endianness
        if (!d_assert_standalone((D_ENV_ARCH_IS_LITTLE_ENDIAN == 0) &&
                                 (D_ENV_ARCH_IS_BIG_ENDIAN == 0),
                                 "both endianness macros are 0 for unknown",
                                 "both should be 0 for unknown endianness",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // verify at most one endianness is active
    if (!d_assert_standalone((D_ENV_ARCH_IS_LITTLE_ENDIAN + D_ENV_ARCH_IS_BIG_ENDIAN) <= 1,
                             "at most one endianness is active",
                             "cannot be both little and big endian",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Endianness macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Endianness macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_env_arch_utility_macros_all
  Runs all architecture utility macro tests.
  Tests the following:
  - architecture family macros
  - bit width macros
  - endianness macros
*/
bool
d_tests_sa_env_arch_utility_macros_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter subsection_counter;
    bool family_result;
    bool bitwidth_result;
    bool endianness_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    subsection_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[SUBSECTION] Testing Architecture Utility Macros\n");
    printf("----------------------------------------"
           "----------------------------------------\n");

    family_result     = d_tests_sa_env_arch_family_macros(&subsection_counter);
    bitwidth_result   = d_tests_sa_env_arch_bitwidth_macros(&subsection_counter);
    endianness_result = d_tests_sa_env_arch_endianness_macros(&subsection_counter);

    // update totals
    _test_info->assertions_total  += subsection_counter.assertions_total;
    _test_info->assertions_passed += subsection_counter.assertions_passed;
    _test_info->tests_total         += subsection_counter.tests_total;
    _test_info->tests_passed      += subsection_counter.tests_passed;

    overall_result = (family_result && bitwidth_result && endianness_result);

    if (overall_result)
    {
        printf("[PASS] Architecture Utility Macros: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Architecture Utility Macros: %zu/%zu tests passed\n",
               subsection_counter.tests_passed,
               subsection_counter.tests_total);
    }

    return overall_result;
}


/******************************************************************************
* ARCHITECTURE CONSISTENCY TESTS
******************************************************************************/

/*
d_tests_sa_env_arch_consistency_check
  Tests consistency between architecture properties.
  Tests the following:
  - known architectures have non-zero bit width
  - known little-endian architectures are correctly classified
  - known big-endian architectures are correctly classified
  - 32-bit variants have correct bit width
  - 64-bit variants have correct bit width
*/
bool
d_tests_sa_env_arch_consistency_check
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

    printf("%s--- Testing Architecture Consistency ---\n", D_INDENT);

    // verify known architectures (not unknown) have non-zero bit width
    #ifndef D_ENV_ARCH_UNKNOWN
        if (!d_assert_standalone(D_ENV_ARCH_BITS != 0,
                                 "known architecture has non-zero bit width",
                                 "BITS should be 32 or 64 for known architectures",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify x86 family is little endian
    #if ( defined(D_ENV_ARCH_X86) ||  \
          defined(D_ENV_ARCH_X64) )
        if (!d_assert_standalone(D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE,
                                 "x86 family is little endian",
                                 "x86 and x64 should be little endian",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify ARM family is little endian
    #if ( defined(D_ENV_ARCH_ARM) ||  \
          defined(D_ENV_ARCH_ARM64) )
        if (!d_assert_standalone(D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE,
                                 "ARM family is little endian",
                                 "ARM and ARM64 should be little endian",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify RISC-V is little endian
    #ifdef D_ENV_ARCH_RISCV
        if (!d_assert_standalone(D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE,
                                 "RISC-V is little endian",
                                 "RISC-V should be little endian",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify PowerPC is big endian
    #ifdef D_ENV_ARCH_POWERPC
        if (!d_assert_standalone(D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG,
                                 "PowerPC is big endian",
                                 "PowerPC should be big endian",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify MIPS is big endian
    #ifdef D_ENV_ARCH_MIPS
        if (!d_assert_standalone(D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG,
                                 "MIPS is big endian",
                                 "MIPS should be big endian",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify SPARC is big endian
    #ifdef D_ENV_ARCH_SPARC
        if (!d_assert_standalone(D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG,
                                 "SPARC is big endian",
                                 "SPARC should be big endian",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify S390 is big endian
    #ifdef D_ENV_ARCH_S390
        if (!d_assert_standalone(D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG,
                                 "S390 is big endian",
                                 "S390 should be big endian",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify IA64 is little endian
    #ifdef D_ENV_ARCH_IA64
        if (!d_assert_standalone(D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE,
                                 "IA64 is little endian",
                                 "Itanium should be little endian",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify Alpha is little endian
    #ifdef D_ENV_ARCH_ALPHA
        if (!d_assert_standalone(D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE,
                                 "Alpha is little endian",
                                 "Alpha should be little endian",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify 32-bit architectures
    #ifdef D_ENV_ARCH_X86
        if (!d_assert_standalone(D_ENV_ARCH_BITS == 32,
                                 "x86 is 32-bit",
                                 "x86 should be 32-bit",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ARM
        if (!d_assert_standalone(D_ENV_ARCH_BITS == 32,
                                 "ARM is 32-bit",
                                 "ARM should be 32-bit",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify 64-bit architectures
    #ifdef D_ENV_ARCH_X64
        if (!d_assert_standalone(D_ENV_ARCH_BITS == 64,
                                 "x64 is 64-bit",
                                 "x64 should be 64-bit",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ARM64
        if (!d_assert_standalone(D_ENV_ARCH_BITS == 64,
                                 "ARM64 is 64-bit",
                                 "ARM64 should be 64-bit",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_IA64
        if (!d_assert_standalone(D_ENV_ARCH_BITS == 64,
                                 "IA64 is 64-bit",
                                 "Itanium should be 64-bit",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #ifdef D_ENV_ARCH_ALPHA
        if (!d_assert_standalone(D_ENV_ARCH_BITS == 64,
                                 "Alpha is 64-bit",
                                 "Alpha should be 64-bit",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    printf("%s    Architecture is consistent\n", D_INDENT);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Architecture consistency test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Architecture consistency test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* MODULE AGGREGATOR
******************************************************************************/

/*
d_tests_sa_env_arch_all
  Runs all architecture detection tests.
  Tests the following:
  - architecture type constants
  - endianness constants
  - architecture detection
  - architecture utility macros
  - architecture consistency
*/
bool
d_tests_sa_env_arch_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool type_constants_result;
    bool endian_constants_result;
    bool detection_result;
    bool utility_macros_result;
    bool consistency_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[MODULE] Testing Architecture Detection\n");
    printf("========================================="
           "=======================================\n");

    type_constants_result   = d_tests_sa_env_arch_type_constants(&module_counter);
    endian_constants_result = d_tests_sa_env_arch_endian_constants(&module_counter);
    detection_result        = d_tests_sa_env_arch_detection_all(&module_counter);
    utility_macros_result   = d_tests_sa_env_arch_utility_macros_all(&module_counter);
    consistency_result      = d_tests_sa_env_arch_consistency_check(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total         += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( type_constants_result   &&
                       endian_constants_result &&
                       detection_result        &&
                       utility_macros_result   &&
                       consistency_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Architecture Detection Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Architecture Detection Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Type Constants:      %s\n",
               type_constants_result ? "PASSED" : "FAILED");
        printf("  - Endian Constants:    %s\n",
               endian_constants_result ? "PASSED" : "FAILED");
        printf("  - Detection:           %s\n",
               detection_result ? "PASSED" : "FAILED");
        printf("  - Utility Macros:      %s\n",
               utility_macros_result ? "PASSED" : "FAILED");
        printf("  - Consistency:         %s\n",
               consistency_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
