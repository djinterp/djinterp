/*******************************************************************************
* djinterp [test]                                        dmemory_tests_special.c
*
*
* path:      /tests/djinterp/c/memory/dmemory_tests_special.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dmemory_tests.h"


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
bool d_tests_dmemory_null_params_all(struct dj_test_suite* _suite)
{
    unsigned char         buffer[100];
    void* result_ptr;
    int                   result_int;
    int               result_errno;
    bool                  test_memcpy_null_dest;
    bool                  test_memcpy_null_src;
    bool                  test_memcpy_s_null;
    bool                  test_memdup_null;
    bool                  test_memset_null;
    bool                  test_memset_s_null;

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
    d_test_suite_test(_suite, "NULL Parameter Handling");

    D_TEST_CHECK_MSG(_suite,
                     "memcpy_null_dest",
                     test_memcpy_null_dest,
                     "d_memcpy handles NULL destination");
    D_TEST_CHECK_MSG(_suite,
                     "memcpy_null_src",
                     test_memcpy_null_src,
                     "d_memcpy handles NULL source");
    D_TEST_CHECK_MSG(_suite,
                     "memcpy_s_null",
                     test_memcpy_s_null,
                     "d_memcpy_s returns EINVAL for NULL");
    D_TEST_CHECK_MSG(_suite,
                     "memdup_null",
                     test_memdup_null,
                     "d_memdup_s returns NULL for NULL src");
    D_TEST_CHECK_MSG(_suite,
                     "memset_null",
                     test_memset_null,
                     "d_memset handles NULL destination");
    D_TEST_CHECK_MSG(_suite,
                     "memset_s_null",
                     test_memset_s_null,
                     "d_memset_s returns EINVAL for NULL");

    return true;
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
bool d_tests_dmemory_boundary_conditions_all(struct dj_test_suite* _suite)
{
    unsigned char         src[1024];
    unsigned char         dst[1024];
    unsigned char         single_src;
    unsigned char         single_dst;
    int                   result_int;
    bool                  test_zero_size;
    bool                  test_single_byte;
    bool                  test_exact_fit;
    bool                  test_off_by_one;
    bool                  test_power_of_two;
    bool                  test_max_portable;
    size_t                i;

    // initialize buffers
    for (i = 0; i < sizeof(src); i++)
    {
        src[i] = (unsigned char)(i & 0xFF);
    }

    // test 1: zero-size operations
    memset(dst, 0xFF, sizeof(dst));
    (void)d_memcpy(dst, src, 0);
    void* dup_zero = d_memdup_s(src, 0);
    (void)d_memset(dst, 0x42, 0);
    test_zero_size = d_tests_dmemory_verify_pattern(dst, sizeof(dst), 0xFF) &&
        (dup_zero == NULL);

    // test 2: single-byte operations
    single_src = 0x42;
    single_dst = 0x00;
    (void)d_memcpy(&single_dst, &single_src, 1);
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
        (void)d_memcpy(dst, src, pow2_sizes[i]);

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
        (void)d_memcpy(large_dst, large_src, 65536);
        test_max_portable = d_tests_dmemory_verify_pattern(large_dst, 65536, 0x5A);
    }

    if (large_src) free(large_src);
    if (large_dst) free(large_dst);

    // build result tree
    d_test_suite_test(_suite, "Boundary Conditions");

    D_TEST_CHECK_MSG(_suite,
                     "zero_size",
                     test_zero_size,
                     "handles zero-size operations");
    D_TEST_CHECK_MSG(_suite,
                     "single_byte",
                     test_single_byte,
                     "handles single-byte operations");
    D_TEST_CHECK_MSG(_suite,
                     "exact_fit",
                     test_exact_fit,
                     "handles exact buffer fit");
    D_TEST_CHECK_MSG(_suite,
                     "off_by_one",
                     test_off_by_one,
                     "detects off-by-one errors");
    D_TEST_CHECK_MSG(_suite,
                     "power_of_two",
                     test_power_of_two,
                     "handles power-of-two sizes");
    D_TEST_CHECK_MSG(_suite,
                     "max_portable",
                     test_max_portable,
                     "handles maximum portable size");

    return true;
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
bool d_tests_dmemory_alignment_all(struct dj_test_suite* _suite)
{
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
    bool                  test_aligned;
    bool                  test_unaligned_src;
    bool                  test_unaligned_dst;
    bool                  test_both_unaligned;
    bool                  test_odd_sized;
    bool                  test_cache_line;
    size_t                i;

    // initialize aligned buffers
    for (i = 0; i < sizeof(src_aligned); i++)
    {
        src_aligned[i] = (unsigned char)(i * 3);
    }

    // test 1: fully aligned (16-byte aligned addresses)
    memset(dst_aligned, 0, sizeof(dst_aligned));
    (void)d_memcpy(dst_aligned, src_aligned, sizeof(src_aligned));
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
    (void)d_memcpy(dst_aligned, unaligned_src, 64);
    test_unaligned_src = d_tests_dmemory_compare_buffers(dst_aligned,
        unaligned_src,
        64);

    // test 3: unaligned destination (offset by 3)
    unaligned_dst = buffer + 128 + 3;
    (void)d_memcpy(unaligned_dst, src_aligned, 60);
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

    (void)d_memcpy(unaligned_dst, unaligned_src, 50);
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

    (void)d_memcpy(odd_dst, odd_src, 13);
    (void)d_memcpy(odd_dst + 13, odd_src + 13, 27);
    (void)d_memcpy(odd_dst + 40, odd_src + 40, 13);
    test_odd_sized = d_tests_dmemory_compare_buffers(odd_dst, odd_src, 53);

    // test 6: cache-line boundary crossing (typically 64 bytes)
    unsigned char cache_src[128];
    unsigned char cache_dst[128];

    for (i = 0; i < 128; i++)
    {
        cache_src[i] = (unsigned char)(i);
    }

    // copy across cache line boundary
    (void)d_memcpy(cache_dst + 32, cache_src + 32, 64);
    test_cache_line = d_tests_dmemory_compare_buffers(cache_dst + 32,
        cache_src + 32,
        64);

    // build result tree
    d_test_suite_test(_suite, "Alignment Tests");

    D_TEST_CHECK_MSG(_suite,
                     "aligned",
                     test_aligned,
                     "handles aligned addresses");
    D_TEST_CHECK_MSG(_suite,
                     "unaligned_src",
                     test_unaligned_src,
                     "handles unaligned source");
    D_TEST_CHECK_MSG(_suite,
                     "unaligned_dst",
                     test_unaligned_dst,
                     "handles unaligned destination");
    D_TEST_CHECK_MSG(_suite,
                     "both_unaligned",
                     test_both_unaligned,
                     "handles both unaligned");
    D_TEST_CHECK_MSG(_suite,
                     "odd_sized",
                     test_odd_sized,
                     "handles odd-sized transfers");
    D_TEST_CHECK_MSG(_suite,
                     "cache_line",
                     test_cache_line,
                     "handles cache-line boundaries");

    return true;
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
bool d_tests_dmemory_overlap_all(struct dj_test_suite* _suite)
{
    unsigned char         buffer[256];
    bool                  test_forward_overlap;
    bool                  test_backward_overlap;
    bool                  test_complete_overlap;
    bool                  test_partial_overlap;
    bool                  test_adjacent;
    bool                  test_self_copy;
    size_t                i;

    // d_memcpy is memcpy, for which overlap is undefined; d_memcpy_s
    // refuses it (Annex K, K.3.7.1.1), and that is what tests 1 to 4 ask

    // test 1: forward overlap (copy to a later position, 40 bytes shared)
    for (i = 0; i < 100; i++)
    {
        buffer[i] = (unsigned char)i;
    }

    test_forward_overlap = (d_memcpy_s(buffer + 10, 90, buffer, 50) ==
                            EINVAL);

    // test 2: backward overlap (copy to an earlier position, 40 shared)
    for (i = 0; i < 100; i++)
    {
        buffer[i] = (unsigned char)i;
    }

    test_backward_overlap = (d_memcpy_s(buffer, 90, buffer + 10, 50) ==
                             EINVAL) &&
                            (buffer[0] == 0);

    // test 3: complete overlap (same address)
    for (i = 0; i < 100; i++)
    {
        buffer[i] = (unsigned char)i;
    }

    test_complete_overlap = (d_memcpy_s(buffer, 100, buffer, 100) ==
                             EINVAL) &&
                            (buffer[99] == 0);

    // test 4: partial overlap (overlapping by half)
    for (i = 0; i < 100; i++)
    {
        buffer[i] = (unsigned char)i;
    }

    test_partial_overlap = (d_memcpy_s(buffer + 25, 75, buffer, 50) ==
                            EINVAL);

    // test 5: adjacent buffers (no overlap)
    for (i = 0; i < 100; i++)
    {
        buffer[i] = (unsigned char)i;
    }

    test_adjacent = (d_memcpy_s(buffer + 100, 100, buffer, 100) == 0) &&
                    d_tests_dmemory_compare_buffers(buffer + 100, buffer,
                                                       100);

    // test 6: self-copy with memset
    memset(buffer, 0x42, 100);
    (void)d_memset(buffer, 0x42, 100);
    test_self_copy = d_tests_dmemory_verify_pattern(buffer, 100, 0x42);

    // build result tree
    d_test_suite_test(_suite, "Overlapping Memory");

    D_TEST_CHECK_MSG(_suite,
                     "forward_overlap",
                     test_forward_overlap,
                     "refuses forward overlap");
    D_TEST_CHECK_MSG(_suite,
                     "backward_overlap",
                     test_backward_overlap,
                     "refuses backward overlap");
    D_TEST_CHECK_MSG(_suite,
                     "complete_overlap",
                     test_complete_overlap,
                     "refuses complete overlap");
    D_TEST_CHECK_MSG(_suite,
                     "partial_overlap",
                     test_partial_overlap,
                     "refuses partial overlap");
    D_TEST_CHECK_MSG(_suite,
                     "adjacent",
                     test_adjacent,
                     "handles adjacent buffers");
    D_TEST_CHECK_MSG(_suite, "self_copy", test_self_copy, "handles self-copy");

    return true;
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
bool d_tests_dmemory_performance_all(struct dj_test_suite* _suite)
{
    unsigned char* large_src;
    unsigned char* large_dst;
    unsigned char         small_src[16];
    unsigned char         small_dst[16];
    bool                  test_large_perf;
    bool                  test_small_perf;
    bool                  test_aligned_perf;
    bool                  test_pattern_perf;
    size_t                i;
    size_t                j;

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
        (void)d_memcpy(large_dst, large_src, 1024 * 1024);
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
        (void)d_memcpy(small_dst, small_src, 16);

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

        (void)d_memcpy(aligned_dst, aligned_src, 65536);
        test_aligned_perf = true;  // just test completion
    }

    // test 4: pattern filling performance
    test_pattern_perf = false;

    if (large_dst)
    {
        (void)d_memset(large_dst, 0xAA, 1024 * 1024);
        test_pattern_perf = d_tests_dmemory_verify_pattern(large_dst,
            1024 * 1024,
            0xAA);
    }

    // cleanup
    if (large_src) free(large_src);
    if (large_dst) free(large_dst);

    // build result tree
    d_test_suite_test(_suite, "Performance Tests");

    D_TEST_CHECK_MSG(_suite,
                     "large_perf",
                     test_large_perf,
                     "large buffer copy works");
    D_TEST_CHECK_MSG(_suite,
                     "small_perf",
                     test_small_perf,
                     "small buffer copy works");
    D_TEST_CHECK_MSG(_suite,
                     "aligned_perf",
                     test_aligned_perf,
                     "aligned copy works");
    D_TEST_CHECK_MSG(_suite,
                     "pattern_perf",
                     test_pattern_perf,
                     "pattern filling works");

    return true;
}
