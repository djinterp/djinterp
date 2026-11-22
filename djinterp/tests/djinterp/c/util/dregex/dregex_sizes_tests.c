/*******************************************************************************
* djinterp [c][utility]                                     dregex_sizes_tests.c
*
* Checks that every size dregex.c computes for an allocation refuses to wrap,
* in DTest's C face.
*   Each pool, buffer and scratch array is sized by a count the engine keeps
* itself, so no pattern a test can write reaches a count that wraps a size_t.
* The suite therefore includes dregex.c, as a white-box test, and gives each
* growth step and each allocator a count too large: every one must report
* D_REGEX_ERR_MEMORY, or false, with nothing allocated and nothing it was
* handed changed. The two-patches-a-copy list of an interval is the one site
* not here: its count is an int, bounded by D_REGEX_DUP_MAX, so its product
* cannot wrap a 64-bit size_t; the check guards 32-bit builds.
*   Its target links no dregex.c of its own: this file includes it.
*
*
* path:      /tests/djinterp/c/util/dregex/dregex_sizes_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "./dregex_sizes_tests.h"  // corresponding header
// djinterp
#include "../../../../../src/djinterp/c/util/dregex.c"  // the engine, whole
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // SIZE_MAX


// g_suite
//   state: the suite every check goes to, set by the section.
static struct dj_test_suite* g_suite = NULL;

// D_TESTS_DREGEX_SIZES_CHECK
//   macro: records one check, named by what it checks, with its line.
#define D_TESTS_DREGEX_SIZES_CHECK(_passed, _what)                            \
    (void)d_test_suite_check(g_suite, (_passed), (_what), __FILE__, __LINE__)

/*
d_tests_node_pool
  The parse tree's pool, full at a capacity whose double wraps, and at one
whose double fits but whose bytes wrap:
  - d_internal_node_new returns -1 and latches D_REGEX_ERR_MEMORY
  - the pool is not reallocated, so its pointer and counts are as they were
*/
static
void d_tests_node_pool(void)
{
    (void)d_test_suite_test(g_suite, "the node pool");

    const size_t capacities[2] =
    {
        (SIZE_MAX / 2u) + 1u,
        ((SIZE_MAX / sizeof(struct dj_internal_node)) / 2u) + 1u
    };

    for (size_t i = 0u; i < 2u; ++i)
    {
        struct dj_internal_parser parser;

        memset(&parser, 0, sizeof(parser));
        parser.node_count    = capacities[i];
        parser.node_capacity = capacities[i];
        parser.status        = D_REGEX_OK;

        const int index = d_internal_node_new(&parser,
                                              D_INTERNAL_NODE_EMPTY);

        D_TESTS_DREGEX_SIZES_CHECK( (index == -1)                           &&
                                    (parser.status == D_REGEX_ERR_MEMORY)   &&
                                    (parser.nodes == NULL)                  &&
                                    (parser.node_capacity == capacities[i]) &&
                                    (parser.node_count == capacities[i]),
                                    (i == 0u) ? "node pool: doubling wraps"
                                              : "node pool: bytes wrap");
    }

    return;
}

/*
d_tests_class_pool
  The bracket-expression pool, 32 bytes a class, full at a capacity whose
double wraps and at one whose bytes wrap:
  - d_internal_class_new returns -1 and latches D_REGEX_ERR_MEMORY
  - the pool is untouched
*/
static
void d_tests_class_pool(void)
{
    (void)d_test_suite_test(g_suite, "the class pool");

    const size_t capacities[2] =
    {
        (SIZE_MAX / 2u) + 1u,
        ((SIZE_MAX / 32u) / 2u) + 1u
    };

    for (size_t i = 0u; i < 2u; ++i)
    {
        struct dj_internal_parser parser;

        memset(&parser, 0, sizeof(parser));
        parser.class_count    = capacities[i];
        parser.class_capacity = capacities[i];
        parser.status         = D_REGEX_OK;

        const int index = d_internal_class_new(&parser);

        D_TESTS_DREGEX_SIZES_CHECK( (index == -1)                         &&
                                    (parser.status == D_REGEX_ERR_MEMORY) &&
                                    (parser.classes == NULL)              &&
                                    (parser.class_capacity == capacities[i]),
                                    (i == 0u) ? "class pool: doubling wraps"
                                              : "class pool: bytes wrap");
    }

    return;
}

/*
d_tests_program
  The instruction buffer, full at a capacity whose double wraps and at one
whose bytes wrap:
  - d_internal_emit returns -1 and latches D_REGEX_ERR_MEMORY
  - the buffer is untouched
*/
static
void d_tests_program(void)
{
    (void)d_test_suite_test(g_suite, "the program");

    const size_t capacities[2] =
    {
        (SIZE_MAX / 2u) + 1u,
        ((SIZE_MAX / sizeof(struct dj_internal_inst)) / 2u) + 1u
    };

    for (size_t i = 0u; i < 2u; ++i)
    {
        struct dj_internal_emitter emitter;

        memset(&emitter, 0, sizeof(emitter));
        emitter.count    = capacities[i];
        emitter.capacity = capacities[i];
        emitter.status   = D_REGEX_OK;

        const int address = d_internal_emit(&emitter,
                                            D_INTERNAL_OP_MATCH);

        D_TESTS_DREGEX_SIZES_CHECK( (address == -1)                        &&
                                    (emitter.status == D_REGEX_ERR_MEMORY) &&
                                    (emitter.program == NULL)              &&
                                    (emitter.capacity == capacities[i]),
                                    (i == 0u) ? "program: doubling wraps"
                                              : "program: bytes wrap");
    }

    return;
}

/*
d_tests_scratch
  A compiled pattern's scratch buffers, for a program too long to size:
  - a slot count whose start offsets' bytes wrap allocates nothing and fails
  - so does one whose program counters' bytes wrap too; the closure stack's
    2n + 8 entries cannot wrap first, as each entry is wider than two bytes
  - a small program still gets all six buffers
*/
static
void d_tests_scratch(void)
{
    (void)d_test_suite_test(g_suite, "the scratch buffers");

    const size_t slots[2] =
    {
        (SIZE_MAX / sizeof(size_t)) + 1u,
        (SIZE_MAX / sizeof(int)) + 1u
    };

    for (size_t i = 0u; i < 2u; ++i)
    {
        struct dj_regex regex;

        memset(&regex, 0, sizeof(regex));

        const bool made = d_internal_scratch_new(&regex,
                                                 slots[i]);

        D_TESTS_DREGEX_SIZES_CHECK( (!made)                       &&
                                    (regex.list_a.pc == NULL)     &&
                                    (regex.list_a.starts == NULL) &&
                                    (regex.list_b.pc == NULL)     &&
                                    (regex.list_b.starts == NULL) &&
                                    (regex.seen == NULL)          &&
                                    (regex.stack == NULL),
                                    (i == 0u)
                                        ? "scratch: start offsets' bytes wrap"
                                        : "scratch: every list's bytes wrap");
    }

    struct dj_regex small;

    memset(&small, 0, sizeof(small));

    const bool made = d_internal_scratch_new(&small,
                                             4u);

    D_TESTS_DREGEX_SIZES_CHECK(made,
                               "scratch: a small program gets its buffers");
    free(small.list_a.pc);
    free(small.list_a.starts);
    free(small.list_b.pc);
    free(small.list_b.starts);
    free(small.seen);
    free(small.stack);

    return;
}

/*
d_tests_capture_state
  The capturing pass's buffers, for a pattern or window too large to size:
  - group count + 1 wraps
  - program length + window wraps
  - (program + window + 8) * 8 entries wraps
  - the stacks' bytes wrap, though their entry count fits
  - in each, nothing is allocated and the state stays zero
  - a small pattern still gets all four buffers
*/
static
void d_tests_capture_state(void)
{
    (void)d_test_suite_test(g_suite, "the capture state");

    struct
    {
        size_t      groups;
        size_t      program;
        size_t      extent;
        const char* what;
    } const cases[4] =
    {
        { SIZE_MAX,             1u,                    1u,
          "capture: the group count wraps" },
        { 1u,                   SIZE_MAX - 4u,         8u,
          "capture: program and window wrap" },
        { 1u,                   SIZE_MAX / 16u,        SIZE_MAX / 16u,
          "capture: the stacks' entries wrap" },
        { 1u,                   (SIZE_MAX / 8u) / 8u,  0u,
          "capture: the stacks' bytes wrap" }
    };

    for (size_t i = 0u; i < 4u; ++i)
    {
        struct dj_regex                  regex;
        struct dj_internal_capture_state state;

        memset(&regex, 0, sizeof(regex));
        memset(&state, 0, sizeof(state));
        regex.group_count   = cases[i].groups;
        regex.program_count = cases[i].program;

        const bool made = d_internal_capture_state_new(&state,
                                                       &regex,
                                                       cases[i].extent);

        D_TESTS_DREGEX_SIZES_CHECK( (!made)                     &&
                                    (state.marks == NULL)       &&
                                    (state.best == NULL)        &&
                                    (state.undo == NULL)        &&
                                    (state.choice == NULL)      &&
                                    (state.undo_capacity == 0u) &&
                                    (state.choice_capacity == 0u),
                                    cases[i].what);
    }

    struct dj_regex                  small;
    struct dj_internal_capture_state state;

    memset(&small, 0, sizeof(small));
    memset(&state, 0, sizeof(state));
    small.group_count   = 2u;
    small.loop_count    = 1u;
    small.program_count = 12u;

    const bool made = d_internal_capture_state_new(&state,
                                                   &small,
                                                   5u);

    D_TESTS_DREGEX_SIZES_CHECK( (made)                        &&
                                (state.undo_capacity == 200u) &&
                                (state.choice_capacity == 200u),
                                "capture: a small pattern gets its buffers");
    free(state.marks);
    free(state.best);
    free(state.undo);
    free(state.choice);

    return;
}

/*
d_tests_dregex_sizes
  The suite's section: each group, in order.
*/
void d_tests_dregex_sizes(
    struct dj_test_suite* _suite
)
{
    g_suite = _suite;

    (void)d_test_suite_module(_suite, "dregex_sizes");
    (void)d_test_suite_describe(_suite,
                                "c/util/dregex: sizes that would wrap");
    (void)d_test_suite_block(_suite, "dregex");

    d_tests_node_pool();
    d_tests_class_pool();
    d_tests_program();
    d_tests_scratch();
    d_tests_capture_state();

    g_suite = NULL;

    return;
}
