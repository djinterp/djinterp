/*******************************************************************************
* djinterp [tools]                                                  dext_cmake.c
*
* Builds a CMake file's node tree from dcmake's records: see dext_cmake.h.
*   The reader gives commands and comments in two arrays, each in source
* order; the build walks both at once, so siblings stand in the order the
* file has them. Blocks nest by a stack of open block nodes, kept in step
* with the reader's own pairing: an opener pushes, its closer pops, and a
* closer that pairs with a block further down pops the blocks above it,
* which were never closed.
*
* path:      /src/djinterp/tools/dawk/ext/check/dext_cmake.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/check/dext_cmake.h"  // corresponding header
// std
#include <stdio.h>   // FILE, fopen, fread, fclose, snprintf
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memset, strcmp, strlen, strrchr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/ext/cmake/dcmake.h"  // d_cmake_read
#include "../../../../../../inc/djinterp/tools/dawk/dss.h"               // D_DSS_NO_INDEX


// d_internal_build
//   struct: one build's tree, the records it reads, and a scratch buffer
//   for values.
struct d_internal_build
{
    struct d_node_tree*       tree;
    const struct d_cmake*     cmake;
    struct d_ext_cmake_stats* stats;
    char*                     scratch;
    size_t                    scratch_capacity;
};


/*
d_internal_number
  Writes a count as an attribute's text, which the tree keeps as a number.
*/
static void
d_internal_number(
    struct d_node_tree* _tree,
    uint32_t            _node,
    const char*         _name,
    size_t              _value
)
{
    char text[32];

    (void)snprintf(text, sizeof(text), "%lu", (unsigned long)_value);
    (void)d_node_set_attribute(_tree, _node, _name, text);

    return;
}


/*
d_internal_place
  Gives a node the geometry of a record's first line: where it starts, and
how far along that line it reaches.
*/
static void
d_internal_place(
    const struct d_internal_build* _build,
    uint32_t                       _node,
    size_t                         _start,
    size_t                         _end,
    size_t                         _line,
    size_t                         _column
)
{
    struct d_node* const node = d_node_at(_build->tree, _node);
    const char* const    text = _build->cmake->text;
    size_t               last = _start;

    // the record's first line ends at its end or at a line ending
    while ( (last < _end) &&
            (text[last] != '\n') &&
            (text[last] != '\r') )
    {
        last++;
    }

    node->line         = (uint32_t)_line;
    node->start_column = (uint32_t)_column;
    node->width        = (uint32_t)(last - _start);
    node->end_column   = (uint32_t)(_column + (last - _start)) - 1u;

    return;
}


/*
d_internal_comment
  Adds one comment beneath a parent.
*/
static void
d_internal_comment(
    struct d_internal_build*      _build,
    uint32_t                      _parent,
    const struct d_cmake_comment* _comment
)
{
    const uint32_t node = d_node_add(_build->tree, _parent, "comment");

    // a node that cannot be added leaves the rest of the tree as it is
    if (node == D_DSS_NO_INDEX)
    {
        return;
    }

    (void)d_node_set_attribute(_build->tree, node, "kind",
                               _comment->bracket ? "bracket" : "line");

    if (_comment->trailing)
    {
        (void)d_node_set_attribute(_build->tree, node, "trailing", NULL);
    }

    (void)d_node_set_text(_build->tree, node,
                          &_build->cmake->text[_comment->start],
                          _comment->end - _comment->start);
    d_internal_place(_build, node, _comment->start, _comment->end,
                     _comment->line, _comment->column);
    _build->stats->comments++;

    return;
}


/*
d_internal_value
  The argument's value, in the build's scratch buffer.
*/
static const char*
d_internal_value(
    struct d_internal_build*       _build,
    const struct d_cmake_argument* _argument
)
{
    const size_t wanted = (_argument->end - _argument->start) + 1u;

    // the buffer grows to the longest argument
    if (wanted > _build->scratch_capacity)
    {
        char* const grown = realloc(_build->scratch, wanted);

        if (!grown)
        {
            return NULL;
        }

        _build->scratch          = grown;
        _build->scratch_capacity = wanted;
    }

    (void)d_cmake_value(_build->cmake, _argument, _build->scratch);

    return _build->scratch;
}


/*
d_internal_argument
  Adds one argument beneath its command.
*/
static void
d_internal_argument(
    struct d_internal_build*       _build,
    uint32_t                       _command,
    const struct d_cmake_argument* _argument
)
{
    static const char* const kinds[] = { "unquoted", "quoted", "bracket" };

    const uint32_t node = d_node_add(_build->tree, _command, "argument");

    // a node that cannot be added leaves the rest of the tree as it is
    if (node == D_DSS_NO_INDEX)
    {
        return;
    }

    const char* const value = d_internal_value(_build, _argument);

    (void)d_node_set_attribute(_build->tree, node, "kind",
                               kinds[_argument->kind]);

    if (value)
    {
        (void)d_node_set_attribute(_build->tree, node, "value", value);
    }

    if (_argument->paren != '\0')
    {
        const char paren[2] = { _argument->paren, '\0' };

        (void)d_node_set_attribute(_build->tree, node, "paren", paren);
    }

    if (_argument->legacy)
    {
        (void)d_node_set_attribute(_build->tree, node, "legacy", NULL);
    }

    (void)d_node_set_text(_build->tree, node,
                          &_build->cmake->text[_argument->start],
                          _argument->end - _argument->start);
    d_internal_place(_build, node, _argument->start, _argument->end,
                     _argument->line, _argument->column);
    _build->stats->arguments++;

    return;
}


/*
d_internal_case
  Whether a name is written in lower case, upper case, or both.
*/
static const char*
d_internal_case(
    const char* _name,
    size_t      _length
)
{
    bool lower = false;
    bool upper = false;

    for (size_t at = 0; at < _length; ++at)
    {
        lower = lower || ( (_name[at] >= 'a') && (_name[at] <= 'z') );
        upper = upper || ( (_name[at] >= 'A') && (_name[at] <= 'Z') );
    }

    return ( (lower) && (upper) ) ? "mixed"
         : (upper)                ? "upper"
         :                          "lower";
}


/*
d_internal_command
  Adds one command beneath a parent, its arguments and the comments among
them in the order the file has them, and the comment cursor past them.
*/
static uint32_t
d_internal_command(
    struct d_internal_build* _build,
    uint32_t                 _parent,
    size_t                   _which,
    size_t*                  _comment
)
{
    static const char* const roles[] = { "", "open", "branch", "close" };

    const struct d_cmake* const         cmake   = _build->cmake;
    const struct d_cmake_command* const command = &cmake->commands[_which];
    const uint32_t node = d_node_add(_build->tree, _parent, "command");

    // a node that cannot be added leaves the rest of the tree as it is
    if (node == D_DSS_NO_INDEX)
    {
        return D_DSS_NO_INDEX;
    }

    char   name[128];
    size_t span = (command->name_length < (sizeof(name) - 1u))
                ? command->name_length : (sizeof(name) - 1u);

    for (size_t at = 0; at < span; ++at)
    {
        const char byte = cmake->text[command->start + at];

        name[at] = ( (byte >= 'A') && (byte <= 'Z') )
                 ? (char)(byte - 'A' + 'a') : byte;
    }

    name[span] = '\0';

    (void)d_node_set_attribute(_build->tree, node, "name", name);
    (void)d_node_set_attribute(_build->tree, node, "case",
                               d_internal_case(&cmake->text[command->start],
                                               command->name_length));
    d_internal_number(_build->tree, node, "depth", command->depth);

    if (command->role != D_CMAKE_ROLE_NONE)
    {
        (void)d_node_set_attribute(_build->tree, node, "role",
                                   roles[command->role]);
    }

    d_internal_number(_build->tree, node, "arguments",
                      command->argument_count);
    d_internal_number(_build->tree, node, "lines",
                      (command->end_line - command->line) + 1u);

    if (command->unpaired)
    {
        (void)d_node_set_attribute(_build->tree, node, "unpaired", NULL);
        _build->stats->unpaired++;
    }

    (void)d_node_set_text(_build->tree, node, &cmake->text[command->start],
                          command->name_length);
    d_internal_place(_build, node, command->start, command->end,
                     command->line, command->column);
    _build->stats->commands++;

    size_t       argument = command->first_argument;
    const size_t last     = command->first_argument
                          + command->argument_count;

    // the arguments, and the comments among them, by where they start
    while ( (argument < last) ||
            ( (*_comment < cmake->comment_count) &&
              (cmake->comments[*_comment].within == _which) ) )
    {
        const bool comment_first =
            (*_comment < cmake->comment_count)               &&
            (cmake->comments[*_comment].within == _which)    &&
            ( (argument >= last) ||
              (cmake->comments[*_comment].start <
               cmake->arguments[argument].start) );

        if (comment_first)
        {
            d_internal_comment(_build, node, &cmake->comments[*_comment]);
            (*_comment)++;
        }
        else
        {
            d_internal_argument(_build, node, &cmake->arguments[argument]);
            argument++;
        }
    }

    return node;
}


/*
d_internal_block
  Opens a block node for an opener, beneath a parent.
*/
static uint32_t
d_internal_block(
    struct d_internal_build* _build,
    uint32_t                 _parent,
    size_t                   _which
)
{
    const struct d_cmake* const         cmake   = _build->cmake;
    const struct d_cmake_command* const command = &cmake->commands[_which];
    const uint32_t node = d_node_add(_build->tree, _parent, "block");

    // a node that cannot be added leaves the rest of the tree as it is
    if (node == D_DSS_NO_INDEX)
    {
        return D_DSS_NO_INDEX;
    }

    (void)d_node_set_attribute(_build->tree, node, "kind",
                               d_cmake_block_name(command->block));

    // a function or a macro is named by its first argument
    if ( ( (command->block == D_CMAKE_BLOCK_FUNCTION) ||
           (command->block == D_CMAKE_BLOCK_MACRO) ) &&
         (command->argument_count > 0u) )
    {
        const char* const value =
            d_internal_value(_build,
                             &cmake->arguments[command->first_argument]);

        if (value)
        {
            (void)d_node_set_attribute(_build->tree, node, "name", value);
        }
    }

    if (command->unpaired)
    {
        (void)d_node_set_attribute(_build->tree, node, "unclosed", NULL);
    }

    (void)d_node_set_text(_build->tree, node, &cmake->text[command->start],
                          command->name_length);
    d_internal_place(_build, node, command->start, command->end,
                     command->line, command->column);
    _build->stats->blocks++;

    return node;
}


/*
d_internal_tree
  Walks the commands and the comments outside them together, nesting each
block's commands beneath it.
*/
static void
d_internal_tree(
    struct d_internal_build* _build,
    uint32_t                 _file
)
{
    const struct d_cmake* const cmake = _build->cmake;
    uint32_t* const nodes  = malloc((cmake->command_count + 1u)
                                    * sizeof(uint32_t));
    size_t* const   owners = malloc((cmake->command_count + 1u)
                                    * sizeof(size_t));
    size_t          open    = 0;
    size_t          command = 0;
    size_t          comment = 0;

    // the stack is as deep as the file could nest
    if ( (!nodes) ||
         (!owners) )
    {
        free(nodes);
        free(owners);

        return;
    }

    while ( (command < cmake->command_count) ||
            (comment < cmake->comment_count) )
    {
        const uint32_t parent = (open > 0u) ? nodes[open - 1u] : _file;

        // a comment outside every command, where it stands among them
        if ( (comment < cmake->comment_count) &&
             ( (command >= cmake->command_count) ||
               (cmake->comments[comment].start <
                cmake->commands[command].start) ) )
        {
            d_internal_comment(_build, parent, &cmake->comments[comment]);
            comment++;
            continue;
        }

        const struct d_cmake_command* const record =
            &cmake->commands[command];

        // an opener holds itself and what follows, until its closer
        if (record->role == D_CMAKE_ROLE_OPEN)
        {
            const uint32_t block = d_internal_block(_build, parent, command);

            if (block != D_DSS_NO_INDEX)
            {
                (void)d_internal_command(_build, block, command, &comment);
                nodes[open]  = block;
                owners[open] = command;
                open++;
            }
            else
            {
                (void)d_internal_command(_build, parent, command, &comment);
            }

            command++;
            continue;
        }

        // a paired closer pops to its own block, which ends with it
        if ( (record->role == D_CMAKE_ROLE_CLOSE) &&
             (!record->unpaired) )
        {
            size_t found = open;

            while ( (found > 0u) &&
                    (owners[found - 1u] != record->partner) )
            {
                found--;
            }

            // a block the stack lost is no reason to lose the command
            if (found > 0u)
            {
                (void)d_internal_command(_build, nodes[found - 1u], command,
                                         &comment);
                open = found - 1u;
            }
            else
            {
                (void)d_internal_command(_build, parent, command, &comment);
            }

            command++;
            continue;
        }

        (void)d_internal_command(_build, parent, command, &comment);
        command++;
    }

    free(nodes);
    free(owners);

    return;
}


/*
d_ext_cmake_accepts
  The base name decides: CMakeLists.txt, *.cmake, *.cmake.in.
*/
bool
d_ext_cmake_accepts(
    const char* _path
)
{
    // parameter validation first
    if (!_path)
    {
        return false;
    }

    const char* const slash     = strrchr(_path, '/');
    const char* const backslash = strrchr(_path, '\\');
    const char*       name      = _path;

    // the base name follows the last separator of either kind
    if ( (slash) &&
         (slash >= name) )
    {
        name = slash + 1;
    }

    if ( (backslash) &&
         (backslash >= name) )
    {
        name = backslash + 1;
    }

    const size_t span = strlen(name);

    return ( (strcmp(name, "CMakeLists.txt") == 0)                 ||
             ( (span > 6u) &&
               (strcmp(&name[span - 6u], ".cmake") == 0) )         ||
             ( (span > 9u) &&
               (strcmp(&name[span - 9u], ".cmake.in") == 0) ) );
}


/*
d_ext_cmake_build
  The file node is marked before anything is read, so a sheet can key on the
language even for a file that is generated or refused.
*/
uint32_t
d_ext_cmake_build(
    struct d_node_tree*       _tree,
    uint32_t                  _parent,
    const char*               _text,
    size_t                    _length,
    const char*               _path,
    struct d_ext_cmake_stats* _out_stats
)
{
    struct d_ext_cmake_stats        scratch;
    struct d_ext_cmake_stats* const stats = _out_stats ? _out_stats
                                                       : &scratch;

    memset(stats, 0, sizeof(*stats));

    // parameter validation first
    if (!_tree)
    {
        return D_DSS_NO_INDEX;
    }

    const uint32_t file = (_parent == D_DSS_NO_INDEX)
                        ? d_node_add(_tree, D_DSS_NO_INDEX, "file")
                        : _parent;

    // with no file node there is nowhere to build
    if (file == D_DSS_NO_INDEX)
    {
        return D_DSS_NO_INDEX;
    }

    (void)d_node_set_attribute(_tree, file, "lang", "cmake");

    stats->generated = d_cmake_generated(_text, _text ? _length : 0u) ||
                       ( (_path) &&
                         ( (strstr(_path, "/CMakeFiles/") != NULL) ||
                           (strstr(_path, "\\CMakeFiles\\") != NULL) ) );

    // a generated file is marked, and given no tree
    if (stats->generated)
    {
        (void)d_node_set_attribute(_tree, file, "generated", NULL);

        return file;
    }

    struct d_cmake cmake;
    char           why[256];

    stats->refused = !d_cmake_read(&cmake, _text, _text ? _length : 0u, why,
                                   sizeof(why));

    struct d_internal_build build;

    memset(&build, 0, sizeof(build));
    build.tree  = _tree;
    build.cmake = &cmake;
    build.stats = stats;

    d_internal_tree(&build, file);

    // where the text stopped being CMake, after everything before it
    if (stats->refused)
    {
        const uint32_t refusal = d_node_add(_tree, file, "refusal");

        if (refusal != D_DSS_NO_INDEX)
        {
            struct d_node* const node = d_node_at(_tree, refusal);

            node->line         = (uint32_t)cmake.error_line;
            node->start_column = (uint32_t)cmake.error_column;
            node->end_column   = (uint32_t)cmake.error_column;
            node->width        = 1u;

            (void)d_node_set_text(_tree, refusal, why, strlen(why));
        }
    }

    free(build.scratch);
    d_cmake_release(&cmake);

    return file;
}


/*
d_internal_slurp
  Reads a whole file; the caller frees it.
*/
static char*
d_internal_slurp(
    const char* _path,
    size_t*     _length
)
{
    FILE* const handle = fopen(_path, "rb");

    // a file that cannot be opened has no text
    if (!handle)
    {
        return NULL;
    }

    size_t capacity = 4096u;
    size_t used     = 0;
    char*  text     = malloc(capacity);

    while (text)
    {
        used += fread(&text[used], 1u, capacity - used, handle);

        // a short read is the end of the file
        if (used < capacity)
        {
            break;
        }

        char* const grown = realloc(text, capacity * 2u);

        if (!grown)
        {
            free(text);
            text = NULL;
            break;
        }

        text      = grown;
        capacity *= 2u;
    }

    (void)fclose(handle);
    *_length = used;

    return text;
}


/*
d_ext_cmake_build_path
  The driver's adapter: read the file, then build beneath its node.
*/
uint32_t
d_ext_cmake_build_path(
    struct d_node_tree*       _tree,
    uint32_t                  _file,
    const char*               _path,
    struct d_ext_cmake_stats* _out_stats
)
{
    // parameter validation first
    if ( (!_tree)                        ||
         (_file == D_DSS_NO_INDEX)       ||
         (!d_ext_cmake_accepts(_path)) )
    {
        return D_DSS_NO_INDEX;
    }

    size_t      length = 0;
    char* const text   = d_internal_slurp(_path, &length);

    // a file that cannot be read is CMake still, with nothing in it
    if (!text)
    {
        (void)d_node_set_attribute(_tree, _file, "lang", "cmake");

        if (_out_stats)
        {
            memset(_out_stats, 0, sizeof(*_out_stats));
        }

        return _file;
    }

    const uint32_t file = d_ext_cmake_build(_tree, _file, text, length, _path,
                                            _out_stats);

    free(text);

    return file;
}
