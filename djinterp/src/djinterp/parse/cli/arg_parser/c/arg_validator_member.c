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
* made, then each member filtered and keyed in turn.
*   Without D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS nothing here is compiled.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_validator_member.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator_extra.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // calloc, malloc, free
#include <string.h>  // memcpy, strlen
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

/*
d_arg_internal_member_filters
  Whether a set filters: a built-in filter among its flags, or a function. A
filter with neither is none, and a match through none replaces nothing.
*/
D_STATIC bool
d_arg_internal_member_filters(
    const struct d_arg_internal_members* _set
)
{
    return ( ((_set->filter.flags & FILTER_FLAGS) != 0u) ||
             (_set->filter.function != NULL) );
}

/*
d_arg_internal_member_text
  The text one of a set's slices covers.
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

/*
d_arg_internal_member_release
  The filter's context, when the set owns its copy, then the members, their
text and the record; the validator's release function.
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

/*
d_arg_internal_member_duplicate
  A copy of a set's record, members and text, whole, which their slices
make enough; its filter's context is the set's, borrowed. NULL, with
nothing kept, when an allocation is refused.
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

/*
d_arg_internal_member_copy
  The validator's copy function: the set duplicated, then a context the set
owns copied with the filter's own function, for the copy to own; NULL, with
nothing kept, when any of it is refused.
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

/*
d_arg_internal_member_builtin
  CLI11's ignore_case, ignore_underscore and ignore_space, whichever the
flags hold, in one pass, since they commute: each character kept, lowered,
or dropped.
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

/*
d_arg_internal_member_filter
  A text filtered as the set filters, into the empty buffer `_out`: the
built-in filters first, then the function, if any, on what they left, as
CLI11 nests the filters it is given.
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

/*
d_arg_internal_member_key_of
  The key a text compares by: the text itself in a set of text; else the
text converted to the set's type, under `_context`'s rule, then widened for
an arithmetic type, or written back as the type writes it, into `_form`,
for any other. D_ARG_ERROR_CONVERSION when the text does not convert.
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

/*
d_arg_internal_member_stored
  A member's own key, as the set made it.
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

/*
d_arg_internal_member_same
  Whether two keys are one: numbers compared as the type compares them, so
that -0 is 0 and a NaN is nothing, as with CLI11's `==`; texts byte for
byte.
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

/*
d_arg_internal_member_filtered
  A member's key through the set's filter: its text filtered, then keyed as
the members' texts were when the set was made, under the configured
integer rule.
*/
D_STATIC int
d_arg_internal_member_filtered(
    const struct d_arg_internal_members* _set,
    size_t                               _index,
    struct d_arg_buffer*                 _filtered,
    struct d_arg_internal_member_key*    _key,
    struct d_arg_buffer*                 _form
)
{
    d_arg_buffer_clear(_filtered);
    d_arg_buffer_clear(_form);

    const int status = d_arg_internal_member_filter(
                           &_set->filter,
                           d_arg_internal_member_text(
                               _set,
                               _set->members[_index].from),
                           _filtered);

    return (status == D_ARG_OK)
           ? d_arg_internal_member_key_of(_set,
                                          d_arg_buffer_text(_filtered),
                                          NULL,
                                          _key,
                                          _form)
           : status;
}

//==============================================================================
// 3.  SEARCH
//==============================================================================

/*
d_arg_internal_member_search_filtered
  CLI11's second search: each member filtered and keyed in turn, the first
whose key is the value's found; a member whose filtered text does not
convert matches nothing.
*/
D_STATIC int
d_arg_internal_member_search_filtered(
    const struct d_arg_internal_members*    _set,
    const struct d_arg_internal_member_key* _key,
    size_t*                                 _found
)
{
    struct d_arg_buffer filtered = D_ARG_BUFFER_INIT;
    struct d_arg_buffer form     = D_ARG_BUFFER_INIT;
    int                 status   = D_ARG_OK;

    // each member until one matches
    for (size_t index = 0u; index < _set->count; ++index)
    {
        struct d_arg_internal_member_key member = KEY_NONE;
        const int                        keyed  =
            d_arg_internal_member_filtered(_set,
                                           index,
                                           &filtered,
                                           &member,
                                           &form);

        // a member that does not convert matches nothing
        if (keyed == D_ARG_ERROR_CONVERSION)
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

    d_arg_buffer_release(&form);
    d_arg_buffer_release(&filtered);

    return status;
}

/*
d_arg_internal_member_search
  CLI11's search: the members' own keys first, compared with the value's;
then, when the set filters, each member filtered and keyed in turn. The
first member that matches, or the count for none, in `*_found`.
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

/*
d_arg_internal_member_find
  The member a value matches: its text filtered when the set filters, keyed
under the validation's rule, then searched for; `*_found` the member's
index, or the count for none. D_ARG_ERROR_CONVERSION when the value does not
convert to the set's type.
*/
D_STATIC int
d_arg_internal_member_find(
    const struct d_arg_internal_members* _set,
    const struct d_arg_validation*       _validation,
    size_t*                              _found
)
{
    const struct d_arg_convert_context context  =
    {
        _validation->integer_rule,  // integer_rule
        _validation->option,        // option
        NULL                        // message
    };
    const bool                         filters  =
        d_arg_internal_member_filters(_set);
    struct d_arg_buffer                filtered = D_ARG_BUFFER_INIT;
    struct d_arg_buffer                form     = D_ARG_BUFFER_INIT;
    struct d_arg_internal_member_key   key      = KEY_NONE;
    int                                status   =
        (filters) ? d_arg_internal_member_filter(&_set->filter,
                                                 _validation->value,
                                                 &filtered)
                  : D_ARG_OK;

    *_found = _set->count;

    // the value's key
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_member_key_of(
                     _set,
                     (filters) ? d_arg_buffer_text(&filtered)
                               : _validation->value,
                     &context,
                     &key,
                     &form);
    }

    // the member it matches
    if (status == D_ARG_OK)
    {
        status = d_arg_internal_member_search(_set,
                                              &key,
                                              _found);
    }

    d_arg_buffer_release(&form);
    d_arg_buffer_release(&filtered);

    return status;
}

//==============================================================================
// 4.  VALIDATORS
//==============================================================================

/*
d_arg_internal_member_write_set
  CLI11's generate_set: the members between braces, joined by commas.
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

/*
d_arg_internal_member_write_map
  CLI11's generate_map: each pair as `from->to`, between braces, joined by
commas.
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

/*
d_arg_internal_member_write_checked
  CLI11's CheckedTransformer description: "value in " and the map, then
" OR " and the values the pairs map to, between braces, joined by commas.
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

/*
d_arg_internal_member_set_validate
  CLI11's IsMember: a member found through a filter puts itself, as written,
in the value's place, and one found without a filter leaves the value
alone; a value that does not convert to the set's type stops validation
with the value itself as the message, as CLI11 throws it; any other value
is rejected with "<value> not in {<members>}".
*/
D_STATIC int
d_arg_internal_member_set_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_internal_members* const set    =
        (const struct d_arg_internal_members*)_validator->data;
    size_t                                     found  = 0u;
    const int                                  status =
        d_arg_internal_member_find(set,
                                   _validation,
                                   &found);

    // a value that does not convert, thrown as it is
    if (status == D_ARG_ERROR_CONVERSION)
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

/*
d_arg_internal_member_set_describe
  CLI11's IsMember description: the members, as generate_set writes them.
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

/*
d_arg_internal_member_transform_validate
  CLI11's Transformer: a value matching a pair's key becomes the pair's
value; anything else -- a value that does not convert to the type among it
-- stands.
*/
D_STATIC int
d_arg_internal_member_transform_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_internal_members* const set    =
        (const struct d_arg_internal_members*)_validator->data;
    size_t                                     found  = 0u;
    const int                                  status =
        d_arg_internal_member_find(set,
                                   _validation,
                                   &found);

    // a pair's key: its value replaces the value
    if ( (status == D_ARG_OK) &&
         (found < set->count) )
    {
        return d_arg_validation_replace(
                   _validation,
                   d_arg_internal_member_text(set,
                                              set->members[found].to));
    }

    return (status == D_ARG_ERROR_CONVERSION)
           ? D_ARG_OK
           : status;
}

/*
d_arg_internal_member_transform_describe
  CLI11's Transformer description: the map, as generate_map writes it.
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

/*
d_arg_internal_member_is_target
  Whether a value is, as text, what some pair maps to: CLI11's last chance
for a checked transformer's value.
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

/*
d_arg_internal_member_checked_validate
  CLI11's CheckedTransformer: a value matching a pair's key becomes the
pair's value; one equal, as text, to a pair's value stands; any other --
one that does not convert to the type among it -- is rejected with "Check
<value> value in {...} OR {...} FAILED".
*/
D_STATIC int
d_arg_internal_member_checked_validate(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const struct d_arg_internal_members* const set    =
        (const struct d_arg_internal_members*)_validator->data;
    size_t                                     found  = 0u;
    const int                                  status =
        d_arg_internal_member_find(set,
                                   _validation,
                                   &found);

    // stopped by more than a value that does not convert
    if ( (status != D_ARG_OK) &&
         (status != D_ARG_ERROR_CONVERSION) )
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

/*
d_arg_internal_member_checked_describe
  CLI11's CheckedTransformer description.
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

/*
d_arg_internal_member_each_from
  The values a set accepts, or the keys a mapping's pairs are found by, in
the order given.
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

/*
d_arg_internal_member_listed
  Whether a text is a pair's key, or the value of a pair before `_index`:
listed already, among a checked transformer's choices.
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

/*
d_arg_internal_member_each_value
  The values a checked transformer accepts: the keys, then each value a pair
maps to that is not listed already.
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

/*
d_arg_internal_member_compare_of
  How a set of a type compares: as text for none, or for a text type; as
numbers for an arithmetic type; as the text it writes for any other type
whose values convert, plain, and are written back; -1 for a type it cannot
compare.
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

/*
d_arg_internal_member_source_from
  A source's member, or its pair's key.
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

/*
d_arg_internal_member_source_to
  A source's pair's value; a member maps to nothing.
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

/*
d_arg_internal_member_given
  Whether a source has every text it promises, and a count memory can hold.
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

/*
d_arg_internal_member_one
  One member's texts gathered into the buffer -- `from`, then `to` -- and its
key: `from` itself in a set of text; else `from` converted under the
configured integer rule, widened, or written back as its type writes it,
after the other two.
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

/*
d_arg_internal_member_body
  Every member's texts and key gathered into one buffer, a NUL after them so
that the text is never empty and always has storage, which the set then
owns. D_ARG_ERROR_BAD_ARGUMENT for a member that does not convert.
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

/*
d_arg_internal_member_take_filter
  The filter the set keeps: none when none is given; else a copy of it, its
context duplicated -- and owned -- when the filter can copy it, borrowed
when it cannot.
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

/*
d_arg_internal_member_make
  The record: how it compares, its members' texts and keys, and its filter;
nothing is kept on failure.
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

/*
d_arg_internal_member_build
  One of CLI11's three over a source: the record made, then given to a
validator of the kind's functions, which owns, copies and frees it.
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
