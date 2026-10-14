/*******************************************************************************
* djinterp [parse]                                         arg_validator_extra.h
*
* CLI11's extra validators: type checks, bounds, IPv4 addresses, sets and
* mappings, and numbers with units.
*   Each makes a `struct d_arg_validator` (arg_validator.h) that behaves,
* describes itself and rejects exactly as its CLI11 namesake does, but for
* the numbers it writes: a value it makes, written as the type writes
* values, has floating point as the shortest text that reads back as the
* same value and an 8-bit integer in decimal, where CLI11 writes the one to
* every digit max_digits10 allows and the other as a character. Sets and
* mappings are given as text, which the validator copies, and are compared
* as text or -- given a type descriptor -- as values of that type, the way
* CLI11 compares them as values of the container's element type.
*   For shell completion, the sets and mappings list their members or keys,
* and the bounds, the IPv4 check, the units and the type checks of
* arithmetic and truth types complete as numbers (D_ARG_VALIDATOR_NUMBER).
*   Everything here is compiled only with D_INTERNAL_ARG_PARSER_EXTRA_
* VALIDATORS, as CLI11 compiles its own unless told not to; without it the
* header declares nothing.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_validator_extra.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CHECKS
    ------
    1.  Types
         1.  d_arg_validator_type
         2.  d_arg_validator_number
    2.  Bounds
         1.  d_arg_validator_bound
         2.  d_arg_validator_bound_long and kin
    3.  Addresses
         1.  d_arg_validator_valid_ipv4
2.  SETS AND MAPPINGS
    -----------------
    1.  Filters
         1.  d_arg_filter_fn
         2.  Filter flags
              1.  D_ARG_FILTER_IGNORE_CASE
              2.  D_ARG_FILTER_IGNORE_UNDERSCORE
              3.  D_ARG_FILTER_IGNORE_SPACE
         3.  d_arg_filter
    2.  Mappings
         1.  d_arg_mapping
    3.  Validators
         1.  d_arg_validator_is_member
         2.  d_arg_validator_transformer
         3.  d_arg_validator_checked_transformer
3.  NUMBERS WITH UNITS
    ------------------
    1.  Unit options
         1.  D_ARG_UNIT_CASE_SENSITIVE
         2.  D_ARG_UNIT_CASE_INSENSITIVE
         3.  D_ARG_UNIT_OPTIONAL
         4.  D_ARG_UNIT_REQUIRED
         5.  D_ARG_UNIT_DEFAULT
    2.  Validators
         1.  d_arg_validator_as_number_with_unit
         2.  d_arg_validator_as_size_value
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_EXTRA_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_EXTRA_H 1

// std
#include <stddef.h>                          // size_t
// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "../../../../c/util/sink_common.h"  // d_pack_text
#include "./arg_common.h"                    // d_arg_buffer, d_arg_copy_fn,
                                             // d_arg_release_fn
#include "./arg_type.h"                      // d_arg_type
#include "./arg_validator.h"                 // d_arg_validator
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // uint32_t

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)


//==============================================================================
// 1.  CHECKS
//==============================================================================


D_EXTERN_C_BEGIN

// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// d_arg_validator_type
/**
 * @brief Makes CLI11's TypeValidator: a value must convert to a type.
 *
 * @note Rejects with "Failed parsing <value> as a <TYPE>", and is described
 *       as the type's name. The value is converted as CLI11's lexical_cast
 *       converts it -- as an option of the type would, but for the empty
 *       text, which no arithmetic type takes -- into a temporary object, so
 *       the type's objects must be plain data that a conversion writes
 *       without construction and leaves without cleanup, as every built-in
 *       type's are.
 *
 * @param[in] _type  the type's descriptor, which must outlive the
 *                   validator.
 * @return the validator; on failure, a failed validator holding
 *         D_ARG_ERROR_BAD_ARGUMENT for a NULL descriptor, one without a
 *         conversion, or a container's, whose objects are not plain data.
 */
struct d_arg_validator d_arg_validator_type(const struct d_arg_type* _type);

// 1.1.2
// d_arg_validator_number
//   constructor: CLI11's Number, the type validator of the built-in `double`
// described as "NUMBER" (and so rejecting with "... as a FLOAT"). Does not
// fail.
struct d_arg_validator d_arg_validator_number(void);

// 1.2    Bounds
//------------------------------------------------------------------------------
// 1.2.1
// d_arg_validator_bound
/**
 * @brief Makes CLI11's Bound, a transformation that clamps a value to two
 *        bounds.
 *
 * @note A value that converts to the type and lies outside the bounds
 *       becomes the nearer bound, written as the type writes values; one
 *       that does not convert is rejected with "Value <value> could not be
 *       converted". Described as "<TYPE> bounded to [<low> - <high>]", the
 *       bounds written as by d_arg_validator_range.
 *
 * @param[in] _type  the descriptor of an arithmetic type, as for
 *                   d_arg_validator_range.
 * @param[in] _low   the lower bound, an object of the type, copied.
 * @param[in] _high  the upper bound, an object of the type, copied.
 * @return the validator; on failure, a failed validator holding
 *         D_ARG_ERROR_BAD_ARGUMENT, as d_arg_validator_range's.
 */
struct d_arg_validator d_arg_validator_bound(const struct d_arg_type* _type,
                                             const void*              _low,
                                             const void*              _high);

// 1.2.2
// d_arg_validator_bound_long and kin
//   constructors: d_arg_validator_bound over the built-in `long`, `unsigned
// long` and `double` types. None fails.
struct d_arg_validator d_arg_validator_bound_long(long _low,
                                                  long _high);
struct d_arg_validator d_arg_validator_bound_ulong(unsigned long _low,
                                                   unsigned long _high);
struct d_arg_validator d_arg_validator_bound_double(double _low,
                                                    double _high);

// 1.3    Addresses
//------------------------------------------------------------------------------
// 1.3.1
// d_arg_validator_valid_ipv4
/**
 * @brief Makes CLI11's ValidIPV4: a value must be four integers from 0 to
 *        255 joined by dots.
 *
 * @note Rejects with CLI11's messages: "Invalid IPV4 address: must have 3
 *       separators", "Failed parsing number (<part>)", and "Each IP number
 *       must be between 0 and 255 <part>". Each part is read as an integer
 *       under the validation's rule, so `0x7f` is 127, as in CLI11; unlike
 *       CLI11, whose conversion to `int` reads it as 1, the word `true` is
 *       no number. Described as "IPV4".
 *
 * @return the validator. Does not fail.
 */
struct d_arg_validator d_arg_validator_valid_ipv4(void);

D_EXTERN_C_END


//==============================================================================
// 2.  SETS AND MAPPINGS
//==============================================================================
// CLI11's IsMember, Transformer and CheckedTransformer. Each copies the text
// it is given, so neither the array nor its strings need outlive the call.


// 2.1    Filters
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_filter_fn
//   type: writes the filtered form of `_value` into the empty buffer `_out`
// -- CLI11's filter function, applied to a value and to every member before
// they are compared. Returns D_ARG_OK, or the buffer's status.
typedef int (*d_arg_filter_fn)(struct d_pack_text   _value,
                               struct d_arg_buffer* _out,
                               void*                _context);

// 2.1.2
// Filter flags
//   constant: CLI11's three built-in filters, for d_arg_filter's `flags`.
// They commute, and run before the filter's function.

// 2.1.2.1
// D_ARG_FILTER_IGNORE_CASE
//   constant: CLI11's ignore_case: ASCII letters compare as lowercase.
#define D_ARG_FILTER_IGNORE_CASE       0x0001u

// 2.1.2.2
// D_ARG_FILTER_IGNORE_UNDERSCORE
//   constant: CLI11's ignore_underscore: `_` is removed before comparing.
#define D_ARG_FILTER_IGNORE_UNDERSCORE 0x0002u

// 2.1.2.3
// D_ARG_FILTER_IGNORE_SPACE
//   constant: CLI11's ignore_space: spaces and tabs are removed before
// comparing.
#define D_ARG_FILTER_IGNORE_SPACE      0x0004u

// 2.1.3
// d_arg_filter
//   struct: how a set or mapping compares a value with its members: the
// built-in filters in `flags`, then `function`, if any, with `context`. The
// validator keeps the context: duplicated with `copy`, when given, so that
// the validator owns its copy and frees it with `release`; borrowed, and
// `release` ignored, when `copy` is NULL.
struct d_arg_filter
{
    uint32_t         flags;
    d_arg_filter_fn  function;
    void*            context;
    d_arg_copy_fn    copy;
    d_arg_release_fn release;
};

// 2.2    Mappings
//------------------------------------------------------------------------------
// 2.2.1
// d_arg_mapping
//   struct: one pair of a mapping, CLI11's std::pair<std::string, T>: the
// text a value is matched against, and what a matching value becomes.
struct d_arg_mapping
{
    const char* from;
    const char* to;
};


D_EXTERN_C_BEGIN

// 2.3    Validators
//------------------------------------------------------------------------------
// 2.3.1
// d_arg_validator_is_member
/**
 * @brief Makes CLI11's IsMember: a value must be one of a set.
 *
 * @note Compares as text -- given no type, or a text type -- or as values
 *       of the type, so that `08` is a member of {8} for an integer type:
 *       as numbers for an arithmetic type, so that -0 is a member of {0}
 *       and a NaN of nothing, and for any other type as the text it writes
 *       its values as. The members are converted when the validator is
 *       made, under the configured integer rule, and a value under the
 *       validation's; the filter applies to the text of the value and of
 *       each member before either is converted. CLI11's search: the value
 *       is compared with the members as given, then, with a filter, with
 *       each member filtered. A member found when there is a filter
 *       replaces the value, as written. Rejects with "<value> not in
 *       {<members>}", and is described as "{<members>}". With a type, a
 *       value that does not convert stops validation with the value itself
 *       as the message, as CLI11 throws it.
 *
 * @param[in] _as      the type to compare as, which must outlive the
 *                     validator; NULL compares text. Its objects must be
 *                     plain data, as for d_arg_validator_type.
 * @param[in] _values  the members, each copied; may be NULL when `_count`
 *                     is 0.
 * @param[in] _count   how many members.
 * @param[in] _filter  the filter, copied; NULL for none.
 * @return the validator; on failure, a failed validator holding
 *         D_ARG_ERROR_OUT_OF_MEMORY, or D_ARG_ERROR_BAD_ARGUMENT for a NULL
 *         member, a NULL array with a nonzero count, a member that does not
 *         convert to the type, or a type it cannot compare as: a container,
 *         or one neither arithmetic nor writing its values.
 */
struct d_arg_validator
            d_arg_validator_is_member(const struct d_arg_type*   _as,
                                      const char* const*         _values,
                                      size_t                     _count,
                                      const struct d_arg_filter* _filter);

// 2.3.2
// d_arg_validator_transformer
/**
 * @brief Makes CLI11's Transformer: a value matching a pair's `from` becomes
 *        its `to`.
 *
 * @note Matches as d_arg_validator_is_member does; a value that matches no
 *       pair, or does not convert to the type, passes unchanged, so the
 *       transformer never rejects. Described as "{<from>-><to>,...}".
 *
 * @param[in] _as      the type to compare `from` as; NULL compares text.
 * @param[in] _pairs   the mapping, each string copied; may be NULL when
 *                     `_count` is 0.
 * @param[in] _count   how many pairs.
 * @param[in] _filter  the filter, copied; NULL for none.
 * @return the validator, or a failed validator, as
 *         d_arg_validator_is_member's.
 */
struct d_arg_validator
            d_arg_validator_transformer(const struct d_arg_type*    _as,
                                        const struct d_arg_mapping* _pairs,
                                        size_t                      _count,
                                        const struct d_arg_filter*  _filter);

// 2.3.3
// d_arg_validator_checked_transformer
/**
 * @brief Makes CLI11's CheckedTransformer: a transformer that also rejects
 *        what it cannot place.
 *
 * @note A value matching a pair's `from` becomes its `to`; one equal, as
 *       text, to some pair's `to` passes unchanged; any other is rejected
 *       with "Check <value> value in {<from>-><to>,...} OR {<to>,...}
 *       FAILED". Described as "value in {<from>-><to>,...} OR {<to>,...}".
 *
 * @param[in] _as      the type to compare `from` as; NULL compares text.
 * @param[in] _pairs   the mapping, each string copied; may be NULL when
 *                     `_count` is 0.
 * @param[in] _count   how many pairs.
 * @param[in] _filter  the filter, copied; NULL for none.
 * @return the validator, or a failed validator, as
 *         d_arg_validator_is_member's.
 */
struct d_arg_validator
     d_arg_validator_checked_transformer(const struct d_arg_type*    _as,
                                         const struct d_arg_mapping* _pairs,
                                         size_t                      _count,
                                         const struct d_arg_filter*  _filter);

D_EXTERN_C_END


//==============================================================================
// 3.  NUMBERS WITH UNITS
//==============================================================================


// 3.1    Unit options
//------------------------------------------------------------------------------
//   constant: CLI11's AsNumberWithUnit::Options, combined with `|`.

// 3.1.1
// D_ARG_UNIT_CASE_SENSITIVE
//   constant: units match exactly.
#define D_ARG_UNIT_CASE_SENSITIVE   0x0000u

// 3.1.2
// D_ARG_UNIT_CASE_INSENSITIVE
//   constant: units match without regard to ASCII case.
#define D_ARG_UNIT_CASE_INSENSITIVE 0x0001u

// 3.1.3
// D_ARG_UNIT_OPTIONAL
//   constant: a number may be given without a unit.
#define D_ARG_UNIT_OPTIONAL         0x0000u

// 3.1.4
// D_ARG_UNIT_REQUIRED
//   constant: a number must be given with a unit.
#define D_ARG_UNIT_REQUIRED         0x0002u

// 3.1.5
// D_ARG_UNIT_DEFAULT
//   constant: CLI11's DEFAULT, case-insensitive units that may be omitted.
#define D_ARG_UNIT_DEFAULT          (D_ARG_UNIT_CASE_INSENSITIVE |            \
                                     D_ARG_UNIT_OPTIONAL)


D_EXTERN_C_BEGIN

// 3.2    Validators
//------------------------------------------------------------------------------
// 3.2.1
// d_arg_validator_as_number_with_unit
/**
 * @brief Makes CLI11's AsNumberWithUnit, a transformation from a number
 *        with a unit to the number the unit stands for.
 *
 * @note A value is a number of the type followed by an optional unit -- the
 *       trailing ASCII letters, with spaces allowed between -- and becomes
 *       the number multiplied by the unit's factor in the type's own
 *       arithmetic, written as the type writes values; a unit alone stands
 *       for its factor, and a number without a unit is kept as written,
 *       trimmed. Overflow is refused where CLI11's checked_multiply refuses
 *       it. The units are kept in the order CLI11's std::map keeps them --
 *       by name, by lowercase name when they match without case -- and of
 *       units given twice the first is kept, as a std::map made from them
 *       keeps it; the factors are converted under the configured integer
 *       rule. Every refusal stops validation with CLI11's message, as CLI11
 *       throws it: "Input is empty", "Missing mandatory unit", "Value
 *       <number> could not be converted to <TYPE>", "<unit> unit not
 *       recognized. Allowed values: {<units>}", and "<number> multiplied by
 *       <unit> factor would cause number overflow. Use smaller value."
 *       Described as "<TYPE> [<name>]", or "<TYPE> <name>" when the unit is
 *       required.
 *
 * @param[in] _number     the descriptor of an arithmetic type, as for
 *                        d_arg_validator_range.
 * @param[in] _units      the units: each `from` a name, nonempty and of
 *                        letters only, and each `to` its factor as text of
 *                        the type; copied.
 * @param[in] _count      how many units.
 * @param[in] _options    D_ARG_UNIT_* flags; D_ARG_UNIT_DEFAULT for CLI11's
 *                        default.
 * @param[in] _name       the name help gives the unit; NULL for "UNIT".
 * @return the validator; on failure, a failed validator holding CLI11's
 *         construction error -- D_ARG_ERROR_VALIDATION with "Unit must not
 *         be empty.", "Unit must contain only letters." or "Several
 *         matching lowercase unit representations are found: <unit>" --
 *         or D_ARG_ERROR_BAD_ARGUMENT for a NULL argument, a type that is
 *         not arithmetic, or a factor that does not convert;
 *         D_ARG_ERROR_OUT_OF_MEMORY when the units could not be copied.
 */
struct d_arg_validator
     d_arg_validator_as_number_with_unit(const struct d_arg_type*    _number,
                                         const struct d_arg_mapping* _units,
                                         size_t                      _count,
                                         uint32_t                    _options,
                                         const char*                 _name);

// 3.2.2
// d_arg_validator_as_size_value
/**
 * @brief Makes CLI11's AsSizeValue: a size with an optional unit becomes a
 *        number of bytes.
 *
 * @note The units are `b` and, for each of k, m, g, t, p and e, the prefix
 *       alone and with `b`, `i` and `ib` after it, matched without case.
 *       With `_kb_is_1000`, `k` and `kb` mean 1000 and `ki` and `kib` 1024;
 *       otherwise all four mean 1024. The result is a 64-bit unsigned
 *       integer. Described as "SIZE [b, kb(=1000b), kib(=1024b), ...]" or
 *       "SIZE [b, kb(=1024b), ...]".
 *
 * @param[in] _kb_is_1000  whether `kb` is 1000 bytes rather than 1024.
 * @return the validator. Does not fail.
 */
struct d_arg_validator d_arg_validator_as_size_value(bool _kb_is_1000);

D_EXTERN_C_END


#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_EXTRA_H
