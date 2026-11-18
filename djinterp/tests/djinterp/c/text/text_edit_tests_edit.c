/*******************************************************************************
* djinterp [test]                                         text_edit_tests_edit.c
*
* The text_edit suite: every block. A helper applies a set to a text and
* compares the result with what is expected, freeing it either way.
*
*
* path:      /tests/djinterp/c/text/text_edit_tests_edit.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
#include "./text_edit_tests.h"  // corresponding header

// std
#include <stdlib.h>  // free
#include <string.h>  // strlen, memcmp


/*
d_tests_text_edit_gives
  Applies _edit to _text and says whether the result is _expected, with its
length; hook and user as d_text_edit_apply takes them.
*/
static bool
d_tests_text_edit_gives(
    struct dj_text_edit*    _edit,
    const char*             _text,
    dj_text_edit_fn_between _between,
    void*                   _user,
    const char*             _expected
)
{
    size_t      length = 99u;
    char* const out    = d_text_edit_apply(_edit, _text, strlen(_text),
                                           _between, _user, &length);
    const bool  same   = ( (out != NULL)                         &&
                           (length == strlen(_expected))         &&
                           (memcmp(out, _expected, length) == 0) &&
                           (out[length] == '\0') );

    free(out);

    return same;
}

/*
d_tests_text_edit_space
  A hook: a space between two letters or digits, so that two words do not
run together; nothing elsewhere. Counts its calls in _user.
*/
static char
d_tests_text_edit_space(
    void* _user,
    char  _before,
    char  _after
)
{
    const bool word_before = ( ((_before >= 'a') && (_before <= 'z')) ||
                               ((_before >= '0') && (_before <= '9')) );
    const bool word_after  = ( ((_after >= 'a') && (_after <= 'z')) ||
                               ((_after >= '0') && (_after <= '9')) );

    ++*(int*)_user;

    return ( (word_before) && (word_after) ) ? ' ' : '\0';
}


/*
d_tests_text_edit_new
  A new set.
  Tests the following:
  - it has no changes
  - free takes NULL, an empty set, and a set holding changes
*/
bool
d_tests_text_edit_new(void)
{
    struct dj_text_edit* edit = d_text_edit_new();

    D_TEST_REQUIRE(edit != NULL);
    D_TEST_EXPECT(d_text_edit_count(edit) == 0u);
    D_TEST_EXPECT(d_text_edit_count(NULL) == 0u);
    d_text_edit_free(edit);
    d_text_edit_free(NULL);

    edit = d_text_edit_new();
    D_TEST_REQUIRE(edit != NULL);
    D_TEST_EXPECT(d_text_edit_add(edit, 0u, 1u, "x", 1u) ==
                  D_TEXT_EDIT_ADDED);
    d_text_edit_free(edit);

    return true;
}

/*
d_tests_text_edit_clear
  Clearing.
  Tests the following:
  - clear forgets every change, so a span once taken can be changed again
  - clear takes NULL
*/
bool
d_tests_text_edit_clear(void)
{
    struct dj_text_edit* edit = d_text_edit_new();

    D_TEST_REQUIRE(edit != NULL);
    D_TEST_EXPECT(d_text_edit_add(edit, 0u, 3u, "X", 1u) ==
                  D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_text_edit_add(edit, 4u, 5u, "Y", 1u) ==
                  D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_text_edit_count(edit) == 2u);
    d_text_edit_clear(edit);
    d_text_edit_clear(NULL);
    D_TEST_EXPECT(d_text_edit_count(edit) == 0u);
    D_TEST_EXPECT(d_text_edit_add(edit, 1u, 2u, "Z", 1u) ==
                  D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "abc def", NULL, NULL,
                                          "aZc def"));
    d_text_edit_free(edit);

    return true;
}

/*
d_tests_text_edit_overlaps
  Overlapping changes.
  Tests the following:
  - a replacement meeting another replacement is refused, from either side
  - an insertion strictly inside a replacement is refused, in either order
  - a refused change is not kept
*/
bool
d_tests_text_edit_overlaps(void)
{
    struct dj_text_edit* edit = d_text_edit_new();

    D_TEST_REQUIRE(edit != NULL);
    D_TEST_REQUIRE(d_text_edit_add(edit, 2u, 6u, "R", 1u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_text_edit_add(edit, 0u, 3u, "a", 1u) ==
                  D_TEXT_EDIT_OVERLAPS);
    D_TEST_EXPECT(d_text_edit_add(edit, 5u, 8u, "b", 1u) ==
                  D_TEXT_EDIT_OVERLAPS);
    D_TEST_EXPECT(d_text_edit_add(edit, 3u, 4u, "c", 1u) ==
                  D_TEXT_EDIT_OVERLAPS);
    D_TEST_EXPECT(d_text_edit_add(edit, 1u, 7u, "d", 1u) ==
                  D_TEXT_EDIT_OVERLAPS);
    D_TEST_EXPECT(d_text_edit_add(edit, 4u, 4u, "e", 1u) ==
                  D_TEXT_EDIT_OVERLAPS);
    D_TEST_EXPECT(d_text_edit_count(edit) == 1u);

    // an insertion made first, then a replacement around it
    d_text_edit_clear(edit);
    D_TEST_REQUIRE(d_text_edit_add(edit, 4u, 4u, "i", 1u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_text_edit_add(edit, 2u, 6u, "R", 1u) ==
                  D_TEXT_EDIT_OVERLAPS);
    D_TEST_EXPECT(d_text_edit_count(edit) == 1u);
    d_text_edit_free(edit);

    return true;
}

/*
d_tests_text_edit_touching
  Changes that touch without overlapping.
  Tests the following:
  - two replacements that meet end to start are both kept
  - insertions at a replacement's start and at its end are kept
  - two insertions at one place are both kept
*/
bool
d_tests_text_edit_touching(void)
{
    struct dj_text_edit* edit = d_text_edit_new();

    D_TEST_REQUIRE(edit != NULL);
    D_TEST_EXPECT(d_text_edit_add(edit, 2u, 4u, "AB", 2u) ==
                  D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_text_edit_add(edit, 4u, 6u, "CD", 2u) ==
                  D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_text_edit_add(edit, 2u, 2u, "<", 1u) ==
                  D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_text_edit_add(edit, 6u, 6u, ">", 1u) ==
                  D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_text_edit_add(edit, 6u, 6u, ">", 1u) ==
                  D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_text_edit_count(edit) == 5u);
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "01abcd89", NULL, NULL,
                                          "01<ABCD>>89"));
    d_text_edit_free(edit);

    return true;
}

/*
d_tests_text_edit_bad_arguments
  Changes that cannot be kept.
  Tests the following:
  - no set, or an end before its start, is D_TEXT_EDIT_OVERLAPS
  - NULL text with a length is D_TEXT_EDIT_OVERLAPS; NULL with none deletes
*/
bool
d_tests_text_edit_bad_arguments(void)
{
    struct dj_text_edit* edit = d_text_edit_new();

    D_TEST_REQUIRE(edit != NULL);
    D_TEST_EXPECT(d_text_edit_add(NULL, 0u, 1u, "x", 1u) ==
                  D_TEXT_EDIT_OVERLAPS);
    D_TEST_EXPECT(d_text_edit_add(edit, 3u, 2u, "x", 1u) ==
                  D_TEXT_EDIT_OVERLAPS);
    D_TEST_EXPECT(d_text_edit_add(edit, 0u, 1u, NULL, 1u) ==
                  D_TEXT_EDIT_OVERLAPS);
    D_TEST_EXPECT(d_text_edit_count(edit) == 0u);
    D_TEST_EXPECT(d_text_edit_add(edit, 0u, 1u, NULL, 0u) ==
                  D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "xyz", NULL, NULL, "yz"));
    d_text_edit_free(edit);

    return true;
}

/*
d_tests_text_edit_apply_none
  Nothing to apply.
  Tests the following:
  - an empty set, or none, gives a terminated copy of the text
  - an empty text gives an empty one
  - NULL text with a length gives NULL
*/
bool
d_tests_text_edit_apply_none(void)
{
    struct dj_text_edit* edit = d_text_edit_new();
    size_t               length = 7u;
    char*                out;

    D_TEST_REQUIRE(edit != NULL);
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "same", NULL, NULL, "same"));
    D_TEST_EXPECT(d_tests_text_edit_gives(NULL, "same", NULL, NULL, "same"));
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "", NULL, NULL, ""));

    out = d_text_edit_apply(edit, NULL, 3u, NULL, NULL, &length);
    D_TEST_EXPECT(out == NULL);
    D_TEST_EXPECT(length == 0u);
    free(out);
    d_text_edit_free(edit);

    return true;
}

/*
d_tests_text_edit_apply_kinds
  The three kinds of change.
  Tests the following:
  - a replacement puts its text in place of its span, longer or shorter
  - an insertion puts its text at its place
  - a deletion is a replacement with no text
  - every byte no change touched is the text's own
*/
bool
d_tests_text_edit_apply_kinds(void)
{
    struct dj_text_edit* edit = d_text_edit_new();

    D_TEST_REQUIRE(edit != NULL);
    D_TEST_REQUIRE(d_text_edit_add(edit, 4u, 9u, "slow", 4u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_REQUIRE(d_text_edit_add(edit, 10u, 10u, "red ", 4u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_REQUIRE(d_text_edit_add(edit, 0u, 4u, "", 0u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "the quick fox", NULL, NULL,
                                          "slow red fox"));
    d_text_edit_free(edit);

    return true;
}

/*
d_tests_text_edit_apply_order
  The order changes apply in.
  Tests the following:
  - changes made out of order apply in the text's order
  - insertions at one place apply in the order they were made
  - an insertion at a replacement's start goes before what replaces
*/
bool
d_tests_text_edit_apply_order(void)
{
    struct dj_text_edit* edit = d_text_edit_new();

    D_TEST_REQUIRE(edit != NULL);
    D_TEST_REQUIRE(d_text_edit_add(edit, 6u, 7u, "Z", 1u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_REQUIRE(d_text_edit_add(edit, 3u, 4u, "R", 1u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_REQUIRE(d_text_edit_add(edit, 3u, 3u, "1", 1u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_REQUIRE(d_text_edit_add(edit, 0u, 0u, "[", 1u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_REQUIRE(d_text_edit_add(edit, 3u, 3u, "2", 1u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "abcdefg", NULL, NULL,
                                          "[abc12RefZ"));
    d_text_edit_free(edit);

    return true;
}

/*
d_tests_text_edit_apply_between
  The hook between a change's text and the text beside it.
  Tests the following:
  - it is asked where an inserted word meets the text before and after it,
    and its space is put between two words
  - where it answers '\0', nothing is put between
  - it is not asked before a change at the text's start, after one at its
    end, or for a deletion
*/
bool
d_tests_text_edit_apply_between(void)
{
    struct dj_text_edit* edit  = d_text_edit_new();
    int                  calls = 0;

    D_TEST_REQUIRE(edit != NULL);

    // "a1" + "b" + "(c)": a space after 1, nothing before "("
    D_TEST_REQUIRE(d_text_edit_add(edit, 2u, 2u, "b", 1u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "a1(c)",
                                          d_tests_text_edit_space, &calls,
                                          "a1 b(c)"));
    D_TEST_EXPECT(calls == 2);

    // at the text's start and end the hook is not asked
    d_text_edit_clear(edit);
    calls = 0;
    D_TEST_REQUIRE(d_text_edit_add(edit, 0u, 0u, "x", 1u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_REQUIRE(d_text_edit_add(edit, 2u, 2u, "y", 1u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "ab",
                                          d_tests_text_edit_space, &calls,
                                          "x ab y"));
    D_TEST_EXPECT(calls == 2);

    // a deletion puts in no text, so there is no meeting to ask about
    d_text_edit_clear(edit);
    calls = 0;
    D_TEST_REQUIRE(d_text_edit_add(edit, 1u, 2u, "", 0u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "a b",
                                          d_tests_text_edit_space, &calls,
                                          "ab"));
    D_TEST_EXPECT(calls == 0);
    d_text_edit_free(edit);

    return true;
}

/*
d_tests_text_edit_apply_past_end
  A change past the text.
  Tests the following:
  - a replacement or insertion past the text's end gives NULL, length 0
  - one ending exactly at the end applies
*/
bool
d_tests_text_edit_apply_past_end(void)
{
    struct dj_text_edit* edit   = d_text_edit_new();
    size_t               length = 5u;
    char*                out;

    D_TEST_REQUIRE(edit != NULL);
    D_TEST_REQUIRE(d_text_edit_add(edit, 2u, 9u, "x", 1u) ==
                   D_TEXT_EDIT_ADDED);
    out = d_text_edit_apply(edit, "abc", 3u, NULL, NULL, &length);
    D_TEST_EXPECT(out == NULL);
    D_TEST_EXPECT(length == 0u);
    free(out);

    d_text_edit_clear(edit);
    D_TEST_REQUIRE(d_text_edit_add(edit, 4u, 4u, "x", 1u) ==
                   D_TEXT_EDIT_ADDED);
    out = d_text_edit_apply(edit, "abc", 3u, NULL, NULL, &length);
    D_TEST_EXPECT(out == NULL);
    free(out);

    d_text_edit_clear(edit);
    D_TEST_REQUIRE(d_text_edit_add(edit, 3u, 3u, "d", 1u) ==
                   D_TEXT_EDIT_ADDED);
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "abc", NULL, NULL, "abcd"));
    d_text_edit_free(edit);

    return true;
}

/*
d_tests_text_edit_apply_again
  The changes outlive an apply.
  Tests the following:
  - applying twice gives the same text, and keeps the count
  - the change's text is the set's copy: the caller's buffer may change
*/
bool
d_tests_text_edit_apply_again(void)
{
    struct dj_text_edit* edit      = d_text_edit_new();
    char                 buffer[4] = "new";

    D_TEST_REQUIRE(edit != NULL);
    D_TEST_REQUIRE(d_text_edit_add(edit, 0u, 3u, buffer, 3u) ==
                   D_TEXT_EDIT_ADDED);
    buffer[0] = 'X';
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "old text", NULL, NULL,
                                          "new text"));
    D_TEST_EXPECT(d_tests_text_edit_gives(edit, "old text", NULL, NULL,
                                          "new text"));
    D_TEST_EXPECT(d_text_edit_count(edit) == 1u);
    d_text_edit_free(edit);

    return true;
}
