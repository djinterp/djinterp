/*******************************************************************************
* djinterp [parse]                                                    arg_type.h
*
* Value types for the argument parser: how text becomes a value, and back.
*   A `struct d_arg_type` describes one destination type the way CLI11's
* templates describe one at compile time: its help name ("INT", "FLOAT",
* "TEXT"), how many values make one item and how many items it expects, the
* function that converts an option's values into it, and the one that turns
* a value back into text for help and configuration files. An option bound
* to a variable holds a pointer to a descriptor and to the variable; the C++
* face supplies a descriptor for each C++ type at compile time, so binding an
* `int` from C++ and from C goes through the same descriptor at the same cost.
*   The conversions are CLI11's: integers in four bases with `_` and `'` as
* digit separators, overflow refused rather than wrapped, booleans spelled
* any of CLI11's twelve ways or as a number, characters as themselves or as
* a small integer. Unlike strtod, the floating-point conversions read `.` as
* the decimal point whatever the locale, and they write the shortest text
* that reads back as the same value.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_type.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPE DESCRIPTORS
    ----------------
    1.  The descriptor
         1.  d_arg_convert_context
         2.  d_arg_convert_fn
         3.  d_arg_format_fn
         4.  Type flags
              1.  D_ARG_TYPE_FLAG_INTEGER
              2.  D_ARG_TYPE_FLAG_SIGNED
              3.  D_ARG_TYPE_FLAG_FLOAT
              4.  D_ARG_TYPE_FLAG_BOOL
              5.  D_ARG_TYPE_FLAG_CHAR
              6.  D_ARG_TYPE_FLAG_STRING
              7.  D_ARG_TYPE_FLAG_CONTAINER
              8.  D_ARG_TYPE_FLAG_ENUM
         5.  d_arg_type
    2.  Built-in types
         1.  d_arg_builtin
         2.  d_arg_type_builtin
2.  CONVERSIONS
    -----------
    1.  Integers
         1.  d_arg_parse_long / d_arg_parse_ulong
         2.  d_arg_parse_intmax / d_arg_parse_uintmax
    2.  Floating point
         1.  d_arg_parse_double / d_arg_parse_long_double
    3.  Truth values
         1.  d_arg_parse_bool
         2.  d_arg_flag_value
         3.  d_arg_flag_value_intmax
    4.  Through a descriptor
         1.  d_arg_convert / d_arg_format
    5.  Sums
         1.  d_arg_sum_values
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TYPE_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TYPE_H 1

// std
#include <stddef.h>                          // size_t
// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "../../../../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "./arg_common.h"                    // d_arg_buffer, d_arg_status
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // uint32_t, intmax_t,
                                                    // uintmax_t, INTMAX_MAX


//==============================================================================
// 1.  TYPE DESCRIPTORS
//==============================================================================


// 1.1    The descriptor
//------------------------------------------------------------------------------
// 1.1.1
// d_arg_convert_context
//   struct: what a conversion needs besides its values: the rule for leading
// zeros, the option converting (NULL outside a parse), and a buffer that
// receives the reason a conversion failed (NULL when nobody asks).
struct d_arg_convert_context
{
    int                        integer_rule;
    const struct d_arg_option* option;
    struct d_arg_buffer*       message;
};

// 1.1.2
// d_arg_convert_fn
//   type: converts an option's values -- one item's worth for a scalar, all
// of them for a container -- into the object at `_target`. Returns D_ARG_OK
// or D_ARG_ERROR_CONVERSION, writing the reason into `_context->message`
// when there is one. It must not keep a pointer to the values beyond the
// call except where the type says so (D_ARG_BUILTIN_STRING and _TEXT). The
// built-in converters read an empty value -- or none -- as the type's zero,
// as CLI11's lexical_assign value-initializes, and the text types as empty
// text; and they write `_target` only when they succeed, so a variable a
// value does not convert into keeps its value, where CLI11's conversions
// leave a partial result.
typedef int (*d_arg_convert_fn)(const struct d_arg_type*            _type,
                                const struct d_pack_text*           _values,
                                size_t                              _count,
                                void*                               _target,
                                const struct d_arg_convert_context* _context);

// 1.1.3
// d_arg_format_fn
//   type: writes the value at `_value` as text that the matching convert
// function reads back as the same value; used to capture defaults for help
// and to write configuration files. Returns D_ARG_OK, or the status of a
// sink that refused the text.
typedef int (*d_arg_format_fn)(const struct d_arg_type* _type,
                               const void*              _value,
                               struct d_pack_sink       _sink);

// 1.1.4
// Type flags
//   constant: the facts about a type that change how an option bound to it
// behaves, for d_arg_type's `flags`.

// 1.1.4.1
// D_ARG_TYPE_FLAG_INTEGER
//   constant: an integer type of any width -- not `bool`, and not `char`,
// which reads a single character as itself. A flag bound to one wider than
// a byte sums its occurrences (`-vvv` is 3), as CLI11's
// default_flag_modifiers arranges.
#define D_ARG_TYPE_FLAG_INTEGER   0x0001u

// 1.1.4.2
// D_ARG_TYPE_FLAG_SIGNED
//   constant: a type that holds negative values, so a negated flag
// (`--no-verbose`) yields -1 rather than 0.
#define D_ARG_TYPE_FLAG_SIGNED    0x0002u

// 1.1.4.3
// D_ARG_TYPE_FLAG_FLOAT
//   constant: a floating-point type.
#define D_ARG_TYPE_FLAG_FLOAT     0x0004u

// 1.1.4.4
// D_ARG_TYPE_FLAG_BOOL
//   constant: a truth value.
#define D_ARG_TYPE_FLAG_BOOL      0x0008u

// 1.1.4.5
// D_ARG_TYPE_FLAG_CHAR
//   constant: a single character, read as itself or as a small integer.
#define D_ARG_TYPE_FLAG_CHAR      0x0010u

// 1.1.4.6
// D_ARG_TYPE_FLAG_STRING
//   constant: text, which every value converts to.
#define D_ARG_TYPE_FLAG_STRING    0x0020u

// 1.1.4.7
// D_ARG_TYPE_FLAG_CONTAINER
//   constant: a collection that takes every value it is given, so an option
// bound to one expects an unlimited number of values and allows extra ones.
// Its descriptor carries its element's INTEGER, FLOAT, BOOL, CHAR or STRING
// flag beside this one, which is how shell completion knows what the
// values are.
#define D_ARG_TYPE_FLAG_CONTAINER 0x0040u

// 1.1.4.8
// D_ARG_TYPE_FLAG_ENUM
//   constant: an enumeration, named "ENUM" in help.
#define D_ARG_TYPE_FLAG_ENUM      0x0080u

// 1.1.5
// d_arg_type
//   struct: one destination type. `name` is what help prints between an
// option's names and its description. `type_size_min` and `type_size_max`
// are how many values make one item (2 for a pair; a range for a type that
// takes a variable number), and `expected` how many items an option bound
// to it takes, D_ARG_EXPECTED_UNLIMITED for a container. `format` may be
// NULL when a value has no text form; `data` is the descriptor's own,
// such as a container's element type. A descriptor is immutable and is
// never copied or freed by the parser, so it must outlive every option
// bound through it -- a static object, as every built-in one is.
struct d_arg_type
{
    const char*      name;
    d_arg_convert_fn convert;
    d_arg_format_fn  format;
    const void*      data;
    size_t           size;
    int              type_size_min;
    int              type_size_max;
    int              expected;
    uint32_t         flags;
};

// 1.2    Built-in types
//------------------------------------------------------------------------------
// 1.2.1
// d_arg_builtin
//   enum: the descriptors the module provides, one for each C type a value
// commonly lands in. STRING binds a `const char*`, which is left pointing at
// the parser's copy of the value and stays valid until the parser's next
// parse, clear or release; TEXT binds a `struct d_pack_text`, with the same
// lifetime. The fixed-width and the `long long` entries exist in every
// build; their descriptors convert in the C sources, which always have the
// types, so only a caller that cannot spell the type cannot bind one.
enum d_arg_builtin
{
    D_ARG_BUILTIN_BOOL               = 0,
    D_ARG_BUILTIN_CHAR               = 1,
    D_ARG_BUILTIN_SIGNED_CHAR        = 2,
    D_ARG_BUILTIN_UNSIGNED_CHAR      = 3,
    D_ARG_BUILTIN_SHORT              = 4,
    D_ARG_BUILTIN_UNSIGNED_SHORT     = 5,
    D_ARG_BUILTIN_INT                = 6,
    D_ARG_BUILTIN_UNSIGNED_INT       = 7,
    D_ARG_BUILTIN_LONG               = 8,
    D_ARG_BUILTIN_UNSIGNED_LONG      = 9,
    D_ARG_BUILTIN_LONG_LONG          = 10,
    D_ARG_BUILTIN_UNSIGNED_LONG_LONG = 11,
    D_ARG_BUILTIN_SIZE               = 12,
    D_ARG_BUILTIN_PTRDIFF            = 13,
    D_ARG_BUILTIN_INT8               = 14,
    D_ARG_BUILTIN_INT16              = 15,
    D_ARG_BUILTIN_INT32              = 16,
    D_ARG_BUILTIN_INT64              = 17,
    D_ARG_BUILTIN_UINT8              = 18,
    D_ARG_BUILTIN_UINT16             = 19,
    D_ARG_BUILTIN_UINT32             = 20,
    D_ARG_BUILTIN_UINT64             = 21,
    D_ARG_BUILTIN_INTMAX             = 22,
    D_ARG_BUILTIN_UINTMAX            = 23,
    D_ARG_BUILTIN_FLOAT              = 24,
    D_ARG_BUILTIN_DOUBLE             = 25,
    D_ARG_BUILTIN_LONG_DOUBLE        = 26,
    D_ARG_BUILTIN_STRING             = 27,
    D_ARG_BUILTIN_TEXT               = 28,
    D_ARG_BUILTIN_COUNT              = 29
};


D_EXTERN_C_BEGIN

// 1.2.2
// d_arg_type_builtin
/**
 * @brief Looks up a built-in type descriptor.
 *
 * @param[in] _builtin  a d_arg_builtin value.
 * @return the descriptor, which lives as long as the program; NULL for a
 *         value outside the enum.
 */
const struct d_arg_type* d_arg_type_builtin(int _builtin);


//==============================================================================
// 2.  CONVERSIONS
//==============================================================================
// The conversions the built-in descriptors use, for a caller converting text
// itself -- in an option callback, say. Each reads the whole span, accepting
// spaces around the value, and on failure leaves its output untouched.


// 2.1    Integers
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_parse_long / d_arg_parse_ulong
/**
 * @brief Converts text to a `long`.
 *
 * @note Accepts an optional sign; `0x`, `0o` and `0b` prefixes in either
 *       case; under D_ARG_INTEGER_C, a leading `0` as octal; and `_` or `'`
 *       anywhere, as digit separators. The word `true` reads as 1, as in
 *       CLI11.
 *
 * @param[in]  _text  the text.
 * @param[in]  _rule  a d_arg_integer_rule value.
 * @param[out] _out   receives the value on success.
 * @retval D_ARG_OK                  converted.
 * @retval D_ARG_ERROR_CONVERSION    not an integer, or out of range.
 * @retval D_ARG_ERROR_BAD_ARGUMENT  `_out` is NULL.
 */
int  d_arg_parse_long(struct d_pack_text _text,
                      int                _rule,
                      long*              _out);
/**
 * @brief Converts text to an `unsigned long`, refusing any negative value.
 *
 * @param[in]  _text  the text.
 * @param[in]  _rule  a d_arg_integer_rule value.
 * @param[out] _out   receives the value on success.
 * @return a status, as d_arg_parse_long's.
 */
int  d_arg_parse_ulong(struct d_pack_text _text,
                       int                _rule,
                       unsigned long*     _out);

#if defined(INTMAX_MAX)

// 2.1.2
// d_arg_parse_intmax / d_arg_parse_uintmax
/**
 * @brief Converts text to an `intmax_t`, as d_arg_parse_long.
 *
 * @note Declared where dstdint.h declares intmax_t, which is everywhere but
 *       ISO strict C++98 on a target wider than `long` can spell.
 *
 * @param[in]  _text  the text.
 * @param[in]  _rule  a d_arg_integer_rule value.
 * @param[out] _out   receives the value on success.
 * @return a status, as d_arg_parse_long's.
 */
int  d_arg_parse_intmax(struct d_pack_text _text,
                        int                _rule,
                        intmax_t*          _out);
/**
 * @brief Converts text to a `uintmax_t`, as d_arg_parse_ulong.
 *
 * @param[in]  _text  the text.
 * @param[in]  _rule  a d_arg_integer_rule value.
 * @param[out] _out   receives the value on success.
 * @return a status, as d_arg_parse_long's.
 */
int  d_arg_parse_uintmax(struct d_pack_text _text,
                         int                _rule,
                         uintmax_t*         _out);

#endif  // INTMAX_MAX

// 2.2    Floating point
//------------------------------------------------------------------------------
// 2.2.1
// d_arg_parse_double / d_arg_parse_long_double
/**
 * @brief Converts text to a `double`, reading `.` as the decimal point in
 *        every locale.
 *
 * @note Accepts what strtod accepts -- exponents, hexadecimal significands,
 *       `inf`, `nan` -- and `_` or `'` as digit separators.
 *
 * @param[in]  _text  the text.
 * @param[out] _out   receives the value on success.
 * @retval D_ARG_OK                   converted.
 * @retval D_ARG_ERROR_CONVERSION     not a number.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  a long value could not be copied for
 *                                    conversion.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   `_out` is NULL.
 */
int  d_arg_parse_double(struct d_pack_text _text,
                        double*            _out);
/**
 * @brief Converts text to a `long double`, as d_arg_parse_double.
 *
 * @param[in]  _text  the text.
 * @param[out] _out   receives the value on success.
 * @return a status, as d_arg_parse_double's.
 */
int  d_arg_parse_long_double(struct d_pack_text _text,
                             long double*       _out);

// 2.3    Truth values
//------------------------------------------------------------------------------
// 2.3.1
// d_arg_parse_bool
/**
 * @brief Converts text to a truth value, as CLI11 does.
 *
 * @note True: `true`, `on`, `yes`, `enable`, `t`, `y`, `+`, or an integer
 *       above 0. False: `false`, `off`, `no`, `disable`, `f`, `n`, `-`, an
 *       integer of 0 or below, or the empty text. Words are matched without
 *       case; an integer is read under `_rule`, and one too large for any
 *       type is true or false by its sign. As C23's strtoll reads it, which
 *       CLI11 reads it with, whitespace may come before an integer and
 *       nowhere else, a `0b` prefix is read, and neither digit separators
 *       nor a `0o` prefix are.
 *
 * @param[in]  _text  the text.
 * @param[in]  _rule  a d_arg_integer_rule value.
 * @param[out] _out   receives the value on success.
 * @retval D_ARG_OK                  converted.
 * @retval D_ARG_ERROR_CONVERSION    not a truth value.
 * @retval D_ARG_ERROR_BAD_ARGUMENT  `_out` is NULL.
 */
int  d_arg_parse_bool(struct d_pack_text _text,
                      int                _rule,
                      bool*              _out);

// 2.3.2
// d_arg_flag_value
/**
 * @brief Converts a flag's text to the number CLI11's to_flag_value gives.
 *
 * @note A true word of d_arg_parse_bool, or `+`, is 1; a false word, `-`
 *       or `0` is -1; any other single digit is its value; any other
 *       integer is itself, read under `_rule`; the empty text is 0. As
 *       C23's strtoll reads it, which CLI11 reads it with, whitespace may
 *       come before an integer and nowhere else, a `0b` prefix is read, and
 *       neither digit separators nor a `0o` prefix are. CLI11's numbers are
 *       64 bits wide; where `long` is narrower, as on 64-bit Windows,
 *       d_arg_flag_value_intmax reads them all.
 *
 * @param[in]  _text  the text.
 * @param[in]  _rule  a d_arg_integer_rule value.
 * @param[out] _out   receives the value on success.
 * @retval D_ARG_OK                  converted.
 * @retval D_ARG_ERROR_CONVERSION    not a flag value, or out of `long`'s
 *                                   range.
 * @retval D_ARG_ERROR_BAD_ARGUMENT  `_out` is NULL.
 */
int  d_arg_flag_value(struct d_pack_text _text,
                      int                _rule,
                      long*              _out);

#if defined(INTMAX_MAX)

// 2.3.3
// d_arg_flag_value_intmax
/**
 * @brief Converts a flag's text to the number CLI11's to_flag_value gives,
 *        as d_arg_flag_value, over the whole range CLI11 reads.
 *
 * @note An integer must fit a `long long`, which CLI11's strtoll reads it
 *       into. Declared where dstdint.h declares intmax_t, as
 *       d_arg_parse_intmax is.
 *
 * @param[in]  _text  the text.
 * @param[in]  _rule  a d_arg_integer_rule value.
 * @param[out] _out   receives the value on success.
 * @retval D_ARG_OK                  converted.
 * @retval D_ARG_ERROR_CONVERSION    not a flag value, or out of `long
 *                                   long`'s range.
 * @retval D_ARG_ERROR_BAD_ARGUMENT  `_out` is NULL.
 */
int  d_arg_flag_value_intmax(struct d_pack_text _text,
                             int                _rule,
                             intmax_t*          _out);

#endif  // INTMAX_MAX

// 2.4    Through a descriptor
//------------------------------------------------------------------------------
// 2.4.1
// d_arg_convert / d_arg_format
/**
 * @brief Converts values into an object through its type's descriptor.
 *
 * @param[in]  _type     the descriptor.
 * @param[in]  _values   the values; may be NULL when `_count` is 0.
 * @param[in]  _count    how many values.
 * @param[out] _target   the object to write.
 * @param[in]  _context  the conversion context; NULL for the defaults (the
 *                       configured D_INTERNAL_ARG_PARSER_INTEGER_RULE, no
 *                       option, no message).
 * @return the descriptor's status; D_ARG_ERROR_BAD_ARGUMENT for a NULL
 *         descriptor, converter or target.
 */
int  d_arg_convert(const struct d_arg_type*            _type,
                   const struct d_pack_text*           _values,
                   size_t                              _count,
                   void*                               _target,
                   const struct d_arg_convert_context* _context);
/**
 * @brief Writes an object's value as text through its type's descriptor.
 *
 * @param[in] _type   the descriptor.
 * @param[in] _value  the object to read.
 * @param[in] _sink   where the text goes.
 * @return the descriptor's status; D_ARG_ERROR_UNSUPPORTED when the type has
 *         no format function; D_ARG_ERROR_BAD_ARGUMENT for a NULL argument.
 */
int  d_arg_format(const struct d_arg_type* _type,
                  const void*              _value,
                  struct d_pack_sink       _sink);

// 2.5    Sums
//------------------------------------------------------------------------------
// 2.5.1
// d_arg_sum_values
/**
 * @brief Sums values as CLI11's sum policy does, writing the sum as text.
 *
 * @note An all-integer list sums exactly, as a 64-bit integer, unless the
 *       sum overflows; otherwise the values are summed as `double` and
 *       written to sixteen significant digits, as CLI11 writes them, flag
 *       words counting as their flag values; a value that is neither makes
 *       the result the values concatenated.
 *
 * @param[in]     _values  the values; may be NULL when `_count` is 0.
 * @param[in]     _count   how many values.
 * @param[in]     _rule    a d_arg_integer_rule value, for the integers.
 * @param[in,out] _out     receives the text, appended.
 * @return D_ARG_OK, or the buffer's status when it could not grow;
 *         D_ARG_ERROR_BAD_ARGUMENT for a NULL buffer, or NULL values with a
 *         nonzero count.
 */
int  d_arg_sum_values(const struct d_pack_text* _values,
                      size_t                    _count,
                      int                       _rule,
                      struct d_arg_buffer*      _out);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TYPE_H
