/******************************************************************************
* djinterp [parsegen]                                               feature.c
*
*   Definitions for the non-inline declarations in feature.h.
*
*
* path:      /src/djinterp/parsegen/feature.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../inc/djinterp/parsegen/feature.h"  // corresponding header
// std
#include <stdio.h>   // snprintf


/*
d_parsegen_feature_name
  The name of one capability bit.
NOTE:
  Indexed by bit position rather than by mask, so the table is a flat array and
a gap in the numbering reads as the empty string rather than as a wrong name.

Parameter(s):
  _bit: the bit position, from 0 to 63.
Return:
  The name, or "" for a position this level does not name. Never NULL.
*/
const char*
d_parsegen_feature_name(
    unsigned _bit
)
{
    static const char* const names[D_PARSEGEN_FEATURE_COUNT] =
    {
        "ORDERED_CHOICE",     "UNORDERED_CHOICE",  "BOUNDED_REPEAT",
        "EMPTY_PRODUCTION",   "LEFT_RECURSION",    "SYNTACTIC_PREDICATE",
        "PRECEDENCE",         "RULE_PARAMETERS",   "CHARACTER_CLASS",
        "LITERAL_STRING",     "CASE_INSENSITIVE",  "UNICODE",
        "CAPTURE",            "HOST_ACTION",       "ERROR_RECOVERY",
        "",                   "TOKEN_STREAM",      "INDENTATION",
        "LOOKAHEAD_K",        "",                  "AMBIGUITY",
        "MEMOIZATION",        "INCREMENTAL",       "",
        "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "",
        "USER+0",  "USER+1",  "USER+2",  "USER+3",
        "USER+4",  "USER+5",  "USER+6",  "USER+7",
        "USER+8",  "USER+9",  "USER+10", "USER+11",
        "USER+12", "USER+13", "USER+14", "USER+15"
    };

    if (_bit >= D_PARSEGEN_FEATURE_COUNT)
    {
        return "";
    }

    return names[_bit];
}


/*
d_parsegen_features_render
  Writes a capability set as its named bits, separated by `|`.
NOTE:
  This is what every capability diagnostic prints, which is why it lives beside
the bits rather than in whichever stage reported the problem.

Parameter(s):
  _set:  the set to render.
  _out:  the buffer to write into; may be NULL when _size is 0.
  _size: the size of _out in bytes, including the terminator.
Return:
  The number of characters the full rendering would occupy, excluding the
terminator -- so a return of _size or more means the output was truncated.
*/
size_t
d_parsegen_features_render(
    d_parsegen_features _set,
    char*               _out,
    size_t              _size
)
{
    // an empty set has a name of its own, so a diagnostic never prints nothing
    if (_set == D_PARSEGEN_FEATURE_NONE)
    {
        const int written = snprintf(_out, _size, "none");

        return (written < 0) ? 0u : (size_t)written;
    }

    size_t needed    = 0u;
    char*  cursor    = _out;
    size_t remaining = _size;
    int    first     = 1;

    for (unsigned bit = 0u; bit < D_PARSEGEN_FEATURE_COUNT; bit++)
    {
        if ((_set & ((d_parsegen_features)1 << bit)) == 0u)
        {
            continue;
        }

        const char* const name = d_parsegen_feature_name(bit);

        char piece[40];

        // an unnamed bit still prints, as its position, so a set is never
        // rendered as less than it is
        if (name[0] == '\0')
        {
            (void)snprintf(piece,
                           sizeof(piece),
                           "%sbit%u",
                           first ? "" : "|",
                           bit);
        }
        else
        {
            (void)snprintf(piece,
                           sizeof(piece),
                           "%s%s",
                           first ? "" : "|",
                           name);
        }

        first = 0;

        const int written = snprintf(cursor, remaining, "%s", piece);
        const size_t grew = (written < 0) ? 0u : (size_t)written;

        needed += grew;

        // keep counting after the buffer fills, so the return stays correct
        if (grew < remaining)
        {
            cursor    += grew;
            remaining -= grew;
        }
        else
        {
            cursor    = NULL;
            remaining = 0u;
        }
    }

    return needed;
}
