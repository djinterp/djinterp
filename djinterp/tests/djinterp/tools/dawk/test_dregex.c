/******************************************************************************
* djinterp [dawk]                                                test_dregex.c
*
*   Unit tests for the POSIX ERE engine in regex.h.
*     The cases that matter most are the leftmost-longest ones: a backtracking
* engine passes almost everything here except those, so they are grouped first
* and stated explicitly against the alternative a Perl-style engine would give.
*
*
* path:      /tests/djinterp/c/dawk/test_dregex.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dregex.h"  // corresponding header
// std
#include <stdio.h>   // printf
#include <string.h>  // strlen, strcmp
#include <sys/types.h>  // ssize_t, for reporting an absent group


static int g_checks = 0;
static int g_failed = 0;


/*
d_tests_expect
  Compares one located match against the expected extent and records the
  outcome.

Parameter(s):
  _pattern:  the pattern under test.
  _subject:  the subject text.
  _expect:   true when a match is expected.
  _start:    expected offset; ignored when no match is expected.
  _length:   expected length; ignored when no match is expected.
Return:
  none.
*/
static void
d_tests_expect(
    const char* _pattern,
    const char* _subject,
    bool        _expect,
    size_t      _start,
    size_t      _length
)
{
    enum d_regex_status status = D_REGEX_OK;
    struct d_regex*     regex  = d_regex_compile(_pattern, &status);

    g_checks++;

    // a pattern that fails to compile cannot be evaluated
    if (!regex)
    {
        g_failed++;
        printf("  FAIL  /%s/ did not compile: %s\n",
               _pattern,
               d_regex_status_text(status));
        return;
    }

    struct d_regex_match match = { 0, 0 };

    const bool found = d_regex_search(regex,
                                      _subject,
                                      strlen(_subject),
                                      0,
                                      &match);

    // compare the outcome against the expectation
    if (found != _expect)
    {
        g_failed++;
        printf("  FAIL  /%s/ on \"%s\": expected %s, got %s\n",
               _pattern,
               _subject,
               _expect ? "a match" : "no match",
               found ? "a match" : "no match");
    }
    else if ( (_expect)                 &&
              ((match.start != _start)  ||
               (match.length != _length)) )
    {
        g_failed++;
        printf("  FAIL  /%s/ on \"%s\": expected (%zu,%zu), got (%zu,%zu)\n",
               _pattern,
               _subject,
               _start,
               _length,
               match.start,
               match.length);
    }

    d_regex_free(regex);

    return;
}


/*
d_tests_expect_error
  Asserts that a pattern is rejected at compile time.

Parameter(s):
  _pattern: the pattern under test.
Return:
  none.
*/
static void
d_tests_expect_error(
    const char* _pattern
)
{
    enum d_regex_status status = D_REGEX_OK;
    struct d_regex*     regex  = d_regex_compile(_pattern, &status);

    g_checks++;

    // the pattern should not have compiled
    if (regex)
    {
        g_failed++;
        printf("  FAIL  /%s/ compiled but should not have\n", _pattern);
        d_regex_free(regex);
    }

    return;
}


/*
d_tests_leftmost_longest
  brief description
  Tests the following:
  - alternation prefers the longest branch, not the first
  - the leftmost start wins even when a later start matches more
  - concatenated alternations maximise the whole match
  - repetition is greedy across an alternation boundary
*/
static void
d_tests_leftmost_longest(void)
{
    printf("leftmost-longest\n");

    // a backtracking engine reports (0,1) for each of the next three
    d_tests_expect("a|ab",        "abc",   true, 0, 2);
    d_tests_expect("a|ab|abc",    "abcd",  true, 0, 3);
    d_tests_expect("(a|ab)(c|bcd)", "abcd", true, 0, 4);

    // the leftmost start wins regardless of the length available later
    d_tests_expect("a+",          "aa baaa", true, 0, 2);
    d_tests_expect("x|yyy",       "zyyy",    true, 1, 3);

    // an empty branch still permits a longer alternative to win
    d_tests_expect("|ab",         "ab",      true, 0, 2);
    d_tests_expect("(a*)(ab)?",   "aab",     true, 0, 3);

    return;
}


/*
d_tests_basics
  brief description
  Tests the following:
  - literal text, the wildcard, and simple repetition
  - the wildcard matches a newline, as POSIX requires
  - anchors bind to the ends of the subject only
  - an empty match is distinguished from no match
*/
static void
d_tests_basics(void)
{
    printf("basics\n");

    d_tests_expect("abc",   "xxabcyy", true,  2, 3);
    d_tests_expect("abc",   "xxabyy",  false, 0, 0);
    d_tests_expect("a.c",   "xabcx",   true,  1, 3);
    d_tests_expect("a.c",   "xa\ncx",  true,  1, 3);
    d_tests_expect("^abc",  "abcx",    true,  0, 3);
    d_tests_expect("^abc",  "xabc",    false, 0, 0);
    d_tests_expect("abc$",  "xabc",    true,  1, 3);
    d_tests_expect("abc$",  "abcx",    false, 0, 0);
    d_tests_expect("^$",    "",        true,  0, 0);
    d_tests_expect("^a*$",  "aaa",     true,  0, 3);
    d_tests_expect("b*",    "aaa",     true,  0, 0);
    d_tests_expect("x*",    "",        true,  0, 0);

    return;
}


/*
d_tests_classes
  brief description
  Tests the following:
  - bracket ranges, negation, and a literal bracket in first position
  - POSIX named classes inside a bracket expression
  - a hyphen in final position is a literal member
  - escapes are honoured inside a bracket expression
*/
static void
d_tests_classes(void)
{
    printf("bracket expressions\n");

    d_tests_expect("[a-c]+",       "xxabcyy",  true,  2, 3);
    d_tests_expect("[^a-c]+",      "abcxyz",   true,  3, 3);
    d_tests_expect("[]]",          "x]y",      true,  1, 1);
    d_tests_expect("[[:digit:]]+", "ab1234cd", true,  2, 4);
    d_tests_expect("[[:space:]]",  "ab cd",    true,  2, 1);
    d_tests_expect("[[:alpha:]]+", "12abc34",  true,  2, 3);
    d_tests_expect("[a-]",         "x-y",      true,  1, 1);
    d_tests_expect("[\\t]",        "a\tb",     true,  1, 1);
    d_tests_expect("[^[:digit:]]", "123a",     true,  3, 1);

    return;
}


/*
d_tests_intervals
  brief description
  Tests the following:
  - exact, open and bounded repetition counts
  - a bounded interval still matches greedily
  - a malformed brace is treated as a literal rather than an error
*/
static void
d_tests_intervals(void)
{
    printf("intervals\n");

    d_tests_expect("a{3}",    "aaaaa",   true, 0, 3);
    d_tests_expect("a{2,}",   "aaaa",    true, 0, 4);
    d_tests_expect("a{2,3}",  "aaaaa",   true, 0, 3);
    d_tests_expect("a{0,2}",  "bbb",     true, 0, 0);
    d_tests_expect("(ab){2}", "ababab",  true, 0, 4);
    d_tests_expect("a{1,2}b", "xaab",    true, 1, 3);
    d_tests_expect("a{x}",    "a{x}",    true, 0, 4);

    return;
}


/*
d_tests_escapes
  brief description
  Tests the following:
  - C escapes resolve to their control characters
  - a backslash before a metacharacter yields the literal
  - octal escapes resolve to the corresponding byte
*/
static void
d_tests_escapes(void)
{
    printf("escapes\n");

    d_tests_expect("a\\tb",   "xa\tb",  true, 1, 3);
    d_tests_expect("a\\nb",   "a\nb",   true, 0, 3);
    d_tests_expect("a\\.b",   "axb",    false, 0, 0);
    d_tests_expect("a\\.b",   "a.b",    true, 0, 3);
    d_tests_expect("\\*",     "x*y",    true, 1, 1);
    d_tests_expect("\\101",   "xAy",    true, 1, 1);
    d_tests_expect("a\\\\b",  "a\\b",   true, 0, 3);

    return;
}


/*
d_tests_errors
  brief description
  Tests the following:
  - unbalanced parentheses are rejected
  - an unterminated bracket expression is rejected
  - a quantifier with no preceding atom is rejected
  - an inverted range is rejected
*/
static void
d_tests_errors(void)
{
    printf("rejections\n");

    d_tests_expect_error("(ab");
    d_tests_expect_error("ab)");
    d_tests_expect_error("[abc");
    d_tests_expect_error("*ab");
    d_tests_expect_error("+");
    d_tests_expect_error("[z-a]");
    d_tests_expect_error("a\\");

    return;
}


/*
d_tests_iteration
  brief description
  Tests the following:
  - repeated search from an advancing offset, as gsub performs it
  - a null match does not stall the scan
  - the count of non-overlapping matches matches awk's gsub return
*/
static void
d_tests_iteration(void)
{
    printf("iteration\n");

    enum d_regex_status status  = D_REGEX_OK;
    struct d_regex*     regex   = d_regex_compile("x*", &status);
    const char* const   subject = "abc";
    const size_t        length  = strlen(subject);

    g_checks++;

    // the engine must compile before the scan can run
    if (!regex)
    {
        g_failed++;
        printf("  FAIL  /x*/ did not compile\n");
        return;
    }

    size_t position = 0;
    int    matches  = 0;

    // scan as gsub does, advancing one byte past every empty match
    while (position <= length)
    {
        struct d_regex_match match = { 0, 0 };

        // stop when no further match is available
        if (!d_regex_search(regex, subject, length, position, &match))
        {
            break;
        }

        matches++;

        // an empty match must advance the cursor to terminate
        if (match.length == 0)
        {
            position = match.start + 1;
        }
        else
        {
            position = match.start + match.length;
        }
    }

    // gsub(/x*/, "-", "abc") replaces four times, giving "-a-b-c-"
    if (matches != 4)
    {
        g_failed++;
        printf("  FAIL  /x*/ over \"abc\": expected 4 matches, got %d\n",
               matches);
    }

    d_regex_free(regex);

    return;
}


/*
d_tests_offsets
  brief description
  Tests the following:
  - a search beginning past an earlier match finds the next one
  - an anchored match reports only what begins at the offset
*/
static void
d_tests_offsets(void)
{
    printf("offsets\n");

    enum d_regex_status status = D_REGEX_OK;
    struct d_regex*     regex  = d_regex_compile("ab", &status);
    const char* const   text   = "abxxab";

    g_checks += 3;

    // the engine must compile before the offsets can be probed
    if (!regex)
    {
        g_failed += 3;
        printf("  FAIL  /ab/ did not compile\n");
        return;
    }

    struct d_regex_match match = { 0, 0 };

    // a search from the middle skips the first occurrence
    if ( (!d_regex_search(regex, text, 6, 1, &match)) ||
         (match.start != 4) )
    {
        g_failed++;
        printf("  FAIL  /ab/ from offset 1: expected start 4\n");
    }

    size_t extent = 0;

    // an anchored probe succeeds exactly where the text begins with it
    if ( (!d_regex_match_at(regex, text, 6, 0, &extent)) ||
         (extent != 2) )
    {
        g_failed++;
        printf("  FAIL  /ab/ anchored at 0: expected length 2\n");
    }

    // an anchored probe fails where the text does not begin with it
    if (d_regex_match_at(regex, text, 6, 1, &extent))
    {
        g_failed++;
        printf("  FAIL  /ab/ anchored at 1: expected no match\n");
    }

    d_regex_free(regex);

    return;
}


/*
d_tests_expect_groups
  Compares a located group assignment against an expected list and records the
  outcome.  An expected start of D_REGEX_NO_MATCH asserts that the group did
  not participate.

Parameter(s):
  _pattern:  the pattern under test.
  _subject:  the subject text.
  _expected: alternating start and length values, one pair per group.
  _count:    the number of pairs, including the whole match at index zero.
Return:
  none.
*/
static void
d_tests_expect_groups(
    const char*   _pattern,
    const char*   _subject,
    const size_t* _expected,
    size_t        _count
)
{
    enum d_regex_status status = D_REGEX_OK;
    struct d_regex*     regex  = d_regex_compile(_pattern, &status);

    g_checks++;

    // a pattern that fails to compile cannot be evaluated
    if (!regex)
    {
        g_failed++;
        printf("  FAIL  /%s/ did not compile\n", _pattern);
        return;
    }

    struct d_regex_match groups[D_REGEX_GROUP_MAX + 1];

    status = d_regex_search_groups(regex,
                                   _subject,
                                   strlen(_subject),
                                   0,
                                   groups,
                                   _count);

    // the pass must complete before the assignment can be compared
    if (status != D_REGEX_OK)
    {
        g_failed++;
        printf("  FAIL  /%s/ on \"%s\": %s\n",
               _pattern,
               _subject,
               d_regex_status_text(status));
        d_regex_free(regex);
        return;
    }

    // compare each group against its expected extent
    for (size_t group = 0; group < _count; ++group)
    {
        const size_t want_start  = _expected[group * 2u];
        const size_t want_length = _expected[(group * 2u) + 1u];

        if ( (groups[group].start != want_start) ||
             ((want_start != D_REGEX_NO_MATCH) &&
              (groups[group].length != want_length)) )
        {
            g_failed++;
            printf("  FAIL  /%s/ on \"%s\" group %zu: "
                   "expected (%zd,%zu), got (%zd,%zu)\n",
                   _pattern,
                   _subject,
                   group,
                   (ssize_t)want_start,
                   want_length,
                   (ssize_t)groups[group].start,
                   groups[group].length);
            break;
        }
    }

    d_regex_free(regex);

    return;
}


/*
d_tests_captures
  brief description
  Tests the following:
  - group extents are reported for simple and nested groups
  - a group inside an unmatched alternative is reported as absent
  - the last iteration of a repeated group is the one reported
  - POSIX preference is applied, not the first parse a greedy walk finds
  - a pattern with no groups still reports the whole match at index zero
*/
static void
d_tests_captures(void)
{
    printf("capture extents\n");

    static const size_t simple[]  = { 0, 2,  0, 1,  1, 1 };
    static const size_t nested[]  = { 1, 3,  1, 2,  1, 1,  2, 1 };
    static const size_t absent[]  = { 0, 1,  0, 1,  D_REGEX_NO_MATCH, 0 };
    static const size_t repeat[]  = { 0, 6,  4, 2 };
    static const size_t whole[]   = { 2, 3 };

    d_tests_expect_groups("(a)(b)",     "ab",   simple, 3);
    d_tests_expect_groups("((a)(b))c",  "xabc", nested, 4);
    d_tests_expect_groups("(a)|(b)",    "a",    absent, 3);
    d_tests_expect_groups("(ab){3}",    "ababab", repeat, 2);
    d_tests_expect_groups("abc",        "xxabc", whole,  1);

    // GNU regex, which gawk uses, prefers the earlier alternative rather than
    // the longest one, so /(a|ab)(b?)/ on "ab" gives ("a", "b").  Strict POSIX
    // would give ("ab", ""); gensub compatibility decides the case.
    static const size_t gnu_pref[] = { 0, 2,  0, 1,  1, 1 };

    d_tests_expect_groups("(a|ab)(b?)", "ab", gnu_pref, 3);

    // a repeated group reports its last iteration, and an iteration that
    // consumes nothing ends the loop rather than spinning at the subject end
    static const size_t last_iter[]  = { 0, 3,  2, 1 };
    static const size_t empty_iter[] = { 0, 2,  1, 1 };

    d_tests_expect_groups("(a)*",  "aaa", last_iter,  2);
    d_tests_expect_groups("(a?)*", "aa",  empty_iter, 2);

    // the classic POSIX example: the whole match wins, then groups left to
    // right; group 1 cannot be "ab" because no branch of group 2 matches "cd"
    static const size_t classic[] = { 0, 4,  0, 1,  1, 3 };

    d_tests_expect_groups("(a|ab)(c|bcd)", "abcd", classic, 3);

    // an empty iteration is kept only when no iteration consumed
    static const size_t only_empty[] = { 0, 1,  0, 0 };
    static const size_t plus_last[]  = { 0, 3,  1, 1 };

    d_tests_expect_groups("(a{0,1})*b",   "b",    only_empty, 2);
    d_tests_expect_groups("([abc]|a?)+b", "aabc", plus_last,  2);

    // Bounded repetition around a group that can match empty is the one place
    // this engine and GNU regex still part company, and GNU is not itself
    // consistent there: it skips the empty optional copy of
    // /(b{0,0}[^c]?){1,2}/ but takes one in /[^c](b|a?){1,3}c/.  These two pin
    // the present behaviour so that a change is noticed rather than assumed.
    static const size_t bounded_skip[] = { 0, 1,  0, 1 };
    static const size_t bounded_take[] = { 0, 4,  2, 1 };

    d_tests_expect_groups("(b{0,0}[^c]?){1,2}b*", "accacc", bounded_skip, 2);
    d_tests_expect_groups("[^c](b|a?){1,3}c",     "abbccb", bounded_take, 2);

    // an unmatched pattern reports OK with an absent whole match
    enum d_regex_status  status = D_REGEX_OK;
    struct d_regex*      regex  = d_regex_compile("(z)", &status);
    struct d_regex_match groups[2];

    g_checks++;

    // the pattern must compile before the absent case can be probed
    if (!regex)
    {
        g_failed++;
        printf("  FAIL  /(z)/ did not compile\n");
        return;
    }

    if ( (d_regex_search_groups(regex, "ab", 2, 0, groups, 2) != D_REGEX_OK) ||
         (groups[0].start != D_REGEX_NO_MATCH) )
    {
        g_failed++;
        printf("  FAIL  /(z)/ on \"ab\": expected OK with an absent match\n");
    }

    d_regex_free(regex);

    return;
}


/*
d_tests_groups
  brief description
  Tests the following:
  - capturing groups are counted by opening parenthesis, including nested ones
  - compiling group boundaries does not alter the overall match
  - a pattern exceeding D_REGEX_GROUP_MAX is rejected
  - d_regex_search_groups fails loudly rather than reporting wrong extents
*/
static void
d_tests_groups(void)
{
    printf("capture groups\n");

    static const struct
    {
        const char* pattern;
        size_t      count;
    }
    counts[] =
    {
        { "abc",            0 },
        { "(a)(b)",         2 },
        { "((a)(b))",       3 },
        { "(a|(b))(c)",     3 },
        { "[(]a[)]",        0 }
    };

    // the group count must follow opening-parenthesis order
    const size_t total = (sizeof(counts) / sizeof(counts[0]));

    for (size_t index = 0; index < total; ++index)
    {
        enum d_regex_status status = D_REGEX_OK;
        struct d_regex*     regex  = d_regex_compile(counts[index].pattern,
                                                     &status);

        g_checks++;

        // a pattern that fails to compile cannot be counted
        if (!regex)
        {
            g_failed++;
            printf("  FAIL  /%s/ did not compile\n", counts[index].pattern);
            continue;
        }

        // compare the reported count against the expectation
        if (d_regex_group_count(regex) != counts[index].count)
        {
            g_failed++;
            printf("  FAIL  /%s/: expected %zu groups, got %zu\n",
                   counts[index].pattern,
                   counts[index].count,
                   d_regex_group_count(regex));
        }

        d_regex_free(regex);
    }

    // emitting group boundaries must not perturb the overall match
    d_tests_expect("(a|ab)",       "abc",  true, 0, 2);
    d_tests_expect("((a)(b))c",    "xabc", true, 1, 3);
    d_tests_expect("(a*)(b*)",     "aabb", true, 0, 4);

    // a pattern beyond the group limit is rejected rather than truncated
    char many[(D_REGEX_GROUP_MAX * 3) + 8];
    size_t used = 0;

    for (int group = 0; group < (D_REGEX_GROUP_MAX + 1); ++group)
    {
        many[used++] = '(';
        many[used++] = 'a';
        many[used++] = ')';
    }

    many[used] = '\0';
    d_tests_expect_error(many);

    return;
}


/*
main
  Runs every test group and reports the aggregate result.

Parameter(s):
  none.
Return:
  Zero when every check passed, and one otherwise.
*/
int
main(void)
{
    d_tests_leftmost_longest();
    d_tests_basics();
    d_tests_classes();
    d_tests_intervals();
    d_tests_escapes();
    d_tests_errors();
    d_tests_iteration();
    d_tests_offsets();
    d_tests_groups();
    d_tests_captures();

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
