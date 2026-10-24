/*******************************************************************************
* djinterp [parse]                                                  cli_format.c
*
* Help as GNU programs lay it out, declared in cli_format.h.
*   One step builds the whole page: the usage line, the description as a
* sentence, then the sections -- Arguments, Commands, each group of options,
* and the root's options a subcommand takes -- and the footer; the root's page
* ends with the exit statuses (GRAMMAR.md, section 8). Every entry of every
* section is laid out in two columns, its name on the left and its
* description on the right, the right column starting where the page's widest
* name needs and never past D_INTERNAL_CLI_HELP_COLUMN_MAX: a name past it puts
* its description on the next line. Text is wrapped at spaces to the page's
* width; a description's own line breaks are kept, each new line starting at
* the right column. Widths are counted in code points, which the style
* guide's single-width rule makes columns.
*   The formatter's context, when the kit made the program, is the program,
* which says whether a command has work of its own: a command without work
* requires one of its subcommands, and its usage line says so.
*
*
* path:      /src/djinterp/parse/cli/c/cli_format.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/
#include "../../../../../inc/djinterp/parse/cli/c/cli_format.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdlib.h>   // the hooks' malloc, realloc and free
#include <string.h>   // memset, strlen, strcmp
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options
#include "../../../../../inc/djinterp/parse/cli/c/cli_io.h"  // d_cli_is_terminal
#include "../../../../../inc/djinterp/parse/cli/c/cli_program.h"  // d_cli_program
#include "./cli_internal.h"  // the terminal's width


//==============================================================================
// 1.  WIDTHS
//==============================================================================

/*
d_cli_help_width
  A terminal's columns, held between 40 and the most help is wrapped to; a
page bound for anything else, the fixed width, so a captured page does not
depend on the window it was captured in.
*/
size_t
d_cli_help_width(void)
{
    // not a terminal: the fixed width
    if (!d_cli_is_terminal((int)D_CLI_STREAM_OUTPUT))
    {
        return (size_t)D_INTERNAL_CLI_HELP_WIDTH;
    }

    const size_t columns = d_cli_terminal_columns();

    // a terminal that will not say
    if (columns == 0u)
    {
        return (size_t)D_INTERNAL_CLI_HELP_WIDTH;
    }

    return (columns < 40u)
           ? 40u
           : (columns > (size_t)D_INTERNAL_CLI_HELP_WIDTH_MAX)
             ? (size_t)D_INTERNAL_CLI_HELP_WIDTH_MAX
             : columns;
}


#if (D_INTERNAL_CLI_FORMATTER == 1)

//==============================================================================
// 2.  TEXT
//==============================================================================

/**
 * @brief How many columns a span takes: its code points, every byte that
 *        does not continue a UTF-8 sequence.
 *
 * @param[in] _text    the text.
 * @param[in] _length  its length in bytes.
 * @return its columns.
 */
static size_t
d_cli_internal_columns(
    const char* _text,
    size_t      _length
)
{
    size_t columns = 0u;

    for (size_t index = 0u; index < _length; ++index)
    {
        if (((unsigned char)_text[index] & 0xC0u) != 0x80u)
        {
            ++columns;
        }
    }

    return columns;
}

/**
 * @brief Appends spaces.
 *
 * @param[in,out] _out    the buffer.
 * @param[in]     _count  how many.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_spaces(
    struct d_arg_buffer* _out,
    size_t               _count
)
{
    int status = D_ARG_OK;

    for (size_t index = 0u; (status == D_ARG_OK) && (index < _count); ++index)
    {
        status = d_arg_buffer_append_char(_out,
                                          ' ');
    }

    return status;
}

/**
 * @brief Appends a line break and the next line's indent.
 *
 * @param[in,out] _out     the buffer.
 * @param[in]     _indent  the column the next line starts at.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_new_line(
    struct d_arg_buffer* _out,
    size_t               _indent
)
{
    const int status = d_arg_buffer_append_char(_out,
                                                '\n');

    return (status == D_ARG_OK)
           ? d_cli_internal_spaces(_out,
                                   _indent)
           : status;
}

/**
 * @brief Where the word at an index ends: at the next space or line break,
 *        or at the text's end.
 *
 * @param[in] _text    the text.
 * @param[in] _index   where the word starts.
 * @param[in] _length  the text's length.
 * @return the index just past the word.
 */
static size_t
d_cli_internal_word_end(
    const char* _text,
    size_t      _index,
    size_t      _length
)
{
    size_t end = _index;

    while ( (end < _length)     &&
            (_text[end] != ' ') &&
            (_text[end] != '\n') )
    {
        ++end;
    }

    return end;
}

/**
 * @brief Appends text wrapped at spaces: the first word where the line
 *        stands, each line no wider than the page, and every line after the
 *        first starting at a column -- a line break in the text starting one
 *        too.
 *
 * @param[in,out] _out     the buffer.
 * @param[in]     _text    the text.
 * @param[in]     _length  its length.
 * @param[in]     _at      the column the line stands at.
 * @param[in]     _indent  the column every further line starts at.
 * @param[in]     _width   the page's width.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_wrap(
    struct d_arg_buffer* _out,
    const char*          _text,
    size_t               _length,
    size_t               _at,
    size_t               _indent,
    size_t               _width
)
{
    size_t index  = 0u;
    size_t at     = _at;
    bool   fresh  = true;   // nothing yet on this line of the text
    int    status = D_ARG_OK;

    while ( (status == D_ARG_OK) &&
            (index < _length) )
    {
        // the spaces between words
        if (_text[index] == ' ')
        {
            ++index;

            continue;
        }

        // a word; or a line break of the text's own, a word of no columns
        // that always starts a new line and leaves it fresh
        const bool   own     = (_text[index] == '\n');
        const size_t end     = (own) ? index
                                     : d_cli_internal_word_end(_text,
                                                               index,
                                                               _length);
        const size_t columns = d_cli_internal_columns(_text + index,
                                                      end - index);
        const bool   breaks  = ( (own) ||
                                 ( (!fresh) &&
                                   (at + 1u + columns > _width) ) );

        // a new line; or a space, unless the word starts its line
        status = (breaks) ? d_cli_internal_new_line(_out,
                                                    _indent)
               : (!fresh) ? d_arg_buffer_append_char(_out,
                                                     ' ')
                          : D_ARG_OK;
        at     = (breaks) ? _indent
               : (!fresh) ? (at + 1u)
                          : at;
        status = ( (status == D_ARG_OK) &&
                   (!own) )
                 ? d_arg_buffer_append(_out,
                                       _text + index,
                                       end - index)
                 : status;
        at    += columns;
        fresh  = (own);
        index  = (own) ? (index + 1u) : end;
    }

    return status;
}


//==============================================================================
// 3.  ENTRIES
//==============================================================================

/**
 * @brief Whether help shows an option: one in a group named "" is hidden.
 *
 * @param[in] _option  the option.
 * @return `true` if it is shown.
 */
static bool
d_cli_internal_shown_option(
    const struct d_arg_option* _option
)
{
    return ( (_option != NULL) &&
             ( (_option->group == NULL) ||
               (_option->group[0] != '\0') ) );
}

/**
 * @brief Whether help lists a subcommand: a named one, not hidden, not
 *        disabled.
 *
 * @param[in] _command  the subcommand.
 * @return `true` if it is listed.
 */
static bool
d_cli_internal_shown_command(
    const struct d_arg_command* _command
)
{
    return ( (!d_arg_command_is_option_group(_command))             &&
             ( (_command->group == NULL) ||
               (_command->group[0] != '\0') )                       &&
             (!d_arg_command_has_flag(_command,
                                      D_ARG_COMMAND_DISABLED)) );
}

/**
 * @brief The placeholder an option's value is shown as: its type name, or
 *        VALUE.
 *
 * @param[in] _option  the option.
 * @return the placeholder.
 */
static const char*
d_cli_internal_placeholder(
    const struct d_arg_option* _option
)
{
    return ( (_option->type_name != NULL) &&
             (_option->type_name[0] != '\0') )
           ? _option->type_name
           : "VALUE";
}

/**
 * @brief Whether a long name of a flag takes a value after `=`: it has a
 *        default value that is not a negation's `false`, and the flag a type
 *        name to show it by.
 *
 * @param[in] _option  the option.
 * @param[in] _name    the long name, without its dashes.
 * @return `true` if `[=TYPE]` follows it.
 */
static bool
d_cli_internal_optional_value(
    const struct d_arg_option* _option,
    const char*                _name
)
{
    // only a flag with a type name to show
    if ( (_option->expected_max != 0) ||
         (_option->type_name == NULL) ||
         (_option->type_name[0] == '\0') )
    {
        return false;
    }

    for (size_t index = 0u; index < _option->flag_names.count; ++index)
    {
        if ( (strcmp(_option->flag_names.items[index],
                     _name) == 0)                          &&
             (index < _option->flag_values.count)          &&
             (strcmp(_option->flag_values.items[index],
                     "false") != 0) )
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Appends an option's short names, `-o, -p`; or two spaces for an
 *        option with none, so that long names line up.
 *
 * @param[in]     _option  the option.
 * @param[in,out] _out     the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_short_names(
    const struct d_arg_option* _option,
    struct d_arg_buffer*       _out
)
{
    int status = D_ARG_OK;

    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _option->short_names.count);
         ++index)
    {
        status = d_arg_buffer_printf(_out,
                                     "%s-%s",
                                     (index == 0u) ? "" : ", ",
                                     _option->short_names.items[index]);
    }

    return ( (status == D_ARG_OK) &&
             (_option->short_names.count == 0u) )
           ? d_arg_buffer_append_cstr(_out,
                                      "  ")
           : status;
}

/**
 * @brief Appends an option's long names, each after `, ` -- or after two
 *        spaces where it comes first -- and the value after the last:
 *        `=TYPE`, or `[=TYPE]` where the value may be left out.
 *
 * @param[in]     _option  the option.
 * @param[in,out] _out     the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_long_names(
    const struct d_arg_option* _option,
    struct d_arg_buffer*       _out
)
{
    const char* const value  = d_cli_internal_placeholder(_option);
    const bool        takes  = (_option->expected_max != 0);
    bool              first  = (_option->short_names.count == 0u);
    int               status = D_ARG_OK;

    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _option->long_names.count);
         ++index)
    {
        const char* const name = _option->long_names.items[index];
        const bool        last = (index + 1u == _option->long_names.count);

        status = d_arg_buffer_printf(_out,
                                     "%s--%s",
                                     (first) ? "  " : ", ",
                                     name);
        first  = false;

        // the value, after the last
        status = ( (status != D_ARG_OK) ||
                   (!last) )
                 ? status
                 : (takes)
                   ? d_arg_buffer_printf(_out,
                                         "=%s",
                                         value)
                   : (d_cli_internal_optional_value(_option,
                                                    name))
                     ? d_arg_buffer_printf(_out,
                                           "[=%s]",
                                           value)
                     : status;
    }

    return status;
}

/**
 * @brief Writes an option's left column: `-o, --out=FILE`, `    --level=N`,
 *        `-v, --verbose`, `    --color, --colour[=WHEN]`, or a positional's
 *        name, with `...` when it takes more than one value.
 *
 * @param[in]     _option  the option.
 * @param[in,out] _out     the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_option_left(
    const struct d_arg_option* _option,
    struct d_arg_buffer*       _out
)
{
    // a positional: its name
    if (d_arg_option_is_positional(_option))
    {
        const int status = d_arg_buffer_append_cstr(_out,
                                                    _option->positional_name);

        return ( (status == D_ARG_OK) &&
                 (_option->expected_max != 1) )
               ? d_arg_buffer_append_cstr(_out,
                                          "...")
               : status;
    }

    int status = d_cli_internal_short_names(_option,
                                            _out);

    status = (status == D_ARG_OK)
             ? d_cli_internal_long_names(_option,
                                         _out)
             : status;

    // a short name alone takes its value after a space
    return ( (status == D_ARG_OK)              &&
             (_option->expected_max != 0)      &&
             (_option->long_names.count == 0u) )
           ? d_arg_buffer_printf(_out,
                                 " %s",
                                 d_cli_internal_placeholder(_option))
           : status;
}

/**
 * @brief Writes an option's description, and its default where it has one
 *        and takes a value.
 *
 * @param[in]     _option  the option.
 * @param[in,out] _out     the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_option_right(
    const struct d_arg_option* _option,
    struct d_arg_buffer*       _out
)
{
    int status = d_arg_buffer_append_cstr(_out,
                                          d_arg_option_description(_option));

    // the default, for an option that takes a value
    if ( (status == D_ARG_OK)                   &&
         (_option->expected_max != 0)           &&
         (_option->default_text != NULL)        &&
         (_option->default_text[0] != '\0') )
    {
        status = d_arg_buffer_printf(_out,
                                     "%s(default: %s)",
                                     (_out->length > 0u) ? " " : "",
                                     _option->default_text);
    }

    return status;
}

// D_INTERNAL_CLI_ENTRIES_MAX
//   constant: the entries one page lays out; a command with more shows the
// first this many of each kind, which no command of the grammar's reaches.
#define D_INTERNAL_CLI_ENTRIES_MAX 256

// d_cli_internal_entry
//   struct: one line of a section before it is laid out: its left column
// and its description, each a span into the page's scratch buffer.
struct d_cli_internal_entry
{
    size_t left;
    size_t left_length;
    size_t right;
    size_t right_length;
    size_t section;
};

// d_cli_internal_page
//   struct: a page being laid out: the entries of every section, their text
// in one buffer, each section's heading, and the page's widths.
struct d_cli_internal_page
{
    struct d_arg_buffer         scratch;
    struct d_cli_internal_entry entries[D_INTERNAL_CLI_ENTRIES_MAX];
    size_t                      count;
    const char*                 headings[D_INTERNAL_CLI_ENTRIES_MAX];
    size_t                      sections;
    size_t                      width;
    size_t                      column;
};

/**
 * @brief Begins a section of the page: later entries are its own.
 *
 * @param[in,out] _page     the page.
 * @param[in]     _heading  the heading, without its colon; it must outlive
 *                          the page.
 */
static void
d_cli_internal_section(
    struct d_cli_internal_page* _page,
    const char*                 _heading
)
{
    if (_page->sections < D_INTERNAL_CLI_ENTRIES_MAX)
    {
        _page->headings[_page->sections++] = _heading;
    }

    return;
}

/**
 * @brief Adds an option to the page's current section.
 *
 * @param[in,out] _page    the page.
 * @param[in]     _option  the option.
 * @return D_ARG_OK, or the scratch buffer's status.
 */
static int
d_cli_internal_add_option(
    struct d_cli_internal_page* _page,
    const struct d_arg_option*  _option
)
{
    // no room, or no section to put it in
    if ( (_page->count >= D_INTERNAL_CLI_ENTRIES_MAX) ||
         (_page->sections == 0u) )
    {
        return D_ARG_OK;
    }

    struct d_cli_internal_entry* const entry = &_page->entries[_page->count];
    struct d_arg_buffer                right = D_ARG_BUFFER_INIT;

    entry->section = _page->sections - 1u;
    entry->left    = _page->scratch.length;

    int status = d_cli_internal_option_left(_option,
                                            &_page->scratch);

    entry->left_length = _page->scratch.length - entry->left;
    status             = (status == D_ARG_OK)
                         ? d_cli_internal_option_right(_option,
                                                       &right)
                         : status;
    entry->right       = _page->scratch.length;
    status             = ( (status == D_ARG_OK) &&
                           (right.length > 0u) )
                         ? d_arg_buffer_append(&_page->scratch,
                                               right.data,
                                               right.length)
                         : status;
    entry->right_length = _page->scratch.length - entry->right;

    d_arg_buffer_release(&right);

    if (status == D_ARG_OK)
    {
        ++_page->count;
    }

    return status;
}

/**
 * @brief Adds a subcommand to the page's current section: its name and
 *        aliases, and its description.
 *
 * @param[in,out] _page     the page.
 * @param[in]     _command  the subcommand.
 * @return D_ARG_OK, or the scratch buffer's status.
 */
static int
d_cli_internal_add_command(
    struct d_cli_internal_page* _page,
    const struct d_arg_command* _command
)
{
    // no room, or no section to put it in
    if ( (_page->count >= D_INTERNAL_CLI_ENTRIES_MAX) ||
         (_page->sections == 0u) )
    {
        return D_ARG_OK;
    }

    struct d_cli_internal_entry* const entry = &_page->entries[_page->count];

    entry->section = _page->sections - 1u;
    entry->left    = _page->scratch.length;

    int status = d_arg_command_display_name(_command,
                                            true,
                                            &_page->scratch);

    entry->left_length  = _page->scratch.length - entry->left;
    entry->right        = _page->scratch.length;
    status              = (status == D_ARG_OK)
                          ? d_arg_buffer_append_cstr(
                                &_page->scratch,
                                d_arg_command_description(_command))
                          : status;
    entry->right_length = _page->scratch.length - entry->right;

    if (status == D_ARG_OK)
    {
        ++_page->count;
    }

    return status;
}


//==============================================================================
// 4.  THE PAGE
//==============================================================================

/**
 * @brief Whether a command has a subcommand help lists.
 *
 * @param[in] _command  the command.
 * @return `true` if it has one.
 */
static bool
d_cli_internal_lists_commands(
    const struct d_arg_command* _command
)
{
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        if (d_cli_internal_shown_command(_command->subcommands.items[index]))
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Whether a command must be followed by one of its subcommands: it
 *        has some to show and no work of its own, or the parser requires
 *        one.
 *
 * @param[in] _formatter  the formatter, whose context may be the program.
 * @param[in] _command    the command.
 * @return `true` if a subcommand is required.
 */
static bool
d_cli_internal_needs_command(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command
)
{
    // nothing to require
    if (!d_cli_internal_lists_commands(_command))
    {
        return false;
    }

    // the program's own say, where the kit made it
    if (_formatter->context != NULL)
    {
        const struct d_cli_program* const program =
            (const struct d_cli_program*)_formatter->context;

        for (size_t index = 0u; index < program->handler_count; ++index)
        {
            if (program->handlers[index].command == _command)
            {
                return (_command->require_subcommand_min > 0u);
            }
        }

        return true;
    }

    return (_command->require_subcommand_min > 0u);
}

/**
 * @brief Appends the words of a usage line that follow the command's name:
 *        `[OPTION]...`, the subcommand -- `COMMAND [ARGUMENT]...`, in
 *        brackets where it may be left out -- then each positional, in
 *        brackets where it may be left out and with `...` where it takes
 *        more than one value.
 *
 * @param[in]     _formatter  the formatter.
 * @param[in]     _command    the command.
 * @param[in,out] _words      the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_usage_words(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _words
)
{
    int status = d_arg_buffer_append_cstr(_words,
                                          "[OPTION]...");

    // the subcommands
    if ( (status == D_ARG_OK) &&
         (d_cli_internal_lists_commands(_command)) )
    {
        status = d_arg_buffer_append_cstr(
                     _words,
                     (d_cli_internal_needs_command(_formatter,
                                                   _command))
                     ? " COMMAND [ARGUMENT]..."
                     : " [COMMAND [ARGUMENT]...]");
    }

    // each positional, in order
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->options.count);
         ++index)
    {
        const struct d_arg_option* const each = _command->options.items[index];

        if ( (!d_arg_option_is_positional(each)) ||
             (!d_cli_internal_shown_option(each)) )
        {
            continue;
        }

        const bool required = d_arg_option_has_flag(each,
                                                    D_ARG_OPTION_REQUIRED);

        status = d_arg_buffer_printf(_words,
                                     " %s%s%s%s",
                                     (required) ? "" : "[",
                                     each->positional_name,
                                     (required) ? "" : "]",
                                     (each->expected_max != 1) ? "..." : "");
    }

    return status;
}

/**
 * @brief Appends `Usage: NAME ` and the usage line's words, wrapped under
 *        the first of them -- or eight columns in, where the name takes
 *        half the page -- and a newline.
 *
 * @param[in]     _name    the command's name as the page shows it.
 * @param[in]     _words   the words.
 * @param[in]     _width   the page's width.
 * @param[in,out] _out     the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_usage_line(
    struct d_pack_text         _name,
    const struct d_arg_buffer* _words,
    size_t                     _width,
    struct d_arg_buffer*       _out
)
{
    const size_t lead   = 7u + d_cli_internal_columns(_name.data,
                                                      _name.length) + 1u;
    int          status = d_arg_buffer_append_cstr(_out,
                                                   "Usage: ");

    status = (status == D_ARG_OK)
             ? d_arg_buffer_append(_out,
                                   _name.data,
                                   _name.length)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_char(_out,
                                        ' ')
             : status;
    status = (status == D_ARG_OK)
             ? d_cli_internal_wrap(_out,
                                   _words->data,
                                   _words->length,
                                   lead,
                                   (lead < (_width / 2u)) ? lead : 8u,
                                   _width)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_char(_out,
                                      '\n')
           : status;
}

/**
 * @brief Appends a buffer's text as a line: a newline after it, unless it
 *        ends in one.
 *
 * @param[in,out] _out   the buffer appended to.
 * @param[in]     _text  the text; not empty.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_append_line(
    struct d_arg_buffer*       _out,
    const struct d_arg_buffer* _text
)
{
    const int status = d_arg_buffer_append(_out,
                                           _text->data,
                                           _text->length);

    return ( (status == D_ARG_OK) &&
             (_text->data[_text->length - 1u] != '\n') )
           ? d_arg_buffer_append_char(_out,
                                      '\n')
           : status;
}

/**
 * @brief Writes the usage line: the command's own usage, where it has one;
 *        otherwise `Usage: NAME [OPTION]... COMMAND [ARGUMENT]...`, then each
 *        positional (d_cli_internal_usage_words).
 *
 * @param[in]     _formatter  the formatter.
 * @param[in]     _command    the command.
 * @param[in]     _name       its name as the page shows it.
 * @param[in]     _width      the page's width.
 * @param[in,out] _out        the buffer.
 * @return D_ARG_OK, or the status that stopped it.
 */
static int
d_cli_internal_usage(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_pack_text            _name,
    size_t                        _width,
    struct d_arg_buffer*          _out
)
{
    struct d_arg_buffer own    = D_ARG_BUFFER_INIT;
    struct d_arg_buffer words  = D_ARG_BUFFER_INIT;
    int                 status = d_arg_command_usage(_command,
                                                     d_arg_buffer_sink(&own));
    const bool          stands = ( (status == D_ARG_OK) &&
                                   (own.length > 0u) );

    // the command's own usage stands for the line
    status = (status != D_ARG_OK)
             ? status
             : (stands)
               ? d_cli_internal_append_line(_out,
                                            &own)
               : d_cli_internal_usage_words(_formatter,
                                            _command,
                                            &words);
    status = ( (status == D_ARG_OK) &&
               (!stands) )
             ? d_cli_internal_usage_line(_name,
                                         &words,
                                         _width,
                                         _out)
             : status;

    d_arg_buffer_release(&own);
    d_arg_buffer_release(&words);

    return status;
}

/**
 * @brief Writes the description as a sentence: its first letter raised,
 *        and a full stop where it ends in neither one, `?`, `!` nor `:`.
 *
 * @param[in]     _command  the command.
 * @param[in]     _width    the page's width.
 * @param[in,out] _out      the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_description(
    const struct d_arg_command* _command,
    size_t                      _width,
    struct d_arg_buffer*        _out
)
{
    const char* const text   = d_arg_command_description(_command);
    size_t            length = strlen(text);

    // nothing to say
    if (length == 0u)
    {
        return D_ARG_OK;
    }

    struct d_arg_buffer sentence = D_ARG_BUFFER_INIT;
    int                 status   = d_arg_buffer_append(&sentence,
                                                       text,
                                                       length);

    if ( (status == D_ARG_OK)        &&
         (sentence.data[0] >= 'a')   &&
         (sentence.data[0] <= 'z') )
    {
        sentence.data[0] = (char)(sentence.data[0] - 'a' + 'A');
    }

    while ( (length > 0u) &&
            ( (text[length - 1u] == ' ') ||
              (text[length - 1u] == '\n') ) )
    {
        --length;
    }

    if ( (status == D_ARG_OK)     &&
         (length > 0u)            &&
         (text[length - 1u] != '.') &&
         (text[length - 1u] != '?') &&
         (text[length - 1u] != '!') &&
         (text[length - 1u] != ':') )
    {
        sentence.length = length;
        status          = d_arg_buffer_append_char(&sentence,
                                                   '.');
    }

    status = (status == D_ARG_OK)
             ? d_cli_internal_wrap(_out,
                                   sentence.data,
                                   sentence.length,
                                   0u,
                                   0u,
                                   _width)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_char(_out,
                                        '\n')
             : status;

    d_arg_buffer_release(&sentence);

    return status;
}

// D_INTERNAL_CLI_GROUPS_MAX
//   constant: the groups of commands, and of options, one page heads; a
// command with more puts the rest under none, which no command of the
// grammar's reaches.
#define D_INTERNAL_CLI_GROUPS_MAX 32

/**
 * @brief The group help lists a subcommand under: its own, or "Commands".
 *
 * @param[in] _command  the subcommand.
 * @return the group's name.
 */
static const char*
d_cli_internal_command_group(
    const struct d_arg_command* _command
)
{
    return (_command->group != NULL) ? _command->group : "Commands";
}

/**
 * @brief The group help lists an option under: its own, or "Options" for
 *        the default group -- unset, or named "OPTIONS", as CLI11 names it
 *        -- so that the two are one section.
 *
 * @param[in] _option  the option.
 * @return the group's name.
 */
static const char*
d_cli_internal_option_group(
    const struct d_arg_option* _option
)
{
    return ( (_option->group == NULL) ||
             (strcmp(_option->group,
                     "OPTIONS") == 0) )
           ? "Options"
           : _option->group;
}

/**
 * @brief Adds a name to a list of D_INTERNAL_CLI_GROUPS_MAX names, unless
 *        it is there already or the list is full.
 *
 * @param[in,out] _names  the list.
 * @param[in,out] _count  how many it holds.
 * @param[in]     _name   the name.
 */
static void
d_cli_internal_remember(
    const char** _names,
    size_t*      _count,
    const char*  _name
)
{
    for (size_t index = 0u; index < *_count; ++index)
    {
        // there already
        if (strcmp(_names[index],
                   _name) == 0)
        {
            return;
        }
    }

    if (*_count < (size_t)D_INTERNAL_CLI_GROUPS_MAX)
    {
        _names[(*_count)++] = _name;
    }

    return;
}

/**
 * @brief Collects a command's positionals, under "Arguments".
 *
 * @param[in]     _command  the command.
 * @param[in,out] _page     the page.
 * @return D_ARG_OK, or the scratch buffer's status.
 */
static int
d_cli_internal_collect_arguments(
    const struct d_arg_command* _command,
    struct d_cli_internal_page* _page
)
{
    int  status  = D_ARG_OK;
    bool started = false;

    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->options.count);
         ++index)
    {
        const struct d_arg_option* const each = _command->options.items[index];

        // a positional, shown
        if ( (!d_arg_option_is_positional(each)) ||
             (!d_cli_internal_shown_option(each)) )
        {
            continue;
        }

        if (!started)
        {
            d_cli_internal_section(_page,
                                   "Arguments");
            started = true;
        }

        status = d_cli_internal_add_option(_page,
                                           each);
    }

    return status;
}

/**
 * @brief Collects a command's subcommands: a section for each group, in
 *        the order first met -- "Commands" for those in none.
 *
 * @param[in]     _command  the command.
 * @param[in,out] _page     the page.
 * @return D_ARG_OK, or the scratch buffer's status.
 */
static int
d_cli_internal_collect_commands(
    const struct d_arg_command* _command,
    struct d_cli_internal_page* _page
)
{
    const char* groups[D_INTERNAL_CLI_GROUPS_MAX];
    size_t      count  = 0u;
    int         status = D_ARG_OK;

    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        const struct d_arg_command* const each =
            _command->subcommands.items[index];

        if (d_cli_internal_shown_command(each))
        {
            d_cli_internal_remember(groups,
                                    &count,
                                    d_cli_internal_command_group(each));
        }
    }

    for (size_t known = 0u;
         (status == D_ARG_OK) && (known < count);
         ++known)
    {
        d_cli_internal_section(_page,
                               groups[known]);

        for (size_t index = 0u;
             (status == D_ARG_OK) && (index < _command->subcommands.count);
             ++index)
        {
            const struct d_arg_command* const each =
                _command->subcommands.items[index];

            if ( (d_cli_internal_shown_command(each)) &&
                 (strcmp(d_cli_internal_command_group(each),
                         groups[known]) == 0) )
            {
                status = d_cli_internal_add_command(_page,
                                                    each);
            }
        }
    }

    return status;
}

/**
 * @brief Collects the options of one of a command's groups: those other
 *        than help and version, or those alone.
 *
 * @param[in]     _command   the command.
 * @param[in]     _group     the group.
 * @param[in]     _standard  `true` for help and version alone.
 * @param[in,out] _page      the page.
 * @return D_ARG_OK, or the scratch buffer's status.
 */
static int
d_cli_internal_collect_group(
    const struct d_arg_command* _command,
    const char*                 _group,
    bool                        _standard,
    struct d_cli_internal_page* _page
)
{
    int status = D_ARG_OK;

    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->options.count);
         ++index)
    {
        const struct d_arg_option* const each     =
            _command->options.items[index];
        const bool                       standard =
            ( (each == _command->help)     ||
              (each == _command->help_all) ||
              (each == _command->version) );

        if ( (!d_arg_option_is_positional(each))  &&
             (d_cli_internal_shown_option(each))  &&
             (standard == _standard)              &&
             (strcmp(d_cli_internal_option_group(each),
                     _group) == 0) )
        {
            status = d_cli_internal_add_option(_page,
                                               each);
        }
    }

    return status;
}

/**
 * @brief Collects a command's named options: a section for each group, in
 *        the order first met, its help and version last, as GNU lists them.
 *
 * @param[in]     _command  the command.
 * @param[in,out] _page     the page.
 * @return D_ARG_OK, or the scratch buffer's status.
 */
static int
d_cli_internal_collect_options(
    const struct d_arg_command* _command,
    struct d_cli_internal_page* _page
)
{
    const char* groups[D_INTERNAL_CLI_GROUPS_MAX];
    size_t      count  = 0u;
    int         status = D_ARG_OK;

    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        const struct d_arg_option* const each = _command->options.items[index];

        if ( (!d_arg_option_is_positional(each)) &&
             (d_cli_internal_shown_option(each)) )
        {
            d_cli_internal_remember(groups,
                                    &count,
                                    d_cli_internal_option_group(each));
        }
    }

    for (size_t known = 0u;
         (status == D_ARG_OK) && (known < count);
         ++known)
    {
        d_cli_internal_section(_page,
                               groups[known]);
        status = d_cli_internal_collect_group(_command,
                                              groups[known],
                                              false,
                                              _page);
        status = (status == D_ARG_OK)
                 ? d_cli_internal_collect_group(_command,
                                                groups[known],
                                                true,
                                                _page)
                 : status;
    }

    return status;
}

/**
 * @brief Collects each option group's options, under its name.
 *
 * @param[in]     _command  the command.
 * @param[in,out] _page     the page.
 * @return D_ARG_OK, or the scratch buffer's status.
 */
static int
d_cli_internal_collect_option_groups(
    const struct d_arg_command* _command,
    struct d_cli_internal_page* _page
)
{
    int status = D_ARG_OK;

    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        const struct d_arg_command* const group =
            _command->subcommands.items[index];

        // an option group, enabled
        if ( (!d_arg_command_is_option_group(group)) ||
             (d_arg_command_has_flag(group,
                                     D_ARG_COMMAND_DISABLED)) )
        {
            continue;
        }

        d_cli_internal_section(_page,
                               d_arg_command_group(group));

        for (size_t at = 0u;
             (status == D_ARG_OK) && (at < group->options.count);
             ++at)
        {
            status = (d_cli_internal_shown_option(group->options.items[at]))
                     ? d_cli_internal_add_option(_page,
                                                 group->options.items[at])
                     : status;
        }
    }

    return status;
}

/**
 * @brief The command a command takes options from: its parent, when it
 *        takes them -- fallthrough -- or none.
 *
 * @param[in] _command  the command.
 * @return the parent, or NULL.
 */
static const struct d_arg_command*
d_cli_internal_above(
    const struct d_arg_command* _command
)
{
    return (d_arg_command_has_flag(_command,
                                   D_ARG_COMMAND_FALLTHROUGH))
           ? _command->parent
           : NULL;
}

/**
 * @brief Collects the options a command above a subcommand holds -- in
 *        itself, or in one of its option groups -- under "Global options",
 *        begun at the first, the help and version flags of the command
 *        above aside.
 *
 * @param[in]     _holder   the command whose options to take: the command
 *                          above, or one of its option groups.
 * @param[in]     _above    the command above.
 * @param[in,out] _page     the page.
 * @param[in,out] _started  whether the section has begun.
 * @return D_ARG_OK, or the scratch buffer's status.
 */
static int
d_cli_internal_collect_held(
    const struct d_arg_command* _holder,
    const struct d_arg_command* _above,
    struct d_cli_internal_page* _page,
    bool*                       _started
)
{
    int status = D_ARG_OK;

    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _holder->options.count);
         ++index)
    {
        const struct d_arg_option* const each = _holder->options.items[index];

        if ( (d_arg_option_is_positional(each))   ||
             (!d_cli_internal_shown_option(each)) ||
             (each == _above->help)               ||
             (each == _above->help_all)           ||
             (each == _above->version) )
        {
            continue;
        }

        if (!*_started)
        {
            d_cli_internal_section(_page,
                                   "Global options");
            *_started = true;
        }

        status = d_cli_internal_add_option(_page,
                                           each);
    }

    return status;
}

/**
 * @brief Collects the options a subcommand takes from the commands above
 *        it -- theirs, and their option groups', which fall through as
 *        theirs do -- under "Global options".
 *
 * @param[in]     _command  the command.
 * @param[in,out] _page     the page.
 * @return D_ARG_OK, or the scratch buffer's status.
 */
static int
d_cli_internal_collect_global(
    const struct d_arg_command* _command,
    struct d_cli_internal_page* _page
)
{
    int  status  = D_ARG_OK;
    bool started = false;

    for (const struct d_arg_command* above = d_cli_internal_above(_command);
         (status == D_ARG_OK) && (above != NULL);
         above = d_cli_internal_above(above))
    {
        status = d_cli_internal_collect_held(above,
                                             above,
                                             _page,
                                             &started);

        // each option group of the command above, enabled
        for (size_t index = 0u;
             (status == D_ARG_OK) && (index < above->subcommands.count);
             ++index)
        {
            const struct d_arg_command* const group =
                above->subcommands.items[index];

            status = ( (d_arg_command_is_option_group(group)) &&
                       (!d_arg_command_has_flag(group,
                                                D_ARG_COMMAND_DISABLED)) )
                     ? d_cli_internal_collect_held(group,
                                                   above,
                                                   _page,
                                                   &started)
                     : status;
        }
    }

    return status;
}

/**
 * @brief Collects a command's sections: its positionals, its subcommands,
 *        its options by group, its option groups' options under their names,
 *        and, for a subcommand that takes them, its parents' options.
 *
 * @param[in]     _command  the command.
 * @param[in,out] _page     the page.
 * @return D_ARG_OK, or the scratch buffer's status.
 */
static int
d_cli_internal_collect(
    const struct d_arg_command* _command,
    struct d_cli_internal_page* _page
)
{
    int status = d_cli_internal_collect_arguments(_command,
                                                  _page);

    status = (status == D_ARG_OK)
             ? d_cli_internal_collect_commands(_command,
                                               _page)
             : status;
    status = (status == D_ARG_OK)
             ? d_cli_internal_collect_options(_command,
                                              _page)
             : status;
    status = (status == D_ARG_OK)
             ? d_cli_internal_collect_option_groups(_command,
                                                    _page)
             : status;

    return (status == D_ARG_OK)
           ? d_cli_internal_collect_global(_command,
                                           _page)
           : status;
}

/**
 * @brief Chooses the right column: two past the widest left column of the
 *        page, after its two-space indent, and never past
 *        D_INTERNAL_CLI_HELP_COLUMN_MAX.
 *
 * @param[in,out] _page  the page.
 */
static void
d_cli_internal_choose_column(
    struct d_cli_internal_page* _page
)
{
    size_t widest = 0u;

    for (size_t index = 0u; index < _page->count; ++index)
    {
        const struct d_cli_internal_entry* const each = &_page->entries[index];
        const size_t columns = d_cli_internal_columns(
                                   _page->scratch.data + each->left,
                                   each->left_length);

        if ( (columns + 4u <= (size_t)D_INTERNAL_CLI_HELP_COLUMN_MAX) &&
             (columns > widest) )
        {
            widest = columns;
        }
    }

    _page->column = (widest > 0u)
                    ? (widest + 4u)
                    : (size_t)D_INTERNAL_CLI_HELP_COLUMN_MAX;

    return;
}

/**
 * @brief Writes one entry of a section in the two columns: two spaces and
 *        its name, then its description, beside the name or, where the
 *        name reaches past the right column, under it; and a newline.
 *
 * @param[in]     _page   the page.
 * @param[in]     _entry  the entry.
 * @param[in,out] _out    the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_entry_line(
    const struct d_cli_internal_page*  _page,
    const struct d_cli_internal_entry* _entry,
    struct d_arg_buffer*               _out
)
{
    const char* const left    = _page->scratch.data + _entry->left;
    const size_t      columns = d_cli_internal_columns(left,
                                                       _entry->left_length);
    const bool        under   = ((2u + columns + 2u) > _page->column);
    const bool        right   = (_entry->right_length > 0u);
    int               status  = d_arg_buffer_append_cstr(_out,
                                                         "  ");

    status = (status == D_ARG_OK)
             ? d_arg_buffer_append(_out,
                                   left,
                                   _entry->left_length)
             : status;

    // a description: beside the name, or under it past the column
    status = ( (status != D_ARG_OK) ||
               (!right) )
             ? status
             : (under)
               ? d_cli_internal_new_line(_out,
                                         _page->column)
               : d_cli_internal_spaces(_out,
                                       _page->column - 2u - columns);
    status = ( (status == D_ARG_OK) &&
               (right) )
             ? d_cli_internal_wrap(_out,
                                   _page->scratch.data + _entry->right,
                                   _entry->right_length,
                                   _page->column,
                                   _page->column,
                                   _page->width)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_char(_out,
                                      '\n')
           : status;
}

/**
 * @brief Writes the page's sections, each after a blank line and its
 *        heading, each entry in the two columns.
 *
 * @param[in]     _page  the page.
 * @param[in,out] _out   the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_sections(
    const struct d_cli_internal_page* _page,
    struct d_arg_buffer*              _out
)
{
    int status = D_ARG_OK;

    for (size_t section = 0u;
         (status == D_ARG_OK) && (section < _page->sections);
         ++section)
    {
        bool headed = false;

        for (size_t index = 0u;
             (status == D_ARG_OK) && (index < _page->count);
             ++index)
        {
            const struct d_cli_internal_entry* const each =
                &_page->entries[index];

            if (each->section != section)
            {
                continue;
            }

            // the heading, before the section's first entry
            status = (headed)
                     ? D_ARG_OK
                     : d_arg_buffer_printf(_out,
                                           "\n%s:\n",
                                           _page->headings[section]);
            headed = true;
            status = (status == D_ARG_OK)
                     ? d_cli_internal_entry_line(_page,
                                                 each,
                                                 _out)
                     : status;
        }
    }

    return status;
}

// D_INTERNAL_CLI_EXIT_STATUSES
//   constant: what each exit status of a program the kit made means
// (GRAMMAR.md, section 8), as its root's help says it.
static const char* const D_INTERNAL_CLI_EXIT_STATUSES[] =
{
    "if it did what was asked, and the answer, if one was asked, is yes",
    "if it did, and the answer is no",
    "if there was trouble: the command line, a file, or the program"
};

/**
 * @brief Writes what the exit statuses mean, after a blank line and a
 *        heading: each status two columns in, and its meaning wrapped five
 *        columns in.
 *
 * @param[in]     _width  the page's width.
 * @param[in,out] _out    the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_cli_internal_exit_statuses(
    size_t               _width,
    struct d_arg_buffer* _out
)
{
    const size_t count  = sizeof(D_INTERNAL_CLI_EXIT_STATUSES) /
                          sizeof(D_INTERNAL_CLI_EXIT_STATUSES[0]);
    int          status = d_arg_buffer_append_cstr(_out,
                                                   "\nExit status:\n");

    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < count);
         ++index)
    {
        const char* const meaning = D_INTERNAL_CLI_EXIT_STATUSES[index];

        status = d_arg_buffer_printf(_out,
                                     "  %lu  ",
                                     (unsigned long)index);
        status = (status == D_ARG_OK)
                 ? d_cli_internal_wrap(_out,
                                       meaning,
                                       strlen(meaning),
                                       5u,
                                       5u,
                                       _width)
                 : status;
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_char(_out,
                                            '\n')
                 : status;
    }

    return status;
}

/**
 * @brief Writes the command's footer after a blank line, wrapped; and, on
 *        the root's page of a program the kit made, the exit statuses.
 *
 * @param[in]     _formatter  the formatter.
 * @param[in]     _command    the command.
 * @param[in]     _width      the page's width.
 * @param[in,out] _out        the buffer.
 * @return D_ARG_OK, or the status that stopped it.
 */
static int
d_cli_internal_footer(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    size_t                        _width,
    struct d_arg_buffer*          _out
)
{
    struct d_arg_buffer footer = D_ARG_BUFFER_INIT;
    int                 status = d_arg_command_footer(_command,
                                                      d_arg_buffer_sink(
                                                          &footer));

    while ( (footer.length > 0u) &&
            (footer.data[footer.length - 1u] == '\n') )
    {
        --footer.length;
    }

    // the footer, wrapped
    if ( (status == D_ARG_OK) &&
         (footer.length > 0u) )
    {
        status = d_arg_buffer_append_char(_out,
                                          '\n');
        status = (status == D_ARG_OK)
                 ? d_cli_internal_wrap(_out,
                                       footer.data,
                                       footer.length,
                                       0u,
                                       0u,
                                       _width)
                 : status;
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_char(_out,
                                            '\n')
                 : status;
    }

    d_arg_buffer_release(&footer);

    // the root of a program the kit made: what its exit statuses mean
    return ( (status == D_ARG_OK)            &&
             (_command->parent == NULL)      &&
             (_formatter->context != NULL) )
           ? d_cli_internal_exit_statuses(_width,
                                          _out)
           : status;
}

/**
 * @brief Lays a page out: usage, description, sections, footer.
 *
 * @param[in]     _formatter  the formatter.
 * @param[in]     _command    the command.
 * @param[in]     _name       its name as the page shows it.
 * @param[in,out] _page       the page, empty, its width set.
 * @param[in,out] _out        the buffer.
 * @return D_ARG_OK, or the status that stopped it.
 */
static int
d_cli_internal_lay_out(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_pack_text            _name,
    struct d_cli_internal_page*   _page,
    struct d_arg_buffer*          _out
)
{
    int status = d_cli_internal_usage(_formatter,
                                      _command,
                                      _name,
                                      _page->width,
                                      _out);

    status = (status == D_ARG_OK)
             ? d_cli_internal_description(_command,
                                          _page->width,
                                          _out)
             : status;
    status = (status == D_ARG_OK)
             ? d_cli_internal_collect(_command,
                                      _page)
             : status;

    // the sections, once every entry says how wide its name is
    if (status == D_ARG_OK)
    {
        d_cli_internal_choose_column(_page);
        status = d_cli_internal_sections(_page,
                                         _out);
    }

    return (status == D_ARG_OK)
           ? d_cli_internal_footer(_formatter,
                                   _command,
                                   _page->width,
                                   _out)
           : status;
}

/**
 * @brief The kit's page: usage, description, sections, footer. Every other
 *        mode of help -- every subcommand expanded -- is the parser's own.
 *
 * @param[in]     _formatter  the formatter.
 * @param[in]     _command    the command.
 * @param[in]     _name       its name as the page shows it.
 * @param[in]     _mode       a d_arg_help_mode value.
 * @param[in,out] _out        the buffer.
 * @return D_ARG_OK, or the status that stopped it.
 */
static int
d_cli_internal_make_help(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_pack_text            _name,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    // the modes the kit does not lay out
    if (_mode != (int)D_ARG_HELP_NORMAL)
    {
        return d_arg_format_help_default(_formatter,
                                         _command,
                                         _name,
                                         _mode,
                                         _out);
    }

    struct d_cli_internal_page* const page =
        (struct d_cli_internal_page*)D_INTERNAL_CLI_MALLOC(sizeof(*page));

    // no room for the page
    if (!page)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    memset(page,
           0,
           sizeof(*page));
    page->width = (_formatter->line_width > 0u)
                  ? _formatter->line_width
                  : d_cli_help_width();

    const int status = d_cli_internal_lay_out(_formatter,
                                              _command,
                                              _name,
                                              page,
                                              _out);

    d_arg_buffer_release(&page->scratch);
    D_INTERNAL_CLI_FREE(page);

    return status;
}

/**
 * @brief The kit's usage line alone, for whoever asks the parser for it: a
 *        usage error's message, say.
 *
 * @param[in]     _formatter  the formatter.
 * @param[in]     _command    the command.
 * @param[in]     _name       its name as the page shows it.
 * @param[in,out] _out        the buffer.
 * @return D_ARG_OK, or the status that stopped it.
 */
static int
d_cli_internal_make_usage(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_pack_text            _name,
    struct d_arg_buffer*          _out
)
{
    // validate the parameters
    if ( (!_formatter) ||
         (!_command)   ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_cli_internal_usage(_formatter,
                                _command,
                                _name,
                                (_formatter->line_width > 0u)
                                ? _formatter->line_width
                                : d_cli_help_width(),
                                _out);
}

/*
d_cli_formatter_steps
  make_help builds the whole page, so no other step is reached from it;
make_usage is the usage line alone, which the parser's d_arg_format_usage
gives whoever asks.
*/
const struct d_arg_formatter_vtable d_cli_formatter_steps =
{
    d_cli_internal_make_help,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    d_cli_internal_make_usage,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

#endif  // D_INTERNAL_CLI_FORMATTER

/*
d_cli_formatter_use
  The steps, when the kit lays help out.
*/
void
d_cli_formatter_use(
    struct d_arg_formatter* _formatter
)
{
    // nothing to set
    if (!_formatter)
    {
        return;
    }

#if (D_INTERNAL_CLI_FORMATTER == 1)
    _formatter->vtable = &d_cli_formatter_steps;
#endif

    return;
}
