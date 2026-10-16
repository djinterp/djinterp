/*******************************************************************************
* djinterp [text]                               text_template_tests_sa_binding.c
*
*
* path:      /tests/djinterp/c/text/text_template_tests_sa_binding.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./text_template_tests_sa.h"


/******************************************************************************
 * STATIC TEST HELPERS
 *****************************************************************************/

/*
d_test_internal_dummy_template_fn
  Dummy callback for function binding tests. Returns a heap-allocated
string "dummy".
*/
static enum d_text_template_error
d_test_internal_dummy_template_fn
(
    void*  _context,
    char** _out_result
)
{
    (void)_context;

    *_out_result = d_strdup("dummy");

    if (!*_out_result)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_test_internal_dummy_list_fn
  Dummy per-item callback for list binding tests. Binds no additional
keys; simply returns true to allow iteration to proceed.
*/
static bool
d_test_internal_dummy_list_fn
(
    size_t                  _index,
    size_t                  _count,
    struct d_text_template* _item_template,
    void*                   _context
)
{
    (void)_index;
    (void)_count;
    (void)_item_template;
    (void)_context;

    return true;
}


/******************************************************************************
 * BINDING MANAGEMENT TESTS
 *****************************************************************************/

/*
d_tests_sa_text_template_bind_string
  Tests d_text_template_bind_string for adding string bindings.
  Tests the following:
  - adds a single string binding successfully
  - increments binding_count
  - stored key matches input
  - stored value matches input
  - duplicate key updates instead of duplicating
  - returns ERROR_NULL_PARAM for NULL template
  - returns ERROR_NULL_PARAM for NULL key
  - returns ERROR_NULL_PARAM for NULL value
  - returns ERROR_TOO_MANY_BINDINGS at capacity
*/
struct d_test_object*
d_tests_sa_text_template_bind_string
(
    void
)
{
    struct d_test_object*       group;
    struct d_text_template*     tmpl;
    enum d_text_template_error  err;
    bool                        test_success;
    bool                        test_count;
    bool                        test_key_match;
    bool                        test_value_match;
    bool                        test_duplicate;
    bool                        test_null_tmpl;
    bool                        test_null_key;
    bool                        test_null_val;
    bool                        test_too_many;
    size_t                      i;
    size_t                      idx;
    char                        key_buf[32];

    tmpl = d_text_template_new();

    test_success   = false;
    test_count     = false;
    test_key_match = false;
    test_value_match = false;
    test_duplicate = false;
    test_too_many  = false;

    if (tmpl)
    {
        // test 1: add succeeds
        err = d_text_template_bind_string(tmpl, "name", "Alice");
        test_success = (err == D_TEXT_TEMPLATE_SUCCESS);

        // test 2: count incremented
        test_count = (tmpl->binding_count == 1);

        // test 3: key matches
        test_key_match = (d_strequals(tmpl->bindings[0].key,
                                      tmpl->bindings[0].key_length,
                                      "name",
                                      4));

        // test 4: value matches
        test_value_match = ( (tmpl->bindings[0].type ==
                              D_TEXT_TEMPLATE_BINDING_STRING) &&
                             (d_strequals(
                                  tmpl->bindings[0].value.string.text,
                                  tmpl->bindings[0].value.string.length,
                                  "Alice",
                                  5)) );

        // test 5: duplicate key updates
        err = d_text_template_bind_string(tmpl, "name", "Bob");
        test_duplicate = ( (err == D_TEXT_TEMPLATE_SUCCESS)  &&
                           (tmpl->binding_count == 1)        &&
                           (d_strequals(
                                tmpl->bindings[0].value.string.text,
                                tmpl->bindings[0].value.string.length,
                                "Bob",
                                3)) );

        // test 9: fill to capacity, then overflow
        d_text_template_clear(tmpl);
        for (i = 0; i < D_TEXT_TEMPLATE_MAX_BINDINGS; i++)
        {
            d_memset(key_buf, 0, sizeof(key_buf));
            key_buf[0] = 'k';
            key_buf[1] = (char)('0' + (i / 10));
            key_buf[2] = (char)('0' + (i % 10));
            key_buf[3] = '\0';

            d_text_template_bind_string(tmpl, key_buf, "v");
        }

        err = d_text_template_bind_string(tmpl, "overflow", "boom");
        test_too_many = (err == D_TEXT_TEMPLATE_ERROR_TOO_MANY_BINDINGS);

        d_text_template_free(tmpl);
    }

    // test 6: NULL template
    err = d_text_template_bind_string(NULL, "k", "v");
    test_null_tmpl = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    // test 7: NULL key
    tmpl = d_text_template_new();
    test_null_key = false;
    if (tmpl)
    {
        err = d_text_template_bind_string(tmpl, NULL, "v");
        test_null_key = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    // test 8: NULL value
    tmpl = d_text_template_new();
    test_null_val = false;
    if (tmpl)
    {
        err = d_text_template_bind_string(tmpl, "k", NULL);
        test_null_val = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    // build result tree
    group = d_test_object_new_interior("d_text_template_bind_string", 9);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("success",
                                           test_success,
                                           "bind_string returns SUCCESS");
    group->elements[idx++] = D_ASSERT_TRUE("count",
                                           test_count,
                                           "binding_count incremented");
    group->elements[idx++] = D_ASSERT_TRUE("key_match",
                                           test_key_match,
                                           "stored key matches input");
    group->elements[idx++] = D_ASSERT_TRUE("value_match",
                                           test_value_match,
                                           "stored value matches input");
    group->elements[idx++] = D_ASSERT_TRUE("duplicate",
                                           test_duplicate,
                                           "duplicate key updates value");
    group->elements[idx++] = D_ASSERT_TRUE("null_tmpl",
                                           test_null_tmpl,
                                           "ERROR_NULL_PARAM for NULL template");
    group->elements[idx++] = D_ASSERT_TRUE("null_key",
                                           test_null_key,
                                           "ERROR_NULL_PARAM for NULL key");
    group->elements[idx++] = D_ASSERT_TRUE("null_val",
                                           test_null_val,
                                           "ERROR_NULL_PARAM for NULL value");
    group->elements[idx++] = D_ASSERT_TRUE("too_many",
                                           test_too_many,
                                           "ERROR_TOO_MANY_BINDINGS at capacity");

    return group;
}

/*
d_tests_sa_text_template_bind_string_n
  Tests d_text_template_bind_string_n for explicit-length binding.
  Tests the following:
  - adds with explicit lengths
  - key_length stored correctly
  - value length stored correctly
  - handles substring key via length
  - handles zero-length value
  - returns ERROR_NULL_PARAM for NULL inputs
*/
struct d_test_object*
d_tests_sa_text_template_bind_string_n
(
    void
)
{
    struct d_test_object*       group;
    struct d_text_template*     tmpl;
    enum d_text_template_error  err;
    bool                        test_success;
    bool                        test_key_len;
    bool                        test_val_len;
    bool                        test_substring;
    bool                        test_empty_val;
    bool                        test_null_param;
    size_t                      idx;

    tmpl = d_text_template_new();

    test_success   = false;
    test_key_len   = false;
    test_val_len   = false;
    test_substring = false;
    test_empty_val = false;

    if (tmpl)
    {
        // test 1: add with explicit lengths
        err = d_text_template_bind_string_n(tmpl,
                                            "fullname",
                                            8,
                                            "Alice",
                                            5);
        test_success = (err == D_TEXT_TEMPLATE_SUCCESS);

        // test 2: key_length stored
        test_key_len = (tmpl->bindings[0].key_length == 8);

        // test 3: value length stored
        test_val_len = (tmpl->bindings[0].value.string.length == 5);

        // test 4: substring key
        err = d_text_template_bind_string_n(tmpl,
                                            "username_extra",
                                            8,
                                            "Bob",
                                            3);
        test_substring = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                           (tmpl->bindings[1].key_length == 8) &&
                           (d_strequals(tmpl->bindings[1].key,
                                        8,
                                        "username",
                                        8)) );

        // test 5: zero-length value
        err = d_text_template_bind_string_n(tmpl,
                                            "empty",
                                            5,
                                            "",
                                            0);
        test_empty_val = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                           (tmpl->bindings[2].value.string.length == 0) );

        d_text_template_free(tmpl);
    }

    // test 6: NULL inputs
    err = d_text_template_bind_string_n(NULL, "a", 1, "b", 1);
    test_null_param = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    // build result tree
    group = d_test_object_new_interior("d_text_template_bind_string_n", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("success",
                                           test_success,
                                           "bind_string_n returns SUCCESS");
    group->elements[idx++] = D_ASSERT_TRUE("key_len",
                                           test_key_len,
                                           "key_length stored correctly");
    group->elements[idx++] = D_ASSERT_TRUE("val_len",
                                           test_val_len,
                                           "value length stored correctly");
    group->elements[idx++] = D_ASSERT_TRUE("substring",
                                           test_substring,
                                           "handles substring key via length");
    group->elements[idx++] = D_ASSERT_TRUE("empty_val",
                                           test_empty_val,
                                           "handles zero-length value");
    group->elements[idx++] = D_ASSERT_TRUE("null_param",
                                           test_null_param,
                                           "ERROR_NULL_PARAM for NULL inputs");

    return group;
}

/*
d_tests_sa_text_template_bind_template
  Tests d_text_template_bind_template for nested template bindings.
  Tests the following:
  - binds a nested template successfully
  - binding type is D_TEXT_TEMPLATE_BINDING_TEMPLATE
  - binding_count incremented
  - nested pointer stored correctly
  - returns ERROR_NULL_PARAM for NULL template
  - returns ERROR_NULL_PARAM for NULL key
  - returns ERROR_NULL_PARAM for NULL nested
  - duplicate key replaces binding type
*/
struct d_test_object*
d_tests_sa_text_template_bind_template
(
    void
)
{
    struct d_test_object*       group;
    struct d_text_template*     parent;
    struct d_text_template*     child;
    enum d_text_template_error  err;
    bool                        test_success;
    bool                        test_type;
    bool                        test_count;
    bool                        test_ptr;
    bool                        test_null_tmpl;
    bool                        test_null_key;
    bool                        test_null_nested;
    bool                        test_replaces;
    size_t                      idx;

    parent = d_text_template_new();
    child  = d_text_template_new();

    test_success  = false;
    test_type     = false;
    test_count    = false;
    test_ptr      = false;
    test_replaces = false;

    if (parent && child)
    {
        d_text_template_bind_string(child, "inner", "value");

        // test 1: bind succeeds
        err = d_text_template_bind_template(parent, "child", child);
        test_success = (err == D_TEXT_TEMPLATE_SUCCESS);

        // test 2: type is TEMPLATE
        test_type = (parent->bindings[0].type ==
                     D_TEXT_TEMPLATE_BINDING_TEMPLATE);

        // test 3: count incremented
        test_count = (parent->binding_count == 1);

        // test 4: pointer stored
        test_ptr = (parent->bindings[0].value.tmpl == child);

        // test 8: bind a string with the same key replaces the template
        err = d_text_template_bind_string(parent, "child", "override");
        test_replaces = ( (err == D_TEXT_TEMPLATE_SUCCESS)  &&
                          (parent->binding_count == 1)      &&
                          (parent->bindings[0].type ==
                           D_TEXT_TEMPLATE_BINDING_STRING) );
    }

    if (parent)
    {
        d_text_template_free(parent);
    }

    if (child)
    {
        d_text_template_free(child);
    }

    // test 5: NULL template
    err = d_text_template_bind_template(NULL, "k", child);
    test_null_tmpl = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    // test 6: NULL key
    parent = d_text_template_new();
    child  = d_text_template_new();
    test_null_key = false;
    if (parent && child)
    {
        err = d_text_template_bind_template(parent, NULL, child);
        test_null_key = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
    }

    if (parent)
    {
        d_text_template_free(parent);
    }

    if (child)
    {
        d_text_template_free(child);
    }

    // test 7: NULL nested
    parent = d_text_template_new();
    test_null_nested = false;
    if (parent)
    {
        err = d_text_template_bind_template(parent, "k", NULL);
        test_null_nested = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(parent);
    }

    // build result tree
    group = d_test_object_new_interior("d_text_template_bind_template", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("success",
                                           test_success,
                                           "bind_template returns SUCCESS");
    group->elements[idx++] = D_ASSERT_TRUE("type",
                                           test_type,
                                           "binding type is TEMPLATE");
    group->elements[idx++] = D_ASSERT_TRUE("count",
                                           test_count,
                                           "binding_count incremented");
    group->elements[idx++] = D_ASSERT_TRUE("ptr",
                                           test_ptr,
                                           "nested pointer stored correctly");
    group->elements[idx++] = D_ASSERT_TRUE("null_tmpl",
                                           test_null_tmpl,
                                           "ERROR_NULL_PARAM for NULL template");
    group->elements[idx++] = D_ASSERT_TRUE("null_key",
                                           test_null_key,
                                           "ERROR_NULL_PARAM for NULL key");
    group->elements[idx++] = D_ASSERT_TRUE("null_nested",
                                           test_null_nested,
                                           "ERROR_NULL_PARAM for NULL nested");
    group->elements[idx++] = D_ASSERT_TRUE("replaces",
                                           test_replaces,
                                           "string rebind replaces template type");

    return group;
}

/*
d_tests_sa_text_template_bind_function
  Tests d_text_template_bind_function for function callback bindings.
  Tests the following:
  - binds a function callback successfully
  - binding type is D_TEXT_TEMPLATE_BINDING_FUNCTION
  - binding_count incremented
  - function pointer stored correctly
  - context pointer stored correctly
  - NULL context is valid (succeeds)
  - returns ERROR_NULL_PARAM for NULL template
  - returns ERROR_NULL_PARAM for NULL key
  - returns ERROR_NULL_PARAM for NULL fn
  - duplicate key replaces binding type
*/
struct d_test_object*
d_tests_sa_text_template_bind_function
(
    void
)
{
    struct d_test_object*       group;
    struct d_text_template*     tmpl;
    enum d_text_template_error  err;
    bool                        test_success;
    bool                        test_type;
    bool                        test_count;
    bool                        test_fn_ptr;
    bool                        test_ctx_ptr;
    bool                        test_null_ctx;
    bool                        test_null_tmpl;
    bool                        test_null_key;
    bool                        test_null_fn;
    bool                        test_replaces;
    size_t                      idx;
    int                         dummy_ctx;

    tmpl = d_text_template_new();

    test_success  = false;
    test_type     = false;
    test_count    = false;
    test_fn_ptr   = false;
    test_ctx_ptr  = false;
    test_null_ctx = false;
    test_replaces = false;
    dummy_ctx     = 42;

    if (tmpl)
    {
        // test 1: bind succeeds
        err = d_text_template_bind_function(
                  tmpl,
                  "greeting",
                  d_test_internal_dummy_template_fn,
                  &dummy_ctx);
        test_success = (err == D_TEXT_TEMPLATE_SUCCESS);

        // test 2: type is FUNCTION
        test_type = (tmpl->bindings[0].type ==
                     D_TEXT_TEMPLATE_BINDING_FUNCTION);

        // test 3: count incremented
        test_count = (tmpl->binding_count == 1);

        // test 4: function pointer stored
        test_fn_ptr = (tmpl->bindings[0].value.function.fn ==
                       d_test_internal_dummy_template_fn);

        // test 5: context pointer stored
        test_ctx_ptr = (tmpl->bindings[0].value.function.context ==
                        &dummy_ctx);

        // test 6: NULL context is valid
        d_text_template_clear(tmpl);
        err = d_text_template_bind_function(
                  tmpl,
                  "no_ctx",
                  d_test_internal_dummy_template_fn,
                  NULL);
        test_null_ctx = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                          (tmpl->bindings[0].value.function.context
                           == NULL) );

        // test 10: duplicate key replaces binding type
        d_text_template_clear(tmpl);
        d_text_template_bind_string(tmpl, "shared", "text_value");
        err = d_text_template_bind_function(
                  tmpl,
                  "shared",
                  d_test_internal_dummy_template_fn,
                  NULL);
        test_replaces = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                          (tmpl->binding_count == 1)       &&
                          (tmpl->bindings[0].type ==
                           D_TEXT_TEMPLATE_BINDING_FUNCTION) );

        d_text_template_free(tmpl);
    }

    // test 7: NULL template
    err = d_text_template_bind_function(
              NULL,
              "k",
              d_test_internal_dummy_template_fn,
              NULL);
    test_null_tmpl = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    // test 8: NULL key
    tmpl = d_text_template_new();
    test_null_key = false;
    if (tmpl)
    {
        err = d_text_template_bind_function(
                  tmpl,
                  NULL,
                  d_test_internal_dummy_template_fn,
                  NULL);
        test_null_key = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    // test 9: NULL fn
    tmpl = d_text_template_new();
    test_null_fn = false;
    if (tmpl)
    {
        err = d_text_template_bind_function(tmpl,
                                            "k",
                                            NULL,
                                            NULL);
        test_null_fn = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    // build result tree
    group = d_test_object_new_interior(
                "d_text_template_bind_function",
                10);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "success",
                                 test_success,
                                 "bind_function returns SUCCESS");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "type",
                                 test_type,
                                 "binding type is FUNCTION");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "count",
                                 test_count,
                                 "binding_count incremented");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "fn_ptr",
                                 test_fn_ptr,
                                 "function pointer stored");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "ctx_ptr",
                                 test_ctx_ptr,
                                 "context pointer stored");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_ctx",
                                 test_null_ctx,
                                 "NULL context is valid");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_tmpl",
                                 test_null_tmpl,
                                 "ERROR_NULL_PARAM for NULL template");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_key",
                                 test_null_key,
                                 "ERROR_NULL_PARAM for NULL key");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_fn",
                                 test_null_fn,
                                 "ERROR_NULL_PARAM for NULL fn");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "replaces",
                                 test_replaces,
                                 "duplicate key replaces type");

    return group;
}

/*
d_tests_sa_text_template_bind_list
  Tests d_text_template_bind_list for list iteration bindings.
  Tests the following:
  - binds a list binding successfully
  - binding type is D_TEXT_TEMPLATE_BINDING_LIST
  - binding_count incremented
  - item_template pointer stored correctly
  - bind_fn pointer stored correctly
  - list count stored correctly
  - NULL options uses defaults (all zeroed)
  - options values are deep-copied correctly
  - returns ERROR_NULL_PARAM for NULL template
  - returns ERROR_NULL_PARAM for NULL key
  - returns ERROR_NULL_PARAM for NULL item_template
  - returns ERROR_NULL_PARAM for NULL bind_fn
  - duplicate key replaces binding type
*/
struct d_test_object*
d_tests_sa_text_template_bind_list
(
    void
)
{
    struct d_test_object*                group;
    struct d_text_template*              tmpl;
    struct d_text_template*              item_tmpl;
    enum d_text_template_error           err;
    struct d_text_template_list_options   opts;
    bool                                 test_success;
    bool                                 test_type;
    bool                                 test_count;
    bool                                 test_item_ptr;
    bool                                 test_bind_fn;
    bool                                 test_list_count;
    bool                                 test_null_opts;
    bool                                 test_opts_copied;
    bool                                 test_null_tmpl;
    bool                                 test_null_key;
    bool                                 test_null_item;
    bool                                 test_null_fn;
    bool                                 test_replaces;
    size_t                               idx;
    int                                  dummy_ctx;

    tmpl      = d_text_template_new();
    item_tmpl = d_text_template_new();

    test_success     = false;
    test_type        = false;
    test_count       = false;
    test_item_ptr    = false;
    test_bind_fn     = false;
    test_list_count  = false;
    test_null_opts   = false;
    test_opts_copied = false;
    test_replaces    = false;
    dummy_ctx        = 99;

    if (tmpl && item_tmpl)
    {
        // test 1: bind with NULL options succeeds
        err = d_text_template_bind_list(
                  tmpl,
                  "items",
                  item_tmpl,
                  5,
                  d_test_internal_dummy_list_fn,
                  &dummy_ctx,
                  NULL);
        test_success = (err == D_TEXT_TEMPLATE_SUCCESS);

        // test 2: type is LIST
        test_type = (tmpl->bindings[0].type ==
                     D_TEXT_TEMPLATE_BINDING_LIST);

        // test 3: count incremented
        test_count = (tmpl->binding_count == 1);

        // test 4: item_template pointer stored
        test_item_ptr =
            (tmpl->bindings[0].value.list.item_template ==
             item_tmpl);

        // test 5: bind_fn pointer stored
        test_bind_fn =
            (tmpl->bindings[0].value.list.bind_fn ==
             d_test_internal_dummy_list_fn);

        // test 6: list count stored
        test_list_count =
            (tmpl->bindings[0].value.list.count == 5);

        // test 7: NULL options -> all defaults (zeroed)
        test_null_opts =
            ( (tmpl->bindings[0].value.list.options.separator
               == NULL) &&
              (tmpl->bindings[0].value.list.options.empty_text
               == NULL) &&
              (tmpl->bindings[0].value.list.options
                   .number_pad_width == 0) &&
              (tmpl->bindings[0].value.list.options
                   .number_pad_char == 0) &&
              (tmpl->bindings[0].value.list.options
                   .number_from_zero == false) );

        // test 8: options with values are deep-copied
        d_text_template_clear(tmpl);

        d_memset(&opts, 0, sizeof(opts));
        opts.separator        = ", ";
        opts.empty_text       = "(none)";
        opts.number_pad_width = 3;
        opts.number_pad_char  = '0';
        opts.number_from_zero = true;

        err = d_text_template_bind_list(
                  tmpl,
                  "items2",
                  item_tmpl,
                  10,
                  d_test_internal_dummy_list_fn,
                  NULL,
                  &opts);

        test_opts_copied =
            ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
              (tmpl->bindings[0].value.list.options.separator
               != opts.separator) &&
              (strcmp(
                   tmpl->bindings[0].value.list.options
                       .separator,
                   ", ") == 0) &&
              (tmpl->bindings[0].value.list.options.empty_text
               != opts.empty_text) &&
              (strcmp(
                   tmpl->bindings[0].value.list.options
                       .empty_text,
                   "(none)") == 0) &&
              (tmpl->bindings[0].value.list.options
                   .number_pad_width == 3) &&
              (tmpl->bindings[0].value.list.options
                   .number_pad_char == '0') &&
              (tmpl->bindings[0].value.list.options
                   .number_from_zero == true) );

        // test 13: duplicate key replaces binding type
        d_text_template_clear(tmpl);
        d_text_template_bind_string(tmpl, "reuse", "placeholder");
        err = d_text_template_bind_list(
                  tmpl,
                  "reuse",
                  item_tmpl,
                  2,
                  d_test_internal_dummy_list_fn,
                  NULL,
                  NULL);
        test_replaces =
            ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
              (tmpl->binding_count == 1)       &&
              (tmpl->bindings[0].type ==
               D_TEXT_TEMPLATE_BINDING_LIST) );
    }

    if (tmpl)
    {
        d_text_template_free(tmpl);
    }

    if (item_tmpl)
    {
        d_text_template_free(item_tmpl);
    }

    // test 9: NULL template
    item_tmpl = d_text_template_new();
    err = d_text_template_bind_list(
              NULL,
              "k",
              item_tmpl,
              1,
              d_test_internal_dummy_list_fn,
              NULL,
              NULL);
    test_null_tmpl = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    if (item_tmpl)
    {
        d_text_template_free(item_tmpl);
    }

    // test 10: NULL key
    tmpl      = d_text_template_new();
    item_tmpl = d_text_template_new();
    test_null_key = false;
    if (tmpl && item_tmpl)
    {
        err = d_text_template_bind_list(
                  tmpl,
                  NULL,
                  item_tmpl,
                  1,
                  d_test_internal_dummy_list_fn,
                  NULL,
                  NULL);
        test_null_key = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
    }

    if (tmpl)
    {
        d_text_template_free(tmpl);
    }

    if (item_tmpl)
    {
        d_text_template_free(item_tmpl);
    }

    // test 11: NULL item_template
    tmpl = d_text_template_new();
    test_null_item = false;
    if (tmpl)
    {
        err = d_text_template_bind_list(
                  tmpl,
                  "k",
                  NULL,
                  1,
                  d_test_internal_dummy_list_fn,
                  NULL,
                  NULL);
        test_null_item =
            (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    // test 12: NULL bind_fn
    tmpl      = d_text_template_new();
    item_tmpl = d_text_template_new();
    test_null_fn = false;
    if (tmpl && item_tmpl)
    {
        err = d_text_template_bind_list(
                  tmpl,
                  "k",
                  item_tmpl,
                  1,
                  NULL,
                  NULL,
                  NULL);
        test_null_fn =
            (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
    }

    if (tmpl)
    {
        d_text_template_free(tmpl);
    }

    if (item_tmpl)
    {
        d_text_template_free(item_tmpl);
    }

    // build result tree
    group = d_test_object_new_interior(
                "d_text_template_bind_list",
                13);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "success",
                                 test_success,
                                 "bind_list returns SUCCESS");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "type",
                                 test_type,
                                 "binding type is LIST");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "count",
                                 test_count,
                                 "binding_count incremented");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "item_ptr",
                                 test_item_ptr,
                                 "item_template pointer stored");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "bind_fn",
                                 test_bind_fn,
                                 "bind_fn pointer stored");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "list_count",
                                 test_list_count,
                                 "list count stored correctly");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_opts",
                                 test_null_opts,
                                 "NULL options uses defaults");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "opts_copied",
                                 test_opts_copied,
                                 "options deep-copied correctly");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_tmpl",
                                 test_null_tmpl,
                                 "ERROR_NULL_PARAM for NULL template");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_key",
                                 test_null_key,
                                 "ERROR_NULL_PARAM for NULL key");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_item",
                                 test_null_item,
                                 "ERROR_NULL_PARAM for NULL item_template");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null_fn",
                                 test_null_fn,
                                 "ERROR_NULL_PARAM for NULL bind_fn");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "replaces",
                                 test_replaces,
                                 "duplicate key replaces type");

    return group;
}

/*
d_tests_sa_text_template_unbind
  Tests d_text_template_unbind for removing bindings.
  Tests the following:
  - removes an existing binding
  - count decremented after removal
  - removed binding no longer found
  - remaining bindings preserved
  - returns ERROR_KEY_NOT_FOUND for unknown key
  - returns ERROR_NULL_PARAM for NULL template
  - returns ERROR_NULL_PARAM for NULL key
  - removes until empty
*/
struct d_test_object*
d_tests_sa_text_template_unbind
(
    void
)
{
    struct d_test_object*       group;
    struct d_text_template*     tmpl;
    enum d_text_template_error  err;
    bool                        test_remove;
    bool                        test_count;
    bool                        test_gone;
    bool                        test_remain;
    bool                        test_not_found;
    bool                        test_null_tmpl;
    bool                        test_null_key;
    bool                        test_empty;
    size_t                      idx;

    tmpl = d_text_template_new();

    test_remove    = false;
    test_count     = false;
    test_gone      = false;
    test_remain    = false;
    test_not_found = false;
    test_empty     = false;

    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "alpha", "1");
        d_text_template_bind_string(tmpl, "beta",  "2");
        d_text_template_bind_string(tmpl, "gamma", "3");

        // test 1: remove succeeds
        err = d_text_template_unbind(tmpl, "beta");
        test_remove = (err == D_TEXT_TEMPLATE_SUCCESS);

        // test 2: count decremented
        test_count = (tmpl->binding_count == 2);

        // test 3: removed binding gone
        test_gone = (!d_text_template_has_binding(tmpl, "beta"));

        // test 4: remaining preserved
        test_remain = ( (d_text_template_has_binding(tmpl, "alpha")) &&
                        (d_text_template_has_binding(tmpl, "gamma")) );

        // test 5: unknown key
        err = d_text_template_unbind(tmpl, "nonexistent");
        test_not_found = (err == D_TEXT_TEMPLATE_ERROR_KEY_NOT_FOUND);

        // test 8: remove until empty
        d_text_template_unbind(tmpl, "alpha");
        err = d_text_template_unbind(tmpl, "gamma");
        test_empty = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                       (tmpl->binding_count == 0) );

        d_text_template_free(tmpl);
    }

    // test 6: NULL template
    err = d_text_template_unbind(NULL, "key");
    test_null_tmpl = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    // test 7: NULL key
    tmpl = d_text_template_new();
    test_null_key = false;
    if (tmpl)
    {
        err = d_text_template_unbind(tmpl, NULL);
        test_null_key = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);
        d_text_template_free(tmpl);
    }

    // build result tree
    group = d_test_object_new_interior("d_text_template_unbind", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("remove",
                                           test_remove,
                                           "removes existing binding");
    group->elements[idx++] = D_ASSERT_TRUE("count",
                                           test_count,
                                           "count decremented");
    group->elements[idx++] = D_ASSERT_TRUE("gone",
                                           test_gone,
                                           "removed binding no longer found");
    group->elements[idx++] = D_ASSERT_TRUE("remain",
                                           test_remain,
                                           "remaining bindings preserved");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "ERROR_KEY_NOT_FOUND for unknown");
    group->elements[idx++] = D_ASSERT_TRUE("null_tmpl",
                                           test_null_tmpl,
                                           "ERROR_NULL_PARAM for NULL template");
    group->elements[idx++] = D_ASSERT_TRUE("null_key",
                                           test_null_key,
                                           "ERROR_NULL_PARAM for NULL key");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "removes until empty");

    return group;
}

/*
d_tests_sa_text_template_update_string
  Tests d_text_template_update_string for updating binding values.
  Tests the following:
  - updates value of existing binding
  - value_length updated correctly
  - count unchanged after update
  - returns ERROR_KEY_NOT_FOUND for unknown key
  - returns ERROR_NULL_PARAM for NULL inputs
  - updates value to shorter string
*/
struct d_test_object*
d_tests_sa_text_template_update_string
(
    void
)
{
    struct d_test_object*       group;
    struct d_text_template*     tmpl;
    enum d_text_template_error  err;
    bool                        test_update;
    bool                        test_length;
    bool                        test_count;
    bool                        test_not_found;
    bool                        test_null_param;
    bool                        test_shorter;
    size_t                      idx;

    tmpl = d_text_template_new();

    test_update    = false;
    test_length    = false;
    test_count     = false;
    test_not_found = false;
    test_shorter   = false;

    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "color", "red");

        // test 1: update succeeds
        err = d_text_template_update_string(tmpl, "color", "blue");
        test_update = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                        (d_strequals(
                             tmpl->bindings[0].value.string.text,
                             tmpl->bindings[0].value.string.length,
                             "blue",
                             4)) );

        // test 2: value_length updated
        test_length = (tmpl->bindings[0].value.string.length == 4);

        // test 3: count unchanged
        test_count = (tmpl->binding_count == 1);

        // test 4: unknown key
        err = d_text_template_update_string(tmpl, "shape", "circle");
        test_not_found = (err == D_TEXT_TEMPLATE_ERROR_KEY_NOT_FOUND);

        // test 6: update to shorter value
        err = d_text_template_update_string(tmpl, "color", "a");
        test_shorter = ( (err == D_TEXT_TEMPLATE_SUCCESS) &&
                         (tmpl->bindings[0].value.string.length == 1) );

        d_text_template_free(tmpl);
    }

    // test 5: NULL inputs
    err = d_text_template_update_string(NULL, "k", "v");
    test_null_param = (err == D_TEXT_TEMPLATE_ERROR_NULL_PARAM);

    // build result tree
    group = d_test_object_new_interior("d_text_template_update_string", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("update",
                                           test_update,
                                           "updates value correctly");
    group->elements[idx++] = D_ASSERT_TRUE("length",
                                           test_length,
                                           "value_length updated");
    group->elements[idx++] = D_ASSERT_TRUE("count",
                                           test_count,
                                           "count unchanged after update");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "ERROR_KEY_NOT_FOUND for unknown");
    group->elements[idx++] = D_ASSERT_TRUE("null_param",
                                           test_null_param,
                                           "ERROR_NULL_PARAM for NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("shorter",
                                           test_shorter,
                                           "updates to shorter value");

    return group;
}

/*
d_tests_sa_text_template_has_binding
  Tests d_text_template_has_binding for existence checking.
  Tests the following:
  - returns true for existing binding
  - returns false for unknown key
  - returns false for NULL template
  - returns false for NULL key
  - returns false after unbind
  - returns true after re-bind
*/
struct d_test_object*
d_tests_sa_text_template_has_binding
(
    void
)
{
    struct d_test_object*   group;
    struct d_text_template* tmpl;
    bool                    test_exists;
    bool                    test_unknown;
    bool                    test_null_tmpl;
    bool                    test_null_key;
    bool                    test_after_unbind;
    bool                    test_rebind;
    size_t                  idx;

    tmpl = d_text_template_new();

    test_exists       = false;
    test_unknown      = false;
    test_after_unbind = false;
    test_rebind       = false;

    if (tmpl)
    {
        d_text_template_bind_string(tmpl, "fruit", "apple");

        // test 1: exists
        test_exists = d_text_template_has_binding(tmpl, "fruit");

        // test 2: unknown
        test_unknown = (!d_text_template_has_binding(tmpl, "veggie"));

        // test 5: after unbind
        d_text_template_unbind(tmpl, "fruit");
        test_after_unbind = (!d_text_template_has_binding(tmpl, "fruit"));

        // test 6: re-bind
        d_text_template_bind_string(tmpl, "fruit", "banana");
        test_rebind = d_text_template_has_binding(tmpl, "fruit");

        d_text_template_free(tmpl);
    }

    // test 3: NULL template
    test_null_tmpl = (!d_text_template_has_binding(NULL, "key"));

    // test 4: NULL key
    tmpl = d_text_template_new();
    test_null_key = false;
    if (tmpl)
    {
        test_null_key = (!d_text_template_has_binding(tmpl, NULL));
        d_text_template_free(tmpl);
    }

    // build result tree
    group = d_test_object_new_interior("d_text_template_has_binding", 6);

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
    group->elements[idx++] = D_ASSERT_TRUE("null_tmpl",
                                           test_null_tmpl,
                                           "returns false for NULL template");
    group->elements[idx++] = D_ASSERT_TRUE("null_key",
                                           test_null_key,
                                           "returns false for NULL key");
    group->elements[idx++] = D_ASSERT_TRUE("after_unbind",
                                           test_after_unbind,
                                           "returns false after unbind");
    group->elements[idx++] = D_ASSERT_TRUE("rebind",
                                           test_rebind,
                                           "returns true after re-bind");

    return group;
}

/*
d_tests_sa_text_template_binding_count
  Tests d_text_template_binding_count for querying binding count.
  Tests the following:
  - returns 0 for a new template
  - returns 1 after one binding added
  - returns correct count after multiple bindings
  - decrements after unbind
  - returns 0 after clear
  - returns 0 for NULL template
*/
struct d_test_object*
d_tests_sa_text_template_binding_count
(
    void
)
{
    struct d_test_object*   group;
    struct d_text_template* tmpl;
    bool                    test_zero;
    bool                    test_one;
    bool                    test_multi;
    bool                    test_unbind;
    bool                    test_clear;
    bool                    test_null;
    size_t                  idx;

    tmpl = d_text_template_new();

    test_zero    = false;
    test_one     = false;
    test_multi   = false;
    test_unbind  = false;
    test_clear   = false;

    if (tmpl)
    {
        // test 1: new template has 0 bindings
        test_zero = (d_text_template_binding_count(tmpl) == 0);

        // test 2: one binding
        d_text_template_bind_string(tmpl, "a", "1");
        test_one = (d_text_template_binding_count(tmpl) == 1);

        // test 3: multiple bindings
        d_text_template_bind_string(tmpl, "b", "2");
        d_text_template_bind_string(tmpl, "c", "3");
        test_multi = (d_text_template_binding_count(tmpl) == 3);

        // test 4: count decrements after unbind
        d_text_template_unbind(tmpl, "b");
        test_unbind = (d_text_template_binding_count(tmpl) == 2);

        // test 5: count returns 0 after clear
        d_text_template_clear(tmpl);
        test_clear = (d_text_template_binding_count(tmpl) == 0);

        d_text_template_free(tmpl);
    }

    // test 6: NULL template
    test_null = (d_text_template_binding_count(NULL) == 0);

    // build result tree
    group = d_test_object_new_interior(
                "d_text_template_binding_count",
                6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "zero",
                                 test_zero,
                                 "returns 0 for new template");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "one",
                                 test_one,
                                 "returns 1 after one binding");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "multi",
                                 test_multi,
                                 "returns correct after multiple");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "unbind",
                                 test_unbind,
                                 "decrements after unbind");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "clear",
                                 test_clear,
                                 "returns 0 after clear");
    group->elements[idx++] = D_ASSERT_TRUE(
                                 "null",
                                 test_null,
                                 "returns 0 for NULL template");

    return group;
}

/*
d_tests_sa_text_template_binding_all
  Runs all binding management tests.
  Tests the following:
  - d_text_template_bind_string
  - d_text_template_bind_string_n
  - d_text_template_bind_template
  - d_text_template_bind_function
  - d_text_template_bind_list
  - d_text_template_unbind
  - d_text_template_update_string
  - d_text_template_has_binding
  - d_text_template_binding_count
*/
struct d_test_object*
d_tests_sa_text_template_binding_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Binding Management", 9);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_text_template_bind_string();
    group->elements[idx++] = d_tests_sa_text_template_bind_string_n();
    group->elements[idx++] = d_tests_sa_text_template_bind_template();
    group->elements[idx++] = d_tests_sa_text_template_bind_function();
    group->elements[idx++] = d_tests_sa_text_template_bind_list();
    group->elements[idx++] = d_tests_sa_text_template_unbind();
    group->elements[idx++] = d_tests_sa_text_template_update_string();
    group->elements[idx++] = d_tests_sa_text_template_has_binding();
    group->elements[idx++] = d_tests_sa_text_template_binding_count();

    return group;
}
