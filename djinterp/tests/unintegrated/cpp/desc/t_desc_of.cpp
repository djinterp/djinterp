/*******************************************************************************
* djinterp [cpp][desc]                                             t_desc_of.cpp
*
*   The harness of the C++ faces of desc's describers of filesys's and
* DTest's types (cpp/desc/of/filesys_desc.hpp and test_desc.hpp), at every
* level from C++98: each type's units are its C description and its name
* is its noun; describe() writes what d_desc_say writes; facts() reaches
* the field list, a spec's contents included; and the describers' units
* lay the types out as this program does. filesys's headers reach
* memory's, whose variadic macros C++98 has only as an extension: build at
* C++98 with -Wno-variadic-macros beside -pedantic-errors. A standalone
* program, as the other tests not yet in DTest are: it prints a line per
* group and one per failed check, ends with "N checks, M failed", and
* exits 0 only when nothing failed.
*
*
* path:      /tests/unintegrated/cpp/desc/t_desc_of.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

// std
#include <cstdio>   // std::printf
#include <cstring>  // std::strcmp
#include <string>   // std::string
// djinterp
#include "../../../../inc/djinterp/cpp/desc/of/filesys_desc.hpp"  // a face
#include "../../../../inc/djinterp/cpp/desc/of/test_desc.hpp"     // a face


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
    char                  room[320];
    struct dj_desc_buffer buffer;

    d_desc_buffer_init(&buffer, room, sizeof(room));
    (void)d_desc_say(_type, _object, _ring, D_DESC_LAYERS_ALL, 0u,
                     d_desc_put_buffer, &buffer);

    return std::string(room);
}

// d_tests_pass
//   function: a spec's one test.
static bool
d_tests_pass()
{
    return true;
}

#if D_INTERNAL_DESC_OF_FILESYS
// d_tests_filesys
//   function: filesys's face.
static void
d_tests_filesys()
{
    using djinterp::desc::describe;
    using djinterp::desc::type_name;
    using djinterp::desc::units;

    struct dj_stat32_t    status;
    struct dj_file_notice notice;
    uint32_t              ring;

    std::printf("  filesys: units, names and describe()\n");
    std::memset(&status, 0, sizeof(status));
    status.st_size  = 64u;
    status.st_mode  = 0100640u;
    status.st_nlink = 1u;
    notice.level    = D_FILE_NOTIFY_WARN;
    notice.error    = 0;
    notice.function = "d_file_open_stream";
    notice.path     = NULL;
    notice.message  = "setvbuf declined";
    D_TESTS_CHECK(&units<dj_stat32_t>::type() == &d_desc_type_dj_stat32_t);
    D_TESTS_CHECK(&units<dj_file_notice>::type() ==
                  D_DESC_OF(dj_file_notice));
    D_TESTS_CHECK(std::strcmp(type_name<dj_dirent32_t>::text(),
                              "directory entry") == 0);
    for (ring = D_DESC_RING_NAME; ring <= D_DESC_RING_DETAILED; ++ring)
    {
        D_TESTS_CHECK(describe(status, ring, D_DESC_LAYERS_ALL) ==
                      d_tests_said(D_DESC_OF(dj_stat32_t), &status, ring));
        D_TESTS_CHECK(describe(notice, ring, D_DESC_LAYERS_ALL) ==
                      d_tests_said(D_DESC_OF(dj_file_notice), &notice,
                                   ring));
    }
#if D_CFG_DESC_UNITS && D_CFG_DESC_ARTICLES && (D_CFG_DESC_RING_MAX >= 1)
    D_TESTS_CHECK(describe(status, D_DESC_RING_LINE, D_DESC_LAYERS_ALL) ==
                  "a regular file with 64 bytes");
#endif
    D_TESTS_CHECK(d_desc_of_filesys_agrees(NULL) == 1);

    return;
}
#endif  // D_INTERNAL_DESC_OF_FILESYS

#if D_INTERNAL_DESC_OF_TEST
// d_tests_dtest
//   function: DTest's face.
static void
d_tests_dtest()
{
    using djinterp::desc::describe;
    using djinterp::desc::type_name;
    using djinterp::desc::units;

    static const struct dj_test_case_spec   cases[] =
        { { "d_tests_pass", "#1: it passes", d_tests_pass } };
    static const struct dj_test_block_spec  blocks[] =
        { { "only", "the one block", cases, 1u } };
    static const struct dj_test_module_spec module =
        { "harness", "the harness's spec", blocks, 1u };
    struct dj_test_summary summary;
    uint32_t               ring;

    std::printf("  DTest: units, names, describe() and facts()\n");
    std::memset(&summary, 0, sizeof(summary));
    summary.modules = 1u;
    summary.tests   = 2u;
    summary.checks  = 9u;
    D_TESTS_CHECK(&units<dj_test_summary>::type() ==
                  &d_desc_type_dj_test_summary);
    D_TESTS_CHECK(std::strcmp(type_name<dj_test_case_spec>::text(),
                              "test") == 0);
    for (ring = D_DESC_RING_NAME; ring <= D_DESC_RING_DETAILED; ++ring)
    {
        D_TESTS_CHECK(describe(summary, ring, D_DESC_LAYERS_ALL) ==
                      d_tests_said(D_DESC_OF(dj_test_summary), &summary,
                                   ring));
        D_TESTS_CHECK(describe(module, ring, D_DESC_LAYERS_ALL) ==
                      d_tests_said(D_DESC_OF(dj_test_module_spec), &module,
                                   ring));
    }
#if D_CFG_DESC_UNITS && D_CFG_DESC_ARTICLES && (D_CFG_DESC_RING_MAX >= 1)
    D_TESTS_CHECK(describe(summary, D_DESC_RING_LINE, D_DESC_LAYERS_ALL) ==
                  "a passing test summary with 2 tests and 9 checks");
#endif
#if D_CFG_DESC_FACTS
    {
        static const struct dj_desc_machine machine =
            { NULL, d_tests_count_fact, NULL, NULL, NULL, NULL };
        unsigned long       count = 0u;
        struct dj_desc_sink sink;

        // the module's type and four fields; the block's type and three
        // fields, a level down; the block's one test, a level further
        d_desc_sink_init(&sink, &machine, &count);
        D_TESTS_CHECK(djinterp::desc::facts(module, &sink) == 0);
        D_TESTS_CHECK(count == 10u);
    }
#endif
    D_TESTS_CHECK(d_desc_of_test_agrees(NULL) == 1);

    return;
}
#endif  // D_INTERNAL_DESC_OF_TEST

int
main()
{
    std::printf("t_desc_of (C++%ld)\n", static_cast<long>(__cplusplus));

#if D_INTERNAL_DESC_OF_FILESYS
    d_tests_filesys();
#endif
#if D_INTERNAL_DESC_OF_TEST
    d_tests_dtest();
#endif
    (void)d_tests_said;
    (void)d_tests_pass;
    (void)d_tests_count_fact;

    std::printf("%lu checks, %lu failed\n", g_checks, g_failures);

    return (g_failures == 0u) ? 0 : 1;
}
