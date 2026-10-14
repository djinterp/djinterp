/*******************************************************************************
* djinterp [parse]                                                arg_internal.h
*
* Private helpers shared by the argument parser's C sources.
*   Locale-independent ASCII classification, span trimming and comparison,
* copies onto the heap, integer and floating-point text in both directions,
* the arithmetic values of a type descriptor, scratch objects for
* conversions, array growth, and CLI11's escapes; then the record sources'
* interface to one another -- the lists the records keep, failures recorded
* in the parser, the parts of options and commands the parse reaches into,
* the parts of a validator the validator sources share, the parse itself,
* what it asks of the system, and the text help is laid out with. Not
* installed, and not part of the public interface.
*   Sections 1 to 4 are defined in arg_internal.c, 5 in arg_escape.c, 6 in
* arg_list.c, 7 in arg_parser.c, arg_validator.c and the arg_option*.c and
* arg_command*.c sources, 8 in the arg_parse*.c, arg_process*.c and
* arg_argv*.c sources, 9 in arg_system.c and arg_suggest.c, 10 in
* arg_formatter.c, and 11 in the arg_completion*.c sources.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CHARACTERS
    ----------
    1.  Classification
         1.  d_arg_internal_is_space and kin
         2.  d_arg_internal_valid_first_char / d_arg_internal_valid_later_char
2.  TEXT
    ----
    1.  Spans
         1.  d_arg_internal_trim
         2.  d_arg_internal_equal_cstr / d_arg_internal_equal_nocase_cstr
         3.  d_arg_internal_equal / d_arg_internal_names_match
         4.  d_arg_internal_valid_name
         5.  d_arg_internal_span / d_arg_internal_text_of
         6.  d_arg_internal_print_length
         7.  d_arg_internal_slice / d_arg_internal_slice_text
    2.  Copies
         1.  d_arg_internal_copy_text / d_arg_internal_copy_cstr
    3.  Formatting
         1.  d_arg_internal_vformat
3.  NUMBERS
    -------
    1.  Integers
         1.  d_arg_internal_integer
         2.  d_arg_internal_read_integer
         3.  d_arg_internal_checked_multiply
         4.  d_arg_internal_in_range
    2.  Floating point
         1.  d_arg_internal_real
         2.  d_arg_internal_read_real
    3.  Number text
         1.  D_INTERNAL_ARG_NUMBER_TEXT_MAX
         2.  d_arg_internal_number_text
         3.  d_arg_internal_text_signed / d_arg_internal_text_unsigned
         4.  d_arg_internal_text_real
    4.  Values of a descriptor's type
         1.  d_arg_internal_number_kind
         2.  D_INTERNAL_ARG_UNORDERED
         3.  d_arg_internal_number
         4.  d_arg_internal_number_of
         5.  d_arg_internal_number_load / d_arg_internal_number_store
         6.  d_arg_internal_number_compare / d_arg_internal_number_stream
         7.  d_arg_internal_type_convertible
         8.  d_arg_internal_lexical_cast
         9.  d_arg_internal_number_write
4.  STORAGE
    -------
    1.  Scratch objects
         1.  d_arg_internal_scratch
         2.  d_arg_internal_scratch_acquire / d_arg_internal_scratch_release
    2.  Arrays
         1.  d_arg_internal_grow
5.  ESCAPES
    -------
    1.  Escaped text
         1.  d_arg_internal_unescape
         2.  d_arg_internal_escapable / d_arg_internal_escape
    2.  Binary strings
         1.  d_arg_internal_is_binary_string
         2.  d_arg_internal_extract_binary
         3.  d_arg_internal_binary_escape
6.  LISTS
    -----
    1.  Text and strings
         1.  d_arg_internal_text_push and kin
         2.  d_arg_internal_string_adopt / d_arg_internal_string_copy
         3.  d_arg_internal_string_truncate
         4.  d_arg_internal_find_member
    2.  Records
         1.  d_arg_internal_option_push and kin
         2.  d_arg_internal_command_push and kin
         3.  d_arg_internal_extra_push
         4.  d_arg_internal_validator_push / d_arg_internal_validator_insert
    3.  Releasing
         1.  d_arg_internal_option_list_release and kin
         2.  d_arg_internal_context_release
         3.  d_arg_internal_context_replace
7.  RECORDS
    -------
    1.  Failures
         1.  d_arg_internal_fail_definition / d_arg_internal_fail
         2.  d_arg_internal_fail_text / d_arg_internal_fail_adopt
         3.  d_arg_internal_watch
         4.  d_arg_internal_watch_begin and kin
         5.  d_arg_internal_unreport
    2.  Options
         1.  d_arg_internal_option_create / d_arg_internal_option_destroy
         2.  d_arg_internal_option_replace_text
         3.  d_arg_internal_option_add / d_arg_internal_option_add_built
         4.  d_arg_internal_option_validate
         5.  d_arg_internal_option_judge
         6.  d_arg_internal_option_reduce
         7.  d_arg_internal_option_results_text
         8.  d_arg_internal_option_attach
         9.  d_arg_internal_option_reduced
    3.  Commands
         1.  d_arg_internal_command_init / d_arg_internal_command_release
         2.  d_arg_internal_fallthrough_parent
         3.  d_arg_internal_compare_subcommand_names
         4.  d_arg_internal_find_subcommand
         5.  d_arg_internal_find_option
         6.  d_arg_internal_parser_of
         7.  d_arg_internal_command_options and kin
         8.  d_arg_internal_command_forget_option
         9.  d_arg_internal_command_untrigger
    4.  Validators
         1.  d_arg_internal_validator_failed
         2.  d_arg_internal_validator_bounds and kin
         3.  d_arg_internal_validator_reject / d_arg_internal_validator_stop
         4.  d_arg_internal_validator_replace
8.  THE PARSE
    ---------
    1.  Parsing
         1.  d_arg_internal_parse_run
         2.  d_arg_internal_validate / d_arg_internal_configure
         3.  d_arg_internal_parse_command
         4.  d_arg_internal_recognize
         5.  d_arg_internal_increment_parsed
         6.  d_arg_internal_trigger_pre_parse
         7.  d_arg_internal_parse_positional / d_arg_internal_parse_arg
         8.  d_arg_internal_parse_begin and kin
    2.  Processing
         1.  d_arg_internal_process / d_arg_internal_process_extras
         2.  d_arg_internal_run_callback
         3.  d_arg_internal_process_completion_callbacks
         4.  d_arg_internal_parse_config
         5.  d_arg_internal_add_flag_like_result
         6.  d_arg_internal_process_config
    3.  Arguments
         1.  d_arg_internal_pending_push / d_arg_internal_pending_push_copy
         2.  d_arg_internal_expand_responses
         3.  d_arg_internal_split_up and kin
         4.  d_arg_internal_parse_back and kin
         5.  d_arg_internal_parse_push_built
         6.  d_arg_internal_parse_at and kin
    4.  Steps
         1.  d_arg_internal_parse_stored
         2.  d_arg_internal_parse_to_missing and kin
         3.  d_arg_internal_parse_remaining
         4.  d_arg_internal_parse_judge
         5.  d_arg_internal_parse_add / d_arg_internal_parse_separate
         6.  d_arg_internal_parse_restart
         7.  d_arg_internal_parse_open_group and kin
         8.  d_arg_internal_parse_min / d_arg_internal_parse_max
9.  SYSTEM
    ------
    1.  Files
         1.  d_arg_internal_path_kind
         2.  d_arg_internal_path_info / d_arg_internal_path_query
         3.  d_arg_internal_read_file
    2.  Environment
         1.  d_arg_internal_getenv
    3.  Text encodings
         1.  d_arg_internal_wide_from_utf8
         2.  d_arg_internal_utf8_append
         3.  d_arg_internal_utf16
         4.  d_arg_internal_utf16_feed / d_arg_internal_utf16_finish
    4.  Suggestions
         1.  d_arg_internal_edit_distance
         2.  d_arg_internal_suggest
         3.  d_arg_internal_suggest_extras
    5.  The process
         1.  d_arg_internal_command_line
10. HELP
    ----
    1.  Numbers
         1.  d_arg_internal_help_mode
         2.  d_arg_internal_help_ratio
         3.  d_arg_internal_help_int
         4.  d_arg_internal_help_signed / d_arg_internal_help_unsigned
    2.  Text
         1.  d_arg_internal_help_spaces / d_arg_internal_help_pad
         2.  d_arg_internal_help_paragraph
         3.  d_arg_internal_help_column
         4.  d_arg_internal_help_label
    3.  Widths
         1.  d_arg_internal_help_fitted
    4.  Building
         1.  d_arg_internal_help_make
11. COMPLETION
    ----------
    1.  The model
         1.  d_arg_internal_complete_kind
         2.  D_INTERNAL_ARG_COMPLETE_UNLIMITED
         3.  d_arg_internal_complete_command
         4.  d_arg_internal_complete_name
         5.  d_arg_internal_complete_option
         6.  d_arg_internal_complete_entry
         7.  d_arg_internal_model
         8.  d_arg_internal_complete_build / d_arg_internal_complete_release
    2.  Writing
         1.  d_arg_internal_complete_ident
         2.  d_arg_internal_complete_lines
         3.  d_arg_internal_complete_line_text
         4.  d_arg_internal_complete_bash and kin
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_INTERNAL_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_INTERNAL_H 1

// std
#include <stdarg.h>  // va_list
#include <stddef.h>  // size_t
#include <string.h>  // strlen
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // d_arg_buffer, lists
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_config.h"  // d_arg_config_item
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_type
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator
#include "../../../../../../inc/djinterp/env/c/env_vendor_attributes.h"  // D_FORMAT_PRINTF
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // intmax_t,
                                                           // uintmax_t


// Linkage: declared inside the C linkage block so that a C++ test unit
// links against the C definitions.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CHARACTERS
//==============================================================================
// ASCII only, whatever the locale: the parser's syntax is ASCII, and
// <ctype.h> answers by the locale, so `isspace` may accept a byte of a UTF-8
// sequence.


// 1.1    Classification
//------------------------------------------------------------------------------
// 1.1.1
// d_arg_internal_is_space and kin
//   predicates and maps: space is ' ', \t, \n, \v, \f and \r; a letter is A-Z
// or a-z; to_lower folds A-Z only; digit_value is a hexadecimal digit's value,
// or -1 for anything else.
D_INLINE bool
d_arg_internal_is_space(
    char _c
)
{
    return ( (_c == ' ')  ||
             (_c == '\t') ||
             (_c == '\n') ||
             (_c == '\v') ||
             (_c == '\f') ||
             (_c == '\r') );
}

D_INLINE bool
d_arg_internal_is_digit(
    char _c
)
{
    return ( (_c >= '0') &&
             (_c <= '9') );
}

D_INLINE bool
d_arg_internal_is_alpha(
    char _c
)
{
    return ( ( (_c >= 'a') &&
               (_c <= 'z') ) ||
             ( (_c >= 'A') &&
               (_c <= 'Z') ) );
}

D_INLINE char
d_arg_internal_to_lower(
    char _c
)
{
    return ( (_c >= 'A') &&
             (_c <= 'Z') )
           ? (char)(_c - 'A' + 'a')
           : _c;
}

D_INLINE int
d_arg_internal_digit_value(
    char _c
)
{
    const char lower = d_arg_internal_to_lower(_c);

    // decimal digits first
    if (d_arg_internal_is_digit(lower))
    {
        return lower - '0';
    }

    // then a through f
    if ( (lower >= 'a') &&
         (lower <= 'f') )
    {
        return (lower - 'a') + 10;
    }

    return -1;
}

// 1.1.2
// d_arg_internal_valid_first_char / d_arg_internal_valid_later_char
//   predicates: CLI11's valid_first_char -- not `-`, and above `!`, so no
// space or control character -- and valid_later_char -- not `=`, `:` or
// `{`, and above the space, or a tab. Bytes past ASCII pass both, so names
// may be UTF-8.
D_INLINE bool
d_arg_internal_valid_first_char(
    char _c
)
{
    return ( (_c != '-') &&
             ((unsigned char)_c > 33u) );
}

D_INLINE bool
d_arg_internal_valid_later_char(
    char _c
)
{
    return ( (_c != '=')                    &&
             (_c != ':')                    &&
             (_c != '{')                    &&
             ( ((unsigned char)_c > 32u) ||
               (_c == '\t') ) );
}


//==============================================================================
// 2.  TEXT
//==============================================================================


// 2.1    Spans
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_internal_trim
//   view: the span without the ASCII space at either end; never allocates.
struct d_pack_text d_arg_internal_trim(struct d_pack_text _text);

// 2.1.2
// d_arg_internal_equal_cstr / d_arg_internal_equal_nocase_cstr
//   predicates: whether a span holds exactly the NUL-terminated `_word`,
// byte for byte, or with ASCII case folded. A NULL word equals nothing.
bool               d_arg_internal_equal_cstr(struct d_pack_text _text,
                                             const char*        _word);
bool               d_arg_internal_equal_nocase_cstr(struct d_pack_text _text,
                                                    const char*        _word);

// 2.1.3
// d_arg_internal_equal / d_arg_internal_names_match
//   predicates: equal compares two spans byte for byte; names_match compares
// two names as CLI11's find_member does, with `_` removed and ASCII case
// folded on both sides when asked.
bool               d_arg_internal_equal(struct d_pack_text _a,
                                        struct d_pack_text _b);
bool               d_arg_internal_names_match(struct d_pack_text _a,
                                              struct d_pack_text _b,
                                              bool               _case,
                                              bool               _underscore);

// 2.1.4
// d_arg_internal_valid_name
//   predicate: CLI11's valid_name_string: not empty, its first character
// valid first and the rest valid later.
bool               d_arg_internal_valid_name(struct d_pack_text _name);

// 2.1.5
// d_arg_internal_span / d_arg_internal_text_of
//   views: span makes a span of `_length` bytes at `_data`; text_of makes one
// of a NUL-terminated string, and of NULL the empty span, whose `data` is a
// valid empty string. Neither allocates.
D_INLINE struct d_pack_text
d_arg_internal_span(
    const char* _data,
    size_t      _length
)
{
    const struct d_pack_text text = { _data, _length };

    return text;
}

D_INLINE struct d_pack_text
d_arg_internal_text_of(
    const char* _text
)
{
    return (_text != NULL)
           ? d_arg_internal_span(_text,
                                 strlen(_text))
           : d_arg_internal_span("",
                                 0u);
}

// 2.1.6
// d_arg_internal_print_length
//   conversion: a span's length as `%.*s` takes it, an int, held at INT_MAX
// for a span no message could show whole anyway.
int                d_arg_internal_print_length(struct d_pack_text _text);

// 2.1.7
// d_arg_internal_slice / d_arg_internal_slice_text
//   struct and view: a slice is a run of a text a record owns, kept as an
// offset and a length rather than a pointer, so that a record copied with
// its text needs no fixing; slice_text makes the span a slice covers in
// `_text`, without allocating.
struct d_arg_internal_slice
{
    size_t offset;
    size_t length;
};

D_INLINE struct d_pack_text
d_arg_internal_slice_text(
    const char*                 _text,
    struct d_arg_internal_slice _slice
)
{
    return d_arg_internal_span(_text + _slice.offset,
                               _slice.length);
}

// 2.2    Copies
//------------------------------------------------------------------------------
// 2.2.1
// d_arg_internal_copy_text / d_arg_internal_copy_cstr
/**
 * @brief Copies a span onto the heap, NUL-terminated.
 *
 * @param[in] _text  the span; it may hold NUL bytes, which are copied.
 * @return the copy, which the caller frees with free(); NULL when the
 *         allocation was refused.
 */
char*              d_arg_internal_copy_text(struct d_pack_text _text);
/**
 * @brief Copies a NUL-terminated string onto the heap.
 *
 * @param[in] _text  the string; NULL copies the empty string.
 * @return the copy, which the caller frees with free(); NULL when the
 *         allocation was refused.
 */
char*              d_arg_internal_copy_cstr(const char* _text);

// 2.3    Formatting
//------------------------------------------------------------------------------
// 2.3.1
// d_arg_internal_vformat
/**
 * @brief Appends the expansion of a `printf`-style format, from an argument
 *        list, to a buffer; what d_arg_buffer_printf and every recorded
 *        message are written with. Defined in arg_common.c, beside the
 *        buffer it grows.
 *
 * @note The text is formatted twice, to measure and to write, and the
 *       buffer grows once.
 *
 * @param[in,out] _buffer     the buffer.
 * @param[in]     _format     the format.
 * @param[in]     _arguments  the arguments, which are consumed.
 * @return D_ARG_OK; D_ARG_ERROR_OUT_OF_MEMORY when the buffer could not
 *         grow, or had failed, leaving its content as it was;
 *         D_ARG_ERROR_BAD_ARGUMENT when the format cannot be expanded.
 */
int                d_arg_internal_vformat(struct d_arg_buffer* _buffer,
                                          const char*          _format,
                                          va_list              _arguments)
    D_FORMAT_PRINTF(2, 0);


//==============================================================================
// 3.  NUMBERS
//==============================================================================


// 3.1    Integers
//------------------------------------------------------------------------------
// 3.1.1
// d_arg_internal_integer
//   struct: an integer as read, before it is fitted to a type: its magnitude
// and its sign, so that every type's range can be checked exactly, and
// whether its digits were too many for uintmax_t, which a truth value still
// reads by its sign.
struct d_arg_internal_integer
{
    uintmax_t magnitude;
    bool      negative;
    bool      overflow;
};

// D_INTERNAL_ARG_INTEGER_INIT
//   constant: the initializer of a zero integer.
#define D_INTERNAL_ARG_INTEGER_INIT { 0u, false, false }

// 3.1.2
// d_arg_internal_read_integer
/**
 * @brief Reads an integer as CLI11's integral_conversion does, under a rule.
 *
 * @note Spaces around the integer, one sign, a `0x`, `0o` or `0b` prefix in
 *       either case, and `_` or `'` anywhere are accepted; under
 *       D_ARG_INTEGER_C a leading 0 followed by more digits reads as octal.
 *       The word `true` is not an integer here: the signed conversions,
 *       which take it, check for it themselves.
 *
 * @param[in]  _text  the text.
 * @param[in]  _rule  a d_arg_integer_rule value.
 * @param[out] _out   receives the integer on success; `overflow` is set,
 *                    and `magnitude` is UINTMAX_MAX, when the digits name a
 *                    larger number.
 * @retval D_ARG_OK                converted, perhaps with overflow.
 * @retval D_ARG_ERROR_CONVERSION  not an integer.
 */
int         d_arg_internal_read_integer(struct d_pack_text             _text,
                                        int                            _rule,
                                        struct d_arg_internal_integer* _out);

// 3.1.3
// d_arg_internal_checked_multiply
//   arithmetic: CLI11's checked_multiply for `int`: `*_a` becomes the
// product and the result is true, unless the product would overflow, when
// `*_a` is unchanged and the result false.
bool               d_arg_internal_checked_multiply(int* _a,
                                                   int  _b);

// 3.1.4
// d_arg_internal_in_range
//   predicate: whether `_value` lies from `_first` to `_last`, both included:
// an enum's value checked against its first and last, so that a setter can
// ignore one outside it.
D_INLINE bool
d_arg_internal_in_range(
    int _value,
    int _first,
    int _last
)
{
    return ( (_value >= _first) &&
             (_value <= _last) );
}

// 3.2    Floating point
//------------------------------------------------------------------------------
// 3.2.1
// d_arg_internal_real
//   enum: the three floating-point types, by which the functions below know
// what a `void*` points at.
enum d_arg_internal_real
{
    D_ARG_INTERNAL_REAL_FLOAT       = 0,
    D_ARG_INTERNAL_REAL_DOUBLE      = 1,
    D_ARG_INTERNAL_REAL_LONG_DOUBLE = 2
};

// 3.2.2
// d_arg_internal_read_real
/**
 * @brief Reads a floating-point number with `.` as the decimal point in
 *        every locale, rounded once, as the type's own strto* rounds it.
 *
 * @note Spaces around the number and `_` or `'` anywhere are accepted, as
 *       CLI11 accepts them; the current locale's decimal point, when it is
 *       not `.`, is refused.
 *
 * @param[in]  _text  the text.
 * @param[in]  _real  a d_arg_internal_real value.
 * @param[out] _out   receives the number on success.
 * @retval D_ARG_OK                   converted.
 * @retval D_ARG_ERROR_CONVERSION     not a number.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  a long text could not be copied.
 */
int                d_arg_internal_read_real(struct d_pack_text _text,
                                            int                _real,
                                            void*              _out);

// 3.3    Number text
//------------------------------------------------------------------------------
// 3.3.1
// D_INTERNAL_ARG_NUMBER_TEXT_MAX
//   constant: room for any number the functions below write -- a 128-bit
// integer in decimal, or a quadruple-precision value to all its digits --
// with a terminator.
#define D_INTERNAL_ARG_NUMBER_TEXT_MAX 128u

// 3.3.2
// d_arg_internal_number_text
//   struct: a number written as text on the stack, so that formatting into a
// sink never allocates. `data` is NUL-terminated.
struct d_arg_internal_number_text
{
    char   data[D_INTERNAL_ARG_NUMBER_TEXT_MAX];
    size_t length;
};

// D_INTERNAL_ARG_NUMBER_TEXT_INIT
//   constant: the initializer of an empty number text.
#define D_INTERNAL_ARG_NUMBER_TEXT_INIT { { '\0' }, 0u }

// 3.3.3
// d_arg_internal_text_signed / d_arg_internal_text_unsigned
//   writing: an integer in decimal. Neither fails.
void    d_arg_internal_text_signed(struct d_arg_internal_number_text* _out,
                                   intmax_t                           _value);
void    d_arg_internal_text_unsigned(struct d_arg_internal_number_text* _out,
                                     uintmax_t                          _value);

// 3.3.4
// d_arg_internal_text_real
/**
 * @brief Writes a floating-point number, with `.` as the decimal point.
 *
 * @note With `_precision` 0, the shortest text that reads back as the same
 *       value; otherwise `%.<precision>g`, as a stream with that precision
 *       writes it. Infinities are `inf` and `-inf`, NaNs `nan`, and the
 *       exponent has the two digits at least that C99 gives it, whatever
 *       the runtime.
 *
 * @param[out] _out        receives the text.
 * @param[in]  _real       a d_arg_internal_real value.
 * @param[in]  _value      the number.
 * @param[in]  _precision  significant digits, or 0 for the shortest.
 * @return `true`, or `false` when the runtime could not format the number,
 *         with `_out` empty.
 */
bool    d_arg_internal_text_real(struct d_arg_internal_number_text* _out,
                                 int                                _real,
                                 const void*                        _value,
                                 int                                _precision);

// 3.4    Values of a descriptor's type
//------------------------------------------------------------------------------
// 3.4.1
// d_arg_internal_number_kind
//   enum: how a descriptor's objects compare: as signed or unsigned integers,
// as floating point, or not at all.
enum d_arg_internal_number_kind
{
    D_ARG_INTERNAL_NUMBER_NONE     = 0,
    D_ARG_INTERNAL_NUMBER_SIGNED   = 1,
    D_ARG_INTERNAL_NUMBER_UNSIGNED = 2,
    D_ARG_INTERNAL_NUMBER_REAL     = 3
};

// 3.4.2
// D_INTERNAL_ARG_UNORDERED
//   constant: what d_arg_internal_number_compare returns for a NaN, which is
// neither less than, equal to, nor greater than anything.
#define D_INTERNAL_ARG_UNORDERED 2

// 3.4.3
// d_arg_internal_number
//   struct: an object of an arithmetic descriptor's type, widened without
// loss: to intmax_t, uintmax_t or long double, as `kind` says.
struct d_arg_internal_number
{
    int         kind;
    intmax_t    s;
    uintmax_t   u;
    long double r;
};

// D_INTERNAL_ARG_NUMBER_INIT
//   constant: the initializer of a number of no kind.
#define D_INTERNAL_ARG_NUMBER_INIT { 0, 0, 0u, 0.0L }

// 3.4.4
// d_arg_internal_number_of
//   classification: the kind of a descriptor's objects, from its flags and
// size; NONE for a type that is not an integer, a character or floating
// point, or whose size matches no C type of its kind.
int                d_arg_internal_number_of(const struct d_arg_type* _type);

// 3.4.5
// d_arg_internal_number_load / d_arg_internal_number_store
//   conversion: load widens an object of the descriptor's type; store
// narrows a number of the same kind into one, and returns false, writing
// nothing, when the number is outside the type's range. Both return false
// for a type that is not arithmetic.
bool d_arg_internal_number_load(const struct d_arg_type*      _type,
                                const void*                   _object,
                                struct d_arg_internal_number* _out);
bool d_arg_internal_number_store(const struct d_arg_type*            _type,
                                 const struct d_arg_internal_number* _number,
                                 void*                               _object);

// 3.4.6
// d_arg_internal_number_compare / d_arg_internal_number_stream
//   compare orders two numbers of one kind: -1, 0 or 1, or
// D_INTERNAL_ARG_UNORDERED when either is a NaN. stream writes one as CLI11
// streams a value of the type -- integers in full, a `char` as the
// character, floating point to six significant digits.
int  d_arg_internal_number_compare(const struct d_arg_internal_number* _a,
                                   const struct d_arg_internal_number* _b);
void d_arg_internal_number_stream(struct d_arg_internal_number_text*  _out,
                                  const struct d_arg_type*            _type,
                                  const struct d_arg_internal_number* _number);

// 3.4.7
// d_arg_internal_type_convertible
//   predicate: whether a validator can convert a value into a scratch object
// of a descriptor's type and drop it: a type that converts, but not a
// container, which keeps what it is given on the heap.
D_INLINE bool
d_arg_internal_type_convertible(
    const struct d_arg_type* _type
)
{
    return ( (_type != NULL)                                    &&
             (_type->convert != NULL)                           &&
             ((_type->flags & D_ARG_TYPE_FLAG_CONTAINER) == 0u) );
}

// 3.4.8
// d_arg_internal_lexical_cast
/**
 * @brief Converts a text to an object of a descriptor's type as CLI11's
 *        lexical_cast does, for the validators that convert.
 *
 * @note The text is converted as an option of the type converts it, but the
 *       empty text, which an option reads as the type's zero, is refused for
 *       an arithmetic type, as lexical_cast refuses it.
 *
 * @param[in]  _type     the descriptor, which converts.
 * @param[in]  _text     the text.
 * @param[in]  _context  the conversion's context.
 * @param[out] _object   receives the object; it holds the type's size.
 * @return D_ARG_OK when the text converted; D_ARG_ERROR_CONVERSION when it
 *         is not a value of the type; otherwise the status the conversion
 *         stopped with, such as D_ARG_ERROR_OUT_OF_MEMORY.
 */
int  d_arg_internal_lexical_cast(const struct d_arg_type*            _type,
                                 struct d_pack_text                  _text,
                                 const struct d_arg_convert_context* _context,
                                 void*                               _object);

// 3.4.9
// d_arg_internal_number_write
/**
 * @brief Writes an object of an arithmetic descriptor's type as the type
 *        writes its values.
 *
 * @note Through the type's format function; a type without one is written
 *       as the built-in types write theirs: a character as itself, an
 *       integer in decimal, and floating point as the shortest text that
 *       reads back as the same value.
 *
 * @param[in]     _type    the descriptor.
 * @param[in]     _object  the object.
 * @param[in,out] _out     receives the text, appended.
 * @return D_ARG_OK; D_ARG_ERROR_OUT_OF_MEMORY when the buffer could not
 *         grow; otherwise the status the format function stopped with.
 */
int  d_arg_internal_number_write(const struct d_arg_type* _type,
                                 const void*              _object,
                                 struct d_arg_buffer*     _out);


//==============================================================================
// 4.  STORAGE
//==============================================================================


// 4.1    Scratch objects
//------------------------------------------------------------------------------
// 4.1.1
// d_arg_internal_scratch
//   union: room on the stack for a temporary object of any built-in type --
// what a validator converts a value into to judge it -- aligned for every
// one of them.
union d_arg_internal_scratch
{
    long double   r;
    intmax_t      s;
    uintmax_t     u;
    double        d;
    void*         p;
    unsigned char bytes[64];
};

// 4.1.2
// d_arg_internal_scratch_acquire / d_arg_internal_scratch_release
/**
 * @brief Provides zeroed storage for a temporary object of a size.
 *
 * @param[in,out] _scratch  the stack storage, used when it is large enough.
 * @param[in]     _size     the object's size in bytes.
 * @return the storage -- `_scratch`'s, or a heap block -- zeroed; NULL when
 *         a heap block was needed and refused. Release it with
 *         d_arg_internal_scratch_release.
 */
void*   d_arg_internal_scratch_acquire(union d_arg_internal_scratch* _scratch,
                                       size_t                        _size);
void    d_arg_internal_scratch_release(union d_arg_internal_scratch* _scratch,
                                       void*                         _storage);

// 4.2    Arrays
//------------------------------------------------------------------------------
// 4.2.1
// d_arg_internal_grow
/**
 * @brief Grows a heap array so that it holds at least a number of elements.
 *
 * @note Takes and returns the array as `void*`, never through a `void**`:
 *       writing a `struct x*` through a `void**` is undefined, which is why
 *       the module does not use c/container's vector_common.
 *
 * @param[in]  _items         the array; NULL when it has no storage yet.
 * @param[in]  _capacity      its capacity in elements.
 * @param[in]  _needed        the elements it must hold, more than
 *                            `_capacity`.
 * @param[in]  _size          the size of one element.
 * @param[out] _new_capacity  receives the grown capacity on success.
 * @return the grown array, which replaces `_items`; NULL when growth was
 *         refused or would overflow, `_items` then being unchanged.
 */
void*              d_arg_internal_grow(void*   _items,
                                       size_t  _capacity,
                                       size_t  _needed,
                                       size_t  _size,
                                       size_t* _new_capacity);


//==============================================================================
// 5.  ESCAPES
//==============================================================================
// CLI11's string escapes, which its EscapedString validator and its
// configuration files share; defined in arg_escape.c.


// 5.1    Escaped text
//------------------------------------------------------------------------------
// 5.1.1
// d_arg_internal_unescape
/**
 * @brief Resolves backslash escapes as CLI11's remove_escaped_characters.
 *
 * @note The escapes are `\\b`, `\\t`, `\\n`, `\\f`, `\\r`, `\\"`, `\\\\`,
 *       `\\0`, `\\uXXXX` and `\\UXXXXXXXX`, the last two written as UTF-8. A
 *       malformed escape is described in CLI11's words, which name the whole
 *       text: "invalid escape sequence <text>", "unicode sequence must have
 *       4 hex codes <text>" (or 8), "unrecognized escape sequence \\<c> in
 *       <text>", "[0xD800, 0xDFFF] are not valid code points.", and "values
 *       above 0x10FFFF are not valid code points."
 *
 * @param[in]     _text   the escaped text.
 * @param[in,out] _out    receives the resolved text, appended.
 * @param[in,out] _error  receives the description of a malformed escape,
 *                        appended.
 * @retval D_ARG_OK                   resolved.
 * @retval D_ARG_ERROR_VALIDATION     a malformed escape, described.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  a buffer could not grow.
 */
int                d_arg_internal_unescape(struct d_pack_text   _text,
                                           struct d_arg_buffer* _out,
                                           struct d_arg_buffer* _error);

// 5.1.2
// d_arg_internal_escapable / d_arg_internal_escape
//   escaping: escapable is CLI11's has_escapable_character, whether the text
// holds a backspace, tab, line feed, form feed, carriage return, `"` or
// backslash; escape is its add_escaped_characters, which appends the text
// with each of those as a backslash and its letter -- `\b`, `\t`, `\n`,
// `\f`, `\r`, `\"`, `\\` -- returning the buffer's status.
bool               d_arg_internal_escapable(struct d_pack_text _text);
int                d_arg_internal_escape(struct d_pack_text   _text,
                                         struct d_arg_buffer* _out);

// 5.2    Binary strings
//------------------------------------------------------------------------------
// 5.2.1
// d_arg_internal_is_binary_string
//   predicate: whether the text is a CLI11 binary string, `B"(...)"` or
// `'B"(...)"'`.
bool               d_arg_internal_is_binary_string(struct d_pack_text _text);

// 5.2.2
// d_arg_internal_extract_binary
/**
 * @brief Decodes a binary string as CLI11's extract_binary_string: each
 *        `\\xHH` between the delimiters is one byte, and every other
 *        character stands for itself.
 *
 * @param[in]     _text  the text; anything but a binary string is appended
 *                       unchanged.
 * @param[in,out] _out   receives the bytes, appended.
 * @return D_ARG_OK, or the buffer's status.
 */
int                d_arg_internal_extract_binary(struct d_pack_text   _text,
                                                 struct d_arg_buffer* _out);

// 5.2.3
// d_arg_internal_binary_escape
/**
 * @brief Writes text as CLI11's binary_escape_string writes it.
 *
 * @note Each byte that does not print is `\\xHH`, in lowercase; an `x`
 *       or `X` right after a backslash is `\\x78` or `\\x58`, so that no
 *       escape appears that the text did not hold. Text that changed, or any
 *       text when forced, then has each `'` as `\\x27` and is wrapped as
 *       `'B"(...)"'`; text that did not is appended unchanged.
 *
 * @param[in]     _text   the text.
 * @param[in]     _force  whether unchanged text is wrapped too.
 * @param[in,out] _out    receives the written text, appended.
 * @return D_ARG_OK, or the buffer's status.
 */
int                d_arg_internal_binary_escape(struct d_pack_text   _text,
                                                bool                 _force,
                                                struct d_arg_buffer* _out);



//==============================================================================
// 6.  LISTS
//==============================================================================
// The growable arrays of arg_common.h, grown through d_arg_internal_grow.
// Each push either appends or, refused, leaves the list as it was and
// returns D_ARG_ERROR_OUT_OF_MEMORY. Defined in arg_list.c.


// 6.1    Text and strings
//------------------------------------------------------------------------------
// 6.1.1
// d_arg_internal_text_push and kin
//   growth: push appends a span, which stays borrowed; reserve makes room for
// `_count` spans in all, so that as many pushes cannot fail; append appends
// the `_count` spans of `_source` from `_first` on, all of them or, refused,
// none.
int    d_arg_internal_text_push(struct d_arg_text_list* _list,
                                struct d_pack_text      _text);
int    d_arg_internal_text_reserve(struct d_arg_text_list* _list,
                                   size_t                  _count);
int    d_arg_internal_text_append(struct d_arg_text_list*       _list,
                                  const struct d_arg_text_list* _source,
                                  size_t                        _first,
                                  size_t                        _count);

// 6.1.2
// d_arg_internal_string_adopt / d_arg_internal_string_copy
/**
 * @brief Appends a heap string to a string list, which then owns it.
 *
 * @param[in,out] _list    the list.
 * @param[in]     _string  the string, from malloc; NULL is refused.
 * @post   on failure `_string` has been freed, so the caller never frees it.
 * @retval D_ARG_OK                   appended.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  the list could not grow, or `_string`
 *                                    is NULL, as a refused copy is.
 */
int         d_arg_internal_string_adopt(struct d_arg_string_list* _list,
                                        char*                     _string);
/**
 * @brief Appends a copy of a span to a string list.
 *
 * @param[in,out] _list  the list.
 * @param[in]     _text  the text, copied with a terminator.
 * @param[out]    _copy  receives the copy, which the list owns; may be NULL.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY with the list unchanged.
 */
int         d_arg_internal_string_copy(struct d_arg_string_list* _list,
                                       struct d_pack_text        _text,
                                       const char**              _copy);

// 6.1.3
// d_arg_internal_string_truncate
//   shrinking: frees the strings past the first `_count`; a longer count
// changes nothing.
void        d_arg_internal_string_truncate(struct d_arg_string_list* _list,
                                           size_t                    _count);

// 6.1.4
// d_arg_internal_find_member
//   search: CLI11's find_member: the index of the first string in a list
// that matches a name as d_arg_internal_names_match matches, or -1.
long    d_arg_internal_find_member(struct d_pack_text              _name,
                                   const struct d_arg_string_list* _list,
                                   bool                            _case,
                                   bool                            _underscore);

// 6.2    Records
//------------------------------------------------------------------------------
// 6.2.1
// d_arg_internal_option_push and kin
//   lists of options, held as pointers: push appends; insert appends unless
// the option is there already, a set's insert; has finds; remove takes out
// the first occurrence, keeping the order of the rest, and says whether
// there was one.
int         d_arg_internal_option_push(struct d_arg_option_list* _list,
                                       struct d_arg_option*      _option);
int         d_arg_internal_option_insert(struct d_arg_option_list* _list,
                                         struct d_arg_option*      _option);
bool        d_arg_internal_option_has(const struct d_arg_option_list* _list,
                                      const struct d_arg_option*      _option);
bool        d_arg_internal_option_remove(struct d_arg_option_list*  _list,
                                         const struct d_arg_option* _option);

// 6.2.2
// d_arg_internal_command_push and kin
//   the same for lists of commands.
int     d_arg_internal_command_push(struct d_arg_command_list* _list,
                                    struct d_arg_command*      _command);
int     d_arg_internal_command_insert(struct d_arg_command_list* _list,
                                      struct d_arg_command*      _command);
bool    d_arg_internal_command_has(const struct d_arg_command_list* _list,
                                   const struct d_arg_command*      _command);
bool    d_arg_internal_command_remove(struct d_arg_command_list*  _list,
                                      const struct d_arg_command* _command);

// 6.2.3
// d_arg_internal_extra_push
//   growth: appends an unused argument with its d_arg_classifier value.
int                d_arg_internal_extra_push(struct d_arg_extra_list* _list,
                                             struct d_pack_text       _text,
                                             int                      _class);

// 6.2.4
// d_arg_internal_validator_push / d_arg_internal_validator_insert
//   growth: push appends a validator and insert puts it first; either takes
// the validator over, releasing it when the list cannot grow.
int  d_arg_internal_validator_push(struct d_arg_validator_list* _list,
                                   struct d_arg_validator       _validator);
int  d_arg_internal_validator_insert(struct d_arg_validator_list* _list,
                                     struct d_arg_validator       _validator);

// 6.3    Releasing
//------------------------------------------------------------------------------
// 6.3.1
// d_arg_internal_option_list_release and kin
//   lifetime: free a list's array, and, for validators, every validator in
// it, leaving the list empty; the options and commands a list points at are
// not touched. Each ignores NULL.
void d_arg_internal_option_list_release(struct d_arg_option_list* _list);
void d_arg_internal_command_list_release(struct d_arg_command_list* _list);
void d_arg_internal_extra_list_release(struct d_arg_extra_list* _list);
void d_arg_internal_validator_list_release(struct d_arg_validator_list* _list);

// 6.3.2
// d_arg_internal_context_release
//   lifetime: frees a callback's context through the release function kept
// beside it, when there is one, and forgets both.
void d_arg_internal_context_release(void**            _context,
                                    d_arg_release_fn* _release);

// 6.3.3
// d_arg_internal_context_replace
//   lifetime: puts a new context and its release function in place of the
// ones kept, freeing the old context first unless it is the new one, so
// that a context given again is not freed.
void d_arg_internal_context_replace(void**            _context,
                                    d_arg_release_fn* _release,
                                    void*             _new_context,
                                    d_arg_release_fn  _new_release);


//==============================================================================
// 7.  RECORDS
//==============================================================================
// What one record source needs of another: recording failures in the parser
// (arg_parser.c), making options (arg_option.c), adding, judging and
// reducing their values (arg_option_value.c), attaching their variables
// (arg_option_link.c), making, searching and cleaning up after commands
// (the arg_command*.c sources), and the parts of a validator the validator
// sources share (arg_validator.c).


// 7.1    Failures
//------------------------------------------------------------------------------
// 7.1.1
// d_arg_internal_fail_definition / d_arg_internal_fail
/**
 * @brief Records a failed definition in the parser, unless one is recorded
 *        already, and returns its status.
 *
 * @note fail is the same for a failure of the parse, which replaces the
 *       parser's error report whatever it held. Either finds the parser
 *       through the command, or through the option's command when the
 *       command is NULL, and only returns `_status` when there is none.
 *
 * @param[in] _command  the command concerned; may be NULL.
 * @param[in] _option   the option concerned; may be NULL.
 * @param[in] _status   the d_arg_status to record.
 * @param[in] _format   the message's format string; NULL for the status's
 *                      own text.
 * @return `_status`.
 */
int         d_arg_internal_fail_definition(const struct d_arg_command* _command,
                                           const struct d_arg_option*  _option,
                                           int                         _status,
                                           const char*                 _format,
                                           ...) D_FORMAT_PRINTF(4, 5);
int         d_arg_internal_fail(const struct d_arg_command* _command,
                                const struct d_arg_option*  _option,
                                int                         _status,
                                const char*                 _format,
                                ...) D_FORMAT_PRINTF(4, 5);

// 7.1.2
// d_arg_internal_fail_text / d_arg_internal_fail_adopt
//   recording: d_arg_internal_fail with a message given as a span -- one a
// validator or a converter composed -- which is copied, rather than a
// format; adopt's message is on the heap already, and the report takes it
// over, freeing it when there is no parser to report to. A completed
// request whose message is what it asked for -- D_ARG_CALL_FOR_VERSION,
// D_ARG_CALL_FOR_COMPLETION -- whose copy was refused, or which comes
// without one, is recorded as D_ARG_ERROR_OUT_OF_MEMORY instead; each
// returns the status recorded.
int         d_arg_internal_fail_text(const struct d_arg_command* _command,
                                     const struct d_arg_option*  _option,
                                     int                         _status,
                                     struct d_pack_text          _message);
int         d_arg_internal_fail_adopt(const struct d_arg_command* _command,
                                      const struct d_arg_option*  _option,
                                      int                         _status,
                                      char*                       _message);

// 7.1.3
// d_arg_internal_watch
//   struct: a parser's error report, set aside while a caller's function runs
// -- a callback, an each() function -- so that whether the function recorded
// a failure can be told afterwards. `parser` is NULL when there is none to
// watch.
struct d_arg_internal_watch
{
    struct d_arg_parser* parser;
    struct d_arg_error   saved;
};

// 7.1.4
// d_arg_internal_watch_begin and kin
/**
 * @brief Sets a parser's error report aside, leaving it a clean one, before
 *        a caller's function runs.
 *
 * @param[out] _watch   the watch, whatever it held ignored.
 * @param[in]  _parser  the parser; NULL watches nothing.
 */
void        d_arg_internal_watch_begin(struct d_arg_internal_watch* _watch,
                                       struct d_arg_parser*         _parser);
/**
 * @brief Ends a watch, telling whether the function recorded the failure it
 *        returned.
 *
 * @note When the report holds `_status`, a failure, it is the function's,
 *       and stays; the report set aside is dropped. Otherwise the report set
 *       aside comes back, and whatever the function recorded is dropped.
 *
 * @param[in,out] _watch   the watch begun.
 * @param[in]     _status  what the function returned.
 * @return whether the function recorded the failure it returned.
 */
bool        d_arg_internal_watch_end(struct d_arg_internal_watch* _watch,
                                     int                          _status);
/**
 * @brief Ends a watch over a command's callback, as
 *        d_arg_internal_watch_end does, so that the parse's every failure is
 *        in the parser's report.
 *
 * @note A status other than D_ARG_OK that the callback returned without
 *       recording it -- a completed request, D_ARG_SUCCESS, an error -- is
 *       recorded bare against the command, with the status's own text and
 *       exit code.
 *
 * @param[in,out] _watch    the watch begun.
 * @param[in]     _command  the command whose callback ran.
 * @param[in]     _status   what the callback returned.
 * @return `_status`.
 */
int         d_arg_internal_watch_close(struct d_arg_internal_watch* _watch,
                                       const struct d_arg_command*  _command,
                                       int                          _status);

// 7.1.5
// d_arg_internal_unreport
//   cleanup: clears the parser's error reports' pointers to a command or an
// option about to be released, so that a report never names a record that
// is gone. Called as each command and each option is released; NULL for the
// parser, the command or the option is ignored.
void        d_arg_internal_unreport(struct d_arg_parser*        _parser,
                                    const struct d_arg_command* _command,
                                    const struct d_arg_option*  _option);

// 7.2    Options
//------------------------------------------------------------------------------
// 7.2.1
// d_arg_internal_option_create / d_arg_internal_option_destroy
/**
 * @brief Makes an option from CLI11's comma-separated names, outside any
 *        command's list; Option's constructor.
 *
 * @param[in] _command      the command the option is for, whose
 *                          non-standard-names switch the names are read
 *                          under.
 * @param[in] _names        the names, without flag values.
 * @param[in] _description  the description, copied; NULL for none.
 * @return the option, its `parent` set; NULL after recording a definition
 *         error: a BadNameString, or D_ARG_ERROR_OUT_OF_MEMORY.
 */
struct d_arg_option*
            d_arg_internal_option_create(struct d_arg_command* _command,
                                         struct d_pack_text    _names,
                                         const char*           _description);
/**
 * @brief Releases everything an option owns, and the option.
 *
 * @param[in] _option  the option, no longer in any list; may be NULL.
 */
void        d_arg_internal_option_destroy(struct d_arg_option* _option);

// 7.2.2
// d_arg_internal_option_replace_text
/**
 * @brief Replaces an option's owned text field with a copy.
 *
 * @param[in,out] _option  the option, for the failure's report.
 * @param[in,out] _field   the field.
 * @param[in]     _text    the text; NULL clears the field.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY recorded as a definition
 *         error, the field then unchanged.
 */
int         d_arg_internal_option_replace_text(struct d_arg_option* _option,
                                               char**               _field,
                                               const char*          _text);

// 7.2.3
// d_arg_internal_option_add / d_arg_internal_option_add_built
/**
 * @brief Adds a value to a list of an option's values -- its results, or a
 *        list being worked out -- as CLI11's _add_result does.
 *
 * @note A value that is not split is kept as the span it is when `_stable`
 *       says it outlives the option's values; it then lies within a
 *       NUL-terminated string, and is kept only when it ends at the NUL.
 *       Split, or not stable, it is copied into the option's `owned`. The
 *       option's state is not changed.
 *
 * @param[in,out] _option  the option.
 * @param[in,out] _list    the list to add to.
 * @param[in]     _value   the value.
 * @param[in]     _stable  whether `_value` may be kept as it is.
 * @param[out]    _added   receives how many values the split made; may be
 *                         NULL.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, recorded, with the values
 *         added before the failure kept.
 */
int         d_arg_internal_option_add(struct d_arg_option*    _option,
                                      struct d_arg_text_list* _list,
                                      struct d_pack_text      _value,
                                      bool                    _stable,
                                      int*                    _added);
/**
 * @brief Adds a value built in a buffer as d_arg_internal_option_add adds
 *        one, without copying it again: the buffer's storage becomes one of
 *        the option's copies first, and the value is kept as its span.
 *
 * @param[in,out] _option  the option, which takes the storage.
 * @param[in,out] _list    the list to add to.
 * @param[in,out] _built   the value; its storage is taken, so the buffer is
 *                         left empty, but for one that had failed.
 * @param[out]    _added   receives how many values the split made; may be
 *                         NULL.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, recorded -- for a buffer
 *         that had failed too.
 */
int         d_arg_internal_option_add_built(struct d_arg_option*    _option,
                                            struct d_arg_text_list* _list,
                                            struct d_arg_buffer*    _built,
                                            int*                    _added);

// 7.2.4
// d_arg_internal_option_validate
/**
 * @brief Runs an option's validators over a list of its values; CLI11's
 *        _validate_results.
 *
 * @note A value a validator rewrote is replaced in the list by a copy the
 *       option keeps. Indices count as CLI11 counts them: within each item
 *       for a multi-value type, from the end when the policy keeps the last
 *       values of too many.
 *
 * @param[in,out] _option  the option.
 * @param[in,out] _list    the values: the option's results, or a copy.
 * @return D_ARG_OK; D_ARG_ERROR_VALIDATION, recorded as "<name>:
 *         <reason>"; or the error a validator stopped with or a copy failed
 *         with, recorded as raised.
 */
int         d_arg_internal_option_validate(struct d_arg_option*    _option,
                                           struct d_arg_text_list* _list);

// 7.2.5
// d_arg_internal_option_judge
/**
 * @brief Runs an option's validators over a copy of a value; CLI11's
 *        _validate.
 *
 * @note An empty value passes when the option expects no values. The
 *       validators that apply to `_index` -- or to every value -- run in
 *       order, each on what the one before made of the value, until one
 *       rejects it. One that stops with D_ARG_ERROR_VALIDATION rejects the
 *       value with its message -- or passes it, with none -- as CLI11's
 *       _validate catches a ValidationError.
 *
 * @param[in]     _option   the option.
 * @param[in]     _value    the value.
 * @param[in]     _index    its position, for validators applying to one.
 * @param[in,out] _message  receives the rejection, appended; empty when the
 *                          value passes.
 * @param[in,out] _result   receives what the validators made of the value,
 *                          appended, when it passes; may be NULL.
 * @return D_ARG_OK when a verdict was reached and written; otherwise the
 *         error a validator stopped with, its message in `_message`, or
 *         D_ARG_ERROR_OUT_OF_MEMORY when the verdict or the result could not
 *         be written. Nothing is recorded.
 */
int         d_arg_internal_option_judge(const struct d_arg_option* _option,
                                        struct d_pack_text         _value,
                                        int                        _index,
                                        struct d_arg_buffer*       _message,
                                        struct d_arg_buffer*       _result);

// 7.2.6
// d_arg_internal_option_reduce
/**
 * @brief Applies an option's multi-option policy; CLI11's _reduce_results.
 *
 * @note `_out` is emptied, then holds what the policy kept of `_original`
 *       -- nothing where the values stand as they are -- with CLI11's
 *       empty-container indicator completed. A joined or summed value is a
 *       copy in the option's `owned`; every other is a span of
 *       `_original`'s.
 *
 * @param[in,out] _option    the option, which keeps a new value's copy.
 * @param[out]    _out       the reduced values; not `_original`.
 * @param[in]     _original  the values to reduce.
 * @return D_ARG_OK; D_ARG_ERROR_ARGUMENT_MISMATCH, recorded as CLI11 words
 *         it, for too few or too many values under D_ARG_MULTI_THROW; or
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded.
 */
int         d_arg_internal_option_reduce(struct d_arg_option*          _option,
                                         struct d_arg_text_list*       _out,
                                         const struct d_arg_text_list* _original);

// 7.2.7
// d_arg_internal_option_results_text
//   writing: CLI11's detail::join of an option's values as given -- a comma
// between each two, and a comma that ends the text dropped, as CLI11 drops
// it -- for "Could not convert" messages.
int     d_arg_internal_option_results_text(const struct d_arg_option* _option,
                                           struct d_arg_buffer*       _out);

// 7.2.8
// d_arg_internal_option_attach
/**
 * @brief Attaches a variable to an option: its type, and the variable as
 *        the default function; the part of d_arg_option_bind an option
 *        made for a variable needs before its defaults are applied.
 *
 * @param[in,out] _option  the option.
 * @param[in]     _target  the variable; NULL detaches, dropping the variable
 *                         as the default function.
 * @param[in]     _type    its type.
 * @return D_ARG_OK, or D_ARG_ERROR_BAD_ARGUMENT, recorded as a definition
 *         error, for a variable without a type that converts.
 */
int         d_arg_internal_option_attach(struct d_arg_option*     _option,
                                         void*                    _target,
                                         const struct d_arg_type* _type);

// 7.2.9
// d_arg_internal_option_reduced
/**
 * @brief The values an option's policy keeps; CLI11's reduced_results().
 *        Defined in arg_option_result.c.
 *
 * @note Values already reduced are the kept ones; any others -- the values
 *       as given -- are validated and reduced on the copy, the option's
 *       values and state left as they are, but for the copies a validator
 *       or the policy makes, which the option keeps.
 *
 * @param[in,out] _option  the option.
 * @param[in,out] _out     an empty list, which receives the values as spans
 *                         valid until the option changes.
 * @return D_ARG_OK, or the failure validation or reduction recorded.
 */
int         d_arg_internal_option_reduced(struct d_arg_option*    _option,
                                          struct d_arg_text_list* _out);

// 7.3    Commands
//------------------------------------------------------------------------------
// 7.3.1
// d_arg_internal_command_init / d_arg_internal_command_release
/**
 * @brief Makes a command in place; CLI11's App constructor.
 *
 * @note With a parent, the command inherits what CLI11's does: the help
 *       flags, the option defaults, the modes, the inherited switches, the
 *       group, usage and footer text, the formatter and configuration
 *       format, the subcommand maximum. The failure message is not copied:
 *       a command without one uses its parent's. Without a parent, it is a
 *       root with CLI11's App defaults and no help flag yet.
 *
 * @param[out] _command      the command; whatever it held is ignored.
 * @param[in]  _parser       the parser it belongs to.
 * @param[in]  _parent       its parent; NULL for a root.
 * @param[in]  _name         its name, copied; NULL or "" for none.
 * @param[in]  _description  its description, copied; NULL for none.
 * @return D_ARG_OK; otherwise the definition error, recorded against the
 *         parent when there is one, with the command left releasable.
 */
int         d_arg_internal_command_init(struct d_arg_command* _command,
                                        struct d_arg_parser*  _parser,
                                        struct d_arg_command* _parent,
                                        const char*           _name,
                                        const char*           _description);
/**
 * @brief Releases everything a command owns -- its options and, in turn,
 *        its subcommands -- but not the command itself.
 *
 * @param[in,out] _command  the command; may be NULL.
 */
void        d_arg_internal_command_release(struct d_arg_command* _command);

// 7.3.2
// d_arg_internal_fallthrough_parent
//   lookup: CLI11's _get_fallthrough_parent: the nearest ancestor that has a
// name or is the root; NULL for a root.
struct d_arg_command*
        d_arg_internal_fallthrough_parent(const struct d_arg_command* _command);

// 7.3.3
// d_arg_internal_compare_subcommand_names
//   matching: CLI11's _compare_subcommand_names: a name or alias of
// `_subcommand` that matches one of `_base`'s other enabled subcommands, or
// the reverse, looking into option groups on both sides; NULL when none
// does. The name returned is owned by one of the commands.
const char*
     d_arg_internal_compare_subcommand_names(const struct d_arg_command* _subcommand,
                                             const struct d_arg_command* _base);

// 7.3.4
// d_arg_internal_find_subcommand
//   lookup: CLI11's _find_subcommand: the subcommand a name selects, looking
// into option groups, skipping disabled ones when asked and, when asked,
// those already used; with prefix matching an unambiguous prefix selects,
// and an ambiguous one selects nothing.
struct d_arg_command*
     d_arg_internal_find_subcommand(const struct d_arg_command* _command,
                                    struct d_pack_text          _name,
                                    bool                        _ignore_disabled,
                                    bool                        _ignore_used);

// 7.3.5
// d_arg_internal_find_option
//   lookup: d_arg_command_option, the name given as a span.
struct d_arg_option*
            d_arg_internal_find_option(const struct d_arg_command* _command,
                                       struct d_pack_text          _name);

// 7.3.6
// d_arg_internal_parser_of
//   lookup: the parser an option belongs to, through its command; NULL for
// NULL.
struct d_arg_parser*
                   d_arg_internal_parser_of(const struct d_arg_option* _option);

// 7.3.7
// d_arg_internal_command_options and kin
//   d_arg_internal_filter_fn is the filter's type, CLI11's get_options
// filter: whether an option is to be collected.
typedef bool (*d_arg_internal_filter_fn)(const struct d_arg_option* _option,
                                         const void*                _context);
/**
 * @brief Collects the options a command lists; CLI11's get_options.
 *
 * @note The command's own options, then those of the subcommands it
 *       merges, then -- for a named command that falls through -- those of
 *       its fallthrough parent that answer to none of the display names
 *       collected so far. With `_help` the merged subcommands are the
 *       nameless ones whose group starts with `+`, as CLI11's const form,
 *       which help and option checks use; otherwise every nameless
 *       subcommand and every one whose group starts with `+`, as its other
 *       form, which configuration and requirements use.
 *
 * @param[in]     _command  the command.
 * @param[in]     _help     which form.
 * @param[in,out] _out      receives the options, appended.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY with the options collected
 *         before the failure kept.
 */
int  d_arg_internal_command_options(const struct d_arg_command* _command,
                                    bool                        _help,
                                    struct d_arg_option_list*   _out);
/**
 * @brief Collects the options a command lists that a filter passes;
 *        CLI11's get_options(filter).
 *
 * @note The filter is applied at every step, so that a fallthrough
 *       parent's option is compared with the options it kept, as in CLI11.
 *
 * @param[in]     _command  the command.
 * @param[in]     _help     which form, as d_arg_internal_command_options.
 * @param[in]     _test     the filter; NULL passes every option.
 * @param[in]     _context  `_test`'s context.
 * @param[in,out] _out      receives the options, appended.
 * @return a status, as d_arg_internal_command_options's.
 */
int  d_arg_internal_command_options_where(const struct d_arg_command* _command,
                                          bool                        _help,
                                          d_arg_internal_filter_fn    _test,
                                          const void*                 _context,
                                          struct d_arg_option_list*   _out);

// 7.3.8
// d_arg_internal_command_forget_option
//   cleanup: drops every reference to an option from the whole tree a
// command belongs to -- other options' needs and excludes, the commands'
// own, their help, version and configuration pointers, the last parse's
// order -- and the parser's completion pointer, before the option is
// released. Defined in arg_command_option.c.
void d_arg_internal_command_forget_option(struct d_arg_command*      _command,
                                          const struct d_arg_option* _option);

// 7.3.9
// d_arg_internal_command_untrigger
//   cleanup: takes a command about to be released out of the target list of
// a trigger's pre-parse callback -- one d_arg_command_trigger_on or _off
// installed -- leaving any other callback alone. Defined in
// arg_command_util.c.
void d_arg_internal_command_untrigger(struct d_arg_command*       _trigger,
                                      const struct d_arg_command* _gone);

// 7.4    Validators
//------------------------------------------------------------------------------
// 7.4.1
// d_arg_internal_validator_failed
/**
 * @brief Makes a validator that holds only a failure.
 *
 * @param[in] _status  the failure's status.
 * @param[in] _reason  why it failed, copied as the validator's description
 *                     when the copy can be made; NULL for none.
 * @return the failed validator, which owns its description.
 */
struct d_arg_validator
     d_arg_internal_validator_failed(int         _status,
                                     const char* _reason);

// 7.4.2
// d_arg_internal_validator_bounds and kin
//   range parts: a validator made as d_arg_validator_range makes one keeps
// its type as `parameter` and its bounds in `low` and `high`. bounds loads
// the two bounds; interval appends "[<low> - <high>]", the bounds streamed
// as CLI11 streams them; read converts the value under validation
// to the type as CLI11's lexical_cast does, `*_read` set when it converted,
// and returns D_ARG_OK or the status the conversion stopped with.
void d_arg_internal_validator_bounds(const struct d_arg_validator* _validator,
                                     struct d_arg_internal_number* _low,
                                     struct d_arg_internal_number* _high);
int  d_arg_internal_validator_interval(const struct d_arg_validator* _validator,
                                       struct d_arg_buffer*          _out);
int  d_arg_internal_validator_read(const struct d_arg_validator*  _validator,
                                   const struct d_arg_validation* _validation,
                                   struct d_arg_internal_number*  _value,
                                   bool*                          _read);

// 7.4.3
// d_arg_internal_validator_reject / d_arg_internal_validator_stop
//   verdicts: reject records CLI11's message built by concatenation --
// `_before`, then `_text`, its bytes kept whole, NUL among them, then
// `_after` -- as the value's rejection, and returns D_ARG_OK, or
// D_ARG_ERROR_OUT_OF_MEMORY when the message could not be kept; stop
// records it as the message of the ValidationError CLI11 throws, and
// returns D_ARG_ERROR_VALIDATION, or D_ARG_ERROR_OUT_OF_MEMORY.
int  d_arg_internal_validator_reject(struct d_arg_validation* _validation,
                                     const char*              _before,
                                     struct d_pack_text       _text,
                                     const char*              _after);
int  d_arg_internal_validator_stop(struct d_arg_validation* _validation,
                                   const char*              _before,
                                   struct d_pack_text       _text,
                                   const char*              _after);

// 7.4.4
// d_arg_internal_validator_replace
//   transformation: replaces the value under validation with an object of a
// type, written as d_arg_internal_number_write writes it -- the text a
// transformation such as Bound's or AsNumberWithUnit's makes. Returns
// D_ARG_OK, D_ARG_ERROR_OUT_OF_MEMORY, or the status the writing stopped
// with, the value then unchanged.
int  d_arg_internal_validator_replace(struct d_arg_validation* _validation,
                                      const struct d_arg_type* _type,
                                      const void*              _object);


//==============================================================================
// 8.  THE PARSE
//==============================================================================
// CLI11's parse, one C function for each of its member functions or for a
// step of one, each returning D_ARG_OK where CLI11's returns normally and
// the status of the error it throws otherwise, recorded in the parser by
// the function that raised it; a step whose CLI11 function returns a
// boolean gives it through `_go_on`. The argument stack is the parser's
// `pending`, the next argument last, as CLI11's vector holds it, and every
// command's steps share it. Defined in the arg_parse*.c sources, but for
// the processing, arg_process.c's, and what arg_argv.c reads and splits.


// 8.1    Parsing
//------------------------------------------------------------------------------
// 8.1.1
// d_arg_internal_parse_run
//   parse: what CLI11's parse(vector&&) does once the parser is cleared: the
// checks, the configuration of the tree, the parse of `pending` by the
// root, the processing and the callbacks. Defined in arg_parse.c.
int                d_arg_internal_parse_run(struct d_arg_parser* _parser);

// 8.1.2
// d_arg_internal_validate / d_arg_internal_configure
//   preparation: CLI11's _validate, which fails with D_ARG_ERROR_INVALID,
// and _configure, which cannot fail.
int         d_arg_internal_validate(const struct d_arg_command* _command);
void        d_arg_internal_configure(struct d_arg_command* _command);

// 8.1.3
// d_arg_internal_parse_command
//   parse: CLI11's _parse(args): the command parses the parser's pending
// arguments until they run out or one belongs elsewhere.
int                d_arg_internal_parse_command(struct d_arg_command* _command);

// 8.1.4
// d_arg_internal_recognize
//   classification: CLI11's _recognize: what an argument is taken to be by a
// command, a d_arg_classifier value.
int         d_arg_internal_recognize(const struct d_arg_command* _command,
                                     struct d_pack_text          _argument,
                                     bool                        _ignore_used);

// 8.1.5
// d_arg_internal_increment_parsed
//   counting: CLI11's increment_parsed: the command and its option groups
// count one more use.
void        d_arg_internal_increment_parsed(struct d_arg_command* _command);

// 8.1.6
// d_arg_internal_trigger_pre_parse
//   parse: CLI11's _trigger_pre_parse: the pre-parse callback, once, or, for
// an immediate-callback subcommand met again, a fresh start that keeps its
// count and its unused arguments.
int         d_arg_internal_trigger_pre_parse(struct d_arg_command* _command,
                                             size_t                _remaining);

// 8.1.7
// d_arg_internal_parse_positional / d_arg_internal_parse_arg
/**
 * @brief Offers the argument on top of the stack to a command as a
 *        positional; CLI11's _parse_positional. Defined in
 *        arg_parse_positional.c.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _halt     CLI11's haltOnSubcommand: a subcommand of the
 *                          command's met again is left on the stack for the
 *                          caller rather than parsed again.
 * @param[out]    _go_on    receives CLI11's result: `true` when the argument
 *                          was dealt with -- placed, parsed as a subcommand
 *                          or kept unused -- and `false` when it is left on
 *                          the stack for a parent.
 * @return D_ARG_OK, or the error a step raised, recorded.
 */
int  d_arg_internal_parse_positional(struct d_arg_command* _command,
                                     bool                  _halt,
                                     bool*                 _go_on);
/**
 * @brief Offers the argument on top of the stack, recognized as an option,
 *        to a command; CLI11's _parse_arg. Defined in arg_parse_option.c.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _class    the option's form: D_ARG_CLASS_LONG,
 *                          D_ARG_CLASS_SHORT or D_ARG_CLASS_WINDOWS_STYLE.
 * @param[in]     _local    CLI11's local_processing_only: an argument the
 *                          command's own search does not place is left on
 *                          the stack, neither handed to a parent nor kept
 *                          unused.
 * @param[out]    _go_on    receives CLI11's result: `true` when the argument
 *                          was dealt with, `false` when it is left on the
 *                          stack for a parent.
 * @return D_ARG_OK, or the error a step raised, recorded.
 */
int  d_arg_internal_parse_arg(struct d_arg_command* _command,
                              int                   _class,
                              bool                  _local,
                              bool*                 _go_on);

// 8.1.8
// d_arg_internal_parse_begin and kin
/**
 * @brief Starts a parse, as every entry point does: CLI11's clear() of a root
 *        parsed before, the last parse's error, storage and program name
 *        forgotten, and a definition error reported as this parse's.
 *        Defined in arg_parse.c.
 *
 * @param[in,out] _parser  the parser.
 * @return D_ARG_OK, or the definition error, which ends the parse.
 */
int  d_arg_internal_parse_begin(struct d_arg_parser* _parser);
/**
 * @brief Starts a call that parses without clearing the root, as CLI11's
 *        parse_from_stream adds to a parse: the last call's error and
 *        stacked arguments forgotten -- its storage, which values may borrow
 *        from, kept -- and a definition error reported as this call's.
 *        Defined in arg_parse.c.
 *
 * @param[in,out] _parser  the parser.
 * @return D_ARG_OK, or the definition error, which ends the call.
 */
int  d_arg_internal_parse_resume(struct d_arg_parser* _parser);
/**
 * @brief Keeps a copy of the program's name as the parser's `program`, then
 *        names the root from it when the root takes its name from there, as
 *        CLI11 names an App from `argv[0]`: a root without a name, or one
 *        named so before.
 *
 * @param[in,out] _parser  the parser.
 * @param[in]     _name    the program's name, NUL-terminated.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, recorded, the parser then
 *         as it was.
 */
int  d_arg_internal_parse_name(struct d_arg_parser* _parser,
                               const char*          _name);
/**
 * @brief Stacks words the parser's pool keeps -- a split line's, converted
 *        wide arguments -- from `_first` on, the first on top; then expands
 *        the response files, as the other entry points do.
 *
 * @param[in,out] _parser  the parser.
 * @param[in]     _first   the index of the pool's first word to stack.
 * @return D_ARG_OK, or a failure, recorded.
 */
int  d_arg_internal_parse_words(struct d_arg_parser* _parser,
                                size_t               _first);

// 8.2    Processing
//------------------------------------------------------------------------------
// 8.2.1
// d_arg_internal_process / d_arg_internal_process_extras
//   processing: CLI11's _process -- configuration files, the environment,
// the callbacks by priority with the help flags among them, the
// requirements -- and _process_extras. Defined in arg_process.c.
int         d_arg_internal_process(struct d_arg_command* _command);
int         d_arg_internal_process_extras(struct d_arg_command* _command);

// 8.2.2
// d_arg_internal_run_callback
//   processing: CLI11's run_callback(final_mode, suppress_final_callback).
// Defined in arg_process.c.
int         d_arg_internal_run_callback(struct d_arg_command* _command,
                                        bool                  _final_mode,
                                        bool                  _suppress_final);

// 8.2.3
// d_arg_internal_process_completion_callbacks
//   processing: CLI11's _process_completion_callbacks, for a subcommand with
// a parse-complete callback. Defined in arg_process.c.
int  d_arg_internal_process_completion_callbacks(struct d_arg_command* _command,
                                                 bool                  _with_help_flags);

// 8.2.4
// d_arg_internal_parse_config
//   processing: CLI11's _parse_config: each item to the option it names, an
// item nothing takes refused when configuration extras are errors. Defined
// in arg_process_config.c, with D_INTERNAL_ARG_PARSER_CONFIG_FILES.
int  d_arg_internal_parse_config(struct d_arg_command*                _command,
                                 const struct d_arg_config_item_list* _items);

// 8.2.5
// d_arg_internal_add_flag_like_result
/**
 * @brief Gives an option expecting no values its value from an item;
 *        CLI11's _add_flag_like_result. Defined in arg_process_config.c, in
 *        every build: a positional expecting no values takes its argument
 *        this way.
 *
 * @note A single input, or none, is read through the command's
 *       configuration format as a flag and handed to the option as a flag
 *       given by the item's name; several are refused beyond the option's
 *       maximum -- unless its policy takes or joins them all -- or else each
 *       added, an option refusing flag overrides accepting only its flag
 *       values.
 *
 * @param[in]     _command  the command whose parse or configuration reads
 *                          the item, whose format reads it.
 * @param[in,out] _option   the option.
 * @param[in]     _item     the item.
 * @param[in]     _inputs   the inputs to use: the item's, or a filtered
 *                          view of them.
 * @param[out]    _handled  receives whether the option took the inputs;
 *                          `false` leaves them for the caller to add.
 * @return D_ARG_OK, or the error CLI11 throws, recorded.
 */
int  d_arg_internal_add_flag_like_result(const struct d_arg_command*     _command,
                                         struct d_arg_option*            _option,
                                         const struct d_arg_config_item* _item,
                                         const struct d_arg_text_list*   _inputs,
                                         bool*                           _handled);

// 8.2.6
// d_arg_internal_process_config
//   processing: CLI11's _process_config_file for the root: the configuration
// option, given nothing, takes its environment variable's value, is called
// back, and the files it names are read and parsed -- each must be read
// when the option is required or given -- a failure recorded. Without
// D_INTERNAL_ARG_PARSER_CONFIG_FILES a file that is there is refused with
// D_ARG_ERROR_CONFIG. Defined in arg_process_config.c.
int  d_arg_internal_process_config(struct d_arg_command* _command);

// 8.3    Arguments
//------------------------------------------------------------------------------
// 8.3.1
// d_arg_internal_pending_push / d_arg_internal_pending_push_copy
//   the argument stack: push puts back an argument whose bytes outlive the
// parse and end in a NUL; push_copy puts back a copy the parser's pool
// keeps until the next parse. Defined in arg_parse.c.
int         d_arg_internal_pending_push(struct d_arg_parser* _parser,
                                        struct d_pack_text   _text);
int         d_arg_internal_pending_push_copy(struct d_arg_parser* _parser,
                                             struct d_pack_text   _text);

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// 8.3.2
// d_arg_internal_expand_responses
//   preparation: replaces each `@file` on the stack before a `--` with the
// file's words, as d_arg_parser_set_response_files describes, recording a
// failure. Defined in arg_argv_source.c.
int         d_arg_internal_expand_responses(struct d_arg_parser* _parser);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

// 8.3.3
// d_arg_internal_split_up and kin
/**
 * @brief Splits text as CLI11's split_up does.
 *
 * @note Surrounding whitespace is trimmed, and so is each piece's. A piece
 *       starting with a quote or a bracket runs to its match, quotes and
 *       brackets kept, and a delimiter or an opening quote or bracket right
 *       after it is consumed; any other piece runs to the next delimiter --
 *       with NUL for the delimiter, any whitespace. Defined in arg_argv.c.
 *
 * @param[in]     _text       the text.
 * @param[in]     _delimiter  the separator, or NUL for whitespace.
 * @param[in,out] _out        receives the pieces, appended.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY with the pieces stored
 *         before the failure kept.
 */
int         d_arg_internal_split_up(struct d_pack_text        _text,
                                    char                      _delimiter,
                                    struct d_arg_string_list* _out);
/**
 * @brief One turn of d_arg_internal_split_up, for a caller that keeps the
 *        pieces as spans of the text: each piece is one, NUL bytes and all.
 *        Defined in arg_argv.c.
 *
 * @param[in,out] _rest       the text still to split, trimmed and not
 *                            empty; receives what follows the piece,
 *                            trimmed.
 * @param[in]     _delimiter  the separator, or NUL for whitespace.
 * @param[out]    _word       receives the piece.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY when a bracket's nesting
 *         could not be followed.
 */
int         d_arg_internal_split_step(struct d_pack_text* _rest,
                                      char                _delimiter,
                                      struct d_pack_text* _word);
/**
 * @brief Whether a value is one of the four d_arg_quoting sets of rules.
 *        Defined in arg_argv.c.
 *
 * @param[in] _quoting  the value.
 * @return `true` for D_ARG_QUOTING_POSIX, _WINDOWS, _CLI11 and _GNU, which
 *         NATIVE and NATIVE_RESPONSE are among.
 */
bool        d_arg_internal_quoting_known(int _quoting);
/**
 * @brief Finds where a quoted or bracketed run closes; CLI11's
 *        close_sequence. Defined in arg_argv.c.
 *
 * @note A double quote closes at the next one no backslash escapes, past
 *       the end by one when a backslash is last; a single or back quote --
 *       or any byte that is not a bracket's closer -- at the next such byte;
 *       a bracket where its nesting, quotes skipped whole, comes back to it.
 *       A run that never closes ends at the text's length.
 *
 * @param[in]  _text     the text.
 * @param[in]  _start    the position of the opening byte.
 * @param[in]  _closure  the byte that closes it.
 * @param[out] _end      receives the closing byte's position.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY when a bracket's nesting
 *         could not be followed.
 */
int         d_arg_internal_close_sequence(struct d_pack_text _text,
                                          size_t             _start,
                                          char               _closure,
                                          size_t*            _end);
/**
 * @brief Removes one word's quotes as CLI11's remove_quotes(vector) does.
 *        Defined in arg_argv.c.
 *
 * @note A word in double quotes loses them, then its escapes, as CLI11's
 *       remove_escaped_characters resolves them; one in single quotes or
 *       backquotes loses them alone; any other is kept as it is.
 *
 * @param[in]     _word   the word; not empty.
 * @param[in,out] _out    receives the word, appended.
 * @param[in,out] _error  receives the description of a malformed escape,
 *                        appended; may be NULL.
 * @retval D_ARG_OK                   appended.
 * @retval D_ARG_ERROR_INVALID        a malformed escape, as CLI11's
 *                                    ParseError(InvalidError).
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  the word could not be stored.
 */
int         d_arg_internal_remove_quotes(struct d_pack_text        _word,
                                         struct d_arg_string_list* _out,
                                         struct d_arg_buffer*      _error);
/**
 * @brief d_arg_internal_remove_quotes into a buffer, which keeps whatever
 *        bytes an escape resolves to, NUL among them. Defined in arg_argv.c.
 *
 * @param[in]     _word   the word; not empty.
 * @param[in,out] _out    receives the word, appended.
 * @param[in,out] _error  receives the description of a malformed escape,
 *                        appended; may be NULL.
 * @return a status, as d_arg_internal_remove_quotes's.
 */
int         d_arg_internal_remove_quotes_text(struct d_pack_text   _word,
                                              struct d_arg_buffer* _out,
                                              struct d_arg_buffer* _error);

// 8.3.4
// d_arg_internal_parse_back and kin
//   the stack as a command's steps see it: back is the next argument, which
// must be there; pop takes it off, its bytes staying where they are; left
// counts the arguments waiting, CLI11's args.size(). Defined in arg_parse.c.
struct d_pack_text
       d_arg_internal_parse_back(const struct d_arg_command* _command);
void   d_arg_internal_parse_pop(const struct d_arg_command* _command);
size_t d_arg_internal_parse_left(const struct d_arg_command* _command);

// 8.3.5
// d_arg_internal_parse_push_built
/**
 * @brief Pushes an argument CLI11 builds and puts back on the stack: the
 *        prefix, the name, then `=` and the value when there is a value,
 *        written once into storage the parser's pool keeps until the next
 *        parse. Defined in arg_parse.c.
 *
 * @param[in] _command  the command whose step builds it.
 * @param[in] _prefix   "-" or "--".
 * @param[in] _name     the name, or what followed a short name.
 * @param[in] _value    the value; empty for none.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, recorded.
 */
int  d_arg_internal_parse_push_built(const struct d_arg_command* _command,
                                     const char*                 _prefix,
                                     struct d_pack_text          _name,
                                     struct d_pack_text          _value);

// 8.3.6
// d_arg_internal_parse_at and kin
//   splitting: at cuts text at its first `_separator` into the name before
// it and the value after it -- empty, at the text's end, when there is no
// separator; split_long, split_short and split_windows are CLI11's
// split_long, split_short and split_windows_style on spans of the
// argument, each false, its outputs untouched, for an argument not of its
// form. Defined in arg_parse.c.
void d_arg_internal_parse_at(struct d_pack_text  _text,
                             char                _separator,
                             struct d_pack_text* _name,
                             struct d_pack_text* _value);
bool d_arg_internal_parse_split_long(struct d_pack_text  _argument,
                                     struct d_pack_text* _name,
                                     struct d_pack_text* _value);
bool d_arg_internal_parse_split_short(struct d_pack_text  _argument,
                                      struct d_pack_text* _name,
                                      struct d_pack_text* _rest);
bool d_arg_internal_parse_split_windows(struct d_pack_text  _argument,
                                        struct d_pack_text* _name,
                                        struct d_pack_text* _value);

// 8.4    Steps
//------------------------------------------------------------------------------
//   what the parse's three sources share, defined in arg_parse.c.

// 8.4.1
// d_arg_internal_parse_stored
//   recording: a status from storage -- a list that grew, or could not --
// recorded against the command when it is a refusal; returns the status.
int  d_arg_internal_parse_stored(const struct d_arg_command* _command,
                                 int                         _status);

// 8.4.2
// d_arg_internal_parse_to_missing and kin
/**
 * @brief Keeps an argument a command did not use; CLI11's
 *        _move_to_missing.
 *
 * @note Under D_ARG_EXTRAS_ERROR_IMMEDIATELY, an error at once; otherwise
 *       among the command's unused arguments where it captures them, has
 *       no subcommands or is a prefix command, else among those of its
 *       first option group that captures them, else among its own -- but
 *       nowhere save that group when extras are ignored.
 *
 * @param[in,out] _command   the command.
 * @param[in]     _class     what the argument was taken to be, a
 *                           d_arg_classifier value.
 * @param[in]     _argument  the argument, from the stack.
 * @return D_ARG_OK; D_ARG_ERROR_EXTRAS, recorded as CLI11's ExtrasError;
 *         or D_ARG_ERROR_OUT_OF_MEMORY, recorded.
 */
int  d_arg_internal_parse_to_missing(struct d_arg_command* _command,
                                     int                   _class,
                                     struct d_pack_text    _argument);
/**
 * @brief Keeps everything left on the stack among a command's unused
 *        arguments as positionals, whatever its extras mode, as CLI11's
 *        prefix commands keep it.
 *
 * @param[in,out] _command  the command.
 * @post   the stack is empty, unless the keeping failed.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, recorded.
 */
int  d_arg_internal_parse_rest_to_missing(struct d_arg_command* _command);
/**
 * @brief Records CLI11's ExtrasError(name, args): "<name>: The following
 *        argument was not expected: <argument>", or "arguments were" for
 *        several, joined by spaces, without the name's part for a command
 *        without a name.
 *
 * @note The arguments are joined as CLI11's join joins them, which drops a
 *       delimiter that ends the joined text -- here, a space that ends the
 *       last argument. The suggestions d_arg_internal_suggest_extras makes
 *       follow, when the parser asks for them.
 *
 * @param[in] _command  the command.
 * @param[in] _stack    the arguments as a stack holds them, read from its
 *                      top -- the last -- down; NULL to name `_unused`.
 * @param[in] _unused   unused arguments, their texts read in order, when
 *                      `_stack` is NULL.
 * @param[in] _count    how many.
 * @return D_ARG_ERROR_EXTRAS, or D_ARG_ERROR_OUT_OF_MEMORY when the message
 *         could not be written; either recorded.
 */
int  d_arg_internal_parse_extras_error(const struct d_arg_command* _command,
                                       const struct d_pack_text*   _stack,
                                       const struct d_arg_extra*   _unused,
                                       size_t                      _count);

// 8.4.3
// d_arg_internal_parse_remaining
//   arithmetic: CLI11's _count_remaining_positionals: the values a command's
// positionals -- only the required ones, when asked -- still need to reach
// their minimums.
size_t d_arg_internal_parse_remaining(const struct d_arg_command* _command,
                                      bool                        _required);

// 8.4.4
// d_arg_internal_parse_judge
/**
 * @brief Asks an option's validators about a value as the parse asks them,
 *        of a positional or of an optional value; CLI11's
 *        opt->_validate(value, 0).
 *
 * @param[in]  _command   the command whose step asks, for the report.
 * @param[in]  _option    the option.
 * @param[in]  _value     the value, as given.
 * @param[out] _accepted  receives whether the validators accept it.
 * @return D_ARG_OK when they reached a verdict; otherwise the error a
 *         validator stopped with, recorded, as the exception escapes
 *         CLI11's.
 */
int  d_arg_internal_parse_judge(const struct d_arg_command* _command,
                                const struct d_arg_option*  _option,
                                struct d_pack_text          _value,
                                bool*                       _accepted);

// 8.4.5
// d_arg_internal_parse_add / d_arg_internal_parse_separate
/**
 * @brief Adds a value from the stack to an option's values -- split,
 *        perhaps -- as CLI11's add_result(value) adds it.
 *
 * @param[in,out] _option  the option, PARSING afterwards.
 * @param[in]     _value   the value, which outlives the option's values.
 * @param[out]    _added   receives how many values the split made; may be
 *                         NULL.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, recorded.
 */
int  d_arg_internal_parse_add(struct d_arg_option* _option,
                              struct d_pack_text   _value,
                              int*                 _added);
/**
 * @brief Adds CLI11's separator between two occurrences' values, an empty
 *        value, to an option that asks for one, after a last value that is
 *        not empty.
 *
 * @param[in,out] _option  the option.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, recorded.
 */
int  d_arg_internal_parse_separate(struct d_arg_option* _option);

// 8.4.6
// d_arg_internal_parse_restart
//   values: CLI11's clear() of an option that triggers on parse, met again
// after its callback ran: no values, and PARSING -- the values first set
// aside in `_set_aside`, when given, which then holds their array. The
// copies the values lie in stay with the option until it is next cleared,
// so a value set aside, or a variable bound to one, outlives the restart.
void d_arg_internal_parse_restart(struct d_arg_option*    _option,
                                  struct d_arg_text_list* _set_aside);

// 8.4.7
// d_arg_internal_parse_open_group and kin
//   subcommands: open_group is whether a subcommand is an option group that
// takes arguments, nameless and enabled; below_max whether a command may
// take another subcommand, having no maximum or one not met.
bool d_arg_internal_parse_open_group(const struct d_arg_command* _command);
bool d_arg_internal_parse_below_max(const struct d_arg_command* _command);
/**
 * @brief Takes CLI11's pre-parse step for an option group that took an
 *        argument: once, with the count of arguments left.
 *
 * @param[in,out] _group  the option group.
 * @return D_ARG_OK, or the failure its pre-parse callback returned,
 *         recorded.
 */
int  d_arg_internal_parse_entered(struct d_arg_command* _group);
/**
 * @brief Lists a subcommand among a command's parsed subcommands, unless it
 *        is silent.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _sub      the subcommand.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, recorded.
 */
int  d_arg_internal_parse_list(struct d_arg_command* _command,
                               struct d_arg_command* _sub);

// 8.4.8
// d_arg_internal_parse_min / d_arg_internal_parse_max
//   arithmetic: CLI11's min_num and max_num, the fewest and the most values
// a name of the option takes from the arguments: its type's size, or fewer
// when it expects fewer items; and the items it expects at most -- for one
// that could take very many without taking extra arguments, its type's
// largest size for each item it expects at least, the unlimited count
// should that overflow.
int  d_arg_internal_parse_min(const struct d_arg_option* _option);
int  d_arg_internal_parse_max(const struct d_arg_option* _option);


//==============================================================================
// 9.  SYSTEM
//==============================================================================
// What the parse asks of the operating system, each answered one way on
// Windows -- where a path is UTF-8, as CLI11 widens one, or, when it is not
// valid UTF-8, in the ANSI code page, as `argv` holds it -- and another
// elsewhere, as env_arg_parser.h's facts select. Defined in arg_system.c,
// the one source that includes a system header, but for the suggestions,
// which are arg_suggest.c's.


// 9.1    Files
//------------------------------------------------------------------------------
// 9.1.1
// d_arg_internal_path_kind
//   query: CLI11's check_path: 0 for a path that does not exist, 1 for
// anything else but a directory, 2 for a directory; symbolic links are
// followed.
int                d_arg_internal_path_kind(const char* _path);

// 9.1.2
// d_arg_internal_path_info / d_arg_internal_path_query
//   query: one status query's answer: the kind, as d_arg_internal_path_kind
// gives it, whether the path is a regular file -- not a directory, a
// device, a pipe or a socket -- the size in bytes, and the read, write and
// execute permissions the file's bits grant anyone: on Windows, where there
// are no such bits, as Microsoft's std::filesystem reports them, read and
// execute always and write unless the file is read-only. query fills it and
// returns false, the kind 0, for a path that does not exist or cannot be
// examined.
struct d_arg_internal_path_info
{
    int       kind;
    bool      regular;
    uintmax_t size;
    bool      readable;
    bool      writable;
    bool      executable;
};

bool        d_arg_internal_path_query(const char*                      _path,
                                      struct d_arg_internal_path_info* _info);

// 9.1.3
// d_arg_internal_read_file
/**
 * @brief Reads a whole file, as its bytes or as text.
 *
 * @note As text, the file is read as the C runtime reads a file opened in
 *       text mode, which is how a std::ifstream opened without
 *       std::ios::binary reads it: on Windows a carriage return before a
 *       line feed is dropped, and a Ctrl-Z ends the text; elsewhere text is
 *       the bytes.
 *
 * @param[in]     _path  the path.
 * @param[in]     _text  whether the file is read as text.
 * @param[in,out] _out   receives the bytes, appended.
 * @post   on failure `_out` holds what it held.
 * @retval D_ARG_OK                   read.
 * @retval D_ARG_ERROR_FILE           the file could not be opened or read,
 *                                    a directory among them.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  the bytes could not be stored.
 */
int                d_arg_internal_read_file(const char*          _path,
                                            bool                 _text,
                                            struct d_arg_buffer* _out);

// 9.2    Environment
//------------------------------------------------------------------------------
// 9.2.1
// d_arg_internal_getenv
//   lookup: CLI11's get_environment_value: an environment variable's value
// appended to `_out`, or nothing when it is unset; returns the buffer's
// status.
int                d_arg_internal_getenv(const char*          _name,
                                         struct d_arg_buffer* _out);

// 9.3    Text encodings
//------------------------------------------------------------------------------
// 9.3.1
// d_arg_internal_wide_from_utf8
/**
 * @brief Converts UTF-8 to UTF-16 on the heap, for the wide calls of
 *        Windows.
 *
 * @param[in] _text  the text, NUL-terminated.
 * @return the UTF-16 text, NUL-terminated, which the caller frees; NULL
 *         when the text is not valid UTF-8 or the copy was refused.
 */
uint16_t*          d_arg_internal_wide_from_utf8(const char* _text);

// 9.3.2
// d_arg_internal_utf8_append
//   conversion: a code point appended as UTF-8 -- a surrogate, or a value past
// U+10FFFF, as U+FFFD; returns the buffer's status.
int                d_arg_internal_utf8_append(struct d_arg_buffer* _out,
                                              uint32_t             _code_point);

// 9.3.3
// d_arg_internal_utf16
//   struct: a UTF-16 decoder's state: the high surrogate waiting for the low
// one that completes it, or 0. Starts zeroed.
struct d_arg_internal_utf16
{
    uint32_t high;
};

// 9.3.4
// d_arg_internal_utf16_feed / d_arg_internal_utf16_finish
//   decoding: feed takes one UTF-16 unit, appending as UTF-8 the code point it
// completes -- an unpaired surrogate as U+FFFD, as Windows converts one --
// and finish appends U+FFFD for a high surrogate left waiting. Both return
// the buffer's status.
int  d_arg_internal_utf16_feed(struct d_arg_internal_utf16* _state,
                               uint32_t                     _unit,
                               struct d_arg_buffer*         _out);
int  d_arg_internal_utf16_finish(struct d_arg_internal_utf16* _state,
                                 struct d_arg_buffer*         _out);

// 9.4    Suggestions
//------------------------------------------------------------------------------
//   Defined in arg_suggest.c; the first two only with
// D_INTERNAL_ARG_PARSER_SUGGESTIONS.

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

// 9.4.1
// d_arg_internal_edit_distance
//   arithmetic: the optimal string alignment distance between two texts --
// Levenshtein's, with a swap of neighbours counting as one edit, byte by
// byte -- or `_limit` + 1 as soon as it is known to exceed `_limit`, which
// is taken as 8 when larger. Never allocates.
size_t             d_arg_internal_edit_distance(struct d_pack_text _a,
                                                struct d_pack_text _b,
                                                size_t             _limit);

// 9.4.2
// d_arg_internal_suggest
/**
 * @brief Writes the name an unexpected argument most likely meant.
 *
 * @note For `-x`, `--name` or `/name`, the short or long name of an option
 *       the command would have matched -- its own, its enabled option
 *       groups', and, with fallthrough, its parents' -- hidden ones left
 *       out, the name and value after `=` (or `:`) compared without its
 *       dashes or slash; for anything else, the name or alias of a
 *       subcommand it would have taken -- its own, its option groups', and,
 *       with subcommand fallthrough, its parents' -- disabled and hidden ones
 *       left out; and, either way, those of the commands the last parse used
 *       under it, where an argument they did not take may have meant one of
 *       theirs. Names are compared as their option or subcommand compares
 *       them, case or underscores ignored where it ignores them. Written as
 *       the user would type it -- with its dashes, or after a slash for a
 *       Windows-style argument -- only when one name is nearest, within
 *       D_INTERNAL_ARG_PARSER_SUGGESTION_DISTANCE edits, and not the
 *       argument itself.
 *
 * @param[in]     _command  the command that did not expect the argument.
 * @param[in]     _extra    the argument, with what it was taken to be.
 * @param[in,out] _out      receives the name, appended.
 * @return `true` when a name was written.
 */
bool               d_arg_internal_suggest(const struct d_arg_command* _command,
                                          const struct d_arg_extra*   _extra,
                                          struct d_arg_buffer*        _out);

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

// 9.4.3
// d_arg_internal_suggest_extras
/**
 * @brief Adds suggestions to the message about unexpected arguments.
 *
 * @note With D_ARG_PARSER_SUGGESTIONS set on the command's parser, a line
 *       for each argument with a suggestion: "Did you mean '<name>'?" for a
 *       single argument, "Did you mean '<name>' for '<argument>'?" among
 *       several. Without D_INTERNAL_ARG_PARSER_SUGGESTIONS, nothing.
 *
 * @param[in]     _command  the command that did not expect them.
 * @param[in]     _stack    the arguments as a stack, the first last, which
 *                          the command classifies; or NULL.
 * @param[in]     _unused   or the arguments kept with what they were taken
 *                          to be, in order.
 * @param[in]     _count    how many arguments.
 * @param[in,out] _text     the message, appended to.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY when the message's buffer
 *         has failed.
 */
int     d_arg_internal_suggest_extras(const struct d_arg_command* _command,
                                      const struct d_pack_text*   _stack,
                                      const struct d_arg_extra*   _unused,
                                      size_t                      _count,
                                      struct d_arg_buffer*        _text);

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// 9.5    The process
//------------------------------------------------------------------------------
// 9.5.1
// d_arg_internal_command_line
/**
 * @brief Reads the process's command line as the user gave it.
 *
 * @note Where D_ENV_ARG_PARSER_COMMAND_LINE reaches one: on Windows,
 *       GetCommandLineW's UTF-16, converted to UTF-8.
 *
 * @param[in,out] _out  receives the line, appended.
 * @retval D_ARG_OK                   read.
 * @retval D_ARG_ERROR_UNSUPPORTED    the platform keeps no command line
 *                                    beside `argv`.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  the line could not be stored.
 */
int                d_arg_internal_command_line(struct d_arg_buffer* _out);

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV


//==============================================================================
// 10.  HELP
//==============================================================================
// What the help formatter's default steps share: CLI11's numbers as it
// computes them, its text helpers, the widths a line width leaves, and its
// App::help. Each text helper appends and returns D_ARG_OK or the buffer's
// status.


// 10.1   Numbers
//------------------------------------------------------------------------------
// 10.1.1
// d_arg_internal_help_mode
//   predicate: whether a value is one of d_arg_help_mode's.
bool    d_arg_internal_help_mode(int _mode);

// 10.1.2
// d_arg_internal_help_ratio
//   arithmetic: a long-name alignment ratio as CLI11's setter stores it --
// kept from 0 to 1, inverted above, taken by its magnitude below 0 -- and
// 1/3, CLI11's default, for a NaN.
float   d_arg_internal_help_ratio(float _ratio);

// 10.1.3
// d_arg_internal_help_int
//   conversion: a width as CLI11's static_cast<int> takes it, held at INT_MAX
// where the cast would wrap.
int     d_arg_internal_help_int(size_t _value);

// 10.1.4
// d_arg_internal_help_signed / d_arg_internal_help_unsigned
//   writing: a count in decimal.
int     d_arg_internal_help_signed(struct d_arg_buffer* _out,
                                   intmax_t             _value);
int     d_arg_internal_help_unsigned(struct d_arg_buffer* _out,
                                     uintmax_t            _value);

// 10.2   Text
//------------------------------------------------------------------------------
// 10.2.1
// d_arg_internal_help_spaces / d_arg_internal_help_pad
//   writing: spaces appends `_count` spaces; pad is std::setw with std::left,
// padding what was appended since `_start` to `_width` characters, and
// adding nothing to text that fills it or for a width below 1.
int     d_arg_internal_help_spaces(struct d_arg_buffer* _out,
                                   size_t               _count);
int     d_arg_internal_help_pad(struct d_arg_buffer* _out,
                                size_t               _start,
                                int                  _width);

// 10.2.2
// d_arg_internal_help_paragraph
/**
 * @brief Appends text as CLI11's streamOutAsParagraph streams it.
 *
 * @note Each line -- a line feed ends one, and only a line that ended in
 *       one is followed by one -- has its words rejoined by single spaces and
 *       broken before a word that would carry it past the width; every line
 *       starts with the prefix.
 *
 * @param[in,out] _out         the text.
 * @param[in]     _text        the paragraph.
 * @param[in]     _width       the most characters a line holds after the
 *                             prefix, but for a longer word.
 * @param[in]     _prefix      the spaces each line starts with.
 * @param[in]     _skip_first  whether the first line goes without them.
 * @return D_ARG_OK, or the buffer's status.
 */
int     d_arg_internal_help_paragraph(struct d_arg_buffer* _out,
                                      struct d_pack_text   _text,
                                      size_t               _width,
                                      size_t               _prefix,
                                      bool                 _skip_first);

// 10.2.3
// d_arg_internal_help_column
//   writing: a description in the right column, as make_option and
// make_subcommand write it -- nothing for an empty one; a line feed first,
// when the left part reached the column, so that every line goes behind the
// column's indent; otherwise the first line where the left part ends.
int     d_arg_internal_help_column(const struct d_arg_formatter* _formatter,
                                   struct d_pack_text            _text,
                                   bool                          _next_line,
                                   struct d_arg_buffer*          _out);

// 10.2.4
// d_arg_internal_help_label
//   writing: CLI11's get_label(key).
int     d_arg_internal_help_label(const struct d_arg_formatter* _formatter,
                                  const char*                   _key,
                                  struct d_arg_buffer*          _out);

// 10.3   Widths
//------------------------------------------------------------------------------
// 10.3.1
// d_arg_internal_help_fitted
//   arithmetic: a width -- the right column's, after the left column, or a
// paragraph's -- narrowed so that its lines, after `_indent` characters,
// keep within the formatter's line width, when it has one; one character at
// least.
size_t  d_arg_internal_help_fitted(const struct d_arg_formatter* _formatter,
                                   size_t                        _width,
                                   size_t                        _indent);

// 10.4   Building
//------------------------------------------------------------------------------
// 10.4.1
// d_arg_internal_help_make
/**
 * @brief Builds a command's help; CLI11's App::help(prev, mode).
 *
 * @note The help of the last subcommand the last parse used, if any, under
 *       the name `_prefix` and the commands' names make, built by that
 *       command's formatter with its line width fitted: to the terminal, for
 *       a formatter with D_ARG_FORMAT_TERMINAL_WIDTH, and to `_limit`.
 *
 * @param[in]     _command  the command.
 * @param[in]     _prefix   the words before its name.
 * @param[in]     _mode     a d_arg_help_mode value.
 * @param[in]     _limit    the room the help it is part of leaves it, for a
 *                          formatter that keeps to a width; 0 for none.
 * @param[in,out] _out      receives the help, appended.
 * @return D_ARG_OK, or the status of the step that failed;
 *         D_ARG_ERROR_BAD_ARGUMENT for a NULL argument, a bad mode, or a
 *         command with no formatter to reach.
 */
int     d_arg_internal_help_make(const struct d_arg_command* _command,
                                 struct d_pack_text          _prefix,
                                 int                         _mode,
                                 size_t                      _limit,
                                 struct d_arg_buffer*        _out);


//==============================================================================
// 11.  COMPLETION
//==============================================================================
// The command tree as shell completion sees it -- what help shows, in the
// order the parse searches it -- and the writers that make a script of it
// for each shell. Defined in arg_completion.c, the writers in
// arg_completion_sh.c (bash and zsh), arg_completion_fish.c and
// arg_completion_pwsh.c; compiled only with D_INTERNAL_ARG_PARSER_COMPLETION.


// 11.1   The model
//------------------------------------------------------------------------------
#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

// 11.1.1
// d_arg_internal_complete_kind
//   enum: how an option's values complete: not at all, for one that takes
// none; as the choices its validators list; as files; as directories; or as
// nothing, for values no listing helps with -- numbers, truth values,
// characters, and a hidden positional's.
enum d_arg_internal_complete_kind
{
    D_ARG_INTERNAL_COMPLETE_NONE        = 0,
    D_ARG_INTERNAL_COMPLETE_CHOICES     = 1,
    D_ARG_INTERNAL_COMPLETE_FILES       = 2,
    D_ARG_INTERNAL_COMPLETE_DIRECTORIES = 3,
    D_ARG_INTERNAL_COMPLETE_NOTHING     = 4
};

// 11.1.2
// D_INTERNAL_ARG_COMPLETE_UNLIMITED
//   constant: a count of values with no limit, as the scripts write it.
#define D_INTERNAL_ARG_COMPLETE_UNLIMITED (-1)

// 11.1.3
// d_arg_internal_complete_command
//   struct: a command completion knows -- the root, or a subcommand help
// shows: the command; its parent's index, the root's own (0) for the root;
// the most subcommands it takes, 0 for any number; where its subcommands'
// names and its options' names begin in the model, and how many there are;
// and its switches as the scripts spell them, NUL-terminated: `s` for
// subcommand fallthrough, `f` for fallthrough, `w` for Windows-style
// options, `e` for extra arguments kept, or `-` alone for none.
struct d_arg_internal_complete_command
{
    const struct d_arg_command* command;
    size_t                      parent;
    size_t                      most;
    size_t                      first_name;
    size_t                      names;
    size_t                      first_entry;
    size_t                      entries;
    char                        flags[5];
};

// 11.1.4
// d_arg_internal_complete_name
//   struct: a name or an alias of a subcommand of the command at `command`:
// the subcommand's index, and the first line of its description.
struct d_arg_internal_complete_name
{
    size_t             command;
    size_t             target;
    const char*        name;
    struct d_pack_text summary;
};

// 11.1.5
// d_arg_internal_complete_option
//   struct: an option of the command at `command`, or of an option group it
// holds: the fewest and the most values a name of it takes, the most 0 for
// a flag and D_INTERNAL_ARG_COMPLETE_UNLIMITED for no limit; the most values
// it takes as a positional, the same way, 0 when it is not one; a
// d_arg_internal_complete_kind value; its choices in the model's list; and
// the first line of its description.
struct d_arg_internal_complete_option
{
    const struct d_arg_option* option;
    size_t                     command;
    int                        least;
    int                        most;
    int                        slots;
    int                        kind;
    size_t                     first_choice;
    size_t                     choices;
    struct d_pack_text         summary;
};

// 11.1.6
// d_arg_internal_complete_entry
//   struct: one name of an option: the option's index, the name without its
// dashes, and whether it is a long name.
struct d_arg_internal_complete_entry
{
    size_t      option;
    const char* name;
    bool        long_name;
};

// 11.1.7
// d_arg_internal_model
//   struct: the model shell completion writes its scripts from: the
// commands completion knows, in breadth-first order from the root; their
// subcommands' names, their options and their options' names, each
// command's together and in the order the parse searches them; and the
// options' choices, copied. The rest borrows from the command tree, which
// must not change while the model is in use.
struct d_arg_internal_model
{
    struct d_arg_internal_complete_command* commands;
    size_t                                  command_count;
    size_t                                  command_capacity;
    struct d_arg_internal_complete_name*    names;
    size_t                                  name_count;
    size_t                                  name_capacity;
    struct d_arg_internal_complete_option*  options;
    size_t                                  option_count;
    size_t                                  option_capacity;
    struct d_arg_internal_complete_entry*   entries;
    size_t                                  entry_count;
    size_t                                  entry_capacity;
    struct d_arg_string_list                choices;
};

// D_INTERNAL_ARG_MODEL_INIT
//   constant: the initializer of an empty model, which owns nothing.
#define D_INTERNAL_ARG_MODEL_INIT                                              \
    { NULL, 0u, 0u, NULL, 0u, 0u, NULL, 0u, 0u, NULL, 0u, 0u, D_ARG_LIST_INIT }

// 11.1.8
// d_arg_internal_complete_build / d_arg_internal_complete_release
/**
 * @brief Builds the model of a command and what lies below it.
 *
 * @note A subcommand or an option group that is disabled, or in the empty
 *       group, is left out with all it holds; so is an option in the empty
 *       group, but for a positional, which is kept -- completing as nothing
 *       -- so that the positionals after it are counted rightly.
 *
 * @param[out] _model  the model, whatever it held ignored.
 * @param[in]  _root   the command.
 * @return D_ARG_OK; D_ARG_ERROR_OUT_OF_MEMORY, the model then empty; or the
 *         status of a validator's listing that failed, likewise.
 */
int  d_arg_internal_complete_build(struct d_arg_internal_model* _model,
                                   const struct d_arg_command*  _root);
/**
 * @brief Frees what a model holds, leaving it empty; accepts NULL.
 *
 * @param[in,out] _model  the model.
 */
void d_arg_internal_complete_release(struct d_arg_internal_model* _model);

// 11.2   Writing
//------------------------------------------------------------------------------
// 11.2.1
// d_arg_internal_complete_ident
//   text: the program's name as an identifier, appended: ASCII letters and
// digits as they are, `_` doubled, and any other byte `_` and two
// hexadecimal digits -- so that two programs never share one. Returns the
// buffer's status.
int  d_arg_internal_complete_ident(struct d_pack_text   _program,
                                   struct d_arg_buffer* _out);

// 11.2.2
// d_arg_internal_complete_lines
//   text: lines of a script, each followed by a newline, with every `@@` in
// them replaced by `_ident`, appended. Returns the buffer's status.
int  d_arg_internal_complete_lines(const char* const*   _lines,
                                   size_t               _count,
                                   struct d_pack_text   _ident,
                                   struct d_arg_buffer* _out);

// 11.2.3
// d_arg_internal_complete_line_text
//   view: the first line of a description, without the spaces around it --
// what a script shows beside a name; empty for NULL.
struct d_pack_text d_arg_internal_complete_line_text(const char* _text);

// 11.2.4
// d_arg_internal_complete_bash and kin
/**
 * @brief Writes a model's completion script for one shell.
 *
 * @param[in]     _model    the model.
 * @param[in]     _program  the name the shell runs the program by; not
 *                          empty.
 * @param[in,out] _out      receives the script, appended.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY when the buffer could not
 *         grow.
 */
int  d_arg_internal_complete_bash(const struct d_arg_internal_model* _model,
                                  struct d_pack_text                 _program,
                                  struct d_arg_buffer*               _out);
int  d_arg_internal_complete_zsh(const struct d_arg_internal_model* _model,
                                 struct d_pack_text                 _program,
                                 struct d_arg_buffer*               _out);
int  d_arg_internal_complete_fish(const struct d_arg_internal_model* _model,
                                  struct d_pack_text                 _program,
                                  struct d_arg_buffer*               _out);
int  d_arg_internal_complete_pwsh(const struct d_arg_internal_model* _model,
                                  struct d_pack_text                 _program,
                                  struct d_arg_buffer*               _out);

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION


D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_INTERNAL_H
