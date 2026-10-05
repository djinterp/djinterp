/*******************************************************************************
* djinterp [parse]                                                syntax_group.c
*
* Definitions for the non-inline declarations in syntax_group.h.
*   One forward pass.  `open` names the innermost unclosed group, and
* `parent[open]` the one around it, so pushing is an assignment and popping
* is a read: the stack lives in the output and costs nothing to keep.
*
*
* path:      /src/djinterp/parse/syntax/syntax_group.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.02
*******************************************************************************/
#include "../../../../inc/djinterp/parse/syntax/syntax_group.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"              // uint32_t


//==============================================================================
// 1.  CLASSIFICATION
//==============================================================================


/*
d_syntax_is_opener
  Reports whether a kind opens a bracket group.
*/
bool
d_syntax_is_opener(
    int _kind
)
{
    return ( (_kind == D_TOKEN_BRACE_OPEN)   ||
             (_kind == D_TOKEN_BRACKET_OPEN) ||
             (_kind == D_TOKEN_PAREN_OPEN) );
}


/*
d_syntax_is_closer
  Reports whether a kind closes a bracket group.
*/
bool
d_syntax_is_closer(
    int _kind
)
{
    return ( (_kind == D_TOKEN_BRACE_CLOSE)   ||
             (_kind == D_TOKEN_BRACKET_CLOSE) ||
             (_kind == D_TOKEN_PAREN_CLOSE) );
}


/*
d_syntax_closer_for
  Reports the kind that closes an opener.
*/
int
d_syntax_closer_for(
    int _opener_kind
)
{
    switch (_opener_kind)
    {
        case D_TOKEN_BRACE_OPEN:   return D_TOKEN_BRACE_CLOSE;
        case D_TOKEN_BRACKET_OPEN: return D_TOKEN_BRACKET_CLOSE;
        case D_TOKEN_PAREN_OPEN:   return D_TOKEN_PAREN_CLOSE;
        default:                   return D_TOKEN_END;
    }
}


/*
d_syntax_is_directive_start
  Reports whether a token is the `#` that opens a directive line.  A `#`
inside a macro body is the stringizing operator, not a directive, and is told
apart by standing first on its line.
*/
bool
d_syntax_is_directive_start(
    const struct d_token* _token
)
{
    return ( _token &&
             (_token->kind == D_TOKEN_HASH) &&
             ((_token->flags & D_TOKEN_FLAG_LINE_START)   != 0u) &&
             ((_token->flags & D_TOKEN_FLAG_IN_DIRECTIVE) != 0u) );
}


//==============================================================================
// 2.  GROUPING
//==============================================================================


/**
 * @brief Ends the directive whose `#` is at `_directive`, recording its last
 *        token and discarding whatever brackets it left open.
 *
 * @param[in,out] _groups     the arrays being filled.
 * @param[in]     _directive  the directive's `#` index.
 * @param[in]     _last       the directive's last token index.
 * @return the group that encloses the directive, which becomes the open group.
 */
static uint32_t
d_internal_close_directive(
    struct d_syntax_groups* _groups,
    uint32_t                _directive,
    uint32_t                _last
)
{
    _groups->match[_directive] = _last;

    return _groups->parent[_directive];
}


/*
d_syntax_group
  Describes the bracket and directive structure of a token array.
*/
bool
d_syntax_group(
    const struct d_token*   _tokens,
    size_t                  _count,
    struct d_syntax_groups* _out_groups
)
{
    // parameter validation first
    if ((!_out_groups) || ((!_tokens) && (_count > 0u)))
    {
        return false;
    }

    _out_groups->count      = _count;
    _out_groups->parent     = NULL;
    _out_groups->match      = NULL;
    _out_groups->unclosed   = 0u;
    _out_groups->unopened   = 0u;
    _out_groups->mismatched = 0u;

    // nothing to describe
    if (_count == 0u)
    {
        return true;
    }

    _out_groups->parent = malloc(_count * sizeof(uint32_t));
    _out_groups->match  = malloc(_count * sizeof(uint32_t));

    // the arrays could not be held
    if ((!_out_groups->parent) || (!_out_groups->match))
    {
        d_syntax_groups_free(_out_groups);
        return false;
    }

    uint32_t open      = D_SYNTAX_NONE;
    uint32_t directive = D_SYNTAX_NONE;

    for (uint32_t at = 0u; at < (uint32_t)_count; ++at)
    {
        const struct d_token* const token = &_tokens[at];

        // a directive ends at the first token not on its line
        if ( (directive != D_SYNTAX_NONE) &&
             ( ((token->flags & D_TOKEN_FLAG_IN_DIRECTIVE) == 0u) ||
               d_syntax_is_directive_start(token) ) )
        {
            open      = d_internal_close_directive(_out_groups, directive,
                                                   at - 1u);
            directive = D_SYNTAX_NONE;
        }

        _out_groups->parent[at] = open;
        _out_groups->match[at]  = D_SYNTAX_NONE;

        // a directive opens a group of its own
        if (d_syntax_is_directive_start(token))
        {
            directive = at;
            open      = at;
            continue;
        }

        // an opener is pushed by naming it
        if (d_syntax_is_opener(token->kind))
        {
            open = at;
            continue;
        }

        // a closer pairs with the nearest opener of its kind in scope
        if (d_syntax_is_closer(token->kind))
        {
            uint32_t partner = open;

            while ( (partner != D_SYNTAX_NONE) && (partner != directive) &&
                    (d_syntax_closer_for(_tokens[partner].kind) !=
                     token->kind) )
            {
                partner = _out_groups->parent[partner];
            }

            // no opener of this kind is in scope: the closer is a leaf
            if ((partner == D_SYNTAX_NONE) || (partner == directive))
            {
                if (directive == D_SYNTAX_NONE)
                {
                    ++_out_groups->unopened;
                }

                continue;
            }

            // closing past an unclosed opener leaves that opener unpaired
            if ((partner != open) && (directive == D_SYNTAX_NONE))
            {
                ++_out_groups->mismatched;
            }

            _out_groups->match[partner] = at;
            _out_groups->match[at]      = partner;
            _out_groups->parent[at]     = _out_groups->parent[partner];

            open = _out_groups->parent[partner];
        }
    }

    // a directive may be the last line of the file
    if (directive != D_SYNTAX_NONE)
    {
        open = d_internal_close_directive(_out_groups, directive,
                                          (uint32_t)(_count - 1u));
    }

    // whatever remains open outside a directive was never closed
    while (open != D_SYNTAX_NONE)
    {
        if (d_syntax_is_opener(_tokens[open].kind))
        {
            ++_out_groups->unclosed;
        }

        open = _out_groups->parent[open];
    }

    return true;
}


/*
d_syntax_groups_free
  Releases the arrays of a description.  The structure itself is the
caller's.
*/
void
d_syntax_groups_free(
    struct d_syntax_groups* _groups
)
{
    // parameter validation first
    if (!_groups)
    {
        return;
    }

    free(_groups->parent);
    free(_groups->match);

    _groups->parent = NULL;
    _groups->match  = NULL;
    _groups->count  = 0u;

    return;
}
