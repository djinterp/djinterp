/******************************************************************************
* djinterp [test]                                 min_enum_map_tests_sa_core.c
*
*   Core operation tests for min_enum_map module.
*   Tests creation, insertion, retrieval, and removal operations.
*
*
* path:      \test\container\map\min_enum_map_tests_sa_core.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.24
******************************************************************************/

#include ".\min_enum_map_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

/*
d_test_min_enum_map_create_int
  Helper function to create a heap-allocated integer for testing.

Parameter(s):
  _value: integer value to allocate
Return:
  Pointer to newly allocated integer, or NULL on failure.
*/
int*
d_test_min_enum_map_create_int
(
    int _value
)
{
    int* ptr = malloc(sizeof(int));
    
    if (ptr)
    {
        *ptr = _value;
    }
    
    return ptr;
}

/*
d_test_min_enum_map_create_string
  Helper function to create a heap-allocated string copy for testing.

Parameter(s):
  _str: string to duplicate
Return:
  Pointer to newly allocated string, or NULL on failure.
*/
char*
d_test_min_enum_map_create_string
(
    const char* _str
)
{
    char* copy;
    size_t len;
    
    if (!_str)
    {
        return NULL;
    }
    
    len = d_strnlen(_str, SIZE_MAX);
    copy = malloc(len + 1);
    
    if (copy)
    {
        d_strcpy_s(copy, len + 1, _str);
    }
    
    return copy;
}


/******************************************************************************
 * MAP CREATION TESTS
 *****************************************************************************/

/*
d_tests_min_enum_map_new
  Tests d_min_enum_map_new for map creation.
  Tests the following:
  - allocates map successfully
  - initializes entries to NULL
  - initializes count to 0
  - initializes capacity to 0
  - map is ready for use
  - multiple maps can be created
*/
struct d_test_object*
d_tests_min_enum_map_new
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* map1;
    struct d_min_enum_map* map2;
    bool                   test_allocation;
    bool                   test_entries_null;
    bool                   test_count_zero;
    bool                   test_capacity_zero;
    bool                   test_ready_for_use;
    bool                   test_multiple_maps;
    size_t                 idx;

    // test 1: allocation succeeds
    map1 = d_min_enum_map_new();
    test_allocation = (map1 != NULL);

    // test 2: entries initialized to NULL
    test_entries_null = ( (map1 != NULL) && 
                          (map1->entries == NULL) );

    // test 3: count initialized to 0
    test_count_zero = ( (map1 != NULL) && 
                        (map1->count == 0) );

    // test 4: capacity initialized to 0
    test_capacity_zero = ( (map1 != NULL) && 
                           (map1->capacity == 0) );

    // test 5: ready for use (can insert immediately)
    test_ready_for_use = false;
    
    if (map1)
    {
        int* value = d_test_min_enum_map_create_int(42);
        bool result = d_min_enum_map_put(map1, TEST_COLOR_RED, value);
        test_ready_for_use = result;
        
        if (value)
        {
            free(value);
        }
    }

    // test 6: multiple maps can be created
    map2 = d_min_enum_map_new();
    test_multiple_maps = ( (map2 != NULL) && 
                           (map2 != map1) );

    // cleanup
    if (map1)
    {
        d_min_enum_map_free(map1);
    }

    if (map2)
    {
        d_min_enum_map_free(map2);
    }

    // build result tree
    group = d_test_object_new_interior("d_min_enum_map_new", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("allocation",
                                           test_allocation,
                                           "allocates map successfully");
    group->elements[idx++] = D_ASSERT_TRUE("entries_null",
                                           test_entries_null,
                                           "initializes entries to NULL");
    group->elements[idx++] = D_ASSERT_TRUE("count_zero",
                                           test_count_zero,
                                           "initializes count to 0");
    group->elements[idx++] = D_ASSERT_TRUE("capacity_zero",
                                           test_capacity_zero,
                                           "initializes capacity to 0");
    group->elements[idx++] = D_ASSERT_TRUE("ready_for_use",
                                           test_ready_for_use,
                                           "map is ready for use");
    group->elements[idx++] = D_ASSERT_TRUE("multiple_maps",
                                           test_multiple_maps,
                                           "multiple maps can be created");

    return group;
}


/******************************************************************************
 * INSERTION TESTS
 *****************************************************************************/

/*
d_tests_min_enum_map_put
  Tests d_min_enum_map_put for inserting entries.
  Tests the following:
  - returns false for NULL map
  - inserts single entry
  - updates count correctly
  - retrieves inserted value
  - updates existing key
  - handles multiple insertions
  - maintains sorted order
  - grows capacity when needed
*/
struct d_test_object*
d_tests_min_enum_map_put
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* map;
    int*                   value1;
    int*                   value2;
    int*                   value3;
    int*                   old_value;
    void*                  retrieved;
    void*                  freed_ptrs[50];
    bool                   result;
    bool                   already_freed;
    size_t                 initial_capacity;
    size_t                 freed_count;
    bool                   test_null_map;
    bool                   test_single_insert;
    bool                   test_count_update;
    bool                   test_retrieval;
    bool                   test_update_existing;
    bool                   test_multiple_inserts;
    bool                   test_sorted_order;
    bool                   test_capacity_growth;
    size_t                 i;
    size_t                 j;
    size_t                 idx;

    // allocate test values
    value1 = d_test_min_enum_map_create_int(10);
    value2 = d_test_min_enum_map_create_int(20);
    value3 = d_test_min_enum_map_create_int(30);

    // test 1: NULL map returns false
    result = d_min_enum_map_put(NULL, TEST_COLOR_RED, value1);
    test_null_map = !(result);

    // test 2: single insert succeeds
    map = d_min_enum_map_new();
    
    if (map)
    {
        result = d_min_enum_map_put(map, TEST_COLOR_RED, value1);
    }
    else
    {
        result = false;
    }
    
    test_single_insert = result;

    // test 3: count updated correctly
    test_count_update = ( (map) && 
                          (map->count == 1) );

    // test 4: can retrieve inserted value
    if (map)
    {
        retrieved = d_min_enum_map_get(map, TEST_COLOR_RED);
    }
    else
    {
        retrieved = NULL;
    }
    
    test_retrieval = (retrieved == value1);

    // test 5: update existing key (replacing value1 with value2)
    if (map)
    {
        // get old value before replacing
        old_value = (int*)d_min_enum_map_get(map, TEST_COLOR_RED);
        d_min_enum_map_put(map, TEST_COLOR_RED, value2);
        retrieved = d_min_enum_map_get(map, TEST_COLOR_RED);
    }
    else
    {
        old_value = NULL;
        retrieved = NULL;
    }
    
    test_update_existing = ( (retrieved == value2) && 
                             ( (map) && 
                               (map->count == 1) ) );

    // free the old value that was replaced
    if (old_value)
    {
        free(old_value);
    }

    // test 6: multiple inserts (reusing value2, and adding value3)
    if (map)
    {
        d_min_enum_map_put(map, TEST_COLOR_GREEN, value2);
        d_min_enum_map_put(map, TEST_COLOR_BLUE, value3);
    }
    
    test_multiple_inserts = ( (map) && 
                              (map->count == 3) );

    // test 7: maintains sorted order (keys should be ascending)
    test_sorted_order = false;
    
    if ( (map)          &&
         (map->entries) &&
         (map->count >= 3) )
    {
        test_sorted_order = ( (map->entries[0].key < map->entries[1].key) &&
                              (map->entries[1].key < map->entries[2].key) );
    }

    // test 8: capacity grows when needed
    if (map)
    {
        initial_capacity = map->capacity;
    }
    else
    {
        initial_capacity = 0;
    }
    
    for (i = 10; ( (i < 30) && (map) ); i++)
    {
        int* val;
        
        val = d_test_min_enum_map_create_int((int)i);
        d_min_enum_map_put(map, (int)i, val);
    }
    
    test_capacity_growth = ( (map) && 
                             (map->capacity > initial_capacity) );

    // cleanup: free all unique values in map, then free map
    // NOTE: value2 is stored at BOTH RED and GREEN (same pointer!)
    // We need to track freed pointers to avoid double-free
    if (map)
    {
        void* freed_ptrs[50];  // enough for our test
        size_t freed_count;
        size_t j;
        bool already_freed;
        
        freed_count = 0;
        
        for (i = 0; i < map->count; i++)
        {
            void* current;
            
            current = map->entries[i].value;
            
            if (!current)
            {
                continue;
            }
            
            // check if we've already freed this pointer
            already_freed = false;
            
            for (j = 0; j < freed_count; j++)
            {
                if (freed_ptrs[j] == current)
                {
                    already_freed = true;
                    break;
                }
            }
            
            // only free if not already freed
            if (!already_freed)
            {
                free(current);
                freed_ptrs[freed_count++] = current;
            }
        }
        
        d_min_enum_map_free(map);
    }

    // build result tree
    group = d_test_object_new_interior("d_min_enum_map_put", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("null_map",
                                           test_null_map,
                                           "returns false for NULL map");
    group->elements[idx++] = D_ASSERT_TRUE("single_insert",
                                           test_single_insert,
                                           "inserts single entry");
    group->elements[idx++] = D_ASSERT_TRUE("count_update",
                                           test_count_update,
                                           "updates count correctly");
    group->elements[idx++] = D_ASSERT_TRUE("retrieval",
                                           test_retrieval,
                                           "retrieves inserted value");
    group->elements[idx++] = D_ASSERT_TRUE("update_existing",
                                           test_update_existing,
                                           "updates existing key");
    group->elements[idx++] = D_ASSERT_TRUE("multiple_inserts",
                                           test_multiple_inserts,
                                           "handles multiple insertions");
    group->elements[idx++] = D_ASSERT_TRUE("sorted_order",
                                           test_sorted_order,
                                           "maintains sorted order");
    group->elements[idx++] = D_ASSERT_TRUE("capacity_growth",
                                           test_capacity_growth,
                                           "grows capacity when needed");

    return group;
}


/******************************************************************************
 * RETRIEVAL TESTS
 *****************************************************************************/

/*
d_tests_min_enum_map_get
  Tests d_min_enum_map_get for retrieving values.
  Tests the following:
  - returns NULL for NULL map
  - returns NULL for empty map
  - retrieves existing value
  - returns NULL for missing key
  - retrieves correct value among many
  - handles NULL values correctly
*/
struct d_test_object*
d_tests_min_enum_map_get
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* map;
    int*                   value1;
    int*                   value2;
    void*                  result;
    bool                   test_null_map;
    bool                   test_empty_map;
    bool                   test_existing_value;
    bool                   test_missing_key;
    bool                   test_correct_among_many;
    bool                   test_null_value;
    size_t                 idx;

    // test 1: NULL map returns NULL
    result = d_min_enum_map_get(NULL, TEST_COLOR_RED);
    test_null_map = (result == NULL);

    // test 2: empty map returns NULL
    map = d_min_enum_map_new();
    result = map 
             ? d_min_enum_map_get(map, TEST_COLOR_RED)
             : NULL;
    test_empty_map = (result == NULL);

    // test 3: retrieves existing value
    value1 = d_test_min_enum_map_create_int(42);
    
    if (map)
    {
        d_min_enum_map_put(map, TEST_COLOR_RED, value1);
        result = d_min_enum_map_get(map, TEST_COLOR_RED);
    }
    else
    {
        result = NULL;
    }
    
    test_existing_value = (result == value1);

    // test 4: returns NULL for missing key
    result = map 
             ? d_min_enum_map_get(map, TEST_COLOR_BLUE)
             : NULL;
    test_missing_key = (result == NULL);

    // test 5: retrieves correct value among many
    value2 = d_test_min_enum_map_create_int(99);
    
    if (map)
    {
        d_min_enum_map_put(map, TEST_COLOR_GREEN, value2);
        d_min_enum_map_put(map, TEST_COLOR_BLUE, value1);
        d_min_enum_map_put(map, TEST_COLOR_YELLOW, value2);

        result = d_min_enum_map_get(map, TEST_COLOR_GREEN);
    }
    else
    {
        result = NULL;
    }
    
    test_correct_among_many = (result == value2);

    // test 6: handles NULL value correctly
    if (map)
    {
        d_min_enum_map_put(map, TEST_COLOR_PURPLE, NULL);

        result = d_min_enum_map_get(map, TEST_COLOR_PURPLE);
    }
    else
    {
        result = (void*)1;  // non-NULL for failure
    }
    
    // Note: getting NULL could mean "not found" or "value is NULL"
    // We need to use contains to distinguish
    bool contains = map 
                    ? d_min_enum_map_contains(map, TEST_COLOR_PURPLE)
                    : false;
    test_null_value = ( (result == NULL) && 
                        (contains) );

    // cleanup
    if (value1)
    {
        free(value1);
    }

    if (value2)
    {
        free(value2);
    }

    if (map)
    {
        d_min_enum_map_free(map);
    }

    // build result tree
    group = d_test_object_new_interior("d_min_enum_map_get", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("null_map",
                                           test_null_map,
                                           "returns NULL for NULL map");
    group->elements[idx++] = D_ASSERT_TRUE("empty_map",
                                           test_empty_map,
                                           "returns NULL for empty map");
    group->elements[idx++] = D_ASSERT_TRUE("existing_value",
                                           test_existing_value,
                                           "retrieves existing value");
    group->elements[idx++] = D_ASSERT_TRUE("missing_key",
                                           test_missing_key,
                                           "returns NULL for missing key");
    group->elements[idx++] = D_ASSERT_TRUE("correct_among_many",
                                           test_correct_among_many,
                                           "retrieves correct value among many");
    group->elements[idx++] = D_ASSERT_TRUE("null_value",
                                           test_null_value,
                                           "handles NULL values correctly");

    return group;
}

/*
d_tests_min_enum_map_contains
  Tests d_min_enum_map_contains for checking key existence.
  Tests the following:
  - returns false for NULL map
  - returns false for empty map
  - returns true for existing key
  - returns false for missing key
  - distinguishes NULL value from missing key
  - works after removals
*/
struct d_test_object*
d_tests_min_enum_map_contains
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* map;
    int*                   value1;
    bool                   result;
    bool                   test_null_map;
    bool                   test_empty_map;
    bool                   test_existing_key;
    bool                   test_missing_key;
    bool                   test_null_value_distinction;
    bool                   test_after_removal;
    size_t                 idx;

    // test 1: NULL map returns false
    result = d_min_enum_map_contains(NULL, TEST_COLOR_RED);
    test_null_map = !(result);

    // test 2: empty map returns false
    map = d_min_enum_map_new();
    result = map 
             ? d_min_enum_map_contains(map, TEST_COLOR_RED) 
             : false;
    test_empty_map = !(result);

    // test 3: returns true for existing key
    value1 = d_test_min_enum_map_create_int(42);
    
    if (map)
    {
        d_min_enum_map_put(map, TEST_COLOR_RED, value1);
        result = d_min_enum_map_contains(map, TEST_COLOR_RED);
    }
    else
    {
        result = false;
    }
    
    test_existing_key = result;

    // test 4: returns false for missing key
    result = map ? d_min_enum_map_contains(map, TEST_COLOR_BLUE) : true;
    test_missing_key = !result;

    // test 5: distinguishes NULL value from missing key
    if (map)
    {
        d_min_enum_map_put(map, TEST_COLOR_GREEN, NULL);
    }
    
    bool contains_null = map 
                         ? d_min_enum_map_contains(map, TEST_COLOR_GREEN)
                         : false;
    bool contains_missing = map 
                            ? d_min_enum_map_contains(map, TEST_COLOR_PURPLE)
                            : true;
    test_null_value_distinction = contains_null && !contains_missing;

    // test 6: works after removals
    if (map)
    {
        d_min_enum_map_remove(map, TEST_COLOR_RED);

        result = d_min_enum_map_contains(map, TEST_COLOR_RED);
    }
    else
    {
        result = true;
    }
    
    test_after_removal = !result;

    // cleanup
    if (value1)
    {
        free(value1);
    }

    if (map)
    {
        d_min_enum_map_free(map);
    }

    // build result tree
    group = d_test_object_new_interior("d_min_enum_map_contains", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("null_map",
                                           test_null_map,
                                           "returns false for NULL map");
    group->elements[idx++] = D_ASSERT_TRUE("empty_map",
                                           test_empty_map,
                                           "returns false for empty map");
    group->elements[idx++] = D_ASSERT_TRUE("existing_key",
                                           test_existing_key,
                                           "returns true for existing key");
    group->elements[idx++] = D_ASSERT_TRUE("missing_key",
                                           test_missing_key,
                                           "returns false for missing key");
    group->elements[idx++] = D_ASSERT_TRUE("null_value_distinction",
                                           test_null_value_distinction,
                                           "distinguishes NULL value from missing");
    group->elements[idx++] = D_ASSERT_TRUE("after_removal",
                                           test_after_removal,
                                           "works after removals");

    return group;
}

/*
d_tests_min_enum_map_count
  Tests d_min_enum_map_count for getting entry count.
  Tests the following:
  - returns 0 for NULL map
  - returns 0 for empty map
  - returns correct count after inserts
  - updates after removals
  - updates after clear
  - handles updates (no count change)
*/
struct d_test_object*
d_tests_min_enum_map_count
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* map;
    int*                   value1;
    size_t                 count;
    bool                   test_null_map;
    bool                   test_empty_map;
    bool                   test_after_inserts;
    bool                   test_after_removals;
    bool                   test_after_clear;
    bool                   test_after_update;
    size_t                 idx;

    // test 1: NULL map returns 0
    count         = d_min_enum_map_count(NULL);
    test_null_map = (count == 0);

    // test 2: empty map returns 0
    map            = d_min_enum_map_new();
    count          = d_min_enum_map_count(map);
    test_empty_map = (count == 0);

    // test 3: correct count after inserts
    value1 = d_test_min_enum_map_create_int(42);
    
    if (map)
    {
        d_min_enum_map_put(map, TEST_COLOR_RED, value1);
        d_min_enum_map_put(map, TEST_COLOR_GREEN, value1);
        d_min_enum_map_put(map, TEST_COLOR_BLUE, value1);
        count = d_min_enum_map_count(map);
    }
    else
    {
        count = 999;
    }
    
    test_after_inserts = (count == 3);

    // test 4: updates after removals
    if (map)
    {
        d_min_enum_map_remove(map, TEST_COLOR_GREEN);
        count = d_min_enum_map_count(map);
    }
    else
    {
        count = 999;
    }
    
    test_after_removals = (count == 2);

    // test 5: updates after clear
    if (map)
    {
        d_min_enum_map_clear(map);
        count = d_min_enum_map_count(map);
    }
    else
    {
        count = 999;
    }
    
    test_after_clear = (count == 0);

    // test 6: no change on update
    if (map)
    {
        d_min_enum_map_put(map, TEST_COLOR_RED, value1);
        size_t count1 = d_min_enum_map_count(map);
        d_min_enum_map_put(map, TEST_COLOR_RED, value1);  // update
        size_t count2 = d_min_enum_map_count(map);
        test_after_update = (count1 == 1) && (count2 == 1);
    }
    else
    {
        test_after_update = false;
    }

    // cleanup
    if (value1)
    {
        free(value1);
    }

    if (map)
    {
        d_min_enum_map_free(map);
    }

    // build result tree
    group = d_test_object_new_interior("d_min_enum_map_count", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("null_map",
                                           test_null_map,
                                           "returns 0 for NULL map");
    group->elements[idx++] = D_ASSERT_TRUE("empty_map",
                                           test_empty_map,
                                           "returns 0 for empty map");
    group->elements[idx++] = D_ASSERT_TRUE("after_inserts",
                                           test_after_inserts,
                                           "returns correct count after inserts");
    group->elements[idx++] = D_ASSERT_TRUE("after_removals",
                                           test_after_removals,
                                           "updates after removals");
    group->elements[idx++] = D_ASSERT_TRUE("after_clear",
                                           test_after_clear,
                                           "updates after clear");
    group->elements[idx++] = D_ASSERT_TRUE("after_update",
                                           test_after_update,
                                           "no change on update");

    return group;
}


/******************************************************************************
 * REMOVAL TESTS
 *****************************************************************************/

/*
d_tests_min_enum_map_remove
  Tests d_min_enum_map_remove for removing entries.
  Tests the following:
  - returns false for NULL map
  - returns false for empty map
  - removes existing entry
  - returns false for missing key
  - maintains sorted order after removal
  - handles removal from middle
  - handles removal from end
  - handles removal from beginning
*/
struct d_test_object*
d_tests_min_enum_map_remove
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* map;
    int*                   value1;
    bool                   result;
    bool                   test_null_map;
    bool                   test_empty_map;
    bool                   test_existing_entry;
    bool                   test_missing_key;
    bool                   test_sorted_after_removal;
    bool                   test_remove_middle;
    bool                   test_remove_end;
    bool                   test_remove_beginning;
    size_t                 idx;

    // test 1: NULL map returns false
    result = d_min_enum_map_remove(NULL, TEST_COLOR_RED);
    test_null_map = !(result);

    // test 2: empty map returns false
    map = d_min_enum_map_new();
    result = map 
             ? d_min_enum_map_remove(map, TEST_COLOR_RED)
             : false;
    test_empty_map = !result;

    // test 3: removes existing entry
    value1 = d_test_min_enum_map_create_int(42);
    
    if (map)
    {
        d_min_enum_map_put(map, TEST_COLOR_RED, value1);
        result = d_min_enum_map_remove(map, TEST_COLOR_RED);
        bool still_contains = d_min_enum_map_contains(map, TEST_COLOR_RED);
        test_existing_entry = result && !still_contains;
    }
    else
    {
        test_existing_entry = false;
    }

    // test 4: returns false for missing key
    result = map 
             ? d_min_enum_map_remove(map, TEST_COLOR_BLUE)
             : true;
    test_missing_key = !result;

    // test 5: maintains sorted order after removal
    if (map)
    {
        d_min_enum_map_put(map, 10, value1);
        d_min_enum_map_put(map, 20, value1);
        d_min_enum_map_put(map, 30, value1);
        d_min_enum_map_put(map, 40, value1);
        d_min_enum_map_remove(map, 20);
    }
    
    test_sorted_after_removal = false;
    
    if ( (map)          && 
         (map->entries) && 
         (map->count >= 3) )
    {
        test_sorted_after_removal = (map->entries[0].key == 10) &&
                                    (map->entries[1].key == 30) &&
                                    (map->entries[2].key == 40);
    }

    // test 6: remove from middle
    test_remove_middle = test_sorted_after_removal;  // already tested

    // test 7: remove from end
    if (map)
    {
        result = d_min_enum_map_remove(map, 40);
    }
    else
    {
        result = false;
    }
    
    bool count_ok   = ( (map) && 
                        (map->count == 2) );
    bool last_ok    = ( (map)          && 
                        (map->entries) && 
                        (map->entries[map->count - 1].key == 30) );
    test_remove_end = ( (result)       && 
                        (count_ok)     && 
                        (last_ok) );

    // test 8: remove from beginning
    if (map)
    {
        result = d_min_enum_map_remove(map, 10);
    }
    else
    {
        result = false;
    }
    
    count_ok = ( (map) && 
                 (map->count == 1) );
    bool first_ok = ( (map)          && 
                      (map->entries) && 
                      (map->entries[0].key == 30) );
    test_remove_beginning = ( (result)   && 
                              (count_ok) && 
                              (first_ok) );

    // cleanup
    if (value1)
    {
        free(value1);
    }

    if (map)
    {
        d_min_enum_map_free(map);
    }

    // build result tree
    group = d_test_object_new_interior("d_min_enum_map_remove", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("null_map",
                                           test_null_map,
                                           "returns false for NULL map");
    group->elements[idx++] = D_ASSERT_TRUE("empty_map",
                                           test_empty_map,
                                           "returns false for empty map");
    group->elements[idx++] = D_ASSERT_TRUE("existing_entry",
                                           test_existing_entry,
                                           "removes existing entry");
    group->elements[idx++] = D_ASSERT_TRUE("missing_key",
                                           test_missing_key,
                                           "returns false for missing key");
    group->elements[idx++] = D_ASSERT_TRUE("sorted_after_removal",
                                           test_sorted_after_removal,
                                           "maintains sorted order");
    group->elements[idx++] = D_ASSERT_TRUE("remove_middle",
                                           test_remove_middle,
                                           "removes from middle");
    group->elements[idx++] = D_ASSERT_TRUE("remove_end",
                                           test_remove_end,
                                           "removes from end");
    group->elements[idx++] = D_ASSERT_TRUE("remove_beginning",
                                           test_remove_beginning,
                                           "removes from beginning");

    return group;
}


/******************************************************************************
 * CORE OPERATIONS AGGREGATOR
 *****************************************************************************/

/*
d_tests_min_enum_map_core_all
  Runs all core operation tests.
  Tests the following:
  - d_min_enum_map_new
  - d_min_enum_map_put
  - d_min_enum_map_get
  - d_min_enum_map_contains
  - d_min_enum_map_count
  - d_min_enum_map_remove
*/
struct d_test_object*
d_tests_min_enum_map_core_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Core Map Operations", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_min_enum_map_new();
    group->elements[idx++] = d_tests_min_enum_map_put();
    group->elements[idx++] = d_tests_min_enum_map_get();
    group->elements[idx++] = d_tests_min_enum_map_contains();
    group->elements[idx++] = d_tests_min_enum_map_count();
    group->elements[idx++] = d_tests_min_enum_map_remove();

    return group;
}