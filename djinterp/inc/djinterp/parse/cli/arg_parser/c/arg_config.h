/*******************************************************************************
* djinterp [parse]                                                  arg_config.h
*
* Configuration files: TOML and INI read into options, and written from them.
*   A `struct d_arg_config_format` is CLI11's ConfigBase in C: the characters
* that mark comments, arrays, values, quotes and nested sections, the
* section and index to read, and a table of the three operations a format
* performs -- reading text into items, writing a command's values as text,
* and reading an item as a flag -- whose NULL entries run CLI11's own.
* d_arg_config_format_init makes CLI11's ConfigTOML, the default, and
* d_arg_config_format_init_ini its ConfigINI.
*   A `struct d_arg_config_item` is one entry read: its section path, its
* name, and its values. A command's config option (d_arg_command_set_config)
* reads files through its format during the parse; d_arg_command_config_to_
* sink writes the values a parse produced, CLI11's config_to_str.
*   The records exist in every build, as does reading an item as a flag,
* which the parse asks of a positional expecting no values; reading and
* writing text and files exist only with D_INTERNAL_ARG_PARSER_CONFIG_FILES.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Format flags
         1.  D_ARG_CONFIG_COMMENT_DEFAULTS
         2.  D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS
2.  RECORDS
    -------
    1.  Items
         1.  d_arg_config_item
         2.  D_ARG_CONFIG_ITEM_INIT
         3.  d_arg_config_item_list
    2.  Formats
         1.  d_arg_config_format_vtable
         2.  d_arg_config_format
3.  FORMATS
    -------
    1.  Lifetime
         1.  d_arg_config_format_init / d_arg_config_format_init_ini
         2.  d_arg_config_format_release
    2.  Settings
         1.  d_arg_config_format_set_section
         2.  d_arg_config_format_set_index and kin
    3.  Resolution
         1.  d_arg_command_config_format
4.  ITEMS
    -----
    1.  Lifetime
         1.  d_arg_config_item_release / d_arg_config_item_list_release
    2.  Building
         1.  d_arg_config_item_set_name / d_arg_config_item_add_parent and kin
         2.  d_arg_config_item_list_adopt
    3.  Names
         1.  d_arg_config_item_fullname
    4.  Flags
         1.  d_arg_config_to_flag / d_arg_config_to_flag_default
5.  READING AND WRITING
    -------------------
    1.  Dispatch
         1.  d_arg_config_from_text
         2.  d_arg_config_from_file
         3.  d_arg_config_to_text
    2.  The default operations
         1.  d_arg_config_from_text_default / d_arg_config_to_text_default
    3.  Commands
         1.  d_arg_command_config_to_sink
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_CONFIG_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_CONFIG_H 1

// std
#include <stddef.h>                          // size_t
// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "../../../../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "./arg_common.h"                    // d_arg_buffer, lists, statuses
#include "./arg_command.h"                   // d_arg_command
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Format flags
//------------------------------------------------------------------------------
//   constant: the switches of d_arg_config_format's `flags`.

// 1.1.1
// D_ARG_CONFIG_COMMENT_DEFAULTS
//   constant: a written default value is commented out; CLI11's
// commentDefaults().
#define D_ARG_CONFIG_COMMENT_DEFAULTS       0x0001u

// 1.1.2
// D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS
//   constant: a name given again in the same section adds its values to the
// first, wherever it appeared; CLI11's allowDuplicateFields().
#define D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS 0x0002u


//==============================================================================
// 2.  RECORDS
//==============================================================================


// 2.1    Items
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_config_item
//   struct: CLI11's ConfigItem: the sections enclosing the entry, outermost
// first, its name, and its values. Each span's bytes are copies `storage`
// owns, NUL-terminated at `data[length]`, which may hold NUL bytes of their
// own, as an escape or a binary string makes them. The names `++` and `--`
// mark a section's start and end, which open and close a subcommand.
// `multiline` records that the values hold `%%` separators between groups.
//   Public-payload layout: read `parents`, `name` and `inputs`, and read or
// set `multiline`; change the rest only through the d_arg_config_item_*
// calls, which copy what they are given into `storage`.
struct d_arg_config_item
{
    struct d_arg_text_list   parents;
    struct d_pack_text       name;
    struct d_arg_text_list   inputs;
    bool                     multiline;
    struct d_arg_string_list storage;
};

// 2.1.2
// D_ARG_CONFIG_ITEM_INIT
//   constant: the initializer of an empty item that owns nothing: no
// sections, the empty name, no values.
#define D_ARG_CONFIG_ITEM_INIT                                                 \
    { D_ARG_LIST_INIT, { "", 0u }, D_ARG_LIST_INIT, false, D_ARG_LIST_INIT }

// 2.1.3
// d_arg_config_item_list
//   struct: the items a read produced, in order, owned by the list.
struct d_arg_config_item_list
{
    struct d_arg_config_item* items;
    size_t                    count;
    size_t                    capacity;
};

// 2.2    Formats
//------------------------------------------------------------------------------
// 2.2.1
// d_arg_config_format_vtable
//   struct: CLI11's Config virtual functions. from_config reads `_input` into
// items appended to `_out`, describing a failure in `_error`; to_config
// appends a command's values, each name preceded by `_prefix`, with
// `_mode` a d_arg_config_output_mode value; to_flag appends the text a
// flag-like option takes from an item. Each returns D_ARG_OK or the status
// that stopped it; a NULL entry runs the default operation of 4.4.1 or
// 5.2. A from_config builds its items as 4.2 describes.
struct d_arg_config_format_vtable
{
    int (*from_config)(const struct d_arg_config_format* _format,
                       struct d_pack_text                _input,
                       struct d_arg_config_item_list*    _out,
                       struct d_arg_buffer*              _error);
    int (*to_config)(const struct d_arg_config_format* _format,
                     const struct d_arg_command*       _command,
                     int                               _mode,
                     bool                              _write_description,
                     struct d_pack_text                _prefix,
                     struct d_arg_buffer*              _out);
    int (*to_flag)(const struct d_arg_config_format* _format,
                   const struct d_arg_config_item*   _item,
                   struct d_arg_buffer*              _out);
};

// 2.2.2
// d_arg_config_format
//   struct: CLI11's ConfigBase settings with the operation table. `vtable` and
// `context` are borrowed; `section` is owned, NULL to read every section;
// `index` picks one occurrence of a repeated section, -1 for all;
// `max_layers` is how deeply sections may nest and still be read. A
// character set to '\0' disables what it marks.
//   Public-payload layout: read any field; change `section` only through
// d_arg_config_format_set_section.
struct d_arg_config_format
{
    const struct d_arg_config_format_vtable* vtable;
    void*                                    context;
    char*                                    section;
    int                                      index;
    int                                      max_layers;
    uint32_t                                 flags;
    char                                     comment;
    char                                     array_start;
    char                                     array_end;
    char                                     array_separator;
    char                                     value_delimiter;
    char                                     string_quote;
    char                                     literal_quote;
    char                                     parent_separator;
};


D_EXTERN_C_BEGIN

//==============================================================================
// 3.  FORMATS
//==============================================================================


// 3.1    Lifetime
//------------------------------------------------------------------------------
// 3.1.1
// d_arg_config_format_init / d_arg_config_format_init_ini
//   lifetime: init makes CLI11's ConfigTOML -- `#` comments, `[` `]` arrays
// separated by `,`, `=` between name and value, `"` and `'` quotes, `.`
// between section names, 255 layers -- and init_ini its ConfigINI, which
// comments with `;`, has no array brackets and separates values with
// spaces. Neither fails, and each ignores NULL.
void d_arg_config_format_init(struct d_arg_config_format* _format);
void d_arg_config_format_init_ini(struct d_arg_config_format* _format);

// 3.1.2
// d_arg_config_format_release
//   lifetime: frees the section name and leaves the format as init makes it.
// Ignores NULL.
void d_arg_config_format_release(struct d_arg_config_format* _format);

// 3.2    Settings
//------------------------------------------------------------------------------
// 3.2.1
// d_arg_config_format_set_section
/**
 * @brief Reads only one section of a file, as if it were the top level;
 *        CLI11's section().
 *
 * @param[in,out] _format   the format.
 * @param[in]     _section  the section's name, copied; NULL or "" reads
 *                          every section.
 * @retval D_ARG_OK                   set.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  the copy could not be made; the format
 *                                    is unchanged.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   `_format` is NULL.
 */
int  d_arg_config_format_set_section(struct d_arg_config_format* _format,
                                     const char*                 _section);

// 3.2.2
// d_arg_config_format_set_index and kin
//   settings: CLI11's index(), maxLayers() (0 to 255), commentDefaults() and
// allowDuplicateFields(). None fails.
void d_arg_config_format_set_index(struct d_arg_config_format* _format,
                                   int                         _index);
void d_arg_config_format_set_max_layers(struct d_arg_config_format* _format,
                                        int                         _layers);
void d_arg_config_format_set_flag(struct d_arg_config_format* _format,
                                  uint32_t                    _flag,
                                  bool                        _value);

// 3.3    Resolution
//------------------------------------------------------------------------------
// 3.3.1
// d_arg_command_config_format
//   lookup: the format a command reads and writes with: its own, else the
// nearest ancestor's, else its parser's; NULL for NULL.
const struct d_arg_config_format*
     d_arg_command_config_format(const struct d_arg_command* _command);


//==============================================================================
// 4.  ITEMS
//==============================================================================


// 4.1    Lifetime
//------------------------------------------------------------------------------
// 4.1.1
// d_arg_config_item_release / d_arg_config_item_list_release
//   lifetime: item_release frees an item's lists and copies, leaving it as
// D_ARG_CONFIG_ITEM_INIT makes one; list_release frees every item and the
// list's storage, leaving it empty. Each ignores NULL.
void d_arg_config_item_release(struct d_arg_config_item* _item);
void d_arg_config_item_list_release(struct d_arg_config_item_list* _items);

// 4.2    Building
//------------------------------------------------------------------------------
//   How a format that reads its own way -- a table's from_config -- makes the
// items it appends: each started as D_ARG_CONFIG_ITEM_INIT makes it, named,
// given its sections and values, then adopted by the list.

// 4.2.1
// d_arg_config_item_set_name / d_arg_config_item_add_parent and kin
/**
 * @brief Names an item, or appends a section to its sections, innermost
 *        last, or a value to its values.
 *
 * @param[in,out] _item  the item.
 * @param[in]     _text  the name, section or value, copied; it may hold NUL
 *                       bytes.
 * @retval D_ARG_OK                   set or appended.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  the copy could not be made, or the list
 *                                    could not grow; the item is unchanged.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   `_item` is NULL, or `_text` has a length
 *                                    but no data.
 */
int  d_arg_config_item_set_name(struct d_arg_config_item* _item,
                                struct d_pack_text        _text);
int  d_arg_config_item_add_parent(struct d_arg_config_item* _item,
                                  struct d_pack_text        _text);
int  d_arg_config_item_add_input(struct d_arg_config_item* _item,
                                 struct d_pack_text        _text);

// 4.2.2
// d_arg_config_item_list_adopt
/**
 * @brief Appends an item to a list, which takes everything it holds.
 *
 * @param[in,out] _items  the list.
 * @param[in,out] _item   the item; left as D_ARG_CONFIG_ITEM_INIT makes one
 *                        once adopted.
 * @retval D_ARG_OK                   appended.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  the list could not grow; the list and
 *                                    the item are unchanged.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   an argument is NULL.
 */
int  d_arg_config_item_list_adopt(struct d_arg_config_item_list* _items,
                                  struct d_arg_config_item*      _item);

// 4.3    Names
//------------------------------------------------------------------------------
// 4.3.1
// d_arg_config_item_fullname
//   writing: CLI11's fullname(): the parents and the name joined by `.`.
// Returns D_ARG_OK or the buffer's status.
int  d_arg_config_item_fullname(const struct d_arg_config_item* _item,
                                struct d_arg_buffer*            _out);

// 4.4    Flags
//------------------------------------------------------------------------------
// 4.4.1
// d_arg_config_to_flag / d_arg_config_to_flag_default
/**
 * @brief Writes the text a flag-like option takes from an item through a
 *        format's table; CLI11's to_flag. In every build: the parse asks it
 *        of a positional that expects no values.
 *
 * @param[in]     _format  the format.
 * @param[in]     _item    the item.
 * @param[in,out] _out     receives the text, appended: the single value,
 *                         or `{}` for none.
 * @return D_ARG_OK, or the buffer's status; D_ARG_ERROR_CONVERSION for an
 *         item with more than one value, "<name>: too many inputs for a
 *         flag"; D_ARG_ERROR_BAD_ARGUMENT for a NULL argument.
 */
int  d_arg_config_to_flag(const struct d_arg_config_format* _format,
                          const struct d_arg_config_item*   _item,
                          struct d_arg_buffer*              _out);
/**
 * @brief CLI11's Config::to_flag itself, which d_arg_config_to_flag runs
 *        for a table without the entry, for a replacement to call.
 *
 * @param[in]     _format  the format.
 * @param[in]     _item    the item.
 * @param[in,out] _out     receives the text, appended.
 * @return a status, as d_arg_config_to_flag's.
 */
int  d_arg_config_to_flag_default(const struct d_arg_config_format* _format,
                                  const struct d_arg_config_item*   _item,
                                  struct d_arg_buffer*              _out);


#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

//==============================================================================
// 5.  READING AND WRITING
//==============================================================================


// 5.1    Dispatch
//------------------------------------------------------------------------------
// 5.1.1
// d_arg_config_from_text
/**
 * @brief Reads configuration text into items; CLI11's from_config.
 *
 * @param[in]     _format  the format.
 * @param[in]     _input   the text.
 * @param[in,out] _out     receives the items, appended.
 * @param[in,out] _error   receives the description of a failure, appended.
 * @retval D_ARG_OK                   read.
 * @retval D_ARG_ERROR_PARSE          an escape is malformed: CLI11's
 *                                    ParseError.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  an item could not be stored; the items
 *                                    read before it stay.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   an argument is NULL.
 */
int  d_arg_config_from_text(const struct d_arg_config_format* _format,
                            struct d_pack_text                _input,
                            struct d_arg_config_item_list*    _out,
                            struct d_arg_buffer*              _error);

// 5.1.2
// d_arg_config_from_file
/**
 * @brief Reads a configuration file into items; CLI11's from_file.
 *
 * @note The file is read as text, as CLI11's std::ifstream reads it: on
 *       Windows a carriage return before a line feed is dropped, and a
 *       Ctrl-Z ends the text. On Windows the path is UTF-8, or, when it is
 *       not valid UTF-8, in the ANSI code page.
 *
 * @param[in]     _format  the format.
 * @param[in]     _path    the file's path.
 * @param[in,out] _out     receives the items, appended.
 * @param[in,out] _error   receives the description of a failure, appended.
 * @return a status, as d_arg_config_from_text's; D_ARG_ERROR_FILE for a file
 *         that cannot be read: "<path> was not readable (missing?)".
 */
int  d_arg_config_from_file(const struct d_arg_config_format* _format,
                            const char*                       _path,
                            struct d_arg_config_item_list*    _out,
                            struct d_arg_buffer*              _error);

// 5.1.3
// d_arg_config_to_text
/**
 * @brief Writes a command's values as configuration text; CLI11's
 *        to_config.
 *
 * @note The command's own configurable options are written group by group,
 *       each with the values its policy keeps -- or, for the modes that ask
 *       for defaults, its default -- then its option groups and other
 *       nameless subcommands, then its named subcommands, as sections of
 *       their own when configurable and used. Values not yet reduced are
 *       reduced on a copy, as CLI11's reduced_results reduces them, and a
 *       default's values are kept by the option, as CLI11's results(T&)
 *       keeps them; nothing else about the options changes.
 *
 * @param[in]     _format             the format.
 * @param[in]     _command            the command.
 * @param[in]     _mode               a d_arg_config_output_mode value.
 * @param[in]     _write_description  whether descriptions are written as
 *                                    comments.
 * @param[in]     _prefix             what precedes each name; empty for
 *                                    none.
 * @param[in,out] _out                receives the text, appended.
 * @return D_ARG_OK, or the status of the buffer or of an option whose values
 *         could not be validated or reduced, recorded as a parse records
 *         it; D_ARG_ERROR_BAD_ARGUMENT for a NULL argument or a mode that is
 *         none of d_arg_config_output_mode's.
 */
int  d_arg_config_to_text(const struct d_arg_config_format* _format,
                          const struct d_arg_command*       _command,
                          int                               _mode,
                          bool                              _write_description,
                          struct d_pack_text                _prefix,
                          struct d_arg_buffer*              _out);

// 5.2    The default operations
//------------------------------------------------------------------------------
// 5.2.1
// d_arg_config_from_text_default / d_arg_config_to_text_default
//   operations: CLI11's ConfigBase implementations of the reading and
// writing table entries, for a replacement to call, with the same
// parameters and statuses.
int  d_arg_config_from_text_default(const struct d_arg_config_format* _format,
                                    struct d_pack_text                _input,
                                    struct d_arg_config_item_list*    _out,
                                    struct d_arg_buffer*              _error);
int  d_arg_config_to_text_default(const struct d_arg_config_format* _format,
                                  const struct d_arg_command*       _command,
                                  int                               _mode,
                                  bool                              _write_description,
                                  struct d_pack_text                _prefix,
                                  struct d_arg_buffer*              _out);

// 5.3    Commands
//------------------------------------------------------------------------------
// 5.3.1
// d_arg_command_config_to_sink
/**
 * @brief Writes a command's values through its format; CLI11's
 *        config_to_str.
 *
 * @param[in] _command            the command.
 * @param[in] _mode               a d_arg_config_output_mode value.
 * @param[in] _write_description  whether descriptions are written as
 *                                comments.
 * @param[in] _sink               where the text goes.
 * @return D_ARG_OK, D_ARG_ERROR_FILE when the sink refused the text, or a
 *         status of d_arg_config_to_text.
 */
int  d_arg_command_config_to_sink(const struct d_arg_command* _command,
                                  int                         _mode,
                                  bool                        _write_description,
                                  struct d_pack_sink          _sink);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_CONFIG_H
