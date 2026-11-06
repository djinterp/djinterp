/*******************************************************************************
* djinterp [c/parse]                                               syntax_item.c
*
* Definitions for the non-inline declarations in syntax_item.h.
*   A scope is walked child by child, where a child is a token or a whole
* group, and each scope's walk is a small state machine: a declaration runs
* to its `;` or its body, a control statement moves from keyword to
* condition to body, and an `if` waits one child longer to see whether an
* `else` follows.  A braceless body is an item inside the control item that
* owns it; when it completes it notifies its owner, which may complete in
* turn, and that notification climbs the parent chain in a loop.
*   The role of a brace is decided by what precedes it, in this order: an
* `=` makes it an initializer, a parameter group a function body, a
* `struct` a member list, and an `extern "C"` or a namespace a transparent
* block.  The order matters: `struct d_x* d_x_new(void) {` has both a
* `struct` and a parameter group, and is a function.
*   In C++ three things in a head hide what they hold (lane H, 11-05 (2)):
* template arguments, between a `<` after a name and its `>`; a class's base
* list, after its `:`; and a constructor's initializer list, after the `:`
* that follows its parameters, where each initializer's own `(` or `{` comes
* before the body's brace.  Nothing among them ends, starts or names an
* item.
*
*
* path:      /src/djinterp/c/modules/parse/syntax/syntax_item.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.11.06
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/modules/parse/syntax/syntax_item.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcmp, memset
// djinterp
#include "../../../../../../inc/djinterp/c/dckdint.h"                 // d_ckd_mul
#include "../../../../../../inc/djinterp/c/modules/parse/storage.h"  // d_parse_grow
// re_std
#include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t, uint16_t,
                                                     // uint8_t, UINT32_MAX

// the recognizer grows its arrays and has no fixed-storage mode
#if (D_INTERNAL_PARSE_HEAP != 1)
    #error "this file requires D_CFG_PARSE_HEAP, for d_parse_grow"
#endif


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    Scope modes
//------------------------------------------------------------------------------
// 1.1.1
// dj_internal_mode
//   enum: how a scope's children divide into items.  A group with no mode is
//   not a scope -- a condition, an argument list, an initializer -- and is
//   never walked; its tokens belong to the item around it.
enum dj_internal_mode
{
    D_INTERNAL_MODE_NONE = 0,
    D_INTERNAL_MODE_FILE,     // declarations to `;` or a body
    D_INTERNAL_MODE_BLOCK,    // statements
    D_INTERNAL_MODE_MEMBERS,  // struct, union and class members
    D_INTERNAL_MODE_ENUM,     // enumerators, split at commas
    D_INTERNAL_MODE_PARAMS    // parameters, split at commas
};

// 1.2    Item states
//------------------------------------------------------------------------------
// 1.2.1
// dj_internal_state
//   enum: where an open item stands in its grammar.  ELSE, DO_WHILE and
//   CATCH are the states in which an item has a complete body and waits one
//   child to learn whether it continues.
enum dj_internal_state
{
    D_INTERNAL_ST_SIMPLE = 0,
    D_INTERNAL_ST_LABEL,
    D_INTERNAL_ST_COND,
    D_INTERNAL_ST_BODY,
    D_INTERNAL_ST_ELSE,
    D_INTERNAL_ST_ELSE_TAIL,
    D_INTERNAL_ST_DO_WHILE,
    D_INTERNAL_ST_DO_COND,
    D_INTERNAL_ST_DO_SEMI,
    D_INTERNAL_ST_CATCH,
    D_INTERNAL_ST_CATCH_COND,
    D_INTERNAL_ST_CATCH_BODY
};

// 1.3    Build state
//------------------------------------------------------------------------------
// 1.3.1
// d_internal_scratch
//   struct: what an open item has seen, parallel to the item array and used
//   only while the item is open.
struct dj_internal_scratch
{
    uint32_t  prev;          // previous significant child
    uint32_t  fn_name;       // candidate function name
    uint32_t  fn_params;     // candidate parameter group
    uint32_t  struct_at;     // first struct, union, class or enum keyword
    uint32_t  assign_at;     // first `=`
    uint32_t  operator_at;   // an `operator` keyword
    uint32_t  provisional;   // end of a completed body, while waiting
    uint16_t  seen;          // significant children so far
    uint8_t   state;
    uint8_t   saw_typedef;
    uint8_t   linkage;
    uint8_t   struct_body;
    uint8_t   macro_head;
    uint8_t   else_seen;
    uint8_t   enum_like;
    uint8_t   angle;         // template argument lists open
    uint8_t   base_list;     // in a class head's base list
    uint8_t   ctor_init;     // in an initializer list: 1 before an
                             // initializer's group, 2 after it
    uint8_t   scope_macro;   // a namespace macro, waiting for its arguments
};

// 1.3.2
// d_internal_build
//   struct: one recognition.  The four per-token arrays hold what a scope
//   walk records for the scopes nested inside it, which are walked later.
struct dj_internal_build
{
    const struct dj_token*          tokens;
    uint32_t                       count;
    const struct dj_syntax_groups*  groups;
    const char*                    text;
    struct dj_syntax_items*         out;
    struct dj_internal_scratch*     scratch;

    uint8_t*                       mode;       // per opener: its scope mode
    uint32_t*                      owner;      // per opener: owning item
    uint32_t*                      last_item;  // per opener: last item in it
    uint8_t*                       pp;         // per token: open conditionals
    uint8_t                        file_pp;    // around the whole file: its
                                               // include guard
    const struct dj_syntax_options* options;   // the caller's; may be NULL
};

// 1.3.3
// dj_internal_scope
//   struct: one scope's walk.  `cur` is the innermost open item; the items
//   around it are reached through `parent`.
struct dj_internal_scope
{
    uint32_t  opener;
    uint32_t  end;
    uint8_t   mode;
    uint8_t   base_level;
    uint8_t   base_pp;
    bool      under_label;
    uint32_t  cur;
    uint32_t  prev_extent;
};


//==============================================================================
// 2.  CHILDREN
//==============================================================================
// A scope's children are its tokens and its groups, each group counted once
// at its opener.  A directive is a child that no item begins or ends at.


/**
 * @brief Reports the last token a child covers: a group's closer, a directive's
 *        last token, or the token itself.
 *
 * @param[in] _b   the recognition in progress.
 * @param[in] _at  the child's token index.
 * @return the index of the last token the child covers.
 */
static
uint32_t d_internal_extent(
    const struct dj_internal_build* _b,
    uint32_t                        _at
)
{
    const struct dj_token* const token = &_b->tokens[_at];

    if ( ( d_syntax_is_opener(token->kind) ||
           d_syntax_is_directive_start(token) ) &&
         (_b->groups->match[_at] != D_SYNTAX_NONE) )
    {
        return _b->groups->match[_at];
    }

    return _at;
}


/**
 * @brief Steps from one child of a scope to the next.
 *
 * An unclosed opener has no closer to jump to, so the step scans for the next
 * token the scope owns.
 *
 * @param[in] _b      the recognition in progress.
 * @param[in] _at     the child's token index.
 * @param[in] _scope  the opener of the scope being stepped through.
 * @param[in] _end    one past the scope's last child.
 * @return the index of the scope's next child, or `_end`.
 */
static
uint32_t d_internal_next_child(
    const struct dj_internal_build* _b,
    uint32_t                        _at,
    uint32_t                        _scope,
    uint32_t                        _end
)
{
    const struct dj_token* const token = &_b->tokens[_at];

    if ( d_syntax_is_opener(token->kind) ||
         d_syntax_is_directive_start(token) )
    {
        if (_b->groups->match[_at] != D_SYNTAX_NONE)
        {
            return _b->groups->match[_at] + 1u;
        }

        uint32_t next = _at + 1u;

        while ((next < _end) && (_b->groups->parent[next] != _scope))
        {
            ++next;
        }

        return next;
    }

    return _at + 1u;
}


/**
 * @brief Reports whether a child is one no item begins or ends at.
 *
 * @param[in] _token  the token to test.
 * @return `true` if the child is a comment or a directive, `false` otherwise.
 */
static
bool d_internal_is_trivia(
    const struct dj_token* _token
)
{
    return ( (_token->kind == D_TOKEN_COMMENT) ||
             d_syntax_is_directive_start(_token) );
}


/**
 * @brief Finds the first significant child inside a group.
 *
 * @param[in] _b       the recognition in progress.
 * @param[in] _opener  the group's opening token index.
 * @return its index, or D_SYNTAX_NONE for an empty or unclosed group.
 */
static
uint32_t d_internal_first_inner(
    const struct dj_internal_build* _b,
    uint32_t                        _opener
)
{
    const uint32_t end = _b->groups->match[_opener];

    if (end == D_SYNTAX_NONE)
    {
        return D_SYNTAX_NONE;
    }

    for (uint32_t at = _opener + 1u;
         at < end;
         at = d_internal_next_child(_b, at, _opener, end))
    {
        if (!d_internal_is_trivia(&_b->tokens[at]))
        {
            return at;
        }
    }

    return D_SYNTAX_NONE;
}


/**
 * @brief Finds the first identifier among a group's own children: the name
 *        inside `(*name)`.
 *
 * @param[in] _b       the recognition in progress.
 * @param[in] _opener  the group's opening token index.
 * @return its index, or D_SYNTAX_NONE.
 */
static
uint32_t d_internal_inner_identifier(
    const struct dj_internal_build* _b,
    uint32_t                        _opener
)
{
    const uint32_t end = _b->groups->match[_opener];

    if (end == D_SYNTAX_NONE)
    {
        return D_SYNTAX_NONE;
    }

    for (uint32_t at = _opener + 1u;
         at < end;
         at = d_internal_next_child(_b, at, _opener, end))
    {
        if (_b->tokens[at].kind == D_TOKEN_IDENTIFIER)
        {
            return at;
        }
    }

    return D_SYNTAX_NONE;
}


//==============================================================================
// 3.  TOKEN CLASSES
//==============================================================================


/**
 * @brief Reports whether a keyword can only begin a declaration, so that a
 *        block statement beginning with it is one.
 *
 * A statement beginning with an identifier is left `simple`, since the
 * identifier may or may not name a type.
 *
 * @param[in] _kind  the token kind to test.
 * @return `true` if the keyword begins only declarations, `false` otherwise.
 */
static
bool d_internal_is_decl_keyword(
    int _kind
)
{
    switch (_kind)
    {
        case D_TOKEN_KW_AUTO:          case D_TOKEN_KW_BOOL:
        case D_TOKEN_KW_CHAR:          case D_TOKEN_KW_CONST:
        case D_TOKEN_KW_CONSTEXPR:     case D_TOKEN_KW_DOUBLE:
        case D_TOKEN_KW_ENUM:          case D_TOKEN_KW_EXTERN:
        case D_TOKEN_KW_FLOAT:         case D_TOKEN_KW_INLINE:
        case D_TOKEN_KW_INT:           case D_TOKEN_KW_LONG:
        case D_TOKEN_KW_REGISTER:      case D_TOKEN_KW_SHORT:
        case D_TOKEN_KW_SIGNED:        case D_TOKEN_KW_STATIC:
        case D_TOKEN_KW_STRUCT:        case D_TOKEN_KW_THREAD_LOCAL:
        case D_TOKEN_KW_UNION:         case D_TOKEN_KW_UNSIGNED:
        case D_TOKEN_KW_VOID:          case D_TOKEN_KW_VOLATILE:
        case D_TOKEN_KW_ALIGNAS:       case D_TOKEN_KW_STATIC_ASSERT:
        case D_TOKEN_KW_RESTRICT:      case D_TOKEN_KW_TYPEOF:
        case D_TOKEN_KW_TYPEOF_UNQUAL: case D_TOKEN_KW_ATOMIC:
        case D_TOKEN_KW_BOOL_RESERVED: case D_TOKEN_KW_COMPLEX:
        case D_TOKEN_KW_BITINT:        case D_TOKEN_KW_NORETURN:
        case D_TOKEN_KW_CLASS:         case D_TOKEN_KW_MUTABLE:
        case D_TOKEN_KW_CHAR8_T:       case D_TOKEN_KW_CHAR16_T:
        case D_TOKEN_KW_CHAR32_T:      case D_TOKEN_KW_WCHAR_T:
        case D_TOKEN_KW_CONSTINIT:     case D_TOKEN_KW_USING:
        case D_TOKEN_KW_DECLTYPE:      case D_TOKEN_KW_TEMPLATE:
        case D_TOKEN_KW_ALIGNAS_RESERVED:
        case D_TOKEN_KW_STATIC_ASSERT_RESERVED:
        case D_TOKEN_KW_THREAD_LOCAL_RESERVED:
            return true;

        default:
            return false;
    }
}


/**
 * @brief Reports whether a token names a type rather than qualifying one.
 *
 * An identifier counts, being a typedef name or a macro standing for one;
 * `const` does not. This is how `const T` is told from `T _x`: the first names
 * no type before its last identifier, so that identifier is the type.
 *
 * @param[in] _kind  the token kind to test.
 * @return `true` if the token names a type, `false` otherwise.
 */
static
bool d_internal_is_type_specifier(
    int _kind
)
{
    switch (_kind)
    {
        case D_TOKEN_IDENTIFIER:
        case D_TOKEN_KW_VOID:     case D_TOKEN_KW_CHAR:
        case D_TOKEN_KW_SHORT:    case D_TOKEN_KW_INT:
        case D_TOKEN_KW_LONG:     case D_TOKEN_KW_FLOAT:
        case D_TOKEN_KW_DOUBLE:   case D_TOKEN_KW_SIGNED:
        case D_TOKEN_KW_UNSIGNED: case D_TOKEN_KW_BOOL:
        case D_TOKEN_KW_STRUCT:   case D_TOKEN_KW_UNION:
        case D_TOKEN_KW_ENUM:     case D_TOKEN_KW_CLASS:
        case D_TOKEN_KW_AUTO:     case D_TOKEN_KW_TYPEOF:
        case D_TOKEN_KW_BOOL_RESERVED:
        case D_TOKEN_KW_COMPLEX:  case D_TOKEN_KW_BITINT:
        case D_TOKEN_KW_CHAR8_T:  case D_TOKEN_KW_CHAR16_T:
        case D_TOKEN_KW_CHAR32_T: case D_TOKEN_KW_WCHAR_T:
        case D_TOKEN_KW_DECLTYPE:
            return true;

        default:
            return false;
    }
}


/**
 * @brief Reports whether a keyword introduces a tag, after which an identifier
 *        names the tag and never a declared object.
 *
 * @param[in] _kind  the token kind to test.
 * @return `true` if the keyword introduces a tag, `false` otherwise.
 */
static
bool d_internal_is_tag_keyword(
    int _kind
)
{
    return ( (_kind == D_TOKEN_KW_STRUCT) || (_kind == D_TOKEN_KW_UNION) ||
             (_kind == D_TOKEN_KW_ENUM)   || (_kind == D_TOKEN_KW_CLASS) );
}


/**
 * @brief Reports whether an extent spells one of a NULL-terminated list of
 *        words.
 *
 * A function rather than a macro, as the guide asks wherever one will do.
 *
 * @param[in] _text   the extent's first byte.
 * @param[in] _size   the extent's length in bytes.
 * @param[in] _words  the words, `NULL`-terminated.
 * @return `true` if the extent spells one of the words, `false` otherwise.
 */
static
bool d_internal_spelled(
    const char*        _text,
    size_t             _size,
    const char* const* _words
)
{
    for (size_t at = 0u; _words[at]; ++at)
    {
        if ( (strlen(_words[at]) == _size) &&
             (memcmp(_words[at], _text, _size) == 0) )
        {
            return true;
        }
    }

    return false;
}


/**
 * @brief Counts a child into a depth of template argument lists, and reports
 *        whether it is an angle bracket of one or among its arguments.
 *
 * A `<` after a name or after `template` opens a list, a `>` closes one and a
 * `>>` two. A `<` after anything else is a comparison. The rule is wrong only
 * where a `<` after a name compares, which no head has outside a group.
 *
 * @param[in]     _kind       the child's token kind.
 * @param[in]     _prev_kind  the previous significant child's kind, or
 *                            D_TOKEN_END where no list may open.
 * @param[in,out] _depth      the lists open; 255 at most.
 * @return `true` if the child is an angle bracket of a list or inside one,
 *         `false` otherwise.
 */
static
bool d_internal_angle_step(
    int      _kind,
    int      _prev_kind,
    uint8_t* _depth
)
{
    if ( (_kind == D_TOKEN_LESS) &&
         ( (_prev_kind == D_TOKEN_IDENTIFIER) ||
           (_prev_kind == D_TOKEN_KW_TEMPLATE) ) )
    {
        if (*_depth < 0xFFu)
        {
            ++(*_depth);
        }

        return true;
    }

    if (*_depth == 0u)
    {
        return false;
    }

    if (_kind == D_TOKEN_GREATER)
    {
        --(*_depth);
    }
    else if (_kind == D_TOKEN_SHIFT_RIGHT)
    {
        *_depth = (*_depth > 2u) ? (uint8_t)(*_depth - 2u) : 0u;
    }

    return true;
}


//==============================================================================
// 4.  ITEMS
//==============================================================================


/**
 * @brief Appends an item, growing the item and scratch arrays together.
 *
 * @param[in,out] _b  the recognition in progress.
 * @return the item's index, or D_SYNTAX_NONE when memory ran out.
 */
static
uint32_t d_internal_new_item(
    struct dj_internal_build* _b
)
{
    struct dj_syntax_items* const out = _b->out;

    // the two arrays grow in step through the subframework's one policy; an
    // item index is 32 bits, so a count at the limit cannot grow further
    if (out->count == out->capacity)
    {
        const uint32_t needed           = (uint32_t)out->count + 1u;
        uint32_t       items_capacity   = (uint32_t)out->capacity;
        uint32_t       scratch_capacity = (uint32_t)out->capacity;

        if (out->count >= (size_t)UINT32_MAX)
        {
            return D_SYNTAX_NONE;
        }

        struct dj_syntax_item* const items =
            d_parse_grow(out->items,
                         &items_capacity,
                         needed,
                         (uint32_t)sizeof(struct dj_syntax_item),
                         1);

        // check if memory allocation was successful
        if (!items)
        {
            return D_SYNTAX_NONE;
        }

        out->items = items;

        struct dj_internal_scratch* const scratch =
            d_parse_grow(_b->scratch,
                         &scratch_capacity,
                         needed,
                         (uint32_t)sizeof(struct dj_internal_scratch),
                         1);

        // the item array may now be the larger; `capacity` stays the smaller
        if (!scratch)
        {
            return D_SYNTAX_NONE;
        }

        _b->scratch   = scratch;
        out->capacity = (items_capacity < scratch_capacity) ? items_capacity
                                                            : scratch_capacity;
    }

    const uint32_t                   at      = (uint32_t)out->count;
    struct dj_syntax_item* const      item    = &out->items[at];
    struct dj_internal_scratch* const scratch = &_b->scratch[at];

    memset(item, 0, sizeof(*item));
    memset(scratch, 0, sizeof(*scratch));

    item->parent = D_SYNTAX_NONE;
    item->scope  = D_SYNTAX_NONE;
    item->name   = D_SYNTAX_NONE;
    item->params = D_SYNTAX_NONE;
    item->body   = D_SYNTAX_NONE;

    scratch->prev        = D_SYNTAX_NONE;
    scratch->fn_name     = D_SYNTAX_NONE;
    scratch->fn_params   = D_SYNTAX_NONE;
    scratch->struct_at   = D_SYNTAX_NONE;
    scratch->assign_at   = D_SYNTAX_NONE;
    scratch->operator_at = D_SYNTAX_NONE;
    scratch->provisional = D_SYNTAX_NONE;

    ++out->count;

    return at;
}


/**
 * @brief Declares that a group is a scope of a given mode, owned by an item.
 *
 * The group's own level is its owner's: a brace sits where its owner begins.
 *
 * @param[in,out] _b       the recognition in progress.
 * @param[in]     _opener  the group's opening token index.
 * @param[in]     _mode    the scope's mode.
 * @param[in]     _owner   the item that owns the scope.
 */
static
void d_internal_set_scope(
    struct dj_internal_build* _b,
    uint32_t                  _opener,
    uint8_t                   _mode,
    uint32_t                  _owner
)
{
    _b->mode[_opener]          = _mode;
    _b->owner[_opener]         = _owner;
    _b->out->level_of[_opener] = _b->out->items[_owner].level;

    return;
}


/**
 * @brief Finds what a declaration or typedef declares.
 *
 * A parenthesis whose first child is `*` and which is followed by a parameter
 * group declares a function pointer, named inside it; an identifier followed by
 * a parameter group declares a function type; otherwise the declared name is
 * the last identifier before an initializer, a bitfield or the end, and an
 * identifier following a tag keyword is the tag and never the name.
 * Template arguments, and so a template head, are passed over.
 *
 * @param[in,out] _b  the recognition in progress.
 * @param[in]     _k  the item's index.
 */
static
void d_internal_declarator(
    struct dj_internal_build* _b,
    uint32_t                  _k
)
{
    struct dj_syntax_item* const item  = &_b->out->items[_k];
    const uint32_t              scope = item->scope;
    const uint32_t              end   = item->last + 1u;

    uint32_t name      = D_SYNTAX_NONE;
    int      prev_kind = D_TOKEN_END;
    uint8_t  angle     = 0u;

    for (uint32_t at = item->first;
         at < end;
         at = d_internal_next_child(_b, at, scope, end))
    {
        const struct dj_token* const token = &_b->tokens[at];

        if (d_internal_is_trivia(token))
        {
            continue;
        }

        if (d_internal_angle_step(token->kind, prev_kind, &angle))
        {
            prev_kind = token->kind;
            continue;
        }

        // an initializer, a bitfield width or a second declarator ends the
        // search for the first one
        if ( (token->kind == D_TOKEN_ASSIGN) ||
             (token->kind == D_TOKEN_COMMA)  ||
             (token->kind == D_TOKEN_COLON)  ||
             (token->kind == D_TOKEN_SEMICOLON) )
        {
            break;
        }

        if (token->kind == D_TOKEN_PAREN_OPEN)
        {
            const uint32_t inner = d_internal_first_inner(_b, at);
            const uint32_t after = d_internal_next_child(_b,
                                       d_internal_extent(_b, at),
                                       scope, end);

            // `(*name)(...)`: a pointer to a function
            if ( (inner != D_SYNTAX_NONE) &&
                 ( (_b->tokens[inner].kind == D_TOKEN_STAR) ||
                   (_b->tokens[inner].kind == D_TOKEN_CARET) ) &&
                 (after < end) &&
                 (_b->tokens[after].kind == D_TOKEN_PAREN_OPEN) )
            {
                item->name   = d_internal_inner_identifier(_b, at);
                item->flags |= D_SYNTAX_ITEM_FLAG_FN_POINTER;
                return;
            }

            // `name(...)`: a function type
            if (prev_kind == D_TOKEN_IDENTIFIER)
            {
                item->flags |= D_SYNTAX_ITEM_FLAG_FN_TYPE;
                break;
            }
        }

        if ( (token->kind == D_TOKEN_IDENTIFIER) &&
             (!d_internal_is_tag_keyword(prev_kind)) )
        {
            name = at;
        }

        prev_kind = token->kind;
    }

    item->name = name;

    if (name == D_SYNTAX_NONE)
    {
        item->flags |= D_SYNTAX_ITEM_FLAG_UNNAMED;
    }

    return;
}


/**
 * @brief Finds a parameter's name, or learns that it has none.
 *
 * `(void)` and `...` declare nothing; `void (*_fn)(int)` is named inside its
 * parenthesis; an array parameter is named before its brackets; and a last
 * identifier is a name only when a type is named before it, which is what tells
 * `T _x` from `const T`.
 *
 * @param[in,out] _b  the recognition in progress.
 * @param[in]     _k  the item's index.
 */
static
void d_internal_classify_param(
    struct dj_internal_build* _b,
    uint32_t                  _k
)
{
    struct dj_syntax_item* const item  = &_b->out->items[_k];
    const uint32_t              scope = item->scope;
    const uint32_t              end   = item->last + 1u;

    uint32_t count      = 0u;
    int      first_kind = D_TOKEN_END;
    int      prev_kind  = D_TOKEN_END;
    uint32_t candidate  = D_SYNTAX_NONE;
    int      cand_prev  = D_TOKEN_END;
    uint32_t specs      = 0u;
    uint32_t cand_specs = 0u;

    for (uint32_t at = item->first;
         at < end;
         at = d_internal_next_child(_b, at, scope, end))
    {
        const struct dj_token* const token = &_b->tokens[at];

        // the separating comma and a default argument are not declarator
        if ( d_internal_is_trivia(token) || (token->kind == D_TOKEN_COMMA) )
        {
            continue;
        }

        if (token->kind == D_TOKEN_ASSIGN)
        {
            break;
        }

        if (count == 0u)
        {
            first_kind = token->kind;
        }

        ++count;

        // `(*name)` names a function pointer parameter
        if (token->kind == D_TOKEN_PAREN_OPEN)
        {
            const uint32_t inner = d_internal_first_inner(_b, at);

            if ( (inner != D_SYNTAX_NONE) &&
                 (_b->tokens[inner].kind == D_TOKEN_STAR) )
            {
                item->name   = d_internal_inner_identifier(_b, at);
                item->flags |= D_SYNTAX_ITEM_FLAG_FN_POINTER;

                if (item->name == D_SYNTAX_NONE)
                {
                    item->flags |= D_SYNTAX_ITEM_FLAG_UNNAMED;
                }

                return;
            }
        }

        // an array's brackets do not move the name
        if (token->kind != D_TOKEN_BRACKET_OPEN)
        {
            candidate  = at;
            cand_prev  = prev_kind;
            cand_specs = specs;
        }

        if (d_internal_is_type_specifier(token->kind))
        {
            ++specs;
        }

        prev_kind = token->kind;
    }

    if ((count == 1u) && (first_kind == D_TOKEN_KW_VOID))
    {
        item->flags |= D_SYNTAX_ITEM_FLAG_VOID | D_SYNTAX_ITEM_FLAG_UNNAMED;
        return;
    }

    if (first_kind == D_TOKEN_ELLIPSIS)
    {
        item->flags |= D_SYNTAX_ITEM_FLAG_VARIADIC |
                       D_SYNTAX_ITEM_FLAG_UNNAMED;
        return;
    }

    // a named parameter: an identifier, not a tag, with a type before it
    if ( (candidate != D_SYNTAX_NONE) &&
         (_b->tokens[candidate].kind == D_TOKEN_IDENTIFIER) &&
         (!d_internal_is_tag_keyword(cand_prev)) &&
         (cand_specs >= 1u) )
    {
        item->name = candidate;
        return;
    }

    item->flags |= D_SYNTAX_ITEM_FLAG_UNNAMED;

    return;
}


/**
 * @brief Reports whether the tokens before a function's name declare a `void`
 *        return and not a pointer.
 *
 * A `void` among template arguments, `d_result<void> run()`, is none.
 *
 * @param[in] _b  the recognition in progress.
 * @param[in] _k  the item's index.
 * @return `true` if the function returns `void` and not a pointer, `false`
 *         otherwise.
 */
static
bool d_internal_returns_void(
    const struct dj_internal_build* _b,
    uint32_t                        _k
)
{
    const struct dj_syntax_item* const item  = &_b->out->items[_k];
    const uint32_t                    scope = item->scope;
    const uint32_t                    end   = item->name;
    bool                              seen  = false;
    int                               prev  = D_TOKEN_END;
    uint8_t                           angle = 0u;

    if (end == D_SYNTAX_NONE)
    {
        return false;
    }

    for (uint32_t at = item->first;
         at < end;
         at = d_internal_next_child(_b, at, scope, end))
    {
        const int kind = _b->tokens[at].kind;

        if (d_internal_is_trivia(&_b->tokens[at]))
        {
            continue;
        }

        if (d_internal_angle_step(kind, prev, &angle))
        {
            prev = kind;
            continue;
        }

        prev = kind;

        if ((kind == D_TOKEN_STAR) || (kind == D_TOKEN_PAREN_OPEN))
        {
            return false;
        }

        if (kind == D_TOKEN_KW_VOID)
        {
            seen = true;
        }
    }

    return seen;
}


/**
 * @brief Settles what a just-completed item is.
 *
 * @param[in,out] _b  the recognition in progress.
 * @param[in]     _s  the scope being walked.
 * @param[in]     _k  the item's index.
 */
static
void d_internal_classify(
    struct dj_internal_build*       _b,
    const struct dj_internal_scope* _s,
    uint32_t                        _k
)
{
    struct dj_syntax_item* const      item    = &_b->out->items[_k];
    struct dj_internal_scratch* const scratch = &_b->scratch[_k];
    const int first_kind = _b->tokens[item->first].kind;

    switch (item->kind)
    {
        case D_SYNTAX_ITEM_PARAM:
            d_internal_classify_param(_b, _k);
            return;

        case D_SYNTAX_ITEM_ENUMERATOR:
            item->name = (first_kind == D_TOKEN_IDENTIFIER) ? item->first
                                                            : D_SYNTAX_NONE;
            return;

        case D_SYNTAX_ITEM_FN_DEF:
            item->name   = scratch->macro_head ? D_SYNTAX_NONE
                                               : scratch->fn_name;
            item->params = scratch->fn_params;

            if (scratch->macro_head)
            {
                item->flags |= D_SYNTAX_ITEM_FLAG_MACRO_HEAD;
            }
            else if (d_internal_returns_void(_b, _k))
            {
                item->flags |= D_SYNTAX_ITEM_FLAG_RETURNS_VOID;
            }

            return;

        case D_SYNTAX_ITEM_LABEL:
        case D_SYNTAX_ITEM_BLOCK:
            return;

        default:
            break;
    }

    // a block statement is a declaration only when a keyword says so
    if ( (_s->mode == D_INTERNAL_MODE_BLOCK) &&
         (item->kind == D_SYNTAX_ITEM_STATEMENT) )
    {
        if (first_kind == D_TOKEN_KW_TYPEDEF)
        {
            item->kind      = D_SYNTAX_ITEM_TYPEDEF;
            item->statement = D_SYNTAX_STMT_NONE;
            d_internal_declarator(_b, _k);

            if (scratch->struct_at != D_SYNTAX_NONE)
            {
                item->flags |= D_SYNTAX_ITEM_FLAG_HIDES_TAG;
            }
        }
        else if (d_internal_is_decl_keyword(first_kind))
        {
            item->kind      = D_SYNTAX_ITEM_DECLARATION;
            item->statement = D_SYNTAX_STMT_NONE;
            d_internal_declarator(_b, _k);
        }

        return;
    }

    if (scratch->saw_typedef)
    {
        item->kind = D_SYNTAX_ITEM_TYPEDEF;
        d_internal_declarator(_b, _k);

        // a tag keyword at the typedef's own level: `typedef struct x y;`
        if (scratch->struct_at != D_SYNTAX_NONE)
        {
            item->flags |= D_SYNTAX_ITEM_FLAG_HIDES_TAG;
        }

        return;
    }

    // a parameter group after a name, with no `=` before it, is a function
    if ( (scratch->fn_params != D_SYNTAX_NONE) && (!scratch->macro_head) &&
         ( (scratch->assign_at == D_SYNTAX_NONE) ||
           (scratch->assign_at > scratch->fn_params) ) )
    {
        item->kind   = D_SYNTAX_ITEM_FN_DECL;
        item->name   = scratch->fn_name;
        item->params = scratch->fn_params;

        d_internal_set_scope(_b, scratch->fn_params,
                             D_INTERNAL_MODE_PARAMS, _k);

        if (d_internal_returns_void(_b, _k))
        {
            item->flags |= D_SYNTAX_ITEM_FLAG_RETURNS_VOID;
        }

        return;
    }

    item->kind = D_SYNTAX_ITEM_DECLARATION;

    // a macro invocation declares nothing a tool could name
    if (scratch->macro_head)
    {
        item->flags |= D_SYNTAX_ITEM_FLAG_MACRO_HEAD |
                       D_SYNTAX_ITEM_FLAG_UNNAMED;
        return;
    }

    d_internal_declarator(_b, _k);

    return;
}


/**
 * @brief Tells a control item that one of its bodies has completed.
 *
 * @param[in,out] _b     the recognition in progress.
 * @param[in]     _k     the item's index.
 * @param[in]     _last  the last token the body covers.
 * @return `true` if the item is now complete, `false` if it waits to see
 *         whether an `else`, `while` or `catch` follows.
 */
static
bool d_internal_body_done(
    struct dj_internal_build* _b,
    uint32_t                  _k,
    uint32_t                  _last
)
{
    struct dj_internal_scratch* const scratch   = &_b->scratch[_k];
    const uint8_t                    statement = _b->out->items[_k].statement;

    if ((statement == D_SYNTAX_STMT_IF) && (!scratch->else_seen))
    {
        scratch->state       = D_INTERNAL_ST_ELSE;
        scratch->provisional = _last;
        return false;
    }

    if (statement == D_SYNTAX_STMT_DO)
    {
        scratch->state       = D_INTERNAL_ST_DO_WHILE;
        scratch->provisional = _last;
        return false;
    }

    if (statement == D_SYNTAX_STMT_TRY)
    {
        scratch->state       = D_INTERNAL_ST_CATCH;
        scratch->provisional = _last;
        return false;
    }

    return true;
}


/**
 * @brief Completes an item and, when it was a braceless body, tells its owner,
 *        which may complete in turn.
 *
 * The climb is a loop, not a recursion.
 *
 * @param[in,out] _b     the recognition in progress.
 * @param[in,out] _s     the scope being walked.
 * @param[in]     _k     the item's index.
 * @param[in]     _last  the last token the body covers.
 */
static
void d_internal_complete(
    struct dj_internal_build* _b,
    struct dj_internal_scope* _s,
    uint32_t                  _k,
    uint32_t                  _last
)
{
    uint32_t k = _k;

    for (;;)
    {
        struct dj_syntax_item* const item = &_b->out->items[k];

        item->last = _last;
        d_internal_classify(_b, _s, k);

        const uint32_t parent = item->parent;

        _s->cur = parent;

        // a top-level item of the scope is, for now, its last
        if (parent == D_SYNTAX_NONE)
        {
            if (_s->opener != D_SYNTAX_NONE)
            {
                _b->last_item[_s->opener] = k;
            }

            return;
        }

        // the owner learns its body is done, and may be done itself
        if (!d_internal_body_done(_b, parent, _last))
        {
            return;
        }

        k = parent;
    }
}


/**
 * @brief Opens an item at a child.
 *
 * A braceless body sits one level inside its owner; an item under a `case`
 * label one level inside the label.
 *
 * @param[in,out] _b       the recognition in progress.
 * @param[in,out] _s       the scope being walked.
 * @param[in]     _at      the child's token index.
 * @param[in]     _parent  the item this one is a braceless body of, or
 *                         `D_SYNTAX_NONE`.
 * @return the new item's index, or `D_SYNTAX_NONE` if memory ran out.
 */
static
uint32_t d_internal_open(
    struct dj_internal_build* _b,
    struct dj_internal_scope* _s,
    uint32_t                  _at,
    uint32_t                  _parent
)
{
    const uint32_t k = d_internal_new_item(_b);

    if (k == D_SYNTAX_NONE)
    {
        return D_SYNTAX_NONE;
    }

    struct dj_syntax_item* const      item    = &_b->out->items[k];
    struct dj_internal_scratch* const scratch = &_b->scratch[k];
    const int                        kind    = _b->tokens[_at].kind;
    const bool                       label   =
        ((kind == D_TOKEN_KW_CASE) || (kind == D_TOKEN_KW_DEFAULT));

    item->first  = _at;
    item->last   = _at;
    item->parent = _parent;
    item->scope  = _s->opener;

    // under a label a statement is indented a level, but a braced body is
    // not: `case X:` then `{` on the label's column, its contents one in
    const bool     compound = (kind == D_TOKEN_BRACE_OPEN);
    const bool     indented = ( _s->under_label &&
                                (!label)        &&
                                (!compound) );
    const unsigned level    =
        (_parent != D_SYNTAX_NONE)
        ? (unsigned)_b->out->items[_parent].level + 1u
        : (unsigned)_s->base_level + (indented ? 1u : 0u);

    item->level    = (uint8_t)((level > 255u) ? 255u : level);
    item->pp_depth = (_b->pp[_at] > _s->base_pp)
                   ? (uint8_t)(_b->pp[_at] - _s->base_pp)
                   : 0u;
    item->pp_total = (_b->pp[_at] > _b->file_pp)
                   ? (uint8_t)(_b->pp[_at] - _b->file_pp)
                   : 0u;

    if (_parent != D_SYNTAX_NONE)
    {
        item->flags |= D_SYNTAX_ITEM_FLAG_BRACELESS;
    }
    else if (_s->under_label && (!label))
    {
        item->flags |= D_SYNTAX_ITEM_FLAG_UNDER_LABEL;
    }

    switch (_s->mode)
    {
        case D_INTERNAL_MODE_PARAMS: item->kind = D_SYNTAX_ITEM_PARAM;  break;
        case D_INTERNAL_MODE_ENUM:   item->kind = D_SYNTAX_ITEM_ENUMERATOR;
                                     break;
        case D_INTERNAL_MODE_BLOCK:  item->kind = D_SYNTAX_ITEM_STATEMENT;
                                     break;
        default:                     item->kind = D_SYNTAX_ITEM_DECLARATION;
                                     break;
    }

    scratch->state = D_INTERNAL_ST_SIMPLE;

    // a statement's leading keyword decides its grammar
    if (_s->mode == D_INTERNAL_MODE_BLOCK)
    {
        switch (kind)
        {
            case D_TOKEN_KW_IF:
                item->statement = D_SYNTAX_STMT_IF;
                scratch->state  = D_INTERNAL_ST_COND;
                break;
            case D_TOKEN_KW_FOR:
                item->statement = D_SYNTAX_STMT_FOR;
                scratch->state  = D_INTERNAL_ST_COND;
                break;
            case D_TOKEN_KW_WHILE:
                item->statement = D_SYNTAX_STMT_WHILE;
                scratch->state  = D_INTERNAL_ST_COND;
                break;
            case D_TOKEN_KW_SWITCH:
                item->statement = D_SYNTAX_STMT_SWITCH;
                scratch->state  = D_INTERNAL_ST_COND;
                break;
            case D_TOKEN_KW_DO:
                item->statement = D_SYNTAX_STMT_DO;
                scratch->state  = D_INTERNAL_ST_BODY;
                break;
            case D_TOKEN_KW_TRY:
                item->statement = D_SYNTAX_STMT_TRY;
                scratch->state  = D_INTERNAL_ST_BODY;
                break;
            case D_TOKEN_KW_RETURN:
                item->statement = D_SYNTAX_STMT_RETURN;
                break;
            case D_TOKEN_KW_BREAK:
                item->statement = D_SYNTAX_STMT_BREAK;
                break;
            case D_TOKEN_KW_CONTINUE:
                item->statement = D_SYNTAX_STMT_CONTINUE;
                break;
            case D_TOKEN_KW_GOTO:
                item->statement = D_SYNTAX_STMT_GOTO;
                break;
            case D_TOKEN_KW_CASE:
            case D_TOKEN_KW_DEFAULT:
                item->kind      = D_SYNTAX_ITEM_LABEL;
                item->statement = (kind == D_TOKEN_KW_CASE)
                                ? D_SYNTAX_STMT_CASE
                                : D_SYNTAX_STMT_DEFAULT;
                scratch->state  = D_INTERNAL_ST_LABEL;
                break;
            default:
                item->statement = D_SYNTAX_STMT_SIMPLE;
                break;
        }

        // `name:` is a label for `goto`
        if (kind == D_TOKEN_IDENTIFIER)
        {
            const uint32_t next = d_internal_next_child(_b, _at, _s->opener,
                                                        _s->end);

            if ( (next < _s->end) &&
                 (_b->tokens[next].kind == D_TOKEN_COLON) )
            {
                item->kind      = D_SYNTAX_ITEM_LABEL;
                item->statement = D_SYNTAX_STMT_NAMED;
                scratch->state  = D_INTERNAL_ST_LABEL;
            }
        }
    }

    return k;
}


//==============================================================================
// 5.  THE STATE MACHINE
//==============================================================================


/**
 * @brief Moves an item past a significant child: the child is its previous
 *        one, and one more is seen.
 *
 * @param[in,out] _scratch  the item's scratch.
 * @param[in]     _at       the child's token index.
 */
static
void d_internal_step(
    struct dj_internal_scratch* _scratch,
    uint32_t                    _at
)
{
    _scratch->prev = _at;

    if (_scratch->seen < 0xFFFFu)
    {
        ++_scratch->seen;
    }

    return;
}


/**
 * @brief Records what a child tells a C++ head, and reports whether the child
 *        is among template arguments, which tell the rest of the head nothing.
 *
 * A template head is a list of template arguments after `template`. A `:`
 * after a class's tag and before its body begins its base list; a `:` after a
 * function's parameters begins its initializer list, where an initializer's
 * `(` or `{` and a `,` alternate until the body's brace. Only a declaration
 * or a parameter has a head: a statement's `<` compares.
 *
 * @param[in,out] _b   the recognition in progress.
 * @param[in]     _s   the scope being walked.
 * @param[in]     _k   the item's index.
 * @param[in]     _at  the child's token index.
 * @return `true` if the child is an angle bracket or a template argument,
 *         `false` otherwise.
 */
static
bool d_internal_note_head(
    struct dj_internal_build*       _b,
    const struct dj_internal_scope* _s,
    uint32_t                        _k,
    uint32_t                        _at
)
{
    struct dj_internal_scratch* const scratch = &_b->scratch[_k];
    const int  kind   = _b->tokens[_at].kind;
    const bool params = (_s->mode == D_INTERNAL_MODE_PARAMS);

    if ( (_s->mode != D_INTERNAL_MODE_FILE)    &&
         (_s->mode != D_INTERNAL_MODE_MEMBERS) &&
         (!params) )
    {
        return false;
    }

    // a list opens before the item's `=`, and anywhere in a parameter's
    // default; once open it is counted to its end
    const bool may_open  = ( (params) ||
                             (scratch->assign_at == D_SYNTAX_NONE) ||
                             (scratch->angle > 0u) );
    const int  prev_kind = ( (may_open) && (scratch->prev != D_SYNTAX_NONE) )
                         ? _b->tokens[scratch->prev].kind
                         : D_TOKEN_END;

    if (d_internal_angle_step(kind, prev_kind, &scratch->angle))
    {
        return true;
    }

    if ( (kind == D_TOKEN_COLON) && (!params) &&
         (scratch->assign_at == D_SYNTAX_NONE) )
    {
        if ( (scratch->struct_at != D_SYNTAX_NONE) &&
             (scratch->fn_params == D_SYNTAX_NONE) &&
             (!scratch->struct_body) )
        {
            scratch->base_list = 1u;
        }
        else if ( (scratch->fn_params != D_SYNTAX_NONE) &&
                  (!scratch->macro_head) && (scratch->ctor_init == 0u) )
        {
            scratch->ctor_init = 1u;
        }
    }
    else if ( (scratch->ctor_init == 1u) &&
              ( (kind == D_TOKEN_PAREN_OPEN) ||
                (kind == D_TOKEN_BRACE_OPEN) ) )
    {
        scratch->ctor_init = 2u;
    }
    else if ( (scratch->ctor_init == 2u) && (kind == D_TOKEN_COMMA) )
    {
        scratch->ctor_init = 1u;
    }

    return false;
}


/**
 * @brief Records what a significant child tells an item in its simple state.
 *
 * @note `name(` with nothing before it is a macro invocation at file or block
 *       scope -- `D_TEST(name) {` -- and a constructor in a class, where a
 *       function may have no return type. The scope is what tells them apart.
 *
 * @param[in,out] _b   the recognition in progress.
 * @param[in]     _s   the scope being walked.
 * @param[in]     _k   the item's index.
 * @param[in]     _at  the child's token index.
 */
static
void d_internal_note(
    struct dj_internal_build*       _b,
    const struct dj_internal_scope* _s,
    uint32_t                        _k,
    uint32_t                        _at
)
{
    struct dj_internal_scratch* const scratch = &_b->scratch[_k];
    const int                        kind    = _b->tokens[_at].kind;
    const bool in_operator =
        ( (scratch->operator_at != D_SYNTAX_NONE) &&
          (scratch->fn_params == D_SYNTAX_NONE) );

    // template arguments name, begin and end nothing
    if (d_internal_note_head(_b, _s, _k, _at))
    {
        d_internal_step(scratch, _at);
        return;
    }

    // the `=` of `operator=` is the operator's name, not an initializer
    if ( (kind == D_TOKEN_ASSIGN) && (!in_operator) &&
         (scratch->assign_at == D_SYNTAX_NONE) )
    {
        scratch->assign_at = _at;
    }

    if (kind == D_TOKEN_KW_TYPEDEF)
    {
        scratch->saw_typedef = 1u;
    }

    if ( d_internal_is_tag_keyword(kind) &&
         (scratch->struct_at == D_SYNTAX_NONE) &&
         (scratch->fn_params == D_SYNTAX_NONE) )
    {
        scratch->struct_at = _at;
        scratch->enum_like = (kind == D_TOKEN_KW_ENUM) ? 1u : 0u;
    }

    if (kind == D_TOKEN_KW_NAMESPACE)
    {
        scratch->linkage = 1u;
    }

    // `extern "C"`: the string is the second child
    if ( (kind == D_TOKEN_STRING) && (scratch->seen == 1u) &&
         (_b->tokens[_b->out->items[_k].first].kind == D_TOKEN_KW_EXTERN) )
    {
        scratch->linkage = 1u;
    }

    if (kind == D_TOKEN_KW_OPERATOR)
    {
        scratch->operator_at = _at;
    }

    // the first parenthesis after a name, before any `=` and outside a base
    // list, is a candidate parameter list
    if ( (kind == D_TOKEN_PAREN_OPEN) &&
         (scratch->fn_params == D_SYNTAX_NONE) &&
         (scratch->assign_at == D_SYNTAX_NONE) &&
         (!scratch->base_list) &&
         (scratch->prev != D_SYNTAX_NONE) )
    {
        const int prev_kind = _b->tokens[scratch->prev].kind;

        if (scratch->operator_at != D_SYNTAX_NONE)
        {
            // `operator()` spells its own name with an empty parenthesis
            const bool call_name =
                ( (scratch->prev == scratch->operator_at) &&
                  (_b->groups->match[_at] == (_at + 1u)) );

            if (!call_name)
            {
                scratch->fn_params = _at;
                scratch->fn_name   = scratch->operator_at;
            }
        }
        else if (prev_kind == D_TOKEN_IDENTIFIER)
        {
            // a name with nothing before it heads a macro invocation, except
            // in a class, where it heads a constructor
            scratch->fn_params  = _at;
            scratch->fn_name    = scratch->prev;
            scratch->macro_head =
                ( (scratch->seen == 1u) &&
                  (_s->mode != D_INTERNAL_MODE_MEMBERS) ) ? 1u : 0u;
        }
    }

    d_internal_step(scratch, _at);

    return;
}


/**
 * @brief Decides what a brace is to an item in its simple state.
 *
 * @param[in,out] _b   the recognition in progress.
 * @param[in,out] _s   the scope being walked.
 * @param[in]     _k   the item's index.
 * @param[in]     _at  the child's token index.
 */
static
void d_internal_brace(
    struct dj_internal_build* _b,
    struct dj_internal_scope* _s,
    uint32_t                  _k,
    uint32_t                  _at
)
{
    struct dj_syntax_item* const      item    = &_b->out->items[_k];
    struct dj_internal_scratch* const scratch = &_b->scratch[_k];
    const bool block = (_s->mode == D_INTERNAL_MODE_BLOCK);

    // an `extern "C"` block or a namespace, whose contents are at its level
    if ((!block) && scratch->linkage)
    {
        item->kind = D_SYNTAX_ITEM_BLOCK;
        item->body = _at;
        d_internal_set_scope(_b, _at, D_INTERNAL_MODE_FILE, _k);
        d_internal_complete(_b, _s, _k, d_internal_extent(_b, _at));
        return;
    }

    // after `=`, an initializer, and the item runs on to its `;`
    if (scratch->assign_at != D_SYNTAX_NONE)
    {
        d_internal_note(_b, _s, _k, _at);
        return;
    }

    // in an initializer list, an initializer's brace: `m_values{}`
    if (scratch->ctor_init == 1u)
    {
        d_internal_note(_b, _s, _k, _at);
        return;
    }

    // after a parameter group, a function's body
    if ((!block) && (scratch->fn_params != D_SYNTAX_NONE))
    {
        item->kind = D_SYNTAX_ITEM_FN_DEF;
        item->body = _at;
        d_internal_set_scope(_b, _at, D_INTERNAL_MODE_BLOCK, _k);

        if (!scratch->macro_head)
        {
            d_internal_set_scope(_b, scratch->fn_params,
                                 D_INTERNAL_MODE_PARAMS, _k);
        }

        d_internal_complete(_b, _s, _k, d_internal_extent(_b, _at));
        return;
    }

    // after `struct` and its tag, a member list
    if ( (scratch->struct_at != D_SYNTAX_NONE) && (!scratch->struct_body) &&
         (scratch->fn_params == D_SYNTAX_NONE) )
    {
        scratch->struct_body = 1u;
        item->body           = _at;
        d_internal_set_scope(_b, _at,
                             scratch->enum_like ? D_INTERNAL_MODE_ENUM
                                                : D_INTERNAL_MODE_MEMBERS,
                             _k);
        d_internal_note(_b, _s, _k, _at);
        return;
    }

    // `D_FOREACH(x) { }`: a macro invocation with a braced body
    if ( block && scratch->macro_head &&
         (scratch->prev == scratch->fn_params) )
    {
        item->statement = D_SYNTAX_STMT_MACRO;
        item->body      = _at;
        d_internal_set_scope(_b, _at, D_INTERNAL_MODE_BLOCK, _k);
        d_internal_complete(_b, _s, _k, d_internal_extent(_b, _at));
        return;
    }

    // a compound literal, or a brace this layer does not recognize
    d_internal_note(_b, _s, _k, _at);

    return;
}


/**
 * @brief Recognizes a macro the caller named as opening or closing a
 *        namespace, as the first child of an item at file scope, and takes
 *        an argument group on its line as its own.
 *
 * Such a macro is its own item whatever follows it: `NS_DJINTERP` then a
 * function on the next line is two items, not a function whose head begins
 * with a macro.
 *
 * @param[in,out] _b   the recognition in progress.
 * @param[in,out] _s   the scope being walked.
 * @param[in]     _k   the item's index.
 * @param[in]     _at  the child's token index.
 * @return `true` if the child belongs to such a macro, `false` otherwise.
 */
static
bool d_internal_scope_macro(
    struct dj_internal_build* _b,
    struct dj_internal_scope* _s,
    uint32_t                  _k,
    uint32_t                  _at
)
{
    struct dj_syntax_item* const      item    = &_b->out->items[_k];
    struct dj_internal_scratch* const scratch = &_b->scratch[_k];
    const struct dj_token* const      token   = &_b->tokens[_at];

    // the argument group the macro waited for
    if (_at != item->first)
    {
        if (!scratch->scope_macro)
        {
            return false;
        }

        item->params = _at;
        d_internal_complete(_b, _s, _k, d_internal_extent(_b, _at));

        return true;
    }

    const char* const spelling = _b->text + token->span.offset;
    const bool        opens    =
        ( (_b->options->scope_open) &&
          (d_internal_spelled(spelling, token->span.length,
                              _b->options->scope_open)) );

    if ( (!opens) &&
         ( (!_b->options->scope_close) ||
           (!d_internal_spelled(spelling, token->span.length,
                                _b->options->scope_close)) ) )
    {
        return false;
    }

    item->kind      = D_SYNTAX_ITEM_BLOCK;
    item->statement = (opens) ? D_SYNTAX_STMT_OPEN : D_SYNTAX_STMT_CLOSE;
    item->flags    |= D_SYNTAX_ITEM_FLAG_MACRO_HEAD;

    uint32_t next = d_internal_next_child(_b, _at, _s->opener, _s->end);

    while ( (next < _s->end) && (d_internal_is_trivia(&_b->tokens[next])) )
    {
        next = d_internal_next_child(_b, next, _s->opener, _s->end);
    }

    // `D_NAMESPACE(name)`: its arguments are on its line
    if ( (next < _s->end) &&
         (_b->tokens[next].kind == D_TOKEN_PAREN_OPEN) &&
         ((_b->tokens[next].flags & D_TOKEN_FLAG_LINE_START) == 0u) )
    {
        scratch->scope_macro = 1u;
        scratch->prev        = _at;

        return true;
    }

    d_internal_complete(_b, _s, _k, _at);

    return true;
}


/**
 * @brief Offers a child to an item in its simple state.
 *
 * @param[in,out] _b   the recognition in progress.
 * @param[in,out] _s   the scope being walked.
 * @param[in]     _k   the item's index.
 * @param[in]     _at  the child's token index.
 */
static
void d_internal_simple(
    struct dj_internal_build* _b,
    struct dj_internal_scope* _s,
    uint32_t                  _k,
    uint32_t                  _at
)
{
    const int kind = _b->tokens[_at].kind;

    // a namespace macro the caller named
    if ( (_s->mode == D_INTERNAL_MODE_FILE) && (_b->options) &&
         ( (kind == D_TOKEN_IDENTIFIER) || (_b->scratch[_k].scope_macro) ) &&
         (d_internal_scope_macro(_b, _s, _k, _at)) )
    {
        return;
    }

    // among template arguments nothing ends at a `,` or begins at a `{`
    if ( (_b->scratch[_k].angle > 0u) && (kind != D_TOKEN_SEMICOLON) )
    {
        d_internal_note(_b, _s, _k, _at);
        return;
    }

    // parameters and enumerators end at their commas
    if ( (_s->mode == D_INTERNAL_MODE_PARAMS) ||
         (_s->mode == D_INTERNAL_MODE_ENUM) )
    {
        if (kind == D_TOKEN_COMMA)
        {
            d_internal_complete(_b, _s, _k, _at);
            return;
        }

        d_internal_note(_b, _s, _k, _at);
        return;
    }

    if (kind == D_TOKEN_SEMICOLON)
    {
        d_internal_complete(_b, _s, _k, _at);
        return;
    }

    if (kind == D_TOKEN_BRACE_OPEN)
    {
        d_internal_brace(_b, _s, _k, _at);
        return;
    }

    // `public:` in a class
    if ( (kind == D_TOKEN_COLON) && (_s->mode == D_INTERNAL_MODE_MEMBERS) &&
         (_b->scratch[_k].seen == 1u) )
    {
        const int first = _b->tokens[_b->out->items[_k].first].kind;

        if ( (first == D_TOKEN_KW_PUBLIC)  || (first == D_TOKEN_KW_PRIVATE) ||
             (first == D_TOKEN_KW_PROTECTED) )
        {
            _b->out->items[_k].kind      = D_SYNTAX_ITEM_LABEL;
            _b->out->items[_k].statement = D_SYNTAX_STMT_ACCESS;
            d_internal_complete(_b, _s, _k, _at);
            return;
        }
    }

    d_internal_note(_b, _s, _k, _at);

    return;
}


/**
 * @brief Offers one child to the scope's innermost open item.
 *
 * An item that learns from the child that it has already ended completes, and
 * the child is offered again to whatever is open after it, which is why this is
 * a loop.
 *
 * @param[in,out] _b   the recognition in progress.
 * @param[in,out] _s   the scope being walked.
 * @param[in]     _at  the child's token index.
 * @return `true` on success, `false` if memory ran out.
 */
static
bool d_internal_feed(
    struct dj_internal_build* _b,
    struct dj_internal_scope* _s,
    uint32_t                  _at
)
{
    const int kind = _b->tokens[_at].kind;

    for (;;)
    {
        // a new top-level item
        if (_s->cur == D_SYNTAX_NONE)
        {
            if ((kind == D_TOKEN_KW_CASE) || (kind == D_TOKEN_KW_DEFAULT))
            {
                _s->under_label = false;
            }

            const uint32_t k = d_internal_open(_b, _s, _at, D_SYNTAX_NONE);

            if (k == D_SYNTAX_NONE)
            {
                return false;
            }

            _s->cur = k;

            if (_s->mode == D_INTERNAL_MODE_BLOCK)
            {
                // `{ }` and `;` are whole statements
                if (kind == D_TOKEN_BRACE_OPEN)
                {
                    _b->out->items[k].statement = D_SYNTAX_STMT_COMPOUND;
                    _b->out->items[k].body      = _at;
                    d_internal_set_scope(_b, _at, D_INTERNAL_MODE_BLOCK, k);
                    d_internal_complete(_b, _s, k,
                                        d_internal_extent(_b, _at));
                    return true;
                }

                if (kind == D_TOKEN_SEMICOLON)
                {
                    _b->out->items[k].statement = D_SYNTAX_STMT_EMPTY;
                    d_internal_complete(_b, _s, k, _at);
                    return true;
                }
            }

            // a leading keyword is consumed by the state it chose
            if (_b->scratch[k].state != D_INTERNAL_ST_SIMPLE)
            {
                _b->scratch[k].prev = _at;
                _b->scratch[k].seen = 1u;
                return true;
            }

            d_internal_simple(_b, _s, k, _at);
            return true;
        }

        const uint32_t                   k       = _s->cur;
        struct dj_internal_scratch* const scratch = &_b->scratch[k];

        switch (scratch->state)
        {
            case D_INTERNAL_ST_SIMPLE:
                // `MACRO(args)` alone on its line, followed by neither a body
                // nor a `;`, was a whole item: end it before the next line's
                // first token joins it
                if ( scratch->macro_head                                   &&
                     (scratch->prev == scratch->fn_params)                 &&
                     ((_b->tokens[_at].flags & D_TOKEN_FLAG_LINE_START)
                      != 0u)                                               &&
                     (kind != D_TOKEN_BRACE_OPEN)                          &&
                     (kind != D_TOKEN_SEMICOLON)                           &&
                     (kind != D_TOKEN_ASSIGN)                              &&
                     (_s->mode != D_INTERNAL_MODE_PARAMS)                  &&
                     (_s->mode != D_INTERNAL_MODE_ENUM) )
                {
                    d_internal_complete(_b,
                                        _s,
                                        k,
                                        d_internal_extent(_b, scratch->prev));
                    continue;
                }

                d_internal_simple(_b, _s, k, _at);
                return true;

            case D_INTERNAL_ST_LABEL:
                if (kind == D_TOKEN_COLON)
                {
                    const uint8_t statement = _b->out->items[k].statement;

                    d_internal_complete(_b, _s, k, _at);

                    if ( (statement == D_SYNTAX_STMT_CASE) ||
                         (statement == D_SYNTAX_STMT_DEFAULT) )
                    {
                        _s->under_label = true;
                    }
                }

                return true;

            case D_INTERNAL_ST_COND:
                if (kind == D_TOKEN_PAREN_OPEN)
                {
                    scratch->state = D_INTERNAL_ST_BODY;
                    scratch->prev  = _at;
                    return true;
                }

                // `if constexpr (` and `if !consteval {`
                if ( (kind == D_TOKEN_KW_CONSTEXPR) ||
                     (kind == D_TOKEN_KW_CONSTEVAL) ||
                     (kind == D_TOKEN_NOT) )
                {
                    return true;
                }

                scratch->state = D_INTERNAL_ST_SIMPLE;
                continue;

            case D_INTERNAL_ST_BODY:
                if (kind == D_TOKEN_BRACE_OPEN)
                {
                    if (_b->out->items[k].body == D_SYNTAX_NONE)
                    {
                        _b->out->items[k].body = _at;
                    }

                    d_internal_set_scope(_b, _at, D_INTERNAL_MODE_BLOCK, k);

                    if (d_internal_body_done(_b, k,
                                             d_internal_extent(_b, _at)))
                    {
                        d_internal_complete(_b, _s, k,
                                            d_internal_extent(_b, _at));
                    }

                    return true;
                }

                // an unbraced body is an item of its own, one level in
                {
                    const uint32_t inner = d_internal_open(_b, _s, _at, k);

                    if (inner == D_SYNTAX_NONE)
                    {
                        return false;
                    }

                    _s->cur = inner;

                    if (kind == D_TOKEN_SEMICOLON)
                    {
                        _b->out->items[inner].statement = D_SYNTAX_STMT_EMPTY;
                        d_internal_complete(_b, _s, inner, _at);
                        return true;
                    }

                    if (_b->scratch[inner].state != D_INTERNAL_ST_SIMPLE)
                    {
                        _b->scratch[inner].prev = _at;
                        _b->scratch[inner].seen = 1u;
                        return true;
                    }

                    d_internal_simple(_b, _s, inner, _at);
                    return true;
                }

            case D_INTERNAL_ST_ELSE:
                if (kind == D_TOKEN_KW_ELSE)
                {
                    scratch->state = D_INTERNAL_ST_ELSE_TAIL;
                    return true;
                }

                d_internal_complete(_b, _s, k, scratch->provisional);
                continue;

            case D_INTERNAL_ST_ELSE_TAIL:
                // `else if` continues one flat chain, not a nested one
                if (kind == D_TOKEN_KW_IF)
                {
                    scratch->state     = D_INTERNAL_ST_COND;
                    scratch->else_seen = 0u;
                    return true;
                }

                scratch->state     = D_INTERNAL_ST_BODY;
                scratch->else_seen = 1u;
                continue;

            case D_INTERNAL_ST_DO_WHILE:
                if (kind == D_TOKEN_KW_WHILE)
                {
                    scratch->state = D_INTERNAL_ST_DO_COND;
                    return true;
                }

                d_internal_complete(_b, _s, k, scratch->provisional);
                continue;

            case D_INTERNAL_ST_DO_COND:
                if (kind == D_TOKEN_PAREN_OPEN)
                {
                    scratch->state       = D_INTERNAL_ST_DO_SEMI;
                    scratch->provisional = d_internal_extent(_b, _at);
                    return true;
                }

                d_internal_complete(_b, _s, k, scratch->provisional);
                continue;

            case D_INTERNAL_ST_DO_SEMI:
                if (kind == D_TOKEN_SEMICOLON)
                {
                    d_internal_complete(_b, _s, k, _at);
                    return true;
                }

                d_internal_complete(_b, _s, k, scratch->provisional);
                continue;

            case D_INTERNAL_ST_CATCH:
                if (kind == D_TOKEN_KW_CATCH)
                {
                    scratch->state = D_INTERNAL_ST_CATCH_COND;
                    return true;
                }

                d_internal_complete(_b, _s, k, scratch->provisional);
                continue;

            case D_INTERNAL_ST_CATCH_COND:
                if (kind == D_TOKEN_PAREN_OPEN)
                {
                    scratch->state = D_INTERNAL_ST_CATCH_BODY;
                    return true;
                }

                d_internal_complete(_b, _s, k, scratch->provisional);
                continue;

            case D_INTERNAL_ST_CATCH_BODY:
                if (kind == D_TOKEN_BRACE_OPEN)
                {
                    d_internal_set_scope(_b, _at, D_INTERNAL_MODE_BLOCK, k);
                    scratch->state       = D_INTERNAL_ST_CATCH;
                    scratch->provisional = d_internal_extent(_b, _at);
                    return true;
                }

                d_internal_complete(_b, _s, k, scratch->provisional);
                continue;

            default:
                d_internal_simple(_b, _s, k, _at);
                return true;
        }
    }
}


/**
 * @brief Walks one scope's children, dividing them into items, then records
 *        each new item as the innermost owner of its tokens.
 *
 * The recording runs in creation order, so an item created inside another -- a
 * braceless body -- claims its tokens after its owner has claimed them all.
 *
 * @param[in,out] _b       the recognition in progress.
 * @param[in]     _opener  the group's opening token index.
 * @param[in]     _mode    the scope's mode.
 * @return `true` on success, `false` if memory ran out.
 */
static
bool d_internal_walk(
    struct dj_internal_build* _b,
    uint32_t                  _opener,
    uint8_t                   _mode
)
{
    struct dj_internal_scope scope;
    const uint32_t          owner = (_opener == D_SYNTAX_NONE)
                                  ? D_SYNTAX_NONE
                                  : _b->owner[_opener];
    const uint32_t          first_item = (uint32_t)_b->out->count;

    scope.opener      = _opener;
    scope.end         = (_opener == D_SYNTAX_NONE)
                      ? _b->count
                      : _b->groups->match[_opener];
    scope.mode        = _mode;
    scope.under_label = false;
    scope.cur         = D_SYNTAX_NONE;
    scope.prev_extent = (_opener == D_SYNTAX_NONE) ? 0u : _opener;
    scope.base_pp     = (_opener == D_SYNTAX_NONE) ? 0u : _b->pp[_opener];

    // a transparent block's contents sit at its own level; any other
    // scope's contents one level in
    if (owner == D_SYNTAX_NONE)
    {
        scope.base_level = 0u;
    }
    else
    {
        const unsigned level =
            (unsigned)_b->out->items[owner].level +
            ((_mode == D_INTERNAL_MODE_FILE) ? 0u : 1u);

        scope.base_level = (uint8_t)((level > 255u) ? 255u : level);
    }

    const uint32_t begin = (_opener == D_SYNTAX_NONE) ? 0u : (_opener + 1u);

    for (uint32_t at = begin;
         at < scope.end;
         at = d_internal_next_child(_b, at, _opener, scope.end))
    {
        if (d_internal_is_trivia(&_b->tokens[at]))
        {
            continue;
        }

        if (!d_internal_feed(_b, &scope, at))
        {
            return false;
        }

        scope.prev_extent = d_internal_extent(_b, at);
    }

    // the scope ended: whatever is open ends with it
    while (scope.cur != D_SYNTAX_NONE)
    {
        const uint32_t                   k       = scope.cur;
        struct dj_internal_scratch* const scratch = &_b->scratch[k];
        const bool waiting =
            ( (scratch->state == D_INTERNAL_ST_ELSE)     ||
              (scratch->state == D_INTERNAL_ST_DO_WHILE) ||
              (scratch->state == D_INTERNAL_ST_DO_COND)  ||
              (scratch->state == D_INTERNAL_ST_DO_SEMI)  ||
              (scratch->state == D_INTERNAL_ST_CATCH) );

        // a last parameter or enumerator needs no separator
        if ( (!waiting) &&
             (_mode != D_INTERNAL_MODE_PARAMS) &&
             (_mode != D_INTERNAL_MODE_ENUM) )
        {
            _b->out->items[k].flags |= D_SYNTAX_ITEM_FLAG_UNTERMINATED;
        }

        d_internal_complete(_b, &scope, k,
                            waiting ? scratch->provisional
                                    : scope.prev_extent);
    }

    // a parameter list whose first parameter shares the `(` line is laid
    // out by alignment; record that, and which parameters break it
    if ( (_mode == D_INTERNAL_MODE_PARAMS) &&
         (first_item < (uint32_t)_b->out->count) )
    {
        const struct dj_token* const lead =
            &_b->tokens[_b->out->items[first_item].first];

        // a declaration whose first parameter begins a line has left the
        // guide's declaration layout; a definition's parameters do so by rule
        if ( ((lead->flags & D_TOKEN_FLAG_LINE_START) != 0u) &&
             (owner != D_SYNTAX_NONE)                         &&
             (_b->out->items[owner].kind == D_SYNTAX_ITEM_FN_DECL) )
        {
            for (uint32_t k = first_item;
                 k < (uint32_t)_b->out->count;
                 ++k)
            {
                _b->out->items[k].flags |= D_SYNTAX_ITEM_FLAG_DETACHED;
            }
        }

        if ((lead->flags & D_TOKEN_FLAG_LINE_START) == 0u)
        {
            for (uint32_t k = first_item;
                 k < (uint32_t)_b->out->count;
                 ++k)
            {
                struct dj_syntax_item* const  param = &_b->out->items[k];
                const struct dj_token* const  start = &_b->tokens[param->first];

                param->flags |= D_SYNTAX_ITEM_FLAG_ALIGNED;

                if ( ((start->flags & D_TOKEN_FLAG_LINE_START) != 0u) &&
                     (start->span.column != lead->span.column) )
                {
                    param->flags |= D_SYNTAX_ITEM_FLAG_MISALIGNED;
                }
            }
        }
    }

    // claim tokens, outer items first
    for (uint32_t k = first_item; k < (uint32_t)_b->out->count; ++k)
    {
        const struct dj_syntax_item* const item = &_b->out->items[k];

        for (uint32_t at = item->first; at <= item->last; ++at)
        {
            _b->out->item_of[at] = k;
        }
    }

    return true;
}


/**
 * @brief Reports whether a directive begins at a token and is named by one of
 *        a list of words.
 *
 * @param[in] _b      the recognition in progress.
 * @param[in] _at     the token index; may be past the end.
 * @param[in] _words  the names, `NULL`-terminated.
 * @return `true` if a directive so named begins at `_at`, `false` otherwise.
 */
static
bool d_internal_directive_is(
    const struct dj_internal_build* _b,
    uint32_t                        _at,
    const char* const*              _words
)
{
    if ( ((_at + 1u) >= _b->count) ||
         (!d_syntax_is_directive_start(&_b->tokens[_at])) ||
         ((_b->tokens[_at + 1u].flags & D_TOKEN_FLAG_DIRECTIVE_NAME) == 0u) )
    {
        return false;
    }

    return d_internal_spelled(_b->text + _b->tokens[_at + 1u].span.offset,
                              _b->tokens[_at + 1u].span.length,
                              _words);
}


/**
 * @brief Reports the first token at or after an index that is no comment.
 *
 * @param[in] _b   the recognition in progress.
 * @param[in] _at  where to begin.
 * @return the token's index, or the token count when only comments remain.
 */
static
uint32_t d_internal_past_comments(
    const struct dj_internal_build* _b,
    uint32_t                        _at
)
{
    while ( (_at < _b->count) &&
            ( (_b->tokens[_at].kind == D_TOKEN_COMMENT) ||
              (_b->tokens[_at].kind == D_TOKEN_END) ) )
    {
        ++_at;
    }

    return _at;
}


/**
 * @brief Reports whether the file is held in an include guard: an `#ifndef X`
 *        that is its first token past comments, a `#define X` that is the
 *        next, and the `#endif` that closes the first as its last directive,
 *        with nothing but comments after it.
 *
 * Nothing else is a guard: an `#if` around the whole of a source, after an
 * `#include`, is a conditional like any other (lane H, 11-05 (2)).
 *
 * @param[in] _b  the recognition in progress, its depths counted.
 * @return `true` if the file has an include guard, `false` otherwise.
 */
static
bool d_internal_pp_guarded(
    const struct dj_internal_build* _b
)
{
    static const char* const ifndef[] = { "ifndef", NULL };
    static const char* const define[] = { "define", NULL };
    static const char* const endif[]  = { "endif", NULL };

    const uint32_t open = d_internal_past_comments(_b, 0u);

    if ( (!d_internal_directive_is(_b, open, ifndef)) ||
         ((open + 2u) > d_internal_extent(_b, open)) )
    {
        return false;
    }

    const uint32_t named = d_internal_past_comments(_b,
                               d_internal_extent(_b, open) + 1u);
    const struct dj_token* const guard = &_b->tokens[open + 2u];

    // the `#define` names what the `#ifndef` tests
    if ( (!d_internal_directive_is(_b, named, define))           ||
         ((named + 2u) > d_internal_extent(_b, named))            ||
         (_b->tokens[named + 2u].span.length != guard->span.length) ||
         (memcmp(_b->text + _b->tokens[named + 2u].span.offset,
                 _b->text + guard->span.offset,
                 guard->span.length) != 0) )
    {
        return false;
    }

    // the first directive back at the guard's own depth closes it: an
    // `#endif`, not an `#else`, and nothing but comments after it
    for (uint32_t at = d_internal_extent(_b, open) + 1u; at < _b->count; ++at)
    {
        if ( (d_syntax_is_directive_start(&_b->tokens[at])) &&
             (_b->pp[at] == 0u) )
        {
            return ( (d_internal_directive_is(_b, at, endif)) &&
                     (d_internal_past_comments(_b,
                          d_internal_extent(_b, at) + 1u) == _b->count) );
        }
    }

    return false;
}


/**
 * @brief Counts, for every token, the preprocessor conditionals open around it.
 *
 * A conditional's own `#if`, `#else` and `#endif` lines stand outside it.
 *
 * @param[in,out] _b  the recognition in progress.
 */
static
void d_internal_pp_depths(
    struct dj_internal_build* _b
)
{
    unsigned depth = 0u;

    for (uint32_t at = 0u; at < _b->count; )
    {
        const struct dj_token* const token = &_b->tokens[at];

        if (!d_syntax_is_directive_start(token))
        {
            _b->pp[at] = (uint8_t)((depth > 255u) ? 255u : depth);
            ++at;
            continue;
        }

        const uint32_t last = (_b->groups->match[at] == D_SYNTAX_NONE)
                            ? at
                            : _b->groups->match[at];
        unsigned       own  = depth;
        unsigned       next = depth;

        if ( ((at + 1u) <= last) &&
             ((_b->tokens[at + 1u].flags & D_TOKEN_FLAG_DIRECTIVE_NAME) != 0u) )
        {
            const struct dj_token* const name = &_b->tokens[at + 1u];
            const char* const           text = _b->text + name->span.offset;
            const size_t                size = name->span.length;

            static const char* const opening[] =
            {
                "if", "ifdef", "ifndef", NULL
            };
            static const char* const middle[] =
            {
                "else", "elif", "elifdef", "elifndef", NULL
            };
            static const char* const closing[] =
            {
                "endif", NULL
            };

            if (d_internal_spelled(text, size, opening))
            {
                next = depth + 1u;
            }
            else if (d_internal_spelled(text, size, closing))
            {
                own  = (depth > 0u) ? (depth - 1u) : 0u;
                next = own;
            }
            else if (d_internal_spelled(text, size, middle))
            {
                own = (depth > 0u) ? (depth - 1u) : 0u;
            }
        }

        for (uint32_t line = at; line <= last; ++line)
        {
            _b->pp[line] = (uint8_t)((own > 255u) ? 255u : own);
        }

        depth = next;
        at    = last + 1u;
    }

    // an include guard is no conditional a layout rule cares about
    _b->file_pp = (d_internal_pp_guarded(_b)) ? 1u : 0u;

    return;
}


/**
 * @brief Rebases every token's count of open conditionals past the file's
 *        include guard, if it has one, as an item's `pp_total` is: what the
 *        recognition keeps as `pp_of`.
 *
 * @param[in,out] _b  the recognition, its walk done.
 */
static
void d_internal_pp_rebase(
    struct dj_internal_build* _b
)
{
    for (uint32_t at = 0u; ( (_b->pp) && (at < _b->count) ); ++at)
    {
        _b->pp[at] = (_b->pp[at] > _b->file_pp)
                   ? (uint8_t)(_b->pp[at] - _b->file_pp)
                   : 0u;
    }

    return;
}


/**
 * @brief Records how each function's body ends -- empty, or with `return` --
 *        once every body has been walked.
 *
 * @param[in,out] _b  the recognition, its walks done.
 */
static
void d_internal_body_ends(
    struct dj_internal_build* _b
)
{
    struct dj_syntax_items* const out = _b->out;

    for (size_t k = 0u; k < out->count; ++k)
    {
        struct dj_syntax_item* const item = &out->items[k];

        if ( (item->kind != D_SYNTAX_ITEM_FN_DEF) ||
             (item->body == D_SYNTAX_NONE) )
        {
            continue;
        }

        const uint32_t last = _b->last_item[item->body];

        if (last == D_SYNTAX_NONE)
        {
            item->flags |= D_SYNTAX_ITEM_FLAG_EMPTY_BODY;
        }
        else if ( (out->items[last].kind == D_SYNTAX_ITEM_STATEMENT) &&
                  (out->items[last].statement == D_SYNTAX_STMT_RETURN) )
        {
            item->flags |= D_SYNTAX_ITEM_FLAG_ENDS_RETURN;
        }
    }

    return;
}


/**
 * @brief Walks the file, then every scope in the order of its opener, so that
 *        an item always exists before the scopes inside it are walked; then
 *        records how each function's body ends.
 *
 * @param[in,out] _b  the recognition, its depths counted.
 * @return `true` on success, `false` if memory ran out.
 */
static
bool d_internal_walk_all(
    struct dj_internal_build* _b
)
{
    bool ok = d_internal_walk(_b, D_SYNTAX_NONE, D_INTERNAL_MODE_FILE);

    for (uint32_t at = 0u; ( (ok) && (at < _b->count) ); ++at)
    {
        if ( (_b->mode[at] != D_INTERNAL_MODE_NONE)          &&
             (d_syntax_is_opener(_b->tokens[at].kind))        &&
             (_b->groups->match[at] != D_SYNTAX_NONE) )
        {
            ok = d_internal_walk(_b, at, _b->mode[at]);
        }
    }

    // how each function's body ends, now that every body has been walked
    if (ok)
    {
        d_internal_body_ends(_b);
    }

    return ok;
}


//==============================================================================
// 6.  PUBLIC INTERFACE
//==============================================================================


/*
d_syntax_items_build
  Recognizes the items of a grouped token stream.
*/
bool d_syntax_items_build(
    const struct dj_token*         _tokens,
    size_t                         _count,
    const struct dj_syntax_groups* _groups,
    const char*                    _text,
    struct dj_syntax_items*        _out_items
)
{
    return d_syntax_items_build_with(_tokens, _count, _groups, _text, NULL,
                                     _out_items);
}


/*
d_syntax_items_build_with
  Recognizes the items of a grouped token stream, told what tokens cannot
tell.
*/
bool d_syntax_items_build_with(
    const struct dj_token*          _tokens,
    size_t                          _count,
    const struct dj_syntax_groups*  _groups,
    const char*                     _text,
    const struct dj_syntax_options* _options,
    struct dj_syntax_items*         _out_items
)
{
    struct dj_internal_build build;

    // parameter validation first
    if ( (!_out_items) || (!_groups) || (!_text) ||
         ((!_tokens) && (_count > 0u)) )
    {
        return false;
    }

    memset(_out_items, 0, sizeof(*_out_items));
    memset(&build, 0, sizeof(build));

    _out_items->token_count = _count;

    // nothing to recognize
    if (_count == 0u)
    {
        return true;
    }

    size_t wide = 0u;

    // the arrays are indexed by 32 bits, and their sizes must fit a size_t
    if ( (_count > (size_t)0xFFFFFFFFu) ||
         (d_ckd_mul(&wide, _count, sizeof(uint32_t))) )
    {
        return false;
    }

    build.tokens  = _tokens;
    build.count   = (uint32_t)_count;
    build.groups  = _groups;
    build.text    = _text;
    build.out     = _out_items;
    build.options = _options;

    _out_items->item_of  = malloc(wide);
    _out_items->level_of = malloc(_count);
    build.mode           = malloc(_count);
    build.owner          = malloc(wide);
    build.last_item      = malloc(wide);
    _out_items->pp_of    = malloc(_count);
    build.pp             = _out_items->pp_of;

    bool ok = ( _out_items->item_of && _out_items->level_of &&
                build.mode && build.owner && build.last_item && build.pp );

    if (ok)
    {
        for (size_t at = 0u; at < _count; ++at)
        {
            _out_items->item_of[at]  = D_SYNTAX_NONE;
            _out_items->level_of[at] = 0xFFu;
            build.mode[at]           = D_INTERNAL_MODE_NONE;
            build.owner[at]          = D_SYNTAX_NONE;
            build.last_item[at]      = D_SYNTAX_NONE;
        }

        d_internal_pp_depths(&build);
        ok = d_internal_walk_all(&build);
    }

    d_internal_pp_rebase(&build);
    free(build.scratch);
    free(build.mode);
    free(build.owner);
    free(build.last_item);

    if (!ok)
    {
        d_syntax_items_free(_out_items);
    }

    return ok;
}


/*
d_syntax_items_free
  Releases a recognition's arrays.  The structure itself is the caller's.
*/
void d_syntax_items_free(
    struct dj_syntax_items* _items
)
{
    // parameter validation first
    if (!_items)
    {
        return;
    }

    free(_items->items);
    free(_items->item_of);
    free(_items->level_of);
    free(_items->pp_of);

    memset(_items, 0, sizeof(*_items));

    return;
}


/*
d_syntax_item_kind_name
  Spells an item kind as a node type.
*/
const char* d_syntax_item_kind_name(
    unsigned _kind
)
{
    static const char* const names[] =
    {
        "declaration", "fn-def", "fn-decl", "typedef", "statement",
        "label", "param", "enumerator", "block"
    };

    return (_kind < (sizeof(names) / sizeof(names[0]))) ? names[_kind]
                                                        : "item";
}


/*
d_syntax_statement_name
  Spells a statement kind as an attribute value.
*/
const char* d_syntax_statement_name(
    unsigned _statement
)
{
    static const char* const names[] =
    {
        "", "simple", "compound", "empty", "if", "for", "while", "do",
        "switch", "return", "break", "continue", "goto", "try", "macro",
        "case", "default", "named", "access", "open", "close"
    };

    return (_statement < (sizeof(names) / sizeof(names[0])))
           ? names[_statement]
           : "";
}
