/*******************************************************************************
* djinterp [djinterp]                                              dtoken_node.c
*
* Definitions for the non-inline declarations in dtoken_node.h.
*   Every attribute of a node is computed before the node's first one is
* written, because a node's attributes must be contiguous in the tree's one
* attribute array.  That is why a group reads its closer's position through
* the match array at the moment it is emitted, and a directive scans its own
* line for a macro or header name before it has any children: the facts are
* all known up front, since the tokens and the grouping precede emission.
*   Emission is one forward pass with no recursion.  A token's parent
* precedes it in source order, so the parent's node always exists when the
* child is added.
*
*
* path:      /src/djinterp/tools/dawk/ext/dtoken_node.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.23
*******************************************************************************/
#include "../../../../../inc/djinterp/tools/dawk/ext/dtoken_node.h"  // corresponding header
// std
#include <ctype.h>    // isalnum
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint32_t
#include <stdio.h>    // snprintf
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy, memset, strlen

// djinterp
#include "../../../../../inc/djinterp/parse/syntax/syntax_group.h"  // grouping
#include "../../../../../inc/djinterp/parse/syntax/syntax_item.h"  // items
#include "../../../../../inc/djinterp/parse/c/storage.h"  // d_parse_grow

// the recognizer grows its arrays and has no fixed-storage mode
#if (D_INTERNAL_PARSE_HEAP != 1)
    #error "this file requires D_CFG_PARSE_HEAP, for d_parse_grow"
#endif


//==============================================================================
// 1.  INTERNAL CONSTANTS AND TYPES
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_INTERNAL_NUMBER_MAX
//   constant: room for a decimal attribute value, terminator included.
#define D_INTERNAL_NUMBER_MAX 24

// 1.1.2
// D_INTERNAL_NAME_MAX
//   constant: room for a kind name in its attribute form; the longest is
// `kw-static-assert-reserved` at twenty-five.
#define D_INTERNAL_NAME_MAX 48

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// d_internal_build
//   struct: the state of one build.  `node_of` and `depth` are parallel to
//   the token array, as `parent` and `match` are in the grouping.
struct d_internal_build
{
    struct d_node_tree*        tree;
    const struct d_source*     source;
    struct d_lexer*            lexer;
    struct d_token*            tokens;
    size_t                     count;
    struct d_syntax_groups     groups;
    uint32_t*                  node_of;
    uint32_t*                  depth;
    char*                      spelling;
    uint32_t                   spelling_size;
    struct d_token_node_stats* stats;

    struct d_syntax_items      items;
    uint32_t*                  item_node;   // per item: its node
    uint32_t*                  head_of;     // per token: first item begun there
    uint32_t*                  next_start;  // per item: next begun at its token
    uint8_t*                   declarator;  // per token: a declared name

    uint32_t*                  cond_macro;  // per open conditional: its macro
    uint8_t*                   cond_kind;   // per open conditional: its form
    uint32_t*                  cond_hash;   // per open conditional: its `#`
    uint32_t                   cond_depth;  // open conditionals
};

// 1.2.2
// d_internal_fn_facts
//   struct: what a function definition's body shows, gathered in one scan of
//   it for the guide's size tripwires and its conditional-compilation rule.
struct d_internal_fn_facts
{
    uint32_t  lines;     // first line to closing brace, inclusive
    uint32_t  nesting;   // deepest brace level inside the body
    uint32_t  params;    // parameters, `(void)` counting none
    bool      pp_split;  // an item in the body sits inside a conditional
};


//==============================================================================
// 2.  SPELLING AND SHAPE
//==============================================================================


/**
 * @brief Produces a token's spelling into the build's buffer, growing it as
 *        needed.
 *
 * A token carrying no rewriting flag is copied from its byte range; any other
 * is re-read through the scanner, which resolves what the flags warned about.
 *
 * @param[in,out] _build  the build whose buffer to fill.
 * @param[in]     _token  the token to spell.
 * @return the spelling's length, or (size_t)-1 when the buffer could not grow.
 */
static size_t
d_internal_spell(
    struct d_internal_build* _build,
    const struct d_token*    _token
)
{
    const uint32_t rewritten = D_TOKEN_FLAG_SPLICED | D_TOKEN_FLAG_TRIGRAPH;
    const bool     copy      = ((_token->flags & rewritten) == 0u);

    // the spelling of an unrewritten token is its byte range
    const size_t needed = copy
                        ? _token->span.length
                        : d_token_spell(_build->lexer, _token, NULL, 0u);

    uint32_t capacity = _build->spelling_size;

    // a token is at most a source long, and a source at most 32 bits
    if (needed >= (size_t)UINT32_MAX)
    {
        return (size_t)-1;
    }

    char* const grown = d_parse_grow(_build->spelling,
                                     &capacity,
                                     (uint32_t)needed + 1u,
                                     1u,
                                     1);

    // grow to hold the spelling and its terminator
    if (!grown)
    {
        return (size_t)-1;
    }

    _build->spelling      = grown;
    _build->spelling_size = capacity;

    if (copy)
    {
        memcpy(_build->spelling, _build->source->text + _token->span.offset,
               needed);
        _build->spelling[needed] = '\0';
    }
    else
    {
        (void)d_token_spell(_build->lexer, _token, _build->spelling,
                            _build->spelling_size);
    }

    return needed;
}


/**
 * @brief Measures a spelling up to its first newline.
 *
 * A token that spans lines -- a block comment, a raw string, a spliced name --
 * has a width only on the line where it begins, which is the line a layout rule
 * is about.
 *
 * @param[in] _text    the spelling.
 * @param[in] _length  its length.
 * @return the width of the first line, in bytes.
 */
static size_t
d_internal_first_line_width(
    const char* _text,
    size_t      _length
)
{
    size_t width = 0u;

    while ((width < _length) && (_text[width] != '\n'))
    {
        ++width;
    }

    return width;
}


/**
 * @brief Writes a kind's name in its attribute form: the enumerator without its
 *        `D_TOKEN_` prefix, lowercased, with hyphens for underscores, so that
 *        D_TOKEN_KW_STRUCT is `kw-struct` as a CSS idiom would spell it.
 *
 * @param[in]     _kind    the kind to name.
 * @param[in,out] _buffer  receives the name.
 * @param[in]     _size    the buffer's size.
 */
static void
d_internal_kind_attribute(
    int    _kind,
    char*  _buffer,
    size_t _size
)
{
    const char* name   = d_token_kind_name(_kind);
    size_t      length = 0u;

    // an unknown kind is named for what it is
    if (!name)
    {
        name = "D_TOKEN_UNKNOWN";
    }

    name += strlen("D_TOKEN_");

    while ((name[length] != '\0') && ((length + 1u) < _size))
    {
        const char c = name[length];

        _buffer[length] = (c == '_')
                        ? '-'
                        : (((c >= 'A') && (c <= 'Z')) ? (char)(c + 32) : c);
        ++length;
    }

    _buffer[length] = '\0';

    return;
}


/**
 * @brief Classifies the letters of a spelling.
 *
 * Only letters count: `D_2` is upper and `_1` has no case at all.
 *
 * @param[in] _text    the spelling.
 * @param[in] _length  its length.
 * @return "upper", "lower", "mixed", or "none".
 */
static const char*
d_internal_case_of(
    const char* _text,
    size_t      _length
)
{
    bool upper = false;
    bool lower = false;

    for (size_t at = 0u; at < _length; ++at)
    {
        if ((_text[at] >= 'A') && (_text[at] <= 'Z'))
        {
            upper = true;
        }
        else if ((_text[at] >= 'a') && (_text[at] <= 'z'))
        {
            lower = true;
        }
    }

    if (upper && lower)
    {
        return "mixed";
    }

    return upper ? "upper" : (lower ? "lower" : "none");
}


//==============================================================================
// 3.  EMISSION
//==============================================================================


/**
 * @brief Sets a numeric attribute.
 *
 * @param[in,out] _tree   the tree.
 * @param[in]     _node   the node.
 * @param[in]     _name   the attribute's name.
 * @param[in]     _value  its value.
 */
static void
d_internal_number(
    struct d_node_tree* _tree,
    uint32_t            _node,
    const char*         _name,
    size_t              _value
)
{
    char text[D_INTERNAL_NUMBER_MAX];

    (void)snprintf(text, sizeof(text), "%zu", _value);
    (void)d_node_set_attribute(_tree, _node, _name, text);

    return;
}


/**
 * @brief Sets a presence attribute for each token flag a sheet can use.
 *
 * @param[in,out] _tree   the tree.
 * @param[in]     _node   the node.
 * @param[in]     _flags  the token's D_TOKEN_FLAG_* bits.
 */
static void
d_internal_flag_attributes(
    struct d_node_tree* _tree,
    uint32_t            _node,
    uint32_t            _flags
)
{
    static const struct
    {
        uint32_t    flag;
        const char* name;
    } table[] =
    {
        { D_TOKEN_FLAG_LINE_START,     "line-start"     },
        { D_TOKEN_FLAG_LEADING_SPACE,  "leading-space"  },
        { D_TOKEN_FLAG_SPLICED,        "spliced"        },
        { D_TOKEN_FLAG_TRIGRAPH,       "trigraph"       },
        { D_TOKEN_FLAG_DIGRAPH,        "alternative"    },
        { D_TOKEN_FLAG_UCN,            "ucn"            },
        { D_TOKEN_FLAG_EXTENDED,       "extended"       },
        { D_TOKEN_FLAG_USER_SUFFIX,    "user-suffix"    },
        { D_TOKEN_FLAG_UNTERMINATED,   "unterminated"   },
        { D_TOKEN_FLAG_DIRECTIVE_NAME, "directive-name" },
        { D_TOKEN_FLAG_MACRO_NAME,     "macro"          },
        { D_TOKEN_FLAG_COMMENT_DOC,    "doc"            }
    };

    for (size_t at = 0u; at < (sizeof(table) / sizeof(table[0])); ++at)
    {
        if ((_flags & table[at].flag) != 0u)
        {
            (void)d_node_set_attribute(_tree, _node, table[at].name, NULL);
        }
    }

    return;
}


/**
 * @brief Names the node type for a token that is not a group or a directive.
 *
 * @param[in] _kind  the token's kind.
 * @return the type name.
 */
static const char*
d_internal_type_of(
    int _kind
)
{
    if (d_token_is_keyword(_kind))
    {
        return "keyword";
    }

    if (d_token_is_punctuator(_kind))
    {
        return "punctuator";
    }

    switch (_kind)
    {
        case D_TOKEN_IDENTIFIER:  return "identifier";
        case D_TOKEN_NUMBER:      return "number";
        case D_TOKEN_CHARACTER:   return "character";
        case D_TOKEN_STRING:      return "string";
        case D_TOKEN_RAW_STRING:  return "raw-string";
        case D_TOKEN_HEADER_NAME: return "header-name";
        case D_TOKEN_COMMENT:     return "comment";
        case D_TOKEN_ERROR:       return "error";
        default:                  return "other";
    }
}


/**
 * @brief Records a node's line and columns from a token and its spelling.
 *
 * @param[in,out] _tree   the tree.
 * @param[in]     _node   the node.
 * @param[in]     _token  the token the node stands for.
 * @param[in]     _width  the spelling's width on its first line.
 */
static void
d_internal_geometry(
    struct d_node_tree*   _tree,
    uint32_t              _node,
    const struct d_token* _token,
    size_t                _width
)
{
    struct d_node* const node = d_node_at(_tree, _node);

    if (!node)
    {
        return;
    }

    node->line         = _token->span.line;
    node->start_column = _token->span.column;
    node->width        = (uint32_t)_width;
    node->end_column   = (_width > 0u)
                       ? (uint32_t)(_token->span.column + _width - 1u)
                       : _token->span.column;

    return;
}


/**
 * @brief Reports whether a `#define` supplies a name only if nobody else has.
 *
 * `#ifndef X_OK` then `#define X_OK 1`: the second line is a fallback for a
 * name another header owns, so its spelling is that header's to choose, not
 * this file's.  Comments between the two lines are passed over.
 *
 * @param[in] _build  the build.
 * @param[in] _at     the `#define`'s `#` token index.
 * @param[in] _macro  the defined macro's token index.
 * @return `true` if the directive before it tests the same name, `false`
 *         otherwise.
 */
static bool
d_internal_define_guarded(
    const struct d_internal_build* _build,
    uint32_t                       _at,
    uint32_t                       _macro
)
{
    uint32_t prev = _at;

    // step back over comments to the last token of the line before
    while (prev > 0u)
    {
        --prev;

        if (_build->tokens[prev].kind != D_TOKEN_COMMENT)
        {
            break;
        }
    }

    // nothing before it, or the line before is not a directive
    if ( (prev == _at) ||
         ((_build->tokens[prev].flags & D_TOKEN_FLAG_IN_DIRECTIVE) == 0u) )
    {
        return false;
    }

    uint32_t hash = prev;

    // climb out of any group on that line to its `#`
    while ( (hash != D_SYNTAX_NONE) &&
            (!d_syntax_is_directive_start(&_build->tokens[hash])) )
    {
        hash = _build->groups.parent[hash];
    }

    if ( (hash == D_SYNTAX_NONE) ||
         (_build->groups.match[hash] == D_SYNTAX_NONE) )
    {
        return false;
    }

    const struct d_token* const defined = &_build->tokens[_macro];

    // the line before must test this very name
    for (uint32_t at = hash + 1u; at <= _build->groups.match[hash]; ++at)
    {
        const struct d_token* const tested = &_build->tokens[at];

        if ( ((tested->flags & D_TOKEN_FLAG_MACRO_NAME) != 0u) &&
             (tested->span.length == defined->span.length)                &&
             (memcmp(_build->source->text + tested->span.offset,
                     _build->source->text + defined->span.offset,
                     defined->span.length) == 0) )
        {
            return true;
        }
    }

    return false;
}


/**
 * @brief Reports whether a directive's name token is spelled `_word`.
 *
 * @param[in] _build  the build.
 * @param[in] _name   the name token's index, or D_SYNTAX_NONE.
 * @param[in] _word   the spelling to compare against.
 * @return `true` if the name is spelled exactly `_word`, `false` otherwise.
 */
static bool
d_internal_directive_is(
    const struct d_internal_build* _build,
    uint32_t                       _name,
    const char*                    _word
)
{
    const size_t length = strlen(_word);

    if (_name == D_SYNTAX_NONE)
    {
        return false;
    }

    return ( (_build->tokens[_name].span.length == length) &&
             (memcmp(_build->source->text + _build->tokens[_name].span.offset,
                     _word,
                     length) == 0) );
}


/**
 * @brief Reports whether a comment names a macro as a whole word.
 *
 * @param[in] _build    the build.
 * @param[in] _comment  the comment token's index.
 * @param[in] _macro    the macro name token's index.
 * @return `true` if the macro's spelling occurs in the comment with no
 *         identifier character on either side, `false` otherwise.
 */
static bool
d_internal_comment_names(
    const struct d_internal_build* _build,
    uint32_t                       _comment,
    uint32_t                       _macro
)
{
    const char* const text   = _build->source->text +
                               _build->tokens[_comment].span.offset;
    const size_t      length = _build->tokens[_comment].span.length;
    const char* const name   = _build->source->text +
                               _build->tokens[_macro].span.offset;
    const size_t      size   = _build->tokens[_macro].span.length;

    for (size_t at = 0u; (size > 0u) && (at + size <= length); ++at)
    {
        const bool bounded_before = (at == 0u) ||
                                    ( (text[at - 1u] != '_') &&
                                      (!isalnum((unsigned char)text[at - 1u])) );
        const bool bounded_after  = (at + size == length) ||
                                    ( (text[at + size] != '_') &&
                                      (!isalnum((unsigned char)text[at + size])) );

        // an occurrence with no identifier character touching it
        if ( (bounded_before) &&
             (bounded_after)  &&
             (memcmp(text + at, name, size) == 0) )
        {
            return true;
        }
    }

    return false;
}


/**
 * @brief Counts the blank lines between a token and the token before it.
 *
 * @note No token lies on the lines strictly between two consecutive tokens,
 *       comments and directives included, so each of them is blank.
 *
 * @param[in] _build  the build.
 * @param[in] _at     the token's index.
 * @return the count; for the first token, the lines above it.
 */
static uint32_t
d_internal_blank_before(
    const struct d_internal_build* _build,
    uint32_t                       _at
)
{
    const uint32_t line     = _build->tokens[_at].span.line;
    const uint32_t previous = (_at == 0u) ? 0u
                                          : _build->tokens[_at - 1u].end_line;

    return (line > previous + 1u) ? (line - previous - 1u) : 0u;
}


/**
 * @brief Sets `blank-before` on a node that begins its line, when blank lines
 *        stand above it.
 *
 * @param[in,out] _build  the build.
 * @param[in]     _node   the node.
 * @param[in]     _at     the index of the token it begins with.
 */
static void
d_internal_blank_attribute(
    struct d_internal_build* _build,
    uint32_t                 _node,
    uint32_t                 _at
)
{
    const uint32_t blank = d_internal_blank_before(_build, _at);

    if ( ((_build->tokens[_at].flags & D_TOKEN_FLAG_LINE_START) != 0u) &&
         (blank > 0u) )
    {
        d_internal_number(_build->tree, _node, "blank-before", blank);
    }

    return;
}


/**
 * @brief Reports whether one line of a comment is exactly a name.
 *
 * @note Comment punctuation (`/`, `*`, `!`) and spaces around the name are
 *       passed over, so `// D_NAME`, and the `d_fn` line of a block comment, both
 *       name their entity.
 *
 * @param[in] _build   the build.
 * @param[in] _at      the comment token's index.
 * @param[in] _name    the name.
 * @param[in] _length  its length.
 * @return `true` if some line of the comment holds the name and nothing
 *         else, or names a family the name belongs to (`PREFIX<...>`).
 */
static bool
d_internal_comment_line_is(
    const struct d_internal_build* _build,
    uint32_t                       _at,
    const char*                    _name,
    size_t                         _length
)
{
    const char* const text = _build->source->text +
                             _build->tokens[_at].span.offset;
    const size_t      size = _build->tokens[_at].span.length;
    size_t            at   = 0u;

    while (at < size)
    {
        size_t end = at;

        while ( (end < size) && (text[end] != '\n') )
        {
            ++end;
        }

        size_t first = at;
        size_t last  = end;

        // comment punctuation and space on either side of the words
        while ( (first < last) &&
                ( (text[first] == '/') || (text[first] == '*') ||
                  (text[first] == '!') || (text[first] == ' ') ||
                  (text[first] == '\r') ) )
        {
            ++first;
        }

        while ( (last > first) &&
                ( (text[last - 1u] == '/') || (text[last - 1u] == '*') ||
                  (text[last - 1u] == ' ') || (text[last - 1u] == '\r') ) )
        {
            --last;
        }

        if ( (last - first == _length) &&
             (memcmp(text + first, _name, _length) == 0) )
        {
            return true;
        }

        // a family, `D_INTERNAL_ARGS<01-10>`, names each member it prefixes
        const char* const open = (last > first)
            ? memchr(text + first, '<', last - first)
            : NULL;

        if ( (open) && (text[last - 1u] == '>') &&
             ((size_t)(open - (text + first)) > 0u) &&
             ((size_t)(open - (text + first)) < _length) &&
             (memcmp(text + first, _name, (size_t)(open - (text + first)))
              == 0) )
        {
            return true;
        }

        at = end + 1u;
    }

    return false;
}


/**
 * @brief Reports whether the comments directly above a token name an entity.
 *
 * @note The run is whole-line comments, each ending on the line just above
 *       the next, so a blank line or a line of code ends it.
 *
 * @param[in] _build   the build.
 * @param[in] _first   the entity's first token.
 * @param[in] _name    its name.
 * @param[in] _length  the name's length.
 * @return `true` if a comment of the run has a line that is the name.
 */
static bool
d_internal_documented(
    const struct d_internal_build* _build,
    uint32_t                       _first,
    const char*                    _name,
    size_t                         _length
)
{
    uint32_t line = _build->tokens[_first].span.line;

    for (uint32_t at = _first; at > 0u; --at)
    {
        const struct d_token* const comment = &_build->tokens[at - 1u];

        // the run ends at anything but a whole-line comment just above
        if ( (comment->kind != D_TOKEN_COMMENT)                        ||
             ((comment->flags & D_TOKEN_FLAG_LINE_START) == 0u)        ||
             (comment->end_line + 1u != line) )
        {
            break;
        }

        if (d_internal_comment_line_is(_build, at - 1u, _name, _length))
        {
            return true;
        }

        line = comment->span.line;
    }

    return false;
}


/**
 * @brief Reports whether a declared name is followed by an initializer.
 *
 * @note Groups within the declarator (`[3]`, a parameter list) are passed
 *       over whole; a top-level `,` or `;`, or a closer, ends the declarator.
 *
 * @param[in] _build  the build.
 * @param[in] _at     the declared name's token.
 * @return `true` if a top-level `=` follows before the declarator ends.
 */
static bool
d_internal_initialized(
    const struct d_internal_build* _build,
    uint32_t                       _at
)
{
    for (uint32_t at = _at + 1u; at < (uint32_t)_build->count; ++at)
    {
        const struct d_token* const token   = &_build->tokens[at];
        const char* const           spelled = d_token_spelling(token->kind);

        if (token->kind == D_TOKEN_COMMENT)
        {
            continue;
        }

        // a directive or a closer ends the search
        if ( ((token->flags & D_TOKEN_FLAG_IN_DIRECTIVE) != 0u) ||
             (d_syntax_is_closer(token->kind)) )
        {
            return false;
        }

        // a nested group is part of the declarator
        if ( (d_syntax_is_opener(token->kind)) &&
             (_build->groups.match[at] != D_SYNTAX_NONE) )
        {
            at = _build->groups.match[at];

            continue;
        }

        if ( (spelled) && (strcmp(spelled, "=") == 0) )
        {
            return true;
        }

        if ( (spelled) &&
             ( (strcmp(spelled, ",") == 0) || (strcmp(spelled, ";") == 0) ) )
        {
            return false;
        }
    }

    return false;
}


/**
 * @brief Finds a path's stem: its last component without its extension.
 *
 * @param[in]  _path        the path; need not be terminated at `_length`.
 * @param[in]  _length      its length.
 * @param[out] _out_length  receives the stem's length.
 * @return the stem's first character.
 */
static const char*
d_internal_stem(
    const char* _path,
    size_t      _length,
    size_t*     _out_length
)
{
    size_t start = _length;

    while ( (start > 0u) &&
            (_path[start - 1u] != '/') && (_path[start - 1u] != '\\') )
    {
        --start;
    }

    size_t end = _length;

    // the last dot of the component, if it is not the first character
    for (size_t at = _length; at > start + 1u; --at)
    {
        if (_path[at - 1u] == '.')
        {
            end = at - 1u;
            break;
        }
    }

    *_out_length = end - start;

    return _path + start;
}


/**
 * @brief Emits the node for a directive's `#`.
 *
 * The directive's name, the macro it defines or tests, and the header it
 * includes are all read from its line before the node has an attribute, since
 * the attributes must be contiguous.
 *
 * @param[in,out] _build   the build.
 * @param[in]     _at      the `#` token's index.
 * @param[in]     _parent  the node to add beneath.
 * @return the node, or D_DSS_NO_INDEX.
 */
static uint32_t
d_internal_emit_directive(
    struct d_internal_build* _build,
    uint32_t                 _at,
    uint32_t                 _parent
)
{
    const uint32_t last =
        (_build->groups.match[_at] == D_SYNTAX_NONE)
        ? _at
        : _build->groups.match[_at];

    uint32_t name   = D_SYNTAX_NONE;
    uint32_t macro  = D_SYNTAX_NONE;
    uint32_t header = D_SYNTAX_NONE;

    // read the line before writing anything
    for (uint32_t at = _at + 1u; at <= last; ++at)
    {
        const struct d_token* const token = &_build->tokens[at];

        if ( (name == D_SYNTAX_NONE) &&
             ((token->flags & D_TOKEN_FLAG_DIRECTIVE_NAME) != 0u) )
        {
            name = at;
        }

        if ( (macro == D_SYNTAX_NONE) &&
             ((token->flags & D_TOKEN_FLAG_MACRO_NAME) != 0u) )
        {
            macro = at;
        }

        if ((header == D_SYNTAX_NONE) && (token->kind == D_TOKEN_HEADER_NAME))
        {
            header = at;
        }
    }

    const uint32_t node = d_node_add(_build->tree, _parent, "directive");

    if (node == D_DSS_NO_INDEX)
    {
        return D_DSS_NO_INDEX;
    }

    struct d_node_tree* const tree = _build->tree;
    const size_t              none = (size_t)-1;

    if (name != D_SYNTAX_NONE)
    {
        const size_t length = d_internal_spell(_build, &_build->tokens[name]);

        if (length != none)
        {
            (void)d_node_set_attribute(tree, node, "name", _build->spelling);
        }
    }

    if (macro != D_SYNTAX_NONE)
    {
        const size_t length = d_internal_spell(_build, &_build->tokens[macro]);

        if (length != none)
        {
            (void)d_node_set_attribute(tree, node, "macro", _build->spelling);

            // defined with `(` touching the name: a function-like macro
            if ( (d_internal_directive_is(_build, name, "define")) &&
                 (macro + 1u <= last) &&
                 (d_token_spelling(_build->tokens[macro + 1u].kind)) &&
                 (strcmp(d_token_spelling(_build->tokens[macro + 1u].kind),
                         "(") == 0) &&
                 ((_build->tokens[macro + 1u].flags &
                   D_TOKEN_FLAG_LEADING_SPACE) == 0u) )
            {
                (void)d_node_set_attribute(tree, node, "function-like",
                                           NULL);
            }

            // the comment run above the directive names what it defines,
            // or the run above a conditional around it does: a definition
            // chosen by #if / #else is documented once, above the #if
            bool documented = false;

            for (uint32_t open = _build->cond_depth + 1u;
                 (d_internal_directive_is(_build, name, "define")) &&
                 (!documented) && (open > 0u);
                 --open)
            {
                const uint32_t anchor = (open == _build->cond_depth + 1u)
                                        ? _at
                                        : _build->cond_hash[open - 1u];

                documented = d_internal_documented(_build, anchor,
                                                   _build->spelling, length);
            }

            if (documented)
            {
                (void)d_node_set_attribute(tree, node, "documented", NULL);
            }
        }
    }

    // a definition that is only a fallback for another's name says so; a
    // sheet asks for the rest with :not([guarded])
    if ( (name != D_SYNTAX_NONE) &&
         (macro != D_SYNTAX_NONE) &&
         (_build->tokens[name].span.length == 6u) &&
         (memcmp(_build->source->text + _build->tokens[name].span.offset,
                 "define",
                 6u) == 0) &&
         d_internal_define_guarded(_build,
                                   _at,
                                   macro) )
    {
        (void)d_node_set_attribute(tree,
                                   node,
                                   "guarded",
                                   "yes");
    }

    // a header name is recorded without its delimiters, and its form apart
    if (header != D_SYNTAX_NONE)
    {
        const size_t length =
            d_internal_spell(_build, &_build->tokens[header]);

        if ((length != none) && (length >= 2u))
        {
            const bool angle = (_build->spelling[0] == '<');

            _build->spelling[length - 1u] = '\0';
            (void)d_node_set_attribute(tree, node, "header",
                                       _build->spelling + 1);
            (void)d_node_set_attribute(tree, node, "form",
                                       angle ? "angle" : "quote");

            size_t            header_stem_length = 0u;
            size_t            file_stem_length   = 0u;
            const char* const header_stem =
                d_internal_stem(_build->spelling + 1, length - 2u,
                                &header_stem_length);
            const char* const file_stem =
                _build->source->name
                ? d_internal_stem(_build->source->name,
                                  strlen(_build->source->name),
                                  &file_stem_length)
                : NULL;

            // a project header, in quotes, sharing the file's stem
            if ( (!angle)                                  &&
                 (file_stem)                               &&
                 (header_stem_length == file_stem_length)  &&
                 (header_stem_length > 0u)                 &&
                 (memcmp(header_stem, file_stem, file_stem_length) == 0) )
            {
                (void)d_node_set_attribute(tree, node, "corresponding", NULL);
            }
        }
    }

    // the conditional an #endif closes, and whether its comment names the
    // symbol an #ifdef or #ifndef tested: facts the closure rule reads
    static const char* const forms[] = { "if", "ifdef", "ifndef" };

    for (uint8_t form = 0u; form < 3u; ++form)
    {
        if ( (d_internal_directive_is(_build, name, forms[form])) &&
             (_build->cond_depth <= (uint32_t)_build->count) )
        {
            _build->cond_kind[_build->cond_depth]  = form;
            _build->cond_macro[_build->cond_depth] = macro;
            _build->cond_hash[_build->cond_depth]  = _at;
            ++_build->cond_depth;
        }
    }

    if ( (d_internal_directive_is(_build, name, "endif")) &&
         (_build->cond_depth > 0u) )
    {
        --_build->cond_depth;

        const uint8_t  form   = _build->cond_kind[_build->cond_depth];
        const uint32_t tested = _build->cond_macro[_build->cond_depth];
        bool           named  = false;

        // a comment anywhere on the line may carry the name
        for (uint32_t at = _at + 1u;
             (tested != D_SYNTAX_NONE) && (at <= last) && (!named);
             ++at)
        {
            named = ( (_build->tokens[at].kind == D_TOKEN_COMMENT) &&
                      (d_internal_comment_names(_build, at, tested)) );
        }

        (void)d_node_set_attribute(tree, node, "closes", forms[form]);

        if (named)
        {
            (void)d_node_set_attribute(tree, node, "named", NULL);
        }
    }

    d_internal_number(tree, node, "depth", _build->depth[_at]);
    d_internal_blank_attribute(_build, node, _at);

    // the directive spans its line, from the `#` to its last token
    const struct d_token* const first = &_build->tokens[_at];
    const struct d_token* const end   = &_build->tokens[last];
    size_t                      width = 1u;

    if (end->span.line == first->span.line)
    {
        width = (end->span.column + end->span.length) - first->span.column;
    }

    d_internal_geometry(tree, node, first, width);
    (void)d_node_set_text(tree, node, "#", 1u);

    ++_build->stats->directives;

    return node;
}


/**
 * @brief Emits the node for an opening bracket.
 *
 * The closer's position is read through the match array now, since it must be
 * an attribute of the group and the group's attributes must be contiguous.
 *
 * @param[in,out] _build   the build.
 * @param[in]     _at      the opener's index.
 * @param[in]     _parent  the node to add beneath.
 * @return the node, or D_DSS_NO_INDEX.
 */
static uint32_t
d_internal_emit_group(
    struct d_internal_build* _build,
    uint32_t                 _at,
    uint32_t                 _parent
)
{
    const struct d_token* const token = &_build->tokens[_at];
    const uint32_t              close = _build->groups.match[_at];
    struct d_node_tree* const   tree  = _build->tree;

    const uint32_t node = d_node_add(tree, _parent, "group");

    if (node == D_DSS_NO_INDEX)
    {
        return D_DSS_NO_INDEX;
    }

    char kind[D_INTERNAL_NAME_MAX];

    d_internal_kind_attribute(token->kind, kind, sizeof(kind));

    (void)d_node_set_attribute(tree, node, "kind", kind);
    (void)d_node_set_attribute(tree, node, "open",
                               d_token_spelling(token->kind));
    (void)d_node_set_attribute(
        tree, node, "close",
        d_token_spelling(d_syntax_closer_for(token->kind)));

    if (close != D_SYNTAX_NONE)
    {
        const struct d_token* const closer = &_build->tokens[close];

        d_internal_number(tree, node, "close-line",   closer->span.line);
        d_internal_number(tree, node, "close-column", closer->span.column);

        // a closer first on its line is what a brace-placement rule reads
        if ((closer->flags & D_TOKEN_FLAG_LINE_START) != 0u)
        {
            (void)d_node_set_attribute(tree, node, "close-line-start", NULL);
        }
    }
    else
    {
        (void)d_node_set_attribute(tree, node, "unclosed", NULL);
    }

    d_internal_flag_attributes(tree, node,
                               token->flags & (D_TOKEN_FLAG_LINE_START    |
                                               D_TOKEN_FLAG_LEADING_SPACE |
                                               D_TOKEN_FLAG_DIGRAPH));
    d_internal_number(tree, node, "depth", _build->depth[_at]);

    // a body's brace sits at the level of the item that owns it
    if (_build->items.level_of[_at] != 0xFFu)
    {
        d_internal_number(tree,
                          node,
                          "level",
                          _build->items.level_of[_at]);
    }

    if (_build->head_of[_at] == D_SYNTAX_NONE)
    {
        d_internal_blank_attribute(_build, node, _at);
    }

    d_internal_geometry(tree, node, token, token->span.length);

    (void)d_node_set_text(tree, node, d_token_spelling(token->kind),
                          strlen(d_token_spelling(token->kind)));

    ++_build->stats->groups;

    return node;
}


/**
 * @brief Emits the node for any token that is neither a group nor a directive.
 *
 * @param[in,out] _build   the build.
 * @param[in]     _at      the token's index.
 * @param[in]     _parent  the node to add beneath.
 * @return the node, or D_DSS_NO_INDEX.
 */
static uint32_t
d_internal_emit_leaf(
    struct d_internal_build* _build,
    uint32_t                 _at,
    uint32_t                 _parent
)
{
    const struct d_token* const token  = &_build->tokens[_at];
    struct d_node_tree* const   tree   = _build->tree;
    const size_t                length = d_internal_spell(_build, token);

    // the spelling could not be held
    if (length == (size_t)-1)
    {
        return D_DSS_NO_INDEX;
    }

    const uint32_t node = d_node_add(tree, _parent,
                                     d_internal_type_of(token->kind));

    if (node == D_DSS_NO_INDEX)
    {
        return D_DSS_NO_INDEX;
    }

    char kind[D_INTERNAL_NAME_MAX];

    d_internal_kind_attribute(token->kind, kind, sizeof(kind));

    (void)d_node_set_attribute(tree, node, "kind", kind);
    (void)d_node_set_attribute(tree, node, "text", _build->spelling);

    // an identifier carries its case, a fact a naming rule reads; a prefix
    // rule reads the name's own `text` through :not([text^=...])
    if (token->kind == D_TOKEN_IDENTIFIER)
    {
        (void)d_node_set_attribute(tree, node, "case",
                                   d_internal_case_of(_build->spelling,
                                                      length));
    }

    // a comment carries its form
    if (token->kind == D_TOKEN_COMMENT)
    {
        (void)d_node_set_attribute(
            tree, node, "form",
            ((token->flags & D_TOKEN_FLAG_COMMENT_LINE) != 0u) ? "line"
                                                                : "block");
        ++_build->stats->comments;
    }

    // a closer with no opener is a leaf, and says so
    if (d_syntax_is_closer(token->kind))
    {
        (void)d_node_set_attribute(tree, node, "unpaired", NULL);
    }

    d_internal_flag_attributes(tree, node, token->flags);

    // the identifier a declaration names, which is what a naming rule reads
    if (_build->declarator[_at])
    {
        (void)d_node_set_attribute(tree,
                                   node,
                                   "declarator",
                                   NULL);

        if (d_internal_initialized(_build, _at))
        {
            (void)d_node_set_attribute(tree, node, "initialized", NULL);
        }
    }

    if (_build->head_of[_at] == D_SYNTAX_NONE)
    {
        d_internal_blank_attribute(_build, node, _at);
    }

    d_internal_number(tree, node, "depth", _build->depth[_at]);
    d_internal_geometry(tree, node, token,
                        d_internal_first_line_width(_build->spelling,
                                                    length));

    (void)d_node_set_text(tree, node, _build->spelling, length);

    return node;
}


/**
 * @brief Counts a function's parameters from its parameter group.
 *
 * A top-level comma separates two parameters, and a list holding only `void`
 * holds none, so the count needs no item lookup.
 *
 * @param[in] _build   the build.
 * @param[in] _opener  the parameter group's opening token index.
 * @return the parameter count; zero for `()` and `(void)`.
 */
static uint32_t
d_internal_param_count(
    const struct d_internal_build* _build,
    uint32_t                       _opener
)
{
    const uint32_t end = (_opener == D_SYNTAX_NONE)
                       ? D_SYNTAX_NONE
                       : _build->groups.match[_opener];

    // no group, or one that never closed
    if (end == D_SYNTAX_NONE)
    {
        return 0u;
    }

    uint32_t count = 0u;
    uint32_t first = D_SYNTAX_NONE;

    for (uint32_t at = _opener + 1u; at < end; ++at)
    {
        const struct d_token* const token = &_build->tokens[at];

        // only the group's own children separate its parameters
        if ( (_build->groups.parent[at] != _opener) ||
             (token->kind == D_TOKEN_COMMENT) )
        {
            continue;
        }

        if (first == D_SYNTAX_NONE)
        {
            first = at;
            count = 1u;
        }
        else if (token->kind == D_TOKEN_COMMA)
        {
            ++count;
        }
    }

    // `(void)` declares no parameter
    if ( (count == 1u) &&
         (_build->tokens[first].kind == D_TOKEN_KW_VOID) )
    {
        return 0u;
    }

    return count;
}


/**
 * @brief Gathers a function definition's body facts in one scan of the body.
 *
 * Nesting counts braces opened inside the body, so a statement directly in
 * it is at level zero; the split test looks for any item beginning in the
 * body that sits inside a conditional, which is the shape the guide's
 * Conditional Compilation section forbids.
 *
 * @param[in]  _build      the build.
 * @param[in]  _k          the function definition's item index.
 * @param[out] _out_facts  receives the facts.
 */
static void
d_internal_fn_facts(
    const struct d_internal_build* _build,
    uint32_t                       _k,
    struct d_internal_fn_facts*    _out_facts
)
{
    const struct d_syntax_item* const item = &_build->items.items[_k];
    const uint32_t                    body = item->body;

    memset(_out_facts, 0, sizeof(*_out_facts));

    _out_facts->params = d_internal_param_count(_build, item->params);

    // a body that never closed has no extent to measure
    if ( (body == D_SYNTAX_NONE) ||
         (_build->groups.match[body] == D_SYNTAX_NONE) )
    {
        return;
    }

    const uint32_t end = _build->groups.match[body];

    _out_facts->lines = _build->tokens[end].span.line -
                        _build->tokens[item->first].span.line + 1u;

    for (uint32_t at = body + 1u; at < end; ++at)
    {
        const uint32_t depth = _build->depth[at] - _build->depth[body] - 1u;
        const uint32_t owner = _build->items.item_of[at];

        if (depth > _out_facts->nesting)
        {
            _out_facts->nesting = depth;
        }

        // an item that begins here inside a conditional splits the body
        if ( (owner != D_SYNTAX_NONE)                     &&
             (_build->items.items[owner].first == at)     &&
             (_build->items.items[owner].pp_depth > 0u) )
        {
            _out_facts->pp_split = true;
        }
    }

    return;
}


/**
 * @brief Names the tag keyword a typedef's type is spelled with.
 *
 * @param[in] _build  the build.
 * @param[in] _k      the typedef's item index.
 * @return `struct`, `union`, `enum` or `class`, or `NULL` if none stands at the
 *         typedef's own level.
 */
static const char*
d_internal_hidden_tag(
    const struct d_internal_build* _build,
    uint32_t                       _k
)
{
    const struct d_syntax_item* const item = &_build->items.items[_k];

    for (uint32_t at = item->first; at <= item->last; ++at)
    {
        const int kind = _build->tokens[at].kind;

        // a tag keyword inside a parameter group names no hidden type
        if (_build->groups.parent[at] != item->scope)
        {
            continue;
        }

        if ( (kind == D_TOKEN_KW_STRUCT) ||
             (kind == D_TOKEN_KW_UNION)  ||
             (kind == D_TOKEN_KW_ENUM)   ||
             (kind == D_TOKEN_KW_CLASS) )
        {
            return d_token_spelling(kind);
        }
    }

    return NULL;
}


/**
 * @brief Reports the layout flags of a function's first parameter.
 *
 * Alignment and detachment are set on every parameter of a list alike, so the
 * first one speaks for the list.
 *
 * @param[in] _build   the build.
 * @param[in] _opener  the parameter group's opening token index.
 * @return the first parameter's `D_SYNTAX_ITEM_FLAG_*` bits, or zero when the
 *         list holds no parameter item.
 */
static uint16_t
d_internal_first_param_flags(
    const struct d_internal_build* _build,
    uint32_t                       _opener
)
{
    // no group, or one that never closed
    if ( (_opener == D_SYNTAX_NONE) ||
         (_build->groups.match[_opener] == D_SYNTAX_NONE) )
    {
        return 0u;
    }

    for (uint32_t at = _opener + 1u; at < _build->groups.match[_opener]; ++at)
    {
        const uint32_t owner = _build->items.item_of[at];

        if ( (owner != D_SYNTAX_NONE) &&
             (_build->items.items[owner].kind == D_SYNTAX_ITEM_PARAM) )
        {
            return _build->items.items[owner].flags;
        }
    }

    return 0u;
}


/**
 * @brief Sets the attributes only a function carries.
 *
 * @param[in] _build  the build.
 * @param[in] _k      the function's item index.
 * @param[in] _node   the function's node.
 */
static void
d_internal_fn_attributes(
    struct d_internal_build* _build,
    uint32_t                 _k,
    uint32_t                 _node
)
{
    const struct d_syntax_item* const item = &_build->items.items[_k];
    struct d_node_tree* const         tree = _build->tree;
    struct d_internal_fn_facts        facts;

    d_internal_fn_facts(_build, _k, &facts);

    (void)d_node_set_attribute(
        tree,
        _node,
        "returns",
        ((item->flags & D_SYNTAX_ITEM_FLAG_RETURNS_VOID) != 0u) ? "void"
                                                               : "value");
    d_internal_number(tree,
                      _node,
                      "params",
                      facts.params);

    if ((item->flags & D_SYNTAX_ITEM_FLAG_MACRO_HEAD) != 0u)
    {
        (void)d_node_set_attribute(tree,
                                   _node,
                                   "macro",
                                   NULL);
    }

    // the parameter layout, once per function: how its first one lies
    const uint16_t layout = d_internal_first_param_flags(_build, item->params);

    if ((layout & D_SYNTAX_ITEM_FLAG_ALIGNED) != 0u)
    {
        (void)d_node_set_attribute(tree,
                                   _node,
                                   "aligned",
                                   NULL);
    }

    if ((layout & D_SYNTAX_ITEM_FLAG_DETACHED) != 0u)
    {
        (void)d_node_set_attribute(tree,
                                   _node,
                                   "detached",
                                   NULL);
    }

    // only a definition has a body to measure
    if (item->kind != D_SYNTAX_ITEM_FN_DEF)
    {
        return;
    }

    const char* ends = "other";

    if ((item->flags & D_SYNTAX_ITEM_FLAG_ENDS_RETURN) != 0u)
    {
        ends = "return";
    }
    else if ((item->flags & D_SYNTAX_ITEM_FLAG_EMPTY_BODY) != 0u)
    {
        ends = "none";
    }

    (void)d_node_set_attribute(tree,
                               _node,
                               "ends",
                               ends);
    d_internal_number(tree,
                      _node,
                      "lines",
                      facts.lines);
    d_internal_number(tree,
                      _node,
                      "nesting",
                      facts.nesting);

    if (facts.pp_split)
    {
        (void)d_node_set_attribute(tree,
                                   _node,
                                   "pp-split",
                                   NULL);
    }

    return;
}


/**
 * @brief Sets the attributes only a typedef or a parameter carries.
 *
 * @param[in] _build  the build.
 * @param[in] _k      the item's index.
 * @param[in] _node   the item's node.
 */
static void
d_internal_decl_attributes(
    struct d_internal_build* _build,
    uint32_t                 _k,
    uint32_t                 _node
)
{
    static const struct
    {
        uint16_t    flag;
        const char* name;
    } presence[] =
    {
        { D_SYNTAX_ITEM_FLAG_ALIGNED,    "aligned"    },
        { D_SYNTAX_ITEM_FLAG_MISALIGNED, "misaligned" },
        { D_SYNTAX_ITEM_FLAG_VARIADIC,   "variadic"   },
        { D_SYNTAX_ITEM_FLAG_VOID,       "void"       },
        { D_SYNTAX_ITEM_FLAG_DETACHED,   "detached"   }
    };

    const struct d_syntax_item* const item = &_build->items.items[_k];
    struct d_node_tree* const         tree = _build->tree;

    if (item->kind == D_SYNTAX_ITEM_TYPEDEF)
    {
        const char* form = "object";

        if ((item->flags & D_SYNTAX_ITEM_FLAG_FN_POINTER) != 0u)
        {
            form = "function-pointer";
        }
        else if ((item->flags & D_SYNTAX_ITEM_FLAG_FN_TYPE) != 0u)
        {
            form = "function";
        }

        (void)d_node_set_attribute(tree,
                                   _node,
                                   "form",
                                   form);

        // the one typedef the guide forbids: one hiding a tag
        if ((item->flags & D_SYNTAX_ITEM_FLAG_HIDES_TAG) != 0u)
        {
            const char* const tag = d_internal_hidden_tag(_build, _k);

            (void)d_node_set_attribute(tree,
                                       _node,
                                       "hides",
                                       tag ? tag : "tag");
        }

        return;
    }

    for (size_t at = 0u; at < (sizeof(presence) / sizeof(presence[0])); ++at)
    {
        if ((item->flags & presence[at].flag) != 0u)
        {
            (void)d_node_set_attribute(tree,
                                       _node,
                                       presence[at].name,
                                       NULL);
        }
    }

    return;
}


/**
 * @brief Emits the node for an item, before its first token's own node.
 *
 * Every attribute is set before any other node is added, since a node's
 * attributes must be contiguous.  `level` is withheld from an aligned
 * parameter: its column follows the first parameter, not its level, so an
 * indentation rule must never select it.
 *
 * @param[in,out] _build   the build.
 * @param[in]     _k       the item's index.
 * @param[in]     _parent  the node to add beneath.
 * @return the node, or `D_DSS_NO_INDEX` if it could not be added.
 */
static uint32_t
d_internal_emit_item(
    struct d_internal_build* _build,
    uint32_t                 _k,
    uint32_t                 _parent
)
{
    const struct d_syntax_item* const item  = &_build->items.items[_k];
    const struct d_token* const       first = &_build->tokens[item->first];
    struct d_node_tree* const         tree  = _build->tree;
    const size_t                      none  = (size_t)-1;

    const uint32_t node = d_node_add(tree,
                                     _parent,
                                     d_syntax_item_kind_name(item->kind));

    // the tree refused the node
    if (node == D_DSS_NO_INDEX)
    {
        return D_DSS_NO_INDEX;
    }

    // statements and labels say which they are
    if ( (item->kind == D_SYNTAX_ITEM_STATEMENT) ||
         (item->kind == D_SYNTAX_ITEM_LABEL) )
    {
        (void)d_node_set_attribute(tree,
                                   node,
                                   "kind",
                                   d_syntax_statement_name(item->statement));
    }

    // the name the item declares
    if ( (item->name != D_SYNTAX_NONE) &&
         (d_internal_spell(_build, &_build->tokens[item->name]) != none) )
    {
        (void)d_node_set_attribute(tree,
                                   node,
                                   "name",
                                   _build->spelling);
    }

    if ( (item->kind == D_SYNTAX_ITEM_FN_DEF) ||
         (item->kind == D_SYNTAX_ITEM_FN_DECL) )
    {
        d_internal_fn_attributes(_build, _k, node);
    }
    else if ( (item->kind == D_SYNTAX_ITEM_TYPEDEF) ||
              (item->kind == D_SYNTAX_ITEM_PARAM) )
    {
        d_internal_decl_attributes(_build, _k, node);
    }

    // an aligned or detached parameter's column follows other parameters,
    // not its level, so an indentation rule must never select it
    if ((item->flags & ( D_SYNTAX_ITEM_FLAG_ALIGNED |
                         D_SYNTAX_ITEM_FLAG_DETACHED )) == 0u)
    {
        d_internal_number(tree,
                          node,
                          "level",
                          item->level);
    }

    // every conditional around the item, the include guard excluded: the
    // guide says nothing of indenting inside one, so layout rules skip them
    d_internal_number(tree,
                      node,
                      "pp-depth",
                      item->pp_total);

    if ((first->flags & D_TOKEN_FLAG_LINE_START) != 0u)
    {
        (void)d_node_set_attribute(tree,
                                   node,
                                   "line-start",
                                   NULL);
    }

    // one node per token carries its blank lines: the outermost item
    if (_build->head_of[item->first] == _k)
    {
        d_internal_blank_attribute(_build, node, item->first);
    }

    // the comment run above names what the item declares
    if ( (item->name != D_SYNTAX_NONE) &&
         (d_internal_spell(_build, &_build->tokens[item->name]) != none) &&
         (d_internal_documented(_build, item->first, _build->spelling,
                                strlen(_build->spelling))) )
    {
        (void)d_node_set_attribute(tree, node, "documented", NULL);
    }

    // shown as its name, or as the token it begins with
    const struct d_token* const shown =
        (item->name != D_SYNTAX_NONE) ? &_build->tokens[item->name] : first;
    const size_t length = d_internal_spell(_build, shown);

    d_internal_geometry(tree,
                        node,
                        first,
                        first->span.length);

    if (length != none)
    {
        (void)d_node_set_text(tree,
                              node,
                              _build->spelling,
                              length);
    }

    ++_build->stats->items;

    return node;
}


/**
 * @brief Finds the node a token belongs beneath.
 *
 * Its innermost group and its innermost item both contain it, so whichever
 * began later is the inner one.  On a tie the group wins: an item that begins
 * with `{` holds that group, so the group is inside it.
 *
 * @param[in] _build  the build.
 * @param[in] _at     the token's index.
 * @param[in] _root   the node the tree hangs from.
 * @return the containing node.
 */
static uint32_t
d_internal_container(
    const struct d_internal_build* _build,
    uint32_t                       _at,
    uint32_t                       _root
)
{
    const uint32_t group = _build->groups.parent[_at];
    const uint32_t item  = _build->items.item_of[_at];

    if ( (item != D_SYNTAX_NONE)                              &&
         (_build->item_node[item] != D_DSS_NO_INDEX)           &&
         ( (group == D_SYNTAX_NONE) ||
           (_build->items.items[item].first > group) ) )
    {
        return _build->item_node[item];
    }

    if ( (group != D_SYNTAX_NONE) &&
         (_build->node_of[group] != D_DSS_NO_INDEX) )
    {
        return _build->node_of[group];
    }

    return _root;
}


/**
 * @brief Finds the node an item belongs beneath.
 *
 * A braceless body belongs to the control item that owns it; any other item
 * to the group its scope is, or to the root at file scope.
 *
 * @param[in] _build  the build.
 * @param[in] _k      the item's index.
 * @param[in] _root   the node the tree hangs from.
 * @return the containing node.
 */
static uint32_t
d_internal_item_container(
    const struct d_internal_build* _build,
    uint32_t                       _k,
    uint32_t                       _root
)
{
    const struct d_syntax_item* const item = &_build->items.items[_k];

    if ( (item->parent != D_SYNTAX_NONE) &&
         (_build->item_node[item->parent] != D_DSS_NO_INDEX) )
    {
        return _build->item_node[item->parent];
    }

    const uint32_t group = _build->groups.parent[item->first];

    if ( (group != D_SYNTAX_NONE) &&
         (_build->node_of[group] != D_DSS_NO_INDEX) )
    {
        return _build->node_of[group];
    }

    return _root;
}


/**
 * @brief Computes everything emission reads ahead of the token it is at.
 *
 * Brace depth, which a function's nesting reads across its whole body; the
 * identifiers declarations name; and, per token, the chain of items that
 * begin there, outermost first, which creation order already is.
 *
 * @param[in,out] _build  the build, its arrays allocated.
 */
static void
d_internal_prepare(
    struct d_internal_build* _build
)
{
    for (uint32_t at = 0u; at < (uint32_t)_build->count; ++at)
    {
        const uint32_t parent = _build->groups.parent[at];

        // brace depth: what indentation follows, parentheses excluded
        _build->depth[at] =
            (parent == D_SYNTAX_NONE)
            ? 0u
            : ( _build->depth[parent] +
                ((_build->tokens[parent].kind == D_TOKEN_BRACE_OPEN) ? 1u
                                                                     : 0u) );
        _build->node_of[at]    = D_DSS_NO_INDEX;
        _build->head_of[at]    = D_SYNTAX_NONE;
        _build->declarator[at] = 0u;
    }

    for (uint32_t k = 0u; k < (uint32_t)_build->items.count; ++k)
    {
        const struct d_syntax_item* const item = &_build->items.items[k];

        _build->item_node[k]  = D_DSS_NO_INDEX;
        _build->next_start[k] = D_SYNTAX_NONE;

        if (item->name != D_SYNTAX_NONE)
        {
            _build->declarator[item->name] = 1u;
        }

        // append to the chain at the item's first token
        uint32_t* link = &_build->head_of[item->first];

        while (*link != D_SYNTAX_NONE)
        {
            link = &_build->next_start[*link];
        }

        *link = k;
    }

    return;
}


//==============================================================================
// 4.  BUILDING
//==============================================================================


/**
 * @brief Lexes a source into a growing token array, comments included.
 *
 * @param[in,out] _build  the build to fill.
 * @return `true` if the source was lexed, `false` if memory ran out.
 */
static bool
d_internal_lex_all(
    struct d_internal_build* _build
)
{
    uint32_t capacity = 0u;

    for (;;)
    {
        // a token count is 32 bits, as every index into the array is
        if (_build->count >= (size_t)UINT32_MAX)
        {
            return false;
        }

        struct d_token* const grown =
            d_parse_grow(_build->tokens,
                         &capacity,
                         (uint32_t)_build->count + 1u,
                         (uint32_t)sizeof(struct d_token),
                         1);

        // grow before the array is full, so the next token always fits
        if (!grown)
        {
            return false;
        }

        _build->tokens = grown;

        if (!d_lex_next(_build->lexer, &_build->tokens[_build->count]))
        {
            break;
        }

        ++_build->count;
    }

    return true;
}


/**
 * @brief Releases everything a build holds except the tree.
 *
 * @param[in,out] _build  the build to release.
 */
static void
d_internal_release(
    struct d_internal_build* _build
)
{
    d_lex_destroy(_build->lexer);
    d_syntax_groups_free(&_build->groups);
    d_syntax_items_free(&_build->items);

    free(_build->tokens);
    free(_build->node_of);
    free(_build->depth);
    free(_build->spelling);
    free(_build->item_node);
    free(_build->head_of);
    free(_build->next_start);
    free(_build->declarator);
    free(_build->cond_macro);
    free(_build->cond_kind);
    free(_build->cond_hash);

    return;
}


/*
d_token_node_build
  Lexes, groups and emits a source beneath a parent node.
*/
uint32_t
d_token_node_build(
    struct d_node_tree*              _tree,
    uint32_t                         _parent,
    const struct d_lex_dialect*      _dialect,
    const struct d_token_node_input* _input,
    struct d_token_node_stats*       _out_stats
)
{
    struct d_token_node_stats scratch;
    struct d_internal_build   build;
    struct d_parse_diag_sink  counter;

    // parameter validation first
    if ( (!_tree)          ||
         (!_dialect)       ||
         (!_input)         ||
         (!_input->source) )
    {
        return D_DSS_NO_INDEX;
    }

    const struct d_source* const source = _input->source;

    memset(&build, 0, sizeof(build));
    memset(&scratch, 0, sizeof(scratch));

    build.tree   = _tree;
    build.source = source;
    build.stats  = _out_stats ? _out_stats : &scratch;

    memset(build.stats, 0, sizeof(*build.stats));

    build.lexer = d_lex_create(source, _dialect,
                               D_LEX_OPT_KEYWORDS | D_LEX_OPT_EMIT_COMMENTS);

    // with no sink of the caller's, a storage-less one still counts
    d_parse_diag_sink_init(&counter,
                           NULL,
                           0u,
                           NULL,
                           0u);

    struct d_parse_diag_sink* const sink   = _input->sink ? _input->sink
                                                         : &counter;
    const uint32_t                  before =
        d_parse_diag_tally(sink,
                           D_PARSE_SEVERITY_ERROR);

    d_lex_set_diagnostics(build.lexer,
                          sink);

    if ( (!build.lexer)                        ||
         (!d_internal_lex_all(&build))         ||
         (!d_syntax_group(build.tokens, build.count, &build.groups)) )
    {
        d_internal_release(&build);
        return D_DSS_NO_INDEX;
    }

    build.stats->tokens     = build.count;
    build.stats->lex_errors = d_parse_diag_tally(sink,
                                                 D_PARSE_SEVERITY_ERROR) -
                              before;
    build.stats->unbalanced = build.groups.unclosed +
                              build.groups.unopened +
                              build.groups.mismatched;

    // the items are recognized once the grouping exists
    if (!d_syntax_items_build(build.tokens,
                              build.count,
                              &build.groups,
                              source->text,
                              &build.items))
    {
        d_internal_release(&build);
        return D_DSS_NO_INDEX;
    }

    const size_t slots = build.count + 1u;
    const size_t items = build.items.count + 1u;

    build.node_of    = malloc(slots * sizeof(uint32_t));
    build.depth      = malloc(slots * sizeof(uint32_t));
    build.head_of    = malloc(slots * sizeof(uint32_t));
    build.declarator = malloc(slots * sizeof(uint8_t));
    build.item_node  = malloc(items * sizeof(uint32_t));
    build.next_start = malloc(items * sizeof(uint32_t));
    build.cond_macro = malloc(slots * sizeof(uint32_t));
    build.cond_kind  = malloc(slots * sizeof(uint8_t));
    build.cond_hash  = malloc(slots * sizeof(uint32_t));

    // any array missing ends the build
    if ( (!build.node_of)    ||
         (!build.depth)      ||
         (!build.head_of)    ||
         (!build.declarator) ||
         (!build.item_node)  ||
         (!build.next_start) ||
         (!build.cond_macro) ||
         (!build.cond_kind)  ||
         (!build.cond_hash) )
    {
        d_internal_release(&build);
        return D_DSS_NO_INDEX;
    }

    d_internal_prepare(&build);

    uint32_t root = _parent;

    // a host with no file node of its own still gets the element model
    if (root == D_DSS_NO_INDEX)
    {
        root = d_node_add(_tree, D_DSS_NO_INDEX, "file");

        if (root == D_DSS_NO_INDEX)
        {
            d_internal_release(&build);
            return D_DSS_NO_INDEX;
        }

        if (source->name)
        {
            (void)d_node_set_attribute(_tree, root, "path", source->name);
        }

        (void)d_node_set_attribute(_tree, root, "lang", _dialect->language);
        (void)d_node_set_attribute(_tree, root, "std",  _dialect->name);
    }

    for (uint32_t at = 0u; at < (uint32_t)build.count; ++at)
    {
        const struct d_token* const token = &build.tokens[at];

        // the items that begin here come first, outermost first
        for (uint32_t k = build.head_of[at];
             k != D_SYNTAX_NONE;
             k = build.next_start[k])
        {
            build.item_node[k] =
                d_internal_emit_item(&build,
                                     k,
                                     d_internal_item_container(&build,
                                                               k,
                                                               root));

            if (build.item_node[k] != D_DSS_NO_INDEX)
            {
                ++build.stats->nodes;
            }
        }

        // a paired closer is represented by its group
        if ( d_syntax_is_closer(token->kind) &&
             (build.groups.match[at] != D_SYNTAX_NONE) )
        {
            continue;
        }

        const uint32_t parent_node = d_internal_container(&build,
                                                          at,
                                                          root);

        if (d_syntax_is_directive_start(token))
        {
            build.node_of[at] = d_internal_emit_directive(&build,
                                                          at,
                                                          parent_node);
        }
        else if (d_syntax_is_opener(token->kind))
        {
            build.node_of[at] = d_internal_emit_group(&build,
                                                      at,
                                                      parent_node);
        }
        else
        {
            build.node_of[at] = d_internal_emit_leaf(&build,
                                                     at,
                                                     parent_node);
        }

        if (build.node_of[at] != D_DSS_NO_INDEX)
        {
            ++build.stats->nodes;
        }
    }

    d_internal_release(&build);

    return root;
}
