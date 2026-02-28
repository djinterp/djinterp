#include "./text_lineup_tests_sa.h"


/******************************************************************************
 * SECTION I: d_text_buffer OPERATIONS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_text_buffer_new
  Tests d_text_buffer_new for correct buffer allocation.
  Tests the following:
  - returns non-NULL for a valid capacity
  - pos field is initialised to zero
  - buffer field is non-NULL (memory allocated)
  - capacity field reflects at least the requested capacity
  - returns non-NULL for capacity of 1
  - returns non-NULL for a large capacity
*/
struct d_test_object*
d_tests_sa_text_lineup_text_buffer_new
(
    void
)
{
    struct d_test_object* group;
    struct d_text_buffer* buf_normal;
    struct d_text_buffer* buf_one;
    struct d_text_buffer* buf_large;
    bool                  test_not_null;
    bool                  test_pos_zero;
    bool                  test_data_not_null;
    bool                  test_capacity;
    bool                  test_capacity_one;
    bool                  test_capacity_large;
    size_t                idx;

    // test 1: normal capacity returns non-NULL
    buf_normal       = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_not_null    = (buf_normal != NULL);

    // test 2: pos is zero on fresh allocation
    test_pos_zero = test_not_null && (buf_normal->pos == 0);

    // test 3: data pointer is valid
    test_data_not_null = test_not_null && (buf_normal->buffer != NULL);

    // test 4: capacity is at least what was requested
    test_capacity = test_not_null &&
                    (buf_normal->capacity >= D_TEST_LINEUP_BUF_CAPACITY);

    // test 5: capacity of 1
    buf_one            = d_text_buffer_new(1);
    test_capacity_one  = (buf_one != NULL) && (buf_one->capacity >= 1);

    // test 6: large capacity
    buf_large            = d_text_buffer_new(65536);
    test_capacity_large  = (buf_large != NULL) &&
                           (buf_large->capacity >= 65536);

    // cleanup
    d_text_buffer_free(buf_normal);
    d_text_buffer_free(buf_one);
    d_text_buffer_free(buf_large);

    // build result tree
    group = d_test_object_new_interior("d_text_buffer_new", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("not_null",
                                           test_not_null,
                                           "returns non-NULL for valid capacity");
    group->elements[idx++] = D_ASSERT_TRUE("pos_zero",
                                           test_pos_zero,
                                           "pos is 0 on fresh allocation");
    group->elements[idx++] = D_ASSERT_TRUE("data_not_null",
                                           test_data_not_null,
                                           "buffer pointer is non-NULL");
    group->elements[idx++] = D_ASSERT_TRUE("capacity",
                                           test_capacity,
                                           "capacity >= requested amount");
    group->elements[idx++] = D_ASSERT_TRUE("capacity_one",
                                           test_capacity_one,
                                           "handles capacity of 1");
    group->elements[idx++] = D_ASSERT_TRUE("capacity_large",
                                           test_capacity_large,
                                           "handles large capacity");

    return group;
}


/*
d_tests_sa_text_lineup_text_buffer_ensure_capacity
  Tests d_text_buffer_ensure_capacity for correct reallocation behaviour.
  Tests the following:
  - returns true when existing capacity is already sufficient
  - returns false for NULL buffer argument
  - capacity is increased when requested size exceeds current
  - buffer pointer remains valid after growth
  - content is preserved across a growth operation
  - pos is unchanged after growth
*/
struct d_test_object*
d_tests_sa_text_lineup_text_buffer_ensure_capacity
(
    void
)
{
    struct d_test_object* group;
    struct d_text_buffer* buf;
    bool                  result;
    size_t                old_cap;
    bool                  test_sufficient;
    bool                  test_null;
    bool                  test_grows;
    bool                  test_ptr_valid;
    bool                  test_content;
    bool                  test_pos_unchanged;
    size_t                idx;

    // test 1: capacity already sufficient → true, no realloc needed
    buf            = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_sufficient = false;

    if (buf)
    {
        result          = d_text_buffer_ensure_capacity(buf,
                                                        D_TEST_LINEUP_BUF_CAPACITY / 2);
        test_sufficient = (result == true) &&
                          (buf->capacity >= D_TEST_LINEUP_BUF_CAPACITY);
        d_text_buffer_free(buf);
    }

    // test 2: NULL buffer → false
    test_null = (d_text_buffer_ensure_capacity(NULL, 64) == false);

    // test 3-6: growth tests — start with a very small buffer
    buf = d_text_buffer_new(D_TEST_LINEUP_SMALL_CAPACITY);

    test_grows        = false;
    test_ptr_valid    = false;
    test_content      = false;
    test_pos_unchanged = false;

    if (buf)
    {
        // seed some content so we can verify preservation
        d_text_buffer_append(buf, "Hi");
        old_cap = buf->capacity;

        result = d_text_buffer_ensure_capacity(buf, old_cap * 16);

        // test 3: capacity was increased
        test_grows = (result == true) && (buf->capacity >= old_cap * 16);

        // test 4: buffer pointer is valid after growth
        test_ptr_valid = (buf->buffer != NULL);

        // test 5: content ("Hi") is preserved
        test_content =
            d_strequals(buf->buffer,
                        d_strnlen(buf->buffer, D_TEST_LINEUP_STRNLEN_MAX),
                        "Hi",
                        2);

        // test 6: pos is unchanged
        test_pos_unchanged = (buf->pos == 2);

        d_text_buffer_free(buf);
    }

    // build result tree
    group = d_test_object_new_interior("d_text_buffer_ensure_capacity", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("sufficient",
                                           test_sufficient,
                                           "returns true when capacity sufficient");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "returns false for NULL buffer");
    group->elements[idx++] = D_ASSERT_TRUE("grows",
                                           test_grows,
                                           "capacity grows when needed");
    group->elements[idx++] = D_ASSERT_TRUE("ptr_valid",
                                           test_ptr_valid,
                                           "buffer pointer valid after growth");
    group->elements[idx++] = D_ASSERT_TRUE("content",
                                           test_content,
                                           "content preserved across growth");
    group->elements[idx++] = D_ASSERT_TRUE("pos_unchanged",
                                           test_pos_unchanged,
                                           "pos is unchanged after growth");

    return group;
}


/*
d_tests_sa_text_lineup_text_buffer_append
  Tests d_text_buffer_append for correct text accumulation.
  Tests the following:
  - appends to an empty buffer and returns byte count
  - pos advances by the number of bytes appended
  - content is null-terminated after append
  - successive appends concatenate correctly
  - returns (size_t)-1 for NULL buffer
  - returns (size_t)-1 for NULL text
  - empty string append returns 0 and leaves pos unchanged
  - auto-grows the buffer when capacity is exceeded
*/
struct d_test_object*
d_tests_sa_text_lineup_text_buffer_append
(
    void
)
{
    struct d_test_object* group;
    struct d_text_buffer* buf;
    size_t                ret;
    bool                  test_return_value;
    bool                  test_pos_advance;
    bool                  test_null_terminated;
    bool                  test_concatenate;
    bool                  test_null_buf;
    bool                  test_null_text;
    bool                  test_empty_string;
    bool                  test_auto_grow;
    size_t                idx;

    // test 1 & 2: return value and pos advancement on empty buffer
    buf              = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_return_value = false;
    test_pos_advance  = false;

    if (buf)
    {
        ret               = d_text_buffer_append(buf, D_TEST_LINEUP_TOKEN_SHORT);
        test_return_value = (ret == 3);                 // "abc" = 3 bytes
        test_pos_advance  = (buf->pos == 3);
        d_text_buffer_free(buf);
    }

    // test 3: null-terminated after append
    buf                 = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_null_terminated = false;

    if (buf)
    {
        d_text_buffer_append(buf, D_TEST_LINEUP_TOKEN_SHORT);
        test_null_terminated = (buf->buffer[buf->pos] == '\0');
        d_text_buffer_free(buf);
    }

    // test 4: successive appends concatenate
    buf              = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_concatenate  = false;

    if (buf)
    {
        d_text_buffer_append(buf, "foo");
        d_text_buffer_append(buf, "bar");
        test_concatenate =
            d_strequals(buf->buffer,
                        buf->pos,
                        "foobar",
                        6);
        d_text_buffer_free(buf);
    }

    // test 5: NULL buffer → (size_t)-1
    test_null_buf = (d_text_buffer_append(NULL, "test") == (size_t)-1);

    // test 6: NULL text → (size_t)-1
    buf           = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_null_text = false;

    if (buf)
    {
        test_null_text = (d_text_buffer_append(buf, NULL) == (size_t)-1);
        d_text_buffer_free(buf);
    }

    // test 7: empty string → 0 bytes, pos unchanged
    buf              = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_empty_string = false;

    if (buf)
    {
        d_text_buffer_append(buf, "seed");
        ret               = d_text_buffer_append(buf, "");
        test_empty_string = (ret == 0) && (buf->pos == 4);
        d_text_buffer_free(buf);
    }

    // test 8: auto-grow — start tiny and append beyond initial capacity
    buf            = d_text_buffer_new(D_TEST_LINEUP_SMALL_CAPACITY);
    test_auto_grow  = false;

    if (buf)
    {
        ret = d_text_buffer_append(buf,
                                   "This string is longer than 8 bytes");
        test_auto_grow = (ret != (size_t)-1) && (buf->pos > 8);
        d_text_buffer_free(buf);
    }

    // build result tree
    group = d_test_object_new_interior("d_text_buffer_append", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("return_value",
                                           test_return_value,
                                           "returns byte count of appended text");
    group->elements[idx++] = D_ASSERT_TRUE("pos_advance",
                                           test_pos_advance,
                                           "pos advances by bytes appended");
    group->elements[idx++] = D_ASSERT_TRUE("null_terminated",
                                           test_null_terminated,
                                           "buffer is null-terminated after append");
    group->elements[idx++] = D_ASSERT_TRUE("concatenate",
                                           test_concatenate,
                                           "successive appends concatenate");
    group->elements[idx++] = D_ASSERT_TRUE("null_buf",
                                           test_null_buf,
                                           "returns (size_t)-1 for NULL buffer");
    group->elements[idx++] = D_ASSERT_TRUE("null_text",
                                           test_null_text,
                                           "returns (size_t)-1 for NULL text");
    group->elements[idx++] = D_ASSERT_TRUE("empty_string",
                                           test_empty_string,
                                           "empty string returns 0, pos unchanged");
    group->elements[idx++] = D_ASSERT_TRUE("auto_grow",
                                           test_auto_grow,
                                           "auto-grows when capacity exceeded");

    return group;
}


/*
d_tests_sa_text_lineup_text_buffer_append_s
  Tests d_text_buffer_append_s for length-bounded text accumulation.
  Tests the following:
  - appends exactly _length bytes and returns that count
  - can append a partial string (first n chars only)
  - pos advances by _length
  - returns 0 for NULL buffer
  - returns 0 for NULL text pointer
  - length 0 is a safe no-op (returns 0, pos unchanged)
  - content is null-terminated after append
  - auto-grows the buffer when capacity is exceeded
*/
struct d_test_object*
d_tests_sa_text_lineup_text_buffer_append_s
(
    void
)
{
    struct d_test_object* group;
    struct d_text_buffer* buf;
    size_t                ret;
    bool                  test_return_value;
    bool                  test_partial;
    bool                  test_pos_advance;
    bool                  test_null_buf;
    bool                  test_null_text;
    bool                  test_zero_length;
    bool                  test_null_terminated;
    bool                  test_auto_grow;
    size_t                idx;

    // test 1: return value equals _length
    buf              = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_return_value = false;

    if (buf)
    {
        ret               = d_text_buffer_append_s(buf, "Hello", 5);
        test_return_value = (ret == 5);
        d_text_buffer_free(buf);
    }

    // test 2: partial append (only first 3 chars of "abcdefgh")
    buf           = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_partial   = false;

    if (buf)
    {
        ret          = d_text_buffer_append_s(buf,
                                             D_TEST_LINEUP_TOKEN_LONG,
                                             3);
        test_partial = (ret == 3) &&
                       d_strequals(buf->buffer, 3, "abc", 3);
        d_text_buffer_free(buf);
    }

    // test 3: pos advances by exactly _length
    buf              = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_pos_advance  = false;

    if (buf)
    {
        d_text_buffer_append_s(buf, "World", 5);
        test_pos_advance = (buf->pos == 5);
        d_text_buffer_free(buf);
    }

    // test 4: NULL buffer → 0
    test_null_buf = (d_text_buffer_append_s(NULL, "test", 4) == 0);

    // test 5: NULL text → 0
    buf           = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_null_text = false;

    if (buf)
    {
        test_null_text = (d_text_buffer_append_s(buf, NULL, 4) == 0);
        d_text_buffer_free(buf);
    }

    // test 6: length 0 is a no-op
    buf              = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_zero_length  = false;

    if (buf)
    {
        d_text_buffer_append_s(buf, "seed", 4);
        ret              = d_text_buffer_append_s(buf, "ignored", 0);
        test_zero_length = (ret == 0) && (buf->pos == 4);
        d_text_buffer_free(buf);
    }

    // test 7: null-terminated after append
    buf                 = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_null_terminated = false;

    if (buf)
    {
        d_text_buffer_append_s(buf, "Hello", 5);
        test_null_terminated = (buf->buffer[buf->pos] == '\0');
        d_text_buffer_free(buf);
    }

    // test 8: auto-grow
    buf            = d_text_buffer_new(D_TEST_LINEUP_SMALL_CAPACITY);
    test_auto_grow  = false;

    if (buf)
    {
        ret = d_text_buffer_append_s(buf,
                                     "This is definitely longer than eight",
                                     36);
        test_auto_grow = (ret == 36) && (buf->pos == 36);
        d_text_buffer_free(buf);
    }

    // build result tree
    group = d_test_object_new_interior("d_text_buffer_append_s", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("return_value",
                                           test_return_value,
                                           "returns _length on success");
    group->elements[idx++] = D_ASSERT_TRUE("partial",
                                           test_partial,
                                           "appends only n characters");
    group->elements[idx++] = D_ASSERT_TRUE("pos_advance",
                                           test_pos_advance,
                                           "pos advances by _length");
    group->elements[idx++] = D_ASSERT_TRUE("null_buf",
                                           test_null_buf,
                                           "returns 0 for NULL buffer");
    group->elements[idx++] = D_ASSERT_TRUE("null_text",
                                           test_null_text,
                                           "returns 0 for NULL text");
    group->elements[idx++] = D_ASSERT_TRUE("zero_length",
                                           test_zero_length,
                                           "length 0 is a safe no-op");
    group->elements[idx++] = D_ASSERT_TRUE("null_terminated",
                                           test_null_terminated,
                                           "buffer null-terminated after append");
    group->elements[idx++] = D_ASSERT_TRUE("auto_grow",
                                           test_auto_grow,
                                           "auto-grows when capacity exceeded");

    return group;
}


/*
d_tests_sa_text_lineup_text_buffer_prepend
  Tests d_text_buffer_prepend for correct front-insertion.
  Tests the following:
  - prepend to empty buffer writes the string at pos 0
  - existing content is shifted right after prepend
  - full concatenated result is correct after prepend
  - return value equals byte count of prepended text
  - pos advances by the number of bytes prepended
  - returns 0 or (size_t)-1 for NULL buffer
  - returns 0 or (size_t)-1 for NULL text
  - empty string prepend is a safe no-op
*/
struct d_test_object*
d_tests_sa_text_lineup_text_buffer_prepend
(
    void
)
{
    struct d_test_object* group;
    struct d_text_buffer* buf;
    size_t                ret;
    bool                  test_prepend_empty;
    bool                  test_shift;
    bool                  test_result;
    bool                  test_return_value;
    bool                  test_pos_advance;
    bool                  test_null_buf;
    bool                  test_null_text;
    bool                  test_empty_prepend;
    size_t                idx;

    // test 1: prepend to empty buffer
    buf               = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_prepend_empty = false;

    if (buf)
    {
        d_text_buffer_prepend(buf, "hello");
        test_prepend_empty =
            d_strequals(buf->buffer,
                        buf->pos,
                        "hello",
                        5);
        d_text_buffer_free(buf);
    }

    // test 2 & 3: existing content is shifted; combined result is correct
    buf         = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_shift  = false;
    test_result = false;

    if (buf)
    {
        d_text_buffer_append(buf, "world");
        d_text_buffer_prepend(buf, "hello ");

        // "world" should now follow "hello "
        test_shift  = d_strcontains(buf->buffer, buf->pos, "world");
        test_result =
            d_strequals(buf->buffer,
                        buf->pos,
                        "hello world",
                        11);
        d_text_buffer_free(buf);
    }

    // test 4: return value equals prepended byte count
    buf               = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_return_value  = false;

    if (buf)
    {
        ret               = d_text_buffer_prepend(buf, "pre");
        test_return_value = (ret == 3);
        d_text_buffer_free(buf);
    }

    // test 5: pos advances by bytes prepended
    buf              = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_pos_advance  = false;

    if (buf)
    {
        d_text_buffer_append(buf, "base");
        d_text_buffer_prepend(buf, "12");
        test_pos_advance = (buf->pos == 6);
        d_text_buffer_free(buf);
    }

    // test 6: NULL buffer → failure (0 or (size_t)-1)
    ret          = d_text_buffer_prepend(NULL, "test");
    test_null_buf = (ret == 0) || (ret == (size_t)-1);

    // test 7: NULL text → failure
    buf           = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_null_text = false;

    if (buf)
    {
        ret            = d_text_buffer_prepend(buf, NULL);
        test_null_text = (ret == 0) || (ret == (size_t)-1);
        d_text_buffer_free(buf);
    }

    // test 8: empty string prepend is a no-op
    buf               = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_empty_prepend = false;

    if (buf)
    {
        d_text_buffer_append(buf, "data");
        ret               = d_text_buffer_prepend(buf, "");
        test_empty_prepend = (ret == 0) &&
                             (buf->pos == 4) &&
                             d_strequals(buf->buffer, buf->pos, "data", 4);
        d_text_buffer_free(buf);
    }

    // build result tree
    group = d_test_object_new_interior("d_text_buffer_prepend", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("prepend_empty",
                                           test_prepend_empty,
                                           "prepend to empty buffer");
    group->elements[idx++] = D_ASSERT_TRUE("shift",
                                           test_shift,
                                           "existing content is shifted right");
    group->elements[idx++] = D_ASSERT_TRUE("result",
                                           test_result,
                                           "combined result is correct");
    group->elements[idx++] = D_ASSERT_TRUE("return_value",
                                           test_return_value,
                                           "returns prepended byte count");
    group->elements[idx++] = D_ASSERT_TRUE("pos_advance",
                                           test_pos_advance,
                                           "pos advances by bytes prepended");
    group->elements[idx++] = D_ASSERT_TRUE("null_buf",
                                           test_null_buf,
                                           "safe for NULL buffer");
    group->elements[idx++] = D_ASSERT_TRUE("null_text",
                                           test_null_text,
                                           "safe for NULL text");
    group->elements[idx++] = D_ASSERT_TRUE("empty_prepend",
                                           test_empty_prepend,
                                           "empty string is a no-op");

    return group;
}


/*
d_tests_sa_text_lineup_text_buffer_write_over
  Tests d_text_buffer_write_over for correct in-place overwrite.
  Tests the following:
  - overwrites a mid-string range with spaces
  - returns the number of characters overwritten (end - start)
  - positions before _start are unchanged
  - positions at or after _end are unchanged
  - returns 0 for NULL buffer
  - returns 0 when start == end (zero-length range)
  - handles a range that starts at position 0
  - handles a range that ends at the last written byte
*/
struct d_test_object*
d_tests_sa_text_lineup_text_buffer_write_over
(
    void
)
{
    struct d_test_object* group;
    struct d_text_buffer* buf;
    size_t                ret;
    bool                  test_spaces;
    bool                  test_return_value;
    bool                  test_prefix_unchanged;
    bool                  test_suffix_unchanged;
    bool                  test_null_buf;
    bool                  test_zero_range;
    bool                  test_at_start;
    bool                  test_at_end;
    size_t                idx;
    size_t                i;
    bool                  all_spaces;

    // test 1 & 2: overwrites with spaces, correct return value
    // seed buffer with "Hello World"
    buf               = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_spaces        = false;
    test_return_value  = false;

    if (buf)
    {
        d_text_buffer_append(buf, "Hello World");

        // overwrite positions 5-7 ("Wo" → "  "), i.e. [5, 8)
        ret = d_text_buffer_write_over(buf, 5, 8);

        // return value should be 3 (end - start)
        test_return_value = (ret == 3);

        // those positions should now be spaces
        all_spaces = true;

        for (i = 5; i < 8; i++)
        {
            if (buf->buffer[i] != ' ')
            {
                all_spaces = false;
            }
        }

        test_spaces = all_spaces;
        d_text_buffer_free(buf);
    }

    // test 3: prefix (positions before start) is unchanged
    buf                  = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_prefix_unchanged = false;

    if (buf)
    {
        d_text_buffer_append(buf, "Hello World");
        d_text_buffer_write_over(buf, 5, 8);

        // "Hello" (indices 0-4) must still be "Hello"
        test_prefix_unchanged = (buf->buffer[0] == 'H') &&
                                 (buf->buffer[1] == 'e') &&
                                 (buf->buffer[2] == 'l') &&
                                 (buf->buffer[3] == 'l') &&
                                 (buf->buffer[4] == 'o');
        d_text_buffer_free(buf);
    }

    // test 4: suffix (positions at/after end) is unchanged
    buf                  = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_suffix_unchanged = false;

    if (buf)
    {
        d_text_buffer_append(buf, "Hello World");
        d_text_buffer_write_over(buf, 5, 8);

        // "rld" (indices 8-10) must still be "rld"
        test_suffix_unchanged = (buf->buffer[8]  == 'r') &&
                                 (buf->buffer[9]  == 'l') &&
                                 (buf->buffer[10] == 'd');
        d_text_buffer_free(buf);
    }

    // test 5: NULL buffer → 0
    test_null_buf = (d_text_buffer_write_over(NULL, 0, 4) == 0);

    // test 6: zero-length range (start == end) → 0
    buf            = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_zero_range = false;

    if (buf)
    {
        d_text_buffer_append(buf, "data");
        ret            = d_text_buffer_write_over(buf, 2, 2);
        test_zero_range = (ret == 0);
        d_text_buffer_free(buf);
    }

    // test 7: range starting at index 0
    buf          = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_at_start = false;

    if (buf)
    {
        d_text_buffer_append(buf, "abcdef");
        ret = d_text_buffer_write_over(buf, 0, 3);

        test_at_start = (ret == 3) &&
                        (buf->buffer[0] == ' ') &&
                        (buf->buffer[1] == ' ') &&
                        (buf->buffer[2] == ' ') &&
                        (buf->buffer[3] == 'd');
        d_text_buffer_free(buf);
    }

    // test 8: range ending at last written byte
    buf        = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    test_at_end = false;

    if (buf)
    {
        d_text_buffer_append(buf, "abcdef");

        // overwrite positions 3-5 ("def")
        ret = d_text_buffer_write_over(buf, 3, 6);

        test_at_end = (ret == 3) &&
                      (buf->buffer[3] == ' ') &&
                      (buf->buffer[4] == ' ') &&
                      (buf->buffer[5] == ' ');
        d_text_buffer_free(buf);
    }

    // build result tree
    group = d_test_object_new_interior("d_text_buffer_write_over", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("spaces",
                                           test_spaces,
                                           "range is overwritten with spaces");
    group->elements[idx++] = D_ASSERT_TRUE("return_value",
                                           test_return_value,
                                           "returns count of overwritten chars");
    group->elements[idx++] = D_ASSERT_TRUE("prefix_unchanged",
                                           test_prefix_unchanged,
                                           "prefix before range is unchanged");
    group->elements[idx++] = D_ASSERT_TRUE("suffix_unchanged",
                                           test_suffix_unchanged,
                                           "suffix at/after range is unchanged");
    group->elements[idx++] = D_ASSERT_TRUE("null_buf",
                                           test_null_buf,
                                           "returns 0 for NULL buffer");
    group->elements[idx++] = D_ASSERT_TRUE("zero_range",
                                           test_zero_range,
                                           "zero-length range returns 0");
    group->elements[idx++] = D_ASSERT_TRUE("at_start",
                                           test_at_start,
                                           "handles range starting at 0");
    group->elements[idx++] = D_ASSERT_TRUE("at_end",
                                           test_at_end,
                                           "handles range ending at last byte");

    return group;
}


/*
d_tests_sa_text_lineup_text_buffer_all
  Runs all d_text_buffer tests (Section I).
  Tests the following:
  - d_text_buffer_new
  - d_text_buffer_ensure_capacity
  - d_text_buffer_append
  - d_text_buffer_append_s
  - d_text_buffer_prepend
  - d_text_buffer_write_over
*/
struct d_test_object*
d_tests_sa_text_lineup_text_buffer_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("d_text_buffer Operations", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_text_lineup_text_buffer_new();
    group->elements[idx++] =
        d_tests_sa_text_lineup_text_buffer_ensure_capacity();
    group->elements[idx++] = d_tests_sa_text_lineup_text_buffer_append();
    group->elements[idx++] = d_tests_sa_text_lineup_text_buffer_append_s();
    group->elements[idx++] = d_tests_sa_text_lineup_text_buffer_prepend();
    group->elements[idx++] = d_tests_sa_text_lineup_text_buffer_write_over();

    return group;
}
