/*******************************************************************************
* djinterp [parse]                                         arg_completion_fish.c
*
* The completion script for fish, declared in arg_internal.h (11.2.4).
*   The script holds the model as global lists named after the program --
* each command's row, the subcommands' names and the options' names with
* their keys, targets and descriptions, each option's row, the positionals'
* keys and rows, the choices -- which fish's `contains -i` searches by
* exact text, as no `switch` can, its cases being patterns. Its functions
* walk the words before the cursor as the parse takes them and print what
* may come at the cursor, one candidate a line with its description after a
* tab; fish matches them against the word itself. Files come from fish's own
* __fish_complete_path and __fish_complete_directories.
*   `@@` in the lines below stands for the program's name as
* d_arg_internal_complete_ident writes it.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_completion_fish.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_internal.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, statuses


#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// KIND_LETTERS
//   constant: each d_arg_internal_complete_kind as the script spells it:
// `-` for none, `c` for choices, `f` for files, `d` for directories and `n`
// for nothing.
static const char KIND_LETTERS[] = "-cfdn";

// FISH_LINES
//   constant: the script's functions: a row of a table; the checks and the
// unquoting of a word; the walk over the words before the cursor -- whose
// state the functions run with -S keep in the caller's variables: cmd and
// cf, the command reached and its switches; opt, need and room, the option
// whose values the words are among, the values it still needs and how many
// more it may take (-1 for any number); only, set past `--`; used and
// counts, the subcommands each command was given and the values each
// positional took; extra, the switches of the command a word no positional
// takes ends at -- and what may come at the cursor.
static const char* const FISH_LINES[] =
{
    "function __djarg_@@_row",
    "    # the fields of row `$argv[2]` of table `$argv[1]`",
    "    set -l table __djarg_@@_$argv[1]",
    "    string split ' ' -- $$table[1][(math $argv[2] + 1)]",
    "end",
    "",
    "function __djarg_@@_first",
    "    # whether `$argv[1]` may begin a name",
    "    string match -qr -- '^[^\\-!\\s\\x00-\\x1f\\x7f]' \"$argv[1]\"",
    "end",
    "",
    "function __djarg_@@_unq",
    "    # the token `$argv[1]` with its quotes and escapes removed",
    "    set -l s \"$argv[1]\"",
    "    if string match -qr -- '^\\'[^\\']*$|^\"[^\"]*$' \"$s\"",
    "        string sub -s 2 -- \"$s\"",
    "    else if not string unescape -- \"$s\"",
    "        printf '%s\\n' \"$s\"",
    "    end",
    "end",
    "",
    "function __djarg_@@_find -S",
    "    # o: the option named `$argv[1]` at the command or, falling",
    "    # through, above it",
    "    set -l L $cmd",
    "    set o",
    "    while true",
    "        set -l i (contains -i -- \"$L:$argv[1]\" $__djarg_@@_ok)",
    "        if test -n \"$i\"",
    "            set o $__djarg_@@_ov[$i]",
    "            return",
    "        end",
    "        set -l c (__djarg_@@_row cmds $L)",
    "        if string match -q -- '*f*' $c[2]; and test $L -ne 0",
    "            set L $c[1]",
    "        else",
    "            return",
    "        end",
    "    end",
    "end",
    "",
    "function __djarg_@@_target -S",
    "    # t: the subcommand `$argv[1]` names, from the command up, and",
    "    # l: the command it is under",
    "    set -l L $cmd",
    "    set t",
    "    set l",
    "    while true",
    "        set -l c (__djarg_@@_row cmds $L)",
    "        set -l u $used[(math $L + 1)]",
    "        if test -z \"$u\"",
    "            set u 0",
    "        end",
    "        if test $c[3] -eq 0; or test $u -lt $c[3]",
    "            or not string match -q -- '*s*' $c[2]",
    "            set -l i (contains -i -- \"$L:$argv[1]\" $__djarg_@@_sk)",
    "            if test -n \"$i\"",
    "                set t $__djarg_@@_st[$i]",
    "                set l $L",
    "                return",
    "            end",
    "        end",
    "        if string match -q -- '*s*' $c[2]; and test $L -ne 0",
    "            set L $c[1]",
    "        else",
    "            return",
    "        end",
    "    end",
    "end",
    "",
    "function __djarg_@@_class -S",
    "    # r: what `$argv[1]` is to the command -- mark, sub, long,",
    "    # short, win, term or none",
    "    set -l w \"$argv[1]\"",
    "    set r none",
    "    if test \"$w\" = --",
    "        set r mark",
    "        return",
    "    end",
    "    __djarg_@@_target \"$w\"",
    "    if test -n \"$t\"",
    "        set r sub",
    "        return",
    "    end",
    "    if string match -q -- '--?*' \"$w\"",
    "        if __djarg_@@_first (string sub -s 3 -l 1 -- \"$w\")",
    "            set r long",
    "        end",
    "        return",
    "    end",
    "    if string match -q -- '-?*' \"$w\"",
    "        set -l first (string sub -s 2 -l 1 -- \"$w\")",
    "        if not __djarg_@@_first \"$first\"",
    "            return",
    "        end",
    "        if string match -qr -- '^-([0-9]|\\.[0-9])' \"$w\"",
    "            and not contains -- \"$cmd:-$first\" $__djarg_@@_ok",
    "            return",
    "        end",
    "        set r short",
    "        return",
    "    end",
    "    if string match -q -- '*w*' $cf; and string match -q -- '/?*' \"$w\"",
    "        and __djarg_@@_first (string sub -s 2 -l 1 -- \"$w\")",
    "        set r win",
    "    else if test \"$w\" = ++; and test $cmd -ne 0",
    "        set r term",
    "    end",
    "end",
    "",
    "function __djarg_@@_take -S",
    "    # o: the positional that takes a word, from the command up,",
    "    # counted when `$argv[1]` is 1; extra: the last command's flags",
    "    set -l L $cmd",
    "    set o",
    "    while true",
    "        set -l s 0",
    "        while true",
    "            set -l i (contains -i -- \"$L:$s\" $__djarg_@@_pk)",
    "            if test -z \"$i\"",
    "                break",
    "            end",
    "            set -l e (string split ' ' -- $__djarg_@@_pv[$i])",
    "            set -l n $counts[(math $e[1] + 1)]",
    "            if test -z \"$n\"",
    "                set n 0",
    "            end",
    "            if test $e[2] -lt 0; or test $n -lt $e[2]",
    "                set o $e[1]",
    "                if test $argv[1] -eq 1",
    "                    set counts[(math $e[1] + 1)] (math $n + 1)",
    "                end",
    "                return",
    "            end",
    "            set s (math $s + 1)",
    "        end",
    "        set -l c (__djarg_@@_row cmds $L)",
    "        if string match -q -- '*f*' $c[2]; and test $L -ne 0",
    "            set L $c[1]",
    "        else",
    "            set extra $c[2]",
    "            return",
    "        end",
    "    end",
    "end",
    "",
    "function __djarg_@@_given -S",
    "    # the option found, met with the value `$argv[1]` -- empty for",
    "    # none: what it still needs, and what more it may take",
    "    if test -z \"$o\"",
    "        return",
    "    end",
    "    set -l e (__djarg_@@_row info $o)",
    "    if test $e[2] -eq 0",
    "        return",
    "    end",
    "    set -l got 0",
    "    if test -n \"$argv[1]\"",
    "        set got 1",
    "    end",
    "    set need (math $e[1] - $got)",
    "    if test $need -lt 0",
    "        set need 0",
    "    end",
    "    if test $e[2] -lt 0",
    "        set room -1",
    "    else if test $got -gt $e[1]",
    "        set room (math $e[2] - $got)",
    "    else",
    "        set room (math $e[2] - $e[1])",
    "    end",
    "    if test $need -gt 0; or test $room -ne 0",
    "        set opt $o",
    "    end",
    "end",
    "",
    "function __djarg_@@_step -S",
    "    # one word the parse meets",
    "    set -l w \"$argv[1]\"",
    "    set -l r",
    "    set -l t",
    "    set -l l",
    "    set -l o",
    "    __djarg_@@_class \"$w\"",
    "    switch $r",
    "        case mark",
    "            set only 1",
    "        case sub",
    "            set -l u $used[(math $l + 1)]",
    "            if test -z \"$u\"",
    "                set u 0",
    "            end",
    "            set used[(math $l + 1)] (math $u + 1)",
    "            set cmd $t",
    "            set cf (__djarg_@@_row cmds $cmd)[2]",
    "        case term",
    "            set cmd (__djarg_@@_row cmds $cmd)[1]",
    "            set cf (__djarg_@@_row cmds $cmd)[2]",
    "        case long win",
    "            set -l at =",
    "            if test $r = win",
    "                set at :",
    "            end",
    "            set -l name (string split -m 1 -- $at \"$w\")",
    "            set -l value \"$name[2]\"",
    "            if test $r = long",
    "                __djarg_@@_find \"$name[1]\"",
    "            else",
    "                set name (string sub -s 2 -- \"$name[1]\")",
    "                __djarg_@@_find \"--$name\"",
    "                if test -z \"$o\"",
    "                    __djarg_@@_find \"-$name\"",
    "                end",
    "            end",
    "            __djarg_@@_given \"$value\"",
    "        case short",
    "            set -l rest (string sub -s 2 -- \"$w\")",
    "            while test -n \"$rest\"",
    "                set -l name -(string sub -l 1 -- \"$rest\")",
    "                set rest (string sub -s 2 -- \"$rest\")",
    "                __djarg_@@_find \"$name\"",
    "                if test -z \"$o\"",
    "                    break",
    "                end",
    "                if test (__djarg_@@_row info $o)[2] -ne 0",
    "                    __djarg_@@_given \"$rest\"",
    "                    break",
    "                end",
    "            end",
    "        case '*'",
    "            __djarg_@@_take 1",
    "    end",
    "end",
    "",
    "function __djarg_@@_kind",
    "    # prints the values of kind `$argv[1]` -- c, choices",
    "    # `$argv[4]` on; f, files; d, directories -- after `$argv[2]`,",
    "    # for the text `$argv[3]`",
    "    switch $argv[1]",
    "        case c",
    "            for v in $argv[4..-1]",
    "                printf '%s%s\\n' \"$argv[2]\" \"$v\"",
    "            end",
    "        case f",
    "            set -l w (string escape -- \"$argv[3]\")",
    "            for v in (__fish_complete_path \"$w\")",
    "                printf '%s%s\\n' \"$argv[2]\" \"$v\"",
    "            end",
    "        case d",
    "            set -l w (string escape -- \"$argv[3]\")",
    "            for v in (__fish_complete_directories \"$w\" '')",
    "                printf '%s%s\\n' \"$argv[2]\" \"$v\"",
    "            end",
    "    end",
    "end",
    "",
    "function __djarg_@@_values",
    "    # prints the values option `$argv[1]` takes, after `$argv[2]`,",
    "    # for the text `$argv[3]`",
    "    set -l e (__djarg_@@_row info $argv[1])",
    "    set -l v",
    "    if test $e[3] = c; and test $e[5] -gt 0",
    "        set v $__djarg_@@_vv[$e[4]..(math $e[4] + $e[5] - 1)]",
    "    end",
    "    __djarg_@@_kind $e[3] \"$argv[2]\" \"$argv[3]\" $v",
    "end",
    "",
    "function __djarg_@@_names -S",
    "    # prints the names of kind `$argv[1]` -- s, subcommands; o,",
    "    # options -- each once with its description: at the command",
    "    # and, for options falling through, above it",
    "    set -l L $cmd",
    "    set -l seen",
    "    while true",
    "        set -l c (__djarg_@@_row cmds $L)",
    "        set -l i $c[4]",
    "        set -l z (math $c[4] + $c[5])",
    "        set -l names __djarg_@@_sn",
    "        set -l texts __djarg_@@_sd",
    "        if test $argv[1] = o",
    "            set i $c[6]",
    "            set z (math $c[6] + $c[7])",
    "            set names __djarg_@@_on",
    "            set texts __djarg_@@_od",
    "        end",
    "        while test $i -lt $z",
    "            set -l name $$names[1][$i]",
    "            if not contains -- \"$name\" $seen",
    "                set -a seen \"$name\"",
    "                printf '%s\\t%s\\n' \"$name\" $$texts[1][$i]",
    "            end",
    "            set i (math $i + 1)",
    "        end",
    "        if test $argv[1] = o; and string match -q -- '*f*' $c[2]",
    "            and test $L -ne 0",
    "            set L $c[1]",
    "        else",
    "            return",
    "        end",
    "    end",
    "end",
    "",
    "function __djarg_@@_offer -S",
    "    # prints what may come at the cursor, `$cur`",
    "    if test $need -gt 0",
    "        __djarg_@@_values $opt '' \"$cur\"",
    "        return",
    "    end",
    "    if test $only -eq 1",
    "        __djarg_@@_take 0",
    "        if test -n \"$o\"",
    "            __djarg_@@_values $o '' \"$cur\"",
    "        end",
    "        return",
    "    end",
    "    if test -n \"$opt\"",
    "        __djarg_@@_values $opt '' \"$cur\"",
    "    end",
    "    if string match -q -- '--*=*' \"$cur\"",
    "        set -l name (string split -m 1 -- = \"$cur\")",
    "        __djarg_@@_find \"$name[1]\"",
    "        if test -n \"$o\"",
    "            __djarg_@@_values $o \"$name[1]=\" \"$name[2]\"",
    "        end",
    "    else if string match -q -- '-*' \"$cur\"",
    "        __djarg_@@_names o",
    "    else",
    "        __djarg_@@_take 0",
    "        if test -n \"$o\"",
    "            __djarg_@@_values $o '' \"$cur\"",
    "        else if string match -q -- '*e*' \"$extra\"",
    "            __djarg_@@_kind f '' \"$cur\"",
    "        end",
    "        set -l c (__djarg_@@_row cmds $cmd)",
    "        set -l u $used[(math $cmd + 1)]",
    "        if test -z \"$u\"",
    "            set u 0",
    "        end",
    "        if test $c[5] -gt 0",
    "            and begin",
    "                test $c[3] -eq 0; or test $u -lt $c[3]",
    "            end",
    "            __djarg_@@_names s",
    "        else if test -z \"$o\"",
    "            and not string match -q -- '*e*' \"$extra\"",
    "            __djarg_@@_names o",
    "        end",
    "    end",
    "end",
    "",
    "function __djarg_@@_complete",
    "    # prints the completions for the command line",
    "    set -l tokens (commandline -opc)",
    "    set -l cur (__djarg_@@_unq (commandline -ct))",
    "    set -l cmd 0",
    "    set -l cf (__djarg_@@_row cmds 0)[2]",
    "    set -l opt",
    "    set -l need 0",
    "    set -l room 0",
    "    set -l only 0",
    "    set -l extra",
    "    set -l used",
    "    set -l counts",
    "    set -l r",
    "    set -l t",
    "    set -l l",
    "    set -l o",
    "    for w in $tokens[2..-1]",
    "        if test $need -gt 0",
    "            set need (math $need - 1)",
    "            if test $need -eq 0; and test $room -eq 0",
    "                set opt",
    "            end",
    "            continue",
    "        end",
    "        if test $only -eq 1",
    "            __djarg_@@_take 1",
    "            continue",
    "        end",
    "        if test -n \"$opt\"",
    "            if test \"$w\" = --",
    "                set opt",
    "                continue",
    "            end",
    "            __djarg_@@_class \"$w\"",
    "            if test $r = none; and test $room -ne 0",
    "                if test $room -gt 0",
    "                    set room (math $room - 1)",
    "                end",
    "                if test $room -eq 0",
    "                    set opt",
    "                end",
    "                continue",
    "            end",
    "            set opt",
    "        end",
    "        __djarg_@@_step \"$w\"",
    "    end",
    "    __djarg_@@_offer",
    "end"
};

// FISH_LINE_COUNT
//   constant: how many lines FISH_LINES holds.
static const size_t FISH_LINE_COUNT =
    sizeof(FISH_LINES) / sizeof(FISH_LINES[0]);

//==============================================================================
// 1.  QUOTING
//==============================================================================

/**
 * @brief Appends text as one single-quoted word after a space, as fish reads
 *        it.
 *
 * @param[in,out] _out     the buffer that gains the word, its failure kept in
 *                         it.
 * @param[in]     _dashes  what goes before the text, inside the quotes.
 * @param[in]     _text    the text.
 * @param[in]     _line    whether it is a line of a description, each control
 *                         character then a space.
 *
 * Text as one single-quoted word after a space -- `\` and `'` escaped with
 * a backslash, as fish reads them between single quotes -- and, as a line of
 * a description, each control character a space; `_dashes` before it, inside
 * the quotes.
 */
D_STATIC void
d_arg_internal_complete_fish_word(
    struct d_arg_buffer* _out,
    const char*          _dashes,
    struct d_pack_text   _text,
    bool                 _line
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   " '");
    (void)d_arg_buffer_append_cstr(_out,
                                   _dashes);

    // each byte
    for (size_t index = 0u; index < _text.length; ++index)
    {
        const char          c    = _text.data[index];
        const unsigned char byte = (unsigned char)c;

        if ( (c == '\\') ||
             (c == '\'') )
        {
            (void)d_arg_buffer_append_char(_out,
                                           '\\');
            (void)d_arg_buffer_append_char(_out,
                                           c);
        }
        else
        {
            (void)d_arg_buffer_append_char(
                _out,
                ( (_line) &&
                  ( (byte < 0x20u) ||
                    (byte == 0x7Fu) ) ) ? ' ' : c);
        }
    }

    (void)d_arg_buffer_append_char(_out,
                                   '\'');

    return;
}

/**
 * @brief Appends text as it goes in a comment.
 *
 * @param[in,out] _out   the buffer that gains the text, its failure kept in it.
 * @param[in]     _text  the text.
 *
 * Text in a comment: each control character a space.
 */
D_STATIC void
d_arg_internal_complete_fish_line(
    struct d_arg_buffer* _out,
    struct d_pack_text   _text
)
{
    // each byte
    for (size_t index = 0u; index < _text.length; ++index)
    {
        const unsigned char byte = (unsigned char)_text.data[index];

        (void)d_arg_buffer_append_char(_out,
                                       ( (byte < 0x20u) ||
                                         (byte == 0x7Fu) )
                                       ? ' '
                                       : _text.data[index]);
    }

    return;
}

//==============================================================================
// 2.  THE DATA
//==============================================================================

// d_arg_internal_complete_fish_value
//   type: writes the value of a list for one record of the model -- the
// break before it and the value -- or nothing, for a record the list leaves
// out.
typedef void (*d_arg_internal_complete_fish_value)(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out);

// d_arg_internal_complete_fish_table
//   struct: one list of the script's data: its name, how many records of
// the model it covers, and the function that writes each record's value.
struct d_arg_internal_complete_fish_table
{
    const char*                        name;
    size_t                             count;
    d_arg_internal_complete_fish_value value;
};

/**
 * @brief Appends a global list named after the program.
 *
 * @param[in]     _model  the model.
 * @param[in]     _ident  the program's identifier.
 * @param[in]     _table  the list's name, how many records it covers, and the
 *                        function that writes each one's value.
 * @param[in,out] _out    the buffer that gains the list, its failure kept in
 *                        it.
 *
 * A global list, named after the program: its first line, then a value for
 * each record the table covers, each on a line of its own.
 */
D_STATIC void
d_arg_internal_complete_fish_list(
    const struct d_arg_internal_model*               _model,
    struct d_pack_text                               _ident,
    const struct d_arg_internal_complete_fish_table* _table,
    struct d_arg_buffer*                             _out
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "set -g -- __djarg_");
    (void)d_arg_buffer_append_text(_out,
                                   _ident);
    (void)d_arg_buffer_printf(_out,
                              "_%s",
                              _table->name);

    // each record's value
    for (size_t index = 0u; index < _table->count; ++index)
    {
        _table->value(_model,
                      index,
                      _out);
    }

    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends the break before a list's next value.
 *
 * @param[in,out] _out  the buffer that gains it, its failure kept in it.
 *
 * The break before a list's next value.
 */
D_STATIC void
d_arg_internal_complete_fish_next(
    struct d_arg_buffer* _out
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   " \\\n   ");

    return;
}

/**
 * @brief Appends a command's row of the `cmds` list.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the command's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A command's row: its parent, switches and most subcommands, then where
 * its subcommand names begin and how many, and where its option names begin
 * and how many, counting from 1.
 */
D_STATIC void
d_arg_internal_complete_fish_cmds(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    const struct d_arg_internal_complete_command* const command =
        &_model->commands[_index];

    d_arg_internal_complete_fish_next(_out);
    (void)d_arg_buffer_printf(_out,
                              " '%zu %s %zu %zu %zu %zu %zu'",
                              command->parent,
                              command->flags,
                              command->most,
                              command->first_name + 1u,
                              command->names,
                              command->first_entry + 1u,
                              command->entries);

    return;
}

/**
 * @brief Appends a subcommand name's key, as "command:name".
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A subcommand name's key: its command and the name, as "command:name".
 */
D_STATIC void
d_arg_internal_complete_fish_sk(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    struct d_arg_buffer prefix = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_printf(&prefix,
                              "%zu:",
                              _model->names[_index].command);
    d_arg_internal_complete_fish_next(_out);
    d_arg_internal_complete_fish_word(
        _out,
        (prefix.data != NULL) ? prefix.data : "",
        d_arg_internal_text_of(_model->names[_index].name),
        false);
    _out->failed |= prefix.failed;
    d_arg_buffer_release(&prefix);

    return;
}

/**
 * @brief Appends a subcommand name.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A subcommand name.
 */
D_STATIC void
d_arg_internal_complete_fish_sn(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    d_arg_internal_complete_fish_next(_out);
    d_arg_internal_complete_fish_word(
        _out,
        "",
        d_arg_internal_text_of(_model->names[_index].name),
        false);

    return;
}

/**
 * @brief Appends the command a subcommand name leads to.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * The command a subcommand name leads to.
 */
D_STATIC void
d_arg_internal_complete_fish_st(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    d_arg_internal_complete_fish_next(_out);
    (void)d_arg_buffer_printf(_out,
                              " %zu",
                              _model->names[_index].target);

    return;
}

/**
 * @brief Appends a subcommand name's description.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A subcommand name's description.
 */
D_STATIC void
d_arg_internal_complete_fish_sd(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    d_arg_internal_complete_fish_next(_out);
    d_arg_internal_complete_fish_word(_out,
                                      "",
                                      _model->names[_index].summary,
                                      true);

    return;
}

/**
 * @brief Appends an option name's key, as "command:--name".
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * An option name's key: its command and the name with its dashes, as
 * "command:--name".
 */
D_STATIC void
d_arg_internal_complete_fish_ok(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    const struct d_arg_internal_complete_entry* const entry =
        &_model->entries[_index];
    struct d_arg_buffer                               prefix =
        D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_printf(&prefix,
                              "%zu:%s",
                              _model->options[entry->option].command,
                              (entry->long_name) ? "--" : "-");
    d_arg_internal_complete_fish_next(_out);
    d_arg_internal_complete_fish_word(
        _out,
        (prefix.data != NULL) ? prefix.data : "",
        d_arg_internal_text_of(entry->name),
        false);
    _out->failed |= prefix.failed;
    d_arg_buffer_release(&prefix);

    return;
}

/**
 * @brief Appends an option name, with its dashes.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * An option name, with its dashes.
 */
D_STATIC void
d_arg_internal_complete_fish_on(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    const struct d_arg_internal_complete_entry* const entry =
        &_model->entries[_index];

    d_arg_internal_complete_fish_next(_out);
    d_arg_internal_complete_fish_word(_out,
                                      (entry->long_name) ? "--" : "-",
                                      d_arg_internal_text_of(entry->name),
                                      false);

    return;
}

/**
 * @brief Appends the option a name stands for.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * The option a name stands for.
 */
D_STATIC void
d_arg_internal_complete_fish_ov(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    d_arg_internal_complete_fish_next(_out);
    (void)d_arg_buffer_printf(_out,
                              " %zu",
                              _model->entries[_index].option);

    return;
}

/**
 * @brief Appends an option name's description: its option's.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the name's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * An option name's description: its option's.
 */
D_STATIC void
d_arg_internal_complete_fish_od(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    d_arg_internal_complete_fish_next(_out);
    d_arg_internal_complete_fish_word(
        _out,
        "",
        _model->options[_model->entries[_index].option].summary,
        true);

    return;
}

/**
 * @brief Appends an option's row of the `info` list.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the option's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * An option's row: its fewest and most values after a name, the letter of
 * its kind, and where its choices begin, counting from 1, and how many.
 */
D_STATIC void
d_arg_internal_complete_fish_info(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    const struct d_arg_internal_complete_option* const option =
        &_model->options[_index];

    d_arg_internal_complete_fish_next(_out);
    (void)d_arg_buffer_printf(_out,
                              " '%d %d %c %zu %zu'",
                              option->least,
                              option->most,
                              KIND_LETTERS[option->kind],
                              option->first_choice + 1u,
                              option->choices);

    return;
}

/**
 * @brief Finds where a positional comes among its command's.
 *
 * @param[in] _model  the model.
 * @param[in] _index  the positional's place in the model.
 * @return how many positionals of the same command come before it.
 *
 * Where a positional comes among its command's: the positionals before it
 * under the same command.
 */
D_STATIC size_t
d_arg_internal_complete_fish_slot(
    const struct d_arg_internal_model* _model,
    size_t                             _index
)
{
    size_t slot = 0u;

    // each option before it
    for (size_t index = 0u; index < _index; ++index)
    {
        if ( (_model->options[index].slots != 0) &&
             (_model->options[index].command ==
              _model->options[_index].command) )
        {
            ++slot;
        }
    }

    return slot;
}

/**
 * @brief Appends a positional's key, as "command:slot".
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the option's place in the model; one that is no
 *                        positional writes nothing.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A positional's key, as "command:slot"; nothing for another option.
 */
D_STATIC void
d_arg_internal_complete_fish_pk(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    // a positional only
    if (_model->options[_index].slots == 0)
    {
        return;
    }

    d_arg_internal_complete_fish_next(_out);
    (void)d_arg_buffer_printf(
        _out,
        " '%zu:%zu'",
        _model->options[_index].command,
        d_arg_internal_complete_fish_slot(_model,
                                          _index));

    return;
}

/**
 * @brief Appends a positional's row: its option and the most values it takes.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the option's place in the model; one that is no
 *                        positional writes nothing.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A positional's row: its option and the most values it takes; nothing
 * for another option.
 */
D_STATIC void
d_arg_internal_complete_fish_pv(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    // a positional only
    if (_model->options[_index].slots == 0)
    {
        return;
    }

    d_arg_internal_complete_fish_next(_out);
    (void)d_arg_buffer_printf(_out,
                              " '%zu %d'",
                              _index,
                              _model->options[_index].slots);

    return;
}

/**
 * @brief Appends a choice.
 *
 * @param[in]     _model  the model.
 * @param[in]     _index  the choice's place in the model.
 * @param[in,out] _out    the buffer that gains the value, its failure kept in
 *                        it.
 *
 * A choice.
 */
D_STATIC void
d_arg_internal_complete_fish_vv(
    const struct d_arg_internal_model* _model,
    size_t                             _index,
    struct d_arg_buffer*               _out
)
{
    d_arg_internal_complete_fish_next(_out);
    d_arg_internal_complete_fish_word(
        _out,
        "",
        d_arg_internal_text_of(_model->choices.items[_index]),
        false);

    return;
}

/**
 * @brief Appends every list of the model.
 *
 * @param[in]     _model  the model.
 * @param[in]     _ident  the program's identifier.
 * @param[in,out] _out    the buffer that gains the lists, its failure kept in
 *                        it.
 *
 * Every list of the model.
 */
D_STATIC void
d_arg_internal_complete_fish_data(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _ident,
    struct d_arg_buffer*               _out
)
{
    const struct d_arg_internal_complete_fish_table lists[] =
    {
        { "cmds", _model->command_count, d_arg_internal_complete_fish_cmds },
        { "sk",   _model->name_count,    d_arg_internal_complete_fish_sk   },
        { "sn",   _model->name_count,    d_arg_internal_complete_fish_sn   },
        { "st",   _model->name_count,    d_arg_internal_complete_fish_st   },
        { "sd",   _model->name_count,    d_arg_internal_complete_fish_sd   },
        { "ok",   _model->entry_count,   d_arg_internal_complete_fish_ok   },
        { "on",   _model->entry_count,   d_arg_internal_complete_fish_on   },
        { "ov",   _model->entry_count,   d_arg_internal_complete_fish_ov   },
        { "od",   _model->entry_count,   d_arg_internal_complete_fish_od   },
        { "info", _model->option_count,  d_arg_internal_complete_fish_info },
        { "pk",   _model->option_count,  d_arg_internal_complete_fish_pk   },
        { "pv",   _model->option_count,  d_arg_internal_complete_fish_pv   },
        { "vv",   _model->choices.count, d_arg_internal_complete_fish_vv   }
    };
    const size_t count = sizeof(lists) / sizeof(lists[0]);

    // each list
    for (size_t index = 0u; index < count; ++index)
    {
        d_arg_internal_complete_fish_list(_model,
                                          _ident,
                                          &lists[index],
                                          _out);
    }

    return;
}

//==============================================================================
// 3.  THE SCRIPT
//==============================================================================

/*
d_arg_internal_complete_fish
  The header, the data, the functions, and the completion registered for
the program, fish's own file completion turned off.
*/
int
d_arg_internal_complete_fish(
    const struct d_arg_internal_model* _model,
    struct d_pack_text                 _program,
    struct d_arg_buffer*               _out
)
{
    struct d_arg_buffer ident  = D_ARG_BUFFER_INIT;
    int                 status = d_arg_internal_complete_ident(_program,
                                                               &ident);

    (void)d_arg_buffer_append_cstr(_out,
                                   "# fish completion for ");
    d_arg_internal_complete_fish_line(_out,
                                      _program);
    (void)d_arg_buffer_append_cstr(
        _out,
        ", generated by djinterp's arg_parser.\n"
        "# Source it in fish, or save it as <program>.fish in a directory "
        "of\n# $fish_complete_path.\n\n");
    d_arg_internal_complete_fish_data(_model,
                                      d_arg_buffer_text(&ident),
                                      _out);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');
    (void)d_arg_internal_complete_lines(FISH_LINES,
                                        FISH_LINE_COUNT,
                                        d_arg_buffer_text(&ident),
                                        _out);
    (void)d_arg_buffer_append_cstr(_out,
                                   "\ncomplete -c");
    d_arg_internal_complete_fish_word(_out,
                                      "",
                                      _program,
                                      false);
    (void)d_arg_buffer_append_cstr(_out,
                                   " -e\ncomplete -c");
    d_arg_internal_complete_fish_word(_out,
                                      "",
                                      _program,
                                      false);
    (void)d_arg_buffer_append_cstr(_out,
                                   " -f -a '(__djarg_");
    (void)d_arg_buffer_append_text(_out,
                                   d_arg_buffer_text(&ident));
    (void)d_arg_buffer_append_cstr(_out,
                                   "_complete)'\n");
    status = ( (status == D_ARG_OK) &&
               (_out->failed == 0) )
             ? D_ARG_OK
             : D_ARG_ERROR_OUT_OF_MEMORY;
    d_arg_buffer_release(&ident);

    return status;
}

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION
