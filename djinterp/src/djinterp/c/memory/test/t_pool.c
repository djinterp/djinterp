/*******************************************************************************
* djinterp [c]                                                          t_pool.c
*
*   Pool conformance harness. Covers all three release policies, generational
* handles, slot geometry, iteration and the block-table addressing, and runs
* the whole suite over a buffer-backed arena where no heap exists.
*
*   Build (from the repo root):
*     cc  src/djinterp/c/memory/mem_common.c \
*         src/djinterp/c/memory/mem_source.c \
*         src/djinterp/c/memory/arena.c \
*         src/djinterp/c/memory/pool.c \
*         src/djinterp/c/memory/test/t_pool.c -o t_pool
*   No -I is required: every include is repo-root relative.
*
*
* path:      /src/djinterp/c/memory/test/t_pool.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*   Pool conformance harness.
 *   Like the arena harness, this runs against ANY configuration: a policy the
 * build did not compile is skipped rather than failed, and a build with no
 * heap runs the whole thing over a buffer-backed arena instead.
 */
#include <stdio.h>
#include <string.h>
#include "../../../../../inc/djinterp/c/memory/pool.h"
#include "../../../../../inc/djinterp/c/memory/arena.h"

static int fails   = 0;
static int skipped = 0;

#define CHECK(c)                                                              \
    do                                                                        \
    {                                                                         \
        if (!(c))                                                             \
        {                                                                     \
            printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #c);             \
            ++fails;                                                          \
        }                                                                     \
    } while (0)

/* a payload with a strict alignment and a recognisable shape */
struct node
{
    double   value;
    void*    link;
    unsigned tag;
};


/* exercise_basic
 *   The behaviour every policy shares: slots are distinct, aligned, writable,
 * and do not overlap.
 */
static void
exercise_basic(
    struct d_pool* _p,
    int            _count
)
{
    void* slots[128];
    int   i;
    int   j;

    if (_count > 128)
    {
        _count = 128;
    }

    for (i = 0; i < _count; ++i)
    {
        slots[i] = d_pool_acquire(_p);
        CHECK(slots[i] != NULL);

        if (slots[i])
        {
            CHECK(d_mem_is_aligned(slots[i], d_pool_slot_align(_p)));
            memset(slots[i], (unsigned char)(i + 1), sizeof(struct node));
        }
    }

    CHECK(d_pool_size(_p) == (d_pool_index)_count);

    /* every slot is distinct, and every write survived every other write */
    for (i = 0; i < _count; ++i)
    {
        if (!slots[i])
        {
            continue;
        }

        CHECK(((unsigned char*)slots[i])[0] == (unsigned char)(i + 1));
        CHECK(((unsigned char*)slots[i])[sizeof(struct node) - 1] ==
              (unsigned char)(i + 1));

        for (j = i + 1; j < _count; ++j)
        {
            CHECK(slots[i] != slots[j]);
        }
    }

#if (D_INTERNAL_POOL_OWNS == 1)
    for (i = 0; i < _count; ++i)
    {
        if (slots[i])
        {
            CHECK(d_pool_owns(_p, slots[i]));
        }
    }

    {
        char elsewhere[16];

        CHECK(!d_pool_owns(_p, elsewhere));

        /* a pointer partway into a slot names no slot */
        if ((slots[0]) && (d_pool_slot_size(_p) > 1))
        {
            CHECK(!d_pool_owns(_p, (char*)slots[0] + 1));
        }
    }
#endif

    return;
}


static void
test_monotonic(void)
{
    struct d_pool        p;
    struct d_pool_config cfg;
    void*                s;

    cfg = d_pool_config_default(sizeof(struct node), 16);
    cfg.policy         = D_POOL_POLICY_MONOTONIC;
    cfg.slots_per_block = 8;

    CHECK(d_pool_init(&p, &cfg) == D_MEM_OK);
    CHECK(d_pool_capacity(&p) == 0);   /* nothing until asked */

    exercise_basic(&p, 40);
    CHECK(d_pool_block_count(&p) >= 5);

    /* a monotonic pool says so rather than pretending to free */
    s = d_pool_at(&p, 0);
    CHECK(s != NULL);
    CHECK(d_pool_release_slot(&p, s) == D_MEM_ERR_UNSUPPORTED);
    CHECK(d_pool_size(&p) == 40);

    /* reset returns everything at once and reuses the same storage */
    d_pool_reset(&p);
    CHECK(d_pool_size(&p) == 0);
    CHECK(d_pool_acquire(&p) == s);

    d_pool_release(&p);
    CHECK(d_pool_capacity(&p) == 0);

    return;
}


static void
test_free_list(void)
{
#if (D_INTERNAL_POOL_FREE_LIST == 1)
    struct d_pool        p;
    struct d_pool_config cfg;
    void*                a;
    void*                b;
    void*                c;
    d_pool_index         cap;

    cfg = d_pool_config_default(sizeof(struct node), 16);
    cfg.policy         = D_POOL_POLICY_FREE_LIST;
    cfg.slots_per_block = 16;

    CHECK(d_pool_init(&p, &cfg) == D_MEM_OK);
    exercise_basic(&p, 32);

    cap = d_pool_capacity(&p);

    /* a released slot comes straight back, and the pool does not grow */
    a = d_pool_at(&p, 3);
    CHECK(a != NULL);
    CHECK(d_pool_release_slot(&p, a) == D_MEM_OK);
    CHECK(d_pool_size(&p) == 31);

    b = d_pool_acquire(&p);
    CHECK(b == a);
    CHECK(d_pool_capacity(&p) == cap);

    /* release several, reacquire the same number, still no growth */
    {
        void* held[8];
        int   i;

        for (i = 0; i < 8; ++i)
        {
            held[i] = d_pool_at(&p, (d_pool_index)i);
            CHECK(d_pool_release_slot(&p, held[i]) == D_MEM_OK);
        }

        CHECK(d_pool_size(&p) == 24);

        for (i = 0; i < 8; ++i)
        {
            c = d_pool_acquire(&p);
            CHECK(c != NULL);
        }

        CHECK(d_pool_capacity(&p) == cap);
        CHECK(d_pool_size(&p) == 32);
    }

    /* a foreign pointer is refused */
    {
        char elsewhere[64];

        CHECK(d_pool_release_slot(&p, elsewhere) == D_MEM_ERR_FOREIGN);
    }

    /* a double free is caught where the check is compiled */
#if (D_INTERNAL_POOL_DOUBLE_FREE_CHECK == 1)
    a = d_pool_at(&p, 5);
    CHECK(d_pool_release_slot(&p, a) == D_MEM_OK);
    CHECK(d_pool_release_slot(&p, a) == D_MEM_ERR_CORRUPT);
#else
    ++skipped;
#endif

    d_pool_release(&p);
#else
    ++skipped;
#endif

    return;
}


static void
test_generational(void)
{
#if (D_INTERNAL_POOL_GENERATIONAL == 1)
    struct d_pool        p;
    struct d_pool_config cfg;
    struct d_pool_handle h1;
    struct d_pool_handle h2;
    struct d_pool_handle held[16];
    void*                s;
    int                  i;

    cfg = d_pool_config_default(sizeof(struct node), 16);
    cfg.policy         = D_POOL_POLICY_GENERATIONAL;
    cfg.slots_per_block = 8;

    CHECK(d_pool_init(&p, &cfg) == D_MEM_OK);

    /* a handle resolves to writable, correctly aligned storage */
    CHECK(d_pool_acquire_handle(&p, &h1) == D_MEM_OK);
    CHECK(!d_pool_handle_is_null(h1));

    s = d_pool_resolve(&p, h1);
    CHECK(s != NULL);
    CHECK(d_mem_is_aligned(s, d_pool_slot_align(&p)));
    memset(s, 0x7C, sizeof(struct node));

    /* THE POINT OF THE POLICY: a handle held across a release goes stale
       rather than silently naming the next occupant */
    CHECK(d_pool_release_handle(&p, h1) == D_MEM_OK);
    CHECK(d_pool_resolve(&p, h1) == NULL);
    CHECK(!d_pool_handle_is_live(&p, h1));

    {
        void* out = (void*)1;

        CHECK(d_pool_resolve_ex(&p, h1, &out) == D_MEM_ERR_STALE);
        CHECK(out == NULL);
    }

    /* the reused slot is a different handle, at the same address */
    CHECK(d_pool_acquire_handle(&p, &h2) == D_MEM_OK);
    CHECK(h2.index == h1.index);
    CHECK(h2.generation != h1.generation);
    CHECK(d_pool_resolve(&p, h2) == s);
    CHECK(d_pool_resolve(&p, h1) == NULL);

    /* releasing a stale handle is detected, not double-freed */
    CHECK(d_pool_release_handle(&p, h1) == D_MEM_ERR_STALE);

    /* the null handle never resolves */
    CHECK(d_pool_resolve(&p, d_pool_handle_null()) == NULL);

    /* a zeroed handle must not resolve to a never-used slot: this is why
       acquire advances the counter as well as release */
    {
        struct d_pool_handle zeroed;

        memset(&zeroed, 0, sizeof(zeroed));
        CHECK(d_pool_resolve(&p, zeroed) == NULL);
    }

    /* many handles at once, across several blocks, all distinct and live */
    for (i = 0; i < 16; ++i)
    {
        CHECK(d_pool_acquire_handle(&p, &held[i]) == D_MEM_OK);
        s = d_pool_resolve(&p, held[i]);
        CHECK(s != NULL);

        if (s)
        {
            memset(s, (unsigned char)i, sizeof(struct node));
        }
    }

    for (i = 0; i < 16; ++i)
    {
        s = d_pool_resolve(&p, held[i]);
        CHECK(s != NULL);

        if (s)
        {
            CHECK(((unsigned char*)s)[0] == (unsigned char)i);
        }
    }

    /* reset must invalidate every outstanding handle */
    d_pool_reset(&p);

    for (i = 0; i < 16; ++i)
    {
        CHECK(!d_pool_handle_is_live(&p, held[i]));
    }

    CHECK(!d_pool_handle_is_live(&p, h2));

    d_pool_release(&p);
#else
    ++skipped;
#endif

    return;
}


static void
test_ceiling_and_geometry(void)
{
    struct d_pool        p;
    struct d_pool_config cfg;
    struct d_mem_block   blk;
    int                  i;

    /* a slot ceiling is hard */
    cfg = d_pool_config_default(32, 8);
    cfg.slots_per_block = 4;
    cfg.max_slots       = 10;

    CHECK(d_pool_init(&p, &cfg) == D_MEM_OK);

    for (i = 0; i < 10; ++i)
    {
        CHECK(d_pool_acquire(&p) != NULL);
    }

    CHECK(d_pool_acquire_ex(&p, &blk) == D_MEM_ERR_EXHAUSTED);
    CHECK(d_pool_capacity(&p) == 10);   /* clamped, not rounded up to 12 */
    d_pool_release(&p);

    /* geometry is answerable before a pool exists, and agrees with one */
    {
        d_mem_size stride;
        d_mem_size align;

        stride = d_pool_stride_for(sizeof(struct node), 16,
                                   D_POOL_POLICY_MONOTONIC);
        align  = d_pool_align_for(16, D_POOL_POLICY_MONOTONIC);

        CHECK(stride >= sizeof(struct node));
        CHECK((stride % align) == 0);
        CHECK(align >= 16);

        cfg = d_pool_config_default(sizeof(struct node), 16);
        cfg.policy = D_POOL_POLICY_MONOTONIC;
        CHECK(d_pool_init(&p, &cfg) == D_MEM_OK);
        CHECK(d_pool_slot_align(&p) == align);
        d_pool_release(&p);
    }

    /* a one-byte slot still works, and the free list still fits in it */
    cfg = d_pool_config_default(1, 0);
    CHECK(d_pool_init(&p, &cfg) == D_MEM_OK);

    {
        void* a = d_pool_acquire(&p);
        void* b = d_pool_acquire(&p);

        CHECK(a && b);
        CHECK(a != b);

#if (D_INTERNAL_POOL_FREE_LIST == 1)
        CHECK(d_pool_release_slot(&p, a) == D_MEM_OK);
        CHECK(d_pool_acquire(&p) == a);
#endif
    }

    d_pool_release(&p);

    /*   Malformed arguments are REPORTED here and ASSERTED under
       D_CFG_MEM_STRICT_ARGS, which is what that knob means -- so a strict
       build cannot be asked to return from them. */
#if (D_INTERNAL_MEM_STRICT_ARGS == 0)
    /* a zero-sized slot is refused */
    cfg = d_pool_config_default(0, 0);
    CHECK(d_pool_init(&p, &cfg) == D_MEM_ERR_INVALID);

    /* an alignment that is not a power of two is refused rather than raised
       to the configured floor, even when the floor would cover it */
    cfg = d_pool_config_default(16, 3);
    CHECK(d_pool_init(&p, &cfg) == D_MEM_ERR_INVALID);

    /* an alignment above the ceiling is refused too */
    cfg = d_pool_config_default(16, (d_mem_size)D_MEM_ALIGN_MAX * 2);
    CHECK(d_pool_init(&p, &cfg) == D_MEM_ERR_INVALID);
#else
    ++skipped;
#endif

    return;
}


static void
test_over_arena(void)
{
#if (D_INTERNAL_ARENA_AS_SOURCE == 1)
    static char           buffer[262144];
    struct d_arena        arena;
    struct d_pool         p;
    struct d_pool_config  cfg;

    /* an entire pool whose every byte came from a static array: no heap
       anywhere in this section, in any configuration */
    CHECK(d_arena_init_buffer(&arena, buffer, (d_mem_size)sizeof(buffer)) ==
          D_MEM_OK);

    cfg = d_pool_config_default(sizeof(struct node), 16);
    cfg.source          = d_arena_as_source(&arena);
    cfg.slots_per_block = 32;

    CHECK(d_pool_init(&p, &cfg) == D_MEM_OK);
    exercise_basic(&p, 64);

#if (D_INTERNAL_ARENA_OWNS == 1)
    {
        void* s = d_pool_at(&p, 0);

        CHECK(s != NULL);
        CHECK(d_arena_owns(&arena, s));   /* really from the static array */
    }
#endif

    d_pool_release(&p);
    d_arena_release(&arena);
#else
    ++skipped;
#endif

    return;
}


static void
test_iteration(void)
{
#if (D_INTERNAL_POOL_ITERATE == 1)
    struct d_pool        p;
    struct d_pool_config cfg;
    struct d_pool_cursor cursor;
    d_pool_index         seen;
    int                  i;

    cfg = d_pool_config_default(sizeof(struct node), 8);
    cfg.policy         = D_POOL_POLICY_MONOTONIC;
    cfg.slots_per_block = 5;   /* deliberately not a power of two */

    CHECK(d_pool_init(&p, &cfg) == D_MEM_OK);

    for (i = 0; i < 23; ++i)
    {
        void* s = d_pool_acquire(&p);

        CHECK(s != NULL);

        if (s)
        {
            memset(s, (unsigned char)i, sizeof(struct node));
        }
    }

    /* the walk visits every handed-out slot exactly once, in index order,
       and crosses block boundaries correctly */
    seen   = 0;
    cursor = d_pool_first(&p);

    while (d_pool_cursor_valid(&p, &cursor))
    {
        void* s = d_pool_cursor_slot(&p, &cursor);

        CHECK(s != NULL);
        CHECK(s == d_pool_at(&p, seen));

        if (s)
        {
            CHECK(((unsigned char*)s)[0] == (unsigned char)seen);
        }

        ++seen;
        d_pool_next(&p, &cursor);
    }

    CHECK(seen == 23);

    d_pool_release(&p);
#else
    ++skipped;
#endif

    return;
}


int
main(void)
{
    struct d_mem_source probe;
    void*               p;
    int                 heap;

    probe = d_mem_source_default();
    p     = d_mem_source_allocate(&probe, 64, 0);
    heap  = (p != NULL);

    if (p)
    {
        d_mem_source_release(&probe, p, 64, 0);
    }

    printf("pool harness: heap=%s  free_list=%d  generational=%d  "
           "indexed=%d  index_bits=%d  gen_bits=%d\n",
           heap ? "yes" : "no",
           (int)D_INTERNAL_POOL_FREE_LIST,
           (int)D_INTERNAL_POOL_GENERATIONAL,
           (int)D_INTERNAL_POOL_INDEXED_LINKS,
           (int)D_INTERNAL_POOL_INDEX_BITS,
           (int)D_INTERNAL_POOL_GENERATION_BITS);

    /* this section needs no heap in any configuration */
    test_over_arena();

    if (!heap)
    {
        printf("pool harness: no heap in this configuration; "
               "heap sections skipped\n");
        printf(fails ? "POOL: %d FAILURE(S)\n" : "POOL: all checks passed\n",
               fails);

        return fails ? 1 : 0;
    }

    test_monotonic();
    test_free_list();
    test_generational();
    test_ceiling_and_geometry();
    test_iteration();

    printf("pool harness: %d section(s) skipped by configuration\n", skipped);
    printf(fails ? "POOL: %d FAILURE(S)\n" : "POOL: all checks passed\n",
           fails);

    return fails ? 1 : 0;
}
