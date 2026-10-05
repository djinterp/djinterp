/*******************************************************************************
* djinterp [c]                                                         t_arena.c
*
*   Arena conformance harness. Runs against ANY configuration of the memory
* subframework, including builds with no heap: a section the configuration did
* not compile is skipped rather than failed.
*
*   Build (from the repo root):
*     cc  src/djinterp/c/memory/mem_common.c \
*         src/djinterp/c/memory/mem_source.c \
*         src/djinterp/c/memory/arena.c \
*         src/djinterp/c/memory/pool.c \
*         src/djinterp/c/memory/test/t_arena.c -o t_arena
*   No -I is required: every include is repo-root relative.
*
*
* path:      /src/djinterp/c/memory/test/t_arena.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*   Arena conformance harness.
 *   Runs against ANY configuration of the memory subframework, including ones
 * with no heap at all: the MINIMAL preset compiles no system source, so the
 * default source refuses every request, and every heap-dependent section here
 * is skipped rather than failed. That is the point -- a configuration knob
 * that made this harness fail would be a knob that broke the module.
 */
#include <stdio.h>
#include <string.h>
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


/* heap_available
 *   Reports whether this configuration's default source can vend anything at
 * all. Asked once, by allocating and releasing a block.
 */
static int
heap_available(void)
{
    struct d_mem_source s;
    void*               p;

    s = d_mem_source_default();
    p = d_mem_source_allocate(&s, 64, 0);

    if (!p)
    {
        return 0;
    }

    d_mem_source_release(&s, p, 64, 0);

    return 1;
}


/* exercise
 *   The body every arena must satisfy, whatever supplies its regions. Takes a
 * ready arena and a flag saying whether it is allowed to grow, so the same
 * checks run over a heap-backed arena, a buffer-backed one, and an
 * arena-backed one.
 */
static void
exercise(
    struct d_arena* _a,
    int             _can_grow
)
{
    void*               p1;
    void*               p2;
    int                 i;
    char*               s;
#if (D_INTERNAL_ARENA_MARKS == 1)
    struct d_arena_mark m;
#endif

    /* alignment is honoured for every power of two the module admits */
    for (i = 1; i <= 64; i <<= 1)
    {
        void* q = d_arena_allocate(_a, (d_mem_size)i + 1, (d_mem_size)i);

        CHECK(q != NULL);

        if (q)
        {
            CHECK(d_mem_is_aligned(q, (d_mem_size)i));
            memset(q, 0xEE, (size_t)i + 1);
#if (D_INTERNAL_ARENA_OWNS == 1)
            CHECK(d_arena_owns(_a, q));
#endif
        }
    }

    /* successive allocations do not overlap */
    p1 = d_arena_allocate(_a, 64, 8);
    p2 = d_arena_allocate(_a, 64, 8);
    CHECK(p1 && p2);

    if (p1 && p2)
    {
        CHECK(((char*)p2 >= (char*)p1 + 64) ||
              ((char*)p1 >= (char*)p2 + 64));
    }

    /* strings, terminator included */
    s = d_arena_duplicate_string(_a, "hello arena");
    CHECK(s != NULL);

    if (s)
    {
        CHECK(strcmp(s, "hello arena") == 0);
    }

    /* the guarded product refuses to wrap */
    CHECK(d_arena_allocate_array(_a, D_MEM_SIZE_MAX, 2, 8) == NULL);

#if (D_INTERNAL_ARENA_MARKS == 1)
    /* a mark restores the cursor exactly */
    m  = d_arena_mark_get(_a);
    p1 = d_arena_allocate(_a, 128, 8);

    if (p1)
    {
        CHECK(d_arena_rewind(_a, m) == D_MEM_OK);
        p2 = d_arena_allocate(_a, 128, 8);
        CHECK(p2 == p1);
    }

    /* a foreign mark is refused where validation is compiled */
#   if (D_INTERNAL_ARENA_VALIDATE_MARKS == 1)
    {
        struct d_arena_mark bogus;
        char                elsewhere[64];

        bogus.region = (struct d_arena_region*)(void*)elsewhere;
        bogus.used   = 0;

        CHECK(d_arena_rewind(_a, bogus) == D_MEM_ERR_FOREIGN);
    }
#   else
    ++skipped;
#   endif
#else
    ++skipped;
#endif

    /* reset returns every byte and keeps the regions */
    d_arena_reset(_a);
    CHECK(d_arena_used(_a) == 0);
    CHECK(d_arena_allocate(_a, 16, 8) != NULL);

    if (!_can_grow)
    {
        CHECK(d_arena_region_count(_a) <= 1);
    }

    return;
}


int
main(void)
{
    struct d_arena        a;
    struct d_arena        b;
    struct d_arena_config cfg;
    struct d_mem_stats    st;
    static char           buffer[65536];
    int                   heap;

    heap = heap_available();

    printf("arena harness: heap=%s  chain=%d  stats=%d  redzone=%d  "
           "size_bits=%d\n",
           heap ? "yes" : "no",
           (int)D_INTERNAL_ARENA_CHAIN,
           (int)D_INTERNAL_MEM_STATS,
           (int)D_INTERNAL_MEM_REDZONE,
           (int)D_INTERNAL_MEM_SIZE_BITS);

    /* 1. an arena over the caller's own buffer -- works in every build */
    CHECK(d_arena_init_buffer(&b, buffer, (d_mem_size)sizeof(buffer)) ==
          D_MEM_OK);
    CHECK(d_arena_capacity(&b) > 0);
    CHECK(d_arena_capacity(&b) < (d_mem_size)sizeof(buffer));
    exercise(&b, 0);

    /* the buffer arena must never hand the caller's memory to a source */
    d_arena_release(&b);
    CHECK(d_arena_capacity(&b) == 0);

    /* 2. a buffer arena reports exhaustion rather than corrupting */
    {
        static char        small[512];
        struct d_mem_block blk;
        enum d_mem_status  status;
        d_mem_size         handed;

        CHECK(d_arena_init_buffer(&a, small, (d_mem_size)sizeof(small)) ==
              D_MEM_OK);

        handed = 0;
        status = D_MEM_OK;

        for (;;)
        {
            status = d_arena_allocate_ex(&a, 32, 8, &blk);

            if (status != D_MEM_OK)
            {
                break;
            }

            memset(blk.ptr, 0x5A, 32);
            handed = (d_mem_size)(handed + 32);
            CHECK(handed <= (d_mem_size)sizeof(small));
        }

        CHECK(handed > 0);
        CHECK((status == D_MEM_ERR_EXHAUSTED) ||
              (status == D_MEM_ERR_UNSUPPORTED));
        d_arena_release(&a);
    }

    if (!heap)
    {
        printf("arena harness: no heap in this configuration; "
               "heap sections skipped\n");
        printf(fails ? "ARENA: %d FAILURE(S)\n" : "ARENA: all checks passed\n",
               fails);

        return fails ? 1 : 0;
    }

    /* 3. the ordinary heap-backed arena */
    CHECK(d_arena_init(&a, NULL) == D_MEM_OK);
    CHECK(d_arena_region_count(&a) == 0);   /* nothing obtained until asked */
    exercise(&a, D_INTERNAL_ARENA_CHAIN);

    /* 4. growth across many regions keeps every earlier pointer valid */
#if (D_INTERNAL_ARENA_CHAIN == 1)
    {
        void* kept[64];
        int   i;

        for (i = 0; i < 64; ++i)
        {
            kept[i] = d_arena_allocate(&a, 8192, 16);
            CHECK(kept[i] != NULL);

            if (kept[i])
            {
                memset(kept[i], (unsigned char)i, 8192);
            }
        }

        CHECK(d_arena_region_count(&a) > 1);

        /* every byte written before the growth is still where it was */
        for (i = 0; i < 64; ++i)
        {
            if (kept[i])
            {
                CHECK(((unsigned char*)kept[i])[0]    == (unsigned char)i);
                CHECK(((unsigned char*)kept[i])[8191] == (unsigned char)i);
            }
        }

        d_arena_trim(&a);
        CHECK(d_arena_region_count(&a) == 1);
    }
#else
    ++skipped;
#endif

    /* 5. accounting, where it is compiled */
    if (d_arena_stats(&a, &st) == D_MEM_OK)
    {
        CHECK(st.bytes_reserved > 0);
        CHECK(st.acquire_count > 0);
    }
    else
    {
        ++skipped;
    }

    d_arena_release(&a);
    CHECK(d_arena_region_count(&a) == 0);

    /* 6. a total budget is a hard ceiling */
    cfg               = d_arena_config_default();
    cfg.initial_bytes = 1024;
    cfg.max_bytes     = 2048;
    CHECK(d_arena_init(&a, &cfg) == D_MEM_OK);
    CHECK(d_arena_allocate(&a, 900, 8) != NULL);

    {
        struct d_mem_block blk;

        /* larger than the whole budget: refused, and refused for the right
           reason rather than by an upstream failure */
        CHECK(d_arena_allocate_ex(&a, 8192, 8, &blk) == D_MEM_ERR_EXHAUSTED);
        CHECK(d_arena_capacity(&a) <= 2048);
    }

    d_arena_release(&a);

    /* 7. an arena drawing from another arena */
#if (D_INTERNAL_ARENA_AS_SOURCE == 1) && (D_INTERNAL_ARENA_OWNS == 1)
    {
        void* p;

        CHECK(d_arena_init(&a, NULL) == D_MEM_OK);

        cfg               = d_arena_config_default();
        cfg.source        = d_arena_as_source(&a);
        cfg.initial_bytes = 4096;

        CHECK(d_arena_init(&b, &cfg) == D_MEM_OK);

        p = d_arena_allocate(&b, 100, 8);
        CHECK(p != NULL);
        CHECK(d_arena_owns(&a, p));   /* the bytes really came from a */
        CHECK(d_arena_owns(&b, p));

        d_arena_release(&b);
        d_arena_release(&a);
    }
#else
    ++skipped;
#endif

    /* 8. the counting source measures what an arena costs its upstream */
#if (D_INTERNAL_MEM_SOURCE_COUNTER == 1)
    {
        struct d_mem_counting_source counter;
        struct d_mem_stats           cs;

        CHECK(d_mem_counting_source_init(&counter, d_mem_source_default()) ==
              D_MEM_OK);

        cfg        = d_arena_config_default();
        cfg.source = d_mem_source_counting(&counter);

        CHECK(d_arena_init(&a, &cfg) == D_MEM_OK);
        CHECK(d_arena_allocate(&a, 64, 8) != NULL);

        CHECK(d_mem_counting_source_stats(&counter, &cs) == D_MEM_OK);
        CHECK(cs.upstream_count >= 1);
        CHECK(cs.bytes_live > 0);

        d_arena_release(&a);

        CHECK(d_mem_counting_source_stats(&counter, &cs) == D_MEM_OK);
        CHECK(cs.bytes_live == 0);   /* everything went back */
    }
#else
    ++skipped;
#endif

    printf("arena harness: %d section(s) skipped by configuration\n", skipped);
    printf(fails ? "ARENA: %d FAILURE(S)\n" : "ARENA: all checks passed\n",
           fails);

    return fails ? 1 : 0;
}
