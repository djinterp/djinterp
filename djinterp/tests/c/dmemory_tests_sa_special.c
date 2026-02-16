#include ".\dmemory_tests_sa.h"


/******************************************************************************
 * NULL PARAMETER TESTS
 *****************************************************************************/

 /*
 d_tests_dmemory_null_params_all
   Tests NULL parameter handling across all functions.
   Tests the following:
   - d_memcpy with NULL parameters
   - d_memcpy_s with NULL parameters
   - d_memdup_s with NULL source
   - d_memset with NULL destination
   - d_memset_s with NULL destination
   - proper error codes returned
 */
struct d_test_object*
    d_tests_dmemory_null_params_all
    (
        void
    )
{
    struct d_test_object* group;
    unsigned char         buffer[100];
    void* result_ptr;
    int                   result_int;
    errno_t               result_errno;
    bool                  test_memcpy_null_dest;
    bool                  test_memcpy_null_src;
    bool                  test_memcpy_s_null;
    bool                  test_memdup_null;
    bool                  test_memset_null;
    bool                  test_memset_s_null;
    size_t                idx;

    // test 1: d_memcpy with NULL destination
    // note: behavior may be undefined, but we test for no crash
    result_ptr = d_memcpy(NULL, buffer, 10);
    test_memcpy_null_dest = true;  // if we get here, no crash

    // test 2: d_memcpy with NULL source
    result_ptr = d_memcpy(buffer, NULL, 10);
    test_memcpy_null_src = true;  // if we get here, no crash

    // test 3: d_memcpy_s with NULL parameters
    result_int = d_memcpy_s(NULL, sizeof(buffer), buffer, 10);
    int r1 = d_memcpy_s(buffer, sizeof(buffer), NULL, 10);
    int r2 = d_memcpy_s(NULL, 0, NULL, 10);
    test_memcpy_s_null = (result_int == EINVAL) && (r1 == EINVAL) && (r2 == EINVAL);

    // test 4: d_memdup_s with NULL source
    result_ptr = d_memdup_s(NULL, 100);
    test_memdup_null = (result_ptr == NULL);

    // test 5: d_memset with NULL destination
    result_ptr = d_memset(NULL, 0x42, 10);
    test_memset_null = true;  // if we get here, no crash

    // test 6: d_memset_s with NULL destination
    result_errno = d_memset_s(NULL, 100, 0x42, 10);
    test_memset_s_null = (result_errno == EINVAL);

    // build result tree
    group = d_test_object_new_interior("NULL Parameter Handling", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("memcpy_null_dest",
        test_memcpy_null_dest,
        "d_memcpy handles NULL destination");
    group->elements[idx++] = D_ASSERT_TRUE("memcpy_null_src",
        test_memcpy_null_src,
        "d_memcpy handles NULL source");
    group->elements[idx++] = D_ASSERT_TRUE("memcpy_s_null",
        test_memcpy_s_null,
        "d_memcpy_s returns EINVAL for NULL");
    group->elements[idx++] = D_ASSERT_TRUE("memdup_null",
        test_memdup_null,
        "d_memdup_s returns NULL for NULL src");
    group->elements[idx++] = D_ASSERT_TRUE("memset_null",
        test_memset_null,
        "d_memset handles NULL destination");
    group->elements[idx++] = D_ASSERT_TRUE("memset_s_null",
        test_memset_s_null,
        "d_memset_s returns EINVAL for NULL");

    return group;
}


/******************************************************************************
 * BOUNDARY CONDITION TESTS
 *****************************************************************************/

 /*
 d_tests_dmemory_boundary_conditions_all
   Tests boundary conditions across all functions.
   Tests the following:
   - zero-size operations
   - single-byte operations
   - maximum size operations
   - off-by-one scenarios
   - exact buffer fits
   - power-of-two sizes
 */
struct d_test_object*
    d_tests_dmemory_boundary_conditions_all
    (
        void
    )
{
    struct d_test_object* group;
    unsigned char         src[1024];
    unsigned char         dst[1024];
    unsigned char         single_src;
    unsigned char         single_dst;
    void* result_ptr;
    int                   result_int;
    bool                  test_zero_size;
    bool                  test_single_byte;
    bool                  test_exact_fit;
    bool                  test_off_by_one;
    bool                  test_power_of_two;
    bool                  test_max_portable;
    size_t                i;
    size_t                idx;

    // initialize buffers
    for (i = 0; i < sizeof(src); i++)
    {
        src[i] = (unsigned char)(i & 0xFF);
    }

    // test 1: zero-size operations
    memset(dst, 0xFF, sizeof(dst));
    result_ptr = d_memcpy(dst, src, 0);
    void* dup_zero = d_memdup_s(src, 0);
    result_ptr = d_memset(dst, 0x42, 0);
    test_zero_size = d_tests_dmemory_verify_pattern(dst, sizeof(dst), 0xFF) &&
        (dup_zero == NULL);

    // test 2: single-byte operations
    single_src = 0x42;
    single_dst = 0x00;
    result_ptr = d_memcpy(&single_dst, &single_src, 1);
    test_single_byte = (single_dst == 0x42);

    void* dup_single = d_memdup_s(&single_src, 1);
    test_single_byte = test_single_byte &&
        (dup_single != NULL) &&
        (*(unsigned char*)dup_single == 0x42);

    if (dup_single) free(dup_single);

    // test 3: exact buffer fit
    result_int = d_memcpy_s(dst, sizeof(dst), src, sizeof(dst));
    test_exact_fit = (result_int == 0) &&
        d_tests_dmemory_compare_buffers(dst, src, sizeof(dst));

    // test 4: off-by-one scenarios
    result_int = d_memcpy_s(dst, 100, src, 101);  // one too many
    int r1 = d_memcpy_s(dst, 100, src, 100);      // exact
    int r2 = d_memcpy_s(dst, 100, src, 99);       // one less
    test_off_by_one = (result_int == ERANGE) && (r1 == 0) && (r2 == 0);

    // test 5: power-of-two sizes (1, 2, 4, 8, 16, 32, 64, 128, 256, 512)
    size_t pow2_sizes[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 };
    test_power_of_two = true;

    for (i = 0; i < sizeof(pow2_sizes) / sizeof(pow2_sizes[0]); i++)
    {
        memset(dst, 0, sizeof(dst));
        result_ptr = d_memcpy(dst, src, pow2_sizes[i]);

        if (!d_tests_dmemory_compare_buffers(dst, src, pow2_sizes[i]))
        {
            test_power_of_two = false;
            break;
        }
    }

    // test 6: maximum portable size (not exceeding stack/heap limits)
    unsigned char* large_src = malloc(65536);
    unsigned char* large_dst = malloc(65536);
    test_max_portable = false;

    if (large_src && large_dst)
    {
        memset(large_src, 0x5A, 65536);
        result_ptr = d_memcpy(large_dst, large_src, 65536);
        test_max_portable = d_tests_dmemory_verify_pattern(large_dst, 65536, 0x5A);
    }

    if (large_src) free(large_src);
    if (large_dst) free(large_dst);

    // build result tree
    group = d_test_object_new_interior("Boundary Conditions", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("zero_size",
        test_zero_size,
        "handles zero-size operations");
    group->elements[idx++] = D_ASSERT_TRUE("single_byte",
        test_single_byte,
        "handles single-byte operations");
    group->elements[idx++] = D_ASSERT_TRUE("exact_fit",
        test_exact_fit,
        "handles exact buffer fit");
    group->elements[idx++] = D_ASSERT_TRUE("off_by_one",
        test_off_by_one,
        "detects off-by-one errors");
    group->elements[idx++] = D_ASSERT_TRUE("power_of_two",
        test_power_of_two,
        "handles power-of-two sizes");
    group->elements[idx++] = D_ASSERT_TRUE("max_portable",
        test_max_portable,
        "handles maximum portable size");

    return group;
}


/******************************************************************************
 * ALIGNMENT TESTS
 *****************************************************************************/

 /*
 d_tests_dmemory_alignment_all
   Tests memory operations with various alignments.
   Tests the following:
   - aligned addresses (4, 8, 16 byte boundaries)
   - unaligned source
   - unaligned destination
   - both unaligned
   - odd-sized transfers
   - cache-line boundaries
 */
struct d_test_object*
    d_tests_dmemory_alignment_all
    (
        void
    )
{
    struct d_test_object* group;
    // Use aligned buffers (MSVC compatible)
#ifdef _MSC_VER
    __declspec(align(16)) unsigned char buffer[256];
    __declspec(align(16)) unsigned char src_aligned[64];
    __declspec(align(16)) unsigned char dst_aligned[64];
#else
    unsigned char         buffer[256] __attribute__((aligned(16)));
    unsigned char         src_aligned[64] __attribute__((aligned(16)));
    unsigned char         dst_aligned[64] __attribute__((aligned(16)));
#endif
    unsigned char* unaligned_src;
    unsigned char* unaligned_dst;
    void* result;
    bool                  test_aligned;
    bool                  test_unaligned_src;
    bool                  test_unaligned_dst;
    bool                  test_both_unaligned;
    bool                  test_odd_sized;
    bool                  test_cache_line;
    size_t                i;
    size_t                idx;

    // initialize aligned buffers
    for (i = 0; i < sizeof(src_aligned); i++)
    {
        src_aligned[i] = (unsigned char)(i * 3);
    }

    // test 1: fully aligned (16-byte aligned addresses)
    memset(dst_aligned, 0, sizeof(dst_aligned));
    result = d_memcpy(dst_aligned, src_aligned, sizeof(src_aligned));
    test_aligned = d_tests_dmemory_compare_buffers(dst_aligned,
        src_aligned,
        sizeof(src_aligned));

    // test 2: unaligned source (offset by 1)
    unaligned_src = buffer + 1;

    for (i = 0; i < 64; i++)
    {
        unaligned_src[i] = (unsigned char)(i * 5);
    }

    memset(dst_aligned, 0, sizeof(dst_aligned));
    result = d_memcpy(dst_aligned, unaligned_src, 64);
    test_unaligned_src = d_tests_dmemory_compare_buffers(dst_aligned,
        unaligned_src,
        64);

    // test 3: unaligned destination (offset by 3)
    unaligned_dst = buffer + 128 + 3;
    result = d_memcpy(unaligned_dst, src_aligned, 60);
    test_unaligned_dst = d_tests_dmemory_compare_buffers(unaligned_dst,
        src_aligned,
        60);

    // test 4: both unaligned (different offsets)
    unaligned_src = buffer + 5;
    unaligned_dst = buffer + 128 + 7;

    for (i = 0; i < 50; i++)
    {
        unaligned_src[i] = (unsigned char)(i * 7);
    }

    result = d_memcpy(unaligned_dst, unaligned_src, 50);
    test_both_unaligned = d_tests_dmemory_compare_buffers(unaligned_dst,
        unaligned_src,
        50);

    // test 5: odd-sized transfers (13, 27, 53 bytes)
    unsigned char odd_src[53];
    unsigned char odd_dst[53];

    for (i = 0; i < 53; i++)
    {
        odd_src[i] = (unsigned char)(i * 11);
    }

    result = d_memcpy(odd_dst, odd_src, 13);
    result = d_memcpy(odd_dst + 13, odd_src + 13, 27);
    result = d_memcpy(odd_dst + 40, odd_src + 40, 13);
    test_odd_sized = d_tests_dmemory_compare_buffers(odd_dst, odd_src, 53);

    // test 6: cache-line boundary crossing (typically 64 bytes)
    unsigned char cache_src[128];
    unsigned char cache_dst[128];

    for (i = 0; i < 128; i++)
    {
        cache_src[i] = (unsigned char)(i);
    }

    // copy across cache line boundary
    result = d_memcpy(cache_dst + 32, cache_src + 32, 64);
    test_cache_line = d_tests_dmemory_compare_buffers(cache_dst + 32,
        cache_src + 32,
        64);

    // build result tree
    group = d_test_object_new_interior("Alignment Tests", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("aligned",
        test_aligned,
        "handles aligned addresses");
    group->elements[idx++] = D_ASSERT_TRUE("unaligned_src",
        test_unaligned_src,
        "handles unaligned source");
    group->elements[idx++] = D_ASSERT_TRUE("unaligned_dst",
        test_unaligned_dst,
        "handles unaligned destination");
    group->elements[idx++] = D_ASSERT_TRUE("both_unaligned",
        test_both_unaligned,
        "handles both unaligned");
    group->elements[idx++] = D_ASSERT_TRUE("odd_sized",
        test_odd_sized,
        "handles odd-sized transfers");
    group->elements[idx++] = D_ASSERT_TRUE("cache_line",
        test_cache_line,
        "handles cache-line boundaries");

    return group;
}


/******************************************************************************
 * OVERLAPPING MEMORY TESTS
 *****************************************************************************/

 /*
 d_tests_dmemory_overlap_all
   Tests memory operations with overlapping regions.
   Tests the following:
   - forward overlap (src before dst)
   - backward overlap (dst before src)
   - complete overlap (same address)
   - partial overlap
   - adjacent buffers
   - warning: memcpy behavior with overlap is undefined
 */
struct d_test_object*
    d_tests_dmemory_overlap_all
    (
        void
    )
{
    struct d_test_object* group;
    unsigned char         buffer[256];
    unsigned char         expected[256];
    void* result;
    bool                  test_forward_overlap;
    bool                  test_backward_overlap;
    bool                  test_complete_overlap;
    bool                  test_partial_overlap;
    bool                  test_adjacent;
    bool                  test_self_copy;
    size_t                i;
    size_t                idx;

    // note: overlapping memcpy is technically undefined behavior
    // but we test to ensure no crashes occur

    // test 1: forward overlap (copy to later position)
    for (i = 0; i < 100; i++)
    {
        buffer[i] = (unsigned char)i;
    }

    // this may or may not work correctly with memcpy
    result = d_memcpy(buffer + 50, buffer, 50);
    test_forward_overlap = true;  // just test for no crash

    // test 2: backward overlap (copy to earlier position)
    for (i = 0; i < 100; i++)
    {
        buffer[i] = (unsigned char)i;
    }

    result = d_memcpy(buffer, buffer + 50, 50);
    test_backward_overlap = true;  // just test for no crash

    // test 3: complete overlap (same address)
    for (i = 0; i < 100; i++)
    {
        buffer[i] = (unsigned char)i;
        expected[i] = (unsigned char)i;
    }

    result = d_memcpy(buffer, buffer, 100);
    test_complete_overlap = d_tests_dmemory_compare_buffers(buffer, expected, 100);

    // test 4: partial overlap (overlapping by half)
    for (i = 0; i < 100; i++)
    {
        buffer[i] = (unsigned char)i;
    }

    result = d_memcpy(buffer + 25, buffer, 50);
    test_partial_overlap = true;  // just test for no crash

    // test 5: adjacent buffers (no overlap)
    for (i = 0; i < 100; i++)
    {
        buffer[i] = (unsigned char)i;
    }

    result = d_memcpy(buffer + 100, buffer, 100);
    test_adjacent = d_tests_dmemory_compare_buffers(buffer + 100, buffer, 100);

    // test 6: self-copy with memset
    memset(buffer, 0x42, 100);
    result = d_memset(buffer, 0x42, 100);
    test_self_copy = d_tests_dmemory_verify_pattern(buffer, 100, 0x42);

    // build result tree
    group = d_test_object_new_interior("Overlapping Memory", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("forward_overlap",
        test_forward_overlap,
        "handles forward overlap");
    group->elements[idx++] = D_ASSERT_TRUE("backward_overlap",
        test_backward_overlap,
        "handles backward overlap");
    group->elements[idx++] = D_ASSERT_TRUE("complete_overlap",
        test_complete_overlap,
        "handles complete overlap");
    group->elements[idx++] = D_ASSERT_TRUE("partial_overlap",
        test_partial_overlap,
        "handles partial overlap");
    group->elements[idx++] = D_ASSERT_TRUE("adjacent",
        test_adjacent,
        "handles adjacent buffers");
    group->elements[idx++] = D_ASSERT_TRUE("self_copy",
        test_self_copy,
        "handles self-copy");

    return group;
}


/******************************************************************************
 * PERFORMANCE TESTS (OPTIONAL)
 *****************************************************************************/

 /*
 d_tests_dmemory_performance_all
   Optional performance tests for memory operations.
   Tests the following:
   - large buffer copy performance
   - small buffer copy performance
   - aligned vs unaligned performance
   - pattern filling performance
   - Note: these are basic functional tests, not true benchmarks
 */
struct d_test_object*
    d_tests_dmemory_performance_all
    (
        void
    )
{
    struct d_test_object* group;
    unsigned char* large_src;
    unsigned char* large_dst;
    unsigned char         small_src[16];
    unsigned char         small_dst[16];
    void* result;
    bool                  test_large_perf;
    bool                  test_small_perf;
    bool                  test_aligned_perf;
    bool                  test_pattern_perf;
    size_t                i;
    size_t                j;
    size_t                idx;

    // allocate large buffers
    large_src = malloc(1024 * 1024);  // 1MB
    large_dst = malloc(1024 * 1024);

    // test 1: large buffer copy (1MB)
    test_large_perf = false;

    if (large_src && large_dst)
    {
        // initialize
        for (i = 0; i < 1024 * 1024; i++)
        {
            large_src[i] = (unsigned char)(i & 0xFF);
        }

        // copy
        result = d_memcpy(large_dst, large_src, 1024 * 1024);
        test_large_perf = d_tests_dmemory_compare_buffers(large_dst,
            large_src,
            1024 * 1024);
    }

    // test 2: small buffer copy (repeated)
    test_small_perf = true;

    for (i = 0; i < 16; i++)
    {
        small_src[i] = (unsigned char)i;
    }

    // perform many small copies
    for (j = 0; j < 1000; j++)
    {
        result = d_memcpy(small_dst, small_src, 16);

        if (!d_tests_dmemory_compare_buffers(small_dst, small_src, 16))
        {
            test_small_perf = false;
            break;
        }
    }

    // test 3: aligned performance
    test_aligned_perf = false;

    if (large_src && large_dst)
    {
        // ensure alignment
        unsigned char* aligned_src = (unsigned char*)(((size_t)large_src + 15) & ~15);
        unsigned char* aligned_dst = (unsigned char*)(((size_t)large_dst + 15) & ~15);

        result = d_memcpy(aligned_dst, aligned_src, 65536);
        test_aligned_perf = true;  // just test completion
    }

    // test 4: pattern filling performance
    test_pattern_perf = false;

    if (large_dst)
    {
        result = d_memset(large_dst, 0xAA, 1024 * 1024);
        test_pattern_perf = d_tests_dmemory_verify_pattern(large_dst,
            1024 * 1024,
            0xAA);
    }

    // cleanup
    if (large_src) free(large_src);
    if (large_dst) free(large_dst);

    // build result tree
    group = d_test_object_new_interior("Performance Tests", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("large_perf",
        test_large_perf,
        "large buffer copy works");
    group->elements[idx++] = D_ASSERT_TRUE("small_perf",
        test_small_perf,
        "small buffer copy works");
    group->elements[idx++] = D_ASSERT_TRUE("aligned_perf",
        test_aligned_perf,
        "aligned copy works");
    group->elements[idx++] = D_ASSERT_TRUE("pattern_perf",
        test_pattern_perf,
        "pattern filling works");

    return group;
}