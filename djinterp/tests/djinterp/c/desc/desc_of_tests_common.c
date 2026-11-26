/*******************************************************************************
* djinterp [c][desc]                                      desc_of_tests_common.c
*
*   The helpers the suites of desc's describers share (desc_of_tests.h): the
* keeping machine and its finders, a put into a buffer, and a module's
* layout check.
*
*
* path:      /tests/djinterp/c/desc/desc_of_tests_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#include "./desc_of_tests.h"  // corresponding header

// std
#include <stdio.h>   // snprintf
#include <string.h>  // memset, strcmp


//==============================================================================
// THE KEEPER
//==============================================================================

/*
d_tests_of_keep_fact
  Keeps one fact, while there is room.
*/
static int
d_tests_of_keep_fact(
    void*                      _context,
    const struct dj_desc_fact* _fact
)
{
    struct dj_tests_of_keep* keep = (struct dj_tests_of_keep*)_context;

    if (keep->count < D_TESTS_OF_KEEP_MAX)
    {
        keep->facts[keep->count] = *_fact;
        ++keep->count;
    }

    return 0;
}

/*
d_tests_of_keep_enter
  Counts the levels opened.
*/
static int
d_tests_of_keep_enter(
    void*       _context,
    const char* _key,
    const char* _label,
    uint32_t    _depth
)
{
    (void)_key;
    (void)_label;
    (void)_depth;
    ++((struct dj_tests_of_keep*)_context)->enters;

    return 0;
}

static const struct dj_desc_machine d_tests_of_keeping =
{
    NULL, d_tests_of_keep_fact, d_tests_of_keep_enter, NULL, NULL, NULL
};

/*
d_tests_of_facts
  An object's facts, every layer and level, into a fresh keeper.
*/
int
d_tests_of_facts(
    const struct dj_desc_type* _type,
    const void*                _object,
    struct dj_tests_of_keep*   _keep
)
{
    struct dj_desc_sink sink;

    memset(_keep, 0, sizeof(*_keep));
    d_desc_sink_init(&sink, &d_tests_of_keeping, _keep);

    return d_desc_facts(_type, _object, &sink);
}

/*
d_tests_of_find
  The first kept fact of a key at a depth, or null.
*/
const struct dj_desc_fact*
d_tests_of_find(
    const struct dj_tests_of_keep* _keep,
    const char*                    _key,
    uint32_t                       _depth
)
{
    size_t index;

    for (index = 0u; index < _keep->count; ++index)
    {
        if ( (_keep->facts[index].depth == _depth) &&
             (strcmp(_keep->facts[index].key, _key) == 0) )
        {
            return &_keep->facts[index];
        }
    }

    return NULL;
}

/*
d_tests_of_is
  1 when the kept fact of a key at a depth is the whole number _value.
*/
int
d_tests_of_is(
    const struct dj_tests_of_keep* _keep,
    const char*                    _key,
    uint32_t                       _depth,
    unsigned long long             _value
)
{
    const struct dj_desc_fact* fact = d_tests_of_find(_keep, _key, _depth);

    return ( (fact != NULL)                                        &&
             (fact->value.kind == (int32_t)D_OSTREAM_VALUE_INTEGER) &&
             (fact->value.negative == 0)                           &&
             ( ( ((unsigned long long)fact->value.high << 32) |
                 (unsigned long long)fact->value.low ) == _value ) );
}

/*
d_tests_of_flag
  1 when the kept fact of a key, at depth 0, is the truth _value.
*/
int
d_tests_of_flag(
    const struct dj_tests_of_keep* _keep,
    const char*                    _key,
    uint32_t                       _value
)
{
    const struct dj_desc_fact* fact = d_tests_of_find(_keep, _key, 0u);

    return ( (fact != NULL)                                        &&
             (fact->value.kind == (int32_t)D_OSTREAM_VALUE_BOOLEAN) &&
             (fact->value.low == _value) );
}

/*
d_tests_of_text
  1 when the kept fact of a key, at depth 0, is the text _value; a null
_value asks for none.
*/
int
d_tests_of_text(
    const struct dj_tests_of_keep* _keep,
    const char*                    _key,
    const char*                    _value
)
{
    const struct dj_desc_fact* fact = d_tests_of_find(_keep, _key, 0u);

    if (fact == NULL)
    {
        return 0;
    }

    if (_value == NULL)
    {
        return (fact->value.kind == (int32_t)D_OSTREAM_VALUE_NONE) ? 1 : 0;
    }

    return ( (fact->value.kind == (int32_t)D_OSTREAM_VALUE_TEXT) &&
             (fact->value.text != NULL)                         &&
             (strcmp(fact->value.text, _value) == 0) );
}

/*
d_tests_of_say
  An object's prose at a ring, every layer, into _text.
*/
int
d_tests_of_say(
    const struct dj_desc_type* _type,
    const void*                _object,
    uint32_t                   _ring,
    char*                      _text,
    size_t                     _capacity
)
{
    struct dj_desc_buffer buffer;

    d_desc_buffer_init(&buffer, _text, _capacity);

    return d_desc_say(_type, _object, _ring, D_DESC_LAYERS_ALL, 0u,
                      d_desc_put_buffer, &buffer);
}


//==============================================================================
// THE LAYOUT
//==============================================================================

/*
d_tests_of_layout
  A module's layout check: its rows exist where its describers are built
and none elsewhere; each row holds, a check each, named for the type and
the field; and the describers' build agrees with this program's.
*/
void
d_tests_of_layout(
    struct dj_test_suite*           _suite,
    const char*                     _module,
    int                             _built,
    const struct dj_desc_of_layout* _rows,
    size_t                          _count,
    int                             _agree,
    size_t                          _index
)
{
    char   name[96];
    char   message[192];
    size_t index;
    int    holds;

    (void)d_test_suite_test(_suite, "each described type against its record");
    if (!_built)
    {
        D_TEST_CHECK(_suite, (_count == 0u) && (_rows == NULL));
    }
    else
    {
        D_TEST_CHECK(_suite, (_count > 0u) && (_rows != NULL));
    }

    for (index = 0u; (_rows != NULL) && (index < _count); ++index)
    {
        holds = ( (_rows[index].recorded == _rows[index].actual) &&
                  (_rows[index].recorded_width ==
                   _rows[index].actual_width) ) ? 1 : 0;
        (void)snprintf(name, sizeof(name), "%s%s%s", _rows[index].type,
                       (_rows[index].field != NULL) ? "." : " ",
                       (_rows[index].field != NULL) ? _rows[index].field
                                                    : "size");
        (void)snprintf(message, sizeof(message),
                       "recorded %llu, %llu wide; the type has %llu, %llu "
                       "wide%s",
                       D_TESTS_OF_N(_rows[index].recorded),
                       D_TESTS_OF_N(_rows[index].recorded_width),
                       D_TESTS_OF_N(_rows[index].actual),
                       D_TESTS_OF_N(_rows[index].actual_width),
                       holds ? "" : ": change the describer with the type");
        D_TEST_CHECK_MSG(_suite, name, holds, message);
    }
    D_TEST_CHECK(_suite, d_desc_of_holds(_rows, _count, NULL) == 1);

    if (!D_TEST_CHECK(_suite, _agree == 1))
    {
        (void)snprintf(message, sizeof(message),
                       "row %llu: the describers' unit lays %s out "
                       "otherwise than this program: build both with the "
                       "same %s knobs", D_TESTS_OF_N(_index),
                       ( (_rows != NULL) && (_index < _count) )
                           ? _rows[_index].type : "a type", _module);
        D_TEST_CHECK_MSG(_suite, "the describers' build", 0, message);
    }

    return;
}
