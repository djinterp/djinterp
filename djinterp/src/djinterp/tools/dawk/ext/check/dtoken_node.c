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
* path:      /src/djinterp/tools/dawk/ext/check/dtoken_node.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.23
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/check/dtoken_node.h"  // corresponding header
// std
#include <ctype.h>    // isalnum, isalpha, toupper
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint32_t
#include <stdio.h>    // snprintf
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy, memset, strlen

// djinterp
#include "../../../../../../inc/djinterp/parse/syntax/syntax_group.h"  // grouping
#include "../../../../../../inc/djinterp/parse/syntax/syntax_item.h"   // items
#include "../../../../../../inc/djinterp/parse/c/storage.h"            // d_parse_grow

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

// 1.1.3
// D_INTERNAL_TOC_INDENT_MAX
//   constant: the deepest indentation a table of contents' entries are told
// apart at; anything further in is counted with it.
#define D_INTERNAL_TOC_INDENT_MAX 32

// 1.1.4
// D_INTERNAL_ORDINAL_MAX
//   constant: the most letters a heading's ordinal is read to; the longest
// Roman numeral below a hundred, `LXXXVIII`, has eight.
#define D_INTERNAL_ORDINAL_MAX 8

// 1.1.5
// D_INTERNAL_LIST_MAX
//   constant: how far back through a list of one-letter headings one of them
// is asked what it counts as; a list has at most the alphabet's letters.
#define D_INTERNAL_LIST_MAX 32

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

    const struct d_lex_dialect* dialect;    // the language being read

    uint32_t*                  cond_macro;  // per open conditional: its macro
    uint8_t*                   cond_kind;   // per open conditional: its form
    uint32_t*                  cond_hash;   // per open conditional: its `#`
    uint8_t*                   cond_else;   // per open conditional: whether
                                            // its #else or an #elif is passed
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
 * @brief Reports whether a `#define` stands inside an `#ifndef` of its own
 *        name, at any depth, before that `#ifndef`'s `#else`.
 *
 * @note The configuration skeleton's form, where a choice stands between
 *       the guard and the definition:
 *
 *           #ifndef D_CFG_KNOB
 *               #if defined(D_CFG_ALL)
 *                   #define D_CFG_KNOB D_CFG_ALL
 *               #else
 *                   #define D_CFG_KNOB 0
 *               #endif
 *           #endif
 *
 *       Either definition supplies the name only if nobody else has, as
 *       `d_internal_define_guarded` asks of the line before a `#define`.
 *       The `#else` that is passed is the inner conditional's, not the
 *       guard's; past the guard's own, the name is one somebody defined.
 *
 * @param[in] _build  the build, at the `#define`: its open conditionals are
 *                    the ones around the directive.
 * @param[in] _macro  the defined macro's token index.
 * @return `true` if an open `#ifndef` tests this very name and is still in
 *         its first branch, `false` otherwise.
 */
static bool
d_internal_define_under_guard(
    const struct d_internal_build* _build,
    uint32_t                       _macro
)
{
    const struct d_token* const defined = &_build->tokens[_macro];

    for (uint32_t level = _build->cond_depth; level > 0u; --level)
    {
        const uint32_t tested = _build->cond_macro[level - 1u];

        // `#ifndef` is the third of the forms a level records
        if ( (_build->cond_kind[level - 1u] != 2u)  ||
             (_build->cond_else[level - 1u] != 0u)  ||
             (tested == D_SYNTAX_NONE) )
        {
            continue;
        }

        if ( (_build->tokens[tested].span.length == defined->span.length) &&
             (memcmp(_build->source->text
                         + _build->tokens[tested].span.offset,
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

        // a member defined outside its class is named with its class,
        // `integer_value::to_string`, and an operator with its symbol,
        // `integer_value::operator+=`
        if ( (last - first > _length + 2u)                         &&
             (memcmp(text + last - _length, _name, _length) == 0)  &&
             (text[last - _length - 1u] == ':')                    &&
             (text[last - _length - 2u] == ':') )
        {
            return true;
        }

        for (size_t at_word = first;
             (_length == 8u) && (memcmp(_name, "operator", 8u) == 0) &&
             ((at_word + _length) <= last);
             ++at_word)
        {
            if ( (memcmp(text + at_word, _name, _length) == 0) &&
                 ( (at_word == first) ||
                   ( (at_word >= first + 2u)        &&
                     (text[at_word - 1u] == ':')    &&
                     (text[at_word - 2u] == ':') ) ) )
            {
                return true;
            }
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

                // `...` among its parameters: a variadic macro
                for (uint32_t at = macro + 2u; at <= last; ++at)
                {
                    const char* const spelt =
                        d_token_spelling(_build->tokens[at].kind);

                    if ((spelt) && (strcmp(spelt, ")") == 0))
                    {
                        break;
                    }

                    if (_build->tokens[at].kind == D_TOKEN_ELLIPSIS)
                    {
                        (void)d_node_set_attribute(tree, node, "variadic",
                                                   NULL);
                    }
                }
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
         ( (d_internal_define_guarded(_build, _at, macro)) ||
           (d_internal_define_under_guard(_build, macro)) ) )
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
            _build->cond_else[_build->cond_depth]  = 0u;
            ++_build->cond_depth;
        }
    }

    // past its #else or an #elif, a conditional's first test no longer
    // says what is defined: `d_internal_define_under_guard` reads this
    if ( (_build->cond_depth > 0u) &&
         ( (d_internal_directive_is(_build, name, "else"))    ||
           (d_internal_directive_is(_build, name, "elif"))    ||
           (d_internal_directive_is(_build, name, "elifdef")) ||
           (d_internal_directive_is(_build, name, "elifndef")) ) )
    {
        _build->cond_else[_build->cond_depth - 1u] = 1u;
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

        // the macro the conditional tested, which is what its #endif names
        if ( (tested != D_SYNTAX_NONE) &&
             (d_internal_spell(_build, &_build->tokens[tested])
              != (size_t)-1) )
        {
            (void)d_node_set_attribute(tree, node, "symbol",
                                       _build->spelling);
        }

        if (named)
        {
            (void)d_node_set_attribute(tree, node, "named", NULL);
        }
    }

    d_internal_number(tree, node, "depth", _build->depth[_at]);
    d_internal_number(tree, node, "pp-depth", _build->items.pp_of[_at]);
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
        const uint32_t owner = _build->items.item_of[_at];
        const bool     known = ( (owner != D_SYNTAX_NONE) &&
                                 (owner < (uint32_t)_build->items.count) );

        d_internal_number(tree,
                          node,
                          "level",
                          _build->items.level_of[_at]);

        // and under the conditionals that item is under, which is what a
        // layout rule asks before it asks for a column.  Only a brace is
        // asked: a parameter list's parenthesis has rules of its own.  And
        // a block that is a statement by itself begins its own item, which
        // answers for the column: the brace is given none to answer with
        if ( (known)                                        &&
             (_build->tokens[_at].kind == D_TOKEN_BRACE_OPEN) &&
             (_build->items.items[owner].first != _at) )
        {
            const unsigned own   = _build->items.pp_of[_at];
            const unsigned outer = _build->items.items[owner].pp_total;

            d_internal_number(tree, node, "pp-depth",
                              (outer > own) ? outer : own);
        }
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
 * @brief Finds the standard that made a keyword one, from the lexer's own
 *        keyword table.
 *
 * @pre `_build->spelling` holds the keyword's spelling.
 *
 * @param[in] _build  the build.
 * @param[in] _kind   the keyword's token kind.
 * @return the level of the earliest row giving the spelling that kind, a
 *         year such as 2011; 0 if the table has no such row.
 */
static unsigned
d_internal_keyword_since(
    const struct d_internal_build* _build,
    int                            _kind
)
{
    const struct d_lex_dialect* const dialect = _build->dialect;
    unsigned                          since   = 0u;

    for (size_t at = 0u; (dialect) && (at < dialect->keyword_count); ++at)
    {
        const struct d_lex_keyword* const row = &dialect->keywords[at];

        if ( (row->kind == _kind)                         &&
             (row->since != D_LEX_NEVER)                  &&
             (strcmp(row->text, _build->spelling) == 0)   &&
             ((since == 0u) || (row->since < since)) )
        {
            since = row->since;
        }
    }

    return since;
}


/**
 * @brief Reports whether a token is a delimiter rule: a line comment holding
 *        nothing but a run of `=`, `-` or `/`.
 *
 * @param[in] _build  the build.
 * @param[in] _at     the token's index.
 * @return `true` for `//====`, `//----`, `// ====` and `//////`, each at
 *         least ten characters long.
 */
static bool
d_internal_comment_is_rule(
    const struct d_internal_build* _build,
    uint32_t                       _at
)
{
    const struct d_token* const token = &_build->tokens[_at];

    if ( (token->kind != D_TOKEN_COMMENT) ||
         ((token->flags & D_TOKEN_FLAG_COMMENT_LINE) == 0u) )
    {
        return false;
    }

    const char* const text = _build->source->text + token->span.offset;
    size_t            size = token->span.length;

    while ( (size > 0u) &&
            ( (text[size - 1u] == ' ') || (text[size - 1u] == '\r') ) )
    {
        --size;
    }

    size_t at = 0u;

    while ( (at < size) && (text[at] == '/') )
    {
        ++at;
    }

    while ( (at < size) && (text[at] == ' ') )
    {
        ++at;
    }

    // what follows the slashes is one character repeated to the end
    for (size_t scan = at; scan < size; ++scan)
    {
        if ( (text[scan] != '=') && (text[scan] != '-') &&
             (text[scan] != '/') )
        {
            return false;
        }
    }

    return (size >= 10u);
}

/**
 * @brief Reads a Roman numeral in its usual spelling, in either case.
 *
 * @param[in] _text  the letters, terminated.
 * @return its value, from 1 to 399; 0 when the letters are no numeral, or
 *         spell one in a way it is not written (`IIII`, `VX`, `IC`).
 */
static unsigned
d_internal_roman_value(
    const char* _text
)
{
    static const unsigned    values[]  = { 100u, 90u, 50u, 40u, 10u,
                                           9u, 5u, 4u, 1u };
    static const char* const letters[] = { "C", "XC", "L", "XL", "X",
                                           "IX", "V", "IV", "I" };

    unsigned total = 0u;
    size_t   at    = 0u;

    // the usual spelling is the greedy one: each step as often as it fits,
    // largest first, and none more than three times
    for (size_t which = 0u; which < (sizeof(values) / sizeof(values[0]));
         ++which)
    {
        const size_t length = strlen(letters[which]);
        unsigned     times  = 0u;

        while ( (times < 3u) &&
                (toupper((unsigned char)_text[at]) == letters[which][0]) &&
                ( (length == 1u) ||
                  (toupper((unsigned char)_text[at + 1u]) ==
                   letters[which][1]) ) )
        {
            total += values[which];
            at    += length;
            ++times;

            // a subtractive pair is written once
            if (length == 2u)
            {
                break;
            }
        }
    }

    return ( (at > 0u) && (_text[at] == '\0') ) ? total : 0u;
}

/**
 * @brief Finds what a comment's delimiter is drawn in, when one stands
 *        directly above it.
 *
 * @param[in] _build  the build.
 * @param[in] _at     the comment's token.
 * @return the character -- `=`, `-` or `/` -- or 0 when the line above is
 *         no delimiter: the shape a heading shares with the others of its
 *         list.
 */
static char
d_internal_rule_above(
    const struct d_internal_build* _build,
    uint32_t                       _at
)
{
    if ( (_at == 0u)                                          ||
         (!d_internal_comment_is_rule(_build, _at - 1u))      ||
         (_build->tokens[_at - 1u].end_line + 1u
          != _build->tokens[_at].span.line) )
    {
        return '\0';
    }

    const struct d_token* const rule = &_build->tokens[_at - 1u];
    const char* const           text = _build->source->text +
                                       rule->span.offset;
    size_t                      size = rule->span.length;

    // a delimiter ends in the character it is drawn in
    while ( (size > 1u) &&
            ( (text[size - 1u] == ' ') || (text[size - 1u] == '\r') ) )
    {
        --size;
    }

    return text[size - 1u];
}

/**
 * @brief Finds the letters a line comment opens with after its slashes,
 *        when a period ends them: `// C.  name`, `/// IV.  name`.
 *
 * @param[in]  _text       the comment's text, from its first slash.
 * @param[in]  _size       its length.
 * @param[out] _out_first  where the letters begin.
 * @param[out] _out_apart  whether two spaces follow the period, or nothing
 *                         does.
 * @return the count of letters; 0 when the comment does not open with two
 *         or three slashes, a space, letters, a period and then a space or
 *         its end.
 */
static size_t
d_internal_ordinal_span(
    const char* _text,
    size_t      _size,
    size_t*     _out_first,
    bool*       _out_apart
)
{
    size_t at = 0u;

    while ( (_size > 0u) &&
            ( (_text[_size - 1u] == ' ') || (_text[_size - 1u] == '\r') ) )
    {
        --_size;
    }

    while ( (at < _size) && (_text[at] == '/') )
    {
        ++at;
    }

    const size_t slashes = at;

    while ( (at < _size) && (_text[at] == ' ') )
    {
        ++at;
    }

    const size_t first = at;

    while ( (at < _size) && (isalpha((unsigned char)_text[at]) != 0) )
    {
        ++at;
    }

    if ( (slashes < 2u) || (slashes > 3u) || (first == slashes)      ||
         (at == first) || (at >= _size) || (_text[at] != '.')        ||
         ( ((at + 1u) < _size) && (_text[at + 1u] != ' ') ) )
    {
        return 0u;
    }

    *_out_first = first;
    *_out_apart = ( ((at + 1u) >= _size) ||
                    ( ((at + 2u) < _size) && (_text[at + 2u] == ' ') ) );

    return (at - first);
}

/**
 * @brief Reads the ordinal a heading comment opens with, when it is spelled
 *        in letters: `// C.  name`, `/// IV.  name`, `// i.  name`.
 *
 * @note A heading's ordinal is one letter, or a Roman numeral in its usual
 *       spelling: `// Note. the rest` opens with neither.  Under a delimiter
 *       that is enough.  With none above it a heading stands two spaces from
 *       its name, as the older headings over a run of declarations do, and a
 *       line with one is a sentence or an entry of a list in prose.
 *
 * @param[in]  _build     the build.
 * @param[in]  _at        a token's index.
 * @param[out] _out       the ordinal's letters, terminated; room for
 *                        D_INTERNAL_ORDINAL_MAX of them and the terminator.
 * @param[out] _out_rule  what the delimiter directly above the comment is
 *                        drawn in, or 0 (d_internal_rule_above).
 * @return the count of letters, or 0 when the token is not a line comment
 *         that opens with such an ordinal.
 */
static size_t
d_internal_letter_ordinal(
    const struct d_internal_build* _build,
    uint32_t                       _at,
    char*                          _out,
    char*                          _out_rule
)
{
    const struct d_token* const token = &_build->tokens[_at];
    size_t                      first = 0u;
    bool                        apart = false;

    if ( (token->kind != D_TOKEN_COMMENT) ||
         ((token->flags & D_TOKEN_FLAG_COMMENT_LINE) == 0u) )
    {
        return 0u;
    }

    const char* const text   = _build->source->text + token->span.offset;
    const size_t      length = d_internal_ordinal_span(text,
                                                       token->span.length,
                                                       &first, &apart);

    if ( (length == 0u) || (length > D_INTERNAL_ORDINAL_MAX) )
    {
        return 0u;
    }

    memcpy(_out, text + first, length);
    _out[length] = '\0';
    *_out_rule   = d_internal_rule_above(_build, _at);

    // one letter or a numeral; and with no delimiter above, apart from its
    // name
    if ( ( (length > 1u) && (d_internal_roman_value(_out) == 0u) ) ||
         ( (*_out_rule == '\0') && (!apart) ) )
    {
        _out[0] = '\0';

        return 0u;
    }

    return length;
}

/**
 * @brief Finds the heading before one in its own list: the nearest above it
 *        whose ordinal is spelled in letters, under the same delimiter or
 *        under none as this one is, and in the same case.
 *
 * @param[in]  _build  the build.
 * @param[in]  _at     the heading's token.
 * @param[out] _out    that heading's ordinal, terminated; empty when there
 *                     is none.
 * @return that heading's token, or `_at` itself when there is none.
 */
static uint32_t
d_internal_ordinal_before(
    const struct d_internal_build* _build,
    uint32_t                       _at,
    char*                          _out
)
{
    char own[D_INTERNAL_ORDINAL_MAX + 1u] = "";
    char rule                             = '\0';

    _out[0] = '\0';

    if (d_internal_letter_ordinal(_build, _at, own, &rule) == 0u)
    {
        return _at;
    }

    for (uint32_t at = _at; at > 0u; --at)
    {
        char shape = '\0';

        if ( (d_internal_letter_ordinal(_build, at - 1u, _out,
                                        &shape) != 0u)            &&
             (shape == rule)                                      &&
             ((_out[0] <= 'Z') == (own[0] <= 'Z')) )
        {
            return (at - 1u);
        }

        _out[0] = '\0';
    }

    return _at;
}

/**
 * @brief Reports what a heading's ordinal counts as, when it is spelled in
 *        letters: its value as a Roman numeral, or 0 when it is a letter.
 *
 * @note `C.` is the hundredth of a list numbered in Roman numerals and the
 *       third of one numbered in letters, and so with `I`, `L`, `V` and `X`.
 *       The heading before it in its list says which:
 *       - `L` and `C` are numerals only after the numeral before them,
 *         `XLIX.` and `XCIX.`: fifty sections are not skipped to, and a
 *         list that reaches `L.` by letters, or skips to it, has letters.
 *       - `I`, `V` and `X` are numerals when no heading stands before them,
 *         or a numeral does: whichever, since a list of numerals may skip
 *         one, begin again, or have two the wrong way round.  After a
 *         letter they are letters: `H. I.`, `U. V.`, and whatever follows
 *         `A. B. C.`.
 *       A `C.` that only opens a line of prose is so a letter.  An `I.`,
 *       `V.` or `X.` that does, with two spaces after it, is still taken
 *       for a heading.
 *
 * @param[in] _build  the build.
 * @param[in] _at     a comment's token.
 * @param[in] _depth  how many headings have been asked on the way here; the
 *                    one beyond D_INTERNAL_LIST_MAX is taken as its letters
 *                    spell.
 * @return the value; 0 for a letter and for a token that opens with no
 *         ordinal in letters.
 */
static unsigned
d_internal_ordinal_value(
    const struct d_internal_build* _build,
    uint32_t                       _at,
    unsigned                       _depth
)
{
    char own[D_INTERNAL_ORDINAL_MAX + 1u]    = "";
    char before[D_INTERNAL_ORDINAL_MAX + 1u] = "";
    char rule                                = '\0';

    const size_t   length = d_internal_letter_ordinal(_build, _at, own,
                                                      &rule);
    const unsigned value  = (length != 0u) ? d_internal_roman_value(own)
                                           : 0u;

    // more than one letter says what it is by itself
    if ( (length != 1u) || (value == 0u) || (_depth >= D_INTERNAL_LIST_MAX) )
    {
        return value;
    }

    const uint32_t previous = d_internal_ordinal_before(_build, _at, before);

    if (value >= 50u)
    {
        return (d_internal_roman_value(before) == (value - 1u)) ? value : 0u;
    }

    if (previous == _at)
    {
        return value;
    }

    const unsigned earlier = d_internal_ordinal_value(_build, previous,
                                                      _depth + 1u);

    return (earlier != 0u) ? value : 0u;
}


/**
 * @brief Reports whether a heading's ordinal is one letter that counts as a
 *        letter and not as a Roman numeral.
 *
 * @note Only an ordinal that is a single letter and nothing more is asked:
 *       `C.  name`.  One with a second part -- `VII.1`, `I.a`, `C.II.` --
 *       is read by its first run of numeral letters, as it always was.
 *
 * @param[in] _build  the build.
 * @param[in] _at     a comment's token.
 * @return `true` if the comment opens with one letter, a period and then a
 *         space or its end, and d_internal_ordinal_value counts the letter
 *         as none of a numeral; `false` otherwise.
 */
static bool
d_internal_ordinal_is_letter(
    const struct d_internal_build* _build,
    uint32_t                       _at
)
{
    char own[D_INTERNAL_ORDINAL_MAX + 1u] = "";
    char rule                             = '\0';

    return ( (d_internal_letter_ordinal(_build, _at, own, &rule) == 1u) &&
             (d_internal_ordinal_value(_build, _at, 0u) == 0u) );
}

/**
 * @brief Marks a line comment that is one of the guide's structural markers.
 *
 * @note A marker is recognised by its whole shape, never by its number
 *       alone, since prose opens with numbers too: `// 3.38.0 the JSON
 *       functions are built in` is a sentence.  An item or a fourth-level
 *       marker is its number and nothing else; a section sits under a
 *       delimiter rule; a decimal subsection sits over one.
 *
 * @param[in,out] _build  the build.
 * @param[in]     _at     the comment's token.
 * @param[in]     _node   its node, which takes `marker` (section,
 *                        subsection, item or entry) and `numeral` (decimal
 *                        or roman).
 */
static void
d_internal_comment_marker(
    struct d_internal_build* _build,
    uint32_t                 _at,
    uint32_t                 _node
)
{
    const struct d_token* const token = &_build->tokens[_at];
    const char* const           text  = _build->source->text +
                                        token->span.offset;
    size_t                      size  = token->span.length;

    while ( (size > 0u) &&
            ( (text[size - 1u] == ' ') || (text[size - 1u] == '\r') ||
              (text[size - 1u] == '/') ) )
    {
        --size;
    }

    size_t at = 0u;

    while ( (at < size) && (text[at] == '/') )
    {
        ++at;
    }

    const size_t slashes = at;

    while ( (at < size) && (text[at] == ' ') )
    {
        ++at;
    }

    // `//` or `///`, a space, and then the ordinal
    if ( (slashes < 2u) || (slashes > 3u) || (at == slashes) || (at >= size) )
    {
        return;
    }

    const size_t ordinal = at;
    unsigned     parts   = 0u;
    bool         dotted  = false;
    const char*  numeral = "decimal";

    if ( (text[at] >= '0') && (text[at] <= '9') )
    {
        // digits, and further digits after each period
        while (at < size)
        {
            const size_t digits = at;

            while ( (at < size) && (text[at] >= '0') && (text[at] <= '9') )
            {
                ++at;
            }

            if (at == digits)
            {
                break;
            }

            ++parts;
            dotted = false;

            if ( (at < size) && (text[at] == '.') )
            {
                dotted = true;
                ++at;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        const bool upper = (strchr("IVXLC", text[at]) != NULL);

        while ( (at < size) &&
                (strchr(upper ? "IVXLC" : "ivx", text[at]) != NULL) )
        {
            ++at;
        }

        // a Roman ordinal ends in its period; and one that is a single
        // letter and no more is not a letter of a list that counts in
        // letters
        if ( (at == ordinal) || (at >= size) || (text[at] != '.')       ||
             (d_internal_ordinal_is_letter(_build, _at)) )
        {
            return;
        }

        ++at;
        parts   = upper ? 1u : 2u;
        dotted  = true;
        numeral = "roman";
    }

    const size_t after = at;

    while ( (at < size) && (text[at] == ' ') )
    {
        ++at;
    }

    const bool   named = (at < size);
    const size_t gap   = at - after;
    const bool   under = ( (_at > 0u)                                    &&
                           (d_internal_comment_is_rule(_build, _at - 1u)) &&
                           (_build->tokens[_at - 1u].end_line + 1u
                            == token->span.line) );
    const bool   over  = ( ((size_t)_at + 1u < _build->count)            &&
                           (d_internal_comment_is_rule(_build, _at + 1u)) );
    const char*  marker = NULL;

    if (numeral[0] == 'r')
    {
        // `// II.  NAME` under a rule is a section; the older headings
        // over a run of declarations, `// I.    creation` and
        // `// i.    creation`, stand apart from their names by two spaces
        if ( (parts == 1u) && (named) && (under) )
        {
            marker = "section";
        }
        else if ( (named) && (gap >= 2u) && (slashes == 2u) )
        {
            marker = "subsection";
        }
    }
    else if ( (parts == 4u) && (!dotted) && (!named) && (after == size) )
    {
        marker = "entry";
    }
    else if ( (parts == 3u) && (!dotted) && (!named) && (after == size) )
    {
        marker = "item";
    }
    else if ( (parts == 2u) && (!dotted) && (named) && (gap >= 2u) &&
              (over) )
    {
        marker = "subsection";
    }
    else if ( (parts == 1u) && (dotted) && (named) && (under) )
    {
        marker = "section";
    }

    if (marker)
    {
        (void)d_node_set_attribute(_build->tree, _node, "marker", marker);
        (void)d_node_set_attribute(_build->tree, _node, "numeral", numeral);
    }

    return;
}


/**
 * @brief Marks a block comment that is a table of contents, and counts what
 *        the guide's format forbids inside one.
 *
 * @param[in,out] _build  the build.
 * @param[in]     _at     the comment's token.
 * @param[in]     _node   its node, which takes `toc`, and as numbers
 *                        `lettered` (entries whose ordinal is one lower-case
 *                        letter that is no numeral), `roman` (entries with
 *                        a Roman ordinal) and `blank` (empty lines between
 *                        two entries).  Which one-letter ordinals are
 *                        numerals is decided as it is for a heading, by the
 *                        entry before at the same indentation.
 */
static void
d_internal_comment_toc(
    struct d_internal_build* _build,
    uint32_t                 _at,
    uint32_t                 _node
)
{
    static const char heading[] = "TABLE OF CONTENTS";

    const struct d_token* const token = &_build->tokens[_at];
    const char* const           text  = _build->source->text +
                                        token->span.offset;
    const size_t                size  = token->span.length;
    size_t                      at    = 0u;

    // the opener and the space after it
    while ( (at < size) &&
            ( (text[at] == '/') || (text[at] == '*') || (text[at] == ' ') ||
              (text[at] == '\r') || (text[at] == '\n') ) )
    {
        ++at;
    }

    if ( (size - at < sizeof(heading) - 1u) ||
         (memcmp(text + at, heading, sizeof(heading) - 1u) != 0) )
    {
        return;
    }

    // what the last entry at each indentation counted as -- a numeral's
    // value, 0 for a letter -- and whether there was one: the next entry
    // there is read by it, as a heading is by the heading before it
    unsigned last[D_INTERNAL_TOC_INDENT_MAX + 1u] = { 0u };
    bool     seen[D_INTERNAL_TOC_INDENT_MAX + 1u] = { false };
    size_t lettered = 0u;
    size_t roman    = 0u;
    size_t blank    = 0u;
    size_t pending  = 0u;
    bool   begun    = false;

    while (at < size)
    {
        // one line: its first and last characters that are not space
        size_t end = at;

        while ( (end < size) && (text[end] != '\n') )
        {
            ++end;
        }

        size_t first = at;

        while ( (first < end) &&
                ( (text[first] == ' ') || (text[first] == '\r') ) )
        {
            ++first;
        }

        const size_t indent = ((first - at) < D_INTERNAL_TOC_INDENT_MAX)
                            ? (first - at)
                            : D_INTERNAL_TOC_INDENT_MAX;

        at = end + 1u;

        // the closer's line holds no entry
        if ( (first < end) && (text[first] == '*') )
        {
            continue;
        }

        if (first == end)
        {
            pending += begun ? 1u : 0u;

            continue;
        }

        // an empty line counts once an entry follows it
        blank   += pending;
        pending  = 0u;
        begun    = true;

        size_t scan = first;

        while ( (scan < end) && (text[scan] != '.') && (text[scan] != ' ') )
        {
            ++scan;
        }

        // an ordinal is a word ending in a period, with a name after it
        if ( (scan == first) || (scan >= end) || (text[scan] != '.') ||
             (scan + 1u >= end) || (text[scan + 1u] != ' ') )
        {
            continue;
        }

        const size_t length = scan - first;
        bool         upper  = true;
        bool         lower  = true;

        bool         alpha  = true;

        for (size_t which = first; which < scan; ++which)
        {
            upper = (upper) && (strchr("IVXLC", text[which]) != NULL);
            lower = (lower) && (strchr("ivx", text[which]) != NULL);
            alpha = (alpha) && (isalpha((unsigned char)text[which]) != 0);
        }

        // as a heading's: an ordinal of several numeral letters is one;
        // a single letter is a numeral by the entry before it at its own
        // indentation (d_internal_ordinal_value)
        char     word[D_INTERNAL_ORDINAL_MAX + 1u] = "";
        unsigned value                             = 0u;

        if ( ((upper) || (lower)) && (length <= D_INTERNAL_ORDINAL_MAX) )
        {
            memcpy(word, text + first, length);
            value = d_internal_roman_value(word);
        }

        if ( (length == 1u) && (value >= 50u) )
        {
            value = (last[indent] == (value - 1u)) ? value : 0u;
        }
        else if ( (length == 1u) && (seen[indent]) && (last[indent] == 0u) )
        {
            value = 0u;
        }

        // an entry numbered in digits is of no list of letters
        if (alpha)
        {
            last[indent] = value;
            seen[indent] = true;
        }

        if ( (length > 1u) ? ((upper) || (lower)) : (value != 0u) )
        {
            ++roman;
        }
        else if ( (length == 1u) && (text[first] >= 'a') &&
                  (text[first] <= 'z') )
        {
            ++lettered;
        }
    }

    (void)d_node_set_attribute(_build->tree, _node, "toc", NULL);
    d_internal_number(_build->tree, _node, "lettered", lettered);
    d_internal_number(_build->tree, _node, "roman", roman);
    d_internal_number(_build->tree, _node, "blank", blank);

    return;
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

    // the conditionals around the token, the include guard excluded
    d_internal_number(tree, node, "pp-depth", _build->items.pp_of[_at]);

    // a keyword newer than its language's first standard says which
    // standard made it one: above C89 in C, above C++98 in C++
    if (strcmp(d_internal_type_of(token->kind), "keyword") == 0)
    {
        const unsigned since = d_internal_keyword_since(_build, token->kind);
        const unsigned first =
            ( (_build->dialect) &&
              (strcmp(_build->dialect->language, "c") == 0) ) ? D_LEX_C89
                                                               : D_LEX_CPP98;

        if (since > first)
        {
            d_internal_number(tree, node, "since", since);
        }
    }

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

        // the guide's structure: a marker is a line of its own, a table of
        // contents a block
        if ((token->flags & D_TOKEN_FLAG_COMMENT_LINE) == 0u)
        {
            d_internal_comment_toc(_build, _at, node);
        }
        else if ((token->flags & D_TOKEN_FLAG_LINE_START) != 0u)
        {
            d_internal_comment_marker(_build, _at, node);
        }
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


// D_INTERNAL_CONTRACT_MAX
//   constant: the parameters, and the @param tags, one contract is read for;
//   a function past it is counted to this many.
#define D_INTERNAL_CONTRACT_MAX 32

/**
 * @brief Reads the contract block directly above a function, and records how
 *        its tags cover the function's parameters.
 *
 * @note A contract is a block comment opening with a slash and two stars,
 *       beginning its own line and ending on the line just above the
 *       function.  The banner, which opens with a longer run of stars, is
 *       not one.  A tag opens its line: `@param` met inside a sentence is
 *       prose about tags.
 *
 * @param[in,out] _build  the build.
 * @param[in]     _k      the function's item index.
 * @param[in]     _node   the function's node, which takes `contract`; as
 *                        numbers `undocumented` (named parameters no @param
 *                        names), `undirected` (@param tags with no
 *                        direction) and `strangers` (@param tags naming no
 *                        parameter); and `return-tag` when the block holds
 *                        `@return` or `@retval`.
 */
static void
d_internal_contract(
    struct d_internal_build* _build,
    uint32_t                 _k,
    uint32_t                 _node
)
{
    const struct d_syntax_item* const item = &_build->items.items[_k];

    if (item->first == 0u)
    {
        return;
    }

    const struct d_token* const first   = &_build->tokens[item->first];
    const struct d_token* const comment = &_build->tokens[item->first - 1u];

    if ( (comment->kind != D_TOKEN_COMMENT)                         ||
         ((comment->flags & D_TOKEN_FLAG_COMMENT_LINE) != 0u)       ||
         ((comment->flags & D_TOKEN_FLAG_LINE_START) == 0u)         ||
         (comment->end_line + 1u != first->span.line)               ||
         (comment->span.length < 5u) )
    {
        return;
    }

    const char* const text = _build->source->text + comment->span.offset;
    const size_t      size = comment->span.length;

    if ( (memcmp(text, "/**", 3u) != 0) || (text[3] == '*') )
    {
        return;
    }

    // the tags: each @param's name, and whether a return is described
    size_t tag_at[D_INTERNAL_CONTRACT_MAX];
    size_t tag_length[D_INTERNAL_CONTRACT_MAX];
    size_t tags       = 0u;
    size_t undirected = 0u;
    bool   returns    = false;

    bool   fresh      = true;

    for (size_t at = 0u; at + 1u < size; ++at)
    {
        if (text[at] == '\n')
        {
            fresh = true;

            continue;
        }

        // the comment's own margin: space and stars
        if ( (text[at] == ' ') || (text[at] == '*') || (text[at] == '\r') ||
             (text[at] == '/') )
        {
            continue;
        }

        const bool opens = fresh;

        fresh = false;

        if ( (text[at] != '@') || (!opens) )
        {
            continue;
        }

        size_t end = at + 1u;

        while ( (end < size) && (text[end] >= 'a') && (text[end] <= 'z') )
        {
            ++end;
        }

        const size_t word = end - at - 1u;

        if ( ((word == 6u) && (memcmp(text + at + 1u, "return", 6u) == 0))  ||
             ((word == 7u) && (memcmp(text + at + 1u, "returns", 7u) == 0)) ||
             ((word == 6u) && (memcmp(text + at + 1u, "retval", 6u) == 0)) )
        {
            returns = true;
        }

        if ( (word != 5u) || (memcmp(text + at + 1u, "param", 5u) != 0) )
        {
            continue;
        }

        // `[in]`, `[out]` or `[in,out]`, hard against the tag
        if ( (end < size) && (text[end] == '[') )
        {
            while ( (end < size) && (text[end] != ']') &&
                    (text[end] != '\n') )
            {
                ++end;
            }

            end += ((end < size) && (text[end] == ']')) ? 1u : 0u;
        }
        else
        {
            ++undirected;
        }

        while ( (end < size) && (text[end] == ' ') )
        {
            ++end;
        }

        const size_t name = end;

        while ( (end < size) &&
                ( (text[end] == '_')                           ||
                  ((text[end] >= 'a') && (text[end] <= 'z'))   ||
                  ((text[end] >= 'A') && (text[end] <= 'Z'))   ||
                  ((text[end] >= '0') && (text[end] <= '9')) ) )
        {
            ++end;
        }

        if ( (end > name) && (tags < D_INTERNAL_CONTRACT_MAX) )
        {
            tag_at[tags]     = name;
            tag_length[tags] = end - name;
            ++tags;
        }
    }

    // the function's own parameters: an item begun directly inside its list
    uint32_t names[D_INTERNAL_CONTRACT_MAX];
    size_t   named = 0u;

    if ( (item->params != D_SYNTAX_NONE) &&
         (_build->groups.match[item->params] != D_SYNTAX_NONE) )
    {
        for (uint32_t at = item->params + 1u;
             at < _build->groups.match[item->params];
             ++at)
        {
            const uint32_t owner = _build->items.item_of[at];

            if ( (owner == D_SYNTAX_NONE)                                 ||
                 (_build->items.items[owner].kind != D_SYNTAX_ITEM_PARAM) ||
                 (_build->items.items[owner].first != at)                 ||
                 (_build->groups.parent[at] != item->params)              ||
                 (_build->items.items[owner].name == D_SYNTAX_NONE)       ||
                 (named == D_INTERNAL_CONTRACT_MAX) )
            {
                continue;
            }

            names[named] = _build->items.items[owner].name;
            ++named;
        }
    }

    size_t undocumented = 0u;
    size_t strangers    = tags;
    bool   met[D_INTERNAL_CONTRACT_MAX] = { false };

    for (size_t which = 0u; which < named; ++which)
    {
        const struct d_token* const name   = &_build->tokens[names[which]];
        const char* const           spelt  = _build->source->text +
                                             name->span.offset;
        bool                        tagged = false;

        for (size_t tag = 0u; tag < tags; ++tag)
        {
            if ( (tag_length[tag] == name->span.length) &&
                 (memcmp(text + tag_at[tag], spelt, tag_length[tag]) == 0) )
            {
                tagged = true;

                // a tag is a stranger until some parameter answers to it
                if (!met[tag])
                {
                    met[tag] = true;
                    --strangers;
                }
            }
        }

        undocumented += tagged ? 0u : 1u;
    }

    (void)d_node_set_attribute(_build->tree, _node, "contract", NULL);
    d_internal_number(_build->tree, _node, "undocumented", undocumented);
    d_internal_number(_build->tree, _node, "undirected", undirected);
    d_internal_number(_build->tree, _node, "strangers", strangers);

    if (returns)
    {
        (void)d_node_set_attribute(_build->tree, _node, "return-tag", NULL);
    }

    return;
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
        ((item->flags & D_SYNTAX_ITEM_FLAG_NO_RETURN) != 0u)    ? "none"
      : ((item->flags & D_SYNTAX_ITEM_FLAG_RETURNS_VOID) != 0u) ? "void"
                                                                : "value");
    d_internal_number(tree,
                      _node,
                      "params",
                      facts.params);

    // the contract block above it, when it carries one
    d_internal_contract(_build, _k, _node);

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

    // the line the item's last token is on: a repair that moves an item
    // moves every line of it
    if (item->last < _build->count)
    {
        d_internal_number(tree,
                          node,
                          "end-line",
                          _build->tokens[item->last].span.line);
    }

    if ((first->flags & D_TOKEN_FLAG_LINE_START) != 0u)
    {
        (void)d_node_set_attribute(tree,
                                   node,
                                   "line-start",
                                   NULL);
    }

    // its last token begins a line too: a statement that ends in a brace
    // on a line of its own, as the guide writes one, and not on one line
    if ( (item->last < _build->count) &&
         ((_build->tokens[item->last].flags & D_TOKEN_FLAG_LINE_START)
          != 0u) )
    {
        (void)d_node_set_attribute(tree,
                                   node,
                                   "end-line-start",
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
    free(_build->cond_else);

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

    build.tree    = _tree;
    build.source  = source;
    build.dialect = _dialect;
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
    build.cond_else  = malloc(slots * sizeof(uint8_t));

    // any array missing ends the build
    if ( (!build.node_of)    ||
         (!build.depth)      ||
         (!build.head_of)    ||
         (!build.declarator) ||
         (!build.item_node)  ||
         (!build.next_start) ||
         (!build.cond_macro) ||
         (!build.cond_kind)  ||
         (!build.cond_hash)  ||
         (!build.cond_else) )
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
