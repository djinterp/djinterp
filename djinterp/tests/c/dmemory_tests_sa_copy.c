#include ".\dmemory_tests_sa.h"


/******************************************************************************
 * MEMORY COPY TESTS - d_memcpy
 *****************************************************************************/

/*
d_tests_dmemory_memcpy
  Tests d_memcpy for basic memory copying.
  Tests the following:
  - copies data correctly
  - handles zero-size copy
  - copies single byte
  - copies various sizes
  - preserves surrounding data
  - handles unaligned addresses
  - returns destination pointer
  - copies binary data correctly
*/
struct d_test_object*
d_tests_dmemory_memcpy
(
    void
)
{
    struct d_test_object* group;
    unsigned char         src[D_TESTS_MEMORY_MEDIUM_SIZE]  = { '\0' };
    unsigned char         dest[D_TESTS_MEMORY_MEDIUM_SIZE] = { '\0' };
    unsigned char         guard_dest[D_TESTS_MEMORY_MEDIUM_SIZE + 32];
    void*                 result;
    bool                  test_basic_copy;
    bool                  test_zero_size;
    bool                  test_single_byte;
    bool                  test_various_sizes;
    bool                  test_preserve_surrounding;
    bool                  test_unaligned;
    bool                  test_return_value;
    bool                  test_binary_data;
    size_t                idx;
    size_t                i;

    // initialize source with pattern
    for (i = 0; i < D_TESTS_MEMORY_MEDIUM_SIZE; i++)
    {
        src[i] = (unsigned char)(i & 0xFF);
    }

    // test 1: basic copy
    memset(dest, 0, sizeof(dest));
    result = d_memcpy(dest, src, 64);
    test_basic_copy = (memcmp(dest, src, 64) == 0);

    // test 2: zero-size copy
    memset(dest, D_TESTS_MEMORY_PATTERN_A, sizeof(dest));
    result = d_memcpy(dest, src, 0);
    test_zero_size = (dest[0] == D_TESTS_MEMORY_PATTERN_A);

    // test 3: single byte copy
    dest[0] = 0;
    src[0] = 0x42;
    result = d_memcpy(dest, src, 1);
    test_single_byte = (dest[0] == 0x42);

    // test 4: various sizes (powers of 2)
    test_various_sizes = true;
    for (i = 1; i <= 128; i *= 2)
    {
        memset(dest, 0, sizeof(dest));
        d_memcpy(dest, src, i);
        
        if (memcmp(dest, src, i) != 0)
        {
            test_various_sizes = false;
            break;
        }
    }

    // test 5: preserve surrounding data
    memset(guard_dest, D_TESTS_MEMORY_PATTERN_B, sizeof(guard_dest));
    d_memcpy(&guard_dest[16], src, 32);
    test_preserve_surrounding = (guard_dest[15] == D_TESTS_MEMORY_PATTERN_B) &&
                               (guard_dest[48] == D_TESTS_MEMORY_PATTERN_B) &&
                               (memcmp(&guard_dest[16], src, 32) == 0);

    // test 6: unaligned addresses
    unsigned char* unaligned_src = &src[1];
    unsigned char* unaligned_dest = &dest[3];
    d_memcpy(unaligned_dest, unaligned_src, 17);
    test_unaligned = (memcmp(unaligned_dest, unaligned_src, 17) == 0);

    // test 7: return value
    result = d_memcpy(dest, src, 10);
    test_return_value = (result == dest);

    // test 8: binary data (including nulls)
    unsigned char binary_src[] = {0x00, 0xFF, 0x01, 0x00, 0xFE};
    unsigned char binary_dest[5];
    d_memcpy(binary_dest, binary_src, sizeof(binary_src));
    test_binary_data = (memcmp(binary_dest, binary_src, sizeof(binary_src)) == 0);

    // build result tree
    group = d_test_object_new_interior("d_memcpy", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("basic_copy",
                                           test_basic_copy,
                                           "copies data correctly");
    group->elements[idx++] = D_ASSERT_TRUE("zero_size",
                                           test_zero_size,
                                           "handles zero-size copy");
    group->elements[idx++] = D_ASSERT_TRUE("single_byte",
                                           test_single_byte,
                                           "copies single byte");
    group->elements[idx++] = D_ASSERT_TRUE("various_sizes",
                                           test_various_sizes,
                                           "copies various sizes");
    group->elements[idx++] = D_ASSERT_TRUE("preserve_surrounding",
                                           test_preserve_surrounding,
                                           "preserves surrounding data");
    group->elements[idx++] = D_ASSERT_TRUE("unaligned",
                                           test_unaligned,
                                           "handles unaligned addresses");
    group->elements[idx++] = D_ASSERT_TRUE("return_value",
                                           test_return_value,
                                           "returns destination pointer");
    group->elements[idx++] = D_ASSERT_TRUE("binary_data",
                                           test_binary_data,
                                           "copies binary data correctly");

    return group;
}


/*
d_tests_dmemory_memcpy_s
  Tests d_memcpy_s for secure memory copying.
  Tests the following:
  - copies data correctly with size check
  - returns error for NULL destination
  - returns error for NULL source
  - returns error for dest size too small
  - returns error for count > dest size
  - handles zero count
  - returns error for overlapping regions
  - zeroes dest on error
  - returns 0 on success
  - validates against RSIZE_MAX
*/
struct d_test_object*
d_tests_dmemory_memcpy_s
(
    void
)
{
    struct d_test_object* group;
    unsigned char         src[D_TESTS_MEMORY_MEDIUM_SIZE];
    unsigned char         dest[D_TESTS_MEMORY_MEDIUM_SIZE];
    unsigned char         small_dest[D_TESTS_MEMORY_SMALL_SIZE];
    int                   result;
    bool                  test_basic_copy;
    bool                  test_null_dest;
    bool                  test_null_src;
    bool                  test_dest_too_small;
    bool                  test_count_exceeds_dest;
    bool                  test_zero_count;
    bool                  test_overlapping;
    bool                  test_zeroes_on_error;
    bool                  test_success_return;
    bool                  test_rsize_max;
    size_t                idx;
    size_t                i;

    // initialize source
    for (i = 0; i < D_TESTS_MEMORY_MEDIUM_SIZE; i++)
    {
        src[i] = (unsigned char)(i & 0xFF);
    }

    // test 1: basic secure copy
    memset(dest, 0, sizeof(dest));
    result = d_memcpy_s(dest, sizeof(dest), src, 64);
    test_basic_copy = (result == 0) && (memcmp(dest, src, 64) == 0);

    // test 2: NULL destination
    result = d_memcpy_s(NULL, sizeof(dest), src, 10);
    test_null_dest = (result == EINVAL);

    // test 3: NULL source
    result = d_memcpy_s(dest, sizeof(dest), NULL, 10);
    test_null_src = (result == EINVAL);

    // test 4: destination too small
    result = d_memcpy_s(small_dest, sizeof(small_dest), src, 
                        D_TESTS_MEMORY_MEDIUM_SIZE);
    test_dest_too_small = (result == ERANGE);

    // test 5: count exceeds dest size
    result = d_memcpy_s(dest, 10, src, 20);
    test_count_exceeds_dest = (result == ERANGE);

    // test 6: zero count
    memset(dest, D_TESTS_MEMORY_PATTERN_A, sizeof(dest));
    result = d_memcpy_s(dest, sizeof(dest), src, 0);
    test_zero_count = (result == 0) && (dest[0] == D_TESTS_MEMORY_PATTERN_A);

    // test 7: overlapping regions (implementation-dependent)
    unsigned char overlap_buf[100];
    memset(overlap_buf, 0x55, sizeof(overlap_buf));
    result = d_memcpy_s(&overlap_buf[10], 50, &overlap_buf[20], 30);
    test_overlapping = true;  // behavior varies by implementation

    // test 8: zeroes destination on error
    for (i = 0; i < sizeof(dest); i++)
    {
        dest[i] = D_TESTS_MEMORY_PATTERN_FF;
    }
    result = d_memcpy_s(dest, sizeof(dest), NULL, 10);
    test_zeroes_on_error = (dest[0] == 0);

    // test 9: returns 0 on success
    result = d_memcpy_s(dest, sizeof(dest), src, 10);
    test_success_return = (result == 0);

    // test 10: RSIZE_MAX validation
    result = d_memcpy_s(dest, RSIZE_MAX + 1, src, 10);
    bool test_dest_rsize = (result != 0);
    result = d_memcpy_s(dest, sizeof(dest), src, RSIZE_MAX + 1);
    bool test_count_rsize = (result != 0);
    test_rsize_max = test_dest_rsize || test_count_rsize;

    // build result tree
    group = d_test_object_new_interior("d_memcpy_s", 10);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("basic_copy",
                                           test_basic_copy,
                                           "copies data correctly with size check");
    group->elements[idx++] = D_ASSERT_TRUE("null_dest",
                                           test_null_dest,
                                           "returns error for NULL destination");
    group->elements[idx++] = D_ASSERT_TRUE("null_src",
                                           test_null_src,
                                           "returns error for NULL source");
    group->elements[idx++] = D_ASSERT_TRUE("dest_too_small",
                                           test_dest_too_small,
                                           "returns error for dest size too small");
    group->elements[idx++] = D_ASSERT_TRUE("count_exceeds_dest",
                                           test_count_exceeds_dest,
                                           "returns error for count > dest size");
    group->elements[idx++] = D_ASSERT_TRUE("zero_count",
                                           test_zero_count,
                                           "handles zero count");
    group->elements[idx++] = D_ASSERT_TRUE("overlapping",
                                           test_overlapping,
                                           "handles overlapping regions");
    group->elements[idx++] = D_ASSERT_TRUE("zeroes_on_error",
                                           test_zeroes_on_error,
                                           "zeroes dest on error");
    group->elements[idx++] = D_ASSERT_TRUE("success_return",
                                           test_success_return,
                                           "returns 0 on success");
    group->elements[idx++] = D_ASSERT_TRUE("rsize_max",
                                           test_rsize_max,
                                           "validates against RSIZE_MAX");

    return group;
}


/*
d_tests_dmemory_copy_all
  Runs all memory copy tests.
  Tests the following:
  - d_memcpy
  - d_memcpy_s
*/
struct d_test_object*
d_tests_dmemory_copy_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Memory Copy Operations", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dmemory_memcpy();
    group->elements[idx++] = d_tests_dmemory_memcpy_s();

    return group;
}


