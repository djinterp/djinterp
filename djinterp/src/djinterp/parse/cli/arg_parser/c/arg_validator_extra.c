/*******************************************************************************
* djinterp [parse]                                         arg_validator_extra.c
*
* CLI11's extra checks, declared in arg_validator_extra.h (1).
*   TypeValidator and Number convert a value as CLI11's lexical_cast does and
* reject what does not convert; Bound clamps what does to its bounds, which
* it keeps as a range keeps them; and ValidIPV4 reads four integers between
* three dots. None allocates. The sets and mappings are arg_validator_
* member.c's, and the numbers with units arg_validator_unit.c's.
*   Without D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS nothing here is compiled.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_validator_extra.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator_extra.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_type, d_arg_type_builtin
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator, d_arg_validation
#include "./arg_internal.h"  // numbers, scratch, spans, validator parts
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uintmax_t


#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

//==============================================================================
// 1.  CHECKS
//==============================================================================

/*
d_arg_internal_extra_type_validate
  CLI11's TypeValidator: the value converted into a scratch object, which is
then dropped, and rejected with "Failed parsing <value> as a <TYPE>" when it
does not convert.
*/
D_STATIC int
d_arg_internal_extra_type_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_type* const     type    =
        (const struct d_arg_type*)_validator->parameter;
    const struct d_arg_convert_context context = { _validation->integer_rule,
                                                   _validation->option,
                                                   NULL };
    union d_arg_internal_scratch       scratch = { 0.0L };
    void* const                        object  =
        d_arg_internal_scratch_acquire(&scratch,
                                       type->size);

    // check if memory allocation was successful
    if (!object)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    const int status = d_arg_internal_lexical_cast(type,
                                                   _validation->value,
                                                   &context,
                                                   object);

    d_arg_internal_scratch_release(&scratch,
                                   object);

    // converted, or stopped by more than the value
    if (status != D_ARG_ERROR_CONVERSION)
    {
        return status;
    }

    (void)d_arg_internal_validator_reject(_validation,
                                          "Failed parsing ",
                                          _validation->value,
                                          " as a ");

    return d_arg_buffer_append_cstr(&_validation->message,
                                    type->name);
}

/*
d_arg_internal_extra_type_describe
  CLI11's TypeValidator description: the type's name.
*/
D_STATIC int
d_arg_internal_extra_type_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    const struct d_arg_type* const type =
        (const struct d_arg_type*)_validator->parameter;

    return d_arg_buffer_append_cstr(_out,
                                    type->name);
}

/*
d_arg_internal_extra_number_describe
  Number's description, as CLI11 names it.
*/
D_STATIC int
d_arg_internal_extra_number_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    (void)_validator;

    return d_arg_buffer_append_cstr(_out,
                                    "NUMBER");
}

/*
d_arg_validator_type
  The type kept as the parameter, so that nothing is allocated; its values
must be plain data, converted into scratch storage and dropped.
*/
struct d_arg_validator
d_arg_validator_type(
    const struct d_arg_type* _type
)
{
    // a type whose values can be converted and dropped
    if (!d_arg_internal_type_convertible(_type))
    {
        return d_arg_internal_validator_failed(D_ARG_ERROR_BAD_ARGUMENT,
                                               NULL);
    }

    struct d_arg_validator validator =
        d_arg_validator_custom(d_arg_internal_extra_type_validate,
                               NULL,
                               NULL);

    validator.describe  = d_arg_internal_extra_type_describe;
    validator.parameter = _type;

    return validator;
}

/*
d_arg_validator_number
  The type validator of the built-in `double`, described as CLI11's Number.
*/
struct d_arg_validator
d_arg_validator_number(void)
{
    struct d_arg_validator validator =
        d_arg_validator_type(d_arg_type_builtin(D_ARG_BUILTIN_DOUBLE));

    validator.describe = d_arg_internal_extra_number_describe;

    return validator;
}

/*
d_arg_internal_extra_bound_validate
  CLI11's Bound: a value that does not convert is rejected with "Value
<value> could not be converted"; one below the low bound becomes the low
bound, one above the high bound the high bound; anything else -- a NaN,
which neither comparison catches, among it -- stands.
*/
D_STATIC int
d_arg_internal_extra_bound_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_type* const type   =
        (const struct d_arg_type*)_validator->parameter;
    struct d_arg_internal_number   value  = D_INTERNAL_ARG_NUMBER_INIT;
    struct d_arg_internal_number   low    = D_INTERNAL_ARG_NUMBER_INIT;
    struct d_arg_internal_number   high   = D_INTERNAL_ARG_NUMBER_INIT;
    bool                           read   = false;
    const int                      status =
        d_arg_internal_validator_read(_validator,
                                      _validation,
                                      &value,
                                      &read);

    // a refused allocation, or a value that does not convert
    if ( (status != D_ARG_OK) ||
         (!read) )
    {
        return (status != D_ARG_OK)
               ? status
               : d_arg_internal_validator_reject(_validation,
                                                 "Value ",
                                                 _validation->value,
                                                 " could not be converted");
    }

    d_arg_internal_validator_bounds(_validator,
                                    &low,
                                    &high);

    // below the low bound
    if (d_arg_internal_number_compare(&value,
                                      &low) == -1)
    {
        return d_arg_internal_validator_replace(_validation,
                                                type,
                                                &_validator->low);
    }

    return (d_arg_internal_number_compare(&value,
                                          &high) == 1)
           ? d_arg_internal_validator_replace(_validation,
                                              type,
                                              &_validator->high)
           : D_ARG_OK;
}

/*
d_arg_internal_extra_bound_describe
  CLI11's Bound description: "<TYPE> bounded to [<low> - <high>]", the
bounds streamed as a range's are.
*/
D_STATIC int
d_arg_internal_extra_bound_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    const struct d_arg_type* const type =
        (const struct d_arg_type*)_validator->parameter;

    (void)d_arg_buffer_append_cstr(_out,
                                   type->name);
    (void)d_arg_buffer_append_cstr(_out,
                                   " bounded to ");

    return d_arg_internal_validator_interval(_validator,
                                             _out);
}

/*
d_arg_validator_bound
  A range -- its checks on the arguments, its type and bounds kept in the
validator -- with Bound's functions in place of the range's.
*/
struct d_arg_validator
d_arg_validator_bound(
    const struct d_arg_type* _type,
    const void*              _low,
    const void*              _high
)
{
    struct d_arg_validator validator = d_arg_validator_range(_type,
                                                             _low,
                                                             _high);

    // Bound's functions on a range that could be made
    if (validator.status == D_ARG_OK)
    {
        validator.validate = d_arg_internal_extra_bound_validate;
        validator.describe = d_arg_internal_extra_bound_describe;
    }

    return validator;
}

/*
d_arg_validator_bound_long
  d_arg_validator_bound over the built-in `long`.
*/
struct d_arg_validator
d_arg_validator_bound_long(
    long _low,
    long _high
)
{
    return d_arg_validator_bound(d_arg_type_builtin(D_ARG_BUILTIN_LONG),
                                 &_low,
                                 &_high);
}

/*
d_arg_validator_bound_ulong
  d_arg_validator_bound over the built-in `unsigned long`.
*/
struct d_arg_validator
d_arg_validator_bound_ulong(
    unsigned long _low,
    unsigned long _high
)
{
    return d_arg_validator_bound(
               d_arg_type_builtin(D_ARG_BUILTIN_UNSIGNED_LONG),
               &_low,
               &_high);
}

/*
d_arg_validator_bound_double
  d_arg_validator_bound over the built-in `double`.
*/
struct d_arg_validator
d_arg_validator_bound_double(
    double _low,
    double _high
)
{
    return d_arg_validator_bound(d_arg_type_builtin(D_ARG_BUILTIN_DOUBLE),
                                 &_low,
                                 &_high);
}

/*
d_arg_internal_extra_ipv4_end
  Where a part of an address ends: at the next dot, or at the end.
*/
D_STATIC size_t
d_arg_internal_extra_ipv4_end(
    struct d_pack_text _value,
    size_t             _start
)
{
    size_t end = _start;

    // up to the next dot
    while ( (end < _value.length) &&
            (_value.data[end] != '.') )
    {
        ++end;
    }

    return end;
}

/*
d_arg_internal_extra_ipv4_part
  One part of an address, read as an integer under the validation's rule:
CLI11's "Failed parsing number (<part>)" for one that is not an integer an
`int` holds -- the empty part among them -- and "Each IP number must be
between 0 and 255 <part>" for an `int` outside an octet's range. Unlike
CLI11's conversion to `int`, the word `true` is not an integer here.
*/
D_STATIC int
d_arg_internal_extra_ipv4_part(
    struct d_pack_text       _part,
    struct d_arg_validation* _validation
)
{
    struct d_arg_internal_integer number = D_INTERNAL_ARG_INTEGER_INIT;
    const bool                    read   =
        ( (_part.length > 0u)                                  &&
          (d_arg_internal_read_integer(_part,
                                       _validation->integer_rule,
                                       &number) == D_ARG_OK)    &&
          (!number.overflow)                                   &&
          (number.magnitude <= ( (number.negative)
                                 ? ((uintmax_t)INT_MAX + 1u)
                                 : (uintmax_t)INT_MAX )) );

    // not an integer an int holds
    if (!read)
    {
        return d_arg_internal_validator_reject(_validation,
                                               "Failed parsing number (",
                                               _part,
                                               ")");
    }

    // an int, but no octet
    if ( ( (number.negative) &&
           (number.magnitude > 0u) ) ||
         (number.magnitude > 255u) )
    {
        return d_arg_internal_validator_reject(
                   _validation,
                   "Each IP number must be between 0 and 255 ",
                   _part,
                   "");
    }

    return D_ARG_OK;
}

/*
d_arg_internal_extra_ipv4_validate
  CLI11's ValidIPV4: three dots, then the four parts they separate, an empty
one among them, judged in turn until one is rejected.
*/
D_STATIC int
d_arg_internal_extra_ipv4_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_pack_text value = _validation->value;
    size_t                   dots  = 0u;
    size_t                   start = 0u;

    (void)_validator;

    // CLI11 counts the separators first
    for (size_t index = 0u; index < value.length; ++index)
    {
        dots += (value.data[index] == '.') ? 1u : 0u;
    }

    // three, and no more
    if (dots != 3u)
    {
        return d_arg_validation_reject(_validation,
                                       "Invalid IPV4 address: must have 3 "
                                       "separators");
    }

    // each part, to the next dot or the end
    for (size_t part = 0u; part < 4u; ++part)
    {
        const size_t end    = d_arg_internal_extra_ipv4_end(value,
                                                            start);
        const int    status = d_arg_internal_extra_ipv4_part(
                                  d_arg_internal_span(value.data + start,
                                                      end - start),
                                  _validation);

        // a part rejected, or a refused allocation, ends the reading
        if ( (status != D_ARG_OK) ||
             (!d_arg_validation_passed(_validation)) )
        {
            return status;
        }

        start = end + 1u;
    }

    return D_ARG_OK;
}

/*
d_arg_internal_extra_ipv4_describe
  ValidIPV4's description, as CLI11 names it.
*/
D_STATIC int
d_arg_internal_extra_ipv4_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    (void)_validator;

    return d_arg_buffer_append_cstr(_out,
                                    "IPV4");
}

/*
d_arg_validator_valid_ipv4
  The default validator with ValidIPV4's functions; it holds nothing.
*/
struct d_arg_validator
d_arg_validator_valid_ipv4(void)
{
    struct d_arg_validator validator =
        d_arg_validator_custom(d_arg_internal_extra_ipv4_validate,
                               NULL,
                               NULL);

    validator.describe = d_arg_internal_extra_ipv4_describe;

    return validator;
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
