/*******************************************************************************
* djinterp [c]                                                ostream_template.h
*
*   Templates onto a stream: text with named fields in it, compiled once into
* a small program and run as often as there is something to print. What the
* fields are, and where their values come from, is the caller's: a template
* names them, a lookup turns each name into an index when it is compiled, and
* a resolver gives each index a value when it runs.
*
*   THE LANGUAGE. Text is copied as it is; braces mark everything else.
*     {name}               the field's value
*     {name!conv}          converted for a format: json, csv, md, xml (raw is
*                          the default)
*     {name:spec}          formatted: [[fill]align][sign][#][0][width]
*                          [.precision][type], as std::format and Python
*                          have it; width and precision may be fields,
*                          {name:<{width}}
*     {?name}...{/}        shown when the field is true (text not empty, a
*                          number not zero, true); {:} starts the other branch
*     {^name}...{/}        shown when it is not
*     {{ and }}            a brace
*   A block may be closed as {/name}, and the name is then checked. Width and
* precision count characters (UTF-8 code points), not bytes. Precision cuts
* text before it is converted, so a cut JSON string is still a JSON string.
*
*   EVERY NAME IS CHECKED WHEN THE TEMPLATE IS COMPILED. A misspelt field is
* an error with its offset, not an empty space in the output. Running a
* compiled template parses nothing and looks nothing up: literals are writes
* of ranges of the source, and fields are calls to the resolver by index.
*
*   NOTHING HERE ALLOCATES. A compiled template is an array of operations the
* caller provides, over the source text, which the template borrows.
*
*   A TEMPLATE IS COMPILED ONCE, AND IS THE SAME EVERYWHERE IT RUNS. A program
* built of templates, such as the environment printer, compiles each once and
* runs it in every place it prints: a value that differs from place to place
* is a field, never a second template. To vary a part of the output, a
* template tests a field -- {?compact}...{:}...{/} -- and the caller binds that
* field where it wants the change (stream 7, ruled 11-04). Nesting depth is
* the one compile-time limit: D_CFG_OSTREAM_TEMPLATE_DEPTH, in
* config/c/text/cfg_ostream_template.h.
*
*   Values are 64-bit magnitudes with a sign, held as two 32-bit halves, so a
* value has one layout in every language and at every level, ISO strict
* C++98 included, where a 64-bit integer type may not exist.
*
*
* path:      /inc/djinterp/c/text/ostream_template.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VALUES
    ------
    1.  The value
         1.  dj_ostream_value_kind
         2.  dj_ostream_value
    2.  Making values
    3.  Reading values
2.  FORMATTING ONE VALUE
    --------------------
    1.  The specification
         1.  dj_ostream_align
         2.  dj_ostream_convert
         3.  dj_ostream_sign
         4.  D_OSTREAM_SPEC_ALTERNATE
         5.  D_OSTREAM_SPEC_ZERO
         6.  D_OSTREAM_SPEC_NO_PRECISION
         7.  D_OSTREAM_SPEC_MAX_WIDTH
         8.  dj_ostream_spec
    2.  Specifications and writing
3.  TEMPLATES
    ---------
    1.  Compiled templates
         1.  dj_ostream_template_status
         2.  dj_ostream_template_code
         3.  D_OSTREAM_TEMPLATE_NO_FIELD
         4.  dj_ostream_template_op
         5.  dj_ostream_template
         6.  dj_ostream_template_error
    2.  The caller's fields
         1.  dj_ostream_template_lookup_fn
         2.  dj_ostream_template_resolve_fn
    3.  Compiling and running
*/

#ifndef DJINTERP_C_TEXT_OSTREAM_TEMPLATE_H
#define DJINTERP_C_TEXT_OSTREAM_TEMPLATE_H 1

// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strlen
// djinterp
#include "../djinterp.h"              // framework root
#include "../util/ostream/ostream.h"  // struct dj_ostream
// re_std
#include "../../../re_std/c/cstdint/dstdint.h"  // int32_t, uint32_t, uint16_t;
                                                // INT64_MAX, int64_t, uint64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  VALUES
//==============================================================================


// 1.1    The value
//------------------------------------------------------------------------------
// 1.1.1
// dj_ostream_value_kind
//   enum: what a value is.
enum dj_ostream_value_kind
{
    D_OSTREAM_VALUE_NONE    = 0,  // nothing: empty, or null as JSON
    D_OSTREAM_VALUE_TEXT    = 1,  // bytes, UTF-8 by convention
    D_OSTREAM_VALUE_INTEGER = 2,  // a sign and a 64-bit magnitude
    D_OSTREAM_VALUE_BOOLEAN = 3,  // true or false
    D_OSTREAM_VALUE_REAL    = 4   // a double
};

// 1.1.2
// dj_ostream_value
//   struct: one field's value, made by section 1.2. The text is borrowed, and
// must stay alive until the value is written; it need not end in a NUL.
//
//   An integer is a magnitude, `high` and `low`, and a sign, so every int64_t
// and every uint64_t is one, and the struct is the same in C and in every C++,
// whether or not that build can spell a 64-bit type.
struct dj_ostream_value
{
    const char* text;      // TEXT: the bytes
    size_t      size;      // TEXT: their count
    double      real;      // REAL: the value
    uint32_t    low;       // INTEGER: magnitude, bits 0-31; BOOLEAN: 0 or 1
    uint32_t    high;      // INTEGER: magnitude, bits 32-63
    int32_t     kind;      // enum dj_ostream_value_kind
    int32_t     negative;  // INTEGER: 1 when below zero; else 0
};

// 1.2    Making values
//------------------------------------------------------------------------------
//   None of these can fail. Text is borrowed: a null pointer makes NONE.

/**
 * @brief Nothing.
 *
 * @return a NONE value.
 */
D_INLINE struct dj_ostream_value
d_ostream_value_none(void)
{
    struct dj_ostream_value value = { NULL, 0u, 0.0, 0u, 0u, 0, 0 };

    return value;
}

/**
 * @brief Text of a known length.
 *
 * @param[in] _text  the bytes, borrowed; null makes NONE.
 * @param[in] _size  how many.
 * @return the value.
 */
D_INLINE struct dj_ostream_value
d_ostream_value_text_n(
    const char* _text,
    size_t      _size
)
{
    struct dj_ostream_value value = d_ostream_value_none();

    // a null pointer has no text to borrow
    if (_text != NULL)
    {
        value.text = _text;
        value.size = _size;
        value.kind = D_OSTREAM_VALUE_TEXT;
    }

    return value;
}

/**
 * @brief A NUL-terminated string.
 *
 * @param[in] _text  the string, borrowed; null makes NONE.
 * @return the value.
 */
D_INLINE struct dj_ostream_value
d_ostream_value_text(
    const char* _text
)
{
    return d_ostream_value_text_n(_text,
                                  (_text != NULL) ? strlen(_text) : 0u);
}

/**
 * @brief An unsigned long.
 *
 * @param[in] _value  the value.
 * @return the value.
 */
D_INLINE struct dj_ostream_value
d_ostream_value_ulong(
    unsigned long _value
)
{
    struct dj_ostream_value value = d_ostream_value_none();

    // two shifts of 16, so a 32-bit unsigned long shifts to 0, never too far
    value.kind = D_OSTREAM_VALUE_INTEGER;
    value.low  = (uint32_t)(_value & 0xFFFFFFFFul);
    value.high = (uint32_t)(((_value >> 16) >> 16) & 0xFFFFFFFFul);

    return value;
}

/**
 * @brief A long.
 *
 * @param[in] _value  the value.
 * @return the value.
 */
D_INLINE struct dj_ostream_value
d_ostream_value_long(
    long _value
)
{
    // the magnitude is taken in unsigned arithmetic, which LONG_MIN survives
    struct dj_ostream_value value =
        d_ostream_value_ulong( (_value < 0L)
                                   ? (0ul - (unsigned long)_value)
                                   : (unsigned long)_value );

    value.negative = (_value < 0L) ? 1 : 0;

    return value;
}

/**
 * @brief A size_t, which may be wider than unsigned long.
 *
 * @param[in] _value  the value.
 * @return the value.
 */
D_INLINE struct dj_ostream_value
d_ostream_value_size(
    size_t _value
)
{
    struct dj_ostream_value value = d_ostream_value_none();

    value.kind = D_OSTREAM_VALUE_INTEGER;
    value.low  = (uint32_t)(_value & 0xFFFFFFFFu);
    value.high = (uint32_t)(((_value >> 16) >> 16) & 0xFFFFFFFFu);

    return value;
}

#if defined(INT64_MAX)
/**
 * @brief A uint64_t.
 *
 * @param[in] _value  the value.
 * @return the value.
 */
D_INLINE struct dj_ostream_value
d_ostream_value_uint64(
    uint64_t _value
)
{
    struct dj_ostream_value value = d_ostream_value_none();

    value.kind = D_OSTREAM_VALUE_INTEGER;
    value.low  = (uint32_t)(_value & 0xFFFFFFFFu);
    value.high = (uint32_t)(_value >> 32);

    return value;
}

/**
 * @brief An int64_t.
 *
 * @param[in] _value  the value.
 * @return the value.
 */
D_INLINE struct dj_ostream_value
d_ostream_value_int64(
    int64_t _value
)
{
    // as for long: INT64_MIN's magnitude exists only unsigned
    struct dj_ostream_value value =
        d_ostream_value_uint64( (_value < 0)
                                    ? ((uint64_t)0u - (uint64_t)_value)
                                    : (uint64_t)_value );

    value.negative = (_value < 0) ? 1 : 0;

    return value;
}
#endif  // INT64_MAX

/**
 * @brief A boolean.
 *
 * @param[in] _value  non-zero for true.
 * @return the value.
 */
D_INLINE struct dj_ostream_value
d_ostream_value_bool(
    int _value
)
{
    struct dj_ostream_value value = d_ostream_value_none();

    value.kind = D_OSTREAM_VALUE_BOOLEAN;
    value.low  = (_value != 0) ? 1u : 0u;

    return value;
}

/**
 * @brief A double.
 *
 * @param[in] _value  the value.
 * @return the value.
 */
D_INLINE struct dj_ostream_value
d_ostream_value_real(
    double _value
)
{
    struct dj_ostream_value value = d_ostream_value_none();

    value.kind = D_OSTREAM_VALUE_REAL;
    value.real = _value;

    return value;
}

// 1.3    Reading values
//------------------------------------------------------------------------------
/**
 * @brief Whether a value counts as true where a template asks ({?name}).
 *
 * @note Text is true when it is not empty, a number when it is not zero
 *       (not-a-number is not zero), a boolean when it is true; NONE never.
 *
 * @param[in] _value  the value; null is false.
 * @return 1 or 0.
 */
D_INLINE int32_t
d_ostream_value_is_true(
    const struct dj_ostream_value* _value
)
{
    // each kind's own sense of empty
    if (_value == NULL)
    {
        return 0;
    }

    switch (_value->kind)
    {
        case D_OSTREAM_VALUE_TEXT:
            return (_value->size != 0u) ? 1 : 0;
        case D_OSTREAM_VALUE_INTEGER:
            return ((_value->low | _value->high) != 0u) ? 1 : 0;
        case D_OSTREAM_VALUE_BOOLEAN:
            return (_value->low != 0u) ? 1 : 0;
        case D_OSTREAM_VALUE_REAL:
            return (_value->real != 0.0) ? 1 : 0;
        default:
            return 0;
    }
}

/**
 * @brief An integer value as a long, clamped to long's range.
 *
 * @note Text, NONE and REAL read as 0; a boolean as 0 or 1.
 *
 * @param[in] _value  the value; null reads as 0.
 * @return the value, or LONG_MAX or LONG_MIN when it does not fit.
 */
long    d_ostream_value_to_long(const struct dj_ostream_value* _value);


//==============================================================================
// 2.  FORMATTING ONE VALUE
//==============================================================================


// 2.1    The specification
//------------------------------------------------------------------------------
// 2.1.1
// dj_ostream_align
//   enum: where a value sits in its width.
enum dj_ostream_align
{
    D_OSTREAM_ALIGN_DEFAULT = 0,  // text left, numbers right
    D_OSTREAM_ALIGN_LEFT    = 1,  // '<'
    D_OSTREAM_ALIGN_RIGHT   = 2,  // '>'
    D_OSTREAM_ALIGN_CENTER  = 3   // '^'; an odd pad's extra goes right
};

// 2.1.2
// dj_ostream_convert
//   enum: the format a value is written for.
enum dj_ostream_convert
{
    D_OSTREAM_CONVERT_RAW      = 0,  // as it is
    D_OSTREAM_CONVERT_JSON     = 1,  // a JSON value: "text", 12, true, null
    D_OSTREAM_CONVERT_CSV      = 2,  // an RFC 4180 field, quoted if need be
    D_OSTREAM_CONVERT_MARKDOWN = 3,  // inline Markdown, a table cell's too
    D_OSTREAM_CONVERT_XML      = 4   // XML or HTML character data
};

// 2.1.3
// dj_ostream_sign
//   enum: what a number that is not negative is written with.
enum dj_ostream_sign
{
    D_OSTREAM_SIGN_MINUS = 0,  // '-': nothing
    D_OSTREAM_SIGN_PLUS  = 1,  // '+': a plus sign
    D_OSTREAM_SIGN_SPACE = 2   // ' ': a space
};

// 2.1.4
// D_OSTREAM_SPEC_ALTERNATE
//   constant: flag: '#': hexadecimal, octal and binary get 0x, 0o and 0b.
#define D_OSTREAM_SPEC_ALTERNATE     0x01

// 2.1.5
// D_OSTREAM_SPEC_ZERO
//   constant: flag: '0': a number with no alignment given is padded with
// zeros after its sign and prefix.
#define D_OSTREAM_SPEC_ZERO          0x02

// 2.1.6
// D_OSTREAM_SPEC_NO_PRECISION
//   constant: dj_ostream_spec's precision when none was given.
#define D_OSTREAM_SPEC_NO_PRECISION  0xFFFFu

// 2.1.7
// D_OSTREAM_SPEC_MAX_WIDTH
//   constant: the largest width and precision; a field's value above it is
// taken as it, so a stray number in a variable cannot write megabytes.
#define D_OSTREAM_SPEC_MAX_WIDTH     4096u

// 2.1.8
// dj_ostream_spec
//   struct: how one value is written. d_ostream_spec_init gives the default:
// as it is, no width.
//
//   `type` is printf's letter or 0: d (decimal), x and X (hexadecimal), o
// (octal) and b (binary) for integers and booleans; f, e and g for reals; s
// for booleans as true and false. A letter applies to the kinds it names and
// is ignored by the rest, so one template serves values of several kinds.
struct dj_ostream_spec
{
    uint16_t width;      // the least characters written; 0 for any
    uint16_t precision;  // text: the most characters kept; a real's digits
    char     fill;       // what pads; ' ' unless given
    uint8_t  align;      // enum dj_ostream_align
    uint8_t  convert;    // enum dj_ostream_convert
    uint8_t  sign;       // enum dj_ostream_sign
    uint8_t  type;       // see above; 0 for the kind's default
    uint8_t  flags;      // D_OSTREAM_SPEC_*
};

// 2.2    Specifications and writing
//------------------------------------------------------------------------------
/**
 * @brief Gives the default specification: as it is, no width.
 *
 * @param[out] _spec  the specification.
 */
void    d_ostream_spec_init(struct dj_ostream_spec* _spec);

/**
 * @brief Reads a specification as a template writes one after a field's
 *        name: an optional "!conv", then an optional ":spec".
 *
 * @note Width and precision must be numbers here; a field in their place
 *       is for templates.
 *
 * @param[out] _spec  the specification; left at the default on failure.
 * @param[in]  _text  the text, "!json:>12" say; may be null when _size is 0.
 * @param[in]  _size  its length.
 * @return D_OSTREAM_TEMPLATE_OK, or why it was refused:
 *         D_OSTREAM_TEMPLATE_BAD_CONVERSION or D_OSTREAM_TEMPLATE_BAD_SPEC.
 */
int32_t d_ostream_spec_parse(struct dj_ostream_spec* _spec,
                             const char*             _text,
                             size_t                  _size);

/**
 * @brief Writes one value as a specification says.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _value  the value; null writes as NONE.
 * @param[in]     _spec   how; null for the default.
 * @return as d_ostream_write().
 */
int32_t d_ostream_write_value(struct dj_ostream*             _os,
                              const struct dj_ostream_value* _value,
                              const struct dj_ostream_spec*  _spec);


//==============================================================================
// 3.  TEMPLATES
//==============================================================================


// 3.1    Compiled templates
//------------------------------------------------------------------------------
// 3.1.1
// dj_ostream_template_status
//   enum: whether a template compiled, and if not, why.
enum dj_ostream_template_status
{
    D_OSTREAM_TEMPLATE_OK             = 0,
    D_OSTREAM_TEMPLATE_INVALID        = 1,  // a bad argument
    D_OSTREAM_TEMPLATE_SYNTAX         = 2,  // a stray brace; an unclosed tag
    D_OSTREAM_TEMPLATE_UNKNOWN_FIELD  = 3,  // the lookup does not know it
    D_OSTREAM_TEMPLATE_BAD_SPEC       = 4,  // after the ':'
    D_OSTREAM_TEMPLATE_BAD_CONVERSION = 5,  // after the '!'
    D_OSTREAM_TEMPLATE_NESTING        = 6,  // a block unclosed, unopened,
                                            // misnamed or too deep
    D_OSTREAM_TEMPLATE_CAPACITY       = 7,  // more operations than room
    D_OSTREAM_TEMPLATE_STATUS_COUNT   = 8
};

// 3.1.2
// dj_ostream_template_code
//   enum: what one operation does.
enum dj_ostream_template_code
{
    D_OSTREAM_TEMPLATE_LITERAL = 0,  // write source[operand, operand+extent)
    D_OSTREAM_TEMPLATE_FIELD   = 1,  // write field `operand` per spec
    D_OSTREAM_TEMPLATE_IF      = 2,  // field `operand` false: go to `extent`
    D_OSTREAM_TEMPLATE_UNLESS  = 3,  // field `operand` true: go to `extent`
    D_OSTREAM_TEMPLATE_JUMP    = 4   // go to `extent` (a block's {:})
};

// 3.1.3
// D_OSTREAM_TEMPLATE_NO_FIELD
//   constant: an operation's width_field or precision_field when the width or
// precision is a number, not a field. Field indices stop below it.
#define D_OSTREAM_TEMPLATE_NO_FIELD  0xFFFFu

// 3.1.4
// dj_ostream_template_op
//   struct: one operation of a compiled template.
struct dj_ostream_template_op
{
    uint32_t               operand;          // LITERAL: an offset; else a field
    uint32_t               extent;           // LITERAL: a length; else a target
    struct dj_ostream_spec spec;             // FIELD: how
    uint16_t               width_field;      // FIELD: or NO_FIELD
    uint16_t               precision_field;  // FIELD: or NO_FIELD
    uint8_t                code;             // enum dj_ostream_template_code
    uint8_t                reserved;         // pad; 0
};

// 3.1.5
// dj_ostream_template
//   struct: a compiled template: its operations, and the source their
// literals point into. Both are borrowed.
struct dj_ostream_template
{
    const char*                          source;
    const struct dj_ostream_template_op* ops;
    uint32_t                             count;
    uint32_t                             reserved;  // pad; 0
};

// 3.1.6
// dj_ostream_template_error
//   struct: where and why a template failed to compile, and how many
// operations it needs, which is known even when there was no room for them.
struct dj_ostream_template_error
{
    size_t   offset;  // the byte the problem was found at
    uint32_t needed;  // CAPACITY: operations the template needs
    int32_t  status;  // enum dj_ostream_template_status
};

// 3.2    The caller's fields
//------------------------------------------------------------------------------
// 3.2.1
// dj_ostream_template_lookup_fn
//   function pointer: a field's index from its name, or -1 for a name it does
// not know. Called while a template compiles, never while it runs. An index
// must be below D_OSTREAM_TEMPLATE_NO_FIELD. The name is not NUL-terminated.
typedef int32_t (*dj_ostream_template_lookup_fn)(void*       _context,
                                                 const char* _name,
                                                 size_t      _size);

// 3.2.2
// dj_ostream_template_resolve_fn
//   function pointer: a field's value, by the index its lookup gave. Returns
// 1, or 0 to stop the template, which is then recorded on the stream as
// D_OSTREAM_INVALID.
typedef int32_t (*dj_ostream_template_resolve_fn)(
                    void*                    _context,
                    uint32_t                 _field,
                    struct dj_ostream_value* _value);

// 3.3    Compiling and running
//------------------------------------------------------------------------------
/**
 * @brief Compiles a template into the caller's operations.
 *
 * @note The template borrows _source: it must outlive the template and stay
 *       unchanged. A null _source compiles as an empty template.
 *
 * @param[out] _template  the template; empty on failure.
 * @param[out] _ops       the operations' storage.
 * @param[in]  _capacity  how many operations it holds.
 * @param[in]  _source    the template's text, NUL-terminated.
 * @param[in]  _lookup    names to indices; null when the template may have
 *                        no fields.
 * @param[in]  _context   the lookup's.
 * @param[out] _error     where and why it failed, and the operations it
 *                        needs; may be null.
 * @return D_OSTREAM_TEMPLATE_OK, or why it failed.
 */
int32_t d_ostream_template_compile(
            struct dj_ostream_template*       _template,
            struct dj_ostream_template_op*    _ops,
            size_t                            _capacity,
            const char*                       _source,
            dj_ostream_template_lookup_fn     _lookup,
            void*                             _context,
            struct dj_ostream_template_error* _error);

/**
 * @brief Runs a compiled template onto a stream.
 *
 * @param[in,out] _os        the stream.
 * @param[in]     _template  the template.
 * @param[in]     _resolve   the fields' values; may be null only for a
 *                           template with no fields.
 * @param[in]     _context   the resolver's.
 * @return as d_ostream_write(); 0 as well when the resolver stopped it or a
 *         field had no resolver, recorded as D_OSTREAM_INVALID.
 */
int32_t d_ostream_template_render(
            struct dj_ostream*                _os,
            const struct dj_ostream_template* _template,
            dj_ostream_template_resolve_fn    _resolve,
            void*                             _context);

/**
 * @brief A template status's name, "D_OSTREAM_TEMPLATE_SYNTAX" say.
 *
 * @param[in] _status  the status.
 * @return its name; "unknown" for a value that is none.
 */
const char* d_ostream_template_status_name(int32_t _status);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEXT_OSTREAM_TEMPLATE_H
