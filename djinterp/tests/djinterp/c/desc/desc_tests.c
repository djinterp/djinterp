/*******************************************************************************
* djinterp [test]                                                   desc_tests.c
*
*   The suite of c/desc's core (desc.h), in DTest's C face. A collecting
* machine keeps every fact, level and word it is sent, so each test can read
* back what a describer did: the fact's members, the sink's filters, its
* stop, its levels, a describer that meets itself again ending at the sink's
* limit, both describer forms, and a machine whose operations are all null.
*
*
* path:      /tests/djinterp/c/desc/desc_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#include "./desc_tests.h"  // corresponding header

// std
#include <string.h>  // memcmp, memcpy, memset, strcmp
// djinterp
#include "../../../../inc/djinterp/c/desc/desc.h"  // the core under test


// dj_tests_desc_collector
//   struct: a machine's context that keeps what it is sent. It stops at the
// fact numbered stop_at, at the level numbered stop_enter, or at begin when
// stop_begin is 1 (0 for never).
struct dj_tests_desc_collector
{
    struct dj_desc_fact facts[16];
    unsigned            count;        // facts sent
    unsigned            stop_at;
    unsigned            begins;
    unsigned            ends;
    unsigned            enters;
    unsigned            leaves;
    unsigned            stop_enter;
    unsigned            stop_begin;
    const char*         enter_key;    // the last level's key
    uint32_t            enter_depth;  // the depth it was opened at
    uint32_t            leave_depth;  // the depth the last close returned to
    char                text[32];     // words, as written
    size_t              text_size;
};

/*
d_tests_desc_begin
  The collector's begin: counts, and stops when stop_begin asks.
*/
static int
d_tests_desc_begin(
    void* _context
)
{
    struct dj_tests_desc_collector* collector =
        (struct dj_tests_desc_collector*)_context;

    ++collector->begins;

    return (collector->stop_begin != 0u) ? 1 : 0;
}

/*
d_tests_desc_collect
  The collector's fact: keeps the fact, and asks to stop at stop_at.
*/
static int
d_tests_desc_collect(
    void*                      _context,
    const struct dj_desc_fact* _fact
)
{
    struct dj_tests_desc_collector* collector =
        (struct dj_tests_desc_collector*)_context;

    if (collector->count < 16u)
    {
        collector->facts[collector->count] = *_fact;
    }

    ++collector->count;

    return ( (collector->stop_at != 0u) &&
             (collector->count >= collector->stop_at) ) ? 1 : 0;
}

/*
d_tests_desc_enter_level
  The collector's enter: keeps the key and depth, and stops at stop_enter.
*/
static int
d_tests_desc_enter_level(
    void*       _context,
    const char* _key,
    const char* _label,
    uint32_t    _depth
)
{
    struct dj_tests_desc_collector* collector =
        (struct dj_tests_desc_collector*)_context;

    (void)_label;
    ++collector->enters;
    collector->enter_key   = _key;
    collector->enter_depth = _depth;

    return ( (collector->stop_enter != 0u) &&
             (collector->enters >= collector->stop_enter) ) ? 1 : 0;
}

/*
d_tests_desc_leave_level
  The collector's leave: counts, and keeps the depth returned to.
*/
static int
d_tests_desc_leave_level(
    void*    _context,
    uint32_t _depth
)
{
    struct dj_tests_desc_collector* collector =
        (struct dj_tests_desc_collector*)_context;

    ++collector->leaves;
    collector->leave_depth = _depth;

    return 0;
}

/*
d_tests_desc_write
  The collector's text: appends the words, as far as they fit.
*/
static int
d_tests_desc_write(
    void*       _context,
    const char* _text,
    size_t      _size
)
{
    struct dj_tests_desc_collector* collector =
        (struct dj_tests_desc_collector*)_context;
    size_t                          room;

    room = (sizeof(collector->text) - 1u) - collector->text_size;

    if (_size < room)
    {
        room = _size;
    }

    memcpy(collector->text + collector->text_size, _text, room);
    collector->text_size                  += room;
    collector->text[collector->text_size]  = '\0';

    return 0;
}

/*
d_tests_desc_end
  The collector's end: counts.
*/
static void
d_tests_desc_end(
    void* _context
)
{
    ++((struct dj_tests_desc_collector*)_context)->ends;

    return;
}

// the collecting machine, every operation present
static const struct dj_desc_machine d_tests_desc_machine =
{
    d_tests_desc_begin,
    d_tests_desc_collect,
    d_tests_desc_enter_level,
    d_tests_desc_leave_level,
    d_tests_desc_write,
    d_tests_desc_end
};

// a machine with facts only, so text arrives as a fact
static const struct dj_desc_machine d_tests_desc_facts_only =
{
    NULL, d_tests_desc_collect, NULL, NULL, NULL, NULL
};

// a machine with no operation at all
static const struct dj_desc_machine d_tests_desc_empty =
{
    NULL, NULL, NULL, NULL, NULL, NULL
};

/*
d_tests_desc_wants
  Checks the following:
  - a fresh sink wants every layer at every importance, eight levels deep
  - it wants no layer past the last, and no importance past 3
  - a sink with no machine wants nothing
*/
static void
d_tests_desc_wants(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_desc_collector collector;
    struct dj_desc_sink            sink;
    int                            all = 1;
    uint32_t                       layer;
    uint32_t                       importance;

    (void)d_test_suite_test(_suite, "what a fresh sink wants");
    memset(&collector, 0, sizeof(collector));
    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);

    for (layer = 0u; layer < (uint32_t)D_DESC_LAYER_COUNT; ++layer)
    {
        for (importance = 0u; importance <= 3u; ++importance)
        {
            all = all && (d_desc_wants(&sink, layer, importance) != 0);
        }
    }

    D_TEST_CHECK(_suite, all);
    D_TEST_CHECK(_suite, sink.max_depth == 8u);
    D_TEST_CHECK(_suite, d_desc_wants(&sink, D_DESC_LAYER_COUNT, 0u) == 0);
    D_TEST_CHECK(_suite, d_desc_wants(&sink, D_DESC_LAYER_STATE, 4u) == 0);

    d_desc_sink_init(&sink, NULL, NULL);
    D_TEST_CHECK(_suite, d_desc_wants(&sink, D_DESC_LAYER_IDENTITY, 0u) == 0);

    return;
}

/*
d_tests_desc_fact
  Checks the following:
  - a fact carries its key, label, value, layer, importance and truth as
    given, at the sink's depth
  - a null label stays null
*/
static void
d_tests_desc_fact(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_desc_collector collector;
    struct dj_desc_sink            sink;
    const struct dj_desc_fact*     fact = &collector.facts[0];

    (void)d_test_suite_test(_suite, "a fact as given");
    memset(&collector, 0, sizeof(collector));
    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);

    D_TEST_CHECK(_suite, d_desc_emit(&sink, "element_count", "elements",
                                     d_ostream_value_ulong(42u),
                                     D_DESC_LAYER_STATE, 1u,
                                     D_DESC_TRUTH_OBSERVED) == 0);
    D_TEST_CHECK(_suite, collector.count == 1u);
    D_TEST_CHECK(_suite, strcmp(fact->key, "element_count") == 0);
    D_TEST_CHECK(_suite, strcmp(fact->label, "elements") == 0);
    D_TEST_CHECK(_suite, (fact->value.kind == D_OSTREAM_VALUE_INTEGER) &&
                         (fact->value.low == 42u));
    D_TEST_CHECK(_suite, (fact->layer == D_DESC_LAYER_STATE) &&
                         (fact->importance == 1u) &&
                         (fact->truth == D_DESC_TRUTH_OBSERVED) &&
                         (fact->depth == 0u));

    (void)d_desc_emit(&sink, "kind", NULL, d_ostream_value_text("tree map"),
                      D_DESC_LAYER_TYPE, 0u, D_DESC_TRUTH_IMPLEMENTATION);
    D_TEST_CHECK(_suite, (collector.count == 2u) &&
                         (collector.facts[1].label == NULL));

    return;
}

/*
d_tests_desc_filters
  Checks the following:
  - a fact on a layer the sink does not want is skipped, and emitting goes on
  - so is one less important than the sink wants
*/
static void
d_tests_desc_filters(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_desc_collector collector;
    struct dj_desc_sink            sink;

    (void)d_test_suite_test(_suite, "unwanted facts are skipped");
    memset(&collector, 0, sizeof(collector));
    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);
    sink.layers     = (uint32_t)1u << D_DESC_LAYER_STATE;
    sink.importance = 1u;

    D_TEST_CHECK(_suite, d_desc_emit(&sink, "kind", NULL,
                                     d_ostream_value_none(),
                                     D_DESC_LAYER_TYPE, 0u,
                                     D_DESC_TRUTH_GUARANTEED) == 0);
    D_TEST_CHECK(_suite, d_desc_emit(&sink, "detail", NULL,
                                     d_ostream_value_none(),
                                     D_DESC_LAYER_STATE, 2u,
                                     D_DESC_TRUTH_OBSERVED) == 0);
    D_TEST_CHECK(_suite, collector.count == 0u);
    D_TEST_CHECK(_suite, d_desc_emit(&sink, "size", NULL,
                                     d_ostream_value_none(),
                                     D_DESC_LAYER_STATE, 1u,
                                     D_DESC_TRUTH_OBSERVED) == 0);
    D_TEST_CHECK(_suite, collector.count == 1u);

    return;
}

/*
d_tests_desc_stop
  Checks the following:
  - when the machine asks to stop, d_desc_emit says so, and the sink is
    stopped
  - a stopped sink sends nothing more, wants nothing, and refuses to nest
*/
static void
d_tests_desc_stop(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_desc_collector collector;
    struct dj_desc_sink            sink;

    (void)d_test_suite_test(_suite, "a sink that stops");
    memset(&collector, 0, sizeof(collector));
    collector.stop_at = 2u;
    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);

    D_TEST_CHECK(_suite, d_desc_emit(&sink, "a", NULL, d_ostream_value_none(),
                                     D_DESC_LAYER_STATE, 0u,
                                     D_DESC_TRUTH_OBSERVED) == 0);
    D_TEST_CHECK(_suite, d_desc_emit(&sink, "b", NULL, d_ostream_value_none(),
                                     D_DESC_LAYER_STATE, 0u,
                                     D_DESC_TRUTH_OBSERVED) == 1);
    D_TEST_CHECK(_suite, sink.stopped == 1u);
    D_TEST_CHECK(_suite, d_desc_emit(&sink, "c", NULL, d_ostream_value_none(),
                                     D_DESC_LAYER_STATE, 0u,
                                     D_DESC_TRUTH_OBSERVED) == 1);
    D_TEST_CHECK(_suite, collector.count == 2u);
    D_TEST_CHECK(_suite, d_desc_wants(&sink, D_DESC_LAYER_STATE, 0u) == 0);
    D_TEST_CHECK(_suite, d_desc_enter(&sink, "holds", NULL) == 0);
    D_TEST_CHECK(_suite, collector.enters == 0u);

    return;
}

// dj_tests_desc_node
//   struct: a toy type for a describer: a name and what it holds, which may be
// itself.
struct dj_tests_desc_node
{
    const char*                      name;
    const struct dj_tests_desc_node* holds;
};

/*
d_tests_desc_node_describe
  A fact describer of one level: the node's name, then what it holds, one
level down, while the sink lets it.
*/
static int
d_tests_desc_node_describe(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    const struct dj_tests_desc_node* node =
        (const struct dj_tests_desc_node*)_object;
    int                              stopped;

    stopped = d_desc_emit(_sink, "name", NULL, d_ostream_value_text(node->name),
                          D_DESC_LAYER_IDENTITY, 0u, D_DESC_TRUTH_OBSERVED);

    if ( (stopped == 0)                                &&
         (node->holds != NULL)                         &&
         (d_desc_enter(_sink, "holds", "holding") != 0) )
    {
        stopped = d_tests_desc_node_describe(node->holds, _sink);
        d_desc_leave(_sink);
    }

    return stopped;
}

/*
d_tests_desc_node_say
  A text describer of a node: "node " and its name, in two writes.
*/
static int
d_tests_desc_node_say(
    const void* _object,
    fn_desc_put _put,
    void*       _context
)
{
    const struct dj_tests_desc_node* node =
        (const struct dj_tests_desc_node*)_object;

    if (_put(_context, "node ", 5u) != 0)
    {
        return 1;
    }

    return _put(_context, node->name, strlen(node->name));
}

/*
d_tests_desc_count_calls
  A fact describer that only counts its calls, in the int its object points
to.
*/
static int
d_tests_desc_count_calls(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    int* calls = *(int* const*)_object;

    (void)_sink;
    ++*calls;

    return 0;
}

/*
d_tests_desc_nesting
  Checks the following:
  - d_desc_enter counts the depth down to the sink's limit, and refuses past
    it; d_desc_leave counts it back, and not below 0
  - the machine is told each level's key, the depth it opens at, and the
    depth each close returns to
  - a fact emitted a level down carries that depth
  - a describer that meets itself again ends at the limit: a cycle ends
*/
static void
d_tests_desc_nesting(
    struct dj_test_suite* _suite
)
{
    fn_desc_describer               describe = d_tests_desc_node_describe;
    struct dj_tests_desc_collector  collector;
    struct dj_desc_sink             sink;
    struct dj_tests_desc_node       loop = { "loop", NULL };

    (void)d_test_suite_test(_suite, "nesting, and a cycle that ends");
    memset(&collector, 0, sizeof(collector));
    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);
    sink.max_depth = 2u;

    D_TEST_CHECK(_suite, (d_desc_enter(&sink, "a", NULL) == 1) &&
                         (d_desc_enter(&sink, "b", NULL) == 1) &&
                         (d_desc_enter(&sink, "c", NULL) == 0));
    D_TEST_CHECK(_suite, (sink.depth == 2u) && (collector.enters == 2u));
    D_TEST_CHECK(_suite, (strcmp(collector.enter_key, "b") == 0) &&
                         (collector.enter_depth == 1u));
    (void)d_desc_emit(&sink, "deep", NULL, d_ostream_value_none(),
                      D_DESC_LAYER_CONTENTS, 0u, D_DESC_TRUTH_OBSERVED);
    D_TEST_CHECK(_suite, collector.facts[0].depth == 2u);
    d_desc_leave(&sink);
    D_TEST_CHECK(_suite, collector.leave_depth == 1u);
    d_desc_leave(&sink);
    d_desc_leave(&sink);
    D_TEST_CHECK(_suite, (sink.depth == 0u) && (collector.leaves == 2u));

    // a node that holds itself: names at depths 0, 1 and 2, then the limit
    loop.holds = &loop;
    memset(&collector, 0, sizeof(collector));
    D_TEST_CHECK(_suite, describe(&loop, &sink) == 0);
    D_TEST_CHECK(_suite, collector.count == 3u);
    D_TEST_CHECK(_suite, (collector.facts[0].depth == 0u) &&
                         (collector.facts[2].depth == 2u) &&
                         (strcmp(collector.facts[2].value.text, "loop") == 0));
    D_TEST_CHECK(_suite, (sink.depth == 0u)         &&
                         (collector.enters == 2u)   &&
                         (collector.leaves == 2u));

    return;
}

/*
d_tests_desc_levels
  Checks the following:
  - a sink that does not want contents refuses to nest, and tells the
    machine nothing
  - a machine's enter that asks to stop stops the sink, and the level is
    not opened
*/
static void
d_tests_desc_levels(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_desc_collector collector;
    struct dj_desc_sink            sink;

    (void)d_test_suite_test(_suite, "levels the sink refuses");
    memset(&collector, 0, sizeof(collector));
    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);
    sink.layers &= ~((uint32_t)1u << D_DESC_LAYER_CONTENTS);

    D_TEST_CHECK(_suite, d_desc_enter(&sink, "holds", NULL) == 0);
    D_TEST_CHECK(_suite, (sink.depth == 0u) && (collector.enters == 0u));
    D_TEST_CHECK(_suite, sink.stopped == 0u);

    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);
    collector.stop_enter = 1u;

    D_TEST_CHECK(_suite, d_desc_enter(&sink, "holds", NULL) == 0);
    D_TEST_CHECK(_suite, (sink.depth == 0u) && (sink.stopped == 1u));
    D_TEST_CHECK(_suite, collector.enters == 1u);

    return;
}

/*
d_tests_desc_text
  Checks the following:
  - words go to a machine's text operation, as written, and are no fact
  - to a machine without one, they arrive as one fact: "text", on the
    intent layer, as its headline, the words its text value
  - a sink that does not want intent takes no words
*/
static void
d_tests_desc_text(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_desc_collector collector;
    struct dj_desc_sink            sink;
    const struct dj_desc_fact*     fact = &collector.facts[0];

    (void)d_test_suite_test(_suite, "words, to either kind of machine");
    memset(&collector, 0, sizeof(collector));
    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);

    D_TEST_CHECK(_suite, d_desc_text(&sink, "a tree", 6u) == 0);
    D_TEST_CHECK(_suite, (collector.text_size == 6u) &&
                         (strcmp(collector.text, "a tree") == 0));
    D_TEST_CHECK(_suite, collector.count == 0u);

    d_desc_sink_init(&sink, &d_tests_desc_facts_only, &collector);
    D_TEST_CHECK(_suite, d_desc_text(&sink, "a tree map", 6u) == 0);
    D_TEST_CHECK(_suite, (collector.count == 1u) &&
                         (strcmp(fact->key, "text") == 0));
    D_TEST_CHECK(_suite, (fact->layer == D_DESC_LAYER_INTENT) &&
                         (fact->importance == 0u));
    D_TEST_CHECK(_suite, (fact->value.kind == D_OSTREAM_VALUE_TEXT) &&
                         (fact->value.size == 6u) &&
                         (memcmp(fact->value.text, "a tree", 6u) == 0));

    memset(&collector, 0, sizeof(collector));
    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);
    sink.layers = (uint32_t)1u << D_DESC_LAYER_STATE;
    D_TEST_CHECK(_suite, d_desc_text(&sink, "unwanted", 8u) == 0);
    D_TEST_CHECK(_suite, (collector.text_size == 0u) &&
                         (collector.count == 0u));

    return;
}

/*
d_tests_desc_run
  Checks the following:
  - d_desc_run calls begin, the describer and end, once each
  - when begin asks to stop, the describer is not called, end still is,
    and d_desc_run says the sink stopped
  - with no machine, the describer is not called
  - d_desc_run_text gives a text describer's words to the machine's text
    operation, or as facts to a machine without one
*/
static void
d_tests_desc_run(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_desc_collector collector;
    struct dj_desc_sink            sink;
    struct dj_tests_desc_node      leaf = { "leaf", NULL };
    int                            calls = 0;
    int*                           counter = &calls;

    (void)d_test_suite_test(_suite, "running both describer forms");
    memset(&collector, 0, sizeof(collector));
    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);

    D_TEST_CHECK(_suite, d_desc_run(&sink, d_tests_desc_node_describe,
                                    &leaf) == 0);
    D_TEST_CHECK(_suite, (collector.begins == 1u) &&
                         (collector.count == 1u)  &&
                         (collector.ends == 1u));

    memset(&collector, 0, sizeof(collector));
    collector.stop_begin = 1u;
    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);
    D_TEST_CHECK(_suite, d_desc_run(&sink, d_tests_desc_count_calls,
                                    &counter) == 1);
    D_TEST_CHECK(_suite, (calls == 0) && (collector.ends == 1u));

    d_desc_sink_init(&sink, NULL, NULL);
    D_TEST_CHECK(_suite, d_desc_run(&sink, d_tests_desc_count_calls,
                                    &counter) == 0);
    D_TEST_CHECK(_suite, calls == 0);

    memset(&collector, 0, sizeof(collector));
    d_desc_sink_init(&sink, &d_tests_desc_machine, &collector);
    D_TEST_CHECK(_suite, d_desc_run_text(&sink, d_tests_desc_node_say,
                                         &leaf) == 0);
    D_TEST_CHECK(_suite, (strcmp(collector.text, "node leaf") == 0) &&
                         (collector.begins == 1u)                   &&
                         (collector.ends == 1u));

    memset(&collector, 0, sizeof(collector));
    d_desc_sink_init(&sink, &d_tests_desc_facts_only, &collector);
    D_TEST_CHECK(_suite, d_desc_run_text(&sink, d_tests_desc_node_say,
                                         &leaf) == 0);
    D_TEST_CHECK(_suite, (collector.count == 2u) &&
                         (collector.facts[1].value.size == 4u));

    return;
}

/*
d_tests_desc_no_ops
  Checks the following:
  - a machine whose operations are all null takes a whole description:
    facts and words are dropped, levels still count, and nothing stops
  - a sink with no machine, its layers set by hand, still emits and nests
    without calling anything
*/
static void
d_tests_desc_no_ops(
    struct dj_test_suite* _suite
)
{
    struct dj_desc_sink       sink;
    struct dj_tests_desc_node loop = { "loop", NULL };

    (void)d_test_suite_test(_suite, "a machine with no operations");
    loop.holds = &loop;
    d_desc_sink_init(&sink, &d_tests_desc_empty, NULL);

    D_TEST_CHECK(_suite, d_desc_emit(&sink, "a", NULL, d_ostream_value_none(),
                                     D_DESC_LAYER_STATE, 0u,
                                     D_DESC_TRUTH_OBSERVED) == 0);
    D_TEST_CHECK(_suite, d_desc_text(&sink, "words", 5u) == 0);
    D_TEST_CHECK(_suite, d_desc_enter(&sink, "holds", NULL) == 1);
    D_TEST_CHECK(_suite, sink.depth == 1u);
    d_desc_leave(&sink);
    D_TEST_CHECK(_suite, d_desc_run(&sink, d_tests_desc_node_describe,
                                    &loop) == 0);
    D_TEST_CHECK(_suite, d_desc_run_text(&sink, d_tests_desc_node_say,
                                         &loop) == 0);
    D_TEST_CHECK(_suite, (sink.depth == 0u) && (sink.stopped == 0u));

    d_desc_sink_init(&sink, NULL, NULL);
    sink.layers = D_DESC_LAYERS_ALL;
    D_TEST_CHECK(_suite, d_desc_emit(&sink, "a", NULL, d_ostream_value_none(),
                                     D_DESC_LAYER_STATE, 0u,
                                     D_DESC_TRUTH_OBSERVED) == 0);
    D_TEST_CHECK(_suite, d_desc_enter(&sink, "holds", NULL) == 1);
    d_desc_leave(&sink);
    D_TEST_CHECK(_suite, d_desc_run(&sink, d_tests_desc_node_describe,
                                    &loop) == 0);

    return;
}

/*
d_tests_desc
  The suite's one section: desc's module, the block of desc.h, and its tests.
*/
void
d_tests_desc(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_module(_suite, "desc");
    (void)d_test_suite_describe(_suite,
                                "desc's core: facts, machines, sinks and "
                                "describers");
    (void)d_test_suite_block(_suite, "desc");

    d_tests_desc_wants(_suite);
    d_tests_desc_fact(_suite);
    d_tests_desc_filters(_suite);
    d_tests_desc_stop(_suite);
    d_tests_desc_nesting(_suite);
    d_tests_desc_levels(_suite);
    d_tests_desc_text(_suite);
    d_tests_desc_run(_suite);
    d_tests_desc_no_ops(_suite);

    return;
}
