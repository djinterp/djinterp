/*******************************************************************************
* djinterp [tools]                                                     drepair.c
*
* Implements drepair.h: notes the repair a rule states for a finding, and
* makes a file's noted repairs once its pass is over.
*   Every edit goes through dedit.h's whole-file primitives, one write each,
* and is verified against the line on disk first. The edits are made from the
* last line up: an insertion or a removal moves only the lines below it, and
* those are done.
*
* path:      /src/djinterp/tools/dawk/drepair.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.10
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/drepair.h"  // corresponding header
// std
#include <stdio.h>                                        // fprintf, snprintf
#include <stdlib.h>                                       // qsort, realloc
#include <string.h>                                       // memcpy, strcmp
// djinterp
#include "../../../../inc/djinterp/tools/dawk/daudit.h"   // d_audit_verify
#include "../../../../inc/djinterp/tools/dawk/dbanner.h"  // D_BANNER_LINE_MAX
#include "../../../../inc/djinterp/tools/dawk/dedit.h"    // d_edit_*


/*
d_internal_is_blank
  A line of nothing but spaces, tabs and its own terminator.
*/
static bool
d_internal_is_blank(
    const char* _text
)
{
    for (size_t at = 0u; _text[at] != '\0'; ++at)
    {
        if ( (_text[at] != ' ')  &&
             (_text[at] != '\t') &&
             (_text[at] != '\r') &&
             (_text[at] != '\n') )
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_attribute
  A node's attribute by name: the value's text, or NULL when the node has no
such attribute.
*/
static const char*
d_internal_attribute(
    const struct d_node_tree* _tree,
    const struct d_node*      _node,
    const char*               _name
)
{
    for (uint32_t which = 0u; which < _node->attribute_count; ++which)
    {
        const struct d_node_attribute* const attribute =
            d_node_attribute_at(_tree, _node->first_attribute + which);

        if ( (attribute) &&
             (strcmp(d_node_name(_tree, attribute->name), _name) == 0) )
        {
            return d_node_text(_tree, attribute->value);
        }
    }

    return NULL;
}

/*
d_internal_refused
  Says which repair was not made and why, as dcheck's own refusals are said,
and counts it.
*/
static void
d_internal_refused(
    size_t*     _refused,
    const char* _path,
    uint32_t    _line,
    const char* _what,
    const char* _why
)
{
    (void)fprintf(stderr,
                  "%s:%u: %s repair refused: %s\n",
                  _path,
                  _line,
                  _what,
                  _why ? _why : "unstated");
    ++(*_refused);

    return;
}

/*
d_internal_order
  Last line first; on one line, what rewrites it before what moves it, which
is the order of d_repair_kind.
*/
static int
d_internal_order(
    const void* _a,
    const void* _b
)
{
    const struct d_repair_edit* const a = _a;
    const struct d_repair_edit* const b = _b;

    // the later line is made first
    if (a->line != b->line)
    {
        return (a->line > b->line) ? -1 : 1;
    }

    return (int)a->kind - (int)b->kind;
}

/*
d_internal_opens_line
  Reports whether a line, as the file now stands, begins at a column: that
is, whether the node noted there is still there and is first on its line.
*/
static bool
d_internal_opens_line(
    const char* _path,
    uint32_t    _line,
    uint32_t    _column
)
{
    char   text[D_BANNER_LINE_MAX] = "";
    size_t first                   = 0u;

    if (!d_edit_read_line(_path, _line, text, sizeof(text)))
    {
        return false;
    }

    while (text[first] == ' ')
    {
        ++first;
    }

    return ((first + 1u) == _column);
}

/*
d_internal_apply_blanks
  Counts the empty lines above the line as the file now stands and adds or
removes the difference. The node must open its line: an empty line above a
line says nothing of what stands in the middle of it, and would only part
the line from the comment over it. A line is removed only after it has been
read and found empty, so nothing but white space can be lost; each removal
is of the line just above, which is where the next empty one sits once the
one before it is gone.
*/
static size_t
d_internal_apply_blanks(
    const char*                 _path,
    const struct d_repair_edit* _edit
)
{
    char     text[D_BANNER_LINE_MAX] = "";
    uint32_t line                    = _edit->line;
    uint32_t have                    = 0u;
    size_t   done                    = 0u;

    if (!d_internal_opens_line(_path, line, _edit->column))
    {
        return 0u;
    }

    // the empty lines already above it
    while ( (line > (have + 1u))                                         &&
            (d_edit_read_line(_path, line - have - 1u, text,
                              sizeof(text)))                             &&
            (d_internal_is_blank(text)) )
    {
        ++have;
    }

    // too few: empty lines go in at the line itself
    for (; have < _edit->count; ++have)
    {
        if (!d_edit_insert_line(_path, line, ""))
        {
            return done;
        }

        ++line;
        ++done;
    }

    // too many: the one just above comes out, read again first
    for (; have > _edit->count; --have)
    {
        if ( (!d_edit_read_line(_path, line - 1u, text, sizeof(text))) ||
             (!d_internal_is_blank(text))                              ||
             (!d_edit_delete_line(_path, line - 1u)) )
        {
            return done;
        }

        --line;
        ++done;
    }

    return done;
}

/*
d_internal_summary_column
  Where a comment added to a line should begin so as to stand in the column
its neighbours' comments stand in: the include above or below it, when
that one carries a summary. 0 when neither does, and the comment goes two
spaces after the line's text.
*/
static size_t
d_internal_summary_column(
    const char* _path,
    uint32_t    _line
)
{
    for (uint32_t which = 0u; which < 2u; ++which)
    {
        char              near[D_BANNER_LINE_MAX] = "";
        const uint32_t    line = (which == 0u) ? (_line - 1u) : (_line + 1u);
        const bool        read = ( (line > 0u) &&
                                   (d_edit_read_line(_path, line, near,
                                                     sizeof(near))) );
        const char* const note = read ? strstr(near, "//") : NULL;

        // an include, with a summary after it
        if ( (note) && (strncmp(near, "#include", 8u) == 0) )
        {
            return (size_t)(note - near);
        }
    }

    return 0u;
}

/*
d_internal_apply_comment
  Adds `// words` to a line that has no comment. A line that has one is a
person's to reword: its comment may say something the words would lose. The
comment goes in its neighbours' column when they have one it can reach, and
two spaces after the text otherwise; and a line that would pass 80
characters by it is left, since a repair may not buy one rule with another
-- unless the rule states the comment `unmeasured`, of a line no rule
measures. The audit is asked as well as the check here, since it is the
gate every repair of dcheck's passes.
*/
static size_t
d_internal_apply_comment(
    const char* _path,
    uint32_t    _line,
    const char* _words,
    bool        _unmeasured,
    size_t*     _refused
)
{
    char                 before[D_BANNER_LINE_MAX] = "";
    char                 after[D_BANNER_LINE_MAX]  = "";
    struct d_audit_claim claim                     = { 0 };
    const char*          why                       = NULL;

    if (!d_edit_read_line(_path, _line, before, sizeof(before)))
    {
        return 0u;
    }

    const size_t length = d_edit_trim_end(before, strlen(before));
    const size_t column = d_internal_summary_column(_path, _line);
    const size_t gap    = (column >= (length + 2u)) ? (column - length) : 2u;

    before[length] = '\0';

    const size_t wanted = length + gap + 3u + strlen(_words);

    // a comment already, or no room for one within the line's 80 characters
    if ( (strstr(before, "//") != NULL)                           ||
         (strstr(before, "/*") != NULL)                           ||
         ((!_unmeasured) && (length <= 80u) && (wanted > 80u))    ||
         ((wanted + 1u) >= sizeof(after)) )
    {
        return 0u;
    }

    (void)snprintf(after, sizeof(after), "%s%*s// %s", before, (int)gap, "",
                   _words);

    claim.kind  = D_AUDIT_INSERT;
    claim.head  = length;
    claim.value = after + length;

    if (!d_audit_verify(before, after, &claim, &why))
    {
        d_internal_refused(_refused, _path, _line, "trailing-comment", why);

        return 0u;
    }

    return d_edit_rewrite_line(_path, _line, after) ? 1u : 0u;
}

// D_INTERNAL_REGION_MAX
//   constant: the most text a parameter list's lines may hold, old or new.
#define D_INTERNAL_REGION_MAX 4096

// D_INTERNAL_REGION_LINES
//   constant: the most lines a parameter list may run over.
#define D_INTERNAL_REGION_LINES 32

// d_internal_region
//   struct: a parameter list as it stands in the file: its lines' text,
// joined by line feeds; where each line begins in that text; and where its
// two brackets are.
struct d_internal_region
{
    char    text[D_INTERNAL_REGION_MAX];
    size_t  begins[D_INTERNAL_REGION_LINES];
    size_t  open;
    size_t  close;
};

// d_internal_writer
//   struct: a text being written, and how much of it is.
struct d_internal_writer
{
    char    text[D_INTERNAL_REGION_MAX];
    size_t  used;
};

// d_internal_parts
//   struct: the parameters found in a region: where each begins and how
// long it is, where its name begins within it (0 when it has none to split
// at), the longest type among them, and whether every one has a name.
struct d_internal_parts
{
    size_t  start[D_REPAIR_PARAM_MAX];
    size_t  length[D_REPAIR_PARAM_MAX];
    size_t  name[D_REPAIR_PARAM_MAX];
    size_t  widest;
    bool    named;
};

/*
d_internal_read_region
  Reads a bracketed run's lines into one text, joined by line feeds, and
finds its brackets in it. `_pair` is the two brackets expected. Fails when
the run is too long, a line cannot be read, or the brackets are not where
the edit says.
*/
static bool
d_internal_read_region(
    const char*                 _path,
    const struct d_repair_edit* _edit,
    const char*                 _pair,
    struct d_internal_region*   _out
)
{
    const uint32_t lines = (_edit->close_line - _edit->line) + 1u;
    size_t         used  = 0u;

    if ( (_edit->close_line < _edit->line) ||
         (lines > D_INTERNAL_REGION_LINES) )
    {
        return false;
    }

    for (uint32_t which = 0u; which < lines; ++which)
    {
        char line[D_BANNER_LINE_MAX] = "";

        if (!d_edit_read_line(_path, _edit->line + which, line,
                              sizeof(line)))
        {
            return false;
        }

        const size_t length = d_edit_trim_end(line, strlen(line));

        if ((used + length + 2u) >= sizeof(_out->text))
        {
            return false;
        }

        _out->begins[which] = used;
        memcpy(_out->text + used, line, length);
        used += length;
        _out->text[used] = '\n';
        ++used;
    }

    // the last line's feed is the run's own terminator, not its text
    _out->text[used - 1u] = '\0';
    _out->open            = _out->begins[0] + _edit->column - 1u;
    _out->close           = _out->begins[lines - 1u] + _edit->close_column
                          - 1u;

    return ( (_out->open < _out->close)             &&
             (_out->close < (used - 1u))            &&
             (_out->text[_out->open] == _pair[0])   &&
             (_out->text[_out->close] == _pair[1]) );
}

/*
d_internal_same_run
  Reports whether two texts hold the same characters that are neither
spaces nor line feeds, in the same order: what a repair of layout must
leave as it found.
*/
static bool
d_internal_same_run(
    const char* _a,
    const char* _b
)
{
    size_t at_a = 0u;
    size_t at_b = 0u;

    for (;;)
    {
        while ( (_a[at_a] == ' ') || (_a[at_a] == '\n') )
        {
            ++at_a;
        }

        while ( (_b[at_b] == ' ') || (_b[at_b] == '\n') )
        {
            ++at_b;
        }

        if ( (_a[at_a] != _b[at_b]) || (_a[at_a] == '\0') )
        {
            break;
        }

        ++at_a;
        ++at_b;
    }

    return ( (_a[at_a] == '\0') && (_b[at_b] == '\0') );
}

/*
d_internal_put
  Appends to a bounded text and reports whether it fitted. With `_text`
NULL, appends `_length` spaces. With `_words` set, each run of spaces in the
text becomes one: a parameter's own spacing was for the column it stood in,
which it is leaving.
*/
static bool
d_internal_put(
    struct d_internal_writer* _out,
    const char*               _text,
    size_t                    _length,
    bool                      _words
)
{
    for (size_t at = 0u; at < _length; ++at)
    {
        const char next = _text ? _text[at] : ' ';

        // one space where the text had several
        if ( (_words) && (_text) && (next == ' ') && (at > 0u) &&
             (_text[at - 1u] == ' ') )
        {
            continue;
        }

        if ((_out->used + 2u) >= sizeof(_out->text))
        {
            return false;
        }

        _out->text[_out->used] = next;
        ++_out->used;
    }

    _out->text[_out->used] = '\0';

    return true;
}

/*
d_internal_put_head
  Writes what stood before a brace that is moving to a line of its own: as
it stands, but for an `else` behind a closing brace. `} else` and
`} else if (...)` are written with the `}` alone on its line and the `else`
under it, which is where the tree has every other `else`: the brace being
moved goes under its owner, and a head left `} else` above it would be laid
out as nothing else in the tree is. The `}` must stand `_indent` spaces in,
where the moving brace is going, or the head is written as it stands.
*/
static bool
d_internal_put_head(
    struct d_internal_writer* _out,
    const char*               _head,
    size_t                    _length,
    uint32_t                  _indent
)
{
    size_t brace = 0u;

    while ( (brace < _length) && (_head[brace] == ' ') )
    {
        ++brace;
    }

    size_t word = brace + 1u;

    while ( (word < _length) && (_head[word] == ' ') )
    {
        ++word;
    }

    // `}`, then `else` as a whole word
    if ( (brace != _indent)                               ||
         (brace >= _length)                               ||
         (_head[brace] != '}')                            ||
         ((word + 4u) > _length)                          ||
         (memcmp(_head + word, "else", 4u) != 0)          ||
         ( ((word + 4u) < _length) && (_head[word + 4u] != ' ') ) )
    {
        return d_internal_put(_out, _head, _length, false);
    }

    return ( (d_internal_put(_out, _head, brace + 1u, false))        &&
             (d_internal_put(_out, "\n", 1u, false))                 &&
             (d_internal_put(_out, NULL, _indent, false))            &&
             (d_internal_put(_out, _head + word, _length - word, false)) );
}

/*
d_internal_apply_split
  Moves a brace that ends its line to a line of its own. The line must still
read as it did when the edit was noted -- the brace at its column, nothing
after it, something before it -- or the edit is dropped for the next pass to
measure again. What is written is checked first.
*/
static size_t
d_internal_apply_split(
    const char*                 _path,
    const struct d_repair_edit* _edit,
    size_t*                     _refused
)
{
    char                     before[D_BANNER_LINE_MAX] = "";
    struct d_internal_writer fresh                     = { "", 0u };

    if (!d_edit_read_line(_path, _edit->line, before, sizeof(before)))
    {
        return 0u;
    }

    const size_t length = d_edit_trim_end(before, strlen(before));

    before[length] = '\0';

    // the brace where it was, and last on its line
    if ( (_edit->column == 0u)                   ||
         (_edit->column != length)               ||
         (before[_edit->column - 1u] != '{') )
    {
        return 0u;
    }

    size_t kept = _edit->column - 1u;

    while ( (kept > 0u) && (before[kept - 1u] == ' ') )
    {
        --kept;
    }

    // nothing before it: it opens its line already
    if ( (kept == 0u)                                                  ||
         (!d_internal_put_head(&fresh, before, kept, _edit->indent))   ||
         (!d_internal_put(&fresh, "\n", 1u, false))                    ||
         (!d_internal_put(&fresh, NULL, _edit->indent, false))         ||
         (!d_internal_put(&fresh, "{", 1u, false)) )
    {
        return 0u;
    }

    if (!d_internal_same_run(before, fresh.text))
    {
        d_internal_refused(_refused, _path, _edit->line, "own-line",
                           "the line's text would not survive");

        return 0u;
    }

    return d_edit_replace_lines(_path, _edit->line, _edit->line, fresh.text)
           ? 1u
           : 0u;
}

/*
d_internal_offset
  Where a line and column of the file fall in a region's text.
*/
static size_t
d_internal_offset(
    const struct d_internal_region* _region,
    const struct d_repair_edit*     _edit,
    uint32_t                        _line,
    uint32_t                        _column
)
{
    return _region->begins[_line - _edit->line] + _column - 1u;
}

/*
d_internal_part
  Finds one parameter in the list's text: from its first token to the comma
that ends it, or to the list's `)` for the last, with the comma and the
space around it left off. Fails when the parameter runs over a line's end,
since joining its lines is more than moving it.
*/
static bool
d_internal_part(
    const struct d_internal_region* _region,
    const struct d_repair_edit*     _edit,
    uint32_t                        _which,
    struct d_internal_parts*        _out
)
{
    const char* const text  = _region->text;
    const bool        last  = ((_which + 1u) == _edit->parts);
    const size_t      start = d_internal_offset(_region, _edit,
                                                _edit->part_line[_which],
                                                _edit->part_column[_which]);
    size_t            end   = last
        ? _region->close
        : d_internal_offset(_region, _edit, _edit->part_line[_which + 1u],
                            _edit->part_column[_which + 1u]);

    while ( (end > start) &&
            ( (text[end - 1u] == ' ') || (text[end - 1u] == '\n') ) )
    {
        --end;
    }

    // every parameter but the last ends in its comma, which is put back
    // when the line is written
    if ( (!last) && (end > start) && (text[end - 1u] == ',') )
    {
        --end;

        while ( (end > start) && (text[end - 1u] == ' ') )
        {
            --end;
        }
    }
    else if (!last)
    {
        return false;
    }

    _out->start[_which]  = start;
    _out->length[_which] = (end > start) ? (end - start) : 0u;

    return ( (end > start) &&
             (memchr(text + start, '\n', end - start) == NULL) );
}

/*
d_internal_measure
  Finds every parameter and, for each, where its name splits it: after a
type and inside its own text. Names go in one column only when every
parameter has one to split at; the column is a space past the longest type.
*/
static bool
d_internal_measure(
    const struct d_internal_region* _region,
    const struct d_repair_edit*     _edit,
    struct d_internal_parts*        _out
)
{
    _out->widest = 0u;
    _out->named  = true;

    for (uint32_t which = 0u; which < _edit->parts; ++which)
    {
        if (!d_internal_part(_region, _edit, which, _out))
        {
            return false;
        }

        const size_t start = _out->start[which];
        const size_t name  = (_edit->name_line[which] != 0u)
            ? d_internal_offset(_region, _edit, _edit->name_line[which],
                                _edit->name_column[which])
            : 0u;
        size_t       type  = ( (name > start) &&
                               (name < (start + _out->length[which])) )
                           ? (name - start)
                           : 0u;

        _out->name[which] = type;
        _out->named       = ( (_out->named) && (type > 0u) );

        while ( (type > 0u) && (_region->text[start + type - 1u] == ' ') )
        {
            --type;
        }

        _out->widest = (type > _out->widest) ? type : _out->widest;
    }

    return true;
}

/*
d_internal_put_part
  Writes one parameter on a line of its own, one level in from the
function: its type, then its name in the names' column when they have one.
*/
static bool
d_internal_put_part(
    const struct d_internal_region* _region,
    const struct d_internal_parts*  _parts,
    uint32_t                        _which,
    size_t                          _indent,
    struct d_internal_writer*       _out
)
{
    const char* const text = _region->text + _parts->start[_which];
    size_t            type = _parts->named ? _parts->name[_which]
                                           : _parts->length[_which];

    while ( (_parts->named) && (type > 0u) && (text[type - 1u] == ' ') )
    {
        --type;
    }

    const size_t column = _out->used + 1u + _indent + _parts->widest + 1u;

    if ( (!d_internal_put(_out, "\n", 1u, false))     ||
         (!d_internal_put(_out, NULL, _indent, false)) ||
         (!d_internal_put(_out, text, type, true)) )
    {
        return false;
    }

    // the name's column: the longest type, and a space
    return ( (!_parts->named) ||
             ( (d_internal_put(_out, NULL, column - _out->used, false)) &&
               (d_internal_put(_out, text + _parts->name[_which],
                               _parts->length[_which] - _parts->name[_which],
                               true)) ) );
}

/*
d_internal_lay_out
  Writes the list again: the text up to its `(` as it was, each parameter
on a line of its own, and the `)` under the function with whatever followed
it. A parameter with no name to split at -- a function pointer, whose name
is inside its own brackets -- leaves every name a space from its type.
*/
static bool
d_internal_lay_out(
    const struct d_internal_region* _region,
    const struct d_repair_edit*     _edit,
    struct d_internal_writer*       _out
)
{
    struct d_internal_parts parts = { { 0 }, { 0 }, { 0 }, 0u, true };

    if ( (!d_internal_measure(_region, _edit, &parts)) ||
         (!d_internal_put(_out, _region->text, _region->open + 1u, false)) )
    {
        return false;
    }

    for (uint32_t which = 0u; which < _edit->parts; ++which)
    {
        if ( (!d_internal_put_part(_region, &parts, which,
                                   _edit->indent + 4u, _out))    ||
             ( ((which + 1u) < _edit->parts) &&
               (!d_internal_put(_out, ",", 1u, false)) ) )
        {
            return false;
        }
    }

    return ( (d_internal_put(_out, "\n", 1u, false))            &&
             (d_internal_put(_out, NULL, _edit->indent, false)) &&
             (d_internal_put(_out, _region->text + _region->close,
                             strlen(_region->text + _region->close),
                             false)) );
}

/*
d_internal_apply_layout
  Lays a parameter list out one to a line. The function must still open its
line where it did, and the list its brackets; a list that holds a comment, a
directive or a quoted text is left, since moving its lines might move what a
comment is beside or what a quote holds. What is written is checked first:
the same characters, in the same order, but for spaces and line ends.
*/
static size_t
d_internal_apply_layout(
    const char*                 _path,
    const struct d_repair_edit* _edit,
    size_t*                     _refused
)
{
    struct d_internal_region region = { "", { 0 }, 0u, 0u };
    struct d_internal_writer fresh  = { "", 0u };

    if ( (!d_internal_opens_line(_path, _edit->owner_line,
                                 _edit->indent + 1u))        ||
         (!d_internal_read_region(_path, _edit, "()", &region)) ||
         (strstr(region.text, "//") != NULL)                 ||
         (strstr(region.text, "/*") != NULL)                 ||
         (strchr(region.text, '#') != NULL)                  ||
         (strchr(region.text, '"') != NULL)                  ||
         (strchr(region.text, '\'') != NULL) )
    {
        return 0u;
    }

    const size_t first = d_internal_offset(&region, _edit,
                                           _edit->part_line[0],
                                           _edit->part_column[0]);

    // nothing but space between the `(` and the first parameter
    for (size_t at = region.open + 1u; at < first; ++at)
    {
        if ( (region.text[at] != ' ') && (region.text[at] != '\n') )
        {
            return 0u;
        }
    }

    if (!d_internal_lay_out(&region, _edit, &fresh))
    {
        return 0u;
    }

    if (!d_internal_same_run(region.text, fresh.text))
    {
        d_internal_refused(_refused, _path, _edit->line, "one-per-line",
                           "the list's text would not survive");

        return 0u;
    }

    return d_edit_replace_lines(_path, _edit->line, _edit->close_line,
                                fresh.text)
           ? 1u
           : 0u;
}

/*
d_internal_apply_join
  Puts an empty pair of braces on one line: `{` and, on a later line, `}`
with nothing between them become `{}`, followed by whatever followed the
`}`. The opening brace must still be first on its line -- one that ends a
line of text is the brace rule's to move first -- and there must be nothing
between the two but space.
*/
static size_t
d_internal_apply_join(
    const char*                 _path,
    const struct d_repair_edit* _edit,
    size_t*                     _refused
)
{
    struct d_internal_region region = { "", { 0 }, 0u, 0u };
    struct d_internal_writer fresh  = { "", 0u };

    if ( (!d_internal_opens_line(_path, _edit->line, _edit->column)) ||
         (!d_internal_read_region(_path, _edit, "{}", &region)) )
    {
        return 0u;
    }

    for (size_t at = region.open + 1u; at < region.close; ++at)
    {
        if ( (region.text[at] != ' ') && (region.text[at] != '\n') )
        {
            return 0u;
        }
    }

    if ( (!d_internal_put(&fresh, region.text, region.open + 1u, false)) ||
         (!d_internal_put(&fresh, region.text + region.close,
                          strlen(region.text + region.close), false)) )
    {
        return 0u;
    }

    if (!d_internal_same_run(region.text, fresh.text))
    {
        d_internal_refused(_refused, _path, _edit->line, "join",
                           "the braces' text would not survive");

        return 0u;
    }

    return d_edit_replace_lines(_path, _edit->line, _edit->close_line,
                                fresh.text)
           ? 1u
           : 0u;
}

/*
d_internal_put_held
  Writes each thing a pair of braces holds on a line of its own, one level
in from the braces: from where it begins to where the next does, or to the
closing brace, with the space after it left off.
*/
static bool
d_internal_put_held(
    const struct d_internal_region* _region,
    const struct d_repair_edit*     _edit,
    struct d_internal_writer*       _out
)
{
    for (uint32_t which = 0u; which < _edit->parts; ++which)
    {
        const size_t start = _edit->part_column[which] - 1u;
        size_t       end   = ((which + 1u) < _edit->parts)
                           ? (_edit->part_column[which + 1u] - 1u)
                           : _region->close;

        while ( (end > start) && (_region->text[end - 1u] == ' ') )
        {
            --end;
        }

        if ( (end <= start) || (start <= _region->open)                 ||
             (!d_internal_put(_out, "\n", 1u, false))                   ||
             (!d_internal_put(_out, NULL, _edit->indent + 4u, false))   ||
             (!d_internal_put(_out, _region->text + start, end - start,
                              false)) )
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_apply_expand
  Lays out a body that stands on its brace's line: the text before the
brace keeps its line (d_internal_put_head), the brace goes under its owner,
each thing the braces
hold goes on a line of its own one level in, and the closing brace goes
under the opening one with whatever followed it. A body that holds a
comment is left, and so is one followed by another body on the same line,
as `} else { ... }` is: each is more than this moves.
*/
static size_t
d_internal_apply_expand(
    const char*                 _path,
    const struct d_repair_edit* _edit,
    size_t*                     _refused
)
{
    struct d_internal_region region = { "", { 0 }, 0u, 0u };
    struct d_internal_writer fresh  = { "", 0u };

    if ( (_edit->close_line != _edit->line)                       ||
         (!d_internal_read_region(_path, _edit, "{}", &region))   ||
         (strstr(region.text + region.open, "//") != NULL)        ||
         (strstr(region.text + region.open, "/*") != NULL)        ||
         (strpbrk(region.text + region.close + 1u, "{}") != NULL) )
    {
        return 0u;
    }

    size_t head = region.open;

    while ( (head > 0u) && (region.text[head - 1u] == ' ') )
    {
        --head;
    }

    // something before the brace, or it opens its line already
    if ( (head == 0u)                                                  ||
         (!d_internal_put_head(&fresh, region.text, head,
                               _edit->indent))                         ||
         (!d_internal_put(&fresh, "\n", 1u, false))                    ||
         (!d_internal_put(&fresh, NULL, _edit->indent, false))         ||
         (!d_internal_put(&fresh, "{", 1u, false))                     ||
         (!d_internal_put_held(&region, _edit, &fresh))                ||
         (!d_internal_put(&fresh, "\n", 1u, false))                    ||
         (!d_internal_put(&fresh, NULL, _edit->indent, false))         ||
         (!d_internal_put(&fresh, region.text + region.close,
                          strlen(region.text + region.close), false)) )
    {
        return 0u;
    }

    if (!d_internal_same_run(region.text, fresh.text))
    {
        d_internal_refused(_refused, _path, _edit->line, "own-line",
                           "the body's text would not survive");

        return 0u;
    }

    return d_edit_replace_lines(_path, _edit->line, _edit->line, fresh.text)
           ? 1u
           : 0u;
}

/*
d_internal_number_of
  A node's numeric attribute by name, or 0 when it has none.
*/
static uint32_t
d_internal_number_of(
    const struct d_node_tree* _tree,
    const struct d_node*      _node,
    const char*               _name
)
{
    for (uint32_t which = 0u; which < _node->attribute_count; ++which)
    {
        const struct d_node_attribute* const attribute =
            d_node_attribute_at(_tree, _node->first_attribute + which);

        if ( (attribute) && (attribute->numeric) &&
             (strcmp(d_node_name(_tree, attribute->name), _name) == 0) )
        {
            return (uint32_t)attribute->number;
        }
    }

    return 0u;
}

/*
d_internal_is
  Reports whether a node is of a type, and, when a name is given, carries
that attribute.
*/
static bool
d_internal_is(
    const struct d_node_tree* _tree,
    const struct d_node*      _node,
    const char*               _type,
    const char*               _carrying
)
{
    if (strcmp(d_node_name(_tree, _node->type), _type) != 0)
    {
        return false;
    }

    for (uint32_t which = 0u;
         (_carrying) && (which < _node->attribute_count);
         ++which)
    {
        const struct d_node_attribute* const attribute =
            d_node_attribute_at(_tree, _node->first_attribute + which);

        if ( (attribute) &&
             (strcmp(d_node_name(_tree, attribute->name), _carrying) == 0) )
        {
            return true;
        }
    }

    return (_carrying == NULL);
}

/*
d_internal_note_part
  Records one parameter of a list being noted: where it begins, and where
its declared name stands when the name is the parameter's own child -- a
function pointer's is a level further in, and is left unrecorded.
*/
static void
d_internal_note_part(
    struct d_node_tree*   _tree,
    const struct d_node*  _part,
    struct d_repair_edit* _out
)
{
    const uint32_t which = _out->parts;

    _out->part_line[which]   = _part->line;
    _out->part_column[which] = _part->start_column;
    _out->name_line[which]   = 0u;
    _out->name_column[which] = 0u;

    for (uint32_t at = _part->first_child; at != D_DSS_NO_INDEX; )
    {
        const struct d_node* const child = d_node_at(_tree, at);

        if (!child)
        {
            break;
        }

        if (d_internal_is(_tree, child, "identifier", "declarator"))
        {
            _out->name_line[which]   = child->line;
            _out->name_column[which] = child->start_column;
        }

        at = child->next_sibling;
    }

    ++_out->parts;

    return;
}

/*
d_internal_note_parts
  Records every parameter that is a group's own child, up to the most an
edit can carry.
*/
static void
d_internal_note_parts(
    struct d_node_tree*   _tree,
    const struct d_node*  _group,
    struct d_repair_edit* _out
)
{
    for (uint32_t at = _group->first_child; at != D_DSS_NO_INDEX; )
    {
        const struct d_node* const part = d_node_at(_tree, at);

        if (!part)
        {
            break;
        }

        at = part->next_sibling;

        if ( (d_internal_is(_tree, part, "param", NULL)) &&
             (_out->parts < D_REPAIR_PARAM_MAX) )
        {
            d_internal_note_part(_tree, part, _out);
        }
    }

    return;
}

/*
d_internal_layout_from
  Finds a function's parameter list -- its first parenthesis that holds a
parameter -- and records where the list and each parameter stand. A list of
one parameter, or of more than an edit can carry, is left. So is a function
that does not open its line, or stands in another column than its level
gives: its column is what the list is laid out from, and a wrong one would
only be copied into the lines written.
*/
static bool
d_internal_layout_from(
    struct d_node_tree*   _tree,
    const struct d_node*  _node,
    struct d_repair_edit* _out
)
{
    const uint32_t level = d_internal_number_of(_tree, _node, "level");

    if ( (!d_internal_is(_tree, _node, "fn-def", "line-start")) ||
         (_node->start_column != ((level * 4u) + 1u)) )
    {
        return false;
    }

    for (uint32_t at = _node->first_child; at != D_DSS_NO_INDEX; )
    {
        const struct d_node* const group = d_node_at(_tree, at);

        if (!group)
        {
            break;
        }

        at          = group->next_sibling;
        _out->parts = 0u;

        if (d_internal_is(_tree, group, "group", "close-line"))
        {
            d_internal_note_parts(_tree, group, _out);
        }

        if (_out->parts >= 2u)
        {
            _out->kind         = D_REPAIR_LAYOUT;
            _out->line         = group->line;
            _out->column       = group->start_column;
            _out->close_line   = d_internal_number_of(_tree, group,
                                                      "close-line");
            _out->close_column = d_internal_number_of(_tree, group,
                                                      "close-column");
            _out->indent       = _node->start_column - 1u;
            _out->owner_line   = _node->line;

            return (_out->close_line != 0u);
        }
    }

    return false;
}

/*
d_internal_stated
  The rule's own `repair` declaration, or NULL when it states none.
*/
static const struct d_dss_declaration*
d_internal_stated(
    const struct d_dss_sheet* _sheet,
    const struct d_dss_rule*  _rule
)
{
    for (uint32_t which = 0u; which < _rule->declaration_count; ++which)
    {
        const struct d_dss_declaration* const declaration =
            d_dss_declaration_at(_sheet, _rule->first_declaration + which);

        if (strcmp(d_dss_text(_sheet, declaration->property), "repair") == 0)
        {
            return declaration;
        }
    }

    return NULL;
}

/*
d_internal_words
  The words of a `trailing-comment` repair: a string is the words, and a
name is the node's attribute that holds them. NULL when there are none, or
more than an edit can carry.
*/
static const char*
d_internal_words(
    const struct d_dss_sheet* _sheet,
    const struct d_node_tree* _tree,
    const struct d_node*      _node,
    const struct d_dss_value* _what
)
{
    const char* const words =
        (_what->kind == D_DSS_VALUE_STRING)
        ? d_dss_text(_sheet, _what->text)
        : (_what->kind == D_DSS_VALUE_IDENT)
        ? d_internal_attribute(_tree, _node, d_dss_text(_sheet, _what->text))
        : NULL;

    if ( (!words) || (words[0] == '\0') ||
         (strlen(words) >= D_REPAIR_TEXT_MAX) )
    {
        return NULL;
    }

    return words;
}

/*
d_internal_owner_indent
  Where a brace moved to its own line goes: under the item that owns it -- a
statement, a definition, a type -- at that item's own column. A block that
is a statement by itself owns its own brace and has no column to give but
the brace's; when it stands after a case label on the label's line, it goes
under the label. Any other is left.
*/
static bool
d_internal_owner_indent(
    struct d_node_tree*  _tree,
    const struct d_node* _node,
    uint32_t*            _out_indent
)
{
    const struct d_node* const owner = d_node_at(_tree, _node->parent);

    if ( (!owner) || (owner->line == 0u) || (owner->start_column == 0u) ||
         (strcmp(d_node_name(_tree, owner->type), "group") == 0)        ||
         (strcmp(d_node_name(_tree, owner->type), "file") == 0) )
    {
        return false;
    }

    *_out_indent = owner->start_column - 1u;

    // the owner begins before its brace: its column is the one
    if ( (owner->line < _node->line) ||
         (owner->start_column < _node->start_column) )
    {
        return true;
    }

    const struct d_node* const around = d_node_at(_tree, owner->parent);
    const struct d_node*       before = NULL;

    // what stands just before the block, among its siblings
    for (uint32_t at = around ? around->first_child : D_DSS_NO_INDEX;
         at != D_DSS_NO_INDEX; )
    {
        const struct d_node* const sibling = d_node_at(_tree, at);

        if ( (!sibling) || (sibling == owner) )
        {
            break;
        }

        before = sibling;
        at     = sibling->next_sibling;
    }

    if ( (!before) || (before->line != _node->line) ||
         (strcmp(d_node_name(_tree, before->type), "label") != 0) )
    {
        return false;
    }

    *_out_indent = before->start_column - 1u;

    return true;
}

/*
d_internal_brace_from
  Turns `repair: own-line` into an edit against a brace. One whose partner
is on a later line is moved; one whose partner is on its own line has what
is between them noted, to be laid out under it.
*/
static bool
d_internal_brace_from(
    struct d_node_tree*   _tree,
    const struct d_node*  _node,
    struct d_repair_edit* _out
)
{
    _out->kind         = D_REPAIR_SPLIT;
    _out->close_line   = d_internal_number_of(_tree, _node, "close-line");
    _out->close_column = d_internal_number_of(_tree, _node, "close-column");
    _out->parts        = 0u;

    if (!d_internal_owner_indent(_tree, _node, &_out->indent))
    {
        return false;
    }

    if (_out->close_line != _node->line)
    {
        return true;
    }

    _out->kind = D_REPAIR_EXPAND;

    for (uint32_t at = _node->first_child; at != D_DSS_NO_INDEX; )
    {
        const struct d_node* const part = d_node_at(_tree, at);

        if ( (!part) || (part->line != _node->line) ||
             (_out->parts == D_REPAIR_PARAM_MAX) )
        {
            return false;
        }

        _out->part_line[_out->parts]   = part->line;
        _out->part_column[_out->parts] = part->start_column;
        ++_out->parts;
        at = part->next_sibling;
    }

    return (_out->parts > 0u);
}

/*
d_internal_third_is
  Whether the third value of a `repair` declaration is a given word: what
may follow a `trailing-comment`'s words.
*/
static bool
d_internal_third_is(
    const struct d_dss_sheet*       _sheet,
    const struct d_dss_declaration* _stated,
    const char*                     _word
)
{
    const struct d_dss_value* const third =
        (_stated->value_count > 2u)
        ? d_dss_value_at(_sheet, _stated->first_value + 2u)
        : NULL;

    return ( (third)                            &&
             (third->kind == D_DSS_VALUE_IDENT) &&
             (strcmp(d_dss_text(_sheet, third->text), _word) == 0) );
}

/*
d_internal_comment_from
  Turns `repair: trailing-comment` into an edit: the comment's words, from
the declaration or from the node. Nothing is noted when there are none.
*/
static bool
d_internal_comment_from(
    const struct d_dss_sheet* _sheet,
    const struct d_node_tree* _tree,
    const struct d_node*      _node,
    const struct d_dss_value* _what,
    struct d_repair_edit*     _out
)
{
    const char* const words = d_internal_words(_sheet, _tree, _node, _what);

    if (!words)
    {
        return false;
    }

    _out->kind = D_REPAIR_COMMENT;
    memcpy(_out->text, words, strlen(words) + 1u);

    return true;
}

/*
d_internal_join_from
  Turns `repair: join` into an edit against a brace: where it stands, and
where the tree says its partner does. Nothing is noted for a pair already
on one line.
*/
static bool
d_internal_join_from(
    const struct d_node_tree* _tree,
    const struct d_node*      _node,
    struct d_repair_edit*     _out
)
{
    _out->kind         = D_REPAIR_JOIN;
    _out->close_line   = d_internal_number_of(_tree, _node, "close-line");
    _out->close_column = d_internal_number_of(_tree, _node, "close-column");

    return (_out->close_line > _node->line);
}

/*
d_internal_edit_from
  Turns a `repair` declaration into an edit against the node's line. The
three forms are told by the first word. A brace is moved only when its owner
is an item -- a statement, a definition, a type -- whose own column says
where the brace goes; a compound statement standing by itself has no such
owner and is left.
*/
static bool
d_internal_edit_from(
    const struct d_dss_sheet*       _sheet,
    struct d_node_tree*             _tree,
    const struct d_node*            _node,
    const struct d_dss_declaration* _stated,
    struct d_repair_edit*           _out
)
{
    const struct d_dss_value* const how  =
        d_dss_value_at(_sheet, _stated->first_value);
    const struct d_dss_value* const what =
        (_stated->value_count > 1u)
        ? d_dss_value_at(_sheet, _stated->first_value + 1u)
        : NULL;
    const char* const               word =
        ( (how) && (how->kind == D_DSS_VALUE_IDENT) )
        ? d_dss_text(_sheet, how->text)
        : "";

    _out->line   = _node->line;
    _out->column = _node->start_column;

    if ( (strcmp(word, "blank-before") == 0) &&
         (what)                              &&
         (what->kind == D_DSS_VALUE_NUMBER) )
    {
        _out->kind  = D_REPAIR_BLANKS;
        _out->count = (uint32_t)what->number;

        return true;
    }

    // `unmeasured` after the words: the line may pass the limit by them
    if ( (strcmp(word, "trailing-comment") == 0) && (what) )
    {
        _out->count = d_internal_third_is(_sheet, _stated, "unmeasured")
                    ? 1u
                    : 0u;

        return d_internal_comment_from(_sheet, _tree, _node, what, _out);
    }

    // a function's parameters, one to a line
    if (strcmp(word, "one-per-line") == 0)
    {
        return d_internal_layout_from(_tree, _node, _out);
    }

    // an empty pair of braces, on one
    if (strcmp(word, "join") == 0)
    {
        return d_internal_join_from(_tree, _node, _out);
    }

    // a brace goes under its owner, and a body on its line under it
    return ( (strcmp(word, "own-line") == 0) &&
             (d_internal_brace_from(_tree, _node, _out)) );
}

/*
d_internal_comment_lines
  Where a comment that begins its line in a column ends: its last line, a
block comment counted to the line that closes it. 0 for a node that is no
such comment.
*/
static uint32_t
d_internal_comment_lines(
    const struct d_node_tree* _tree,
    const struct d_node*      _node,
    uint32_t                  _column
)
{
    if ( (!d_internal_is(_tree, _node, "comment", "line-start")) ||
         (_node->start_column != _column) )
    {
        return 0u;
    }

    const char* const text = d_node_text(_tree, _node->text);
    uint32_t          last = _node->line;

    for (size_t at = 0u; (text) && (text[at] != '\0'); ++at)
    {
        last += (text[at] == '\n') ? 1u : 0u;
    }

    return last;
}

/*
d_internal_comments_above
  The line the comments that stand before an item begin on, or the item's
own. They are its siblings just before it that are comments in its column,
with nothing between them and it but each other and empty lines: a comment
above an item, in the item's column, speaks of the item.
  A comment left where it was when the item under it moved would stand in
another column than what it speaks of, and no rule would say so: a comment
has no level.
*/
static uint32_t
d_internal_comments_above(
    struct d_node_tree*  _tree,
    const struct d_node* _node
)
{
    const struct d_node* const around = d_node_at(_tree, _node->parent);
    uint32_t                   first  = 0u;

    for (uint32_t at = around ? around->first_child : D_DSS_NO_INDEX;
         at != D_DSS_NO_INDEX; )
    {
        const struct d_node* const sibling = d_node_at(_tree, at);

        if ( (!sibling) || (sibling == _node) )
        {
            break;
        }

        // the run begins at its first comment, and anything else ends it
        if (d_internal_comment_lines(_tree, sibling,
                                     _node->start_column) == 0u)
        {
            first = 0u;
        }
        else if (first == 0u)
        {
            first = sibling->line;
        }

        at = sibling->next_sibling;
    }

    return (first != 0u) ? first : _node->line;
}

/*
d_internal_comments_below
  The line the comments that stand after an item end on, or `_last`, the
item's own last. They are its siblings just after it that are comments in
its column -- when what follows them is not in that column, or nothing
follows: they close what the item began. When what follows them is in the
item's column they are that one's, and move when it does.
*/
static uint32_t
d_internal_comments_below(
    struct d_node_tree*  _tree,
    const struct d_node* _node,
    uint32_t             _last
)
{
    uint32_t below = 0u;

    for (uint32_t at = _node->next_sibling; at != D_DSS_NO_INDEX; )
    {
        const struct d_node* const sibling = d_node_at(_tree, at);

        if (!sibling)
        {
            break;
        }

        const uint32_t ends = d_internal_comment_lines(_tree, sibling,
                                                       _node->start_column);

        // what follows the comments: in the item's column, they are its
        if (ends == 0u)
        {
            below = (sibling->start_column == _node->start_column) ? 0u
                                                                   : below;

            break;
        }

        below = ends;
        at    = sibling->next_sibling;
    }

    return (below > _last) ? below : _last;
}

/*
d_internal_shift_from
  Turns `repair: shift` into an edit: the rule's own `start-column` says
where the node belongs, the node says where it is, and the tree says which
line the node ends on. Nothing is noted for a rule with no column to move
towards, or a node already in it. Nor for three kinds of node, each left
for a person:
  - one that is not an item. A body's brace has no last line of its own
    here, and moved alone would leave its partner where it was; it moves
    with the item that owns it.
  - a parameter. Where a parameter stands follows from how its function's
    head is laid out, which has repairs of its own.
  - one that is not a whole number of levels from its column. A level is
    four columns; what stands 25 from where its level puts it was put
    there to line up with something.
*/
static bool
d_internal_shift_from(
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    const struct d_node*      _node,
    const struct d_dss_rule*  _rule,
    struct d_repair_edit*     _out
)
{
    for (uint32_t which = 0u; which < _rule->declaration_count; ++which)
    {
        const struct d_dss_declaration* const declaration =
            d_dss_declaration_at(_sheet, _rule->first_declaration + which);
        const struct d_dss_value* const       column      =
            (declaration->value_count > 0u)
            ? d_dss_value_at(_sheet, declaration->first_value)
            : NULL;

        if ( (!column)                                             ||
             (column->kind != D_DSS_VALUE_NUMBER)                  ||
             (strcmp(d_dss_text(_sheet, declaration->property),
                     "start-column") != 0) )
        {
            continue;
        }

        const uint32_t last = d_internal_number_of(_tree, _node, "end-line");

        _out->kind       = D_REPAIR_SHIFT;
        _out->line       = _node->line;
        _out->column     = _node->start_column;
        _out->close_line = (last > _node->line) ? last : _node->line;
        _out->delta      = (int)column->number - (int)_node->start_column;
        _out->group      = _node->parent;
        _out->open_line  = d_internal_comments_above(_tree, _node);
        _out->close_line = d_internal_comments_below(_tree, _node,
                                                     _out->close_line);

        return ( (last != 0u)                                        &&
                 (!d_internal_is(_tree, _node, "param", NULL))       &&
                 (_out->delta != 0)                                  &&
                 ((_out->delta % 4) == 0) );
    }

    return false;
}

// d_internal_move
//   struct: a shift as it will be made: the lines that move, how far, the
// node whose children the item is one of, and whether the lines may move.
struct d_internal_move
{
    uint32_t  first;
    uint32_t  last;
    uint32_t  group;
    int       delta;
    bool      may;
};

/*
d_internal_gather_moves
  Lists a plan's shifts as they would be made, from the first line down,
and says of each whether its lines may move. An item that lies inside one
already listed is passed over: its lines move with the item around it, or
stay with it, and the next pass measures it again. `_moves` has room for
one move an edit; how many it was given is returned.
*/
static size_t
d_internal_gather_moves(
    const struct d_repair_plan* _plan,
    const char*                 _path,
    struct d_internal_move*     _moves
)
{
    size_t   count   = 0u;
    uint32_t covered = 0u;

    // the plan is in order last line first
    for (size_t at = _plan->count; at > 0u; --at)
    {
        const struct d_repair_edit* const edit = &_plan->edits[at - 1u];

        if ( (edit->kind != D_REPAIR_SHIFT) || (edit->line <= covered) )
        {
            continue;
        }

        covered = edit->close_line;

        struct d_internal_move* const move = &_moves[count];

        move->first = edit->open_line;
        move->last  = edit->close_line;
        move->group = edit->group;
        move->delta = edit->delta;
        move->may   =
            ( (d_internal_opens_line(_path, edit->line, edit->column))  &&
              (d_edit_may_shift_lines(_path, move->first, move->last,
                                      move->delta)) );
        ++count;
    }

    return count;
}

/*
d_internal_apply_shifts
  Makes a plan's shifts and nothing else: each item with the comments that
are its (d_internal_comments_above, d_internal_comments_below).
  An item whose lines cannot all move is left whole, and what is inside it
with it, so that no item is ever half moved. Its siblings that were to move
as far are left with it, so that what stood in one column still does: a
`case` label is not moved away from the one statement under it that has no
room to follow.
*/
static size_t
d_internal_apply_shifts(
    const struct d_repair_plan* _plan,
    const char*                 _path
)
{
    struct d_internal_move* const moves =
        malloc(_plan->count * sizeof(*moves));
    size_t                        done  = 0u;

    if (!moves)
    {
        return 0u;
    }

    const size_t count = d_internal_gather_moves(_plan, _path, moves);

    for (size_t at = 0u; at < count; ++at)
    {
        bool held = (!moves[at].may);

        // one that may not move holds back its siblings that were to move
        // as far
        for (size_t other = 0u; (!held) && (other < count); ++other)
        {
            held = ( (!moves[other].may)                        &&
                     (moves[other].group == moves[at].group)    &&
                     (moves[other].delta == moves[at].delta) );
        }

        if ( (!held) &&
             (d_edit_shift_lines(_path, moves[at].first, moves[at].last,
                                 moves[at].delta)) )
        {
            ++done;
        }
    }

    free(moves);

    return done;
}

/*
d_repair_plan_init
  Empty, with no storage yet.
*/
void
d_repair_plan_init(
    struct d_repair_plan* _plan
)
{
    _plan->edits    = NULL;
    _plan->count    = 0u;
    _plan->capacity = 0u;

    return;
}

/*
d_repair_plan_clear
  Forgets the edits and keeps the storage for the next file.
*/
void
d_repair_plan_clear(
    struct d_repair_plan* _plan
)
{
    _plan->count = 0u;

    return;
}

/*
d_repair_plan_free
  Releases the storage; the plan is empty again and may be reused.
*/
void
d_repair_plan_free(
    struct d_repair_plan* _plan
)
{
    free(_plan->edits);
    d_repair_plan_init(_plan);

    return;
}

/*
d_repair_stated
  The rule's own `repair` declaration is there and says something.
*/
bool
d_repair_stated(
    const struct d_dss_sheet* _sheet,
    const struct d_dss_rule*  _rule
)
{
    const struct d_dss_declaration* const stated = d_internal_stated(_sheet,
                                                                     _rule);

    return ( (stated != NULL) && (stated->value_count > 0u) );
}

/*
d_repair_plan_note
  Finds the rule's `repair`, has it turned into an edit, and keeps the edit,
growing the plan's storage by doubling when it is full.
*/
bool
d_repair_plan_note(
    struct d_repair_plan*     _plan,
    const struct d_dss_sheet* _sheet,
    struct d_node_tree*       _tree,
    uint32_t                  _node,
    const struct d_dss_rule*  _rule
)
{
    const struct d_node* const            node   = d_node_at(_tree, _node);
    const struct d_dss_declaration* const stated = d_internal_stated(_sheet,
                                                                     _rule);
    struct d_repair_edit                  edit   = { 0 };

    if ( (!node) || (node->line == 0u) || (!stated) ||
         (stated->value_count == 0u) )
    {
        return false;
    }

    const struct d_dss_value* const how = d_dss_value_at(_sheet,
                                                         stated->first_value);

    // a shift is told by the rule's own column; the rest by their words
    if ( (how) && (how->kind == D_DSS_VALUE_IDENT) &&
         (strcmp(d_dss_text(_sheet, how->text), "shift") == 0) )
    {
        if (!d_internal_shift_from(_sheet, _tree, node, _rule, &edit))
        {
            return false;
        }
    }
    else if (!d_internal_edit_from(_sheet, _tree, node, stated, &edit))
    {
        return false;
    }

    // room for one more
    if (_plan->count == _plan->capacity)
    {
        const size_t                capacity = (_plan->capacity == 0u)
                                             ? 64u
                                             : (_plan->capacity * 2u);
        struct d_repair_edit* const grown    =
            realloc(_plan->edits, capacity * sizeof(*grown));

        if (!grown)
        {
            return false;
        }

        _plan->edits    = grown;
        _plan->capacity = capacity;
    }

    _plan->edits[_plan->count] = edit;
    ++_plan->count;

    return true;
}

/*
d_internal_apply_one
  Makes one edit of a kind that is made beside others, and reports how many
lines it changed.
*/
static size_t
d_internal_apply_one(
    const char*                 _path,
    const struct d_repair_edit* _edit,
    size_t*                     _refused
)
{
    if (_edit->kind == D_REPAIR_BLANKS)
    {
        return d_internal_apply_blanks(_path, _edit);
    }

    if (_edit->kind == D_REPAIR_LAYOUT)
    {
        return d_internal_apply_layout(_path, _edit, _refused);
    }

    if (_edit->kind == D_REPAIR_JOIN)
    {
        return d_internal_apply_join(_path, _edit, _refused);
    }

    if (_edit->kind == D_REPAIR_EXPAND)
    {
        return d_internal_apply_expand(_path, _edit, _refused);
    }

    if (_edit->kind == D_REPAIR_COMMENT)
    {
        return d_internal_apply_comment(_path, _edit->line, _edit->text,
                                        (_edit->count != 0u), _refused);
    }

    return d_internal_apply_split(_path, _edit, _refused);
}

/*
d_internal_has_shift
  Whether a plan holds a shift.
*/
static bool
d_internal_has_shift(
    const struct d_repair_plan* _plan
)
{
    for (size_t at = 0u; at < _plan->count; ++at)
    {
        if (_plan->edits[at].kind == D_REPAIR_SHIFT)
        {
            return true;
        }
    }

    return false;
}

/*
d_repair_plan_apply
  Puts the edits in order and makes each, skipping a line dcheck has already
rewritten this pass and an edit that repeats the one before it: two rules may
ask the same thing of one line, and it is done once.
  A shift moves the columns every other edit was noted against, so a file
with any is given its shifts alone, and the rest are noted again on the next
pass, against the columns as they then stand. Where no shift could be made
nothing has moved, and the rest are made now: a file with a shift that can
never be made would otherwise have nothing else repaired, on any pass.
*/
size_t
d_repair_plan_apply(
    struct d_repair_plan* _plan,
    const char*           _path,
    const uint32_t*       _touched,
    size_t                _touched_count,
    size_t*               _refused
)
{
    size_t done = 0u;

    if (_plan->count == 0u)
    {
        return 0u;
    }

    qsort(_plan->edits, _plan->count, sizeof(*_plan->edits),
          d_internal_order);

    const size_t shifted = d_internal_has_shift(_plan)
                         ? d_internal_apply_shifts(_plan, _path)
                         : 0u;

    if (shifted > 0u)
    {
        return shifted;
    }

    for (size_t at = 0u; at < _plan->count; ++at)
    {
        const struct d_repair_edit* const edit = &_plan->edits[at];
        bool                              skip =
            ( (edit->kind == D_REPAIR_SHIFT)                 ||
              ( (at > 0u)                                    &&
                (_plan->edits[at - 1u].line == edit->line)   &&
                (_plan->edits[at - 1u].kind == edit->kind) ) );

        // a line rewritten this pass is measured again next pass
        for (size_t which = 0u; (!skip) && (which < _touched_count); ++which)
        {
            skip = (_touched[which] == edit->line);
        }

        if (skip)
        {
            continue;
        }

        done += d_internal_apply_one(_path, edit, _refused);
    }

    return done;
}
