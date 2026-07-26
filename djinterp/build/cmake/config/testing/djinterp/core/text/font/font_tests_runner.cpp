// djinterp [test]  font_tests_runner.cpp
//   Entry point for the font.hpp suite: registers every section's tests with
//   report_builder, one module per like-group semantic section of the header.

// djinterp
#include "font_tests.hpp"
#include "../../../../test/output/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- font.hpp Test Suite");
    rb.set_subtitle("text : an abstract, framework-agnostic typography model "
                    "with feature-gated EBO mixins");
    rb.set_author("teer");
    rb.set_description(
        "font.hpp is a compile-time-configurable data aggregate: a fixed "
        "core -- family, size, weight, slant -- surrounded by seventeen "
        "optional capabilities, each gated by a bit in font_feat and carried "
        "by an EBO mixin whose disabled face is empty. The suite is built to "
        "hold that design to account from every side. The feature bits are "
        "pinned as the wire-level ABI they are (they mirror OS/2 "
        "usWeightClass and usWidthClass), and the mixins are proven "
        "zero-cost in the one way no ABI can dispute -- by holding the mixin "
        "arrangement fixed and varying only the color payload, so a font "
        "that disables color is shown to be byte-identical whether its color "
        "type weighs three bytes or sixty-four. The free functions get their "
        "genuine edges walked: the two-track weight where a numeric override "
        "silently wins over the enum; the upsert-or-append list setters; the "
        "setters that quietly reset a paired field (fn_set_script wiping a "
        "language, fn_set_file_path rewinding a face index); and "
        "fn_convert_size, the one function with real branching, whose every "
        "switch arm, both em-guard arms, and two opaque device_units early "
        "returns are exercised, with the asymmetry between its guarded target "
        "side and its unguarded source side made explicit. The traits in "
        "section 10 are STRUCTURAL, so they are driven not only by font<> "
        "across its whole feature lattice but by duck-typed probes that share "
        "nothing with font<> but a member name -- the only witnesses that can "
        "tell a conjunction (has_font_background, has_font_script_hint) from "
        "a single detector, or a disjunction (has_font_decorations, "
        "has_font_backend_handles) from an AND. The SFINAE helpers and the "
        "C++20 concepts are tested through real overload sets, so each is "
        "shown to STEER resolution rather than merely evaluate. Verified "
        "under C++17 and C++20.");

    rb.use_pdf("font_tests_report.pdf");

    // -- 1. feature flags -------------------------------------------------
    rb.module("1. feature flags (font_feat)",
              "the seventeen capability bits, the aggregates and profiles, "
              "operator| and has_ff");
    rb.run("ff_none is the zero bit set",               &tests_ff_none_is_zero);
    rb.run("each flag holds its assigned bit",           &tests_ff_individual_bit_values);
    rb.run("flags are distinct powers of two",           &tests_ff_flags_are_distinct_powers_of_two);
    rb.run("font_feat is backed by unsigned",            &tests_ff_underlying_type_is_unsigned);
    rb.run("the bits fit in unsigned",                   &tests_ff_bits_fit_in_unsigned);
    rb.run("ff_decorations aggregate",                   &tests_ff_decorations_aggregate);
    rb.run("ff_casing aggregate",                        &tests_ff_casing_aggregate);
    rb.run("ff_metrics aggregate",                       &tests_ff_metrics_aggregate);
    rb.run("ff_axes aggregate (not variable axes)",      &tests_ff_axes_aggregate);
    rb.run("terminal profiles",                          &tests_ff_terminal_profiles);
    rb.run("gui profiles nest",                          &tests_ff_gui_profiles);
    rb.run("gui_standard omits opentype + variable",     &tests_ff_gui_standard_omits_opentype_and_variable);
    rb.run("ff_all contains every flag",                 &tests_ff_all_contains_every_flag);
    rb.run("ff_all equals ff_gui_rich",                  &tests_ff_all_equals_gui_rich);
    rb.run("operator| combines bits",                    &tests_operator_or_combines_bits);
    rb.run("operator| yields unsigned",                  &tests_operator_or_yields_unsigned);
    rb.run("operator| is constexpr + noexcept",          &tests_operator_or_is_constexpr_and_noexcept);
    rb.run("operator| is commutative + idempotent",      &tests_operator_or_is_commutative_and_idempotent);
    rb.run("has_ff detects set bits",                    &tests_has_ff_detects_set_bits);
    rb.run("has_ff rejects unset bits",                  &tests_has_ff_rejects_unset_bits);
    rb.run("has_ff on ff_none is always false",          &tests_has_ff_on_ff_none_is_always_false);
    rb.run("has_ff WITH the ff_none bit is always false",&tests_has_ff_with_the_ff_none_bit_is_always_false);
    rb.run("has_ff on aggregates is an ANY test",        &tests_has_ff_on_aggregates);
    rb.run("has_ff is constexpr + noexcept",             &tests_has_ff_is_constexpr_and_noexcept);

    // -- 2. core enums ----------------------------------------------------
    rb.module("2. core enums",
              "font_weight / font_slant / font_stretch / font_spacing / "
              "font_size_unit -- OS/2-aligned wire values");
    rb.run("font_weight underlying type (uint16)",       &tests_font_weight_underlying_type);
    rb.run("font_weight values match OS/2 scale",        &tests_font_weight_values);
    rb.run("font_weight is monotonic",                   &tests_font_weight_is_monotonic);
    rb.run("font_slant underlying type (uint8)",         &tests_font_slant_underlying_type);
    rb.run("font_slant values",                          &tests_font_slant_values);
    rb.run("font_stretch underlying type (uint8)",       &tests_font_stretch_underlying_type);
    rb.run("font_stretch values are 1-based",            &tests_font_stretch_values);
    rb.run("font_stretch normal is the midpoint",        &tests_font_stretch_normal_is_the_midpoint);
    rb.run("font_spacing underlying type (uint8)",       &tests_font_spacing_underlying_type);
    rb.run("font_spacing values",                        &tests_font_spacing_values);
    rb.run("font_size_unit underlying type (uint8)",     &tests_font_size_unit_underlying_type);
    rb.run("font_size_unit values",                      &tests_font_size_unit_values);
    rb.run("all five enums are scoped",                  &tests_enums_are_scoped);
    rb.run("enum defaults match the model",              &tests_enum_defaults_match_the_model);

    // -- 3-6. value types -------------------------------------------------
    rb.module("3-6. value types",
              "font_color, ot_tag packing, opentype_feature, variable_axis, "
              "font_family_info");
    rb.run("font_color defaults (alpha opaque)",         &tests_font_color_defaults);
    rb.run("font_color aggregate init",                  &tests_font_color_aggregate_init);
    rb.run("font_color equality",                        &tests_font_color_equality);
    rb.run("font_color inequality per channel",          &tests_font_color_inequality_per_channel);
    rb.run("font_color equality is constexpr + noexcept",&tests_font_color_equality_is_constexpr_and_noexcept);
    rb.run("opentype_tag is uint32",                     &tests_opentype_tag_is_uint32);
    rb.run("ot_tag packs big-endian",                    &tests_ot_tag_packs_big_endian);
    rb.run("ot_tag known tags",                          &tests_ot_tag_known_tags);
    rb.run("ot_tag is constexpr + noexcept",             &tests_ot_tag_is_constexpr_and_noexcept);
    rb.run("ot_tag handles high-bit bytes (no sign ext)",&tests_ot_tag_handles_high_bit_bytes);
    rb.run("ot_tag space padding is the caller's job",   &tests_ot_tag_space_padding_is_the_callers_job);
    rb.run("ot_tag distinct tags are distinct values",   &tests_ot_tag_distinct_tags_are_distinct_values);
    rb.run("opentype_feature defaults (value enabled)",  &tests_opentype_feature_defaults);
    rb.run("opentype_feature aggregate init",            &tests_opentype_feature_aggregate_init);
    rb.run("variable_axis defaults (value 0)",           &tests_variable_axis_defaults);
    rb.run("variable_axis aggregate init",               &tests_variable_axis_aggregate_init);
    rb.run("font_family_info defaults",                  &tests_font_family_info_defaults);
    rb.run("font_family_info scalable is the odd default",&tests_font_family_info_scalable_is_the_odd_default);
    rb.run("font_family_info population",                &tests_font_family_info_population);

    // -- 7. EBO mixins ----------------------------------------------------
    rb.module("7. EBO mixins",
              "empty disabled faces, populated enabled faces, and the "
              "zero-cost claim measured by color-payload independence");
    rb.run("disabled mixins are empty",                  &tests_mixin_disabled_are_empty);
    rb.run("enabled mixins are not empty",               &tests_mixin_enabled_are_not_empty);
    rb.run("decoration mixin defaults",                  &tests_mixin_decoration_defaults);
    rb.run("casing mixin defaults",                      &tests_mixin_casing_defaults);
    rb.run("metrics mixin defaults",                     &tests_mixin_metrics_defaults);
    rb.run("axes mixin defaults (stretch NOT zero-init)",&tests_mixin_axes_defaults);
    rb.run("color mixin default is value-initialised",   &tests_mixin_color_default_is_value_initialised);
    rb.run("background mixin defaults (flag off)",        &tests_mixin_background_defaults);
    rb.run("opentype features mixin default empty",      &tests_mixin_opentype_features_default_empty);
    rb.run("variable axes mixin default empty",          &tests_mixin_variable_axes_default_empty);
    rb.run("script hint mixin defaults empty",           &tests_mixin_script_hint_defaults_empty);
    rb.run("backend handles mixin defaults",             &tests_mixin_backend_handles_defaults);
    rb.run("color mixin is parameterised on the type",   &tests_mixin_color_type_is_parameterised);
    rb.run("disabled color costs nothing",               &tests_ebo_disabled_color_costs_nothing);
    rb.run("disabled color costs nothing beside features",&tests_ebo_disabled_color_costs_nothing_alongside_features);
    rb.run("enabled color costs its weight",             &tests_ebo_enabled_color_costs_its_weight);
    rb.run("features grow the type",                     &tests_ebo_features_grow_the_type);
    rb.run("profiles are ordered by payload",            &tests_ebo_profiles_are_ordered_by_payload);

    // -- 8. the font struct -----------------------------------------------
    rb.module("8. the font struct",
              "core defaults, the seventeen static gates, empty(), the "
              "constructor set, and value semantics");
    rb.run("default member values",                      &tests_font_default_member_values);
    rb.run("default color type is rgb",                  &tests_font_default_color_type_is_rgb);
    rb.run("color_type alias follows the parameter",     &tests_font_color_type_alias_follows_the_parameter);
    rb.run("the features constant echoes _Feat",         &tests_font_features_constant_echoes_feat);
    rb.run("static flags agree with has_ff",             &tests_font_static_flags_agree_with_has_ff);
    rb.run("static flags are constant expressions",      &tests_font_static_flags_are_constant_expressions);
    rb.run("static flags all false on ff_none",          &tests_font_static_flags_all_false_on_ff_none);
    rb.run("static flags all true on ff_all",            &tests_font_static_flags_all_true_on_ff_all);
    rb.run("empty() on default",                         &tests_font_empty_on_default);
    rb.run("empty() tracks the family only",             &tests_font_empty_tracks_the_family_only);
    rb.run("empty() is const + noexcept",                &tests_font_empty_is_const_and_noexcept);
    rb.run("empty() after clearing the family",          &tests_font_empty_after_clearing_the_family);
    rb.run("the one-arg family ctor is explicit",        &tests_font_family_ctor_is_explicit);
    rb.run("family + size ctor",                         &tests_font_family_size_ctor);
    rb.run("family + size + weight ctor",                &tests_font_family_size_weight_ctor);
    rb.run("family + size + weight + slant ctor",        &tests_font_family_size_weight_slant_ctor);
    rb.run("ctors leave the other members default",      &tests_font_ctors_leave_the_other_members_default);
    rb.run("font is copyable + movable",                 &tests_font_is_copyable_and_movable);
    rb.run("copy is deep",                               &tests_font_copy_is_deep);
    rb.run("move preserves the value",                   &tests_font_move_preserves_the_value);
    rb.run("mixin members are reachable",                &tests_font_mixin_members_are_reachable);
    rb.run("distinct feature sets are distinct types",   &tests_font_distinct_feature_sets_are_distinct_types);

    // -- 9a. core free functions ------------------------------------------
    rb.module("9a. core free functions",
              "family / style / size / unit, the two-track weight, slant, "
              "and the bold / italic conveniences");
    rb.run("fn_set_family",                              &tests_fn_set_family);
    rb.run("fn_set_style_name (label, not command)",     &tests_fn_set_style_name);
    rb.run("fn_set_size",                                &tests_fn_set_size);
    rb.run("fn_set_size does not validate",              &tests_fn_set_size_does_not_validate);
    rb.run("fn_set_size_unit (no conversion)",           &tests_fn_set_size_unit);
    rb.run("fn_set_weight",                              &tests_fn_set_weight);
    rb.run("fn_set_weight clears the numeric override",  &tests_fn_set_weight_clears_the_numeric_override);
    rb.run("fn_set_weight_numeric",                      &tests_fn_set_weight_numeric);
    rb.run("numeric 0 restores the symbolic weight",     &tests_fn_set_weight_numeric_zero_restores_the_symbolic_weight);
    rb.run("fn_effective_weight prefers the numeric",    &tests_fn_effective_weight_prefers_the_numeric);
    rb.run("fn_effective_weight falls back to the enum", &tests_fn_effective_weight_falls_back_to_the_enum);
    rb.run("fn_effective_weight honours any nonzero",    &tests_fn_effective_weight_honours_any_nonzero_numeric);
    rb.run("fn_effective_weight is noexcept",            &tests_fn_effective_weight_is_noexcept);
    rb.run("fn_set_slant (oblique preserved)",           &tests_fn_set_slant);
    rb.run("fn_set_bold true",                           &tests_fn_set_bold_true);
    rb.run("fn_set_bold false forces normal",            &tests_fn_set_bold_false_forces_normal);
    rb.run("fn_set_bold clears the numeric override",    &tests_fn_set_bold_clears_the_numeric_override);
    rb.run("fn_is_bold threshold is semi_bold (600)",    &tests_fn_is_bold_threshold_is_semi_bold);
    rb.run("fn_is_bold below the threshold",             &tests_fn_is_bold_below_the_threshold);
    rb.run("fn_is_bold reads the effective weight",      &tests_fn_is_bold_reads_the_effective_weight);
    rb.run("fn_is_bold numeric boundary (599/600/601)",  &tests_fn_is_bold_numeric_boundary);
    rb.run("fn_set_italic",                              &tests_fn_set_italic);
    rb.run("fn_set_italic false clears oblique too",     &tests_fn_set_italic_false_from_oblique);
    rb.run("fn_set_italic true overwrites oblique",      &tests_fn_set_italic_true_overwrites_oblique);
    rb.run("fn_is_italic covers italic AND oblique",     &tests_fn_is_italic_covers_italic_and_oblique);
    rb.run("bold / italic are orthogonal",               &tests_fn_bold_italic_round_trip);
    rb.run("core ops are generic over the feature set",  &tests_fn_core_ops_are_generic_over_the_feature_set);

    // -- 9b. feature setters ----------------------------------------------
    rb.module("9b. decoration / casing / metric / axis / color setters",
              "the flag-gated setters -- positive behaviour, independence, "
              "and the background enable/clear pairing");
    rb.run("fn_set_underline",                           &tests_fn_set_underline);
    rb.run("fn_set_strikethrough",                       &tests_fn_set_strikethrough);
    rb.run("fn_set_overline",                            &tests_fn_set_overline);
    rb.run("decorations are independent",                &tests_fn_decorations_are_independent);
    rb.run("fn_set_small_caps",                          &tests_fn_set_small_caps);
    rb.run("fn_set_all_caps",                            &tests_fn_set_all_caps);
    rb.run("fn_set_subscript",                           &tests_fn_set_subscript);
    rb.run("fn_set_superscript",                         &tests_fn_set_superscript);
    rb.run("casing features are independent",            &tests_fn_casing_are_independent);
    rb.run("casing permits contradictory combinations",  &tests_fn_casing_permits_contradictory_combinations);
    rb.run("fn_set_letter_spacing",                      &tests_fn_set_letter_spacing);
    rb.run("fn_set_letter_spacing accepts negative",     &tests_fn_set_letter_spacing_accepts_negative);
    rb.run("fn_set_line_height",                         &tests_fn_set_line_height);
    rb.run("line_height 0 means adapter default",        &tests_fn_line_height_zero_means_adapter_default);
    rb.run("fn_set_stretch (all nine classes)",          &tests_fn_set_stretch);
    rb.run("fn_set_spacing (all five modes)",            &tests_fn_set_spacing);
    rb.run("fn_set_foreground",                          &tests_fn_set_foreground);
    rb.run("fn_set_foreground with a custom color type", &tests_fn_set_foreground_with_a_custom_color_type);
    rb.run("fn_set_background enables it",                &tests_fn_set_background_enables_it);
    rb.run("fn_clear_background preserves the color",     &tests_fn_clear_background_preserves_the_color);
    rb.run("fn_clear_background then set again",          &tests_fn_clear_background_then_set_again);
    rb.run("fn_clear_background on a never-set bg",       &tests_fn_clear_background_on_a_never_set_background);
    rb.run("foreground and background are independent",   &tests_fn_foreground_and_background_are_independent);
    rb.run("color ops leave the core alone",             &tests_fn_color_ops_leave_the_core_alone);

    // -- 9c. collection setters -------------------------------------------
    rb.module("9c. opentype / variable / script / backend setters",
              "the upsert list setters, their sentinels, and the paired-"
              "field reset traps");
    rb.run("fn_set_opentype_feature appends",            &tests_fn_set_opentype_feature_appends);
    rb.run("fn_set_opentype_feature defaults value to 1",&tests_fn_set_opentype_feature_defaults_the_value_to_one);
    rb.run("fn_set_opentype_feature upserts",            &tests_fn_set_opentype_feature_upserts);
    rb.run("upsert preserves insertion order",           &tests_fn_set_opentype_feature_preserves_insertion_order);
    rb.run("value 0 stores a DISABLED entry",            &tests_fn_set_opentype_feature_zero_stores_a_disabled_entry);
    rb.run("get returns 0 when absent",                  &tests_fn_get_opentype_feature_returns_zero_when_absent);
    rb.run("get conflates absent and disabled",          &tests_fn_get_opentype_feature_conflates_absent_and_disabled);
    rb.run("remove reports the hit",                     &tests_fn_remove_opentype_feature_reports_the_hit);
    rb.run("remove reports the miss",                    &tests_fn_remove_opentype_feature_reports_the_miss);
    rb.run("remove removes only the match",              &tests_fn_remove_opentype_feature_removes_only_the_match);
    rb.run("remove keeps the order",                     &tests_fn_remove_opentype_feature_keeps_the_order);
    rb.run("remove on an empty list",                    &tests_fn_remove_opentype_feature_on_an_empty_list);
    rb.run("opentype features scale (20 entries)",       &tests_fn_opentype_features_scale);
    rb.run("fn_set_variable_axis appends",               &tests_fn_set_variable_axis_appends);
    rb.run("fn_set_variable_axis upserts",               &tests_fn_set_variable_axis_upserts);
    rb.run("get axis returns the default when absent",   &tests_fn_get_variable_axis_returns_the_default_when_absent);
    rb.run("get axis takes a caller default",            &tests_fn_get_variable_axis_takes_a_caller_default);
    rb.run("stored axis 0 beats the default",            &tests_fn_get_variable_axis_stored_zero_beats_the_default);
    rb.run("axis accepts the whole float range",         &tests_fn_variable_axis_accepts_the_whole_float_range);
    rb.run("axes preserve insertion order",              &tests_fn_variable_axes_preserve_insertion_order);
    rb.run("fn_set_script sets both tags",               &tests_fn_set_script_sets_both_tags);
    rb.run("fn_set_script defaults the language empty",  &tests_fn_set_script_defaults_the_language_to_empty);
    rb.run("fn_set_script wipes a previous language",    &tests_fn_set_script_wipes_a_previously_set_language);
    rb.run("fn_set_file_path sets path + index",         &tests_fn_set_file_path_sets_the_path_and_the_index);
    rb.run("fn_set_file_path defaults the index to 0",   &tests_fn_set_file_path_defaults_the_face_index_to_zero);
    rb.run("fn_set_file_path resets a previous index",   &tests_fn_set_file_path_resets_a_previously_set_face_index);
    rb.run("handles without setters are plain members",  &tests_fn_backend_handles_without_setters_are_plain_members);

    // -- 9d. fn_convert_size ----------------------------------------------
    rb.module("9d. fn_convert_size",
              "full branch coverage of the unit matrix, the em-anchor "
              "guards, and the opaque device_units early returns");
    rb.run("identity for every unit",                    &tests_convert_identity_for_every_unit);
    rb.run("points -> pixels",                           &tests_convert_points_to_pixels);
    rb.run("pixels -> points",                           &tests_convert_pixels_to_points);
    rb.run("points -> pixels honours the dpi",           &tests_convert_points_to_pixels_honours_the_dpi);
    rb.run("pixels -> points honours the dpi",           &tests_convert_pixels_to_points_honours_the_dpi);
    rb.run("em -> points",                               &tests_convert_em_to_points);
    rb.run("points -> em",                               &tests_convert_points_to_em);
    rb.run("percent -> points",                          &tests_convert_percent_to_points);
    rb.run("points -> percent",                          &tests_convert_points_to_percent);
    rb.run("em -> percent (anchor cancels)",             &tests_convert_em_to_percent);
    rb.run("percent -> em (anchor cancels)",             &tests_convert_percent_to_em);
    rb.run("em -> pixels (two hops)",                    &tests_convert_em_to_pixels);
    rb.run("pixels -> em",                               &tests_convert_pixels_to_em);
    rb.run("percent -> pixels",                          &tests_convert_percent_to_pixels);
    rb.run("pixels -> percent",                          &tests_convert_pixels_to_percent);
    rb.run("device_units source is opaque",              &tests_convert_device_units_as_source_is_opaque);
    rb.run("device_units target is opaque",              &tests_convert_device_units_as_target_is_opaque);
    rb.run("zero em_pts guards the em target",           &tests_convert_zero_em_pts_guards_the_em_target);
    rb.run("zero em_pts guards the percent target",      &tests_convert_zero_em_pts_guards_the_percent_target);
    rb.run("zero em_pts is unguarded on the source side",&tests_convert_zero_em_pts_is_unguarded_on_the_source_side);
    rb.run("round trips",                                &tests_convert_round_trips);
    rb.run("the whole 25-pair unit matrix",              &tests_convert_the_whole_unit_matrix);
    rb.run("zero and negative values",                   &tests_convert_zero_and_negative_values);
    rb.run("default arguments (96 dpi, 10pt em)",        &tests_convert_default_arguments);
    rb.run("fn_convert_size is constexpr + noexcept",    &tests_convert_is_constexpr_and_noexcept);

    // -- 10a. traits: detectors + capability traits -----------------------
    rb.module("10a. traits: detectors + capability traits",
              "structural detection driven by font<> AND by duck-typed "
              "probes -- the only witnesses for the conjunctions/disjunctions");
    rb.run("detector core member types",                 &tests_detector_core_member_types);
    rb.run("detector yields the declared type (no ref)", &tests_detector_yields_the_declared_type);
    rb.run("detector feature member types",              &tests_detector_feature_member_types);
    rb.run("detector empty() returns bool",              &tests_detector_empty_returns_bool);
    rb.run("detector color_type alias (via clean_t)",    &tests_detector_color_type_alias);
    rb.run("detectors are const + reference tolerant",   &tests_detectors_are_const_and_reference_tolerant);
    rb.run("core traits fire on any font",               &tests_has_font_core_traits_on_font);
    rb.run("core traits reject non-fonts",               &tests_has_font_core_traits_reject_non_fonts);
    rb.run("decoration traits track the flags",          &tests_has_font_decoration_traits_track_the_flags);
    rb.run("has_font_decorations is a disjunction",      &tests_has_font_decorations_is_a_disjunction);
    rb.run("casing traits track the flags",              &tests_has_font_casing_traits_track_the_flags);
    rb.run("has_font_casing is a disjunction",           &tests_has_font_casing_is_a_disjunction);
    rb.run("metrics traits track the flags",             &tests_has_font_metrics_traits_track_the_flags);
    rb.run("has_font_metrics_overrides is a disjunction", &tests_has_font_metrics_overrides_is_a_disjunction);
    rb.run("axis traits track the flags",                &tests_has_font_axis_traits_track_the_flags);
    rb.run("has_font_color tracks the flag",             &tests_has_font_color_tracks_the_flag);
    rb.run("has_font_background requires BOTH members",   &tests_has_font_background_requires_both_members);
    rb.run("opentype + variable traits track the flags", &tests_has_font_opentype_and_variable_track_the_flags);
    rb.run("has_font_script_hint requires BOTH tags",     &tests_has_font_script_hint_requires_both_tags);
    rb.run("backend traits track the flags",             &tests_has_font_backend_traits_track_the_flags);
    rb.run("has_font_backend_handles is a disjunction",  &tests_has_font_backend_handles_is_a_disjunction);
    rb.run("color_type alias is independent of ff_color",&tests_has_font_color_type_alias_is_independent_of_ff_color);
    rb.run("traits track the flags across the lattice",  &tests_traits_track_the_flags_across_the_lattice);
    rb.run("trait _v aliases agree with ::value",        &tests_trait_v_aliases_agree_with_value);
    rb.run("traits derive from integral_constant",       &tests_traits_derive_from_integral_constant);

    // -- 10b. traits: composites + SFINAE helpers -------------------------
    rb.module("10b. traits: composite profiles + SFINAE helpers",
              "the is_font_like floor, the composite gates built on it, and "
              "the enable_if_* helpers proven to steer overload resolution");
    rb.run("is_font_like across the lattice",            &tests_is_font_like_across_the_lattice);
    rb.run("is_font_like on a duck-typed struct",        &tests_is_font_like_on_a_duck_typed_struct);
    rb.run("is_font_like needs the family",              &tests_is_font_like_needs_the_family);
    rb.run("is_font_like needs the size",                &tests_is_font_like_needs_the_size);
    rb.run("is_font_like needs the weight",              &tests_is_font_like_needs_the_weight);
    rb.run("is_font_like needs the slant",               &tests_is_font_like_needs_the_slant);
    rb.run("is_font_like rejects non-fonts",             &tests_is_font_like_rejects_non_fonts);
    rb.run("is_font_bold_like == is_font_like",          &tests_is_font_bold_like_is_is_font_like);
    rb.run("is_font_italic_like == is_font_like",        &tests_is_font_italic_like_is_is_font_like);
    rb.run("is_font_with_decorations",                   &tests_is_font_with_decorations);
    rb.run("...needs the font-like floor",               &tests_is_font_with_decorations_needs_the_font_like_floor);
    rb.run("is_font_with_casing",                        &tests_is_font_with_casing);
    rb.run("is_font_with_color (fg, not bg)",            &tests_is_font_with_color);
    rb.run("is_font_with_background",                    &tests_is_font_with_background);
    rb.run("is_font_with_metrics",                       &tests_is_font_with_metrics);
    rb.run("is_font_with_opentype",                      &tests_is_font_with_opentype);
    rb.run("is_font_variable",                           &tests_is_font_variable);
    rb.run("is_font_terminal accepts color OR background",&tests_is_font_terminal_accepts_color_or_background);
    rb.run("...needs the font-like floor",               &tests_is_font_terminal_needs_the_font_like_floor);
    rb.run("is_font_rich on the rich profile",           &tests_is_font_rich_on_the_rich_profile);
    rb.run("is_font_rich rejects gui_standard",          &tests_is_font_rich_rejects_gui_standard);
    rb.run("is_font_rich rejects gui_basic",             &tests_is_font_rich_rejects_gui_basic);
    rb.run("is_font_rich has a minimal satisfying set",  &tests_is_font_rich_has_a_minimal_satisfying_set);
    rb.run("composite _v aliases agree with ::value",    &tests_composite_v_aliases_agree_with_value);
    rb.run("enable_if_* helpers steer overload resolution",&tests_enable_if_helpers_steer_overload_resolution);
    rb.run("enable_if_* helpers are void on success",    &tests_enable_if_helpers_are_void_on_success);
    rb.run("enable_if_* helpers are ill-formed on failure",&tests_enable_if_helpers_are_ill_formed_on_failure);

    // -- 11. concepts (C++20 face) ----------------------------------------
    rb.module("11. concepts (C++20 face)",
              "each concept wraps a section-10 trait; tested for agreement "
              "AND proven to constrain via real overload sets");
    rb.run("the concept gate matches the compiler",      &tests_concept_gate_matches_the_compiler);
    rb.run("core-identity concepts",                     &tests_concept_core_identity);
    rb.run("font_like_type concept",                     &tests_concept_font_like_type);
    rb.run("decoration concepts",                        &tests_concept_decorations);
    rb.run("decorated_font is a disjunction",            &tests_concept_decorated_font_is_a_disjunction);
    rb.run("casing concepts",                            &tests_concept_casing);
    rb.run("casing_font is a disjunction",               &tests_concept_casing_font_is_a_disjunction);
    rb.run("metrics concepts",                           &tests_concept_metrics);
    rb.run("axis concepts",                              &tests_concept_axes);
    rb.run("color concepts (fg / bg)",                   &tests_concept_colors);
    rb.run("color_typed_font is independent of ff_color",&tests_concept_color_typed_font_is_independent_of_ff_color);
    rb.run("opentype + variable concepts",               &tests_concept_opentype_and_variable);
    rb.run("script hint concept (paired)",               &tests_concept_script_hint);
    rb.run("backend concepts",                           &tests_concept_backend);
    rb.run("backend_resolvable_font is a disjunction",   &tests_concept_backend_resolvable_is_a_disjunction);
    rb.run("composite profile concepts (floor required)",&tests_concept_composite_profiles);
    rb.run("terminal + rich concepts",                   &tests_concept_terminal_and_rich);
    rb.run("concepts agree with their traits",           &tests_concepts_agree_with_their_traits);
    rb.run("a constrained overload is preferred",        &tests_concept_constrained_overload_is_preferred);
    rb.run("two concepts composed in a requires-clause", &tests_concept_conjunction_in_a_requires_clause);
    rb.run("concepts reject non-fonts",                  &tests_concepts_reject_non_fonts);

    return rb.finish();
}
