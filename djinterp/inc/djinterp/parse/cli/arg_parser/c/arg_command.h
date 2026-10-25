/*******************************************************************************
* djinterp [parse]                                                 arg_command.h
*
* Commands: the parser's root, its subcommands, and option groups.
*   A `struct d_arg_command` is CLI11's App in C. A command owns its options
* and its subcommands, and holds everything that decides how arguments are
* matched to them: whether unrecognized arguments are errors, kept or
* dropped, whether a subcommand may fall through to its parent, how many
* subcommands and options are required, which commands and options it needs
* or excludes, and the callbacks that run before, during and after parsing.
* An option group -- CLI11's Option_group -- is a command without a name:
* its options are matched as if they were its parent's, and it counts as one
* option toward its parent's requirements.
*   Every command belongs to one parser (arg_parser.h), whose root command
* is the only one not made by d_arg_command_add_subcommand or
* d_arg_command_add_option_group. A command lives until it is removed or the
* parser is released; options and subcommands are owned by the command they
* were made on.
*   As with options, every call accepts NULL for the command, and a failed
* definition records its error in the parser for the next parse to report.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_command.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Command flags
         1.  D_ARG_COMMAND_AUTOMATIC_NAME
         2.  D_ARG_COMMAND_REQUIRED
         3.  D_ARG_COMMAND_DISABLED
         4.  D_ARG_COMMAND_PRE_PARSE_CALLED
         5.  D_ARG_COMMAND_IMMEDIATE_CALLBACK
         6.  D_ARG_COMMAND_IGNORE_CASE
         7.  D_ARG_COMMAND_IGNORE_UNDERSCORE
         8.  D_ARG_COMMAND_FALLTHROUGH
         9.  D_ARG_COMMAND_SUBCOMMAND_FALLTHROUGH
         10. D_ARG_COMMAND_WINDOWS_STYLE
         11. D_ARG_COMMAND_POSITIONALS_AT_END
         12. D_ARG_COMMAND_CONFIGURABLE
         13. D_ARG_COMMAND_VALIDATE_POSITIONALS
         14. D_ARG_COMMAND_VALIDATE_OPTIONAL_ARGUMENTS
         15. D_ARG_COMMAND_SILENT
         16. D_ARG_COMMAND_NON_STANDARD_NAMES
         17. D_ARG_COMMAND_PREFIX_MATCHING
    2.  Name matches
         1.  d_arg_name_match
    3.  Callbacks
         1.  d_arg_flag_fn
         2.  d_arg_flag_intmax_fn
         3.  d_arg_option_visit_fn
         4.  d_arg_extra_visit_fn
         5.  d_arg_group_visit_fn
2.  RECORDS
    -------
    1.  The command
         1.  d_arg_command
3.  OPTIONS
    -------
    1.  Adding options
         1.  d_arg_command_add_option
         2.  d_arg_command_add_option_bound
         3.  d_arg_command_add_option_callback
    2.  Adding flags
         1.  d_arg_command_add_flag
         2.  d_arg_command_add_flag_bound
         3.  d_arg_command_add_flag_callback / d_arg_command_add_flag_function
         4.  d_arg_command_add_flag_function_intmax
    3.  Special options
         1.  d_arg_command_set_help_flag / d_arg_command_set_help_all_flag
         2.  d_arg_command_set_version_flag / d_arg_command_set_version_fn
         3.  d_arg_command_set_config
    4.  Finding and removing
         1.  d_arg_command_option
         2.  d_arg_command_remove_option
4.  SUBCOMMANDS AND GROUPS
    ----------------------
    1.  Adding
         1.  d_arg_command_add_subcommand
         2.  d_arg_command_add_option_group
    2.  Moving into a group
         1.  d_arg_command_group_add_option
         2.  d_arg_command_group_add_subcommand
    3.  Finding and removing
         1.  d_arg_command_subcommand / d_arg_command_subcommand_at
         2.  d_arg_command_option_group
         3.  d_arg_command_remove_subcommand
5.  SETTINGS
    --------
    1.  Names and text
         1.  d_arg_command_set_name
         2.  d_arg_command_add_alias / d_arg_command_clear_aliases
         3.  d_arg_command_set_description and kin
         4.  d_arg_command_set_usage_fn / d_arg_command_set_footer_fn
    2.  Modes
         1.  d_arg_command_set_allow_extras and kin
         2.  d_arg_command_set_allow_config_extras_bool
    3.  Switches
         1.  d_arg_command_set_required and kin
         2.  d_arg_command_set_ignore_case / d_arg_command_set_ignore_underscore
         3.  d_arg_command_set_immediate_callback
         4.  d_arg_command_set_disabled_by_default and kin
    4.  Requirements
         1.  d_arg_command_require_subcommand and kin
         2.  d_arg_command_require_option and kin
    5.  Callbacks
         1.  d_arg_command_set_callback
         2.  d_arg_command_set_final_callback and kin
         3.  d_arg_command_set_preparse_callback
         4.  d_arg_command_set_pre_callback
         5.  d_arg_command_set_failure_message
    6.  Relations
         1.  d_arg_command_excludes_option and kin
         2.  d_arg_command_remove_excludes_option and kin
    7.  Formatting
         1.  d_arg_command_set_formatter / d_arg_command_set_config_format
    8.  Option defaults
         1.  d_arg_command_option_defaults
         2.  d_arg_option_defaults_set_group and kin
6.  QUERIES
    -------
    1.  Fields
         1.  d_arg_command_has_flag
         2.  d_arg_command_name and kin
         3.  d_arg_command_is_option_group
    2.  Parse state
         1.  d_arg_command_count / d_arg_command_count_all
         2.  d_arg_command_parsed_subcommand_count and kin
         3.  d_arg_command_got_subcommand / d_arg_command_got_subcommand_name
    3.  Names
         1.  d_arg_command_display_name
         2.  d_arg_command_check_name / d_arg_command_check_name_detail
    4.  Traversal
         1.  d_arg_command_each_option
         2.  d_arg_command_each_group
    5.  Leftovers
         1.  d_arg_command_each_remaining
         2.  d_arg_command_remaining_size
    6.  Text on demand
         1.  d_arg_command_usage / d_arg_command_footer / d_arg_command_version
7.  UTILITIES
    ---------
    1.  Triggers
         1.  d_arg_command_trigger_on / d_arg_command_trigger_off
    2.  Life cycle of options
         1.  d_arg_command_deprecate_option
         2.  d_arg_command_deprecate_option_name
         3.  d_arg_command_retire_option / d_arg_command_retire_option_name
    3.  Parse state
         1.  d_arg_command_clear
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMAND_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMAND_H 1

// std
#include <stddef.h>                          // size_t
// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "../../../../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "./arg_common.h"                    // lists, callbacks, statuses
#include "./arg_option.h"                    // d_arg_option,
                                             // d_arg_option_defaults
#include "./arg_type.h"                      // d_arg_type
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // uint32_t, intmax_t


//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Command flags
//------------------------------------------------------------------------------
//   constant: the switches of d_arg_command's `flags`, CLI11's App booleans.
// A subcommand starts with its parent's, but for the parse state
// (AUTOMATIC_NAME, PRE_PARSE_CALLED) and the ones CLI11 does not inherit
// (REQUIRED, DISABLED, SILENT, POSITIONALS_AT_END, NON_STANDARD_NAMES,
// SUBCOMMAND_FALLTHROUGH, which is set).

// 1.1.1
// D_ARG_COMMAND_AUTOMATIC_NAME
//   constant: the name was taken from `argv[0]`, and is taken again by the
// next parse.
#define D_ARG_COMMAND_AUTOMATIC_NAME              0x00000001u

// 1.1.2
// D_ARG_COMMAND_REQUIRED
//   constant: the subcommand or option group must be used.
#define D_ARG_COMMAND_REQUIRED                    0x00000002u

// 1.1.3
// D_ARG_COMMAND_DISABLED
//   constant: the subcommand or group is not matched.
#define D_ARG_COMMAND_DISABLED                    0x00000004u

// 1.1.4
// D_ARG_COMMAND_PRE_PARSE_CALLED
//   constant: parse state: the pre-parse callback has run in this parse.
#define D_ARG_COMMAND_PRE_PARSE_CALLED            0x00000008u

// 1.1.5
// D_ARG_COMMAND_IMMEDIATE_CALLBACK
//   constant: the callback runs as soon as the subcommand is parsed, not
// after the whole command line.
#define D_ARG_COMMAND_IMMEDIATE_CALLBACK          0x00000010u

// 1.1.6
// D_ARG_COMMAND_IGNORE_CASE
//   constant: subcommand names match without regard to ASCII case.
#define D_ARG_COMMAND_IGNORE_CASE                 0x00000020u

// 1.1.7
// D_ARG_COMMAND_IGNORE_UNDERSCORE
//   constant: subcommand names match with `_` removed.
#define D_ARG_COMMAND_IGNORE_UNDERSCORE           0x00000040u

// 1.1.8
// D_ARG_COMMAND_FALLTHROUGH
//   constant: what a subcommand does not recognize is offered to its parent.
#define D_ARG_COMMAND_FALLTHROUGH                 0x00000080u

// 1.1.9
// D_ARG_COMMAND_SUBCOMMAND_FALLTHROUGH
//   constant: a subcommand may end at the name of one of its parent's
// subcommands. Set by default.
#define D_ARG_COMMAND_SUBCOMMAND_FALLTHROUGH      0x00000100u

// 1.1.10
// D_ARG_COMMAND_WINDOWS_STYLE
//   constant: `/name` and `/name:value` are options.
#define D_ARG_COMMAND_WINDOWS_STYLE               0x00000200u

// 1.1.11
// D_ARG_COMMAND_POSITIONALS_AT_END
//   constant: once a positional argument is met, everything after it is
// positional.
#define D_ARG_COMMAND_POSITIONALS_AT_END          0x00000400u

// 1.1.12
// D_ARG_COMMAND_CONFIGURABLE
//   constant: the subcommand may be opened from a configuration file's
// section.
#define D_ARG_COMMAND_CONFIGURABLE                0x00000800u

// 1.1.13
// D_ARG_COMMAND_VALIDATE_POSITIONALS
//   constant: a positional argument goes to the first positional option whose
// validators accept it.
#define D_ARG_COMMAND_VALIDATE_POSITIONALS        0x00001000u

// 1.1.14
// D_ARG_COMMAND_VALIDATE_OPTIONAL_ARGUMENTS
//   constant: an option's optional values stop at the first one its
// validators refuse.
#define D_ARG_COMMAND_VALIDATE_OPTIONAL_ARGUMENTS 0x00002000u

// 1.1.15
// D_ARG_COMMAND_SILENT
//   constant: the subcommand is parsed but not listed among the parsed
// subcommands.
#define D_ARG_COMMAND_SILENT                      0x00004000u

// 1.1.16
// D_ARG_COMMAND_NON_STANDARD_NAMES
//   constant: short names may have more than one character (`-ab`).
#define D_ARG_COMMAND_NON_STANDARD_NAMES          0x00008000u

// 1.1.17
// D_ARG_COMMAND_PREFIX_MATCHING
//   constant: an unambiguous prefix of a subcommand's name selects it.
#define D_ARG_COMMAND_PREFIX_MATCHING             0x00010000u

// 1.2    Name matches
//------------------------------------------------------------------------------
// 1.2.1
// d_arg_name_match
//   enum: how a name matched a command's: not at all, exactly (the name or an
// alias), or as a prefix; CLI11's NameMatch.
enum d_arg_name_match
{
    D_ARG_NAME_MATCH_NONE   = 0,
    D_ARG_NAME_MATCH_EXACT  = 1,
    D_ARG_NAME_MATCH_PREFIX = 2
};

// 1.3    Callbacks
//------------------------------------------------------------------------------
// 1.3.1
// d_arg_flag_fn
//   type: a flag's callback, given the flag's count or value: for
// d_arg_command_add_flag_callback, called with 1 only when the flag is
// true; for d_arg_command_add_flag_function, the summed count. Returns a
// status, as the other callbacks do.
typedef int (*d_arg_flag_fn)(struct d_arg_option* _option,
                             long                 _count,
                             void*                _context);

#if defined(INTMAX_MAX)

// 1.3.2
// d_arg_flag_intmax_fn
//   type: d_arg_flag_fn with the count as wide as CLI11's std::int64_t, for
// d_arg_command_add_flag_function_intmax, where a `long` may be narrower.
// Declared where dstdint.h declares intmax_t, as d_arg_parse_intmax is.
typedef int (*d_arg_flag_intmax_fn)(struct d_arg_option* _option,
                                    intmax_t             _count,
                                    void*                _context);

#endif  // INTMAX_MAX

// 1.3.3
// d_arg_option_visit_fn
//   type: called for each option a traversal finds; a status other than
// D_ARG_OK stops the traversal, which returns it.
typedef int (*d_arg_option_visit_fn)(struct d_arg_option* _option,
                                     void*                _context);

// 1.3.4
// d_arg_extra_visit_fn
//   type: called for each argument a command did not use, in order, with
// what it was taken to be; a status other than D_ARG_OK stops the
// traversal, which returns it.
typedef int (*d_arg_extra_visit_fn)(const struct d_arg_extra* _extra,
                                    void*                     _context);

// 1.3.5
// d_arg_group_visit_fn
//   type: called with each option group name a traversal finds; a status
// other than D_ARG_OK stops the traversal, which returns it.
typedef int (*d_arg_group_visit_fn)(struct d_pack_text _group,
                                    void*              _context);


//==============================================================================
// 2.  RECORDS
//==============================================================================


// 2.1    The command
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_command
//   struct: one command. Text fields are owned and NULL when unset, `group`
// meaning "SUBCOMMANDS" when NULL. The callbacks each come with a context
// and the function that frees it; `failure_fn` NULL means the parent's.
// `options` and `subcommands` are owned;
// `help`, `help_all`, `version` and `config` point into `options`. The
// lists after `subcommands` refer to records owned elsewhere, and the
// parse state -- `missing`, `parse_order`, `parsed_subcommands`, `parsed`
// -- belongs to the last parse. `formatter` and `config_format` are
// borrowed, NULL meaning the parent's (and the parser's at the root).
//   Public-payload layout: read any field; change one only through the calls
// below.
struct d_arg_command
{
    struct d_arg_parser*         parser;
    struct d_arg_command*        parent;
    char*                        name;
    char*                        description;
    char*                        group;
    char*                        usage;
    char*                        footer;
    char*                        version_text;
    struct d_arg_string_list     aliases;
    d_arg_command_text_fn        usage_fn;
    void*                        usage_context;
    d_arg_release_fn             usage_release;
    d_arg_command_text_fn        footer_fn;
    void*                        footer_context;
    d_arg_release_fn             footer_release;
    d_arg_command_text_fn        version_fn;
    void*                        version_context;
    d_arg_release_fn             version_release;
    d_arg_preparse_fn            preparse_fn;
    void*                        preparse_context;
    d_arg_release_fn             preparse_release;
    d_arg_command_fn             parse_complete_fn;
    void*                        parse_complete_context;
    d_arg_release_fn             parse_complete_release;
    d_arg_command_fn             final_fn;
    void*                        final_context;
    d_arg_release_fn             final_release;
    d_arg_command_fn             pre_callback_fn;
    void*                        pre_callback_context;
    d_arg_release_fn             pre_callback_release;
    d_arg_failure_fn             failure_fn;
    void*                        failure_context;
    d_arg_release_fn             failure_release;
    struct d_arg_formatter*      formatter;
    struct d_arg_config_format*  config_format;
    struct d_arg_option_defaults option_defaults;
    struct d_arg_option_list     options;
    struct d_arg_command_list    subcommands;
    struct d_arg_option*         help;
    struct d_arg_option*         help_all;
    struct d_arg_option*         version;
    struct d_arg_option*         config;
    struct d_arg_extra_list      missing;
    struct d_arg_option_list     parse_order;
    struct d_arg_command_list    parsed_subcommands;
    struct d_arg_command_list    exclude_subcommands;
    struct d_arg_option_list     exclude_options;
    struct d_arg_command_list    need_subcommands;
    struct d_arg_option_list     need_options;
    size_t                       require_subcommand_min;
    size_t                       require_subcommand_max;
    size_t                       require_option_min;
    size_t                       require_option_max;
    size_t                       parsed;
    int                          allow_extras;
    int                          allow_config_extras;
    int                          prefix_command;
    int                          startup;
    uint32_t                     flags;
};


D_EXTERN_C_BEGIN

//==============================================================================
// 3.  OPTIONS
//==============================================================================
// Names are given as CLI11 gives them: comma-separated, `-x` for a short
// name, `--name` for a long one, a bare word for the positional name, and,
// for a flag, `{value}` after a name for the value it stands for and `!`
// before one for a name that stands for false. Every function here returns
// the option, owned by the command, or NULL after recording CLI11's
// construction error in the parser.
//   A function of the caller's is taken, here as everywhere in the module,
// with its context and the function that frees that context, so that the
// record it is given to can own what the function needs; the calls taking
// one beside a command, names and a description have six parameters for
// that reason, one past the style guide's five.


// 3.1    Adding options
//------------------------------------------------------------------------------
// 3.1.1
// d_arg_command_add_option
/**
 * @brief Adds an option bound to nothing; CLI11's add_option(name, desc).
 *
 * @note The option starts from the command's option defaults. Bad names are
 *       CLI11's BadNameString errors ("Invalid one char name: <name>", "Long
 *       names strings require 2 dashes <name>", "Bad long name: <name>",
 *       "Invalid positional Name: <name>", "Names '-','--','++' are
 *       reserved and not allowed as option names <name>", "Only one
 *       positional name allowed, remove: <name>"); a name already in use is
 *       its OptionAlreadyAdded error, "cannot add '<names>': long name
 *       '--<name>' already used by '<name>'" and the others.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the names, as described above.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @return the option; NULL on failure, recorded as D_ARG_ERROR_BAD_NAME,
 *         D_ARG_ERROR_OPTION_ALREADY_ADDED or D_ARG_ERROR_OUT_OF_MEMORY, or,
 *         unrecorded, for a NULL argument.
 */
struct d_arg_option*
     d_arg_command_add_option(struct d_arg_command* _command,
                              const char*           _names,
                              const char*           _description);

// 3.1.2
// d_arg_command_add_option_bound
/**
 * @brief Adds an option bound to a variable; CLI11's add_option(name,
 *        variable, desc).
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the names.
 * @param[in]     _target       the variable, which must outlive the
 *                              option's use.
 * @param[in]     _type         the variable's type, which must outlive the
 *                              option; see d_arg_option_bind.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @return the option, or NULL, as d_arg_command_add_option.
 */
struct d_arg_option*
     d_arg_command_add_option_bound(struct d_arg_command*    _command,
                                    const char*              _names,
                                    void*                    _target,
                                    const struct d_arg_type* _type,
                                    const char*              _description);

// 3.1.3
// d_arg_command_add_option_callback
/**
 * @brief Adds an option whose values go to a function; CLI11's
 *        add_option(name, callback, desc).
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the names.
 * @param[in]     _callback     the function.
 * @param[in]     _context      its context.
 * @param[in]     _release      frees `_context` with the option, and on
 *                              failure; NULL leaves it borrowed.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @return the option, or NULL, as d_arg_command_add_option.
 */
struct d_arg_option*
     d_arg_command_add_option_callback(struct d_arg_command* _command,
                                       const char*           _names,
                                       d_arg_results_fn      _callback,
                                       void*                 _context,
                                       d_arg_release_fn      _release,
                                       const char*           _description);

// 3.2    Adding flags
//------------------------------------------------------------------------------
// 3.2.1
// d_arg_command_add_flag
/**
 * @brief Adds a flag, an option expecting no values; CLI11's add_flag.
 *
 * @note The flag keeps the last value given (D_ARG_MULTI_TAKE_LAST) and is
 *       never required. A positional name is CLI11's IncorrectConstruction,
 *       "<name>: Flags cannot be positional".
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the names, with `{value}` and `!` as
 *                              described above.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @return the flag, or NULL, as d_arg_command_add_option; also
 *         D_ARG_ERROR_INCORRECT_CONSTRUCTION, recorded.
 */
struct d_arg_option* d_arg_command_add_flag(struct d_arg_command* _command,
                                            const char*           _names,
                                            const char*           _description);

// 3.2.2
// d_arg_command_add_flag_bound
/**
 * @brief Adds a flag bound to a variable; CLI11's add_flag(name, variable,
 *        desc).
 *
 * @note The flag converts its value into the variable and has no type name,
 *       as CLI11's has none. As CLI11's default_flag_modifiers arrange, a
 *       container takes every value and is set even from a default; an
 *       integer wider than a byte counts the flag's uses -- the flag sums
 *       its values, defaults to "0" and always calls back; and any other
 *       type is marked to capture its default, which, as in CLI11, it never
 *       does, the variable not being its default function.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the names.
 * @param[in]     _target       the variable.
 * @param[in]     _type         its type.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @return the flag, or NULL, as d_arg_command_add_flag.
 */
struct d_arg_option*
     d_arg_command_add_flag_bound(struct d_arg_command*    _command,
                                  const char*              _names,
                                  void*                    _target,
                                  const struct d_arg_type* _type,
                                  const char*              _description);

// 3.2.3
// d_arg_command_add_flag_callback / d_arg_command_add_flag_function
/**
 * @brief Adds a flag that calls a function when it is given true; CLI11's
 *        add_flag_callback.
 *
 * @note add_flag_function is CLI11's add_flag_function: the flag sums its
 *       values and the function is called with the sum -- 0 when the sum is
 *       not an integer a `long` holds, where CLI11 reads 64 bits, as
 *       d_arg_command_add_flag_function_intmax does -- whenever the flag is
 *       given.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the names.
 * @param[in]     _function     the function.
 * @param[in]     _context      its context.
 * @param[in]     _release      frees `_context` with the flag, and on
 *                              failure; NULL leaves it borrowed.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @return the flag, or NULL, as d_arg_command_add_flag.
 */
struct d_arg_option*
     d_arg_command_add_flag_callback(struct d_arg_command* _command,
                                     const char*           _names,
                                     d_arg_flag_fn         _function,
                                     void*                 _context,
                                     d_arg_release_fn      _release,
                                     const char*           _description);
struct d_arg_option*
     d_arg_command_add_flag_function(struct d_arg_command* _command,
                                     const char*           _names,
                                     d_arg_flag_fn         _function,
                                     void*                 _context,
                                     d_arg_release_fn      _release,
                                     const char*           _description);

#if defined(INTMAX_MAX)

// 3.2.4
// d_arg_command_add_flag_function_intmax
/**
 * @brief Adds a flag that sums its values and calls a function with the
 *        sum as CLI11 gives it; CLI11's add_flag_function, whose function
 *        takes a std::int64_t.
 *
 * @note As d_arg_command_add_flag_function, but the sum is read as the
 *       64-bit integer CLI11 reads, on every system, and is 0 only when it
 *       is not one. Declared where dstdint.h declares intmax_t.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the names.
 * @param[in]     _function     the function.
 * @param[in]     _context      its context.
 * @param[in]     _release      frees `_context` with the flag, and on
 *                              failure; NULL leaves it borrowed.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @return the flag, or NULL, as d_arg_command_add_flag.
 */
struct d_arg_option*
     d_arg_command_add_flag_function_intmax(struct d_arg_command* _command,
                                            const char*           _names,
                                            d_arg_flag_intmax_fn  _function,
                                            void*                 _context,
                                            d_arg_release_fn      _release,
                                            const char*           _description);

#endif  // INTMAX_MAX

// 3.3    Special options
//------------------------------------------------------------------------------
// 3.3.1
// d_arg_command_set_help_flag / d_arg_command_set_help_all_flag
/**
 * @brief Replaces the flag that asks for help; CLI11's set_help_flag.
 *
 * @note The root starts with `-h,--help`, "Print this help message and
 *       exit", and a subcommand with a copy of its parent's. The flag is not
 *       configurable and runs first. The old flag is removed first, so that
 *       a refused name leaves none. set_help_all_flag is the same for the
 *       flag asking for help with every subcommand expanded, which no
 *       command has by default.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the flag's names; NULL or "" removes the
 *                              flag.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @return the flag; NULL when removed, or on failure, as
 *         d_arg_command_add_flag.
 */
struct d_arg_option*
     d_arg_command_set_help_flag(struct d_arg_command* _command,
                                 const char*           _names,
                                 const char*           _description);
struct d_arg_option*
     d_arg_command_set_help_all_flag(struct d_arg_command* _command,
                                     const char*           _names,
                                     const char*           _description);

// 3.3.2
// d_arg_command_set_version_flag / d_arg_command_set_version_fn
/**
 * @brief Replaces the flag that asks for the version; CLI11's
 *        set_version_flag(name, version, help).
 *
 * @note When given true, the parse ends with D_ARG_CALL_FOR_VERSION, and the
 *       parser's exit writes the version. The old flag is removed first,
 *       with its version. set_version_fn takes, instead of the text, a
 *       function that writes the version when it is asked for, its context,
 *       and the function that frees the context when the flag is replaced or
 *       removed, with the command, or at once when the flag is refused.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _names        the flag's names; NULL or "" removes it.
 * @param[in]     _version      the version text, copied.
 * @param[in]     _description  the help text, copied; NULL for CLI11's
 *                              "Display program version information and
 *                              exit".
 * @return the flag; NULL when removed, or on failure, as
 *         d_arg_command_add_flag.
 */
struct d_arg_option*
     d_arg_command_set_version_flag(struct d_arg_command* _command,
                                    const char*           _names,
                                    const char*           _version,
                                    const char*           _description);
struct d_arg_option*
     d_arg_command_set_version_fn(struct d_arg_command* _command,
                                  const char*           _names,
                                  d_arg_command_text_fn _write,
                                  void*                 _context,
                                  d_arg_release_fn      _release,
                                  const char*           _description);

// 3.3.3
// d_arg_command_set_config
/**
 * @brief Replaces the option that names configuration files; CLI11's
 *        set_config.
 *
 * @note The option takes file names, reads them last first, is not itself
 *       configurable, and, with a default file, is always processed. A file
 *       that cannot be read is an error when the option is required or was
 *       given, and is skipped otherwise. Without
 *       D_INTERNAL_ARG_PARSER_CONFIG_FILES a file that is there is refused
 *       with D_ARG_ERROR_CONFIG, and one that is not is missing, or
 *       skipped, as before. The old option is removed first, so that a
 *       refused name leaves none.
 *
 * @param[in,out] _command       the command.
 * @param[in]     _names         the option's names; NULL or "" removes it.
 * @param[in]     _default_file  the file read when none is given; NULL for
 *                               none.
 * @param[in]     _description   the help text, copied; NULL for CLI11's
 *                               "Read an ini file".
 * @param[in]     _required      whether a file must be given and read.
 * @return the option; NULL when removed, or on failure, as
 *         d_arg_command_add_option.
 */
struct d_arg_option*
     d_arg_command_set_config(struct d_arg_command* _command,
                              const char*           _names,
                              const char*           _default_file,
                              const char*           _description,
                              bool                  _required);

// 3.4    Finding and removing
//------------------------------------------------------------------------------
// 3.4.1
// d_arg_command_option
//   lookup: CLI11's get_option_no_throw(name): the option whose name matches
// as d_arg_option_check_name matches, searching the command, its option
// groups, and, with fallthrough, its parent; NULL when none does.
struct d_arg_option* d_arg_command_option(const struct d_arg_command* _command,
                                          const char*                 _name);

// 3.4.2
// d_arg_command_remove_option
/**
 * @brief Removes and releases an option; CLI11's remove_option.
 *
 * @note Every reference to the option anywhere in the parser's tree is
 *       dropped -- other options' needs and excludes, the commands' own, the
 *       help, version and configuration pointers, the parse order, the error
 *       reports -- where CLI11 drops only the links from its own command's
 *       options. Not to be called from a callback while a parse runs, as
 *       CLI11's is not.
 *
 * @param[in,out] _command  the command that owns the option.
 * @param[in]     _option   the option.
 * @post   on success the option, and every pointer to it, is invalid.
 * @return whether the command owned the option.
 */
bool                 d_arg_command_remove_option(struct d_arg_command* _command,
                                                 struct d_arg_option*  _option);


//==============================================================================
// 4.  SUBCOMMANDS AND GROUPS
//==============================================================================


// 4.1    Adding
//------------------------------------------------------------------------------
// 4.1.1
// d_arg_command_add_subcommand
/**
 * @brief Adds a subcommand; CLI11's add_subcommand(name, desc).
 *
 * @note The subcommand inherits CLI11's inheritable settings and its
 *       parent's help flags, option defaults, usage, footer and formatter;
 *       until given a failure message of its own, it uses its parent's. A
 *       name that is not valid is CLI11's IncorrectConstruction: one
 *       starting with `-`, `!`, a space or a character below it,
 *       "Subcommand name starts with invalid character, '!' and '-' and
 *       control characters"; one holding `=`, `:`, `{`, a space, or a
 *       character below it other than a tab, "Subcommand name contains
 *       invalid character ('<c>'), all characters are allowed
 *       except'=',':','{','}', ' ', and control characters", as CLI11 words
 *       it, though it allows `}`. A name or alias matching one of the
 *       parent's other subcommands -- or, under an option group, one of the
 *       group's command's -- is its OptionAlreadyAdded, "subcommand name or
 *       alias matches existing subcommand: <name> is already added".
 *
 * @param[in,out] _command      the parent.
 * @param[in]     _name         the name, copied; "" or NULL for a nameless
 *                              subcommand, which matches no argument.
 * @param[in]     _description  the help text, copied; NULL for none.
 * @return the subcommand, owned by `_command`; NULL after recording the
 *         failure, or, unrecorded, for a NULL command.
 */
struct d_arg_command*
     d_arg_command_add_subcommand(struct d_arg_command* _command,
                                  const char*           _name,
                                  const char*           _description);

// 4.1.2
// d_arg_command_add_option_group
/**
 * @brief Adds an option group; CLI11's add_option_group.
 *
 * @note The group is a nameless subcommand whose group is `_group_name`. It
 *       has no help flag and no footer, and does not fall through; a group
 *       whose name is empty or starts with `+` has no help-all flag either.
 *       A group whose name starts with `+` is listed with its parent's
 *       options in help; an empty name hides it. A newline in the name is
 *       CLI11's IncorrectConstruction, "option group names may not contain
 *       newlines or null characters".
 *
 * @param[in,out] _command      the parent.
 * @param[in]     _group_name   the group's name, copied; NULL is "".
 * @param[in]     _description  the help text, copied; NULL for none.
 * @return the group, or NULL, as d_arg_command_add_subcommand.
 */
struct d_arg_command*
     d_arg_command_add_option_group(struct d_arg_command* _command,
                                    const char*           _group_name,
                                    const char*           _description);

// 4.2    Moving into a group
//------------------------------------------------------------------------------
// 4.2.1
// d_arg_command_group_add_option
/**
 * @brief Moves an option of a group's parent into the group; CLI11's
 *        Option_group::add_option(Option*).
 *
 * @note The option is unchanged but for its `parent`, which becomes the
 *       group: it is matched, listed and removed as the group's. Every
 *       failure but D_ARG_ERROR_BAD_ARGUMENT is recorded, worded as CLI11
 *       words it.
 *
 * @param[in,out] _group   the option group.
 * @param[in,out] _option  the option, owned by the group's parent.
 * @retval D_ARG_OK                          moved.
 * @retval D_ARG_ERROR_OPTION_ALREADY_ADDED  the option is the parent's help,
 *                                           help-all or configuration option
 *                                           ("cannot move help options is
 *                                           already added", "cannot move
 *                                           config file options is already
 *                                           added"), or one of its names is
 *                                           taken in the group ("option was
 *                                           not located: <name> is already
 *                                           added").
 * @retval D_ARG_ERROR_OPTION_NOT_FOUND      `_option` is NULL ("the option
 *                                           is NULL not found"), or the
 *                                           parent does not own it ("could
 *                                           not locate the given Option not
 *                                           found").
 * @retval D_ARG_ERROR_OUT_OF_MEMORY         the group's list could not grow.
 * @retval D_ARG_ERROR_BAD_ARGUMENT          `_group` is not an option group.
 */
int  d_arg_command_group_add_option(struct d_arg_command* _group,
                                    struct d_arg_option*  _option);

// 4.2.2
// d_arg_command_group_add_subcommand
/**
 * @brief Moves a subcommand into a group; CLI11's
 *        Option_group::add_subcommand(App*).
 *
 * @note The subcommand keeps its settings; the needs and excludes between
 *       it and its old siblings are dropped, as CLI11's remove_subcommand
 *       drops them. Its names are checked before anything moves, so that a
 *       refused subcommand stays where it was, where CLI11 destroys it.
 *
 * @param[in,out] _group       the option group.
 * @param[in,out] _subcommand  the subcommand, under any command of the
 *                             group's parser but the group itself or one
 *                             below it.
 * @retval D_ARG_OK                          moved.
 * @retval D_ARG_ERROR_OPTION_ALREADY_ADDED  a name or alias clashes, as for
 *                                           d_arg_command_add_subcommand;
 *                                           recorded.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY         the group's list could not grow;
 *                                           recorded.
 * @retval D_ARG_ERROR_BAD_ARGUMENT          `_group` is not an option group;
 *                                           `_subcommand` is NULL, a root, or
 *                                           another parser's; or the move
 *                                           would put a command inside
 *                                           itself.
 */
int  d_arg_command_group_add_subcommand(struct d_arg_command* _group,
                                        struct d_arg_command* _subcommand);

// 4.3    Finding and removing
//------------------------------------------------------------------------------
// 4.3.1
// d_arg_command_subcommand / d_arg_command_subcommand_at
//   lookups: CLI11's get_subcommand_no_throw(name), which looks into option
// groups and reads the command's own matching settings, and
// get_subcommand(index), over the command's own list. NULL when there is no
// such subcommand.
struct d_arg_command*
     d_arg_command_subcommand(const struct d_arg_command* _command,
                              const char*                 _name);
struct d_arg_command*
     d_arg_command_subcommand_at(const struct d_arg_command* _command,
                                 size_t                      _index);

// 4.3.2
// d_arg_command_option_group
//   lookup: CLI11's get_option_group(name): the nameless subcommand whose
// group is `_group_name`; NULL when there is none.
struct d_arg_command*
     d_arg_command_option_group(const struct d_arg_command* _command,
                                const char*                 _group_name);

// 4.3.3
// d_arg_command_remove_subcommand
/**
 * @brief Removes and releases a subcommand; CLI11's remove_subcommand.
 *
 * @note As in CLI11, the needs and excludes between the command's
 *       subcommands and `_subcommand` are dropped first, whether or not the
 *       command owns it. When it does, every reference anywhere in the
 *       parser's tree to the subcommand, to a command below it or to one of
 *       their options is dropped too, where CLI11 drops only its siblings'.
 *       Not to be called from a callback while a parse runs, as CLI11's is
 *       not.
 *
 * @param[in,out] _command     the parent.
 * @param[in]     _subcommand  the subcommand.
 * @post   on success the subcommand, its options and its own subcommands are
 *         invalid.
 * @return whether the command owned the subcommand.
 */
bool d_arg_command_remove_subcommand(struct d_arg_command* _command,
                                     struct d_arg_command* _subcommand);


//==============================================================================
// 5.  SETTINGS
//==============================================================================
// Setters returning a status return D_ARG_OK, D_ARG_ERROR_OUT_OF_MEMORY when
// a copy could not be made, or the construction error CLI11 would throw,
// recorded in the parser; NULL commands are ignored with
// D_ARG_ERROR_BAD_ARGUMENT, unrecorded.


// 5.1    Names and text
//------------------------------------------------------------------------------
// 5.1.1
// d_arg_command_set_name
//   setting: CLI11's name(): the name, copied; NULL or "" for none, which
// leaves a root to take its name from `argv[0]` and makes a subcommand
// nameless. A subcommand's new name must not match one of its fallthrough
// parent's other subcommands (D_ARG_ERROR_OPTION_ALREADY_ADDED, "<name>
// conflicts with existing subcommand names is already added"), and is not
// otherwise checked, as in CLI11. Clears AUTOMATIC_NAME.
int  d_arg_command_set_name(struct d_arg_command* _command,
                            const char*           _name);

// 5.1.2
// d_arg_command_add_alias / d_arg_command_clear_aliases
//   settings: CLI11's alias(), which refuses a NULL or empty alias or one with
// a newline (D_ARG_ERROR_INCORRECT_CONSTRUCTION, "Aliases may not be empty
// or contain newlines or null characters") and, for a subcommand, one that
// makes it match another of its fallthrough parent's subcommands
// (D_ARG_ERROR_OPTION_ALREADY_ADDED, "alias already matches an existing
// subcommand: <alias> is already added"); and clear_aliases().
int  d_arg_command_add_alias(struct d_arg_command* _command,
                             const char*           _alias);
void d_arg_command_clear_aliases(struct d_arg_command* _command);

// 5.1.3
// d_arg_command_set_description and kin
//   settings: CLI11's description(), usage(string), footer(string) and
// group(). Each copies its text; NULL clears it, and for the group
// restores "SUBCOMMANDS".
int  d_arg_command_set_description(struct d_arg_command* _command,
                                   const char*           _description);
int  d_arg_command_set_usage(struct d_arg_command* _command,
                             const char*           _usage);
int  d_arg_command_set_footer(struct d_arg_command* _command,
                              const char*           _footer);
int  d_arg_command_set_group(struct d_arg_command* _command,
                             const char*           _group);

// 5.1.4
// d_arg_command_set_usage_fn / d_arg_command_set_footer_fn
//   settings: CLI11's usage(function) and footer(function): text written on
// demand, before a newline and the fixed text. NULL removes the function.
// The release function frees the context as the callbacks' in 5.5 do.
void d_arg_command_set_usage_fn(struct d_arg_command* _command,
                                d_arg_command_text_fn _write,
                                void*                 _context,
                                d_arg_release_fn      _release);
void d_arg_command_set_footer_fn(struct d_arg_command* _command,
                                 d_arg_command_text_fn _write,
                                 void*                 _context,
                                 d_arg_release_fn      _release);

// 5.2    Modes
//------------------------------------------------------------------------------
// 5.2.1
// d_arg_command_set_allow_extras and kin
//   settings: CLI11's allow_extras(ExtrasMode), allow_config_extras(Config-
// ExtrasMode) and prefix_command(PrefixCommandMode): a d_arg_extras_mode,
// d_arg_config_extras_mode or d_arg_prefix_mode value; one outside its
// enum is ignored. None fails.
void d_arg_command_set_allow_extras(struct d_arg_command* _command,
                                    int                   _mode);
void d_arg_command_set_allow_config_extras(struct d_arg_command* _command,
                                           int                   _mode);
void d_arg_command_set_prefix_command(struct d_arg_command* _command,
                                      int                   _mode);

// 5.2.2
// d_arg_command_set_allow_config_extras_bool
//   setting: CLI11's allow_config_extras(bool): true captures both
// configuration extras and command-line extras, false makes configuration
// extras errors.
void d_arg_command_set_allow_config_extras_bool(struct d_arg_command* _command,
                                                bool                  _allow);

// 5.3    Switches
//------------------------------------------------------------------------------
// 5.3.1
// d_arg_command_set_required and kin
//   settings: set or clear one D_ARG_COMMAND_* flag: required, disabled,
// silent, allow_windows_style_options, positionals_at_end, configurable,
// validate_positionals, validate_optional_arguments,
// allow_non_standard_option_names, allow_subcommand_prefix_matching,
// fallthrough and subcommand_fallthrough. None fails.
void d_arg_command_set_required(struct d_arg_command* _command,
                                bool                  _value);
void d_arg_command_set_disabled(struct d_arg_command* _command,
                                bool                  _value);
void d_arg_command_set_silent(struct d_arg_command* _command,
                              bool                  _value);
void d_arg_command_set_windows_style(struct d_arg_command* _command,
                                     bool                  _value);
void d_arg_command_set_positionals_at_end(struct d_arg_command* _command,
                                          bool                  _value);
void d_arg_command_set_configurable(struct d_arg_command* _command,
                                    bool                  _value);
void d_arg_command_set_validate_positionals(struct d_arg_command* _command,
                                            bool                  _value);
void d_arg_command_set_validate_optional_arguments(struct d_arg_command* _command,
                                                   bool                  _value);
void d_arg_command_set_non_standard_names(struct d_arg_command* _command,
                                          bool                  _value);
void d_arg_command_set_prefix_matching(struct d_arg_command* _command,
                                       bool                  _value);
void d_arg_command_set_fallthrough(struct d_arg_command* _command,
                                   bool                  _value);
void d_arg_command_set_subcommand_fallthrough(struct d_arg_command* _command,
                                              bool                  _value);

// 5.3.2
// d_arg_command_set_ignore_case / d_arg_command_set_ignore_underscore
//   settings: CLI11's ignore_case() and ignore_underscore(), which refuse to
// turn on when the command's name or an alias would then match another of
// its fallthrough parent's subcommands -- or, at the root, one of its own --
// (D_ARG_ERROR_OPTION_ALREADY_ADDED, "ignore case would cause subcommand
// name conflicts: <name> is already added", or "ignore underscore ..."),
// keeping the setting off.
int  d_arg_command_set_ignore_case(struct d_arg_command* _command,
                                   bool                  _value);
int  d_arg_command_set_ignore_underscore(struct d_arg_command* _command,
                                         bool                  _value);

// 5.3.3
// d_arg_command_set_immediate_callback
//   setting: CLI11's immediate_callback(): moves the command's callback
// between running when the command is parsed and running after the whole
// command line, when only one of the two is set.
void d_arg_command_set_immediate_callback(struct d_arg_command* _command,
                                          bool                  _value);

// 5.3.4
// d_arg_command_set_disabled_by_default and kin
//   settings: CLI11's disabled_by_default() and enabled_by_default(): the
// d_arg_startup state each parse starts the command in. Turning one off
// leaves the other as it was.
void d_arg_command_set_disabled_by_default(struct d_arg_command* _command,
                                           bool                  _value);
void d_arg_command_set_enabled_by_default(struct d_arg_command* _command,
                                          bool                  _value);

// 5.4    Requirements
//------------------------------------------------------------------------------
// 5.4.1
// d_arg_command_require_subcommand and kin
//   settings: CLI11's require_subcommand(min, max) -- a maximum of 0 is
// unlimited -- and require_subcommand(int): `n` requires exactly `n`, `-n`
// allows at most `n`; CLI11's require_subcommand() is (1, 0). None fails.
void d_arg_command_require_subcommand(struct d_arg_command* _command,
                                      size_t                _minimum,
                                      size_t                _maximum);
void d_arg_command_require_subcommand_count(struct d_arg_command* _command,
                                            int                   _value);

// 5.4.2
// d_arg_command_require_option and kin
//   settings: CLI11's require_option(min, max) and require_option(int), with
// the same meanings, counting option groups used as one option each.
void d_arg_command_require_option(struct d_arg_command* _command,
                                  size_t                _minimum,
                                  size_t                _maximum);
void d_arg_command_require_option_count(struct d_arg_command* _command,
                                        int                   _value);

// 5.5    Callbacks
//------------------------------------------------------------------------------
//   Each takes the function, its context, and the function that frees the
// context with the command or when the callback is replaced (NULL leaves it
// borrowed), as d_arg_release_fn describes; a NULL function removes the
// callback, freeing both the old context and the one given. None fails.

// 5.5.1
// d_arg_command_set_callback
//   setting: CLI11's callback(): the parse-complete callback with
// D_ARG_COMMAND_IMMEDIATE_CALLBACK, the final callback without.
void d_arg_command_set_callback(struct d_arg_command* _command,
                                d_arg_command_fn      _callback,
                                void*                 _context,
                                d_arg_release_fn      _release);

// 5.5.2
// d_arg_command_set_final_callback and kin
//   settings: CLI11's final_callback(), which runs after the whole command
// line is processed, and parse_complete_callback(), which runs as soon as
// the command's own arguments are.
void d_arg_command_set_final_callback(struct d_arg_command* _command,
                                      d_arg_command_fn      _callback,
                                      void*                 _context,
                                      d_arg_release_fn      _release);
void d_arg_command_set_parse_complete_callback(struct d_arg_command* _command,
                                               d_arg_command_fn      _callback,
                                               void*                 _context,
                                               d_arg_release_fn      _release);

// 5.5.3
// d_arg_command_set_preparse_callback
//   setting: CLI11's preparse_callback(): runs once per parse, when the
// command is first met, with the number of arguments left.
void d_arg_command_set_preparse_callback(struct d_arg_command* _command,
                                         d_arg_preparse_fn     _callback,
                                         void*                 _context,
                                         d_arg_release_fn      _release);

// 5.5.4
// d_arg_command_set_pre_callback
//   setting: what CLI11's virtual pre_callback() gives a subclass: a function
// run each time the command's callbacks are about to run, after its
// arguments are parsed and before its parse-complete callback, its
// subcommands' callbacks and its final callback.
void d_arg_command_set_pre_callback(struct d_arg_command* _command,
                                    d_arg_command_fn      _callback,
                                    void*                 _context,
                                    d_arg_release_fn      _release);

// 5.5.5
// d_arg_command_set_failure_message
//   setting: CLI11's failure_message(): what d_arg_command_exit writes for
// an error reported from the command, and d_arg_parser_exit from the root;
// d_arg_failure_simple or d_arg_failure_help (arg_parser.h), or a function
// of the caller's. NULL -- what every command starts with -- uses the
// parent's, and at the root d_arg_failure_simple, CLI11's default; so a
// subcommand follows its parent's later changes, where CLI11's keeps a copy
// of the message its parent had when it was made. A function that fails --
// returns anything but D_ARG_OK -- is followed by d_arg_failure_simple's
// message, so that no failed parse goes unreported.
void d_arg_command_set_failure_message(struct d_arg_command* _command,
                                       d_arg_failure_fn      _write,
                                       void*                 _context,
                                       d_arg_release_fn      _release);

// 5.6    Relations
//------------------------------------------------------------------------------
// 5.6.1
// d_arg_command_excludes_option and kin
/**
 * @brief Makes a command refuse to be used with an option; CLI11's
 *        excludes(Option*).
 *
 * @note excludes_command is CLI11's excludes(App*), recorded on both
 *       commands; needs_option and needs_command are its needs(), which
 *       require the option or command whenever this command is used. A
 *       command excluding or needing itself is CLI11's OptionNotFound,
 *       "cannot self reference in excludes not found" (or "needs"). Every
 *       failure but D_ARG_ERROR_BAD_ARGUMENT is recorded.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _option   the option.
 * @retval D_ARG_OK                            recorded, or already
 *                                             recorded.
 * @retval D_ARG_ERROR_OPTION_NOT_FOUND        `_option` is NULL: "nullptr
 *                                             passed not found".
 * @retval D_ARG_ERROR_INCORRECT_CONSTRUCTION  the two belong to different
 *                                             parsers: "options and commands
 *                                             of different parsers cannot be
 *                                             linked" (or "commands of ...").
 * @retval D_ARG_ERROR_OUT_OF_MEMORY           the list could not grow.
 * @retval D_ARG_ERROR_BAD_ARGUMENT            `_command` is NULL.
 */
int  d_arg_command_excludes_option(struct d_arg_command* _command,
                                   struct d_arg_option*  _option);
int  d_arg_command_excludes_command(struct d_arg_command* _command,
                                    struct d_arg_command* _other);
int  d_arg_command_needs_option(struct d_arg_command* _command,
                                struct d_arg_option*  _option);
int  d_arg_command_needs_command(struct d_arg_command* _command,
                                 struct d_arg_command* _other);

// 5.6.2
// d_arg_command_remove_excludes_option and kin
//   relations: CLI11's remove_excludes and remove_needs. Removing a command
// exclusion removes both halves. Each returns whether there was one.
bool d_arg_command_remove_excludes_option(struct d_arg_command*      _command,
                                          const struct d_arg_option* _option);
bool d_arg_command_remove_excludes_command(struct d_arg_command* _command,
                                           struct d_arg_command* _other);
bool d_arg_command_remove_needs_option(struct d_arg_command*      _command,
                                       const struct d_arg_option* _option);
bool d_arg_command_remove_needs_command(struct d_arg_command*       _command,
                                        const struct d_arg_command* _other);

// 5.7    Formatting
//------------------------------------------------------------------------------
// 5.7.1
// d_arg_command_set_formatter / d_arg_command_set_config_format
//   settings: CLI11's formatter() and config_formatter(), but borrowed: the
// formatter or configuration format must outlive the command, and NULL
// returns to the parent's. Subcommands made afterwards share it, as CLI11's
// share their parent's.
void d_arg_command_set_formatter(struct d_arg_command*   _command,
                                 struct d_arg_formatter* _formatter);
void d_arg_command_set_config_format(struct d_arg_command*       _command,
                                     struct d_arg_config_format* _format);

// 5.8    Option defaults
//------------------------------------------------------------------------------
// 5.8.1
// d_arg_command_option_defaults
//   lookup: CLI11's option_defaults(): the defaults the command's new options
// start from, owned by the command; NULL for a NULL command.
struct d_arg_option_defaults*
     d_arg_command_option_defaults(struct d_arg_command* _command);

// 5.8.2
// d_arg_option_defaults_set_group and kin
//   settings: CLI11's OptionDefaults setters. The group is copied, with NULL
// restoring "OPTIONS"; the switches take D_ARG_OPTION_REQUIRED,
// _IGNORE_CASE, _IGNORE_UNDERSCORE, _CONFIGURABLE, _DISABLE_FLAG_OVERRIDE or
// _ALWAYS_CAPTURE_DEFAULT, and ignore any other flag; a value outside its
// enum is ignored. Only the group can fail, with D_ARG_ERROR_OUT_OF_MEMORY,
// which is not recorded, for the defaults do not know their command.
int  d_arg_option_defaults_set_group(struct d_arg_option_defaults* _defaults,
                                     const char*                   _group);
void d_arg_option_defaults_set_flag(struct d_arg_option_defaults* _defaults,
                                    uint32_t                      _flag,
                                    bool                          _value);
void d_arg_option_defaults_set_multi_option_policy(struct d_arg_option_defaults* _defaults,
                                                   int                           _policy);
void d_arg_option_defaults_set_callback_priority(struct d_arg_option_defaults* _defaults,
                                                 int                           _priority);
void d_arg_option_defaults_set_integer_rule(struct d_arg_option_defaults* _defaults,
                                            int                           _rule);
void d_arg_option_defaults_set_delimiter(struct d_arg_option_defaults* _defaults,
                                         char                          _delimiter);


//==============================================================================
// 6.  QUERIES
//==============================================================================


// 6.1    Fields
//------------------------------------------------------------------------------
// 6.1.1
// d_arg_command_has_flag
/**
 * @brief Reads one of a command's D_ARG_COMMAND_* switches.
 *
 * @param[in] _command  the command.
 * @param[in] _flag     a D_ARG_COMMAND_* constant.
 * @return whether the switch is set; `false` for NULL.
 */
D_INLINE bool
d_arg_command_has_flag(
    const struct d_arg_command* _command,
    uint32_t                    _flag
)
{
    return ( (_command != NULL) &&
             ((_command->flags & _flag) != 0u) );
}

// 6.1.2
// d_arg_command_name and kin
//   lookups: the command's name, description and group ("SUBCOMMANDS" when
// unset), "" when unset and never NULL; NULL reads as a command with
// nothing set.
const char* d_arg_command_name(const struct d_arg_command* _command);
const char* d_arg_command_description(const struct d_arg_command* _command);
const char* d_arg_command_group(const struct d_arg_command* _command);

// 6.1.3
// d_arg_command_is_option_group
//   predicate: whether the command is a nameless subcommand -- an option group
// -- rather than the root or a named subcommand.
bool        d_arg_command_is_option_group(const struct d_arg_command* _command);

// 6.2    Parse state
//------------------------------------------------------------------------------
// 6.2.1
// d_arg_command_count / d_arg_command_count_all
//   lookups: count is CLI11's count(), how many times the command was used in
// the last parse; count_all is CLI11's count_all(), the values of its
// options and those of its subcommands, plus its own uses when it has a
// name. 0 for NULL.
size_t      d_arg_command_count(const struct d_arg_command* _command);
size_t      d_arg_command_count_all(const struct d_arg_command* _command);

// 6.2.2
// d_arg_command_parsed_subcommand_count and kin
//   lookups: CLI11's get_subcommands(): the subcommands the last parse used,
// in order -- its count, and the one at an index, NULL past the end.
size_t
     d_arg_command_parsed_subcommand_count(const struct d_arg_command* _command);
struct d_arg_command*
     d_arg_command_parsed_subcommand_at(const struct d_arg_command* _command,
                                        size_t                      _index);

// 6.2.3
// d_arg_command_got_subcommand / d_arg_command_got_subcommand_name
//   predicates: CLI11's got_subcommand(App*) and got_subcommand(name): whether
// the subcommand was used in the last parse. False for NULL or a
// subcommand that is not this command's.
bool d_arg_command_got_subcommand(const struct d_arg_command* _command,
                                  const struct d_arg_command* _subcommand);
bool d_arg_command_got_subcommand_name(const struct d_arg_command* _command,
                                       const char*                 _name);

// 6.3    Names
//------------------------------------------------------------------------------
// 6.3.1
// d_arg_command_display_name
/**
 * @brief Writes CLI11's get_display_name: the name, followed by its
 *        aliases when asked, or "[Option Group: <group>]" for a group.
 *
 * @param[in]     _command       the command.
 * @param[in]     _with_aliases  whether to add ", <alias>" for each alias.
 * @param[in,out] _out           receives the name, appended.
 * @return D_ARG_OK, or the buffer's status; D_ARG_ERROR_BAD_ARGUMENT for a
 *         NULL argument.
 */
int  d_arg_command_display_name(const struct d_arg_command* _command,
                                bool                        _with_aliases,
                                struct d_arg_buffer*        _out);

// 6.3.2
// d_arg_command_check_name / d_arg_command_check_name_detail
//   matching: CLI11's check_name and check_name_detail: whether a name is the
// command's name or an alias, honouring its ignore-case and
// ignore-underscore switches, and, with prefix matching, as a prefix; the
// detail is a d_arg_name_match value.
bool d_arg_command_check_name(const struct d_arg_command* _command,
                              struct d_pack_text          _name);
int  d_arg_command_check_name_detail(const struct d_arg_command* _command,
                                     struct d_pack_text          _name);

// 6.4    Traversal
//------------------------------------------------------------------------------
// 6.4.1
// d_arg_command_each_option
/**
 * @brief Visits a command's options as CLI11's get_options() lists them.
 *
 * @note The command's own options, then those of every nameless
 *       subcommand and of every subcommand whose group starts with `+`,
 *       then, with fallthrough, its parent's that answer to none of the
 *       display names already listed -- the list configuration files and
 *       requirements see. Help lists a narrower set; see arg_formatter.h.
 *
 * @param[in] _command  the command.
 * @param[in] _visit    called for each option.
 * @param[in] _context  `_visit`'s context.
 * @return D_ARG_OK after every option, or the first other status `_visit`
 *         returned; D_ARG_ERROR_BAD_ARGUMENT for a NULL argument.
 */
int         d_arg_command_each_option(struct d_arg_command* _command,
                                      d_arg_option_visit_fn _visit,
                                      void*                 _context);

// 6.4.2
// d_arg_command_each_group
//   traversal: CLI11's get_groups(): calls `_visit` once for each distinct
// option group name among the command's own options, in the order first
// met, with the group as a span; stops at the first other status, which it
// returns.
int         d_arg_command_each_group(const struct d_arg_command* _command,
                                     d_arg_group_visit_fn        _visit,
                                     void*                       _context);

// 6.5    Leftovers
//------------------------------------------------------------------------------
// 6.5.1
// d_arg_command_each_remaining
/**
 * @brief Visits the arguments a command did not use; CLI11's remaining().
 *
 * @note With `_recurse`, also those of its option groups -- when its extras
 *       are errors or ignored -- and of each subcommand it parsed, in the
 *       order met.
 *
 * @param[in] _command  the command.
 * @param[in] _recurse  whether to descend.
 * @param[in] _visit    called for each argument.
 * @param[in] _context  `_visit`'s context.
 * @return D_ARG_OK after every argument, or the first other status `_visit`
 *         returned; D_ARG_ERROR_BAD_ARGUMENT for a NULL argument.
 */
int         d_arg_command_each_remaining(const struct d_arg_command* _command,
                                         bool                        _recurse,
                                         d_arg_extra_visit_fn        _visit,
                                         void*                       _context);

// 6.5.2
// d_arg_command_remaining_size
//   lookup: CLI11's remaining_size(): the unused arguments that are not the
// `--` that ends options, with `_recurse` those of every subcommand too.
size_t      d_arg_command_remaining_size(const struct d_arg_command* _command,
                                         bool                        _recurse);

// 6.6    Text on demand
//------------------------------------------------------------------------------
// 6.6.1
// d_arg_command_usage / d_arg_command_footer / d_arg_command_version
//   writing: CLI11's get_usage(), get_footer() and version(): the function's
// text and a newline, if there is a function, then the fixed text; the
// version flag's text or function, or nothing. Each returns D_ARG_OK or
// the status of the sink or function that failed.
int         d_arg_command_usage(const struct d_arg_command* _command,
                                struct d_pack_sink          _sink);
int         d_arg_command_footer(const struct d_arg_command* _command,
                                 struct d_pack_sink          _sink);
int         d_arg_command_version(const struct d_arg_command* _command,
                                  struct d_pack_sink          _sink);


//==============================================================================
// 7.  UTILITIES
//==============================================================================


// 7.1    Triggers
//------------------------------------------------------------------------------
// 7.1.1
// d_arg_command_trigger_on / d_arg_command_trigger_off
/**
 * @brief Makes a command enable others when it is met; CLI11's TriggerOn,
 *        for one target or a list of them.
 *
 * @note The targets start every parse disabled, and the trigger's pre-parse
 *       callback, which this replaces, enables them. trigger_off is CLI11's
 *       TriggerOff, the reverse. A target removed later is dropped from the
 *       list.
 *
 * @param[in,out] _trigger  the command whose use enables the targets.
 * @param[in]     _targets  the commands enabled, which the trigger keeps a
 *                          copy of the list of; NULL for none.
 * @param[in]     _count    how many there are.
 * @retval D_ARG_OK                            set.
 * @retval D_ARG_ERROR_INCORRECT_CONSTRUCTION  a target belongs to another
 *                                             parser: "commands of different
 *                                             parsers cannot be linked";
 *                                             recorded, nothing changed.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY           the list could not be copied;
 *                                             recorded, nothing changed.
 * @retval D_ARG_ERROR_BAD_ARGUMENT            `_trigger` or a target is NULL,
 *                                             or `_targets` is NULL with
 *                                             `_count` above 0.
 */
int         d_arg_command_trigger_on(struct d_arg_command*        _trigger,
                                     struct d_arg_command* const* _targets,
                                     size_t                       _count);
int         d_arg_command_trigger_off(struct d_arg_command*        _trigger,
                                      struct d_arg_command* const* _targets,
                                      size_t                       _count);

// 7.2    Life cycle of options
//------------------------------------------------------------------------------
// 7.2.1
// d_arg_command_deprecate_option
/**
 * @brief Marks an option deprecated; CLI11's deprecate_option.
 *
 * @note Using the option writes "<name> is deprecated please use
 *       '<replacement>' instead" to the parser's output; with a
 *       replacement, its description gains " DEPRECATED: please use
 *       '<replacement>' instead".
 *
 * @param[in,out] _option       the option.
 * @param[in]     _replacement  what to use instead, copied; NULL or "" for
 *                              nothing named.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, recorded;
 *         D_ARG_ERROR_BAD_ARGUMENT for a NULL option.
 */
int         d_arg_command_deprecate_option(struct d_arg_option* _option,
                                           const char*          _replacement);

// 7.2.2
// d_arg_command_deprecate_option_name
/**
 * @brief Marks deprecated the option a command finds by name; CLI11's
 *        deprecate_option(app, name, replacement).
 *
 * @note The option is found as d_arg_command_option finds it -- CLI11's
 *       get_option -- and deprecated as d_arg_command_deprecate_option
 *       deprecates it.
 *
 * @param[in,out] _command      the command.
 * @param[in]     _name         the option's name.
 * @param[in]     _replacement  what to use instead, copied; NULL or "" for
 *                              nothing named.
 * @return D_ARG_OK; D_ARG_ERROR_OPTION_NOT_FOUND, recorded as CLI11's
 *         OptionNotFound -- "<name> not found" -- for a name no option has;
 *         D_ARG_ERROR_OUT_OF_MEMORY, recorded; D_ARG_ERROR_BAD_ARGUMENT,
 *         nothing recorded, for a NULL command or name.
 */
int
     d_arg_command_deprecate_option_name(struct d_arg_command* _command,
                                         const char*           _name,
                                         const char*           _replacement);

// 7.2.3
// d_arg_command_retire_option / d_arg_command_retire_option_name
/**
 * @brief Replaces an option with one that takes the same values and does
 *        nothing; CLI11's retire_option.
 *
 * @note Using the retired option writes "WARNING <name> is retired and has
 *       no effect" to the parser's output. retire_option_name finds the
 *       option by name, or, when there is none, adds a retired option
 *       taking at most one value.
 *
 * @param[in,out] _command  the command owning the option.
 * @param[in]     _option   the option, released by the call.
 * @return the retired option, owned by the command; NULL on failure, as
 *         d_arg_command_add_option.
 */
struct d_arg_option*
     d_arg_command_retire_option(struct d_arg_command* _command,
                                 struct d_arg_option*  _option);
struct d_arg_option*
     d_arg_command_retire_option_name(struct d_arg_command* _command,
                                      const char*           _name);

// 7.3    Parse state
//------------------------------------------------------------------------------
// 7.3.1
// d_arg_command_clear
/**
 * @brief Forgets what the last parse did to a command and its subcommands;
 *        CLI11's clear().
 *
 * @note A parse clears the whole tree before it begins, so this is needed
 *       only to forget a parse early, or one part of it.
 *
 * @param[in,out] _command  the command.
 * @post   the command and its subcommands are unparsed, their options hold
 *         no values, and the arguments they did not use are forgotten; every
 *         value and remaining argument read from them before is invalid.
 */
void        d_arg_command_clear(struct d_arg_command* _command);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMAND_H
