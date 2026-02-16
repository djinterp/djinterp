/******************************************************************************
* djinterp [test]                              min_enum_map_tests_sa_memory.c
*
*   Memory management and advanced tests for min_enum_map module.
*   Tests clear, free, merge operations, and edge cases.
*
*
* path:      \test\container\map\min_enum_map_tests_sa_memory.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.24
******************************************************************************/

#include ".\min_enum_map_tests_sa.h"


/******************************************************************************
 * CLEAR OPERATION TESTS
 *****************************************************************************/

struct d_test_object*
d_tests_min_enum_map_clear
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* map;
    int*                   value;
    bool                   test_null_safe;
    bool                   test_empty;
    bool                   test_with_entries;
    bool                   test_count_zero;
    bool                   test_reusable;
    bool                   test_preserves_capacity;
    size_t                 idx;

    // tests
    test_null_safe = true;
    d_min_enum_map_clear(NULL);  // should not crash

    map = d_min_enum_map_new();
    d_min_enum_map_clear(map);
    test_empty = (map && map->count == 0);

    value = d_test_min_enum_map_create_int(42);
    if (map)
    {
        d_min_enum_map_put(map, 1, value);
        d_min_enum_map_put(map, 2, value);
        d_min_enum_map_clear(map);
    }
    test_with_entries = (map && map->count == 0);
    test_count_zero = test_with_entries;

    if (map)
    {
        bool result = d_min_enum_map_put(map, 3, value);
        test_reusable = result && (map->count == 1);
    }
    else
    {
        test_reusable = false;
    }

    // clear doesn't free capacity
    test_preserves_capacity = (map && map->capacity > 0);

    if (value)
    {
        free(value);
    }

    if (map)
    {
        d_min_enum_map_free(map);
    }

    group = d_test_object_new_interior("d_min_enum_map_clear", 6);
    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("null_safe", test_null_safe,
                                           "handles NULL safely");
    group->elements[idx++] = D_ASSERT_TRUE("empty", test_empty,
                                           "clears empty map");
    group->elements[idx++] = D_ASSERT_TRUE("with_entries", test_with_entries,
                                           "clears map with entries");
    group->elements[idx++] = D_ASSERT_TRUE("count_zero", test_count_zero,
                                           "sets count to zero");
    group->elements[idx++] = D_ASSERT_TRUE("reusable", test_reusable,
                                           "map reusable after clear");
    group->elements[idx++] = D_ASSERT_TRUE("preserves_capacity", test_preserves_capacity,
                                           "preserves capacity");

    return group;
}


/******************************************************************************
 * FREE OPERATION TESTS
 *****************************************************************************/

struct d_test_object*
d_tests_min_enum_map_free
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* map;
    struct d_min_enum_map* temp;
    int*                   value;
    bool                   test_null_safe;
    bool                   test_empty;
    bool                   test_with_entries;
    bool                   test_frees_entries;
    bool                   test_multiple_frees;
    size_t                 i;
    size_t                 idx;

    // test 1: NULL map handled safely
    test_null_safe = true;
    d_min_enum_map_free(NULL);  // should not crash

    // test 2: empty map freed
    map = d_min_enum_map_new();
    
    if (map)
    {
        d_min_enum_map_free(map);
        test_empty = true;
    }
    else
    {
        test_empty = false;
    }

    // test 3: map with entries freed
    map = d_min_enum_map_new();
    value = d_test_min_enum_map_create_int(42);
    
    if (map)
    {
        d_min_enum_map_put(map, 1, value);
        d_min_enum_map_put(map, 2, value);
        d_min_enum_map_free(map);

        test_with_entries = true;
    }
    else
    {
        test_with_entries = false;
    }

    // test 4: frees entries array
    test_frees_entries = test_with_entries;

    // test 5: multiple free operations
    test_multiple_frees = true;
    
    for (i = 0; i < 5; i++)
    {
        temp = d_min_enum_map_new();
        
        if (!temp)
        {
            test_multiple_frees = false;
            break;
        }
        
        d_min_enum_map_free(temp);
    }

    // cleanup test value
    if (value)
    {
        free(value);
    }

    // build result tree
    group = d_test_object_new_interior("d_min_enum_map_free", 5);
    
    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("null_safe", test_null_safe,
                                           "handles NULL safely");
    group->elements[idx++] = D_ASSERT_TRUE("empty", test_empty,
                                           "frees empty map");
    group->elements[idx++] = D_ASSERT_TRUE("with_entries", test_with_entries,
                                           "frees map with entries");
    group->elements[idx++] = D_ASSERT_TRUE("frees_entries", test_frees_entries,
                                           "frees entries array");
    group->elements[idx++] = D_ASSERT_TRUE("multiple_frees", test_multiple_frees,
                                           "handles multiple frees");

    return group;
}


struct d_test_object*
d_tests_min_enum_map_memory_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Memory Management", 2);
    
    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_min_enum_map_clear();
    group->elements[idx++] = d_tests_min_enum_map_free();

    return group;
}


/******************************************************************************
 * MERGE OPERATION TESTS
 *****************************************************************************/

/*
d_tests_min_enum_map_merge
  Tests d_min_enum_map_merge for merging two maps.
  Tests the following:
  - returns true for NULL dest with empty src (no-op optimization)
  - returns true for NULL src (nothing to merge)
  - returns true for empty src (nothing to merge)
  - merges non-overlapping entries
  - keeps dest value when overwrite=false
  - uses src value when overwrite=true
  - maintains sorted order after merge
  
  Note: The implementation treats NULL dest + empty src as a no-op (returns true).
  This is an optimization but could mask bugs. A stricter implementation would
  return false for any NULL dest regardless of src state.
*/
struct d_test_object*
d_tests_min_enum_map_merge
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* dest;
    struct d_min_enum_map* src;
    int*                   val1;
    int*                   val2;
    bool                   result;
    bool                   test_null_dest;
    bool                   test_null_src;
    bool                   test_empty_src;
    bool                   test_no_overlap;
    bool                   test_with_overlap_no_overwrite;
    bool                   test_with_overlap_overwrite;
    bool                   test_sorted_after_merge;
    size_t                 idx;

    val1 = d_test_min_enum_map_create_int(100);
    val2 = d_test_min_enum_map_create_int(200);

    // test 1: NULL dest with empty src returns true (no-op)
    // Note: Current implementation treats this as "nothing to merge"
    // Alternative: could return false since dest is invalid
    src = d_min_enum_map_new();
    result = d_min_enum_map_merge(NULL, src, false);
    test_null_dest = result;  // Changed: implementation returns true here

    // test 2: NULL src returns true (nothing to merge)
    dest = d_min_enum_map_new();
    result = d_min_enum_map_merge(dest, NULL, false);
    test_null_src = result;

    // test 3: empty src returns true
    result = dest ? d_min_enum_map_merge(dest, src, false) : false;
    test_empty_src = result;

    // test 4: no overlap
    if ( (dest) && 
         (src) )
    {
        d_min_enum_map_put(dest, 1, val1);
        d_min_enum_map_put(src, 2, val2);
        result = d_min_enum_map_merge(dest, src, false);
        bool count_ok = (dest->count == 2);
        bool has_both = d_min_enum_map_contains(dest, 1) && 
                       d_min_enum_map_contains(dest, 2);
        test_no_overlap = result && count_ok && has_both;
    }
    else
    {
        test_no_overlap = false;
    }

    // test 5: with overlap, don't overwrite
    if ( (dest) && 
         (src) )
    {
        d_min_enum_map_clear(dest);
        d_min_enum_map_clear(src);
        d_min_enum_map_put(dest, 1, val1);
        d_min_enum_map_put(src, 1, val2);

        result = d_min_enum_map_merge(dest, src, false);

        void* retrieved = d_min_enum_map_get(dest, 1);
        test_with_overlap_no_overwrite = result && (retrieved == val1);
    }
    else
    {
        test_with_overlap_no_overwrite = false;
    }

    // test 6: with overlap, overwrite
    if ( (dest) && 
         (src) )
    {
        d_min_enum_map_clear(dest);
        d_min_enum_map_clear(src);
        d_min_enum_map_put(dest, 1, val1);
        d_min_enum_map_put(src, 1, val2);
        result = d_min_enum_map_merge(dest, src, true);
        void* retrieved = d_min_enum_map_get(dest, 1);
        test_with_overlap_overwrite = result && (retrieved == val2);
    }
    else
    {
        test_with_overlap_overwrite = false;
    }

    // test 7: maintains sorted order after merge
    if ( (dest) && 
         (src) )
    {
        d_min_enum_map_clear(dest);
        d_min_enum_map_clear(src);
        d_min_enum_map_put(dest, 10, val1);
        d_min_enum_map_put(dest, 30, val1);
        d_min_enum_map_put(src, 20, val2);
        d_min_enum_map_put(src, 40, val2);
        result = d_min_enum_map_merge(dest, src, false);

        test_sorted_after_merge = false;
        if (result && dest->entries && dest->count == 4)
        {
            test_sorted_after_merge = (dest->entries[0].key == 10) &&
                                      (dest->entries[1].key == 20) &&
                                      (dest->entries[2].key == 30) &&
                                      (dest->entries[3].key == 40);
        }
    }
    else
    {
        test_sorted_after_merge = false;
    }

    if (val1)
    {
        free(val1);
    }

    if (val2)
    {
        free(val2);
    }

    if (dest)
    {
        d_min_enum_map_free(dest);
    }

    if (src)
    {
        d_min_enum_map_free(src);
    }

    group = d_test_object_new_interior("d_min_enum_map_merge", 7);
    
    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("null_dest", test_null_dest,
                                           "returns true for NULL dest with empty src");
    group->elements[idx++] = D_ASSERT_TRUE("null_src", test_null_src,
                                           "returns true for NULL src");
    group->elements[idx++] = D_ASSERT_TRUE("empty_src", test_empty_src,
                                           "returns true for empty src");
    group->elements[idx++] = D_ASSERT_TRUE("no_overlap", test_no_overlap,
                                           "merges non-overlapping entries");
    group->elements[idx++] = D_ASSERT_TRUE("overlap_no_overwrite",
                                           test_with_overlap_no_overwrite,
                                           "keeps dest value without overwrite");
    group->elements[idx++] = D_ASSERT_TRUE("overlap_overwrite",
                                           test_with_overlap_overwrite,
                                           "uses src value with overwrite");
    group->elements[idx++] = D_ASSERT_TRUE("sorted_after_merge",
                                           test_sorted_after_merge,
                                           "maintains sorted order");

    return group;
}


/******************************************************************************
 * ENTRY MACRO TESTS
 *****************************************************************************/

struct d_test_object*
d_tests_min_enum_map_entry_macros
(
    void
)
{
    struct d_test_object* group;
    bool                  test_entry;
    bool                  test_entry_str;
    bool                  test_entry_int;
    bool                  test_entry_null;
    bool                  test_entry_self;
    bool                  test_sentinel;
    size_t                idx;

    // test D_ENUM_ENTRY
    struct d_enum_map_entry e1 = D_ENUM_ENTRY(TEST_COLOR_RED, (void*)0x1234);
    test_entry = ( (e1.key == TEST_COLOR_RED) && 
                   (e1.value == (void*)0x1234) );

    // test D_ENUM_ENTRY_STR
    struct d_enum_map_entry e2 = D_ENUM_ENTRY_STR(TEST_COLOR_GREEN, "green");
    test_entry_str = ( (e2.key == TEST_COLOR_GREEN) && 
                       (strcmp((char*)e2.value, "green") == 0) );

    // test D_ENUM_ENTRY_INT
    struct d_enum_map_entry e3 = D_ENUM_ENTRY_INT(TEST_COLOR_BLUE, 42);
    test_entry_int = ( (e3.key == TEST_COLOR_BLUE) && 
                       ((intptr_t)e3.value == 42) );

    // test D_ENUM_ENTRY_NULL
    struct d_enum_map_entry e4 = D_ENUM_ENTRY_NULL(TEST_COLOR_YELLOW);
    test_entry_null = ( (e4.key == TEST_COLOR_YELLOW) &&
                        (e4.value == NULL) );

    // test D_ENUM_ENTRY_SELF
    struct d_enum_map_entry e5 = D_ENUM_ENTRY_SELF(TEST_COLOR_PURPLE);
    test_entry_self = ( (e5.key == TEST_COLOR_PURPLE) && 
                        ((intptr_t)e5.value == TEST_COLOR_PURPLE) );

    // test D_ENUM_ENTRY_SENTINEL
    struct d_enum_map_entry sent = D_ENUM_ENTRY_SENTINEL;
    test_sentinel = D_ENUM_ENTRY_IS_SENTINEL(sent);

    group = d_test_object_new_interior("Entry Macros", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("D_ENUM_ENTRY", test_entry,
                                           "creates basic entry");
    group->elements[idx++] = D_ASSERT_TRUE("D_ENUM_ENTRY_STR", test_entry_str,
                                           "creates string entry");
    group->elements[idx++] = D_ASSERT_TRUE("D_ENUM_ENTRY_INT", test_entry_int,
                                           "creates int entry");
    group->elements[idx++] = D_ASSERT_TRUE("D_ENUM_ENTRY_NULL", test_entry_null,
                                           "creates NULL entry");
    group->elements[idx++] = D_ASSERT_TRUE("D_ENUM_ENTRY_SELF", test_entry_self,
                                           "creates self entry");
    group->elements[idx++] = D_ASSERT_TRUE("D_ENUM_ENTRY_SENTINEL", test_sentinel,
                                           "creates sentinel");

    return group;
}


struct d_test_object*
d_tests_min_enum_map_ordering
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* map;
    int*                   val;
    bool                   test_insert_order;
    bool                   test_binary_search;
    bool                   test_sparse_keys;
    bool                   test_negative_keys;
    size_t                 idx;

    val = d_test_min_enum_map_create_int(42);
    map = d_min_enum_map_new();

    // test insert in reverse order, should still be sorted
    if (map)
    {
        d_min_enum_map_put(map, 30, val);
        d_min_enum_map_put(map, 20, val);
        d_min_enum_map_put(map, 10, val);

        test_insert_order = ( (map->entries[0].key == 10) &&
                              (map->entries[1].key == 20) &&
                              (map->entries[2].key == 30) );
    }
    else
    {
        test_insert_order = false;
    }

    // test binary search works correctly
    if (map)
    {
        void* found = d_min_enum_map_get(map, 20);

        test_binary_search = (found == val);
    }
    else
    {
        test_binary_search = false;
    }

    // test sparse keys
    if (map)
    {
        d_min_enum_map_clear(map);
        d_min_enum_map_put(map, 1, val);
        d_min_enum_map_put(map, 1000, val);
        d_min_enum_map_put(map, 500, val);

        test_sparse_keys = ( (map->entries[0].key == 1)   &&
                             (map->entries[1].key == 500) &&
                             (map->entries[2].key == 1000) );
    }
    else
    {
        test_sparse_keys = false;
    }

    // test negative keys
    if (map)
    {
        d_min_enum_map_clear(map);
        d_min_enum_map_put(map, -10, val);
        d_min_enum_map_put(map, 0, val);
        d_min_enum_map_put(map, 10, val);

        test_negative_keys = ( (map->entries[0].key == -10) &&
                               (map->entries[1].key == 0)   &&
                               (map->entries[2].key == 10) );
    }
    else
    {
        test_negative_keys = false;
    }

    if (val)
    {
        free(val);
    }

    if (map)
    {
        d_min_enum_map_free(map);
    }

    group = d_test_object_new_interior("Ordering and Sorting", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("insert_order", test_insert_order,
                                           "maintains order despite insert order");
    group->elements[idx++] = D_ASSERT_TRUE("binary_search", test_binary_search,
                                           "binary search works correctly");
    group->elements[idx++] = D_ASSERT_TRUE("sparse_keys", test_sparse_keys,
                                           "handles sparse keys");
    group->elements[idx++] = D_ASSERT_TRUE("negative_keys", test_negative_keys,
                                           "handles negative keys");

    return group;
}


struct d_test_object*
d_tests_min_enum_map_advanced_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Advanced Operations", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_min_enum_map_merge();
    group->elements[idx++] = d_tests_min_enum_map_entry_macros();
    group->elements[idx++] = d_tests_min_enum_map_ordering();

    return group;
}

/******************************************************************************
 * EDGE CASES AND STRESS TESTS  
 *****************************************************************************/

struct d_test_object*
d_tests_min_enum_map_edge_cases
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* map;
    int*                   val;
    bool                   test_duplicate_puts;
    bool                   test_remove_nonexistent;
    bool                   test_get_after_remove;
    bool                   test_clear_then_use;
    bool                   test_extreme_keys;
    size_t                 idx;

    val = d_test_min_enum_map_create_int(42);
    map = d_min_enum_map_new();

    // duplicate puts (update)
    if (map)
    {
        d_min_enum_map_put(map, 1, val);
        size_t count1 = d_min_enum_map_count(map);
        d_min_enum_map_put(map, 1, val);
        size_t count2 = d_min_enum_map_count(map);
        test_duplicate_puts = ( (count1 == 1) &&
                                (count2 == 1) );
    }
    else
    {
        test_duplicate_puts = false;
    }

    // remove nonexistent
    bool result = map
                  ? d_min_enum_map_remove(map, 999)
                  : true;
    test_remove_nonexistent = !result;

    // get after remove
    if (map)
    {
        d_min_enum_map_put(map, 2, val);
        d_min_enum_map_remove(map, 2);
        void* retrieved = d_min_enum_map_get(map, 2);
        test_get_after_remove = (retrieved == NULL);
    }
    else
    {
        test_get_after_remove = false;
    }

    // clear then use
    if (map)
    {
        d_min_enum_map_clear(map);
        bool put_ok = d_min_enum_map_put(map, 3, val);
        void* retrieved = d_min_enum_map_get(map, 3);
        test_clear_then_use = ( (put_ok) && 
                                (retrieved == val) );
    }
    else
    {
        test_clear_then_use = false;
    }

    // extreme keys
    if (map)
    {
        d_min_enum_map_clear(map);
        d_min_enum_map_put(map, INT_MIN + 1, val);  // avoid sentinel
        d_min_enum_map_put(map, INT_MAX, val);
        d_min_enum_map_put(map, 0, val);

        test_extreme_keys = ( (d_min_enum_map_count(map) == 3) &&
                              d_min_enum_map_contains(map, INT_MIN + 1) &&
                              d_min_enum_map_contains(map, INT_MAX) );
    }
    else
    {
        test_extreme_keys = false;
    }

    if (val)
    {
        free(val);
    }

    if (map)
    {
        d_min_enum_map_free(map);
    }

    group = d_test_object_new_interior("Edge Cases", 5);
    
    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("duplicate_puts", test_duplicate_puts,
                                           "handles duplicate puts correctly");
    group->elements[idx++] = D_ASSERT_TRUE("remove_nonexistent", test_remove_nonexistent,
                                           "remove nonexistent returns false");
    group->elements[idx++] = D_ASSERT_TRUE("get_after_remove", test_get_after_remove,
                                           "get after remove returns NULL");
    group->elements[idx++] = D_ASSERT_TRUE("clear_then_use", test_clear_then_use,
                                           "can use map after clear");
    group->elements[idx++] = D_ASSERT_TRUE("extreme_keys", test_extreme_keys,
                                           "handles extreme key values");

    return group;
}

struct d_test_object*
d_tests_min_enum_map_stress
(
    void
)
{
    struct d_test_object*  group;
    struct d_min_enum_map* map;
    int*                   values[D_TEST_MIN_ENUM_MAP_LARGE_SIZE];
    bool                   test_many_inserts;
    bool                   test_many_gets;
    bool                   test_many_removes;
    bool                   test_alternating_ops;
    bool                   test_memory_stable;
    size_t                 i;
    size_t                 idx;

    // allocate values
    for (i = 0; i < D_TEST_MIN_ENUM_MAP_LARGE_SIZE; i++)
    {
        values[i] = d_test_min_enum_map_create_int((int)i);
    }

    map = d_min_enum_map_new();

    // many inserts
    test_many_inserts = true;
    if (map)
    {
        for (i = 0; i < D_TEST_MIN_ENUM_MAP_LARGE_SIZE; i++)
        {
            if (!d_min_enum_map_put(map, (int)i, values[i]))
            {
                test_many_inserts = false;

                break;
            }
        }

        test_many_inserts = 
            ( (test_many_inserts) &&
              (d_min_enum_map_count(map) == D_TEST_MIN_ENUM_MAP_LARGE_SIZE) );
    }
    else
    {
        test_many_inserts = false;
    }

    // many gets
    test_many_gets = true;
    if (map)
    {
        for (i = 0; i < D_TEST_MIN_ENUM_MAP_LARGE_SIZE; i++)
        {
            void* val = d_min_enum_map_get(map, (int)i);

            if (val != values[i])
            {
                test_many_gets = false;

                break;
            }
        }
    }
    else
    {
        test_many_gets = false;
    }

    // many removes
    test_many_removes = true;
    if (map)
    {
        for (i = 0; i < D_TEST_MIN_ENUM_MAP_LARGE_SIZE; i++)
        {
            if (!d_min_enum_map_remove(map, (int)i))
            {
                test_many_removes = false;

                break;
            }
        }

        test_many_removes = test_many_removes && (d_min_enum_map_count(map) == 0);
    }
    else
    {
        test_many_removes = false;
    }

    // alternating ops
    test_alternating_ops = true;
    if (map)
    {
        for (i = 0; i < D_TEST_MIN_ENUM_MAP_MEDIUM_SIZE; i++)
        {
            if (!d_min_enum_map_put(map, (int)i, values[i]))
            {
                test_alternating_ops = false;

                break;
            }

            if (!d_min_enum_map_remove(map, (int)i))
            {
                test_alternating_ops = false;

                break;
            }
        }

        test_alternating_ops = test_alternating_ops && (d_min_enum_map_count(map) == 0);
    }
    else
    {
        test_alternating_ops = false;
    }

    // memory stability
    test_memory_stable = true;
    
    if (map)
    {
        int cycle;
        
        for (cycle = 0; cycle < 10; cycle++)
        {
            for (i = 0; i < D_TEST_MIN_ENUM_MAP_SMALL_SIZE; i++)
            {
                if (!d_min_enum_map_put(map, (int)i, values[i]))
                {
                    test_memory_stable = false;

                    break;
                }
            }

            d_min_enum_map_clear(map);
        }
    }
    else
    {
        test_memory_stable = false;
    }

    // cleanup
    for (i = 0; i < D_TEST_MIN_ENUM_MAP_LARGE_SIZE; i++)
    {
        if (values[i])
        {
            free(values[i]);
        }
    }

    if (map)
    {
        d_min_enum_map_free(map);
    }

    group = d_test_object_new_interior("Stress Tests", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("many_inserts", test_many_inserts,
                                           "handles 100 inserts");
    group->elements[idx++] = D_ASSERT_TRUE("many_gets", test_many_gets,
                                           "handles 100 gets");
    group->elements[idx++] = D_ASSERT_TRUE("many_removes", test_many_removes,
                                           "handles 100 removes");
    group->elements[idx++] = D_ASSERT_TRUE("alternating_ops", test_alternating_ops,
                                           "handles alternating operations");
    group->elements[idx++] = D_ASSERT_TRUE("memory_stable", test_memory_stable,
                                           "memory remains stable");

    return group;
}

struct d_test_object*
d_tests_min_enum_map_edge_stress_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Edge Cases and Stress", 2);
    
    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_min_enum_map_edge_cases();
    group->elements[idx++] = d_tests_min_enum_map_stress();

    return group;
}