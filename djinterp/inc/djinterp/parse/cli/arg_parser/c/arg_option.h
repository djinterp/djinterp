/*******************************************************************************
* djinterp [parse]                                                  arg_option.h
*
* Options: the named, flag and positional arguments a command accepts.
*   A `struct d_arg_option` is CLI11's Option in C: its names (short, long,
* positional, and the flag names that carry default values), the text help
* shows for it, how many values it takes and how they group, what happens
* when it is given too many, its validators, the options it needs and
* excludes, its callback or bound variable, and the values a parse gave it.
* Options are made by a command (arg_command.h), which owns them; one lives
* until its command removes it or is released.
*   Every setter accepts NULL and then does nothing, so a call chained onto
* an option whose creation failed is harmless; a setter that can fail
* records the failure in its parser's error report as a definition error,
* which the parser's next parse reports, as CLI11 throws it at once.
*   Values are spans (struct d_pack_text), NUL-terminated, that stay valid
* until the option is cleared -- by the next parse, d_arg_option_clear, or
* the parser's clear or release. They borrow `argv` where a value is a whole
* argument or its tail, and are the option's own copies otherwise; the
* copies a default value made outlive clearing, so that a variable bound to
* text, which points at them, stays valid until the option is released.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_option.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Option flags
         1.  D_ARG_OPTION_REQUIRED
         2.  D_ARG_OPTION_IGNORE_CASE
         3.  D_ARG_OPTION_IGNORE_UNDERSCORE
         4.  D_ARG_OPTION_CONFIGURABLE
         5.  D_ARG_OPTION_DISABLE_FLAG_OVERRIDE
         6.  D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT
         7.  D_ARG_OPTION_ALLOW_EXTRA_ARGS
         8.  D_ARG_OPTION_FLAG_LIKE
         9.  D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT
         10. D_ARG_OPTION_INJECT_SEPARATOR
         11. D_ARG_OPTION_TRIGGER_ON_PARSE
         12. D_ARG_OPTION_FORCE_CALLBACK
    2.  States
         1.  d_arg_option_state
    3.  Name forms
         1.  D_ARG_NAME_POSITIONAL
         2.  D_ARG_NAME_ALL
         3.  D_ARG_NAME_NO_FLAG_VALUES
2.  RECORDS
    -------
    1.  Option defaults
         1.  d_arg_option_defaults
    2.  The option
         1.  d_arg_option
3.  NAMES
    -----
    1.  Display
         1.  d_arg_option_name
         2.  d_arg_option_single_name
    2.  Matching
         1.  d_arg_option_check_name and kin
         2.  d_arg_option_matching_name
4.  DEFINITION
    ----------
    1.  Text
         1.  d_arg_option_set_description and kin
         2.  d_arg_option_set_type_name_fn
    2.  Counts
         1.  d_arg_option_set_expected
         2.  d_arg_option_set_expected_range
         3.  d_arg_option_set_type_size
         4.  d_arg_option_set_type_size_range
    3.  Policies
         1.  d_arg_option_set_multi_option_policy
         2.  d_arg_option_set_delimiter and kin
    4.  Switches
         1.  d_arg_option_set_required and kin
         2.  d_arg_option_set_ignore_case / d_arg_option_set_ignore_underscore
    5.  Defaults
         1.  d_arg_option_set_default_text
         2.  d_arg_option_set_default_value
         3.  d_arg_option_set_default_typed
         4.  d_arg_option_set_default_fn
         5.  d_arg_option_capture_default
    6.  Relations
         1.  d_arg_option_needs / d_arg_option_excludes
         2.  d_arg_option_needs_name / d_arg_option_excludes_name
         3.  d_arg_option_remove_needs / d_arg_option_remove_excludes
    7.  Validators
         1.  d_arg_option_check
         2.  d_arg_option_transform
         3.  d_arg_option_each
         4.  d_arg_option_validator / d_arg_option_validator_at
    8.  Callbacks
         1.  d_arg_option_set_callback
         2.  d_arg_option_bind
5.  QUERIES
    -------
    1.  Fields
         1.  d_arg_option_description and kin
         2.  d_arg_option_has_flag
         3.  d_arg_option_items_expected_min / d_arg_option_items_expected_max
         4.  d_arg_option_is_positional / d_arg_option_is_nonpositional
    2.  Composed text
         1.  d_arg_option_type_name
6.  RESULTS
    -------
    1.  Counting
         1.  d_arg_option_count / d_arg_option_is_given
    2.  Reading
         1.  d_arg_option_results / d_arg_option_values
         2.  d_arg_option_reduce
         3.  d_arg_option_as
         4.  d_arg_option_callback_run
    3.  Writing
         1.  d_arg_option_add_result / d_arg_option_add_results
         2.  d_arg_option_clear
         3.  d_arg_option_run_callback
    4.  Flags
         1.  d_arg_option_flag_value
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_OPTION_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_OPTION_H 1

// std
#include <stddef.h>                          // size_t
// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "../../../../c/util/sink_common.h"  // d_pack_text
#include "./arg_common.h"                    // lists, callbacks, statuses
#include "./arg_type.h"                      // d_arg_type
#include "./arg_validator.h"                 // d_arg_validator
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Option flags
//------------------------------------------------------------------------------
//   constant: the switches of d_arg_option's `flags`, CLI11's Option booleans.
// The first six are also d_arg_option_defaults' `flags`, which a new option
// starts from.

// 1.1.1
// D_ARG_OPTION_REQUIRED
//   constant: the option must be given; CLI11's required().
#define D_ARG_OPTION_REQUIRED                 0x00000001u

// 1.1.2
// D_ARG_OPTION_IGNORE_CASE
//   constant: names match without regard to ASCII case.
#define D_ARG_OPTION_IGNORE_CASE              0x00000002u

// 1.1.3
// D_ARG_OPTION_IGNORE_UNDERSCORE
//   constant: names match with `_` removed.
#define D_ARG_OPTION_IGNORE_UNDERSCORE        0x00000004u

// 1.1.4
// D_ARG_OPTION_CONFIGURABLE
//   constant: the option may be set from a configuration file. Set by
// default.
#define D_ARG_OPTION_CONFIGURABLE             0x00000008u

// 1.1.5
// D_ARG_OPTION_DISABLE_FLAG_OVERRIDE
//   constant: a flag refuses a value other than its default value
// (`--flag=other`).
#define D_ARG_OPTION_DISABLE_FLAG_OVERRIDE    0x00000010u

// 1.1.6
// D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT
//   constant: the default text is captured from the bound variable as the
// option is made, as if d_arg_option_capture_default were called.
#define D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT   0x00000020u

// 1.1.7
// D_ARG_OPTION_ALLOW_EXTRA_ARGS
//   constant: the option keeps taking values past its maximum, as long as
// they are not options.
#define D_ARG_OPTION_ALLOW_EXTRA_ARGS         0x00000040u

// 1.1.8
// D_ARG_OPTION_FLAG_LIKE
//   constant: the option expects no values, so it reads as a flag.
#define D_ARG_OPTION_FLAG_LIKE                0x00000080u

// 1.1.9
// D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT
//   constant: setting a default value runs the callback with it, so a bound
// variable takes the default at once.
#define D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT 0x00000100u

// 1.1.10
// D_ARG_OPTION_INJECT_SEPARATOR
//   constant: each use of the option starts a new group of values, marked by
// an empty value, for a container of containers.
#define D_ARG_OPTION_INJECT_SEPARATOR         0x00000200u

// 1.1.11
// D_ARG_OPTION_TRIGGER_ON_PARSE
//   constant: the callback runs each time the option is met, rather than once
// after parsing.
#define D_ARG_OPTION_TRIGGER_ON_PARSE         0x00000400u

// 1.1.12
// D_ARG_OPTION_FORCE_CALLBACK
//   constant: the callback runs even when the option was not given, with its
// default text.
#define D_ARG_OPTION_FORCE_CALLBACK           0x00000800u

// 1.2    States
//------------------------------------------------------------------------------
// 1.2.1
// d_arg_option_state
//   enum: how far an option's values have been processed; CLI11's
// option_state, value for value. Adding a value returns it to PARSING.
enum d_arg_option_state
{
    D_ARG_OPTION_STATE_PARSING      = 0,
    D_ARG_OPTION_STATE_VALIDATED    = 2,
    D_ARG_OPTION_STATE_REDUCED      = 4,
    D_ARG_OPTION_STATE_CALLBACK_RUN = 6
};

// 1.3    Name forms
//------------------------------------------------------------------------------
//   constant: the forms of d_arg_option_name, CLI11's get_name arguments.

// 1.3.1
// D_ARG_NAME_POSITIONAL
//   constant: the positional name, whatever other names there are.
#define D_ARG_NAME_POSITIONAL      0x0001u

// 1.3.2
// D_ARG_NAME_ALL
//   constant: every name, comma-separated, as the names were given.
#define D_ARG_NAME_ALL             0x0002u

// 1.3.3
// D_ARG_NAME_NO_FLAG_VALUES
//   constant: with D_ARG_NAME_ALL, no `{value}` after a flag name.
#define D_ARG_NAME_NO_FLAG_VALUES  0x0004u


//==============================================================================
// 2.  RECORDS
//==============================================================================


// 2.1    Option defaults
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_option_defaults
//   struct: what a command's new options start from; CLI11's OptionDefaults,
// with the integer rule beside it. `group` is owned, NULL for "OPTIONS";
// `flags` holds the first six option flags, D_ARG_OPTION_CONFIGURABLE by
// default. A subcommand copies its parent's when it is made.
struct d_arg_option_defaults
{
    char*    group;
    uint32_t flags;
    int      multi_option_policy;
    int      callback_priority;
    int      integer_rule;
    char     delimiter;
};

// 2.2    The option
//------------------------------------------------------------------------------
// 2.2.1
// d_arg_option
//   struct: one option, owned by its `parent`, the command in whose
// `options` it is -- an option group, once moved into one. The name lists
// hold names without their dashes; `flag_names` and `flag_values` are
// parallel, a flag name and the value it stands for (`--no-x{false}`, or
// `!--x` for "false"). Every string is owned; `group` NULL means "OPTIONS";
// the other text fields are NULL when unset. `type` and `target` are the
// bound variable, if any. `results` are the values as given, `processed`
// what validation and the multi-option policy made of them, and `owned` the
// copies either list refers to: the first `kept` of them made for a default
// value, which clearing leaves.
//   Public-payload layout: read any field; change one only through the calls
// below, which keep the lists, the counts and the state consistent.
struct d_arg_option
{
    struct d_arg_command*       parent;
    struct d_arg_string_list    short_names;
    struct d_arg_string_list    long_names;
    struct d_arg_string_list    flag_names;
    struct d_arg_string_list    flag_values;
    char*                       positional_name;
    char*                       env_name;
    char*                       description;
    char*                       default_text;
    char*                       option_text;
    char*                       group;
    char*                       type_name;
    d_arg_option_text_fn        type_name_fn;
    void*                       type_name_context;
    d_arg_release_fn            type_name_release;
    d_arg_option_text_fn        default_fn;
    void*                       default_context;
    d_arg_release_fn            default_release;
    d_arg_results_fn            callback;
    void*                       callback_context;
    d_arg_release_fn            callback_release;
    const struct d_arg_type*    type;
    void*                       target;
    struct d_arg_validator_list validators;
    struct d_arg_option_list    needs;
    struct d_arg_option_list    excludes;
    struct d_arg_text_list      results;
    struct d_arg_text_list      processed;
    struct d_arg_string_list    owned;
    size_t                      kept;
    int                         type_size_min;
    int                         type_size_max;
    int                         expected_min;
    int                         expected_max;
    int                         multi_option_policy;
    int                         callback_priority;
    int                         integer_rule;
    int                         state;
    uint32_t                    flags;
    char                        delimiter;
};


D_EXTERN_C_BEGIN

//==============================================================================
// 3.  NAMES
//==============================================================================


// 3.1    Display
//------------------------------------------------------------------------------
// 3.1.1
// d_arg_option_name
/**
 * @brief Writes an option's name as CLI11's get_name writes it.
 *
 * @note With no form, the first long name with `--`, else the first short
 *       name with `-`, else the positional name. D_ARG_NAME_POSITIONAL
 *       writes the positional name. D_ARG_NAME_ALL writes every name joined
 *       by commas -- the positional name only when asked for or alone --
 *       and, for an option expecting no values, each flag name's value as
 *       `{value}` unless D_ARG_NAME_NO_FLAG_VALUES. An option in the empty
 *       group, hidden from help, writes nothing.
 *
 * @param[in]     _option  the option.
 * @param[in]     _form    D_ARG_NAME_* flags; 0 for the plain name.
 * @param[in,out] _out     receives the name, appended.
 * @return D_ARG_OK, or the buffer's status; D_ARG_ERROR_BAD_ARGUMENT for a
 *         NULL argument.
 */
int         d_arg_option_name(const struct d_arg_option* _option,
                              uint32_t                   _form,
                              struct d_arg_buffer*       _out);

// 3.1.2
// d_arg_option_single_name
//   lookup: CLI11's get_single_name -- the first long name, else the first
// short name, else the positional name, else the environment variable's
// name, all without dashes; "" when there is none or `_option` is NULL.
const char* d_arg_option_single_name(const struct d_arg_option* _option);

// 3.2    Matching
//------------------------------------------------------------------------------
// 3.2.1
// d_arg_option_check_name and kin
//   predicates: CLI11's check_name, which reads `--x` as a long name, `-x` as
// a short name and anything else as the positional name or, exactly, the
// environment variable's; and check_sname, check_lname and check_fname,
// which take a name without dashes. Each honours the option's ignore-case
// and ignore-underscore switches, and is false for NULL.
bool        d_arg_option_check_name(const struct d_arg_option* _option,
                                    struct d_pack_text         _name);
bool        d_arg_option_check_short_name(const struct d_arg_option* _option,
                                          struct d_pack_text         _name);
bool        d_arg_option_check_long_name(const struct d_arg_option* _option,
                                         struct d_pack_text         _name);
bool        d_arg_option_check_flag_name(const struct d_arg_option* _option,
                                         struct d_pack_text         _name);

// 3.2.2
// d_arg_option_matching_name
/**
 * @brief Finds a name two options share; CLI11's matching_name.
 *
 * @note Short and long names collide with each other only when both options
 *       are configurable, as they would in a configuration file, and a
 *       positional name collides with any name of a configurable option.
 *
 * @param[in] _option  one option.
 * @param[in] _other   the other.
 * @return the shared name, without dashes, owned by one of the options; NULL
 *         when they share none or an argument is NULL.
 */
const char* d_arg_option_matching_name(const struct d_arg_option* _option,
                                       const struct d_arg_option* _other);


//==============================================================================
// 4.  DEFINITION
//==============================================================================
// Each setter takes the option first and ignores NULL. Those that return a
// status return D_ARG_OK, D_ARG_ERROR_OUT_OF_MEMORY when a copy could not be
// made, or the construction error CLI11 would throw, recorded in the
// parser's error report; D_ARG_ERROR_BAD_ARGUMENT for a NULL option is not
// recorded.


// 4.1    Text
//------------------------------------------------------------------------------
// 4.1.1
// d_arg_option_set_description and kin
//   settings: CLI11's description(), option_text() -- which replaces the type
// name, default and requirements in help -- group(), which hides the
// option from help when empty and refuses a newline with
// D_ARG_ERROR_INCORRECT_CONSTRUCTION, type_name(), and envname(). Each
// copies its text; NULL clears it, and for the group restores "OPTIONS".
int         d_arg_option_set_description(struct d_arg_option* _option,
                                         const char*          _description);
int         d_arg_option_set_option_text(struct d_arg_option* _option,
                                         const char*          _text);
int         d_arg_option_set_group(struct d_arg_option* _option,
                                   const char*          _group);
int         d_arg_option_set_type_name(struct d_arg_option* _option,
                                       const char*          _type_name);
int         d_arg_option_set_env_name(struct d_arg_option* _option,
                                      const char*          _env_name);

// 4.1.2
// d_arg_option_set_type_name_fn
/**
 * @brief Makes the type name be written when help asks for it; CLI11's
 *        type_name_fn().
 *
 * @param[in,out] _option   the option.
 * @param[in]     _write    writes the type name; NULL returns to the fixed
 *                          name, or the bound type's.
 * @param[in]     _context  `_write`'s context.
 * @param[in]     _release  frees `_context` when the option is released or
 *                          the function replaced; NULL leaves it borrowed.
 * @post   a context the option held before is released.
 */
void        d_arg_option_set_type_name_fn(struct d_arg_option* _option,
                                          d_arg_option_text_fn _write,
                                          void*                _context,
                                          d_arg_release_fn     _release);

// 4.2    Counts
//------------------------------------------------------------------------------
// 4.2.1
// d_arg_option_set_expected
/**
 * @brief Sets how many items the option takes each time it is given;
 *        CLI11's expected(int).
 *
 * @note A negative count `-n` means at least `n` and allows extra
 *       arguments; D_ARG_EXPECTED_UNLIMITED means at least one and allows
 *       extra arguments; any other count means exactly that many, and 0
 *       makes the option flag-like.
 *
 * @param[in,out] _option  the option.
 * @param[in]     _count   the count.
 */
void        d_arg_option_set_expected(struct d_arg_option* _option,
                                      int                  _count);

// 4.2.2
// d_arg_option_set_expected_range
//   setting: CLI11's expected(min, max): negative bounds count as their
// magnitude for the minimum and as unlimited for the maximum, and reversed
// bounds are swapped.
void        d_arg_option_set_expected_range(struct d_arg_option* _option,
                                            int                  _minimum,
                                            int                  _maximum);

// 4.2.3
// d_arg_option_set_type_size
//   setting: CLI11's type_size(int): how many values make one item. A
// negative size `-n` sets `n` and unlimited items, as CLI11 kept for
// compatibility; a size of D_ARG_EXPECTED_UNLIMITED or more makes items of
// any size, separated by empty values; a size of 0 also clears required.
void        d_arg_option_set_type_size(struct d_arg_option* _option,
                                       int                  _size);

// 4.2.4
// d_arg_option_set_type_size_range
//   setting: CLI11's type_size(min, max): items of between `_minimum` and
// `_maximum` values, with negative sizes and reversed bounds treated as in
// CLI11.
void        d_arg_option_set_type_size_range(struct d_arg_option* _option,
                                             int                  _minimum,
                                             int                  _maximum);

// 4.3    Policies
//------------------------------------------------------------------------------
// 4.3.1
// d_arg_option_set_multi_option_policy
//   setting: CLI11's multi_option_policy(): a d_arg_multi_option_policy
// value, which returns the values to PARSING so they are reduced again; a
// value outside the enum is ignored. Leaving the default policy on an
// option expecting at least two items and unlimited ones limits it to its
// minimum, as CLI11 does for compatibility.
void        d_arg_option_set_multi_option_policy(struct d_arg_option* _option,
                                                 int                  _policy);

// 4.3.2
// d_arg_option_set_delimiter and kin
//   settings: CLI11's delimiter(), which splits each value at a character
// ('\0' for none) and is what D_ARG_MULTI_JOIN joins with; callback_priority(),
// a d_arg_callback_priority value; and the integer rule the option's
// values convert by, a d_arg_integer_rule value. A value outside its enum
// is ignored.
void        d_arg_option_set_delimiter(struct d_arg_option* _option,
                                       char                 _delimiter);
void        d_arg_option_set_callback_priority(struct d_arg_option* _option,
                                               int                  _priority);
void        d_arg_option_set_integer_rule(struct d_arg_option* _option,
                                          int                  _rule);

// 4.4    Switches
//------------------------------------------------------------------------------
// 4.4.1
// d_arg_option_set_required and kin
//   settings: set or clear one D_ARG_OPTION_* flag -- required (CLI11's
// required() and mandatory()), configurable, disable_flag_override,
// always_capture_default, allow_extra_args, trigger_on_parse,
// force_callback, run_callback_for_default, inject_separator. None fails.
void    d_arg_option_set_required(struct d_arg_option* _option,
                                  bool                 _value);
void    d_arg_option_set_configurable(struct d_arg_option* _option,
                                      bool                 _value);
void    d_arg_option_set_disable_flag_override(struct d_arg_option* _option,
                                               bool                 _value);
void    d_arg_option_set_always_capture_default(struct d_arg_option* _option,
                                                bool                 _value);
void    d_arg_option_set_allow_extra_args(struct d_arg_option* _option,
                                          bool                 _value);
void    d_arg_option_set_trigger_on_parse(struct d_arg_option* _option,
                                          bool                 _value);
void    d_arg_option_set_force_callback(struct d_arg_option* _option,
                                        bool                 _value);
void    d_arg_option_set_run_callback_for_default(struct d_arg_option* _option,
                                                  bool                 _value);
void    d_arg_option_set_inject_separator(struct d_arg_option* _option,
                                          bool                 _value);

// 4.4.2
// d_arg_option_set_ignore_case / d_arg_option_set_ignore_underscore
/**
 * @brief Makes an option's names match without regard to case; CLI11's
 *        ignore_case().
 *
 * @note set_ignore_underscore is the same for `_`, with "adding ignore
 *       underscore caused a name conflict with <name>".
 *
 * @param[in,out] _option  the option.
 * @param[in]     _value   whether case is ignored.
 * @retval D_ARG_OK                          set.
 * @retval D_ARG_ERROR_OPTION_ALREADY_ADDED  ignoring case would make a name
 *                                           of the option match another
 *                                           option's: "adding ignore case
 *                                           caused a name conflict with
 *                                           <name>"; the setting is kept
 *                                           off.
 * @retval D_ARG_ERROR_BAD_ARGUMENT          `_option` is NULL.
 */
int         d_arg_option_set_ignore_case(struct d_arg_option* _option,
                                         bool                 _value);
int         d_arg_option_set_ignore_underscore(struct d_arg_option* _option,
                                               bool                 _value);

// 4.5    Defaults
//------------------------------------------------------------------------------
// 4.5.1
// d_arg_option_set_default_text
//   setting: CLI11's default_str(): the text help shows as the default, and
// the value the option takes when forced or asked for with no results.
// Copied; NULL clears it. Neither validates nor converts it.
int         d_arg_option_set_default_text(struct d_arg_option* _option,
                                          const char*          _text);

// 4.5.2
// d_arg_option_set_default_value
/**
 * @brief Sets a default that the option checks as it would a value;
 *        CLI11's default_val().
 *
 * @note The text runs through the option's validators -- and, with
 *       D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT, through its callback, so a
 *       bound variable takes it -- and becomes the default text only when
 *       accepted. The option's values are kept as they were either way.
 *
 * @param[in,out] _option  the option.
 * @param[in]     _text    the default as text.
 * @retval D_ARG_OK                   set.
 * @retval D_ARG_ERROR_CONVERSION     the callback refused it: "The value
 *                                    <name> is not an allowed value for
 *                                    given default value("<text>")
 *                                    produces an error : <reason>", as
 *                                    CLI11 words it.
 * @retval D_ARG_ERROR_VALIDATION     a validator refused it.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  a copy could not be made.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   an argument is NULL.
 */
int         d_arg_option_set_default_value(struct d_arg_option* _option,
                                           const char*          _text);

// 4.5.3
// d_arg_option_set_default_typed
/**
 * @brief Sets a default given as a value of a type; CLI11's default_val()
 *        given a value, as d_arg_option_bind and d_arg_option_as take one.
 *
 * @note The value is written as its type writes it (d_arg_format), then
 *       set as d_arg_option_set_default_value sets text. A value that could
 *       not be written is the parser's definition error, as CLI11's
 *       default_val() throws where the default is set; the default is then
 *       kept as it was.
 *
 * @param[in,out] _option  the option.
 * @param[in]     _type    the value's type.
 * @param[in]     _value   the value, read as `_type` reads one.
 * @return D_ARG_OK; the status of the type's writing --
 *         D_ARG_ERROR_UNSUPPORTED for a type that writes no value,
 *         D_ARG_ERROR_OUT_OF_MEMORY for text that could not be built -- or
 *         of d_arg_option_set_default_value; D_ARG_ERROR_BAD_ARGUMENT for a
 *         NULL argument.
 */
int         d_arg_option_set_default_typed(struct d_arg_option*     _option,
                                           const struct d_arg_type* _type,
                                           const void*              _value);

// 4.5.4
// d_arg_option_set_default_fn
/**
 * @brief Sets how the default text is captured; CLI11's default_function().
 *
 * @param[in,out] _option   the option.
 * @param[in]     _write    writes the default; NULL removes the function,
 *                          so that capturing changes nothing.
 * @param[in]     _context  `_write`'s context.
 * @param[in]     _release  frees `_context` with the option; NULL leaves it
 *                          borrowed.
 * @post   a context the option held before is released.
 */
void        d_arg_option_set_default_fn(struct d_arg_option* _option,
                                        d_arg_option_text_fn _write,
                                        void*                _context,
                                        d_arg_release_fn     _release);

// 4.5.5
// d_arg_option_capture_default
//   operation: CLI11's capture_default_str(): replaces the default text with
// what the default function writes -- for a variable bound by
// d_arg_option_bind, its current value as its type writes it. Does nothing
// without a function. Returns D_ARG_OK, or the status of a write or copy
// that failed.
int         d_arg_option_capture_default(struct d_arg_option* _option);

// 4.6    Relations
//------------------------------------------------------------------------------
// 4.6.1
// d_arg_option_needs / d_arg_option_excludes
/**
 * @brief Makes an option require another whenever it is given; CLI11's
 *        needs().
 *
 * @note An option needing itself is ignored, as in CLI11. excludes is the
 *       same for an option that may not be given with it, recorded on both;
 *       excluding itself is D_ARG_ERROR_INCORRECT_CONSTRUCTION, "and option
 *       cannot exclude itself".
 *
 * @param[in,out] _option  the option.
 * @param[in]     _other   the option it needs.
 * @retval D_ARG_OK                            recorded, or already recorded.
 * @retval D_ARG_ERROR_INCORRECT_CONSTRUCTION  the options belong to two
 *                                             parsers: "options of different
 *                                             parsers cannot be linked".
 * @retval D_ARG_ERROR_OUT_OF_MEMORY           the list could not grow.
 * @retval D_ARG_ERROR_BAD_ARGUMENT            an argument is NULL.
 */
int         d_arg_option_needs(struct d_arg_option* _option,
                               struct d_arg_option* _other);
int         d_arg_option_excludes(struct d_arg_option* _option,
                                  struct d_arg_option* _other);

// 4.6.2
// d_arg_option_needs_name / d_arg_option_excludes_name
//   relations by name: as above, the other option found from the option's
// command as d_arg_command_option finds it -- and, for an option in an option
// group, then from the command the group belongs to, where CLI11 finds one
// moved into the group; a name nothing matches is
// D_ARG_ERROR_INCORRECT_CONSTRUCTION, "Option <name> is not defined".
int         d_arg_option_needs_name(struct d_arg_option* _option,
                                    const char*          _name);
int         d_arg_option_excludes_name(struct d_arg_option* _option,
                                       const char*          _name);

// 4.6.3
// d_arg_option_remove_needs / d_arg_option_remove_excludes
//   relations: undo a needs or an excludes -- only this option's half of an
// exclusion, as in CLI11. Each returns whether there was one to undo.
bool        d_arg_option_remove_needs(struct d_arg_option*       _option,
                                      const struct d_arg_option* _other);
bool        d_arg_option_remove_excludes(struct d_arg_option*       _option,
                                         const struct d_arg_option* _other);

// 4.7    Validators
//------------------------------------------------------------------------------
// 4.7.1
// d_arg_option_check
/**
 * @brief Adds a validator that judges values without changing them;
 *        CLI11's check().
 *
 * @param[in,out] _option     the option.
 * @param[in]     _validator  the validator, which is consumed: the option
 *                            owns it, made non-modifying, after the
 *                            validators already added.
 * @param[in]     _name       a name to find it by, copied; NULL keeps its
 *                            own.
 * @return D_ARG_OK when added; D_ARG_ERROR_OUT_OF_MEMORY when the list could
 *         not grow or the name could not be copied; a failed validator's
 *         own status, recorded as a definition error; or
 *         D_ARG_ERROR_BAD_ARGUMENT for a NULL option.
 */
int         d_arg_option_check(struct d_arg_option*   _option,
                               struct d_arg_validator _validator,
                               const char*            _name);

// 4.7.2
// d_arg_option_transform
/**
 * @brief Adds a validator whose changes replace the values; CLI11's
 *        transform().
 *
 * @note Transformations run before the validators already added, as CLI11
 *       inserts them first.
 *
 * @param[in,out] _option       the option.
 * @param[in]     _validator    the validator, which is consumed.
 * @param[in]     _name         a name to find it by, copied; NULL keeps its
 *                              own.
 * @param[in]     _description  a description for help, copied; NULL keeps
 *                              its own.
 * @return a status, as d_arg_option_check's.
 */
int         d_arg_option_transform(struct d_arg_option*   _option,
                                   struct d_arg_validator _validator,
                                   const char*            _name,
                                   const char*            _description);

// 4.7.3
// d_arg_option_each
/**
 * @brief Adds a function run on each value as it is validated; CLI11's
 *        each().
 *
 * @param[in,out] _option   the option.
 * @param[in]     _each     the function.
 * @param[in]     _context  its context.
 * @param[in]     _release  frees `_context` with the option; NULL leaves it
 *                          borrowed.
 * @return a status, as d_arg_option_check's.
 */
int         d_arg_option_each(struct d_arg_option* _option,
                              d_arg_each_fn        _each,
                              void*                _context,
                              d_arg_release_fn     _release);

// 4.7.4
// d_arg_option_validator / d_arg_option_validator_at
//   lookups: CLI11's get_validator(name) -- the first validator so named, a
// NULL or empty name matching an unnamed one, else, for a NULL or empty
// name, the first validator -- and get_validator(index). NULL when there is
// none, which CLI11 throws as OptionNotFound; the validator stays the
// option's.
struct d_arg_validator* d_arg_option_validator(struct d_arg_option* _option,
                                               const char*          _name);
struct d_arg_validator* d_arg_option_validator_at(struct d_arg_option* _option,
                                                  size_t               _index);

// 4.8    Callbacks
//------------------------------------------------------------------------------
// 4.8.1
// d_arg_option_set_callback
/**
 * @brief Sets the function the option's values are given to; CLI11's
 *        callback_t.
 *
 * @param[in,out] _option    the option.
 * @param[in]     _callback  the function; NULL removes it.
 * @param[in]     _context   its context.
 * @param[in]     _release   frees `_context` with the option or when the
 *                           callback is replaced; NULL leaves it borrowed.
 * @post   a context the option held before is released.
 */
void        d_arg_option_set_callback(struct d_arg_option* _option,
                                      d_arg_results_fn     _callback,
                                      void*                _context,
                                      d_arg_release_fn     _release);

// 4.8.2
// d_arg_option_bind
/**
 * @brief Binds a variable the option's values convert into; what CLI11's
 *        add_option(name, variable) does after making the option.
 *
 * @note Takes the type's name for help, its sizes and expected count, and
 *       sets D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT; the variable becomes the
 *       default function, replacing any, so that its value is the default
 *       whenever the default is captured. The conversion runs before any
 *       callback set with d_arg_option_set_callback. Unbinding drops the
 *       variable as the default function.
 *
 * @param[in,out] _option  the option.
 * @param[in]     _target  the variable, which must outlive the option's
 *                         use; NULL unbinds.
 * @param[in]     _type    the variable's type, which must outlive the
 *                         option.
 * @retval D_ARG_OK                  bound.
 * @retval D_ARG_ERROR_BAD_ARGUMENT  `_option` is NULL, or `_target` is set
 *                                   and `_type` is NULL or cannot convert.
 */
int         d_arg_option_bind(struct d_arg_option*     _option,
                              void*                    _target,
                              const struct d_arg_type* _type);


//==============================================================================
// 5.  QUERIES
//==============================================================================


// 5.1    Fields
//------------------------------------------------------------------------------
// 5.1.1
// d_arg_option_description and kin
//   lookups: the option's text fields, "" when unset (and "OPTIONS" for an
// unset group), never NULL; NULL reads as an option with nothing set.
const char* d_arg_option_description(const struct d_arg_option* _option);
const char* d_arg_option_option_text(const struct d_arg_option* _option);
const char* d_arg_option_group(const struct d_arg_option* _option);
const char* d_arg_option_default_text(const struct d_arg_option* _option);
const char* d_arg_option_env_name(const struct d_arg_option* _option);
const char* d_arg_option_positional_name(const struct d_arg_option* _option);

// 5.1.2
// d_arg_option_has_flag
/**
 * @brief Reads one of an option's D_ARG_OPTION_* switches.
 *
 * @param[in] _option  the option.
 * @param[in] _flag    a D_ARG_OPTION_* constant.
 * @return whether the switch is set; `false` for NULL.
 */
D_INLINE bool
d_arg_option_has_flag(
    const struct d_arg_option* _option,
    uint32_t                   _flag
)
{
    return ( (_option != NULL) &&
             ((_option->flags & _flag) != 0u) );
}

// 5.1.3
// d_arg_option_items_expected_min / d_arg_option_items_expected_max
//   arithmetic: CLI11's get_items_expected_min and _max, the values -- not
// items -- the option takes: the type size times the expected count, held
// at D_ARG_EXPECTED_UNLIMITED where the product would overflow. 0 for NULL.
int         d_arg_option_items_expected_min(const struct d_arg_option* _option);
int         d_arg_option_items_expected_max(const struct d_arg_option* _option);

// 5.1.4
// d_arg_option_is_positional / d_arg_option_is_nonpositional
//   predicates: CLI11's get_positional (the option has a positional name)
// and nonpositional (it has a short or long name); an option can be both.
bool        d_arg_option_is_positional(const struct d_arg_option* _option);
bool        d_arg_option_is_nonpositional(const struct d_arg_option* _option);

// 5.2    Composed text
//------------------------------------------------------------------------------
// 5.2.1
// d_arg_option_type_name
/**
 * @brief Writes the type name help shows; CLI11's get_type_name.
 *
 * @note The written function's text, the fixed name or the bound type's,
 *       followed by `:` and each active validator's description.
 *
 * @param[in]     _option  the option.
 * @param[in,out] _out     receives the name, appended.
 * @return D_ARG_OK, or the status of the buffer or of a function that
 *         failed; D_ARG_ERROR_BAD_ARGUMENT for a NULL argument.
 */
int         d_arg_option_type_name(const struct d_arg_option* _option,
                                   struct d_arg_buffer*       _out);


//==============================================================================
// 6.  RESULTS
//==============================================================================


// 6.1    Counting
//------------------------------------------------------------------------------
// 6.1.1
// d_arg_option_count / d_arg_option_is_given
//   lookups: count is CLI11's count(), the number of values given, separators
// included; is_given is CLI11's operator bool, true when there is a value or
// the callback is forced. Both read NULL as an option not given.
size_t      d_arg_option_count(const struct d_arg_option* _option);
bool        d_arg_option_is_given(const struct d_arg_option* _option);

// 6.2    Reading
//------------------------------------------------------------------------------
// 6.2.1
// d_arg_option_results / d_arg_option_values
//   views: results are the values as given (CLI11's results()); values are
// what the multi-option policy kept, once reduced, else the values as
// given (CLI11's send_results). Each writes the count to `*_count` and
// returns the array, NULL when empty; valid until the option changes.
const struct d_pack_text*
            d_arg_option_results(const struct d_arg_option* _option,
                                 size_t*                    _count);
const struct d_pack_text*
            d_arg_option_values(const struct d_arg_option* _option,
                                size_t*                    _count);

// 6.2.2
// d_arg_option_reduce
/**
 * @brief Validates and reduces the option's values now, as its callback
 *        would.
 *
 * @param[in,out] _option  the option.
 * @post   on success the option is REDUCED or beyond, and
 *         d_arg_option_values returns what its policy kept.
 * @retval D_ARG_OK                       reduced.
 * @retval D_ARG_ERROR_VALIDATION         a validator refused a value:
 *                                        "<name>: <reason>".
 * @retval D_ARG_ERROR_ARGUMENT_MISMATCH  too few or too many values for the
 *                                        D_ARG_MULTI_THROW policy.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY      a value could not be stored.
 * @retval D_ARG_ERROR_BAD_ARGUMENT       `_option` is NULL.
 */
int         d_arg_option_reduce(struct d_arg_option* _option);

// 6.2.3
// d_arg_option_as
/**
 * @brief Converts the option's values into a variable of a type; CLI11's
 *        results(T&) and as<T>().
 *
 * @note Reduces the values first when they are not yet reduced; with no
 *       values, the default text is used, else a single empty value, which
 *       an arithmetic type reads as zero, as CLI11's value-initialization.
 *
 * @param[in,out] _option  the option, whose reduced values may be updated.
 * @param[in]     _type    the type to convert to.
 * @param[out]    _target  the variable.
 * @retval D_ARG_OK                converted.
 * @retval D_ARG_ERROR_CONVERSION  the values do not convert: "Could not
 *                                 convert: <name> = <values>".
 * @return a status of d_arg_option_reduce; D_ARG_ERROR_BAD_ARGUMENT for a
 *         NULL argument.
 */
int         d_arg_option_as(struct d_arg_option*     _option,
                            const struct d_arg_type* _type,
                            void*                    _target);

// 6.2.4
// d_arg_option_callback_run
//   predicate: CLI11's get_callback_run, whether the option's values have
// been given to its callback since they last changed.
bool        d_arg_option_callback_run(const struct d_arg_option* _option);

// 6.3    Writing
//------------------------------------------------------------------------------
// 6.3.1
// d_arg_option_add_result / d_arg_option_add_results
/**
 * @brief Adds a value as if the command line had given it; CLI11's
 *        add_result().
 *
 * @note The value is split as a given one is: at the option's delimiter,
 *       and, for an option taking several values, as a bracketed list
 *       `[a,b]`; `[[...]]` with every character doubled is the escape for a
 *       literal bracketed value. The option returns to PARSING.
 *
 * @param[in,out] _option  the option.
 * @param[in]     _value   the value, which is copied.
 * @param[out]    _added   receives how many values the split made; may be
 *                         NULL.
 * @retval D_ARG_OK                   added.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  a value could not be stored; the values
 *                                    added before it stay.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   `_option` is NULL.
 */
int         d_arg_option_add_result(struct d_arg_option* _option,
                                    struct d_pack_text   _value,
                                    int*                 _added);
/**
 * @brief Adds several values, each as d_arg_option_add_result.
 *
 * @param[in,out] _option  the option.
 * @param[in]     _values  the values; may be NULL when `_count` is 0.
 * @param[in]     _count   how many.
 * @return a status, as d_arg_option_add_result's.
 */
int         d_arg_option_add_results(struct d_arg_option*      _option,
                                     const struct d_pack_text* _values,
                                     size_t                    _count);

// 6.3.2
// d_arg_option_clear
//   operation: CLI11's clear(): drops the values and their copies -- all but
// those a default value made -- and returns the option to PARSING. Ignores
// NULL.
void        d_arg_option_clear(struct d_arg_option* _option);

// 6.3.3
// d_arg_option_run_callback
/**
 * @brief Validates, reduces, and hands the option's values to its bound
 *        variable and callback; CLI11's run_callback().
 *
 * @note A forced option with no values first takes its default text, and
 *       drops it again after its callback ran. An option whose values are
 *       empty after reduction runs no callback.
 *
 * @param[in,out] _option  the option.
 * @post   the option is CALLBACK_RUN.
 * @retval D_ARG_OK                converted and called back.
 * @retval D_ARG_ERROR_CONVERSION  the bound variable could not take the
 *                                 values, or the callback refused them with
 *                                 no message of its own: "Could not
 *                                 convert: <name> = <values>".
 * @return a status of d_arg_option_reduce, or whatever the callback
 *         returned; D_ARG_ERROR_BAD_ARGUMENT for NULL.
 */
int         d_arg_option_run_callback(struct d_arg_option* _option);

// 6.4    Flags
//------------------------------------------------------------------------------
// 6.4.1
// d_arg_option_flag_value
/**
 * @brief Writes the value a flag name given with an input stands for;
 *        CLI11's get_flag_value.
 *
 * @note With no input (or `{}`), a flag-like option gives `true`, or the
 *       name's own value; any other option gives its default text, or the
 *       name's value. With an input, a name whose value is `false` inverts
 *       the input -- `true` for 1, `false` for -1, the negation otherwise --
 *       and any other gives the input.
 *
 * @param[in]     _option  the option.
 * @param[in]     _name    the name the flag was given by, without dashes.
 * @param[in]     _input   the input after `=`, or empty.
 * @param[in,out] _out     receives the value, appended.
 * @retval D_ARG_OK                       written.
 * @retval D_ARG_ERROR_ARGUMENT_MISMATCH  the option disables flag
 *                                        overrides and the input is not the
 *                                        name's own value: "<name> was given
 *                                        a disallowed flag override".
 * @return the buffer's status; D_ARG_ERROR_BAD_ARGUMENT for a NULL
 *         argument.
 */
int         d_arg_option_flag_value(const struct d_arg_option* _option,
                                    struct d_pack_text         _name,
                                    struct d_pack_text         _input,
                                    struct d_arg_buffer*       _out);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_OPTION_H
