/*******************************************************************************
* djinterp [c]                                                      desc_units.c
*
*   The units engine, and the facts a field list derives. A description is
* made in two steps: decide, from the object's values, which units show and
* what each slot's separators are; then write, front to back, once. A
* separator is chosen from counts known before the first byte is written, an
* article from the first unit's own text or value, and a capital as its
* letter goes out, so nothing written is ever changed.
*
*
* path:      /src/djinterp/c/desc/desc_units.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#include "../../../../inc/djinterp/c/desc/desc_units.h"  // corresponding header

// std
#include <ctype.h>   // tolower, toupper
#include <stdio.h>   // fwrite, snprintf
#include <string.h>  // strchr, strlen
// djinterp
#include "../../../../inc/djinterp/c/desc/desc_macro.h"  // D_DESC_MACRO_BUILD


#if D_CFG_DESC_ENABLED && D_CFG_DESC_MACRO && D_CFG_DESC_AT_BUILD
    #pragma message(D_DESC_MACRO_BUILD)
#endif


/*
d_desc_put_file
  A put into a FILE*.
*/
int
d_desc_put_file(
    void*       _context,
    const char* _text,
    size_t      _size
)
{
    if ( (_context == NULL) || (_size == 0u) )
    {
        return 0;
    }

    return (fwrite(_text, 1u, _size, (FILE*)_context) == _size) ? 0 : 1;
}


#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

// dj_internal_desc_run
//   struct: one description's state, so each step is a short function.
struct dj_internal_desc_run
{
    const struct dj_desc_type* type;
    const void*                object;
    fn_desc_put                put;
    void*                      context;
    uint32_t                   ring;          // the ring in force
    uint32_t                   layers;        // wanted and built
    uint32_t                   bits;          // the plan
    uint32_t                   capital;       // 1: the next letter is a capital
    uint32_t                   written;       // 1 once anything is written
    uint32_t                   stopped;       // 1 once the put asked to stop
    uint32_t                   last_one;      // 1: the last number was 1
    const struct dj_desc_unit* lead;          // the unit the article is for
    struct dj_ostream_value    lead_value;    // its first field, read once
    uint32_t                   lead_read;     // 1: lead_value holds it
};

/*
d_internal_desc_put
  Appends text. A capital asked for changes the letter on its way out; no
byte already written is touched.
*/
static void
d_internal_desc_put(
    struct dj_internal_desc_run* _run,
    const char*                  _text,
    size_t                       _size
)
{
    char first;

    if ( (_run->stopped != 0u) || (_text == NULL) || (_size == 0u) )
    {
        return;
    }

    if (_run->capital != 0u)
    {
        first         = (char)toupper((unsigned char)_text[0]);
        _run->capital = 0u;

        if (_run->put(_run->context, &first, 1u) != 0)
        {
            _run->stopped = 1u;

            return;
        }

        ++_text;
        --_size;
    }

    if ( (_size != 0u) && (_run->put(_run->context, _text, _size) != 0) )
    {
        _run->stopped = 1u;
    }

    _run->written = 1u;

    return;
}

/*
d_internal_desc_active
  Whether a unit shows: its slot, its ring, its layer (the noun's always),
and its plan bits.
*/
static int
d_internal_desc_active(
    const struct dj_internal_desc_run* _run,
    const struct dj_desc_unit*         _unit
)
{
    if ( (_unit->slot == D_DESC_SLOT_NONE)  ||
         (_unit->slot > D_DESC_SLOT_REMARK) ||
         ((uint32_t)_unit->ring > _run->ring) )
    {
        return 0;
    }

    if ( (_unit->slot != D_DESC_SLOT_NOUN) &&
         ( (_unit->layer >= (uint8_t)D_DESC_LAYER_COUNT) ||
           (((_run->layers >> _unit->layer) & 1u) == 0u) ) )
    {
        return 0;
    }

    return ( ((_run->bits & _unit->on) == _unit->on) &&
             ((_run->bits & _unit->off) == 0u) ) ? 1 : 0;
}

/*
d_internal_desc_count
  How many units of a slot show: known before the first is written, so each
knows its separator.
*/
static uint32_t
d_internal_desc_count(
    const struct dj_internal_desc_run* _run,
    uint32_t                           _slot
)
{
    uint32_t count = 0u;
    size_t   index;

    for (index = 0u; index < _run->type->unit_count; ++index)
    {
        if ( (_run->type->units[index].slot == _slot) &&
             (d_internal_desc_active(_run, &_run->type->units[index]) != 0) )
        {
            ++count;
        }
    }

    return count;
}

/*
d_internal_desc_read
  A field's value. The lead unit's first field is read once, for its article
and its text both.
*/
static struct dj_ostream_value
d_internal_desc_read(
    struct dj_internal_desc_run* _run,
    const struct dj_desc_unit*   _unit,
    const struct dj_desc_op*     _op
)
{
    if ( (_unit == _run->lead) && (_op == &_unit->ops[0]) )
    {
        if (_run->lead_read == 0u)
        {
            _run->lead_value = (_run->type->read != NULL)
                                   ? _run->type->read(_run->object, _op->field)
                                   : d_ostream_value_none();
            _run->lead_read  = 1u;
        }

        return _run->lead_value;
    }

    return (_run->type->read != NULL)
               ? _run->type->read(_run->object, _op->field)
               : d_ostream_value_none();
}

/*
d_internal_desc_magnitude
  An integer value's magnitude, as 64 bits.
*/
static uint64_t
d_internal_desc_magnitude(
    const struct dj_ostream_value* _value
)
{
    return ((uint64_t)_value->high << 32) | (uint64_t)_value->low;
}

#if D_CFG_DESC_ARTICLES

/*
d_internal_desc_an_number
  Whether a number is said with "an": 8, 11, 18, 80, 800, 8000, 11000.
*/
static int
d_internal_desc_an_number(
    uint64_t _number
)
{
    while (_number >= 1000u)
    {
        _number /= 1000u;
    }

    if ( (_number == 11u) || (_number == 18u) )
    {
        return 1;
    }

    while (_number >= 10u)
    {
        _number /= 10u;
    }

    return (_number == 8u) ? 1 : 0;
}

/*
d_internal_desc_article
  The article before a unit: its own, or the one its first letter or number
takes. It reads the unit's first text or value, never what was written.
*/
static const char*
d_internal_desc_article(
    struct dj_internal_desc_run* _run,
    const struct dj_desc_unit*   _unit
)
{
    const struct dj_desc_op* op     = &_unit->ops[0];
    int                      letter = 0;
    struct dj_ostream_value  value;

    if (_unit->article != D_DESC_ARTICLE_AUTO)
    {
        return (_unit->article == D_DESC_ARTICLE_AN) ? "an" : "a";
    }

    if (op->kind == D_DESC_OP_FIELD)
    {
        value = d_internal_desc_read(_run, _unit, op);

        if ( (value.kind == D_OSTREAM_VALUE_INTEGER) && (value.negative == 0) )
        {
            return (d_internal_desc_an_number(
                        d_internal_desc_magnitude(&value)) != 0) ? "an" : "a";
        }

        if ( (value.kind == D_OSTREAM_VALUE_TEXT) && (value.size != 0u) )
        {
            letter = (unsigned char)value.text[0];
        }
    }
    else if ( (op->kind == D_DESC_OP_TEXT) && (op->text != NULL) )
    {
        letter = (unsigned char)op->text[0];
    }

    letter = tolower(letter);

    return ( (letter != 0) && (strchr("aeiou", letter) != NULL) ) ? "an"
                                                                   : "a";
}

#endif  // D_CFG_DESC_ARTICLES

/*
d_internal_desc_find_lead
  The unit the article belongs to, from the line ring out: the first
adjective's that shows, else the noun's. Known before anything is written.
*/
static void
d_internal_desc_find_lead(
    struct dj_internal_desc_run* _run
)
{
    uint32_t slot;
    size_t   index;

    _run->lead = NULL;

    for (slot = D_DESC_SLOT_CONDITION;
         (_run->lead == NULL) && (slot <= D_DESC_SLOT_NOUN);
         ++slot)
    {
        for (index = 0u; index < _run->type->unit_count; ++index)
        {
            if ( (_run->type->units[index].slot == slot) &&
                 (d_internal_desc_active(_run,
                                         &_run->type->units[index]) != 0) )
            {
                _run->lead = &_run->type->units[index];

                break;
            }
        }
    }

    return;
}

/*
d_internal_desc_lead
  Writes the article, from the line ring out, when articles are built.
*/
static void
d_internal_desc_lead(
    struct dj_internal_desc_run* _run
)
{
#if D_CFG_DESC_ARTICLES
    const char* article;

    if ( (_run->ring < D_DESC_RING_LINE) || (_run->lead == NULL) )
    {
        return;
    }

    article = d_internal_desc_article(_run, _run->lead);
    d_internal_desc_put(_run, article, strlen(article));
#else
    (void)_run;
#endif

    return;
}

/*
d_internal_desc_decimal
  Writes a magnitude in decimal, a '-' before it when negative, into a
buffer of 24 bytes; returns where the digits start.
*/
static const char*
d_internal_desc_decimal(
    uint64_t _magnitude,
    int32_t  _negative,
    char*    _buffer,
    size_t*  _size
)
{
    char* at = _buffer + 23;

    *at = '\0';

    do
    {
        *--at       = (char)('0' + (int)(_magnitude % 10u));
        _magnitude /= 10u;
    }
    while (_magnitude != 0u);

    if (_negative != 0)
    {
        *--at = '-';
    }

    *_size = (size_t)((_buffer + 23) - at);

    return at;
}

/*
d_internal_desc_value
  Writes a value's text, and notes whether a number was 1, for the plural
marks after it.
*/
static void
d_internal_desc_value(
    struct dj_internal_desc_run*   _run,
    const struct dj_ostream_value* _value
)
{
    char        buffer[32];
    const char* text = NULL;
    size_t      size = 0u;

    switch (_value->kind)
    {
        case D_OSTREAM_VALUE_TEXT:
            text = _value->text;
            size = _value->size;
            break;
        case D_OSTREAM_VALUE_INTEGER:
            text = d_internal_desc_decimal(d_internal_desc_magnitude(_value),
                                           _value->negative, buffer, &size);
            _run->last_one = (d_internal_desc_magnitude(_value) == 1u)
                                 ? 1u : 0u;
            break;
        case D_OSTREAM_VALUE_BOOLEAN:
            text = (_value->low != 0u) ? "true" : "false";
            size = strlen(text);
            break;
        case D_OSTREAM_VALUE_REAL:
            (void)snprintf(buffer, sizeof(buffer), "%g", _value->real);
            text           = buffer;
            size           = strlen(buffer);
            _run->last_one = ( (_value->real == 1.0) ||
                               (_value->real == -1.0) ) ? 1u : 0u;
            break;
        default:
            break;
    }

    d_internal_desc_put(_run, text, size);

    return;
}

/*
d_internal_desc_unit
  Writes one unit's ops, in order.
*/
static void
d_internal_desc_unit(
    struct dj_internal_desc_run* _run,
    const struct dj_desc_unit*   _unit
)
{
    const struct dj_desc_op* op;
    struct dj_ostream_value  value;
    const char*              text;

    for (op = _unit->ops;
         (op != NULL) && (op->kind != D_DESC_OP_END) && (_run->stopped == 0u);
         ++op)
    {
        text = NULL;

        if (op->kind == D_DESC_OP_TEXT)
        {
            text = op->text;
        }
        else if (op->kind == D_DESC_OP_FIELD)
        {
            value = d_internal_desc_read(_run, _unit, op);
            d_internal_desc_value(_run, &value);
        }
        else if (op->kind == D_DESC_OP_PLURAL)
        {
            text = (_run->last_one == 0u) ? "s" : NULL;
        }
        else if (op->kind == D_DESC_OP_WORDS)
        {
            text = (_run->last_one != 0u) ? op->text : op->other;
        }

        if (text != NULL)
        {
            d_internal_desc_put(_run, text, strlen(text));
        }
    }

    return;
}

/*
d_internal_desc_separator
  What goes before a slot's unit at its place: the place, the slot's count
and the complements' count decide it, so nothing written changes later.
*/
static const char*
d_internal_desc_separator(
    const struct dj_internal_desc_run* _run,
    uint32_t                           _slot,
    uint32_t                           _place,
    uint32_t                           _count,
    uint32_t                           _complements
)
{
    // an adjective: a comma after one of its kind, else a space
    if (_slot < D_DESC_SLOT_NOUN)
    {
        return (_place > 0u) ? ", " : ((_run->written != 0u) ? " " : NULL);
    }

    if (_slot == D_DESC_SLOT_NOUN)
    {
        return (_run->written != 0u) ? " " : NULL;
    }

    if (_slot == D_DESC_SLOT_COMPLEMENT)
    {
        return (_place > 0u) ? ", " : " ";
    }

    if (_slot == D_DESC_SLOT_WITH)
    {
        if (_place > 0u)
        {
            return (_place + 1u == _count) ? " and " : ", ";
        }

        return (_complements > 0u) ? ", with " : " with ";
    }

    // a remark: a sentence of its own
    return (_place == 0u) ? ". " : " ";
}

/*
d_internal_desc_slot
  Writes a slot's units that show, each after its separator.
*/
static void
d_internal_desc_slot(
    struct dj_internal_desc_run* _run,
    uint32_t                     _slot,
    uint32_t                     _complements
)
{
    const struct dj_desc_unit* unit;
    const char*                separator;
    uint32_t                   count = d_internal_desc_count(_run, _slot);
    uint32_t                   place = 0u;
    size_t                     index;

    for (index = 0u;
         (index < _run->type->unit_count) && (_run->stopped == 0u);
         ++index)
    {
        unit = &_run->type->units[index];

        if ( (unit->slot != _slot) ||
             (d_internal_desc_active(_run, unit) == 0) )
        {
            continue;
        }

        separator = d_internal_desc_separator(_run, _slot, place, count,
                                              _complements);

        if (separator != NULL)
        {
            d_internal_desc_put(_run, separator, strlen(separator));
        }

        d_internal_desc_unit(_run, unit);
        ++place;
    }

    return;
}

/*
d_internal_desc_begin
  Sets a run up: what is asked for, as the options and the build allow.
*/
static void
d_internal_desc_begin(
    struct dj_internal_desc_run* _run,
    uint32_t                     _ring,
    uint32_t                     _layers,
    uint32_t                     _flags
)
{
    _run->ring      = (_ring == D_DESC_RING_OPTION)
                          ? d_desc_option_ring(_run->type) : _ring;
    _run->layers    = (_layers == D_DESC_LAYERS_OPTION)
                          ? d_desc_option_layers() : _layers;
    _run->layers   &= (uint32_t)(D_CFG_DESC_LAYERS_BUILT);
    _run->capital   = ((_flags & D_DESC_CAPITAL) != 0u) ? 1u : 0u;
    _run->written   = 0u;
    _run->stopped   = 0u;
    _run->last_one  = 0u;
    _run->lead_read = 0u;

    if (_run->ring > (uint32_t)(D_CFG_DESC_RING_MAX))
    {
        _run->ring = (uint32_t)(D_CFG_DESC_RING_MAX);
    }

    _run->bits = (_run->type->plan != NULL)
                     ? _run->type->plan(_run->object, _run->ring) : 0u;

    d_internal_desc_find_lead(_run);

    return;
}

int
d_desc_say(
    const struct dj_desc_type* _type,
    const void*                _object,
    uint32_t                   _ring,
    uint32_t                   _layers,
    uint32_t                   _flags,
    fn_desc_put                _put,
    void*                      _context
)
{
    struct dj_internal_desc_run run;
    uint32_t                    complements;
    uint32_t                    slot;

    if ( (_type == NULL) || (_object == NULL) || (_put == NULL) )
    {
        return -1;
    }

    run.type    = _type;
    run.object  = _object;
    run.put     = _put;
    run.context = _context;

    // 1. decide, from values: nothing is written yet
    d_internal_desc_begin(&run, _ring, _layers, _flags);
    complements = d_internal_desc_count(&run, D_DESC_SLOT_COMPLEMENT);

    // 2. write, front to back, once, in a sentence's order
    d_internal_desc_lead(&run);

    for (slot = D_DESC_SLOT_CONDITION;
         (slot <= D_DESC_SLOT_REMARK) && (run.stopped == 0u);
         ++slot)
    {
        d_internal_desc_slot(&run, slot, complements);
    }

    return (run.stopped != 0u) ? 1 : 0;
}

#else  // !(D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS)

int
d_desc_say(
    const struct dj_desc_type* _type,
    const void*                _object,
    uint32_t                   _ring,
    uint32_t                   _layers,
    uint32_t                   _flags,
    fn_desc_put                _put,
    void*                      _context
)
{
    (void)_type;
    (void)_object;
    (void)_ring;
    (void)_layers;
    (void)_flags;
    (void)_put;
    (void)_context;

    return -1;
}

#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS


int
d_desc_facts(
    const struct dj_desc_type* _type,
    const void*                _object,
    struct dj_desc_sink*       _sink
)
{
#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
    const struct dj_desc_field* field;
    size_t                      index;

    if ( (_type == NULL) || (_object == NULL) || (_sink == NULL) )
    {
        return -1;
    }

    if ( (_type->noun != NULL) &&
         (d_desc_emit(_sink, "type", NULL, d_ostream_value_text(_type->noun),
                      (uint32_t)D_DESC_LAYER_TYPE, 0u,
                      (uint32_t)D_DESC_TRUTH_GUARANTEED) != 0) )
    {
        return 1;
    }

    for (index = 0u; (index < _type->field_count) && (_type->read != NULL);
         ++index)
    {
        field = &_type->fields[index];

        // a field nobody wants is not read
        if (d_desc_wants(_sink, field->layer, field->importance) == 0)
        {
            if (_sink->stopped != 0u)
            {
                return 1;
            }

            continue;
        }

        if (d_desc_emit(_sink, field->key, field->label,
                        _type->read(_object, (uint32_t)index), field->layer,
                        field->importance, field->truth) != 0)
        {
            return 1;
        }
    }

    if ( (_type->contents != NULL) && (_type->contents(_object, _sink) != 0) )
    {
        return 1;
    }

    return (_sink->stopped != 0u) ? 1 : 0;
#else
    (void)_type;
    (void)_object;
    (void)_sink;

    return -1;
#endif
}

int
d_desc_run_facts(
    struct dj_desc_sink*       _sink,
    const struct dj_desc_type* _type,
    const void*                _object
)
{
    int result = 0;

    if ( (_sink == NULL) || (_type == NULL) || (_object == NULL) )
    {
        return -1;
    }

    if (_sink->machine == NULL)
    {
        return 0;
    }

    if ( (_sink->machine->begin != NULL) &&
         (_sink->machine->begin(_sink->context) != 0) )
    {
        _sink->stopped = 1u;
    }

    if (_sink->stopped == 0u)
    {
        result = d_desc_facts(_type, _object, _sink);
    }

    if (_sink->machine->end != NULL)
    {
        _sink->machine->end(_sink->context);
    }

    return (result < 0) ? result : ((_sink->stopped != 0u) ? 1 : 0);
}
