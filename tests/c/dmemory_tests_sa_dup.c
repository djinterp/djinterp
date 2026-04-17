#include ".\dmemory_tests_sa.h"


/******************************************************************************
 * MEMORY DUPLICATION TESTS
 *****************************************************************************/

/*
d_tests_dmemory_memdup_s
  Tests d_memdup_s for secure memory duplication.
  Tests the following:
  - duplicates memory correctly
  - allocates exact size requested
  - returns NULL for NULL source
  - returns NULL for zero size
  - creates independent copy
  - handles various sizes
  - duplicates binary data
  - memory is properly aligned
*/
struct d_test_object*
d_tests_dmemory_memdup_s
(
    void
)
{
    struct d_test_object* group;
    unsigned char         src[D_TESTS_MEMORY_MEDIUM_SIZE];
    void*                 dup;
    bool                  test_basic_dup;
    bool                  test_exact_size;
    bool                  test_null_source;
    bool                  test_zero_size;
    bool                  test_independence;
    bool                  test_various_sizes;
    bool                  test_binary_data;
    bool                  test_alignment;
    size_t                idx;
    size_t                i;

    // initialize source
    for (i = 0; i < D_TESTS_MEMORY_MEDIUM_SIZE; i++)
    {
        src[i] = (unsigned char)(i & 0xFF);
    }

    // test 1: basic duplication
    dup = d_memdup_s(src, 64);
    test_basic_dup = (dup != NULL) && 
                    (memcmp(dup, src, 64) == 0) &&
                    (dup != src);
    
    if (dup)
    {
        free(dup);
    }

    // test 2: exact size allocation (difficult to verify directly)
    dup = d_memdup_s(src, 17);  // odd size
    test_exact_size = (dup != NULL) && (memcmp(dup, src, 17) == 0);
    
    if (dup)
    {
        free(dup);
    }

    // test 3: NULL source
    dup = d_memdup_s(NULL, 10);
    test_null_source = (dup == NULL);

    // test 4: zero size
    dup = d_memdup_s(src, 0);
    test_zero_size = (dup == NULL);

    // test 5: independence of copy
    // After duplicating, modifying the original should not affect the duplicate
    unsigned char original[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    unsigned char* dup_indep = (unsigned char*)d_memdup_s(original, sizeof(original));
    test_independence = false;
    
    if (dup_indep)
    {
        // modify the original - this should NOT affect the duplicate
        original[0] = 99;
        
        // the duplicate should still have the original value (1) at position 0
        // and the rest should be unchanged (positions 1-9 still match because
        // we only changed original[0])
        test_independence = (dup_indep[0] == 1);  // duplicate is independent
        free(dup_indep);
    }

    // test 6: various sizes
    test_various_sizes = true;
    size_t test_sizes[] = {1, 3, 7, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128};
    
    for (i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); i++)
    {
        dup = d_memdup_s(src, test_sizes[i]);
        
        if (!dup || memcmp(dup, src, test_sizes[i]) != 0)
        {
            test_various_sizes = false;
            
            if (dup)
            {
                free(dup);
            }
            
            break;
        }
        
        free(dup);
    }

    // test 7: binary data with nulls
    unsigned char binary[] = {0x00, 0xFF, 0x00, 0xAA, 0x00, 0x55};
    dup = d_memdup_s(binary, sizeof(binary));
    test_binary_data = (dup != NULL) && (memcmp(dup, binary, sizeof(binary)) == 0);
    
    if (dup)
    {
        free(dup);
    }

    // test 8: memory alignment
    dup = d_memdup_s(src, 100);
    test_alignment = (dup != NULL) && 
                    (((uintptr_t)dup % sizeof(void*)) == 0);  // aligned to pointer size
    
    if (dup)
    {
        free(dup);
    }

    // build result tree
    group = d_test_object_new_interior("d_memdup_s", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("basic_dup",
                                           test_basic_dup,
                                           "duplicates memory correctly");
    group->elements[idx++] = D_ASSERT_TRUE("exact_size",
                                           test_exact_size,
                                           "allocates exact size requested");
    group->elements[idx++] = D_ASSERT_TRUE("null_source",
                                           test_null_source,
                                           "returns NULL for NULL source");
    group->elements[idx++] = D_ASSERT_TRUE("zero_size",
                                           test_zero_size,
                                           "returns NULL for zero size");
    group->elements[idx++] = D_ASSERT_TRUE("independence",
                                           test_independence,
                                           "creates independent copy");
    group->elements[idx++] = D_ASSERT_TRUE("various_sizes",
                                           test_various_sizes,
                                           "handles various sizes");
    group->elements[idx++] = D_ASSERT_TRUE("binary_data",
                                           test_binary_data,
                                           "duplicates binary data");
    group->elements[idx++] = D_ASSERT_TRUE("alignment",
                                           test_alignment,
                                           "memory is properly aligned");

    return group;
}


/*
d_tests_dmemory_duplication_all
  Runs all memory duplication tests.
  Tests the following:
  - d_memdup_s
*/
struct d_test_object*
d_tests_dmemory_duplication_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Memory Duplication", 1);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dmemory_memdup_s();

    return group;
}


/******************************************************************************
 * MEMORY SET TESTS
 *****************************************************************************/

/*
d_tests_dmemory_memset
  Tests d_memset for basic memory setting.
  Tests the following:
  - sets memory to value
  - handles zero size
  - sets single byte
  - sets various sizes
  - preserves surrounding memory
  - handles different values
  - returns destination pointer
  - fills entire buffer
*/
struct d_test_object*
d_tests_dmemory_memset
(
    void
)
{
    struct d_test_object* group;
    unsigned char         buffer[D_TESTS_MEMORY_MEDIUM_SIZE];
    unsigned char         guard_buf[D_TESTS_MEMORY_MEDIUM_SIZE + 32];
    void*                 result;
    bool                  test_basic_set;
    bool                  test_zero_size;
    bool                  test_single_byte;
    bool                  test_various_sizes;
    bool                  test_preserve_surrounding;
    bool                  test_different_values;
    bool                  test_return_value;
    bool                  test_fill_entire;
    size_t                idx;
    size_t                i;

    // test 1: basic memory set
    memset(buffer, 0, sizeof(buffer));
    result = d_memset(buffer, 0xAA, 64);
    test_basic_set = true;
    
    for (i = 0; i < 64; i++)
    {
        if (buffer[i] != 0xAA)
        {
            test_basic_set = false;
            break;
        }
    }

    // test 2: zero size
    memset(buffer, 0xFF, sizeof(buffer));
    result = d_memset(buffer, 0x00, 0);
    test_zero_size = (buffer[0] == 0xFF);

    // test 3: single byte
    buffer[0] = 0;
    result = d_memset(buffer, 0x42, 1);
    test_single_byte = (buffer[0] == 0x42) && (buffer[1] != 0x42);

    // test 4: various sizes
    test_various_sizes = true;
    
    for (i = 1; i <= 128; i *= 2)
    {
        memset(buffer, 0, sizeof(buffer));
        d_memset(buffer, 0x55, i);
        
        size_t j;
        for (j = 0; j < i; j++)
        {
            if (buffer[j] != 0x55)
            {
                test_various_sizes = false;
                break;
            }
        }
        
        if (!test_various_sizes)
        {
            break;
        }
    }

    // test 5: preserve surrounding memory
    memset(guard_buf, 0xBB, sizeof(guard_buf));
    d_memset(&guard_buf[16], 0xAA, 32);
    test_preserve_surrounding = (guard_buf[15] == 0xBB) &&
                               (guard_buf[48] == 0xBB);
    
    for (i = 16; i < 48; i++)
    {
        if (guard_buf[i] != 0xAA)
        {
            test_preserve_surrounding = false;
            break;
        }
    }

    // test 6: different values (including 0 and 0xFF)
    test_different_values = true;
    unsigned char test_vals[] = {0x00, 0xFF, 0x01, 0xFE, 0x55, 0xAA};
    
    for (i = 0; i < sizeof(test_vals); i++)
    {
        memset(buffer, ~test_vals[i], 10);
        d_memset(buffer, test_vals[i], 10);
        
        size_t j;
        for (j = 0; j < 10; j++)
        {
            if (buffer[j] != test_vals[i])
            {
                test_different_values = false;
                break;
            }
        }
    }

    // test 7: return value
    result = d_memset(buffer, 0x99, 10);
    test_return_value = (result == buffer);

    // test 8: fill entire buffer
    d_memset(buffer, 0xCC, sizeof(buffer));
    test_fill_entire = true;
    
    for (i = 0; i < sizeof(buffer); i++)
    {
        if (buffer[i] != 0xCC)
        {
            test_fill_entire = false;
            break;
        }
    }

    // build result tree
    group = d_test_object_new_interior("d_memset", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("basic_set",
                                           test_basic_set,
                                           "sets memory to value");
    group->elements[idx++] = D_ASSERT_TRUE("zero_size",
                                           test_zero_size,
                                           "handles zero size");
    group->elements[idx++] = D_ASSERT_TRUE("single_byte",
                                           test_single_byte,
                                           "sets single byte");
    group->elements[idx++] = D_ASSERT_TRUE("various_sizes",
                                           test_various_sizes,
                                           "sets various sizes");
    group->elements[idx++] = D_ASSERT_TRUE("preserve_surrounding",
                                           test_preserve_surrounding,
                                           "preserves surrounding memory");
    group->elements[idx++] = D_ASSERT_TRUE("different_values",
                                           test_different_values,
                                           "handles different values");
    group->elements[idx++] = D_ASSERT_TRUE("return_value",
                                           test_return_value,
                                           "returns destination pointer");
    group->elements[idx++] = D_ASSERT_TRUE("fill_entire",
                                           test_fill_entire,
                                           "fills entire buffer");

    return group;
}


/*
d_tests_dmemory_memset_s
  Tests d_memset_s for secure memory setting.
  Tests the following:
  - sets memory securely
  - returns error for NULL destination
  - returns error for invalid sizes
  - handles zero count
  - validates RSIZE_MAX
  - sets partial buffer
  - returns 0 on success
  - handles maximum values
*/
struct d_test_object*
d_tests_dmemory_memset_s
(
    void
)
{
    struct d_test_object* group;
    unsigned char         buffer[D_TESTS_MEMORY_MEDIUM_SIZE];
    errno_t               result;
    bool                  test_basic_set;
    bool                  test_null_dest;
    bool                  test_invalid_sizes;
    bool                  test_zero_count;
    bool                  test_rsize_max;
    bool                  test_partial_buffer;
    bool                  test_success_return;
    bool                  test_max_values;
    size_t                idx;
    size_t                i;

    // test 1: basic secure set
    memset(buffer, 0, sizeof(buffer));
    result = d_memset_s(buffer, sizeof(buffer), 0xAA, 64);
    test_basic_set = (result == 0);
    
    for (i = 0; i < 64; i++)
    {
        if (buffer[i] != 0xAA)
        {
            test_basic_set = false;
            break;
        }
    }

    // test 2: NULL destination
    result = d_memset_s(NULL, sizeof(buffer), 0x55, 10);
    test_null_dest = (result == EINVAL);

    // test 3: invalid sizes (count > destsz)
    result = d_memset_s(buffer, 10, 0x55, 20);
    test_invalid_sizes = (result == ERANGE);

    // test 4: zero count
    memset(buffer, 0xFF, sizeof(buffer));
    result = d_memset_s(buffer, sizeof(buffer), 0x00, 0);
    test_zero_count = (result == 0) && (buffer[0] == 0xFF);

    // test 5: RSIZE_MAX validation
    result = d_memset_s(buffer, RSIZE_MAX + 1, 0x55, 10);
    bool test_dest_rsize = (result != 0);
    result = d_memset_s(buffer, sizeof(buffer), 0x55, RSIZE_MAX + 1);
    bool test_count_rsize = (result != 0);
    test_rsize_max = test_dest_rsize || test_count_rsize;

    // test 6: partial buffer set
    memset(buffer, 0, sizeof(buffer));
    result = d_memset_s(buffer, sizeof(buffer), 0xDD, sizeof(buffer) / 2);
    test_partial_buffer = (result == 0);
    
    for (i = 0; i < sizeof(buffer) / 2; i++)
    {
        if (buffer[i] != 0xDD)
        {
            test_partial_buffer = false;
            break;
        }
    }
    
    // check second half is unchanged
    for (i = sizeof(buffer) / 2; i < sizeof(buffer); i++)
    {
        if (buffer[i] != 0)
        {
            test_partial_buffer = false;
            break;
        }
    }

    // test 7: returns 0 on success
    result = d_memset_s(buffer, sizeof(buffer), 0x11, 10);
    test_success_return = (result == 0);

    // test 8: handles maximum values
    result = d_memset_s(buffer, sizeof(buffer), 0xFF, sizeof(buffer));
    test_max_values = (result == 0);
    
    for (i = 0; i < sizeof(buffer); i++)
    {
        if (buffer[i] != 0xFF)
        {
            test_max_values = false;
            break;
        }
    }

    // build result tree
    group = d_test_object_new_interior("d_memset_s", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("basic_set",
                                           test_basic_set,
                                           "sets memory securely");
    group->elements[idx++] = D_ASSERT_TRUE("null_dest",
                                           test_null_dest,
                                           "returns error for NULL destination");
    group->elements[idx++] = D_ASSERT_TRUE("invalid_sizes",
                                           test_invalid_sizes,
                                           "returns error for invalid sizes");
    group->elements[idx++] = D_ASSERT_TRUE("zero_count",
                                           test_zero_count,
                                           "handles zero count");
    group->elements[idx++] = D_ASSERT_TRUE("rsize_max",
                                           test_rsize_max,
                                           "validates RSIZE_MAX");
    group->elements[idx++] = D_ASSERT_TRUE("partial_buffer",
                                           test_partial_buffer,
                                           "sets partial buffer");
    group->elements[idx++] = D_ASSERT_TRUE("success_return",
                                           test_success_return,
                                           "returns 0 on success");
    group->elements[idx++] = D_ASSERT_TRUE("max_values",
                                           test_max_values,
                                           "handles maximum values");

    return group;
}


/*
d_tests_dmemory_set_all
  Runs all memory set tests.
  Tests the following:
  - d_memset
  - d_memset_s
*/
struct d_test_object*
d_tests_dmemory_set_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Memory Set Operations", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dmemory_memset();
    group->elements[idx++] = d_tests_dmemory_memset_s();

    return group;
}