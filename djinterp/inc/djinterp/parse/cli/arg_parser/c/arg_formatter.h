/*******************************************************************************
* djinterp [parse]                                               arg_formatter.h
*
* Help: the formatter that lays out a command's help text.
*   A `struct d_arg_formatter` is CLI11's Formatter in C: the column widths,
* the paragraph widths, the switches for what an option's line shows, the
* labels that translate help's fixed words, and a table of the steps help
* is built from -- CLI11's virtual make_* functions. A NULL entry in the
* table runs the default step, which is also callable by name, so a
* replacement can do its own work around CLI11's.
*   Help is built in a d_arg_buffer and written to a sink; the layout is
* CLI11's to the character, so a program ported from CLI11 prints the same
* help. Beyond CLI11, a formatter can keep its lines within a width -- one
* it is given, or, with D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH, the terminal's,
* measured each time help is made.
*   Every command uses its own formatter, if set, else its parent's, and
* the root the one its parser holds.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_formatter.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Formatter flags
         1.  D_ARG_FORMAT_DESCRIPTION_PARAGRAPHS
         2.  D_ARG_FORMAT_FOOTER_PARAGRAPHS
         3.  D_ARG_FORMAT_OPTION_DEFAULTS
         4.  D_ARG_FORMAT_OPTION_TYPE_NAMES
         5.  D_ARG_FORMAT_DEFAULT_FLAG_VALUES
         6.  D_ARG_FORMAT_TERMINAL_WIDTH
         7.  D_ARG_FORMAT_DEFAULTS
2.  RECORDS
    -------
    1.  The steps
         1.  d_arg_formatter_vtable
    2.  The formatter
         1.  d_arg_formatter
3.  FORMATTERS
    ----------
    1.  Lifetime
         1.  d_arg_formatter_init / d_arg_formatter_release
         2.  d_arg_formatter_copy
    2.  Settings
         1.  d_arg_formatter_set_label
         2.  d_arg_formatter_label
         3.  d_arg_formatter_set_column_width and kin
         4.  d_arg_formatter_set_long_option_alignment_ratio
         5.  d_arg_formatter_set_flag
    3.  Resolution
         1.  d_arg_command_formatter
4.  STEPS
    -----
    1.  Dispatch
         1.  d_arg_format_help and kin
    2.  The default steps
         1.  d_arg_format_help_default and kin
    3.  Groups
         1.  d_arg_format_groups
5.  HELP
    ----
    1.  Producing help
         1.  d_arg_help
    2.  Terminal
         1.  d_arg_terminal_width
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_FORMATTER_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_FORMATTER_H 1

// std
#include <stddef.h>                          // size_t
// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "../../../../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "./arg_common.h"                    // d_arg_buffer, lists, statuses
#include "./arg_command.h"                   // d_arg_command
#include "./arg_option.h"                    // d_arg_option
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Formatter flags
//------------------------------------------------------------------------------
//   constant: the switches of d_arg_formatter's `flags`, CLI11's enable_*
// settings and one of this module's own.

// 1.1.1
// D_ARG_FORMAT_DESCRIPTION_PARAGRAPHS
//   constant: a command's description is wrapped as a paragraph; CLI11's
// enable_description_formatting.
#define D_ARG_FORMAT_DESCRIPTION_PARAGRAPHS 0x0001u

// 1.1.2
// D_ARG_FORMAT_FOOTER_PARAGRAPHS
//   constant: the footer is wrapped as a paragraph; CLI11's
// enable_footer_formatting.
#define D_ARG_FORMAT_FOOTER_PARAGRAPHS      0x0002u

// 1.1.3
// D_ARG_FORMAT_OPTION_DEFAULTS
//   constant: an option's line shows its default; CLI11's
// enable_option_defaults.
#define D_ARG_FORMAT_OPTION_DEFAULTS        0x0004u

// 1.1.4
// D_ARG_FORMAT_OPTION_TYPE_NAMES
//   constant: an option's line shows its type name; CLI11's
// enable_option_type_names.
#define D_ARG_FORMAT_OPTION_TYPE_NAMES      0x0008u

// 1.1.5
// D_ARG_FORMAT_DEFAULT_FLAG_VALUES
//   constant: a flag's names show their `{value}`; CLI11's
// enable_default_flag_values.
#define D_ARG_FORMAT_DEFAULT_FLAG_VALUES    0x0010u

// 1.1.6
// D_ARG_FORMAT_TERMINAL_WIDTH
//   constant: help keeps within the terminal standard output is shown on,
// measured each time help is made: the formatter's `line_width` is taken as
// one less than the terminal's width -- its last column left free -- when
// that is narrower. Off by default; where the width cannot be measured, or
// without D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH, it does nothing.
#define D_ARG_FORMAT_TERMINAL_WIDTH         0x0020u

// 1.1.7
// D_ARG_FORMAT_DEFAULTS
//   constant: the switches a formatter starts with: CLI11's five.
#define D_ARG_FORMAT_DEFAULTS                                                  \
    (D_ARG_FORMAT_DESCRIPTION_PARAGRAPHS |                                     \
     D_ARG_FORMAT_FOOTER_PARAGRAPHS      |                                     \
     D_ARG_FORMAT_OPTION_DEFAULTS        |                                     \
     D_ARG_FORMAT_OPTION_TYPE_NAMES      |                                     \
     D_ARG_FORMAT_DEFAULT_FLAG_VALUES)


//==============================================================================
// 2.  RECORDS
//==============================================================================


// 2.1    The steps
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_formatter_vtable
//   struct: CLI11's virtual make_* functions, each appending its piece of help
// to `_out` and returning D_ARG_OK or the status that stopped it. A NULL
// entry runs the default step of section 4.2. `_mode` is a d_arg_help_mode
// value, and `_name` the command's name as help shows it.
struct d_arg_formatter_vtable
{
    int (*make_help)(const struct d_arg_formatter*    _formatter,
                     const struct d_arg_command*      _command,
                     struct d_pack_text               _name,
                     int                              _mode,
                     struct d_arg_buffer*             _out);
    int (*make_group)(const struct d_arg_formatter*   _formatter,
                      struct d_pack_text              _group,
                      bool                            _positional,
                      const struct d_arg_option_list* _options,
                      struct d_arg_buffer*            _out);
    int (*make_positionals)(const struct d_arg_formatter* _formatter,
                            const struct d_arg_command*   _command,
                            struct d_arg_buffer*          _out);
    int (*make_subcommands)(const struct d_arg_formatter* _formatter,
                            const struct d_arg_command*   _command,
                            int                           _mode,
                            struct d_arg_buffer*          _out);
    int (*make_subcommand)(const struct d_arg_formatter* _formatter,
                           const struct d_arg_command*   _command,
                           struct d_arg_buffer*          _out);
    int (*make_expanded)(const struct d_arg_formatter* _formatter,
                         const struct d_arg_command*   _command,
                         int                           _mode,
                         struct d_arg_buffer*          _out);
    int (*make_footer)(const struct d_arg_formatter* _formatter,
                       const struct d_arg_command*   _command,
                       struct d_arg_buffer*          _out);
    int (*make_description)(const struct d_arg_formatter* _formatter,
                            const struct d_arg_command*   _command,
                            struct d_arg_buffer*          _out);
    int (*make_usage)(const struct d_arg_formatter* _formatter,
                      const struct d_arg_command*   _command,
                      struct d_pack_text            _name,
                      struct d_arg_buffer*          _out);
    int (*make_option)(const struct d_arg_formatter* _formatter,
                       const struct d_arg_option*    _option,
                       bool                          _positional,
                       struct d_arg_buffer*          _out);
    int (*make_option_name)(const struct d_arg_formatter* _formatter,
                            const struct d_arg_option*    _option,
                            bool                          _positional,
                            struct d_arg_buffer*          _out);
    int (*make_option_opts)(const struct d_arg_formatter* _formatter,
                            const struct d_arg_option*    _option,
                            struct d_arg_buffer*          _out);
    int (*make_option_desc)(const struct d_arg_formatter* _formatter,
                            const struct d_arg_option*    _option,
                            struct d_arg_buffer*          _out);
    int (*make_option_usage)(const struct d_arg_formatter* _formatter,
                             const struct d_arg_option*    _option,
                             struct d_arg_buffer*          _out);
};

// 2.2    The formatter
//------------------------------------------------------------------------------
// 2.2.1
// d_arg_formatter
//   struct: CLI11's FormatterBase settings with the step table. `vtable` and
// `context` are borrowed: the table usually static, the context whatever
// the table's steps need. `label_keys` and `label_values` are parallel,
// owned copies; a word with no label is shown as itself. The widths start
// at the configured D_INTERNAL_ARG_PARSER_* sizes and the ratio at 1/3.
//   `line_width`, beyond CLI11, is the longest line help is to write: the
// right column and the paragraphs are narrowed so that the lines they make
// keep within it -- an option group's, indented, within what the indent
// leaves -- though a word longer than the room, or an option's names, still
// pass it. 0, the default, sets no such limit: CLI11's layout.
//   Public-payload layout: read any field; change the labels only through
// d_arg_formatter_set_label.
struct d_arg_formatter
{
    const struct d_arg_formatter_vtable* vtable;
    void*                                context;
    struct d_arg_string_list             label_keys;
    struct d_arg_string_list             label_values;
    size_t                               column_width;
    size_t                               right_column_width;
    size_t                               description_paragraph_width;
    size_t                               footer_paragraph_width;
    size_t                               line_width;
    float                                long_option_alignment_ratio;
    uint32_t                             flags;
};


D_EXTERN_C_BEGIN

//==============================================================================
// 3.  FORMATTERS
//==============================================================================


// 3.1    Lifetime
//------------------------------------------------------------------------------
// 3.1.1
// d_arg_formatter_init / d_arg_formatter_release
//   lifetime: init makes CLI11's default Formatter, with no step replaced and
// no labels; release frees the labels and leaves it as init makes it.
// Neither fails, and each ignores NULL.
void        d_arg_formatter_init(struct d_arg_formatter* _formatter);
void        d_arg_formatter_release(struct d_arg_formatter* _formatter);

// 3.1.2
// d_arg_formatter_copy
/**
 * @brief Copies a formatter's settings, labels included, into another.
 *
 * @param[out] _destination  an initialized formatter, whose labels are
 *                           released first.
 * @param[in]  _source       the formatter to copy.
 * @retval D_ARG_OK                   copied.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  a label could not be copied;
 *                                    `_destination` holds the settings and
 *                                    no labels.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   an argument is NULL.
 */
int         d_arg_formatter_copy(struct d_arg_formatter*       _destination,
                                 const struct d_arg_formatter* _source);

// 3.2    Settings
//------------------------------------------------------------------------------
// 3.2.1
// d_arg_formatter_set_label
/**
 * @brief Translates one of help's fixed words; CLI11's label().
 *
 * @note The words are "OPTIONS", "POSITIONALS", "SUBCOMMAND",
 *       "SUBCOMMANDS", "REQUIRED", "Usage", "Env", "Needs" and "Excludes",
 *       and every type name, which help looks up before showing.
 *
 * @param[in,out] _formatter  the formatter.
 * @param[in]     _key        the word, copied.
 * @param[in]     _value      what to show instead, copied.
 * @retval D_ARG_OK                   set, replacing any earlier label.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  a copy could not be made; the labels
 *                                    are as they were.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   an argument is NULL.
 */
int         d_arg_formatter_set_label(struct d_arg_formatter* _formatter,
                                      const char*             _key,
                                      const char*             _value);

// 3.2.2
// d_arg_formatter_label
//   lookup: CLI11's get_label(): the word's label, or the word itself when it
// has none or the formatter is NULL. The span is the formatter's or
// `_key`'s, valid while both are.
struct d_pack_text
            d_arg_formatter_label(const struct d_arg_formatter* _formatter,
                                  struct d_pack_text            _key);

// 3.2.3
// d_arg_formatter_set_column_width and kin
//   settings: CLI11's column_width(), right_column_width(),
// description_paragraph_width() and footer_paragraph_width(), and the line
// width, beyond CLI11 (0 for none). None fails, and each ignores NULL.
void d_arg_formatter_set_column_width(struct d_arg_formatter* _formatter,
                                      size_t                  _width);
void d_arg_formatter_set_right_column_width(struct d_arg_formatter* _formatter,
                                            size_t                  _width);
void d_arg_formatter_set_description_paragraph_width(struct d_arg_formatter* _formatter,
                                                     size_t                  _width);
void d_arg_formatter_set_footer_paragraph_width(struct d_arg_formatter* _formatter,
                                                size_t                  _width);
void d_arg_formatter_set_line_width(struct d_arg_formatter* _formatter,
                                    size_t                  _width);

// 3.2.4
// d_arg_formatter_set_long_option_alignment_ratio
//   setting: CLI11's long_option_alignment_ratio(): where long names start,
// as a fraction of the left column; a ratio above 1 is inverted and a
// negative one taken by its magnitude, as CLI11 does, and one that is not a
// number restores the default, 1/3. A ratio written to the field directly
// is read as the setter would have stored it.
void d_arg_formatter_set_long_option_alignment_ratio(struct d_arg_formatter* _formatter,
                                                     float                   _ratio);

// 3.2.5
// d_arg_formatter_set_flag
//   setting: sets or clears one D_ARG_FORMAT_* switch. Does not fail.
void        d_arg_formatter_set_flag(struct d_arg_formatter* _formatter,
                                     uint32_t                _flag,
                                     bool                    _value);

// 3.3    Resolution
//------------------------------------------------------------------------------
// 3.3.1
// d_arg_command_formatter
//   lookup: the formatter a command's help uses: its own, else the nearest
// ancestor's, else its parser's; NULL for NULL.
const struct d_arg_formatter*
            d_arg_command_formatter(const struct d_arg_command* _command);


//==============================================================================
// 4.  STEPS
//==============================================================================
// Each step appends to `_out` and returns D_ARG_OK, the buffer's status, or
// the status of a function it called (a usage or footer function, a type
// name function), what it appended before failing left in place; each
// returns D_ARG_ERROR_BAD_ARGUMENT for a NULL formatter, command, option,
// list or buffer, or a mode that is not a d_arg_help_mode value.


// 4.1    Dispatch
//------------------------------------------------------------------------------
// 4.1.1
// d_arg_format_help and kin
//   steps: run the formatter's table entry for the step, or the default step
// when the entry is NULL. Help is built only through these, so a replaced
// step is used wherever CLI11 would call it.
int         d_arg_format_help(const struct d_arg_formatter* _formatter,
                              const struct d_arg_command*   _command,
                              struct d_pack_text            _name,
                              int                           _mode,
                              struct d_arg_buffer*          _out);
int         d_arg_format_group(const struct d_arg_formatter*   _formatter,
                               struct d_pack_text              _group,
                               bool                            _positional,
                               const struct d_arg_option_list* _options,
                               struct d_arg_buffer*            _out);
int         d_arg_format_positionals(const struct d_arg_formatter* _formatter,
                                     const struct d_arg_command*   _command,
                                     struct d_arg_buffer*          _out);
int         d_arg_format_subcommands(const struct d_arg_formatter* _formatter,
                                     const struct d_arg_command*   _command,
                                     int                           _mode,
                                     struct d_arg_buffer*          _out);
int         d_arg_format_subcommand(const struct d_arg_formatter* _formatter,
                                    const struct d_arg_command*   _command,
                                    struct d_arg_buffer*          _out);
int         d_arg_format_expanded(const struct d_arg_formatter* _formatter,
                                  const struct d_arg_command*   _command,
                                  int                           _mode,
                                  struct d_arg_buffer*          _out);
int         d_arg_format_footer(const struct d_arg_formatter* _formatter,
                                const struct d_arg_command*   _command,
                                struct d_arg_buffer*          _out);
int         d_arg_format_description(const struct d_arg_formatter* _formatter,
                                     const struct d_arg_command*   _command,
                                     struct d_arg_buffer*          _out);
int         d_arg_format_usage(const struct d_arg_formatter* _formatter,
                               const struct d_arg_command*   _command,
                               struct d_pack_text            _name,
                               struct d_arg_buffer*          _out);
int         d_arg_format_option(const struct d_arg_formatter* _formatter,
                                const struct d_arg_option*    _option,
                                bool                          _positional,
                                struct d_arg_buffer*          _out);
int         d_arg_format_option_name(const struct d_arg_formatter* _formatter,
                                     const struct d_arg_option*    _option,
                                     bool                          _positional,
                                     struct d_arg_buffer*          _out);
int         d_arg_format_option_opts(const struct d_arg_formatter* _formatter,
                                     const struct d_arg_option*    _option,
                                     struct d_arg_buffer*          _out);
int         d_arg_format_option_desc(const struct d_arg_formatter* _formatter,
                                     const struct d_arg_option*    _option,
                                     struct d_arg_buffer*          _out);
int         d_arg_format_option_usage(const struct d_arg_formatter* _formatter,
                                      const struct d_arg_option*    _option,
                                      struct d_arg_buffer*          _out);

// 4.2    The default steps
//------------------------------------------------------------------------------
// 4.2.1
// d_arg_format_help_default and kin
//   steps: CLI11's Formatter implementations, one for each table entry, with
// the same parameters. They call the other steps through the dispatchers of
// 4.1, as CLI11's virtual calls do.
int  d_arg_format_help_default(const struct d_arg_formatter* _formatter,
                               const struct d_arg_command*   _command,
                               struct d_pack_text            _name,
                               int                           _mode,
                               struct d_arg_buffer*          _out);
int  d_arg_format_group_default(const struct d_arg_formatter*   _formatter,
                                struct d_pack_text              _group,
                                bool                            _positional,
                                const struct d_arg_option_list* _options,
                                struct d_arg_buffer*            _out);
int  d_arg_format_positionals_default(const struct d_arg_formatter* _formatter,
                                      const struct d_arg_command*   _command,
                                      struct d_arg_buffer*          _out);
int  d_arg_format_subcommands_default(const struct d_arg_formatter* _formatter,
                                      const struct d_arg_command*   _command,
                                      int                           _mode,
                                      struct d_arg_buffer*          _out);
int  d_arg_format_subcommand_default(const struct d_arg_formatter* _formatter,
                                     const struct d_arg_command*   _command,
                                     struct d_arg_buffer*          _out);
int  d_arg_format_expanded_default(const struct d_arg_formatter* _formatter,
                                   const struct d_arg_command*   _command,
                                   int                           _mode,
                                   struct d_arg_buffer*          _out);
int  d_arg_format_footer_default(const struct d_arg_formatter* _formatter,
                                 const struct d_arg_command*   _command,
                                 struct d_arg_buffer*          _out);
int  d_arg_format_description_default(const struct d_arg_formatter* _formatter,
                                      const struct d_arg_command*   _command,
                                      struct d_arg_buffer*          _out);
int  d_arg_format_usage_default(const struct d_arg_formatter* _formatter,
                                const struct d_arg_command*   _command,
                                struct d_pack_text            _name,
                                struct d_arg_buffer*          _out);
int  d_arg_format_option_default(const struct d_arg_formatter* _formatter,
                                 const struct d_arg_option*    _option,
                                 bool                          _positional,
                                 struct d_arg_buffer*          _out);
int  d_arg_format_option_name_default(const struct d_arg_formatter* _formatter,
                                      const struct d_arg_option*    _option,
                                      bool                          _positional,
                                      struct d_arg_buffer*          _out);
int  d_arg_format_option_opts_default(const struct d_arg_formatter* _formatter,
                                      const struct d_arg_option*    _option,
                                      struct d_arg_buffer*          _out);
int  d_arg_format_option_desc_default(const struct d_arg_formatter* _formatter,
                                      const struct d_arg_option*    _option,
                                      struct d_arg_buffer*          _out);
int  d_arg_format_option_usage_default(const struct d_arg_formatter* _formatter,
                                       const struct d_arg_option*    _option,
                                       struct d_arg_buffer*          _out);

// 4.3    Groups
//------------------------------------------------------------------------------
// 4.3.1
// d_arg_format_groups
//   step: CLI11's make_groups, which is not replaceable: one group step for
// each named group of the command's non-positional options, the help flags
// left out in D_ARG_HELP_SUB mode.
int         d_arg_format_groups(const struct d_arg_formatter* _formatter,
                                const struct d_arg_command*   _command,
                                int                           _mode,
                                struct d_arg_buffer*          _out);


//==============================================================================
// 5.  HELP
//==============================================================================


// 5.1    Producing help
//------------------------------------------------------------------------------
// 5.1.1
// d_arg_help
/**
 * @brief Writes a command's help; CLI11's help(prev, mode).
 *
 * @note The help of the last subcommand the last parse used, when there is
 *       one -- so `prog sub --help` shows the subcommand's help -- built by
 *       that command's formatter, the name preceded by `_prefix` and the
 *       names of the commands between. A formatter with
 *       D_ARG_FORMAT_TERMINAL_WIDTH measures the terminal first. The text
 *       is built whole before any of it is written.
 *
 * @param[in] _command  the command.
 * @param[in] _prefix   the words help shows before the command's name;
 *                      empty for none.
 * @param[in] _mode     a d_arg_help_mode value.
 * @param[in] _sink     where the help goes.
 * @return D_ARG_OK when written; D_ARG_ERROR_FILE when the sink refused the
 *         text; D_ARG_ERROR_OUT_OF_MEMORY when it could not be built; the
 *         status of a usage, footer or type-name function that failed,
 *         nothing written; or D_ARG_ERROR_BAD_ARGUMENT for a NULL command or
 *         a mode that is not a d_arg_help_mode value.
 */
int         d_arg_help(const struct d_arg_command* _command,
                       struct d_pack_text          _prefix,
                       int                         _mode,
                       struct d_pack_sink          _sink);

#if (D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH == 1)

// 5.2    Terminal
//------------------------------------------------------------------------------
// 5.2.1
// d_arg_terminal_width
/**
 * @brief Measures the terminal standard output is shown on.
 *
 * @note Standard output must be a terminal, measured with
 *       D_ENV_ARG_PARSER_TERMINAL's interface; otherwise, or where there is
 *       none, the COLUMNS environment variable is read, when it holds a
 *       number from 1 to 10000.
 *
 * @return the width in columns; 0 when it cannot be known.
 */
size_t      d_arg_terminal_width(void);

#endif  // D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_FORMATTER_H
