/*******************************************************************************
* djinterp [djinterp]                                                   daudit.c
*
* Repair audit:
*   Each check restates what a repair kind may do in the simplest form that
* can be written, deliberately unlike the code that performs the repair.  A
* bug shared by the repair and its check would pass; two formulations that
* have to agree make that far less likely.
*
*
* path:      /src/djinterp/tools/dawk/daudit.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.21
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/daudit.h"  // corresponding header
// std
#include <string.h>  // strlen, strncmp, memcmp


/*
d_internal_blank
*/
static bool
d_internal_blank(
    char _c
)
{
    return ((_c == ' ') || (_c == '\t'));
}


/*
d_internal_same_runs
  Reports whether two lines hold the same sequence of non-blank runs.  This is
the whole of what a geometric repair may preserve: it may move text, and it
may not change, join, split or reorder it.  Gluing a value to its label joins
two runs into one, and fails here.
*/
static bool
d_internal_same_runs(
    const char* _a,
    const char* _b
)
{
    size_t i = 0;
    size_t j = 0;

    for (;;)
    {
        while ((_a[i] != '\0') && (d_internal_blank(_a[i])))
        {
            ++i;
        }

        while ((_b[j] != '\0') && (d_internal_blank(_b[j])))
        {
            ++j;
        }

        if ((_a[i] == '\0') || (_b[j] == '\0'))
        {
            return ((_a[i] == '\0') && (_b[j] == '\0'));
        }

        // both runs, compared character by character to their ends
        while ( (_a[i] != '\0') && (!d_internal_blank(_a[i])) &&
                (_b[j] != '\0') && (!d_internal_blank(_b[j])) )
        {
            if (_a[i] != _b[j])
            {
                return false;
            }

            ++i;
            ++j;
        }

        // a run that ends in one line and not the other has been split/joined
        const bool a_end = ((_a[i] == '\0') || (d_internal_blank(_a[i])));
        const bool b_end = ((_b[j] == '\0') || (d_internal_blank(_b[j])));

        if (a_end != b_end)
        {
            return false;
        }
    }
}


/*
d_internal_fill_char
  The fill of a rule is its most frequent character.
*/
static char
d_internal_fill_char(
    const char* _line
)
{
    size_t counts[256] = { 0 };
    char   best        = '\0';
    size_t best_count  = 0;

    for (const unsigned char* at = (const unsigned char*)_line; *at; ++at)
    {
        ++counts[*at];

        if (counts[*at] > best_count)
        {
            best       = (char)*at;
            best_count = counts[*at];
        }
    }

    return best;
}


/*
d_internal_same_fill
  A fill repair may change how long the fill run is and nothing else.  With
every fill character removed, the two lines must be identical, and the fill
in the result must be a single run.
*/
static bool
d_internal_same_fill(
    const char* _before,
    const char* _after
)
{
    const char fill = d_internal_fill_char(_before);

    if ((fill == '\0') || (d_internal_fill_char(_after) != fill))
    {
        return false;
    }

    size_t i = 0;
    size_t j = 0;

    for (;;)
    {
        while (_before[i] == fill)
        {
            ++i;
        }

        while (_after[j] == fill)
        {
            ++j;
        }

        if (_before[i] != _after[j])
        {
            return false;
        }

        if (_before[i] == '\0')
        {
            break;
        }

        ++i;
        ++j;
    }

    // the fill in the result is one contiguous run
    const char* const first = strchr(_after, fill);
    const char*       end   = first;

    while ((end) && (*end == fill))
    {
        ++end;
    }

    return ((end) && (strchr(end, fill) == NULL));
}


/*
d_internal_at
  Reports whether `_text` occurs in `_line` at `_head`.
*/
static bool
d_internal_at(
    const char* _line,
    size_t      _head,
    size_t      _run,
    const char* _text
)
{
    if (!_text)
    {
        return true;
    }

    return ( ((_head + _run) <= strlen(_line)) &&
             (strlen(_text) == _run) &&
             (strncmp(_line + _head, _text, _run) == 0) );
}


/*
d_internal_matches
  Reports whether `_after` is exactly the concatenation of up to five pieces.
*/
static bool
d_internal_matches(
    const char* _after,
    const char* _p[5],
    size_t      _n[5]
)
{
    size_t at = 0;

    for (size_t piece = 0; piece < 5u; ++piece)
    {
        if (_n[piece] == 0)
        {
            continue;
        }

        if (memcmp(_after + at, _p[piece], _n[piece]) != 0)
        {
            return false;
        }

        at += _n[piece];

        if (at > strlen(_after))
        {
            return false;
        }
    }

    return (_after[at] == '\0');
}


/*
d_internal_single_word_comment
  Reports whether `_text` is blank, then one block comment holding a single
identifier and nothing else, then blank.
*/
static bool
d_internal_single_word_comment(
    const char* _text
)
{
    while (d_internal_blank(*_text))
    {
        ++_text;
    }

    if (strncmp(_text, "/*", 2u) != 0)
    {
        return false;
    }

    _text += 2;

    while (d_internal_blank(*_text))
    {
        ++_text;
    }

    const char* const word = _text;

    while ( ((*_text >= 'A') && (*_text <= 'Z')) ||
            ((*_text >= 'a') && (*_text <= 'z')) ||
            ((*_text >= '0') && (*_text <= '9')) || (*_text == '_') )
    {
        ++_text;
    }

    if (_text == word)
    {
        return false;
    }

    while (d_internal_blank(*_text))
    {
        ++_text;
    }

    if (strncmp(_text, "*/", 2u) != 0)
    {
        return false;
    }

    _text += 2;

    while (d_internal_blank(*_text))
    {
        ++_text;
    }

    return (*_text == '\0');
}


bool
d_audit_verify(
    const char*                 _before,
    const char*                 _after,
    const struct d_audit_claim* _claim,
    const char**                _out_reason
)
{
    const char* reason = NULL;

    if ((!_before) || (!_after) || (!_claim))
    {
        reason = "no claim";
    }
    else if (!d_internal_at(_before, _claim->head, _claim->run,
                            _claim->expect))
    {
        // the run the repair meant to change is not where it thought
        reason = "the node's text is not at its recorded place on the line";
    }
    else
    {
        switch (_claim->kind)
        {
            case D_AUDIT_GEOMETRY:
            {
                if (!d_internal_same_runs(_before, _after))
                {
                    reason = "a geometric repair changed, joined or split text";
                }

                break;
            }

            case D_AUDIT_FILL:
            {
                if (!d_internal_same_fill(_before, _after))
                {
                    reason = "a fill repair changed more than the fill";
                }

                break;
            }

            case D_AUDIT_SUBSTITUTE:
            {
                const char* p[5] = { _before, _claim->value,
                                     _before + _claim->head + _claim->run,
                                     NULL, NULL };
                size_t      n[5] = { _claim->head,
                                     _claim->value ? strlen(_claim->value) : 0,
                                     strlen(_before) - _claim->head
                                     - _claim->run, 0, 0 };

                if ((!_claim->value) || (!d_internal_matches(_after, p, n)))
                {
                    reason = "a substitution changed more than its run";
                }

                break;
            }

            case D_AUDIT_SWAP:
            {
                const size_t a0 = _claim->head;
                const size_t a1 = a0 + _claim->run;
                const size_t b0 = _claim->head2;
                const size_t b1 = b0 + _claim->run2;

                if ((a1 > b0) ||
                    (!d_internal_at(_before, b0, _claim->run2,
                                    _claim->expect2)))
                {
                    reason = "a swap's runs overlap or are misplaced";
                    break;
                }

                const char* p[5] = { _before, _before + b0, _before + a1,
                                     _before + a0, _before + b1 };
                size_t      n[5] = { a0, _claim->run2, b0 - a1, _claim->run,
                                     strlen(_before) - b1 };

                if (!d_internal_matches(_after, p, n))
                {
                    reason = "a swap changed more than its two runs";
                }

                break;
            }

            case D_AUDIT_INSERT:
            {
                const char* const rest = _before + _claim->head;

                // an insertion discards only blanks, or a verified stale
                // single-word block comment
                bool rest_ok = true;

                for (const char* at = rest; (*at) && (rest_ok); ++at)
                {
                    rest_ok = d_internal_blank(*at);
                }

                if ((!rest_ok) && (_claim->replaces))
                {
                    rest_ok = d_internal_single_word_comment(rest);
                }

                const char* p[5] = { _before, _claim->value, NULL, NULL,
                                     NULL };
                size_t      n[5] = { _claim->head,
                                     _claim->value ? strlen(_claim->value) : 0,
                                     0, 0, 0 };

                if ((!rest_ok) || (!_claim->value) ||
                    (!d_internal_matches(_after, p, n)))
                {
                    reason = "an insertion discarded text or placed more than "
                             "it claimed";
                }

                break;
            }

            default:
                reason = "unknown repair kind";
                break;
        }
    }

    if (_out_reason)
    {
        *_out_reason = reason;
    }

    return (reason == NULL);
}
