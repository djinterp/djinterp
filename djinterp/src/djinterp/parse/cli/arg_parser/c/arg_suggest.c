/*******************************************************************************
* djinterp [parse]                                                 arg_suggest.c
*
* "Did you mean" suggestions, declared in arg_internal.h (9.4).
*   The optimal string alignment distance between two names -- Levenshtein's,
* with the swap of two neighbours counting as one edit -- computed only in
* the band of cells within the greatest distance a suggestion may have, so
* that it needs no memory beyond three short rows on the stack and stops as
* soon as a row has nothing within it. The name an unexpected argument most
* likely meant: among the options the command would have matched for `-x`,
* `--name` or `/name` -- its own and its option groups' -- or among the
* subcommands it would have taken for anything not starting with a dash --
* its own, its option groups', and, with subcommand fallthrough, its
* parents' -- and those of the commands the parse went on to, the one
* nearest, when no other is as near; compared as the option or subcommand
* compares names, case folded or underscores dropped when it ignores them.
* And the lines the message about unexpected arguments gains, "Did you mean
* '--verbose'?", when the parser asks for them.
*   Without D_INTERNAL_ARG_PARSER_SUGGESTIONS the message gains nothing, and
* only that step is compiled.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_suggest.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "./arg_internal.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, lists, classifiers
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // D_ARG_PARSER_SUGGESTIONS
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

//==============================================================================
// 1.  DISTANCE
//==============================================================================

// D_INTERNAL_ARG_SUGGEST_REACH
//   constant: the greatest distance the band is computed for, the knob's
// upper bound (cfg_arg_parser.h holds D_CFG_ARG_PARSER_SUGGESTION_DISTANCE to
// it); a larger limit is taken as this one.
#define D_INTERNAL_ARG_SUGGEST_REACH 8u

// D_INTERNAL_ARG_SUGGEST_BAND
//   constant: the cells a row of the band can hold: the reach on either side
// of the diagonal, and the diagonal.
#define D_INTERNAL_ARG_SUGGEST_BAND  ((2u * D_INTERNAL_ARG_SUGGEST_REACH) + 1u)

// d_arg_internal_suggest_band
//   struct: the distance table's band: `reach` cells on either side of the
// diagonal, of the three rows the recurrence reads, row i in `cells[i % 3]`
// with d[i][j] at column j - i + reach.
struct d_arg_internal_suggest_band
{
    size_t reach;
    size_t cells[3][D_INTERNAL_ARG_SUGGEST_BAND];
};

/*
d_arg_internal_suggest_least
  The smaller of two counts.
*/
D_STATIC size_t
d_arg_internal_suggest_least(
    size_t _a,
    size_t _b
)
{
    return (_a < _b) ? _a : _b;
}

/*
d_arg_internal_suggest_cell
  d[i][j], at column `_t` of row `_i` (j = i + t - reach, at least 1), as
the optimal string alignment recurrence takes it: a substitution from the
cell before above, a deletion from the cell above, an insertion from the
cell before, and the swap of two neighbours from two rows up; every
distance past the limit is the limit plus one.
*/
D_STATIC size_t
d_arg_internal_suggest_cell(
    const struct d_arg_internal_suggest_band* _band,
    struct d_pack_text                        _a,
    struct d_pack_text                        _b,
    size_t                                    _i,
    size_t                                    _t
)
{
    const size_t  reach    = _band->reach;
    const size_t* current  = _band->cells[_i % 3u];
    const size_t* previous = _band->cells[(_i + 2u) % 3u];
    const size_t* earlier  = _band->cells[(_i + 1u) % 3u];
    const size_t  j        = _i + _t - reach;
    size_t        best     = previous[_t] +
                             ((_a.data[_i - 1u] != _b.data[j - 1u]) ? 1u : 0u);

    // a deletion, from the cell above
    if (_t < (2u * reach))
    {
        best = d_arg_internal_suggest_least(best,
                                            previous[_t + 1u] + 1u);
    }

    // an insertion, from the cell before
    if (_t > 0u)
    {
        best = d_arg_internal_suggest_least(best,
                                            current[_t - 1u] + 1u);
    }

    // two neighbours swapped
    if ( (_i > 1u)                             &&
         (j > 1u)                              &&
         (_a.data[_i - 1u] == _b.data[j - 2u]) &&
         (_a.data[_i - 2u] == _b.data[j - 1u]) )
    {
        best = d_arg_internal_suggest_least(best,
                                            earlier[_t] + 1u);
    }

    return d_arg_internal_suggest_least(best,
                                        reach + 1u);
}

/*
d_arg_internal_suggest_row
  Row `_i` of the band: a cell outside the table is past the limit, and
d[i][0] is i -- in the band only for a row within the reach; returns the
row's least cell, which the caller stops on when it passes the limit.
*/
D_STATIC size_t
d_arg_internal_suggest_row(
    struct d_arg_internal_suggest_band* _band,
    struct d_pack_text                  _a,
    struct d_pack_text                  _b,
    size_t                              _i
)
{
    const size_t reach   = _band->reach;
    size_t*      current = _band->cells[_i % 3u];
    size_t       least   = reach + 1u;

    // each column of the band
    for (size_t t = 0u; t <= (2u * reach); ++t)
    {
        // before the table, past it, its first column, or within
        if ( ((_i + t) < reach) ||
             ((_i + t - reach) > _b.length) )
        {
            current[t] = reach + 1u;
        }
        else if ((_i + t) == reach)
        {
            current[t] = _i;
        }
        else
        {
            current[t] = d_arg_internal_suggest_cell(_band,
                                                     _a,
                                                     _b,
                                                     _i,
                                                     t);
        }

        least = d_arg_internal_suggest_least(least,
                                             current[t]);
    }

    return least;
}

/*
d_arg_internal_edit_distance
  Only the band of cells within the limit of the diagonal can hold a
distance within it, so only that is computed, three rows of it at a time,
row 0 being d[0][j] = j; the rows stop as soon as one has nothing within
the limit, and lengths further apart than the limit stop at once. No cell
of the table reads one outside it -- each neighbour of a cell inside lies
inside, or in the first column, set apart -- so row 0 holds only the cells
of the table, the others never read.
*/
size_t
d_arg_internal_edit_distance(
    struct d_pack_text _a,
    struct d_pack_text _b,
    size_t             _limit
)
{
    struct d_arg_internal_suggest_band band  =
    {
        d_arg_internal_suggest_least(_limit,
                                     D_INTERNAL_ARG_SUGGEST_REACH),
        { { 0u } }
    };
    size_t                             least = 0u;

    // lengths further apart than the limit
    if ( (_a.length > (_b.length + band.reach)) ||
         (_b.length > (_a.length + band.reach)) )
    {
        return band.reach + 1u;
    }

    // row 0, d[0][j] = j: as many insertions as columns, as far as either
    // the second text or the band reaches
    for (size_t j = 0u;
         (j <= _b.length) && (j <= band.reach);
         ++j)
    {
        band.cells[0][band.reach + j] = j;
    }

    // each later row, until one has nothing within the limit
    for (size_t i = 1u; (least <= band.reach) && (i <= _a.length); ++i)
    {
        least = d_arg_internal_suggest_row(&band,
                                           _a,
                                           _b,
                                           i);
    }

    return (least > band.reach)
           ? (band.reach + 1u)
           : band.cells[_a.length % 3u][_b.length + band.reach - _a.length];
}

//==============================================================================
// 2.  CANDIDATES
//==============================================================================

// d_arg_internal_suggest_target
//   struct: what an unexpected argument names: its name -- without its dashes
// or slash and value for an option's, the whole argument for a
// subcommand's --; whether it names an option, else a subcommand; whether
// candidates are written after a slash, as a Windows-style option is; and
// whether it names anything at all.
struct d_arg_internal_suggest_target
{
    struct d_pack_text name;
    bool               option;
    bool               slash;
    bool               named;
};

// d_arg_internal_suggestion
//   struct: a search for the nearest name: the argument as given, and what it
// names; the nearest distance so far, and whether another name, written
// differently, is as near; the nearest name, written; and scratch for a
// candidate written and for both names folded.
struct d_arg_internal_suggestion
{
    struct d_pack_text                   given;
    struct d_arg_internal_suggest_target target;
    size_t                               distance;
    bool                                 tied;
    struct d_arg_buffer                  best;
    struct d_arg_buffer                  written;
    struct d_arg_buffer                  left;
    struct d_arg_buffer                  right;
};

/*
d_arg_internal_suggest_fold
  A name as a command or option compares it: ASCII case folded when it
ignores case, underscores dropped when it ignores them; written into
`_out`, which is cleared first.
*/
D_STATIC struct d_pack_text
d_arg_internal_suggest_fold(
    struct d_pack_text   _name,
    bool                 _case,
    bool                 _underscore,
    struct d_arg_buffer* _out
)
{
    d_arg_buffer_clear(_out);

    // each character kept, folded
    for (size_t at = 0u; at < _name.length; ++at)
    {
        // an underscore dropped when ignored
        if ( (!_underscore) ||
             (_name.data[at] != '_') )
        {
            (void)d_arg_buffer_append_char(_out,
                                           (_case)
                                           ? d_arg_internal_to_lower(
                                                 _name.data[at])
                                           : _name.data[at]);
        }
    }

    return d_arg_buffer_text(_out);
}

/*
d_arg_internal_suggest_write
  A candidate as the user would type it: after the slash for a
Windows-style argument; else after its dashes -- one for a short name, two
for a long one, none for a subcommand's.
*/
D_STATIC struct d_pack_text
d_arg_internal_suggest_write(
    struct d_arg_internal_suggestion* _search,
    struct d_pack_text                _candidate,
    size_t                            _dashes
)
{
    d_arg_buffer_clear(&_search->written);

    // the slash, or the dashes
    if (_search->target.slash)
    {
        (void)d_arg_buffer_append_char(&_search->written,
                                       '/');
    }
    else
    {
        (void)d_arg_buffer_append(&_search->written,
                                  "--",
                                  _dashes);
    }

    (void)d_arg_buffer_append_text(&_search->written,
                                   _candidate);

    return d_arg_buffer_text(&_search->written);
}

/*
d_arg_internal_suggest_consider
  One candidate name: its distance from the argument's, both compared as
the candidate's command or option compares them; the nearest kept, and a
tie noted when another name, written differently, is as near. Not a
suggestion: a candidate written exactly as the argument was given, or one
whose distance is more than half the longer name -- so that "u" never
suggests "s", nor "ab" "cd", while "strat" suggests "start".
*/
D_STATIC void
d_arg_internal_suggest_consider(
    struct d_arg_internal_suggestion* _search,
    struct d_pack_text                _candidate,
    size_t                            _dashes,
    uint32_t                          _folding
)
{
    const bool               fold_case  = ((_folding & 1u) != 0u);
    const bool               underscore = ((_folding & 2u) != 0u);
    const struct d_pack_text written    = d_arg_internal_suggest_write(
                                              _search,
                                              _candidate,
                                              _dashes);
    const struct d_pack_text left       = d_arg_internal_suggest_fold(
                                              _search->target.name,
                                              fold_case,
                                              underscore,
                                              &_search->left);
    const struct d_pack_text right      = d_arg_internal_suggest_fold(
                                              _candidate,
                                              fold_case,
                                              underscore,
                                              &_search->right);
    const size_t             longer     = (left.length > right.length)
                                          ? left.length
                                          : right.length;
    const size_t             distance   =
        d_arg_internal_edit_distance(left,
                                     right,
                                     D_INTERNAL_ARG_PARSER_SUGGESTION_DISTANCE);

    // the argument itself, too far to resemble it, or no nearer
    if ( (d_arg_internal_equal(written,
                               _search->given)) ||
         ((2u * distance) > longer)             ||
         (distance > _search->distance) )
    {
        return;
    }

    // as near: a tie, unless it is the same name
    if (distance == _search->distance)
    {
        _search->tied = ( (_search->tied) ||
                          (!d_arg_internal_equal(
                               written,
                               d_arg_buffer_text(&_search->best))) );

        return;
    }

    _search->distance = distance;
    _search->tied     = false;
    d_arg_buffer_clear(&_search->best);
    (void)d_arg_buffer_append_text(&_search->best,
                                   written);

    return;
}

/*
d_arg_internal_suggest_folding
  The way a record compares names, as the bits consider takes: 1 for case
ignored, 2 for underscores ignored.
*/
D_STATIC uint32_t
d_arg_internal_suggest_folding(
    bool _case,
    bool _underscore
)
{
    return ((_case) ? 1u : 0u) |
           ((_underscore) ? 2u : 0u);
}

/*
d_arg_internal_suggest_option
  One option's short and long names; none of a hidden option's.
*/
D_STATIC void
d_arg_internal_suggest_option(
    struct d_arg_internal_suggestion* _search,
    const struct d_arg_option*        _option
)
{
    const uint32_t folding = d_arg_internal_suggest_folding(
                                 d_arg_option_has_flag(
                                     _option,
                                     D_ARG_OPTION_IGNORE_CASE),
                                 d_arg_option_has_flag(
                                     _option,
                                     D_ARG_OPTION_IGNORE_UNDERSCORE));

    // a hidden option is not offered
    if (d_arg_option_group(_option)[0] == '\0')
    {
        return;
    }

    // each short name
    for (size_t index = 0u; index < _option->short_names.count; ++index)
    {
        d_arg_internal_suggest_consider(
            _search,
            d_arg_internal_text_of(_option->short_names.items[index]),
            1u,
            folding);
    }

    // each long name
    for (size_t index = 0u; index < _option->long_names.count; ++index)
    {
        d_arg_internal_suggest_consider(
            _search,
            d_arg_internal_text_of(_option->long_names.items[index]),
            2u,
            folding);
    }

    return;
}

/*
d_arg_internal_suggest_options
  The options a command matches: its own, and its enabled option groups' --
not their parents again. Never its parents': a command that falls through
hands an option it does not know to its fallthrough parent, so the command
an option is reported by never falls through.
*/
D_STATIC void
d_arg_internal_suggest_options(
    struct d_arg_internal_suggestion* _search,
    const struct d_arg_command*       _command
)
{
    // its own options
    for (size_t index = 0u; index < _command->options.count; ++index)
    {
        d_arg_internal_suggest_option(_search,
                                      _command->options.items[index]);
    }

    // its enabled option groups
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        const struct d_arg_command* sub = _command->subcommands.items[index];

        // a nameless one that is not disabled
        if ( (d_arg_command_name(sub)[0] == '\0') &&
             (!d_arg_command_has_flag(sub,
                                      D_ARG_COMMAND_DISABLED)) )
        {
            d_arg_internal_suggest_options(_search,
                                           sub);
        }
    }

    return;
}

/*
d_arg_internal_suggest_command
  One subcommand's name and aliases, unless it is disabled or hidden.
*/
D_STATIC void
d_arg_internal_suggest_command(
    struct d_arg_internal_suggestion* _search,
    const struct d_arg_command*       _sub
)
{
    const uint32_t folding = d_arg_internal_suggest_folding(
                                 d_arg_command_has_flag(
                                     _sub,
                                     D_ARG_COMMAND_IGNORE_CASE),
                                 d_arg_command_has_flag(
                                     _sub,
                                     D_ARG_COMMAND_IGNORE_UNDERSCORE));

    // a disabled or hidden subcommand is not offered
    if ( (d_arg_command_has_flag(_sub,
                                 D_ARG_COMMAND_DISABLED)) ||
         (d_arg_command_group(_sub)[0] == '\0') )
    {
        return;
    }

    d_arg_internal_suggest_consider(
        _search,
        d_arg_internal_text_of(d_arg_command_name(_sub)),
        0u,
        folding);

    // each alias
    for (size_t index = 0u; index < _sub->aliases.count; ++index)
    {
        d_arg_internal_suggest_consider(
            _search,
            d_arg_internal_text_of(_sub->aliases.items[index]),
            0u,
            folding);
    }

    return;
}

/*
d_arg_internal_suggest_commands
  The subcommands a command takes itself: its own named ones, and those of
its option groups -- not their parents again.
*/
D_STATIC void
d_arg_internal_suggest_commands(
    struct d_arg_internal_suggestion* _search,
    const struct d_arg_command*       _command
)
{
    // each subcommand, an option group's in turn
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        const struct d_arg_command* sub = _command->subcommands.items[index];

        // an option group's subcommands, or a subcommand
        if (d_arg_command_name(sub)[0] == '\0')
        {
            d_arg_internal_suggest_commands(_search,
                                            sub);
        }
        else
        {
            d_arg_internal_suggest_command(_search,
                                           sub);
        }
    }

    return;
}

/*
d_arg_internal_suggest_climb
  The subcommands a command takes, then, while subcommands fall through,
its parent's, and so on up, as CLI11's _valid_subcommand climbs to them.
*/
D_STATIC void
d_arg_internal_suggest_climb(
    struct d_arg_internal_suggestion* _search,
    const struct d_arg_command*       _command
)
{
    const struct d_arg_command* at = _command;

    // the command, then each parent its subcommands fall through to
    while (at != NULL)
    {
        d_arg_internal_suggest_commands(_search,
                                        at);
        at = (d_arg_command_has_flag(at,
                                     D_ARG_COMMAND_SUBCOMMAND_FALLTHROUGH))
             ? at->parent
             : NULL;
    }

    return;
}

/*
d_arg_internal_suggest_reached
  The commands the last parse used under this one, and under them in turn:
an argument they did not take can end up with a command above them -- one
that falls through, say -- while the user meant one of theirs.
*/
D_STATIC void
d_arg_internal_suggest_reached(
    struct d_arg_internal_suggestion* _search,
    const struct d_arg_command*       _command
)
{
    // each subcommand used, and those it used
    for (size_t index = 0u; index < _command->parsed_subcommands.count; ++index)
    {
        const struct d_arg_command* sub =
            _command->parsed_subcommands.items[index];

        // its options, or its subcommands
        if (_search->target.option)
        {
            d_arg_internal_suggest_options(_search,
                                           sub);
        }
        else
        {
            d_arg_internal_suggest_commands(_search,
                                            sub);
        }

        d_arg_internal_suggest_reached(_search,
                                       sub);
    }

    return;
}

//==============================================================================
// 3.  THE SUGGESTION
//==============================================================================

/*
d_arg_internal_suggest_target
  What an argument names: for `-x`, `--name` or `/name`, an option, by its
name without the dashes or slash and without the value after `=` -- or,
for a Windows-style one, `:` -- written after a slash for a slash; for
anything else, a subcommand, by the whole argument, unless the argument is
empty or starts with a dash, as `--` and a negative number do, when it
names nothing.
*/
D_STATIC struct d_arg_internal_suggest_target
d_arg_internal_suggest_target(
    const struct d_arg_extra* _extra
)
{
    const struct d_pack_text text  = _extra->text;
    const int                kind  = _extra->classifier;
    const bool               slash = (kind == (int)D_ARG_CLASS_WINDOWS_STYLE);
    const size_t             skip  = (kind == (int)D_ARG_CLASS_LONG)
                                     ? 2u
                                     : ( (kind == (int)D_ARG_CLASS_SHORT) ||
                                         (slash) )
                                       ? 1u
                                       : 0u;
    size_t                   end   = skip;

    // a subcommand's name -- which never starts with a dash -- or nothing
    if (skip == 0u)
    {
        const struct d_arg_internal_suggest_target bare =
        {
            text,                       // name
            false,                      // option
            false,                      // slash
            ( (text.length > 0u) &&
              (text.data[0] != '-') )   // named
        };

        return bare;
    }

    // up to the value
    while ( (end < text.length)          &&
            (text.data[end] != '=')      &&
            ( (!slash) ||
              (text.data[end] != ':') ) )
    {
        end++;
    }

    const struct d_arg_internal_suggest_target option =
    {
        d_arg_internal_span(text.data + skip,
                            end - skip),  // name
        true,                             // option
        slash,                            // slash
        true                              // named
    };

    return option;
}

/*
d_arg_internal_suggest_search
  Every candidate the argument calls for: the options the command matches,
or the subcommands it takes; then those of the commands the parse went on
to under it.
*/
D_STATIC void
d_arg_internal_suggest_search(
    struct d_arg_internal_suggestion* _search,
    const struct d_arg_command*       _command
)
{
    // an option's name, or a subcommand's
    if (_search->target.option)
    {
        d_arg_internal_suggest_options(_search,
                                       _command);
    }
    else
    {
        d_arg_internal_suggest_climb(_search,
                                     _command);
    }

    d_arg_internal_suggest_reached(_search,
                                   _command);

    return;
}

/**
 * @brief Writes the name an unexpected argument most likely meant.
 *
 * @note For `-x`, `--name` or `/name`, the short or long name of an option
 *       the command would have matched -- its own and its enabled option
 *       groups' -- hidden ones left out, the name and value after `=` (or
 *       `:`) compared without its dashes or slash; for anything else not
 *       starting with a dash, the name or alias of a subcommand it would
 *       have taken -- its own, its option groups', and, with subcommand
 *       fallthrough, its parents' -- disabled and hidden ones left out; and,
 *       either way, those of the commands the last parse used under it,
 *       where an argument they did not take may have meant one of theirs.
 *       Names are compared as their option or subcommand compares them,
 *       case or underscores ignored where it ignores them. Written as the
 *       user would type it -- with its dashes, or after a slash for a
 *       Windows-style argument -- only when one name is nearest, within
 *       D_INTERNAL_ARG_PARSER_SUGGESTION_DISTANCE edits and half the longer
 *       name, and is not the argument itself; and only when no scratch ran
 *       out of memory, which could have hidden a nearer name. The scratch is
 *       freed before returning.
 *
 * @param[in]     _command  the command that did not expect the argument.
 * @param[in]     _extra    the argument, with what it was taken to be.
 * @param[in,out] _out      receives the name, appended.
 * @return `true` when a name was written.
 */
D_STATIC bool
d_arg_internal_suggest(
    const struct d_arg_command* _command,
    const struct d_arg_extra*   _extra,
    struct d_arg_buffer*        _out
)
{
    struct d_arg_internal_suggestion search =
    {
        _extra->text,                                   // given
        d_arg_internal_suggest_target(_extra),          // target
        D_INTERNAL_ARG_PARSER_SUGGESTION_DISTANCE + 1u, // distance
        false,                                          // tied
        D_ARG_BUFFER_INIT,                              // best
        D_ARG_BUFFER_INIT,                              // written
        D_ARG_BUFFER_INIT,                              // left
        D_ARG_BUFFER_INIT                               // right
    };

    // the candidates, when the argument names anything
    if (search.target.named)
    {
        d_arg_internal_suggest_search(&search,
                                      _command);
    }

    const bool found = ( (search.best.length > 0u)    &&
                         (!search.tied)               &&
                         (search.best.failed == 0)    &&
                         (search.written.failed == 0) &&
                         (search.left.failed == 0)    &&
                         (search.right.failed == 0)   &&
                         (d_arg_buffer_append_text(
                              _out,
                              d_arg_buffer_text(&search.best)) == D_ARG_OK) );

    d_arg_buffer_release(&search.right);
    d_arg_buffer_release(&search.left);
    d_arg_buffer_release(&search.written);
    d_arg_buffer_release(&search.best);

    return found;
}

/*
d_arg_internal_suggest_line
  One argument's line, when it has a suggestion: "Did you mean '<name>'?",
with " for '<argument>'" before the question mark among several. `_name`
is scratch, cleared first.
*/
D_STATIC void
d_arg_internal_suggest_line(
    const struct d_arg_command* _command,
    const struct d_arg_extra*   _extra,
    bool                        _several,
    struct d_arg_buffer*        _name,
    struct d_arg_buffer*        _text
)
{
    d_arg_buffer_clear(_name);

    // no suggestion, no line
    if (!d_arg_internal_suggest(_command,
                                _extra,
                                _name))
    {
        return;
    }

    (void)d_arg_buffer_printf(_text,
                              "\nDid you mean '%.*s'",
                              d_arg_internal_print_length(
                                  d_arg_buffer_text(_name)),
                              _name->data);

    // among several, which argument it is for
    if (_several)
    {
        (void)d_arg_buffer_printf(_text,
                                  " for '%.*s'",
                                  d_arg_internal_print_length(_extra->text),
                                  _extra->text.data);
    }

    (void)d_arg_buffer_append_char(_text,
                                   '?');

    return;
}

/*
d_arg_internal_suggest_extras
  Each argument's line in turn. Arguments from the stack, which the parse
has not classified, are classified as the command would take them. A line
that could not be written has failed the message's buffer, which the
caller reads.
*/
void
d_arg_internal_suggest_extras(
    const struct d_arg_command* _command,
    const struct d_pack_text*   _stack,
    const struct d_arg_extra*   _unused,
    size_t                      _count,
    struct d_arg_buffer*        _text
)
{
    struct d_arg_buffer name = D_ARG_BUFFER_INIT;

    // only when the parser asks for them
    if ( (_command->parser == NULL) ||
         ((_command->parser->flags & D_ARG_PARSER_SUGGESTIONS) == 0u) )
    {
        return;
    }

    // each argument in turn
    for (size_t index = 0u; index < _count; ++index)
    {
        const struct d_pack_text text  = (_stack != NULL)
                                         ? _stack[_count - 1u - index]
                                         : _unused[index].text;
        const struct d_arg_extra extra =
        {
            text,                                         // text
            (_stack != NULL)
            ? d_arg_internal_recognize(_command,
                                       text,
                                       true)
            : _unused[index].classifier                   // classifier
        };

        d_arg_internal_suggest_line(_command,
                                    &extra,
                                    (_count > 1u),
                                    &name,
                                    _text);
    }

    d_arg_buffer_release(&name);

    return;
}

#else

/*
d_arg_internal_suggest_extras
  Suggestions are not compiled: the message gains nothing.
*/
void
d_arg_internal_suggest_extras(
    const struct d_arg_command* _command,
    const struct d_pack_text*   _stack,
    const struct d_arg_extra*   _unused,
    size_t                      _count,
    struct d_arg_buffer*        _text
)
{
    (void)_command;
    (void)_stack;
    (void)_unused;
    (void)_count;
    (void)_text;

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS
