#include "./str_interp_tests_sa.h"


/******************************************************************************
 * SPECIFIER MANAGEMENT TESTS
 *****************************************************************************/

/*
d_tests_sa_str_interp_add_specifier
  Tests d_str_interp_add_specifier for adding specifiers.
  Tests the following:
  - adds a single specifier successfully
  - increments specifier_count
  - stored name matches input
  - stored value matches input
  - adding duplicate name updates instead of duplicating
  - returns ERROR_NULL_PARAM for NULL context
  - returns ERROR_NULL_PARAM for NULL name
  - returns ERROR_NULL_PARAM for NULL value
  - returns ERROR_TOO_MANY_SPECIFIERS at capacity
*/
struct d_test_object*
d_tests_sa_str_interp_add_specifier
(
    void
)
{
    struct d_test_object*        group;
    struct d_str_interp_context* ctx;
    enum d_str_interp_error      err;
    bool                         test_success;
    bool                         test_count;
    bool                         test_name_match;
    bool                         test_value_match;
    bool                         test_duplicate_updates;
    bool                         test_null_ctx;
    bool                         test_null_name;
    bool                         test_null_value;
    bool                         test_too_many;
    size_t                       i;
    size_t                       idx;
    char                         name_buf[32];

    ctx = d_str_interp_context_new();

    test_success          = false;
    test_count            = false;
    test_name_match       = false;
    test_value_match      = false;
    test_duplicate_updates = false;
    test_too_many         = false;

    if (ctx)
    {
        // test 1: add succeeds
        err = d_str_interp_add_specifier(ctx, "greeting", "Hello");
        test_success = (err == D_STR_INTERP_SUCCESS);

        // test 2: count incremented
        test_count = (ctx->specifier_count == 1);

        // test 3: name matches
        test_name_match = (d_strequals(ctx->specifiers[0].name,
                                       ctx->specifiers[0].name_length,
                                       "greeting",
                                       8));

        // test 4: value matches
        test_value_match = (d_strequals(ctx->specifiers[0].value,
                                        ctx->specifiers[0].value_length,
                                        "Hello",
                                        5));

        // test 5: duplicate name updates value, count stays at 1
        err = d_str_interp_add_specifier(ctx, "greeting", "Hola");
        test_duplicate_updates = ( (err == D_STR_INTERP_SUCCESS) &&
                                   (ctx->specifier_count == 1)   &&
                                   (d_strequals(ctx->specifiers[0].value,
                                                ctx->specifiers[0].value_length,
                                                "Hola",
                                                4)) );

        // test 9: fill to capacity, then try one more
        d_str_interp_context_clear(ctx);
        for (i = 0; i < D_STR_INTERP_MAX_SPECIFIERS; i++)
        {
            d_memset(name_buf, 0, sizeof(name_buf));
            name_buf[0] = 's';
            name_buf[1] = (char)('0' + (i / 10));
            name_buf[2] = (char)('0' + (i % 10));
            name_buf[3] = '\0';

            d_str_interp_add_specifier(ctx, name_buf, "v");
        }

        err = d_str_interp_add_specifier(ctx, "overflow", "boom");
        test_too_many = (err == D_STR_INTERP_ERROR_TOO_MANY_SPECIFIERS);

        d_str_interp_context_free(ctx);
    }

    // test 6: NULL context
    err = d_str_interp_add_specifier(NULL, "name", "val");
    test_null_ctx = (err == D_STR_INTERP_ERROR_NULL_PARAM);

    // test 7: NULL name
    ctx = d_str_interp_context_new();
    test_null_name = false;
    if (ctx)
    {
        err = d_str_interp_add_specifier(ctx, NULL, "val");
        test_null_name = (err == D_STR_INTERP_ERROR_NULL_PARAM);
        d_str_interp_context_free(ctx);
    }

    // test 8: NULL value
    ctx = d_str_interp_context_new();
    test_null_value = false;
    if (ctx)
    {
        err = d_str_interp_add_specifier(ctx, "name", NULL);
        test_null_value = (err == D_STR_INTERP_ERROR_NULL_PARAM);
        d_str_interp_context_free(ctx);
    }

    // build result tree
    group = d_test_object_new_interior("d_str_interp_add_specifier", 9);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("success",
                                           test_success,
                                           "add returns SUCCESS");
    group->elements[idx++] = D_ASSERT_TRUE("count",
                                           test_count,
                                           "specifier_count incremented");
    group->elements[idx++] = D_ASSERT_TRUE("name_match",
                                           test_name_match,
                                           "stored name matches input");
    group->elements[idx++] = D_ASSERT_TRUE("value_match",
                                           test_value_match,
                                           "stored value matches input");
    group->elements[idx++] = D_ASSERT_TRUE("duplicate_updates",
                                           test_duplicate_updates,
                                           "duplicate name updates value");
    group->elements[idx++] = D_ASSERT_TRUE("null_ctx",
                                           test_null_ctx,
                                           "ERROR_NULL_PARAM for NULL context");
    group->elements[idx++] = D_ASSERT_TRUE("null_name",
                                           test_null_name,
                                           "ERROR_NULL_PARAM for NULL name");
    group->elements[idx++] = D_ASSERT_TRUE("null_value",
                                           test_null_value,
                                           "ERROR_NULL_PARAM for NULL value");
    group->elements[idx++] = D_ASSERT_TRUE("too_many",
                                           test_too_many,
                                           "ERROR_TOO_MANY_SPECIFIERS at capacity");

    return group;
}

/*
d_tests_sa_str_interp_add_specifier_n
  Tests d_str_interp_add_specifier_n for explicit-length add.
  Tests the following:
  - adds with explicit lengths
  - name_length is stored correctly
  - value_length is stored correctly
  - handles substring name (not null-terminated at given length)
  - handles zero-length value
  - returns ERROR_NULL_PARAM for NULL inputs
*/
struct d_test_object*
d_tests_sa_str_interp_add_specifier_n
(
    void
)
{
    struct d_test_object*        group;
    struct d_str_interp_context* ctx;
    enum d_str_interp_error      err;
    bool                         test_success;
    bool                         test_name_len;
    bool                         test_value_len;
    bool                         test_substring;
    bool                         test_empty_value;
    bool                         test_null_param;
    size_t                       idx;

    ctx = d_str_interp_context_new();

    test_success     = false;
    test_name_len    = false;
    test_value_len   = false;
    test_substring   = false;
    test_empty_value = false;

    if (ctx)
    {
        // test 1: add with explicit lengths
        err = d_str_interp_add_specifier_n(ctx,
                                           "fullname",
                                           8,
                                           "Alice",
                                           5);
        test_success = (err == D_STR_INTERP_SUCCESS);

        // test 2: name_length stored
        test_name_len = (ctx->specifiers[0].name_length == 8);

        // test 3: value_length stored
        test_value_len = (ctx->specifiers[0].value_length == 5);

        // test 4: use partial name from a longer buffer
        // "username_extra" but only first 8 chars = "username"
        err = d_str_interp_add_specifier_n(ctx,
                                           "username_extra",
                                           8,
                                           "Bob",
                                           3);
        test_substring = ( (err == D_STR_INTERP_SUCCESS) &&
                           (ctx->specifiers[1].name_length == 8) &&
                           (d_strequals(ctx->specifiers[1].name,
                                        8,
                                        "username",
                                        8)) );

        // test 5: zero-length value
        err = d_str_interp_add_specifier_n(ctx,
                                           "empty",
                                           5,
                                           "",
                                           0);
        test_empty_value = ( (err == D_STR_INTERP_SUCCESS) &&
                             (ctx->specifiers[2].value_length == 0) );

        d_str_interp_context_free(ctx);
    }

    // test 6: NULL inputs
    err = d_str_interp_add_specifier_n(NULL, "a", 1, "b", 1);
    test_null_param = (err == D_STR_INTERP_ERROR_NULL_PARAM);

    // build result tree
    group = d_test_object_new_interior("d_str_interp_add_specifier_n", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("success",
                                           test_success,
                                           "add_n returns SUCCESS");
    group->elements[idx++] = D_ASSERT_TRUE("name_len",
                                           test_name_len,
                                           "name_length stored correctly");
    group->elements[idx++] = D_ASSERT_TRUE("value_len",
                                           test_value_len,
                                           "value_length stored correctly");
    group->elements[idx++] = D_ASSERT_TRUE("substring",
                                           test_substring,
                                           "handles substring name via length");
    group->elements[idx++] = D_ASSERT_TRUE("empty_value",
                                           test_empty_value,
                                           "handles zero-length value");
    group->elements[idx++] = D_ASSERT_TRUE("null_param",
                                           test_null_param,
                                           "ERROR_NULL_PARAM for NULL inputs");

    return group;
}

/*
d_tests_sa_str_interp_remove_specifier
  Tests d_str_interp_remove_specifier for removing specifiers.
  Tests the following:
  - removes an existing specifier
  - count decremented after removal
  - removed specifier is no longer found
  - remaining specifiers shift correctly
  - returns ERROR_SPECIFIER_NOT_FOUND for unknown name
  - returns ERROR_NULL_PARAM for NULL context
  - returns ERROR_NULL_PARAM for NULL name
  - handles removing last specifier
*/
struct d_test_object*
d_tests_sa_str_interp_remove_specifier
(
    void
)
{
    struct d_test_object*        group;
    struct d_str_interp_context* ctx;
    enum d_str_interp_error      err;
    bool                         test_remove;
    bool                         test_count;
    bool                         test_not_found_after;
    bool                         test_shift;
    bool                         test_unknown;
    bool                         test_null_ctx;
    bool                         test_null_name;
    bool                         test_remove_last;
    size_t                       idx;

    ctx = d_str_interp_context_new();

    test_remove          = false;
    test_count           = false;
    test_not_found_after = false;
    test_shift           = false;
    test_unknown         = false;
    test_remove_last     = false;

    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "first", "1");
        d_str_interp_add_specifier(ctx, "second", "2");
        d_str_interp_add_specifier(ctx, "third", "3");

        // test 1: remove succeeds
        err = d_str_interp_remove_specifier(ctx, "second");
        test_remove = (err == D_STR_INTERP_SUCCESS);

        // test 2: count decremented
        test_count = (ctx->specifier_count == 2);

        // test 3: removed specifier no longer found
        test_not_found_after = (!d_str_interp_has_specifier(ctx, "second"));

        // test 4: remaining specifiers preserved
        test_shift = ( (d_str_interp_has_specifier(ctx, "first")) &&
                       (d_str_interp_has_specifier(ctx, "third")) );

        // test 5: unknown name
        err = d_str_interp_remove_specifier(ctx, "nonexistent");
        test_unknown = (err == D_STR_INTERP_ERROR_SPECIFIER_NOT_FOUND);

        // test 8: remove until empty
        d_str_interp_remove_specifier(ctx, "first");
        err = d_str_interp_remove_specifier(ctx, "third");
        test_remove_last = ( (err == D_STR_INTERP_SUCCESS) &&
                             (ctx->specifier_count == 0) );

        d_str_interp_context_free(ctx);
    }

    // test 6: NULL context
    err = d_str_interp_remove_specifier(NULL, "name");
    test_null_ctx = (err == D_STR_INTERP_ERROR_NULL_PARAM);

    // test 7: NULL name
    ctx = d_str_interp_context_new();
    test_null_name = false;
    if (ctx)
    {
        err = d_str_interp_remove_specifier(ctx, NULL);
        test_null_name = (err == D_STR_INTERP_ERROR_NULL_PARAM);
        d_str_interp_context_free(ctx);
    }

    // build result tree
    group = d_test_object_new_interior("d_str_interp_remove_specifier", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("remove",
                                           test_remove,
                                           "removes existing specifier");
    group->elements[idx++] = D_ASSERT_TRUE("count",
                                           test_count,
                                           "count decremented after removal");
    group->elements[idx++] = D_ASSERT_TRUE("not_found_after",
                                           test_not_found_after,
                                           "removed specifier is gone");
    group->elements[idx++] = D_ASSERT_TRUE("shift",
                                           test_shift,
                                           "remaining specifiers preserved");
    group->elements[idx++] = D_ASSERT_TRUE("unknown",
                                           test_unknown,
                                           "ERROR_SPECIFIER_NOT_FOUND for unknown");
    group->elements[idx++] = D_ASSERT_TRUE("null_ctx",
                                           test_null_ctx,
                                           "ERROR_NULL_PARAM for NULL context");
    group->elements[idx++] = D_ASSERT_TRUE("null_name",
                                           test_null_name,
                                           "ERROR_NULL_PARAM for NULL name");
    group->elements[idx++] = D_ASSERT_TRUE("remove_last",
                                           test_remove_last,
                                           "removes until empty");

    return group;
}

/*
d_tests_sa_str_interp_update_specifier
  Tests d_str_interp_update_specifier for updating specifier values.
  Tests the following:
  - updates value of existing specifier
  - name is unchanged after update
  - value_length is updated
  - count unchanged after update
  - returns ERROR_SPECIFIER_NOT_FOUND for unknown name
  - returns ERROR_NULL_PARAM for NULL inputs
*/
struct d_test_object*
d_tests_sa_str_interp_update_specifier
(
    void
)
{
    struct d_test_object*        group;
    struct d_str_interp_context* ctx;
    enum d_str_interp_error      err;
    const char*                  val;
    bool                         test_update;
    bool                         test_name_unchanged;
    bool                         test_length_updated;
    bool                         test_count_same;
    bool                         test_not_found;
    bool                         test_null_param;
    size_t                       idx;

    ctx = d_str_interp_context_new();

    test_update         = false;
    test_name_unchanged = false;
    test_length_updated = false;
    test_count_same     = false;
    test_not_found      = false;

    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "color", "red");

        // test 1: update succeeds
        err = d_str_interp_update_specifier(ctx, "color", "blue");
        test_update = (err == D_STR_INTERP_SUCCESS);

        // test 2: value changed
        val = d_str_interp_get_specifier(ctx, "color");
        test_name_unchanged = ( (val != NULL) &&
                                (d_strequals(val,
                                             4,
                                             "blue",
                                             4)) );

        // test 3: value_length updated
        test_length_updated = (ctx->specifiers[0].value_length == 4);

        // test 4: count unchanged
        test_count_same = (ctx->specifier_count == 1);

        // test 5: unknown name
        err = d_str_interp_update_specifier(ctx, "missing", "nope");
        test_not_found = (err == D_STR_INTERP_ERROR_SPECIFIER_NOT_FOUND);

        d_str_interp_context_free(ctx);
    }

    // test 6: NULL inputs
    err = d_str_interp_update_specifier(NULL, "k", "v");
    test_null_param = (err == D_STR_INTERP_ERROR_NULL_PARAM);

    // build result tree
    group = d_test_object_new_interior("d_str_interp_update_specifier", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("update",
                                           test_update,
                                           "update returns SUCCESS");
    group->elements[idx++] = D_ASSERT_TRUE("name_unchanged",
                                           test_name_unchanged,
                                           "value changed to new string");
    group->elements[idx++] = D_ASSERT_TRUE("length_updated",
                                           test_length_updated,
                                           "value_length updated correctly");
    group->elements[idx++] = D_ASSERT_TRUE("count_same",
                                           test_count_same,
                                           "count unchanged after update");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "ERROR_SPECIFIER_NOT_FOUND for unknown");
    group->elements[idx++] = D_ASSERT_TRUE("null_param",
                                           test_null_param,
                                           "ERROR_NULL_PARAM for NULL inputs");

    return group;
}

/*
d_tests_sa_str_interp_get_specifier
  Tests d_str_interp_get_specifier for retrieving specifier values.
  Tests the following:
  - returns correct value for existing specifier
  - returns NULL for unknown name
  - returns NULL for NULL context
  - returns NULL for NULL name
  - returns updated value after update
  - returns NULL after specifier is removed
*/
struct d_test_object*
d_tests_sa_str_interp_get_specifier
(
    void
)
{
    struct d_test_object*        group;
    struct d_str_interp_context* ctx;
    const char*                  val;
    bool                         test_found;
    bool                         test_unknown;
    bool                         test_null_ctx;
    bool                         test_null_name;
    bool                         test_after_update;
    bool                         test_after_remove;
    size_t                       idx;

    ctx = d_str_interp_context_new();

    test_found        = false;
    test_unknown      = false;
    test_after_update = false;
    test_after_remove = false;

    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "animal", "cat");

        // test 1: found
        val = d_str_interp_get_specifier(ctx, "animal");
        test_found = ( (val != NULL) &&
                       (d_strequals(val, 3, "cat", 3)) );

        // test 2: unknown
        val = d_str_interp_get_specifier(ctx, "plant");
        test_unknown = (val == NULL);

        // test 5: after update
        d_str_interp_update_specifier(ctx, "animal", "dog");
        val = d_str_interp_get_specifier(ctx, "animal");
        test_after_update = ( (val != NULL) &&
                              (d_strequals(val, 3, "dog", 3)) );

        // test 6: after remove
        d_str_interp_remove_specifier(ctx, "animal");
        val = d_str_interp_get_specifier(ctx, "animal");
        test_after_remove = (val == NULL);

        d_str_interp_context_free(ctx);
    }

    // test 3: NULL context
    val = d_str_interp_get_specifier(NULL, "anything");
    test_null_ctx = (val == NULL);

    // test 4: NULL name
    ctx = d_str_interp_context_new();
    test_null_name = false;
    if (ctx)
    {
        val = d_str_interp_get_specifier(ctx, NULL);
        test_null_name = (val == NULL);
        d_str_interp_context_free(ctx);
    }

    // build result tree
    group = d_test_object_new_interior("d_str_interp_get_specifier", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("found",
                                           test_found,
                                           "returns value for existing specifier");
    group->elements[idx++] = D_ASSERT_TRUE("unknown",
                                           test_unknown,
                                           "returns NULL for unknown name");
    group->elements[idx++] = D_ASSERT_TRUE("null_ctx",
                                           test_null_ctx,
                                           "returns NULL for NULL context");
    group->elements[idx++] = D_ASSERT_TRUE("null_name",
                                           test_null_name,
                                           "returns NULL for NULL name");
    group->elements[idx++] = D_ASSERT_TRUE("after_update",
                                           test_after_update,
                                           "returns updated value after update");
    group->elements[idx++] = D_ASSERT_TRUE("after_remove",
                                           test_after_remove,
                                           "returns NULL after removal");

    return group;
}

/*
d_tests_sa_str_interp_has_specifier
  Tests d_str_interp_has_specifier for existence checking.
  Tests the following:
  - returns true for existing specifier
  - returns false for unknown name
  - returns false for NULL context
  - returns false for NULL name
  - returns false after removal
  - returns true after re-add
*/
struct d_test_object*
d_tests_sa_str_interp_has_specifier
(
    void
)
{
    struct d_test_object*        group;
    struct d_str_interp_context* ctx;
    bool                         test_exists;
    bool                         test_unknown;
    bool                         test_null_ctx;
    bool                         test_null_name;
    bool                         test_after_remove;
    bool                         test_re_add;
    size_t                       idx;

    ctx = d_str_interp_context_new();

    test_exists       = false;
    test_unknown      = false;
    test_after_remove = false;
    test_re_add       = false;

    if (ctx)
    {
        d_str_interp_add_specifier(ctx, "fruit", "apple");

        // test 1: exists
        test_exists = d_str_interp_has_specifier(ctx, "fruit");

        // test 2: unknown
        test_unknown = (!d_str_interp_has_specifier(ctx, "veggie"));

        // test 5: after removal
        d_str_interp_remove_specifier(ctx, "fruit");
        test_after_remove = (!d_str_interp_has_specifier(ctx, "fruit"));

        // test 6: re-add
        d_str_interp_add_specifier(ctx, "fruit", "banana");
        test_re_add = d_str_interp_has_specifier(ctx, "fruit");

        d_str_interp_context_free(ctx);
    }

    // test 3: NULL context
    test_null_ctx = (!d_str_interp_has_specifier(NULL, "key"));

    // test 4: NULL name
    ctx = d_str_interp_context_new();
    test_null_name = false;
    if (ctx)
    {
        test_null_name = (!d_str_interp_has_specifier(ctx, NULL));
        d_str_interp_context_free(ctx);
    }

    // build result tree
    group = d_test_object_new_interior("d_str_interp_has_specifier", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("exists",
                                           test_exists,
                                           "returns true for existing");
    group->elements[idx++] = D_ASSERT_TRUE("unknown",
                                           test_unknown,
                                           "returns false for unknown");
    group->elements[idx++] = D_ASSERT_TRUE("null_ctx",
                                           test_null_ctx,
                                           "returns false for NULL context");
    group->elements[idx++] = D_ASSERT_TRUE("null_name",
                                           test_null_name,
                                           "returns false for NULL name");
    group->elements[idx++] = D_ASSERT_TRUE("after_remove",
                                           test_after_remove,
                                           "returns false after removal");
    group->elements[idx++] = D_ASSERT_TRUE("re_add",
                                           test_re_add,
                                           "returns true after re-add");

    return group;
}

/*
d_tests_sa_str_interp_specifier_all
  Runs all specifier management tests.
  Tests the following:
  - d_str_interp_add_specifier
  - d_str_interp_add_specifier_n
  - d_str_interp_remove_specifier
  - d_str_interp_update_specifier
  - d_str_interp_get_specifier
  - d_str_interp_has_specifier
*/
struct d_test_object*
d_tests_sa_str_interp_specifier_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Specifier Management", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_str_interp_add_specifier();
    group->elements[idx++] = d_tests_sa_str_interp_add_specifier_n();
    group->elements[idx++] = d_tests_sa_str_interp_remove_specifier();
    group->elements[idx++] = d_tests_sa_str_interp_update_specifier();
    group->elements[idx++] = d_tests_sa_str_interp_get_specifier();
    group->elements[idx++] = d_tests_sa_str_interp_has_specifier();

    return group;
}
