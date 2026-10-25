/*******************************************************************************
* djinterp [parse]                                        arg_validator_member.c
*
* CLI11's sets and mappings -- IsMember, Transformer and CheckedTransformer --
* declared in arg_validator_extra.h (2).
*   A set keeps its members, or a mapping its pairs, as slices of one text it
* owns, with the filter it compares through. A value is filtered as the set
* filters -- CLI11's built-in filters in one pass, then the filter's
* function -- and keyed: by its text, or, in a set of a type, by the value
* it converts to, widened for an arithmetic type and written back as the
* type writes it for any other, so that `08` finds 8 and -0 finds 0. The
* search is CLI11's: the members' own keys first, made once when the set is
* made, then each member filtered and keyed in turn. A value that does not
* convert to the set's type is told apart from a filter that fails: the
* first is CLI11's answer for such a value, the second stops the validation
* with the filter's status, as CLI11's filter throwing does.
*   Without D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS nothing here is compiled.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_validator_member.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator_extra.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // calloc, malloc, free
#include <string.h>   // memcpy, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_type, type flags
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator, d_arg_validation
#include "./arg_internal.h"  // numbers, scratch, slices, validator parts
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

//==============================================================================
// 1.  RECORDS
//==============================================================================

// d_arg_internal_member_compare
//   enum: what a set compares: the text of its members and of a value, the
// numbers an arithmetic type's values widen to, or the text any other type
// writes its values as.
enum d_arg_internal_member_compare
{
    COMPARE_TEXT   = 0,
    COMPARE_NUMBER = 1,
    COMPARE_FORM   = 2
};

// d_arg_internal_member
//   struct: a member of a set, or a pair of a mapping: `from`, the member;
// `to`, what it maps to; and the key `from` compares by -- `form`, a text,
// which is the member itself in a set of text, or `number`.
struct d_arg_internal_member
{
    struct d_arg_internal_slice  from;
    struct d_arg_internal_slice  to;
    struct d_arg_internal_slice  form;
    struct d_arg_internal_number number;
};

// d_arg_internal_members
//   struct: what IsMember, Transformer and CheckedTransformer keep: the type
// compared as -- NULL for text -- how they compare, the filter, its context
// the set's own copy when `owns` is set, and the members, with the text
// their slices are of.
struct d_arg_internal_members
{
    const struct d_arg_type*      type;
    int                           compare;
    struct d_arg_filter           filter;
    bool                          owns;
    struct d_arg_internal_member* members;
    size_t                        count;
    char*                         text;
    size_t                        text_size;
};

// d_arg_internal_member_key
//   struct: a key as compared: a number, or a text.
struct d_arg_internal_member_key
{
    struct d_arg_internal_number number;
    struct d_pack_text           form;
};

// d_arg_internal_member_scratch
//   struct: the buffers keying a text through a set's filter takes: the
// filtered text, and the text the set's type writes its value as; a search
// keeps one for all the members it keys.
struct d_arg_internal_member_scratch
{
    struct d_arg_buffer filtered;
    struct d_arg_buffer form;
};

// d_arg_internal_member_source
//   struct: the texts a set is made from: an array of members, or an array
// of pairs, whichever is given.
struct d_arg_internal_member_source
{
    const char* const*          values;
    const struct d_arg_mapping* pairs;
    size_t                      count;
};

// d_arg_internal_member_kind
//   struct: the functions one of CLI11's three makes its validator of.
struct d_arg_internal_member_kind
{
    d_arg_validate_fn validate;
    d_arg_describe_fn describe;
    d_arg_choices_fn  choices;
};

// FILTER_FLAGS
//   constant: the built-in filters a filter's flags can hold.
static const uint32_t FILTER_FLAGS = D_ARG_FILTER_IGNORE_CASE       |
                                     D_ARG_FILTER_IGNORE_UNDERSCORE |
                                     D_ARG_FILTER_IGNORE_SPACE;

// FILTER_NONE
//   constant: the filter of a set given none, which filters nothing.
static const struct d_arg_filter FILTER_NONE =
{
    0u,    // flags
    NULL,  // function
    NULL,  // context
    NULL,  // copy
    NULL   // release
};

// MEMBERS_NONE
//   constant: a set that holds nothing yet.
static const struct d_arg_internal_members MEMBERS_NONE =
{
    NULL,          // type
    COMPARE_TEXT,  // compare
    {
        0u,        // flags
        NULL,      // function
        NULL,      // context
        NULL,      // copy
        NULL       // release
    },             // filter
    false,         // owns
    NULL,          // members
    0u,            // count
    NULL,          // text
    0u             // text_size
};

// KEY_NONE
//   constant: a key not yet made.
static const struct d_arg_internal_member_key KEY_NONE =
{
    D_INTERNAL_ARG_NUMBER_INIT,  // number
    { "", 0u }                   // form
};

// SCRATCH_NONE
//   constant: a scratch that holds nothing yet.
static const struct d_arg_internal_member_scratch SCRATCH_NONE =
{
    D_ARG_BUFFER_INIT,  // filtered
    D_ARG_BUFFER_INIT   // form
};

/**
 * @brief Tells whether a set filters.
 *
 * @param[in] _set  the set.
 * @return `true` for a built-in filter among its flags, or a function; `false`
 *         otherwise.
 *
 * Whether a set filters: a built-in filter among its flags, or a function. A
 * filter with neither is none, and a match through none replaces nothing.
 */
D_STATIC bool
d_arg_internal_member_filters(
    const struct d_arg_internal_members* _set
)
{
    return ( ((_set->filter.flags & FILTER_FLAGS) != 0u) ||
             (_set->filter.function != NULL) );
}

/**
 * @brief Gives the text one of a set's slices covers.
 *
 * @param[in] _set    the set.
 * @param[in] _slice  the slice.
 * @return the text, a span of the set's own.
 *
 * The text one of a set's slices covers.
 */
D_STATIC struct d_pack_text
d_arg_internal_member_text(
    const struct d_arg_internal_members* _set,
    struct d_arg_internal_slice          _slice
)
{
    return d_arg_internal_slice_text(_set->text,
                                     _slice);
}

/**
 * @brief Frees a set: the filter's context it owns, the members, their text and
 *        the record, as its validator's release function.
 *
 * @param[in] _context  the set; NULL is ignored.
 *
 * The filter's context, when the set owns its copy, then the members, their
 * text and the record; the validator's release function.
 */
D_STATIC void
d_arg_internal_member_release(
    void* _context
)
{
    struct d_arg_internal_members* const set =
        (struct d_arg_internal_members*)_context;

    // nothing to free without a record
    if (!set)
    {
        return;
    }

    // the set's own copy of the filter's context
    if ( (set->owns) &&
         (set->filter.release != NULL) )
    {
        set->filter.release(set->filter.context);
    }

    free(set->members);
    free(set->text);
    free(set);

    return;
}

/**
 * @brief Copies a set's record, members and text.
 *
 * @param[in] _set  the set.
 * @return the copy, whose filter's context is the set's, borrowed; NULL, with
 *         nothing kept, when an allocation is refused.
 *
 * A copy of a set's record, members and text, whole, which their slices
 * make enough; its filter's context is the set's, borrowed. NULL, with
 * nothing kept, when an allocation is refused.
 */
D_STATIC struct d_arg_internal_members*
d_arg_internal_member_duplicate(
    const struct d_arg_internal_members* _set
)
{
    const size_t                         bytes   =
        ((_set->count > 0u) ? _set->count : 1u) * sizeof(*_set->members);
    struct d_arg_internal_members* const copy    =
        (struct d_arg_internal_members*)malloc(sizeof(*copy));
    struct d_arg_internal_member* const  members =
        (struct d_arg_internal_member*)malloc(bytes);
    char* const                          text    =
        (char*)malloc(_set->text_size);

    // each part, or none
    if ( (!copy)    ||
         (!members) ||
         (!text) )
    {
        free(text);
        free(members);
        free(copy);

        return NULL;
    }

    *copy         = *_set;
    copy->members = members;
    copy->text    = text;
    copy->owns    = false;
    memcpy(members,
           _set->members,
           bytes);
    memcpy(text,
           _set->text,
           _set->text_size);

    return copy;
}

/**
 * @brief Copies a set, as its validator's copy function.
 *
 * @param[in] _context  the set; NULL for none.
 * @return the copy, which owns a copy of a context the set owns; NULL for no
 *         set, or, with nothing kept, when any of it is refused.
 *
 * The validator's copy function: the set duplicated, then a context the set
 * owns copied with the filter's own function, for the copy to own; NULL, with
 * nothing kept, when any of it is refused.
 */
D_STATIC void*
d_arg_internal_member_copy(
    const void* _context
)
{
    // nothing to copy
    if (!_context)
    {
        return NULL;
    }

    const struct d_arg_internal_members* const source =
        (const struct d_arg_internal_members*)_context;
    struct d_arg_internal_members* const       copy   =
        d_arg_internal_member_duplicate(source);

    // a borrowed context is shared
    if ( (!copy) ||
         (!source->owns) )
    {
        return copy;
    }

    copy->filter.context = source->filter.copy(source->filter.context);
    copy->owns           = (copy->filter.context != NULL);

    // a context that could not be copied keeps nothing
    if (!copy->owns)
    {
        d_arg_internal_member_release(copy);

        return NULL;
    }

    return copy;
}

//==============================================================================
// 2.  FILTERS AND KEYS
//==============================================================================

/**
 * @brief Filters text by the built-in filters, as CLI11's ignore_case,
 *        ignore_underscore and ignore_space.
 *
 * @param[in]     _flags  the D_ARG_FILTER_* flags.
 * @param[in]     _text   the text.
 * @param[in,out] _out    the buffer that gains the filtered text.
 * @return D_ARG_OK, or the buffer's status.
 *
 * CLI11's ignore_case, ignore_underscore and ignore_space, whichever the
 * flags hold, in one pass, since they commute: each character kept, lowered,
 * or dropped.
 */
D_STATIC int
d_arg_internal_member_builtin(
    uint32_t             _flags,
    struct d_pack_text   _text,
    struct d_arg_buffer* _out
)
{
    const bool lower       = ((_flags & D_ARG_FILTER_IGNORE_CASE) != 0u);
    const bool underscores = ((_flags & D_ARG_FILTER_IGNORE_UNDERSCORE) != 0u);
    const bool spaces      = ((_flags & D_ARG_FILTER_IGNORE_SPACE) != 0u);
    int        status      = D_ARG_OK;

    // each character in turn
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _text.length);
         ++index)
    {
        const char character = _text.data[index];

        // a character a filter drops
        if ( ( (underscores) &&
               (character == '_') )   ||
             ( (spaces) &&
               ( (character == ' ') ||
                 (character == '\t') ) ) )
        {
            continue;
        }

        status = d_arg_buffer_append_char(
                     _out,
                     (lower) ? d_arg_internal_to_lower(character)
                             : character);
    }

    return status;
}

/**
 * @brief Filters text as a set filters it.
 *
 * @param[in]     _filter  the set's filter.
 * @param[in]     _text    the text.
 * @param[in,out] _out     the empty buffer that receives the filtered text.
 * @return D_ARG_OK, or the function's or the buffer's failure.
 *
 * A text filtered as the set filters, into the empty buffer `_out`: the
 * built-in filters first, then the function, if any, on what they left, as
 * CLI11 nests the filters it is given.
 */
D_STATIC int
d_arg_internal_member_filter(
    const struct d_arg_filter* _filter,
    struct d_pack_text         _text,
    struct d_arg_buffer*       _out
)
{
    struct d_arg_buffer built  = D_ARG_BUFFER_INIT;
    int                 status =
        d_arg_internal_member_builtin(_filter->flags,
                                      _text,
                                      (_filter->function != NULL) ? &built
                                                                  : _out);

    // the function, on what the built-in filters left
    if ( (status == D_ARG_OK) &&
         (_filter->function != NULL) )
    {
        status = _filter->function(d_arg_buffer_text(&built),
                                   _out,
                                   _filter->context);
    }

    d_arg_buffer_release(&built);

    return ( (status == D_ARG_OK) &&
             (_out->failed != 0) )
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : status;
}

/**
 * @brief Makes the key a text compares by.
 *
 * @param[in]     _set      the set.
 * @param[in]     _text     the text.
 * @param[in]     _context  the conversion's context; NULL for the configured
 *                          integer rule.
 * @param[out]    _key      receives the key.
 * @param[in,out] _form     the buffer that receives the text the type writes,
 *                          for a set that compares so.
 * @return D_ARG_OK; D_ARG_ERROR_CONVERSION when the text does not convert; or
 *         the status the conversion or the writing stopped with.
 *
 * The key a text compares by: the text itself in a set of text; else the
 * text converted to the set's type, under `_context`'s rule, then widened for
 * an arithmetic type, or written back as the type writes it, into `_form`,
 * for any other. D_ARG_ERROR_CONVERSION when the text does not convert.
 */
D_STATIC int
d_arg_internal_member_key_of(
    const struct d_arg_internal_members* _set,
    struct d_pack_text                   _text,
    const struct d_arg_convert_context*  _context,
    struct d_arg_internal_member_key*    _key,
    struct d_arg_buffer*                 _form
)
{
    // text is its own key
    if (_set->compare == COMPARE_TEXT)
    {
        _key->form = _text;

        return D_ARG_OK;
    }

    union d_arg_internal_scratch scratch = { 0.0L };
    void* const                  object  =
        d_arg_internal_scratch_acquire(&scratch,
                                       _set->type->size);

    // check if memory allocation was successful
    if (!object)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    int status = d_arg_internal_lexical_cast(_set->type,
                                             _text,
                                             _context,
                                             object);

    // a number widens
    if ( (status == D_ARG_OK) &&
         (_set->compare == COMPARE_NUMBER) )
    {
        (void)d_arg_internal_number_load(_set->type,
                                         object,
                                         &_key->number);
    }

    // any other value is written as its type writes it
    if ( (status == D_ARG_OK) &&
         (_set->compare == COMPARE_FORM) )
    {
        status     = d_arg_internal_number_write(_set->type,
                                                 object,
                                                 _form);
        _key->form = d_arg_buffer_text(_form);
    }

    d_arg_internal_scratch_release(&scratch,
                                   object);

    return status;
}

/**
 * @brief Gives a member's own key.
 *
 * @param[in] _set    the set.
 * @param[in] _index  the member's place.
 * @return the key, as the set made it.
 *
 * A member's own key, as the set made it.
 */
D_STATIC struct d_arg_internal_member_key
d_arg_internal_member_stored(
    const struct d_arg_internal_members* _set,
    size_t                               _index
)
{
    const struct d_arg_internal_member* const member = &_set->members[_index];
    const struct d_arg_internal_member_key    key    =
    {
        member->number,                                // number
        d_arg_internal_member_text(_set,
                                   member->form)       // form
    };

    return key;
}

/**
 * @brief Tells whether two keys are one.
 *
 * @param[in] _set  the set, which says how they compare.
 * @param[in] _a    one key.
 * @param[in] _b    the other.
 * @return `true` when they are one, numbers compared as the type compares them
 *         and texts byte for byte; `false` otherwise.
 *
 * Whether two keys are one: numbers compared as the type compares them, so
 * that -0 is 0 and a NaN is nothing, as with CLI11's `==`; texts byte for
 * byte.
 */
D_STATIC bool
d_arg_internal_member_same(
    const struct d_arg_internal_members*    _set,
    const struct d_arg_internal_member_key* _a,
    const struct d_arg_internal_member_key* _b
)
{
    return (_set->compare == COMPARE_NUMBER)
           ? (d_arg_internal_number_compare(&_a->number,
                                            &_b->number) == 0)
           : d_arg_internal_equal(_a->form,
                                  _b->form);
}

/**
 * @brief Makes a member's key through the set's filter.
 *
 * @param[in]     _set       the set.
 * @param[in]     _index     the member's place.
 * @param[in,out] _scratch   receives the filtered text and the text the type
 *                           writes, which the key's text may be.
 * @param[out]    _key       receives the key.
 * @param[out]    _converts  receives whether the filtered text converts.
 * @return D_ARG_OK, also for a text that does not convert; otherwise the
 *         filter's failure, as it is, or the status the keying stopped with.
 *
 * A member's key through the set's filter: its text filtered, then keyed as
 * the members' texts were when the set was made, under the configured
 * integer rule. A filtered text that does not convert is D_ARG_OK with
 * `*_converts` false; the filter's failure is returned as it is, whatever it
 * is, and never read so.
 */
D_STATIC int
d_arg_internal_member_filtered(
    const struct d_arg_internal_members*  _set,
    size_t                                _index,
    struct d_arg_internal_member_scratch* _scratch,
    struct d_arg_internal_member_key*     _key,
    bool*                                 _converts
)
{
    d_arg_buffer_clear(&_scratch->filtered);
    d_arg_buffer_clear(&_scratch->form);
    *_converts = true;

    const int filtered = d_arg_internal_member_filter(
                             &_set->filter,
                             d_arg_internal_member_text(
                                 _set,
                                 _set->members[_index].from),
                             &_scratch->filtered);

    // the filter's failure stops the search
    if (filtered != D_ARG_OK)
    {
        return filtered;
    }

    const int keyed = d_arg_internal_member_key_of(
                          _set,
                          d_arg_buffer_text(&_scratch->filtered),
                          NULL,
                          _key,
                          &_scratch->form);

    *_converts = (keyed != D_ARG_ERROR_CONVERSION);

    return (*_converts) ? keyed : D_ARG_OK;
}

//==============================================================================
// 3.  SEARCH
//==============================================================================

/**
 * @brief Searches a set's members through its filter, as CLI11's second search.
 *
 * @param[in]     _set    the set.
 * @param[in]     _key    the value's key.
 * @param[in,out] _found  receives the first member that matches; untouched when
 *                        none does.
 * @return D_ARG_OK, or the failure that stopped the search, the count then in
 *         `*_found`.
 *
 * CLI11's second search: each member filtered and keyed in turn, the first
 * whose key is the value's found; a member whose filtered text does not
 * convert matches nothing, and a filter that fails stops the search.
 */
D_STATIC int
d_arg_internal_member_search_filtered(
    const struct d_arg_internal_members*    _set,
    const struct d_arg_internal_member_key* _key,
    size_t*                                 _found
)
{
    struct d_arg_internal_member_scratch scratch = SCRATCH_NONE;
    int                                  status  = D_ARG_OK;

    // each member until one matches
    for (size_t index = 0u; index < _set->count; ++index)
    {
        struct d_arg_internal_member_key member   = KEY_NONE;
        bool                             converts = true;
        const int                        keyed    =
            d_arg_internal_member_filtered(_set,
                                           index,
                                           &scratch,
                                           &member,
                                           &converts);

        // a member that does not convert matches nothing
        if ( (keyed == D_ARG_OK) &&
             (!converts) )
        {
            continue;
        }

        // a failure, or a match, ends the search
        if ( (keyed != D_ARG_OK) ||
             (d_arg_internal_member_same(_set,
                                         &member,
                                         _key)) )
        {
            status  = keyed;
            *_found = (keyed == D_ARG_OK) ? index : _set->count;

            break;
        }
    }

    d_arg_buffer_release(&scratch.form);
    d_arg_buffer_release(&scratch.filtered);

    return status;
}

/**
 * @brief Searches a set's members for a key, as CLI11's search.
 *
 * @param[in]  _set    the set.
 * @param[in]  _key    the value's key.
 * @param[out] _found  receives the first member that matches; the count for
 *                     none.
 * @return D_ARG_OK, or the failure that stopped the search.
 *
 * CLI11's search: the members' own keys first, compared with the value's;
 * then, when the set filters, each member filtered and keyed in turn. The
 * first member that matches, or the count for none, in `*_found`.
 */
D_STATIC int
d_arg_internal_member_search(
    const struct d_arg_internal_members*    _set,
    const struct d_arg_internal_member_key* _key,
    size_t*                                 _found
)
{
    // the members' own keys
    for (size_t index = 0u; index < _set->count; ++index)
    {
        const struct d_arg_internal_member_key member =
            d_arg_internal_member_stored(_set,
                                         index);

        // the first match
        if (d_arg_internal_member_same(_set,
                                       &member,
                                       _key))
        {
            *_found = index;

            return D_ARG_OK;
        }
    }

    *_found = _set->count;

    return (d_arg_internal_member_filters(_set))
           ? d_arg_internal_member_search_filtered(_set,
                                                   _key,
                                                   _found)
           : D_ARG_OK;
}

/**
 * @brief Keys a value as its set's members are keyed: its text filtered when
 *        the set filters, then keyed under the validation's rule.
 *
 * @param[in]     _set         the set.
 * @param[in]     _validation  the validation: the value and its rule.
 * @param[in,out] _scratch     receives the filtered text and the text the
 *                             type writes, which the key's text may be.
 * @param[out]    _key         receives the key.
 * @param[out]    _converts    receives whether the value converts to the
 *                             set's type.
 * @return D_ARG_OK, also for a value that does not convert; otherwise the
 *         filter's failure, as it is, or memory's.
 *
 * A value that does not convert is D_ARG_OK with `*_converts` false; the
 * filter's failure is returned whatever its status, and so never read as a
 * value that does not convert.
 */
D_STATIC int
d_arg_internal_member_value_key(
    const struct d_arg_internal_members*  _set,
    const struct d_arg_validation*        _validation,
    struct d_arg_internal_member_scratch* _scratch,
    struct d_arg_internal_member_key*     _key,
    bool*                                 _converts
)
{
    const struct d_arg_convert_context context =
    {
        _validation->integer_rule,  // integer_rule
        _validation->option,        // option
        NULL                        // message
    };
    const bool                         filters =
        d_arg_internal_member_filters(_set);
    int                                status  =
        (filters) ? d_arg_internal_member_filter(&_set->filter,
                                                 _validation->value,
                                                 &_scratch->filtered)
                  : D_ARG_OK;

    *_converts = true;

    // the value's key
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_member_key_of(
                     _set,
                     (filters) ? d_arg_buffer_text(&_scratch->filtered)
                               : _validation->value,
                     &context,
                     _key,
                     &_scratch->form);

        // a value that does not convert matches nothing
        if (status == D_ARG_ERROR_CONVERSION)
        {
            *_converts = false;
            status     = D_ARG_OK;
        }
    }

    return status;
}

/**
 * @brief Finds the member a value matches.
 *
 * @param[in]  _set         the set.
 * @param[in]  _validation  the validation: the value and its rule.
 * @param[out] _found       receives the member's place; the count for none.
 * @param[out] _converts    receives whether the value converts to the set's
 *                          type.
 * @return D_ARG_OK, also for a value that does not convert; otherwise the
 *         filter's failure, as it is, or memory's.
 *
 * The member a value matches: the value keyed as the members are, then
 * searched for; `*_found` the member's index, or the count for none, and
 * `*_converts` false, with D_ARG_OK, for a value that does not convert to
 * the set's type. A failure -- the filter's, whatever its status, or
 * memory's -- is returned as it is.
 */
D_STATIC int
d_arg_internal_member_find(
    const struct d_arg_internal_members* _set,
    const struct d_arg_validation*       _validation,
    size_t*                              _found,
    bool*                                _converts
)
{
    struct d_arg_internal_member_scratch scratch = SCRATCH_NONE;
    struct d_arg_internal_member_key     key     = KEY_NONE;
    int                                  status  =
        d_arg_internal_member_value_key(_set,
                                        _validation,
                                        &scratch,
                                        &key,
                                        _converts);

    *_found = _set->count;

    // the member it matches
    if ( (status == D_ARG_OK) &&
         (*_converts) )
    {
        status = d_arg_internal_member_search(_set,
                                              &key,
                                              _found);
    }

    d_arg_buffer_release(&scratch.form);
    d_arg_buffer_release(&scratch.filtered);

    return status;
}

//==============================================================================
// 4.  VALIDATORS
//==============================================================================

/**
 * @brief Appends a set's members, as CLI11's generate_set.
 *
 * @param[in]     _set  the set.
 * @param[in,out] _out  the buffer that gains them.
 * @return D_ARG_OK, or the buffer's status.
 *
 * CLI11's generate_set: the members between braces, joined by commas.
 */
D_STATIC int
d_arg_internal_member_write_set(
    const struct d_arg_internal_members* _set,
    struct d_arg_buffer*                 _out
)
{
    (void)d_arg_buffer_append_char(_out,
                                   '{');

    // each member
    for (size_t index = 0u; index < _set->count; ++index)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       (index > 0u) ? "," : "");
        (void)d_arg_buffer_append_text(
                  _out,
                  d_arg_internal_member_text(_set,
                                             _set->members[index].from));
    }

    return d_arg_buffer_append_char(_out,
                                    '}');
}

/**
 * @brief Appends a mapping's pairs, as CLI11's generate_map.
 *
 * @param[in]     _set  the mapping.
 * @param[in,out] _out  the buffer that gains them.
 * @return D_ARG_OK, or the buffer's status.
 *
 * CLI11's generate_map: each pair as `from->to`, between braces, joined by
 * commas.
 */
D_STATIC int
d_arg_internal_member_write_map(
    const struct d_arg_internal_members* _set,
    struct d_arg_buffer*                 _out
)
{
    (void)d_arg_buffer_append_char(_out,
                                   '{');

    // each pair
    for (size_t index = 0u; index < _set->count; ++index)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       (index > 0u) ? "," : "");
        (void)d_arg_buffer_append_text(
                  _out,
                  d_arg_internal_member_text(_set,
                                             _set->members[index].from));
        (void)d_arg_buffer_append_cstr(_out,
                                       "->");
        (void)d_arg_buffer_append_text(
                  _out,
                  d_arg_internal_member_text(_set,
                                             _set->members[index].to));
    }

    return d_arg_buffer_append_char(_out,
                                    '}');
}

/**
 * @brief Appends a CheckedTransformer's description, as CLI11 writes it.
 *
 * @param[in]     _set  the mapping.
 * @param[in,out] _out  the buffer that gains it.
 * @return D_ARG_OK, or the buffer's status.
 *
 * CLI11's CheckedTransformer description: "value in " and the map, then
 * " OR " and the values the pairs map to, between braces, joined by commas.
 */
D_STATIC int
d_arg_internal_member_write_checked(
    const struct d_arg_internal_members* _set,
    struct d_arg_buffer*                 _out
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "value in ");
    (void)d_arg_internal_member_write_map(_set,
                                          _out);
    (void)d_arg_buffer_append_cstr(_out,
                                   " OR {");

    // each pair's value
    for (size_t index = 0u; index < _set->count; ++index)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       (index > 0u) ? "," : "");
        (void)d_arg_buffer_append_text(
                  _out,
                  d_arg_internal_member_text(_set,
                                             _set->members[index].to));
    }

    return d_arg_buffer_append_char(_out,
                                    '}');
}

/**
 * @brief Judges a value by a set, as CLI11's IsMember.
 *
 * @param[in]     _validator   the validator, whose data is the set.
 * @param[in,out] _validation  the validation, whose value is replaced by a
 *                             member found through a filter, or rejected when
 *                             no member matches.
 * @return D_ARG_OK once a verdict is reached; D_ARG_ERROR_VALIDATION, the value
 *         its message, for a value that does not convert; otherwise the failure
 *         that stopped the search, or the buffer's.
 *
 * CLI11's IsMember: a member found through a filter puts itself, as written,
 * in the value's place, and one found without a filter leaves the value
 * alone; a value that does not convert to the set's type stops validation
 * with the value itself as the message, as CLI11 throws it; a failure stops
 * it with its status; any other value is rejected with "<value> not in
 * {<members>}".
 */
D_STATIC int
d_arg_internal_member_set_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_internal_members* const set      =
        (const struct d_arg_internal_members*)_validator->data;
    size_t                                     found    = 0u;
    bool                                       converts = true;
    const int                                  status   =
        d_arg_internal_member_find(set,
                                   _validation,
                                   &found,
                                   &converts);

    // a value that does not convert, thrown as it is
    if ( (status == D_ARG_OK) &&
         (!converts) )
    {
        return d_arg_internal_validator_stop(_validation,
                                             "",
                                             _validation->value,
                                             "");
    }

    // stopped, or a member
    if ( (status != D_ARG_OK) ||
         (found < set->count) )
    {
        return ( (status == D_ARG_OK) &&
                 (d_arg_internal_member_filters(set)) )
               ? d_arg_validation_replace(
                     _validation,
                     d_arg_internal_member_text(set,
                                                set->members[found].from))
               : status;
    }

    (void)d_arg_internal_validator_reject(_validation,
                                          "",
                                          _validation->value,
                                          " not in ");

    return d_arg_internal_member_write_set(set,
                                           &_validation->message);
}

/**
 * @brief Describes an IsMember, as CLI11 does.
 *
 * @param[in]     _validator  the validator, whose data is the set.
 * @param[in,out] _out        the buffer that gains the members.
 * @return D_ARG_OK, or the buffer's status.
 *
 * CLI11's IsMember description: the members, as generate_set writes them.
 */
D_STATIC int
d_arg_internal_member_set_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    return d_arg_internal_member_write_set(
               (const struct d_arg_internal_members*)_validator->data,
               _out);
}

/**
 * @brief Transforms a value by a mapping, as CLI11's Transformer.
 *
 * @param[in]     _validator   the validator, whose data is the mapping.
 * @param[in,out] _validation  the validation, whose value is replaced by the
 *                             value of the pair whose key it matches.
 * @return D_ARG_OK once a verdict is reached; otherwise the failure that
 *         stopped the search, or the replacement's.
 *
 * CLI11's Transformer: a value matching a pair's key becomes the pair's
 * value; anything else -- a value that does not convert to the type among it
 * -- stands; a failure stops the validation with its status.
 */
D_STATIC int
d_arg_internal_member_transform_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_internal_members* const set      =
        (const struct d_arg_internal_members*)_validator->data;
    size_t                                     found    = 0u;
    bool                                       converts = true;
    const int                                  status   =
        d_arg_internal_member_find(set,
                                   _validation,
                                   &found,
                                   &converts);

    // a pair's key: its value replaces the value
    if ( (status == D_ARG_OK) &&
         (found < set->count) )
    {
        return d_arg_validation_replace(
                   _validation,
                   d_arg_internal_member_text(set,
                                              set->members[found].to));
    }

    return status;
}

/**
 * @brief Describes a Transformer, as CLI11 does.
 *
 * @param[in]     _validator  the validator, whose data is the mapping.
 * @param[in,out] _out        the buffer that gains the map.
 * @return D_ARG_OK, or the buffer's status.
 *
 * CLI11's Transformer description: the map, as generate_map writes it.
 */
D_STATIC int
d_arg_internal_member_transform_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    return d_arg_internal_member_write_map(
               (const struct d_arg_internal_members*)_validator->data,
               _out);
}

/**
 * @brief Tells whether a value is, as text, what some pair maps to.
 *
 * @param[in] _set    the mapping.
 * @param[in] _value  the value.
 * @return `true` when it is; `false` otherwise.
 *
 * Whether a value is, as text, what some pair maps to: CLI11's last chance
 * for a checked transformer's value.
 */
D_STATIC bool
d_arg_internal_member_is_target(
    const struct d_arg_internal_members* _set,
    struct d_pack_text                   _value
)
{
    // each pair's value
    for (size_t index = 0u; index < _set->count; ++index)
    {
        // the value itself
        if (d_arg_internal_equal(
                d_arg_internal_member_text(_set,
                                           _set->members[index].to),
                _value))
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Transforms a value by a mapping, as CLI11's CheckedTransformer.
 *
 * @param[in]     _validator   the validator, whose data is the mapping.
 * @param[in,out] _validation  the validation, whose value is replaced by the
 *                             value of the pair whose key it matches, or
 *                             rejected when it is neither a key nor a value.
 * @return D_ARG_OK once a verdict is reached; otherwise the failure that
 *         stopped the search, or the buffer's.
 *
 * CLI11's CheckedTransformer: a value matching a pair's key becomes the
 * pair's value; one equal, as text, to a pair's value stands; any other --
 * one that does not convert to the type among it -- is rejected with "Check
 * <value> value in {...} OR {...} FAILED"; a failure stops the validation
 * with its status.
 */
D_STATIC int
d_arg_internal_member_checked_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_internal_members* const set      =
        (const struct d_arg_internal_members*)_validator->data;
    size_t                                     found    = 0u;
    bool                                       converts = true;
    const int                                  status   =
        d_arg_internal_member_find(set,
                                   _validation,
                                   &found,
                                   &converts);

    // stopped
    if (status != D_ARG_OK)
    {
        return status;
    }

    // a pair's key: its value replaces the value
    if (found < set->count)
    {
        return d_arg_validation_replace(
                   _validation,
                   d_arg_internal_member_text(set,
                                              set->members[found].to));
    }

    // a pair's value stands as it is
    if (d_arg_internal_member_is_target(set,
                                        _validation->value))
    {
        return D_ARG_OK;
    }

    (void)d_arg_internal_validator_reject(_validation,
                                          "Check ",
                                          _validation->value,
                                          " ");
    (void)d_arg_internal_member_write_checked(set,
                                              &_validation->message);

    return d_arg_buffer_append_cstr(&_validation->message,
                                    " FAILED");
}

/**
 * @brief Describes a CheckedTransformer, as CLI11 does.
 *
 * @param[in]     _validator  the validator, whose data is the mapping.
 * @param[in,out] _out        the buffer that gains the description.
 * @return D_ARG_OK, or the buffer's status.
 *
 * CLI11's CheckedTransformer description.
 */
D_STATIC int
d_arg_internal_member_checked_describe(
    const struct d_arg_validator* _validator,
    struct d_arg_buffer*          _out
)
{
    return d_arg_internal_member_write_checked(
               (const struct d_arg_internal_members*)_validator->data,
               _out);
}

/**
 * @brief Lists the values a set accepts, or the keys of a mapping's pairs.
 *
 * @param[in] _validator  the validator, whose data is the set.
 * @param[in] _each       receives each value.
 * @param[in] _context    `_each`'s context.
 * @return D_ARG_OK, or the first status other than D_ARG_OK that `_each`
 *         returned.
 *
 * The values a set accepts, or the keys a mapping's pairs are found by, in
 * the order given.
 */
D_STATIC int
d_arg_internal_member_each_from(
    const struct d_arg_validator* _validator,
    d_arg_choice_fn               _each,
    void*                         _context
)
{
    const struct d_arg_internal_members* const set    =
        (const struct d_arg_internal_members*)_validator->data;
    int                                        status = D_ARG_OK;

    // each member, until a listing stops
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < set->count);
         ++index)
    {
        status = _each(d_arg_internal_member_text(set,
                                                  set->members[index].from),
                       _context);
    }

    return status;
}

/**
 * @brief Tells whether a text is already among a checked transformer's choices.
 *
 * @param[in] _set    the mapping.
 * @param[in] _index  the pair whose value is listed next.
 * @param[in] _text   the text.
 * @return `true` for a pair's key, or the value of a pair before `_index`;
 *         `false` otherwise.
 *
 * Whether a text is a pair's key, or the value of a pair before `_index`:
 * listed already, among a checked transformer's choices.
 */
D_STATIC bool
d_arg_internal_member_listed(
    const struct d_arg_internal_members* _set,
    size_t                               _index,
    struct d_pack_text                   _text
)
{
    // each pair
    for (size_t other = 0u; other < _set->count; ++other)
    {
        const struct d_arg_internal_member* const member =
            &_set->members[other];

        // a key, or an earlier value
        if ( (d_arg_internal_equal(
                  d_arg_internal_member_text(_set,
                                             member->from),
                  _text))                                        ||
             ( (other < _index) &&
               (d_arg_internal_equal(
                    d_arg_internal_member_text(_set,
                                               member->to),
                    _text)) ) )
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Lists the values a checked transformer accepts: the keys, then each
 *        value not listed already.
 *
 * @param[in] _validator  the validator, whose data is the mapping.
 * @param[in] _each       receives each value.
 * @param[in] _context    `_each`'s context.
 * @return D_ARG_OK, or the first status other than D_ARG_OK that `_each`
 *         returned.
 *
 * The values a checked transformer accepts: the keys, then each value a pair
 * maps to that is not listed already.
 */
D_STATIC int
d_arg_internal_member_each_value(
    const struct d_arg_validator* _validator,
    d_arg_choice_fn               _each,
    void*                         _context
)
{
    const struct d_arg_internal_members* const set    =
        (const struct d_arg_internal_members*)_validator->data;
    int                                        status =
        d_arg_internal_member_each_from(_validator,
                                        _each,
                                        _context);

    // each pair's value, until a listing stops
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < set->count);
         ++index)
    {
        const struct d_pack_text to =
            d_arg_internal_member_text(set,
                                       set->members[index].to);

        // a value not yet listed
        if (!d_arg_internal_member_listed(set,
                                          index,
                                          to))
        {
            status = _each(to,
                           _context);
        }
    }

    return status;
}

// MEMBER_SET
//   constant: IsMember's functions.
static const struct d_arg_internal_member_kind MEMBER_SET =
{
    d_arg_internal_member_set_validate,  // validate
    d_arg_internal_member_set_describe,  // describe
    d_arg_internal_member_each_from      // choices
};

// MEMBER_TRANSFORM
//   constant: Transformer's functions.
static const struct d_arg_internal_member_kind MEMBER_TRANSFORM =
{
    d_arg_internal_member_transform_validate,  // validate
    d_arg_internal_member_transform_describe,  // describe
    d_arg_internal_member_each_from            // choices
};

// MEMBER_CHECKED
//   constant: CheckedTransformer's functions.
static const struct d_arg_internal_member_kind MEMBER_CHECKED =
{
    d_arg_internal_member_checked_validate,  // validate
    d_arg_internal_member_checked_describe,  // describe
    d_arg_internal_member_each_value         // choices
};

//==============================================================================
// 5.  CONSTRUCTION
//==============================================================================

/**
 * @brief Tells how a set of a type compares.
 *
 * @param[in] _type  the type; NULL for none.
 * @return COMPARE_TEXT, COMPARE_NUMBER or COMPARE_FORM; -1 for a type it cannot
 *         compare.
 *
 * How a set of a type compares: as text for none, or for a text type; as
 * numbers for an arithmetic type; as the text it writes for any other type
 * whose values convert, plain, and are written back; -1 for a type it cannot
 * compare.
 */
D_STATIC int
d_arg_internal_member_compare_of(
    const struct d_arg_type* _type
)
{
    // text, given no type or a text type
    if ( (!_type) ||
         ((_type->flags & D_ARG_TYPE_FLAG_STRING) != 0u) )
    {
        return COMPARE_TEXT;
    }

    // a type whose values cannot be made and dropped
    if (!d_arg_internal_type_convertible(_type))
    {
        return -1;
    }

    // a number, or a value its type writes
    if (d_arg_internal_number_of(_type) != (int)D_ARG_INTERNAL_NUMBER_NONE)
    {
        return COMPARE_NUMBER;
    }

    return (_type->format != NULL)
           ? COMPARE_FORM
           : -1;
}

/**
 * @brief Gives a source's member, or its pair's key.
 *
 * @param[in] _source  the source.
 * @param[in] _index   the member's place.
 * @return the text, the caller's.
 *
 * A source's member, or its pair's key.
 */
D_STATIC const char*
d_arg_internal_member_source_from(
    const struct d_arg_internal_member_source* _source,
    size_t                                     _index
)
{
    return (_source->pairs != NULL)
           ? _source->pairs[_index].from
           : _source->values[_index];
}

/**
 * @brief Gives a source's pair's value.
 *
 * @param[in] _source  the source.
 * @param[in] _index   the member's place.
 * @return the text, the caller's; empty for a member, which maps to nothing.
 *
 * A source's pair's value; a member maps to nothing.
 */
D_STATIC const char*
d_arg_internal_member_source_to(
    const struct d_arg_internal_member_source* _source,
    size_t                                     _index
)
{
    return (_source->pairs != NULL)
           ? _source->pairs[_index].to
           : "";
}

/**
 * @brief Tells whether a source has every text it promises, and a count memory
 *        can hold.
 *
 * @param[in] _source  the source.
 * @return `true` when it does; `false` otherwise.
 *
 * Whether a source has every text it promises, and a count memory can hold.
 */
D_STATIC bool
d_arg_internal_member_given(
    const struct d_arg_internal_member_source* _source
)
{
    // an array for the count, and a count that can be allocated
    if ( ( (_source->values == NULL) &&
           (_source->pairs == NULL)  &&
           (_source->count > 0u) )                         ||
         (_source->count >
          ((size_t)-1 / sizeof(struct d_arg_internal_member))) )
    {
        return false;
    }

    // each member's texts
    for (size_t index = 0u; index < _source->count; ++index)
    {
        // a missing text
        if ( (!d_arg_internal_member_source_from(_source,
                                                 index)) ||
             (!d_arg_internal_member_source_to(_source,
                                               index)) )
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Gathers one member's texts and key into a buffer.
 *
 * @param[in,out] _set     the set, whose member is written.
 * @param[in]     _source  the source.
 * @param[in]     _index   the member's place.
 * @param[in,out] _text    the buffer that gains `from`, `to`, and the key's
 *                         text when it has its own.
 * @return D_ARG_OK; D_ARG_ERROR_CONVERSION for a member that does not convert;
 *         or D_ARG_ERROR_OUT_OF_MEMORY.
 *
 * One member's texts gathered into the buffer -- `from`, then `to` -- and its
 * key: `from` itself in a set of text; else `from` converted under the
 * configured integer rule, widened, or written back as its type writes it,
 * after the other two.
 */
D_STATIC int
d_arg_internal_member_one(
    struct d_arg_internal_members*             _set,
    const struct d_arg_internal_member_source* _source,
    size_t                                     _index,
    struct d_arg_buffer*                       _text
)
{
    struct d_arg_internal_member* const member = &_set->members[_index];
    const char* const                   from   =
        d_arg_internal_member_source_from(_source,
                                          _index);
    const char* const                   to     =
        d_arg_internal_member_source_to(_source,
                                        _index);
    struct d_arg_internal_member_key    key    = KEY_NONE;
    struct d_arg_buffer                 form   = D_ARG_BUFFER_INIT;

    member->from.offset = _text->length;
    member->from.length = strlen(from);
    (void)d_arg_buffer_append(_text,
                              from,
                              member->from.length);
    member->to.offset   = _text->length;
    member->to.length   = strlen(to);
    (void)d_arg_buffer_append(_text,
                              to,
                              member->to.length);
    member->form        = member->from;

    int status = d_arg_internal_member_key_of(_set,
                                              d_arg_internal_text_of(from),
                                              NULL,
                                              &key,
                                              &form);

    // a key of its own, after the texts
    if ( (status == D_ARG_OK) &&
         (_set->compare != COMPARE_TEXT) )
    {
        member->number      = key.number;
        member->form.offset = _text->length;
        member->form.length = form.length;
        status              = d_arg_buffer_append_text(
                                  _text,
                                  d_arg_buffer_text(&form));
    }

    d_arg_buffer_release(&form);

    return ( (status == D_ARG_OK) &&
             (_text->failed != 0) )
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : status;
}

/**
 * @brief Gathers every member's texts and keys into the text a set owns.
 *
 * @param[in,out] _set     the set, whose members and text are written.
 * @param[in]     _source  the source.
 * @return D_ARG_OK; D_ARG_ERROR_BAD_ARGUMENT for a member that does not
 *         convert; or D_ARG_ERROR_OUT_OF_MEMORY.
 *
 * Every member's texts and key gathered into one buffer, a NUL after them so
 * that the text is never empty and always has storage, which the set then
 * owns. D_ARG_ERROR_BAD_ARGUMENT for a member that does not convert.
 */
D_STATIC int
d_arg_internal_member_body(
    struct d_arg_internal_members*             _set,
    const struct d_arg_internal_member_source* _source
)
{
    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    int                 status = D_ARG_OK;

    // each member, until one fails
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _set->count);
         ++index)
    {
        status = d_arg_internal_member_one(_set,
                                           _source,
                                           index,
                                           &text);
    }

    // the closing NUL
    if (status == D_ARG_OK)
    {
        status = d_arg_buffer_append_char(&text,
                                          '\0');
    }

    _set->text_size = text.length;
    _set->text      = (status == D_ARG_OK)
                      ? d_arg_buffer_detach(&text)
                      : NULL;
    d_arg_buffer_release(&text);

    return (status == D_ARG_ERROR_CONVERSION)
           ? D_ARG_ERROR_BAD_ARGUMENT
           : status;
}

/**
 * @brief Gives a set the filter it keeps.
 *
 * @param[in,out] _set     the set.
 * @param[in]     _filter  the filter; NULL for none.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY when its context could not be
 *         copied.
 *
 * The filter the set keeps: none when none is given; else a copy of it, its
 * context duplicated -- and owned -- when the filter can copy it, borrowed
 * when it cannot.
 */
D_STATIC int
d_arg_internal_member_take_filter(
    struct d_arg_internal_members* _set,
    const struct d_arg_filter*     _filter
)
{
    // no filter
    if (!_filter)
    {
        _set->filter = FILTER_NONE;

        return D_ARG_OK;
    }

    _set->filter = *_filter;

    // a context the filter cannot copy, or none, is borrowed
    if ( (!_filter->copy) ||
         (!_filter->context) )
    {
        return D_ARG_OK;
    }

    _set->filter.context = _filter->copy(_filter->context);
    _set->owns           = (_set->filter.context != NULL);

    return (_set->owns)
           ? D_ARG_OK
           : D_ARG_ERROR_OUT_OF_MEMORY;
}

/**
 * @brief Makes a set's record.
 *
 * @param[in]  _type    the type the set compares by; NULL for text.
 * @param[in]  _source  the members, or the pairs.
 * @param[in]  _filter  the filter; NULL for none.
 * @param[out] _out     receives the record; untouched on failure.
 * @return D_ARG_OK; D_ARG_ERROR_BAD_ARGUMENT for a type it cannot compare by, a
 *         text missing, or a member that does not convert; or
 *         D_ARG_ERROR_OUT_OF_MEMORY; nothing kept on failure.
 *
 * The record: how it compares, its members' texts and keys, and its filter;
 * nothing is kept on failure.
 */
D_STATIC int
d_arg_internal_member_make(
    const struct d_arg_type*                   _type,
    const struct d_arg_internal_member_source* _source,
    const struct d_arg_filter*                 _filter,
    struct d_arg_internal_members**            _out
)
{
    const int compare = d_arg_internal_member_compare_of(_type);

    // a type the set can compare as, and the texts promised
    if ( (compare < 0) ||
         (!d_arg_internal_member_given(_source)) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct d_arg_internal_members* const set =
        (struct d_arg_internal_members*)malloc(sizeof(*set));

    // check if memory allocation was successful
    if (!set)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    *set         = MEMBERS_NONE;
    set->type    = _type;
    set->compare = compare;
    set->count   = _source->count;
    set->members = (struct d_arg_internal_member*)calloc(
                       (set->count > 0u) ? set->count : 1u,
                       sizeof(*set->members));

    int status = (set->members != NULL)
                 ? d_arg_internal_member_body(set,
                                              _source)
                 : D_ARG_ERROR_OUT_OF_MEMORY;

    // the filter, once the members are in
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_member_take_filter(set,
                                                   _filter);
    }

    // a failure keeps nothing
    if (status != D_ARG_OK)
    {
        d_arg_internal_member_release(set);

        return status;
    }

    *_out = set;

    return D_ARG_OK;
}

/**
 * @brief Makes one of CLI11's IsMember, Transformer and CheckedTransformer over
 *        a source.
 *
 * @param[in] _type    the type the set compares by; NULL for text.
 * @param[in] _source  the members, or the pairs.
 * @param[in] _filter  the filter; NULL for none.
 * @param[in] _kind    the kind's functions.
 * @return the validator, which owns, copies and frees the record; a failed
 *         validator holding the status when the record could not be made.
 *
 * One of CLI11's three over a source: the record made, then given to a
 * validator of the kind's functions, which owns, copies and frees it.
 */
D_STATIC struct d_arg_validator
d_arg_internal_member_build(
    const struct d_arg_type*                   _type,
    const struct d_arg_internal_member_source* _source,
    const struct d_arg_filter*                 _filter,
    const struct d_arg_internal_member_kind*   _kind
)
{
    struct d_arg_internal_members* set    = NULL;
    const int                      status =
        d_arg_internal_member_make(_type,
                                   _source,
                                   _filter,
                                   &set);

    // a record that could not be made fails the validator
    if (status != D_ARG_OK)
    {
        return d_arg_internal_validator_failed(status,
                                               NULL);
    }

    struct d_arg_validator validator =
        d_arg_validator_custom(_kind->validate,
                               set,
                               NULL);

    validator.describe = _kind->describe;
    validator.choices  = _kind->choices;
    d_arg_validator_own_context(&validator,
                                d_arg_internal_member_copy,
                                d_arg_internal_member_release);

    return validator;
}

/*
d_arg_validator_is_member
  IsMember over the array of members.
*/
struct d_arg_validator
d_arg_validator_is_member(
    const struct d_arg_type*   _as,
    const char* const*         _values,
    size_t                     _count,
    const struct d_arg_filter* _filter
)
{
    const struct d_arg_internal_member_source source =
    {
        _values,  // values
        NULL,     // pairs
        _count    // count
    };

    return d_arg_internal_member_build(_as,
                                       &source,
                                       _filter,
                                       &MEMBER_SET);
}

/*
d_arg_validator_transformer
  Transformer over the array of pairs.
*/
struct d_arg_validator
d_arg_validator_transformer(
    const struct d_arg_type*    _as,
    const struct d_arg_mapping* _pairs,
    size_t                      _count,
    const struct d_arg_filter*  _filter
)
{
    const struct d_arg_internal_member_source source =
    {
        NULL,    // values
        _pairs,  // pairs
        _count   // count
    };

    return d_arg_internal_member_build(_as,
                                       &source,
                                       _filter,
                                       &MEMBER_TRANSFORM);
}

/*
d_arg_validator_checked_transformer
  CheckedTransformer over the array of pairs.
*/
struct d_arg_validator
d_arg_validator_checked_transformer(
    const struct d_arg_type*    _as,
    const struct d_arg_mapping* _pairs,
    size_t                      _count,
    const struct d_arg_filter*  _filter
)
{
    const struct d_arg_internal_member_source source =
    {
        NULL,    // values
        _pairs,  // pairs
        _count   // count
    };

    return d_arg_internal_member_build(_as,
                                       &source,
                                       _filter,
                                       &MEMBER_CHECKED);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
