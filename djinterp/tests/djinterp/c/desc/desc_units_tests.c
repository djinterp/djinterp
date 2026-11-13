/*******************************************************************************
* djinterp [test]                                             desc_units_tests.c
*
*   The suite of c/desc's units engine, its derived facts, its djin and its
* macro layer, in DTest's C face. A toy type, a shelf of books, is described
* in every ring; its reader counts its reads, so a test can show that a field
* nobody asked for is never read, and that the article's read is the text's.
* Expectations follow the knobs a build has, so the suite runs under each.
*
*
* path:      /tests/djinterp/c/desc/desc_units_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#include "./desc_tests.h"  // corresponding header

// std
#include <string.h>  // memset, strcmp, strncmp, strstr
// djinterp
#include "../../../../inc/djinterp/c/desc/desc_units.h"    // the engine
#include "../../../../inc/djinterp/c/desc/desc_options.h"  // the djin
#include "../../../../inc/djinterp/c/desc/desc_macro.h"    // the macro layer


//==============================================================================
// THE SHELF
//==============================================================================

// dj_tests_shelf
//   struct: the toy type: up to 8 books, and what the plan reads.
struct dj_tests_shelf
{
    const char* books[8];
    size_t      count;
    double      weight;    // kilograms
    uint32_t    ordered;
    uint32_t    labelled;
    uint32_t    dusty;
    uint32_t    old;
};

// the fields, in the field list's order
#define D_TESTS_SHELF_BOOKS     0u
#define D_TESTS_SHELF_FIRST     1u
#define D_TESTS_SHELF_CAPACITY  2u
#define D_TESTS_SHELF_WEIGHT    3u
#define D_TESTS_SHELF_FIELDS    4u

// the plan's bits
#define D_TESTS_SHELF_EMPTY     (1u << 0)
#define D_TESTS_SHELF_FULL      (1u << 1)
#define D_TESTS_SHELF_ORDERED   (1u << 2)
#define D_TESTS_SHELF_LABELLED  (1u << 3)
#define D_TESTS_SHELF_DUSTY     (1u << 4)
#define D_TESTS_SHELF_OLD       (1u << 5)

// each field's reads since the last reset
static unsigned d_tests_shelf_reads[D_TESTS_SHELF_FIELDS];

/*
d_tests_shelf_plan
  Reads only values, each when its ring is in force.
*/
static uint32_t
d_tests_shelf_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_tests_shelf* shelf = (const struct dj_tests_shelf*)_object;
    uint32_t                     bits  = 0u;

    if (_ring >= D_DESC_RING_LINE)
    {
        bits |= (shelf->count == 0u) ? D_TESTS_SHELF_EMPTY : 0u;
        bits |= (shelf->count == 8u) ? D_TESTS_SHELF_FULL : 0u;
        bits |= (shelf->labelled != 0u) ? D_TESTS_SHELF_LABELLED : 0u;
        bits |= (shelf->dusty != 0u) ? D_TESTS_SHELF_DUSTY : 0u;
        bits |= (shelf->old != 0u) ? D_TESTS_SHELF_OLD : 0u;
    }

    if ( (_ring >= D_DESC_RING_SENTENCE) && (shelf->ordered != 0u) )
    {
        bits |= D_TESTS_SHELF_ORDERED;
    }

    return bits;
}

/*
d_tests_shelf_read
  Reads one field, and counts the read.
*/
static struct dj_ostream_value
d_tests_shelf_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_tests_shelf* shelf = (const struct dj_tests_shelf*)_object;

    if (_field < D_TESTS_SHELF_FIELDS)
    {
        ++d_tests_shelf_reads[_field];
    }

    switch (_field)
    {
        case D_TESTS_SHELF_BOOKS:
            return d_ostream_value_size(shelf->count);
        case D_TESTS_SHELF_FIRST:
            return (shelf->count != 0u) ? d_ostream_value_text(shelf->books[0])
                                        : d_ostream_value_none();
        case D_TESTS_SHELF_CAPACITY:
            return d_ostream_value_ulong(8u);
        case D_TESTS_SHELF_WEIGHT:
            return d_ostream_value_real(shelf->weight);
        default:
            return d_ostream_value_none();
    }
}

/*
d_tests_shelf_contents
  The hand-written part: each book's title, one level down.
*/
static int
d_tests_shelf_contents(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    const struct dj_tests_shelf* shelf = (const struct dj_tests_shelf*)_object;
    int                          stopped = 0;
    size_t                       index;

    if (d_desc_enter(_sink, "books", "books") == 0)
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    for (index = 0u; (index < shelf->count) && (stopped == 0); ++index)
    {
        stopped = d_desc_emit(_sink, "title", NULL,
                              d_ostream_value_text(shelf->books[index]),
                              D_DESC_LAYER_CONTENTS, 0u,
                              D_DESC_TRUTH_OBSERVED);
    }

    d_desc_leave(_sink);

    return stopped;
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
// the op arrays, shared by pointer, each built where its unit is
#if D_DESC_BUILT(LINE, STATE)
static const struct dj_desc_op D_TESTS_OPS_EMPTY[] =
    { D_DESC_T("empty"), D_DESC_END };
static const struct dj_desc_op D_TESTS_OPS_FULL[] =
    { D_DESC_T("full"), D_DESC_END };
static const struct dj_desc_op D_TESTS_OPS_DUSTY[] =
    { D_DESC_T("dusty"), D_DESC_END };
static const struct dj_desc_op D_TESTS_OPS_OLD[] =
    { D_DESC_T("old"), D_DESC_END };
static const struct dj_desc_op D_TESTS_OPS_BOOKS[] =
    { D_DESC_F(D_TESTS_SHELF_BOOKS), D_DESC_T(" book"), D_DESC_PL,
      D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, POLICY)
static const struct dj_desc_op D_TESTS_OPS_SLOTS[] =
    { D_DESC_F(D_TESTS_SHELF_CAPACITY), D_DESC_T("-slot"), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
static const struct dj_desc_op D_TESTS_OPS_ORDERED[] =
    { D_DESC_T("ordered"), D_DESC_END };
#endif
static const struct dj_desc_op D_TESTS_OPS_NOUN[] =
    { D_DESC_T("shelf"), D_DESC_END };
static const struct dj_desc_op D_TESTS_OPS_NEVER[] =
    { D_DESC_T("Never said."), D_DESC_END };
#if D_DESC_BUILT(SENTENCE, TYPE)
static const struct dj_desc_op D_TESTS_OPS_OF[] =
    { D_DESC_T("of books"), D_DESC_END };
#endif
#if D_DESC_BUILT(LINE, IDENTITY)
static const struct dj_desc_op D_TESTS_OPS_LABEL[] =
    { D_DESC_T("a label"), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, IDENTITY)
static const struct dj_desc_op D_TESTS_OPS_FIRST[] =
    { D_DESC_T("Its first book is "), D_DESC_F(D_TESTS_SHELF_FIRST),
      D_DESC_T("."), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, COST)
static const struct dj_desc_op D_TESTS_OPS_WEIGHT[] =
    { D_DESC_T("It weighs "), D_DESC_F(D_TESTS_SHELF_WEIGHT),
      D_DESC_T(" kilogram"), D_DESC_PL, D_DESC_T("."), D_DESC_END };
static const struct dj_desc_op D_TESTS_OPS_HOLDS[] =
    { D_DESC_T("It holds at most 8 books."), D_DESC_END };
#endif

// the units, each only where the build has its ring and layer
static const struct dj_desc_unit d_tests_shelf_units[] =
{
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_TESTS_SHELF_EMPTY, 0u, D_TESTS_OPS_EMPTY),
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_TESTS_SHELF_FULL, 0u, D_TESTS_OPS_FULL),
#endif
#if D_DESC_BUILT(DETAILED, POLICY)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_POLICY, D_DESC_SLOT_SIZE,
                0u, 0u, D_TESTS_OPS_SLOTS),
#endif
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_QUALITY,
                D_TESTS_SHELF_DUSTY, 0u, D_TESTS_OPS_DUSTY),
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_QUALITY,
                D_TESTS_SHELF_OLD, 0u, D_TESTS_OPS_OLD),
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY, D_DESC_SLOT_SHAPE,
                D_TESTS_SHELF_ORDERED, 0u, D_TESTS_OPS_ORDERED),
#endif
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_TESTS_OPS_NOUN),
    D_DESC_UNIT(D_DESC_RING_NEVER, D_DESC_LAYER_TYPE, D_DESC_SLOT_REMARK,
                0u, 0u, D_TESTS_OPS_NEVER),
#if D_DESC_BUILT(SENTENCE, TYPE)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_TYPE,
                D_DESC_SLOT_COMPLEMENT, 0u, 0u, D_TESTS_OPS_OF),
#endif
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                0u, D_TESTS_SHELF_EMPTY, D_TESTS_OPS_BOOKS),
#endif
#if D_DESC_BUILT(LINE, IDENTITY)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_IDENTITY, D_DESC_SLOT_WITH,
                D_TESTS_SHELF_LABELLED, 0u, D_TESTS_OPS_LABEL),
#endif
#if D_DESC_BUILT(SENTENCE, IDENTITY)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_IDENTITY,
                D_DESC_SLOT_REMARK, 0u, D_TESTS_SHELF_EMPTY,
                D_TESTS_OPS_FIRST),
#endif
#if D_DESC_BUILT(DETAILED, COST)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_COST, D_DESC_SLOT_REMARK,
                0u, D_TESTS_SHELF_EMPTY, D_TESTS_OPS_WEIGHT),
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_COST, D_DESC_SLOT_REMARK,
                0u, 0u, D_TESTS_OPS_HOLDS),
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
// the field list
static const struct dj_desc_field d_tests_shelf_fields[] =
{
    D_DESC_FIELD("book_count", "books", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("first", "first book", D_DESC_LAYER_IDENTITY, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("capacity", "slots", D_DESC_LAYER_COST, 2u,
                 D_DESC_TRUTH_BUILD),
    D_DESC_FIELD("weight", "kilograms", D_DESC_LAYER_COST, 3u,
                 D_DESC_TRUTH_OBSERVED)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

// d_desc_type_dj_tests_shelf
//   constant: the shelf's description.
static const struct dj_desc_type d_desc_type_dj_tests_shelf =
{
    "shelf",
    "shelf",
    D_DESC_UNITS_TABLE(d_tests_shelf_units),
    D_DESC_FIELDS_TABLE(d_tests_shelf_fields),
    d_tests_shelf_plan,
    d_tests_shelf_read,
    d_tests_shelf_contents
};

/*
d_tests_shelf_fill
  A shelf of `_count` books from a list of eight, weighing 2.5 kg.
*/
static void
d_tests_shelf_fill(
    struct dj_tests_shelf* _shelf,
    size_t                 _count
)
{
    static const char* const titles[8] =
    {
        "Emma", "Persuasion", "Ulysses", "Middlemarch",
        "Beloved", "Dubliners", "Kindred", "Orlando"
    };
    size_t index;

    memset(_shelf, 0, sizeof(*_shelf));
    _shelf->count  = _count;
    _shelf->weight = 2.5;

    for (index = 0u; index < _count; ++index)
    {
        _shelf->books[index] = titles[index];
    }

    return;
}

/*
d_tests_shelf_say
  The shelf's description into a buffer, the read counts reset first.
*/
static int
d_tests_shelf_say(
    const struct dj_tests_shelf* _shelf,
    uint32_t                     _ring,
    uint32_t                     _layers,
    uint32_t                     _flags,
    char*                        _text,
    size_t                       _capacity
)
{
    struct dj_desc_buffer buffer;

    memset(d_tests_shelf_reads, 0, sizeof(d_tests_shelf_reads));
    d_desc_buffer_init(&buffer, _text, _capacity);

    return d_desc_say(D_DESC_OF(dj_tests_shelf), _shelf, _ring, _layers,
                      _flags, d_desc_put_buffer, &buffer);
}

// what the suite expects of a build: the default build's text, with or
// without articles
#define D_TESTS_DESC_FULL_BUILD                                                \
    ( D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS && (D_CFG_DESC_RING_MAX == 3) && \
      (D_CFG_DESC_LAYERS_BUILT == 0x1FF) )

#if D_CFG_DESC_ARTICLES
    #define D_TESTS_A   "a "
    #define D_TESTS_AN  "an "
#else
    #define D_TESTS_A   ""
    #define D_TESTS_AN  ""
#endif


//==============================================================================
// THE TESTS
//==============================================================================

/*
d_tests_desc_units_rings
  Checks the following:
  - each ring says more than the one inside it, in a sentence's order
  - the name ring has no article, and the noun alone
  - a ring past the last built is the last built, and a unit no ring
    shows is never shown
*/
static void
d_tests_desc_units_rings(
    struct dj_test_suite* _suite
)
{
#if D_TESTS_DESC_FULL_BUILD
    struct dj_tests_shelf shelf;
    char                  text[256];

    (void)d_test_suite_test(_suite, "each ring, inside out");
    d_tests_shelf_fill(&shelf, 3u);
    shelf.ordered = 1u;

    D_TEST_CHECK(_suite, d_tests_shelf_say(&shelf, D_DESC_RING_NAME,
                                           D_DESC_LAYERS_ALL, 0u, text,
                                           sizeof(text)) == 0);
    D_TEST_CHECK(_suite, strcmp(text, "shelf") == 0);
    (void)d_tests_shelf_say(&shelf, D_DESC_RING_LINE, D_DESC_LAYERS_ALL, 0u,
                            text, sizeof(text));
    D_TEST_CHECK(_suite, strcmp(text, D_TESTS_A "shelf with 3 books") == 0);
    (void)d_tests_shelf_say(&shelf, D_DESC_RING_SENTENCE, D_DESC_LAYERS_ALL,
                            0u, text, sizeof(text));
    D_TEST_CHECK(_suite, strcmp(text, D_TESTS_AN "ordered shelf of books, "
                                "with 3 books. Its first book is Emma.") == 0);
    (void)d_tests_shelf_say(&shelf, D_DESC_RING_DETAILED, D_DESC_LAYERS_ALL,
                            0u, text, sizeof(text));
    D_TEST_CHECK(_suite, strcmp(text, D_TESTS_AN "8-slot ordered shelf of "
                                "books, with 3 books. Its first book is "
                                "Emma. It weighs 2.5 kilograms. It holds at "
                                "most 8 books.") == 0);
    (void)d_tests_shelf_say(&shelf, 0xFFFFFFFFu, D_DESC_LAYERS_ALL, 0u, text,
                            sizeof(text));
    D_TEST_CHECK(_suite, (strncmp(text, D_TESTS_AN "8-slot", 6u) == 0) &&
                         (strstr(text, "Never") == NULL));
#elif D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
    struct dj_tests_shelf shelf;
    char                  text[256];
    char                  last[256];

    (void)d_test_suite_test(_suite, "a ring past the build's is its last");
    d_tests_shelf_fill(&shelf, 3u);
    shelf.ordered = 1u;
    (void)d_tests_shelf_say(&shelf, D_DESC_RING_DETAILED, D_DESC_LAYERS_ALL,
                            0u, text, sizeof(text));
    (void)d_tests_shelf_say(&shelf, (uint32_t)D_CFG_DESC_RING_MAX,
                            D_DESC_LAYERS_ALL, 0u, last, sizeof(last));
    D_TEST_CHECK(_suite, strcmp(text, last) == 0);
#else
    (void)_suite;
#endif

    return;
}

/*
d_tests_desc_units_grammar
  Checks the following:
  - a count of 1 takes the singular, of 0 the plan's "empty"
  - adjectives of one kind take a comma; of different kinds, a space
  - the units of WITH join with ", " and a last " and "
  - an article follows the first word: "an" before a vowel or an 8
  - D_DESC_CAPITAL capitalises the first letter
*/
static void
d_tests_desc_units_grammar(
    struct dj_test_suite* _suite
)
{
#if D_TESTS_DESC_FULL_BUILD
    struct dj_tests_shelf shelf;
    char                  text[256];

    (void)d_test_suite_test(_suite, "numbers, commas, articles, capitals");
    d_tests_shelf_fill(&shelf, 1u);
    shelf.labelled = 1u;
    (void)d_tests_shelf_say(&shelf, D_DESC_RING_LINE, D_DESC_LAYERS_ALL, 0u,
                            text, sizeof(text));
    D_TEST_CHECK(_suite, strcmp(text, D_TESTS_A "shelf with 1 book and a "
                                "label") == 0);

    d_tests_shelf_fill(&shelf, 0u);
    (void)d_tests_shelf_say(&shelf, D_DESC_RING_LINE, D_DESC_LAYERS_ALL, 0u,
                            text, sizeof(text));
    D_TEST_CHECK(_suite, strcmp(text, D_TESTS_AN "empty shelf") == 0);

    d_tests_shelf_fill(&shelf, 8u);
    shelf.dusty = 1u;
    shelf.old   = 1u;
    (void)d_tests_shelf_say(&shelf, D_DESC_RING_LINE, D_DESC_LAYERS_ALL, 0u,
                            text, sizeof(text));
    D_TEST_CHECK(_suite, strcmp(text, D_TESTS_A "full dusty, old shelf with "
                                "8 books") == 0);

    d_tests_shelf_fill(&shelf, 3u);
    shelf.weight = 1.0;
    (void)d_tests_shelf_say(&shelf, D_DESC_RING_DETAILED, D_DESC_LAYERS_ALL,
                            D_DESC_CAPITAL, text, sizeof(text));
    D_TEST_CHECK(_suite, strcmp(text, D_CFG_DESC_ARTICLES ? "An 8-slot shelf "
                                "of books, with 3 books. Its first book is "
                                "Emma. It weighs 1 kilogram. It holds at most "
                                "8 books." : "8-slot shelf of books, with 3 "
                                "books. Its first book is Emma. It weighs 1 "
                                "kilogram. It holds at most 8 books.") == 0);
#else
    (void)_suite;
#endif

    return;
}

/*
d_tests_desc_units_layers
  Checks the following:
  - a layer not wanted hides its units; the noun shows whatever the
    layers, and its complement, of the type layer, does not
  - a field is read only when a unit that shows names it, and the first
    unit's field once, for its article and its text
*/
static void
d_tests_desc_units_layers(
    struct dj_test_suite* _suite
)
{
#if D_TESTS_DESC_FULL_BUILD
    struct dj_tests_shelf shelf;
    char                  text[256];
    const uint32_t        no_policy = D_DESC_LAYERS_ALL &
                                      ~((uint32_t)1u << D_DESC_LAYER_POLICY);

    (void)d_test_suite_test(_suite, "layers, and lazy reads");
    d_tests_shelf_fill(&shelf, 3u);
    shelf.ordered = 1u;
    (void)d_tests_shelf_say(&shelf, D_DESC_RING_DETAILED, no_policy, 0u,
                            text, sizeof(text));
    D_TEST_CHECK(_suite, strcmp(text, D_TESTS_A "shelf of books, with 3 "
                                "books. Its first book is Emma. It weighs "
                                "2.5 kilograms. It holds at most 8 "
                                "books.") == 0);
    D_TEST_CHECK(_suite, d_tests_shelf_reads[D_TESTS_SHELF_CAPACITY] == 0u);

    (void)d_tests_shelf_say(&shelf, D_DESC_RING_LINE,
                            (uint32_t)1u << D_DESC_LAYER_TYPE, 0u, text,
                            sizeof(text));
    D_TEST_CHECK(_suite, strcmp(text, D_TESTS_A "shelf") == 0);
    D_TEST_CHECK(_suite, (d_tests_shelf_reads[D_TESTS_SHELF_BOOKS] == 0u) &&
                         (d_tests_shelf_reads[D_TESTS_SHELF_WEIGHT] == 0u));
    (void)d_tests_shelf_say(&shelf, D_DESC_RING_SENTENCE,
                            (uint32_t)1u << D_DESC_LAYER_STATE, 0u, text,
                            sizeof(text));
    D_TEST_CHECK(_suite, strcmp(text, D_TESTS_A "shelf with 3 books") == 0);

    (void)d_tests_shelf_say(&shelf, D_DESC_RING_DETAILED, D_DESC_LAYERS_ALL,
                            0u, text, sizeof(text));
    D_TEST_CHECK(_suite,
                 (d_tests_shelf_reads[D_TESTS_SHELF_CAPACITY] == 1u) &&
                 (d_tests_shelf_reads[D_TESTS_SHELF_WEIGHT] == 1u) &&
                 (d_tests_shelf_reads[D_TESTS_SHELF_BOOKS] == 1u));
#else
    (void)_suite;
#endif

    return;
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
/*
d_tests_desc_put_refuse
  A put that counts its calls and asks to stop at once.
*/
static int
d_tests_desc_put_refuse(
    void*       _context,
    const char* _text,
    size_t      _size
)
{
    (void)_text;
    (void)_size;
    ++*(unsigned*)_context;

    return 1;
}
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

/*
d_tests_desc_units_stop
  Checks the following:
  - a put that asks to stop stops the description: d_desc_say says 1,
    and the put is not called again, not even after a capital letter
  - a buffer keeps what fit, and a NUL after it
  - a null type, object or put is refused with -1
*/
static void
d_tests_desc_units_stop(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_shelf shelf;
    struct dj_desc_buffer buffer;
    char                  text[8];

    (void)d_test_suite_test(_suite, "a put that stops, and null arguments");
    d_tests_shelf_fill(&shelf, 3u);
#if D_TESTS_DESC_FULL_BUILD && D_CFG_DESC_ARTICLES
    D_TEST_CHECK(_suite, d_tests_shelf_say(&shelf, D_DESC_RING_LINE,
                                           D_DESC_LAYERS_ALL, 0u, text,
                                           sizeof(text)) == 1);
    D_TEST_CHECK(_suite, strcmp(text, "a shelf") == 0);
#endif
#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
    {
        unsigned calls = 0u;

        D_TEST_CHECK(_suite, d_desc_say(D_DESC_OF(dj_tests_shelf), &shelf,
                                        D_DESC_RING_NAME, D_DESC_LAYERS_ALL,
                                        D_DESC_CAPITAL,
                                        d_tests_desc_put_refuse,
                                        &calls) == 1);
        D_TEST_CHECK(_suite, calls == 1u);
    }
#endif
#if D_TESTS_DESC_FULL_BUILD && D_CFG_DESC_ARTICLES
#elif !(D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS)
    D_TEST_CHECK(_suite, d_tests_shelf_say(&shelf, D_DESC_RING_LINE,
                                           D_DESC_LAYERS_ALL, 0u, text,
                                           sizeof(text)) == -1);
#else
    D_TEST_CHECK(_suite, d_tests_shelf_say(&shelf, D_DESC_RING_NAME,
                                           D_DESC_LAYERS_ALL, 0u, text,
                                           sizeof(text)) == 0);
    D_TEST_CHECK(_suite, strcmp(text, "shelf") == 0);
#endif
    d_desc_buffer_init(&buffer, text, sizeof(text));
    D_TEST_CHECK(_suite, d_desc_say(NULL, &shelf, 0u, 0u, 0u,
                                    d_desc_put_buffer, &buffer) == -1);
    D_TEST_CHECK(_suite, d_desc_say(D_DESC_OF(dj_tests_shelf), NULL, 0u, 0u,
                                    0u, d_desc_put_buffer, &buffer) == -1);
    D_TEST_CHECK(_suite, d_desc_say(D_DESC_OF(dj_tests_shelf), &shelf, 0u,
                                    0u, 0u, NULL, &buffer) == -1);
    D_TEST_CHECK(_suite, (buffer.size == 0u) && (text[0] == '\0'));

    return;
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

// dj_tests_desc_keep
//   struct: a machine's context that keeps the facts it is sent.
struct dj_tests_desc_keep
{
    struct dj_desc_fact facts[16];
    unsigned            count;
    unsigned            stop_at;   // 0: never
    unsigned            begins;
    unsigned            ends;
};

/*
d_tests_desc_keep_fact / _begin / _end
  The keeping machine's operations.
*/
static int
d_tests_desc_keep_fact(
    void*                      _context,
    const struct dj_desc_fact* _fact
)
{
    struct dj_tests_desc_keep* keep = (struct dj_tests_desc_keep*)_context;

    if (keep->count < 16u)
    {
        keep->facts[keep->count] = *_fact;
    }

    ++keep->count;

    return ( (keep->stop_at != 0u) && (keep->count >= keep->stop_at) ) ? 1
                                                                       : 0;
}

static int
d_tests_desc_keep_begin(
    void* _context
)
{
    ++((struct dj_tests_desc_keep*)_context)->begins;

    return 0;
}

static void
d_tests_desc_keep_end(
    void* _context
)
{
    ++((struct dj_tests_desc_keep*)_context)->ends;

    return;
}

static const struct dj_desc_machine d_tests_desc_keeping =
{
    d_tests_desc_keep_begin, d_tests_desc_keep_fact, NULL, NULL, NULL,
    d_tests_desc_keep_end
};

#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

/*
d_tests_desc_units_facts
  Checks the following:
  - the facts are "type", then each field in the list's order, then the
    contents a level down
  - a sink that wants one layer gets that layer's facts, and the other
    fields are not read
  - d_desc_run_facts calls begin and end; a stopped sink says 1
*/
static void
d_tests_desc_units_facts(
    struct dj_test_suite* _suite
)
{
#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
    struct dj_tests_shelf     shelf;
    struct dj_tests_desc_keep keep;
    struct dj_desc_sink       sink;

    (void)d_test_suite_test(_suite, "facts from the field list");
    d_tests_shelf_fill(&shelf, 2u);
    memset(&keep, 0, sizeof(keep));
    d_desc_sink_init(&sink, &d_tests_desc_keeping, &keep);
    D_TEST_CHECK(_suite, d_desc_run_facts(&sink, D_DESC_OF(dj_tests_shelf),
                                          &shelf) == 0);
    D_TEST_CHECK(_suite, (keep.count == 7u) && (keep.begins == 1u) &&
                         (keep.ends == 1u));
    D_TEST_CHECK(_suite, (strcmp(keep.facts[0].key, "type") == 0) &&
                         (strcmp(keep.facts[1].key, "book_count") == 0) &&
                         (keep.facts[1].value.low == 2u) &&
                         (strcmp(keep.facts[4].key, "weight") == 0) &&
                         (keep.facts[4].value.real == 2.5));
    D_TEST_CHECK(_suite, (strcmp(keep.facts[6].key, "title") == 0) &&
                         (keep.facts[6].depth == 1u) &&
                         (strcmp(keep.facts[6].value.text,
                                 "Persuasion") == 0));

    memset(&keep, 0, sizeof(keep));
    memset(d_tests_shelf_reads, 0, sizeof(d_tests_shelf_reads));
    d_desc_sink_init(&sink, &d_tests_desc_keeping, &keep);
    sink.layers = (uint32_t)1u << D_DESC_LAYER_STATE;
    D_TEST_CHECK(_suite, d_desc_facts(D_DESC_OF(dj_tests_shelf), &shelf,
                                      &sink) == 0);
    D_TEST_CHECK(_suite, (keep.count == 1u) &&
                         (d_tests_shelf_reads[D_TESTS_SHELF_WEIGHT] == 0u) &&
                         (d_tests_shelf_reads[D_TESTS_SHELF_FIRST] == 0u));

    memset(&keep, 0, sizeof(keep));
    keep.stop_at = 2u;
    d_desc_sink_init(&sink, &d_tests_desc_keeping, &keep);
    D_TEST_CHECK(_suite, d_desc_facts(D_DESC_OF(dj_tests_shelf), &shelf,
                                      &sink) == 1);
    D_TEST_CHECK(_suite, (keep.count == 2u) &&
                         (d_desc_facts(NULL, &shelf, &sink) == -1));
#else
    D_TEST_CHECK(_suite, d_desc_facts(D_DESC_OF(dj_tests_shelf), _suite,
                                      NULL) == -1);
#endif

    return;
}

/*
d_tests_desc_units_options
  Checks the following:
  - without settings, the ring and layers are the knobs'
  - a type declares once, by an identifier
  - desc.ring and desc.layers set through a registry reach d_desc_say, and
    desc.<type>.ring sets one declared type's ring alone
  - a value no ring has is refused
*/
static void
d_tests_desc_units_options(
    struct dj_test_suite* _suite
)
{
    static const struct dj_desc_type unnamed = { "2nd", "x", NULL, 0u, NULL,
                                                 0u, NULL, NULL, NULL };

    (void)d_test_suite_test(_suite, "desc's djin");
    D_TEST_CHECK(_suite,
                 (d_desc_option_ring(NULL) == (uint32_t)D_CFG_DESC_RING) &&
                 (d_desc_option_layers() == (uint32_t)D_CFG_DESC_LAYERS));
    D_TEST_CHECK(_suite, (d_desc_declare(D_DESC_OF(dj_tests_shelf)) == 0) &&
                         (d_desc_declare(D_DESC_OF(dj_tests_shelf)) == 0));
#if D_DESC_OPTIONS_BUILT
    {
        static struct dj_option_env_registry registry;
        const int32_t source = D_OPTION_ENV_SOURCE_COMMAND_LINE;

        D_TEST_CHECK(_suite, (d_desc_declare(&unnamed) == -1) &&
                             (d_desc_declare(NULL) == -1));
        (void)d_option_env_registry_init(&registry, "tests");
        D_TEST_CHECK(_suite, d_desc_register(&registry).is_ok);
        D_TEST_CHECK(_suite, d_option_env_registry_put(&registry,
                         "desc.ring=sentence", source).is_ok);
        D_TEST_CHECK(_suite, d_option_env_registry_put(&registry,
                         "desc.shelf.ring=name", source).is_ok);
        D_TEST_CHECK(_suite, d_option_env_registry_put(&registry,
                         "desc.layers=identity|state", source).is_ok);
        D_TEST_CHECK(_suite,
                     (d_desc_option_ring(NULL) == D_DESC_RING_SENTENCE) &&
                     (d_desc_option_ring(D_DESC_OF(dj_tests_shelf)) ==
                      D_DESC_RING_NAME));
        D_TEST_CHECK(_suite, d_desc_option_layers() ==
                             (((uint32_t)1u << D_DESC_LAYER_IDENTITY) |
                              ((uint32_t)1u << D_DESC_LAYER_STATE)));
        D_TEST_CHECK(_suite, !d_option_env_registry_put(&registry,
                         "desc.ring=huge", source).is_ok);
        d_option_env_clear(d_desc_djin());
        (void)d_option_env_registry_remove(&registry, d_desc_djin());
        D_TEST_CHECK(_suite, d_desc_option_ring(D_DESC_OF(dj_tests_shelf)) ==
                             (uint32_t)D_CFG_DESC_RING);
    }
#else
    (void)unnamed;
#endif

    return;
}

/*
d_tests_desc_units_say_options
  Checks the following:
  - D_DESC_RING_OPTION and D_DESC_LAYERS_OPTION take what the djin says
*/
static void
d_tests_desc_units_say_options(
    struct dj_test_suite* _suite
)
{
#if D_TESTS_DESC_FULL_BUILD && D_DESC_OPTIONS_BUILT
    struct dj_tests_shelf shelf;
    char                  text[256];

    (void)d_test_suite_test(_suite, "describing as the options say");
    d_tests_shelf_fill(&shelf, 3u);
    shelf.ordered = 1u;
    (void)d_option_env_put(d_desc_djin(), "shelf.ring=sentence");
    (void)d_option_env_put(d_desc_djin(), "layers=all");
    (void)d_option_env_put(d_desc_djin(), "layers=-policy");
    (void)d_tests_shelf_say(&shelf, D_DESC_RING_OPTION, D_DESC_LAYERS_OPTION,
                            0u, text, sizeof(text));
    D_TEST_CHECK(_suite, strcmp(text, D_TESTS_A "shelf of books, with 3 "
                                "books. Its first book is Emma.") == 0);
    d_option_env_clear(d_desc_djin());
#else
    (void)_suite;
#endif

    return;
}

/*
d_tests_desc_units_macro
  Checks the following:
  - desc's build sentence names what the knobs built
  - D_DESC_MACRO_SENTENCE puts parts in a sentence's order, with commas only
    between words of one kind
  - the name ring, plurals, numbers and D_DESC_MACRO_WHEN
*/
static void
d_tests_desc_units_macro(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "the macro layer");
#if D_CFG_DESC_ENABLED && D_CFG_DESC_MACRO && D_TESTS_DESC_FULL_BUILD &&      \
    D_CFG_DESC_FACTS && D_CFG_DESC_OPTIONS
    D_TEST_CHECK(_suite, strcmp(D_DESC_MACRO_BUILD, D_CFG_DESC_ARTICLES
                                ? "Units are built to the detailed ring, in "
                                  "every layer; facts are derived. Articles "
                                  "are on, and options are read at run time."
                                : "Units are built to the detailed ring, in "
                                  "every layer; facts are derived. Articles "
                                  "are off, and options are read at run "
                                  "time.") == 0);
#endif
#if !D_DESC_MACRO_KIT
    D_TEST_CHECK(_suite, !(D_CFG_DESC_ENABLED && D_CFG_DESC_MACRO &&
                           D_ENV_PP_HAS_VARIADIC_MACROS));
#else
    D_TEST_CHECK(_suite, strcmp(D_DESC_MACRO_SENTENCE((a, "full"),
                                    (a, "fixed-size"), , (an, "ordered"), ,
                                    (a, "tree"), ("of 8 keys"),
                                    ("It is balanced.")),
                                D_TESTS_A "full fixed-size ordered tree of 8 "
                                "keys. It is balanced.") == 0);
    D_TEST_CHECK(_suite, strcmp(D_DESC_MACRO_SENTENCE(, ,
                                    (a, "dusty")(an, "old"), , , (a, "shelf"),
                                    , ),
                                D_TESTS_A "dusty, old shelf") == 0);
    D_TEST_CHECK(_suite, strcmp(D_DESC_MACRO_NAME((a, "radix tree"),
                                    ("mapping int to text")("of 8 keys")),
                                "radix tree mapping int to text, of 8 "
                                "keys") == 0);
    D_TEST_CHECK(_suite, (strcmp("key" D_DESC_MACRO_PLURAL(1), "key") == 0) &&
                         (strcmp("key" D_DESC_MACRO_PLURAL(64), "keys") == 0));
    D_TEST_CHECK(_suite, (D_DESC_MACRO_IS_ZERO(0) == 1) &&
                         (D_DESC_MACRO_IS_ZERO(5) == 0) &&
                         (D_DESC_MACRO_IS_ONE(1) == 1));
    D_TEST_CHECK(_suite, strcmp("a" D_DESC_MACRO_WHEN(1, "b")
                                D_DESC_MACRO_WHEN(0, "c"), "ab") == 0);
#endif

    return;
}

/*
d_tests_desc_units
  The suite's second section: the block of desc_units.h, and its tests, in
the module d_tests_desc opened, so it runs after it.
*/
void
d_tests_desc_units(
    struct dj_test_suite* _suite
)
{
    // in desc's module, which d_tests_desc opened
    (void)d_test_suite_block(_suite, "desc_units");

    d_tests_desc_units_rings(_suite);
    d_tests_desc_units_grammar(_suite);
    d_tests_desc_units_layers(_suite);
    d_tests_desc_units_stop(_suite);
    d_tests_desc_units_facts(_suite);
    d_tests_desc_units_options(_suite);
    d_tests_desc_units_say_options(_suite);
    d_tests_desc_units_macro(_suite);

    return;
}
