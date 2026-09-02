#include "../../../../../inc/djinterp/c/util/document/layout_parse.h"

#include <string.h>


/*
   THE RECURSION IS THE C++ FILE'S, UNCHANGED.  parse_block and
   parse_block_sequence are mutually recursive there and mutually recursive
   here; the forward declaration below is the C spelling of that file's
   "(mutual recursion: a block may contain a brace-delimited sequence of
   blocks)" comment.

   WHAT DID CHANGE IS WHERE FAILURE GOES.  The C++ side threads a
   parse_result<T>, which carries value-or-error in one return.  C has no such
   carrier that does not allocate, so every parser here returns int32_t (1 ok,
   0 not, matching layout_build.c) and writes its product through an out
   parameter, with the error filled in on the way out.  That is the same
   control flow with the carrier unrolled, not a different design.
*/
static int32_t parse_block_helper(struct d_layout_parse_state*   _state,
                                  const struct d_layout_grammar* _grammar,
                                  struct d_layout_arena*         _tree_arena,
                                  struct d_layout_parse_arena*   _parse_arena,
                                  struct d_cofree**              _out_node,
                                  struct d_layout_parse_error*   _error);


/*
fail_helper
  Fills the error struct and returns 0, so a failing path is one line at the
  call site rather than five.  A NULL _error is the caller declining the
  diagnostic, not an error in itself.

Parameter(s):
  _error:         where to record it; may be NULL.
  _status:        enum d_layout_parse_status.
  _offset:        where in the source.
  _message:       a static literal.
  _token:         a borrowed span into the source, or NULL.
  _token_length:  that span's length.
Return:
  always 0, so callers may `return fail_helper(...)`.
*/
static int32_t
fail_helper
(
    struct d_layout_parse_error* _error,
    int32_t                      _status,
    size_t                       _offset,
    const char*                  _message,
    const char*                  _token,
    size_t                       _token_length
)
{
    if (_error)
    {
        _error->status       = _status;
        _error->reserved     = 0u;
        _error->offset       = _offset;
        _error->message      = _message;
        _error->token        = _token;
        _error->token_length = _token_length;
    }

    return 0;
}

/*
keyword_equals_helper
  Compares a source SPAN against a NUL-terminated table keyword.

  The span is not NUL-terminated, so strcmp is unavailable and strncmp alone is
  wrong: strncmp("section", "sect", 4) is 0, which would match the keyword
  `sect` against the source text `section`.  Both the prefix and the length must
  agree, and forgetting the second half is the classic form of this bug.

Parameter(s):
  _span:         the source span.
  _span_length:  its length.
  _keyword:      a NUL-terminated table entry.
Return:
  1 when they are the same word, 0 otherwise.
*/
static int32_t
keyword_equals_helper
(
    const char* _span,
    size_t      _span_length,
    const char* _keyword
)
{
    size_t _i = 0;

    if ( (!_span) || (!_keyword) )
    {
        return 0;
    }

    while (_i < _span_length)
    {
        if ( (_keyword[_i] == '\0') ||
             (_keyword[_i] != _span[_i]) )
        {
            return 0;
        }

        ++_i;
    }

    // the span ran out; the keyword must have ended in the same place
    return (_keyword[_i] == '\0') ? 1 : 0;
}

/*
arena_copy_helper
  Copies _length bytes out of the source and NUL-terminates them in the arena's
  text region.

Parameter(s):
  _arena:   the parse arena.
  _begin:   the first byte to copy.
  _length:  how many.
Return:
  the NUL-terminated copy, or NULL when the region is full.
*/
static const char*
arena_copy_helper
(
    struct d_layout_parse_arena* _arena,
    const char*                  _begin,
    size_t                       _length
)
{
    char*  _out  = 0;
    size_t _need = _length + 1u;   // + terminator

    if ( (!_arena) || (!_arena->text) )
    {
        return 0;
    }

    // the addition above is the one that can wrap, so it is checked before use
    if (_need < _length)
    {
        return 0;
    }

    if (_need > (_arena->text_capacity - _arena->text_used))
    {
        return 0;
    }

    _out = _arena->text + _arena->text_used;

    if (_length > 0u)
    {
        memcpy(_out, _begin, _length);
    }

    _out[_length]      = '\0';
    _arena->text_used += _need;

    return _out;
}


// ===========================================================================
// I.     the cursor
// ===========================================================================

/*
d_layout_parse_state_init
  Binds the cursor to a borrowed source.

Parameter(s):
  _state:   the cursor to initialise.
  _data:    the source; borrowed, must outlive the tree.
  _length:  its length in bytes, not counting any terminator.
Return:
  nothing.  A NULL _state is ignored rather than trapped, matching
  d_layout_arena_rewind.
*/
void
d_layout_parse_state_init
(
    struct d_layout_parse_state* _state,
    const char*                  _data,
    size_t                       _length
)
{
    if (!_state)
    {
        return;
    }

    _state->data   = _data;
    _state->length = _data ? _length : 0u;
    _state->offset = 0u;

    return;
}

/*
d_layout_parse_remaining
  How many bytes are still available.

Parameter(s):
  _state:  the cursor.
Return:
  the count, saturating at 0 rather than wrapping when offset has passed length.
*/
size_t
d_layout_parse_remaining
(
    const struct d_layout_parse_state* _state
)
{
    if (!_state)
    {
        return 0u;
    }

    return (_state->offset < _state->length)
                ? (_state->length - _state->offset)
                : 0u;
}

/*
d_layout_parse_at_end
  Whether the cursor has consumed its input.

Parameter(s):
  _state:  the cursor.
Return:
  1 at end (and for a NULL cursor, which has nothing left by definition),
  0 otherwise.
*/
int32_t
d_layout_parse_at_end
(
    const struct d_layout_parse_state* _state
)
{
    if (!_state)
    {
        return 1;
    }

    return (_state->offset >= _state->length) ? 1 : 0;
}

/*
d_layout_parse_current
  A pointer to the current byte.

Parameter(s):
  _state:  the cursor.
Return:
  the pointer, or NULL at end -- the C++ side's contract, kept so that a caller
  porting between the two does not discover a different sentinel.
*/
const char*
d_layout_parse_current
(
    const struct d_layout_parse_state* _state
)
{
    if (d_layout_parse_at_end(_state))
    {
        return 0;
    }

    return _state->data + _state->offset;
}

/*
d_layout_parse_advance
  Moves the cursor forward.

  CLAMPS AT THE END, as the C++ side does.  An unclamped advance puts offset
  past length, at_end still reads true, and remaining() then computes a wrapped
  size_t -- which is why that side clamps and why this one must too rather than
  relying on every caller to bound its own step.

Parameter(s):
  _state:  the cursor.
  _count:  how far.
Return:
  nothing.
*/
void
d_layout_parse_advance
(
    struct d_layout_parse_state* _state,
    size_t                       _count
)
{
    if (!_state)
    {
        return;
    }

    _state->offset += _count;

    if (_state->offset > _state->length)
    {
        _state->offset = _state->length;
    }

    return;
}


// ===========================================================================
// II.    diagnostics
// ===========================================================================

/*
d_layout_parse_status_name
  The spelling of a status code, for a fixture's failure message.

Parameter(s):
  _status:  enum d_layout_parse_status.
Return:
  a static literal; "unknown" for anything out of range, never NULL.
*/
const char*
d_layout_parse_status_name
(
    int32_t _status
)
{
    switch (_status)
    {
        case D_LAYOUT_PARSE_SUCCESS:      return "success";
        case D_LAYOUT_PARSE_FAILURE:      return "failure";
        case D_LAYOUT_PARSE_END_OF_INPUT: return "end-of-input";
        case D_LAYOUT_PARSE_OVERFLOW:     return "overflow";
        case D_LAYOUT_PARSE_MALFORMED:    return "malformed";
        default:                          break;
    }

    return "unknown";
}


// ===========================================================================
// III.   the grammar
// ===========================================================================

/*
d_layout_grammar_block_for
  The block-construct row for a keyword span.

  FIRST MATCH WINS, and that is the C++ side's behaviour rather than a choice
  made here: its op_for returns on the first equal key.  A dialect with a
  duplicated keyword therefore gets the earlier row in both languages, which
  is worth having identical even though a dialect should not have one.

Parameter(s):
  _grammar:         the parse table.
  _keyword:         the source span.
  _keyword_length:  its length.
Return:
  the row, or NULL when the keyword is not a block construct.
*/
const struct d_layout_block_word*
d_layout_grammar_block_for
(
    const struct d_layout_grammar* _grammar,
    const char*                    _keyword,
    size_t                         _keyword_length
)
{
    size_t _i = 0;

    if ( (!_grammar) || (!_grammar->block_words) )
    {
        return 0;
    }

    for (_i = 0; _i < _grammar->block_count; ++_i)
    {
        if (keyword_equals_helper(_keyword,
                                  _keyword_length,
                                  _grammar->block_words[_i].keyword))
        {
            return &_grammar->block_words[_i];
        }
    }

    return 0;
}

/*
d_layout_grammar_leaf_for
  The content-leaf row for a keyword span.  First match wins, as above.

Parameter(s):
  _grammar:         the parse table.
  _keyword:         the source span.
  _keyword_length:  its length.
Return:
  the row, or NULL when the keyword is not a leaf construct.
*/
const struct d_layout_leaf_word*
d_layout_grammar_leaf_for
(
    const struct d_layout_grammar* _grammar,
    const char*                    _keyword,
    size_t                         _keyword_length
)
{
    size_t _i = 0;

    if ( (!_grammar) || (!_grammar->leaf_words) )
    {
        return 0;
    }

    for (_i = 0; _i < _grammar->leaf_count; ++_i)
    {
        if (keyword_equals_helper(_keyword,
                                  _keyword_length,
                                  _grammar->leaf_words[_i].keyword))
        {
            return &_grammar->leaf_words[_i];
        }
    }

    return 0;
}


// ===========================================================================
// IV.    the arena
// ===========================================================================

/*
d_layout_parse_arena_init
  Binds the two caller-owned regions.

  EITHER REGION MAY BE ZERO-SIZED, and that is usable rather than degenerate: a
  grammar with no attributes and no quoted arguments parses fine with no text
  region.  What is rejected is a non-zero capacity over a NULL pointer, which
  is the transposed-argument mistake rather than a deliberate choice.

Parameter(s):
  _arena:          the arena to initialise.
  _attrs:          storage for hint entries.
  _attr_capacity:  how many entries.
  _text:           storage for scanned tokens.
  _text_capacity:  how many bytes.
Return:
  1 when the arena is usable, 0 otherwise.
*/
int32_t
d_layout_parse_arena_init
(
    struct d_layout_parse_arena* _arena,
    struct d_doc_attr*           _attrs,
    size_t                       _attr_capacity,
    char*                        _text,
    size_t                       _text_capacity
)
{
    if (!_arena)
    {
        return 0;
    }

    if ( ((_attr_capacity > 0u) && (!_attrs)) ||
         ((_text_capacity > 0u) && (!_text)) )
    {
        return 0;
    }

    _arena->attrs         = _attrs;
    _arena->attr_capacity = _attr_capacity;
    _arena->attr_used     = 0u;

    _arena->text          = _text;
    _arena->text_capacity = _text_capacity;
    _arena->text_used     = 0u;

    return 1;
}

/*
d_layout_parse_arena_rewind
  Returns both regions to empty without touching their storage.

  REWINDING WHILE A TREE IS LIVE SILENTLY REWRITES ITS STRINGS.  The tree's
  atoms and its bags' entries point in here, so this has the same hazard
  d_layout_number_arena carries, for the same reason, and it is stated in both
  places rather than in whichever one the reader happens to open.

Parameter(s):
  _arena:  the arena to rewind.
Return:
  nothing.
*/
void
d_layout_parse_arena_rewind
(
    struct d_layout_parse_arena* _arena
)
{
    if (!_arena)
    {
        return;
    }

    _arena->attr_used = 0u;
    _arena->text_used = 0u;

    return;
}


// ===========================================================================
// VI.    scanning
// ===========================================================================

/*
d_layout_parse_is_space / d_layout_parse_is_ident
  The two character classes the DSL cares about.

  NOT isspace() AND NOT isalpha().  Those are locale-dependent and take an int
  whose value must be representable as unsigned char -- passing a plain char
  with the high bit set is undefined, which is a real hazard on a UTF-8 source.
  The DSL's classes are four characters and three ranges; spelling them out is
  both correct everywhere and the same set the C++ side spells out.

Parameter(s):
  _c:  the character.
Return:
  1 when in the class, 0 otherwise.
*/
int32_t
d_layout_parse_is_space
(
    char _c
)
{
    return ( (_c == ' ')  || (_c == '\t') ||
             (_c == '\n') || (_c == '\r') ) ? 1 : 0;
}

int32_t
d_layout_parse_is_ident
(
    char _c
)
{
    return ( ( (_c >= 'a') && (_c <= 'z') ) ||
             ( (_c >= 'A') && (_c <= 'Z') ) ||
             (_c == '_') ) ? 1 : 0;
}

/*
d_layout_parse_skip_spaces
  Consumes a run of whitespace.

Parameter(s):
  _state:  the cursor.
Return:
  nothing.
*/
void
d_layout_parse_skip_spaces
(
    struct d_layout_parse_state* _state
)
{
    while (!d_layout_parse_at_end(_state))
    {
        if (!d_layout_parse_is_space(_state->data[_state->offset]))
        {
            break;
        }

        d_layout_parse_advance(_state, 1u);
    }

    return;
}

/*
d_layout_parse_scan_identifier
  A run of identifier characters after leading whitespace.

  RETURNS A SPAN, NOT A COPY.  A keyword is compared against the grammar table
  and then discarded, so copying it into the arena would consume text budget
  for something never stored.  Attribute KEYS are the exception -- they do get
  stored -- and scan_attributes_helper copies those itself, at the point it
  learns the identifier was a key rather than the next construct's keyword.

Parameter(s):
  _state:      the cursor.
  _out_begin:  set to the run's first character; may be NULL.
Return:
  the run's length; 0 when no identifier is present.
*/
size_t
d_layout_parse_scan_identifier
(
    struct d_layout_parse_state* _state,
    const char**                 _out_begin
)
{
    size_t _start = 0;

    d_layout_parse_skip_spaces(_state);

    if (!_state)
    {
        return 0u;
    }

    _start = _state->offset;

    while (!d_layout_parse_at_end(_state))
    {
        if (!d_layout_parse_is_ident(_state->data[_state->offset]))
        {
            break;
        }

        d_layout_parse_advance(_state, 1u);
    }

    if (_out_begin)
    {
        *_out_begin = _state->data + _start;
    }

    return _state->offset - _start;
}

/*
d_layout_parse_scan_string
  A "double-quoted" literal after leading whitespace, copied into the arena.

  NO ESCAPE HANDLING, AND THAT IS THE C++ SIDE'S BEHAVIOUR RATHER THAN AN
  OMISSION.  Its loop runs to the next '"' with no backslash case, so `\"` ends
  the literal there and would end it here.  Adding escapes to this side only
  would make the two parsers accept different languages, which is a worse
  outcome than a shared limitation; it belongs in both files or neither.

Parameter(s):
  _state:        the cursor.
  _arena:        where the copy lands.
  _out_status:   why it failed, when it did; may be NULL.
Return:
  the NUL-terminated body, or NULL.  NULL with status SUCCESS means no quote
  opened and NOTHING WAS CONSUMED, which is how the caller distinguishes an
  absent optional argument from a malformed one.
*/
const char*
d_layout_parse_scan_string
(
    struct d_layout_parse_state* _state,
    struct d_layout_parse_arena* _arena,
    int32_t*                     _out_status
)
{
    size_t      _start = 0;
    size_t      _stop  = 0;
    const char* _out   = 0;

    if (_out_status)
    {
        *_out_status = D_LAYOUT_PARSE_SUCCESS;
    }

    d_layout_parse_skip_spaces(_state);

    if (!_state)
    {
        return 0;
    }

    if ( d_layout_parse_at_end(_state) ||
         (_state->data[_state->offset] != '"') )
    {
        return 0;   // not a literal; nothing consumed
    }

    d_layout_parse_advance(_state, 1u);   // opening quote

    _start = _state->offset;

    while (!d_layout_parse_at_end(_state))
    {
        if (_state->data[_state->offset] == '"')
        {
            break;
        }

        d_layout_parse_advance(_state, 1u);
    }

    if (d_layout_parse_at_end(_state))
    {
        if (_out_status)
        {
            *_out_status = D_LAYOUT_PARSE_END_OF_INPUT;
        }

        return 0;   // unterminated
    }

    _stop = _state->offset;

    d_layout_parse_advance(_state, 1u);   // closing quote

    _out = arena_copy_helper(_arena, _state->data + _start, _stop - _start);

    if ( (!_out) && _out_status )
    {
        *_out_status = D_LAYOUT_PARSE_OVERFLOW;
    }

    return _out;
}

/*
make_leaf_atom_helper
  A content-leaf atom of the requested kind carrying _arg.

Parameter(s):
  _kind:  enum d_layout_atom_kind.
  _arg:   the arena-owned token.
Return:
  the atom.  An unrecognised kind falls through to body_ref, which is the C++
  side's shape: its two ifs and a trailing return, not a switch with a default
  that traps.
*/
static struct d_layout_atom
make_leaf_atom_helper
(
    int32_t     _kind,
    const char* _arg
)
{
    if (_kind == D_LAYOUT_ATOM_LITERAL)
    {
        return d_layout_literal(_arg);
    }

    if (_kind == D_LAYOUT_ATOM_META_REF)
    {
        return d_layout_meta_ref(_arg);
    }

    return d_layout_body_ref(_arg);
}

/*
scan_attributes_helper
  Consumes zero or more key="value" pairs into a fresh run of arena entries.

  STOPS WITHOUT CONSUMING at the first identifier that is not followed by '=',
  restoring the offset so the caller sees that identifier as the next
  construct's keyword.  That restore is what makes `content "a" section "b"`
  parse as two constructs rather than one construct with a malformed attribute.

  THE RUN IS CANONICALISED BEFORE IT BECOMES A BAG, not after and not by a sort
  written here.  d_doc_attributes_canonicalise is the only ordering authority;
  a second one in this file is exactly the duplication that produced a bag
  agreeing with itself and disagreeing with the other language.

Parameter(s):
  _state:        the cursor.
  _grammar:      unused for the scan itself, taken so the signature matches the
                 C++ template's and so a future grammar-scoped key policy has a
                 place to land without a signature change.
  _arena:        where entries and value text go.
  _out_bag:      the canonicalised bag.
  _out_first:    the index in _arena->attrs where this bag's run begins; the
                 caller needs it to extend the run, and MUST NOT recompute it
                 from _out_bag->count -- see the note on collapse below.
  _error:        filled on failure; may be NULL.
Return:
  1 on success, 0 on a key with '=' but no valid quoted value, or on either
  arena region running out.
*/
static int32_t
scan_attributes_helper
(
    struct d_layout_parse_state*   _state,
    const struct d_layout_grammar* _grammar,
    struct d_layout_parse_arena*   _arena,
    struct d_doc_attributes*       _out_bag,
    size_t*                        _out_first,
    struct d_layout_parse_error*   _error
)
{
    size_t             _first = 0;
    size_t             _count = 0;
    struct d_doc_attr* _run   = 0;

    (void)_grammar;

    if ( (!_state) || (!_arena) || (!_out_bag) || (!_out_first) )
    {
        return fail_helper(_error, D_LAYOUT_PARSE_FAILURE, 0u,
                           "internal: null argument to attribute scan", 0, 0u);
    }

    _first      = _arena->attr_used;
    *_out_first = _first;

    for (;;)
    {
        size_t      _save       = _state->offset;
        const char* _key_begin  = 0;
        size_t      _key_length = 0;
        const char* _key        = 0;
        const char* _value      = 0;
        int32_t     _status     = D_LAYOUT_PARSE_SUCCESS;

        _key_length = d_layout_parse_scan_identifier(_state, &_key_begin);

        if (_key_length == 0u)
        {
            _state->offset = _save;
            break;
        }

        d_layout_parse_skip_spaces(_state);

        if ( d_layout_parse_at_end(_state) ||
             (_state->data[_state->offset] != '=') )
        {
            // an identifier, but not an attribute
            _state->offset = _save;
            break;
        }

        d_layout_parse_advance(_state, 1u);   // '='

        //   The key is copied only NOW.  Copying it at the scan would spend
        // text budget on every construct keyword in the document, since a
        // keyword and an attribute key are indistinguishable until the '='.
        _key = arena_copy_helper(_arena, _key_begin, _key_length);

        if (!_key)
        {
            return fail_helper(_error, D_LAYOUT_PARSE_OVERFLOW, _state->offset,
                               "attribute key storage exhausted",
                               _key_begin, _key_length);
        }

        _value = d_layout_parse_scan_string(_state, _arena, &_status);

        if (!_value)
        {
            if (_status == D_LAYOUT_PARSE_OVERFLOW)
            {
                return fail_helper(_error, D_LAYOUT_PARSE_OVERFLOW,
                                   _state->offset,
                                   "attribute value storage exhausted",
                                   _key_begin, _key_length);
            }

            return fail_helper(_error, D_LAYOUT_PARSE_FAILURE, _state->offset,
                               "expected a quoted attribute value",
                               _key_begin, _key_length);
        }

        if (_arena->attr_used >= _arena->attr_capacity)
        {
            return fail_helper(_error, D_LAYOUT_PARSE_OVERFLOW, _state->offset,
                               "attribute entry storage exhausted",
                               _key_begin, _key_length);
        }

        _arena->attrs[_arena->attr_used].key   = _key;
        _arena->attrs[_arena->attr_used].value = _value;
        ++_arena->attr_used;

        ++_count;
    }

    if (_count == 0u)
    {
        _out_bag->items    = 0;
        _out_bag->count    = 0u;
        _out_bag->reserved = 0u;

        return 1;
    }

    _run      = _arena->attrs + _first;
    *_out_bag = d_doc_attributes_canonicalise(_run, (uint32_t)_count);

    //   RECLAIM WHAT COLLAPSE DROPPED.  canonicalise de-duplicates in place and
    // returns a bag over the shorter PREFIX, so entries between the new count
    // and _count are stale.  Leaving attr_used past the prefix would mean the
    // next allocation lands after that stale region -- and then folding the
    // positional argument into this run and re-canonicalising would sort the
    // stale entries back in, resurrecting exactly the duplicate binding that
    // was just collapsed.  On `section style="a" style="b"` the bag would come
    // back holding both, with the wrong one winning half the time.
    //   Winding attr_used back to the prefix keeps the run contiguous and
    // canonical, which is the invariant the argument fold below relies on.
    _arena->attr_used = _first + _out_bag->count;

    return 1;
}

/*
parse_block_sequence_helper
  A run of blocks.  When _braced it is bracketed by { } and ends at the
  matching }; otherwise it runs to end of input, which is the top level.

  CHILDREN ARE COLLECTED INTO CALLER STORAGE, NOT A VECTOR.  The C++ side
  push_backs into a std::vector and hands it to apply_node.  d_layout_apply
  takes a contiguous array and a count, so the run is gathered into _children
  and the capacity is the caller's -- which is what makes the nesting depth a
  budget the caller sets rather than a heap allocation this tier hides.

Parameter(s):
  _state:            the cursor.
  _grammar:          the parse table.
  _tree_arena:       where nodes go.
  _parse_arena:      where bags and text go.
  _braced:           1 for a { } block, 0 for the top level.
  _children:         caller storage for the run.
  _child_capacity:   how many it holds.
  _out_count:        how many were written.
  _error:            filled on failure; may be NULL.
Return:
  1 on success, 0 otherwise.
*/
static int32_t
parse_block_sequence_helper
(
    struct d_layout_parse_state*   _state,
    const struct d_layout_grammar* _grammar,
    struct d_layout_arena*         _tree_arena,
    struct d_layout_parse_arena*   _parse_arena,
    int32_t                        _braced,
    struct d_cofree*               _children,
    size_t                         _child_capacity,
    size_t*                        _out_count,
    struct d_layout_parse_error*   _error
)
{
    size_t _count = 0;

    if (_out_count)
    {
        *_out_count = 0u;
    }

    if (_braced)
    {
        d_layout_parse_skip_spaces(_state);

        if ( d_layout_parse_at_end(_state) ||
             (_state->data[_state->offset] != '{') )
        {
            return fail_helper(_error, D_LAYOUT_PARSE_FAILURE, _state->offset,
                               "expected '{'", 0, 0u);
        }

        d_layout_parse_advance(_state, 1u);
    }

    for (;;)
    {
        struct d_cofree* _block = 0;

        d_layout_parse_skip_spaces(_state);

        if (_braced)
        {
            if ( (!d_layout_parse_at_end(_state)) &&
                 (_state->data[_state->offset] == '}') )
            {
                d_layout_parse_advance(_state, 1u);
                break;
            }

            if (d_layout_parse_at_end(_state))
            {
                return fail_helper(_error, D_LAYOUT_PARSE_END_OF_INPUT,
                                   _state->offset,
                                   "unterminated '{' block", 0, 0u);
            }
        }
        else
        {
            if (d_layout_parse_at_end(_state))
            {
                break;
            }
        }

        if (_count >= _child_capacity)
        {
            return fail_helper(_error, D_LAYOUT_PARSE_OVERFLOW, _state->offset,
                               "too many blocks in one sequence", 0, 0u);
        }

        if (!parse_block_helper(_state, _grammar, _tree_arena, _parse_arena,
                                &_block, _error))
        {
            return 0;
        }

        //   BY VALUE, because d_layout_apply copies a contiguous run of nodes
        // out of here into the tree arena -- the same discipline
        // d_layout_close uses, and the reason layout_interpret.h keys its
        // number table on pre-order ordinal rather than on node address.
        _children[_count] = *_block;
        ++_count;
    }

    if (_out_count)
    {
        *_out_count = _count;
    }

    return 1;
}

/*
parse_block_helper
  One construct: keyword, optional quoted argument, optional attributes, and
  for a block construct an optional child block.

  THE ORDER OF THE THREE SCANS IS LOAD-BEARING.  Argument before attributes,
  because `section "T" style="x"` puts the quoted argument first; attributes
  before the leaf/block decision, because both arms need the bag and scanning
  it twice would double-consume.  The C++ file has the same order and it is not
  incidental there either.

Parameter(s):
  _state:        the cursor.
  _grammar:      the parse table.
  _tree_arena:   where nodes go.
  _parse_arena:  where bags and text go.
  _out_node:     the node built.
  _error:        filled on failure; may be NULL.
Return:
  1 on success, 0 otherwise.
*/
static int32_t
parse_block_helper
(
    struct d_layout_parse_state*   _state,
    const struct d_layout_grammar* _grammar,
    struct d_layout_arena*         _tree_arena,
    struct d_layout_parse_arena*   _parse_arena,
    struct d_cofree**              _out_node,
    struct d_layout_parse_error*   _error
)
{
    size_t                            _start       = 0;
    const char*                       _key_begin   = 0;
    size_t                            _key_length  = 0;
    const char*                       _arg         = 0;
    int32_t                           _arg_status  = D_LAYOUT_PARSE_SUCCESS;
    struct d_doc_attributes           _bag;
    const struct d_layout_leaf_word*  _leaf        = 0;
    const struct d_layout_block_word* _block       = 0;
    const char*                       _arg_key     = 0;
    size_t                            _attr_first  = 0;

    //   CHILD STORAGE IS THIS FRAME'S.  A sequence's run lives on the stack of
    // the block that owns it, so the recursion's depth bounds the total rather
    // than a shared pool that a deep document exhausts at an arbitrary point.
    //   The bound is stated here rather than taken as a parameter because it
    // must be a constant expression; a caller needing more nests a document
    // construct, which is what the root op is for.
    struct d_cofree                   _children[D_LAYOUT_PARSE_MAX_SIBLINGS];
    size_t                            _child_count = 0;

    if ( (!_state) || (!_grammar) || (!_out_node) )
    {
        return fail_helper(_error, D_LAYOUT_PARSE_FAILURE, 0u,
                           "internal: null argument to block parse", 0, 0u);
    }

    *_out_node = 0;
    _start     = _state->offset;

    _key_length = d_layout_parse_scan_identifier(_state, &_key_begin);

    if (_key_length == 0u)
    {
        return fail_helper(_error, D_LAYOUT_PARSE_FAILURE, _start,
                           "expected a construct keyword", 0, 0u);
    }

    // optional positional argument
    _arg = d_layout_parse_scan_string(_state, _parse_arena, &_arg_status);

    if ( (!_arg) && (_arg_status != D_LAYOUT_PARSE_SUCCESS) )
    {
        return fail_helper(_error, _arg_status, _state->offset,
                           (_arg_status == D_LAYOUT_PARSE_OVERFLOW)
                               ? "argument storage exhausted"
                               : "unterminated quoted argument",
                           _key_begin, _key_length);
    }

    // optional attributes
    if (!scan_attributes_helper(_state, _grammar, _parse_arena, &_bag,
                                &_attr_first, _error))
    {
        return 0;
    }

    // a content leaf?
    _leaf = d_layout_grammar_leaf_for(_grammar, _key_begin, _key_length);

    if (_leaf)
    {
        if (!_arg)
        {
            return fail_helper(_error, D_LAYOUT_PARSE_FAILURE, _start,
                               "content construct needs a name",
                               _key_begin, _key_length);
        }

        *_out_node = d_layout_leaf(_tree_arena, _bag,
                                   make_leaf_atom_helper(_leaf->kind, _arg));

        if (!*_out_node)
        {
            return fail_helper(_error, D_LAYOUT_PARSE_OVERFLOW, _start,
                               "tree arena exhausted at a leaf",
                               _key_begin, _key_length);
        }

        return 1;
    }

    // a block construct?
    _block = d_layout_grammar_block_for(_grammar, _key_begin, _key_length);

    if (!_block)
    {
        return fail_helper(_error, D_LAYOUT_PARSE_FAILURE, _start,
                           "unknown construct", _key_begin, _key_length);
    }

    //   THE ARGUMENT JOINS THE BAG, AND THEREFORE RE-CANONICALISES IT.  The
    // C++ side calls _bag.set(arg_key, arg) after the attributes are in, and
    // an insertion-ordered store keeps it last.  Here the bag is already
    // canonical, so appending would put arg_key out of order -- the exact
    // failure d_doc_attributes_is_canonical exists to catch.  The entry is
    // appended to the arena run and the whole run re-canonicalised.
    if (_arg)
    {
        _arg_key = _grammar->arg_key ? _grammar->arg_key : "title";

        if (_parse_arena->attr_used >= _parse_arena->attr_capacity)
        {
            return fail_helper(_error, D_LAYOUT_PARSE_OVERFLOW, _start,
                               "attribute entry storage exhausted at argument",
                               _key_begin, _key_length);
        }

        //   THE RUN START IS THE ONE SCAN REPORTED, NEVER ONE DERIVED BACKWARDS
        // FROM _bag.count.  canonicalise collapses duplicates, so the bag may
        // be shorter than the run that produced it, and
        // `attr_used - _bag.count - 1` then points into the middle of the run
        // -- dropping the earliest attributes from the re-sort and silently
        // producing a node missing hints the source plainly wrote.
        //   scan_attributes_helper has wound attr_used back to the end of the
        // canonical prefix, so appending here extends that prefix and the
        // re-canonicalisation below sees a contiguous run of live entries.
        _parse_arena->attrs[_parse_arena->attr_used].key   = _arg_key;
        _parse_arena->attrs[_parse_arena->attr_used].value = _arg;
        ++_parse_arena->attr_used;

        {
            struct d_doc_attr* _run = _parse_arena->attrs + _attr_first;

            _bag = d_doc_attributes_canonicalise(_run, _bag.count + 1u);

            //   And again, for the same reason: an explicit `title="x"` beside
            // a positional argument is a duplicate, and this fold is where it
            // collapses.
            _parse_arena->attr_used = _attr_first + _bag.count;
        }
    }

    // optional child block
    d_layout_parse_skip_spaces(_state);

    if ( (!d_layout_parse_at_end(_state)) &&
         (_state->data[_state->offset] == '{') )
    {
        if (!parse_block_sequence_helper(_state, _grammar, _tree_arena,
                                         _parse_arena, 1,
                                         _children,
                                         D_LAYOUT_PARSE_MAX_SIBLINGS,
                                         &_child_count, _error))
        {
            return 0;
        }
    }

    *_out_node = d_layout_apply(_tree_arena, _bag, _block->op,
                                _children, _child_count);

    if (!*_out_node)
    {
        return fail_helper(_error, D_LAYOUT_PARSE_OVERFLOW, _start,
                           "tree arena exhausted at an application",
                           _key_begin, _key_length);
    }

    return 1;
}


// ===========================================================================
// V.     entry
// ===========================================================================

/*
d_layout_parse_document
  Parses a whole document: a top-level sequence of blocks wrapped in the
  grammar's root construct.

Parameter(s):
  _state:        the cursor, positioned wherever the caller wants it.
  _grammar:      the parse table.
  _tree_arena:   where nodes go.
  _parse_arena:  where bags and text go.
  _out_tree:     the root.
  _error:        filled on failure and ZEROED on success; may be NULL.
Return:
  1 on success, 0 otherwise.
*/
int32_t
d_layout_parse_document
(
    struct d_layout_parse_state*   _state,
    const struct d_layout_grammar* _grammar,
    struct d_layout_arena*         _tree_arena,
    struct d_layout_parse_arena*   _parse_arena,
    struct d_cofree**              _out_tree,
    struct d_layout_parse_error*   _error
)
{
    struct d_cofree         _children[D_LAYOUT_PARSE_MAX_SIBLINGS];
    size_t                  _child_count = 0;
    struct d_doc_attributes _empty       = D_DOC_ATTRIBUTES_EMPTY;

    if ( (!_state) || (!_grammar) || (!_tree_arena) ||
         (!_parse_arena) || (!_out_tree) )
    {
        return fail_helper(_error, D_LAYOUT_PARSE_FAILURE, 0u,
                           "internal: null argument to document parse", 0, 0u);
    }

    *_out_tree = 0;

    if (!parse_block_sequence_helper(_state, _grammar, _tree_arena,
                                     _parse_arena, 0,
                                     _children, D_LAYOUT_PARSE_MAX_SIBLINGS,
                                     &_child_count, _error))
    {
        return 0;
    }

    d_layout_parse_skip_spaces(_state);

    if (!d_layout_parse_at_end(_state))
    {
        return fail_helper(_error, D_LAYOUT_PARSE_FAILURE, _state->offset,
                           "trailing input after document", 0, 0u);
    }

    //   THE ROOT CARRIES THE EMPTY BAG, matching the C++ side's
    // doc_attributes() at the same position.  A root that absorbed the first
    // block's attributes would be a different term for the same source.
    *_out_tree = d_layout_apply(_tree_arena, _empty, _grammar->root_op,
                                _children, _child_count);

    if (!*_out_tree)
    {
        return fail_helper(_error, D_LAYOUT_PARSE_OVERFLOW, _state->offset,
                           "tree arena exhausted at the root", 0, 0u);
    }

    if (_error)
    {
        _error->status       = D_LAYOUT_PARSE_SUCCESS;
        _error->reserved     = 0u;
        _error->offset       = 0u;
        _error->message      = "";
        _error->token        = 0;
        _error->token_length = 0u;
    }

    return 1;
}

/*
d_layout_parse_source
  The convenience overload: parses a NUL-terminated source string.

Parameter(s):
  _source:       the text; borrowed, must outlive the tree.
  _grammar:      the parse table.
  _tree_arena:   where nodes go.
  _parse_arena:  where bags and text go.
  _out_tree:     the root.
  _error:        filled on failure; may be NULL.
Return:
  1 on success, 0 otherwise.  A NULL source is a failure rather than an empty
  document, because the two are different mistakes and only one is deliberate.
*/
int32_t
d_layout_parse_source
(
    const char*                    _source,
    const struct d_layout_grammar* _grammar,
    struct d_layout_arena*         _tree_arena,
    struct d_layout_parse_arena*   _parse_arena,
    struct d_cofree**              _out_tree,
    struct d_layout_parse_error*   _error
)
{
    struct d_layout_parse_state _state;

    if (!_source)
    {
        return fail_helper(_error, D_LAYOUT_PARSE_FAILURE, 0u,
                           "null source", 0, 0u);
    }

    d_layout_parse_state_init(&_state, _source, strlen(_source));

    return d_layout_parse_document(&_state, _grammar, _tree_arena,
                                   _parse_arena, _out_tree, _error);
}
