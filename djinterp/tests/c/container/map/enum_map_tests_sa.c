#include ".\enum_map_tests_standalone.h"


// compile-time constant enum map for testing (manual definition to avoid static issues)
const struct d_enum_map_entry test_const_map_entries[] = 
{
    {TEST_ENUM_FIRST, "first"},
    {TEST_ENUM_SECOND, "second"},
    {TEST_ENUM_THIRD, "third"}
};

const struct d_enum_map test_const_map = 
{
    .entries = (struct d_enum_map_entry*)test_const_map_entries,
    .size = sizeof(test_const_map_entries) / sizeof(test_const_map_entries[0])
};

// ============================================================================
// Helper functions for testing
// ============================================================================

char* 
d_test_create_enum_string
(
    const char* _prefix, 
    int         _value
)
{
    if (!_prefix) return NULL;
    
    char* result = malloc(64);
    if (result) 
    {
        // Use sprintf_s with buffer size parameter
        if (sprintf_s(result, 64, "%s_%d", _prefix, _value) < 0)
        {
            free(result);
            return NULL;
        }
    }
    return result;
}

int 
d_test_enum_comparator
(
    const void* _a, 
    const void* _b
)
{
    if (!_a || !_b) return -1;
    
    int val_a = *(const int*)_a;
    int val_b = *(const int*)_b;
    
    if (val_a < val_b) return -1;
    if (val_a > val_b) return 1;
    return 0;
}

// ============================================================================
// Core enum_map function tests
// ============================================================================

bool 
d_tests_sa_enum_map_new
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_new ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    // Test default size creation
    struct d_enum_map* map = d_enum_map_new();
    
    if (!d_assert_standalone(map != NULL, 
                         "Default enum map creation succeeded", 
                         "Default enum map creation failed", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    if (map) 
    {
        if (!d_assert_standalone(map->size == D_ENUM_DEFAULT_SIZE, 
                             "Default map has correct size", 
                             "Default map should have D_ENUM_DEFAULT_SIZE", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        if (!d_assert_standalone(map->entries != NULL, 
                             "Default map entries allocated", 
                             "Default map entries should be allocated", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        // Check that entries are initialized
        if (map->entries) 
        {
            bool all_invalid = true;
            for (size_t i = 0; i < map->size; i++) 
            {
                if (map->entries[i].key != -1) 
                {
                    all_invalid = false;
                    break;
                }
            }
            
            if (!d_assert_standalone(all_invalid, 
                                 "All entries initialized to invalid", 
                                 "New map entries should be initialized", _test_info)) 
            {
                all_assertions_passed = false;
            }
        }
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_new unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_new unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(map);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_new_sized
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_new_sized ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    // Test specific size creation
    const size_t test_size = 8;
    struct d_enum_map* map = d_enum_map_new_sized(test_size);
    
    if (!d_assert_standalone(map != NULL, 
                         "Sized enum map creation succeeded", 
                         "Sized enum map creation failed", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    if (map) 
    {
        if (!d_assert_standalone(map->size == test_size, 
                             "Sized map has correct size", 
                             "Sized map should have requested size", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        if (!d_assert_standalone(map->entries != NULL, 
                             "Sized map entries allocated", 
                             "Sized map entries should be allocated", _test_info)) 
        {
            all_assertions_passed = false;
        }
    }
    
    // Test zero size (should fail)
    struct d_enum_map* zero_map = d_enum_map_new_sized(0);
    
    if (!d_assert_standalone(zero_map == NULL, 
                         "Zero-sized map creation returns NULL", 
                         "Zero-sized map creation should fail", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_new_sized unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_new_sized unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(map);
    d_enum_map_free(zero_map);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_new_copy
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_new_copy ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    // Create source map with data
    struct d_enum_map* source = d_enum_map_new_sized(4);
    char* test_val1 = d_test_create_enum_string("test", 1);
    char* test_val2 = d_test_create_enum_string("test", 2);
    
    if (source && test_val1 && test_val2) 
    {
        d_enum_map_set(source, TEST_ENUM_FIRST, test_val1);
        d_enum_map_set(source, TEST_ENUM_SECOND, test_val2);
        
        // Test copying
        struct d_enum_map* copy = d_enum_map_new_copy(source);
        
        if (!d_assert_standalone(copy != NULL, 
                             "Copy creation succeeded", 
                             "Copy creation should succeed", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        if (copy) 
        {
            if (!d_assert_standalone(copy->size == source->size, 
                                 "Copy has same size as source", 
                                 "Copy should have same size", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            // Test that data was copied
            void* copied_val = d_enum_map_get(copy, TEST_ENUM_FIRST);
            if (!d_assert_standalone(copied_val == test_val1, 
                                 "Copy contains correct data", 
                                 "Copy should contain source data", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            void* copied_val2 = d_enum_map_get(copy, TEST_ENUM_SECOND);
            if (!d_assert_standalone(copied_val2 == test_val2, 
                                 "Copy contains all source data", 
                                 "Copy should contain all source entries", _test_info)) 
            {
                all_assertions_passed = false;
            }
        }
        
        d_enum_map_free(copy);
    }
    
    // Test copying NULL (should fail)
    struct d_enum_map* null_copy = d_enum_map_new_copy(NULL);
    
    if (!d_assert_standalone(null_copy == NULL, 
                         "Copying NULL returns NULL", 
                         "Copying NULL should return NULL", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_new_copy unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_new_copy unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(source);
    free(test_val1);
    free(test_val2);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_new_args
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_new_args ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    char* val1 = d_test_create_enum_string("arg", 1);
    char* val2 = d_test_create_enum_string("arg", 2);
    
    // Test creating map with variable arguments
    struct d_enum_map* map = d_enum_map_new_args(4, 
                                                 TEST_ENUM_FIRST, val1,
                                                 TEST_ENUM_SECOND, val2,
                                                 -1);
    
    if (!d_assert_standalone(map != NULL, 
                         "Variable args map creation succeeded", 
                         "Variable args map creation should succeed", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    if (map) 
    {
        // Test that values were set correctly
        void* retrieved1 = d_enum_map_get(map, TEST_ENUM_FIRST);
        if (!d_assert_standalone(retrieved1 == val1, 
                             "First argument value stored correctly", 
                             "First value should be stored", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        void* retrieved2 = d_enum_map_get(map, TEST_ENUM_SECOND);
        if (!d_assert_standalone(retrieved2 == val2, 
                             "Second argument value stored correctly", 
                             "Second value should be stored", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        // Test that map is sorted
        if (!d_assert_standalone(d_enum_map_is_sorted(map), 
                             "Args map is properly sorted", 
                             "Args map should be sorted", _test_info)) 
        {
            all_assertions_passed = false;
        }
    }
    
    // Test with unordered keys (should fail)
    struct d_enum_map* bad_map = d_enum_map_new_args(4,
                                                     TEST_ENUM_SECOND, val2,
                                                     TEST_ENUM_FIRST, val1,  // Out of order
                                                     -1);
    
    if (!d_assert_standalone(bad_map == NULL, 
                         "Unordered args map creation fails", 
                         "Unordered args should fail", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_new_args unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_new_args unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(map);
    d_enum_map_free(bad_map);
    free(val1);
    free(val2);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_free
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_free ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    // Test free on NULL (should not crash)
    d_enum_map_free(NULL);
    if (!d_assert_standalone(true, 
                         "Free on NULL completed without crash", 
                         "Free on NULL should not crash", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Test free on valid map
    struct d_enum_map* map = d_enum_map_new_sized(4);
    if (map) 
    {
        d_enum_map_free(map);
        if (!d_assert_standalone(true, 
                             "Free on valid map completed without crash", 
                             "Free on valid map should not crash", _test_info)) 
        {
            all_assertions_passed = false;
        }
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_free unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_free unit test failed\n");
    }
    _test_info->tests_run++;
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_core_all
(
    struct d_test_counter* _test_info
) 
{
    printf("\n--- Testing d_enum_map Core Functions ---\n");
    struct d_test_counter module_counter = {0, 0, 0, 0};
    
    bool new_result = d_tests_sa_enum_map_new(&module_counter);
    bool sized_result = d_tests_sa_enum_map_new_sized(&module_counter);
    bool copy_result = d_tests_sa_enum_map_new_copy(&module_counter);
    bool args_result = d_tests_sa_enum_map_new_args(&module_counter);
    bool free_result = d_tests_sa_enum_map_free(&module_counter);
    
    // Update totals
    _test_info->assertions_total += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_run += module_counter.tests_run;
    _test_info->tests_passed += module_counter.tests_passed;
    
    bool overall_result = (new_result && sized_result && copy_result && args_result && free_result);
    
    if (overall_result) 
    {
        printf("[PASS] Module d_enum_map Core: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    } 
    else 
    {
        printf("[FAIL] Module d_enum_map Core: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    }
    
    return overall_result;
}

// ============================================================================
// Access function tests
// ============================================================================

bool 
d_tests_sa_enum_map_find
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_find ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    struct d_enum_map* map = d_enum_map_new_sized(4);
    char* test_val = d_test_create_enum_string("find", 1);
    
    if (map && test_val) 
    {
        // Set a value
        d_enum_map_set(map, TEST_ENUM_FIRST, test_val);
        
        // Test finding existing key
        struct d_enum_map_entry* entry = d_enum_map_find(map, TEST_ENUM_FIRST);
        if (!d_assert_standalone(entry != NULL, 
                             "Find existing key succeeds", 
                             "Find should locate existing key", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        if (entry) 
        {
            if (!d_assert_standalone(entry->key == TEST_ENUM_FIRST, 
                                 "Found entry has correct key", 
                                 "Found entry should have correct key", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            if (!d_assert_standalone(entry->value == test_val, 
                                 "Found entry has correct value", 
                                 "Found entry should have correct value", _test_info)) 
            {
                all_assertions_passed = false;
            }
        }
        
        // Test finding non-existing key
        struct d_enum_map_entry* missing = d_enum_map_find(map, TEST_ENUM_THIRD);
        if (!d_assert_standalone(missing == NULL, 
                             "Find non-existing key returns NULL", 
                             "Find should return NULL for missing key", _test_info)) 
        {
            all_assertions_passed = false;
        }
    }
    
    // Test find on NULL map
    struct d_enum_map_entry* null_result = d_enum_map_find(NULL, TEST_ENUM_FIRST);
    if (!d_assert_standalone(null_result == NULL, 
                         "Find on NULL map returns NULL", 
                         "Find on NULL should return NULL", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_find unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_find unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(map);
    free(test_val);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_get
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_get ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    struct d_enum_map* map = d_enum_map_new_sized(4);
    char* test_val1 = d_test_create_enum_string("get", 1);
    char* test_val2 = d_test_create_enum_string("get", 2);
    
    if (map && test_val1 && test_val2) 
    {
        // Set multiple values
        d_enum_map_set(map, TEST_ENUM_FIRST, test_val1);
        d_enum_map_set(map, TEST_ENUM_THIRD, test_val2);
        
        // Test getting existing values
        void* retrieved1 = d_enum_map_get(map, TEST_ENUM_FIRST);
        if (!d_assert_standalone(retrieved1 == test_val1, 
                             "Get returns correct value for first key", 
                             "Get should return correct value", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        void* retrieved2 = d_enum_map_get(map, TEST_ENUM_THIRD);
        if (!d_assert_standalone(retrieved2 == test_val2, 
                             "Get returns correct value for third key", 
                             "Get should return correct value", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        // Test getting non-existing value
        void* missing = d_enum_map_get(map, TEST_ENUM_SECOND);
        if (!d_assert_standalone(missing == NULL, 
                             "Get returns NULL for missing key", 
                             "Get should return NULL for missing key", _test_info)) 
        {
            all_assertions_passed = false;
        }
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_get unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_get unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(map);
    free(test_val1);
    free(test_val2);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_set
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_set ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    struct d_enum_map* map = d_enum_map_new_sized(4);
    char* test_val = d_test_create_enum_string("set", 1);
    
    if (map && test_val) 
    {
        // Test setting new value
        bool set_result = d_enum_map_set(map, TEST_ENUM_FIRST, test_val);
        if (!d_assert_standalone(set_result == true, 
                             "Set new value succeeds", 
                             "Set should succeed for new value", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        // Verify value was set
        void* retrieved = d_enum_map_get(map, TEST_ENUM_FIRST);
        if (!d_assert_standalone(retrieved == test_val, 
                             "Set value can be retrieved", 
                             "Set value should be retrievable", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        // Test updating existing value
        char* new_val = d_test_create_enum_string("updated", 1);
        if (new_val) 
        {
            bool update_result = d_enum_map_set(map, TEST_ENUM_FIRST, new_val);
            if (!d_assert_standalone(update_result == true, 
                                 "Update existing value succeeds", 
                                 "Update should succeed", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            // Verify value was updated
            void* updated = d_enum_map_get(map, TEST_ENUM_FIRST);
            if (!d_assert_standalone(updated == new_val, 
                                 "Updated value can be retrieved", 
                                 "Updated value should be retrievable", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            free(new_val);
        }
    }
    
    // Test set on NULL map
    bool null_result = d_enum_map_set(NULL, TEST_ENUM_FIRST, test_val);
    if (!d_assert_standalone(null_result == false, 
                         "Set on NULL map returns false", 
                         "Set on NULL should return false", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_set unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_set unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(map);
    free(test_val);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_access_all
(
    struct d_test_counter* _test_info
) 
{
    printf("\n--- Testing d_enum_map Access Functions ---\n");
    struct d_test_counter module_counter = {0, 0, 0, 0};
    
    bool find_result = d_tests_sa_enum_map_find(&module_counter);
    bool get_result = d_tests_sa_enum_map_get(&module_counter);
    bool set_result = d_tests_sa_enum_map_set(&module_counter);
    
    // Update totals
    _test_info->assertions_total += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_run += module_counter.tests_run;
    _test_info->tests_passed += module_counter.tests_passed;
    
    bool overall_result = (find_result && get_result && set_result);
    
    if (overall_result) 
    {
        printf("[PASS] Module d_enum_map Access: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    } 
    else 
    {
        printf("[FAIL] Module d_enum_map Access: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    }
    
    return overall_result;
}

// ============================================================================
// Static assertion and compile-time tests
// ============================================================================

bool 
d_tests_sa_enum_map_static_assertions
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing static assertions ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    // The static assertions are checked at compile time
    // If we got here, they passed
    if (!d_assert_standalone(true, 
                         "Static assertion macros compiled successfully", 
                         "Static assertions should compile", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Test that enum values are actually ordered as expected
    if (!d_assert_standalone(TEST_ENUM_FIRST < TEST_ENUM_SECOND, 
                         "Test enum values are properly ordered (1)", 
                         "Enum values should be ordered", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    if (!d_assert_standalone(TEST_ENUM_SECOND < TEST_ENUM_THIRD, 
                         "Test enum values are properly ordered (2)", 
                         "Enum values should be ordered", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    if (!d_assert_standalone(TEST_ENUM_THIRD < TEST_ENUM_FOURTH, 
                         "Test enum values are properly ordered (3)", 
                         "Enum values should be ordered", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] static assertions unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] static assertions unit test failed\n");
    }
    _test_info->tests_run++;
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_compile_time_init
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing compile-time initialization ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    // Test the compile-time constant map
    if (!d_assert_standalone(test_const_map.entries != NULL, 
                         "Compile-time map has entries", 
                         "Compile-time map should have entries", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    if (!d_assert_standalone(test_const_map.size == 3, 
                         "Compile-time map has correct size", 
                         "Compile-time map should have correct size", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Test accessing values from compile-time map
    void* val = d_enum_map_get(&test_const_map, TEST_ENUM_FIRST);
    if (!d_assert_standalone(val != NULL, 
                         "Can retrieve from compile-time map", 
                         "Should be able to get from compile-time map", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    if (val) 
    {
        if (!d_assert_standalone(strcmp((char*)val, "first") == 0, 
                             "Compile-time map has correct values", 
                             "Compile-time values should be correct", _test_info)) 
        {
            all_assertions_passed = false;
        }
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] compile-time initialization unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] compile-time initialization unit test failed\n");
    }
    _test_info->tests_run++;
    
    return (_test_info->tests_passed > initial_tests_passed);
}

// ============================================================================
// Query function tests
// ============================================================================

bool 
d_tests_sa_enum_map_contains
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_contains ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    struct d_enum_map* map = d_enum_map_new_sized(6);
    char* val1 = d_test_create_enum_string("contains", 1);
    char* val2 = d_test_create_enum_string("contains", 2);
    
    if (map && val1 && val2) 
    {
        // Set some values
        d_enum_map_set(map, TEST_ENUM_FIRST, val1);
        d_enum_map_set(map, TEST_ENUM_THIRD, val2);
        
        // Test contains with array of existing keys
        int key1 = TEST_ENUM_FIRST;
        int key2 = TEST_ENUM_THIRD;
        const int* keys_existing[] = {&key1, &key2};
        
        bool contains_result = d_enum_map_contains(map, keys_existing, 2);
        if (!d_assert_standalone(contains_result == true, 
                             "Contains finds all existing keys", 
                             "Contains should find all existing keys", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        // Test contains with mix of existing and non-existing keys
        int key3 = TEST_ENUM_SECOND;  // Not in map
        const int* keys_mixed[] = {&key1, &key3};
        
        bool mixed_result = d_enum_map_contains(map, keys_mixed, 2);
        if (!d_assert_standalone(mixed_result == false, 
                             "Contains returns false for mixed key array", 
                             "Contains should return false if any key missing", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        // Test contains with empty array
        bool empty_result = d_enum_map_contains(map, NULL, 0);
        if (!d_assert_standalone(empty_result == false, 
                             "Contains handles NULL key array", 
                             "Contains should handle NULL gracefully", _test_info)) 
        {
            all_assertions_passed = false;
        }
    }
    
    // Test contains on NULL map
    int dummy_key = TEST_ENUM_FIRST;
    const int* dummy_keys[] = {&dummy_key};
    bool null_result = d_enum_map_contains(NULL, dummy_keys, 1);
    if (!d_assert_standalone(null_result == false, 
                         "Contains on NULL map returns false", 
                         "Contains on NULL should return false", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_contains unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_contains unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(map);
    free(val1);
    free(val2);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_contains_all
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_contains_all ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    struct d_enum_map* map = d_enum_map_new_sized(4);
    char* val = d_test_create_enum_string("contains_all", 1);
    
    if (map && val) 
    {
        // Set a value
        d_enum_map_set(map, TEST_ENUM_FIRST, val);
        
        // Test contains_all for existing key
        bool exists = d_enum_map_contains_all(map, TEST_ENUM_FIRST);
        if (!d_assert_standalone(exists == true, 
                             "contains_all finds existing key", 
                             "contains_all should find existing key", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        // Test contains_all for non-existing key
        bool missing = d_enum_map_contains_all(map, TEST_ENUM_SECOND);
        if (!d_assert_standalone(missing == false, 
                             "contains_all returns false for missing key", 
                             "contains_all should return false for missing key", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        // Add more values and test multiple keys
        char* val2 = d_test_create_enum_string("contains_all", 2);
        if (val2) 
        {
            d_enum_map_set(map, TEST_ENUM_THIRD, val2);
            
            bool exists2 = d_enum_map_contains_all(map, TEST_ENUM_THIRD);
            if (!d_assert_standalone(exists2 == true, 
                                 "contains_all finds second existing key", 
                                 "contains_all should find all existing keys", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            free(val2);
        }
    }
    
    // Test contains_all on NULL map
    bool null_result = d_enum_map_contains_all(NULL, TEST_ENUM_FIRST);
    if (!d_assert_standalone(null_result == false, 
                         "contains_all on NULL map returns false", 
                         "contains_all on NULL should return false", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_contains_all unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_contains_all unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(map);
    free(val);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_is_sorted
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_is_sorted ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    // Test empty map (should be sorted)
    struct d_enum_map* empty_map = d_enum_map_new_sized(4);
    if (empty_map) 
    {
        bool empty_sorted = d_enum_map_is_sorted(empty_map);
        if (!d_assert_standalone(empty_sorted == true, 
                             "Empty map is considered sorted", 
                             "Empty map should be sorted", _test_info)) 
        {
            all_assertions_passed = false;
        }
    }
    
    // Test map with properly sorted entries
    struct d_enum_map* sorted_map = d_enum_map_new_sized(6);
    if (sorted_map) 
    {
        char* val1 = d_test_create_enum_string("sorted", 1);
        char* val2 = d_test_create_enum_string("sorted", 2);
        char* val3 = d_test_create_enum_string("sorted", 3);
        
        if (val1 && val2 && val3) 
        {
            // Add in sorted order
            d_enum_map_set(sorted_map, TEST_ENUM_FIRST, val1);
            d_enum_map_set(sorted_map, TEST_ENUM_SECOND, val2);
            d_enum_map_set(sorted_map, TEST_ENUM_THIRD, val3);
            
            bool sorted_result = d_enum_map_is_sorted(sorted_map);
            if (!d_assert_standalone(sorted_result == true, 
                                 "Properly sorted map returns true", 
                                 "Sorted map should return true", _test_info)) 
            {
                all_assertions_passed = false;
            }
        }
        
        free(val1);
        free(val2);
        free(val3);
    }
    
    // Test single element map (should be sorted)
    struct d_enum_map* single_map = d_enum_map_new_sized(2);
    if (single_map) 
    {
        char* single_val = d_test_create_enum_string("single", 1);
        if (single_val) 
        {
            d_enum_map_set(single_map, TEST_ENUM_FIRST, single_val);
            
            bool single_sorted = d_enum_map_is_sorted(single_map);
            if (!d_assert_standalone(single_sorted == true, 
                                 "Single element map is sorted", 
                                 "Single element should be sorted", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            free(single_val);
        }
    }
    
    // Test is_sorted on NULL map
    bool null_sorted = d_enum_map_is_sorted(NULL);
    if (!d_assert_standalone(null_sorted == true, 
                         "NULL map is considered sorted", 
                         "NULL map should return true for sorted", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_is_sorted unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_is_sorted unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(empty_map);
    d_enum_map_free(sorted_map);
    d_enum_map_free(single_map);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_count_valid
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing d_enum_map_count_valid ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    // Test empty map
    struct d_enum_map* map = d_enum_map_new_sized(6);
    if (map) 
    {
        size_t empty_count = d_enum_map_count_valid(map);
        if (!d_assert_standalone(empty_count == 0, 
                             "Empty map has zero valid entries", 
                             "Empty map should have 0 valid entries", _test_info)) 
        {
            all_assertions_passed = false;
        }
        
        // Add some entries and test count
        char* val1 = d_test_create_enum_string("count", 1);
        char* val2 = d_test_create_enum_string("count", 2);
        char* val3 = d_test_create_enum_string("count", 3);
        
        if (val1 && val2 && val3) 
        {
            d_enum_map_set(map, TEST_ENUM_FIRST, val1);
            size_t one_count = d_enum_map_count_valid(map);
            if (!d_assert_standalone(one_count == 1, 
                                 "Map with one entry has count 1", 
                                 "Count should be 1 after adding one entry", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            d_enum_map_set(map, TEST_ENUM_SECOND, val2);
            d_enum_map_set(map, TEST_ENUM_THIRD, val3);
            size_t three_count = d_enum_map_count_valid(map);
            if (!d_assert_standalone(three_count == 3, 
                                 "Map with three entries has count 3", 
                                 "Count should be 3 after adding three entries", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            // Update an existing entry (count should remain same)
            char* updated_val = d_test_create_enum_string("updated", 1);
            if (updated_val) 
            {
                d_enum_map_set(map, TEST_ENUM_FIRST, updated_val);
                size_t update_count = d_enum_map_count_valid(map);
                if (!d_assert_standalone(update_count == 3, 
                                     "Count unchanged after update", 
                                     "Count should not change after update", _test_info)) 
                {
                    all_assertions_passed = false;
                }
                
                free(updated_val);
            }
        }
        
        free(val1);
        free(val2);
        free(val3);
    }
    
    // Test count_valid on NULL map
    size_t null_count = d_enum_map_count_valid(NULL);
    if (!d_assert_standalone(null_count == 0, 
                         "NULL map has zero valid entries", 
                         "NULL map should return 0", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map_count_valid unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map_count_valid unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(map);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_query_all
(
    struct d_test_counter* _test_info
) 
{
    printf("\n--- Testing d_enum_map Query Functions ---\n");
    struct d_test_counter module_counter = {0, 0, 0, 0};
    
    bool contains_result = d_tests_sa_enum_map_contains(&module_counter);
    bool contains_all_result = d_tests_sa_enum_map_contains_all(&module_counter);
    bool is_sorted_result = d_tests_sa_enum_map_is_sorted(&module_counter);
    bool count_valid_result = d_tests_sa_enum_map_count_valid(&module_counter);
    
    // Update totals
    _test_info->assertions_total += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_run += module_counter.tests_run;
    _test_info->tests_passed += module_counter.tests_passed;
    
    bool overall_result = (contains_result && contains_all_result && is_sorted_result && count_valid_result);
    
    if (overall_result) 
    {
        printf("[PASS] Module d_enum_map Query: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    } 
    else 
    {
        printf("[FAIL] Module d_enum_map Query: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    }
    
    return overall_result;
}

bool 
d_tests_sa_enum_map_static_all
(
    struct d_test_counter* _test_info
) 
{
    printf("\n--- Testing d_enum_map Static Features ---\n");
    struct d_test_counter module_counter = {0, 0, 0, 0};
    
    bool static_result = d_tests_sa_enum_map_static_assertions(&module_counter);
    bool compile_result = d_tests_sa_enum_map_compile_time_init(&module_counter);
    
    // Update totals
    _test_info->assertions_total += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_run += module_counter.tests_run;
    _test_info->tests_passed += module_counter.tests_passed;
    
    bool overall_result = (static_result && compile_result);
    
    if (overall_result) 
    {
        printf("[PASS] Module d_enum_map Static: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    } 
    else 
    {
        printf("[FAIL] Module d_enum_map Static: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    }
    
    return overall_result;
}

// ============================================================================
// Integration and stress tests
// ============================================================================

bool 
d_tests_sa_enum_map_integration
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing enum_map integration scenarios ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    // Test comprehensive workflow: create, populate, query, modify
    struct d_enum_map* workflow_map = d_enum_map_new_sized(8);
    
    if (workflow_map) 
    {
        // Phase 1: Population
        char* http_ok = d_test_create_enum_string("HTTP", 200);
        char* http_not_found = d_test_create_enum_string("HTTP", 404);
        char* http_error = d_test_create_enum_string("HTTP", 500);
        
        if (http_ok && http_not_found && http_error) 
        {
            // Add entries in various orders to test sorting
            d_enum_map_set(workflow_map, 404, http_not_found);
            d_enum_map_set(workflow_map, 200, http_ok);
            d_enum_map_set(workflow_map, 500, http_error);
            
            // Phase 2: Validation
            if (!d_assert_standalone(d_enum_map_is_sorted(workflow_map), 
                                 "Integration - map remains sorted after insertions", 
                                 "Map should maintain sorted order", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            if (!d_assert_standalone(d_enum_map_count_valid(workflow_map) == 3, 
                                 "Integration - correct count after population", 
                                 "Map should have 3 valid entries", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            // Phase 3: Queries
            if (!d_assert_standalone(d_enum_map_contains_all(workflow_map, 200), 
                                 "Integration - contains check for 200", 
                                 "Map should contain HTTP 200", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            if (!d_assert_standalone(!d_enum_map_contains_all(workflow_map, 301), 
                                 "Integration - negative contains check", 
                                 "Map should not contain HTTP 301", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            // Phase 4: Retrieval and verification
            void* retrieved_ok = d_enum_map_get(workflow_map, 200);
            if (!d_assert_standalone(retrieved_ok == http_ok, 
                                 "Integration - retrieve HTTP 200 value", 
                                 "Should retrieve correct HTTP 200 value", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            // Phase 5: Updates
            char* updated_ok = d_test_create_enum_string("HTTP_UPDATED", 200);
            if (updated_ok) 
            {
                d_enum_map_set(workflow_map, 200, updated_ok);
                
                void* new_retrieved = d_enum_map_get(workflow_map, 200);
                if (!d_assert_standalone(new_retrieved == updated_ok, 
                                     "Integration - update existing entry", 
                                     "Should update existing entry correctly", _test_info)) 
                {
                    all_assertions_passed = false;
                }
                
                if (!d_assert_standalone(d_enum_map_count_valid(workflow_map) == 3, 
                                     "Integration - count unchanged after update", 
                                     "Count should remain same after update", _test_info)) 
                {
                    all_assertions_passed = false;
                }
                
                free(updated_ok);
            }
        }
        
        free(http_ok);
        free(http_not_found);
        free(http_error);
    }
    
    // Test interaction with compile-time map
    void* const_value = d_enum_map_get(&test_const_map, TEST_ENUM_SECOND);
    if (!d_assert_standalone(const_value != NULL, 
                         "Integration - access compile-time map", 
                         "Should access compile-time map", _test_info)) 
    {
        all_assertions_passed = false;
    }
    
    if (const_value) 
    {
        if (!d_assert_standalone(strcmp((char*)const_value, "second") == 0, 
                             "Integration - compile-time map has correct data", 
                             "Compile-time map should have correct values", _test_info)) 
        {
            all_assertions_passed = false;
        }
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map integration unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map integration unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(workflow_map);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_stress
(
    struct d_test_counter* _test_info
) 
{
    printf("  --- Testing enum_map stress scenarios ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool all_assertions_passed = true;
    
    const int num_entries = 50;
    const int max_key_value = 1000;
    
    // Create large map
    struct d_enum_map* stress_map = d_enum_map_new_sized(num_entries * 2);
    
    if (stress_map) 
    {
        // Phase 1: Bulk insertion with pseudo-random keys
        int* keys = malloc(num_entries * sizeof(int));
        char** values = malloc(num_entries * sizeof(char*));
        
        if (keys && values) 
        {
            // Generate sorted keys to ensure ordering
            for (int i = 0; i < num_entries; i++) 
            {
                keys[i] = (i * 17 + 7) % max_key_value;  // Pseudo-random but deterministic
                values[i] = d_test_create_enum_string("stress", keys[i]);
            }
            
            // Sort keys to maintain ordering requirement
            for (int i = 0; i < num_entries - 1; i++) 
            {
                for (int j = i + 1; j < num_entries; j++) 
                {
                    if (keys[i] > keys[j]) 
                    {
                        int temp_key = keys[i];
                        keys[i] = keys[j];
                        keys[j] = temp_key;
                        
                        char* temp_val = values[i];
                        values[i] = values[j];
                        values[j] = temp_val;
                    }
                }
            }
            
            // Insert all entries
            int successful_inserts = 0;
            for (int i = 0; i < num_entries; i++) 
            {
                if (values[i] && d_enum_map_set(stress_map, keys[i], values[i])) 
                {
                    successful_inserts++;
                }
            }
            
            if (!d_assert_standalone(successful_inserts >= num_entries / 2, 
                                 "Stress test - majority of insertions successful", 
                                 "Most insertions should succeed", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            // Phase 2: Verify map integrity
            if (!d_assert_standalone(d_enum_map_is_sorted(stress_map), 
                                 "Stress test - map remains sorted after bulk operations", 
                                 "Map should remain sorted", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            size_t valid_count = d_enum_map_count_valid(stress_map);
            if (!d_assert_standalone(valid_count > 0, 
                                 "Stress test - map has valid entries", 
                                 "Map should have valid entries after insertions", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            // Phase 3: Bulk lookups
            int successful_lookups = 0;
            for (int i = 0; i < num_entries; i += 3)  // Test every 3rd entry
            {
                void* found = d_enum_map_get(stress_map, keys[i]);
                if (found && found == values[i]) 
                {
                    successful_lookups++;
                }
            }
            
            int expected_lookups = (num_entries + 2) / 3;  // Ceiling division
            if (!d_assert_standalone(successful_lookups >= expected_lookups / 2, 
                                 "Stress test - majority of lookups successful", 
                                 "Most lookups should succeed", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            // Phase 4: Bulk updates
            int successful_updates = 0;
            for (int i = 0; i < num_entries; i += 5)  // Update every 5th entry
            {
                char* new_value = d_test_create_enum_string("updated", keys[i]);
                if (new_value && d_enum_map_set(stress_map, keys[i], new_value)) 
                {
                    successful_updates++;
                    free(new_value);
                }
            }
            
            if (!d_assert_standalone(successful_updates > 0, 
                                 "Stress test - some updates successful", 
                                 "Some updates should succeed", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            // Phase 5: Final integrity check
            if (!d_assert_standalone(d_enum_map_is_sorted(stress_map), 
                                 "Stress test - map sorted after all operations", 
                                 "Map should remain sorted after all operations", _test_info)) 
            {
                all_assertions_passed = false;
            }
            
            // Cleanup test data
            for (int i = 0; i < num_entries; i++) 
            {
                free(values[i]);
            }
        }
        
        // Free dynamically allocated arrays
        free(keys);
        free(values);
    }
    
    // Update test counter
    if (all_assertions_passed) 
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_enum_map stress unit test passed\n");
    } 
    else 
    {
        printf("  [FAIL] d_enum_map stress unit test failed\n");
    }
    _test_info->tests_run++;
    
    // Cleanup
    d_enum_map_free(stress_map);
    
    return (_test_info->tests_passed > initial_tests_passed);
}

bool 
d_tests_sa_enum_map_advanced_all
(
    struct d_test_counter* _test_info
) 
{
    printf("\n--- Testing d_enum_map Advanced Scenarios ---\n");
    struct d_test_counter module_counter = {0, 0, 0, 0};
    
    bool integration_result = d_tests_sa_enum_map_integration(&module_counter);
    bool stress_result = d_tests_sa_enum_map_stress(&module_counter);
    
    // Update totals
    _test_info->assertions_total += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_run += module_counter.tests_run;
    _test_info->tests_passed += module_counter.tests_passed;
    
    bool overall_result = (integration_result && stress_result);
    
    if (overall_result) 
    {
        printf("[PASS] Module d_enum_map Advanced: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    } 
    else 
    {
        printf("[FAIL] Module d_enum_map Advanced: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    }
    
    return overall_result;
}
// ============================================================================

bool 
d_tests_sa_enum_map_all
(
    struct d_test_counter* _test_info
) 
{
    printf("========================================\n");
    printf("Starting Enum Map Test Suite\n");
    printf("========================================\n");
    
    struct d_test_counter suite_counter = {0, 0, 0, 0};
    
    // Run test modules
    bool core_result = d_tests_sa_enum_map_core_all(&suite_counter);
    bool access_result = d_tests_sa_enum_map_access_all(&suite_counter);
    bool query_result = d_tests_sa_enum_map_query_all(&suite_counter);
    bool static_result = d_tests_sa_enum_map_static_all(&suite_counter);
    bool advanced_result = d_tests_sa_enum_map_advanced_all(&suite_counter);
    
    // Update totals
    _test_info->assertions_total += suite_counter.assertions_total;
    _test_info->assertions_passed += suite_counter.assertions_passed;
    _test_info->tests_run += suite_counter.tests_run;
    _test_info->tests_passed += suite_counter.tests_passed;
    
    printf("\n========================================\n");
    printf("Enum Map Test Suite Results\n");
    printf("========================================\n");
    printf("Suite Assertions: %zu/%zu passed\n", suite_counter.assertions_passed, suite_counter.assertions_total);
    printf("Suite Unit Tests: %zu/%zu passed\n", suite_counter.tests_passed, suite_counter.tests_run);
    
    bool overall_result = (core_result && access_result && query_result && static_result && advanced_result);
    
    if (overall_result) 
    {
        printf("%s Enum Map Test Suite: PASSED\n", TEST_SUCCESS_SYMBOL);
    } 
    else 
    {
        printf("%s Enum Map Test Suite: FAILED\n", TEST_FAIL_SYMBOL);
    }
    
    printf("\nSummary of tested components:\n");
    printf("%s Core Functions: d_enum_map_new, d_enum_map_new_sized, d_enum_map_new_copy\n", TEST_INFO_SYMBOL);
    printf("%s Construction: d_enum_map_new_args, d_enum_map_free\n", TEST_INFO_SYMBOL);
    printf("%s Access Functions: d_enum_map_find, d_enum_map_get, d_enum_map_set\n", TEST_INFO_SYMBOL);
    printf("%s Query Functions: d_enum_map_contains, d_enum_map_contains_all\n", TEST_INFO_SYMBOL);
    printf("%s Validation: d_enum_map_is_sorted, d_enum_map_count_valid\n", TEST_INFO_SYMBOL);
    printf("%s Static Features: Compile-time assertions and initialization\n", TEST_INFO_SYMBOL);
    printf("%s Advanced Testing: Integration workflows and stress testing\n", TEST_INFO_SYMBOL);
    
    return overall_result;
}