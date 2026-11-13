/*******************************************************************************
* djinterp [test]                                               t_desc_units.cpp
*
*   desc's C++ face's harness (cpp/desc/desc_units.hpp), at every level from
* C++98: a template container whose units its parameters choose -- its
* element's name from type_name, "fixed-size" kept by pick only where the
* type is fixed -- described in each ring, to a std::string and to a put,
* and its facts from its field list. A standalone program, as the other tests
* not yet in DTest are: it prints a line per group and one per failed check,
* ends with "N checks, M failed", and exits 0 only when nothing failed.
*
*
* path:      /tests/unintegrated/cpp/desc/t_desc_units.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

// std
#include <cstdio>   // std::printf
#include <cstring>  // std::strcmp
#include <string>   // std::string
// djinterp
#include "../../../../inc/djinterp/cpp/desc/desc_units.hpp"  // the face


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


// bag
//   struct: the toy container: up to 4 elements; Fixed says whether its
// size is fixed.
template<typename T, bool Fixed>
struct bag
{
    T           items[4];
    std::size_t count;
};


NS_DJINTERP
D_NAMESPACE(desc)

D_DESC_LITERAL(d_tests_fixed_size, "fixed-size");

// units<bag<T, Fixed>>
//   specialization: the bag's description, chosen by its parameters.
template<typename T, bool Fixed>
struct units< bag<T, Fixed> >
{
    static struct dj_ostream_value
    read(const void* _object, uint32_t _field)
    {
        const bag<T, Fixed>* object =
            static_cast<const bag<T, Fixed>*>(_object);

        return (_field == 0u) ? d_ostream_value_size(object->count)
                              : d_ostream_value_none();
    }

    static uint32_t
    plan(const void* _object, uint32_t _ring)
    {
        (void)_ring;

        return (static_cast<const bag<T, Fixed>*>(_object)->count == 0u)
                   ? 1u : 0u;
    }

    static const struct dj_desc_type&
    type()
    {
        static const struct dj_desc_op size[] =
            { D_DESC_T((pick<Fixed, d_tests_fixed_size>::text())),
              D_DESC_END };
        static const struct dj_desc_op noun[] =
            { D_DESC_T("bag"), D_DESC_END };
        static const struct dj_desc_op of[] =
            { D_DESC_T("of "), D_DESC_T(type_name<T>::text()), D_DESC_END };
        static const struct dj_desc_op items[] =
            { D_DESC_F(0u), D_DESC_T(" item"), D_DESC_PL, D_DESC_END };
        static const struct dj_desc_unit unit_table[] =
        {
            D_DESC_UNIT(D_DESC_RING_IF(Fixed, D_DESC_RING_LINE),
                        D_DESC_LAYER_POLICY, D_DESC_SLOT_SIZE, 0u, 0u, size),
            D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE,
                        D_DESC_SLOT_NOUN, 0u, 0u, noun),
            D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE,
                        D_DESC_SLOT_COMPLEMENT, 0u, 0u, of),
            D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE,
                        D_DESC_SLOT_WITH, 0u, 1u, items)
        };
        static const struct dj_desc_field field_table[] =
        {
            D_DESC_FIELD("item_count", "items", D_DESC_LAYER_STATE, 0u,
                         D_DESC_TRUTH_OBSERVED)
        };
        static const struct dj_desc_type description =
        {
            "bag", "bag",
            D_DESC_UNITS_TABLE(unit_table),
            D_DESC_FIELDS_TABLE(field_table),
            plan, read, NULL
        };

        return description;
    }
};

NS_END  // desc
NS_END  // djinterp


// d_tests_count_fact
//   function: a machine's fact that counts facts and keeps the last value.
static int
d_tests_count_fact(
    void*                      _context,
    const struct dj_desc_fact* _fact
)
{
    unsigned long* counts = static_cast<unsigned long*>(_context);

    ++counts[0];
    counts[1] = _fact->value.low;

    return 0;
}

int
main()
{
    using djinterp::desc::describe;

    bag<int, true>          fixed;
    bag<std::string, false> loose;
    std::string             text;
    struct dj_desc_buffer   buffer;
    char                    room[64];

    std::printf("t_desc_units (C++%ld)\n", static_cast<long>(__cplusplus));
    fixed.count = 2u;
    loose.count = 1u;

    std::printf("  rings, chosen by the type\n");
#if D_DESC_BUILT(LINE, POLICY) && D_CFG_DESC_ARTICLES
    D_TESTS_CHECK(describe(fixed, D_DESC_RING_NAME, D_DESC_LAYERS_ALL) ==
                  "bag of int");
    D_TESTS_CHECK(describe(fixed, D_DESC_RING_LINE, D_DESC_LAYERS_ALL) ==
                  "a fixed-size bag of int, with 2 items");
    D_TESTS_CHECK(describe(loose, D_DESC_RING_LINE, D_DESC_LAYERS_ALL,
                           D_DESC_CAPITAL) ==
                  "A bag of string, with 1 item");
    D_TESTS_CHECK((djinterp::desc::pick<false,
                       djinterp::desc::d_tests_fixed_size>::text() == NULL));
    loose.count = 0u;
    D_TESTS_CHECK(describe(loose, D_DESC_RING_DETAILED, D_DESC_LAYERS_ALL) ==
                  "a bag of string");
#endif

    std::printf("  to a put, and facts\n");
    d_desc_buffer_init(&buffer, room, sizeof(room));
    D_TESTS_CHECK(djinterp::desc::say(fixed, d_desc_put_buffer, &buffer,
                                      D_DESC_RING_NAME) ==
                  ((D_CFG_DESC_UNITS != 0) ? 0 : -1));
#if D_CFG_DESC_UNITS
    D_TESTS_CHECK(std::strcmp(room, "bag of int") == 0);
#endif
#if D_CFG_DESC_FACTS
    {
        static const struct dj_desc_machine machine =
            { NULL, d_tests_count_fact, NULL, NULL, NULL, NULL };
        unsigned long       counts[2] = { 0u, 0u };
        struct dj_desc_sink sink;

        d_desc_sink_init(&sink, &machine, counts);
        D_TESTS_CHECK(djinterp::desc::facts(fixed, &sink) == 0);
        D_TESTS_CHECK((counts[0] == 2u) && (counts[1] == 2u));
    }
#endif
    (void)text;

    std::printf("%lu checks, %lu failed\n", g_checks, g_failures);

    return (g_failures == 0u) ? 0 : 1;
}
