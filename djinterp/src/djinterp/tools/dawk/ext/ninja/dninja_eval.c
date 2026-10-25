/*******************************************************************************
* djinterp [tools]                                                 dninja_eval.c
*
* Definitions for the declarations in dninja_eval.h.
*   Each file is read whole by dninja, then its statements taken in order,
* as ninja's manifest parser takes each as it reads it; a file that dninja
* refused partway has its statements before the refusal taken, and the one
* cut short taken as far as ninja would have taken it, before the refusal
* is reported. Scopes are ninja's BindingEnvs: a chain of bindings, each
* scope's own rules beside them; a build with bindings has a scope of its
* own within its file's. Nodes are kept in a hash table by their canonical
* paths.
*   The files are kept with the build, as a rule's bindings are evaluated
* from the records of the file that defined it.
*
* path:      /src/djinterp/tools/dawk/ext/ninja/dninja_eval.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/ninja/dninja_eval.h"  // corresponding header
// std
#include <errno.h>   // errno
#include <limits.h>  // INT_MAX
#include <stdio.h>   // FILE, fopen, fread, fclose, snprintf
#include <stdlib.h>  // malloc, calloc, realloc, free, atoi, atol
#include <string.h>  // memcmp, memcpy, memset, strcmp, strerror, strlen


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Text
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_buffer
//   struct: text being made, in an array that grows; a failure is kept.
struct d_internal_buffer
{
    char*  text;
    size_t length;
    size_t capacity;
    bool   failed;
};

// 1.2    The build
//------------------------------------------------------------------------------
// 1.2.1
// d_internal_binding
//   struct: a name bound to its value, evaluated.
struct d_internal_binding
{
    char* name;
    char* value;
};

// 1.2.2
// d_internal_env
//   struct: a scope: its parent, D_NINJA_NONE at the top, its bindings and
//   the rules defined in it, by their index.
struct d_internal_env
{
    size_t                     parent;
    struct d_internal_binding* bindings;
    size_t                     binding_count;
    size_t                     binding_capacity;
    size_t*                    rules;
    size_t                     rule_count;
    size_t                     rule_capacity;
};

// 1.2.3
// d_internal_rule
//   struct: a rule, by its name, and the statement that defines it in its
//   file; phony has none.
struct d_internal_rule
{
    char*  name;
    size_t file;
    size_t statement;
    bool   phony;
};

// 1.2.4
// d_internal_pool
//   struct: a pool, by its name, and its depth.
struct d_internal_pool
{
    char* name;
    int   depth;
};

// 1.2.5
// d_internal_list
//   struct: indices in an array that grows.
struct d_internal_list
{
    size_t* items;
    size_t  count;
    size_t  capacity;
};

// 1.2.6
// d_internal_edge
//   struct: a build: its rule, its scope, its pool, where it was written,
//   and its nodes; its inputs' sections are counted from their end, as
//   ninja counts them.
struct d_internal_edge
{
    size_t                 rule;
    size_t                 env;
    size_t                 pool;
    size_t                 file;
    size_t                 statement;
    struct d_internal_list outputs;
    size_t                 implicit_outputs;
    struct d_internal_list inputs;
    size_t                 implicit;
    size_t                 order_only;
    struct d_internal_list validations;
    struct d_internal_list sections[6];
};

// 1.2.7
// d_internal_node
//   struct: a path of the build, and the edges about it.
struct d_internal_node
{
    char*                  path;
    size_t                 in_edge;
    struct d_internal_list out_edges;
    struct d_internal_list validation_edges;
};

// 1.2.8
// d_internal_file
//   struct: a file read, its text and its records.
struct d_internal_file
{
    char*          path;
    char*          text;
    size_t         length;
    struct d_ninja records;
};

// 1.2.9
// d_ninja_graph
//   struct: the build.
struct d_ninja_graph
{
    struct d_internal_file* files;
    size_t                  file_count;
    size_t                  file_capacity;
    struct d_internal_env*  envs;
    size_t                  env_count;
    size_t                  env_capacity;
    struct d_internal_rule* rules;
    size_t                  rule_count;
    size_t                  rule_capacity;
    struct d_internal_pool* pools;
    size_t                  pool_count;
    size_t                  pool_capacity;
    struct d_internal_edge* edges;
    size_t                  edge_count;
    size_t                  edge_capacity;
    struct d_internal_node* nodes;
    size_t                  node_count;
    size_t                  node_capacity;
    size_t*                 table;
    size_t                  table_size;
    struct d_internal_list  defaults;
    struct d_internal_list  roots;
    struct d_internal_list  top_rules;
    char**                  warnings;
    size_t                  warning_count;
    size_t                  warning_capacity;
    char*                   directory;
    char                    version[32];
    int                     major;
    int                     minor;
    bool                    win32;
    bool                    failed;
    char                    problem[768];
};

// 1.2.10
// d_internal_edge_env
//   struct: an evaluation in an edge's sense: the edge, whether $in and $out
//   are shell-escaped, the bindings being evaluated, for cycles, and what
//   went wrong.
struct d_internal_edge_env
{
    const struct d_ninja_graph* graph;
    size_t                      edge;
    bool                        escape;
    bool                        recursive;
    const char*                 stack[64];
    size_t                      lengths[64];
    size_t                      depth;
    bool                        failed;
    char                        problem[512];
};


//==============================================================================
// 2.  TEXT AND LISTS
//==============================================================================


/*
d_internal_reserve
  Room for `_more` bytes and a terminator; false, the buffer failed, when
memory runs out.
*/
static bool
d_internal_reserve(
    struct d_internal_buffer* _buffer,
    size_t                    _more
)
{
    // a buffer that failed takes nothing more
    if (_buffer->failed)
    {
        return false;
    }

    // room already
    if ((_buffer->length + _more + 1u) <= _buffer->capacity)
    {
        return true;
    }

    size_t wanted = (_buffer->capacity > 0u) ? _buffer->capacity : 64u;

    // doubled until it holds the text
    while (wanted < (_buffer->length + _more + 1u))
    {
        wanted *= 2u;
    }

    char* const grown = realloc(_buffer->text, wanted);

    // memory that ran out fails the buffer
    if (!grown)
    {
        _buffer->failed = true;

        return false;
    }

    _buffer->text     = grown;
    _buffer->capacity = wanted;

    return true;
}


/*
d_internal_add, d_internal_add_char
  Appends to a buffer.
*/
static bool
d_internal_add(
    struct d_internal_buffer* _buffer,
    const char*               _text,
    size_t                    _length
)
{
    // room for it
    if (!d_internal_reserve(_buffer, _length))
    {
        return false;
    }

    // the text, when there is any
    if (_length > 0u)
    {
        memcpy(&_buffer->text[_buffer->length], _text, _length);
    }

    _buffer->length                += _length;
    _buffer->text[_buffer->length]  = '\0';

    return true;
}


static bool
d_internal_add_char(
    struct d_internal_buffer* _buffer,
    char                      _c
)
{
    return d_internal_add(_buffer, &_c, 1u);
}


/*
d_internal_take
  The buffer's text, for the caller to free, "" for an empty one; NULL,
the text freed, for one that failed.
*/
static char*
d_internal_take(
    struct d_internal_buffer* _buffer
)
{
    // a buffer that failed gives nothing
    if (_buffer->failed)
    {
        free(_buffer->text);

        return NULL;
    }

    // an empty one, its terminator
    if ( (!_buffer->text) &&
         (!d_internal_reserve(_buffer, 0u)) )
    {
        return NULL;
    }

    _buffer->text[_buffer->length] = '\0';

    return _buffer->text;
}


/*
d_internal_copy
  A string of its own.
*/
static char*
d_internal_copy(
    const char* _text,
    size_t      _length
)
{
    char* const copy = malloc(_length + 1u);

    // a copy that cannot be made is NULL
    if (copy)
    {
        memcpy(copy, _text, _length);
        copy[_length] = '\0';
    }

    return copy;
}


/*
d_internal_push
  Adds an index to a list.
*/
static bool
d_internal_push(
    struct d_internal_list* _list,
    size_t                  _item
)
{
    // room for one more
    if (_list->count >= _list->capacity)
    {
        const size_t  wanted = (_list->capacity > 0u) ? (_list->capacity * 2u)
                                                      : 4u;
        size_t* const grown  = realloc(_list->items,
                                       wanted * sizeof(*grown));

        // memory that ran out
        if (!grown)
        {
            return false;
        }

        _list->items    = grown;
        _list->capacity = wanted;
    }

    _list->items[_list->count++] = _item;

    return true;
}


/*
d_internal_grow
  Room for one more record in an array that doubles as it fills.
*/
static bool
d_internal_grow(
    void**  _array,
    size_t* _capacity,
    size_t  _count,
    size_t  _size
)
{
    // room already
    if (_count < *_capacity)
    {
        return true;
    }

    const size_t wanted = (*_capacity > 0u) ? (*_capacity * 2u) : 8u;
    void* const  grown  = realloc(*_array, wanted * _size);

    // memory that ran out
    if (!grown)
    {
        return false;
    }

    *_array    = grown;
    *_capacity = wanted;

    return true;
}


/*
d_internal_same
  Whether two spans hold the same bytes.
*/
static bool
d_internal_same(
    const char* _left,
    size_t      _left_length,
    const char* _right,
    size_t      _right_length
)
{
    return ( (_left_length == _right_length) &&
             ( (_left_length == 0u) ||
               (memcmp(_left, _right, _left_length) == 0) ) );
}


//==============================================================================
// 3.  THE GRAPH'S RECORDS
//==============================================================================


/*
d_internal_fail
  Says why loading failed, and gives false; the first failure stands.
*/
static bool
d_internal_fail(
    struct d_ninja_graph* _graph,
    const char*           _message
)
{
    // only the first failure is the build's
    if (!_graph->failed)
    {
        _graph->failed = true;
        (void)snprintf(_graph->problem, sizeof(_graph->problem), "%s",
                       _message);
    }

    return false;
}


/*
d_internal_fail_at
  Fails with ninja's own form of a parser's error: the file, the line, and
the message.
*/
static bool
d_internal_fail_at(
    struct d_ninja_graph* _graph,
    size_t                _file,
    size_t                _line,
    const char*           _message
)
{
    char text[768];

    (void)snprintf(text, sizeof(text), "%s:%zu: %s",
                   _graph->files[_file].path, _line, _message);

    return d_internal_fail(_graph, text);
}


/*
d_internal_warn
  Keeps a warning, as ninja would print it.
*/
static bool
d_internal_warn(
    struct d_ninja_graph* _graph,
    const char*           _message
)
{
    // room for one more
    if (!d_internal_grow((void**)&_graph->warnings, &_graph->warning_capacity,
                         _graph->warning_count, sizeof(*_graph->warnings)))
    {
        return d_internal_fail(_graph, "out of memory");
    }

    _graph->warnings[_graph->warning_count] =
        d_internal_copy(_message, strlen(_message));

    // a warning that cannot be kept
    if (!_graph->warnings[_graph->warning_count])
    {
        return d_internal_fail(_graph, "out of memory");
    }

    _graph->warning_count++;

    return true;
}


/*
d_internal_new_env
  A scope within another, or at the top; D_NINJA_NONE when memory runs out.
*/
static size_t
d_internal_new_env(
    struct d_ninja_graph* _graph,
    size_t                _parent
)
{
    // room for one more
    if (!d_internal_grow((void**)&_graph->envs, &_graph->env_capacity,
                         _graph->env_count, sizeof(*_graph->envs)))
    {
        (void)d_internal_fail(_graph, "out of memory");

        return D_NINJA_NONE;
    }

    struct d_internal_env* const env = &_graph->envs[_graph->env_count];

    memset(env, 0, sizeof(*env));
    env->parent = _parent;

    return _graph->env_count++;
}


/*
d_internal_find_binding
  A scope's own binding of a name, or NULL.
*/
static const struct d_internal_binding*
d_internal_find_binding(
    const struct d_ninja_graph* _graph,
    size_t                      _env,
    const char*                 _name,
    size_t                      _length
)
{
    const struct d_internal_env* const env = &_graph->envs[_env];

    // each binding of the scope
    for (size_t at = 0; at < env->binding_count; ++at)
    {
        // the name's
        if (d_internal_same(env->bindings[at].name,
                            strlen(env->bindings[at].name), _name, _length))
        {
            return &env->bindings[at];
        }
    }

    return NULL;
}


/*
d_internal_bind
  Binds a name in a scope, over a binding it has already; the value is
taken.
*/
static bool
d_internal_bind(
    struct d_ninja_graph* _graph,
    size_t                _env,
    const char*           _name,
    size_t                _length,
    char*                 _value
)
{
    struct d_internal_env* const env = &_graph->envs[_env];

    // a value that could not be made
    if (!_value)
    {
        return d_internal_fail(_graph, "out of memory");
    }

    // a name bound already takes the new value
    for (size_t at = 0; at < env->binding_count; ++at)
    {
        // the name's
        if (d_internal_same(env->bindings[at].name,
                            strlen(env->bindings[at].name), _name, _length))
        {
            free(env->bindings[at].value);
            env->bindings[at].value = _value;

            return true;
        }
    }

    char* const name = d_internal_copy(_name, _length);

    // room for one more, and its name
    if ( (!name) ||
         (!d_internal_grow((void**)&env->bindings, &env->binding_capacity,
                           env->binding_count, sizeof(*env->bindings))) )
    {
        free(name);
        free(_value);

        return d_internal_fail(_graph, "out of memory");
    }

    env->bindings[env->binding_count].name  = name;
    env->bindings[env->binding_count].value = _value;
    env->binding_count++;

    return true;
}


/*
d_internal_lookup
  A variable through a scope and those about it, as BindingEnv's
LookupVariable finds it; "" for none.
*/
static const char*
d_internal_lookup(
    const struct d_ninja_graph* _graph,
    size_t                      _env,
    const char*                 _name,
    size_t                      _length
)
{
    size_t env = _env;

    // each scope out to the top
    while (env != D_NINJA_NONE)
    {
        const struct d_internal_binding* const binding =
            d_internal_find_binding(_graph, env, _name, _length);

        // the nearest binding
        if (binding)
        {
            return binding->value;
        }

        env = _graph->envs[env].parent;
    }

    return "";
}


/*
d_internal_rule_in
  A rule a scope defines itself, by its name, or D_NINJA_NONE.
*/
static size_t
d_internal_rule_in(
    const struct d_ninja_graph* _graph,
    size_t                      _env,
    const char*                 _name,
    size_t                      _length
)
{
    const struct d_internal_env* const env = &_graph->envs[_env];

    // each rule the scope defines
    for (size_t at = 0; at < env->rule_count; ++at)
    {
        const char* const name = _graph->rules[env->rules[at]].name;

        // the name's
        if (d_internal_same(name, strlen(name), _name, _length))
        {
            return env->rules[at];
        }
    }

    return D_NINJA_NONE;
}


/*
d_internal_find_rule
  A rule through a scope and those about it, as LookupRule finds it.
*/
static size_t
d_internal_find_rule(
    const struct d_ninja_graph* _graph,
    size_t                      _env,
    const char*                 _name,
    size_t                      _length
)
{
    size_t env = _env;

    // each scope out to the top
    while (env != D_NINJA_NONE)
    {
        const size_t rule = d_internal_rule_in(_graph, env, _name, _length);

        // the nearest
        if (rule != D_NINJA_NONE)
        {
            return rule;
        }

        env = _graph->envs[env].parent;
    }

    return D_NINJA_NONE;
}


/*
d_internal_add_rule
  A rule defined in a scope.
*/
static bool
d_internal_add_rule(
    struct d_ninja_graph* _graph,
    size_t                _env,
    const char*           _name,
    size_t                _length,
    size_t                _file,
    size_t                _statement
)
{
    struct d_internal_env* const env  = &_graph->envs[_env];
    char* const                  name = d_internal_copy(_name, _length);

    // room for it, in the build and in its scope
    if ( (!name) ||
         (!d_internal_grow((void**)&_graph->rules, &_graph->rule_capacity,
                           _graph->rule_count, sizeof(*_graph->rules))) ||
         (!d_internal_grow((void**)&env->rules, &env->rule_capacity,
                           env->rule_count, sizeof(*env->rules))) )
    {
        free(name);

        return d_internal_fail(_graph, "out of memory");
    }

    struct d_internal_rule* const rule = &_graph->rules[_graph->rule_count];

    rule->name                    = name;
    rule->file                    = _file;
    rule->statement               = _statement;
    rule->phony                   = (_file == D_NINJA_NONE);
    env->rules[env->rule_count++] = _graph->rule_count++;

    return true;
}


/*
d_internal_find_pool
  A pool by its name, or D_NINJA_NONE.
*/
static size_t
d_internal_find_pool(
    const struct d_ninja_graph* _graph,
    const char*                 _name,
    size_t                      _length
)
{
    // each pool
    for (size_t at = 0; at < _graph->pool_count; ++at)
    {
        // the name's
        if (d_internal_same(_graph->pools[at].name,
                            strlen(_graph->pools[at].name), _name, _length))
        {
            return at;
        }
    }

    return D_NINJA_NONE;
}


/*
d_internal_add_pool
  A pool of a depth.
*/
static bool
d_internal_add_pool(
    struct d_ninja_graph* _graph,
    const char*           _name,
    size_t                _length,
    int                   _depth
)
{
    char* const name = d_internal_copy(_name, _length);

    // room for it
    if ( (!name) ||
         (!d_internal_grow((void**)&_graph->pools, &_graph->pool_capacity,
                           _graph->pool_count, sizeof(*_graph->pools))) )
    {
        free(name);

        return d_internal_fail(_graph, "out of memory");
    }

    _graph->pools[_graph->pool_count].name  = name;
    _graph->pools[_graph->pool_count].depth = _depth;
    _graph->pool_count++;

    return true;
}


//==============================================================================
// 4.  NODES
//==============================================================================


/*
d_internal_hash
  FNV-1a over a path's bytes.
*/
static size_t
d_internal_hash(
    const char* _text,
    size_t      _length
)
{
    unsigned long hash = 2166136261ul;

    // each byte
    for (size_t at = 0; at < _length; ++at)
    {
        hash ^= (unsigned char)_text[at];
        hash *= 16777619ul;
    }

    return (size_t)hash;
}


/*
d_internal_slot
  Where a path stands in the table, or the empty slot it would take.
*/
static size_t
d_internal_slot(
    const struct d_ninja_graph* _graph,
    const char*                 _path,
    size_t                      _length
)
{
    const size_t mask = _graph->table_size - 1u;
    size_t       slot = d_internal_hash(_path, _length) & mask;

    // each slot from the path's, until its node or an empty one
    while (_graph->table[slot] != D_NINJA_NONE)
    {
        const char* const path = _graph->nodes[_graph->table[slot]].path;

        // the path's own
        if (d_internal_same(path, strlen(path), _path, _length))
        {
            return slot;
        }

        slot = (slot + 1u) & mask;
    }

    return slot;
}


/*
d_internal_rehash
  A table twice the size, every node in it again.
*/
static bool
d_internal_rehash(
    struct d_ninja_graph* _graph
)
{
    const size_t  size  = (_graph->table_size > 0u) ? (_graph->table_size * 2u)
                                                    : 64u;
    size_t* const table = malloc(size * sizeof(*table));

    // memory that ran out
    if (!table)
    {
        return d_internal_fail(_graph, "out of memory");
    }

    free(_graph->table);
    _graph->table      = table;
    _graph->table_size = size;

    // every slot empty
    for (size_t at = 0; at < size; ++at)
    {
        table[at] = D_NINJA_NONE;
    }

    // every node in its slot
    for (size_t at = 0; at < _graph->node_count; ++at)
    {
        const char* const path = _graph->nodes[at].path;

        table[d_internal_slot(_graph, path, strlen(path))] = at;
    }

    return true;
}


/*
d_internal_lookup_node
  The node of a canonical path, or D_NINJA_NONE.
*/
static size_t
d_internal_lookup_node(
    const struct d_ninja_graph* _graph,
    const char*                 _path,
    size_t                      _length
)
{
    // an empty table holds nothing
    if (_graph->table_size == 0u)
    {
        return D_NINJA_NONE;
    }

    return _graph->table[d_internal_slot(_graph, _path, _length)];
}


/*
d_internal_get_node
  The node of a canonical path, made when there is none, as GetNode makes
it; D_NINJA_NONE when memory runs out.
*/
static size_t
d_internal_get_node(
    struct d_ninja_graph* _graph,
    const char*           _path,
    size_t                _length
)
{
    const size_t found = d_internal_lookup_node(_graph, _path, _length);

    // a node already
    if (found != D_NINJA_NONE)
    {
        return found;
    }

    // a table with room to spare, and room for one more node
    if ( ( ((_graph->node_count + 1u) * 2u) > _graph->table_size &&
           (!d_internal_rehash(_graph)) ) ||
         (!d_internal_grow((void**)&_graph->nodes, &_graph->node_capacity,
                           _graph->node_count, sizeof(*_graph->nodes))) )
    {
        (void)d_internal_fail(_graph, "out of memory");

        return D_NINJA_NONE;
    }

    struct d_internal_node* const node = &_graph->nodes[_graph->node_count];

    memset(node, 0, sizeof(*node));
    node->path    = d_internal_copy(_path, _length);
    node->in_edge = D_NINJA_NONE;

    // a path that cannot be kept
    if (!node->path)
    {
        (void)d_internal_fail(_graph, "out of memory");

        return D_NINJA_NONE;
    }

    _graph->table[d_internal_slot(_graph, _path, _length)] =
        _graph->node_count;

    return _graph->node_count++;
}


//==============================================================================
// 5.  EVALUATION
//==============================================================================


/*
d_internal_add_text_piece
  A text piece's text, decoded, appended.
*/
static bool
d_internal_add_text_piece(
    const struct d_ninja*       _ninja,
    const struct d_ninja_piece* _piece,
    struct d_internal_buffer*   _out
)
{
    // room for the piece, which decoding never lengthens
    if (!d_internal_reserve(_out, _piece->end - _piece->start))
    {
        return false;
    }

    _out->length += d_ninja_text_of(_ninja, _piece, &_out->text[_out->length]);

    return true;
}


/*
d_internal_evaluate
  A value evaluated in a scope, as EvalString's Evaluate does with a
BindingEnv; the caller frees it.
*/
static char*
d_internal_evaluate(
    const struct d_ninja_graph* _graph,
    size_t                      _file,
    size_t                      _value,
    size_t                      _env
)
{
    const struct d_ninja* const       ninja = &_graph->files[_file].records;
    const struct d_ninja_value* const value = &ninja->values[_value];
    struct d_internal_buffer          out   = { NULL, 0u, 0u, false };

    // each piece: text, or a variable looked up
    for (size_t at = 0; at < value->piece_count; ++at)
    {
        const struct d_ninja_piece* const piece =
            &ninja->pieces[value->first_piece + at];

        // a table of the two kinds
        if (piece->kind == D_NINJA_TEXT)
        {
            (void)d_internal_add_text_piece(ninja, piece, &out);
        }
        else
        {
            const char* const found =
                d_internal_lookup(_graph, _env,
                                  &ninja->text[piece->name_start],
                                  piece->name_end - piece->name_start);

            (void)d_internal_add(&out, found, strlen(found));
        }
    }

    return d_internal_take(&out);
}


/*
d_internal_rule_value
  The value a rule binds a name to, as the last binding of the name in its
statement: its file and value, or false for none.
*/
static bool
d_internal_rule_value(
    const struct d_ninja_graph* _graph,
    size_t                      _rule,
    const char*                 _name,
    size_t                      _length,
    size_t*                     _out_value
)
{
    const struct d_internal_rule* const rule  = &_graph->rules[_rule];
    bool                                found = false;

    // phony binds nothing
    if (rule->phony)
    {
        return false;
    }

    const struct d_ninja* const           ninja     =
        &_graph->files[rule->file].records;
    const struct d_ninja_statement* const statement =
        &ninja->statements[rule->statement];

    // each binding, the last of the name kept
    for (size_t at = 0; at < statement->binding_count; ++at)
    {
        const struct d_ninja_binding* const binding =
            &ninja->bindings[statement->first_binding + at];

        // the name's
        if (d_internal_same(&ninja->text[binding->name_start],
                            binding->name_end - binding->name_start, _name,
                            _length))
        {
            *_out_value = binding->value;
            found       = true;
        }
    }

    return found;
}


/*
d_internal_path_list
  An edge's nodes from one index to another, a separator between, each
shell-escaped when asked, as MakePathList writes them.
*/
static bool
d_internal_path_list(
    const struct d_internal_edge_env* _env,
    const struct d_internal_list*     _list,
    size_t                            _from,
    size_t                            _to,
    char                              _separator,
    struct d_internal_buffer*         _out
)
{
    const struct d_ninja_graph* const graph = _env->graph;

    // each node of the span
    for (size_t at = _from; at < _to; ++at)
    {
        const char* const path   = graph->nodes[_list->items[at]].path;
        const size_t      length = strlen(path);

        // a separator between two
        if (at > _from)
        {
            (void)d_internal_add_char(_out, _separator);
        }

        // as it stands, or escaped
        if (!_env->escape)
        {
            (void)d_internal_add(_out, path, length);
        }
        else
        {
            size_t      used    = 0;
            char* const escaped = d_ninja_shell_escape(path, length,
                                                       graph->win32, &used);

            (void)d_internal_add(_out, escaped, used);
            _out->failed = (_out->failed) || (!escaped);
            free(escaped);
        }
    }

    return !_out->failed;
}


static bool d_internal_edge_lookup(struct d_internal_edge_env* _env,
                                   const char*                 _name,
                                   size_t                      _length,
                                   struct d_internal_buffer*   _out);


/*
d_internal_edge_evaluate
  A value of a rule's evaluated in an edge's sense: each reference looked
up as EdgeEnv looks one up.
*/
static bool
d_internal_edge_evaluate(
    struct d_internal_edge_env* _env,
    size_t                      _file,
    size_t                      _value,
    struct d_internal_buffer*   _out
)
{
    const struct d_ninja* const       ninja =
        &_env->graph->files[_file].records;
    const struct d_ninja_value* const value = &ninja->values[_value];

    // each piece: text, or a variable looked up in the edge's sense
    for (size_t at = 0; (!_env->failed) && (at < value->piece_count); ++at)
    {
        const struct d_ninja_piece* const piece =
            &ninja->pieces[value->first_piece + at];

        // a table of the two kinds
        if (piece->kind == D_NINJA_TEXT)
        {
            (void)d_internal_add_text_piece(ninja, piece, _out);
        }
        else
        {
            (void)d_internal_edge_lookup(_env,
                                         &ninja->text[piece->name_start],
                                         piece->name_end - piece->name_start,
                                         _out);
        }
    }

    return ( (!_env->failed) && (!_out->failed) );
}


/*
d_internal_cycle
  Whether a name is among those being evaluated: the cycle, from it, is
written as ninja writes one, and the evaluation fails.
*/
static bool
d_internal_cycle(
    struct d_internal_edge_env* _env,
    const char*                 _name,
    size_t                      _length
)
{
    size_t from = _env->depth;

    // the name, the first time it is on the stack
    for (size_t at = 0; at < _env->depth; ++at)
    {
        // the name's
        if (d_internal_same(_env->stack[at], _env->lengths[at], _name,
                            _length))
        {
            from = at;
            break;
        }
    }

    // a name not being evaluated is no cycle
    if (from == _env->depth)
    {
        return false;
    }

    size_t used = (size_t)snprintf(_env->problem, sizeof(_env->problem),
                                   "cycle in rule variables: ");

    // each name from it, then the name again
    for (size_t at = from; (at < _env->depth) && (used < sizeof(_env->problem));
         ++at)
    {
        used += (size_t)snprintf(&_env->problem[used],
                                 sizeof(_env->problem) - used, "%.*s -> ",
                                 (int)_env->lengths[at], _env->stack[at]);
    }

    // the name that closes it
    if (used < sizeof(_env->problem))
    {
        (void)snprintf(&_env->problem[used], sizeof(_env->problem) - used,
                       "%.*s", (int)_length, _name);
    }

    _env->failed = true;

    return true;
}


/*
d_internal_fallback
  BindingEnv's LookupWithFallback from the edge's scope: the scope's own
binding, else the rule's value evaluated in the edge's sense, else the
scopes about it.
*/
static bool
d_internal_fallback(
    struct d_internal_edge_env* _env,
    const char*                 _name,
    size_t                      _length,
    bool                        _has_value,
    size_t                      _value,
    struct d_internal_buffer*   _out
)
{
    const struct d_ninja_graph* const  graph = _env->graph;
    const struct d_internal_edge* const edge = &graph->edges[_env->edge];
    const struct d_internal_binding* const own =
        d_internal_find_binding(graph, edge->env, _name, _length);

    // a table of where the value comes from: the scope, the rule, the
    // scopes about it
    if (own)
    {
        return d_internal_add(_out, own->value, strlen(own->value));
    }
    else if (_has_value)
    {
        return d_internal_edge_evaluate(_env, graph->rules[edge->rule].file,
                                        _value, _out);
    }
    else if (graph->envs[edge->env].parent != D_NINJA_NONE)
    {
        const char* const found =
            d_internal_lookup(graph, graph->envs[edge->env].parent, _name,
                              _length);

        return d_internal_add(_out, found, strlen(found));
    }

    return true;
}


/*
d_internal_edge_lookup
  EdgeEnv's LookupVariable: $in, $in_newline and $out from the edge's
paths; else the name, checked for a cycle once the evaluation is within
another, kept on the stack while its rule's value is evaluated, looked up
with the fallback.
  Long as it is ninja's own order of lookups, each a step.
*/
static bool
d_internal_edge_lookup(
    struct d_internal_edge_env* _env,
    const char*                 _name,
    size_t                      _length,
    struct d_internal_buffer*   _out
)
{
    const struct d_ninja_graph* const   graph = _env->graph;
    const struct d_internal_edge* const edge  = &graph->edges[_env->edge];
    const size_t                        count = edge->inputs.count;
    const size_t                        taken = edge->implicit +
                                                edge->order_only;
    const size_t                        plain = (count > taken)
                                                    ? (count - taken) : 0u;
    size_t                              value = 0;

    // a table of the names an edge's paths give
    if ( (d_internal_same(_name, _length, "in", 2u)) ||
         (d_internal_same(_name, _length, "in_newline", 10u)) )
    {
        return d_internal_path_list(_env, &edge->inputs, 0u, plain,
                                    (_length == 2u) ? ' ' : '\n', _out);
    }
    else if (d_internal_same(_name, _length, "out", 3u))
    {
        return d_internal_path_list(_env, &edge->outputs, 0u,
                                    edge->outputs.count -
                                        edge->implicit_outputs,
                                    ' ', _out);
    }

    // a name being evaluated already is a cycle
    if ( (_env->recursive) &&
         (d_internal_cycle(_env, _name, _length)) )
    {
        return false;
    }

    const bool has    = d_internal_rule_value(graph, edge->rule, _name,
                                              _length, &value);
    const bool record = (_env->recursive) && (has);

    // a nesting too deep to keep is refused, as no cycle could be seen
    if ( (record) &&
         (_env->depth >= (sizeof(_env->stack) / sizeof(_env->stack[0]))) )
    {
        (void)snprintf(_env->problem, sizeof(_env->problem),
                       "rule variables nested too deeply");
        _env->failed = true;

        return false;
    }

    // the name kept while its value is evaluated
    if (record)
    {
        _env->stack[_env->depth]   = _name;
        _env->lengths[_env->depth] = _length;
        _env->depth++;
    }

    _env->recursive = true;

    const bool ok = d_internal_fallback(_env, _name, _length, has, value,
                                        _out);

    // and let go once it is
    if (record)
    {
        _env->depth--;
    }

    return ok;
}


/*
d_internal_edge_binding
  An edge's binding evaluated: the text, for the caller to free, or NULL
with the reason in `_problem`.
*/
static char*
d_internal_edge_binding(
    const struct d_ninja_graph* _graph,
    size_t                      _edge,
    const char*                 _name,
    bool                        _escape,
    char*                       _problem,
    size_t                      _capacity
)
{
    struct d_internal_edge_env env;
    struct d_internal_buffer   out = { NULL, 0u, 0u, false };

    memset(&env, 0, sizeof(env));
    env.graph  = _graph;
    env.edge   = _edge;
    env.escape = _escape;

    // the lookup, as GetBinding begins one
    if (!d_internal_edge_lookup(&env, _name, strlen(_name), &out))
    {
        free(out.text);
        (void)snprintf(_problem, _capacity, "%s",
                       (env.failed) ? env.problem : "out of memory");

        return NULL;
    }

    return d_internal_take(&out);
}


//==============================================================================
// 6.  VERSIONS
//==============================================================================


/*
d_internal_parse_version
  Ninja's ParseVersion: the major number to the first `.`, and the minor to
the next.
*/
static void
d_internal_parse_version(
    const char* _version,
    int*        _out_major,
    int*        _out_minor
)
{
    const char* const dot = strchr(_version, '.');

    *_out_major = atoi(_version);
    *_out_minor = (dot) ? atoi(&dot[1]) : 0;

    return;
}


/*
d_internal_at_least
  Whether the ninja acted as is a version or later.
*/
static bool
d_internal_at_least(
    const struct d_ninja_graph* _graph,
    int                         _major,
    int                         _minor
)
{
    return ( (_graph->major > _major) ||
             ( (_graph->major == _major) && (_graph->minor >= _minor) ) );
}


/*
d_internal_check_version
  Ninja's CheckNinjaVersion: a build that requires a later ninja is
refused; one written for an older major version is warned of.
*/
static bool
d_internal_check_version(
    struct d_ninja_graph* _graph,
    const char*           _required
)
{
    int  major = 0;
    int  minor = 0;
    char message[384];

    d_internal_parse_version(_required, &major, &minor);

    // a table of the three: older major, newer, and the same
    if (_graph->major > major)
    {
        (void)snprintf(message, sizeof(message),
                       "ninja executable version (%s) greater than build "
                       "file ninja_required_version (%s); versions may be "
                       "incompatible.", _graph->version, _required);

        return d_internal_warn(_graph, message);
    }
    else if ( ( (_graph->major == major) && (_graph->minor < minor) ) ||
              (_graph->major < major) )
    {
        (void)snprintf(message, sizeof(message),
                       "ninja version (%s) incompatible with build file "
                       "ninja_required_version version (%s).",
                       _graph->version, _required);

        return d_internal_fail(_graph, message);
    }

    return true;
}


/*
d_internal_depth
  A pool's depth as the version reads it: atol's reading before 1.14, and
from_chars's, the whole text a number, from 1.14; -1 for one refused.
*/
static int
d_internal_depth(
    const struct d_ninja_graph* _graph,
    const char*                 _text
)
{
    // before 1.14, as atol reads it, narrowed to an int
    if (!d_internal_at_least(_graph, 1, 14))
    {
        return (int)atol(_text);
    }

    const bool negative = (_text[0] == '-');
    size_t     at       = (negative) ? 1u : 0u;
    long long  depth    = 0;

    // a number needs a digit
    if ( (_text[at] < '0') ||
         (_text[at] > '9') )
    {
        return -1;
    }

    // each digit, within an int
    while ( (_text[at] >= '0') &&
            (_text[at] <= '9') )
    {
        depth = (depth * 10) + (_text[at] - '0');

        // one past an int is refused
        if (depth > (long long)INT_MAX + 1)
        {
            return -1;
        }

        at++;
    }

    // the whole text, an int, and not below none: "-0" is none
    if ( (_text[at] != '\0') ||
         ( (negative) && (depth != 0) ) ||
         (depth > INT_MAX) )
    {
        return -1;
    }

    return (int)depth;
}


//==============================================================================
// 7.  STATEMENTS
//==============================================================================


static bool d_internal_load_file(struct d_ninja_graph* _graph,
                                 const char*           _path,
                                 char*                 _text,
                                 size_t                _length,
                                 size_t                _env);


/*
d_internal_canonical
  A path evaluated and made canonical; NULL, the build failed, for one
that comes to nothing or memory that ran out.
*/
static char*
d_internal_canonical(
    struct d_ninja_graph* _graph,
    size_t                _file,
    size_t                _value,
    size_t                _env,
    size_t                _line
)
{
    char* const path = d_internal_evaluate(_graph, _file, _value, _env);

    // memory that ran out
    if (!path)
    {
        (void)d_internal_fail(_graph, "out of memory");

        return NULL;
    }

    // a path of nothing
    if (path[0] == '\0')
    {
        free(path);
        (void)d_internal_fail_at(_graph, _file, _line, "empty path");

        return NULL;
    }

    const size_t length = strlen(path);
    char* const  out    = malloc(length + 3u);

    // the canonical form
    if (out)
    {
        (void)d_ninja_canonical(path, length, _graph->win32, out);
    }
    else
    {
        (void)d_internal_fail(_graph, "out of memory");
    }

    free(path);

    return out;
}


/*
d_internal_variable
  A variable bound where it stands, a required version checked first.
*/
static bool
d_internal_variable(
    struct d_ninja_graph*           _graph,
    size_t                          _file,
    const struct d_ninja_statement* _statement,
    size_t                          _env
)
{
    const struct d_ninja* const ninja = &_graph->files[_file].records;
    const char* const           name  = &ninja->text[_statement->name_start];
    const size_t                span  = _statement->name_end -
                                        _statement->name_start;
    char* const                 value = d_internal_evaluate(_graph, _file,
                                                            _statement->value,
                                                            _env);

    // the version a build requires, before anything else
    if ( (value) &&
         (d_internal_same(name, span, "ninja_required_version", 22u)) &&
         (!d_internal_check_version(_graph, value)) )
    {
        free(value);

        return false;
    }

    return d_internal_bind(_graph, _env, name, span, value);
}


/*
d_internal_rule
  A rule, defined once in its scope.
*/
static bool
d_internal_rule(
    struct d_ninja_graph*           _graph,
    size_t                          _file,
    size_t                          _index,
    size_t                          _env
)
{
    const struct d_ninja* const           ninja     =
        &_graph->files[_file].records;
    const struct d_ninja_statement* const statement =
        &ninja->statements[_index];
    const char* const                     name      =
        &ninja->text[statement->name_start];
    const size_t                          span      = statement->name_end -
                                                      statement->name_start;
    char                                  message[256];

    // a rule its scope has already
    if (d_internal_rule_in(_graph, _env, name, span) != D_NINJA_NONE)
    {
        (void)snprintf(message, sizeof(message), "duplicate rule '%.*s'",
                       (int)span, name);

        return d_internal_fail_at(_graph, _file,
                                  d_ninja_line_of(ninja,
                                                  statement->header_end - 1u),
                                  message);
    }

    return d_internal_add_rule(_graph, _env, name, span, _file, _index);
}


/*
d_internal_pool
  A pool, defined once, its depth read from each binding as it comes.
*/
static bool
d_internal_pool(
    struct d_ninja_graph*           _graph,
    size_t                          _file,
    const struct d_ninja_statement* _statement,
    size_t                          _env
)
{
    const struct d_ninja* const ninja = &_graph->files[_file].records;
    const char* const           name  = &ninja->text[_statement->name_start];
    const size_t                span  = _statement->name_end -
                                        _statement->name_start;
    int                         depth = -1;
    char                        message[256];

    // a pool already
    if (d_internal_find_pool(_graph, name, span) != D_NINJA_NONE)
    {
        (void)snprintf(message, sizeof(message), "duplicate pool '%.*s'",
                       (int)span, name);

        return d_internal_fail_at(_graph, _file,
                                  d_ninja_line_of(ninja,
                                                  _statement->header_end - 1u),
                                  message);
    }

    // each depth, as it is read
    for (size_t at = 0; at < _statement->binding_count; ++at)
    {
        const struct d_ninja_binding* const binding =
            &ninja->bindings[_statement->first_binding + at];
        char* const text = d_internal_evaluate(_graph, _file, binding->value,
                                               _env);

        // a value that could not be made
        if (!text)
        {
            return d_internal_fail(_graph, "out of memory");
        }

        depth = d_internal_depth(_graph, text);
        free(text);

        // a depth refused, where ninja reads it
        if (depth < 0)
        {
            return d_internal_fail_at(_graph, _file, binding->end_line,
                                      "invalid pool depth");
        }
    }

    return d_internal_add_pool(_graph, name, span, depth);
}


/*
d_internal_default
  Each target of a default, a node the build names already.
*/
static bool
d_internal_default(
    struct d_ninja_graph*           _graph,
    size_t                          _file,
    const struct d_ninja_statement* _statement,
    size_t                          _env
)
{
    const struct d_ninja* const ninja = &_graph->files[_file].records;
    char                        message[512];

    // each target, as it is read
    for (size_t at = 0; at < _statement->path_count; ++at)
    {
        const size_t value = ninja->paths[_statement->first_path + at].value;
        const size_t line  = d_ninja_line_of(ninja, ninja->values[value].end);
        char* const  path  = d_internal_canonical(_graph, _file, value, _env,
                                                  line);

        // a path of nothing
        if (!path)
        {
            return false;
        }

        const size_t node = d_internal_lookup_node(_graph, path,
                                                   strlen(path));

        // a target no build names
        if (node == D_NINJA_NONE)
        {
            (void)snprintf(message, sizeof(message), "unknown target '%s'",
                           path);
            free(path);

            return d_internal_fail_at(_graph, _file, line, message);
        }

        free(path);

        // the default kept
        if (!d_internal_push(&_graph->defaults, node))
        {
            return d_internal_fail(_graph, "out of memory");
        }
    }

    return true;
}


/*
d_internal_read_file
  A file's bytes, from the directory ninja runs in, terminated; NULL, the
reason given as strerror gives it, when it cannot be read.
*/
static char*
d_internal_read_file(
    const struct d_ninja_graph* _graph,
    const char*                 _path,
    size_t*                     _out_length,
    const char**                _out_why
)
{
    const bool   relative = ( (_graph->directory) &&
                              (_graph->directory[0] != '\0') &&
                              (_path[0] != '/') );
    const size_t span     = strlen(_path) +
                            ( (relative) ? (strlen(_graph->directory) + 2u)
                                         : 1u );
    char* const  full     = malloc(span);

    *_out_why = "out of memory";

    // the path from the directory
    if (!full)
    {
        return NULL;
    }

    (void)snprintf(full, span, "%s%s%s", (relative) ? _graph->directory : "",
                   (relative) ? "/" : "", _path);

    FILE* const file = (_path[0] != '\0') ? fopen(full, "rb") : NULL;

    *_out_why = strerror((_path[0] != '\0') ? errno : ENOENT);
    free(full);

    // a file that does not open
    if (!file)
    {
        return NULL;
    }

    struct d_internal_buffer text = { NULL, 0u, 0u, false };
    char                     block[8192];
    size_t                   got  = 0;

    // each block, to the end
    while ( (got = fread(block, 1u, sizeof(block), file)) > 0u )
    {
        (void)d_internal_add(&text, block, got);
    }

    const bool failed = (ferror(file) != 0);
    const int  error  = errno;

    (void)fclose(file);
    *_out_length = text.length;
    *_out_why    = (failed) ? strerror(error) : "out of memory";

    // a read that failed, a directory's among them, reads nothing
    if (failed)
    {
        free(text.text);

        return NULL;
    }

    return d_internal_take(&text);
}


/*
d_internal_include
  An include, in the scope it stands in, or a subninja, in a scope of its
own within it; a file that cannot be read is reported at the statement.
*/
static bool
d_internal_include(
    struct d_ninja_graph*           _graph,
    size_t                          _file,
    const struct d_ninja_statement* _statement,
    size_t                          _env
)
{
    const struct d_ninja* const ninja = &_graph->files[_file].records;
    const size_t                value = _statement->value;
    const size_t                line  = d_ninja_line_of(ninja,
                                                        ninja->values[value]
                                                            .end);
    char* const                 path  = d_internal_evaluate(_graph, _file,
                                                            value, _env);
    size_t                      length = 0;
    const char*                 why    = NULL;
    char                        message[640];

    // a path that could not be made
    if (!path)
    {
        return d_internal_fail(_graph, "out of memory");
    }

    char* const text = d_internal_read_file(_graph, path, &length, &why);

    // a file that cannot be read is the including statement's to report
    if (!text)
    {
        (void)snprintf(message, sizeof(message), "loading '%s': %s", path,
                       why);
        free(path);

        return d_internal_fail_at(_graph, _file, line, message);
    }

    const size_t env = (_statement->kind == D_NINJA_SUBNINJA)
                           ? d_internal_new_env(_graph, _env) : _env;
    const bool   ok  = (env != D_NINJA_NONE) &&
                       (d_internal_load_file(_graph, path, text, length, env));

    free(path);

    return ok;
}


// 7.1    Builds
//------------------------------------------------------------------------------
/*
d_internal_new_edge
  An edge of a rule, in a scope: the build's own, when it has bindings,
each evaluated in the file's scope as ninja evaluates them.
*/
static size_t
d_internal_new_edge(
    struct d_ninja_graph*           _graph,
    size_t                          _file,
    size_t                          _index,
    size_t                          _rule,
    size_t                          _env
)
{
    const struct d_ninja* const           ninja     =
        &_graph->files[_file].records;
    const struct d_ninja_statement* const statement =
        &ninja->statements[_index];
    size_t                                env       = _env;

    // a scope of its own for a build with bindings
    if (statement->binding_count > 0u)
    {
        env = d_internal_new_env(_graph, _env);
    }

    // each binding, evaluated in the file's scope
    for (size_t at = 0;
         (env != D_NINJA_NONE) && (at < statement->binding_count);
         ++at)
    {
        const struct d_ninja_binding* const binding =
            &ninja->bindings[statement->first_binding + at];

        // the binding kept
        if (!d_internal_bind(_graph, env, &ninja->text[binding->name_start],
                             binding->name_end - binding->name_start,
                             d_internal_evaluate(_graph, _file,
                                                 binding->value, _env)))
        {
            return D_NINJA_NONE;
        }
    }

    // a scope, and room for the edge
    if ( (env == D_NINJA_NONE) ||
         (!d_internal_grow((void**)&_graph->edges, &_graph->edge_capacity,
                           _graph->edge_count, sizeof(*_graph->edges))) )
    {
        (void)d_internal_fail(_graph, "out of memory");

        return D_NINJA_NONE;
    }

    struct d_internal_edge* const edge = &_graph->edges[_graph->edge_count];

    memset(edge, 0, sizeof(*edge));
    edge->rule      = _rule;
    edge->env       = env;
    edge->pool      = 0u;
    edge->file      = _file;
    edge->statement = _index;

    return _graph->edge_count++;
}


/*
d_internal_edge_pool
  An edge's pool, by the binding its rule or its scope gives, evaluated as
ninja evaluates it, before the edge has a path.
*/
static bool
d_internal_edge_pool(
    struct d_ninja_graph* _graph,
    size_t                _edge,
    size_t                _line
)
{
    char        why[512];
    char        message[640];
    char* const name = d_internal_edge_binding(_graph, _edge, "pool", true,
                                               why, sizeof(why));

    // a cycle among the rule's bindings
    if (!name)
    {
        return d_internal_fail(_graph, why);
    }

    // the default pool, or a pool defined
    if (name[0] != '\0')
    {
        const size_t pool = d_internal_find_pool(_graph, name, strlen(name));

        // a pool not defined
        if (pool == D_NINJA_NONE)
        {
            (void)snprintf(message, sizeof(message),
                           "unknown pool name '%s'", name);
            free(name);

            return d_internal_fail_at(_graph, _graph->edges[_edge].file,
                                      _line, message);
        }

        _graph->edges[_edge].pool = pool;
    }

    free(name);

    return true;
}


/*
d_internal_edge_paths
  An edge's paths of the sections `_first` to `_last`, each evaluated in
the edge's scope, made canonical and made a node: as an output, the edge
its in-edge; as an input, a validation, the edge one of its out-edges.
  Long as it is a table of what a path is to the edge.
*/
static bool
d_internal_edge_paths(
    struct d_ninja_graph*           _graph,
    size_t                          _edge,
    const struct d_ninja_statement* _statement,
    enum d_ninja_section            _first,
    enum d_ninja_section            _last,
    size_t                          _line
)
{
    struct d_internal_edge* const edge  = &_graph->edges[_edge];
    const struct d_ninja* const   ninja = &_graph->files[edge->file].records;
    char                          message[640];

    // each path of the sections, in order
    for (size_t at = 0; at < _statement->path_count; ++at)
    {
        const struct d_ninja_path* const written =
            &ninja->paths[_statement->first_path + at];

        // one of another section
        if ( (written->section < _first) ||
             (written->section > _last) )
        {
            continue;
        }

        char* const path = d_internal_canonical(_graph, edge->file,
                                                written->value, edge->env,
                                                _line);

        // a path of nothing
        if (!path)
        {
            return false;
        }

        const size_t node = d_internal_get_node(_graph, path, strlen(path));

        // a node that could not be made
        if (node == D_NINJA_NONE)
        {
            free(path);

            return false;
        }

        struct d_internal_node* const target = &_graph->nodes[node];

        // a table of what the path is: an output, made once; an input; a
        // validation
        if ( (written->section == D_NINJA_OUTPUT) ||
             (written->section == D_NINJA_IMPLICIT_OUTPUT) )
        {
            // an output another edge, or this one, makes already
            if (target->in_edge != D_NINJA_NONE)
            {
                (void)snprintf(message, sizeof(message),
                               ( (target->in_edge == _edge) &&
                                 (d_internal_at_least(_graph, 1, 12)) )
                                   ? "%s is defined as an output multiple "
                                     "times"
                                   : "multiple rules generate %s",
                               path);
                free(path);

                return d_internal_fail_at(_graph, edge->file, _line,
                                          message);
            }

            target->in_edge         = _edge;
            edge->implicit_outputs += (written->section ==
                                       D_NINJA_IMPLICIT_OUTPUT) ? 1u : 0u;
            (void)d_internal_push(&edge->outputs, node);
        }
        else if (written->section == D_NINJA_VALIDATION)
        {
            (void)d_internal_push(&edge->validations, node);
            (void)d_internal_push(&target->validation_edges, _edge);
        }
        else
        {
            edge->implicit   += (written->section == D_NINJA_IMPLICIT)
                                    ? 1u : 0u;
            edge->order_only += (written->section == D_NINJA_ORDER_ONLY)
                                    ? 1u : 0u;
            (void)d_internal_push(&edge->inputs, node);
            (void)d_internal_push(&target->out_edges, _edge);
        }

        free(path);
    }

    return true;
}


/*
d_internal_phony_cycle
  A phony edge of one output that names itself among its inputs, as old
CMakes wrote them: the self-reference dropped, with ninja's warning; from
1.14 the node's out-edge goes too.  The sections' counts are left as they
were, as ninja leaves them.
*/
static bool
d_internal_phony_cycle(
    struct d_ninja_graph* _graph,
    size_t                _edge
)
{
    struct d_internal_edge* const edge = &_graph->edges[_edge];
    char                          message[640];

    // only the form old CMakes wrote: from 1.14, with no order-only input
    if ( (!_graph->rules[edge->rule].phony) ||
         (edge->outputs.count != 1u) ||
         (edge->implicit_outputs != 0u) ||
         (edge->implicit != 0u) ||
         ( (edge->order_only != 0u) &&
           (d_internal_at_least(_graph, 1, 14)) ) )
    {
        return true;
    }

    const size_t output = edge->outputs.items[0];
    size_t       kept   = 0;

    // each input but the output itself
    for (size_t at = 0; at < edge->inputs.count; ++at)
    {
        // the output dropped
        if (edge->inputs.items[at] != output)
        {
            edge->inputs.items[kept++] = edge->inputs.items[at];
        }
    }

    // nothing dropped, nothing said
    if (kept == edge->inputs.count)
    {
        return true;
    }

    edge->inputs.count = kept;

    struct d_internal_list* const outs = &_graph->nodes[output].out_edges;

    // from 1.14, the output's out-edge goes with it
    for (size_t at = 0, left = 0;
         (d_internal_at_least(_graph, 1, 14)) && (at <= outs->count);
         ++at)
    {
        // the list closed up, the edge left out
        if (at == outs->count)
        {
            outs->count = left;
        }
        else if (outs->items[at] != _edge)
        {
            outs->items[left++] = outs->items[at];
        }
    }

    (void)snprintf(message, sizeof(message),
                   "phony target '%s' names itself as an input; ignoring "
                   "[-w phonycycle=warn]", _graph->nodes[output].path);

    return d_internal_warn(_graph, message);
}


/*
d_internal_dyndep
  An edge's dyndep, evaluated unescaped, made canonical and made a node,
which must be among its inputs.
*/
static bool
d_internal_dyndep(
    struct d_ninja_graph* _graph,
    size_t                _edge,
    size_t                _line
)
{
    char        why[512];
    char        message[640];
    char* const path = d_internal_edge_binding(_graph, _edge, "dyndep",
                                               false, why, sizeof(why));

    // a cycle among the rule's bindings
    if (!path)
    {
        return d_internal_fail(_graph, why);
    }

    // none is none
    if (path[0] == '\0')
    {
        free(path);

        return true;
    }

    const size_t length    = strlen(path);
    char* const  canonical = malloc(length + 3u);

    // the canonical form
    if (!canonical)
    {
        free(path);

        return d_internal_fail(_graph, "out of memory");
    }

    (void)d_ninja_canonical(path, length, _graph->win32, canonical);
    free(path);

    const size_t                        node  =
        d_internal_get_node(_graph, canonical, strlen(canonical));
    const struct d_internal_edge* const edge  = &_graph->edges[_edge];
    bool                                input = false;

    // among the inputs
    for (size_t at = 0; at < edge->inputs.count; ++at)
    {
        input = (input) || (edge->inputs.items[at] == node);
    }

    // a dyndep the edge does not take
    if ( (node != D_NINJA_NONE) &&
         (!input) )
    {
        (void)snprintf(message, sizeof(message), "dyndep '%s' is not an input",
                       canonical);
        free(canonical);

        return d_internal_fail_at(_graph, edge->file, _line, message);
    }

    free(canonical);

    return (node != D_NINJA_NONE);
}


/*
d_internal_build
  A build, taken as ninja's ParseEdge takes one: its rule, its scope, its
pool, its outputs, its inputs and validations, a phony cycle, its dyndep.
*/
static bool
d_internal_build(
    struct d_ninja_graph*           _graph,
    size_t                          _file,
    size_t                          _index,
    size_t                          _env
)
{
    const struct d_ninja* const           ninja     =
        &_graph->files[_file].records;
    const struct d_ninja_statement* const statement =
        &ninja->statements[_index];
    const char* const                     name      =
        &ninja->text[statement->name_start];
    const size_t                          span      = statement->name_end -
                                                      statement->name_start;
    const size_t                          rule      =
        d_internal_find_rule(_graph, _env, name, span);
    const size_t                          line      = statement->next_line;
    char                                  message[256];

    // the rule, by its name, where ninja reads the name
    if (rule == D_NINJA_NONE)
    {
        (void)snprintf(message, sizeof(message),
                       "unknown build rule '%.*s'", (int)span, name);

        return d_internal_fail_at(_graph, _file,
                                  d_ninja_line_of(ninja,
                                                  statement->name_start),
                                  message);
    }

    const size_t edge = d_internal_new_edge(_graph, _file, _index, rule,
                                            _env);

    return ( (edge != D_NINJA_NONE) &&
             (d_internal_edge_pool(_graph, edge, line)) &&
             (d_internal_edge_paths(_graph, edge, statement, D_NINJA_OUTPUT,
                                    D_NINJA_IMPLICIT_OUTPUT, line)) &&
             (d_internal_edge_paths(_graph, edge, statement, D_NINJA_INPUT,
                                    D_NINJA_VALIDATION, line)) &&
             (d_internal_phony_cycle(_graph, edge)) &&
             (d_internal_dyndep(_graph, edge, line)) );
}


/*
d_internal_statement
  One statement, by its kind.
*/
static bool
d_internal_statement(
    struct d_ninja_graph* _graph,
    size_t                _file,
    size_t                _index,
    size_t                _env
)
{
    const struct d_ninja_statement* const statement =
        &_graph->files[_file].records.statements[_index];

    // a table of the kinds
    switch (statement->kind)
    {
        case D_NINJA_VARIABLE:
            return d_internal_variable(_graph, _file, statement, _env);

        case D_NINJA_RULE:
            return d_internal_rule(_graph, _file, _index, _env);

        case D_NINJA_BUILD:
            return d_internal_build(_graph, _file, _index, _env);

        case D_NINJA_POOL:
            return d_internal_pool(_graph, _file, statement, _env);

        case D_NINJA_DEFAULT:
            return d_internal_default(_graph, _file, statement, _env);

        case D_NINJA_INCLUDE:
        case D_NINJA_SUBNINJA:
        default:
            break;
    }

    return d_internal_include(_graph, _file, statement, _env);
}


/*
d_internal_partial
  A statement cut short, taken as far as ninja would have taken it before
it failed: a build's rule, once named; a default's targets, each as read; an
include's file, once read; a rule's or a pool's name, once its line ended,
and a pool's depths as read.  True when nothing it holds is refused.
*/
static bool
d_internal_partial(
    struct d_ninja_graph* _graph,
    size_t                _file,
    size_t                _env
)
{
    struct d_ninja* const           ninja     = &_graph->files[_file].records;
    const size_t                    index     = ninja->statement_count;
    const struct d_ninja_statement* statement = &ninja->statements[index];
    char                            message[256];

    // a table of what each kind does before it is whole
    if ( (statement->kind == D_NINJA_BUILD) &&
         (statement->name_end > statement->name_start) &&
         (d_internal_find_rule(_graph, _env,
                               &ninja->text[statement->name_start],
                               statement->name_end -
                                   statement->name_start) == D_NINJA_NONE) )
    {
        (void)snprintf(message, sizeof(message),
                       "unknown build rule '%.*s'",
                       (int)(statement->name_end - statement->name_start),
                       &ninja->text[statement->name_start]);

        return d_internal_fail_at(_graph, _file,
                                  d_ninja_line_of(ninja,
                                                  statement->name_start),
                                  message);
    }
    else if (statement->kind == D_NINJA_DEFAULT)
    {
        return d_internal_default(_graph, _file, statement, _env);
    }
    else if ( ( (statement->kind == D_NINJA_INCLUDE) ||
                (statement->kind == D_NINJA_SUBNINJA) ) &&
              (statement->value != D_NINJA_NONE) )
    {
        return d_internal_include(_graph, _file, statement, _env);
    }
    else if ( (statement->kind == D_NINJA_RULE) &&
              (statement->header_end > 0u) )
    {
        return d_internal_rule(_graph, _file, index, _env);
    }
    else if ( (statement->kind == D_NINJA_POOL) &&
              (statement->header_end > 0u) )
    {
        return d_internal_pool(_graph, _file, statement, _env);
    }

    return true;
}


/*
d_internal_load_file
  A file read and taken, statement by statement, in a scope; the file is
kept, its text and path taken.  A refusal dninja made is reported once
what came before it has been taken.
*/
static bool
d_internal_load_file(
    struct d_ninja_graph* _graph,
    const char*           _path,
    char*                 _text,
    size_t                _length,
    size_t                _env
)
{
    char* const path = d_internal_copy(_path, strlen(_path));

    // room for the file
    if ( (!path) ||
         (!d_internal_grow((void**)&_graph->files, &_graph->file_capacity,
                           _graph->file_count, sizeof(*_graph->files))) )
    {
        free(path);
        free(_text);

        return d_internal_fail(_graph, "out of memory");
    }

    const size_t                  index = _graph->file_count++;
    struct d_internal_file* const file  = &_graph->files[index];

    file->path   = path;
    file->text   = _text;
    file->length = _length;

    const bool read = d_ninja_read(&file->records, _text, _length, NULL, 0u);
    const size_t count = _graph->files[index].records.statement_count;

    // each statement read, in order; the arrays may move as files are
    // added, so the file is found by its index each time
    for (size_t at = 0; at < count; ++at)
    {
        // one refused ends the loading
        if (!d_internal_statement(_graph, index, at, _env))
        {
            return false;
        }
    }

    // a file read whole is done
    if (read)
    {
        return true;
    }

    // what came before the refusal, then the refusal, in the version's words
    if ( (_graph->files[index].records.partial) &&
         (!d_internal_partial(_graph, index, _env)) )
    {
        return false;
    }

    const struct d_ninja* const records = &_graph->files[index].records;
    const bool                  newline =
        (strstr(records->error, "using $^ escape") != NULL);

    return d_internal_fail_at(_graph, index, records->error_line,
                              ( (newline) &&
                                (!d_internal_at_least(_graph, 1, 14)) )
                                  ? "bad $-escape (literal $ must be written "
                                    "as $$)"
                                  : records->error);
}


//==============================================================================
// 8.  THE BUILD
//==============================================================================


/*
d_internal_finish
  What a loaded build gives besides its records: its roots, and the top
scope's rules in order.
*/
static bool
d_internal_finish(
    struct d_ninja_graph* _graph
)
{
    // each output that no edge takes as an input, in edge order
    for (size_t at = 0; at < _graph->edge_count; ++at)
    {
        const struct d_internal_edge* const edge = &_graph->edges[at];

        // each output
        for (size_t out = 0; out < edge->outputs.count; ++out)
        {
            const size_t node = edge->outputs.items[out];

            // a root, kept
            if ( (_graph->nodes[node].out_edges.count == 0u) &&
                 (!d_internal_push(&_graph->roots, node)) )
            {
                return d_internal_fail(_graph, "out of memory");
            }
        }
    }

    const struct d_internal_env* const top   = &_graph->envs[0];
    struct d_internal_list* const      rules = &_graph->top_rules;

    // the top scope's rules, each put in order by its name's bytes, as a
    // std::map orders them
    for (size_t at = 0; at < top->rule_count; ++at)
    {
        const size_t rule  = top->rules[at];
        size_t       place = rules->count;

        // room for one more
        if (!d_internal_push(rules, rule))
        {
            return d_internal_fail(_graph, "out of memory");
        }

        // past each that sorts after it
        while ( (place > 0u) &&
                (strcmp(_graph->rules[rules->items[place - 1u]].name,
                        _graph->rules[rule].name) > 0) )
        {
            rules->items[place] = rules->items[place - 1u];
            place--;
        }

        rules->items[place] = rule;
    }

    return true;
}


/*
d_internal_begin
  A build with nothing loaded: the top scope, phony, and the two pools
ninja defines.
*/
static struct d_ninja_graph*
d_internal_begin(
    const struct d_ninja_options* _options
)
{
    struct d_ninja_graph* const graph = calloc(1u, sizeof(*graph));

    // a build needs room of its own
    if (!graph)
    {
        return NULL;
    }

    const char* const version = ( (_options) && (_options->version) )
                                    ? _options->version : D_NINJA_VERSION;

    (void)snprintf(graph->version, sizeof(graph->version), "%s", version);
    d_internal_parse_version(graph->version, &graph->major, &graph->minor);
    graph->win32     = (_options) && (_options->win32);
    graph->directory = ( (_options) && (_options->directory) )
                           ? d_internal_copy(_options->directory,
                                             strlen(_options->directory))
                           : NULL;

    // the top scope, phony in it, and the pools: "" and console
    if ( ( (_options) && (_options->directory) && (!graph->directory) ) ||
         (d_internal_new_env(graph, D_NINJA_NONE) == D_NINJA_NONE) ||
         (!d_internal_add_rule(graph, 0u, "phony", 5u, D_NINJA_NONE, 0u)) ||
         (!d_internal_add_pool(graph, "", 0u, 0)) ||
         (!d_internal_add_pool(graph, "console", 7u, 1)) )
    {
        d_ninja_graph_free(graph);

        return NULL;
    }

    return graph;
}


/*
d_internal_loaded
  The build, once loaded whole, or NULL with ninja's reason.
*/
static struct d_ninja_graph*
d_internal_loaded(
    struct d_ninja_graph* _graph,
    bool                  _ok,
    char*                 _problem,
    size_t                _capacity
)
{
    // a build ninja would not load
    if ( (!_ok) ||
         (!d_internal_finish(_graph)) )
    {
        (void)snprintf(_problem, _capacity, "%s", _graph->problem);
        d_ninja_graph_free(_graph);

        return NULL;
    }

    return _graph;
}


struct d_ninja_graph*
d_ninja_load_text(
    const char*                   _path,
    const char*                   _text,
    size_t                        _length,
    const struct d_ninja_options* _options,
    char*                         _problem,
    size_t                        _capacity
)
{
    struct d_ninja_graph* const graph = d_internal_begin(_options);
    char* const                 text  = d_internal_copy((_text) ? _text : "",
                                                        (_text) ? _length
                                                                : 0u);

    // a build, and its text
    if ( (!graph) ||
         (!text) )
    {
        d_ninja_graph_free(graph);
        free(text);
        (void)snprintf(_problem, _capacity, "out of memory");

        return NULL;
    }

    return d_internal_loaded(graph,
                             d_internal_load_file(graph,
                                                  (_path) ? _path : "",
                                                  text,
                                                  (_text) ? _length : 0u,
                                                  0u),
                             _problem, _capacity);
}


struct d_ninja_graph*
d_ninja_load(
    const char*                   _path,
    const struct d_ninja_options* _options,
    char*                         _problem,
    size_t                        _capacity
)
{
    struct d_ninja_graph* const graph  = d_internal_begin(_options);
    size_t                      length = 0;
    const char*                 why    = NULL;

    // a build, and a manifest named
    if ( (!graph) ||
         (!_path) )
    {
        d_ninja_graph_free(graph);
        (void)snprintf(_problem, _capacity, "%s",
                       (graph) ? "no manifest is named" : "out of memory");

        return NULL;
    }

    char* const text = d_internal_read_file(graph, _path, &length, &why);

    // a manifest that cannot be read, said as ninja says it
    if (!text)
    {
        (void)snprintf(_problem, _capacity, "loading '%s': %s", _path, why);
        d_ninja_graph_free(graph);

        return NULL;
    }

    return d_internal_loaded(graph,
                             d_internal_load_file(graph, _path, text, length,
                                                  0u),
                             _problem, _capacity);
}


/*
d_ninja_graph_free
  Long as each of the build's arrays, and what each item of them owns, is
freed in turn.
*/
void
d_ninja_graph_free(
    struct d_ninja_graph* _graph
)
{
    // freeing nothing is allowed
    if (!_graph)
    {
        return;
    }

    // each file, its text and its records
    for (size_t at = 0; at < _graph->file_count; ++at)
    {
        d_ninja_release(&_graph->files[at].records);
        free(_graph->files[at].text);
        free(_graph->files[at].path);
    }

    // each scope, its bindings and its rules
    for (size_t at = 0; at < _graph->env_count; ++at)
    {
        // each binding
        for (size_t k = 0; k < _graph->envs[at].binding_count; ++k)
        {
            free(_graph->envs[at].bindings[k].name);
            free(_graph->envs[at].bindings[k].value);
        }

        free(_graph->envs[at].bindings);
        free(_graph->envs[at].rules);
    }

    // each rule's and each pool's name
    for (size_t at = 0; at < _graph->rule_count; ++at)
    {
        free(_graph->rules[at].name);
    }

    // ...and each pool's
    for (size_t at = 0; at < _graph->pool_count; ++at)
    {
        free(_graph->pools[at].name);
    }

    // each edge's lists
    for (size_t at = 0; at < _graph->edge_count; ++at)
    {
        free(_graph->edges[at].outputs.items);
        free(_graph->edges[at].inputs.items);
        free(_graph->edges[at].validations.items);
    }

    // each node's path and lists
    for (size_t at = 0; at < _graph->node_count; ++at)
    {
        free(_graph->nodes[at].path);
        free(_graph->nodes[at].out_edges.items);
        free(_graph->nodes[at].validation_edges.items);
    }

    // each warning
    for (size_t at = 0; at < _graph->warning_count; ++at)
    {
        free(_graph->warnings[at]);
    }

    free(_graph->files);
    free(_graph->envs);
    free(_graph->rules);
    free(_graph->pools);
    free(_graph->edges);
    free(_graph->nodes);
    free(_graph->table);
    free(_graph->defaults.items);
    free(_graph->roots.items);
    free(_graph->top_rules.items);
    free(_graph->warnings);
    free(_graph->directory);
    free(_graph);

    return;
}


//==============================================================================
// 9.  QUERIES
//==============================================================================


size_t
d_ninja_edge_count(
    const struct d_ninja_graph* _graph
)
{
    return (_graph) ? _graph->edge_count : 0u;
}


const char*
d_ninja_edge_rule(
    const struct d_ninja_graph* _graph,
    size_t                      _edge
)
{
    return ( (_graph) && (_edge < _graph->edge_count) )
               ? _graph->rules[_graph->edges[_edge].rule].name : "";
}


bool
d_ninja_edge_phony(
    const struct d_ninja_graph* _graph,
    size_t                      _edge
)
{
    return ( (_graph) &&
             (_edge < _graph->edge_count) &&
             (_graph->rules[_graph->edges[_edge].rule].phony) );
}


/*
d_ninja_edge_nodes
  The inputs' three sections counted from their end, as ninja's is_implicit
and is_order_only count them; the outputs' two from their start.
  Long as it is a table of the six sections.
*/
size_t
d_ninja_edge_nodes(
    const struct d_ninja_graph* _graph,
    size_t                      _edge,
    enum d_ninja_section        _section,
    const size_t**              _out_nodes
)
{
    // an edge the build has
    if ( (!_graph) ||
         (_edge >= _graph->edge_count) )
    {
        *_out_nodes = NULL;

        return 0u;
    }

    const struct d_internal_edge* const edge   = &_graph->edges[_edge];
    const size_t                        count  = edge->inputs.count;
    const size_t                        order  = (edge->order_only < count)
                                                     ? edge->order_only
                                                     : count;
    const size_t                        hidden =
        ((edge->implicit + order) < count) ? edge->implicit
                                           : (count - order);
    const size_t                        plain  = count - order - hidden;
    const size_t                        outs   = edge->outputs.count -
                                                 edge->implicit_outputs;

    // a table of the sections
    switch (_section)
    {
        case D_NINJA_OUTPUT:
            *_out_nodes = edge->outputs.items;

            return outs;

        case D_NINJA_IMPLICIT_OUTPUT:
            *_out_nodes = (edge->outputs.items) ? &edge->outputs.items[outs]
                                                : NULL;

            return edge->implicit_outputs;

        case D_NINJA_INPUT:
            *_out_nodes = edge->inputs.items;

            return plain;

        case D_NINJA_IMPLICIT:
            *_out_nodes = (edge->inputs.items) ? &edge->inputs.items[plain]
                                               : NULL;

            return hidden;

        case D_NINJA_ORDER_ONLY:
            *_out_nodes = (edge->inputs.items)
                              ? &edge->inputs.items[plain + hidden] : NULL;

            return order;

        case D_NINJA_VALIDATION:
            *_out_nodes = edge->validations.items;

            return edge->validations.count;

        case D_NINJA_TARGET:
        case D_NINJA_FILE:
        default:
            break;
    }

    *_out_nodes = NULL;

    return 0u;
}


const char*
d_ninja_edge_pool(
    const struct d_ninja_graph* _graph,
    size_t                      _edge
)
{
    return ( (_graph) && (_edge < _graph->edge_count) )
               ? _graph->pools[_graph->edges[_edge].pool].name : "";
}


const char*
d_ninja_edge_file(
    const struct d_ninja_graph* _graph,
    size_t                      _edge
)
{
    return ( (_graph) && (_edge < _graph->edge_count) )
               ? _graph->files[_graph->edges[_edge].file].path : "";
}


size_t
d_ninja_edge_line(
    const struct d_ninja_graph* _graph,
    size_t                      _edge
)
{
    // an edge the build has
    if ( (!_graph) ||
         (_edge >= _graph->edge_count) )
    {
        return 0u;
    }

    const struct d_internal_edge* const edge = &_graph->edges[_edge];

    return _graph->files[edge->file].records.statements[edge->statement].line;
}


char*
d_ninja_edge_binding(
    const struct d_ninja_graph* _graph,
    size_t                      _edge,
    const char*                 _name,
    bool                        _escape,
    char*                       _problem,
    size_t                      _capacity
)
{
    // an edge the build has, and a name
    if ( (!_graph) ||
         (!_name) ||
         (_edge >= _graph->edge_count) )
    {
        (void)snprintf(_problem, _capacity, "no such edge");

        return NULL;
    }

    return d_internal_edge_binding(_graph, _edge, _name, _escape, _problem,
                                   _capacity);
}


size_t
d_ninja_node_count(
    const struct d_ninja_graph* _graph
)
{
    return (_graph) ? _graph->node_count : 0u;
}


const char*
d_ninja_node_path(
    const struct d_ninja_graph* _graph,
    size_t                      _node
)
{
    return ( (_graph) && (_node < _graph->node_count) )
               ? _graph->nodes[_node].path : "";
}


size_t
d_ninja_node_in_edge(
    const struct d_ninja_graph* _graph,
    size_t                      _node
)
{
    return ( (_graph) && (_node < _graph->node_count) )
               ? _graph->nodes[_node].in_edge : D_NINJA_NONE;
}


size_t
d_ninja_node_out_edges(
    const struct d_ninja_graph* _graph,
    size_t                      _node,
    const size_t**              _out_edges
)
{
    // a node the build has
    if ( (!_graph) ||
         (_node >= _graph->node_count) )
    {
        *_out_edges = NULL;

        return 0u;
    }

    *_out_edges = _graph->nodes[_node].out_edges.items;

    return _graph->nodes[_node].out_edges.count;
}


size_t
d_ninja_node_validation_edges(
    const struct d_ninja_graph* _graph,
    size_t                      _node,
    const size_t**              _out_edges
)
{
    // a node the build has
    if ( (!_graph) ||
         (_node >= _graph->node_count) )
    {
        *_out_edges = NULL;

        return 0u;
    }

    *_out_edges = _graph->nodes[_node].validation_edges.items;

    return _graph->nodes[_node].validation_edges.count;
}


size_t
d_ninja_node_find(
    const struct d_ninja_graph* _graph,
    const char*                 _path,
    size_t                      _length
)
{
    // a build, and a path
    if ( (!_graph) ||
         (!_path) ||
         (_length == 0u) )
    {
        return D_NINJA_NONE;
    }

    char* const canonical = malloc(_length + 3u);

    // the canonical form
    if (!canonical)
    {
        return D_NINJA_NONE;
    }

    const size_t span  = d_ninja_canonical(_path, _length, _graph->win32,
                                           canonical);
    const size_t found = d_internal_lookup_node(_graph, canonical, span);

    free(canonical);

    return found;
}


size_t
d_ninja_defaults(
    const struct d_ninja_graph* _graph,
    const size_t**              _out_nodes
)
{
    *_out_nodes = (_graph) ? _graph->defaults.items : NULL;

    return (_graph) ? _graph->defaults.count : 0u;
}


size_t
d_ninja_roots(
    const struct d_ninja_graph* _graph,
    const size_t**              _out_nodes
)
{
    *_out_nodes = (_graph) ? _graph->roots.items : NULL;

    return (_graph) ? _graph->roots.count : 0u;
}


size_t
d_ninja_rule_count(
    const struct d_ninja_graph* _graph
)
{
    return (_graph) ? _graph->top_rules.count : 0u;
}


const char*
d_ninja_rule_name(
    const struct d_ninja_graph* _graph,
    size_t                      _rule
)
{
    return ( (_graph) && (_rule < _graph->top_rules.count) )
               ? _graph->rules[_graph->top_rules.items[_rule]].name : "";
}


char*
d_ninja_rule_binding(
    const struct d_ninja_graph* _graph,
    size_t                      _rule,
    const char*                 _name
)
{
    size_t value = 0;

    // a rule of the top scope, and a binding it has
    if ( (!_graph) ||
         (!_name) ||
         (_rule >= _graph->top_rules.count) ||
         (!d_internal_rule_value(_graph, _graph->top_rules.items[_rule],
                                 _name, strlen(_name), &value)) )
    {
        return NULL;
    }

    const struct d_internal_rule* const rule  =
        &_graph->rules[_graph->top_rules.items[_rule]];
    const struct d_ninja* const         ninja =
        &_graph->files[rule->file].records;
    const struct d_ninja_value* const   eval  = &ninja->values[value];
    char* const                         out   =
        malloc(((eval->end - eval->start) * 2u) + 1u);

    // the binding, unparsed
    if (out)
    {
        (void)d_ninja_unparse(ninja, eval, out);
    }

    return out;
}


size_t
d_ninja_pool_count(
    const struct d_ninja_graph* _graph
)
{
    return (_graph) ? _graph->pool_count : 0u;
}


const char*
d_ninja_pool_name(
    const struct d_ninja_graph* _graph,
    size_t                      _pool
)
{
    return ( (_graph) && (_pool < _graph->pool_count) )
               ? _graph->pools[_pool].name : "";
}


int
d_ninja_pool_depth(
    const struct d_ninja_graph* _graph,
    size_t                      _pool
)
{
    return ( (_graph) && (_pool < _graph->pool_count) )
               ? _graph->pools[_pool].depth : 0;
}


char*
d_ninja_variable(
    const struct d_ninja_graph* _graph,
    const char*                 _name
)
{
    const char* const value = ( (_graph) && (_name) )
                                  ? d_internal_lookup(_graph, 0u, _name,
                                                      strlen(_name))
                                  : "";

    return d_internal_copy(value, strlen(value));
}


size_t
d_ninja_file_count(
    const struct d_ninja_graph* _graph
)
{
    return (_graph) ? _graph->file_count : 0u;
}


const char*
d_ninja_file_path(
    const struct d_ninja_graph* _graph,
    size_t                      _file
)
{
    return ( (_graph) && (_file < _graph->file_count) )
               ? _graph->files[_file].path : "";
}


size_t
d_ninja_warning_count(
    const struct d_ninja_graph* _graph
)
{
    return (_graph) ? _graph->warning_count : 0u;
}


const char*
d_ninja_warning(
    const struct d_ninja_graph* _graph,
    size_t                      _warning
)
{
    return ( (_graph) && (_warning < _graph->warning_count) )
               ? _graph->warnings[_warning] : "";
}
