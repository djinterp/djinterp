/*******************************************************************************
* djinterp [parse]                                                 syntax_item.c
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
*
*
* path:      /src/djinterp/parse/syntax/syntax_item.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.02
*******************************************************************************/
#include "../../../../inc/djinterp/parse/syntax/syntax_item.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcmp, memset
// djinterp
#include "../../../../inc/djinterp/parse/c/storage.h"  // d_parse_grow
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t, uint16_t,
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
// d_internal_mode
//   enum: how a scope's children divide into items.  A group with no mode is
//   not a scope -- a condition, an argument list, an initializer -- and is
//   never walked; its tokens belong to the item around it.
enum d_internal_mode
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
// d_internal_state
//   enum: where an open item stands in its grammar.  ELSE, DO_WHILE and
//   CATCH are the states in which an item has a complete body and waits one
//   child to learn whether it continues.
enum d_internal_state
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
struct d_internal_scratch
{
    uint32_t  prev;          // previous significant child
    uint32_t  fn_name;       // candidate function name
    uint32_t  fn_params;     // candidate parameter group
    uint32_t  struct_at;     // first struct, union, class or enum keyword
    uint32_t  assign_at;     // first `=`
    uint32_t  operator_at;   // an `operator` keyword
    uint32_t  provisional;   // end of a completed body, while waiting
    uint16_t  seen;          // significant children so far
    uint16_t  angle;         // open `<` of a template's parameter list
    uint8_t   state;
    uint8_t   saw_typedef;
    uint8_t   linkage;
    uint8_t   struct_body;
    uint8_t   macro_head;
    uint8_t   else_seen;
    uint8_t   enum_like;
    uint8_t   init_list;     // a `:` after the parameter group: a
                             // constructor's initializer list is open
    uint8_t   base_clause;   // a `:` after a tag keyword and before any
                             // parameter group: a class's bases are open
    uint16_t  head_angle;    // open `<` of the template arguments a tag's
                             // name is given, before any parameter group
};

// 1.3.2
// d_internal_build
//   struct: one recognition.  The four per-token arrays hold what a scope
//   walk records for the scopes nested inside it, which are walked later.
struct d_internal_build
{
    const struct d_token*          tokens;
    uint32_t                       count;
    const struct d_syntax_groups*  groups;
    const char*                    text;
    struct d_syntax_items*         out;
    struct d_internal_scratch*     scratch;

    uint8_t*                       mode;       // per opener: its scope mode
    uint32_t*                      owner;      // per opener: owning item
    uint32_t*                      last_item;  // per opener: last item in it
    uint8_t*                       pp;         // per token: open conditionals,
                                               // the include guard excluded
};

// 1.3.3
// d_internal_scope
//   struct: one scope's walk.  `cur` is the innermost open item; the items
//   around it are reached through `parent`.
struct d_internal_scope
{
    uint32_t  opener;
    uint32_t  end;
    uint8_t   mode;
    uint8_t   base_level;
    uint8_t   base_pp;
    bool      under_label;
    bool      after_label;   // nothing has been opened since a `case` or
                             // `default` label ended
    uint32_t  ns_indents;    // per open namespace macro: whether it indents
    uint8_t   ns_depth;      // namespace macros open, in a file scope
    uint32_t  skip_to;       // children up to here are an item already made,
                             // or D_SYNTAX_NONE
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
static uint32_t
d_internal_extent(
    const struct d_internal_build* _b,
    uint32_t                       _at
)
{
    const struct d_token* const token = &_b->tokens[_at];

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
static uint32_t
d_internal_next_child(
    const struct d_internal_build* _b,
    uint32_t                       _at,
    uint32_t                       _scope,
    uint32_t                       _end
)
{
    const struct d_token* const token = &_b->tokens[_at];

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
static bool
d_internal_is_trivia(
    const struct d_token* _token
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
static uint32_t
d_internal_first_inner(
    const struct d_internal_build* _b,
    uint32_t                       _opener
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
static uint32_t
d_internal_inner_identifier(
    const struct d_internal_build* _b,
    uint32_t                       _opener
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
static bool
d_internal_is_decl_keyword(
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
static bool
d_internal_is_type_specifier(
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
static bool
d_internal_is_tag_keyword(
    int _kind
)
{
    return ( (_kind == D_TOKEN_KW_STRUCT) || (_kind == D_TOKEN_KW_UNION) ||
             (_kind == D_TOKEN_KW_ENUM)   || (_kind == D_TOKEN_KW_CLASS) );
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
static uint32_t
d_internal_new_item(
    struct d_internal_build* _b
)
{
    struct d_syntax_items* const out = _b->out;

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

        struct d_syntax_item* const items =
            d_parse_grow(out->items,
                         &items_capacity,
                         needed,
                         (uint32_t)sizeof(struct d_syntax_item),
                         1);

        // check if memory allocation was successful
        if (!items)
        {
            return D_SYNTAX_NONE;
        }

        out->items = items;

        struct d_internal_scratch* const scratch =
            d_parse_grow(_b->scratch,
                         &scratch_capacity,
                         needed,
                         (uint32_t)sizeof(struct d_internal_scratch),
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
    struct d_syntax_item* const      item    = &out->items[at];
    struct d_internal_scratch* const scratch = &_b->scratch[at];

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
static void
d_internal_set_scope(
    struct d_internal_build* _b,
    uint32_t                 _opener,
    uint8_t                  _mode,
    uint32_t                 _owner
)
{
    _b->mode[_opener]          = _mode;
    _b->owner[_opener]         = _owner;
    _b->out->level_of[_opener] = _b->out->items[_owner].level;

    return;
}


/**
 * @brief Follows a template argument list through a declaration's head.
 *
 * @note A `<` that follows a name, or `template`, opens one; each `>` closes
 *       one and `>>` two. A caller that stops at an initializer never meets a
 *       comparison, so in a declaration's head a `<` after a name is always
 *       the list's. C has neither, and is left as it was.
 *
 * @param[in]     _kind   the token's kind.
 * @param[in]     _prev   the kind of the significant token before it.
 * @param[in,out] _angle  the lists open before the token; after, on return.
 * @return `true` if the token opens a list, lies inside one or closes one,
 *         and so is no part of the declaration's own spelling.
 */
static bool
d_internal_angle(
    int       _kind,
    int       _prev,
    unsigned* _angle
)
{
    if (*_angle == 0u)
    {
        // the list opens after a name or the keyword
        if ( (_kind == D_TOKEN_LESS) &&
             ( (_prev == D_TOKEN_IDENTIFIER) ||
               (_prev == D_TOKEN_KW_TEMPLATE) ) )
        {
            *_angle = 1u;

            return true;
        }

        return false;
    }

    if (_kind == D_TOKEN_LESS)
    {
        ++*_angle;
    }
    else if (_kind == D_TOKEN_GREATER)
    {
        --*_angle;
    }
    else if (_kind == D_TOKEN_SHIFT_RIGHT)
    {
        *_angle = (*_angle > 2u) ? (*_angle - 2u) : 0u;
    }

    return true;
}

/**
 * @brief Finds what a declaration or typedef declares.
 *
 * A parenthesis whose first child is `*` and which is followed by a parameter
 * group declares a function pointer, named inside it; an identifier followed by
 * a parameter group declares a function type; otherwise the declared name is
 * the last identifier before an initializer, a bitfield or the end, and an
 * identifier following a tag keyword is the tag and never the name.
 *
 * @param[in,out] _b  the recognition in progress.
 * @param[in]     _k  the item's index.
 */
static void
d_internal_declarator(
    struct d_internal_build* _b,
    uint32_t                 _k
)
{
    struct d_syntax_item* const item  = &_b->out->items[_k];
    const uint32_t              scope = item->scope;
    const uint32_t              end   = item->last + 1u;

    uint32_t name      = D_SYNTAX_NONE;
    int      prev_kind = D_TOKEN_END;
    unsigned angle     = 0u;

    for (uint32_t at = item->first;
         at < end;
         at = d_internal_next_child(_b, at, scope, end))
    {
        const struct d_token* const token = &_b->tokens[at];

        if (d_internal_is_trivia(token))
        {
            continue;
        }

        // a template argument list holds neither the name nor the comma
        // that would end the search: `std::map<int, d_x> m_map` declares
        // m_map
        if (d_internal_angle(token->kind, prev_kind, &angle))
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
static void
d_internal_classify_param(
    struct d_internal_build* _b,
    uint32_t                 _k
)
{
    struct d_syntax_item* const item  = &_b->out->items[_k];
    const uint32_t              scope = item->scope;
    const uint32_t              end   = item->last + 1u;

    uint32_t count      = 0u;
    int      first_kind = D_TOKEN_END;
    int      prev_kind  = D_TOKEN_END;
    uint32_t candidate  = D_SYNTAX_NONE;
    int      cand_prev  = D_TOKEN_END;
    uint32_t specs      = 0u;
    uint32_t cand_specs = 0u;
    unsigned angle      = 0u;

    for (uint32_t at = item->first;
         at < end;
         at = d_internal_next_child(_b, at, scope, end))
    {
        const struct d_token* const token = &_b->tokens[at];

        // a template argument list is part of the type: what it holds is
        // no candidate, and the `>` that closes it is what the parameter
        // so far ends with
        if (d_internal_angle(token->kind, prev_kind, &angle))
        {
            if (angle == 0u)
            {
                candidate  = at;
                cand_prev  = prev_kind;
                cand_specs = specs;
            }

            prev_kind = token->kind;

            continue;
        }

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
         (cand_prev != D_TOKEN_SCOPE)            &&
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
 * @param[in] _b  the recognition in progress.
 * @param[in] _k  the item's index.
 * @return `true` if the function returns `void` and not a pointer, `false`
 *         otherwise.
 */
static bool
d_internal_returns_void(
    const struct d_internal_build* _b,
    uint32_t                       _k
)
{
    const struct d_syntax_item* const item  = &_b->out->items[_k];
    const uint32_t                    scope = item->scope;
    const uint32_t                    end   = item->name;
    bool                              seen  = false;
    unsigned                          angle = 0u;
    int                               prev  = D_TOKEN_END;

    if (end == D_SYNTAX_NONE)
    {
        return false;
    }

    for (uint32_t at = item->first;
         at < end;
         at = d_internal_next_child(_b, at, scope, end))
    {
        const int kind = _b->tokens[at].kind;

        // a template argument list: `result<void>` returns a result
        if (d_internal_angle(kind, prev, &angle))
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
 * @brief Reports whether two tokens are spelled alike.
 *
 * @param[in] _b  the recognition in progress.
 * @param[in] _x  one token's index.
 * @param[in] _y  the other's.
 * @return `true` if their text is the same, `false` otherwise.
 */
static bool
d_internal_same_spelling(
    const struct d_internal_build* _b,
    uint32_t                       _x,
    uint32_t                       _y
)
{
    const struct d_token* const x = &_b->tokens[_x];
    const struct d_token* const y = &_b->tokens[_y];

    return ( (x->span.length == y->span.length) &&
             (memcmp(_b->text + x->span.offset,
                     _b->text + y->span.offset,
                     x->span.length) == 0) );
}

/**
 * @brief Finds the name of the class whose member list a scope is.
 *
 * @param[in] _b  the recognition in progress.
 * @param[in] _s  the scope.
 * @return the tag's token index: the identifier after the `struct`, `class`
 *         or `union` of the item that owns the scope; D_SYNTAX_NONE when the
 *         scope is no member list or the class has no name.
 */
static uint32_t
d_internal_class_tag(
    const struct d_internal_build* _b,
    const struct d_internal_scope* _s
)
{
    if ( (_s->mode != D_INTERNAL_MODE_MEMBERS) ||
         (_s->opener == D_SYNTAX_NONE) )
    {
        return D_SYNTAX_NONE;
    }

    const uint32_t owner = _b->owner[_s->opener];

    if (owner == D_SYNTAX_NONE)
    {
        return D_SYNTAX_NONE;
    }

    const uint32_t tag = _b->scratch[owner].struct_at;

    // the keyword, and the identifier straight after it
    if ( (tag == D_SYNTAX_NONE)          ||
         ((tag + 1u) >= _b->count)       ||
         (_b->tokens[tag + 1u].kind != D_TOKEN_IDENTIFIER) )
    {
        return D_SYNTAX_NONE;
    }

    return tag + 1u;
}

/**
 * @brief Reports whether a function has no return type: a constructor or a
 *        destructor.
 *
 * @note Three shapes: `~name`; `name::name`, a constructor defined outside
 *       its class; and, in a member list, the class's own name. A function
 *       whose name merely follows specifiers cannot be told from one whose
 *       name follows a type, so the class's name is what decides.
 *
 * @param[in] _b  the recognition in progress.
 * @param[in] _s  the scope the function was found in.
 * @param[in] _k  the function's item index.
 * @return `true` for a constructor or a destructor, `false` otherwise.
 */
static bool
d_internal_no_return_type(
    const struct d_internal_build* _b,
    const struct d_internal_scope* _s,
    uint32_t                       _k
)
{
    const struct d_syntax_item* const item = &_b->out->items[_k];
    const uint32_t                    name = item->name;

    if ( (name == D_SYNTAX_NONE) ||
         (_b->tokens[name].kind != D_TOKEN_IDENTIFIER) )
    {
        return false;
    }

    // `~name`: a destructor, in its class or out of it
    if ( (name > item->first) &&
         (_b->tokens[name - 1u].kind == D_TOKEN_TILDE) )
    {
        return true;
    }

    // `name::name`: the qualifier is spelled like the function
    if ( (name > item->first) &&
         (_b->tokens[name - 1u].kind == D_TOKEN_SCOPE) )
    {
        for (uint32_t at = item->first; at < (name - 1u); ++at)
        {
            if ( (_b->tokens[at].kind == D_TOKEN_IDENTIFIER) &&
                 (d_internal_same_spelling(_b, at, name)) )
            {
                return true;
            }
        }
    }

    const uint32_t tag = d_internal_class_tag(_b, _s);

    return ( (tag != D_SYNTAX_NONE) &&
             (d_internal_same_spelling(_b, tag, name)) );
}

/**
 * @brief Settles what a just-completed item is.
 *
 * @param[in,out] _b  the recognition in progress.
 * @param[in]     _s  the scope being walked.
 * @param[in]     _k  the item's index.
 */
static void
d_internal_classify(
    struct d_internal_build*       _b,
    const struct d_internal_scope* _s,
    uint32_t                       _k
)
{
    struct d_syntax_item* const      item    = &_b->out->items[_k];
    struct d_internal_scratch* const scratch = &_b->scratch[_k];
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
            else if (d_internal_no_return_type(_b, _s, _k))
            {
                item->flags |= D_SYNTAX_ITEM_FLAG_NO_RETURN;
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

        if (d_internal_no_return_type(_b, _s, _k))
        {
            item->flags |= D_SYNTAX_ITEM_FLAG_NO_RETURN;
        }
        else if (d_internal_returns_void(_b, _k))
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
static bool
d_internal_body_done(
    struct d_internal_build* _b,
    uint32_t                 _k,
    uint32_t                 _last
)
{
    struct d_internal_scratch* const scratch   = &_b->scratch[_k];
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
static void
d_internal_complete(
    struct d_internal_build* _b,
    struct d_internal_scope* _s,
    uint32_t                 _k,
    uint32_t                 _last
)
{
    uint32_t k = _k;

    for (;;)
    {
        struct d_syntax_item* const item = &_b->out->items[k];

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
 * @brief Counts the conditionals an item stands in, or stands under.
 *
 * @note An `if` whose head is chosen by `#if` has two texts and one body:
 *
 *           #if D_HAS_RDHUP
 *               if (_bits & (EPOLLHUP | EPOLLRDHUP))
 *           #else
 *               if (_bits & EPOLLHUP)
 *           #endif
 *               {
 *
 *       Read as it is written that is one `if` inside another, and the body
 *       a level further in than it is. What an item owns takes the item's
 *       conditionals with it: the layout rules ask nothing of it, as they
 *       ask nothing of what stands inside the `#if` itself. A transparent
 *       block passes nothing on -- every C header's declarations stand in
 *       an `extern "C"` opened under `#ifdef __cplusplus`.
 *
 * @param[in] _b       the recognition in progress.
 * @param[in] _s       the scope the item is opened in.
 * @param[in] _at      the item's first token.
 * @param[in] _parent  the item it is a braceless body of, or
 *                     `D_SYNTAX_NONE`.
 * @return the conditionals around the token, or around the item's owner if
 *         that is more.
 */
static uint8_t
d_internal_pp_reach(
    const struct d_internal_build* _b,
    const struct d_internal_scope* _s,
    uint32_t                       _at,
    uint32_t                       _parent
)
{
    const uint32_t owner =
        (_parent != D_SYNTAX_NONE)
        ? _parent
        : ( (_s->opener != D_SYNTAX_NONE) &&
            (_s->mode != D_INTERNAL_MODE_FILE) )
          ? _b->owner[_s->opener]
          : D_SYNTAX_NONE;
    const uint8_t  own   = _b->pp[_at];

    if ( (owner != D_SYNTAX_NONE) &&
         (_b->out->items[owner].pp_total > own) )
    {
        return _b->out->items[owner].pp_total;
    }

    return own;
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
static uint32_t
d_internal_open(
    struct d_internal_build* _b,
    struct d_internal_scope* _s,
    uint32_t                 _at,
    uint32_t                 _parent
)
{
    const uint32_t k = d_internal_new_item(_b);

    if (k == D_SYNTAX_NONE)
    {
        return D_SYNTAX_NONE;
    }

    struct d_syntax_item* const      item    = &_b->out->items[k];
    struct d_internal_scratch* const scratch = &_b->scratch[k];
    const int                        kind    = _b->tokens[_at].kind;
    const bool                       label   =
        ((kind == D_TOKEN_KW_CASE) || (kind == D_TOKEN_KW_DEFAULT));

    item->first  = _at;
    item->last   = _at;
    item->parent = _parent;
    item->scope  = _s->opener;

    // under a label a statement is indented a level, but the label's own
    // braced body is not: `case X:` then `{` on the label's column, its
    // contents one in.  That is the brace that follows the label and
    // nothing else; a block further down the same case is a statement
    // among the others, and indented as they are
    const bool     compound = ( (kind == D_TOKEN_BRACE_OPEN) &&
                                (_s->after_label) );
    const bool     indented = ( _s->under_label &&
                                (!label)        &&
                                (!compound) );
    const unsigned level    =
        (_parent != D_SYNTAX_NONE)
        ? (unsigned)_b->out->items[_parent].level + 1u
        : (unsigned)_s->base_level + (indented ? 1u : 0u);

    // anything but another label ends the place just after the label:
    // `case 1:` `case 2:` `{` is both labels' body
    if ( (!label) && (_parent == D_SYNTAX_NONE) )
    {
        _s->after_label = false;
    }

    item->level    = (uint8_t)((level > 255u) ? 255u : level);
    item->pp_depth = (_b->pp[_at] > _s->base_pp)
                   ? (uint8_t)(_b->pp[_at] - _s->base_pp)
                   : 0u;
    item->pp_total = d_internal_pp_reach(_b, _s, _at, _parent);

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
static void
d_internal_note(
    struct d_internal_build*       _b,
    const struct d_internal_scope* _s,
    uint32_t                       _k,
    uint32_t                       _at
)
{
    struct d_internal_scratch* const scratch = &_b->scratch[_k];
    const int                        kind    = _b->tokens[_at].kind;
    const bool in_operator =
        ( (scratch->operator_at != D_SYNTAX_NONE) &&
          (scratch->fn_params == D_SYNTAX_NONE) );

    // inside a template's parameter list nothing is the item's own: its `=`
    // gives a default argument and its `class` introduces a parameter, and
    // neither may be taken for an initializer or a tag.  The list is told by
    // the `<` straight after `template`, and its end by the `>` that closes
    // it; `>>` closes two, as it does from C++11
    if (scratch->angle > 0u)
    {
        if (kind == D_TOKEN_LESS)
        {
            ++scratch->angle;
        }
        else if (kind == D_TOKEN_GREATER)
        {
            --scratch->angle;
        }
        else if (kind == D_TOKEN_SHIFT_RIGHT)
        {
            scratch->angle = (uint16_t)((scratch->angle > 2u)
                                        ? (scratch->angle - 2u)
                                        : 0u);
        }

        scratch->prev = _at;

        if (scratch->seen < 0xFFFFu)
        {
            ++scratch->seen;
        }

        return;
    }

    if ( (kind == D_TOKEN_LESS)            &&
         (scratch->prev != D_SYNTAX_NONE)  &&
         (_b->tokens[scratch->prev].kind == D_TOKEN_KW_TEMPLATE) )
    {
        scratch->angle = 1u;
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

    // `name(...) :` opens a constructor's initializer list, where a brace
    // after a name initializes a member: `d_internal_brace` reads this
    if ( (kind == D_TOKEN_COLON)                   &&
         (scratch->fn_params != D_SYNTAX_NONE)     &&
         (scratch->assign_at == D_SYNTAX_NONE) )
    {
        scratch->init_list = 1u;
    }

    // `struct name :` opens a class's bases, and what a base is given in
    // parentheses is an argument of it -- `: mixin<has(_a, _b)>` -- never
    // the parameters of a function: the brace after the bases is the
    // class's member list
    if ( (kind == D_TOKEN_COLON)                   &&
         (scratch->struct_at != D_SYNTAX_NONE)     &&
         (scratch->fn_params == D_SYNTAX_NONE)     &&
         (scratch->assign_at == D_SYNTAX_NONE) )
    {
        scratch->base_clause = 1u;
    }

    // `struct name<...>`: the template arguments a tag's name is given,
    // where a parenthesis after a name spells a function type --
    // `traits<Return(Args...)>` -- and is no parameter list either.  The
    // arguments end at the `>` that closes them, so the function a
    // `struct node<Type>* make(int _n)` declares is still seen
    if ( (scratch->struct_at != D_SYNTAX_NONE)     &&
         (scratch->fn_params == D_SYNTAX_NONE)     &&
         (scratch->assign_at == D_SYNTAX_NONE) )
    {
        if ( (kind == D_TOKEN_LESS)                              &&
             ( (scratch->head_angle > 0u)                    ||
               ( (scratch->prev != D_SYNTAX_NONE)            &&
                 (_b->tokens[scratch->prev].kind == D_TOKEN_IDENTIFIER) ) ) )
        {
            ++scratch->head_angle;
        }
        else if ( (kind == D_TOKEN_GREATER) && (scratch->head_angle > 0u) )
        {
            --scratch->head_angle;
        }
        else if (kind == D_TOKEN_SHIFT_RIGHT)
        {
            scratch->head_angle = (uint16_t)((scratch->head_angle > 2u)
                                             ? (scratch->head_angle - 2u)
                                             : 0u);
        }
    }

    // the first parenthesis after a name, before any `=`, is a candidate
    // parameter list
    if ( (kind == D_TOKEN_PAREN_OPEN) &&
         (scratch->fn_params == D_SYNTAX_NONE) &&
         (scratch->assign_at == D_SYNTAX_NONE) &&
         (scratch->base_clause == 0u)          &&
         (scratch->head_angle == 0u)           &&
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
            const uint32_t tag = d_internal_class_tag(_b, _s);

            scratch->macro_head =
                ( (scratch->seen == 1u) &&
                  ( (tag == D_SYNTAX_NONE) ||
                    (!d_internal_same_spelling(_b, tag, scratch->prev)) ) )
                ? 1u
                : 0u;
        }
    }

    scratch->prev = _at;

    if (scratch->seen < 0xFFFFu)
    {
        ++scratch->seen;
    }

    return;
}


/**
 * @brief Reports whether a token can be the last of a member's or a base
 *        class's name in a constructor's initializer list.
 *
 * @param[in] _kind  the token's kind.
 * @return `true` for an identifier, and for the `>` or `>>` that closes a
 *         base class's template arguments; `false` otherwise.
 */
static bool
d_internal_ends_a_name(
    int _kind
)
{
    return ( (_kind == D_TOKEN_IDENTIFIER) ||
             (_kind == D_TOKEN_GREATER)    ||
             (_kind == D_TOKEN_SHIFT_RIGHT) );
}

/**
 * @brief Decides what a brace is to an item in its simple state.
 *
 * @param[in,out] _b   the recognition in progress.
 * @param[in,out] _s   the scope being walked.
 * @param[in]     _k   the item's index.
 * @param[in]     _at  the child's token index.
 */
static void
d_internal_brace(
    struct d_internal_build* _b,
    struct d_internal_scope* _s,
    uint32_t                 _k,
    uint32_t                 _at
)
{
    struct d_syntax_item* const      item    = &_b->out->items[_k];
    struct d_internal_scratch* const scratch = &_b->scratch[_k];
    const bool block = (_s->mode == D_INTERNAL_MODE_BLOCK);

    // a brace inside a template's parameter list is a default argument's
    if (scratch->angle > 0u)
    {
        d_internal_note(_b, _s, _k, _at);

        return;
    }

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

    // in a constructor's initializer list, the brace after a member's or a
    // base's name initializes it -- `: m_values{}`, `: base<Type>{_x}` --
    // and the body is the brace after an initializer's own `)` or `}`
    if ( (!block)                                     &&
         (scratch->fn_params != D_SYNTAX_NONE)        &&
         (scratch->init_list != 0u)                   &&
         (scratch->prev != D_SYNTAX_NONE)             &&
         (d_internal_ends_a_name(_b->tokens[scratch->prev].kind)) )
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
 * @brief Gives a child to a parameter or an enumerator, which ends at its
 *        comma.
 *
 * @note `std::pair<int, float> _p` is one parameter: a `<` after a name,
 *       before any default argument, opens a template argument list, and the
 *       commas inside it separate its arguments.
 *
 * @param[in,out] _b   the recognition in progress.
 * @param[in,out] _s   the scope being walked: a parameter group or an
 *                     enumerator list.
 * @param[in]     _k   the item's index.
 * @param[in]     _at  the child's token index.
 */
static void
d_internal_listed(
    struct d_internal_build* _b,
    struct d_internal_scope* _s,
    uint32_t                 _k,
    uint32_t                 _at
)
{
    struct d_internal_scratch* const scratch = &_b->scratch[_k];
    const int                        kind    = _b->tokens[_at].kind;

    // the list opens: the `<` is noted as any child is, and counted after
    if ( (_s->mode == D_INTERNAL_MODE_PARAMS)                  &&
         (kind == D_TOKEN_LESS)                                &&
         (scratch->angle == 0u)                                &&
         (scratch->assign_at == D_SYNTAX_NONE)                 &&
         (scratch->prev != D_SYNTAX_NONE)                      &&
         (_b->tokens[scratch->prev].kind == D_TOKEN_IDENTIFIER) )
    {
        d_internal_note(_b, _s, _k, _at);
        scratch->angle = 1u;

        return;
    }

    // a comma outside any such list ends the item
    if ( (kind == D_TOKEN_COMMA) && (scratch->angle == 0u) )
    {
        d_internal_complete(_b, _s, _k, _at);

        return;
    }

    d_internal_note(_b, _s, _k, _at);

    return;
}

/**
 * @brief Offers a child to an item in its simple state.
 *
 * @param[in,out] _b   the recognition in progress.
 * @param[in,out] _s   the scope being walked.
 * @param[in]     _k   the item's index.
 * @param[in]     _at  the child's token index.
 */
static void
d_internal_simple(
    struct d_internal_build* _b,
    struct d_internal_scope* _s,
    uint32_t                 _k,
    uint32_t                 _at
)
{
    const int kind = _b->tokens[_at].kind;

    // parameters and enumerators end at their commas
    if ( (_s->mode == D_INTERNAL_MODE_PARAMS) ||
         (_s->mode == D_INTERNAL_MODE_ENUM) )
    {
        d_internal_listed(_b, _s, _k, _at);

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

            // on the class's own column: a level out from its members
            if (_b->out->items[_k].level > 0u)
            {
                --_b->out->items[_k].level;
            }

            d_internal_complete(_b, _s, _k, _at);

            return;
        }
    }

    d_internal_note(_b, _s, _k, _at);

    return;
}


/**
 * @brief Reports whether a token is spelled as a word.
 *
 * @param[in] _b     the recognition in progress.
 * @param[in] _at    the token's index.
 * @param[in] _word  the spelling asked about.
 * @return `true` if the token's text is `_word`, `false` otherwise.
 */
static bool
d_internal_token_is(
    const struct d_internal_build* _b,
    uint32_t                       _at,
    const char*                    _word
)
{
    const struct d_token* const token = &_b->tokens[_at];

    return ( (token->span.length == strlen(_word)) &&
             (memcmp(_b->text + token->span.offset,
                     _word,
                     token->span.length) == 0) );
}

/**
 * @brief Reports whether what a scope holds from a token on stands one level
 *        inside a level: its first token that is no comment begins a line,
 *        in the column one level in gives.
 *
 * @note How a namespace's own contents say whether it indents them. The
 *       guide shows the internal namespace indenting and the framework's
 *       not, and of any other says nothing; the tree does both, 47 braced
 *       namespaces to 70. So neither is a finding: the first thing a
 *       namespace holds decides for the rest of it.
 *
 * @param[in] _b      the recognition in progress.
 * @param[in] _from   the first token to look at.
 * @param[in] _end    one past the last.
 * @param[in] _level  the level the namespace itself stands at.
 * @return `true` if the contents are one level in, `false` if they are not
 *         or there are none.
 */
static bool
d_internal_stands_in(
    const struct d_internal_build* _b,
    uint32_t                       _from,
    uint32_t                       _end,
    unsigned                       _level
)
{
    uint32_t at = _from;

    while ( (at < _end) && (d_internal_is_trivia(&_b->tokens[at])) )
    {
        ++at;
    }

    return ( (at < _end)                                                 &&
             ((_b->tokens[at].flags & D_TOKEN_FLAG_LINE_START) != 0u)    &&
             (_b->tokens[at].span.column == (((_level + 1u) * 4u) + 1u)) );
}

/**
 * @brief Finds the end of a namespace macro that stands alone on its line.
 *
 * @note Two forms open a namespace without a brace the layout can see:
 *       `NS_DJINTERP`, `NS_INTERNAL` and their like, and `NS_END`, which
 *       closes one; and `D_NAMESPACE(name)`, which they are defined by and
 *       which a header uses for a namespace of its own.
 *
 * @param[in] _b   the recognition in progress.
 * @param[in] _at  the token's index.
 * @return the macro's last token -- itself, or the `)` of `D_NAMESPACE(...)`
 *         -- when nothing follows it on its line but a comment;
 *         D_SYNTAX_NONE for anything else.
 */
static uint32_t
d_internal_ns_last(
    const struct d_internal_build* _b,
    uint32_t                       _at
)
{
    const struct d_token* const token = &_b->tokens[_at];
    uint32_t                    last  = _at;

    if (token->kind != D_TOKEN_IDENTIFIER)
    {
        return D_SYNTAX_NONE;
    }

    if (d_internal_token_is(_b, _at, "D_NAMESPACE"))
    {
        // the name it is given, in its parentheses
        if ( ((_at + 1u) >= _b->count)                              ||
             (_b->tokens[_at + 1u].kind != D_TOKEN_PAREN_OPEN)      ||
             (_b->groups->match[_at + 1u] == D_SYNTAX_NONE) )
        {
            return D_SYNTAX_NONE;
        }

        last = _b->groups->match[_at + 1u];
    }
    else if ( (token->span.length < 4u) ||
              (memcmp(_b->text + token->span.offset, "NS_", 3u) != 0) )
    {
        return D_SYNTAX_NONE;
    }

    uint32_t next = last + 1u;

    // a comment on the macro's own line
    while ( (next < _b->count)                                         &&
            (_b->tokens[next].kind == D_TOKEN_COMMENT)                 &&
            ((_b->tokens[next].flags & D_TOKEN_FLAG_LINE_START) == 0u) )
    {
        ++next;
    }

    return ( (next >= _b->count) ||
             ((_b->tokens[next].flags & D_TOKEN_FLAG_LINE_START) != 0u) )
           ? last
           : D_SYNTAX_NONE;
}

/**
 * @brief Opens a namespace macro as an item of its own, and keeps the level
 *        of what follows it.
 *
 * @note The framework's namespaces are opened and closed by macros, which
 *       show the layout no brace. `NS_INTERNAL` indents what follows it until
 *       its `NS_END`, as the guide's own example does and the tree does in
 *       596 places of 601; the framework's namespace and a subsystem's do
 *       not, in 555 of 575. `D_NAMESPACE(name)` is a header's own, and
 *       indents if its contents do: see `d_internal_stands_in`. `NS_END`
 *       sits at the level its namespace was opened at, so the level comes
 *       back before the item is placed and goes in after.
 *
 * @param[in,out] _b     the recognition in progress.
 * @param[in,out] _s     the file scope being walked.
 * @param[in]     _at    the macro's token index.
 * @param[in]     _last  its last token: `_at`, or the `)` of
 *                       `D_NAMESPACE(...)`.
 * @return `true` on success, `false` if memory ran out.
 */
static bool
d_internal_ns_item(
    struct d_internal_build* _b,
    struct d_internal_scope* _s,
    uint32_t                 _at,
    uint32_t                 _last
)
{
    const bool closes = d_internal_token_is(_b, _at, "NS_END");

    // the level its namespace was opened at
    if ( (closes) && (_s->ns_depth > 0u) )
    {
        --_s->ns_depth;

        if (((_s->ns_indents >> _s->ns_depth) & 1u) != 0u)
        {
            --_s->base_level;
        }
    }

    const uint32_t k = d_internal_open(_b, _s, _at, D_SYNTAX_NONE);

    if (k == D_SYNTAX_NONE)
    {
        return false;
    }

    // what follows an opening macro: one level in after the internal one,
    // and after a header's own where the header has it so
    if ( (!closes) && (_s->ns_depth < 32u) )
    {
        const bool     indents =
            ( (d_internal_token_is(_b, _at, "NS_INTERNAL")) ||
              ( (_last != _at) &&
                (d_internal_stands_in(_b, _last + 1u, _s->end,
                                      _s->base_level)) ) );
        const uint32_t bit     = (uint32_t)1u << _s->ns_depth;

        _s->ns_indents = indents ? (_s->ns_indents | bit)
                                 : (_s->ns_indents & ~bit);
        _s->base_level = (uint8_t)(_s->base_level + (indents ? 1u : 0u));
        ++_s->ns_depth;
    }

    _s->cur                     = k;
    _s->skip_to                 = (_last != _at) ? _last : D_SYNTAX_NONE;
    _b->out->items[k].kind      = D_SYNTAX_ITEM_STATEMENT;
    _b->out->items[k].statement = D_SYNTAX_STMT_MACRO;
    d_internal_complete(_b, _s, k, _last);

    return true;
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
static bool
d_internal_feed(
    struct d_internal_build* _b,
    struct d_internal_scope* _s,
    uint32_t                 _at
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

            // a namespace macro alone on its line is an item of its own
            if ( (_s->mode == D_INTERNAL_MODE_FILE) &&
                 (d_internal_ns_last(_b, _at) != D_SYNTAX_NONE) )
            {
                return d_internal_ns_item(_b, _s, _at,
                                          d_internal_ns_last(_b, _at));
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
        struct d_internal_scratch* const scratch = &_b->scratch[k];

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
                        _s->after_label = true;
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
 * @brief Reports whether the item that owns a brace is a namespace, and not
 *        the other transparent block, `extern "C"`.
 *
 * @param[in] _b       the recognition in progress.
 * @param[in] _owner   the item's index.
 * @param[in] _opener  the brace's token index.
 * @return `true` if `namespace` stands in the item before the brace.
 */
static bool
d_internal_is_namespace(
    const struct d_internal_build* _b,
    uint32_t                       _owner,
    uint32_t                       _opener
)
{
    for (uint32_t at = _b->out->items[_owner].first; at < _opener; ++at)
    {
        if (_b->tokens[at].kind == D_TOKEN_KW_NAMESPACE)
        {
            return true;
        }
    }

    return false;
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
static bool
d_internal_walk(
    struct d_internal_build* _b,
    uint32_t                 _opener,
    uint8_t                  _mode
)
{
    struct d_internal_scope scope;
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
    scope.after_label = false;
    scope.ns_indents  = 0u;
    scope.ns_depth    = 0u;
    scope.skip_to     = D_SYNTAX_NONE;
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
        const unsigned own   = (unsigned)_b->out->items[owner].level;
        const bool     in    =
            ( (_mode != D_INTERNAL_MODE_FILE) ||
              ( (d_internal_is_namespace(_b, owner, _opener)) &&
                (d_internal_stands_in(_b, _opener + 1u, scope.end, own)) ) );
        const unsigned level = own + (in ? 1u : 0u);

        scope.base_level = (uint8_t)((level > 255u) ? 255u : level);
    }

    const uint32_t begin = (_opener == D_SYNTAX_NONE) ? 0u : (_opener + 1u);

    for (uint32_t at = begin;
         at < scope.end;
         at = d_internal_next_child(_b, at, _opener, scope.end))
    {
        // trivia, and the rest of an item made whole from its first child
        if ( (d_internal_is_trivia(&_b->tokens[at])) ||
             ( (scope.skip_to != D_SYNTAX_NONE) && (at <= scope.skip_to) ) )
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
        struct d_internal_scratch* const scratch = &_b->scratch[k];
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
        const struct d_token* const lead =
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
                struct d_syntax_item* const  param = &_b->out->items[k];
                const struct d_token* const  start = &_b->tokens[param->first];

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
        const struct d_syntax_item* const item = &_b->out->items[k];

        for (uint32_t at = item->first; at <= item->last; ++at)
        {
            _b->out->item_of[at] = k;
        }
    }

    return true;
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
static bool
d_internal_spelled(
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
 * @brief Counts, for every token, the preprocessor conditionals open around
 *        it, the include guard excluded.
 *
 * A conditional's own `#if`, `#else` and `#endif` lines stand outside it.
 *   The include guard is no conditional a rule cares about, and it is told by
 * its shape: the file's first directive an `#ifndef`, the second a `#define`
 * of the macro it tested, and no code before either.  It is not told by where
 * the first declaration lies: a header may open with a declaration under a
 * conditional of its own, and one that holds nothing but directives has no
 * declaration at all.
 *
 * @param[in,out] _b  the recognition in progress.
 */
static void
d_internal_pp_depths(
    struct d_internal_build* _b
)
{
    unsigned depth   = 0u;
    unsigned floor   = 0u;
    unsigned seen    = 0u;
    bool     guarded = true;
    uint32_t tested  = D_SYNTAX_NONE;

    for (uint32_t at = 0u; at < _b->count; )
    {
        const struct d_token* const token = &_b->tokens[at];

        if (!d_syntax_is_directive_start(token))
        {
            // code ahead of the second directive: the file opens unguarded
            if ( (token->kind != D_TOKEN_COMMENT) && (seen < 2u) )
            {
                guarded = false;
            }

            _b->pp[at] = (uint8_t)((depth > 255u) ? 255u : depth);
            ++at;

            continue;
        }

        const uint32_t last  = (_b->groups->match[at] == D_SYNTAX_NONE)
                             ? at
                             : _b->groups->match[at];
        unsigned       own   = depth;
        unsigned       next  = depth;
        bool           tests = false;
        bool           names = false;

        if ( ((at + 1u) <= last) &&
             ((_b->tokens[at + 1u].flags & D_TOKEN_FLAG_DIRECTIVE_NAME) != 0u) )
        {
            const struct d_token* const name = &_b->tokens[at + 1u];
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
            static const char* const testing[] =
            {
                "ifndef", NULL
            };
            static const char* const defining[] =
            {
                "define", NULL
            };

            tests = d_internal_spelled(text, size, testing);
            names = d_internal_spelled(text, size, defining);

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

        // the macro the line tests or defines
        uint32_t macro = D_SYNTAX_NONE;

        for (uint32_t scan = at + 1u;
             (scan <= last) && (macro == D_SYNTAX_NONE);
             ++scan)
        {
            if ((_b->tokens[scan].flags & D_TOKEN_FLAG_MACRO_NAME) != 0u)
            {
                macro = scan;
            }
        }

        // the guard: `#ifndef X` first, `#define X` straight after it
        if ( (seen == 0u) && (guarded) && (tests) )
        {
            tested = macro;
        }
        else if ( (seen == 1u) && (guarded) && (names)     &&
                  (tested != D_SYNTAX_NONE)                &&
                  (macro != D_SYNTAX_NONE)                 &&
                  (_b->tokens[macro].span.length
                   == _b->tokens[tested].span.length)      &&
                  (memcmp(_b->text + _b->tokens[macro].span.offset,
                          _b->text + _b->tokens[tested].span.offset,
                          _b->tokens[macro].span.length) == 0) )
        {
            floor = 1u;
        }

        ++seen;

        for (uint32_t line = at; line <= last; ++line)
        {
            _b->pp[line] = (uint8_t)((own > 255u) ? 255u : own);
        }

        depth = next;
        at    = last + 1u;
    }

    // the guard's level comes off every count
    for (uint32_t at = 0u; (floor > 0u) && (at < _b->count); ++at)
    {
        _b->pp[at] = (uint8_t)( (_b->pp[at] > floor)
                                ? (_b->pp[at] - floor)
                                : 0u );
    }

    return;
}


//==============================================================================
// 6.  PUBLIC INTERFACE
//==============================================================================


/*
d_syntax_items_build
  Recognizes the items of a grouped token stream.
*/
bool
d_syntax_items_build(
    const struct d_token*         _tokens,
    size_t                        _count,
    const struct d_syntax_groups* _groups,
    const char*                   _text,
    struct d_syntax_items*        _out_items
)
{
    struct d_internal_build build;

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

    build.tokens = _tokens;
    build.count  = (uint32_t)_count;
    build.groups = _groups;
    build.text   = _text;
    build.out    = _out_items;

    _out_items->item_of  = malloc(_count * sizeof(uint32_t));
    _out_items->level_of = malloc(_count * sizeof(uint8_t));
    build.mode           = malloc(_count * sizeof(uint8_t));
    build.owner          = malloc(_count * sizeof(uint32_t));
    build.last_item      = malloc(_count * sizeof(uint32_t));
    build.pp             = malloc(_count * sizeof(uint8_t));

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

        // the file first, then every scope in the order of its opener, so
        // that an item always exists before the scopes inside it are walked
        ok = d_internal_walk(&build, D_SYNTAX_NONE, D_INTERNAL_MODE_FILE);

        for (uint32_t at = 0u; ok && (at < build.count); ++at)
        {
            if ( (build.mode[at] != D_INTERNAL_MODE_NONE) &&
                 d_syntax_is_opener(_tokens[at].kind) &&
                 (_groups->match[at] != D_SYNTAX_NONE) )
            {
                ok = d_internal_walk(&build, at, build.mode[at]);
            }
        }
    }

    // how each function's body ends, now that every body has been walked
    for (size_t k = 0u; ok && (k < _out_items->count); ++k)
    {
        struct d_syntax_item* const item = &_out_items->items[k];

        if ( (item->kind != D_SYNTAX_ITEM_FN_DEF) ||
             (item->body == D_SYNTAX_NONE) )
        {
            continue;
        }

        const uint32_t last = build.last_item[item->body];

        if (last == D_SYNTAX_NONE)
        {
            item->flags |= D_SYNTAX_ITEM_FLAG_EMPTY_BODY;
        }
        else if ( (_out_items->items[last].kind ==
                   D_SYNTAX_ITEM_STATEMENT) &&
                  (_out_items->items[last].statement ==
                   D_SYNTAX_STMT_RETURN) )
        {
            item->flags |= D_SYNTAX_ITEM_FLAG_ENDS_RETURN;
        }
    }

    free(build.scratch);
    free(build.mode);
    free(build.owner);
    free(build.last_item);

    // the conditional depths go out with the items
    if (ok)
    {
        _out_items->pp_of = build.pp;
    }
    else
    {
        free(build.pp);
        d_syntax_items_free(_out_items);
    }

    return ok;
}


/*
d_syntax_items_free
  Releases a recognition's arrays.  The structure itself is the caller's.
*/
void
d_syntax_items_free(
    struct d_syntax_items* _items
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
const char*
d_syntax_item_kind_name(
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
const char*
d_syntax_statement_name(
    unsigned _statement
)
{
    static const char* const names[] =
    {
        "", "simple", "compound", "empty", "if", "for", "while", "do",
        "switch", "return", "break", "continue", "goto", "try", "macro",
        "case", "default", "named", "access"
    };

    return (_statement < (sizeof(names) / sizeof(names[0])))
           ? names[_statement]
           : "";
}
