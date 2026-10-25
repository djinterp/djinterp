/*******************************************************************************
* djinterp [tools]                                                      dcmake.c
*
* Reads the CMake language into records: see dcmake.h.
*   One pass over the text, by hand, in the order cmake-language(7) gives:
* the file is elements -- a command, or comments and space -- each ending its
* line, and a command is a name, `(`, arguments and `)`, with comments and
* newlines free among the arguments. A second pass over the commands pairs
* each block's opener with its closer.
*
* path:      /src/djinterp/tools/dawk/ext/cmake/dcmake.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/cmake/dcmake.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, memset, memcmp, strlen


// d_internal_reader
//   struct: the reader's place in the text, and the records it grows.
struct d_internal_reader
{
    struct d_cmake* cmake;
    const char*     text;
    size_t          length;
    size_t          at;
    size_t          line;
    size_t          line_start;
    size_t          command_line;
    size_t          last_line;
    size_t          command_capacity;
    size_t          argument_capacity;
    size_t          comment_capacity;
    char*           problem;
    size_t          capacity;
    bool            failed;
};


/*
d_internal_fail
  Records where reading stopped and why; the first failure is the one kept.
*/
static bool
d_internal_fail(
    struct d_internal_reader* _reader,
    size_t                    _line,
    size_t                    _column,
    const char*               _why
)
{
    // a later failure follows from the first, and would only mislead
    if (!_reader->failed)
    {
        _reader->failed              = true;
        _reader->cmake->error_line   = _line;
        _reader->cmake->error_column = _column;

        // the reason, where there is room for it
        if ( (_reader->problem) &&
             (_reader->capacity > 0u) )
        {
            (void)snprintf(_reader->problem, _reader->capacity,
                           "line %lu, column %lu: %s",
                           (unsigned long)_line, (unsigned long)_column,
                           _why);
        }
    }

    return false;
}


/*
d_internal_fail_here
  A failure at the reader's place.
*/
static bool
d_internal_fail_here(
    struct d_internal_reader* _reader,
    const char*               _why
)
{
    return d_internal_fail(_reader, _reader->line,
                           (_reader->at - _reader->line_start) + 1u, _why);
}


/*
d_internal_grow
  Makes room for one more record in an array of `_size`-byte records.
*/
static bool
d_internal_grow(
    void**  _array,
    size_t* _capacity,
    size_t  _count,
    size_t  _size
)
{
    // there is room already
    if (_count < *_capacity)
    {
        return true;
    }

    const size_t wanted = (*_capacity == 0u) ? 16u : (*_capacity * 2u);
    void* const  grown  = realloc(*_array, wanted * _size);

    // the records so far stay where they were when memory runs out
    if (!grown)
    {
        return false;
    }

    *_array    = grown;
    *_capacity = wanted;

    return true;
}


/*
d_internal_newline_length
  The length of the line ending at `_at`: 1 for LF, 2 for CR LF, 0 for none.
*/
static size_t
d_internal_newline_length(
    const struct d_internal_reader* _reader,
    size_t                          _at
)
{
    // LF ends a line on its own
    if ( (_at < _reader->length) &&
         (_reader->text[_at] == '\n') )
    {
        return 1u;
    }

    // CR ends one only when LF follows it
    if ( ((_at + 1u) < _reader->length) &&
         (_reader->text[_at] == '\r')    &&
         (_reader->text[_at + 1u] == '\n') )
    {
        return 2u;
    }

    return 0u;
}


/*
d_internal_take_newline
  Moves past the line ending at the reader's place, if there is one.
*/
static bool
d_internal_take_newline(
    struct d_internal_reader* _reader
)
{
    const size_t span = d_internal_newline_length(_reader, _reader->at);

    // nothing ends here
    if (span == 0u)
    {
        return false;
    }

    // the first ending says how the file ends its lines
    if (_reader->line == 1u)
    {
        _reader->cmake->crlf = (span == 2u);
    }

    _reader->at         += span;
    _reader->line       += 1u;
    _reader->line_start  = _reader->at;

    return true;
}


/*
d_internal_is_space
  Space between elements: a blank, a tab, or a CR that ends no line.
*/
static bool
d_internal_is_space(
    const struct d_internal_reader* _reader,
    size_t                          _at
)
{
    const char byte = _reader->text[_at];

    return ( (byte == ' ')  ||
             (byte == '\t') ||
             ( (byte == '\r') &&
               (d_internal_newline_length(_reader, _at) == 0u) ) );
}


/*
d_internal_skip_space
  Moves past blanks and tabs, never past a line ending.
*/
static void
d_internal_skip_space(
    struct d_internal_reader* _reader
)
{
    // past every blank and lone CR
    while ( (_reader->at < _reader->length) &&
            (d_internal_is_space(_reader, _reader->at)) )
    {
        _reader->at++;
    }

    return;
}


/*
d_internal_bracket_open
  The number of `=` in a bracket's opening at `_at`, `[` `=`* `[`; -1 when
no bracket opens there.
*/
static long
d_internal_bracket_open(
    const struct d_internal_reader* _reader,
    size_t                          _at
)
{
    // a bracket opens with `[`
    if ( (_at >= _reader->length) ||
         (_reader->text[_at] != '[') )
    {
        return -1;
    }

    size_t at = _at + 1u;

    // the `=` of the opening
    while ( (at < _reader->length) &&
            (_reader->text[at] == '=') )
    {
        at++;
    }

    // and the `=` run, however long, ends with a second `[`
    if ( (at >= _reader->length) ||
         (_reader->text[at] != '[') )
    {
        return -1;
    }

    return (long)(at - _at - 1u);
}


/*
d_internal_bracket_body
  Moves from a bracket's opening to one past its closing, `]` and the same
number of `=` and `]`, counting the lines it spans.  The opening's length is
`_equals` + 2; a bracket never closed is reported at `_line`, `_column`, where
the element holding it begins.
*/
static bool
d_internal_bracket_body(
    struct d_internal_reader* _reader,
    size_t                    _equals,
    size_t                    _line,
    size_t                    _column,
    const char*               _what
)
{
    _reader->at += _equals + 2u;

    // each byte until the closing of as many `=`
    while (_reader->at < _reader->length)
    {
        // a line ending inside the bracket is counted as the file's
        if (d_internal_take_newline(_reader))
        {
            continue;
        }

        // `]`, the opening's run of `=`, and `]` close it
        if (_reader->text[_reader->at] == ']')
        {
            size_t at = _reader->at + 1u;

            // the `=` after a `]`
            while ( (at < _reader->length)          &&
                    (_reader->text[at] == '=')      &&
                    ((at - _reader->at - 1u) < _equals) )
            {
                at++;
            }

            // as many as opened it, and a `]` after them, closes it
            if ( ((at - _reader->at - 1u) == _equals) &&
                 (at < _reader->length)                &&
                 (_reader->text[at] == ']') )
            {
                _reader->at = at + 1u;

                return true;
            }
        }

        _reader->at++;
    }

    return d_internal_fail(_reader, _line, _column, _what);
}


/*
d_internal_comment
  Reads a comment at the reader's `#`: a bracket comment when a bracket opens
directly after it, a line comment to the end of the line otherwise.
*/
static bool
d_internal_comment(
    struct d_internal_reader* _reader,
    size_t                    _within,
    bool                      _trailing
)
{
    struct d_cmake* const cmake  = _reader->cmake;
    const size_t          start  = _reader->at;
    const size_t          line   = _reader->line;
    const size_t          column = (start - _reader->line_start) + 1u;
    const long            equals = d_internal_bracket_open(_reader,
                                                           start + 1u);

    // a bracket comment runs to its closing bracket, across lines
    if (equals >= 0)
    {
        _reader->at++;

        // the body, up to its own closing
        if (!d_internal_bracket_body(_reader, (size_t)equals, line, column,
                                     "the bracket comment is never closed"))
        {
            return false;
        }
    }
    else
    {
        // a line comment runs to the line's end, which it leaves
        while ( (_reader->at < _reader->length) &&
                (d_internal_newline_length(_reader, _reader->at) == 0u) )
        {
            _reader->at++;
        }
    }

    // the comment is recorded once it is whole
    if (!d_internal_grow((void**)&cmake->comments,
                         &_reader->comment_capacity, cmake->comment_count,
                         sizeof(*cmake->comments)))
    {
        return d_internal_fail_here(_reader, "out of memory");
    }

    struct d_cmake_comment* const comment =
        &cmake->comments[cmake->comment_count++];

    comment->start    = start;
    comment->end      = _reader->at;
    comment->line     = line;
    comment->column   = column;
    comment->end_line = _reader->line;
    comment->bracket  = (equals >= 0);
    comment->within   = _within;
    comment->trailing = _trailing;

    _reader->last_line = _reader->line;

    return true;
}


/*
d_internal_add_argument
  Records the argument from `_start` to the reader's place.
*/
static bool
d_internal_add_argument(
    struct d_internal_reader*  _reader,
    enum d_cmake_argument_kind _kind,
    size_t                     _start,
    size_t                     _line,
    size_t                     _column,
    char                       _paren,
    bool                       _legacy,
    size_t                     _equals
)
{
    struct d_cmake* const cmake = _reader->cmake;

    // the argument is recorded once it is whole
    if (!d_internal_grow((void**)&cmake->arguments,
                         &_reader->argument_capacity, cmake->argument_count,
                         sizeof(*cmake->arguments)))
    {
        return d_internal_fail_here(_reader, "out of memory");
    }

    struct d_cmake_argument* const argument =
        &cmake->arguments[cmake->argument_count++];

    argument->kind     = _kind;
    argument->start    = _start;
    argument->end      = _reader->at;
    argument->line     = _line;
    argument->column   = _column;
    argument->end_line = _reader->line;
    argument->paren    = _paren;
    argument->legacy   = _legacy;
    argument->equals   = _equals;

    _reader->last_line = _reader->line;

    return true;
}


/*
d_internal_quoted
  Reads a quoted argument at the reader's `"`: any text, newlines included,
to the next `"` no backslash escapes.
*/
static bool
d_internal_quoted(
    struct d_internal_reader* _reader
)
{
    const size_t start  = _reader->at;
    const size_t line   = _reader->line;
    const size_t column = (start - _reader->line_start) + 1u;

    _reader->at++;

    // each byte until the closing quote
    while (_reader->at < _reader->length)
    {
        const char byte = _reader->text[_reader->at];

        // a backslash escapes what follows, a line ending included
        if (byte == '\\')
        {
            _reader->at++;

            // a CR LF is one line ending
            if ( (_reader->at < _reader->length) &&
                 (!d_internal_take_newline(_reader)) )
            {
                _reader->at++;
            }

            continue;
        }

        // the closing quote ends it
        if (byte == '"')
        {
            _reader->at++;

            return d_internal_add_argument(_reader, D_CMAKE_QUOTED, start,
                                           line, column, '\0', false, 0u);
        }

        // a line ending inside the quotes is the file's
        if (!d_internal_take_newline(_reader))
        {
            _reader->at++;
        }
    }

    return d_internal_fail(_reader, line, column,
                           "the quoted argument is never closed");
}


/*
d_internal_ends_unquoted
  Whether the byte at `_at` ends an unquoted argument: space, a line ending,
a parenthesis, or a comment's `#`.
*/
static bool
d_internal_ends_unquoted(
    const struct d_internal_reader* _reader,
    size_t                          _at
)
{
    const char byte = _reader->text[_at];

    return ( (d_internal_is_space(_reader, _at))              ||
             (d_internal_newline_length(_reader, _at) != 0u)  ||
             (byte == '(')                                    ||
             (byte == ')')                                    ||
             (byte == '#') );
}


/*
d_internal_unquoted
  Reads an unquoted argument, the legacy forms included: a quoted run inside
one, `-Da="b c"`, keeps its quotes and may hold space, and a make variable,
`$(X)`, may hold parentheses.
  Long as the scanner is one loop, each byte that matters handled in turn.
*/
static bool
d_internal_unquoted(
    struct d_internal_reader* _reader
)
{
    const size_t start  = _reader->at;
    const size_t line   = _reader->line;
    const size_t column = (start - _reader->line_start) + 1u;
    bool         legacy = false;

    // each byte until what ends an unquoted argument
    while ( (_reader->at < _reader->length) &&
            (!d_internal_ends_unquoted(_reader, _reader->at)) )
    {
        const char byte = _reader->text[_reader->at];

        // an escape takes the next character, which may not end the line
        if (byte == '\\')
        {
            // a backslash with nothing after it on its line escapes nothing
            if ( ((_reader->at + 1u) >= _reader->length)                 ||
                 (d_internal_newline_length(_reader,
                                            _reader->at + 1u) != 0u) )
            {
                return d_internal_fail_here(_reader,
                                            "a backslash cannot end an "
                                            "unquoted argument");
            }

            _reader->at += 2u;
            continue;
        }

        // a quoted run inside an unquoted argument is the legacy form
        if (byte == '"')
        {
            const size_t opened = _reader->at;

            _reader->at++;

            // the quoted run, to its closing quote or the line's end
            while ( (_reader->at < _reader->length)                     &&
                    (_reader->text[_reader->at] != '"')                 &&
                    (d_internal_newline_length(_reader,
                                               _reader->at) == 0u) )
            {
                // an escape takes the next character with it
                if ( (_reader->text[_reader->at] == '\\')               &&
                     ((_reader->at + 1u) < _reader->length)             &&
                     (d_internal_newline_length(_reader,
                                                _reader->at + 1u) == 0u) )
                {
                    _reader->at++;
                }

                _reader->at++;
            }

            // the run must close on its own line
            if ( (_reader->at >= _reader->length) ||
                 (_reader->text[_reader->at] != '"') )
            {
                return d_internal_fail(_reader, line,
                                       (opened - _reader->line_start) + 1u,
                                       "a quoted run in an unquoted "
                                       "argument is never closed");
            }

            _reader->at++;
            legacy = true;
            continue;
        }

        // a make variable is `$(`, a name, and `)`
        if ( (byte == '$')                                   &&
             ((_reader->at + 1u) < _reader->length)         &&
             (_reader->text[_reader->at + 1u] == '(') )
        {
            size_t at = _reader->at + 2u;

            // the name of a make variable
            while ( (at < _reader->length)                              &&
                    ( ( (_reader->text[at] >= 'A') &&
                        (_reader->text[at] <= 'Z') )   ||
                      ( (_reader->text[at] >= 'a') &&
                        (_reader->text[at] <= 'z') )   ||
                      ( (_reader->text[at] >= '0') &&
                        (_reader->text[at] <= '9') )   ||
                      (_reader->text[at] == '_') ) )
            {
                at++;
            }

            // anything else leaves the `$` an ordinary character
            if ( (at < _reader->length) &&
                 (_reader->text[at] == ')') )
            {
                _reader->at = at + 1u;
                legacy      = true;
                continue;
            }
        }

        _reader->at++;
    }

    return d_internal_add_argument(_reader, D_CMAKE_UNQUOTED, start, line,
                                   column, '\0', legacy, 0u);
}


/*
d_internal_is_name_start
  Whether a byte begins a command's name.
*/
static bool
d_internal_is_name_start(
    char _byte
)
{
    return ( ( (_byte >= 'A') && (_byte <= 'Z') ) ||
             ( (_byte >= 'a') && (_byte <= 'z') ) ||
             (_byte == '_') );
}


/*
d_internal_command
  Reads a command invocation at the reader's name, its arguments and the
comments among them.
  Long as the argument list is read in one loop, by what each element
opens with, and a command refused is taken back whole.
*/
static bool
d_internal_command(
    struct d_internal_reader* _reader
)
{
    struct d_cmake* const cmake  = _reader->cmake;
    const size_t          start  = _reader->at;
    const size_t          line   = _reader->line;
    const size_t          column = (start - _reader->line_start) + 1u;

    // the name is letters, digits and underscores
    while ( (_reader->at < _reader->length)                         &&
            ( (d_internal_is_name_start(_reader->text[_reader->at])) ||
              ( (_reader->text[_reader->at] >= '0') &&
                (_reader->text[_reader->at] <= '9') ) ) )
    {
        _reader->at++;
    }

    const size_t name_end = _reader->at;

    d_internal_skip_space(_reader);

    // the name must be followed, on its line, by `(`
    if ( (_reader->at >= _reader->length) ||
         (_reader->text[_reader->at] != '(') )
    {
        return d_internal_fail_here(_reader,
                                    "expected `(` after the command name");
    }

    // room for one more command
    if (!d_internal_grow((void**)&cmake->commands,
                         &_reader->command_capacity, cmake->command_count,
                         sizeof(*cmake->commands)))
    {
        return d_internal_fail_here(_reader, "out of memory");
    }

    const size_t which = cmake->command_count++;

    memset(&cmake->commands[which], 0, sizeof(cmake->commands[which]));
    cmake->commands[which].start          = start;
    cmake->commands[which].name_length    = name_end - start;
    cmake->commands[which].open           = _reader->at;
    cmake->commands[which].line           = line;
    cmake->commands[which].column         = column;
    cmake->commands[which].first_argument = cmake->argument_count;
    cmake->commands[which].partner        = D_CMAKE_NONE;

    _reader->at++;
    _reader->last_line = _reader->line;

    size_t nesting = 0;
    bool   ok      = true;

    // each element of the argument list, to its `)`
    while (true)
    {
        // separation: space, line endings and comments
        d_internal_skip_space(_reader);

        // a line ending among the arguments is space
        if (d_internal_take_newline(_reader))
        {
            continue;
        }

        // the file must not end inside the argument list
        if (_reader->at >= _reader->length)
        {
            ok = d_internal_fail(_reader, line, column,
                                 "the command's `(` is never closed");
            break;
        }

        const char byte = _reader->text[_reader->at];

        // a comment among the arguments belongs to the command
        if (byte == '#')
        {
            ok = d_internal_comment(_reader, which,
                                    (_reader->last_line == _reader->line));

            // a comment that could not be read ends the list
            if (!ok)
            {
                break;
            }

            continue;
        }

        // the `)` that matches the command's `(` ends it
        if ( (byte == ')') &&
             (nesting == 0u) )
        {
            _reader->at++;
            break;
        }

        const size_t arg_line   = _reader->line;
        const size_t arg_column = (_reader->at - _reader->line_start) + 1u;

        // a nested parenthesis is an argument of its own
        if ( (byte == '(') ||
             (byte == ')') )
        {
            const size_t paren_at = _reader->at;

            nesting = (byte == '(') ? (nesting + 1u) : (nesting - 1u);
            _reader->at++;
            ok = d_internal_add_argument(_reader, D_CMAKE_UNQUOTED, paren_at,
                                         arg_line, arg_column, byte, false,
                                         0u);
        }
        else if (byte == '"')
        {
            ok = d_internal_quoted(_reader);
        }
        else if (d_internal_bracket_open(_reader, _reader->at) >= 0)
        {
            const size_t bracket_at = _reader->at;
            const size_t equals     =
                (size_t)d_internal_bracket_open(_reader, _reader->at);

            ok = ( (d_internal_bracket_body(_reader, equals, arg_line,
                                            arg_column,
                                            "the bracket argument is never "
                                            "closed")) &&
                   (d_internal_add_argument(_reader, D_CMAKE_BRACKET,
                                            bracket_at, arg_line, arg_column,
                                            '\0', false, equals)) );
        }
        else
        {
            ok = d_internal_unquoted(_reader);
        }

        // a broken argument ends the command, and the reading
        if (!ok)
        {
            break;
        }
    }

    // a command not read whole is not kept, nor what it held
    if (!ok)
    {
        cmake->argument_count = cmake->commands[which].first_argument;
        cmake->command_count  = which;

        // the comments read within the command go with it
        while ( (cmake->comment_count > 0u) &&
                (cmake->comments[cmake->comment_count - 1u].within
                 == which) )
        {
            cmake->comment_count--;
        }

        return false;
    }

    struct d_cmake_command* const command = &cmake->commands[which];

    command->end            = _reader->at;
    command->end_line       = _reader->line;
    command->argument_count = cmake->argument_count
                            - command->first_argument;

    _reader->command_line = _reader->line;
    _reader->last_line    = _reader->line;

    return true;
}


/*
d_internal_classify
  The block a command's name opens, branches or closes, if any.
*/
static void
d_internal_classify(
    const struct d_cmake*   _cmake,
    struct d_cmake_command* _command
)
{
    // d_internal_block_name
    //   struct: one name that takes part in a block, and the part it takes.
    struct d_internal_block_name
    {
        const char*        name;
        enum d_cmake_block block;
        enum d_cmake_role  role;
    };

    static const struct d_internal_block_name names[] =
    {
        { "if",          D_CMAKE_BLOCK_IF,       D_CMAKE_ROLE_OPEN   },
        { "elseif",      D_CMAKE_BLOCK_IF,       D_CMAKE_ROLE_BRANCH },
        { "else",        D_CMAKE_BLOCK_IF,       D_CMAKE_ROLE_BRANCH },
        { "endif",       D_CMAKE_BLOCK_IF,       D_CMAKE_ROLE_CLOSE  },
        { "foreach",     D_CMAKE_BLOCK_FOREACH,  D_CMAKE_ROLE_OPEN   },
        { "endforeach",  D_CMAKE_BLOCK_FOREACH,  D_CMAKE_ROLE_CLOSE  },
        { "while",       D_CMAKE_BLOCK_WHILE,    D_CMAKE_ROLE_OPEN   },
        { "endwhile",    D_CMAKE_BLOCK_WHILE,    D_CMAKE_ROLE_CLOSE  },
        { "function",    D_CMAKE_BLOCK_FUNCTION, D_CMAKE_ROLE_OPEN   },
        { "endfunction", D_CMAKE_BLOCK_FUNCTION, D_CMAKE_ROLE_CLOSE  },
        { "macro",       D_CMAKE_BLOCK_MACRO,    D_CMAKE_ROLE_OPEN   },
        { "endmacro",    D_CMAKE_BLOCK_MACRO,    D_CMAKE_ROLE_CLOSE  },
        { "block",       D_CMAKE_BLOCK_BLOCK,    D_CMAKE_ROLE_OPEN   },
        { "endblock",    D_CMAKE_BLOCK_BLOCK,    D_CMAKE_ROLE_CLOSE  }
    };

    // each name a block's command has
    for (size_t at = 0; at < (sizeof(names) / sizeof(names[0])); ++at)
    {
        // the first name that matches is the command's part
        if (d_cmake_is(_cmake, _command, names[at].name))
        {
            _command->block = names[at].block;
            _command->role  = names[at].role;

            return;
        }
    }

    return;
}


/*
d_internal_pair
  Pairs every block's opener with its closer.  A closer pairs with the
nearest open block of its kind: a block it passes over was never closed, and
a closer with no such block is unpaired, as is a branch outside an if.
  Long as the stack of open blocks is kept here, by each part a command
takes.
*/
static bool
d_internal_pair(
    struct d_cmake* _cmake
)
{
    size_t* const open  = malloc((_cmake->command_count + 1u)
                                 * sizeof(size_t));
    size_t        depth = 0;

    // the pairing needs a stack as deep as the file could nest
    if (!open)
    {
        return false;
    }

    // each command, against the blocks open where it stands
    for (size_t at = 0; at < _cmake->command_count; ++at)
    {
        struct d_cmake_command* const command = &_cmake->commands[at];

        d_internal_classify(_cmake, command);
        command->depth = depth;

        // an opener holds what follows it, until its closer
        if (command->role == D_CMAKE_ROLE_OPEN)
        {
            command->unpaired = true;
            open[depth++]     = at;
            continue;
        }

        // a branch stands at its if's depth, beside it
        if (command->role == D_CMAKE_ROLE_BRANCH)
        {
            // a branch belongs to the if open about it, and to no other block
            if ( (depth > 0u) &&
                 (_cmake->commands[open[depth - 1u]].block
                  == D_CMAKE_BLOCK_IF) )
            {
                command->depth   = depth - 1u;
                command->partner = open[depth - 1u];
            }
            else
            {
                command->unpaired = true;
            }

            continue;
        }

        // a closer finds the nearest open block of its kind
        if (command->role == D_CMAKE_ROLE_CLOSE)
        {
            size_t found = depth;

            // the nearest open block of the closer's kind
            while ( (found > 0u) &&
                    (_cmake->commands[open[found - 1u]].block
                     != command->block) )
            {
                found--;
            }

            // no such block is open
            if (found == 0u)
            {
                command->unpaired = true;
                continue;
            }

            struct d_cmake_command* const opener =
                &_cmake->commands[open[found - 1u]];

            opener->partner   = at;
            opener->unpaired  = false;
            command->partner  = open[found - 1u];
            command->depth    = found - 1u;
            depth             = found - 1u;
        }
    }

    free(open);

    return true;
}


/*
d_cmake_read
  The elements of the file, one after another: space, a line ending, a
comment, or a command, which must be the last thing on its line but for
space and comments.
  Long as the reader is set up, run over the file, and its counts settled,
each a step.
*/
bool
d_cmake_read(
    struct d_cmake* _out,
    const char*     _text,
    size_t          _length,
    char*           _problem,
    size_t          _capacity
)
{
    // parameter validation first
    if (!_out)
    {
        return false;
    }

    memset(_out, 0, sizeof(*_out));
    _out->text   = (_text) ? _text : "";
    _out->length = (_text) ? _length : 0u;

    // no reason yet
    if ( (_problem) &&
         (_capacity > 0u) )
    {
        _problem[0] = '\0';
    }

    struct d_internal_reader reader;

    memset(&reader, 0, sizeof(reader));
    reader.cmake    = _out;
    reader.text     = _out->text;
    reader.length   = _out->length;
    reader.line     = 1u;
    reader.problem  = _problem;
    reader.capacity = _capacity;

    // a byte-order mark is no element, and columns count from after it
    if ( (reader.length >= 3u)                          &&
         ((unsigned char)reader.text[0] == 0xEFu)       &&
         ((unsigned char)reader.text[1] == 0xBBu)       &&
         ((unsigned char)reader.text[2] == 0xBFu) )
    {
        reader.at         = 3u;
        reader.line_start = 3u;
    }

    // each element of the file, to its end
    while (reader.at < reader.length)
    {
        d_internal_skip_space(&reader);

        // a line ending ends the element, whatever it held
        if (d_internal_take_newline(&reader))
        {
            continue;
        }

        // space may end the file
        if (reader.at >= reader.length)
        {
            break;
        }

        const char byte          = reader.text[reader.at];
        const bool after_command = (_out->command_count > 0u) &&
                                   (reader.command_line == reader.line);

        // a comment, wherever on its line it stands
        if (byte == '#')
        {
            // a comment that could not be read ends the file
            if (!d_internal_comment(&reader, D_CMAKE_NONE,
                                    (reader.last_line == reader.line)))
            {
                break;
            }

            continue;
        }

        // a second command on a line is refused, as CMake refuses it
        if (after_command)
        {
            (void)d_internal_fail_here(&reader,
                                       "a command must end its line");
            break;
        }

        // anything else must begin a command
        if (!d_internal_is_name_start(byte))
        {
            (void)d_internal_fail_here(&reader,
                                       "expected a command, a comment or "
                                       "a line ending");
            break;
        }

        // a command that could not be read ends the file
        if (!d_internal_command(&reader))
        {
            break;
        }
    }

    // the last line counts, ended or not, and an empty file has none
    _out->line_count = (reader.line_start == reader.length)
                     ? (reader.line - 1u)
                     : reader.line;

    // an out-of-memory failure in pairing is still a failure
    if (!d_internal_pair(_out))
    {
        (void)d_internal_fail_here(&reader, "out of memory");
    }

    return !reader.failed;
}


/*
d_cmake_release
  Frees the arrays; the text was never the reader's.
*/
void
d_cmake_release(
    struct d_cmake* _cmake
)
{
    // releasing nothing is allowed
    if (!_cmake)
    {
        return;
    }

    free(_cmake->commands);
    free(_cmake->arguments);
    free(_cmake->comments);
    memset(_cmake, 0, sizeof(*_cmake));

    return;
}


/*
d_cmake_generated
  CMake's own files say so on their first line; the tree's generators say so
anywhere near the top.
*/
bool
d_cmake_generated(
    const char* _text,
    size_t      _length
)
{
    static const char* const firsts[] =
    {
        "# CMAKE generated file: DO NOT EDIT!",
        "# CMake generated Testfile for",
        "# Install script for directory:",
        "# This file is configured by CMake automatically",
        "# This is the CMakeCache file.",
        "# Generated by CMake"
    };
    static const char marker[] = "Auto-generated. Do not edit by hand.";

    // nothing was generated
    if (!_text)
    {
        return false;
    }

    // each marker CMake opens its own files with
    for (size_t at = 0; at < (sizeof(firsts) / sizeof(firsts[0])); ++at)
    {
        const size_t span = strlen(firsts[at]);

        // CMake's marker opens the file
        if ( (_length >= span) &&
             (memcmp(_text, firsts[at], span) == 0) )
        {
            return true;
        }
    }

    const size_t window = (_length < 4096u) ? _length : 4096u;
    const size_t span   = sizeof(marker) - 1u;

    // each place in the window
    for (size_t at = 0; (at + span) <= window; ++at)
    {
        // the tree's marker may stand anywhere near the top
        if (memcmp(&_text[at], marker, span) == 0)
        {
            return true;
        }
    }

    return false;
}


/*
d_cmake_is
  ASCII case folding, which is all a CMake name can hold.
*/
bool
d_cmake_is(
    const struct d_cmake*         _cmake,
    const struct d_cmake_command* _command,
    const char*                   _name
)
{
    // a command must be named to be compared
    if ( (!_cmake)   ||
         (!_command) ||
         (!_name) )
    {
        return false;
    }

    const size_t span = strlen(_name);

    // a name of another length is another name
    if (span != _command->name_length)
    {
        return false;
    }

    // each letter, folded to lower case
    for (size_t at = 0; at < span; ++at)
    {
        char left  = _cmake->text[_command->start + at];
        char right = _name[at];

        left  = ( (left >= 'A') && (left <= 'Z') )
              ? (char)(left - 'A' + 'a') : left;
        right = ( (right >= 'A') && (right <= 'Z') )
              ? (char)(right - 'A' + 'a') : right;

        // one letter apart is apart
        if (left != right)
        {
            return false;
        }
    }

    return true;
}


/*
d_cmake_block_name
  The opener's spelling.
*/
const char*
d_cmake_block_name(
    enum d_cmake_block _block
)
{
    switch (_block)
    {
        case D_CMAKE_BLOCK_IF:       return "if";
        case D_CMAKE_BLOCK_FOREACH:  return "foreach";
        case D_CMAKE_BLOCK_WHILE:    return "while";
        case D_CMAKE_BLOCK_FUNCTION: return "function";
        case D_CMAKE_BLOCK_MACRO:    return "macro";
        case D_CMAKE_BLOCK_BLOCK:    return "block";
        case D_CMAKE_BLOCK_NONE:
        default:                     break;
    }

    return "";
}


/*
d_internal_unescape
  Decodes the escapes of a quoted or unquoted argument's text; a quoted one
also drops each backslash before a line ending, and the line ending.  `\;`
is kept as it is in a quoted argument, and in an unquoted one when `_keep`
says so, for the list division that reads it later.
  Long as it is a table of the escapes, each a line or two.
*/
static size_t
d_internal_unescape(
    const char* _raw,
    size_t      _length,
    bool        _quoted,
    bool        _keep,
    char*       _out
)
{
    size_t used = 0;
    size_t at   = 0;

    // each byte of the raw text
    while (at < _length)
    {
        // a CR LF inside a quoted argument is read as the LF CMake reads
        if ( (_quoted)                 &&
             (_raw[at] == '\r')        &&
             ((at + 1u) < _length)     &&
             (_raw[at + 1u] == '\n') )
        {
            _out[used++] = '\n';
            at          += 2u;
            continue;
        }

        // anything but a backslash is itself
        if ( (_raw[at] != '\\') ||
             ((at + 1u) >= _length) )
        {
            _out[used++] = _raw[at++];
            continue;
        }

        const char next = _raw[at + 1u];

        // a continuation joins the lines, in a quoted argument only
        if ( (_quoted) &&
             ( (next == '\n') ||
               ( (next == '\r')          &&
                 ((at + 2u) < _length)  &&
                 (_raw[at + 2u] == '\n') ) ) )
        {
            at += (next == '\n') ? 2u : 3u;
            continue;
        }

        const bool alnum = ( ( (next >= 'A') && (next <= 'Z') ) ||
                             ( (next >= 'a') && (next <= 'z') ) ||
                             ( (next >= '0') && (next <= '9') ) );

        // the three encoded characters; `\;`, which a quoted argument keeps
        // and an unquoted one reads as the `;` it keeps from dividing it;
        // and the rest of the letters and digits, which CMake would refuse,
        // as written
        if (next == 't')
        {
            _out[used++] = '\t';
        }
        else if (next == 'n')
        {
            _out[used++] = '\n';
        }
        else if (next == 'r')
        {
            _out[used++] = '\r';
        }
        else if ( (next == ';') &&
                  (!_quoted)    &&
                  (!_keep) )
        {
            _out[used++] = ';';
        }
        else if ( (next == ';') ||
                  (alnum) )
        {
            _out[used++] = '\\';
            _out[used++] = next;
        }
        else
        {
            _out[used++] = next;
        }

        at += 2u;
    }

    return used;
}


/*
d_cmake_value
  The argument as CMake reads it, before expansion.
  Long as each of the three kinds is read here.
*/
size_t
d_cmake_value(
    const struct d_cmake*          _cmake,
    const struct d_cmake_argument* _argument,
    char*                          _out
)
{
    // no argument has no value
    if ( (!_cmake)    ||
         (!_argument) ||
         (!_out) )
    {
        return 0u;
    }

    const char* const raw    = &_cmake->text[_argument->start];
    const size_t      length = _argument->end - _argument->start;
    size_t            used   = 0;

    // a bracket's content is taken as it stands, but for a first newline
    if (_argument->kind == D_CMAKE_BRACKET)
    {
        const size_t fence = _argument->equals + 2u;
        size_t       from  = fence;
        const size_t to    = (length >= (2u * fence)) ? (length - fence)
                                                      : fence;

        // a table of the line endings directly after the opening, which
        // are no part of it
        if ( (from < to) &&
             (raw[from] == '\n') )
        {
            from += 1u;
        }
        else if ( ((from + 1u) < to)  &&
                  (raw[from] == '\r') &&
                  (raw[from + 1u] == '\n') )
        {
            from += 2u;
        }

        // the content is copied, each CR LF read as the LF CMake reads
        for (size_t at = from; at < to; ++at)
        {
            // a CR before an LF is dropped
            if ( (raw[at] == '\r')    &&
                 ((at + 1u) < to)    &&
                 (raw[at + 1u] == '\n') )
            {
                continue;
            }

            _out[used++] = raw[at];
        }
    }
    else if (_argument->kind == D_CMAKE_QUOTED)
    {
        used = (length >= 2u) ? d_internal_unescape(&raw[1], length - 2u,
                                                    true, false, _out)
                              : 0u;
    }
    else
    {
        used = d_internal_unescape(raw, length, false, false, _out);
    }

    _out[used] = '\0';

    return used;
}


/*
d_cmake_passed
  An unquoted argument's text, decoded but for `\;`; any other's value.
*/
size_t
d_cmake_passed(
    const struct d_cmake*          _cmake,
    const struct d_cmake_argument* _argument,
    char*                          _out
)
{
    // no argument passes nothing
    if ( (!_cmake)    ||
         (!_argument) ||
         (!_out) )
    {
        return 0u;
    }

    // a quoted or a bracket argument is passed whole, as its value
    if (_argument->kind != D_CMAKE_UNQUOTED)
    {
        return d_cmake_value(_cmake, _argument, _out);
    }

    const size_t used = d_internal_unescape(&_cmake->text[_argument->start],
                                            _argument->end - _argument->start,
                                            false, true, _out);

    _out[used] = '\0';

    return used;
}


/*
d_cmake_list_items
  CMake's own division of a list, as cmExpandList makes it: at each `;`
outside square brackets and not escaped, the brackets counted as they come,
so a `]` before any `[` leaves the count below none and divides nothing
more.  A list with no `;` is one item as it stands, its backslashes and all.
  Long as the four bytes that matter are each handled in the one loop.
*/
size_t
d_cmake_list_items(
    const char* _list,
    size_t      _length,
    bool        _keep_empty,
    char*       _out,
    size_t*     _out_length
)
{
    size_t count   = 0;
    size_t used    = 0;
    size_t item    = 0;
    long   nesting = 0;
    bool   divided = false;

    // nothing to divide, or nowhere to write, is no item
    if ( (!_list)      ||
         (!_out)       ||
         (_length == 0u) )
    {
        // nothing written
        if (_out_length)
        {
            *_out_length = 0u;
        }

        return 0u;
    }

    // a `;` anywhere is what makes the text more than one item
    divided = (memchr(_list, ';', _length) != NULL);

    // each byte, by what it does to the list
    for (size_t at = 0; at < _length; ++at)
    {
        const char c = _list[at];

        // a table of what each byte does: `\;` is a `;` kept, a bracket
        // counted, a `;` outside them the end of an item
        if ( (divided)                 &&
             (c == '\\')               &&
             ((at + 1u) < _length)     &&
             (_list[at + 1u] == ';') )
        {
            _out[used++] = ';';
            at++;
            continue;
        }
        else if ( (divided) &&
                  (c == '[') )
        {
            nesting++;
        }
        else if ( (divided) &&
                  (c == ']') )
        {
            nesting--;
        }
        else if ( (divided)    &&
                  (c == ';')   &&
                  (nesting == 0) )
        {
            // an item ends here; an empty one is kept only when asked
            if ( (used > item) ||
                 (_keep_empty) )
            {
                _out[used++] = '\0';
                item         = used;
                count++;
            }

            continue;
        }

        _out[used++] = c;
    }

    // the last item, by the same rule
    if ( (used > item) ||
         (_keep_empty) )
    {
        _out[used++] = '\0';
        count++;
    }

    // how much was written, when it is asked for
    if (_out_length)
    {
        *_out_length = used;
    }

    return count;
}


/*
d_cmake_is_name
  `[A-Za-z_][A-Za-z0-9_]*`, as cmake-language(7) gives an identifier.
*/
bool
d_cmake_is_name(
    const char* _text,
    size_t      _length
)
{
    // a name has at least one byte
    if ( (!_text) ||
         (_length == 0u) )
    {
        return false;
    }

    // each byte, a letter, a digit or `_`
    for (size_t at = 0; at < _length; ++at)
    {
        const char c     = _text[at];
        const bool alpha = ( ( (c >= 'A') && (c <= 'Z') ) ||
                             ( (c >= 'a') && (c <= 'z') ) ||
                             (c == '_') );
        const bool digit = ( (c >= '0') && (c <= '9') );

        // a letter or `_` anywhere, a digit past the first
        if ( (!alpha) &&
             ( (!digit) || (at == 0u) ) )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_plain
  Whether a byte stands in an unquoted argument as itself: not space, not
one that ends, escapes or quotes, not `;`, which would divide it, and not a
control character.
*/
static bool
d_internal_plain(
    char _c
)
{
    const unsigned char c = (unsigned char)_c;

    return ( (c > 0x20u)  &&
             (c != 0x7Fu) &&
             (c != '(')   &&
             (c != ')')   &&
             (c != '#')   &&
             (c != '"')   &&
             (c != '\\')  &&
             (c != ';') );
}


/*
d_internal_fence
  The fewest `=` a bracket argument needs so that no `]`, that many `=`
and `]` stands in its value.
*/
static size_t
d_internal_fence(
    const char* _value,
    size_t      _length
)
{
    size_t equals = 0;

    // each count of `=` from none, until one closes nowhere
    for (;;)
    {
        bool found = false;

        // a closing of this many `=` anywhere in the value
        for (size_t at = 0; (!found) && ((at + equals + 1u) < _length); ++at)
        {
            size_t run = 0;

            // the `=` after a `]`
            while ( (_value[at] == ']') &&
                    (run < equals)      &&
                    (_value[at + 1u + run] == '=') )
            {
                run++;
            }

            found = ( (_value[at] == ']')           &&
                      (run == equals)               &&
                      (_value[at + 1u + run] == ']') );
        }

        // the first count that closes nowhere in the value
        if (!found)
        {
            return equals;
        }

        equals++;
    }
}


/*
d_cmake_write
  Each kind written so that reading it back gives the value: what cannot
be is refused rather than written another way.
  Long as each of the three kinds is written here, each a few lines.
*/
char*
d_cmake_write(
    const char*                _value,
    size_t                     _length,
    enum d_cmake_argument_kind _kind,
    size_t*                    _out_length
)
{
    const char* const value = (_value) ? _value : "";
    const size_t      span  = (_value) ? _length : 0u;
    char*             out   = NULL;
    size_t            used  = 0;

    // a table of the kinds: an unquoted argument holds only plain bytes,
    // and opens with neither a bracket's `[` nor `=`; a quoted one escapes
    // what would not read back; a bracket one is fenced past its value
    if (_kind == D_CMAKE_UNQUOTED)
    {
        bool plain = ( (span > 0u)       &&
                       (value[0] != '[') &&
                       (value[0] != '=') );

        // each byte, plain
        for (size_t at = 0; (plain) && (at < span); ++at)
        {
            plain = d_internal_plain(value[at]);
        }

        out = (plain) ? malloc(span + 1u) : NULL;

        // the value as it stands
        if (out)
        {
            memcpy(out, value, span);
            used = span;
        }
    }
    else if (_kind == D_CMAKE_QUOTED)
    {
        out = malloc((span * 2u) + 3u);

        // a backslash and a quote escaped, and a CR, which a CR LF would
        // otherwise lose; a variable reference stays one
        if (out)
        {
            out[used++] = '"';

            // each byte, escaped where it must be
            for (size_t at = 0; at < span; ++at)
            {
                // a table of the three that would not read back as
                // themselves
                if ( (value[at] == '\\') ||
                     (value[at] == '"') )
                {
                    out[used++] = '\\';
                    out[used++] = value[at];
                }
                else if (value[at] == '\r')
                {
                    out[used++] = '\\';
                    out[used++] = 'r';
                }
                else
                {
                    out[used++] = value[at];
                }
            }

            out[used++] = '"';
        }
    }
    else
    {
        const size_t equals = d_internal_fence(value, span);
        const bool   lead   = ( (span > 0u) &&
                                (value[0] == '\n') );
        bool         crlf   = false;

        // a CR LF in the value would read back as LF
        for (size_t at = 0; (!crlf) && ((at + 1u) < span); ++at)
        {
            crlf = ( (value[at] == '\r') &&
                     (value[at + 1u] == '\n') );
        }

        out = (crlf) ? NULL : malloc(span + (2u * equals) + 6u);

        // the fences, and a newline the opening one drops when the value
        // begins with one of its own
        if (out)
        {
            out[used++] = '[';
            memset(&out[used], '=', equals);
            used        += equals;
            out[used++] = '[';

            // a newline the opening drops, for one the value begins with
            if (lead)
            {
                out[used++] = '\n';
            }

            memcpy(&out[used], value, span);
            used        += span;
            out[used++] = ']';
            memset(&out[used], '=', equals);
            used        += equals;
            out[used++] = ']';
        }
    }

    // the text, terminated
    if (out)
    {
        out[used] = '\0';

        // its length, when it is asked for
        if (_out_length)
        {
            *_out_length = used;
        }
    }

    return out;
}


/*
d_cmake_write_any
  Unquoted when the value can be, quoted otherwise.
*/
char*
d_cmake_write_any(
    const char* _value,
    size_t      _length,
    size_t*     _out_length
)
{
    char* const plain = d_cmake_write(_value, _length, D_CMAKE_UNQUOTED,
                                      _out_length);

    return (plain) ? plain
                   : d_cmake_write(_value, _length, D_CMAKE_QUOTED,
                                   _out_length);
}
