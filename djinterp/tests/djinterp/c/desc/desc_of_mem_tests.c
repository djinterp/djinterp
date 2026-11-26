/*******************************************************************************
* djinterp [c][desc]                                         desc_of_mem_tests.c
*
*   The suite of desc's describers of memory's types (desc/of/mem_desc.h),
* in DTest's C face. First each type against its describer's record of it,
* so a change to a type that its describer has not followed fails here,
* naming the type and the field; then each describer's facts and prose,
* over a real arena and pool, against memory's own answers. Expectations
* follow the knobs a build has, so the suite runs under each.
*   The arena and the pool draw from a source of the suite's own, whose
* name and slots the suite knows. Everything here that reads memory's
* types is built with the suite's knobs, memory's units included (the
* leaf's note says why).
*
*
* path:      /tests/djinterp/c/desc/desc_of_mem_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#include "./desc_of_tests.h"  // the describers' tests' helpers

// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy, memset, strcmp, strstr
// djinterp
#include "../../../../inc/djinterp/c/desc/of/mem_desc.h"  // the describers


#if D_INTERNAL_DESC_OF_MEMORY

//==============================================================================
// THE SOURCE
//==============================================================================

/*
d_tests_of_allocate
  malloc's bytes, to the alignment malloc keeps.
*/
static void*
d_tests_of_allocate(
    void*       _context,
    dj_mem_size _bytes,
    dj_mem_size _align
)
{
    (void)_context;

    if ( (_bytes == 0u) || (_align > (dj_mem_size)D_MEM_MAX_ALIGN) )
    {
        return NULL;
    }

    return malloc((size_t)_bytes);
}

/*
d_tests_of_release
  Gives a block back to free.
*/
static void
d_tests_of_release(
    void*       _context,
    void*       _ptr,
    dj_mem_size _bytes,
    dj_mem_size _align
)
{
    (void)_context;
    (void)_bytes;
    (void)_align;
    free(_ptr);

    return;
}

// the suite's source's table, laid out as the suite's knobs say
static const struct dj_mem_source_vtable d_tests_of_vtable =
{
    d_tests_of_allocate,
    d_tests_of_release
#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)
    , NULL
#endif
#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)
    , NULL
#endif
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    , "suite_heap"
#endif
};

/*
d_tests_of_source
  The suite's source.
*/
static struct dj_mem_source
d_tests_of_source(void)
{
    struct dj_mem_source source;

    source.vtable  = &d_tests_of_vtable;
    source.context = NULL;

    return source;
}

#endif  // D_INTERNAL_DESC_OF_MEMORY


//==============================================================================
// THE TESTS
//==============================================================================

/*
d_tests_desc_of_mem_layout
  Checks the following:
  - memory's rows exist where its describers are built, and none elsewhere
  - each row holds: each described type's size, and each field read, is
    where and as wide as its describer's record says; a row that fails
    names the type and the field
  - the describers' unit was built with this program's memory knobs: its
    rows are this program's
*/
static void
d_tests_desc_of_mem_layout(
    struct dj_test_suite* _suite
)
{
    const struct dj_desc_of_layout* rows  = NULL;
    size_t                          count = d_desc_of_mem_layout_here(&rows);
    size_t                          index = count;
    int                             agree = d_desc_of_mem_agrees(&index);

    d_tests_of_layout(_suite, "memory", D_INTERNAL_DESC_OF_MEMORY, rows,
                      count, agree, index);

    return;
}

// a type, and the records of it a describer might keep
struct dj_tests_of_kept
{
    uint64_t count;
    double   weight;
};
struct dj_tests_of_kept_record   // as the type is
{
    uint64_t count;
    double   weight;
};
struct dj_tests_of_moved_record  // before the type moved a field
{
    double   weight;
    uint64_t count;
};
struct dj_tests_of_narrow_record // before the type widened a field
{
    uint64_t count;
    float    weight;
};

/*
d_tests_desc_of_mem_caught
  Checks the following:
  - a record that matches its type holds, in its size and its field
  - a field the type moved, or widened in place, fails its row
  - two builds that lay a type out alike agree; one that lays it out
    otherwise does not, at the row it differs in
*/
static void
d_tests_desc_of_mem_caught(
    struct dj_test_suite* _suite
)
{
    static const struct dj_desc_of_layout rows[] =
    {
        D_DESC_OF_LAYOUT_SIZE(dj_tests_of_kept, dj_tests_of_kept_record),
        D_DESC_OF_LAYOUT_FIELD(dj_tests_of_kept, dj_tests_of_kept_record,
                               weight),
        D_DESC_OF_LAYOUT_FIELD(dj_tests_of_kept, dj_tests_of_moved_record,
                               weight),
        D_DESC_OF_LAYOUT_FIELD(dj_tests_of_kept, dj_tests_of_narrow_record,
                               weight)
    };
    struct dj_desc_of_layout other[2];
    size_t                   index = 0u;

    (void)d_test_suite_test(_suite, "a changed type is caught");
    D_TEST_CHECK(_suite, (strcmp(rows[0].type, "dj_tests_of_kept") == 0) &&
                         (rows[0].field == NULL) &&
                         (rows[0].recorded == rows[0].actual));
    D_TEST_CHECK(_suite, (strcmp(rows[1].field, "weight") == 0) &&
                         (rows[1].recorded == rows[1].actual) &&
                         (rows[1].recorded_width == rows[1].actual_width));
    D_TEST_CHECK(_suite, rows[2].recorded != rows[2].actual);
    D_TEST_CHECK(_suite, (rows[3].recorded == rows[3].actual) &&
                         (rows[3].recorded_width != rows[3].actual_width));
    D_TEST_CHECK(_suite, (d_desc_of_holds(rows, 2u, &index) == 1) &&
                         (index == 2u) &&
                         (d_desc_of_holds(rows, 4u, &index) == 0) &&
                         (index == 2u));

    memcpy(other, rows, sizeof(other));
    D_TEST_CHECK(_suite, d_desc_of_agree(rows, 2u, other, 2u, &index) == 1);
    other[1].actual = rows[1].actual + 8u;
    D_TEST_CHECK(_suite, (d_desc_of_agree(rows, 2u, other, 2u, &index) == 0) &&
                         (index == 1u));
    D_TEST_CHECK(_suite, (d_desc_of_agree(rows, 1u, other, 2u, &index) == 0) &&
                         (index == 1u));

    return;
}

#if D_INTERNAL_DESC_OF_MEMORY

/*
d_tests_desc_of_mem_arena
  Checks the following:
  - an arena's facts are memory's own answers: used, capacity, regions
  - an arena over a caller's buffer is fixed, and says so, and not where
    its source would have sent it
  - its tally, where memory keeps one, is its contents a level down
  - its prose in each ring, empty and in use
*/
static void
d_tests_desc_of_mem_arena(
    struct dj_test_suite* _suite
)
{
    static unsigned char    storage[1024];
    struct dj_arena         arena;
    struct dj_tests_of_keep keep;
    char                    text[256];
    char                    expected[256];

    (void)d_test_suite_test(_suite, "an arena over a caller's buffer");
    if (!D_TEST_CHECK(_suite, d_arena_init_buffer(&arena, storage,
                                                  sizeof(storage)) ==
                              D_MEM_OK))
    {
        return;
    }
    // never asked for a region; the suite's, so its name is known
    arena.source = d_tests_of_source();
#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_arena), &arena, D_DESC_RING_LINE,
                         text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "an empty arena", text,
                    D_TESTS_OF_AN "empty arena");
#endif

    D_TEST_CHECK(_suite, d_arena_allocate(&arena, 100u, 8u) != NULL);
    D_TESTS_FACT(_suite, d_tests_of_facts(D_DESC_OF(dj_arena), &arena,
                                          &keep) == 0);
    D_TESTS_FACT(_suite,
                 d_tests_of_is(&keep, "used", 0u, d_arena_used(&arena)) &&
                 d_tests_of_is(&keep, "capacity", 0u,
                               d_arena_capacity(&arena)) &&
                 d_tests_of_is(&keep, "region_count", 0u,
                               d_arena_region_count(&arena)));
    D_TESTS_FACT(_suite, d_tests_of_flag(&keep, "fixed", 1u) &&
                         d_tests_of_flag(&keep, "caller_buffer", 1u) &&
                         d_tests_of_is(&keep, "max_bytes", 0u, 0u));
#if (D_INTERNAL_MEM_STATS == 1)
    D_TESTS_FACT(_suite, (keep.enters == 1u) &&
                         d_tests_of_is(&keep, "bytes_live", 1u,
                                       arena.stats.bytes_live) &&
                         d_tests_of_is(&keep, "bytes_reserved", 1u,
                                       arena.stats.bytes_reserved));
#else
    D_TESTS_FACT(_suite, keep.enters == 0u);
#endif

#if D_TESTS_OF_FULL_BUILD
    (void)snprintf(expected, sizeof(expected),
                   D_TESTS_OF_AN "arena with %llu of %llu bytes used",
                   D_TESTS_OF_N(d_arena_used(&arena)),
                   D_TESTS_OF_N(d_arena_capacity(&arena)));
    (void)d_tests_of_say(D_DESC_OF(dj_arena), &arena, D_DESC_RING_LINE,
                         text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "the line", text, expected);
    (void)snprintf(expected, sizeof(expected),
                   D_TESTS_OF_A "fixed-size arena over a caller's buffer, "
                   "with %llu of %llu bytes used. Its regions are aligned "
                   "to %llu bytes.",
                   D_TESTS_OF_N(d_arena_used(&arena)),
                   D_TESTS_OF_N(d_arena_capacity(&arena)),
                   D_TESTS_OF_N(arena.align));
    (void)d_tests_of_say(D_DESC_OF(dj_arena), &arena, D_DESC_RING_DETAILED,
                         text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "the detail", text, expected);
#else
    (void)text;
    (void)expected;
#endif

    d_arena_release(&arena);

    return;
}

#if (D_INTERNAL_ARENA_CHAIN == 1)
/*
d_tests_desc_of_mem_chain
  Checks the following:
  - an arena that grows is not fixed, counts its regions, and says how
    big its next region will be, and where its regions come from
  - its ceiling is a policy it states
*/
static void
d_tests_desc_of_mem_chain(
    struct dj_test_suite* _suite
)
{
    struct dj_arena         arena;
    struct dj_arena_config  config;
    struct dj_tests_of_keep keep;
    char                    text[320];
    char                    expected[320];

    (void)d_test_suite_test(_suite, "an arena that grows");
    config               = d_arena_config_default();
    config.source        = d_tests_of_source();
    config.initial_bytes = 1024u;
    config.max_bytes     = 65536u;
    if (!D_TEST_CHECK(_suite, d_arena_init(&arena, &config) == D_MEM_OK))
    {
        return;
    }
    D_TEST_CHECK(_suite, (d_arena_allocate(&arena, 100u, 8u) != NULL) &&
                         (d_arena_allocate(&arena, 1500u, 8u) != NULL) &&
                         (d_arena_region_count(&arena) == 2u));
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_arena), &arena,
                                           &keep) == 0) &&
                         d_tests_of_is(&keep, "region_count", 0u, 2u) &&
                         d_tests_of_is(&keep, "used", 0u,
                                       d_arena_used(&arena)) &&
                         d_tests_of_flag(&keep, "fixed", 0u) &&
                         d_tests_of_is(&keep, "max_bytes", 0u, 65536u));
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    D_TESTS_FACT(_suite, d_tests_of_text(&keep, "source", "suite_heap"));
#endif

#if D_TESTS_OF_FULL_BUILD
    (void)snprintf(expected, sizeof(expected),
                   D_TESTS_OF_AN "arena with %llu of %llu bytes used and 2 "
                   "regions. It may hold at most 65536 bytes.",
                   D_TESTS_OF_N(d_arena_used(&arena)),
                   D_TESTS_OF_N(d_arena_capacity(&arena)));
    (void)d_tests_of_say(D_DESC_OF(dj_arena), &arena, D_DESC_RING_SENTENCE,
                         text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "the sentence", text, expected);
    (void)snprintf(expected, sizeof(expected),
                   " Its next region will be %llu bytes, by ",
                   D_TESTS_OF_N(arena.next_bytes));
    (void)d_tests_of_say(D_DESC_OF(dj_arena), &arena, D_DESC_RING_DETAILED,
                         text, sizeof(text));
    D_TEST_CHECK_MSG(_suite, "the next region",
                     strstr(text, expected) != NULL, text);
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    D_TEST_CHECK_MSG(_suite, "where its regions come from",
                     strstr(text, " Its regions come from suite_heap.") !=
                     NULL, text);
#endif
#else
    (void)text;
    (void)expected;
#endif

    d_arena_release(&arena);

    return;
}
#endif  // D_INTERNAL_ARENA_CHAIN

/*
d_tests_desc_of_mem_pool
  Checks the following:
  - a pool's facts are memory's own answers: live, capacity, slot size
  - a slot released to a free list is a slot to reuse
  - its prose in each ring
*/
static void
d_tests_desc_of_mem_pool(
    struct dj_test_suite* _suite
)
{
    struct dj_pool          pool;
    struct dj_pool_config   config;
    struct dj_tests_of_keep keep;
    void*                   slots[3];
    char                    text[256];
    char                    expected[256];

    (void)d_test_suite_test(_suite, "a pool, as memory has it");
    config               = d_pool_config_default(32u, 8u);
    config.initial_slots = 8u;
    config.source        = d_tests_of_source();
#if (D_INTERNAL_POOL_FREE_LIST == 1)
    config.policy        = D_POOL_POLICY_FREE_LIST;
#endif
    if (!D_TEST_CHECK(_suite, d_pool_init(&pool, &config) == D_MEM_OK))
    {
        return;
    }
    slots[0] = d_pool_acquire(&pool);
    slots[1] = d_pool_acquire(&pool);
    slots[2] = d_pool_acquire(&pool);
    D_TEST_CHECK(_suite, (slots[0] != NULL) && (slots[2] != NULL));
#if (D_INTERNAL_POOL_FREE_LIST == 1)
    D_TEST_CHECK(_suite, d_pool_release_slot(&pool, slots[1]) == D_MEM_OK);
#endif

    D_TESTS_FACT(_suite, d_tests_of_facts(D_DESC_OF(dj_pool), &pool,
                                          &keep) == 0);
    D_TESTS_FACT(_suite,
                 d_tests_of_is(&keep, "live", 0u, d_pool_size(&pool)) &&
                 d_tests_of_is(&keep, "capacity", 0u,
                               d_pool_capacity(&pool)) &&
                 d_tests_of_is(&keep, "slot_size", 0u,
                               d_pool_slot_size(&pool)));
#if (D_INTERNAL_POOL_FREE_LIST == 1)
    D_TESTS_FACT(_suite, d_tests_of_text(&keep, "policy", "free_list") &&
                         d_tests_of_is(&keep, "live", 0u, 2u) &&
                         d_tests_of_is(&keep, "free_count", 0u, 1u));
#endif

#if D_TESTS_OF_FULL_BUILD
    (void)snprintf(expected, sizeof(expected),
                   D_TESTS_OF_A "pool with %llu of %llu slots in use",
                   D_TESTS_OF_N(d_pool_size(&pool)),
                   D_TESTS_OF_N(d_pool_capacity(&pool)));
    (void)d_tests_of_say(D_DESC_OF(dj_pool), &pool, D_DESC_RING_LINE,
                         text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "the line", text, expected);
#if (D_INTERNAL_POOL_FREE_LIST == 1)
    (void)snprintf(expected, sizeof(expected),
                   D_TESTS_OF_A "free-list pool of %llu-byte slots, with "
                   "%llu of %llu slots in use",
                   D_TESTS_OF_N(d_pool_slot_size(&pool)),
                   D_TESTS_OF_N(d_pool_size(&pool)),
                   D_TESTS_OF_N(d_pool_capacity(&pool)));
    (void)d_tests_of_say(D_DESC_OF(dj_pool), &pool, D_DESC_RING_SENTENCE,
                         text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "the sentence", text, expected);
    (void)d_tests_of_say(D_DESC_OF(dj_pool), &pool, D_DESC_RING_DETAILED,
                         text, sizeof(text));
    D_TEST_CHECK_MSG(_suite, "a slot to reuse",
                     strstr(text, ". It has 1 slot to reuse. Its slots "
                                  "are ") != NULL, text);
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    D_TEST_CHECK_MSG(_suite, "where its blocks come from",
                     strstr(text, " Its blocks come from suite_heap.") !=
                     NULL, text);
#endif
#endif
#else
    (void)text;
    (void)expected;
#endif

    d_pool_release(&pool);

    return;
}

/*
d_tests_desc_of_mem_source
  Checks the following:
  - a zeroed source is unset, and the configured default stands in for it
  - the suite's source is set, named where sources are, and does not grow
    a block in place
  - the default source is named as memory names it
*/
static void
d_tests_desc_of_mem_source(
    struct dj_test_suite* _suite
)
{
    struct dj_mem_source    source;
    struct dj_tests_of_keep keep;
    char                    text[256];

    (void)d_test_suite_test(_suite, "a memory source, set and unset");
    memset(&source, 0, sizeof(source));
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_mem_source), &source,
                                           &keep) == 0) &&
                         d_tests_of_flag(&keep, "set", 0u) &&
                         d_tests_of_text(&keep, "name", NULL));
#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_mem_source), &source,
                         D_DESC_RING_SENTENCE, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "an unset source", text,
                    D_TESTS_OF_AN "unset memory source. The configured "
                    "default stands in for it.");
#endif

    source = d_tests_of_source();
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_mem_source), &source,
                                           &keep) == 0) &&
                         d_tests_of_flag(&keep, "set", 1u) &&
                         d_tests_of_flag(&keep, "reallocates", 0u));
#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    D_TESTS_FACT(_suite, d_tests_of_text(&keep, "name", "suite_heap"));
    source = d_mem_source_default();
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_mem_source), &source,
                                           &keep) == 0) &&
                         d_tests_of_text(&keep, "name",
                                         d_mem_source_name(&source)));
    source = d_tests_of_source();
#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_mem_source), &source,
                         D_DESC_RING_DETAILED, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "a named source", text,
                    D_TESTS_OF_A "memory source named suite_heap");
#endif
#else
    D_TESTS_FACT(_suite, d_tests_of_text(&keep, "name", NULL));
#endif
    (void)text;

    return;
}

/*
d_tests_desc_of_mem_block_tally
  Checks the following:
  - a block's size, and an empty block
  - a tally's live count is acquires less releases; an idle tally is
    unused; its prose in each ring
*/
static void
d_tests_desc_of_mem_block_tally(
    struct dj_test_suite* _suite
)
{
    static unsigned char    storage[64];
    struct dj_mem_block     block;
    struct dj_mem_stats     stats;
    struct dj_tests_of_keep keep;
    char                    text[256];

    (void)d_test_suite_test(_suite, "a block and a tally");
    block.ptr  = storage;
    block.size = (dj_mem_size)sizeof(storage);
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_mem_block), &block,
                                           &keep) == 0) &&
                         d_tests_of_is(&keep, "size", 0u, 64u));
    memset(&stats, 0, sizeof(stats));
    stats.acquire_count  = 3u;
    stats.release_count  = 1u;
    stats.bytes_live     = 96u;
    stats.bytes_reserved = 1024u;
    stats.upstream_count = 1u;
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_mem_stats), &stats,
                                           &keep) == 0) &&
                         d_tests_of_is(&keep, "live_count", 0u, 2u) &&
                         d_tests_of_is(&keep, "bytes_live", 0u, 96u));

#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_mem_block), &block, D_DESC_RING_LINE,
                         text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "a block", text,
                    D_TESTS_OF_A "memory block with 64 bytes");
    block.ptr = NULL;
    (void)d_tests_of_say(D_DESC_OF(dj_mem_block), &block, D_DESC_RING_LINE,
                         text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "an empty block", text,
                    D_TESTS_OF_AN "empty memory block");
    (void)d_tests_of_say(D_DESC_OF(dj_mem_stats), &stats,
                         D_DESC_RING_SENTENCE, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "a tally", text,
                    D_TESTS_OF_AN "allocation tally with 96 live bytes in 2 "
                    "allocations. It reserved 1024 bytes in 1 upstream "
                    "request.");
    memset(&stats, 0, sizeof(stats));
    (void)d_tests_of_say(D_DESC_OF(dj_mem_stats), &stats, D_DESC_RING_LINE,
                         text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "an unused tally", text,
                    D_TESTS_OF_AN "unused allocation tally");
#else
    (void)text;
#endif

    return;
}

/*
d_tests_desc_of_mem_declare
  Checks the following:
  - each type's name is an identifier, so it declares to desc, and its
    ring may be set alone
*/
static void
d_tests_desc_of_mem_declare(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "each type declares by its name");
    D_TEST_CHECK(_suite, (d_desc_declare(D_DESC_OF(dj_arena)) == 0)      &&
                         (d_desc_declare(D_DESC_OF(dj_pool)) == 0)       &&
                         (d_desc_declare(D_DESC_OF(dj_mem_source)) == 0) &&
                         (d_desc_declare(D_DESC_OF(dj_mem_block)) == 0)  &&
                         (d_desc_declare(D_DESC_OF(dj_mem_stats)) == 0));

    return;
}

#endif  // D_INTERNAL_DESC_OF_MEMORY


/*
d_tests_desc_of_mem
  The suite's fourth section: the block of desc/of/mem_desc.h, and its
tests, in the module d_tests_desc opened, so it runs after it.
*/
void
d_tests_desc_of_mem(
    struct dj_test_suite* _suite
)
{
    // in desc's module, which d_tests_desc opened
    (void)d_test_suite_block(_suite, "desc_of_mem");

    d_tests_desc_of_mem_layout(_suite);
    d_tests_desc_of_mem_caught(_suite);
#if D_INTERNAL_DESC_OF_MEMORY
    d_tests_desc_of_mem_arena(_suite);
#if (D_INTERNAL_ARENA_CHAIN == 1)
    d_tests_desc_of_mem_chain(_suite);
#endif
    d_tests_desc_of_mem_pool(_suite);
    d_tests_desc_of_mem_source(_suite);
    d_tests_desc_of_mem_block_tally(_suite);
    d_tests_desc_of_mem_declare(_suite);
#endif

    return;
}
