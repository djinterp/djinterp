/*******************************************************************************
* djinterp [c/parse]                                                      scan.c
*
* Expression core v2: definitions for scan.h, the default scanner.
*   A token is read where the last ended: blanks first, then by its first
* byte a name, a number, a string, or the longest spelling that matches. The
* spellings are found where they are kept, each time, with no table of the
* scanner's own: the caller's words, then each role of an operator's token,
* then each mixfix's parts and separators. That is a pass over the signature
* per token, which a table built once would save; but a table needs storage,
* and this scanner is for small languages and for builds without the heap.
*
*
* path:      /src/djinterp/c/modules/parse/expr/scan.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/modules/parse/expr/scan.h"  // corresponding header

#if (D_INTERNAL_PARSE_EXPR_SCAN == 1)

// std
#include <string.h>  // memcmp, memset, strlen


//==============================================================================
// 1.  BYTES
//==============================================================================


/**
 * @brief Whether a byte may begin a name: a letter, `_`, or a byte from
 *        0x80, so that a UTF-8 name passes whole.
 *
 * @param[in] _c  the byte.
 * @return true where it may.
 */
static bool
d_internal_scan_name_start(
    unsigned char _c
)
{
    return ( ( (_c >= 'a') && (_c <= 'z') ) ||
             ( (_c >= 'A') && (_c <= 'Z') ) ||
             (_c == '_')                    ||
             (_c >= 0x80u) );
}

/**
 * @brief Whether a byte is a decimal digit.
 *
 * @param[in] _c  the byte.
 * @return true where it is.
 */
static bool
d_internal_scan_digit(
    unsigned char _c
)
{
    return ( (_c >= '0') && (_c <= '9') );
}

/**
 * @brief Whether a byte may go on a name.
 *
 * @param[in] _c  the byte.
 * @return true where it may.
 */
static bool
d_internal_scan_name_byte(
    unsigned char _c
)
{
    return ( (d_internal_scan_name_start(_c)) ||
             (d_internal_scan_digit(_c)) );
}

/**
 * @brief Whether a byte is a blank: a space, a tab, a carriage return, a
 *        form feed or a vertical tab. A newline is not.
 *
 * @param[in] _c  the byte.
 * @return true where it is.
 */
static bool
d_internal_scan_blank(
    unsigned char _c
)
{
    return ( (_c == ' ') || (_c == '\t') || (_c == '\r') ||
             (_c == '\f') || (_c == '\v') );
}


//==============================================================================
// 2.  SPELLINGS
//==============================================================================


// dj_internal_scan_best
//   struct: the best spelling found so far: its length and its class.
struct dj_internal_scan_best
{
    const char* text;
    uint32_t    avail;
    bool        whole;
    uint32_t    length;
    uint32_t    cls;
};

/**
 * @brief Weighs one spelling against the text: where it matches, and is
 *        longer than the best so far, it is the best. A whole match must be
 *        the whole of what is offered; any other, a prefix of it.
 *
 * @param[in,out] _best      the best so far, and what is offered.
 * @param[in]     _spelling  the spelling; may be NULL or empty.
 * @param[in]     _class     its token's class.
 */
static void
d_internal_scan_weigh(
    struct dj_internal_scan_best* _best,
    const char*                   _spelling,
    uint32_t                      _class
)
{
    // a spelling to weigh, and one longer than the best
    if ( (!_spelling)                          ||
         (_spelling[0] == '\0')                ||
         (_class == D_PARSE_EXPR_CLASS_END) )
    {
        return;
    }

    const size_t length = strlen(_spelling);

    if ( (length > (size_t)_best->avail)                         ||
         (length <= (size_t)_best->length)                       ||
         ( (_best->whole) && (length != (size_t)_best->avail) )  ||
         (memcmp(_spelling, _best->text, length) != 0) )
    {
        return;
    }

    _best->length = (uint32_t)length;
    _best->cls    = _class;

    return;
}

/**
 * @brief Weighs every spelling a mixfix operator gives: its parts and its
 *        list's separators.
 *
 * @param[in,out] _best  the best so far.
 * @param[in]     _op    the operator.
 */
static void
d_internal_scan_mixfix(
    struct dj_internal_scan_best*  _best,
    const struct dj_parse_expr_op* _op
)
{
    const struct dj_parse_expr_mixfix* m = &_op->mixfix;

    for (uint32_t k = 0u; ( (k < m->parts) &&
                            (k < D_PARSE_EXPR_MAX_PARTS) ); ++k)
    {
        d_internal_scan_weigh(_best, m->part_spelling[k], m->part[k]);
    }

    if (m->list_hole)
    {
        d_internal_scan_weigh(_best, m->separator_spelling, m->separator);
        d_internal_scan_weigh(_best, m->row_separator_spelling,
                              m->row_separator);
    }

    return;
}

/**
 * @brief Finds the longest spelling at a place in the text: the caller's
 *        words first, so that one of them wins a tie, then each operator
 *        role's token, then each mixfix's parts and separators.
 *
 * @param[in]  _s      the scanner.
 * @param[in]  _at     where in the text.
 * @param[in]  _avail  the bytes offered: the rest of the text, or a whole
 *                     name.
 * @param[in]  _whole  whether the spelling must be all of them.
 * @param[out] _class  the spelling's class.
 * @return the spelling's length, or 0 where none matches.
 */
static uint32_t
d_internal_scan_spelling(
    const struct dj_parse_expr_scanner* _s,
    uint32_t                            _at,
    uint32_t                            _avail,
    bool                                _whole,
    uint32_t*                           _class
)
{
    const struct dj_parse_expr_signature* sig = _s->signature;
    struct dj_internal_scan_best          best;

    best.text   = _s->text + _at;
    best.avail  = _avail;
    best.whole  = _whole;
    best.length = 0u;
    best.cls    = D_PARSE_EXPR_CLASS_END;

    for (uint32_t i = 0u; i < _s->config.word_count; ++i)
    {
        d_internal_scan_weigh(&best, _s->config.words[i].spelling,
                              _s->config.words[i].cls);
    }

    // an operator's token, by each role that opens it
    for (uint32_t i = 0u; i < sig->role_count; ++i)
    {
        const struct dj_parse_expr_role* role = &sig->roles[i];
        const struct dj_parse_expr_op*   op   =
            (role->kind == D_PARSE_EXPR_ROLE_OP)
            ? d_parse_expr_find_op(sig, role->op)
            : NULL;

        if ( (op) &&
             (op->form != D_PARSE_EXPR_FORM_MIXFIX) )
        {
            d_internal_scan_weigh(&best, op->spelling, role->cls);
        }
    }

    for (uint32_t i = 0u; i < sig->op_count; ++i)
    {
        if (sig->ops[i].form == D_PARSE_EXPR_FORM_MIXFIX)
        {
            d_internal_scan_mixfix(&best, &sig->ops[i]);
        }
    }

    *_class = best.cls;

    return best.length;
}


//==============================================================================
// 3.  TOKENS
//==============================================================================


/**
 * @brief The end of a number begun at a digit: digits, then a point and
 *        digits, then an exponent, each part only where a digit follows it.
 *
 * @param[in] _s   the scanner.
 * @param[in] _at  the number's first byte.
 * @return one past its last byte.
 */
static uint32_t
d_internal_scan_number(
    const struct dj_parse_expr_scanner* _s,
    uint32_t                            _at
)
{
    const unsigned char* t = (const unsigned char*)_s->text;
    const uint32_t       n = _s->length;
    uint32_t             i = _at;

    while ( (i < n) && (d_internal_scan_digit(t[i])) )
    {
        ++i;
    }

    // a point, where a digit follows it
    if ( ((i + 1u) < n)                      &&
         (t[i] == '.')                       &&
         (d_internal_scan_digit(t[i + 1u])) )
    {
        i += 2u;

        while ( (i < n) && (d_internal_scan_digit(t[i])) )
        {
            ++i;
        }
    }

    // an exponent, where a digit follows it, after a sign or none
    if ( (i < n) && ( (t[i] == 'e') || (t[i] == 'E') ) )
    {
        uint32_t k = i + 1u;

        if ( (k < n) && ( (t[k] == '+') || (t[k] == '-') ) )
        {
            ++k;
        }

        if ( (k < n) && (d_internal_scan_digit(t[k])) )
        {
            i = k;

            while ( (i < n) && (d_internal_scan_digit(t[i])) )
            {
                ++i;
            }
        }
    }

    return i;
}

/**
 * @brief The end of a string begun at its quote, or 0 where it never
 *        closes.
 *
 * @param[in] _s   the scanner.
 * @param[in] _at  the opening quote.
 * @return one past the closing quote, or 0.
 */
static uint32_t
d_internal_scan_string(
    const struct dj_parse_expr_scanner* _s,
    uint32_t                            _at
)
{
    const char quote = _s->text[_at];

    for (uint32_t i = _at + 1u; i < _s->length; ++i)
    {
        // a backslash keeps the byte after it
        if (_s->text[i] == '\\')
        {
            ++i;

            continue;
        }

        if (_s->text[i] == quote)
        {
            return i + 1u;
        }
    }

    return 0u;
}

/**
 * @brief Skips the blanks before a token, and the newlines where they are
 *        blanks; notes each in the token's flags.
 *
 * @param[in,out] _s      the scanner.
 * @param[out]    _flags  the token's flags.
 * @return true where a newline of its own class was reached, unread.
 */
static bool
d_internal_scan_blanks(
    struct dj_parse_expr_scanner* _s,
    uint32_t*                     _flags
)
{
    // what the token before left: a newline that was a token of its own
    *_flags      |= _s->pending;
    _s->pending   = 0u;

    while (_s->at < _s->length)
    {
        const unsigned char c = (unsigned char)_s->text[_s->at];

        if (c == '\n')
        {
            // a newline of its own class is a token; the one after it
            // follows a line break
            if (_s->config.newline_class != D_PARSE_EXPR_CLASS_END)
            {
                _s->pending = ( D_PARSE_EXPR_TOKEN_LINE_BEFORE |
                                D_PARSE_EXPR_TOKEN_SPACE_BEFORE );

                return true;
            }

            *_flags |= ( D_PARSE_EXPR_TOKEN_LINE_BEFORE |
                         D_PARSE_EXPR_TOKEN_SPACE_BEFORE );
        }
        else if (!d_internal_scan_blank(c))
        {
            break;
        }
        else
        {
            *_flags |= D_PARSE_EXPR_TOKEN_SPACE_BEFORE;
        }

        ++_s->at;
    }

    return false;
}

/**
 * @brief Reads a token that begins with a name's byte: the whole name, a
 *        spelling where the signature or the caller spells all of it.
 *
 * @param[in]  _s      the scanner.
 * @param[out] _class  the token's class.
 * @return the token's length.
 */
static uint32_t
d_internal_scan_word(
    const struct dj_parse_expr_scanner* _s,
    uint32_t*                           _class
)
{
    uint32_t end = _s->at;

    while ( (end < _s->length) &&
            (d_internal_scan_name_byte((unsigned char)_s->text[end])) )
    {
        ++end;
    }

    // a word the signature spells is its token, as a whole name only
    if (d_internal_scan_spelling(_s, _s->at, end - _s->at, true,
                                 _class) == 0u)
    {
        *_class = _s->config.name_class;
    }

    return end - _s->at;
}

/**
 * @brief Reads the token at the scanner's place, past its blanks.
 *
 * @param[in]  _s      the scanner, at the token's first byte.
 * @param[out] _class  the token's class; 0 where it is a stray and the
 *                     configuration names no class for one.
 * @return the token's length.
 */
static uint32_t
d_internal_scan_one(
    const struct dj_parse_expr_scanner* _s,
    uint32_t*                           _class
)
{
    const unsigned char c = (unsigned char)_s->text[_s->at];
    uint32_t            length;

    // a name is read whole, whatever it turns out to be
    if (d_internal_scan_name_start(c))
    {
        length = d_internal_scan_word(_s, _class);

        if (*_class == D_PARSE_EXPR_CLASS_END)
        {
            *_class = _s->config.stray_class;
        }

        return length;
    }

    if ( (d_internal_scan_digit(c)) &&
         (_s->config.number_class != D_PARSE_EXPR_CLASS_END) )
    {
        *_class = _s->config.number_class;

        return d_internal_scan_number(_s, _s->at) - _s->at;
    }

    if ( ( (c == '"') || (c == '\'') ) &&
         (_s->config.string_class != D_PARSE_EXPR_CLASS_END) )
    {
        const uint32_t end = d_internal_scan_string(_s, _s->at);

        // a string never closed is a stray, to the end of the text
        *_class = (end != 0u) ? _s->config.string_class
                              : _s->config.stray_class;

        return (end != 0u) ? (end - _s->at) : (_s->length - _s->at);
    }

    // the longest spelling here; else one stray byte
    length = d_internal_scan_spelling(_s, _s->at, _s->length - _s->at, false,
                                      _class);

    if (length > 0u)
    {
        return length;
    }

    *_class = _s->config.stray_class;

    return 1u;
}

/**
 * @brief The source's `next`: the token at the scanner's place.
 *
 * @param[in,out] _ctx       the scanner.
 * @param[in]     _position  not consulted.
 * @param[in]     _mode      not consulted.
 * @param[out]    _out       the token.
 * @return 1 for a token, 0 at the end, -1 at a stray with no class for it.
 */
static int
d_internal_scan_next(
    void*                       _ctx,
    uint8_t                     _position,
    uint32_t                    _mode,
    struct dj_parse_expr_token* _out
)
{
    struct dj_parse_expr_scanner* s     = (struct dj_parse_expr_scanner*)_ctx;
    uint32_t                      flags = 0u;
    uint32_t                      cls   = D_PARSE_EXPR_CLASS_END;
    uint32_t                      length;

    (void)_position;
    (void)_mode;

    memset(_out, 0, sizeof(*_out));

    // a newline of its own class, or the token after the blanks
    if (d_internal_scan_blanks(s, &flags))
    {
        cls    = s->config.newline_class;
        length = 1u;
    }
    else if (s->at >= s->length)
    {
        _out->cls    = D_PARSE_EXPR_CLASS_END;
        _out->flags  = flags;
        _out->offset = s->length;

        return 0;
    }
    else
    {
        length = d_internal_scan_one(s, &cls);
    }

    // a stray with no class for it fails the source
    if (cls == D_PARSE_EXPR_CLASS_END)
    {
        return -1;
    }

    _out->cls    = cls;
    _out->flags  = flags;
    _out->offset = s->at;
    _out->length = length;
    s->at       += length;

    return 1;
}


//==============================================================================
// 4.  BINDING
//==============================================================================


/*
d_parse_expr_scan
  The scanner keeps a copy of the configuration and borrows everything else;
its source is the caller's, bound to it.
*/
bool
d_parse_expr_scan(
    struct dj_parse_expr_source*            _source,
    struct dj_parse_expr_scanner*           _scanner,
    const struct dj_parse_expr_signature*   _signature,
    const struct dj_parse_expr_scan_config* _config,
    const char*                             _text,
    size_t                                  _length
)
{
    // everything given, and a text whose offsets fit 32 bits
    if ( (!_source)                                       ||
         (!_scanner)                                      ||
         (!_signature)                                    ||
         (!_config)                                       ||
         ( (!_text) && (_length > 0u) )                   ||
         (_length >= (size_t)0xFFFFFFFFu)                 ||
         ( (_config->word_count > 0u) && (!_config->words) ) )
    {
        return false;
    }

    memset(_scanner, 0, sizeof(*_scanner));
    _scanner->signature = _signature;
    _scanner->config    = *_config;
    _scanner->text      = (_text) ? _text : "";
    _scanner->length    = (uint32_t)_length;
    _source->next       = d_internal_scan_next;
    _source->ctx        = _scanner;

    return true;
}


#endif  // D_INTERNAL_PARSE_EXPR_SCAN
