/*******************************************************************************
* djinterp [parse]                                          arg_validator_unit.c
*
* CLI11's numbers with units -- AsNumberWithUnit and AsSizeValue -- declared
* in arg_validator_extra.h (3).
*   AsNumberWithUnit splits a value before the letters that end it, finds the
* unit among units kept in the order CLI11's std::map keeps them, and
* multiplies the number by the unit's factor in the type's own arithmetic,
* refusing an overflow exactly where CLI11's checked_multiply refuses one.
* AsSizeValue works its factors out from the unit's spelling, so that it
* holds nothing and never allocates.
*   Without D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS nothing here is compiled.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_validator_unit.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator_extra.h"  // corresponding header
// std
#include <math.h>    // isinf
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // calloc, malloc, free
#include <string.h>  // memchr, memcmp, memcpy, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_type, d_arg_type_builtin
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator, d_arg_validation
#include "./arg_internal.h"  // numbers, scratch, slices, validator parts
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t, intmax_t, uintmax_t


#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

//==============================================================================
// 1.  RECORDS
//==============================================================================

// d_arg_internal_unit
//   struct: one unit: its name, and its factor, an object of the number's
// type, widened.
struct d_arg_internal_unit
{
    struct d_arg_internal_slice  name;
    struct d_arg_internal_number factor;
};

// d_arg_internal_units
//   struct: what AsNumberWithUnit keeps, all of it owned: the number's type,
// CLI11's options, the units in the order CLI11's std::map keeps them --
// by name, the lowercase names when units match without case -- the name
// help gives the unit, and the text the names are slices of.
struct d_arg_internal_units
{
    const struct d_arg_type*    type;
    uint32_t                    options;
    struct d_arg_internal_unit* units;
    size_t                      count;
    struct d_arg_internal_slice unit_name;
    char*                       text;
    size_t                      text_size;
};

// d_arg_internal_unit_parts
//   struct: a value as AsNumberWithUnit splits it: the number before the
// unit, trimmed; the unit, the letters that end the value; and whether the
// unit is matched -- and written in messages -- in lowercase.
struct d_arg_internal_unit_parts
{
    struct d_pack_text number;
    struct d_pack_text unit;
    bool               fold;
};

// NOTHING
//   constant: the empty text.
static const struct d_pack_text NOTHING = { "", 0u };

// UNITS_NONE
//   constant: a units record that holds nothing yet.
static const struct d_arg_internal_units UNITS_NONE =
{
    NULL,        // type
    0u,          // options
    NULL,        // units
    0u,          // count
    { 0u, 0u },  // unit_name
    NULL,        // text
    0u           // text_size
};

// UNIT_PARTS_NONE
//   constant: a value not yet split.
static const struct d_arg_internal_unit_parts UNIT_PARTS_NONE =
{
    { "", 0u },  // number
    { "", 0u },  // unit
    false        // fold
};

// DEFAULT_UNIT_NAME
//   constant: the name help gives a unit when none is given, CLI11's
// default.
static const char DEFAULT_UNIT_NAME[] = "UNIT";

// SIZE_PREFIXES
//   constant: AsSizeValue's prefixes, each standing for the next power of
// its base.
static const char SIZE_PREFIXES[] = "kmgtpe";

// SIZE_UNITS
//   constant: AsSizeValue's units, as CLI11 lists them when a unit is not
// recognized: the keys of its std::map, in order.
static const char SIZE_UNITS[] =
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,"
    "tib}";

// SIZE_DESCRIPTIONS
//   constant: AsSizeValue's descriptions, with a kilobyte of 1024 bytes and
// of 1000.
static const char* const SIZE_DESCRIPTIONS[2] =
{
    "SIZE [b, kb(=1024b), ...]",
    "SIZE [b, kb(=1000b), kib(=1024b), ...]"
};

//==============================================================================
// 2.  VALUES
//==============================================================================

/*
d_arg_internal_unit_append
  A unit, as written or in lowercase; it holds letters only.
*/
D_STATIC int
d_arg_internal_unit_append(
    struct d_arg_buffer* _out,
    struct d_pack_text   _unit,
    bool                 _fold
)
{
    int status = D_ARG_OK;

    // as written
    if (!_fold)
    {
        return d_arg_buffer_append_text(_out,
                                        _unit);
    }

    // a letter at a time, lowered
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _unit.length);
         ++index)
    {
        status = d_arg_buffer_append_char(
                     _out,
                     d_arg_internal_to_lower(_unit.data[index]));
    }

    return status;
}

/*
d_arg_internal_unit_thrown
  The status a message stops validation with: the validation error CLI11
throws, or the failure that cost the message.
*/
D_STATIC int
d_arg_internal_unit_thrown(
    const struct d_arg_buffer* _message
)
{
    return (_message->failed != 0)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_ERROR_VALIDATION;
}

/*
d_arg_internal_unit_split
  AsNumberWithUnit's first steps: the value trimmed -- nothing left stops
validation with "Input is empty" -- then split before the ASCII letters that
end it into a number, trimmed, and a unit; a unit that must be there and is
not stops validation with "Missing mandatory unit". Trimming the front as
well, where CLI11 trims only the end, changes nothing: the number is
trimmed.
*/
D_STATIC int
d_arg_internal_unit_split(
    struct d_arg_validation*          _validation,
    uint32_t                          _options,
    struct d_arg_internal_unit_parts* _parts
)
{
    const struct d_pack_text value = d_arg_internal_trim(_validation->value);
    size_t                   begin = value.length;

    // nothing but space
    if (value.length == 0u)
    {
        return d_arg_internal_validator_stop(_validation,
                                             "Input is empty",
                                             NOTHING,
                                             "");
    }

    // the letters at the end are the unit
    while ( (begin > 0u) &&
            (d_arg_internal_is_alpha(value.data[begin - 1u])) )
    {
        --begin;
    }

    _parts->number = d_arg_internal_trim(d_arg_internal_span(value.data,
                                                             begin));
    _parts->unit   = d_arg_internal_span(value.data + begin,
                                         value.length - begin);
    _parts->fold   = ((_options & D_ARG_UNIT_CASE_INSENSITIVE) != 0u);

    // a unit that must be there
    if ( ((_options & D_ARG_UNIT_REQUIRED) != 0u) &&
         (_parts->unit.length == 0u) )
    {
        return d_arg_internal_validator_stop(_validation,
                                             "Missing mandatory unit",
                                             NOTHING,
                                             "");
    }

    return D_ARG_OK;
}

/*
d_arg_internal_unit_read
  The number converted into an object of the type as CLI11's lexical_cast
converts it; one that does not convert stops validation with "Value
<number> could not be converted to <TYPE>".
*/
D_STATIC int
d_arg_internal_unit_read(
    const struct d_arg_type* _type,
    struct d_arg_validation* _validation,
    struct d_pack_text       _number,
    void*                    _object
)
{
    const struct d_arg_convert_context context = { _validation->integer_rule,
                                                   _validation->option,
                                                   NULL };
    const int                          status  =
        d_arg_internal_lexical_cast(_type,
                                    _number,
                                    &context,
                                    _object);

    // converted, or stopped by more than the number
    if (status != D_ARG_ERROR_CONVERSION)
    {
        return status;
    }

    (void)d_arg_internal_validator_reject(_validation,
                                          "Value ",
                                          _number,
                                          " could not be converted to ");
    (void)d_arg_buffer_append_cstr(&_validation->message,
                                   _type->name);

    return d_arg_internal_unit_thrown(&_validation->message);
}

/*
d_arg_internal_unit_plain
  A number without a unit: once it converts, it stands, trimmed, as CLI11
leaves the value it trimmed in place.
*/
D_STATIC int
d_arg_internal_unit_plain(
    const struct d_arg_type* _type,
    struct d_arg_validation* _validation,
    struct d_pack_text       _number
)
{
    union d_arg_internal_scratch scratch = { 0.0L };
    void* const                  object  =
        d_arg_internal_scratch_acquire(&scratch,
                                       _type->size);

    // check if memory allocation was successful
    if (!object)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    const int status = d_arg_internal_unit_read(_type,
                                                _validation,
                                                _number,
                                                object);

    d_arg_internal_scratch_release(&scratch,
                                   object);

    return (status == D_ARG_OK)
           ? d_arg_validation_replace(_validation,
                                      _number)
           : status;
}

/*
d_arg_internal_unit_times_float
  CLI11's checked_multiply for `float`: the product formed in `float`,
refused when it is infinite though neither factor is.
*/
D_STATIC bool
d_arg_internal_unit_times_float(
    long double _a,
    long double _b,
    void*       _object
)
{
    const float a       = (float)_a;
    const float b       = (float)_b;
    const float product = a * b;

    // an overflow its factors did not bring
    if ( (isinf(product)) &&
         (!isinf(a))      &&
         (!isinf(b)) )
    {
        return false;
    }

    memcpy(_object,
           &product,
           sizeof(product));

    return true;
}

/*
d_arg_internal_unit_times_double
  As d_arg_internal_unit_times_float, in `double`.
*/
D_STATIC bool
d_arg_internal_unit_times_double(
    long double _a,
    long double _b,
    void*       _object
)
{
    const double a       = (double)_a;
    const double b       = (double)_b;
    const double product = a * b;

    // an overflow its factors did not bring
    if ( (isinf(product)) &&
         (!isinf(a))      &&
         (!isinf(b)) )
    {
        return false;
    }

    memcpy(_object,
           &product,
           sizeof(product));

    return true;
}

/*
d_arg_internal_unit_times_long_double
  As d_arg_internal_unit_times_float, in `long double`.
*/
D_STATIC bool
d_arg_internal_unit_times_long_double(
    long double _a,
    long double _b,
    void*       _object
)
{
    const long double product = _a * _b;

    // an overflow its factors did not bring
    if ( (isinf(product)) &&
         (!isinf(_a))     &&
         (!isinf(_b)) )
    {
        return false;
    }

    memcpy(_object,
           &product,
           sizeof(product));

    return true;
}

/*
d_arg_internal_unit_overflows
  CLI11's overflowCheck for two signed values, neither 0, 1 nor the least:
past the greatest value when their signs agree, below the least when they
differ, judged by quotients that cannot overflow themselves.
*/
D_STATIC bool
d_arg_internal_unit_overflows(
    intmax_t _a,
    intmax_t _b
)
{
    const intmax_t a = (_a < 0) ? -_a : _a;
    const intmax_t b = (_b < 0) ? -_b : _b;

    return ( (_a > 0) == (_b > 0) )
           ? ((INTMAX_MAX / a) < b)
           : ((INTMAX_MIN / a) > -b);
}

/*
d_arg_internal_unit_times_integer
  CLI11's checked_multiply for integers, on widened values: the product in
place of `*_a`, false when it would leave intmax_t or uintmax_t. The signed
tests are CLI11's: a product by 0 or 1 first, then the least value, which
has no magnitude, then the quotients.
*/
D_STATIC bool
d_arg_internal_unit_times_integer(
    struct d_arg_internal_number*       _a,
    const struct d_arg_internal_number* _b
)
{
    // unsigned: past the quotient it overflows
    if (_a->kind == (int)D_ARG_INTERNAL_NUMBER_UNSIGNED)
    {
        // a product past UINTMAX_MAX
        if ( (_a->u != 0u) &&
             (_b->u > (UINTMAX_MAX / _a->u)) )
        {
            return false;
        }

        _a->u *= _b->u;

        return true;
    }

    // by 0 or 1 nothing overflows
    if ( (_a->s == 0) ||
         (_b->s == 0) ||
         (_a->s == 1) ||
         (_b->s == 1) )
    {
        _a->s *= _b->s;

        return true;
    }

    // with the least value anything else overflows
    if ( (_a->s == INTMAX_MIN)                     ||
         (_b->s == INTMAX_MIN)                     ||
         (d_arg_internal_unit_overflows(_a->s,
                                        _b->s)) )
    {
        return false;
    }

    _a->s *= _b->s;

    return true;
}

/*
d_arg_internal_unit_multiply
  The object times the factor, in place; false on overflow, the object then
as it was. Floating point is multiplied in its own type, the narrowest
first since sizes may coincide; integers through their widened product,
refused when it leaves the type's range -- exactly where CLI11's tests in
the type itself refuse it.
*/
D_STATIC bool
d_arg_internal_unit_multiply(
    const struct d_arg_type*            _type,
    void*                               _object,
    const struct d_arg_internal_number* _factor
)
{
    struct d_arg_internal_number number = D_INTERNAL_ARG_NUMBER_INIT;

    (void)d_arg_internal_number_load(_type,
                                     _object,
                                     &number);

    // integers exactly, then narrowed into the type
    if (number.kind != (int)D_ARG_INTERNAL_NUMBER_REAL)
    {
        return ( (d_arg_internal_unit_times_integer(&number,
                                                    _factor)) &&
                 (d_arg_internal_number_store(_type,
                                              &number,
                                              _object)) );
    }

    // floating point in the type's own arithmetic
    if (_type->size == sizeof(float))
    {
        return d_arg_internal_unit_times_float(number.r,
                                               _factor->r,
                                               _object);
    }

    return (_type->size == sizeof(double))
           ? d_arg_internal_unit_times_double(number.r,
                                              _factor->r,
                                              _object)
           : d_arg_internal_unit_times_long_double(number.r,
                                                   _factor->r,
                                                   _object);
}

/*
d_arg_internal_unit_overflow
  CLI11's message for a product that overflows: the number, written as the
type writes it, and the unit, lowercase when units match without case.
*/
D_STATIC int
d_arg_internal_unit_overflow(
    const struct d_arg_type*                _type,
    const void*                             _object,
    const struct d_arg_internal_unit_parts* _parts,
    struct d_arg_validation*                _validation
)
{
    struct d_arg_buffer* const message = &_validation->message;

    d_arg_buffer_clear(message);
    (void)d_arg_internal_number_write(_type,
                                      _object,
                                      message);
    (void)d_arg_buffer_append_cstr(message,
                                   " multiplied by ");
    (void)d_arg_internal_unit_append(message,
                                     _parts->unit,
                                     _parts->fold);
    (void)d_arg_buffer_append_cstr(message,
                                   " factor would cause number overflow. "
                                   "Use smaller value.");

    return d_arg_internal_unit_thrown(message);
}

/*
d_arg_internal_unit_scale
  A number with a unit: the number converted, then multiplied by the unit's
factor; a number that does not convert, or a product that overflows, stops
validation.
*/
D_STATIC int
d_arg_internal_unit_scale(
    const struct d_arg_type*                _type,
    struct d_arg_validation*                _validation,
    const struct d_arg_internal_unit_parts* _parts,
    const struct d_arg_internal_number*     _factor,
    void*                                   _object
)
{
    const int status = d_arg_internal_unit_read(_type,
                                                _validation,
                                                _parts->number,
                                                _object);

    // a number that does not convert has stopped validation
    if (status != D_ARG_OK)
    {
        return status;
    }

    return (d_arg_internal_unit_multiply(_type,
                                         _object,
                                         _factor))
           ? D_ARG_OK
           : d_arg_internal_unit_overflow(_type,
                                          _object,
                                          _parts,
                                          _validation);
}

/*
d_arg_internal_unit_apply
  A value with a unit becomes a number of the type: the unit alone stands
for its factor, and a number is multiplied by it; the result, written as the
type writes values, replaces the value.
*/
D_STATIC int
d_arg_internal_unit_apply(
    const struct d_arg_type*                _type,
    struct d_arg_validation*                _validation,
    const struct d_arg_internal_unit_parts* _parts,
    const struct d_arg_internal_number*     _factor
)
{
    union d_arg_internal_scratch scratch = { 0.0L };
    void* const                  object  =
        d_arg_internal_scratch_acquire(&scratch,
                                       _type->size);

    // check if memory allocation was successful
    if (!object)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    int status = D_ARG_OK;

    // the unit alone, or a number with it
    if (_parts->number.length == 0u)
    {
        (void)d_arg_internal_number_store(_type,
                                          _factor,
                                          object);
    }
    else
    {
        status = d_arg_internal_unit_scale(_type,
                                           _validation,
                                           _parts,
                                           _factor,
                                           object);
    }

    // the result replaces the value
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_validator_replace(_validation,
                                                  _type,
                                                  object);
    }

    d_arg_internal_scratch_release(&scratch,
                                   object);

    return status;
}

/*
d_arg_internal_unit_unknown
  The start of CLI11's message for a unit it does not recognize: the unit,
lowercase when units match without case; the units allowed follow it.
*/
D_STATIC void
d_arg_internal_unit_unknown(
    const struct d_arg_internal_unit_parts* _parts,
    struct d_arg_buffer*                    _message
)
{
    d_arg_buffer_clear(_message);
    (void)d_arg_internal_unit_append(_message,
                                     _parts->unit,
                                     _parts->fold);
    (void)d_arg_buffer_append_cstr(_message,
                                   " unit not recognized. Allowed values: ");

    return;
}

//==============================================================================
// 3.  UNITS
//==============================================================================

/*
d_arg_internal_units_find
  The unit a value names: one whose name equals it, both folded to lowercase
when units match without case; NULL for none. The units are few.
*/
D_STATIC const struct d_arg_internal_unit*
d_arg_internal_units_find(
    const struct d_arg_internal_units*      _units,
    const struct d_arg_internal_unit_parts* _parts
)
{
    // each unit in turn
    for (size_t index = 0u; index < _units->count; ++index)
    {
        // a name equal to the unit
        if (d_arg_internal_names_match(
                d_arg_internal_slice_text(_units->text,
                                          _units->units[index].name),
                _parts->unit,
                _parts->fold,
                false))
        {
            return &_units->units[index];
        }
    }

    return NULL;
}

/*
d_arg_internal_units_unknown
  CLI11's message for a unit its map does not hold, the units allowed listed
in the map's order between braces.
*/
D_STATIC int
d_arg_internal_units_unknown(
    const struct d_arg_internal_units*      _units,
    const struct d_arg_internal_unit_parts* _parts,
    struct d_arg_validation*                _validation
)
{
    struct d_arg_buffer* const message = &_validation->message;

    d_arg_internal_unit_unknown(_parts,
                                message);
    (void)d_arg_buffer_append_char(message,
                                   '{');

    // the names, joined by commas
    for (size_t index = 0u; index < _units->count; ++index)
    {
        (void)d_arg_buffer_append_cstr(message,
                                       (index > 0u) ? "," : "");
        (void)d_arg_buffer_append_text(
                  message,
                  d_arg_internal_slice_text(_units->text,
                                            _units->units[index].name));
    }

    (void)d_arg_buffer_append_char(message,
                                   '}');

    return d_arg_internal_unit_thrown(message);
}

/*
d_arg_internal_units_validate
  CLI11's AsNumberWithUnit: the value split, a number alone kept once it
converts, and a unit looked up among the validator's units and applied.
*/
D_STATIC int
d_arg_internal_units_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_internal_units* const units  =
        (const struct d_arg_internal_units*)_validator->data;
    struct d_arg_internal_unit_parts         parts  = UNIT_PARTS_NONE;
    const int                                status =
        d_arg_internal_unit_split(_validation,
                                  units->options,
                                  &parts);

    // stopped, or a number without a unit
    if ( (status != D_ARG_OK) ||
         (parts.unit.length == 0u) )
    {
        return (status != D_ARG_OK)
               ? status
               : d_arg_internal_unit_plain(units->type,
                                           _validation,
                                           parts.number);
    }

    const struct d_arg_internal_unit* const unit =
        d_arg_internal_units_find(units,
                                  &parts);

    // a unit the map does not hold
    if (!unit)
    {
        return d_arg_internal_units_unknown(units,
                                            &parts,
                                            _validation);
    }

    return d_arg_internal_unit_apply(units->type,
                                     _validation,
                                     &parts,
                                     &unit->factor);
}

/*
d_arg_internal_units_describe
  CLI11's description: "<TYPE> [<unit name>]", or "<TYPE> <unit name>" when
a unit is required.
*/
D_STATIC int
d_arg_internal_units_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    const struct d_arg_internal_units* const units    =
        (const struct d_arg_internal_units*)_validator->data;
    const bool                               required =
        ((units->options & D_ARG_UNIT_REQUIRED) != 0u);

    (void)d_arg_buffer_append_cstr(_out,
                                   units->type->name);
    (void)d_arg_buffer_append_cstr(_out,
                                   (required) ? " " : " [");
    (void)d_arg_buffer_append_text(_out,
                                   d_arg_internal_slice_text(
                                       units->text,
                                       units->unit_name));

    return d_arg_buffer_append_cstr(_out,
                                    (required) ? "" : "]");
}

/*
d_arg_internal_units_release
  The units, their text, then the record; the validator's release function.
*/
D_STATIC void
d_arg_internal_units_release(
    void* _context
)
{
    struct d_arg_internal_units* const units =
        (struct d_arg_internal_units*)_context;

    // nothing to free without a record
    if (!units)
    {
        return;
    }

    free(units->units);
    free(units->text);
    free(units);

    return;
}

/*
d_arg_internal_units_copy
  The validator's copy function: the record, its units and its text copied
whole, which their slices make enough; NULL, with nothing kept, when an
allocation is refused.
*/
D_STATIC void*
d_arg_internal_units_copy(
    const void* _context
)
{
    // nothing to copy
    if (!_context)
    {
        return NULL;
    }

    const struct d_arg_internal_units* const source =
        (const struct d_arg_internal_units*)_context;
    const size_t                             bytes  =
        ((source->count > 0u) ? source->count : 1u) * sizeof(*source->units);
    struct d_arg_internal_units* const       copy   =
        (struct d_arg_internal_units*)malloc(sizeof(*copy));
    struct d_arg_internal_unit* const        units  =
        (struct d_arg_internal_unit*)malloc(bytes);
    char* const                              text   =
        (char*)malloc(source->text_size);

    // each allocation, or none
    if ( (!copy)  ||
         (!units) ||
         (!text) )
    {
        free(text);
        free(units);
        free(copy);

        return NULL;
    }

    *copy       = *source;
    copy->units = units;
    copy->text  = text;
    memcpy(units,
           source->units,
           bytes);
    memcpy(text,
           source->text,
           source->text_size);

    return copy;
}

/*
d_arg_internal_units_given
  Whether every unit has a name and a factor to read, and the units' count
can be allocated.
*/
D_STATIC bool
d_arg_internal_units_given(
    const struct d_arg_mapping* _units,
    size_t                      _count
)
{
    // more units than memory can hold
    if (_count > ((size_t)-1 / sizeof(struct d_arg_internal_unit)))
    {
        return false;
    }

    // each unit's two texts
    for (size_t index = 0u; index < _count; ++index)
    {
        // a missing text
        if ( (_units[index].from == NULL) ||
             (_units[index].to == NULL) )
        {
            return false;
        }
    }

    return true;
}

/*
d_arg_internal_units_text
  The units' names and the unit name copied into the record's text, which a
buffer gathers and the record then owns; a NUL closes it, so that the text
is never empty and always has storage to own. False when an allocation was
refused.
*/
D_STATIC bool
d_arg_internal_units_text(
    struct d_arg_internal_units* _units,
    const struct d_arg_mapping*  _mapping,
    const char*                  _unit_name
)
{
    struct d_arg_buffer text = D_ARG_BUFFER_INIT;

    // each name, sliced where it lands
    for (size_t index = 0u; index < _units->count; ++index)
    {
        const struct d_arg_internal_slice name =
        {
            text.length,                  // offset
            strlen(_mapping[index].from)  // length
        };

        _units->units[index].name = name;
        (void)d_arg_buffer_append(&text,
                                  _mapping[index].from,
                                  name.length);
    }

    _units->unit_name.offset = text.length;
    _units->unit_name.length = strlen(_unit_name);
    (void)d_arg_buffer_append_cstr(&text,
                                   _unit_name);
    (void)d_arg_buffer_append_char(&text,
                                   '\0');
    _units->text_size = text.length;
    _units->text      = (text.failed == 0)
                        ? d_arg_buffer_detach(&text)
                        : NULL;
    d_arg_buffer_release(&text);

    return (_units->text != NULL);
}

/*
d_arg_internal_units_factors
  Each unit's factor converted from its text to an object of the type, as
CLI11's lexical_cast converts one, under the configured integer rule, then
widened; D_ARG_ERROR_BAD_ARGUMENT for one that does not convert.
*/
D_STATIC int
d_arg_internal_units_factors(
    struct d_arg_internal_units* _units,
    const struct d_arg_mapping*  _mapping
)
{
    union d_arg_internal_scratch scratch = { 0.0L };
    void* const                  object  =
        d_arg_internal_scratch_acquire(&scratch,
                                       _units->type->size);
    int                          status  = (object != NULL)
                                           ? D_ARG_OK
                                           : D_ARG_ERROR_OUT_OF_MEMORY;

    // each factor in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _units->count);
         ++index)
    {
        status = d_arg_internal_lexical_cast(
                     _units->type,
                     d_arg_internal_text_of(_mapping[index].to),
                     NULL,
                     object);
        (void)d_arg_internal_number_load(_units->type,
                                         object,
                                         &_units->units[index].factor);
    }

    d_arg_internal_scratch_release(&scratch,
                                   object);

    return (status == D_ARG_ERROR_CONVERSION)
           ? D_ARG_ERROR_BAD_ARGUMENT
           : status;
}

/*
d_arg_internal_units_order
  std::string's order, which CLI11's std::map keeps its units in: the bytes
compared as unsigned, a prefix first.
*/
D_STATIC int
d_arg_internal_units_order(
    const struct d_arg_internal_units* _units,
    struct d_arg_internal_slice        _a,
    struct d_arg_internal_slice        _b
)
{
    const size_t shorter = (_a.length < _b.length) ? _a.length : _b.length;
    const int    order   = (shorter > 0u)
                           ? memcmp(_units->text + _a.offset,
                                    _units->text + _b.offset,
                                    shorter)
                           : 0;

    // the bytes decide, else the lengths
    if (order != 0)
    {
        return order;
    }

    return (_a.length < _b.length) ? -1 : (_a.length > _b.length) ? 1 : 0;
}

/*
d_arg_internal_units_sort
  An insertion sort by name: stable, so that of units with one name the
first given stays first, and fast enough for the few units there are.
*/
D_STATIC void
d_arg_internal_units_sort(
    struct d_arg_internal_units* _units
)
{
    // each unit moved down past the names greater than its own
    for (size_t index = 1u; index < _units->count; ++index)
    {
        const struct d_arg_internal_unit unit  = _units->units[index];
        size_t                           place = index;

        // the greater names move up
        while ( (place > 0u)                                         &&
                (d_arg_internal_units_order(_units,
                                            _units->units[place - 1u].name,
                                            unit.name) > 0) )
        {
            _units->units[place] = _units->units[place - 1u];
            --place;
        }

        _units->units[place] = unit;
    }

    return;
}

/*
d_arg_internal_units_unique
  std::map's construction from a list: of sorted units with one name, the
first given is kept and the others dropped.
*/
D_STATIC void
d_arg_internal_units_unique(
    struct d_arg_internal_units* _units
)
{
    size_t kept = (_units->count > 0u) ? 1u : 0u;

    // each unit whose name differs from the last kept
    for (size_t index = 1u; index < _units->count; ++index)
    {
        // a new name
        if (d_arg_internal_units_order(_units,
                                       _units->units[kept - 1u].name,
                                       _units->units[index].name) != 0)
        {
            _units->units[kept] = _units->units[index];
            ++kept;
        }
    }

    _units->count = kept;

    return;
}

/*
d_arg_internal_units_make
  The record: the units' names and the unit name copied, the factors
converted, then the units put in the map's order, of equal names the first
given kept; nothing is kept on failure.
*/
D_STATIC int
d_arg_internal_units_make(
    const struct d_arg_type*      _type,
    const struct d_arg_mapping*   _mapping,
    size_t                        _count,
    const char*                   _unit_name,
    struct d_arg_internal_units** _out
)
{
    struct d_arg_internal_units* const units =
        (struct d_arg_internal_units*)malloc(sizeof(*units));

    // check if memory allocation was successful
    if (!units)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    *units       = UNITS_NONE;
    units->type  = _type;
    units->count = _count;
    units->units = (struct d_arg_internal_unit*)calloc(
                       (_count > 0u) ? _count : 1u,
                       sizeof(*units->units));

    const int status = ( (units->units != NULL) &&
                         (d_arg_internal_units_text(units,
                                                    _mapping,
                                                    _unit_name)) )
                       ? d_arg_internal_units_factors(units,
                                                      _mapping)
                       : D_ARG_ERROR_OUT_OF_MEMORY;

    // a failure keeps nothing
    if (status != D_ARG_OK)
    {
        d_arg_internal_units_release(units);

        return status;
    }

    d_arg_internal_units_sort(units);
    d_arg_internal_units_unique(units);
    *_out = units;

    return D_ARG_OK;
}

/*
d_arg_internal_units_letters
  Whether a name is letters only, as CLI11's isalpha asks of a unit.
*/
D_STATIC bool
d_arg_internal_units_letters(
    struct d_pack_text _name
)
{
    // each character a letter
    for (size_t index = 0u; index < _name.length; ++index)
    {
        // anything else
        if (!d_arg_internal_is_alpha(_name.data[index]))
        {
            return false;
        }
    }

    return true;
}

/*
d_arg_internal_units_seen
  Whether a unit before `_index` has the name the unit at `_index` has.
*/
D_STATIC bool
d_arg_internal_units_seen(
    const struct d_arg_internal_units* _units,
    size_t                             _index
)
{
    // each earlier unit
    for (size_t earlier = 0u; earlier < _index; ++earlier)
    {
        // the same name
        if (d_arg_internal_units_order(_units,
                                       _units->units[earlier].name,
                                       _units->units[_index].name) == 0)
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_units_lower
  CLI11's lowercase map: each name lowered in place, in the map's order, the
first to lower to a name lowered before it refused with CLI11's message;
then the units sorted again, by their lowercase names.
*/
D_STATIC int
d_arg_internal_units_lower(
    struct d_arg_internal_units* _units,
    struct d_arg_buffer*         _reason
)
{
    // each name lowered, then compared with those lowered before it
    for (size_t index = 0u; index < _units->count; ++index)
    {
        const struct d_arg_internal_slice name    = _units->units[index].name;
        char* const                       letters = _units->text + name.offset;

        // the name, a letter at a time
        for (size_t at = 0u; at < name.length; ++at)
        {
            letters[at] = d_arg_internal_to_lower(letters[at]);
        }

        // a lowercase name met before
        if (d_arg_internal_units_seen(_units,
                                      index))
        {
            (void)d_arg_buffer_append_cstr(_reason,
                                           "Several matching lowercase unit "
                                           "representations are found: ");
            (void)d_arg_buffer_append_text(
                      _reason,
                      d_arg_internal_slice_text(_units->text,
                                                name));

            return D_ARG_ERROR_VALIDATION;
        }
    }

    d_arg_internal_units_sort(_units);

    return D_ARG_OK;
}

/*
d_arg_internal_units_check
  CLI11's validate_mapping, on the units in the map's order: an empty name,
then one of anything but letters, refused with CLI11's messages; then, for
units matching without case, the lowercase map.
*/
D_STATIC int
d_arg_internal_units_check(
    struct d_arg_internal_units* _units,
    struct d_arg_buffer*         _reason
)
{
    // each name, in the map's order
    for (size_t index = 0u; index < _units->count; ++index)
    {
        const struct d_pack_text name =
            d_arg_internal_slice_text(_units->text,
                                      _units->units[index].name);

        // empty, or more than letters
        if ( (name.length == 0u) ||
             (!d_arg_internal_units_letters(name)) )
        {
            (void)d_arg_buffer_append_cstr(_reason,
                                           (name.length == 0u)
                                           ? "Unit must not be empty."
                                           : "Unit must contain only "
                                             "letters.");

            return D_ARG_ERROR_VALIDATION;
        }
    }

    return ((_units->options & D_ARG_UNIT_CASE_INSENSITIVE) != 0u)
           ? d_arg_internal_units_lower(_units,
                                        _reason)
           : D_ARG_OK;
}

/*
d_arg_internal_units_validator
  The validator over a units record: CLI11's checks first, a failure
freeing the record and keeping its reason -- CLI11's message -- as the
failed validator's description; otherwise the record is the validator's
context, owned, copied and freed with it, and its values complete as
numbers.
*/
D_STATIC struct d_arg_validator
d_arg_internal_units_validator(
    struct d_arg_internal_units* _units
)
{
    struct d_arg_buffer reason = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_units_check(_units,
                                                            &reason);

    // a reason that could not be kept is a refused allocation
    if ( (status != D_ARG_OK) &&
         (reason.failed != 0) )
    {
        status = D_ARG_ERROR_OUT_OF_MEMORY;
    }

    // a failure frees the units, its reason kept
    if (status != D_ARG_OK)
    {
        const struct d_arg_validator failed =
            d_arg_internal_validator_failed(
                status,
                (status == D_ARG_ERROR_VALIDATION) ? reason.data : NULL);

        d_arg_buffer_release(&reason);
        d_arg_internal_units_release(_units);

        return failed;
    }

    struct d_arg_validator validator =
        d_arg_validator_custom(d_arg_internal_units_validate,
                               _units,
                               NULL);

    d_arg_buffer_release(&reason);
    validator.describe  = d_arg_internal_units_describe;
    validator.parameter = _units->type;
    validator.flags    |= D_ARG_VALIDATOR_NUMBER;
    d_arg_validator_own_context(&validator,
                                d_arg_internal_units_copy,
                                d_arg_internal_units_release);

    return validator;
}

/*
d_arg_validator_as_number_with_unit
  The units read into a record, which d_arg_internal_units_validator checks
as CLI11 checks its map and makes the validator over.
*/
struct d_arg_validator
d_arg_validator_as_number_with_unit(
    const struct d_arg_type*    _number,
    const struct d_arg_mapping* _units,
    size_t                      _count,
    uint32_t                    _options,
    const char*                 _name
)
{
    // an arithmetic type, and units to read
    if ( (d_arg_internal_number_of(_number) ==
          (int)D_ARG_INTERNAL_NUMBER_NONE)               ||
         (!d_arg_internal_type_convertible(_number))     ||
         ( (!_units) &&
           (_count > 0u) )                               ||
         ( (_units != NULL) &&
           (!d_arg_internal_units_given(_units,
                                        _count)) ) )
    {
        return d_arg_internal_validator_failed(D_ARG_ERROR_BAD_ARGUMENT,
                                               NULL);
    }

    struct d_arg_internal_units* units  = NULL;
    const int                    status =
        d_arg_internal_units_make(_number,
                                  _units,
                                  _count,
                                  (_name != NULL) ? _name
                                                  : DEFAULT_UNIT_NAME,
                                  &units);

    // a record that could not be made fails the validator
    if (status != D_ARG_OK)
    {
        return d_arg_internal_validator_failed(status,
                                               NULL);
    }

    units->options = _options;

    return d_arg_internal_units_validator(units);
}

//==============================================================================
// 4.  SIZES
//==============================================================================

/*
d_arg_internal_size_factor
  AsSizeValue's map without the map: `b` is 1; a prefix alone or with `b`
after it is its power of the base -- 1000 with `_kb_is_1000`, else 1024 --
and with `i` or `ib` after it its power of 1024. Case is folded, as
AsSizeValue matches units.
*/
D_STATIC bool
d_arg_internal_size_factor(
    struct d_pack_text            _unit,
    bool                          _kb_is_1000,
    struct d_arg_internal_number* _factor
)
{
    const char* const        prefix = (_unit.length > 0u)
                                      ? (const char*)memchr(
                                            SIZE_PREFIXES,
                                            d_arg_internal_to_lower(
                                                _unit.data[0]),
                                            sizeof(SIZE_PREFIXES) - 1u)
                                      : NULL;
    const struct d_pack_text after  = (prefix != NULL)
                                      ? d_arg_internal_span(_unit.data + 1,
                                                            _unit.length - 1u)
                                      : NOTHING;
    const bool               binary =
        ( (d_arg_internal_equal_nocase_cstr(after,
                                            "i")) ||
          (d_arg_internal_equal_nocase_cstr(after,
                                            "ib")) );

    _factor->kind = (int)D_ARG_INTERNAL_NUMBER_UNSIGNED;
    _factor->u    = 1u;

    // `b` alone is a byte
    if (d_arg_internal_equal_nocase_cstr(_unit,
                                         "b"))
    {
        return true;
    }

    // a prefix, alone or with `b`, `i` or `ib` after it
    if ( (!prefix)                                      ||
         ( (!binary)                                &&
           (after.length > 0u)                      &&
           (!d_arg_internal_equal_nocase_cstr(after,
                                              "b")) ) )
    {
        return false;
    }

    const uintmax_t base = ( (binary) ||
                             (!_kb_is_1000) )
                           ? 1024u
                           : 1000u;

    // the prefix's power of the base
    for (const char* step = SIZE_PREFIXES; step <= prefix; ++step)
    {
        _factor->u *= base;
    }

    return true;
}

/*
d_arg_internal_size_validate
  CLI11's AsSizeValue: AsNumberWithUnit with its options left at their
default -- units matched without case, and optional -- over a 64-bit
unsigned integer, the units' factors worked out rather than looked up.
*/
D_STATIC int
d_arg_internal_size_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_type* const   type   =
        (const struct d_arg_type*)_validator->parameter;
    struct d_arg_internal_unit_parts parts  = UNIT_PARTS_NONE;
    struct d_arg_internal_number     factor = D_INTERNAL_ARG_NUMBER_INIT;
    const int                        status =
        d_arg_internal_unit_split(_validation,
                                  D_ARG_UNIT_DEFAULT,
                                  &parts);

    // stopped, or a number without a unit
    if ( (status != D_ARG_OK) ||
         (parts.unit.length == 0u) )
    {
        return (status != D_ARG_OK)
               ? status
               : d_arg_internal_unit_plain(type,
                                           _validation,
                                           parts.number);
    }

    // a unit AsSizeValue's map does not hold
    if (!d_arg_internal_size_factor(parts.unit,
                                    (_validator->low.integer != 0L),
                                    &factor))
    {
        d_arg_internal_unit_unknown(&parts,
                                    &_validation->message);
        (void)d_arg_buffer_append_cstr(&_validation->message,
                                       SIZE_UNITS);

        return d_arg_internal_unit_thrown(&_validation->message);
    }

    return d_arg_internal_unit_apply(type,
                                     _validation,
                                     &parts,
                                     &factor);
}

/*
d_arg_internal_size_describe
  AsSizeValue's description, as CLI11 sets it for either size of a
kilobyte.
*/
D_STATIC int
d_arg_internal_size_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    return d_arg_buffer_append_cstr(
               _out,
               SIZE_DESCRIPTIONS[(_validator->low.integer != 0L) ? 1 : 0]);
}

/*
d_arg_validator_as_size_value
  The built-in 64-bit unsigned type as the parameter and the size of a
kilobyte in `low`, so that nothing is allocated; its values complete as
numbers.
*/
struct d_arg_validator
d_arg_validator_as_size_value(
    bool _kb_is_1000
)
{
    struct d_arg_validator validator =
        d_arg_validator_custom(d_arg_internal_size_validate,
                               NULL,
                               NULL);

    validator.describe    = d_arg_internal_size_describe;
    validator.parameter   = d_arg_type_builtin(D_ARG_BUILTIN_UINT64);
    validator.low.integer = (_kb_is_1000) ? 1L : 0L;
    validator.flags      |= D_ARG_VALIDATOR_NUMBER;

    return validator;
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
