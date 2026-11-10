/*******************************************************************************
* djinterp [test]                                              t_container_ckd.c
*
*   Regression tests for lane C's checked sizes in c/container and
* c/functional, and for the four memory faults found while making them: each
* size computed from a count is now made with d_ckd_add, d_ckd_sub or
* d_ckd_mul (c/dckdint.h) and a size that does not fit is refused, as the
* allocation failure each caller already handles. The calls below pass a
* count whose size wraps, where the API takes a count without an array of
* that length behind it, and check that the call fails and leaves its
* container as it was; then that an ordinary call still works. The four
* faults:
*   - d_array_new_slice and its kin read the source's elements pointer as
*     the elements, past the end of the dj_array that holds it;
*   - d_min_enum_map_new_copy recorded the source's capacity over an
*     allocation of its count, so a put into the spare room wrote past it
*     (and treated d_memcpy_s's 0, success, as failure);
*   - d_text_buffer_copy_to_buffer_n capped the count by _max_chars or by
*     the destination, not both, and wrote past a small destination;
*   - d_text_buffer_ensure_capacity doubled to zero and looped for ever on a
*     capacity above SIZE_MAX / 2.
*   Section "the sites" then holds one check for each checked site a
* caller's count can reach, in c/container and c/functional, and the growth
* past a first allocation of those that start from a constant, and the
* hand-written guards moved onto the kit; the sites that
* size a copy of something that already exists cannot be handed a count that
* wraps, and are listed in the handoff note instead. array_filter.c's sites
* are checked where D_CFG_CONTAINER_ARRAY_FILTER is 1, in the test and the
* library both.
*   Built with -fsanitize=address,undefined it also proves nothing is read or
* written out of bounds; run it with ASAN_OPTIONS=allocator_may_return_null=1,
* so that a size that fits but cannot be allocated fails as malloc does
* rather than stopping the run.
*   A standalone program, as the other tests not yet in DTest are
* (tests/unintegrated/README.md): it prints a line per failed check, ends
* with "N checks, M failed", and exits 0 only when nothing failed. It moves
* into DTest 2.0 with the container suites in week 2.
*
*
* path:      /tests/unintegrated/c/container/t_container_ckd.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.09
*******************************************************************************/
// std
#include <errno.h>    // ENOMEM
#include <stdarg.h>   // va_list
#include <stdio.h>    // printf, sprintf
#include <stdlib.h>   // free
#include <string.h>   // memcmp, memcpy, memset, strcmp
// djinterp
#include "../../../../inc/djinterp/c/container/array/array.h"
#include "../../../../inc/djinterp/c/container/array/array_common.h"
#include "../../../../inc/djinterp/c/container/array/array_filter.h"
#include "../../../../inc/djinterp/c/container/array/circular_array.h"
#include "../../../../inc/djinterp/c/container/array/circular_array_mtx.h"
#include "../../../../inc/djinterp/c/container/array/ptr_array.h"
#include "../../../../inc/djinterp/c/container/buffer/buffer_common.h"
#include "../../../../inc/djinterp/c/container/buffer/text_buffer.h"
#include "../../../../inc/djinterp/c/container/map/min_enum_map.h"
#include "../../../../inc/djinterp/c/container/registry/registry.h"
#include "../../../../inc/djinterp/c/container/table/table.h"
#include "../../../../inc/djinterp/c/container/tree/file/file_tree.h"
#include "../../../../inc/djinterp/c/container/vector/vector_common.h"
#include "../../../../inc/djinterp/c/functional/filter.h"
#include "../../../../inc/djinterp/c/functional/fn_builder.h"
#include "../../../../inc/djinterp/c/functional/pipeline.h"
#include "../../../../inc/djinterp/c/dstring.h"


// D_TESTS_CKD_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_CKD_CHECK(expr)                                                \
    d_tests_ckd_check(((expr) ? 1 : 0), #expr, __LINE__)

// D_TESTS_CKD_MAX
//   constant: the largest size_t.
#define D_TESTS_CKD_MAX    ((size_t)-1)

// the counts every check adds to
static unsigned long d_tests_ckd_checks = 0;
static unsigned long d_tests_ckd_failed = 0;


/**
 * @brief Counts one check, and prints it where it failed.
 *
 * @param[in] _passed      nonzero when the check held.
 * @param[in] _expression  the check, as written.
 * @param[in] _line        the line it is on.
 */
static void
d_tests_ckd_check(
    int         _passed,
    const char* _expression,
    int         _line
)
{
    ++d_tests_ckd_checks;

    if (!_passed)
    {
        ++d_tests_ckd_failed;
        printf("FAIL line %d: %s\n", _line, _expression);
    }

    return;
}

/*
d_tests_ckd_array_common
  The array core refuses a count whose bytes wrap, leaving the array as it
was: init_from_array, append_elements and insert_elements.
*/
static void
d_tests_ckd_array_common(void)
{
    unsigned short source[4] = { 1u, 2u, 3u, 4u };
    void*          data      = NULL;
    size_t         count     = 99;
    bool           made;

    // a source count whose size wraps to 2 bytes
    made = d_array_common_init_from_array(&data, &count, sizeof(source[0]),
                                          source,
                                          (D_TESTS_CKD_MAX / 2u) + 2u);
    D_TESTS_CKD_CHECK(!made);
    D_TESTS_CKD_CHECK(data == NULL);
    D_TESTS_CKD_CHECK(count == 0);

    // an ordinary one still copies
    made = d_array_common_init_from_array(&data, &count, sizeof(source[0]),
                                          source, 4u);
    D_TESTS_CKD_CHECK(made);
    D_TESTS_CKD_CHECK(count == 4u);
    D_TESTS_CKD_CHECK( (data) &&
                       (memcmp(data, source, sizeof(source)) == 0) );

    // appending or inserting so many that the total's size wraps fails and
    // leaves the four elements as they were
    {
        void* before = data;

        D_TESTS_CKD_CHECK(!d_array_common_append_elements(
                              &data, &count, sizeof(source[0]), source,
                              D_TESTS_CKD_MAX / 2u));
        D_TESTS_CKD_CHECK(!d_array_common_append_elements(
                              &data, &count, sizeof(source[0]), source,
                              D_TESTS_CKD_MAX));
        D_TESTS_CKD_CHECK(!d_array_common_insert_elements(
                              &data, &count, sizeof(source[0]), source,
                              D_TESTS_CKD_MAX / 2u, 0));
        D_TESTS_CKD_CHECK(data == before);
        D_TESTS_CKD_CHECK(count == 4u);
    }

    // an ordinary append still grows it
    D_TESTS_CKD_CHECK(d_array_common_append_elements(&data, &count,
                                                     sizeof(source[0]),
                                                     source, 2u));
    D_TESTS_CKD_CHECK(count == 6u);
    D_TESTS_CKD_CHECK( (data) &&
                       (((unsigned short*)data)[5] == 2u) );

    free(data);

    return;
}

/*
d_tests_ckd_slice
  d_array_new_slice copies the source's elements, not the dj_array that
points at them: the old code read past the dj_array, which ASan reports.
*/
static void
d_tests_ckd_slice(void)
{
    int              values[5] = { 10, 20, 30, 40, 50 };
    struct dj_array* source;
    struct dj_array* slice;

    source = d_array_new_from_arr(sizeof(int), values, 5u);
    D_TESTS_CKD_CHECK(source != NULL);

    if (!source)
    {
        return;
    }

    slice = d_array_new_slice(source, sizeof(int), 2);
    D_TESTS_CKD_CHECK(slice != NULL);

    if (slice)
    {
        D_TESTS_CKD_CHECK(slice->count == 3u);
        D_TESTS_CKD_CHECK( (slice->elements) &&
                           (((int*)slice->elements)[0] == 30) &&
                           (((int*)slice->elements)[2] == 50) );
        d_array_free(slice);
    }

    // a negative start counts from the end
    slice = d_array_new_slice(source, sizeof(int), -2);
    D_TESTS_CKD_CHECK( (slice) &&
                       (slice->count == 2u) &&
                       (((int*)slice->elements)[0] == 40) );

    if (slice)
    {
        d_array_free(slice);
    }

    d_array_free(source);

    return;
}

/*
d_tests_ckd_ptr_array
  d_ptr_array_prepend_elements refuses a count whose total wraps, leaving
the array as it was.
*/
static void
d_tests_ckd_ptr_array(void)
{
    int                  a      = 1;
    int                  b      = 2;
    const void*          two[2] = { &a, &b };
    struct dj_ptr_array* array;

    array = d_ptr_array_new(2u);
    D_TESTS_CKD_CHECK(array != NULL);

    if (!array)
    {
        return;
    }

    D_TESTS_CKD_CHECK(d_ptr_array_prepend_elements(array, two, 2u));
    D_TESTS_CKD_CHECK(array->count == 2u);
    D_TESTS_CKD_CHECK(!d_ptr_array_prepend_elements(array, two,
                                                    D_TESTS_CKD_MAX));
    D_TESTS_CKD_CHECK(!d_ptr_array_prepend_elements(array, two,
                                                    D_TESTS_CKD_MAX / 4u));
    D_TESTS_CKD_CHECK(array->count == 2u);
    D_TESTS_CKD_CHECK( (array->elements) &&
                       (array->elements[0] == &a) &&
                       (array->elements[1] == &b) );

    d_ptr_array_free(array);

    return;
}

/*
d_tests_ckd_vector
  d_vector_common_init_from_array refuses a count whose rounded capacity's
size wraps (to 0 for 2^60 elements of 16 bytes, which the old code
allocated and then copied into).
*/
static void
d_tests_ckd_vector(void)
{
    unsigned char source[32];
    void*         elements = NULL;
    size_t        count    = 99;
    size_t        capacity = 99;
    bool          made;

    memset(source, 0x5A, sizeof(source));

    made = d_vector_common_init_from_array(&elements, &count, &capacity,
                                           16u, source,
                                           (D_TESTS_CKD_MAX / 16u) + 2u);
    D_TESTS_CKD_CHECK(!made);
    D_TESTS_CKD_CHECK(count == 0);
    D_TESTS_CKD_CHECK(capacity == 0);

    // an ordinary one still copies
    made = d_vector_common_init_from_array(&elements, &count, &capacity,
                                           16u, source, 2u);
    D_TESTS_CKD_CHECK(made);
    D_TESTS_CKD_CHECK(count == 2u);
    D_TESTS_CKD_CHECK(capacity >= 2u);
    D_TESTS_CKD_CHECK( (elements) &&
                       (memcmp(elements, source, sizeof(source)) == 0) );

    free(elements);

    return;
}

/*
d_tests_ckd_buffer
  d_buffer_common_alloc refuses a capacity whose size wraps.
*/
static void
d_tests_ckd_buffer(void)
{
    void* mem;

    D_TESTS_CKD_CHECK(d_buffer_common_alloc(16u,
                                            (D_TESTS_CKD_MAX / 16u) + 2u)
                      == NULL);
    D_TESTS_CKD_CHECK(d_buffer_common_alloc(2u, D_TESTS_CKD_MAX) == NULL);

    mem = d_buffer_common_alloc(16u, 4u);
    D_TESTS_CKD_CHECK(mem != NULL);
    free(mem);

    return;
}

/*
d_tests_ckd_text_buffer
  A capacity above SIZE_MAX / 2 fails rather than looping for ever, and a
copy into a small destination stays inside it.
*/
static void
d_tests_ckd_text_buffer(void)
{
    struct dj_text_buffer* buffer;
    char                   dest[16];
    size_t                 copied;

    buffer = d_text_buffer_new_from_string("Hello, World!");
    D_TESTS_CKD_CHECK(buffer != NULL);

    if (!buffer)
    {
        return;
    }

    // the old doubling wrapped to zero here and never finished
    D_TESTS_CKD_CHECK(!d_text_buffer_ensure_capacity(buffer,
                          (D_TESTS_CKD_MAX / 2u) + 2u));
    D_TESTS_CKD_CHECK(d_text_buffer_ensure_capacity(buffer, 64u));

    // capped by _max_chars (10) and by the destination (4, less its
    // terminator): the old code took only the first and wrote 11 bytes
    memset(dest, 0x7F, sizeof(dest));
    copied = d_text_buffer_copy_to_buffer_n(buffer, dest, 4u, 10u);
    D_TESTS_CKD_CHECK(copied == 3u);
    D_TESTS_CKD_CHECK(memcmp(dest, "Hel", 4u) == 0);
    D_TESTS_CKD_CHECK(dest[4] == 0x7F);

    // each cap alone still applies
    copied = d_text_buffer_copy_to_buffer_n(buffer, dest, sizeof(dest), 5u);
    D_TESTS_CKD_CHECK( (copied == 5u) &&
                       (memcmp(dest, "Hello", 6u) == 0) );
    copied = d_text_buffer_copy_to_buffer_n(buffer, dest, 8u, 100u);
    D_TESTS_CKD_CHECK( (copied == 7u) &&
                       (dest[7] == '\0') );

    d_text_buffer_free(buffer);

    return;
}

/*
d_tests_ckd_min_enum_map
  A copy has the capacity it was given, so puts into it grow it in bounds;
seeding from an array of entry pointers keeps every entry, sorted.
*/
static void
d_tests_ckd_min_enum_map(void)
{
    struct dj_min_enum_map*  map;
    struct dj_min_enum_map*  copy;
    struct dj_enum_map_entry e1 = { 3, NULL };
    struct dj_enum_map_entry e2 = { 1, NULL };
    struct dj_enum_map_entry e3 = { 2, NULL };
    struct dj_enum_map_entry* seed[4];
    int                      v[4] = { 10, 20, 30, 40 };

    map = d_min_enum_map_new();
    D_TESTS_CKD_CHECK(map != NULL);

    if (!map)
    {
        return;
    }

    // three entries in a map whose capacity is larger
    D_TESTS_CKD_CHECK(d_min_enum_map_put(map, 1, &v[0]));
    D_TESTS_CKD_CHECK(d_min_enum_map_put(map, 2, &v[1]));
    D_TESTS_CKD_CHECK(d_min_enum_map_put(map, 3, &v[2]));
    D_TESTS_CKD_CHECK(map->capacity > map->count);

    // the old copy returned NULL here, after a successful copy
    copy = d_min_enum_map_new_copy(map);
    D_TESTS_CKD_CHECK(copy != NULL);

    if (copy)
    {
        D_TESTS_CKD_CHECK(copy->count == 3u);
        D_TESTS_CKD_CHECK(copy->capacity == copy->count);

        // with the source's capacity recorded, this put wrote past the copy
        D_TESTS_CKD_CHECK(d_min_enum_map_put(copy, 4, &v[3]));
        D_TESTS_CKD_CHECK(d_min_enum_map_get(copy, 4) == &v[3]);
        D_TESTS_CKD_CHECK(d_min_enum_map_get(copy, 1) == &v[0]);
        d_min_enum_map_free(copy);
    }

    // an empty map copies to an empty one
    d_min_enum_map_clear(map);
    copy = d_min_enum_map_new_copy(map);
    D_TESTS_CKD_CHECK( (copy) &&
                       (copy->count == 0) );

    if (copy)
    {
        d_min_enum_map_free(copy);
    }

    d_min_enum_map_free(map);

    // seeding: out of order, with a NULL slot, ends sorted and complete
    e1.value = &v[2];
    e2.value = &v[0];
    e3.value = &v[1];
    seed[0]  = &e1;
    seed[1]  = NULL;
    seed[2]  = &e2;
    seed[3]  = &e3;
    map      = d_min_enum_map_new_arr(seed, 4u);
    D_TESTS_CKD_CHECK(map != NULL);

    if (map)
    {
        D_TESTS_CKD_CHECK(map->count == 3u);
        D_TESTS_CKD_CHECK( (map->entries) &&
                           (map->entries[0].key == 1) &&
                           (map->entries[1].key == 2) &&
                           (map->entries[2].key == 3) );
        D_TESTS_CKD_CHECK(d_min_enum_map_get(map, 3) == &v[2]);
        d_min_enum_map_free(map);
    }

    return;
}

/*
d_tests_ckd_filter
  An index operation whose indices' size wraps is refused, not returned
with its count and no indices.
*/
static void
d_tests_ckd_filter(void)
{
    size_t                      indices[2] = { 0u, 1u };
    struct dj_filter_operation* op;

    op = d_filter_at_indices(indices, (D_TESTS_CKD_MAX / 4u) + 1u);
    D_TESTS_CKD_CHECK(op == NULL);

    // the operation's own memory is the caller's: free releases its parts
    if (op)
    {
        d_filter_operation_free(op);
        free(op);
    }

    op = d_filter_at_indices(indices, 2u);
    D_TESTS_CKD_CHECK( (op) &&
                       (op->params.indices) &&
                       (op->params.indices[1] == 1u) );

    // the operation's own memory is the caller's: free releases its parts
    if (op)
    {
        d_filter_operation_free(op);
        free(op);
    }

    return;
}

//==============================================================================
// THE SITES
//==============================================================================
//   One check for each checked site that a caller's count can reach: the
// count, or the element size, is one whose bytes do not fit in a size_t, the
// call fails as its allocation failure does, and what it was given is left as
// it was. D_TESTS_CKD_HALF is an element size two of which wrap, so the
// arrays passed here are short: each site refuses before it reads them.
//   The sites not here size a copy of something that already exists (a
// filter's matches, a string's length, a map's or registry's own entries), so
// no caller can hand them a count that wraps; the handoff note lists them.

// D_TESTS_CKD_HALF
//   constant: an element size of which two elements' bytes wrap.
#define D_TESTS_CKD_HALF   ((D_TESTS_CKD_MAX / 2u) + 1u)

// the short array every site below is pointed at
static unsigned char d_tests_ckd_bytes[64];

/*
d_tests_ckd_same
  A comparator that finds every two elements equal, without reading them.
*/
static int
d_tests_ckd_same(
    const void* _element1,
    const void* _element2,
    void*       _context
)
{
    (void)_element1;
    (void)_element2;
    (void)_context;

    return 0;
}

/*
d_tests_ckd_args_array
  Passes its variable arguments to d_array_common_init_from_args.
*/
static bool
d_tests_ckd_args_array(
    void**  _destination,
    size_t* _count,
    size_t  _element_size,
    size_t  _arg_count,
    ...
)
{
    va_list args;
    bool    made;

    va_start(args, _arg_count);
    made = d_array_common_init_from_args(_destination,
                                         _count,
                                         _element_size,
                                         _arg_count,
                                         args);
    va_end(args);

    return made;
}

/*
d_tests_ckd_args_vector
  Passes its variable arguments to d_vector_common_init_from_args.
*/
static bool
d_tests_ckd_args_vector(
    void**  _elements,
    size_t* _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _arg_count,
    ...
)
{
    va_list args;
    bool    made;

    va_start(args, _arg_count);
    made = d_vector_common_init_from_args(_elements,
                                          _count,
                                          _capacity,
                                          _element_size,
                                          _arg_count,
                                          args);
    va_end(args);

    return made;
}

/*
d_tests_ckd_sites_array
  array.c's slices, and array_common.c's initialisers, single-element growth
and circular shifts.
*/
static void
d_tests_ckd_sites_array(void)
{
    unsigned char* bytes = d_tests_ckd_bytes;
    const void*    from  = d_tests_ckd_bytes;
    void*          data  = NULL;
    size_t         count = 99;

    // array.c
    D_TESTS_CKD_CHECK(d_array_slice(bytes, 4u, 0, D_TESTS_CKD_HALF) == NULL);
    D_TESTS_CKD_CHECK(d_array_slice_range(bytes, 4u, 0, 3,
                                          D_TESTS_CKD_HALF) == NULL);

    // array_common.c's initialisers: each fails and leaves an empty array
    D_TESTS_CKD_CHECK(!d_tests_ckd_args_array(&data, &count,
                                              D_TESTS_CKD_HALF, 2u,
                                              bytes, bytes));
    D_TESTS_CKD_CHECK( (data == NULL) && (count == 0) );
    count = 99;
    D_TESTS_CKD_CHECK(!d_array_common_init_copy_reverse(&data, &count,
                                                        D_TESTS_CKD_HALF,
                                                        bytes, 4u, 0, 3));
    D_TESTS_CKD_CHECK(data == NULL);
    D_TESTS_CKD_CHECK(!d_array_common_init_copy_range(&data, &count,
                                                      D_TESTS_CKD_HALF,
                                                      bytes, 4u, 0, 3));
    D_TESTS_CKD_CHECK(data == NULL);
    D_TESTS_CKD_CHECK(!d_array_common_init_slice(&data, &count,
                                                 D_TESTS_CKD_HALF,
                                                 &from, 4u, 0));
    D_TESTS_CKD_CHECK(data == NULL);

    // one more element than one whose size is half the space
    data  = malloc(8u);
    count = 1u;
    D_TESTS_CKD_CHECK(!d_array_common_append_element(&data, &count,
                                                     D_TESTS_CKD_HALF,
                                                     bytes));
    D_TESTS_CKD_CHECK(!d_array_common_insert_element(&data, &count,
                                                     D_TESTS_CKD_HALF,
                                                     bytes, 0));
    D_TESTS_CKD_CHECK( (data != NULL) && (count == 1u) );
    free(data);

    // a shift by two elements needs two elements of scratch
    D_TESTS_CKD_CHECK(!d_array_common_shift_left_circular(bytes, 4u,
                                                          D_TESTS_CKD_HALF,
                                                          2u));
    D_TESTS_CKD_CHECK(!d_array_common_shift_right_circular(bytes, 4u,
                                                           D_TESTS_CKD_HALF,
                                                           2u));

    // an ordinary copy of a range still works
    data  = NULL;
    count = 0;
    D_TESTS_CKD_CHECK(d_array_common_init_copy_range(&data, &count, 1u,
                                                     bytes, 4u, 0, 3));
    D_TESTS_CKD_CHECK( (data != NULL) && (count > 0) );
    free(data);

    return;
}

#if D_CFG_CONTAINER_ARRAY_FILTER

/*
d_tests_ckd_sites_contiguous
  array_filter.c's copies: each reports NO_MEMORY. Compiled where
D_CFG_CONTAINER_ARRAY_FILTER is on, in the test and the library both.
*/
static void
d_tests_ckd_sites_contiguous(void)
{
    static const size_t indices[2] = { 0u, 1u };
    const void*         bytes      = d_tests_ckd_bytes;

    D_TESTS_CKD_CHECK(d_contiguous_filter_take_nth(bytes, 4u,
                                                   D_TESTS_CKD_HALF,
                                                   1u).status
                      == D_FILTER_RESULT_NO_MEMORY);
    D_TESTS_CKD_CHECK(d_contiguous_filter_slice(bytes, 4u,
                                                D_TESTS_CKD_HALF,
                                                0u, 4u, 1u).status
                      == D_FILTER_RESULT_NO_MEMORY);
    D_TESTS_CKD_CHECK(d_contiguous_filter_at_indices(bytes, 4u,
                                                     D_TESTS_CKD_HALF,
                                                     indices, 2u).status
                      == D_FILTER_RESULT_NO_MEMORY);
    D_TESTS_CKD_CHECK(d_contiguous_filter_distinct(bytes, 4u,
                                                   D_TESTS_CKD_HALF,
                                                   d_tests_ckd_same).status
                      == D_FILTER_RESULT_NO_MEMORY);
    D_TESTS_CKD_CHECK(d_contiguous_filter_reverse(bytes, 4u,
                                                  D_TESTS_CKD_HALF).status
                      == D_FILTER_RESULT_NO_MEMORY);

    // a step near SIZE_MAX takes the first element of the range only: its
    // count and its index once wrapped, and the copy wrote past its block
    {
        static const unsigned int          values[4] = { 5u, 6u, 7u, 8u };
        struct dj_contiguous_filter_result res;

        res = d_contiguous_filter_slice(values, 4u, sizeof(values[0]),
                                        1u, 4u, D_TESTS_CKD_MAX);
        D_TESTS_CKD_CHECK( (res.status == D_FILTER_RESULT_SUCCESS) &&
                           (res.count == 1u)                       &&
                           (((const unsigned int*)res.data)[0] == 6u) );
        free(res.data);
        free(res.source_indices);
    }

    return;
}

#endif  // D_CFG_CONTAINER_ARRAY_FILTER

/*
d_tests_ckd_sites_sized
  The sites a caller sizes directly: circular_array_mtx's constructor,
ptr_array's resizes, buffer_common's capacity, and registry's reserves and
constructor.
*/
static void
d_tests_ckd_sites_sized(void)
{
    struct dj_ptr_array* pointers;
    struct dj_registry*  registry;
    void*                block;
    size_t               capacity;

    D_TESTS_CKD_CHECK(d_circular_array_mtx_new_from_args(2u,
                                                         D_TESTS_CKD_HALF,
                                                         0u) == NULL);

    // a resize that would hold more pointers than a size_t can count bytes
    pointers = d_ptr_array_new(2u);
    D_TESTS_CKD_CHECK(pointers != NULL);

    if (pointers)
    {
        D_TESTS_CKD_CHECK(d_ptr_array_append_element(pointers,
                                                     d_tests_ckd_bytes));
        D_TESTS_CKD_CHECK(!d_ptr_array_resize_amount(
                              pointers,
                              (ssize_t)(D_TESTS_CKD_MAX >> 2)));
        D_TESTS_CKD_CHECK(!d_ptr_array_resize_factor(pointers, 1e30));
        D_TESTS_CKD_CHECK(pointers->count == 1u);
        d_ptr_array_free(pointers);
    }

    // buffer_common: a capacity that wraps, grown into or shrunk to
    block    = NULL;
    capacity = 0;
    D_TESTS_CKD_CHECK(!d_buffer_common_ensure_capacity(&block, &capacity,
                                                       D_TESTS_CKD_HALF,
                                                       2u));
    D_TESTS_CKD_CHECK( (block == NULL) && (capacity == 0) );
    block    = malloc(8u);
    capacity = 4u;
    D_TESTS_CKD_CHECK(!d_buffer_common_resize_to_fit(&block, &capacity,
                                                     D_TESTS_CKD_HALF, 2u));
    D_TESTS_CKD_CHECK( (block != NULL) && (capacity == 4u) );
    free(block);

    // registry: rows or lookup entries past what a size_t can count
    D_TESTS_CKD_CHECK(d_registry_new_with_capacity(D_TESTS_CKD_HALF, 2u)
                      == NULL);
    registry = d_registry_new(sizeof(const char*));
    D_TESTS_CKD_CHECK(registry != NULL);

    if (registry)
    {
        D_TESTS_CKD_CHECK(!d_registry_reserve(registry, D_TESTS_CKD_MAX));
        D_TESTS_CKD_CHECK(!d_registry_reserve_lookup(registry,
                                                     D_TESTS_CKD_MAX));
        D_TESTS_CKD_CHECK(d_registry_reserve(registry, 8u));
        d_registry_free(registry);
    }

    return;
}

/*
d_tests_ckd_sites_vector
  vector_common.c's initialisers and shrinks. A shrink that cannot be made
leaves the vector as it was: shrink_to_fit says so, and maybe_shrink, whose
shrink is optional, succeeds without it.
*/
static void
d_tests_ckd_sites_vector(void)
{
    unsigned char* bytes    = d_tests_ckd_bytes;
    void*          elements = NULL;
    size_t         count    = 99;
    size_t         capacity = 99;

    D_TESTS_CKD_CHECK(!d_tests_ckd_args_vector(&elements, &count, &capacity,
                                               D_TESTS_CKD_HALF, 2u,
                                               bytes, bytes));
    D_TESTS_CKD_CHECK(elements == NULL);
    D_TESTS_CKD_CHECK(!d_vector_common_init_copy(&elements, &count,
                                                 &capacity,
                                                 D_TESTS_CKD_HALF,
                                                 bytes, 4u, 4u));
    D_TESTS_CKD_CHECK(elements == NULL);
    D_TESTS_CKD_CHECK(!d_vector_common_init_fill(&elements, &count,
                                                 &capacity,
                                                 D_TESTS_CKD_HALF,
                                                 2u, bytes));
    D_TESTS_CKD_CHECK(elements == NULL);

    elements = malloc(8u);
    capacity = 4u;
    D_TESTS_CKD_CHECK(!d_vector_common_shrink_to_fit(&elements, 2u,
                                                     &capacity,
                                                     D_TESTS_CKD_HALF));
    D_TESTS_CKD_CHECK(capacity == 4u);
    capacity = 64u;
    D_TESTS_CKD_CHECK(d_vector_common_maybe_shrink(&elements, 2u,
                                                   &capacity,
                                                   D_TESTS_CKD_HALF));
    D_TESTS_CKD_CHECK( (elements != NULL) && (capacity == 64u) );
    free(elements);

    return;
}

/*
d_tests_ckd_chain_of
  A chain of the one operation _op, which it takes over and frees; NULL if
the chain cannot be made.
*/
static struct dj_filter_chain*
d_tests_ckd_chain_of(
    struct dj_filter_operation* _op
)
{
    struct dj_filter_chain* chain;

    chain = d_filter_chain_new();

    if ( (!chain) ||
         (!_op)   ||
         (!d_filter_chain_add(chain, _op)) )
    {
        d_filter_chain_free(chain);
        d_filter_operation_free(_op);
        free(_op);

        return NULL;
    }

    // the chain owns the operation's contents now; only its shell is ours
    free(_op);

    return chain;
}

/*
d_tests_ckd_chain_fails
  Whether applying _chain to the short array, as elements of
D_TESTS_CKD_HALF bytes, fails with no elements. Frees the chain.
*/
static bool
d_tests_ckd_chain_fails(
    struct dj_filter_chain* _chain
)
{
    struct dj_filter_result* result;
    bool                     failed;

    if (!_chain)
    {
        return false;
    }

    result = d_filter_apply_chain(_chain, d_tests_ckd_bytes, 4u,
                                  D_TESTS_CKD_HALF);
    failed = ( (result)                                    &&
               (result->elements == NULL)                  &&
               (result->status != D_FILTER_RESULT_SUCCESS) );

    d_filter_result_free(result);
    free(result);
    d_filter_chain_free(_chain);

    return failed;
}

/*
d_tests_ckd_sites_filter
  filter.c: the constructors' capacities, each operation's copy as a chain
applies it, and the union, intersection and difference of chains, which
fail where a sub-chain or their own copy does.
*/
static void
d_tests_ckd_sites_filter(void)
{
    static const size_t           indices[2] = { 0u, 1u };
    struct dj_filter_union*        either;
    struct dj_filter_intersection* both;
    struct dj_filter_difference*   less;
    struct dj_filter_chain*        first;
    struct dj_filter_chain*        second;
    struct dj_filter_result*       result;

    D_TESTS_CKD_CHECK(d_filter_chain_new_with_capacity(D_TESTS_CKD_MAX)
                      == NULL);
    D_TESTS_CKD_CHECK(d_filter_union_new(D_TESTS_CKD_MAX) == NULL);
    D_TESTS_CKD_CHECK(d_filter_intersection_new(D_TESTS_CKD_MAX) == NULL);

    // every operation that copies elements, one chain each
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(d_filter_take_first(2u))));
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(d_filter_take_last(2u))));
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(d_filter_take_nth(1u))));
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(d_filter_skip_first(1u))));
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(d_filter_skip_last(1u))));
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(d_filter_init())));
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(d_filter_rest())));
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(d_filter_range(0u, 3u))));
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(d_filter_slice(0u, 4u, 1u))));
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(
                              d_filter_at_indices(indices, 2u))));
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(
                              d_filter_distinct(d_tests_ckd_same))));
    D_TESTS_CKD_CHECK(d_tests_ckd_chain_fails(
                          d_tests_ckd_chain_of(d_filter_reverse())));

    // a slice's step near SIZE_MAX takes one element; its index once
    // wrapped back below the end and took a second
    {
        static const unsigned int values[4] = { 5u, 6u, 7u, 8u };
        struct dj_filter_chain*   chain;

        chain  = d_tests_ckd_chain_of(d_filter_slice(1u, 4u,
                                                     D_TESTS_CKD_MAX));
        result = (chain) ? d_filter_apply_chain(chain, values, 4u,
                                                sizeof(values[0]))
                         : NULL;
        D_TESTS_CKD_CHECK( (result) && (result->count == 1u) &&
                           (((const unsigned int*)result->elements)[0]
                            == 6u) );
        d_filter_result_free(result);
        free(result);
        d_filter_chain_free(chain);
    }

    // a union of one chain that fails fails, rather than being empty
    either = d_filter_union_new(2u);
    first  = d_tests_ckd_chain_of(d_filter_take_first(2u));
    D_TESTS_CKD_CHECK( (either) && (first) &&
                       (d_filter_union_add(either, first)) );
    result = d_filter_apply_union(either, d_tests_ckd_bytes, 4u,
                                  D_TESTS_CKD_HALF);
    D_TESTS_CKD_CHECK( (result) &&
                       (result->status == D_FILTER_RESULT_ERROR) &&
                       (result->elements == NULL) );
    d_filter_result_free(result);
    free(result);
    d_filter_union_free(either);
    d_filter_chain_free(first);

    // an intersection copies the input first, and that copy fails
    both  = d_filter_intersection_new(2u);
    first = d_tests_ckd_chain_of(d_filter_take_first(2u));
    D_TESTS_CKD_CHECK( (both) && (first) &&
                       (d_filter_intersection_add(both, first)) );
    result = d_filter_apply_intersection(both, d_tests_ckd_bytes, 4u,
                                         D_TESTS_CKD_HALF);
    D_TESTS_CKD_CHECK( (result) &&
                       (result->status == D_FILTER_RESULT_NO_MEMORY) &&
                       (result->elements == NULL) );
    d_filter_result_free(result);
    free(result);
    d_filter_intersection_free(both);
    d_filter_chain_free(first);

    // and a difference
    first  = d_tests_ckd_chain_of(d_filter_take_first(2u));
    second = d_tests_ckd_chain_of(d_filter_take_first(1u));
    less   = d_filter_difference_new(first, second);
    D_TESTS_CKD_CHECK(less != NULL);
    result = d_filter_apply_difference(less, d_tests_ckd_bytes, 4u,
                                       D_TESTS_CKD_HALF);
    D_TESTS_CKD_CHECK( (result) &&
                       (result->status == D_FILTER_RESULT_ERROR) &&
                       (result->elements == NULL) );
    d_filter_result_free(result);
    free(result);
    d_filter_difference_free(less);
    d_filter_chain_free(first);
    d_filter_chain_free(second);

    return;
}

#if D_CFG_CONTAINER_ARRAY_FILTER

/*
d_tests_ckd_sites_contiguous_sets
  array_filter.c's union, intersection and difference of chains, which fail
where one of their chains does, rather than answer without it.
*/
static void
d_tests_ckd_sites_contiguous_sets(void)
{
    struct dj_filter_union*            either;
    struct dj_filter_intersection*     both;
    struct dj_filter_difference*       less;
    struct dj_filter_chain*            failing;
    struct dj_filter_chain*            fine;
    struct dj_contiguous_filter_result result;

    // two elements of D_TESTS_CKD_HALF bytes cannot be copied
    failing = d_tests_ckd_chain_of(d_filter_take_first(2u));
    fine    = d_tests_ckd_chain_of(d_filter_take_first(0u));
    either  = d_filter_union_new(2u);
    both    = d_filter_intersection_new(2u);
    less    = d_filter_difference_new(fine, failing);

    D_TESTS_CKD_CHECK( (failing) && (fine) && (either) && (both) &&
                       (less) );

    if ( (failing) && (fine) && (either) && (both) && (less) )
    {
        D_TESTS_CKD_CHECK( (d_filter_union_add(either, fine)) &&
                           (d_filter_union_add(either, failing)) );
        result = d_contiguous_filter_apply_union(d_tests_ckd_bytes, 4u,
                                                 D_TESTS_CKD_HALF, either,
                                                 d_tests_ckd_same);
        D_TESTS_CKD_CHECK(result.status == D_FILTER_RESULT_ERROR);
        d_contiguous_filter_result_free(&result);

        D_TESTS_CKD_CHECK( (d_filter_intersection_add(both, failing)) &&
                           (d_filter_intersection_add(both, fine)) );
        result = d_contiguous_filter_apply_intersection(
                     d_tests_ckd_bytes, 4u, D_TESTS_CKD_HALF, both,
                     d_tests_ckd_same);
        D_TESTS_CKD_CHECK(result.status == D_FILTER_RESULT_ERROR);
        d_contiguous_filter_result_free(&result);

        // A minus a B that failed is not A
        result = d_contiguous_filter_apply_difference(d_tests_ckd_bytes,
                                                      4u,
                                                      D_TESTS_CKD_HALF,
                                                      less,
                                                      d_tests_ckd_same);
        D_TESTS_CKD_CHECK(result.status == D_FILTER_RESULT_ERROR);
        d_contiguous_filter_result_free(&result);
    }

    d_filter_difference_free(less);
    d_filter_intersection_free(both);
    d_filter_union_free(either);
    d_filter_chain_free(failing);
    d_filter_chain_free(fine);

    return;
}

#endif  // D_CFG_CONTAINER_ARRAY_FILTER

/*
d_tests_ckd_visit
  A file-tree visitor that counts the nodes it sees, in _context[0], and the
deepest depth, in _context[1].
*/
static void
d_tests_ckd_visit(
    struct dj_file_tree_node* _node,
    size_t                    _depth,
    void*                     _context
)
{
    size_t* seen = (size_t*)_context;

    (void)_node;
    ++seen[0];

    if (_depth > seen[1])
    {
        seen[1] = _depth;
    }

    return;
}

/*
d_tests_ckd_sites_file_tree
  file_tree.c's breadth-first queue, which now starts from its checked
doubling, and its path stack, each grown past its first 16 slots: twenty
directories under the root, and a path twenty deep.
*/
static void
d_tests_ckd_sites_file_tree(void)
{
    static const char         deep[] =
        "a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/r/s/t";
    struct dj_file_tree*      tree;
    struct dj_file_tree_node* node;
    struct dj_string*         path;
    char                      name[8];
    size_t                    seen[2] = { 0u, 0u };
    int                       i;

    tree = d_file_tree_new("root");
    D_TESTS_CKD_CHECK(tree != NULL);

    if (!tree)
    {
        return;
    }

    for (i = 0; i < 20; ++i)
    {
        sprintf(name, "d%d", i);
        D_TESTS_CKD_CHECK(d_file_tree_add_dir(tree, name) == 0);
    }

    D_TESTS_CKD_CHECK(d_file_tree_mkdir_p(tree, deep) == 0);

    // the root, its twenty directories, and the twenty of the deep path
    d_file_tree_traverse_breadth_first(tree, d_tests_ckd_visit, seen);
    D_TESTS_CKD_CHECK(seen[0] == 41u);
    D_TESTS_CKD_CHECK(seen[0] == d_file_tree_size(tree));
    D_TESTS_CKD_CHECK(seen[1] == 20u);

    node = d_file_tree_get_node(tree, deep);
    path = d_file_tree_node_path(tree, node);
    D_TESTS_CKD_CHECK( (path) &&
                       (strcmp(d_string_cstr(path), deep) == 0) );
    d_string_free(path);
    d_file_tree_free(tree);

    return;
}

/*
d_tests_ckd_keep
  A predicate that keeps every element.
*/
static bool
d_tests_ckd_keep(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return true;
}

/*
d_tests_ckd_copy
  A transformer that copies one unsigned int.
*/
static bool
d_tests_ckd_copy(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;
    memcpy(_output, _input, sizeof(unsigned int));

    return true;
}

/*
d_tests_ckd_sites_fn_builder
  fn_builder.c's arrays, grown well past their first capacity, and its
scratch, which two elements of D_TESTS_CKD_HALF bytes would wrap.
*/
static void
d_tests_ckd_sites_fn_builder(void)
{
    static const unsigned int input[4] = { 1u, 2u, 3u, 4u };
    unsigned int              output[4];
    struct dj_fn_builder*     builder;
    size_t                    written = 0;
    int                       i;
    bool                      grew    = true;

    builder = d_fn_builder_new();
    D_TESTS_CKD_CHECK(builder != NULL);

    if (!builder)
    {
        return;
    }

    for (i = 0; i < 40; ++i)
    {
        grew = grew &&
               (d_funtional_builder_where(builder, d_tests_ckd_keep)
                == builder) &&
               (d_funtional_builder_map(builder, d_tests_ckd_copy)
                == builder);
    }

    D_TESTS_CKD_CHECK(grew);

    // forty predicates and forty transformers, all kept, all copied
    memset(output, 0, sizeof(output));
    D_TESTS_CKD_CHECK(d_fn_builder_execute(builder, input, 4u,
                                           sizeof(input[0]), output,
                                           &written));
    D_TESTS_CKD_CHECK( (written == 4u) &&
                       (memcmp(output, input, sizeof(input)) == 0) );

    // the scratch for elements whose two copies' bytes wrap is refused
    D_TESTS_CKD_CHECK(!d_fn_builder_execute(builder, d_tests_ckd_bytes, 1u,
                                            D_TESTS_CKD_HALF,
                                            d_tests_ckd_bytes, &written));
    d_fn_builder_free(builder);

    return;
}

/*
d_tests_ckd_sites_guards
  The hand-written guards now on the kit: array_common's capacity and resize
arithmetic, circular_array's constructor, pipeline's copy and table's byte
count each refuse a size that does not fit, and leave the caller's result
as it was.
*/
static void
d_tests_ckd_sites_guards(void)
{
    struct dj_functional_pipeline pipe;
    struct dj_table_alloc         alloc = d_table_alloc_stdlib();
    struct dj_table               table;
    struct dj_circular_array*     ring;
    size_t                        result;
    bool                          valid;

    // a capacity past the largest power of two saturates
    D_TESTS_CKD_CHECK(d_array_common_calc_capacity(D_TESTS_CKD_MAX)
                      == D_TESTS_CKD_MAX);
    D_TESTS_CKD_CHECK(d_array_common_calc_capacity(5u) == 8u);

    result = 7u;
    valid  = d_array_common_is_valid_resize_amount(D_TESTS_CKD_MAX - 1u, 2,
                                                   &result);
    D_TESTS_CKD_CHECK( (!valid) && (result == 7u) );
    valid  = d_array_common_is_valid_resize_amount(4u, 2, &result);
    D_TESTS_CKD_CHECK( (valid) && (result == 6u) );

    D_TESTS_CKD_CHECK(d_circular_array_new(2u, D_TESTS_CKD_HALF) == NULL);
    ring = d_circular_array_new(4u, 1u);
    D_TESTS_CKD_CHECK(ring != NULL);
    d_circular_array_free(ring);

    // a copy of two elements whose bytes wrap fails with ENOMEM
    pipe = d_functional_pipeline_begin_copy(d_tests_ckd_bytes, 2u,
                                            D_TESTS_CKD_HALF);
    D_TESTS_CKD_CHECK( (pipe.error_code == ENOMEM) && (pipe.data == NULL) );
    d_functional_pipeline_free(&pipe);

    // rows, columns and cell size, each storable, whose product is not
    table = d_table_empty_of((size_t)D_INTERNAL_TABLE_EXTENT_MAX,
                             (size_t)D_INTERNAL_TABLE_EXTENT_MAX);
    D_TESTS_CKD_CHECK(d_table_reserve_rows(&table, 2u, &alloc)
                      == D_TABLE_STATUS_OVERFLOW);
    d_table_release(&table, &alloc);

    table = d_table_empty_of(2u, 8u);
    D_TESTS_CKD_CHECK(d_table_reserve_rows(&table, 4u, &alloc)
                      == D_TABLE_STATUS_OK);
    d_table_release(&table, &alloc);

    return;
}



int
main(void)
{
    d_tests_ckd_array_common();
    d_tests_ckd_slice();
    d_tests_ckd_ptr_array();
    d_tests_ckd_vector();
    d_tests_ckd_buffer();
    d_tests_ckd_text_buffer();
    d_tests_ckd_min_enum_map();
    d_tests_ckd_filter();
    d_tests_ckd_sites_array();
#if D_CFG_CONTAINER_ARRAY_FILTER
    d_tests_ckd_sites_contiguous();
#endif
    d_tests_ckd_sites_sized();
    d_tests_ckd_sites_vector();
    d_tests_ckd_sites_filter();
#if D_CFG_CONTAINER_ARRAY_FILTER
    d_tests_ckd_sites_contiguous_sets();
#endif
    d_tests_ckd_sites_file_tree();
    d_tests_ckd_sites_fn_builder();
    d_tests_ckd_sites_guards();

    printf("container ckd: %lu checks, %lu failed\n",
           d_tests_ckd_checks,
           d_tests_ckd_failed);

    return (d_tests_ckd_failed == 0) ? 0 : 1;
}
