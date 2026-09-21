/*******************************************************************************
* djinterp [dawk]                                                          dss.c
*
* DSS front end:
*   A hand-written recursive-descent parser over the declarative subset of
* djinterp-dss.peg.  Descent mirrors the grammar's ordered choice directly,
* so the file reads against the .peg line for line and a grammar change has
* one place to land.
*   Storage is a bump arena and an open-addressed intern table, both local to
* this translation unit.  The repository's d_arena and d_string_intern do the
* same jobs better, and swapping to them is mechanical once the env include
* graph admits them; until then this file stays C11 with no dependency.
*
* path:      /src/djinterp/tools/dawk/dss.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dss.h"  // corresponding header
// std
#include <stdlib.h>  // malloc, realloc, free, strtod
#include <string.h>  // memcpy, memcmp, strlen


//==============================================================================
// 1.  STORAGE
//==============================================================================


// d_internal_vector
//   struct: a growable array of fixed-size records, addressed by index.
struct d_internal_vector
{
    unsigned char*  data;
    size_t          stride;
    size_t          count;
    size_t          capacity;
};

// d_internal_intern
//   struct: text to identifier.  Identifiers index `offsets`, which index
// `text`; the bucket array is open-addressed with a power-of-two mask.
struct d_internal_intern
{
    char*      text;
    size_t     text_used;
    size_t     text_capacity;

    uint32_t*  offsets;
    size_t     count;
    size_t     capacity;

    uint32_t*  buckets;
    size_t     bucket_mask;
};

// d_dss_sheet
//   struct: every array the parse produced, plus the intern table they
// refer into.
struct d_dss_sheet
{
    struct d_internal_intern  intern;

    struct d_internal_vector  simples;
    struct d_internal_vector  compounds;
    struct d_internal_vector  steps;
    struct d_internal_vector  selectors;
    struct d_internal_vector  values;
    struct d_internal_vector  declarations;
    struct d_internal_vector  rules;
    struct d_internal_vector  at_rules;
};


/*
d_internal_vector_push
  Appends one record, doubling the allocation when it is full.  Returns the
index of the record, or D_DSS_NO_INDEX when the growth failed.
*/
static uint32_t
d_internal_vector_push(
    struct d_internal_vector* _vector,
    const void*               _record
)
{
    if (_vector->count == _vector->capacity)
    {
        const size_t grown = (_vector->capacity == 0)
                           ? 32u
                           : (_vector->capacity * 2u);

        unsigned char* const data = realloc(_vector->data,
                                            grown * _vector->stride);

        // the caller reports the failure; nothing is appended
        if (!data)
        {
            return D_DSS_NO_INDEX;
        }

        _vector->data     = data;
        _vector->capacity = grown;
    }

    memcpy(_vector->data + (_vector->count * _vector->stride),
           _record,
           _vector->stride);

    const uint32_t at = (uint32_t)_vector->count;

    ++_vector->count;

    return at;
}


/*
d_internal_hash
  FNV-1a over a counted string.  The table is small and the keys are short
identifiers, so a stronger hash would buy nothing measurable.
*/
static uint32_t
d_internal_hash(
    const char* _text,
    size_t      _length
)
{
    uint32_t hash = 2166136261u;

    for (size_t at = 0; at < _length; ++at)
    {
        hash ^= (unsigned char)_text[at];
        hash *= 16777619u;
    }

    return hash;
}


/*
d_internal_intern_text
  Returns the stored text for an identifier.  Every stored string carries a
terminator, so the result is usable as a C string.
*/
static const char*
d_internal_intern_text(
    const struct d_internal_intern* _table,
    uint32_t                        _id
)
{
    if ((!_table->offsets) || (_id >= _table->count))
    {
        return "";
    }

    return _table->text + _table->offsets[_id];
}


/*
d_internal_intern_grow
  Rebuilds the bucket array at twice its size and refiles every identifier.
*/
static bool
d_internal_intern_grow(
    struct d_internal_intern* _table
)
{
    const size_t grown = (_table->bucket_mask == 0)
                       ? 256u
                       : ((_table->bucket_mask + 1u) * 2u);

    uint32_t* const buckets = malloc(grown * sizeof(uint32_t));

    if (!buckets)
    {
        return false;
    }

    for (size_t at = 0; at < grown; ++at)
    {
        buckets[at] = D_DSS_NO_INDEX;
    }

    free(_table->buckets);

    _table->buckets     = buckets;
    _table->bucket_mask = grown - 1u;

    for (uint32_t id = 0; id < (uint32_t)_table->count; ++id)
    {
        const char* const text   = _table->text + _table->offsets[id];
        size_t            slot   = d_internal_hash(text, strlen(text))
                                 & _table->bucket_mask;

        while (_table->buckets[slot] != D_DSS_NO_INDEX)
        {
            slot = (slot + 1u) & _table->bucket_mask;
        }

        _table->buckets[slot] = id;
    }

    return true;
}


/*
d_internal_intern_add
  Returns the identifier for a counted string, adding it when it is new.
Equal text always yields the same identifier, which is what lets the matcher
compare names as integers.
*/
static uint32_t
d_internal_intern_add(
    struct d_internal_intern* _table,
    const char*               _text,
    size_t                    _length
)
{
    // the first insertion sizes the table
    if ((_table->bucket_mask == 0) && (!d_internal_intern_grow(_table)))
    {
        return D_DSS_NO_INDEX;
    }

    // two thirds full is where open addressing starts to cluster
    if (((_table->count + 1u) * 3u) > ((_table->bucket_mask + 1u) * 2u))
    {
        if (!d_internal_intern_grow(_table))
        {
            return D_DSS_NO_INDEX;
        }
    }

    size_t slot = d_internal_hash(_text, _length) & _table->bucket_mask;

    while (_table->buckets[slot] != D_DSS_NO_INDEX)
    {
        const uint32_t    id    = _table->buckets[slot];
        const char* const found = _table->text + _table->offsets[id];

        if ( (strlen(found) == _length) &&
             (memcmp(found, _text, _length) == 0) )
        {
            return id;
        }

        slot = (slot + 1u) & _table->bucket_mask;
    }

    if ((_table->text_used + _length + 1u) > _table->text_capacity)
    {
        size_t grown = (_table->text_capacity == 0) ? 4096u
                                                    : _table->text_capacity;

        while (grown < (_table->text_used + _length + 1u))
        {
            grown *= 2u;
        }

        char* const text = realloc(_table->text, grown);

        if (!text)
        {
            return D_DSS_NO_INDEX;
        }

        _table->text          = text;
        _table->text_capacity = grown;
    }

    if (_table->count == _table->capacity)
    {
        const size_t grown = (_table->capacity == 0) ? 128u
                                                     : (_table->capacity * 2u);

        uint32_t* const offsets = realloc(_table->offsets,
                                          grown * sizeof(uint32_t));

        if (!offsets)
        {
            return D_DSS_NO_INDEX;
        }

        _table->offsets  = offsets;
        _table->capacity = grown;
    }

    const uint32_t id = (uint32_t)_table->count;

    _table->offsets[id] = (uint32_t)_table->text_used;

    memcpy(_table->text + _table->text_used, _text, _length);

    _table->text[_table->text_used + _length] = '\0';
    _table->text_used                        += _length + 1u;

    ++_table->count;

    _table->buckets[slot] = id;

    return id;
}


//==============================================================================
// 2.  SCANNING
//==============================================================================


// d_internal_parser
//   struct: the cursor over the sheet text, the sheet being built, and the
// error slot that a failure fills.
struct d_internal_parser
{
    const char*          text;
    size_t               length;
    size_t               at;
    uint32_t             line;
    uint32_t             line_start;

    struct d_dss_sheet*  sheet;
    struct d_dss_error*  error;
    bool                 failed;
};


/*
d_internal_fail
  Records the first failure and its position.  Later failures are ignored so
that the report names the place the parse actually went wrong rather than the
place it finally gave up.
*/
static bool
d_internal_fail(
    struct d_internal_parser* _parser,
    const char*               _message
)
{
    if (!_parser->failed)
    {
        _parser->failed = true;

        if (_parser->error)
        {
            _parser->error->line    = _parser->line;
            _parser->error->column  = (uint32_t)(_parser->at
                                               - _parser->line_start) + 1u;
            _parser->error->message = _message;
        }
    }

    return false;
}


/*
d_internal_is_name_start
  A CSSNAME may not begin with a digit.  That is the rule that keeps a
subsystem such as /3d from being spelled as a type selector, and it is
enforced here rather than discovered later.
*/
static bool
d_internal_is_name_start(
    char _c
)
{
    return ( ((_c >= 'A') && (_c <= 'Z')) ||
             ((_c >= 'a') && (_c <= 'z')) ||
             (_c == '_') );
}


/*
d_internal_is_name_char
*/
static bool
d_internal_is_name_char(
    char _c
)
{
    return (d_internal_is_name_start(_c) || ((_c >= '0') && (_c <= '9')));
}


/*
d_internal_skip
  Consumes blanks, newlines and `#` comments.  Newlines are counted so that a
declaration can name its own line in a report.
*/
static void
d_internal_skip(
    struct d_internal_parser* _parser
)
{
    while (_parser->at < _parser->length)
    {
        const char c = _parser->text[_parser->at];

        if ((c == ' ') || (c == '\t') || (c == '\r'))
        {
            ++_parser->at;
        }
        else if (c == '\n')
        {
            ++_parser->at;
            ++_parser->line;
            _parser->line_start = (uint32_t)_parser->at;
        }
        else if (c == '#')
        {
            while ( (_parser->at < _parser->length) &&
                    (_parser->text[_parser->at] != '\n') )
            {
                ++_parser->at;
            }
        }
        else
        {
            break;
        }
    }

    return;
}


/*
d_internal_peek
  The character at the cursor, or NUL at the end of input.
*/
static char
d_internal_peek(
    const struct d_internal_parser* _parser
)
{
    if (_parser->at >= _parser->length)
    {
        return '\0';
    }

    return _parser->text[_parser->at];
}


/*
d_internal_eat
  Consumes a literal when it is present and reports whether it was.
*/
static bool
d_internal_eat(
    struct d_internal_parser* _parser,
    const char*               _literal
)
{
    const size_t length = strlen(_literal);

    if ((_parser->at + length) > _parser->length)
    {
        return false;
    }

    if (memcmp(_parser->text + _parser->at, _literal, length) != 0)
    {
        return false;
    }

    _parser->at += length;

    return true;
}


/*
d_internal_name
  Reads a CSSNAME: a name start followed by name characters, with internal
hyphens permitted only when a name character follows.  Returns the interned
identifier, or D_DSS_NO_INDEX when no name is present.
*/
static uint32_t
d_internal_name(
    struct d_internal_parser* _parser
)
{
    if (!d_internal_is_name_start(d_internal_peek(_parser)))
    {
        return D_DSS_NO_INDEX;
    }

    const size_t start = _parser->at;

    ++_parser->at;

    while (_parser->at < _parser->length)
    {
        const char c = _parser->text[_parser->at];

        if (d_internal_is_name_char(c))
        {
            ++_parser->at;
        }
        else if ( (c == '-') &&
                  ((_parser->at + 1u) < _parser->length) &&
                  (d_internal_is_name_char(_parser->text[_parser->at + 1u])) )
        {
            _parser->at += 2u;
        }
        else
        {
            break;
        }
    }

    return d_internal_intern_add(&_parser->sheet->intern,
                                 _parser->text + start,
                                 _parser->at - start);
}


/*
d_internal_string
  Reads a double-quoted STRING and interns its body with escapes resolved to
the escaped character, which is what the grammar's `Escape <- "\\" .` means.
*/
static uint32_t
d_internal_string(
    struct d_internal_parser* _parser
)
{
    if (d_internal_peek(_parser) != '"')
    {
        return D_DSS_NO_INDEX;
    }

    ++_parser->at;

    const size_t start = _parser->at;

    char*  body     = NULL;
    size_t used     = 0;
    size_t capacity = 0;

    while (_parser->at < _parser->length)
    {
        char c = _parser->text[_parser->at];

        if (c == '"')
        {
            ++_parser->at;

            const uint32_t id = d_internal_intern_add(&_parser->sheet->intern,
                                                      body ? body
                                                           : (_parser->text
                                                              + start),
                                                      used);
            free(body);

            return id;
        }

        if (c == '\\')
        {
            ++_parser->at;

            if (_parser->at >= _parser->length)
            {
                break;
            }

            c = _parser->text[_parser->at];
        }

        if (used == capacity)
        {
            const size_t grown = (capacity == 0) ? 32u : (capacity * 2u);

            char* const data = realloc(body, grown);

            if (!data)
            {
                free(body);
                (void)d_internal_fail(_parser, "out of memory in string");
                return D_DSS_NO_INDEX;
            }

            body     = data;
            capacity = grown;
        }

        body[used] = c;
        ++used;
        ++_parser->at;
    }

    free(body);

    (void)d_internal_fail(_parser, "unterminated string");

    return D_DSS_NO_INDEX;
}


//==============================================================================
// 3.  PARSING
//==============================================================================


static bool d_internal_value_list(struct d_internal_parser* _parser,
                                  uint32_t*                 _out_first,
                                  uint32_t*                 _out_count);


/*
d_internal_number
  Reads a DECIMAL with an optional magnitude suffix.  The suffix multiplies
rather than annotating, so [size>=4k] compares against 4096.
*/
static bool
d_internal_number(
    struct d_internal_parser* _parser,
    double*                   _out_number
)
{
    const char* const start = _parser->text + _parser->at;
    char*             end   = NULL;

    const double parsed = strtod(start, &end);

    // nothing numeric at the cursor
    if (end == start)
    {
        return false;
    }

    _parser->at += (size_t)(end - start);

    double scale = 1.0;

    switch (d_internal_peek(_parser))
    {
        case 'k': case 'K': scale = 1024.0;                      break;
        case 'm': case 'M': scale = 1024.0 * 1024.0;             break;
        case 'g': case 'G': scale = 1024.0 * 1024.0 * 1024.0;    break;
        default:                                                 break;
    }

    // a suffix binds only when no name character follows it
    if (scale != 1.0)
    {
        const size_t next = _parser->at + 1u;

        if ( (next >= _parser->length) ||
             (!d_internal_is_name_char(_parser->text[next])) )
        {
            ++_parser->at;
        }
        else
        {
            scale = 1.0;
        }
    }

    *_out_number = parsed * scale;

    return true;
}


/*
d_internal_attribute
  Reads `[name]` or `[name OP value]`.  Operators are tried longest first, so
`>=` is never read as `>` followed by a value beginning with `=`.
*/
static bool
d_internal_attribute(
    struct d_internal_parser* _parser,
    struct d_dss_simple*      _out_simple
)
{
    d_internal_skip(_parser);

    const uint32_t name = d_internal_name(_parser);

    if (name == D_DSS_NO_INDEX)
    {
        return d_internal_fail(_parser, "attribute name expected");
    }

    _out_simple->kind  = D_DSS_SIMPLE_ATTRIBUTE;
    _out_simple->name  = name;
    _out_simple->op    = D_DSS_ATTR_PRESENCE;
    _out_simple->value = D_DSS_NO_INDEX;

    d_internal_skip(_parser);

    if (d_internal_eat(_parser, "]"))
    {
        return true;
    }

    static const struct
    {
        const char* spelling;
        uint8_t     op;
    }
    operators[] =
    {
        { "^=", D_DSS_ATTR_PREFIX        },
        { "$=", D_DSS_ATTR_SUFFIX        },
        { "*=", D_DSS_ATTR_SUBSTRING     },
        { "~=", D_DSS_ATTR_WORD          },
        { "|=", D_DSS_ATTR_LANG          },
        { "==", D_DSS_ATTR_EQUAL         },
        { "!=", D_DSS_ATTR_NOT_EQUAL     },
        { ">=", D_DSS_ATTR_GREATER_EQUAL },
        { "<=", D_DSS_ATTR_LESS_EQUAL    },
        { "=",  D_DSS_ATTR_EQUAL         },
        { ">",  D_DSS_ATTR_GREATER       },
        { "<",  D_DSS_ATTR_LESS          }
    };

    bool matched = false;

    for (size_t which = 0;
         which < (sizeof(operators) / sizeof(operators[0]));
         ++which)
    {
        if (d_internal_eat(_parser, operators[which].spelling))
        {
            _out_simple->op = operators[which].op;
            matched         = true;
            break;
        }
    }

    if (!matched)
    {
        return d_internal_fail(_parser, "attribute operator expected");
    }

    d_internal_skip(_parser);

    const char lead = d_internal_peek(_parser);

    if (lead == '"')
    {
        _out_simple->value = d_internal_string(_parser);

        if (_out_simple->value == D_DSS_NO_INDEX)
        {
            return false;
        }
    }
    else if (((lead >= '0') && (lead <= '9')) || (lead == '-') || (lead == '+'))
    {
        if (!d_internal_number(_parser, &_out_simple->number))
        {
            return d_internal_fail(_parser, "attribute number expected");
        }

        _out_simple->numeric = true;
    }
    else
    {
        _out_simple->value = d_internal_name(_parser);

        if (_out_simple->value == D_DSS_NO_INDEX)
        {
            return d_internal_fail(_parser, "attribute value expected");
        }
    }

    d_internal_skip(_parser);

    if (!d_internal_eat(_parser, "]"))
    {
        return d_internal_fail(_parser, "']' expected");
    }

    return true;
}


/*
d_internal_compound
  Reads one or more simple selectors with no whitespace between them.  The
universal selector is only legal first, which the grammar implies by making
every other simple a suffix.
*/
static bool
d_internal_compound(
    struct d_internal_parser* _parser,
    uint32_t*                 _out_compound
)
{
    struct d_dss_compound compound;

    compound.first_simple = (uint32_t)_parser->sheet->simples.count;
    compound.simple_count = 0;

    while (!_parser->failed)
    {
        struct d_dss_simple simple;

        memset(&simple, 0, sizeof(simple));

        simple.value = D_DSS_NO_INDEX;
        simple.name  = D_DSS_NO_INDEX;

        const char lead = d_internal_peek(_parser);

        if (lead == '*')
        {
            ++_parser->at;
            simple.kind = D_DSS_SIMPLE_UNIVERSAL;
        }
        else if (lead == '.')
        {
            ++_parser->at;
            simple.kind = D_DSS_SIMPLE_CLASS;
            simple.name = d_internal_name(_parser);

            if (simple.name == D_DSS_NO_INDEX)
            {
                return d_internal_fail(_parser, "class name expected");
            }
        }
        else if (lead == '[')
        {
            ++_parser->at;

            if (!d_internal_attribute(_parser, &simple))
            {
                return false;
            }
        }
        else if (lead == ':')
        {
            ++_parser->at;

            simple.kind = D_DSS_SIMPLE_PSEUDO_CLASS;

            if (d_internal_peek(_parser) == ':')
            {
                ++_parser->at;
                simple.kind = D_DSS_SIMPLE_PSEUDO_ELEMENT;
            }

            simple.name = d_internal_name(_parser);

            if (simple.name == D_DSS_NO_INDEX)
            {
                return d_internal_fail(_parser, "pseudo name expected");
            }
        }
        else if (d_internal_is_name_start(lead))
        {
            simple.kind = D_DSS_SIMPLE_TYPE;
            simple.name = d_internal_name(_parser);
        }
        else
        {
            break;
        }

        if (d_internal_vector_push(&_parser->sheet->simples, &simple)
            == D_DSS_NO_INDEX)
        {
            return d_internal_fail(_parser, "out of memory");
        }

        ++compound.simple_count;
    }

    if (compound.simple_count == 0)
    {
        return d_internal_fail(_parser, "selector expected");
    }

    *_out_compound = d_internal_vector_push(&_parser->sheet->compounds,
                                            &compound);

    return (*_out_compound != D_DSS_NO_INDEX);
}


/*
d_internal_selector
  Reads a chain of compounds joined by combinators.  A run of whitespace is a
descendant combinator only when a compound follows it, which is what keeps
`banner > rule {` from reading the brace as a compound.
*/
static bool
d_internal_selector(
    struct d_internal_parser* _parser,
    uint32_t*                 _out_selector
)
{
    struct d_dss_selector selector;

    d_internal_skip(_parser);

    if (!d_internal_compound(_parser, &selector.head))
    {
        return false;
    }

    selector.first_step = (uint32_t)_parser->sheet->steps.count;
    selector.step_count = 0;

    while (!_parser->failed)
    {
        const size_t   mark      = _parser->at;
        const uint32_t mark_line = _parser->line;

        d_internal_skip(_parser);

        struct d_dss_step step;

        step.combinator = D_DSS_COMBINATOR_DESCENDANT;

        if (d_internal_eat(_parser, ">"))
        {
            step.combinator = D_DSS_COMBINATOR_CHILD;
        }
        else if (d_internal_eat(_parser, "..."))
        {
            step.combinator = D_DSS_COMBINATOR_SIBLING;
        }
        else if (d_internal_eat(_parser, "+"))
        {
            step.combinator = D_DSS_COMBINATOR_ADJACENT;
        }
        else if (mark == _parser->at)
        {
            break;      // no whitespace and no combinator: the chain ended
        }

        d_internal_skip(_parser);

        const char lead = d_internal_peek(_parser);

        // a brace or comma after whitespace ends the selector, not a step
        if ( (step.combinator == D_DSS_COMBINATOR_DESCENDANT) &&
             ((lead == '{') || (lead == ',') || (lead == '\0')) )
        {
            _parser->at   = mark;
            _parser->line = mark_line;
            break;
        }

        if (!d_internal_compound(_parser, &step.compound))
        {
            return false;
        }

        if (d_internal_vector_push(&_parser->sheet->steps, &step)
            == D_DSS_NO_INDEX)
        {
            return d_internal_fail(_parser, "out of memory");
        }

        ++selector.step_count;
    }

    *_out_selector = d_internal_vector_push(&_parser->sheet->selectors,
                                            &selector);

    return (*_out_selector != D_DSS_NO_INDEX);
}


/*
d_internal_value
  Reads one declaration term.  A CSSNAME followed by `(` is a function and its
arguments are a further value list, which is the only recursion here and is
bounded by the source's own nesting.
*/
static bool
d_internal_value(
    struct d_internal_parser* _parser,
    bool*                     _out_present
)
{
    d_internal_skip(_parser);

    struct d_dss_value value;

    memset(&value, 0, sizeof(value));

    value.text      = D_DSS_NO_INDEX;
    value.first_arg = D_DSS_NO_INDEX;

    const char lead = d_internal_peek(_parser);

    *_out_present = true;

    if (lead == '"')
    {
        value.kind = D_DSS_VALUE_STRING;
        value.text = d_internal_string(_parser);

        if (value.text == D_DSS_NO_INDEX)
        {
            return false;
        }
    }
    else if (lead == ',')
    {
        ++_parser->at;
        value.kind = D_DSS_VALUE_COMMA;
    }
    else if (lead == '!')
    {
        ++_parser->at;
        value.kind = D_DSS_VALUE_FLAG;
        value.text = d_internal_name(_parser);

        if (value.text == D_DSS_NO_INDEX)
        {
            return d_internal_fail(_parser, "flag name expected");
        }
    }
    else if ((lead >= '0') && (lead <= '9'))
    {
        value.kind = D_DSS_VALUE_NUMBER;

        if (!d_internal_number(_parser, &value.number))
        {
            return d_internal_fail(_parser, "number expected");
        }
    }
    else if (d_internal_is_name_start(lead))
    {
        const uint32_t name = d_internal_name(_parser);

        if (name == D_DSS_NO_INDEX)
        {
            return d_internal_fail(_parser, "value expected");
        }

        value.kind = D_DSS_VALUE_IDENT;
        value.text = name;

        // a name immediately followed by '(' is a call, not a keyword
        if (d_internal_peek(_parser) == '(')
        {
            ++_parser->at;

            value.kind = D_DSS_VALUE_FUNCTION;

            d_internal_skip(_parser);

            if (d_internal_peek(_parser) == ')')
            {
                ++_parser->at;
                value.first_arg = D_DSS_NO_INDEX;
                value.arg_count = 0;
            }
            else
            {
                if (!d_internal_value_list(_parser,
                                           &value.first_arg,
                                           &value.arg_count))
                {
                    return false;
                }

                d_internal_skip(_parser);

                if (!d_internal_eat(_parser, ")"))
                {
                    return d_internal_fail(_parser, "')' expected");
                }
            }
        }
    }
    else
    {
        *_out_present = false;
        return true;
    }

    return (d_internal_vector_push(&_parser->sheet->values, &value)
            != D_DSS_NO_INDEX);
}


/*
d_internal_value_list
  Reads one or more terms.  Values are appended contiguously, so a run is a
first index and a count; a nested call therefore has to complete before the
caller records its own range, which the recursion already guarantees.
*/
static bool
d_internal_value_list(
    struct d_internal_parser* _parser,
    uint32_t*                 _out_first,
    uint32_t*                 _out_count
)
{
    const uint32_t first = (uint32_t)_parser->sheet->values.count;
    uint32_t       count = 0;

    while (!_parser->failed)
    {
        d_internal_skip(_parser);

        const char lead = d_internal_peek(_parser);

        if ((lead == ';') || (lead == '}') || (lead == ')') || (lead == '\0'))
        {
            break;
        }

        bool present = false;

        if (!d_internal_value(_parser, &present))
        {
            return false;
        }

        if (!present)
        {
            break;
        }

        ++count;
    }

    if (count == 0)
    {
        return d_internal_fail(_parser, "declaration value expected");
    }

    *_out_first = first;
    *_out_count = count;

    return true;
}


/*
d_internal_declaration
  Reads `property : value...` with the grammar's own lookahead: a CSSNAME,
optional blanks, a colon that is not a second colon.  Anything else is left
for the caller to try as a rule.
*/
static bool
d_internal_declaration(
    struct d_internal_parser* _parser,
    bool*                     _out_present
)
{
    d_internal_skip(_parser);

    const size_t   mark      = _parser->at;
    const uint32_t mark_line = _parser->line;

    *_out_present = false;

    const uint32_t property = d_internal_name(_parser);

    if (property == D_DSS_NO_INDEX)
    {
        _parser->at   = mark;
        _parser->line = mark_line;
        return true;
    }

    while ( (_parser->at < _parser->length) &&
            ((_parser->text[_parser->at] == ' ') ||
             (_parser->text[_parser->at] == '\t')) )
    {
        ++_parser->at;
    }

    // a colon makes it a declaration; a second colon makes it a selector
    if ( (d_internal_peek(_parser) != ':') ||
         ( ((_parser->at + 1u) < _parser->length) &&
           (_parser->text[_parser->at + 1u] == ':') ) )
    {
        _parser->at   = mark;
        _parser->line = mark_line;
        return true;
    }

    ++_parser->at;

    struct d_dss_declaration declaration;

    declaration.property = property;
    declaration.line     = mark_line;

    if (!d_internal_value_list(_parser,
                               &declaration.first_value,
                               &declaration.value_count))
    {
        return false;
    }

    d_internal_skip(_parser);

    // the terminator may be omitted before a closing brace
    if ((!d_internal_eat(_parser, ";")) && (d_internal_peek(_parser) != '}'))
    {
        return d_internal_fail(_parser, "';' expected after declaration");
    }

    if (d_internal_vector_push(&_parser->sheet->declarations, &declaration)
        == D_DSS_NO_INDEX)
    {
        return d_internal_fail(_parser, "out of memory");
    }

    *_out_present = true;

    return true;
}


/*
d_internal_block
  Reads a declaration block.  Statements are part of the grammar but not of
this subset, so a block that holds one is rejected with its own message
rather than a generic parse failure.
*/
static bool
d_internal_block(
    struct d_internal_parser* _parser,
    uint32_t*                 _out_first,
    uint32_t*                 _out_count
)
{
    if (!d_internal_eat(_parser, "{"))
    {
        return d_internal_fail(_parser, "'{' expected");
    }

    *_out_first = (uint32_t)_parser->sheet->declarations.count;
    *_out_count = 0;

    while (!_parser->failed)
    {
        d_internal_skip(_parser);

        if (d_internal_eat(_parser, ";"))
        {
            continue;
        }

        if (d_internal_peek(_parser) == '}')
        {
            ++_parser->at;
            return true;
        }

        if (_parser->at >= _parser->length)
        {
            return d_internal_fail(_parser, "'}' expected");
        }

        bool present = false;

        if (!d_internal_declaration(_parser, &present))
        {
            return false;
        }

        if (!present)
        {
            return d_internal_fail(_parser,
                                   "awk statements are not accepted in this "
                                   "subset");
        }

        ++(*_out_count);
    }

    return false;
}


/*
d_internal_at_rule
  Reads `@name prelude? ( block | ';' )`.  The prelude is kept verbatim as
interned text because its meaning belongs to whoever registered the at-rule,
not to the parser.
*/
static bool
d_internal_at_rule(
    struct d_internal_parser* _parser
)
{
    struct d_dss_at_rule at_rule;

    at_rule.line      = _parser->line;
    at_rule.prelude   = D_DSS_NO_INDEX;
    at_rule.has_block = false;

    at_rule.first_declaration = 0;
    at_rule.declaration_count = 0;

    at_rule.name = d_internal_name(_parser);

    if (at_rule.name == D_DSS_NO_INDEX)
    {
        return d_internal_fail(_parser, "at-rule name expected");
    }

    while ( (_parser->at < _parser->length) &&
            ((_parser->text[_parser->at] == ' ') ||
             (_parser->text[_parser->at] == '\t')) )
    {
        ++_parser->at;
    }

    const size_t prelude_start = _parser->at;

    while ( (_parser->at < _parser->length) &&
            (_parser->text[_parser->at] != '{') &&
            (_parser->text[_parser->at] != ';') &&
            (_parser->text[_parser->at] != '\n') )
    {
        ++_parser->at;
    }

    size_t prelude_length = _parser->at - prelude_start;

    while ( (prelude_length > 0) &&
            ((_parser->text[prelude_start + prelude_length - 1u] == ' ') ||
             (_parser->text[prelude_start + prelude_length - 1u] == '\t')) )
    {
        --prelude_length;
    }

    if (prelude_length > 0)
    {
        at_rule.prelude = d_internal_intern_add(&_parser->sheet->intern,
                                                _parser->text + prelude_start,
                                                prelude_length);
    }

    d_internal_skip(_parser);

    if (d_internal_peek(_parser) == '{')
    {
        at_rule.has_block = true;

        if (!d_internal_block(_parser,
                              &at_rule.first_declaration,
                              &at_rule.declaration_count))
        {
            return false;
        }
    }
    else
    {
        (void)d_internal_eat(_parser, ";");
    }

    return (d_internal_vector_push(&_parser->sheet->at_rules, &at_rule)
            != D_DSS_NO_INDEX);
}


/*
d_internal_style_rule
  Reads a comma-separated selector list followed by a declaration block.
*/
static bool
d_internal_style_rule(
    struct d_internal_parser* _parser
)
{
    struct d_dss_rule rule;

    rule.line           = _parser->line;
    rule.first_selector = (uint32_t)_parser->sheet->selectors.count;
    rule.selector_count = 0;

    while (!_parser->failed)
    {
        uint32_t selector = D_DSS_NO_INDEX;

        if (!d_internal_selector(_parser, &selector))
        {
            return false;
        }

        ++rule.selector_count;

        d_internal_skip(_parser);

        if (!d_internal_eat(_parser, ","))
        {
            break;
        }
    }

    d_internal_skip(_parser);

    if (!d_internal_block(_parser,
                          &rule.first_declaration,
                          &rule.declaration_count))
    {
        return false;
    }

    return (d_internal_vector_push(&_parser->sheet->rules, &rule)
            != D_DSS_NO_INDEX);
}


//==============================================================================
// 4.  ENTRY POINTS
//==============================================================================


/*
d_dss_parse
  Parses a sheet.  Returns NULL on failure with `_out_error` filled; the
partially built sheet is released, so a caller never sees half a parse.
*/
struct d_dss_sheet*
d_dss_parse(
    const char*         _text,
    size_t              _length,
    struct d_dss_error* _out_error
)
{
    // parameter validation first
    if (!_text)
    {
        return NULL;
    }

    if (_out_error)
    {
        _out_error->line    = 0;
        _out_error->column  = 0;
        _out_error->message = NULL;
    }

    struct d_dss_sheet* const sheet = calloc(1u, sizeof(*sheet));

    if (!sheet)
    {
        return NULL;
    }

    sheet->simples.stride      = sizeof(struct d_dss_simple);
    sheet->compounds.stride    = sizeof(struct d_dss_compound);
    sheet->steps.stride        = sizeof(struct d_dss_step);
    sheet->selectors.stride    = sizeof(struct d_dss_selector);
    sheet->values.stride       = sizeof(struct d_dss_value);
    sheet->declarations.stride = sizeof(struct d_dss_declaration);
    sheet->rules.stride        = sizeof(struct d_dss_rule);
    sheet->at_rules.stride     = sizeof(struct d_dss_at_rule);

    struct d_internal_parser parser;

    parser.text       = _text;
    parser.length     = _length;
    parser.at         = 0;
    parser.line       = 1;
    parser.line_start = 0;
    parser.sheet      = sheet;
    parser.error      = _out_error;
    parser.failed     = false;

    while (!parser.failed)
    {
        d_internal_skip(&parser);

        if (parser.at >= parser.length)
        {
            return sheet;
        }

        if (d_internal_eat(&parser, "@"))
        {
            (void)d_internal_at_rule(&parser);
        }
        else
        {
            (void)d_internal_style_rule(&parser);
        }
    }

    d_dss_free(sheet);

    return NULL;
}


/*
d_dss_free
  Releases the sheet and everything it owns.
*/
void
d_dss_free(
    struct d_dss_sheet* _sheet
)
{
    if (_sheet)
    {
        free(_sheet->intern.text);
        free(_sheet->intern.offsets);
        free(_sheet->intern.buckets);

        free(_sheet->simples.data);
        free(_sheet->compounds.data);
        free(_sheet->steps.data);
        free(_sheet->selectors.data);
        free(_sheet->values.data);
        free(_sheet->declarations.data);
        free(_sheet->rules.data);
        free(_sheet->at_rules.data);

        free(_sheet);
    }

    return;
}


/*
d_internal_at
  Returns the record at an index, or NULL when the index is out of range.
*/
static const void*
d_internal_at(
    const struct d_internal_vector* _vector,
    uint32_t                        _at
)
{
    if (_at >= _vector->count)
    {
        return NULL;
    }

    return _vector->data + ((size_t)_at * _vector->stride);
}


size_t
d_dss_rule_count(const struct d_dss_sheet* _sheet)
{
    return _sheet ? _sheet->rules.count : 0u;
}

const struct d_dss_rule*
d_dss_rule_at(const struct d_dss_sheet* _sheet, size_t _at)
{
    return _sheet ? d_internal_at(&_sheet->rules, (uint32_t)_at) : NULL;
}

size_t
d_dss_at_rule_count(const struct d_dss_sheet* _sheet)
{
    return _sheet ? _sheet->at_rules.count : 0u;
}

const struct d_dss_at_rule*
d_dss_at_rule_at(const struct d_dss_sheet* _sheet, size_t _at)
{
    return _sheet ? d_internal_at(&_sheet->at_rules, (uint32_t)_at) : NULL;
}

const struct d_dss_selector*
d_dss_selector_at(const struct d_dss_sheet* _sheet, uint32_t _at)
{
    return _sheet ? d_internal_at(&_sheet->selectors, _at) : NULL;
}

const struct d_dss_compound*
d_dss_compound_at(const struct d_dss_sheet* _sheet, uint32_t _at)
{
    return _sheet ? d_internal_at(&_sheet->compounds, _at) : NULL;
}

const struct d_dss_step*
d_dss_step_at(const struct d_dss_sheet* _sheet, uint32_t _at)
{
    return _sheet ? d_internal_at(&_sheet->steps, _at) : NULL;
}

const struct d_dss_simple*
d_dss_simple_at(const struct d_dss_sheet* _sheet, uint32_t _at)
{
    return _sheet ? d_internal_at(&_sheet->simples, _at) : NULL;
}

const struct d_dss_declaration*
d_dss_declaration_at(const struct d_dss_sheet* _sheet, uint32_t _at)
{
    return _sheet ? d_internal_at(&_sheet->declarations, _at) : NULL;
}

const struct d_dss_value*
d_dss_value_at(const struct d_dss_sheet* _sheet, uint32_t _at)
{
    return _sheet ? d_internal_at(&_sheet->values, _at) : NULL;
}

const char*
d_dss_text(const struct d_dss_sheet* _sheet, uint32_t _id)
{
    return _sheet ? d_internal_intern_text(&_sheet->intern, _id) : "";
}
