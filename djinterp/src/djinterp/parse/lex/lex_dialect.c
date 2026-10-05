/*******************************************************************************
* djinterp [parse]                                                 lex_dialect.c
*
* Definitions for the non-inline declarations in lex_dialect.h.
*   Three lookups and one check, none of which knows a language.  Each
* lookup rejects a row on its availability year before looking at its text,
* so one table serves every level of a language and an older level costs
* nothing extra to scan.
*   Where two rows carry the same spelling at different years -- `u8`,
* which prefixes a string from C11 and a character only from C23 -- the
* latest row at or below the level wins, which is what makes a spelling
* acquire a role without losing the one it had.
*
*
* path:      /src/djinterp/parse/lex/lex_dialect.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.22
*******************************************************************************/
#include "../../../../inc/djinterp/parse/lex/lex_dialect.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memcmp, strlen


//==============================================================================
// 1.  VALIDATION
//==============================================================================


/**
 * @brief Records why a descriptor was rejected, and rejects it.
 *
 * @param[out] _out_reason  receives `_reason`; may be `NULL`.
 * @param[in]  _reason      the failure, as a phrase.
 * @return `false`, always, so a caller can return the call.
 */
static bool
d_internal_reject(
    const char** _out_reason,
    const char*  _reason
)
{
    if (_out_reason)
    {
        *_out_reason = _reason;
    }

    return false;
}


/**
 * @brief Checks every operator row against the matcher's window.
 *
 * @param[in]  _dialect     the descriptor to check.
 * @param[out] _out_reason  receives the first failure; may be `NULL`.
 * @return `true` if every row is matchable, `false` otherwise.
 */
static bool
d_internal_validate_operators(
    const struct d_lex_dialect* _dialect,
    const char**                _out_reason
)
{
    for (size_t at = 0u; at < _dialect->operator_count; ++at)
    {
        const char* const text = _dialect->operators[at].text;

        // an empty row matches nothing and would stall the matcher
        if ( (!text) ||
             (*text == '\0') )
        {
            return d_internal_reject(_out_reason,
                                     "an operator row has no spelling");
        }

        // an operator wider than the matcher's window could never be matched
        if (strlen(text) > 4u)
        {
            return d_internal_reject(_out_reason,
                                     "an operator row is wider than four "
                                     "bytes");
        }
    }

    return true;
}


/**
 * @brief Checks that the identifier set agrees with the features that use it.
 *
 * @param[in]  _dialect     the descriptor to check.
 * @param[out] _out_reason  receives the failure; may be `NULL`.
 * @return `true` if the set is known and consistent, `false` otherwise.
 */
static bool
d_internal_validate_identifiers(
    const struct d_lex_dialect* _dialect,
    const char**                _out_reason
)
{
    const unsigned extended = ( D_LEX_FEATURE_UCN |
                                D_LEX_FEATURE_UTF8_IDENTIFIER );

    // an unknown set would make every extended character unclassifiable
    if (_dialect->identifiers > D_LEX_IDSET_XID)
    {
        return d_internal_reject(_out_reason,
                                 "the identifier set is not a D_LEX_IDSET_*");
    }

    // a dialect admitting extended characters must say which ones
    if ( ((_dialect->features & extended) != 0u) &&
         (_dialect->identifiers == D_LEX_IDSET_NONE) )
    {
        return d_internal_reject(_out_reason,
                                 "extended identifiers need an identifier "
                                 "set");
    }

    return true;
}


/*
d_lex_dialect_validate
  Each invariant has its own helper, so this reads as the list of what a
descriptor must satisfy; the helpers stop at the first failure, which is the
one reported.
*/
bool
d_lex_dialect_validate(
    const struct d_lex_dialect* _dialect,
    const char**                _out_reason
)
{
    // parameter validation first
    if (!_dialect)
    {
        return d_internal_reject(_out_reason,
                                 "descriptor is NULL");
    }

    // every table must be present, since the scanner never tests for one
    if ( (!_dialect->keywords)  ||
         (!_dialect->operators) ||
         (!_dialect->prefixes) )
    {
        return d_internal_reject(_out_reason,
                                 "a table is NULL");
    }

    // a keyword whose kind names no row would produce an unprintable token
    for (size_t at = 0u; at < _dialect->keyword_count; ++at)
    {
        if (!d_token_kind_name(_dialect->keywords[at].kind))
        {
            return d_internal_reject(_out_reason,
                                     "a keyword row names no kind");
        }
    }

    return ( d_internal_validate_operators(_dialect,
                                           _out_reason) &&
             d_internal_validate_identifiers(_dialect,
                                             _out_reason) );
}


//==============================================================================
// 2.  LOOKUP
//==============================================================================


/*
d_lex_dialect_has
  Reports whether a dialect carries a feature.
*/
bool
d_lex_dialect_has(
    const struct d_lex_dialect* _dialect,
    unsigned                    _feature
)
{
    return (_dialect && ((_dialect->features & _feature) != 0u));
}


/*
d_lex_dialect_keyword
  Resolves a spelling to a keyword kind at the dialect's level.
  The length and the first byte are compared before the text, because almost
every identifier offered here is not a keyword and the two cheap tests
settle the question for nearly every row.
*/
int
d_lex_dialect_keyword(
    const struct d_lex_dialect* _dialect,
    const char*                 _text,
    size_t                      _length
)
{
    // parameter validation first
    if ((!_dialect) || (!_dialect->keywords) || (!_text) || (_length == 0u))
    {
        return D_TOKEN_IDENTIFIER;
    }

    for (size_t at = 0u; at < _dialect->keyword_count; ++at)
    {
        const struct d_lex_keyword* const row = &_dialect->keywords[at];

        // a row the dialect's level never reached is not a keyword here
        if ((row->since == D_LEX_NEVER) || (row->since > _dialect->level))
        {
            continue;
        }

        if ((row->text[0] != _text[0]) || (strlen(row->text) != _length))
        {
            continue;
        }

        if (memcmp(row->text, _text, _length) == 0)
        {
            return row->kind;
        }
    }

    return D_TOKEN_IDENTIFIER;
}


/*
d_lex_dialect_operator
  Finds the operator row matching a run of characters exactly.
  The caller performs maximal munch by offering the longest window first;
this call answers only about the run it was given.
*/
const struct d_lex_operator*
d_lex_dialect_operator(
    const struct d_lex_dialect* _dialect,
    const char*                 _text,
    size_t                      _length
)
{
    // parameter validation first
    if ((!_dialect) || (!_dialect->operators) || (!_text) || (_length == 0u))
    {
        return NULL;
    }

    for (size_t at = 0u; at < _dialect->operator_count; ++at)
    {
        const struct d_lex_operator* const row = &_dialect->operators[at];

        // a row the dialect's level never reached is not an operator here
        if ((row->since == D_LEX_NEVER) || (row->since > _dialect->level))
        {
            continue;
        }

        if ((row->text[0] != _text[0]) || (strlen(row->text) != _length))
        {
            continue;
        }

        if (memcmp(row->text, _text, _length) == 0)
        {
            return row;
        }
    }

    return NULL;
}


/*
d_lex_dialect_prefix
  Finds the literal-prefix row for a spelling.
  Two rows may carry one spelling at different years, which is how `u8`
prefixes a string from C11 and a character only from C23.  The latest row at
or below the level wins.
*/
const struct d_lex_prefix*
d_lex_dialect_prefix(
    const struct d_lex_dialect* _dialect,
    const char*                 _text,
    size_t                      _length
)
{
    // parameter validation first
    if ((!_dialect) || (!_dialect->prefixes))
    {
        return NULL;
    }

    const struct d_lex_prefix* best = NULL;

    for (size_t at = 0u; at < _dialect->prefix_count; ++at)
    {
        const struct d_lex_prefix* const row = &_dialect->prefixes[at];

        // a row the dialect's level never reached is not a prefix here
        if ((row->since == D_LEX_NEVER) || (row->since > _dialect->level))
        {
            continue;
        }

        if (strlen(row->text) != _length)
        {
            continue;
        }

        if ((_length != 0u) && (memcmp(row->text, _text, _length) != 0))
        {
            continue;
        }

        // the later row supersedes, so that a role is added and none lost
        if ((!best) || (row->since >= best->since))
        {
            best = row;
        }
    }

    return best;
}
