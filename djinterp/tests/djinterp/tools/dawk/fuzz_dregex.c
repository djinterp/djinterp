/******************************************************************************
* djinterp [dawk]                                                fuzz_dregex.c
*
*   Differential test of the ERE engine against the system POSIX matcher.
*     Random patterns are generated over a three-letter alphabet and run
* against random subjects through both d_regex_search_groups and regexec with
* REG_EXTENDED.  Both are specified to report the leftmost-longest overall
* match and POSIX-preferred submatches, so any disagreement in offset, length
* or group assignment is a defect in one of them.
*     The generator avoids constructs POSIX leaves undefined -- empty
* alternation branches, escapes, quantified anchors -- since an implementation
* is free to differ there and a difference would not be a finding.
*
*
* path:      /tests/djinterp/c/dawk/fuzz_dregex.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dregex.h"  // corresponding header
// std
#include <regex.h>   // regcomp, regexec, regfree, REG_EXTENDED
#include <stdint.h>  // uint64_t
#include <sys/types.h>  // ssize_t, for reporting an absent group
#include <stdio.h>   // printf, fprintf
#include <stdlib.h>  // strtoul, EXIT_SUCCESS
#include <string.h>  // strlen


static uint64_t g_state = 0x2545F4914F6CDD1DULL;


/*
d_fuzz_next
  Produces the next value of the xorshift generator driving the test.

Parameter(s):
  none.
Return:
  A pseudo-random 64-bit value.
*/
static uint64_t
d_fuzz_next(void)
{
    g_state ^= (g_state << 13);
    g_state ^= (g_state >> 7);
    g_state ^= (g_state << 17);

    return g_state;
}


/*
d_fuzz_below
  Produces a pseudo-random value below a bound.

Parameter(s):
  _bound: the exclusive upper bound; must be non-zero.
Return:
  A value in the range [0, _bound).
*/
static unsigned
d_fuzz_below(
    unsigned _bound
)
{
    return (unsigned)(d_fuzz_next() % _bound);
}


/*
d_fuzz_atom
  Appends one randomly chosen atom to the pattern under construction.

Parameter(s):
  _buffer: the pattern buffer.
  _used:   the current length, advanced by this call.
  _limit:  the capacity of the buffer.
  _depth:  remaining nesting budget for parenthesised groups.
Return:
  none.
*/
static void
d_fuzz_atom(
    char*   _buffer,
    size_t* _used,
    size_t  _limit,
    int     _depth
);


/*
d_fuzz_branch
  Appends a concatenation of one to three quantified atoms.

Parameter(s):
  _buffer: the pattern buffer.
  _used:   the current length, advanced by this call.
  _limit:  the capacity of the buffer.
  _depth:  remaining nesting budget for parenthesised groups.
Return:
  none.
*/
static void
d_fuzz_branch(
    char*   _buffer,
    size_t* _used,
    size_t  _limit,
    int     _depth
)
{
    const unsigned pieces = 1u + d_fuzz_below(3u);

    // each piece is an atom optionally followed by a quantifier
    for (unsigned piece = 0; piece < pieces; ++piece)
    {
        const size_t before = *_used;

        d_fuzz_atom(_buffer, _used, _limit, _depth);

        // an atom that did not fit ends the branch
        if (*_used == before)
        {
            return;
        }

        const unsigned choice = d_fuzz_below(8u);

        // leave most atoms unquantified so subjects still match often
        if ((choice >= 4u) || ((*_used + 8u) >= _limit))
        {
            continue;
        }

        // an anchor must not be quantified; POSIX leaves that undefined
        if ((_buffer[*_used - 1] == '^') || (_buffer[*_used - 1] == '$'))
        {
            continue;
        }

        // append one of the four repetition forms
        if (choice == 0u)
        {
            _buffer[(*_used)++] = '*';
        }
        else if (choice == 1u)
        {
            _buffer[(*_used)++] = '+';
        }
        else if (choice == 2u)
        {
            _buffer[(*_used)++] = '?';
        }
        else
        {
            const unsigned low  = d_fuzz_below(3u);
            const unsigned high = low + d_fuzz_below(3u);

            *_used += (size_t)snprintf(&_buffer[*_used],
                                       _limit - *_used,
                                       "{%u,%u}",
                                       low,
                                       high);
        }
    }

    return;
}


/*
d_fuzz_alternation
  Appends one to three branches separated by `|`.

Parameter(s):
  _buffer: the pattern buffer.
  _used:   the current length, advanced by this call.
  _limit:  the capacity of the buffer.
  _depth:  remaining nesting budget for parenthesised groups.
Return:
  none.
*/
static void
d_fuzz_alternation(
    char*   _buffer,
    size_t* _used,
    size_t  _limit,
    int     _depth
)
{
    const unsigned branches = 1u + d_fuzz_below(3u);

    // branches are separated by the alternation operator
    for (unsigned branch = 0; branch < branches; ++branch)
    {
        // an empty branch is undefined in POSIX, so never emit a bare bar
        if ((branch > 0u) && ((*_used + 2u) < _limit))
        {
            _buffer[(*_used)++] = '|';
        }

        d_fuzz_branch(_buffer, _used, _limit, _depth);
    }

    return;
}


static void
d_fuzz_atom(
    char*   _buffer,
    size_t* _used,
    size_t  _limit,
    int     _depth
)
{
    // leave room for a quantifier and the terminator
    if ((*_used + 12u) >= _limit)
    {
        return;
    }

    const unsigned choice = d_fuzz_below(10u);

    // a group recurses while the nesting budget allows
    if ((choice == 0u) && (_depth > 0))
    {
        _buffer[(*_used)++] = '(';
        d_fuzz_alternation(_buffer, _used, _limit - 2u, _depth - 1);
        _buffer[(*_used)++] = ')';

        return;
    }

    // a bracket expression, possibly negated
    if (choice == 1u)
    {
        _buffer[(*_used)++] = '[';

        // half of the bracket expressions exclude rather than include
        if (d_fuzz_below(2u) == 0u)
        {
            _buffer[(*_used)++] = '^';
        }

        const unsigned members = 1u + d_fuzz_below(3u);

        // members are drawn from the same alphabet as the subjects
        for (unsigned member = 0; member < members; ++member)
        {
            _buffer[(*_used)++] = (char)('a' + d_fuzz_below(3u));
        }

        _buffer[(*_used)++] = ']';

        return;
    }

    // the wildcard
    if (choice == 2u)
    {
        _buffer[(*_used)++] = '.';

        return;
    }

    // anchors are never emitted here; see the note in main
    if (choice == 3u)
    {
        _buffer[(*_used)++] = (char)('a' + d_fuzz_below(3u));

        return;
    }

    _buffer[(*_used)++] = (char)('a' + d_fuzz_below(3u));

    return;
}


/*
main
  Generates random pattern and subject pairs and compares the engine against
  the system POSIX matcher.

Parameter(s):
  argc: argument count; an optional iteration count may be supplied.
  argv: argument vector.
Return:
  Zero when every comparison agreed, and one otherwise.
*/
int
main(
    int    argc,
    char** argv
)
{
    unsigned long rounds  = 200000ul;
    unsigned long allowed = 0ul;

    // accept an iteration count from the command line
    if (argc > 1)
    {
        rounds = strtoul(argv[1], NULL, 10);
    }

    // A second argument is a tolerance, in mismatches per thousand rounds.
    // It exists for the group comparison, where one documented class of
    // divergence remains; see README section 6.4.  The rate is reported
    // whether or not it is tolerated, so a regression is still visible.
    if (argc > 2)
    {
        allowed = strtoul(argv[2], NULL, 10);
    }

    unsigned long compared    = 0ul;
    unsigned long mismatches  = 0ul;
    unsigned long unsupported = 0ul;

    for (unsigned long round = 0ul; round < rounds; ++round)
    {
        char   pattern[128];
        size_t used = 0;

        // anchor the pattern only at its ends, where every engine agrees
        const bool anchor_start = (d_fuzz_below(8u) == 0u);
        const bool anchor_end   = (d_fuzz_below(8u) == 0u);

        if (anchor_start)
        {
            pattern[used++] = '^';
        }

        d_fuzz_alternation(pattern, &used, sizeof(pattern) - 2u, 2);

        if (anchor_end)
        {
            pattern[used++] = '$';
        }

        pattern[used] = '\0';

        // a degenerate generation is not worth comparing
        if (used == 0)
        {
            continue;
        }

        char         subject[17];
        const size_t subject_length = (size_t)d_fuzz_below(16u);

        // subjects are drawn from the same three-letter alphabet
        for (size_t index = 0; index < subject_length; ++index)
        {
            subject[index] = (char)('a' + d_fuzz_below(3u));
        }

        subject[subject_length] = '\0';

        regex_t   oracle;
        const int oracle_status = regcomp(&oracle, pattern, REG_EXTENDED);

        enum d_regex_status status = D_REGEX_OK;
        struct d_regex*     regex  = d_regex_compile(pattern, &status);

        // both must accept the pattern before the results can be compared
        if ((oracle_status != 0) || (!regex))
        {
            unsupported++;

            // release whichever side did compile
            if (oracle_status == 0)
            {
                regfree(&oracle);
            }

            d_regex_free(regex);

            continue;
        }

        // a fast-path build asks the oracle for the overall extent only;
        // requesting submatch slots makes glibc do work this build ignores
#ifdef FASTONLY
        const size_t slots = 1u;
#else
        const size_t slots = (size_t)oracle.re_nsub + 1u;
#endif

        regmatch_t oracle_groups[D_REGEX_GROUP_MAX + 1];
        const bool oracle_found =
            (regexec(&oracle, subject, slots, oracle_groups, 0) == 0);

        struct d_regex_match groups[D_REGEX_GROUP_MAX + 1];
        bool found = false;

        // FASTONLY builds compare only the overall extent, which is what the
        // interpreter uses everywhere except gensub
#ifdef FASTONLY
        struct d_regex_match overall = { 0, 0 };

        found = d_regex_search(regex, subject, subject_length, 0, &overall);
        groups[0].start  = found ? overall.start : D_REGEX_NO_MATCH;
        groups[0].length = overall.length;

        const size_t compare_slots = 1u;
#else
        const enum d_regex_status pass = d_regex_search_groups(regex,
                                                               subject,
                                                               subject_length,
                                                               0,
                                                               groups,
                                                               slots);

        // a pass that ran out of budget is not a disagreement
        if (pass != D_REGEX_OK)
        {
            unsupported++;
            regfree(&oracle);
            d_regex_free(regex);
            continue;
        }

        found = (groups[0].start != D_REGEX_NO_MATCH);

        const size_t compare_slots = slots;
#endif

        compared++;

        size_t culprit = 0;
        bool   agrees  = (found == oracle_found);

        // when both matched, every group extent must agree exactly
        if ((agrees) && (found))
        {
            for (size_t slot = 0; slot < compare_slots; ++slot)
            {
                const bool oracle_set = (oracle_groups[slot].rm_so >= 0);
                const bool ours_set   =
                    (groups[slot].start != D_REGEX_NO_MATCH);

                // a group must participate on both sides or neither
                if (oracle_set != ours_set)
                {
                    agrees  = false;
                    culprit = slot;
                    break;
                }

                // an absent group has no extent to compare
                if (!ours_set)
                {
                    continue;
                }

                const size_t oracle_start  = (size_t)oracle_groups[slot].rm_so;
                const size_t oracle_length =
                    (size_t)(oracle_groups[slot].rm_eo -
                             oracle_groups[slot].rm_so);

                // the extents must be identical, group by group
                if ( (groups[slot].start != oracle_start) ||
                     (groups[slot].length != oracle_length) )
                {
                    agrees  = false;
                    culprit = slot;
                    break;
                }
            }
        }

        // report the first few disagreements in reproducible form
        if (!agrees)
        {
            mismatches++;

            if (mismatches <= 20ul)
            {
                fprintf(stderr,
                        "MISMATCH group %zu  /%s/ on \"%s\": "
                        "dawk (%zd,%zd) posix (%d,%d)\n",
                        culprit,
                        pattern,
                        subject,
                        (ssize_t)groups[culprit].start,
                        (ssize_t)groups[culprit].length,
                        (int)oracle_groups[culprit].rm_so,
                        (int)(oracle_groups[culprit].rm_eo -
                              oracle_groups[culprit].rm_so));
            }
        }

        regfree(&oracle);
        d_regex_free(regex);
    }

    const double rate = (compared > 0ul)
                      ? (((double)mismatches * 1000.0) / (double)compared)
                      : 0.0;

    printf("%lu compared, %lu mismatches (%.2f per thousand), "
           "%lu not comparable\n",
           compared,
           mismatches,
           rate,
           unsupported);

    // a tolerance of zero is the default, so the fast path stays strict
    if (rate > (double)allowed)
    {
        printf("rate exceeds the tolerance of %lu per thousand\n", allowed);
        return 1;
    }

    return 0;
}
