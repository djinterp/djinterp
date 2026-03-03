#include "./test_common_tests_sa.h"


/******************************************************************************
 * VII. COUNTER SPEC ENCODING TESTS
 *****************************************************************************/

/*
d_tests_sa_test_common_counter_spec_encode
  Tests the D_TEST_COUNTER_SPEC macro encoding.
  Tests the following:
  - spec packs test_type into bits [0..7]
  - spec packs fields into bits [8..11]
  - reserved bits [12..15] are zero
  - round-trip: extract matches input
  - all DTestTypeFlag values encode correctly
*/
bool
d_tests_sa_test_common_counter_spec_encode
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t spec;

    result = true;

    // test 1: ASSERT + STANDARD round-trips
    spec   = D_TEST_COUNTER_SPEC(D_TEST_TYPE_ASSERT,
                                 D_TEST_COUNT_STANDARD);
    result = d_assert_standalone(
        D_TEST_COUNTER_SPEC_TYPE(spec) == D_TEST_TYPE_ASSERT,
        "spec_encode_type_assert",
        "SPEC should extract ASSERT type",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_SPEC_FIELDS(spec) == D_TEST_COUNT_STANDARD,
        "spec_encode_fields_std",
        "SPEC should extract STANDARD fields",
        _counter) && result;

    // test 2: TEST + ALL round-trips
    spec   = D_TEST_COUNTER_SPEC(D_TEST_TYPE_TEST,
                                 D_TEST_COUNT_ALL);
    result = d_assert_standalone(
        D_TEST_COUNTER_SPEC_TYPE(spec) == D_TEST_TYPE_TEST,
        "spec_encode_type_test",
        "SPEC should extract TEST type",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_SPEC_FIELDS(spec) == D_TEST_COUNT_ALL,
        "spec_encode_fields_all",
        "SPEC should extract ALL fields",
        _counter) && result;

    // test 3: MODULE + TOTAL only
    spec   = D_TEST_COUNTER_SPEC(D_TEST_TYPE_MODULE,
                                 D_TEST_COUNT_TOTAL);
    result = d_assert_standalone(
        D_TEST_COUNTER_SPEC_TYPE(spec) == D_TEST_TYPE_MODULE,
        "spec_encode_type_module",
        "SPEC should extract MODULE type",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_SPEC_FIELDS(spec) == D_TEST_COUNT_TOTAL,
        "spec_encode_fields_total",
        "SPEC should extract TOTAL field only",
        _counter) && result;

    // test 4: reserved bits are zero
    spec   = D_TEST_COUNTER_SPEC(D_TEST_TYPE_MODULE,
                                 D_TEST_COUNT_ALL);
    result = d_assert_standalone(
        (spec & 0xF000) == 0,
        "spec_reserved_zero",
        "SPEC reserved bits [12..15] should be zero",
        _counter) && result;

    // test 5: type 0 (UNKNOWN) + no fields
    spec   = D_TEST_COUNTER_SPEC(D_TEST_TYPE_UNKNOWN, 0);
    result = d_assert_standalone(
        spec == 0,
        "spec_encode_zero",
        "SPEC with type 0 and no fields should be 0",
        _counter) && result;

    // test 6: all type flags encode distinctly
    {
        uint16_t s_assert;
        uint16_t s_test;
        uint16_t s_test_fn;
        uint16_t s_block;
        uint16_t s_module;

        s_assert  = D_TEST_COUNTER_SPEC(D_TEST_TYPE_ASSERT,
                                        D_TEST_COUNT_STANDARD);
        s_test    = D_TEST_COUNTER_SPEC(D_TEST_TYPE_TEST,
                                        D_TEST_COUNT_STANDARD);
        s_test_fn = D_TEST_COUNTER_SPEC(D_TEST_TYPE_TEST_FN,
                                        D_TEST_COUNT_STANDARD);
        s_block   = D_TEST_COUNTER_SPEC(D_TEST_TYPE_TEST_BLOCK,
                                        D_TEST_COUNT_STANDARD);
        s_module  = D_TEST_COUNTER_SPEC(D_TEST_TYPE_MODULE,
                                        D_TEST_COUNT_STANDARD);

        result = d_assert_standalone(
            (s_assert != s_test) &&
            (s_assert != s_test_fn) &&
            (s_assert != s_block) &&
            (s_assert != s_module) &&
            (s_test != s_test_fn) &&
            (s_test != s_block) &&
            (s_test != s_module),
            "spec_types_distinct",
            "Different types with same fields should produce distinct specs",
            _counter) && result;
    }

    // test 7: predefined spec macros match manual construction
    result = d_assert_standalone(
        D_TEST_COUNTER_ASSERT_STD ==
            D_TEST_COUNTER_SPEC(D_TEST_TYPE_ASSERT,
                                D_TEST_COUNT_STANDARD),
        "spec_preset_assert_std",
        "D_TEST_COUNTER_ASSERT_STD matches manual spec",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_TEST_ALL ==
            D_TEST_COUNTER_SPEC(D_TEST_TYPE_TEST,
                                D_TEST_COUNT_ALL),
        "spec_preset_test_all",
        "D_TEST_COUNTER_TEST_ALL matches manual spec",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_ASSERT_TOTAL ==
            D_TEST_COUNTER_SPEC(D_TEST_TYPE_ASSERT,
                                D_TEST_COUNT_TOTAL),
        "spec_preset_assert_total",
        "D_TEST_COUNTER_ASSERT_TOTAL matches manual spec",
        _counter) && result;

    return result;
}


/*
d_tests_sa_test_common_counter_spec_popcount
  Tests d_test_popcount4 and the field count/index macros.
  Tests the following:
  - popcount of 0 is 0
  - popcount of single bits is 1
  - popcount of STANDARD (0b0111) is 3
  - popcount of ALL (0b1111) is 4
  - D_TEST_COUNTER_SPEC_COUNT agrees
  - D_TEST_COUNTER_FIELD_INDEX gives compact indices
*/
bool
d_tests_sa_test_common_counter_spec_popcount
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t fields;

    result = true;

    // test 1: popcount of 0 is 0
    result = d_assert_standalone(
        d_test_popcount4(0) == 0,
        "popcount_zero",
        "popcount4(0) should be 0",
        _counter) && result;

    // test 2: popcount of single bits
    result = d_assert_standalone(
        d_test_popcount4(D_TEST_COUNT_TOTAL) == 1,
        "popcount_single_total",
        "popcount4(TOTAL) should be 1",
        _counter) && result;

    result = d_assert_standalone(
        d_test_popcount4(D_TEST_COUNT_REMAINING) == 1,
        "popcount_single_remaining",
        "popcount4(REMAINING) should be 1",
        _counter) && result;

    // test 3: popcount of STANDARD (T|P|F = 0b0111)
    result = d_assert_standalone(
        D_TEST_COUNTER_FIELD_COUNT(D_TEST_COUNT_STANDARD) == 3,
        "popcount_standard",
        "FIELD_COUNT(STANDARD) should be 3",
        _counter) && result;

    // test 4: popcount of ALL (T|P|F|R = 0b1111)
    result = d_assert_standalone(
        D_TEST_COUNTER_FIELD_COUNT(D_TEST_COUNT_ALL) == 4,
        "popcount_all",
        "FIELD_COUNT(ALL) should be 4",
        _counter) && result;

    // test 5: SPEC_COUNT agrees with FIELD_COUNT
    result = d_assert_standalone(
        D_TEST_COUNTER_SPEC_COUNT(D_TEST_COUNTER_ASSERT_STD) == 3,
        "spec_count_assert_std",
        "SPEC_COUNT(ASSERT_STD) should be 3",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_SPEC_COUNT(D_TEST_COUNTER_TEST_ALL) == 4,
        "spec_count_test_all",
        "SPEC_COUNT(TEST_ALL) should be 4",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_SPEC_COUNT(D_TEST_COUNTER_ASSERT_TOTAL) == 1,
        "spec_count_assert_total",
        "SPEC_COUNT(ASSERT_TOTAL) should be 1",
        _counter) && result;

    // test 6: compact indices for STANDARD (T|P|F)
    fields = D_TEST_COUNT_STANDARD;

    result = d_assert_standalone(
        D_TEST_COUNTER_FIELD_INDEX(fields, D_TEST_COUNT_TOTAL) == 0,
        "field_index_std_total",
        "TOTAL index in STANDARD should be 0",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_FIELD_INDEX(fields, D_TEST_COUNT_PASSED) == 1,
        "field_index_std_passed",
        "PASSED index in STANDARD should be 1",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_FIELD_INDEX(fields, D_TEST_COUNT_FAILED) == 2,
        "field_index_std_failed",
        "FAILED index in STANDARD should be 2",
        _counter) && result;

    // test 7: compact indices for ALL (T|P|F|R)
    fields = D_TEST_COUNT_ALL;

    result = d_assert_standalone(
        D_TEST_COUNTER_FIELD_INDEX(fields, D_TEST_COUNT_TOTAL) == 0,
        "field_index_all_total",
        "TOTAL index in ALL should be 0",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_FIELD_INDEX(fields, D_TEST_COUNT_REMAINING) == 3,
        "field_index_all_remaining",
        "REMAINING index in ALL should be 3",
        _counter) && result;

    // test 8: sparse field set — P|R (0b1010), 2 slots
    fields = D_TEST_COUNT_PASSED | D_TEST_COUNT_REMAINING;

    result = d_assert_standalone(
        D_TEST_COUNTER_FIELD_COUNT(fields) == 2,
        "popcount_sparse",
        "FIELD_COUNT(P|R) should be 2",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_FIELD_INDEX(fields, D_TEST_COUNT_PASSED) == 0,
        "field_index_sparse_passed",
        "PASSED index in P|R should be 0",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_COUNTER_FIELD_INDEX(fields, D_TEST_COUNT_REMAINING) == 1,
        "field_index_sparse_remaining",
        "REMAINING index in P|R should be 1",
        _counter) && result;

    // test 9: FIELD_ACTIVE macro
    result = d_assert_standalone(
        D_TEST_COUNTER_FIELD_ACTIVE(D_TEST_COUNT_STANDARD,
                                    D_TEST_COUNT_TOTAL),
        "field_active_yes",
        "TOTAL should be active in STANDARD",
        _counter) && result;

    result = d_assert_standalone(
        !D_TEST_COUNTER_FIELD_ACTIVE(D_TEST_COUNT_STANDARD,
                                     D_TEST_COUNT_REMAINING),
        "field_active_no",
        "REMAINING should not be active in STANDARD",
        _counter) && result;

    return result;
}


/*
d_tests_sa_test_common_counter_spec_init
  Tests d_test_counter_init with spec-based API.
  Tests the following:
  - init from ASSERT_STD spec sets correct type
  - init from ASSERT_STD spec sets correct fields
  - init allocates amount array (non-NULL)
  - all amount slots initialized to zero
  - init from TOTAL spec allocates 1 slot
  - init from ALL spec allocates 4 slots
  - increment/get round-trips through compact indices
  - counter_add works across matching counters
  - counter_free zeros everything
*/
bool
d_tests_sa_test_common_counter_spec_init
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_counter ctr;
    int                   rc;

    result = true;

    // test 1: init from ASSERT_STD
    rc = d_test_counter_init(&ctr, D_TEST_COUNTER_ASSERT_STD);

    result = d_assert_standalone(
        rc == 0,
        "spec_init_returns_zero",
        "d_test_counter_init should return 0 on success",
        _counter) && result;

    result = d_assert_standalone(
        ctr.test_type == D_TEST_TYPE_ASSERT,
        "spec_init_type",
        "init from ASSERT_STD should set type = ASSERT",
        _counter) && result;

    result = d_assert_standalone(
        ctr.fields == D_TEST_COUNT_STANDARD,
        "spec_init_fields",
        "init from ASSERT_STD should set fields = STANDARD",
        _counter) && result;

    result = d_assert_standalone(
        ctr.amount != NULL,
        "spec_init_amount_allocated",
        "init should allocate amount array",
        _counter) && result;

    // test 2: all slots initialized to zero
    result = d_assert_standalone(
        (d_test_counter_get(&ctr, D_TEST_COUNT_TOTAL) == 0) &&
        (d_test_counter_get(&ctr, D_TEST_COUNT_PASSED) == 0) &&
        (d_test_counter_get(&ctr, D_TEST_COUNT_FAILED) == 0),
        "spec_init_zeroed",
        "all active slots should be zero after init",
        _counter) && result;

    // test 3: inactive field returns 0
    result = d_assert_standalone(
        d_test_counter_get(&ctr, D_TEST_COUNT_REMAINING) == 0,
        "spec_init_inactive_zero",
        "inactive field should return 0",
        _counter) && result;

    d_test_counter_free(&ctr);

    // test 4: TOTAL-only spec allocates 1 slot
    d_test_counter_init(&ctr, D_TEST_COUNTER_ASSERT_TOTAL);

    d_test_counter_increment(&ctr, D_TEST_COUNT_TOTAL);

    result = d_assert_standalone(
        d_test_counter_get(&ctr, D_TEST_COUNT_TOTAL) == 1,
        "spec_total_only_increment",
        "TOTAL-only counter should track total",
        _counter) && result;

    // increment on inactive field is no-op
    d_test_counter_increment(&ctr, D_TEST_COUNT_PASSED);

    result = d_assert_standalone(
        d_test_counter_get(&ctr, D_TEST_COUNT_PASSED) == 0,
        "spec_total_only_inactive",
        "inactive field increment should be no-op",
        _counter) && result;

    d_test_counter_free(&ctr);

    // test 5: ALL spec — 4 slots, all work
    d_test_counter_init(&ctr, D_TEST_COUNTER_TEST_ALL);

    d_test_counter_increment(&ctr, D_TEST_COUNT_TOTAL);
    d_test_counter_increment(&ctr, D_TEST_COUNT_TOTAL);
    d_test_counter_increment(&ctr, D_TEST_COUNT_PASSED);
    d_test_counter_increment(&ctr, D_TEST_COUNT_FAILED);
    d_test_counter_increment(&ctr, D_TEST_COUNT_REMAINING);
    d_test_counter_increment(&ctr, D_TEST_COUNT_REMAINING);
    d_test_counter_increment(&ctr, D_TEST_COUNT_REMAINING);

    result = d_assert_standalone(
        (d_test_counter_get(&ctr, D_TEST_COUNT_TOTAL) == 2) &&
        (d_test_counter_get(&ctr, D_TEST_COUNT_PASSED) == 1) &&
        (d_test_counter_get(&ctr, D_TEST_COUNT_FAILED) == 1) &&
        (d_test_counter_get(&ctr, D_TEST_COUNT_REMAINING) == 3),
        "spec_all_four_fields",
        "ALL spec should track all 4 fields independently",
        _counter) && result;

    d_test_counter_free(&ctr);

    // test 6: counter_add aggregates matching fields
    {
        struct d_test_counter dest;
        struct d_test_counter src;

        d_test_counter_init(&dest, D_TEST_COUNTER_ASSERT_STD);
        d_test_counter_init(&src, D_TEST_COUNTER_ASSERT_STD);

        d_test_counter_increment(&dest, D_TEST_COUNT_TOTAL);
        d_test_counter_increment(&dest, D_TEST_COUNT_PASSED);

        d_test_counter_increment(&src, D_TEST_COUNT_TOTAL);
        d_test_counter_increment(&src, D_TEST_COUNT_TOTAL);
        d_test_counter_increment(&src, D_TEST_COUNT_FAILED);

        d_test_counter_add(&dest, &src);

        result = d_assert_standalone(
            (d_test_counter_get(&dest, D_TEST_COUNT_TOTAL) == 3) &&
            (d_test_counter_get(&dest, D_TEST_COUNT_PASSED) == 1) &&
            (d_test_counter_get(&dest, D_TEST_COUNT_FAILED) == 1),
            "spec_counter_add",
            "counter_add should sum matching fields",
            _counter) && result;

        d_test_counter_free(&dest);
        d_test_counter_free(&src);
    }

    // test 7: counter_add refuses mismatched types
    {
        struct d_test_counter dest;
        struct d_test_counter src;

        d_test_counter_init(&dest, D_TEST_COUNTER_ASSERT_STD);
        d_test_counter_init(&src, D_TEST_COUNTER_TEST_STD);

        d_test_counter_increment(&src, D_TEST_COUNT_TOTAL);
        d_test_counter_increment(&src, D_TEST_COUNT_TOTAL);

        d_test_counter_add(&dest, &src);

        result = d_assert_standalone(
            d_test_counter_get(&dest, D_TEST_COUNT_TOTAL) == 0,
            "spec_counter_add_mismatch",
            "counter_add should be no-op for mismatched types",
            _counter) && result;

        d_test_counter_free(&dest);
        d_test_counter_free(&src);
    }

    // test 8: reset zeros without realloc
    {
        struct d_test_counter rst;
        size_t*               ptr_before;

        d_test_counter_init(&rst, D_TEST_COUNTER_ASSERT_STD);

        d_test_counter_increment(&rst, D_TEST_COUNT_TOTAL);
        d_test_counter_increment(&rst, D_TEST_COUNT_PASSED);

        ptr_before = rst.amount;

        d_test_counter_reset(&rst);

        result = d_assert_standalone(
            (d_test_counter_get(&rst, D_TEST_COUNT_TOTAL) == 0) &&
            (d_test_counter_get(&rst, D_TEST_COUNT_PASSED) == 0) &&
            (rst.amount == ptr_before),
            "spec_counter_reset",
            "reset should zero slots without reallocating",
            _counter) && result;

        d_test_counter_free(&rst);
    }

    // test 9: free zeros the struct
    {
        struct d_test_counter f;

        d_test_counter_init(&f, D_TEST_COUNTER_ASSERT_STD);
        d_test_counter_free(&f);

        result = d_assert_standalone(
            (f.amount == NULL) &&
            (f.test_type == 0) &&
            (f.fields == 0),
            "spec_counter_free",
            "free should NULL amount and zero type/fields",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_test_common_counter_spec_all
  Aggregation function that runs all counter spec tests.
*/
bool
d_tests_sa_test_common_counter_spec_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Counter Spec Encoding\n");
    printf("  ---------------------------------\n");

    result = d_tests_sa_test_common_counter_spec_encode(_counter)
                 && result;
    result = d_tests_sa_test_common_counter_spec_popcount(_counter)
                 && result;
    result = d_tests_sa_test_common_counter_spec_init(_counter)
                 && result;

    return result;
}
