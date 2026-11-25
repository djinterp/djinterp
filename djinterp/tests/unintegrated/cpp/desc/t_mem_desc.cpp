/*******************************************************************************
* djinterp [cpp][desc]                                            t_mem_desc.cpp
*
*   The harness of the C++ face of desc's describers of memory's types
* (cpp/desc/of/mem_desc.hpp), at every level from C++98: each type's units
* are its C description and its name is its noun; describe() writes what
* d_desc_say writes, over a real arena and pool; facts() reaches the field
* list; and the describers' unit lays memory's types out as this program
* does. memory's headers use variadic macros, which C++98 has only as an
* extension: build at C++98 with -Wno-variadic-macros beside
* -pedantic-errors. A standalone program, as the other tests not yet in
* DTest are: it
* prints a line per group and one per failed check, ends with "N checks, M
* failed", and exits 0 only when nothing failed.
*
*
* path:      /tests/unintegrated/cpp/desc/t_mem_desc.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

// std
#include <cstdio>   // std::printf
#include <cstring>  // std::strcmp
#include <string>   // std::string
// djinterp
#include "../../../../inc/djinterp/cpp/desc/of/mem_desc.hpp"  // the face


unsigned long g_checks   = 0;
unsigned long g_failures = 0;

// d_tests_check
//   function: one check: counted, and printed when it fails.
static void
d_tests_check(
    bool        _passed,
    int         _line,
    const char* _expression
)
{
    ++g_checks;

    if (!_passed)
    {
        ++g_failures;
        std::printf("    FAILED line %d: %s\n", _line, _expression);
    }

    return;
}

#define D_TESTS_CHECK(e)  d_tests_check((e), __LINE__, #e)

#if D_INTERNAL_DESC_OF_MEMORY

// d_tests_count_fact
//   function: a machine's fact operation that counts the facts.
extern "C" int
d_tests_count_fact(
    void*                      _context,
    const struct dj_desc_fact* _fact
)
{
    (void)_fact;
    ++*static_cast<unsigned long*>(_context);

    return 0;
}

// d_tests_said
//   function: what d_desc_say writes of an object at a ring, as a string.
static std::string
d_tests_said(
    const struct dj_desc_type* _type,
    const void*                _object,
    uint32_t                   _ring
)
{
    char                  room[256];
    struct dj_desc_buffer buffer;

    d_desc_buffer_init(&buffer, room, sizeof(room));
    (void)d_desc_say(_type, _object, _ring, D_DESC_LAYERS_ALL, 0u,
                     d_desc_put_buffer, &buffer);

    return std::string(room);
}

#endif  // D_INTERNAL_DESC_OF_MEMORY

int
main()
{
    std::printf("t_mem_desc (C++%ld)\n", static_cast<long>(__cplusplus));

#if D_INTERNAL_DESC_OF_MEMORY
    using djinterp::desc::describe;
    using djinterp::desc::type_name;
    using djinterp::desc::units;

    static unsigned char storage[512];
    struct dj_arena      arena;
    struct dj_pool       pool;
    struct dj_mem_block  block;
    uint32_t             ring;

    std::printf("  units and names, from the C descriptions\n");
    D_TESTS_CHECK(&units<dj_arena>::type() == &d_desc_type_dj_arena);
    D_TESTS_CHECK(&units<dj_pool>::type() == D_DESC_OF(dj_pool));
    D_TESTS_CHECK(&units<dj_mem_stats>::type() == &d_desc_type_dj_mem_stats);
    D_TESTS_CHECK(std::strcmp(type_name<dj_arena>::text(), "arena") == 0);
    D_TESTS_CHECK(std::strcmp(type_name<dj_mem_stats>::text(),
                              "allocation tally") == 0);
    D_TESTS_CHECK(std::strcmp(type_name<dj_mem_source>::text(),
                              "memory source") == 0);

    std::printf("  describe() is d_desc_say\n");
    D_TESTS_CHECK(d_arena_init_buffer(&arena, storage, sizeof(storage)) ==
                  D_MEM_OK);
    D_TESTS_CHECK(d_arena_allocate(&arena, 40u, 8u) != NULL);
    {
        struct dj_pool_config config = d_pool_config_default(16u, 8u);

        config.initial_slots = 4u;
        D_TESTS_CHECK(d_pool_init(&pool, &config) == D_MEM_OK);
        D_TESTS_CHECK(d_pool_acquire(&pool) != NULL);
    }
    for (ring = D_DESC_RING_NAME; ring <= D_DESC_RING_DETAILED; ++ring)
    {
        D_TESTS_CHECK(describe(arena, ring, D_DESC_LAYERS_ALL) ==
                      d_tests_said(D_DESC_OF(dj_arena), &arena, ring));
        D_TESTS_CHECK(describe(pool, ring, D_DESC_LAYERS_ALL) ==
                      d_tests_said(D_DESC_OF(dj_pool), &pool, ring));
    }
#if D_CFG_DESC_UNITS && D_CFG_DESC_ARTICLES && (D_CFG_DESC_RING_MAX >= 1)
    block.ptr  = storage;
    block.size = 64u;
    D_TESTS_CHECK(describe(block, D_DESC_RING_LINE, D_DESC_LAYERS_ALL) ==
                  "a memory block with 64 bytes");
    D_TESTS_CHECK(describe(arena, D_DESC_RING_NAME, D_DESC_LAYERS_ALL) ==
                  "arena");
#endif

    std::printf("  facts, and the build\n");
    block.ptr  = storage;
    block.size = 64u;
#if D_CFG_DESC_FACTS
    {
        static const struct dj_desc_machine machine =
            { NULL, d_tests_count_fact, NULL, NULL, NULL, NULL };
        unsigned long       count = 0u;
        struct dj_desc_sink sink;

        d_desc_sink_init(&sink, &machine, &count);
        D_TESTS_CHECK(djinterp::desc::facts(block, &sink) == 0);
        D_TESTS_CHECK(count == 2u);  // its type, and its size
    }
#endif
    D_TESTS_CHECK(d_desc_of_mem_agrees(NULL) == 1);

    d_pool_release(&pool);
    d_arena_release(&arena);
#else
    std::printf("  memory's describers are not built\n");
    D_TESTS_CHECK(d_desc_of_mem_layout(NULL) == 0u);
#endif

    std::printf("%lu checks, %lu failed\n", g_checks, g_failures);

    return (g_failures == 0u) ? 0 : 1;
}
