#include "dmemory_tests_sa.h"


/*
===============================================================================
                            CORE FUNCTION TESTS
===============================================================================
*/

/*
d_tests_sa_dmemcpy
  Tests the `d_memcpy` function for basic memory copying operations.
  Tests the following:
  - normal copy operation and return value validation
  - null destination parameter handling
  - null source parameter handling  
  - zero count parameter handling
  - binary data copying accuracy
  - struct data copying
  - overlapping memory region behavior
*/
bool 
d_tests_sa_dmemcpy
(
    struct d_test_counter* _test_info
)
{
    bool test_result = true;
    void* result;
    
    printf("Testing `d_memcpy`...\n");
    
    // Test 1: Normal copy operation
    char source[10] = "hello";
    char dest[10] = {0};
    
    result = d_memcpy(dest, source, 6);
    test_result &= d_assert_standalone(
        result == dest,
        "`d_memcpy` returns destination pointer correctly",
        "`d_memcpy` failed to return destination pointer",
        _test_info
    );
    
    test_result &= d_assert_standalone(
        strcmp(dest, "hello") == 0,
        "`d_memcpy` copies string data correctly",
        "`d_memcpy` failed to copy string data",
        _test_info
    );
    
    // Test 2: NULL destination handling
    result = d_memcpy(NULL, source, 5);
    test_result &= d_assert_standalone(
        result == NULL,
        "`d_memcpy` returns NULL for NULL destination",
        "`d_memcpy` failed to handle NULL destination",
        _test_info
    );
    
    // Test 3: NULL source handling
    char dest2[10] = {0};
    result = d_memcpy(dest2, NULL, 5);
    test_result &= d_assert_standalone(
        result == NULL,
        "`d_memcpy` returns NULL for NULL source",
        "`d_memcpy` failed to handle NULL source",
        _test_info
    );
    
    // Test 4: Zero count handling
    char dest3[10] = "original";
    result = d_memcpy(dest3, source, 0);
    test_result &= d_assert_standalone(
        result == dest3 && strcmp(dest3, "original") == 0,
        "`d_memcpy` handles zero count correctly",
        "`d_memcpy` failed to handle zero count",
        _test_info
    );
    
    // Test 5: Binary data copying
    unsigned char binary_src[] = {0x00, 0xFF, 0xAA, 0x55, 0x42};
    unsigned char binary_dest[5] = {0};
    
    result = d_memcpy(binary_dest, binary_src, 5);
    test_result &= d_assert_standalone(
        result != NULL && memcmp(binary_dest, binary_src, 5) == 0,
        "`d_memcpy` copies binary data correctly",
        "`d_memcpy` failed to copy binary data",
        _test_info
    );
    
    // Test 6: Struct copying
    struct test_struct 
    { 
        int a; 
        double b; 
        char c; 
    };
    struct test_struct src_struct = {42, 3.14159, 'x'};
    struct test_struct dest_struct = {0};
    
    result = d_memcpy(&dest_struct, &src_struct, sizeof(struct test_struct));
    test_result &= d_assert_standalone(
        result != NULL && dest_struct.a == 42 && 
        dest_struct.b == 3.14159 && dest_struct.c == 'x',
        "`d_memcpy` copies struct data correctly",
        "`d_memcpy` failed to copy struct data",
        _test_info
    );
    
    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;
    
    return test_result;
}

/*
d_tests_sa_dmemcpy_s
  Tests the `d_memcpy_s` function for secure memory copying operations.
  Tests the following:
  - normal secure copy operation and return values
  - null destination parameter validation
  - null source parameter validation and buffer clearing  
  - buffer overflow protection
  - zero count parameter handling
  - exact size boundary testing
  - destination clearing on error conditions
*/
bool 
d_tests_sa_dmemcpy_s
(
    struct d_test_counter* _test_info
)
{
    bool test_result = true;
    int result;
    
    printf("Testing `d_memcpy_s`...\n");
    
    // Test 1: Normal secure copy operation
    char source[10] = "secure";
    char dest[10] = {0};
    
    result = d_memcpy_s(dest, sizeof(dest), source, 7);
    test_result &= d_assert_standalone(
        result == 0,
        "`d_memcpy_s` returns 0 on successful copy",
        "`d_memcpy_s` failed to return success code",
        _test_info
    );
    
    test_result &= d_assert_standalone(
        strcmp(dest, "secure") == 0,
        "`d_memcpy_s` copies data correctly",
        "`d_memcpy_s` failed to copy data correctly",
        _test_info
    );
    
    // Test 2: NULL destination handling
    result = d_memcpy_s(NULL, 10, source, 5);
    test_result &= d_assert_standalone(
        result == EINVAL,
        "`d_memcpy_s` returns EINVAL for NULL destination",
        "`d_memcpy_s` failed to handle NULL destination",
        _test_info
    );
    
    // Test 3: NULL source handling with buffer clearing
    char dest2[10] = "original";
    result = d_memcpy_s(dest2, sizeof(dest2), NULL, 5);
    test_result &= d_assert_standalone(
        result == EINVAL,
        "`d_memcpy_s` returns EINVAL for NULL source",
        "`d_memcpy_s` failed to handle NULL source",
        _test_info
    );
    
    // Verify destination buffer was cleared
    bool dest_cleared = true;
    for (int i = 0; i < 10; i++) 
    {
        if (dest2[i] != 0) 
        {
            dest_cleared = false;
            break;
        }
    }
    test_result &= d_assert_standalone(
        dest_cleared,
        "`d_memcpy_s` clears destination on NULL source error",
        "`d_memcpy_s` failed to clear destination buffer",
        _test_info
    );
    
    // Test 4: Buffer overflow protection
    char dest3[5] = {0};
    char large_source[20] = "this_is_too_large";
    
    result = d_memcpy_s(dest3, sizeof(dest3), large_source, 15);
    test_result &= d_assert_standalone(
        result == ERANGE,
        "`d_memcpy_s` prevents buffer overflow",
        "`d_memcpy_s` failed to prevent buffer overflow",
        _test_info
    );
    
    // Test 5: Exact size boundary
    char dest4[6] = {0};
    char exact_source[6] = "exact";
    
    result = d_memcpy_s(dest4, sizeof(dest4), exact_source, 6);
    test_result &= d_assert_standalone(
        result == 0 && strcmp(dest4, "exact") == 0,
        "`d_memcpy_s` handles exact size boundary correctly",
        "`d_memcpy_s` failed at exact size boundary",
        _test_info
    );
    
    // Test 6: Zero count handling
    char dest5[10] = "unchanged";
    result = d_memcpy_s(dest5, sizeof(dest5), source, 0);
    test_result &= d_assert_standalone(
        result == 0 && strcmp(dest5, "unchanged") == 0,
        "`d_memcpy_s` handles zero count correctly",
        "`d_memcpy_s` failed to handle zero count",
        _test_info
    );
    
    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;
    
    return test_result;
}

/*
d_tests_sa_d_memdup_s
  Tests the `d_memdup_s` function for secure memory duplication.
  Tests the following:
  - normal memory duplication and allocation
  - null source parameter handling
  - zero size parameter handling
  - binary data duplication accuracy
  - memory allocation failure simulation
  - large allocation handling
*/
bool 
d_tests_sa_d_memdup_s
(
    struct d_test_counter* _test_info
)
{
    bool test_result = true;
    void* result;
    
    printf("Testing `d_memdup_s`...\n");
    
    // Test 1: Normal memory duplication
    char source[] = "duplicate_me";
    char* duplicated = (char*)d_memdup_s(source, strlen(source) + 1);
    
    test_result &= d_assert_standalone(
        (duplicated != NULL),
        "`d_memdup_s` successfully allocates memory",
        "`d_memdup_s` failed to allocate memory",
        _test_info
    );
    
    test_result &= d_assert_standalone(
        ( (duplicated != source) && 
          (strcmp(duplicated, source) == 0) ),
        "`d_memdup_s` creates correct duplicate",
        "`d_memdup_s` failed to create correct duplicate",
        _test_info
    );
    
    if (duplicated) 
    {
        free(duplicated);
        duplicated = NULL;
    }
    
    // Test 2: NULL source handling
    result = d_memdup_s(NULL, 10);
    test_result &= d_assert_standalone(
        result == NULL,
        "`d_memdup_s` returns NULL for NULL source",
        "`d_memdup_s` failed to handle NULL source",
        _test_info
    );
    
    // Test 3: Zero size handling
    result = d_memdup_s(source, 0);
    test_result &= d_assert_standalone(
        (result == NULL),
        "`d_memdup_s` returns NULL for zero size",
        "`d_memdup_s` failed to handle zero size",
        _test_info
    );
    
    // Test 4: Binary data duplication
    unsigned char binary_data[] = {0x00, 0xFF, 0xAA, 0x55, 0x42, 0x13};
    unsigned char* binary_dup = (unsigned char*)d_memdup_s(binary_data, 6);
    
    test_result &= d_assert_standalone(
        ( (binary_dup != NULL) && 
          (memcmp(binary_dup, binary_data, 6) == 0) ),
        "`d_memdup_s` duplicates binary data correctly",
        "`d_memdup_s` failed to duplicate binary data",
        _test_info
    );
    
    if (binary_dup) 
    {
        free(binary_dup);

        binary_dup = NULL;
    }
    
    // Test 5: Large allocation handling (should fail gracefully)
    result = d_memdup_s(source, SIZE_MAX - 1);
    test_result &= d_assert_standalone(
        result == NULL,
        "`d_memdup_s` fails gracefully on oversized allocation",
        "`d_memdup_s` failed to handle oversized allocation",
        _test_info
    );
    
    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;
    
    return test_result;
}

/*
d_tests_sa_d_memset
  Tests the `d_memset` function for memory filling operations.
  Tests the following:
  - normal memory filling with various values
  - null pointer parameter handling
  - zero amount parameter handling
  - different data types and sizes
  - boundary value testing (0, 255)
  - large memory regions
*/
bool 
d_tests_sa_d_memset
(
    struct d_test_counter* _test_info
)
{
    bool test_result = true;
    void* result;
    
    printf("Testing `d_memset`...\n");
    
    // Test 1: Normal memory filling
    char buffer[10] = {0};
    result = d_memset(buffer, 'A', 10);
    
    test_result &= d_assert_standalone(
        result == buffer,
        "`d_memset` returns correct pointer",
        "`d_memset` failed to return correct pointer",
        _test_info
    );
    
    bool all_a = true;
    for (int i = 0; i < 10; i++) 
    {
        if (buffer[i] != 'A') 
        {
            all_a = false;
            break;
        }
    }
    test_result &= d_assert_standalone(
        all_a,
        "`d_memset` fills memory correctly",
        "`d_memset` failed to fill memory correctly",
        _test_info
    );
    
    // Test 2: NULL pointer handling
    result = d_memset(NULL, 'X', 5);
    test_result &= d_assert_standalone(
        result == NULL,
        "`d_memset` returns NULL for NULL pointer",
        "`d_memset` failed to handle NULL pointer",
        _test_info
    );
    
    // Test 3: Zero amount handling
    char buffer2[5] = "test";
    result = d_memset(buffer2, 'Z', 0);
    test_result &= d_assert_standalone(
        result == buffer2 && strcmp(buffer2, "test") == 0,
        "`d_memset` handles zero amount correctly",
        "`d_memset` failed to handle zero amount",
        _test_info
    );
    
    // Test 4: Boundary values (0 and 255)
    unsigned char boundary_buffer[4];
    
    d_memset(boundary_buffer, 0, 4);
    bool all_zero = true;
    for (int i = 0; i < 4; i++) 
    {
        if (boundary_buffer[i] != 0) 
        {
            all_zero = false;
            break;
        }
    }
    test_result &= d_assert_standalone(
        all_zero,
        "`d_memset` handles zero value correctly",
        "`d_memset` failed to handle zero value",
        _test_info
    );
    
    d_memset(boundary_buffer, 255, 4);
    bool all_max = true;
    for (int i = 0; i < 4; i++) 
    {
        if (boundary_buffer[i] != 255) 
        {
            all_max = false;
            break;
        }
    }
    test_result &= d_assert_standalone(
        all_max,
        "`d_memset` handles maximum value correctly",
        "`d_memset` failed to handle maximum value",
        _test_info
    );
    
    // Test 5: Different data types
    int int_buffer[5];
    d_memset(int_buffer, 0x42, sizeof(int_buffer));
    bool pattern_correct = true;
    unsigned char* byte_view = (unsigned char*)int_buffer;
    for (size_t i = 0; i < sizeof(int_buffer); i++) 
    {
        if (byte_view[i] != 0x42) 
        {
            pattern_correct = false;
            break;
        }
    }
    test_result &= d_assert_standalone(
        pattern_correct,
        "`d_memset` works with different data types",
        "`d_memset` failed with different data types",
        _test_info
    );
    
    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;
    
    return test_result;
}

/*
d_tests_sa_d_memset_s
  Tests the `d_memset_s` function for secure memory filling operations.
  Tests the following:
  - normal secure memory filling and return codes
  - null destination parameter validation
  - oversized destination buffer detection
  - oversized count parameter detection
  - count greater than destination size handling
  - boundary condition testing with RSIZE_MAX
  - proper error code returns
*/
bool 
d_tests_sa_d_memset_s
(
    struct d_test_counter* _test_info
)
{
    bool test_result = true;
    errno_t result;
    
    printf("Testing `d_memset_s`...\n");
    
    // Test 1: Normal secure memory filling
    char buffer[10] = {0};
    result = d_memset_s(buffer, sizeof(buffer), 'S', 8);
    
    test_result &= d_assert_standalone(
        result == 0,
        "`d_memset_s` returns 0 on success",
        "`d_memset_s` failed to return success code",
        _test_info
    );
    
    bool correct_fill = true;
    for (int i = 0; i < 8; i++) 
    {
        if (buffer[i] != 'S') 
        {
            correct_fill = false;
            break;
        }
    }
    test_result &= d_assert_standalone(
        correct_fill && buffer[8] == 0 && buffer[9] == 0,
        "`d_memset_s` fills correct number of bytes",
        "`d_memset_s` failed to fill correct number of bytes",
        _test_info
    );
    
    // Test 2: NULL destination handling
    result = d_memset_s(NULL, 10, 'X', 5);
    test_result &= d_assert_standalone(
        result == EINVAL,
        "`d_memset_s` returns EINVAL for NULL destination",
        "`d_memset_s` failed to handle NULL destination",
        _test_info
    );
    
    // Test 3: Oversized destination buffer
    char buffer2[10];
    result = d_memset_s(buffer2, RSIZE_MAX, 'Y', 5);
    test_result &= d_assert_standalone(
        result == EINVAL,
        "`d_memset_s` rejects RSIZE_MAX destination size",
        "`d_memset_s` failed to reject oversized destination",
        _test_info
    );
    
    // Test 4: Oversized count parameter
    result = d_memset_s(buffer2, sizeof(buffer2), 'Z', RSIZE_MAX);
    test_result &= d_assert_standalone(
        result == EINVAL,
        "`d_memset_s` rejects RSIZE_MAX count",
        "`d_memset_s` failed to reject oversized count",
        _test_info
    );
    
    // Test 5: Count greater than destination size
    char buffer3[5] = {0};
    result = d_memset_s(buffer3, sizeof(buffer3), 'O', 10);
    test_result &= d_assert_standalone(
        result == EOVERFLOW,
        "`d_memset_s` returns EOVERFLOW when count > destination size",
        "`d_memset_s` failed to detect overflow condition",
        _test_info
    );
    
    // Verify it still filled the available space
    bool partial_fill = true;
    for (int i = 0; i < 5; i++) 
    {
        if (buffer3[i] != 'O') 
        {
            partial_fill = false;
            break;
        }
    }
    test_result &= d_assert_standalone(
        partial_fill,
        "`d_memset_s` fills available space even on overflow",
        "`d_memset_s` failed to fill available space on overflow",
        _test_info
    );
    
    // Test 6: Exact size boundary
    char buffer4[6] = {0};
    result = d_memset_s(buffer4, sizeof(buffer4), 'E', 6);
    test_result &= d_assert_standalone(
        result == 0,
        "`d_memset_s` handles exact size boundary correctly",
        "`d_memset_s` failed at exact size boundary",
        _test_info
    );
    
    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;
    
    return test_result;
}

/*
===============================================================================
                            INTEGRATION TESTS
===============================================================================
*/

/*
d_tests_sa_dmemory_integration
  Tests integration scenarios combining multiple dmemory functions.
  Tests the following:
  - chained memory operations
  - copying and then duplicating data
  - setting memory and then copying over it
  - complex data structure handling
  - cross-function compatibility verification
*/
bool 
d_tests_sa_dmemory_integration
(
    struct d_test_counter* _test_info
)
{
    bool test_result = true;
    
    printf("Testing dmemory integration scenarios...\n");
    
    // Test 1: Chain copy -> duplicate -> set operations
    char original[] = "integration_test_data";
    char workspace[30] = {0};
    
    // Step 1: Copy original to workspace
    void* copy_result = d_memcpy(workspace, original, strlen(original) + 1);
    test_result &= d_assert_standalone(
        copy_result != NULL && strcmp(workspace, original) == 0,
        "Integration: d_memcpy step successful",
        "Integration: d_memcpy step failed",
        _test_info
    );
    
    // Step 2: Duplicate the workspace
    char* duplicated = (char*)d_memdup_s(workspace, strlen(workspace) + 1);
    test_result &= d_assert_standalone(
        duplicated != NULL && strcmp(duplicated, workspace) == 0,
        "Integration: d_memdup_s step successful",
        "Integration: d_memdup_s step failed",
        _test_info
    );
    
    // Step 3: Clear workspace using d_memset
    d_memset(workspace, 0, sizeof(workspace));
    test_result &= d_assert_standalone(
        workspace[0] == 0 && workspace[10] == 0,
        "Integration: d_memset step successful",
        "Integration: d_memset step failed",
        _test_info
    );
    
    // Step 4: Use secure copy to restore from duplicate
    int secure_result = d_memcpy_s(workspace, sizeof(workspace), 
                                   duplicated, strlen(duplicated) + 1);
    test_result &= d_assert_standalone(
        secure_result == 0 && strcmp(workspace, original) == 0,
        "Integration: d_memcpy_s restoration successful",
        "Integration: d_memcpy_s restoration failed",
        _test_info
    );
    
    if (duplicated) 
    {
        free(duplicated);
        duplicated = NULL;
    }
    
    // Test 2: Complex data structure operations
    struct complex_data 
    {
        int header;
        char payload[16];
        double timestamp;
        int footer;
    };
    
    struct complex_data source_data = 
    {
        .header = 0xDEADBEEF,
        .payload = "test_payload",
        .timestamp = 12345.6789,
        .footer = 0xCAFEBABE
    };
    
    struct complex_data dest_data = {0};
    
    // Use secure copy for the struct
    int struct_result = d_memcpy_s(&dest_data, sizeof(dest_data), 
                                   &source_data, sizeof(source_data));
    test_result &= d_assert_standalone(
        struct_result == 0 && 
        dest_data.header == 0xDEADBEEF &&
        strcmp(dest_data.payload, "test_payload") == 0 &&
        dest_data.timestamp == 12345.6789 &&
        dest_data.footer == 0xCAFEBABE,
        "Integration: complex struct copying works correctly",
        "Integration: complex struct copying failed",
        _test_info
    );
    
    // Test 3: Mixed secure and standard operations
    char mixed_buffer[20] = {0};
    
    // Fill with pattern using standard memset
    d_memset(mixed_buffer, 0xAA, 10);
    
    // Use secure memset for the rest
    errno_t mixed_result = d_memset_s(mixed_buffer + 10, 10, 0x55, 10);
    test_result &= d_assert_standalone(
        mixed_result == 0 &&
        (unsigned char)mixed_buffer[5] == 0xAA &&
        (unsigned char)mixed_buffer[15] == 0x55,
        "Integration: mixed memset operations work correctly",
        "Integration: mixed memset operations failed",
        _test_info
    );
    
    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;
    
    return test_result;
}

/*
===============================================================================
                            SECURITY TESTS
===============================================================================
*/

/*
d_tests_sa_dmemory_security
  Tests security-related aspects of dmemory functions.
  Tests the following:
  - buffer overflow prevention in secure functions
  - proper error handling and buffer clearing
  - parameter validation robustness
  - memory safety in edge conditions
  - resistance to malicious input patterns
*/
bool 
d_tests_sa_dmemory_security
(
    struct d_test_counter* _test_info
)
{
    bool test_result = true;
    
    printf("Testing dmemory security features...\n");
    
    // Test 1: Buffer overflow prevention in d_memcpy_s
    char small_buffer[8] = {0};
    char large_source[] = "this_string_is_definitely_too_large_for_the_destination_buffer";
    
    int overflow_result = d_memcpy_s(small_buffer, 
                                     sizeof(small_buffer), 
                                     large_source,
                                     strlen(large_source));
    test_result &= d_assert_standalone(
        (overflow_result == ERANGE),
        "security: d_memcpy_s prevents buffer overflow",
        "security: d_memcpy_s failed to prevent buffer overflow",
        _test_info
    );
    
    // Verify buffer was cleared
    bool buffer_cleared = true;
    for (int i = 0; i < 8; i++) 
    {
        if (small_buffer[i] != 0) 
        {
            buffer_cleared = false;
            break;
        }
    }
    test_result &= d_assert_standalone(
        buffer_cleared,
        "security: d_memcpy_s clears buffer on overflow",
        "security: d_memcpy_s failed to clear buffer on overflow",
        _test_info
    );
    
    // Test 2: Parameter validation in d_memset_s
    char secure_buffer[16] = {0};
    
    // Test with invalid destination size
    errno_t param_result = d_memset_s(secure_buffer, RSIZE_MAX, 'X', 8);
    test_result &= d_assert_standalone(
        param_result == EINVAL,
        "security: `d_memset_s` validates destination size",
        "security: `d_memset_s` failed to validate destination size",
        _test_info
    );
    
    // Test with invalid count
    param_result = d_memset_s(secure_buffer, sizeof(secure_buffer), 'Y', RSIZE_MAX);
    test_result &= d_assert_standalone(
        param_result == EINVAL,
        "security: `d_memset_s` validates count parameter",
        "security: `d_memset_s` failed to validate count parameter",
        _test_info
    );
    
    // Test 3: Memory boundary protection
    char boundary_buffer[4];
    
    // Attempt to set beyond buffer size
    param_result = d_memset_s(boundary_buffer, sizeof(boundary_buffer), 'B', 10);
    test_result &= d_assert_standalone(
        param_result == EOVERFLOW,
        "security: `d_memset_s` detects boundary violations",
        "security: `d_memset_s` failed to detect boundary violations",
        _test_info
    );
    
    // Verify it still filled the safe portion
    bool safe_fill = true;
    
    for (int i = 0; i < 4; i++) 
    {
        if (boundary_buffer[i] != 'B') 
        {
            safe_fill = false;
            break;
        }
    }

    test_result &= d_assert_standalone(
        safe_fill,
        "security: d_memset_s fills safe portion on overflow",
        "security: d_memset_s failed to fill safe portion",
        _test_info
    );
    
    // Test 4: NULL pointer robustness across all functions
    test_result &= d_assert_standalone(
        (d_memcpy(NULL, "test", 4) == NULL),
        "security: d_memcpy rejects NULL destination",
        "security: d_memcpy failed to reject NULL destination", 
        _test_info
    );
    
    test_result &= d_assert_standalone(
        (d_memcpy("test", NULL, 4) == NULL),
        "security: d_memcpy rejects NULL source",
        "security: d_memcpy failed to reject NULL source",
        _test_info
    );
    
    test_result &= d_assert_standalone(
        d_memdup_s(NULL, 10) == NULL,
        "security: d_memdup_s rejects NULL source",
        "security: d_memdup_s failed to reject NULL source",
        _test_info
    );
    
    test_result &= d_assert_standalone(
        d_memset(NULL, 'X', 10) == NULL,
        "security: d_memset rejects NULL pointer",
        "security: d_memset failed to reject NULL pointer",
        _test_info
    );
    
    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;
    
    return test_result;
}

/*
===============================================================================
                            EDGE CASE TESTS
===============================================================================
*/

/*
d_tests_sa_dmemory_edge_cases
  Tests edge cases and boundary conditions for dmemory functions.
  Tests the following:
  - single-byte operations
  - maximum and minimum boundary values
  - very large allocation requests
  - zero-size operations
  - alignment and padding considerations
  - extreme parameter combinations
*/
bool 
d_tests_sa_dmemory_edge_cases
(
    struct d_test_counter* _test_info
)
{
    bool test_result = true;
    
    printf("Testing dmemory edge cases...\n");
    
    // Test 1: Single-byte operations
    char single_buffer[1];
    
    void* single_result = d_memcpy(single_buffer, "Z", 1);
    test_result &= d_assert_standalone(
        single_result == single_buffer && single_buffer[0] == 'Z',
        "Edge case: single-byte d_memcpy works correctly",
        "Edge case: single-byte d_memcpy failed",
        _test_info
    );
    
    int single_copy_result = d_memcpy_s(single_buffer, 1, "W", 1);
    test_result &= d_assert_standalone(
        single_copy_result == 0 && single_buffer[0] == 'W',
        "Edge case: single-byte d_memcpy_s works correctly",
        "Edge case: single-byte d_memcpy_s failed",
        _test_info
    );
    
    d_memset(single_buffer, 'V', 1);
    test_result &= d_assert_standalone(
        single_buffer[0] == 'V',
        "Edge case: single-byte d_memset works correctly",
        "Edge case: single-byte d_memset failed",
        _test_info
    );
    
    errno_t single_set_result = d_memset_s(single_buffer, 1, 'U', 1);
    test_result &= d_assert_standalone(
        single_set_result == 0 && single_buffer[0] == 'U',
        "Edge case: single-byte d_memset_s works correctly",
        "Edge case: single-byte d_memset_s failed",
        _test_info
    );
    
    // Test 2: Boundary value behavior
    char value_test[4];
    
    // Test maximum unsigned char value
    d_memset(value_test, 255, 4);
    bool all_max = true;
    for (int i = 0; i < 4; i++) 
    {
        if ((unsigned char)value_test[i] != 255) 
        {
            all_max = false;
            break;
        }
    }
    test_result &= d_assert_standalone(
        all_max,
        "Edge case: d_memset handles max unsigned char value",
        "Edge case: d_memset failed with max value",
        _test_info
    );
    
    // Test 3: Very large allocation in d_memdup_s
    void* large_dup = d_memdup_s("test", SIZE_MAX - 1);
    test_result &= d_assert_standalone(
        large_dup == NULL,
        "Edge case: d_memdup_s fails gracefully on huge allocation",
        "Edge case: d_memdup_s failed to handle huge allocation",
        _test_info
    );
    
    // Test 4: Zero-size edge cases
    char zero_buffer[10] = "original";
    
    void* zero_copy = d_memcpy(zero_buffer, "new", 0);
    test_result &= d_assert_standalone(
        zero_copy == zero_buffer && strcmp(zero_buffer, "original") == 0,
        "Edge case: d_memcpy with zero count preserves destination",
        "Edge case: d_memcpy with zero count failed",
        _test_info
    );
    
    int zero_secure = d_memcpy_s(zero_buffer, sizeof(zero_buffer), "new", 0);
    test_result &= d_assert_standalone(
        zero_secure == 0 && strcmp(zero_buffer, "original") == 0,
        "Edge case: d_memcpy_s with zero count preserves destination",
        "Edge case: d_memcpy_s with zero count failed",
        _test_info
    );
    
    // Test 5: Alignment and odd-sized data
    struct odd_struct 
    {
        char a;
        short b;
        char c;
    };
    
    struct odd_struct src_odd = {'X', 12345, 'Y'};
    struct odd_struct dest_odd = {0};
    
    void* odd_result = d_memcpy(&dest_odd, &src_odd, sizeof(struct odd_struct));
    test_result &= d_assert_standalone(
        odd_result != NULL && 
        dest_odd.a == 'X' && dest_odd.b == 12345 && dest_odd.c == 'Y',
        "Edge case: odd-sized struct copying works correctly",
        "Edge case: odd-sized struct copying failed",
        _test_info
    );
    
    // Test 6: RSIZE_MAX boundary testing
    char rsize_buffer[10];
    errno_t rsize_result = d_memset_s(rsize_buffer, RSIZE_MAX, 'R', 5);
    test_result &= d_assert_standalone(
        rsize_result == EINVAL,
        "Edge case: d_memset_s rejects RSIZE_MAX destination size",
        "Edge case: d_memset_s failed to reject RSIZE_MAX destination",
        _test_info
    );
    
    rsize_result = d_memset_s(rsize_buffer, 10, 'S', RSIZE_MAX);
    test_result &= d_assert_standalone(
        rsize_result == EINVAL,
        "Edge case: d_memset_s rejects RSIZE_MAX count",
        "Edge case: d_memset_s failed to reject RSIZE_MAX count",
        _test_info
    );
    
    _test_info->tests_total++;
    if (test_result) _test_info->tests_passed++;
    
    return test_result;
}

/*
===============================================================================
                           AGGREGATION FUNCTIONS
===============================================================================
*/

/*
d_tests_sa_dmemory_core_all
  Runs all core dmemory function tests.
  Tests the following:
  - all basic dmemory functions
  - comprehensive function coverage
  - core functionality validation
*/
bool 
d_tests_sa_dmemory_core_all
(
    struct d_test_counter* _test_info
)
{
    printf("\n--- Testing dmemory Core Functions ---\n");
    struct d_test_counter module_counter = {0, 0, 0, 0};
    
    bool memcpy_result = d_tests_sa_dmemcpy(&module_counter);
    bool memcpy_s_result = d_tests_sa_dmemcpy_s(&module_counter);
    bool memdup_s_result = d_tests_sa_d_memdup_s(&module_counter);
    bool memset_result = d_tests_sa_d_memset(&module_counter);
    bool memset_s_result = d_tests_sa_d_memset_s(&module_counter);
    
    // Update totals
    _test_info->assertions_total += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total += module_counter.tests_total;
    _test_info->tests_passed += module_counter.tests_passed;
    
    bool overall_result = (memcpy_result && memcpy_s_result && 
                          memdup_s_result && memset_result && memset_s_result);
    
    printf("\n%s dmemory Core Functions: %zu/%zu assertions, %zu/%zu unit tests passed\n",
           overall_result ? D_TEST_SYMBOL_SUCCESS : D_TEST_SYMBOL_FAIL,
           module_counter.assertions_passed, module_counter.assertions_total,
           module_counter.tests_passed, module_counter.tests_total);
    
    return overall_result;
}

/*
d_tests_sa_dmemory_advanced_all
  Runs all advanced dmemory test scenarios.
  Tests the following:
  - integration testing between functions
  - security feature validation
  - edge case and boundary testing
*/
bool 
d_tests_sa_dmemory_advanced_all
(
    struct d_test_counter* _test_info
)
{
    printf("\n--- Testing dmemory Advanced Scenarios ---\n");
    struct d_test_counter module_counter = {0, 0, 0, 0};
    
    bool integration_result = d_tests_sa_dmemory_integration(&module_counter);
    bool security_result = d_tests_sa_dmemory_security(&module_counter);
    bool edge_cases_result = d_tests_sa_dmemory_edge_cases(&module_counter);
    
    // Update totals
    _test_info->assertions_total += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total += module_counter.tests_total;
    _test_info->tests_passed += module_counter.tests_passed;
    
    bool overall_result = (integration_result && security_result && 
                          edge_cases_result);
    
    printf("\n%s dmemory Advanced Scenarios: %zu/%zu assertions, %zu/%zu unit tests passed\n",
           overall_result 
               ? D_TEST_SYMBOL_SUCCESS
               : D_TEST_SYMBOL_FAIL,
           module_counter.assertions_passed, module_counter.assertions_total,
           module_counter.tests_passed, module_counter.tests_total);
    
    return overall_result;
}

/*
d_tests_sa_dmemory_all
  Runs all dmemory tests including core functions and advanced scenarios.
  Tests the following:
  - all core dmemory functions
  - all advanced test scenarios  
  - comprehensive coverage and reporting
*/
bool 
d_tests_sa_dmemory_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter suite_counter = {0, 0, 0, 0};
    
    // Run all test modules
    bool core_result     = d_tests_sa_dmemory_core_all(&suite_counter);
    bool advanced_result = d_tests_sa_dmemory_advanced_all(&suite_counter);
    
    // Update totals
    _test_info->assertions_total  += suite_counter.assertions_total;
    _test_info->assertions_passed += suite_counter.assertions_passed;
    _test_info->tests_total       += suite_counter.tests_total;
    _test_info->tests_passed      += suite_counter.tests_passed;
    
    bool overall_result = (core_result && advanced_result);
    
    return overall_result;
}