#include ".\test_common_tests_sa.h"


/******************************************************************************
 * VI. TYPE DISCRIMINATOR TESTS
 *****************************************************************************/

/*
d_tests_sa_test_common_type_flag_enum
  Tests the DTestTypeFlag enumeration definition.
  Tests the following:
  - DTestTypeFlag type exists
  - All expected enum values are defined
  - Enum values can be assigned to variables
  - Enum values can be used in switch statements
*/
bool
d_tests_sa_test_common_type_flag_enum
(
    struct d_test_counter* _counter
)
{
    bool               result;
    enum DTestTypeFlag flag;

    result = true;

    // test 1: DTestTypeFlag type can hold D_TEST_TYPE_UNKNOWN
    flag   = D_TEST_TYPE_UNKNOWN;
    result = d_assert_standalone(
        flag == D_TEST_TYPE_UNKNOWN,
        "type_flag_unknown_defined",
        "D_TEST_TYPE_UNKNOWN should be defined",
        _counter) && result;

    // test 2: DTestTypeFlag type can hold D_TEST_TYPE_ASSERT
    flag   = D_TEST_TYPE_ASSERT;
    result = d_assert_standalone(
        flag == D_TEST_TYPE_ASSERT,
        "type_flag_assert_defined",
        "D_TEST_TYPE_ASSERT should be defined",
        _counter) && result;

    // test 3: DTestTypeFlag type can hold D_TEST_TYPE_TEST_FN
    flag   = D_TEST_TYPE_TEST_FN;
    result = d_assert_standalone(
        flag == D_TEST_TYPE_TEST_FN,
        "type_flag_test_fn_defined",
        "D_TEST_TYPE_TEST_FN should be defined",
        _counter) && result;

    // test 4: DTestTypeFlag type can hold D_TEST_TYPE_TEST
    flag   = D_TEST_TYPE_TEST;
    result = d_assert_standalone(
        flag == D_TEST_TYPE_TEST,
        "type_flag_test_defined",
        "D_TEST_TYPE_TEST should be defined",
        _counter) && result;

    // test 5: DTestTypeFlag type can hold D_TEST_TYPE_TEST_BLOCK
    flag   = D_TEST_TYPE_TEST_BLOCK;
    result = d_assert_standalone(
        flag == D_TEST_TYPE_TEST_BLOCK,
        "type_flag_test_block_defined",
        "D_TEST_TYPE_TEST_BLOCK should be defined",
        _counter) && result;

    // test 6: DTestTypeFlag type can hold D_TEST_TYPE_MODULE
    flag   = D_TEST_TYPE_MODULE;
    result = d_assert_standalone(
        flag == D_TEST_TYPE_MODULE,
        "type_flag_module_defined",
        "D_TEST_TYPE_MODULE should be defined",
        _counter) && result;

    // test 7: enum values can be used in switch statement
    {
        bool               switch_works;
        enum DTestTypeFlag test_flag;

        switch_works = false;
        test_flag    = D_TEST_TYPE_TEST;

        switch (test_flag)
        {
            case D_TEST_TYPE_UNKNOWN:
            case D_TEST_TYPE_ASSERT:
            case D_TEST_TYPE_TEST_FN:
                switch_works = false;
                break;
            case D_TEST_TYPE_TEST:
                switch_works = true;
                break;
            case D_TEST_TYPE_TEST_BLOCK:
            case D_TEST_TYPE_MODULE:
                switch_works = false;
                break;
            default:
                switch_works = false;
                break;
        }

        result = d_assert_standalone(
            switch_works,
            "type_flag_switch_statement",
            "DTestTypeFlag should work in switch statement",
            _counter) && result;
    }

    // test 8: enum can be converted to int
    {
        int flag_as_int;

        flag_as_int = (int)D_TEST_TYPE_MODULE;

        result = d_assert_standalone(
            flag_as_int == D_TEST_TYPE_MODULE,
            "type_flag_to_int",
            "DTestTypeFlag should be convertible to int",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_test_common_type_flag_values
  Tests the specific values assigned to DTestTypeFlag enumeration.
  Tests the following:
  - D_TEST_TYPE_UNKNOWN equals 0
  - D_TEST_TYPE_ASSERT equals 1
  - D_TEST_TYPE_TEST_FN equals 2
  - D_TEST_TYPE_TEST equals 3
  - D_TEST_TYPE_TEST_BLOCK equals 4
  - D_TEST_TYPE_MODULE equals 5
  - Values are sequential
*/
bool
d_tests_sa_test_common_type_flag_values
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: D_TEST_TYPE_UNKNOWN equals 0
    result = d_assert_standalone(
        D_TEST_TYPE_UNKNOWN == 0,
        "type_flag_unknown_value",
        "D_TEST_TYPE_UNKNOWN should equal 0",
        _counter) && result;

    // test 2: D_TEST_TYPE_ASSERT equals 1
    result = d_assert_standalone(
        D_TEST_TYPE_ASSERT == 1,
        "type_flag_assert_value",
        "D_TEST_TYPE_ASSERT should equal 1",
        _counter) && result;

    // test 3: D_TEST_TYPE_TEST_FN equals 2
    result = d_assert_standalone(
        D_TEST_TYPE_TEST_FN == 2,
        "type_flag_test_fn_value",
        "D_TEST_TYPE_TEST_FN should equal 2",
        _counter) && result;

    // test 4: D_TEST_TYPE_TEST equals 3
    result = d_assert_standalone(
        D_TEST_TYPE_TEST == 3,
        "type_flag_test_value",
        "D_TEST_TYPE_TEST should equal 3",
        _counter) && result;

    // test 5: D_TEST_TYPE_TEST_BLOCK equals 4
    result = d_assert_standalone(
        D_TEST_TYPE_TEST_BLOCK == 4,
        "type_flag_test_block_value",
        "D_TEST_TYPE_TEST_BLOCK should equal 4",
        _counter) && result;

    // test 6: D_TEST_TYPE_MODULE equals 5
    result = d_assert_standalone(
        D_TEST_TYPE_MODULE == 5,
        "type_flag_module_value",
        "D_TEST_TYPE_MODULE should equal 5",
        _counter) && result;

    // test 7: values are sequential (no gaps from 0 to 5)
    result = d_assert_standalone(
        (D_TEST_TYPE_ASSERT == D_TEST_TYPE_UNKNOWN + 1) &&
        (D_TEST_TYPE_TEST_FN == D_TEST_TYPE_ASSERT + 1) &&
        (D_TEST_TYPE_TEST == D_TEST_TYPE_TEST_FN + 1) &&
        (D_TEST_TYPE_TEST_BLOCK == D_TEST_TYPE_TEST + 1) &&
        (D_TEST_TYPE_MODULE == D_TEST_TYPE_TEST_BLOCK + 1),
        "type_flag_values_sequential",
        "DTestTypeFlag values should be sequential",
        _counter) && result;

    // test 8: values can be used as array indices
    {
        const char* type_names[6];
        bool        index_ok;

        type_names[D_TEST_TYPE_UNKNOWN]    = "unknown";
        type_names[D_TEST_TYPE_ASSERT]     = "assert";
        type_names[D_TEST_TYPE_TEST_FN]    = "test_fn";
        type_names[D_TEST_TYPE_TEST]       = "test";
        type_names[D_TEST_TYPE_TEST_BLOCK] = "test_block";
        type_names[D_TEST_TYPE_MODULE]     = "module";

        index_ok = (d_strcasecmp(type_names[D_TEST_TYPE_UNKNOWN], "unknown") == 0) &&
                   (d_strcasecmp(type_names[D_TEST_TYPE_MODULE], "module") == 0);

        result = d_assert_standalone(
            index_ok,
            "type_flag_as_array_index",
            "DTestTypeFlag values should be usable as array indices",
            _counter) && result;
    }

    // test 9: there are exactly 6 type flag values (0 through 5)
    result = d_assert_standalone(
        D_TEST_TYPE_MODULE == 5,
        "type_flag_count",
        "There should be exactly 6 type flags (0-5)",
        _counter) && result;

    // test 10: enum fits in minimal storage
    result = d_assert_standalone(
        sizeof(enum DTestTypeFlag) <= sizeof(int),
        "type_flag_size",
        "DTestTypeFlag should fit in int-sized storage or smaller",
        _counter) && result;

    return result;
}


/*
d_tests_sa_test_common_type_flag_uniqueness
  Tests the uniqueness of DTestTypeFlag enumeration values.
  Tests the following:
  - All type flag values are distinct from each other
  - Can distinguish between all types using equality comparison
  - Values form a valid discriminated union tag set
*/
bool
d_tests_sa_test_common_type_flag_uniqueness
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: all values are unique (comprehensive pairwise check)
    result = d_assert_standalone(
        D_TEST_TYPE_UNKNOWN != D_TEST_TYPE_ASSERT,
        "uniqueness_unknown_assert",
        "UNKNOWN and ASSERT should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_UNKNOWN != D_TEST_TYPE_TEST_FN,
        "uniqueness_unknown_test_fn",
        "UNKNOWN and TEST_FN should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_UNKNOWN != D_TEST_TYPE_TEST,
        "uniqueness_unknown_test",
        "UNKNOWN and TEST should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_UNKNOWN != D_TEST_TYPE_TEST_BLOCK,
        "uniqueness_unknown_test_block",
        "UNKNOWN and TEST_BLOCK should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_UNKNOWN != D_TEST_TYPE_MODULE,
        "uniqueness_unknown_module",
        "UNKNOWN and MODULE should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_ASSERT != D_TEST_TYPE_TEST_FN,
        "uniqueness_assert_test_fn",
        "ASSERT and TEST_FN should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_ASSERT != D_TEST_TYPE_TEST,
        "uniqueness_assert_test",
        "ASSERT and TEST should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_ASSERT != D_TEST_TYPE_TEST_BLOCK,
        "uniqueness_assert_test_block",
        "ASSERT and TEST_BLOCK should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_ASSERT != D_TEST_TYPE_MODULE,
        "uniqueness_assert_module",
        "ASSERT and MODULE should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_TEST_FN != D_TEST_TYPE_TEST,
        "uniqueness_test_fn_test",
        "TEST_FN and TEST should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_TEST_FN != D_TEST_TYPE_TEST_BLOCK,
        "uniqueness_test_fn_test_block",
        "TEST_FN and TEST_BLOCK should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_TEST_FN != D_TEST_TYPE_MODULE,
        "uniqueness_test_fn_module",
        "TEST_FN and MODULE should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_TEST != D_TEST_TYPE_TEST_BLOCK,
        "uniqueness_test_test_block",
        "TEST and TEST_BLOCK should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_TEST != D_TEST_TYPE_MODULE,
        "uniqueness_test_module",
        "TEST and MODULE should be different",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_TYPE_TEST_BLOCK != D_TEST_TYPE_MODULE,
        "uniqueness_test_block_module",
        "TEST_BLOCK and MODULE should be different",
        _counter) && result;

    // test 2: can identify type using switch (discriminated union pattern)
    {
        enum DTestTypeFlag types[6];
        size_t             i;
        bool               all_identified;

        types[0] = D_TEST_TYPE_UNKNOWN;
        types[1] = D_TEST_TYPE_ASSERT;
        types[2] = D_TEST_TYPE_TEST_FN;
        types[3] = D_TEST_TYPE_TEST;
        types[4] = D_TEST_TYPE_TEST_BLOCK;
        types[5] = D_TEST_TYPE_MODULE;

        all_identified = true;

        for (i = 0; i < 6; i++)
        {
            bool identified;

            identified = false;

            switch (types[i])
            {
                case D_TEST_TYPE_UNKNOWN:
                    identified = (i == 0);
                    break;
                case D_TEST_TYPE_ASSERT:
                    identified = (i == 1);
                    break;
                case D_TEST_TYPE_TEST_FN:
                    identified = (i == 2);
                    break;
                case D_TEST_TYPE_TEST:
                    identified = (i == 3);
                    break;
                case D_TEST_TYPE_TEST_BLOCK:
                    identified = (i == 4);
                    break;
                case D_TEST_TYPE_MODULE:
                    identified = (i == 5);
                    break;
                default:
                    identified = false;
                    break;
            }

            if (!identified)
            {
                all_identified = false;
            }
        }

        result = d_assert_standalone(
            all_identified,
            "type_flag_discriminated_union",
            "DTestTypeFlag should identify all types in discriminated union",
            _counter) && result;
    }

    // test 3: UNKNOWN is the default/sentinel value (equals 0)
    result = d_assert_standalone(
        D_TEST_TYPE_UNKNOWN == 0,
        "type_flag_unknown_is_zero",
        "D_TEST_TYPE_UNKNOWN should be 0 (default/sentinel)",
        _counter) && result;

    // test 4: all valid types are non-zero
    result = d_assert_standalone(
        (D_TEST_TYPE_ASSERT != 0) &&
        (D_TEST_TYPE_TEST_FN != 0) &&
        (D_TEST_TYPE_TEST != 0) &&
        (D_TEST_TYPE_TEST_BLOCK != 0) &&
        (D_TEST_TYPE_MODULE != 0),
        "type_flag_valid_nonzero",
        "All valid type flags should be non-zero",
        _counter) && result;

    // test 5: type hierarchy - leaf types vs container types
    // ASSERT and TEST_FN are leaf types (no children)
    // TEST can contain assertions
    // TEST_BLOCK contains tests
    // MODULE contains blocks
    {
        bool is_leaf_assert;
        bool is_leaf_test_fn;
        bool is_container_test;
        bool is_container_block;
        bool is_container_module;
        bool hierarchy_ok;

        is_leaf_assert      = (D_TEST_TYPE_ASSERT < D_TEST_TYPE_TEST);
        is_leaf_test_fn     = (D_TEST_TYPE_TEST_FN < D_TEST_TYPE_TEST);
        is_container_test   = (D_TEST_TYPE_TEST > D_TEST_TYPE_ASSERT);
        is_container_block  = (D_TEST_TYPE_TEST_BLOCK > D_TEST_TYPE_TEST);
        is_container_module = (D_TEST_TYPE_MODULE > D_TEST_TYPE_TEST_BLOCK);

        hierarchy_ok = is_leaf_assert && is_leaf_test_fn &&
                       is_container_test && is_container_block &&
                       is_container_module;

        result = d_assert_standalone(
            hierarchy_ok,
            "type_flag_hierarchy",
            "Type flags should follow leaf < container hierarchy",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_test_common_discriminator_all
  Aggregation function that runs all type discriminator tests.
*/
bool
d_tests_sa_test_common_discriminator_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Type Discriminators\n");
    printf("  -------------------------------\n");

    result = d_tests_sa_test_common_type_flag_enum(_counter) && result;
    result = d_tests_sa_test_common_type_flag_values(_counter) && result;
    result = d_tests_sa_test_common_type_flag_uniqueness(_counter) && result;

    return result;
}
