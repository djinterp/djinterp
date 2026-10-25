/*******************************************************************************
* djinterp [tools]                                                   dmsvs_doc.c
*
* A Visual Studio file held for change: see dmsvs_doc.h.
*   Every change is one splice, read again whole. Handles follow the
* elements across it by arithmetic, since a splice moves only what stands
* after it: an element that starts before the splice keeps its index, one
* inside it is gone, and one after it moves by the count the splice added
* less the count it took. An element that has moved is held to its name, in
* case the splice changed the tree around it.
*
* path:      /src/djinterp/tools/dawk/ext/msvs/dmsvs_doc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dmsvs_doc.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, memcmp, memset, strlen, strchr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dfs.h"      // d_fs_make_parent
#include "../../../../../../inc/djinterp/tools/dawk/dsplice.h"  // d_text_read, d_text_write


// d_msvs_doc
//   struct: the text, owned; its records; for each element its handle, and
//   for each handle ever given its element or D_MSVS_NONE.
struct d_msvs_doc
{
    char*            path;
    char*            text;
    size_t           length;
    enum d_msvs_kind kind;
    struct d_msvs    records;
    size_t*          handles;
    size_t*          places;
    size_t           place_count;
    size_t           place_capacity;
    size_t           changes;
    char             problem[320];
};

// d_internal_attribute
//   struct: one attribute of a tag: the bytes from the space before its name
//   to past its closing quote, its name, its value raw, and its quote.
struct d_internal_attribute
{
    size_t start;
    size_t end;
    size_t name;
    size_t name_length;
    size_t value;
    size_t value_length;
    char   quote;
};


//==============================================================================
// 1.  HELPERS
//==============================================================================


/*
d_internal_say
  Records why a change failed.
*/
static bool
d_internal_say(
    struct d_msvs_doc* _doc,
    const char*        _why
)
{
    (void)snprintf(_doc->problem, sizeof(_doc->problem), "%s", _why);

    return false;
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
        if (_length > 0u)
        {
            memcpy(copy, _text, _length);
        }

        copy[_length] = '\0';
    }

    return copy;
}


/*
d_internal_is_space
  XML's white space.
*/
static bool
d_internal_is_space(
    char _byte
)
{
    return ( (_byte == ' ')  ||
             (_byte == '\t') ||
             (_byte == '\r') ||
             (_byte == '\n') );
}


/*
d_internal_give_handles
  Gives each element from `_from` to `_to` a handle never given before.
*/
static bool
d_internal_give_handles(
    struct d_msvs_doc* _doc,
    size_t*            _handles,
    size_t             _from,
    size_t             _to
)
{
    // each element from `_from` a handle of its own
    for (size_t at = _from; at < _to; ++at)
    {
        // a handle is a place in the list of places, which grows
        if (_doc->place_count >= _doc->place_capacity)
        {
            const size_t  wanted = (_doc->place_capacity == 0u)
                                 ? 64u : (_doc->place_capacity * 2u);
            size_t* const grown  = realloc(_doc->places,
                                           wanted * sizeof(size_t));

            if (!grown)
            {
                return false;
            }

            _doc->places         = grown;
            _doc->place_capacity = wanted;
        }

        _handles[at]                    = _doc->place_count;
        _doc->places[_doc->place_count] = at;
        _doc->place_count++;
    }

    return true;
}


/*
d_internal_new
  A document owning a text, read as the kind its path names.
*/
static struct d_msvs_doc*
d_internal_new(
    const char* _path,
    char*       _text,
    size_t      _length,
    char*       _problem,
    size_t      _capacity
)
{
    struct d_msvs_doc* const doc = malloc(sizeof(*doc));

    // the document needs room, and its path
    if (!doc)
    {
        free(_text);
        (void)snprintf(_problem, _capacity, "out of memory");

        return NULL;
    }

    memset(doc, 0, sizeof(*doc));
    doc->text   = _text;
    doc->length = _length;
    doc->kind   = d_msvs_kind_of(_path);
    doc->path   = d_internal_copy(_path, strlen(_path));

    // the text must read as the kind its path names
    const bool ok = (doc->path != NULL) &&
                    (d_msvs_read(&doc->records, doc->text, doc->length,
                                 doc->kind, _problem, _capacity));

    // a text that does not read makes no document
    if (!ok)
    {
        d_msvs_doc_free(doc);

        return NULL;
    }

    doc->handles        = malloc((doc->records.element_count + 1u) *
                                 sizeof(size_t));
    doc->places         = malloc(64u * sizeof(size_t));
    doc->place_capacity = 64u;

    // handle 0 is no element's, and each element has the next
    if ( (doc->handles) &&
         (doc->places) )
    {
        doc->places[0]   = D_MSVS_NONE;
        doc->place_count = 1u;
    }

    // a list that is not there is memory run out
    if ( (!doc->handles) ||
         (!doc->places) ||
         (!d_internal_give_handles(doc, doc->handles, 0u,
                                   doc->records.element_count)) )
    {
        (void)snprintf(_problem, _capacity, "out of memory");
        d_msvs_doc_free(doc);

        return NULL;
    }

    return doc;
}


//==============================================================================
// 2.  DOCUMENTS
//==============================================================================


/*
d_msvs_doc_read
  The file's text, read whole.
*/
struct d_msvs_doc*
d_msvs_doc_read(
    const char* _path,
    char*       _problem,
    size_t      _capacity
)
{
    size_t length = 0;
    char*  text   = (_path) ? d_text_read(_path, &length) : NULL;

    // a file that cannot be read makes no document
    if (!text)
    {
        (void)snprintf(_problem, _capacity, "cannot read: %s",
                       (_path) ? _path : "(none)");

        return NULL;
    }

    return d_internal_new(_path, text, length, _problem, _capacity);
}


/*
d_msvs_doc_make
  A copy of the text, read as if from the path.
*/
struct d_msvs_doc*
d_msvs_doc_make(
    const char* _path,
    const char* _text,
    size_t      _length,
    char*       _problem,
    size_t      _capacity
)
{
    // both must be given
    if ( (!_path) ||
         (!_text) )
    {
        (void)snprintf(_problem, _capacity, "no path or no text");

        return NULL;
    }

    char* const text = d_internal_copy(_text, _length);

    if (!text)
    {
        (void)snprintf(_problem, _capacity, "out of memory");

        return NULL;
    }

    return d_internal_new(_path, text, _length, _problem, _capacity);
}


/*
d_msvs_doc_free
  The records, the text, and the document.
*/
void
d_msvs_doc_free(
    struct d_msvs_doc* _doc
)
{
    // freeing nothing is allowed
    if (!_doc)
    {
        return;
    }

    d_msvs_release(&_doc->records);
    free(_doc->path);
    free(_doc->text);
    free(_doc->handles);
    free(_doc->places);
    free(_doc);

    return;
}


/*
d_msvs_doc_records, d_msvs_doc_path, d_msvs_doc_kind, d_msvs_doc_changes,
d_msvs_doc_problem
  What the document holds.
*/
const struct d_msvs*
d_msvs_doc_records(
    const struct d_msvs_doc* _doc
)
{
    return (_doc) ? &_doc->records : NULL;
}


const char*
d_msvs_doc_path(
    const struct d_msvs_doc* _doc
)
{
    return (_doc) ? _doc->path : NULL;
}


enum d_msvs_kind
d_msvs_doc_kind(
    const struct d_msvs_doc* _doc
)
{
    return (_doc) ? _doc->records.kind : D_MSVS_OTHER;
}


size_t
d_msvs_doc_changes(
    const struct d_msvs_doc* _doc
)
{
    return (_doc) ? _doc->changes : 0u;
}


const char*
d_msvs_doc_problem(
    const struct d_msvs_doc* _doc
)
{
    return (_doc) ? _doc->problem : "";
}


//==============================================================================
// 3.  HANDLES
//==============================================================================


/*
d_msvs_doc_handle
  The handle kept for the index.
*/
size_t
d_msvs_doc_handle(
    const struct d_msvs_doc* _doc,
    size_t                   _element
)
{
    // an index past the last has no handle
    if ( (!_doc) ||
         (_element >= _doc->records.element_count) )
    {
        return 0u;
    }

    return _doc->handles[_element];
}


/*
d_msvs_doc_element
  The index kept for the handle.
*/
size_t
d_msvs_doc_element(
    const struct d_msvs_doc* _doc,
    size_t                   _handle
)
{
    // a handle never given names nothing
    if ( (!_doc) ||
         (_handle == 0u) ||
         (_handle >= _doc->place_count) )
    {
        return D_MSVS_NONE;
    }

    return _doc->places[_handle];
}


/*
d_internal_same_name
  Whether two elements, one of each reading, have one name.
*/
static bool
d_internal_same_name(
    const struct d_msvs* _old,
    size_t               _old_element,
    const struct d_msvs* _new,
    size_t               _new_element
)
{
    const struct d_msvs_element* const old = &_old->elements[_old_element];
    const struct d_msvs_element* const now = &_new->elements[_new_element];

    return ( (old->name_length == now->name_length) &&
             (memcmp(&_old->text[old->name], &_new->text[now->name],
                     old->name_length) == 0) );
}


/*
d_internal_follow
  Carries the handles from one reading to the next, across a splice of the
bytes from `_start` to `_end`: before it, an element keeps its index; in it,
it is gone; after it, it moves by what the splice added less what it took.
*/
static bool
d_internal_follow(
    struct d_msvs_doc*   _doc,
    const struct d_msvs* _old,
    const struct d_msvs* _new,
    size_t               _start,
    size_t               _end
)
{
    size_t* const handles = malloc((_new->element_count + 1u) *
                                   sizeof(size_t));

    // the new handles need room of their own
    if (!handles)
    {
        return false;
    }

    size_t before = 0;
    size_t inside = 0;

    // the elements that start before the splice keep their places
    while ( (before < _old->element_count) &&
            (_old->elements[before].start < _start) )
    {
        before++;
    }

    // those that start within it are gone
    while ( ((before + inside) < _old->element_count) &&
            (_old->elements[before + inside].start < _end) )
    {
        inside++;
    }

    const size_t after = _old->element_count - before - inside;
    const size_t added = (_new->element_count >= (before + after))
                       ? (_new->element_count - before - after) : 0u;
    const bool   sound = (_new->element_count == (before + added + after));

    // every old handle names nothing until its element is found again
    for (size_t at = 0; at < _old->element_count; ++at)
    {
        _doc->places[_doc->handles[at]] = D_MSVS_NONE;
    }

    // an element before the splice is where it was
    for (size_t at = 0; (sound) && (at < before); ++at)
    {
        handles[at]               = _doc->handles[at];
        _doc->places[handles[at]] = at;
    }

    // one after it has moved by what the splice added less what it took
    for (size_t at = 0; (sound) && (at < after); ++at)
    {
        const size_t old = before + inside + at;
        const size_t now = before + added + at;

        // an element moved is the same element only by the same name
        if (d_internal_same_name(_old, old, _new, now))
        {
            handles[now]               = _doc->handles[old];
            _doc->places[handles[now]] = now;
        }
        else if (!d_internal_give_handles(_doc, handles, now, now + 1u))
        {
            free(handles);

            return false;
        }
    }

    // what the splice made is new, and so is everything when the count
    // will not add up
    const bool given = (sound)
                     ? d_internal_give_handles(_doc, handles, before,
                                               before + added)
                     : d_internal_give_handles(_doc, handles, 0u,
                                               _new->element_count);

    if (!given)
    {
        free(handles);

        return false;
    }

    free(_doc->handles);
    _doc->handles = handles;

    return true;
}


//==============================================================================
// 4.  CHANGES
//==============================================================================


/*
d_msvs_doc_splice
  Builds the new text, reads it, and keeps it only if it reads whole.
*/
bool
d_msvs_doc_splice(
    struct d_msvs_doc* _doc,
    size_t             _start,
    size_t             _end,
    const char*        _text,
    size_t             _length
)
{
    // parameter validation first
    if (!_doc)
    {
        return false;
    }

    // the change must lie within the file
    if ( (_start > _end) ||
         (_end > _doc->length) ||
         ( (!_text) && (_length > 0u) ) )
    {
        return d_internal_say(_doc, "a change outside the file");
    }

    const size_t length = _doc->length - (_end - _start) + _length;
    char* const  text   = malloc(length + 1u);

    if (!text)
    {
        return d_internal_say(_doc, "out of memory");
    }

    memcpy(text, _doc->text, _start);

    // an insertion of nothing has nothing to copy
    if (_length > 0u)
    {
        memcpy(&text[_start], _text, _length);
    }

    memcpy(&text[_start + _length], &_doc->text[_end], _doc->length - _end);
    text[length] = '\0';

    struct d_msvs records;
    char          why[256];

    // the new text must read whole, as the kind the file is
    if (!d_msvs_read(&records, text, length, _doc->kind, why, sizeof(why)))
    {
        d_msvs_release(&records);
        free(text);
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "the change would leave the file unreadable: %s",
                       why);

        return false;
    }

    // the handles follow their elements into the new reading
    if (!d_internal_follow(_doc, &_doc->records, &records, _start, _end))
    {
        d_msvs_release(&records);
        free(text);

        return d_internal_say(_doc, "out of memory");
    }

    d_msvs_release(&_doc->records);
    free(_doc->text);
    _doc->text    = text;
    _doc->length  = length;
    _doc->records = records;
    _doc->changes++;
    _doc->problem[0] = '\0';

    return true;
}


/*
d_internal_element_of
  An element of the document by its index, or NULL with the failure said.
*/
static const struct d_msvs_element*
d_internal_element_of(
    struct d_msvs_doc* _doc,
    size_t             _element
)
{
    // the index must name an element of an XML file
    if ( (!_doc->records.xml) ||
         (_element >= _doc->records.element_count) )
    {
        (void)d_internal_say(_doc, "no such element");

        return NULL;
    }

    return &_doc->records.elements[_element];
}


/*
d_internal_find_attribute
  Walks a tag's attributes, which the reading has found sound, for one by
its name.
*/
static bool
d_internal_find_attribute(
    const struct d_msvs_doc*     _doc,
    const struct d_msvs_element* _element,
    const char*                  _name,
    struct d_internal_attribute* _out
)
{
    const char* const text = _doc->text;
    const size_t      name = strlen(_name);
    const size_t      end  = _element->attributes + _element->attributes_length;
    size_t            at   = _element->attributes;

    // one attribute at a time, from the space before it
    while (at < end)
    {
        const size_t space = at;

        // the space before its name
        while ( (at < end) &&
                (d_internal_is_space(text[at])) )
        {
            at++;
        }

        // only space is left
        if (at >= end)
        {
            break;
        }

        const size_t name_start = at;

        // its name, up to `=` or space
        while ( (at < end) &&
                (text[at] != '=') &&
                (!d_internal_is_space(text[at])) )
        {
            at++;
        }

        const size_t name_length = at - name_start;

        // past `=` and any space, to the quote
        while ( (at < end) &&
                (text[at] != '"') &&
                (text[at] != '\'') )
        {
            at++;
        }

        const char   quote = text[at++];
        const size_t value = at;

        // its value, up to the quote that closes it
        while ( (at < end) &&
                (text[at] != quote) )
        {
            at++;
        }

        const size_t value_length = at - value;

        at++;

        // the one asked for
        if ( (name_length == name) &&
             (memcmp(&text[name_start], _name, name) == 0) )
        {
            _out->start        = space;
            _out->end          = at;
            _out->name         = name_start;
            _out->name_length  = name_length;
            _out->value        = value;
            _out->value_length = value_length;
            _out->quote        = quote;

            return true;
        }
    }

    return false;
}


/*
d_internal_escaped
  A text escaped, allocated: an element's text for no quote, an attribute's
value for the quote around it.
*/
static char*
d_internal_escaped(
    const char* _text,
    char        _quote,
    size_t*     _out_length
)
{
    const size_t length = strlen(_text);
    char* const  out    = malloc((length * 6u) + 1u);

    // the escaped text needs room of its own
    if (out)
    {
        *_out_length = d_msvs_escape(_text, length, _quote, out);
    }

    return out;
}


/*
d_internal_tag_end
  Where the bytes after a tag's last attribute begin: past its name and the
last byte of its attributes that is not space.
*/
static size_t
d_internal_tag_end(
    const struct d_msvs_doc*     _doc,
    const struct d_msvs_element* _element
)
{
    size_t at = _element->attributes + _element->attributes_length;

    // back over the space before the tag's close
    while ( (at > _element->attributes) &&
            (d_internal_is_space(_doc->text[at - 1u])) )
    {
        at--;
    }

    return at;
}


/*
d_internal_local_name
  Whether a name is one a change may write: an XML name with no prefix.  A
prefix needs a namespace bound to it, which MSBuild, reading namespaces,
requires, and no file Visual Studio writes uses one.
*/
static bool
d_internal_local_name(
    const char* _name
)
{
    return ( (_name) &&
             (d_msvs_is_name(_name, strlen(_name))) &&
             (strchr(_name, ':') == NULL) );
}


/*
d_msvs_doc_set_attribute
  In place of the value it has, or after the last attribute.
*/
bool
d_msvs_doc_set_attribute(
    struct d_msvs_doc* _doc,
    size_t             _element,
    const char*        _name,
    const char*        _value
)
{
    // parameter validation first
    if ( (!_doc) ||
         (!_name) ||
         (!_value) )
    {
        return false;
    }

    const struct d_msvs_element* const element =
        d_internal_element_of(_doc, _element);

    if (!element)
    {
        return false;
    }

    // a name a change writes is one MSBuild reads
    if (!d_internal_local_name(_name))
    {
        return d_internal_say(_doc, "an attribute's name is not an XML "
                                    "name without a prefix");
    }

    struct d_internal_attribute attribute;
    const bool                  held   = d_internal_find_attribute(_doc,
                                                                  element,
                                                                  _name,
                                                                  &attribute);
    size_t                      length = 0;
    char* const                 escaped =
        d_internal_escaped(_value, (held) ? attribute.quote : '"', &length);
    bool                        ok     = false;

    if (!escaped)
    {
        return d_internal_say(_doc, "out of memory");
    }

    // a value replaced, in its own quotes, or an attribute added
    if (held)
    {
        ok = d_msvs_doc_splice(_doc, attribute.value,
                               attribute.value + attribute.value_length,
                               escaped, length);
    }
    else
    {
        const size_t name  = strlen(_name);
        char* const  added = malloc(name + length + 5u);

        if (added)
        {
            const size_t span =
                (size_t)snprintf(added, name + length + 5u, " %s=\"%s\"",
                                 _name, escaped);
            const size_t at   = d_internal_tag_end(_doc, element);

            ok = d_msvs_doc_splice(_doc, at, at, added, span);
            free(added);
        }
        else
        {
            (void)d_internal_say(_doc, "out of memory");
        }
    }

    free(escaped);

    return ok;
}


/*
d_msvs_doc_remove_attribute
  With the space before it.
*/
bool
d_msvs_doc_remove_attribute(
    struct d_msvs_doc* _doc,
    size_t             _element,
    const char*        _name
)
{
    // parameter validation first
    if ( (!_doc) ||
         (!_name) )
    {
        return false;
    }

    const struct d_msvs_element* const element =
        d_internal_element_of(_doc, _element);
    struct d_internal_attribute        attribute;

    if (!element)
    {
        return false;
    }

    // an attribute not there is not removed
    if (!d_internal_find_attribute(_doc, element, _name, &attribute))
    {
        return d_internal_say(_doc, "no such attribute");
    }

    return d_msvs_doc_splice(_doc, attribute.start, attribute.end, "", 0u);
}


/*
d_msvs_doc_set_text
  An empty element opens to hold it; any other has its content replaced.
*/
bool
d_msvs_doc_set_text(
    struct d_msvs_doc* _doc,
    size_t             _element,
    const char*        _text
)
{
    // parameter validation first
    if ( (!_doc) ||
         (!_text) )
    {
        return false;
    }

    const struct d_msvs_element* const element =
        d_internal_element_of(_doc, _element);

    if (!element)
    {
        return false;
    }

    // an element holding elements has no text of its own to set
    if (element->child_count > 0u)
    {
        return d_internal_say(_doc, "the element holds elements");
    }

    // nothing need change for an empty element given no text
    if ( (element->empty) &&
         (_text[0] == '\0') )
    {
        return true;
    }

    size_t      length  = 0;
    char* const escaped = d_internal_escaped(_text, '\0', &length);

    if (!escaped)
    {
        return d_internal_say(_doc, "out of memory");
    }

    bool ok = false;

    // content is replaced; an empty element opens to hold the text
    if (!element->empty)
    {
        ok = d_msvs_doc_splice(_doc, element->content_start,
                               element->content_end, escaped, length);
    }
    else
    {
        const size_t name   = element->name_length;
        const size_t span   = length + name + 4u;
        char* const  opened = malloc(span + 1u);

        if (opened)
        {
            const size_t at = d_internal_tag_end(_doc, element);

            (void)snprintf(opened, span + 1u, ">%s</%.*s>", escaped,
                           (int)name, &_doc->text[element->name]);
            ok = d_msvs_doc_splice(_doc, at, element->end, opened, span);
            free(opened);
        }
        else
        {
            (void)d_internal_say(_doc, "out of memory");
        }
    }

    free(escaped);

    return ok;
}


/*
d_internal_line_start
  Where the line holding a byte begins.
*/
static size_t
d_internal_line_start(
    const struct d_msvs_doc* _doc,
    size_t                   _at
)
{
    size_t at = _at;

    // back to the line ending before it
    while ( (at > 0u) &&
            (_doc->text[at - 1u] != '\n') )
    {
        at--;
    }

    return at;
}


/*
d_internal_indent
  The space an element is indented by: what stands before it on its line,
when that is all space; `NULL` when it shares its line.
*/
static const char*
d_internal_indent(
    const struct d_msvs_doc*     _doc,
    const struct d_msvs_element* _element,
    size_t*                      _out_length
)
{
    const size_t line = d_internal_line_start(_doc, _element->start);
    size_t       from = line;

    // a byte-order mark on the first line is not indenting
    if ( (from == 0u) &&
         (_doc->records.bom) )
    {
        from = 3u;
    }

    // only space may stand before it on its line
    for (size_t at = from; at < _element->start; ++at)
    {
        if ( (_doc->text[at] != ' ') &&
             (_doc->text[at] != '\t') )
        {
            return NULL;
        }
    }

    *_out_length = _element->start - from;

    return &_doc->text[from];
}


/*
d_internal_step
  The space the file indents a level by: the first element on a line of its
own beneath the root, less the root's own; two spaces when there is none.
*/
static void
d_internal_step(
    const struct d_msvs_doc* _doc,
    const char**             _out,
    size_t*                  _out_length
)
{
    const struct d_msvs* const records = &_doc->records;
    size_t                     root    = 0;
    const char* const          base    =
        d_internal_indent(_doc, &records->elements[0], &root);

    *_out        = "  ";
    *_out_length = 2u;

    for (size_t at = records->elements[0].first_child; at != D_MSVS_NONE;
         at = records->elements[at].next_sibling)
    {
        size_t            length = 0;
        const char* const indent = d_internal_indent(_doc,
                                                     &records->elements[at],
                                                     &length);

        // the first child on a line of its own, indented past the root
        if ( (indent) &&
             (length > ((base) ? root : 0u)) )
        {
            *_out        = indent + ((base) ? root : 0u);
            *_out_length = length - ((base) ? root : 0u);

            return;
        }
    }

    return;
}


// d_internal_writer
//   struct: a text being built, which grows.
struct d_internal_writer
{
    char*  text;
    size_t length;
    size_t capacity;
    bool   failed;
};


/*
d_internal_write
  Adds bytes to a text being built.
*/
static void
d_internal_write(
    struct d_internal_writer* _writer,
    const char*               _text,
    size_t                    _length
)
{
    // a text that has failed stays failed
    if (_writer->failed)
    {
        return;
    }

    // room for the bytes, the room doubling
    if ((_writer->length + _length + 1u) > _writer->capacity)
    {
        const size_t wanted = (_writer->length + _length + 1u) * 2u;
        char* const  grown  = realloc(_writer->text, wanted);

        if (!grown)
        {
            _writer->failed = true;

            return;
        }

        _writer->text     = grown;
        _writer->capacity = wanted;
    }

    memcpy(&_writer->text[_writer->length], _text, _length);
    _writer->length += _length;
    _writer->text[_writer->length] = '\0';

    return;
}


/*
d_internal_write_element
  An element's tags: <name a="v" /> with no text, <name a="v">text</name>
with one.
*/
static void
d_internal_write_element(
    struct d_internal_writer*        _writer,
    const struct d_msvs_new_element* _element
)
{
    const size_t name = strlen(_element->name);

    d_internal_write(_writer, "<", 1u);
    d_internal_write(_writer, _element->name, name);

    // each attribute, as a name and a value in turn
    for (size_t at = 0; (at + 1u) < (_element->attribute_count * 2u);
         at += 2u)
    {
        size_t      length  = 0;
        char* const escaped = d_internal_escaped(_element->attributes[at + 1u],
                                                 '"', &length);

        // each attribute, its value escaped
        if (!escaped)
        {
            _writer->failed = true;

            return;
        }

        d_internal_write(_writer, " ", 1u);
        d_internal_write(_writer, _element->attributes[at],
                         strlen(_element->attributes[at]));
        d_internal_write(_writer, "=\"", 2u);
        d_internal_write(_writer, escaped, length);
        d_internal_write(_writer, "\"", 1u);
        free(escaped);
    }

    // with no text the tag ends itself
    if (!_element->text)
    {
        d_internal_write(_writer, " />", 3u);

        return;
    }

    size_t      length  = 0;
    char* const escaped = d_internal_escaped(_element->text, '\0', &length);

    if (!escaped)
    {
        _writer->failed = true;

        return;
    }

    d_internal_write(_writer, ">", 1u);
    d_internal_write(_writer, escaped, length);
    d_internal_write(_writer, "</", 2u);
    d_internal_write(_writer, _element->name, name);
    d_internal_write(_writer, ">", 1u);
    free(escaped);

    return;
}


/*
d_internal_new_element_sound
  Whether an element to add has names it can have.
*/
static bool
d_internal_new_element_sound(
    const struct d_msvs_new_element* _element
)
{
    bool sound = d_internal_local_name(_element->name);

    // every attribute named soundly, with a value
    for (size_t at = 0; (sound) && (at < _element->attribute_count); ++at)
    {
        const char* const name  = _element->attributes[at * 2u];
        const char* const value = _element->attributes[(at * 2u) + 1u];

        sound = (value) && (d_internal_local_name(name));
    }

    return sound;
}


/*
d_internal_child_indent
  The space a new child of `_parent` is indented by: its siblings', when one
stands on a line of its own; else its parent's and one step more.
*/
static void
d_internal_child_indent(
    const struct d_msvs_doc*  _doc,
    size_t                    _parent,
    struct d_internal_writer* _out
)
{
    const struct d_msvs* const records = &_doc->records;

    // the indenting of the first sibling on a line of its own
    for (size_t at = records->elements[_parent].first_child;
         at != D_MSVS_NONE; at = records->elements[at].next_sibling)
    {
        size_t            length = 0;
        const char* const indent = d_internal_indent(_doc,
                                                     &records->elements[at],
                                                     &length);

        // a sibling on a line of its own says how
        if (indent)
        {
            d_internal_write(_out, indent, length);

            return;
        }
    }

    size_t            length = 0;
    const char*       step   = NULL;
    size_t            steps  = 0;
    const char* const indent = d_internal_indent(_doc,
                                                 &records->elements[_parent],
                                                 &length);

    d_internal_step(_doc, &step, &steps);

    // the parent's own, or a step for each level when it shares its line
    if (indent)
    {
        d_internal_write(_out, indent, length);
    }
    else
    {
        for (size_t level = 0; level < records->elements[_parent].depth;
             ++level)
        {
            d_internal_write(_out, step, steps);
        }
    }

    d_internal_write(_out, step, steps);

    return;
}


/*
d_internal_parent_indent
  The space a parent's end tag is indented by, on a line of its own.
*/
static void
d_internal_parent_indent(
    const struct d_msvs_doc*  _doc,
    size_t                    _parent,
    struct d_internal_writer* _out
)
{
    const struct d_msvs* const records = &_doc->records;
    size_t                     length  = 0;
    const char* const          indent  =
        d_internal_indent(_doc, &records->elements[_parent], &length);

    // its own line's, or a step for each level
    if (indent)
    {
        d_internal_write(_out, indent, length);

        return;
    }

    const char* step  = NULL;
    size_t      steps = 0;

    d_internal_step(_doc, &step, &steps);

    // a step for each level the parent is deep
    for (size_t level = 0; level < records->elements[_parent].depth; ++level)
    {
        d_internal_write(_out, step, steps);
    }

    return;
}


/*
d_msvs_doc_insert
  Four places an element can go: on the line of the child it goes before;
after the last child; in place of a content that is only space; or in an
empty element, which opens to hold it.
*/
size_t
d_msvs_doc_insert(
    struct d_msvs_doc*               _doc,
    size_t                           _parent,
    size_t                           _before,
    const struct d_msvs_new_element* _element
)
{
    // parameter validation first
    if ( (!_doc) ||
         (!_element) )
    {
        return D_MSVS_NONE;
    }

    const struct d_msvs_element* const parent =
        d_internal_element_of(_doc, _parent);

    if (!parent)
    {
        return D_MSVS_NONE;
    }

    // the names must be XML's, with no prefix
    if (!d_internal_new_element_sound(_element))
    {
        (void)d_internal_say(_doc, "a name is not an XML name without a "
                                   "prefix");

        return D_MSVS_NONE;
    }

    // a sibling to go before is a child of the parent
    if ( (_before != D_MSVS_NONE) &&
         ( (_before >= _doc->records.element_count) ||
           (_doc->records.elements[_before].parent != _parent) ) )
    {
        (void)d_internal_say(_doc, "not a child of the element");

        return D_MSVS_NONE;
    }

    const char* const        eol     = (_doc->records.crlf) ? "\r\n" : "\n";
    const size_t             eols    = strlen(eol);
    struct d_internal_writer writer  = { NULL, 0u, 0u, false };
    size_t                   start   = 0;
    size_t                   end     = 0;
    size_t                   offset  = 0;
    size_t                   last    = parent->first_child;

    // the parent's last child, which a new one follows
    while ( (last != D_MSVS_NONE) &&
            (_doc->records.elements[last].next_sibling != D_MSVS_NONE) )
    {
        last = _doc->records.elements[last].next_sibling;
    }

    // before a sibling, after the last child, or into a parent with none
    if (_before != D_MSVS_NONE)
    {
        const struct d_msvs_element* const sibling =
            &_doc->records.elements[_before];
        size_t                             length  = 0;

        // on a line of its own before the sibling's, or just before it
        if (d_internal_indent(_doc, sibling, &length))
        {
            start = d_internal_line_start(_doc, sibling->start);

            // a byte-order mark stays first
            if ( (start == 0u) &&
                 (_doc->records.bom) )
            {
                start = 3u;
            }

            d_internal_write(&writer, &_doc->text[start],
                             sibling->start - start);
            offset = writer.length;
            d_internal_write_element(&writer, _element);
            d_internal_write(&writer, eol, eols);
        }
        else
        {
            start = sibling->start;
            d_internal_write_element(&writer, _element);
        }

        end = start;
    }
    else if (last != D_MSVS_NONE)
    {
        // after the last child, on a line of its own
        start = _doc->records.elements[last].end;
        end   = start;
        d_internal_write(&writer, eol, eols);
        d_internal_child_indent(_doc, _parent, &writer);
        offset = writer.length;
        d_internal_write_element(&writer, _element);
    }
    else
    {
        const size_t name = parent->name_length;

        // content that is only space, or none, gives way to the child
        for (size_t at = parent->content_start; at < parent->content_end;
             ++at)
        {
            if (!d_internal_is_space(_doc->text[at]))
            {
                (void)d_internal_say(_doc, "the element holds text");

                return D_MSVS_NONE;
            }
        }

        start = (parent->empty) ? d_internal_tag_end(_doc, parent)
                                : parent->content_start;
        end   = (parent->empty) ? parent->end : parent->content_end;

        if (parent->empty)
        {
            d_internal_write(&writer, ">", 1u);
        }

        d_internal_write(&writer, eol, eols);
        d_internal_child_indent(_doc, _parent, &writer);
        offset = writer.length;
        d_internal_write_element(&writer, _element);
        d_internal_write(&writer, eol, eols);
        d_internal_parent_indent(_doc, _parent, &writer);

        if (parent->empty)
        {
            d_internal_write(&writer, "</", 2u);
            d_internal_write(&writer, &_doc->text[parent->name], name);
            d_internal_write(&writer, ">", 1u);
        }
    }

    // a text that could not be built changes nothing
    if (writer.failed)
    {
        free(writer.text);
        (void)d_internal_say(_doc, "out of memory");

        return D_MSVS_NONE;
    }

    const bool ok = d_msvs_doc_splice(_doc, start, end, writer.text,
                                      writer.length);

    free(writer.text);

    // the new element is the one that starts where it was written
    for (size_t at = 0; (ok) && (at < _doc->records.element_count); ++at)
    {
        if (_doc->records.elements[at].start == (start + offset))
        {
            return at;
        }
    }

    return D_MSVS_NONE;
}


/*
d_msvs_doc_remove
  The element's line, when it stands alone on it; else its bytes.  A parent
that held only it, and space, is closed up into an empty element instead.
*/
bool
d_msvs_doc_remove(
    struct d_msvs_doc* _doc,
    size_t             _element
)
{
    // parameter validation first
    if (!_doc)
    {
        return false;
    }

    const struct d_msvs_element* const element =
        d_internal_element_of(_doc, _element);

    if (!element)
    {
        return false;
    }

    // the root holds the file, and stays
    if (element->parent == D_MSVS_NONE)
    {
        return d_internal_say(_doc, "the root cannot be removed");
    }

    size_t       length = 0;
    size_t       start  = element->start;
    size_t       end    = element->end;
    const char*  indent = d_internal_indent(_doc, element, &length);
    size_t       after  = end;

    // past the space after it on its line
    while ( (after < _doc->length) &&
            ( (_doc->text[after] == ' ') || (_doc->text[after] == '\t') ) )
    {
        after++;
    }

    // and a carriage return before the line's feed
    if ( (after < _doc->length) &&
         (_doc->text[after] == '\r') )
    {
        after++;
    }

    // alone on its line, the line goes with it
    if ( (indent) &&
         (after < _doc->length) &&
         (_doc->text[after] == '\n') )
    {
        start = element->start - length;
        end   = after + 1u;
    }

    const struct d_msvs_element* const parent =
        &_doc->records.elements[element->parent];
    bool                               only   = (parent->child_count == 1u);

    // whether the parent holds anything besides it
    for (size_t at = parent->content_start; (only) &&
                                            (at < parent->content_end); ++at)
    {
        // the parent holds nothing else but space
        only = ( (at >= element->start) && (at < element->end) ) ||
               (d_internal_is_space(_doc->text[at]));
    }

    // a parent left holding nothing closes up, as Visual Studio writes it
    if (only)
    {
        return d_msvs_doc_splice(_doc, d_internal_tag_end(_doc, parent),
                                 parent->end, " />", 3u);
    }

    return d_msvs_doc_splice(_doc, start, end, "", 0u);
}


//==============================================================================
// 5.  SAVING
//==============================================================================


/*
d_msvs_doc_save
  Read as the path's kind, compared with the file there, and written only
where it differs.
*/
enum d_msvs_saved
d_msvs_doc_save(
    struct d_msvs_doc* _doc,
    const char*        _path
)
{
    // parameter validation first
    if (!_doc)
    {
        return D_MSVS_SAVE_FAILED;
    }

    const char* const target = (_path) ? _path : _doc->path;
    struct d_msvs     records;
    char              why[256];

    _doc->problem[0] = '\0';

    // the text must be the kind the path names
    if (!d_msvs_read(&records, _doc->text, _doc->length,
                     d_msvs_kind_of(target), why, sizeof(why)))
    {
        d_msvs_release(&records);
        (void)snprintf(_doc->problem, sizeof(_doc->problem),
                       "not a file of the kind %s names: %s", target, why);

        return D_MSVS_SAVE_FAILED;
    }

    d_msvs_release(&records);

    size_t      held   = 0;
    char* const there  = d_text_read(target, &held);
    const bool  same   = (there) && (held == _doc->length) &&
                         (memcmp(there, _doc->text, held) == 0);

    free(there);

    char* const path = (target == _doc->path)
                     ? _doc->path : d_internal_copy(target, strlen(target));

    if (!path)
    {
        (void)d_internal_say(_doc, "out of memory");

        return D_MSVS_SAVE_FAILED;
    }

    // a file holding the text already is left as it is, and one that is
    // not is written where its directories have been made
    if ( (!same) &&
         ( (!d_fs_make_parent(path)) ||
           (!d_text_write(path, _doc->text, _doc->length, _doc->problem,
                          sizeof(_doc->problem))) ) )
    {
        // the problem is the write's, or else the directory's
        if (_doc->problem[0] == '\0')
        {
            (void)snprintf(_doc->problem, sizeof(_doc->problem),
                           "cannot make the directory of %s", path);
        }

        if (path != _doc->path)
        {
            free(path);
        }

        return D_MSVS_SAVE_FAILED;
    }

    // the document is now the file's, and its records of the file's kind
    if (path != _doc->path)
    {
        free(_doc->path);
        _doc->path = path;
    }

    // a path of another kind reads the text as that kind
    if (d_msvs_kind_of(_doc->path) != _doc->kind)
    {
        _doc->kind = d_msvs_kind_of(_doc->path);
        d_msvs_release(&_doc->records);
        (void)d_msvs_read(&_doc->records, _doc->text, _doc->length,
                          _doc->kind, why, sizeof(why));
    }

    _doc->changes    = 0;
    _doc->problem[0] = '\0';

    return (same) ? D_MSVS_SAVE_UNCHANGED : D_MSVS_SAVE_WRITTEN;
}
