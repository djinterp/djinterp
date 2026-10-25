/*******************************************************************************
* djinterp [tools]                                                   dmake_gnu.c
*
* Reads a makefile's statements as GNU make 4.3 reads them: see dmake.h.
*   Each logical line is taken in the order make's eval takes it. Within a
* `define`, a line is the body's unless it opens another `define` or closes
* one, nested ones counted. A line led by the recipe prefix while a rule is
* above it is a recipe's. Any other line loses its comment -- a `#` no
* backslash escapes, outside every reference -- and is tried as an
* assignment, its modifiers before it and its operator found as
* parse_variable_definition finds one; then as a conditional; then as one
* of the other directives; then as a rule, its first `;` or `#` ending what
* is read as targets and prerequisites, a `;` first leaving the rest to the
* recipe. A line with no rule's colon is an expansion. What ends a rule is
* what make's record_waiting_files ends one at: an assignment, a directive
* but a conditional, a rule or an expansion; a comment or an empty line
* does not. A literal `.RECIPEPREFIX` sets the prefix from the next line.
*
* path:      /src/djinterp/tools/dawk/ext/make/dmake_gnu.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "./dmake_internal.h"  // corresponding header
// std
#include <stdlib.h>            // realloc, free
#include <string.h>            // memset


//==============================================================================
// 1.  INTERNAL TYPES
//==============================================================================


// 1.1    The reader
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_open
//   struct: a conditional open: its last branch read, and whether a bare
//   `else` was among them.
struct d_internal_open
{
    size_t branch;
    bool   seen_else;
};

// 1.1.2
// d_internal_gnu
//   struct: where make's eval stands: the recipe prefix, whether a rule is
//   above, the conditionals open, and the `define` being read -- its
//   statement, how many defines deep its body is, where its body starts.
struct d_internal_gnu
{
    struct d_make_reader*   reader;
    const char*             text;
    char                    prefix;
    bool                    in_rule;
    struct d_internal_open* opens;
    size_t                  open_count;
    size_t                  open_capacity;
    size_t                  define;
    size_t                  define_depth;
    size_t                  body_start;
};

// 1.1.3
// d_internal_found
//   enum: what a line was tried as.
enum d_internal_found
{
    D_INTERNAL_NOTHING = 0,
    D_INTERNAL_ASSIGNED,
    D_INTERNAL_DEFINED,
    D_INTERNAL_UNDEFINED
};

// 1.1.4
// d_internal_parts
//   struct: the parts of an assignment, a define or an undefine found.
struct d_internal_parts
{
    struct d_make_span modifiers;
    struct d_make_span word;
    struct d_make_span name;
    struct d_make_span separator;
    struct d_make_span value;
};


//==============================================================================
// 2.  ASSIGNMENTS
//==============================================================================


/**
 * @brief Finds an assignment's operator as GNU's parse_variable_definition
 *        finds one.
 *
 * @param[in]  _text      the text.
 * @param[in]  _at        where the name starts.
 * @param[in]  _end       where the line's code ends.
 * @param[out] _name_end  receives where the name ends: at the first blank
 *                        before the operator, or at the operator.
 * @param[out] _operator  receives the operator.
 * @return `true` if the text is an assignment.
 *
 * A reference's brackets are stepped over, so `$(a:=b)` is no operator; a
 * blank must be followed by the operator, so `a b = c` is no assignment;
 * `#` or the end first means none. `:::=` is GNU make 4.4's, and 4.3 reads
 * it as no operator at all.
 *
 * Long as parse_variable_definition's one loop, its cases kept in its order,
 * as each decides what the next may be.
 */
static bool
d_internal_operator(
    const char*         _text,
    size_t              _at,
    size_t              _end,
    size_t*             _name_end,
    struct d_make_span* _operator
)
{
    size_t at       = _at;
    size_t name_end = D_MAKE_NONE;
    bool   blank    = false;

    // each byte, until the operator or what rules one out
    while (at < _end)
    {
        const size_t skipped = d_make_skip_blanks(_text, at, _end);

        // blanks, which only an operator may follow
        if (skipped > at)
        {
            name_end = (name_end == D_MAKE_NONE) ? at : name_end;
            blank    = true;
            at       = skipped;

            continue;
        }

        const char byte = _text[at];
        const char next = (at + 1u < _end) ? _text[at + 1u] : '\0';

        // a table of the bytes that decide: a reference, `#`, `=`, the
        // two-byte operators, `::=`, and anything else after a blank
        if (byte == '$')
        {
            at = d_make_reference_end(_text, at, _end);

            continue;
        }
        else if (byte == '#')
        {
            return false;
        }
        else if (byte == '=')
        {
            *_name_end = (name_end == D_MAKE_NONE) ? at : name_end;
            _operator->start = at;
            _operator->end   = at + 1u;

            return true;
        }
        else if ( (next == '=') &&
                  ( (byte == ':') ||
                    (byte == '+') ||
                    (byte == '?') ||
                    (byte == '!') ) )
        {
            *_name_end = (name_end == D_MAKE_NONE) ? at : name_end;
            _operator->start = at;
            _operator->end   = at + 2u;

            return true;
        }
        else if (byte == ':')
        {
            // only `::=` of the colons is an operator
            if ( (next != ':') ||
                 (at + 2u >= _end) ||
                 (_text[at + 2u] != '=') )
            {
                return false;
            }

            *_name_end = (name_end == D_MAKE_NONE) ? at : name_end;
            _operator->start = at;
            _operator->end   = at + 3u;

            return true;
        }
        else if (blank)
        {
            return false;
        }

        at++;
    }

    return false;
}


/**
 * @brief Tries a line as GNU's parse_var_assignment does: modifiers, then
 *        an assignment, or, outside a rule, `define` or `undefine`.
 *
 * @param[in]  _text       the text.
 * @param[in]  _at         where the line's first word starts.
 * @param[in]  _end        where its code ends, before any comment.
 * @param[in]  _for_rule   whether it follows a rule's colon, where neither
 *                         `define` nor `undefine` is read.
 * @param[out] _out        receives the parts found.
 * @return what the line is.
 *
 * GNU make 4.3's modifiers are override, export and private; `unexport` is
 * a directive of its own there, and no modifier.
 */
static enum d_internal_found
d_internal_assignment(
    const char*              _text,
    size_t                   _at,
    size_t                   _end,
    bool                     _for_rule,
    struct d_internal_parts* _out
)
{
    size_t at = _at;

    memset(_out, 0, sizeof(*_out));
    _out->modifiers.start = _at;
    _out->modifiers.end   = _at;

    // each word, while it is a modifier
    while (at < _end)
    {
        size_t name_end = 0;

        // an assignment, its name here
        if (d_internal_operator(_text, at, _end, &name_end, &_out->separator))
        {
            _out->modifiers.end = d_make_trim_end(_text, _at, at);
            _out->name.start    = at;
            _out->name.end      = name_end;
            _out->value.start   = d_make_skip_blanks(_text,
                                                     _out->separator.end,
                                                     _end);
            _out->value.end     = d_make_trim_end(_text,
                                                  _out->value.start,
                                                  _end);

            return D_INTERNAL_ASSIGNED;
        }

        const size_t word = d_make_word_end(_text, at, _end);
        const size_t next = d_make_skip_blanks(_text, word, _end);

        // a table of the words: the modifiers, define and undefine
        if ( (d_make_is_word(_text, at, word, "export")) ||
             (d_make_is_word(_text, at, word, "override")) ||
             (d_make_is_word(_text, at, word, "private")) )
        {
            at = next;

            continue;
        }
        else if ( (!_for_rule) &&
                  ( (d_make_is_word(_text, at, word, "define")) ||
                    (d_make_is_word(_text, at, word, "undefine")) ) )
        {
            _out->modifiers.end = d_make_trim_end(_text, _at, at);
            _out->word.start    = at;
            _out->word.end      = word;
            _out->value.start   = next;
            _out->value.end     = _end;

            return (_text[at] == 'd') ? D_INTERNAL_DEFINED
                                      : D_INTERNAL_UNDEFINED;
        }

        return D_INTERNAL_NOTHING;
    }

    return D_INTERNAL_NOTHING;
}


/**
 * @brief Records an assignment found, and the prefix it may set.
 *
 * @param[in,out] _gnu    the reader.
 * @param[in]     _line   the line.
 * @param[in]     _parts  the assignment's parts.
 * @return the statement's index, or D_MAKE_NONE when memory runs out.
 *
 * `.RECIPEPREFIX` set to a text with no reference sets the prefix as make
 * sets it: the text's first byte, or a tab for an empty text.
 */
static size_t
d_internal_assigned(
    struct d_internal_gnu*         _gnu,
    const struct d_make_line*      _line,
    const struct d_internal_parts* _parts
)
{
    const size_t index = d_make_reader_statement(_gnu->reader,
                                                 D_MAKE_ASSIGNMENT,
                                                 _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return index;
    }

    struct d_make_statement* const statement =
        &_gnu->reader->make->statements[index];

    statement->modifiers = _parts->modifiers;
    statement->name      = _parts->name;
    statement->separator = _parts->separator;
    statement->text      = _parts->value;

    const struct d_make_span value = _parts->value;
    const size_t             op    = _parts->separator.end -
                                     _parts->separator.start;

    // a literal prefix set
    if ( (d_make_is_word(_gnu->text,
                         _parts->name.start,
                         _parts->name.end,
                         ".RECIPEPREFIX")) &&
         (_gnu->text[_parts->separator.end - 1u] == '=') &&
         ( (op == 1u) ||
           (_gnu->text[_parts->separator.start] == ':') ) &&
         (d_make_find(_gnu->text, value.start, value.end, "$") == value.end) )
    {
        _gnu->prefix = (value.start < value.end) ? _gnu->text[value.start]
                                                 : '\t';
    }

    _gnu->in_rule = false;

    return index;
}


//==============================================================================
// 3.  DEFINES
//==============================================================================


/**
 * @brief Records a `define` line, and begins reading its body.
 *
 * @param[in,out] _gnu    the reader.
 * @param[in]     _line   the line.
 * @param[in]     _parts  the define's parts; `value` is what follows its
 *                        word.
 * @return the statement's index, or D_MAKE_NONE when memory runs out.
 *
 * What follows the word is the name, and the operator if there is one, as
 * do_define reads them; text after the operator is make's error to report,
 * and dmake_eval's.
 */
static size_t
d_internal_define(
    struct d_internal_gnu*         _gnu,
    const struct d_make_line*      _line,
    const struct d_internal_parts* _parts
)
{
    const size_t index = d_make_reader_statement(_gnu->reader,
                                                 D_MAKE_DEFINE,
                                                 _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return index;
    }

    struct d_make_statement* const statement =
        &_gnu->reader->make->statements[index];
    const struct d_make_span       rest      = _parts->value;
    size_t                         name_end  = rest.end;

    statement->modifiers = _parts->modifiers;
    statement->word      = _parts->word;

    // an operator after the name
    if (!d_internal_operator(_gnu->text,
                             rest.start,
                             rest.end,
                             &name_end,
                             &statement->separator))
    {
        statement->separator.start = d_make_trim_end(_gnu->text,
                                                     rest.start,
                                                     rest.end);
        statement->separator.end   = statement->separator.start;
        name_end                   = statement->separator.start;
    }

    statement->name.start = rest.start;
    statement->name.end   = d_make_trim_end(_gnu->text, rest.start, name_end);
    statement->body.start = _line->next;
    statement->body.end   = _line->next;

    _gnu->define       = index;
    _gnu->define_depth = 1;
    _gnu->body_start   = _line->next;
    _gnu->in_rule      = false;

    return index;
}


/**
 * @brief Takes a line within a `define`: the body's, or a nested define's
 *        opening or closing, or the `endef` that ends it.
 *
 * @param[in,out] _gnu   the reader.
 * @param[in]     _line  the line.
 * @return `true`; `false` when memory runs out.
 *
 * As do_define reads one: a line led by the recipe prefix is the body's;
 * another opens a define when its first word is `define` and a blank or the
 * line's end follows it, and closes one so for `endef`. A comment after the
 * last `endef` is its own.
 */
static bool
d_internal_define_line(
    struct d_internal_gnu*    _gnu,
    const struct d_make_line* _line
)
{
    const char* const text = _gnu->text;

    // a line led by the prefix is the body's
    if ( (_line->start < _line->end) &&
         (text[_line->start] == _gnu->prefix) )
    {
        return true;
    }

    const size_t at   = d_make_skip_blanks(text, _line->start, _line->end);
    const size_t word = d_make_word_end(text, at, _line->end);

    // a table of the words that count: define, and endef
    if (d_make_is_word(text, at, word, "define"))
    {
        _gnu->define_depth++;
    }
    else if (d_make_is_word(text, at, word, "endef"))
    {
        _gnu->define_depth--;
    }

    // the body goes on
    if (_gnu->define_depth > 0u)
    {
        return true;
    }

    struct d_make_statement* const statement =
        &_gnu->reader->make->statements[_gnu->define];
    const size_t                   define    = _gnu->define;

    statement->body.start = _gnu->body_start;
    statement->body.end   = _line->start;
    statement->end        = _line->next;
    statement->end_line   = _line->end_line;
    _gnu->define          = D_MAKE_NONE;

    const size_t comment = d_make_find(text, word, _line->end, "#");

    // a comment after the endef
    if (comment < _line->end)
    {
        return d_make_reader_comment(_gnu->reader,
                                     comment,
                                     _line->end,
                                     define);
    }

    return true;
}


//==============================================================================
// 4.  CONDITIONALS
//==============================================================================


/**
 * @brief Makes room for one more conditional open.
 *
 * @param[in,out] _gnu  the reader.
 * @return `true` if there is room; `false` when memory runs out.
 */
static bool
d_internal_open_room(
    struct d_internal_gnu* _gnu
)
{
    // room already
    if (_gnu->open_count < _gnu->open_capacity)
    {
        return true;
    }

    const size_t wanted = (_gnu->open_capacity > 0u)
                              ? (_gnu->open_capacity * 2u)
                              : 8u;
    void* const  grown  = realloc(_gnu->opens, wanted * sizeof(*_gnu->opens));

    // memory that ran out
    if (!grown)
    {
        return false;
    }

    _gnu->opens         = grown;
    _gnu->open_capacity = wanted;

    return true;
}


/**
 * @brief Reports whether a word is one that opens a conditional.
 *
 * @param[in] _text   the text.
 * @param[in] _start  the word's start.
 * @param[in] _end    its end.
 * @return `true` for ifdef, ifndef, ifeq and ifneq.
 */
static bool
d_internal_is_if(
    const char* _text,
    size_t      _start,
    size_t      _end
)
{
    return ( (d_make_is_word(_text, _start, _end, "ifdef"))  ||
             (d_make_is_word(_text, _start, _end, "ifndef")) ||
             (d_make_is_word(_text, _start, _end, "ifeq"))   ||
             (d_make_is_word(_text, _start, _end, "ifneq")) );
}


/**
 * @brief Takes a conditional's line, as GNU's conditional_line counts them.
 *
 * @param[in,out] _gnu    the reader.
 * @param[in]     _index  the statement, already recorded.
 * @return `true`; `false` when make refuses it, in make's words, or memory
 *         runs out.
 *
 * Make counts its conditionals in every branch, the false ones too, so
 * what it refuses here it refuses whatever the variables hold: an `else`
 * or an `endif` with none open, and an `else` after a bare one. An `else`
 * followed by text sets no bare else, as conditional_line has it, whether
 * the text is a conditional or not.
 *
 * Long as counting's two halves, opening one and continuing one, kept
 * together as conditional_line keeps them.
 */
static bool
d_internal_conditional(
    struct d_internal_gnu* _gnu,
    size_t                 _index
)
{
    struct d_make_statement* const statement =
        &_gnu->reader->make->statements[_index];
    const struct d_make_span       word      = statement->word;
    const char* const              text      = _gnu->text;

    // an if opens one
    if (d_internal_is_if(text, word.start, word.end))
    {
        // room for it
        if (!d_internal_open_room(_gnu))
        {
            return d_make_reader_fail(_gnu->reader,
                                      statement->line,
                                      "out of memory");
        }

        statement->depth = _gnu->open_count;
        _gnu->opens[_gnu->open_count].branch    = _index;
        _gnu->opens[_gnu->open_count].seen_else = false;
        _gnu->open_count++;

        return true;
    }

    const bool is_else = d_make_is_word(text, word.start, word.end, "else");

    // an else or an endif with none open
    if (_gnu->open_count == 0u)
    {
        return d_make_reader_fail(_gnu->reader,
                                  statement->line,
                                  is_else ? "extraneous 'else'"
                                          : "extraneous 'endif'");
    }

    struct d_internal_open* const open = &_gnu->opens[_gnu->open_count - 1u];

    // an else after a bare else
    if ( (is_else) &&
         (open->seen_else) )
    {
        return d_make_reader_fail(_gnu->reader,
                                  statement->line,
                                  "only one 'else' per conditional");
    }

    statement->depth = _gnu->open_count - 1u;
    _gnu->reader->make->statements[open->branch].next_branch = _index;
    open->branch = _index;

    // a table of the two: an else stays open, a bare one marked; an endif
    // closes
    if (is_else)
    {
        open->seen_else = (statement->text.start == statement->text.end);
    }
    else
    {
        _gnu->open_count--;
    }

    return true;
}


//==============================================================================
// 5.  RULES
//==============================================================================


/**
 * @brief Records a recipe's line.
 *
 * @param[in,out] _gnu     the reader.
 * @param[in]     _line    the line.
 * @param[in]     _orphan  whether no rule is above it.
 * @return `true`; `false` when memory runs out.
 *
 * Its word is the prefix and the run of `@`, `-`, `+` and blanks make takes
 * as the line's flags; the rest is its command, comments and all, which are
 * the shell's.
 */
static bool
d_internal_recipe(
    struct d_internal_gnu*    _gnu,
    const struct d_make_line* _line,
    bool                      _orphan
)
{
    const size_t index = d_make_reader_statement(_gnu->reader,
                                                 D_MAKE_RECIPE,
                                                 _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    const char* const text  = _gnu->text;
    size_t            flags = _line->start + 1u;

    // the flags after the prefix
    while ( (flags < _line->end) &&
            ( (text[flags] == '@') ||
              (text[flags] == '-') ||
              (text[flags] == '+') ||
              (text[flags] == ' ') ||
              (text[flags] == '\t') ) )
    {
        flags++;
    }

    struct d_make_statement* const statement =
        &_gnu->reader->make->statements[index];

    statement->word.start = _line->start;
    statement->word.end   = flags;
    statement->text.start = flags;
    statement->text.end   = _line->end;
    statement->orphan     = _orphan;

    return true;
}


/**
 * @brief Records a rule's parts after its colon: a static pattern, the
 *        prerequisites, the order-only ones after `|`, and the recipe.
 *
 * @param[in,out] _statement  the rule, its targets and colon set.
 * @param[in]     _text       the text.
 * @param[in]     _stop       where its first `;` or `#` stands, or its end.
 * @param[in]     _line       the line.
 *
 * As eval reads one once expanded: a second colon after the first makes a
 * static pattern of what is between; the first `|` after that begins the
 * order-only prerequisites; a `;` first leaves the rest of the line to the
 * recipe.
 */
static void
d_internal_rule_parts(
    struct d_make_statement*  _statement,
    const char*               _text,
    size_t                    _stop,
    const struct d_make_line* _line
)
{
    const size_t after = _statement->colon.end;
    const size_t colon = d_make_find(_text, after, _stop, ":");
    size_t       from  = after;

    // a static pattern
    if (colon < _stop)
    {
        _statement->pattern.start = d_make_skip_blanks(_text, after, colon);
        _statement->pattern.end   = d_make_trim_end(_text,
                                                    _statement->pattern.start,
                                                    colon);
        from                      = colon + 1u;
    }
    else
    {
        _statement->pattern.start = after;
        _statement->pattern.end   = after;
    }

    const size_t bar   = d_make_find(_text, from, _stop, "|");
    const size_t start = d_make_skip_blanks(_text, from, bar);

    _statement->text.start = start;
    _statement->text.end   = d_make_trim_end(_text, start, bar);

    // order-only prerequisites
    if (bar < _stop)
    {
        _statement->order_only.start = d_make_skip_blanks(_text,
                                                          bar + 1u,
                                                          _stop);
        _statement->order_only.end   = d_make_trim_end(
                                           _text,
                                           _statement->order_only.start,
                                           _stop);
    }
    else
    {
        _statement->order_only.start = _statement->text.end;
        _statement->order_only.end   = _statement->text.end;
    }

    // a recipe after a `;`
    if ( (_stop < _line->end) &&
         (_text[_stop] == ';') )
    {
        _statement->recipe.start = _stop + 1u;
        _statement->recipe.end   = _line->end;
    }
    else
    {
        _statement->recipe.start = _line->end;
        _statement->recipe.end   = _line->end;
    }

    return;
}


/**
 * @brief Records the comment that ends a rule's line, if its first stop is
 *        a `#`.
 *
 * @param[in,out] _gnu    the reader.
 * @param[in]     _line   the line.
 * @param[in]     _stop   where its first `;` or `#` stands, or its end.
 * @param[in]     _index  the statement the line holds.
 * @return `true`; `false` when memory runs out.
 */
static bool
d_internal_rule_comment(
    struct d_internal_gnu*    _gnu,
    const struct d_make_line* _line,
    size_t                    _stop,
    size_t                    _index
)
{
    // no comment, or a `;` first, after which a `#` is the recipe's
    if ( (_stop == _line->end) ||
         (_gnu->text[_stop] != '#') )
    {
        return true;
    }

    return d_make_reader_comment(_gnu->reader,
                                 _stop,
                                 _line->end,
                                 _index);
}


/**
 * @brief Records a line read as a rule, an assignment for targets, or an
 *        expansion.
 *
 * @param[in,out] _gnu   the reader.
 * @param[in]     _line  the line.
 * @param[in]     _at    where its first word starts.
 * @return `true`; `false` when memory runs out.
 *
 * Long as one reading: the stop that ends the targets, the colon and its
 * kind, then the assignment for targets or the rule's parts, each place
 * a step of the one before.
 */
static bool
d_internal_rule(
    struct d_internal_gnu*    _gnu,
    const struct d_make_line* _line,
    size_t                    _at
)
{
    const char* const text  = _gnu->text;
    const size_t      stop  = d_make_find(text, _at, _line->end, ";#");
    const size_t      colon = d_make_find(text, _at, stop, ":");
    const size_t      code  = ( (stop < _line->end) &&
                                (text[stop] == ';') ) ? _line->end : stop;

    // no colon: what the line is, only its expansion says
    if (colon == stop)
    {
        const size_t index = d_make_reader_statement(_gnu->reader,
                                                     D_MAKE_EXPANSION,
                                                     _line);

        // memory that ran out
        if (index == D_MAKE_NONE)
        {
            return false;
        }

        _gnu->reader->make->statements[index].text.start = _at;
        _gnu->reader->make->statements[index].text.end   =
            d_make_trim_end(text, _at, code);
        _gnu->in_rule = true;

        return d_internal_rule_comment(_gnu, _line, stop, index);
    }

    const bool   grouped = ( (colon > _at) &&
                             (text[colon - 1u] == '&') );
    const size_t first   = grouped ? (colon - 1u) : colon;
    const size_t last    = ( (colon + 1u < stop) &&
                             (text[colon + 1u] == ':') ) ? (colon + 2u)
                                                         : (colon + 1u);

    struct d_internal_parts parts;
    const bool              assigns =
        (d_internal_assignment(text,
                               d_make_skip_blanks(text, last, stop),
                               stop,
                               true,
                               &parts) == D_INTERNAL_ASSIGNED);

    // the operator stands before the `;`, and the value runs past it
    if (assigns)
    {
        parts.value.end = d_make_trim_end(text, parts.value.start, code);
    }
    const size_t            index   = d_make_reader_statement(
                                          _gnu->reader,
                                          assigns ? D_MAKE_ASSIGNMENT
                                                  : D_MAKE_RULE,
                                          _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    struct d_make_statement* const statement =
        &_gnu->reader->make->statements[index];

    statement->targets.start = _at;
    statement->targets.end   = d_make_trim_end(text, _at, first);
    statement->colon.start   = first;
    statement->colon.end     = last;

    // a table of the two: an assignment for the targets, or a rule
    if (assigns)
    {
        statement->modifiers = parts.modifiers;
        statement->name      = parts.name;
        statement->separator = parts.separator;
        statement->text      = parts.value;
        _gnu->in_rule        = false;
    }
    else
    {
        d_internal_rule_parts(statement, text, stop, _line);
        _gnu->in_rule = true;
    }

    return d_internal_rule_comment(_gnu, _line, stop, index);
}


//==============================================================================
// 6.  LINES
//==============================================================================


/**
 * @brief Records a directive: a conditional, an include, or another.
 *
 * @param[in,out] _gnu   the reader.
 * @param[in]     _line  the line.
 * @param[in]     _kind  the directive's kind.
 * @param[in]     _word  its word.
 * @param[in]     _code  where the line's code ends.
 * @return the statement's index, or D_MAKE_NONE when memory runs out.
 */
static size_t
d_internal_directive(
    struct d_internal_gnu*    _gnu,
    const struct d_make_line* _line,
    enum d_make_kind          _kind,
    struct d_make_span        _word,
    size_t                    _code
)
{
    const size_t index = d_make_reader_statement(_gnu->reader,
                                                 _kind,
                                                 _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return index;
    }

    struct d_make_statement* const statement =
        &_gnu->reader->make->statements[index];
    const size_t                   argument  = d_make_skip_blanks(_gnu->text,
                                                                  _word.end,
                                                                  _code);

    statement->word       = _word;
    statement->text.start = argument;
    statement->text.end   = d_make_trim_end(_gnu->text, argument, _code);

    return index;
}


/**
 * @brief Reports what kind of directive a line's first word makes it.
 *
 * @param[in]  _text   the text.
 * @param[in]  _word   the word.
 * @param[out] _out    receives the kind.
 * @return `true` if the word is a directive's, as eval tries them after an
 *         assignment: the conditionals, export, unexport, vpath, the
 *         includes, and load.
 */
static bool
d_internal_directive_kind(
    const char*        _text,
    struct d_make_span _word,
    enum d_make_kind*  _out
)
{
    const size_t start = _word.start;
    const size_t end   = _word.end;

    // a table of the directives, by kind
    if ( (d_internal_is_if(_text, start, end)) ||
         (d_make_is_word(_text, start, end, "else")) ||
         (d_make_is_word(_text, start, end, "endif")) )
    {
        *_out = D_MAKE_CONDITIONAL;
    }
    else if ( (d_make_is_word(_text, start, end, "include")) ||
              (d_make_is_word(_text, start, end, "-include")) ||
              (d_make_is_word(_text, start, end, "sinclude")) )
    {
        *_out = D_MAKE_INCLUDE;
    }
    else if ( (d_make_is_word(_text, start, end, "export")) ||
              (d_make_is_word(_text, start, end, "unexport")) ||
              (d_make_is_word(_text, start, end, "vpath")) ||
              (d_make_is_word(_text, start, end, "load")) ||
              (d_make_is_word(_text, start, end, "-load")) )
    {
        *_out = D_MAKE_DIRECTIVE;
    }
    else
    {
        return false;
    }

    return true;
}


/**
 * @brief Records what an assignment's try found: an assignment, a define or
 *        an undefine.
 *
 * @param[in,out] _gnu    the reader.
 * @param[in]     _line   the line.
 * @param[in]     _found  what was found.
 * @param[in]     _parts  its parts.
 * @return the statement's index, or D_MAKE_NONE when memory runs out.
 */
static size_t
d_internal_found_assignment(
    struct d_internal_gnu*         _gnu,
    const struct d_make_line*      _line,
    enum d_internal_found          _found,
    const struct d_internal_parts* _parts
)
{
    // a table of the three
    if (_found == D_INTERNAL_ASSIGNED)
    {
        return d_internal_assigned(_gnu, _line, _parts);
    }
    else if (_found == D_INTERNAL_DEFINED)
    {
        return d_internal_define(_gnu, _line, _parts);
    }

    const size_t index = d_make_reader_statement(_gnu->reader,
                                                 D_MAKE_DIRECTIVE,
                                                 _line);

    // memory that ran out
    if (index == D_MAKE_NONE)
    {
        return index;
    }

    struct d_make_statement* const statement =
        &_gnu->reader->make->statements[index];

    statement->modifiers  = _parts->modifiers;
    statement->word       = _parts->word;
    statement->name.start = _parts->value.start;
    statement->name.end   = d_make_trim_end(_gnu->text,
                                            _parts->value.start,
                                            _parts->value.end);
    statement->text       = statement->name;
    _gnu->in_rule         = false;

    return index;
}


/**
 * @brief Takes a line that is not a recipe's, nor within a define.
 *
 * @param[in,out] _gnu   the reader.
 * @param[in]     _line  the line.
 * @return `true`; `false` when make refuses it or memory runs out.
 *
 * Long as eval's order of trials, each a few lines: the comment found, then
 * an assignment, a conditional, the other directives, and a rule, an
 * expansion, or a recipe's line with no rule above.
 */
static bool
d_internal_line(
    struct d_internal_gnu*    _gnu,
    const struct d_make_line* _line
)
{
    const char* const text    = _gnu->text;
    const size_t      comment = d_make_find(text,
                                            _line->start,
                                            _line->end,
                                            "#");
    const size_t      at      = d_make_skip_blanks(text,
                                                   _line->start,
                                                   comment);
    size_t            index   = D_MAKE_NONE;

    // nothing but blanks, and perhaps a comment
    if (at == comment)
    {
        return ( (comment == _line->end) ||
                 (d_make_reader_comment(_gnu->reader,
                                        comment,
                                        _line->end,
                                        D_MAKE_NONE)) );
    }

    struct d_internal_parts     parts;
    const enum d_internal_found found = d_internal_assignment(text,
                                                              at,
                                                              comment,
                                                              false,
                                                              &parts);
    const struct d_make_span    word  = { at,
                                          d_make_word_end(text, at, comment) };
    enum d_make_kind            kind  = D_MAKE_DIRECTIVE;

    // a table of eval's trials, in its order
    if (found != D_INTERNAL_NOTHING)
    {
        index = d_internal_found_assignment(_gnu, _line, found, &parts);
    }
    else if (d_internal_directive_kind(text, word, &kind))
    {
        index = d_internal_directive(_gnu, _line, kind, word, comment);

        // a conditional is counted; the others end a rule
        if ( (index != D_MAKE_NONE) &&
             (kind == D_MAKE_CONDITIONAL) &&
             (!d_internal_conditional(_gnu, index)) )
        {
            return false;
        }

        _gnu->in_rule = ( (_gnu->in_rule) &&
                          (kind == D_MAKE_CONDITIONAL) );
    }
    else if (text[_line->start] == _gnu->prefix)
    {
        return d_internal_recipe(_gnu, _line, true);
    }
    else
    {
        return d_internal_rule(_gnu, _line, at);
    }

    // memory that ran out, or a comment that ends the line
    if (index == D_MAKE_NONE)
    {
        return false;
    }

    return ( (comment == _line->end) ||
             (d_make_reader_comment(_gnu->reader,
                                    comment,
                                    _line->end,
                                    index)) );
}


/*
d_make_read_gnu
  Each line is taken by the reading that make's state gives it; at the
end, a define still open is refused at its line, and a conditional still
open one line past the file's last, where make names it.
*/
bool
d_make_read_gnu(
    struct d_make_reader* _reader
)
{
    struct d_internal_gnu gnu;

    memset(&gnu, 0, sizeof(gnu));
    gnu.reader = _reader;
    gnu.text   = _reader->make->text;
    gnu.prefix = '\t';
    gnu.define = D_MAKE_NONE;

    const struct d_make_lines* const lines = &_reader->make->lines;
    bool                             read  = true;

    // each logical line, while reading goes on
    for (size_t index = 0; (read) && (index < lines->count); index++)
    {
        const struct d_make_line* const line = &lines->lines[index];

        // a table of the readings: a define's body, a recipe's line, the rest
        if (gnu.define != D_MAKE_NONE)
        {
            read = d_internal_define_line(&gnu, line);
        }
        else if ( (gnu.in_rule) &&
                  (line->start < line->end) &&
                  (gnu.text[line->start] == gnu.prefix) )
        {
            read = d_internal_recipe(&gnu, line, false);
        }
        else
        {
            read = d_internal_line(&gnu, line);
        }
    }

    const size_t last = (lines->count > 0u)
                            ? lines->lines[lines->count - 1u].end_line
                            : 0u;

    // a table of what may still be open at the end, once all was read
    if ( (read) &&
         (gnu.define != D_MAKE_NONE) )
    {
        read = d_make_reader_fail(
                   _reader,
                   _reader->make->statements[gnu.define].line,
                   "missing 'endef', unterminated 'define'");
    }
    else if ( (read) &&
              (gnu.open_count > 0u) )
    {
        read = d_make_reader_fail(_reader, last + 1u, "missing 'endif'");
    }

    free(gnu.opens);

    return read;
}
