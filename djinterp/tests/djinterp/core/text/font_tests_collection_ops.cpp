// djinterp [test]  font_tests_collection_ops.cpp
//   Section 9c: the free functions over the COLLECTION-shaped mixins -
// the OpenType feature list, the variable-font axis list, the script hint
// pair, and the backend handles.
//
//   These are the only operations in the header with real algorithmic
// content: both list setters are UPSERTS (replace-in-place if the tag is
// already present, append otherwise), and both getters return a sentinel
// when the tag is absent.  Every one of those decisions has an edge, and
// this section walks all of them.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_fn_set_opentype_feature_appends
  Tests the following:
  - the first feature is appended to an empty list
  - a second, distinct tag is appended beside it (not on top of it)
  - the stored tag and value are what was asked for
*/
bool
tests_fn_set_opentype_feature_appends()
{
    opentype_font f;

    D_FT_CHECK(f.opentype_features.empty());

    fn_set_opentype_feature(f, ot_tag("liga"), 1u);

    D_FT_CHECK(f.opentype_features.size() == 1u);
    D_FT_CHECK(f.opentype_features[0].tag   == ot_tag("liga"));
    D_FT_CHECK(f.opentype_features[0].value == 1u);

    fn_set_opentype_feature(f, ot_tag("kern"), 1u);

    D_FT_CHECK(f.opentype_features.size() == 2u);
    D_FT_CHECK(f.opentype_features[1].tag == ot_tag("kern"));

    // the first is still there
    D_FT_CHECK(f.opentype_features[0].tag == ot_tag("liga"));

    return true;
}

/*
tests_fn_set_opentype_feature_defaults_the_value_to_one
  The value parameter defaults to 1, i.e. ENABLED - which is what makes
`fn_set_opentype_feature(f, ot_tag("liga"))` read as "turn ligatures on".
  Tests the following:
  - the one-argument-plus-tag form stores value 1
  - fn_get_opentype_feature then reports 1
*/
bool
tests_fn_set_opentype_feature_defaults_the_value_to_one()
{
    opentype_font f;

    fn_set_opentype_feature(f, ot_tag("liga"));

    D_FT_CHECK(f.opentype_features.size() == 1u);
    D_FT_CHECK(f.opentype_features[0].value == 1u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("liga")) == 1u);

    return true;
}

/*
tests_fn_set_opentype_feature_upserts
  The setter scans for the tag first: an existing entry is REPLACED in
place, not duplicated.  A naive push_back would leave two entries with the
same tag, and the getter (which returns the first hit) would then report a
stale value forever.
  Tests the following:
  - setting a tag that is already present updates it
  - the list does NOT grow
  - the updated value is what the getter reports
  - repeated updates keep working
*/
bool
tests_fn_set_opentype_feature_upserts()
{
    opentype_font f;

    fn_set_opentype_feature(f, ot_tag("ss01"), 1u);

    D_FT_CHECK(f.opentype_features.size() == 1u);

    // same tag, new value
    fn_set_opentype_feature(f, ot_tag("ss01"), 7u);

    D_FT_CHECK(f.opentype_features.size() == 1u);        // no duplicate
    D_FT_CHECK(f.opentype_features[0].value == 7u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("ss01")) == 7u);

    // and again
    fn_set_opentype_feature(f, ot_tag("ss01"), 3u);

    D_FT_CHECK(f.opentype_features.size() == 1u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("ss01")) == 3u);

    return true;
}

/*
tests_fn_set_opentype_feature_preserves_insertion_order
  An upsert must not reorder: a feature updated in place stays where it was.
Shaping engines apply features in list order, so a setter that moved an
updated entry to the back would silently change the rendering.
  Tests the following:
  - three features land in insertion order
  - updating the MIDDLE one leaves it in the middle
  - updating the FIRST one leaves it first
*/
bool
tests_fn_set_opentype_feature_preserves_insertion_order()
{
    opentype_font f;

    fn_set_opentype_feature(f, ot_tag("liga"), 1u);
    fn_set_opentype_feature(f, ot_tag("kern"), 1u);
    fn_set_opentype_feature(f, ot_tag("calt"), 1u);

    D_FT_CHECK(f.opentype_features.size() == 3u);
    D_FT_CHECK(f.opentype_features[0].tag == ot_tag("liga"));
    D_FT_CHECK(f.opentype_features[1].tag == ot_tag("kern"));
    D_FT_CHECK(f.opentype_features[2].tag == ot_tag("calt"));

    // update the middle one - it stays in the middle
    fn_set_opentype_feature(f, ot_tag("kern"), 9u);

    D_FT_CHECK(f.opentype_features.size() == 3u);
    D_FT_CHECK(f.opentype_features[0].tag == ot_tag("liga"));
    D_FT_CHECK(f.opentype_features[1].tag == ot_tag("kern"));
    D_FT_CHECK(f.opentype_features[1].value == 9u);
    D_FT_CHECK(f.opentype_features[2].tag == ot_tag("calt"));

    // update the first one - it stays first
    fn_set_opentype_feature(f, ot_tag("liga"), 4u);

    D_FT_CHECK(f.opentype_features[0].tag   == ot_tag("liga"));
    D_FT_CHECK(f.opentype_features[0].value == 4u);
    D_FT_CHECK(f.opentype_features[2].tag   == ot_tag("calt"));

    return true;
}

/*
tests_fn_set_opentype_feature_zero_stores_a_disabled_entry
  Passing 0 as the value does NOT remove the feature - it stores an entry
whose value is 0, which OpenType reads as "explicitly disabled".  That is a
meaningful distinction from "absent": an explicitly disabled `liga` overrides
a font's own default-on ligatures, whereas an absent one leaves the default
in place.  fn_remove_opentype_feature is the call that actually deletes.
  Tests the following:
  - setting a value of 0 leaves the entry IN the list
  - the entry's value is 0
  - the list size is unchanged by the disable
  - fn_remove_opentype_feature is the only thing that shortens the list
*/
bool
tests_fn_set_opentype_feature_zero_stores_a_disabled_entry()
{
    opentype_font f;

    fn_set_opentype_feature(f, ot_tag("liga"), 1u);

    D_FT_CHECK(f.opentype_features.size() == 1u);

    // disable it - the entry SURVIVES with value 0
    fn_set_opentype_feature(f, ot_tag("liga"), 0u);

    D_FT_CHECK(f.opentype_features.size() == 1u);     // still there!
    D_FT_CHECK(f.opentype_features[0].tag   == ot_tag("liga"));
    D_FT_CHECK(f.opentype_features[0].value == 0u);

    // disabling a NEW tag appends a zero-valued entry
    fn_set_opentype_feature(f, ot_tag("calt"), 0u);

    D_FT_CHECK(f.opentype_features.size() == 2u);
    D_FT_CHECK(f.opentype_features[1].value == 0u);

    // removal is the only thing that shortens the list
    D_FT_CHECK(fn_remove_opentype_feature(f, ot_tag("liga")));
    D_FT_CHECK(f.opentype_features.size() == 1u);

    return true;
}

/*
tests_fn_get_opentype_feature_returns_zero_when_absent
  Tests the following:
  - a lookup on an empty list returns 0
  - a lookup for an unset tag on a populated list returns 0
*/
bool
tests_fn_get_opentype_feature_returns_zero_when_absent()
{
    opentype_font f;

    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("liga")) == 0u);

    fn_set_opentype_feature(f, ot_tag("kern"), 5u);

    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("kern")) == 5u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("liga")) == 0u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("smcp")) == 0u);

    return true;
}

/*
tests_fn_get_opentype_feature_conflates_absent_and_disabled
  The consequence of the previous two tests, spelled out because it is a
genuine ambiguity in the API rather than a bug in the tests.
fn_get_opentype_feature returns 0 for an ABSENT tag and 0 for an EXPLICITLY
DISABLED one - the two are indistinguishable through the getter alone.  A
caller that needs to tell them apart must inspect the vector directly.
  Tests the following:
  - the getter returns 0 for a tag that was never set
  - the getter returns 0 for a tag explicitly set to 0
  - the two fonts are NOT in the same state - the vectors differ - so the
    information exists; it is only the getter that flattens it
*/
bool
tests_fn_get_opentype_feature_conflates_absent_and_disabled()
{
    opentype_font absent;
    opentype_font disabled;

    fn_set_opentype_feature(disabled, ot_tag("liga"), 0u);

    // the getter cannot tell them apart
    D_FT_CHECK(fn_get_opentype_feature(absent,   ot_tag("liga")) == 0u);
    D_FT_CHECK(fn_get_opentype_feature(disabled, ot_tag("liga")) == 0u);
    D_FT_CHECK(fn_get_opentype_feature(absent,   ot_tag("liga")) ==
               fn_get_opentype_feature(disabled, ot_tag("liga")));

    // but the states genuinely differ, and the vector shows it
    D_FT_CHECK(absent.opentype_features.empty());
    D_FT_CHECK(disabled.opentype_features.size() == 1u);
    D_FT_CHECK(disabled.opentype_features[0].tag == ot_tag("liga"));

    // removal, by contrast, IS observable through the return value
    D_FT_CHECK(!fn_remove_opentype_feature(absent,   ot_tag("liga")));
    D_FT_CHECK(fn_remove_opentype_feature(disabled, ot_tag("liga")));

    return true;
}

/*
tests_fn_remove_opentype_feature_reports_the_hit
  Tests the following:
  - removing a present tag returns true
  - the entry is gone afterwards
  - the getter then reports 0
*/
bool
tests_fn_remove_opentype_feature_reports_the_hit()
{
    opentype_font f;

    fn_set_opentype_feature(f, ot_tag("liga"), 1u);

    D_FT_CHECK(f.opentype_features.size() == 1u);

    const bool removed = fn_remove_opentype_feature(f, ot_tag("liga"));

    D_FT_CHECK(removed);
    D_FT_CHECK(f.opentype_features.empty());
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("liga")) == 0u);

    return true;
}

/*
tests_fn_remove_opentype_feature_reports_the_miss
  Tests the following:
  - removing an absent tag returns false
  - removing from an empty list returns false and does not misbehave
  - the list is unchanged by a miss
*/
bool
tests_fn_remove_opentype_feature_reports_the_miss()
{
    opentype_font empty;

    D_FT_CHECK(!fn_remove_opentype_feature(empty, ot_tag("liga")));
    D_FT_CHECK(empty.opentype_features.empty());

    opentype_font f;

    fn_set_opentype_feature(f, ot_tag("kern"), 1u);

    const bool removed = fn_remove_opentype_feature(f, ot_tag("liga"));

    D_FT_CHECK(!removed);
    D_FT_CHECK(f.opentype_features.size() == 1u);      // untouched
    D_FT_CHECK(f.opentype_features[0].tag == ot_tag("kern"));

    // and removing the SAME tag twice: hit, then miss
    D_FT_CHECK(fn_remove_opentype_feature(f, ot_tag("kern")));
    D_FT_CHECK(!fn_remove_opentype_feature(f, ot_tag("kern")));

    return true;
}

/*
tests_fn_remove_opentype_feature_removes_only_the_match
  Tests the following:
  - removing one tag from a three-entry list leaves the other two
  - the survivors keep their values
*/
bool
tests_fn_remove_opentype_feature_removes_only_the_match()
{
    opentype_font f;

    fn_set_opentype_feature(f, ot_tag("liga"), 1u);
    fn_set_opentype_feature(f, ot_tag("kern"), 2u);
    fn_set_opentype_feature(f, ot_tag("calt"), 3u);

    D_FT_CHECK(fn_remove_opentype_feature(f, ot_tag("kern")));

    D_FT_CHECK(f.opentype_features.size() == 2u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("liga")) == 1u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("kern")) == 0u);   // gone
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("calt")) == 3u);

    return true;
}

/*
tests_fn_remove_opentype_feature_keeps_the_order
  Erasing from the middle of a vector shifts the tail down; the surviving
entries must keep their RELATIVE order, since shaping applies them in list
order.
  Tests the following:
  - removing the middle entry leaves the first and third in that order
  - removing the first leaves the rest in order
  - removing the last leaves the rest in order
*/
bool
tests_fn_remove_opentype_feature_keeps_the_order()
{
    // remove from the middle
    opentype_font mid;

    fn_set_opentype_feature(mid, ot_tag("aaaa"), 1u);
    fn_set_opentype_feature(mid, ot_tag("bbbb"), 2u);
    fn_set_opentype_feature(mid, ot_tag("cccc"), 3u);
    fn_set_opentype_feature(mid, ot_tag("dddd"), 4u);

    D_FT_CHECK(fn_remove_opentype_feature(mid, ot_tag("bbbb")));

    D_FT_CHECK(mid.opentype_features.size() == 3u);
    D_FT_CHECK(mid.opentype_features[0].tag == ot_tag("aaaa"));
    D_FT_CHECK(mid.opentype_features[1].tag == ot_tag("cccc"));
    D_FT_CHECK(mid.opentype_features[2].tag == ot_tag("dddd"));
    D_FT_CHECK(mid.opentype_features[1].value == 3u);

    // remove the first
    opentype_font head;

    fn_set_opentype_feature(head, ot_tag("aaaa"), 1u);
    fn_set_opentype_feature(head, ot_tag("bbbb"), 2u);
    fn_set_opentype_feature(head, ot_tag("cccc"), 3u);

    D_FT_CHECK(fn_remove_opentype_feature(head, ot_tag("aaaa")));

    D_FT_CHECK(head.opentype_features.size() == 2u);
    D_FT_CHECK(head.opentype_features[0].tag == ot_tag("bbbb"));
    D_FT_CHECK(head.opentype_features[1].tag == ot_tag("cccc"));

    // remove the last
    opentype_font tail;

    fn_set_opentype_feature(tail, ot_tag("aaaa"), 1u);
    fn_set_opentype_feature(tail, ot_tag("bbbb"), 2u);
    fn_set_opentype_feature(tail, ot_tag("cccc"), 3u);

    D_FT_CHECK(fn_remove_opentype_feature(tail, ot_tag("cccc")));

    D_FT_CHECK(tail.opentype_features.size() == 2u);
    D_FT_CHECK(tail.opentype_features[0].tag == ot_tag("aaaa"));
    D_FT_CHECK(tail.opentype_features[1].tag == ot_tag("bbbb"));

    return true;
}

/*
tests_fn_remove_opentype_feature_on_an_empty_list
  The vector is walked with begin()/end() iterators; on an empty list the
loop must simply not run, and the function must return false rather than
dereferencing anything.
  Tests the following:
  - the call is safe on a never-populated list
  - it is safe on a list emptied by a previous removal
*/
bool
tests_fn_remove_opentype_feature_on_an_empty_list()
{
    opentype_font f;

    D_FT_CHECK(f.opentype_features.empty());
    D_FT_CHECK(!fn_remove_opentype_feature(f, ot_tag("liga")));
    D_FT_CHECK(f.opentype_features.empty());

    // and after emptying it the hard way
    fn_set_opentype_feature(f, ot_tag("kern"), 1u);
    D_FT_CHECK(fn_remove_opentype_feature(f, ot_tag("kern")));
    D_FT_CHECK(f.opentype_features.empty());

    D_FT_CHECK(!fn_remove_opentype_feature(f, ot_tag("kern")));
    D_FT_CHECK(!fn_remove_opentype_feature(f, ot_tag("liga")));

    return true;
}

/*
tests_fn_opentype_features_scale
  The upsert is a linear scan; a list long enough to have a real interior is
worth exercising so the scan's bounds are actually walked.
  Tests the following:
  - twenty distinct stylistic sets land in order, one entry each
  - each is readable by tag
  - re-setting every one of them updates in place, with no growth
  - removing every other one leaves exactly the expected survivors
*/
bool
tests_fn_opentype_features_scale()
{
    opentype_font f;

    // ss01 .. ss20
    for (unsigned i = 1; i <= 20u; ++i)
    {
        char name[5] = { 's', 's', '0', '0', '\0' };

        name[2] = static_cast<char>('0' + static_cast<char>(i / 10u));
        name[3] = static_cast<char>('0' + static_cast<char>(i % 10u));

        fn_set_opentype_feature(f, ot_tag(name), i);
    }

    D_FT_CHECK(f.opentype_features.size() == 20u);
    D_FT_CHECK(f.opentype_features[0].value  == 1u);
    D_FT_CHECK(f.opentype_features[19].value == 20u);

    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("ss01")) == 1u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("ss11")) == 11u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("ss20")) == 20u);

    // update every one in place - no growth
    for (unsigned i = 1; i <= 20u; ++i)
    {
        char name[5] = { 's', 's', '0', '0', '\0' };

        name[2] = static_cast<char>('0' + static_cast<char>(i / 10u));
        name[3] = static_cast<char>('0' + static_cast<char>(i % 10u));

        fn_set_opentype_feature(f, ot_tag(name), i + 100u);
    }

    D_FT_CHECK(f.opentype_features.size() == 20u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("ss07")) == 107u);

    // remove the ten even-numbered ones
    for (unsigned i = 2; i <= 20u; i += 2u)
    {
        char name[5] = { 's', 's', '0', '0', '\0' };

        name[2] = static_cast<char>('0' + static_cast<char>(i / 10u));
        name[3] = static_cast<char>('0' + static_cast<char>(i % 10u));

        D_FT_CHECK(fn_remove_opentype_feature(f, ot_tag(name)));
    }

    D_FT_CHECK(f.opentype_features.size() == 10u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("ss01")) == 101u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("ss02")) == 0u);
    D_FT_CHECK(fn_get_opentype_feature(f, ot_tag("ss19")) == 119u);

    return true;
}

/*
tests_fn_set_variable_axis_appends
  Tests the following:
  - the first axis is appended to an empty list
  - a second, distinct axis is appended beside it
  - the stored tag and value are what was asked for
*/
bool
tests_fn_set_variable_axis_appends()
{
    variable_font_t f;

    D_FT_CHECK(f.variable_axes.empty());

    fn_set_variable_axis(f, ot_tag("wght"), 650.0f);

    D_FT_CHECK(f.variable_axes.size() == 1u);
    D_FT_CHECK(f.variable_axes[0].tag == ot_tag("wght"));
    D_FT_CHECK_NEAR(f.variable_axes[0].value, 650.0f);

    fn_set_variable_axis(f, ot_tag("wdth"), 87.5f);

    D_FT_CHECK(f.variable_axes.size() == 2u);
    D_FT_CHECK(f.variable_axes[1].tag == ot_tag("wdth"));
    D_FT_CHECK_NEAR(f.variable_axes[1].value, 87.5f);

    // the first survived
    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("wght")), 650.0f);

    return true;
}

/*
tests_fn_set_variable_axis_upserts
  Same upsert contract as the feature list: an axis already present is
updated in place.  Duplicating an axis would be worse here than for features
- two `wght` entries would leave a shaper picking one arbitrarily.
  Tests the following:
  - setting an axis that is already present updates it
  - the list does NOT grow
  - the update is visible through the getter
*/
bool
tests_fn_set_variable_axis_upserts()
{
    variable_font_t f;

    fn_set_variable_axis(f, ot_tag("wght"), 400.0f);

    D_FT_CHECK(f.variable_axes.size() == 1u);

    fn_set_variable_axis(f, ot_tag("wght"), 700.0f);

    D_FT_CHECK(f.variable_axes.size() == 1u);       // no duplicate
    D_FT_CHECK_NEAR(f.variable_axes[0].value, 700.0f);
    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("wght")), 700.0f);

    // and again, with a fractional coordinate
    fn_set_variable_axis(f, ot_tag("wght"), 512.5f);

    D_FT_CHECK(f.variable_axes.size() == 1u);
    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("wght")), 512.5f);

    return true;
}

/*
tests_fn_get_variable_axis_returns_the_default_when_absent
  Tests the following:
  - a lookup on an empty list returns the default (0.0f when unspecified)
  - a lookup for an unset axis on a populated list returns the default
*/
bool
tests_fn_get_variable_axis_returns_the_default_when_absent()
{
    variable_font_t f;

    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("wght")), 0.0f);

    fn_set_variable_axis(f, ot_tag("wdth"), 100.0f);

    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("wdth")), 100.0f);
    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("wght")), 0.0f);
    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("slnt")), 0.0f);

    return true;
}

/*
tests_fn_get_variable_axis_takes_a_caller_default
  Unlike the OpenType getter, this one lets the caller name the fallback -
which matters because 0.0f is a MEANINGFUL coordinate on most axes (0 slant,
0 optical size), so a hard-coded zero would be a bad answer to "what is this
font's slnt?".
  Tests the following:
  - the caller's default comes back for an absent axis
  - it comes back for several different defaults
  - the default has no effect when the axis IS present
*/
bool
tests_fn_get_variable_axis_takes_a_caller_default()
{
    variable_font_t f;

    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("wght"), 400.0f), 400.0f);
    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("wdth"), 100.0f), 100.0f);
    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("slnt"), -15.0f), -15.0f);

    // once the axis is set, the default is ignored
    fn_set_variable_axis(f, ot_tag("wght"), 250.0f);

    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("wght"), 400.0f), 250.0f);

    return true;
}

/*
tests_fn_get_variable_axis_stored_zero_beats_the_default
  The getter returns on the first tag match, so a STORED value of 0.0f is
returned as 0.0f - the caller's default does not override it.  That is the
right behaviour and it is the reason the caller-supplied default exists at
all: "absent" and "set to zero" really are different, and this getter, unlike
the OpenType one, keeps them apart.
  Tests the following:
  - an axis explicitly set to 0.0f returns 0.0f even when a nonzero default
    is supplied
  - an ABSENT axis with the same default returns the default
  - so the two states are distinguishable through the getter alone
*/
bool
tests_fn_get_variable_axis_stored_zero_beats_the_default()
{
    variable_font_t f;

    // slnt = 0 is a real coordinate: upright on the slant axis
    fn_set_variable_axis(f, ot_tag("slnt"), 0.0f);

    D_FT_CHECK(f.variable_axes.size() == 1u);
    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("slnt"), -15.0f), 0.0f);

    // an axis that was never set DOES fall back
    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("ital"), -15.0f), -15.0f);

    // the two are therefore distinguishable - unlike the OpenType getter
    D_FT_CHECK(!approx_eq(fn_get_variable_axis(f, ot_tag("slnt"), -15.0f),
                          fn_get_variable_axis(f, ot_tag("ital"), -15.0f)));

    return true;
}

/*
tests_fn_variable_axis_accepts_the_whole_float_range
  Axis coordinates are floats with per-axis ranges the MODEL does not know -
`slnt` is conventionally negative, `opsz` small, `wght` in the hundreds.  So
nothing may be clamped.
  Tests the following:
  - a negative coordinate is stored verbatim (slnt)
  - a large coordinate is stored verbatim (wght)
  - a small fractional coordinate is stored verbatim (opsz)
*/
bool
tests_fn_variable_axis_accepts_the_whole_float_range()
{
    variable_font_t f;

    fn_set_variable_axis(f, ot_tag("slnt"), -15.0f);
    fn_set_variable_axis(f, ot_tag("wght"), 1000.0f);
    fn_set_variable_axis(f, ot_tag("opsz"), 8.25f);
    fn_set_variable_axis(f, ot_tag("ital"), 1.0f);

    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("slnt")), -15.0f);
    D_FT_CHECK(fn_get_variable_axis(f, ot_tag("slnt")) < 0.0f);

    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("wght")), 1000.0f);
    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("opsz")), 8.25f);
    D_FT_CHECK_NEAR(fn_get_variable_axis(f, ot_tag("ital")), 1.0f);

    D_FT_CHECK(f.variable_axes.size() == 4u);

    return true;
}

/*
tests_fn_variable_axes_preserve_insertion_order
  Tests the following:
  - axes land in insertion order
  - an in-place update does not move an axis
*/
bool
tests_fn_variable_axes_preserve_insertion_order()
{
    variable_font_t f;

    fn_set_variable_axis(f, ot_tag("wght"), 400.0f);
    fn_set_variable_axis(f, ot_tag("wdth"), 100.0f);
    fn_set_variable_axis(f, ot_tag("slnt"), 0.0f);

    D_FT_CHECK(f.variable_axes[0].tag == ot_tag("wght"));
    D_FT_CHECK(f.variable_axes[1].tag == ot_tag("wdth"));
    D_FT_CHECK(f.variable_axes[2].tag == ot_tag("slnt"));

    // update the first - it stays first
    fn_set_variable_axis(f, ot_tag("wght"), 900.0f);

    D_FT_CHECK(f.variable_axes.size() == 3u);
    D_FT_CHECK(f.variable_axes[0].tag == ot_tag("wght"));
    D_FT_CHECK_NEAR(f.variable_axes[0].value, 900.0f);
    D_FT_CHECK(f.variable_axes[2].tag == ot_tag("slnt"));

    return true;
}

/*
tests_fn_set_script_sets_both_tags
  Tests the following:
  - the script tag and the language tag both land on the font
  - the values round-trip exactly (BCP-47 tags carry hyphens and case)
*/
bool
tests_fn_set_script_sets_both_tags()
{
    script_font f;

    D_FT_CHECK(f.script_tag.empty());
    D_FT_CHECK(f.language_tag.empty());

    fn_set_script(f, "latn", "en");

    D_FT_CHECK(f.script_tag   == "latn");
    D_FT_CHECK(f.language_tag == "en");

    // a full BCP-47 subtag chain survives
    fn_set_script(f, "hani", "zh-Hans-CN");

    D_FT_CHECK(f.script_tag   == "hani");
    D_FT_CHECK(f.language_tag == "zh-Hans-CN");

    return true;
}

/*
tests_fn_set_script_defaults_the_language_to_empty
  Tests the following:
  - calling with a script alone leaves the language tag empty
  - the script still lands
*/
bool
tests_fn_set_script_defaults_the_language_to_empty()
{
    script_font f;

    fn_set_script(f, "cyrl");

    D_FT_CHECK(f.script_tag == "cyrl");
    D_FT_CHECK(f.language_tag.empty());

    return true;
}

/*
tests_fn_set_script_wipes_a_previously_set_language
  The trap.  fn_set_script ALWAYS assigns both members, and the language
parameter defaults to an empty string - so calling it with a script alone
after having set a language SILENTLY ERASES that language.  The two tags are
not independently settable through this API; a caller changing only the
script must re-supply the language, or write the member directly.
  Tests the following:
  - set(script, language), then set(script) alone - the language is gone
  - re-supplying it restores it
  - assigning the member directly is the way to change one without the other
*/
bool
tests_fn_set_script_wipes_a_previously_set_language()
{
    script_font f;

    fn_set_script(f, "latn", "en-GB");

    D_FT_CHECK(f.script_tag   == "latn");
    D_FT_CHECK(f.language_tag == "en-GB");

    // "just change the script" - and the language quietly evaporates
    fn_set_script(f, "cyrl");

    D_FT_CHECK(f.script_tag == "cyrl");
    D_FT_CHECK(f.language_tag.empty());     // GONE

    // the caller has to say it again
    fn_set_script(f, "cyrl", "ru");

    D_FT_CHECK(f.language_tag == "ru");

    // or bypass the setter entirely
    f.script_tag = "grek";

    D_FT_CHECK(f.script_tag   == "grek");
    D_FT_CHECK(f.language_tag == "ru");     // preserved

    return true;
}

/*
tests_fn_set_file_path_sets_the_path_and_the_index
  Tests the following:
  - the path lands on the font
  - an explicit face index lands with it (TTC / OTC collections)
*/
bool
tests_fn_set_file_path_sets_the_path_and_the_index()
{
    backend_font f;

    D_FT_CHECK(f.file_path.empty());
    D_FT_CHECK(f.face_index == 0);

    fn_set_file_path(f, "/usr/share/fonts/Iosevka.ttc", 3);

    D_FT_CHECK(f.file_path  == "/usr/share/fonts/Iosevka.ttc");
    D_FT_CHECK(f.face_index == 3);

    return true;
}

/*
tests_fn_set_file_path_defaults_the_face_index_to_zero
  Tests the following:
  - calling with a path alone sets the index to 0 - the first face, the
    right answer for a plain .ttf / .otf
*/
bool
tests_fn_set_file_path_defaults_the_face_index_to_zero()
{
    backend_font f;

    fn_set_file_path(f, "/usr/share/fonts/Inter.ttf");

    D_FT_CHECK(f.file_path  == "/usr/share/fonts/Inter.ttf");
    D_FT_CHECK(f.face_index == 0);

    return true;
}

/*
tests_fn_set_file_path_resets_a_previously_set_face_index
  The same shape of trap as fn_set_script.  fn_set_file_path always assigns
BOTH members, and _face_index defaults to 0 - so pointing an existing font at
a new path without naming a face silently rewinds it to face 0.  For a font
that was on face 3 of a collection, that is a real behavioural change.
  Tests the following:
  - set(path, 3), then set(path) alone - the index falls back to 0
  - naming the index again restores it
*/
bool
tests_fn_set_file_path_resets_a_previously_set_face_index()
{
    backend_font f;

    fn_set_file_path(f, "/fonts/Iosevka.ttc", 3);

    D_FT_CHECK(f.face_index == 3);

    // "just change the path" - and the face index quietly rewinds
    fn_set_file_path(f, "/fonts/Iosevka-v2.ttc");

    D_FT_CHECK(f.file_path  == "/fonts/Iosevka-v2.ttc");
    D_FT_CHECK(f.face_index == 0);          // RESET

    // the caller has to say it again
    fn_set_file_path(f, "/fonts/Iosevka-v2.ttc", 3);

    D_FT_CHECK(f.face_index == 3);

    return true;
}

/*
tests_fn_backend_handles_without_setters_are_plain_members
  Only file_path/face_index get a setter; postscript_name, full_name, and
native_handle are written directly.  That is a deliberate asymmetry - they
are RESOLUTION RESULTS an adapter fills in, not inputs a caller supplies - so
the model exposes them as plain members.
  Tests the following:
  - postscript_name, full_name, and native_handle are assignable directly
  - fn_set_file_path leaves all three alone
  - a null native_handle is meaningful (unresolved) and round-trips
*/
bool
tests_fn_backend_handles_without_setters_are_plain_members()
{
    backend_font f;

    D_FT_CHECK(f.postscript_name.empty());
    D_FT_CHECK(f.full_name.empty());
    D_FT_CHECK(f.native_handle == nullptr);

    f.postscript_name = "Inter-SemiBold";
    f.full_name       = "Inter SemiBold";

    int  face_object  = 0;
    f.native_handle   = &face_object;

    D_FT_CHECK(f.postscript_name == "Inter-SemiBold");
    D_FT_CHECK(f.full_name       == "Inter SemiBold");
    D_FT_CHECK(f.native_handle   == &face_object);

    // the one setter there IS does not disturb them
    fn_set_file_path(f, "/fonts/Inter.ttf", 0);

    D_FT_CHECK(f.postscript_name == "Inter-SemiBold");
    D_FT_CHECK(f.full_name       == "Inter SemiBold");
    D_FT_CHECK(f.native_handle   == &face_object);
    D_FT_CHECK(f.file_path       == "/fonts/Inter.ttf");

    // and an unresolved handle is a legal state
    f.native_handle = nullptr;

    D_FT_CHECK(f.native_handle == nullptr);

    return true;
}

NS_END  // testing
NS_END  // djinterp
