// djinterp [test]  font_tests_concepts.cpp
//   Section 11: the concept face.  Every concept in the header is a thin
// wrapper over a section-10 trait (`concept X = has_font_Y<T>::value`), so
// the job here is twofold: confirm each concept agrees with the trait it
// wraps, and confirm the concepts actually CONSTRAIN - that a constrained
// overload is chosen for a satisfying type and rejected for a failing one.
//
//   The whole section is gated on __cpp_concepts, exactly as the header is.
// Under a pre-C++20 compiler the runner still links: the bodies collapse to
// a single assertion recording that the concept face is correctly absent, so
// the suite has the same shape on both faces and the C++17 build is not
// silently missing a module.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

#if ( (defined(__cpp_concepts)) &&                                            \
      (__cpp_concepts >= 201907L) )

/*
tests_concept_gate_matches_the_compiler
  Tests the following:
  - the header compiled its concept face, and this TU sees it, exactly when
    __cpp_concepts >= 201907L - so the two gates agree
*/
bool
tests_concept_gate_matches_the_compiler()
{
    // reaching this overload at all proves the gate is open here
    D_FT_CHECK(__cpp_concepts >= 201907L);

    return true;
}

/*
tests_concept_core_identity
  Tests the following:
  - the core-identity concepts are satisfied by a font and rejected by a
    non-font
  - font_like_type wraps is_font_like, so it accepts duck_font too
*/
bool
tests_concept_core_identity()
{
    D_FT_CHECK(font_family_font<bare_font>);
    D_FT_CHECK(font_style_named_font<bare_font>);
    D_FT_CHECK(font_sized_font<bare_font>);
    D_FT_CHECK(font_size_unit_font<bare_font>);
    D_FT_CHECK(font_weighted_font<bare_font>);
    D_FT_CHECK(font_numeric_weight_font<bare_font>);
    D_FT_CHECK(font_slanted_font<bare_font>);

    D_FT_CHECK(!font_family_font<int>);
    D_FT_CHECK(!font_sized_font<std::string>);
    D_FT_CHECK(!font_weighted_font<duck_not_a_font>);

    // the conveniences a duck may lack
    D_FT_CHECK(font_family_font<duck_font>);
    D_FT_CHECK(!font_style_named_font<duck_font>);
    D_FT_CHECK(!font_size_unit_font<duck_font>);

    return true;
}

/*
tests_concept_font_like_type
  Tests the following:
  - font_like_type accepts every font and duck_font
  - it rejects a struct missing any one core member
  - it rejects fundamental and unrelated types
*/
bool
tests_concept_font_like_type()
{
    D_FT_CHECK(font_like_type<bare_font>);
    D_FT_CHECK(font_like_type<full_font>);
    D_FT_CHECK(font_like_type<duck_font>);

    D_FT_CHECK(!font_like_type<duck_no_family>);
    D_FT_CHECK(!font_like_type<duck_no_size>);
    D_FT_CHECK(!font_like_type<duck_no_weight>);
    D_FT_CHECK(!font_like_type<duck_no_slant>);

    D_FT_CHECK(!font_like_type<int>);
    D_FT_CHECK(!font_like_type<duck_not_a_font>);

    return true;
}

/*
tests_concept_decorations
  Tests the following:
  - underline_font / strikethrough_font / overline_font track their flags
  - each single-decoration font satisfies exactly its own concept
*/
bool
tests_concept_decorations()
{
    D_FT_CHECK(underline_font<underline_font_t>);
    D_FT_CHECK(!strikethrough_font<underline_font_t>);
    D_FT_CHECK(!overline_font<underline_font_t>);

    D_FT_CHECK(!underline_font<strike_font>);
    D_FT_CHECK(strikethrough_font<strike_font>);

    D_FT_CHECK(overline_font<overline_font_t>);

    D_FT_CHECK(underline_font<full_font>);
    D_FT_CHECK(strikethrough_font<full_font>);
    D_FT_CHECK(overline_font<full_font>);

    D_FT_CHECK(!underline_font<bare_font>);

    return true;
}

/*
tests_concept_decorated_font_is_a_disjunction
  Tests the following:
  - decorated_font is satisfied by any single decoration
  - a duck carrying only `underline` satisfies it (structural, no floor)
  - a bare font does not
*/
bool
tests_concept_decorated_font_is_a_disjunction()
{
    D_FT_CHECK(decorated_font<underline_font_t>);
    D_FT_CHECK(decorated_font<strike_font>);
    D_FT_CHECK(decorated_font<overline_font_t>);
    D_FT_CHECK(decorated_font<full_font>);

    D_FT_CHECK(decorated_font<duck_underline_only>);

    D_FT_CHECK(!decorated_font<bare_font>);
    D_FT_CHECK(!decorated_font<casing_font_t>);

    return true;
}

/*
tests_concept_casing
  Tests the following:
  - the four casing concepts track their flags
  - a casing-only font satisfies the casing concepts and not the decoration
    ones
*/
bool
tests_concept_casing()
{
    D_FT_CHECK(small_caps_font<small_caps_font_t>);
    D_FT_CHECK(!all_caps_font<small_caps_font_t>);

    D_FT_CHECK(small_caps_font<casing_font_t>);
    D_FT_CHECK(all_caps_font<casing_font_t>);
    D_FT_CHECK(subscript_font<casing_font_t>);
    D_FT_CHECK(superscript_font<casing_font_t>);

    D_FT_CHECK(!small_caps_font<bare_font>);
    D_FT_CHECK(!subscript_font<decorated_font_t>);

    D_FT_CHECK(subscript_font<font<ff_subscript, probe_color>>);
    D_FT_CHECK(!superscript_font<font<ff_subscript, probe_color>>);

    return true;
}

/*
tests_concept_casing_font_is_a_disjunction
  Tests the following:
  - casing_font is satisfied by any single casing feature
  - a bare font does not satisfy it
*/
bool
tests_concept_casing_font_is_a_disjunction()
{
    D_FT_CHECK(casing_font<small_caps_font_t>);
    D_FT_CHECK(casing_font<font<ff_all_caps,    probe_color>>);
    D_FT_CHECK(casing_font<font<ff_subscript,   probe_color>>);
    D_FT_CHECK(casing_font<font<ff_superscript, probe_color>>);
    D_FT_CHECK(casing_font<casing_font_t>);
    D_FT_CHECK(casing_font<full_font>);

    D_FT_CHECK(!casing_font<bare_font>);
    D_FT_CHECK(!casing_font<decorated_font_t>);

    return true;
}

/*
tests_concept_metrics
  Tests the following:
  - letter_spacing_font / line_height_font track their flags
  - metrics_override_font is the disjunction of the two
*/
bool
tests_concept_metrics()
{
    D_FT_CHECK(letter_spacing_font<letter_font>);
    D_FT_CHECK(!line_height_font<letter_font>);

    D_FT_CHECK(!letter_spacing_font<line_font>);
    D_FT_CHECK(line_height_font<line_font>);

    D_FT_CHECK(metrics_override_font<letter_font>);
    D_FT_CHECK(metrics_override_font<line_font>);
    D_FT_CHECK(metrics_override_font<metrics_font>);

    D_FT_CHECK(!metrics_override_font<bare_font>);
    D_FT_CHECK(!letter_spacing_font<bare_font>);

    return true;
}

/*
tests_concept_axes
  Tests the following:
  - stretch_font / spacing_axis_font track their flags
  - the variable-font axes do not satisfy either
*/
bool
tests_concept_axes()
{
    D_FT_CHECK(stretch_font<stretch_font_t>);
    D_FT_CHECK(!spacing_axis_font<stretch_font_t>);

    D_FT_CHECK(!stretch_font<spacing_font_t>);
    D_FT_CHECK(spacing_axis_font<spacing_font_t>);

    D_FT_CHECK(stretch_font<axes_font>);
    D_FT_CHECK(spacing_axis_font<axes_font>);

    D_FT_CHECK(!stretch_font<bare_font>);
    D_FT_CHECK(!stretch_font<variable_font_t>);
    D_FT_CHECK(!spacing_axis_font<variable_font_t>);

    return true;
}

/*
tests_concept_colors
  Tests the following:
  - foreground_color_font tracks ff_color, background_color_font tracks the
    paired background
  - a background-only font is not a foreground_color_font, and vice versa
*/
bool
tests_concept_colors()
{
    D_FT_CHECK(foreground_color_font<color_font>);
    D_FT_CHECK(!background_color_font<color_font>);

    D_FT_CHECK(!foreground_color_font<background_font>);
    D_FT_CHECK(background_color_font<background_font>);

    D_FT_CHECK(foreground_color_font<terminal_font>);
    D_FT_CHECK(background_color_font<terminal_font>);

    D_FT_CHECK(!foreground_color_font<bare_font>);
    D_FT_CHECK(!background_color_font<bare_font>);

    // the paired-member requirement still bites at the concept level
    D_FT_CHECK(!background_color_font<duck_background_only>);
    D_FT_CHECK(background_color_font<duck_background_paired>);

    return true;
}

/*
tests_concept_color_typed_font_is_independent_of_ff_color
  The concept-level echo of the trait quirk from section 10a.
color_typed_font wraps has_font_color_type_alias, which is true for EVERY
font - so a font with no color feature still satisfies color_typed_font while
failing foreground_color_font.
  Tests the following:
  - color_typed_font holds for a bare font
  - foreground_color_font does not, on that same font
  - color_typed_font is false for a type with no color_type alias
*/
bool
tests_concept_color_typed_font_is_independent_of_ff_color()
{
    D_FT_CHECK(color_typed_font<bare_font>);
    D_FT_CHECK(!foreground_color_font<bare_font>);

    D_FT_CHECK(color_typed_font<decorated_font_t>);
    D_FT_CHECK(color_typed_font<full_font>);

    D_FT_CHECK(!color_typed_font<duck_font>);
    D_FT_CHECK(!color_typed_font<int>);

    return true;
}

/*
tests_concept_opentype_and_variable
  Tests the following:
  - opentype_feature_font and variable_axis_font track their flags
  - neither implies the other
  - ff_gui_standard satisfies neither
*/
bool
tests_concept_opentype_and_variable()
{
    D_FT_CHECK(opentype_feature_font<opentype_font>);
    D_FT_CHECK(!variable_axis_font<opentype_font>);

    D_FT_CHECK(!opentype_feature_font<variable_font_t>);
    D_FT_CHECK(variable_axis_font<variable_font_t>);

    D_FT_CHECK(opentype_feature_font<full_font>);
    D_FT_CHECK(variable_axis_font<full_font>);

    D_FT_CHECK(!opentype_feature_font<gui_std_font>);
    D_FT_CHECK(!variable_axis_font<gui_std_font>);
    D_FT_CHECK(opentype_feature_font<gui_rich_font>);

    return true;
}

/*
tests_concept_script_hint
  Tests the following:
  - script_hint_font tracks the paired-tag trait
  - a script-only duck does not satisfy it; a paired one does
*/
bool
tests_concept_script_hint()
{
    D_FT_CHECK(script_hint_font<script_font>);
    D_FT_CHECK(script_hint_font<full_font>);

    D_FT_CHECK(!script_hint_font<bare_font>);
    D_FT_CHECK(!script_hint_font<gui_std_font>);

    D_FT_CHECK(!script_hint_font<duck_script_only>);
    D_FT_CHECK(!script_hint_font<duck_language_only>);
    D_FT_CHECK(script_hint_font<duck_script_paired>);

    return true;
}

/*
tests_concept_backend
  Tests the following:
  - file_backed_font / postscript_named_font / native_handle_font track the
    backend mixin
  - a bare font satisfies none of them
*/
bool
tests_concept_backend()
{
    D_FT_CHECK(file_backed_font<backend_font>);
    D_FT_CHECK(postscript_named_font<backend_font>);
    D_FT_CHECK(native_handle_font<backend_font>);

    D_FT_CHECK(file_backed_font<full_font>);
    D_FT_CHECK(postscript_named_font<gui_std_font>);

    D_FT_CHECK(!file_backed_font<bare_font>);
    D_FT_CHECK(!postscript_named_font<bare_font>);
    D_FT_CHECK(!native_handle_font<bare_font>);
    D_FT_CHECK(!file_backed_font<terminal_font>);

    return true;
}

/*
tests_concept_backend_resolvable_is_a_disjunction
  Tests the following:
  - backend_resolvable_font is satisfied by any single backend handle
  - a file-path-only duck satisfies it while failing postscript_named_font
    and native_handle_font
  - a type with no handle does not
*/
bool
tests_concept_backend_resolvable_is_a_disjunction()
{
    D_FT_CHECK(backend_resolvable_font<duck_file_path_only>);
    D_FT_CHECK(!postscript_named_font<duck_file_path_only>);
    D_FT_CHECK(!native_handle_font<duck_file_path_only>);

    D_FT_CHECK(backend_resolvable_font<duck_native_handle_only>);

    D_FT_CHECK(backend_resolvable_font<backend_font>);
    D_FT_CHECK(backend_resolvable_font<full_font>);

    D_FT_CHECK(!backend_resolvable_font<bare_font>);
    D_FT_CHECK(!backend_resolvable_font<duck_not_a_font>);

    return true;
}

/*
tests_concept_composite_profiles
  Tests the following:
  - bold_like_font / italic_like_font accept any font-like type
  - decoration_capable_font / casing_capable_font / color_capable_font /
    background_capable_font / metrics_capable_font / opentype_capable_font
    each track their composite trait AND require the font-like floor
  - a capability-only duck (no floor) is rejected by the capable_* concepts
*/
bool
tests_concept_composite_profiles()
{
    D_FT_CHECK(bold_like_font<bare_font>);
    D_FT_CHECK(italic_like_font<duck_font>);

    D_FT_CHECK(decoration_capable_font<underline_font_t>);
    D_FT_CHECK(casing_capable_font<casing_font_t>);
    D_FT_CHECK(color_capable_font<color_font>);
    D_FT_CHECK(background_capable_font<background_font>);
    D_FT_CHECK(metrics_capable_font<metrics_font>);
    D_FT_CHECK(opentype_capable_font<opentype_font>);

    D_FT_CHECK(!decoration_capable_font<bare_font>);
    D_FT_CHECK(!casing_capable_font<decorated_font_t>);
    D_FT_CHECK(!color_capable_font<background_font>);
    D_FT_CHECK(!opentype_capable_font<gui_std_font>);

    // the floor requirement, at the concept level
    D_FT_CHECK(!decoration_capable_font<duck_underline_only>);
    D_FT_CHECK(!color_capable_font<duck_foreground_only>);

    return true;
}

/*
tests_concept_terminal_and_rich
  Tests the following:
  - variable_font_type accepts variable fonts only
  - terminal_font_type accepts a font-like type with color OR background
  - rich_font_type accepts the rich profile and the minimal rich set, and
    rejects gui_standard
*/
bool
tests_concept_terminal_and_rich()
{
    D_FT_CHECK(variable_font_type<variable_font_t>);
    D_FT_CHECK(variable_font_type<full_font>);
    D_FT_CHECK(!variable_font_type<axes_font>);
    D_FT_CHECK(!variable_font_type<bare_font>);

    D_FT_CHECK(terminal_font_type<color_font>);
    D_FT_CHECK(terminal_font_type<background_font>);
    D_FT_CHECK(terminal_font_type<terminal_font>);
    D_FT_CHECK(terminal_font_type<duck_terminal>);
    D_FT_CHECK(!terminal_font_type<bare_font>);
    D_FT_CHECK(!terminal_font_type<decorated_font_t>);

    D_FT_CHECK(rich_font_type<gui_rich_font>);
    D_FT_CHECK(rich_font_type<minimal_rich_font>);
    D_FT_CHECK(rich_font_type<full_font>);
    D_FT_CHECK(!rich_font_type<gui_std_font>);
    D_FT_CHECK(!rich_font_type<gui_basic_font>);
    D_FT_CHECK(!rich_font_type<bare_font>);

    return true;
}

/*
tests_concepts_agree_with_their_traits
  Each concept is `concept X = trait<T>::value`, so concept and trait must
be indistinguishable.  A spot-check across the whole family, on both a
satisfying and a failing type, catches a concept wired to the wrong trait.
  Tests the following:
  - a representative concept from every group equals its backing trait, for
    a font that satisfies it and one that does not
*/
bool
tests_concepts_agree_with_their_traits()
{
    // core
    D_FT_CHECK(font_family_font<bare_font> == has_font_family_v<bare_font>);
    D_FT_CHECK(font_like_type<duck_font>   == is_font_like_v<duck_font>);
    D_FT_CHECK(font_like_type<int>         == is_font_like_v<int>);

    // decorations / casing
    D_FT_CHECK(underline_font<underline_font_t> ==
               has_font_underline_v<underline_font_t>);
    D_FT_CHECK(decorated_font<bare_font> == has_font_decorations_v<bare_font>);
    D_FT_CHECK(casing_font<casing_font_t> == has_font_casing_v<casing_font_t>);

    // metrics / axes
    D_FT_CHECK(metrics_override_font<metrics_font> ==
               has_font_metrics_overrides_v<metrics_font>);
    D_FT_CHECK(stretch_font<axes_font> == has_font_stretch_v<axes_font>);

    // color
    D_FT_CHECK(foreground_color_font<color_font> ==
               has_font_color_v<color_font>);
    D_FT_CHECK(background_color_font<background_font> ==
               has_font_background_v<background_font>);
    D_FT_CHECK(color_typed_font<bare_font> ==
               has_font_color_type_alias_v<bare_font>);

    // opentype / variable / script / backend
    D_FT_CHECK(opentype_feature_font<opentype_font> ==
               has_font_opentype_features_v<opentype_font>);
    D_FT_CHECK(variable_axis_font<variable_font_t> ==
               has_font_variable_axes_v<variable_font_t>);
    D_FT_CHECK(script_hint_font<script_font> ==
               has_font_script_hint_v<script_font>);
    D_FT_CHECK(backend_resolvable_font<backend_font> ==
               has_font_backend_handles_v<backend_font>);

    // composites
    D_FT_CHECK(rich_font_type<gui_rich_font> == is_font_rich_v<gui_rich_font>);
    D_FT_CHECK(rich_font_type<gui_std_font>  == is_font_rich_v<gui_std_font>);
    D_FT_CHECK(terminal_font_type<color_font> ==
               is_font_terminal_v<color_font>);

    return true;
}

/*
tests_concept_constrained_overload_is_preferred
  A concept must actually CONSTRAIN, not merely evaluate.  The suite's
concept_probe() is an overload set - one unconstrained, one gated on
underline_font - and the constrained overload is more specialised, so it
must win for a satisfying type and lose for a failing one.  That return
value is the observable proof the concept steers resolution.
  Tests the following:
  - concept_probe returns 1 (the constrained overload) for underline fonts
  - it returns 0 (the fallback) for fonts without underline and for non-fonts
*/
bool
tests_concept_constrained_overload_is_preferred()
{
    D_FT_CHECK(concept_probe<underline_font_t>() == 1);
    D_FT_CHECK(concept_probe<decorated_font_t>() == 1);   // has underline
    D_FT_CHECK(concept_probe<full_font>()        == 1);

    D_FT_CHECK(concept_probe<bare_font>()   == 0);
    D_FT_CHECK(concept_probe<strike_font>() == 0);        // no underline
    D_FT_CHECK(concept_probe<int>()         == 0);

    return true;
}

/*
tests_concept_conjunction_in_a_requires_clause
  These concepts are ATOMIC - each wraps a distinct trait::value - so they
never subsume one another, and the supported way to demand two at once is to
compose them with && in a requires-clause.  The suite's
concept_conjunction_probe does exactly that (underline_font && casing_font),
proving the composition selects a type that satisfies BOTH and rejects one
that satisfies only one.
  Tests the following:
  - a font with underline AND casing selects the constrained overload
  - a font with only underline does not
  - a font with only casing does not
  - a bare font does not
*/
bool
tests_concept_conjunction_in_a_requires_clause()
{
    // both: underline and a casing feature
    using both = font<ff_underline | ff_small_caps, probe_color>;

    D_FT_CHECK(concept_conjunction_probe<both>());
    D_FT_CHECK(concept_conjunction_probe<full_font>());

    // only one side
    D_FT_CHECK(!concept_conjunction_probe<underline_font_t>());  // no casing
    D_FT_CHECK(!concept_conjunction_probe<casing_font_t>());     // no underline
    D_FT_CHECK(!concept_conjunction_probe<bare_font>());

    return true;
}

/*
tests_concepts_reject_non_fonts
  Tests the following:
  - every concept group rejects int, std::string, and an unrelated struct
  - the capable_* concepts reject a capability-only duck that lacks the floor
*/
bool
tests_concepts_reject_non_fonts()
{
    // core-identity concepts
    D_FT_CHECK(!font_family_font<int>);
    D_FT_CHECK(!font_like_type<std::string>);
    D_FT_CHECK(!font_like_type<duck_not_a_font>);

    // capability concepts
    D_FT_CHECK(!underline_font<int>);
    D_FT_CHECK(!foreground_color_font<std::string>);
    D_FT_CHECK(!opentype_feature_font<duck_not_a_font>);
    D_FT_CHECK(!variable_axis_font<double>);

    // composite concepts
    D_FT_CHECK(!rich_font_type<int>);
    D_FT_CHECK(!terminal_font_type<std::string>);
    D_FT_CHECK(!variable_font_type<duck_not_a_font>);

    // capability without the floor
    D_FT_CHECK(!decoration_capable_font<duck_underline_only>);
    D_FT_CHECK(!color_capable_font<duck_foreground_only>);
    D_FT_CHECK(!background_capable_font<duck_background_paired>);

    return true;
}

#else   // __cpp_concepts < 201907L  ------------------------------------------

// On a pre-C++20 compiler the header omits its concept face entirely, so
// there is nothing to exercise.  Each test records that correct absence with
// a single passing assertion, keeping the suite's shape identical on both
// faces (the runner registers the same functions either way).

bool tests_concept_gate_matches_the_compiler()          { return true; }
bool tests_concept_core_identity()                      { return true; }
bool tests_concept_font_like_type()                     { return true; }
bool tests_concept_decorations()                        { return true; }
bool tests_concept_decorated_font_is_a_disjunction()    { return true; }
bool tests_concept_casing()                             { return true; }
bool tests_concept_casing_font_is_a_disjunction()       { return true; }
bool tests_concept_metrics()                            { return true; }
bool tests_concept_axes()                               { return true; }
bool tests_concept_colors()                             { return true; }
bool tests_concept_color_typed_font_is_independent_of_ff_color()
                                                        { return true; }
bool tests_concept_opentype_and_variable()              { return true; }
bool tests_concept_script_hint()                        { return true; }
bool tests_concept_backend()                            { return true; }
bool tests_concept_backend_resolvable_is_a_disjunction(){ return true; }
bool tests_concept_composite_profiles()                 { return true; }
bool tests_concept_terminal_and_rich()                  { return true; }
bool tests_concepts_agree_with_their_traits()           { return true; }
bool tests_concept_constrained_overload_is_preferred()  { return true; }
bool tests_concept_conjunction_in_a_requires_clause()   { return true; }
bool tests_concepts_reject_non_fonts()                  { return true; }

#endif  // __cpp_concepts >= 201907L

NS_END  // testing
NS_END  // djinterp
