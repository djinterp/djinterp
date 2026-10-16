/*******************************************************************************
* djinterp [test]                                          test_report_tests.hpp
*
*   Unit-test suite for test_report.hpp - the presentation-side data model for a
* DTest run, and the single source of every figure the renderers print.  Nothing
* in this header evaluates anything; it counts, folds, and formats.  That makes it
* unusually testable, and unusually worth testing: EVERY tally in every rendered
* report - text, xml, html, PDF - is read off these functions.  A miscount here is
* a miscount in all four faces at once, and none of them would disagree with each
* other loudly enough to notice.
*
*   DECLARATIONS + SHARED FIXTURES only; every test body lives in one of seven
* section translation units, one per section of the header's own table of
* contents:
*
*     …_tests_verdict.cpp     I.   report_verdict + verdict_from_counts_helper
*     …_tests_check.cpp       II.  report_check
*     …_tests_unit.cpp        III. report_unit    (checks -> a unit verdict)
*     …_tests_module.cpp      IV.  report_module  (units  -> a module verdict)
*     …_tests_report.cpp      V.   test_report    (modules -> a run verdict)
*     …_tests_rates.cpp       VI.  report_pass_rate / report_ratio
*     …_tests_filename.cpp    VII. expand_report_file_name + its helpers
*
*   Every test is a `bool tests_*()` sitting FLAT in djinterp::testing.
*
*   WHAT THIS SUITE IS ACTUALLY LOOKING FOR.  The model is a three-level fold -
* checks fold into a unit verdict, units into a module verdict, modules into a run
* verdict - and the interesting behaviour is entirely in how the DEGENERATE cases
* propagate up those levels: what an empty unit does to its module, what a skipped
* check does to its unit, what a pending unit does to a "failed" count.  The
* sections below are therefore heavy on the empty / skipped / pending / mixed
* cases, because those are the ones the ordinary happy-path run never exercises
* and the ones the fold gets wrong.
*
*   NAMING NOTE.  `internal` is AMBIGUOUS here: both ::djinterp::internal and
* ::djinterp::test::internal exist, and an unqualified `internal::` inside
* djinterp::testing resolves to the former.  Every helper below is therefore
* reached as dt::internal::.
*
*
* path:      /tests/djinterp/test/output/test_report_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_REPORT_TESTS_
#define DJINTERP_TESTS_TEST_REPORT_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
// djinterp
#include "djinterp/djinterp.hpp"   // NS_*, D_* qualifiers
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/test/output/test_report.hpp"              // THE HEADER UNDER TEST
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING


// dt
//   alias: the namespace under test.  Not a convenience - see the note above on
// the ambiguity of `internal`.
namespace dt = ::djinterp::test;


// D_RT_CHECK
//   macro: assert inside a test body; on failure it reports the expression and
// its location and returns false.  Unique letters (RT = report tests) so
// co-compiled suites never collide.
#define D_RT_CHECK(_cond)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            std::printf("        check failed:  %s\n"                          \
                        "        at:            %s:%d\n",                      \
                        #_cond, __FILE__, __LINE__);                           \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
    while (0)


#ifndef DTEST_SPEC_MODE  // fixtures: normal (section-file) mode only


///////////////////////////////////////////////////////////////////////////////
///                FIXTURES                                                  ///
///////////////////////////////////////////////////////////////////////////////

// k_all_statuses
//   the five test_status members, so a sweep can cover every one rather than the
// two anybody remembers.
D_NODISCARD inline const std::vector<dt::test_status>&
k_all_statuses()
{
    static const std::vector<dt::test_status> _s =
    {
        dt::test_status::passed,
        dt::test_status::failed,
        dt::test_status::skipped,
        dt::test_status::pending,
        dt::test_status::error
    };

    return _s;
}


// k_all_verdicts
//   the four report_verdict members.
D_NODISCARD inline const std::vector<dt::report_verdict>&
k_all_verdicts()
{
    static const std::vector<dt::report_verdict> _v =
    {
        dt::report_verdict::empty,
        dt::report_verdict::passed,
        dt::report_verdict::failed,
        dt::report_verdict::pending
    };

    return _v;
}


// unit_of
//   a unit carrying exactly the given statuses, built through add_check so the
// verdict is DERIVED (the ordinary path).  The one-liner most tests below want.
D_NODISCARD inline dt::report_unit
unit_of(
    const std::string&                  _name,
    const std::vector<dt::test_status>& _statuses
)
{
    dt::report_unit _u(_name);

    std::size_t _i = 0;

    for (_i = 0; _i < _statuses.size(); ++_i)
    {
        _u.add_check("check " + std::to_string(_i), _statuses[_i]);
    }

    return _u;
}


// unit_with_verdict
//   an EMPTY unit whose verdict is pinned explicitly.  Used to drive the module-
// level fold over each verdict in turn without having to construct checks that
// happen to derive it - and to reach `pending`, which no combination of checks
// can produce alongside a name.
D_NODISCARD inline dt::report_unit
unit_with_verdict(
    const std::string& _name,
    dt::report_verdict _v
)
{
    dt::report_unit _u(_name);

    _u.set_verdict(_v);

    return _u;
}


// module_of
//   a module carrying the given units.
D_NODISCARD inline dt::report_module
module_of(
    const std::string&                  _name,
    const std::vector<dt::report_unit>& _units
)
{
    dt::report_module _m(_name, _name + " description");

    std::size_t _i = 0;

    for (_i = 0; _i < _units.size(); ++_i)
    {
        _m.add_unit(_units[_i]);
    }

    return _m;
}


// report_of
//   a report carrying the given modules.
D_NODISCARD inline dt::test_report
report_of(
    const std::vector<dt::report_module>& _modules
)
{
    dt::test_report _r;

    _r.title = "The Run";

    std::size_t _i = 0;

    for (_i = 0; _i < _modules.size(); ++_i)
    {
        _r.add_module(_modules[_i]);
    }

    return _r;
}


// is_digits
//   whether every character of _s is a decimal digit (and _s is non-empty).  The
// shape check the live-clock placeholders get, since their VALUE cannot be
// asserted without the suite failing at midnight.
D_NODISCARD inline bool
is_digits(
    const std::string& _s
)
{
    if (_s.empty())
    {
        return false;
    }

    std::size_t _i = 0;

    for (_i = 0; _i < _s.size(); ++_i)
    {
        if ( (_s[_i] < '0') ||
             (_s[_i] > '9') )
        {
            return false;
        }
    }

    return true;
}


// contains
D_NODISCARD inline bool
contains(
    const std::string& _hay,
    const std::string& _needle
)
{
    return _hay.find(_needle) != std::string::npos;
}


#endif  // !DTEST_SPEC_MODE  (fixtures)


///////////////////////////////////////////////////////////////////////////////
///                I.   REPORT VERDICT  (…_tests_verdict.cpp)                ///
///////////////////////////////////////////////////////////////////////////////

bool tests_report_verdict_has_four_members();
bool tests_report_verdict_word_names_every_member();
bool tests_report_verdict_word_default_arm_reads_empty();
bool tests_report_verdict_is_pass_only_for_passed();
bool tests_report_counts_helper_zero_total_is_empty();
bool tests_report_counts_helper_any_failure_is_failed();
bool tests_report_counts_helper_an_error_is_a_failure();
bool tests_report_counts_helper_pending_outranks_passed();
bool tests_report_counts_helper_failure_outranks_pending();
bool tests_report_counts_helper_all_passed_is_passed();
bool tests_report_counts_helper_skipped_alone_reads_as_passed();


///////////////////////////////////////////////////////////////////////////////
///                II.  REPORT CHECK  (…_tests_check.cpp)                    ///
///////////////////////////////////////////////////////////////////////////////

bool tests_report_check_defaults_to_pending();
bool tests_report_check_carries_its_description_and_status();
bool tests_report_check_passed_is_true_only_for_passed();
bool tests_report_check_is_a_copyable_value();


///////////////////////////////////////////////////////////////////////////////
///                III. REPORT UNIT  (…_tests_unit.cpp)                      ///
///////////////////////////////////////////////////////////////////////////////

bool tests_report_unit_defaults_to_an_empty_pending_free_unit();
bool tests_report_unit_named_constructor();
bool tests_report_unit_add_check_appends_and_derives();
bool tests_report_unit_add_check_rederives_on_every_call();
bool tests_report_unit_add_result_maps_bool_to_status();
bool tests_report_unit_set_verdict_pins_it_against_later_checks();
bool tests_report_unit_assigning_the_verdict_field_does_not_pin_it();
bool tests_report_unit_check_tallies();
bool tests_report_unit_failed_checks_counts_errors_too();
bool tests_report_unit_failed_checks_ignores_skipped_and_pending();
bool tests_report_unit_an_all_skipped_unit_reads_as_passed();
bool tests_report_unit_derives_every_status();
bool tests_report_unit_an_unknown_status_splits_the_verdict_from_the_tally();
bool tests_report_unit_carries_its_elapsed_time_untouched();


///////////////////////////////////////////////////////////////////////////////
///                IV.  REPORT MODULE  (…_tests_module.cpp)                  ///
///////////////////////////////////////////////////////////////////////////////

bool tests_report_module_defaults_and_named_construction();
bool tests_report_module_add_unit_appends_and_returns_the_stored_unit();
bool tests_report_module_add_unit_invalidates_the_previous_reference();
bool tests_report_module_add_result_unit_names_the_check_after_the_unit();
bool tests_report_module_unit_tally();
bool tests_report_module_failed_units_counts_pending_as_failed();
bool tests_report_module_failed_units_excludes_empty_units();
bool tests_report_module_check_tallies_sum_over_units();
bool tests_report_module_verdict_folds_the_unit_verdicts();
bool tests_report_module_verdict_ignores_empty_units();
bool tests_report_module_verdict_precedence();
bool tests_report_module_of_no_units_is_empty_and_not_passed();


///////////////////////////////////////////////////////////////////////////////
///                V.   TEST REPORT  (…_tests_report.cpp)                    ///
///////////////////////////////////////////////////////////////////////////////

bool tests_report_report_defaults_to_empty_metadata();
bool tests_report_report_carries_its_metadata_verbatim();
bool tests_report_report_add_module_and_the_module_convenience();
bool tests_report_report_add_module_invalidates_the_previous_reference();
bool tests_report_report_module_tally();
bool tests_report_report_failed_modules_counts_an_empty_module_as_failed();
bool tests_report_report_module_counts_partition_but_unit_counts_do_not();
bool tests_report_report_unit_tally_sums_across_modules();
bool tests_report_report_check_tally_sums_across_modules();
bool tests_report_report_verdict_folds_the_module_verdicts();
bool tests_report_report_verdict_ignores_empty_modules();
bool tests_report_report_of_only_empty_modules_contradicts_itself();
bool tests_report_report_exit_code_is_zero_only_on_a_clean_pass();
bool tests_report_report_tallies_agree_end_to_end();


///////////////////////////////////////////////////////////////////////////////
///                VI.  PASS-RATE HELPERS  (…_tests_rates.cpp)               ///
///////////////////////////////////////////////////////////////////////////////

bool tests_report_pass_rate_of_a_zero_total_is_zero_not_a_division();
bool tests_report_pass_rate_formats_two_decimal_places();
bool tests_report_pass_rate_rounds_rather_than_truncates();
bool tests_report_pass_rate_of_everything_is_one_hundred();
bool tests_report_pass_rate_exceeds_one_hundred_when_passed_exceeds_total();
bool tests_report_pass_rate_overflows_its_buffer_on_absurd_input();
bool tests_report_ratio_formats_the_fraction();
bool tests_report_ratio_survives_the_largest_counts();


///////////////////////////////////////////////////////////////////////////////
///                VII. FILE-NAME EXPANSION  (…_tests_filename.cpp)          ///
///////////////////////////////////////////////////////////////////////////////

bool tests_report_sanitize_replaces_every_hostile_character();
bool tests_report_sanitize_replaces_control_characters();
bool tests_report_sanitize_lets_del_through();
bool tests_report_sanitize_passes_utf8_through();
bool tests_report_sanitize_does_not_trim_despite_the_comment();
bool tests_report_sanitize_keeps_benign_punctuation();
bool tests_report_sanitize_of_empty_is_empty();
bool tests_report_sanitize_does_not_escape_braces();
bool tests_report_replace_all_replaces_every_occurrence();
bool tests_report_replace_all_of_an_empty_token_is_the_subject();
bool tests_report_replace_all_is_not_recursive_within_a_call();
bool tests_report_replace_all_leaves_an_absent_token_alone();
bool tests_report_expand_empty_pattern_falls_back();
bool tests_report_expand_folds_every_placeholder();
bool tests_report_expand_preserves_literals_and_directories();
bool tests_report_expand_leaves_an_unknown_placeholder_alone();
bool tests_report_expand_sanitizes_module_and_title_but_not_ext();
bool tests_report_expand_dates_from_the_live_clock();
bool tests_report_expand_repeats_a_placeholder();
bool tests_report_expand_is_vulnerable_to_placeholder_injection();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from what each test actually asserts.
// =========================================================================

inline dt::module_spec
report_spec()
{
    return dt::module_spec{
        "test_report",
        "test_report.hpp - the report data model and its derivations: the "
        "report_verdict and its fold from counts, report_check / report_unit / "
        "report_module / test_report and how a verdict rolls up the tree, the "
        "pass-rate / ratio formatters, and expand_report_file_name with its "
        "sanitise / replace helpers.",
        {
            dt::block_spec{
                "verdict",
                "report_verdict, report_verdict_word / is_pass, and the "
                "verdict_from_counts fold with its precedence rules.",
                {
                    { "tests_report_verdict_has_four_members",              "report_verdict has four members with pinned ordinals", &tests_report_verdict_has_four_members },
                    { "tests_report_verdict_word_names_every_member",       "report_verdict_word names every member", &tests_report_verdict_word_names_every_member },
                    { "tests_report_verdict_word_default_arm_reads_empty",  "report_verdict_word's default arm reads as empty", &tests_report_verdict_word_default_arm_reads_empty },
                    { "tests_report_verdict_is_pass_only_for_passed",       "report_verdict_is_pass is true only for the passed verdict", &tests_report_verdict_is_pass_only_for_passed },
                    { "tests_report_counts_helper_zero_total_is_empty",     "verdict_from_counts: a zero total is empty", &tests_report_counts_helper_zero_total_is_empty },
                    { "tests_report_counts_helper_any_failure_is_failed",   "verdict_from_counts: any failure is failed", &tests_report_counts_helper_any_failure_is_failed },
                    { "tests_report_counts_helper_an_error_is_a_failure",   "verdict_from_counts: an error is a failure", &tests_report_counts_helper_an_error_is_a_failure },
                    { "tests_report_counts_helper_pending_outranks_passed", "verdict_from_counts: pending outranks passed", &tests_report_counts_helper_pending_outranks_passed },
                    { "tests_report_counts_helper_failure_outranks_pending","verdict_from_counts: failure outranks pending", &tests_report_counts_helper_failure_outranks_pending },
                    { "tests_report_counts_helper_all_passed_is_passed",    "verdict_from_counts: all passed is passed", &tests_report_counts_helper_all_passed_is_passed },
                    { "tests_report_counts_helper_skipped_alone_reads_as_passed","verdict_from_counts: skipped alone reads as passed", &tests_report_counts_helper_skipped_alone_reads_as_passed },
                }
            },
            dt::block_spec{
                "check",
                "report_check: the leaf record of one assertion.",
                {
                    { "tests_report_check_defaults_to_pending",             "a default report_check is empty and pending", &tests_report_check_defaults_to_pending },
                    { "tests_report_check_carries_its_description_and_status","report_check carries its description and status verbatim", &tests_report_check_carries_its_description_and_status },
                    { "tests_report_check_passed_is_true_only_for_passed",  "report_check::passed is true only for the passed status", &tests_report_check_passed_is_true_only_for_passed },
                    { "tests_report_check_is_a_copyable_value",             "report_check is a copyable value", &tests_report_check_is_a_copyable_value },
                }
            },
            dt::block_spec{
                "unit",
                "report_unit: checks fold into a unit verdict, with add_check / "
                "add_result, the pinning set_verdict, and the check tallies.",
                {
                    { "tests_report_unit_defaults_to_an_empty_pending_free_unit","a default report_unit is empty, pending, and free-standing", &tests_report_unit_defaults_to_an_empty_pending_free_unit },
                    { "tests_report_unit_named_constructor",                "the named constructor sets the unit's name", &tests_report_unit_named_constructor },
                    { "tests_report_unit_add_check_appends_and_derives",    "add_check appends a check and re-derives the verdict", &tests_report_unit_add_check_appends_and_derives },
                    { "tests_report_unit_add_check_rederives_on_every_call","add_check re-derives the verdict on every call", &tests_report_unit_add_check_rederives_on_every_call },
                    { "tests_report_unit_add_result_maps_bool_to_status",   "add_result maps a bool to a passed / failed check status", &tests_report_unit_add_result_maps_bool_to_status },
                    { "tests_report_unit_set_verdict_pins_it_against_later_checks","set_verdict pins the verdict against later checks", &tests_report_unit_set_verdict_pins_it_against_later_checks },
                    { "tests_report_unit_assigning_the_verdict_field_does_not_pin_it","assigning the verdict field directly does not pin it - a later add_check re-derives", &tests_report_unit_assigning_the_verdict_field_does_not_pin_it },
                    { "tests_report_unit_check_tallies",                    "the unit's per-status check tallies are correct", &tests_report_unit_check_tallies },
                    { "tests_report_unit_failed_checks_counts_errors_too",  "failed_checks counts errors as well as failures", &tests_report_unit_failed_checks_counts_errors_too },
                    { "tests_report_unit_failed_checks_ignores_skipped_and_pending","failed_checks ignores skipped and pending checks", &tests_report_unit_failed_checks_ignores_skipped_and_pending },
                    { "tests_report_unit_an_all_skipped_unit_reads_as_passed","an all-skipped unit reads as passed", &tests_report_unit_an_all_skipped_unit_reads_as_passed },
                    { "tests_report_unit_derives_every_status",             "the unit derives a verdict from every check status", &tests_report_unit_derives_every_status },
                    { "tests_report_unit_an_unknown_status_splits_the_verdict_from_the_tally","an unknown check status splits the verdict from the tally", &tests_report_unit_an_unknown_status_splits_the_verdict_from_the_tally },
                    { "tests_report_unit_carries_its_elapsed_time_untouched","the unit carries its elapsed time untouched", &tests_report_unit_carries_its_elapsed_time_untouched },
                }
            },
            dt::block_spec{
                "module",
                "report_module: units fold into a module verdict, with add_unit "
                "(and its reference invalidation), the unit tallies, and the "
                "verdict precedence.",
                {
                    { "tests_report_module_defaults_and_named_construction","report_module default and named construction", &tests_report_module_defaults_and_named_construction },
                    { "tests_report_module_add_unit_appends_and_returns_the_stored_unit","add_unit appends and returns the stored unit", &tests_report_module_add_unit_appends_and_returns_the_stored_unit },
                    { "tests_report_module_add_unit_invalidates_the_previous_reference","add_unit may reallocate, invalidating a previously-returned reference", &tests_report_module_add_unit_invalidates_the_previous_reference },
                    { "tests_report_module_add_result_unit_names_the_check_after_the_unit","the add_result_unit convenience names the check after the unit", &tests_report_module_add_result_unit_names_the_check_after_the_unit },
                    { "tests_report_module_unit_tally",                     "the module's per-verdict unit tally is correct", &tests_report_module_unit_tally },
                    { "tests_report_module_failed_units_counts_pending_as_failed","failed_units counts pending units as failed", &tests_report_module_failed_units_counts_pending_as_failed },
                    { "tests_report_module_failed_units_excludes_empty_units","failed_units excludes empty units", &tests_report_module_failed_units_excludes_empty_units },
                    { "tests_report_module_check_tallies_sum_over_units",   "the module's check tallies sum over its units", &tests_report_module_check_tallies_sum_over_units },
                    { "tests_report_module_verdict_folds_the_unit_verdicts","the module verdict folds its unit verdicts", &tests_report_module_verdict_folds_the_unit_verdicts },
                    { "tests_report_module_verdict_ignores_empty_units",    "the module verdict ignores empty units", &tests_report_module_verdict_ignores_empty_units },
                    { "tests_report_module_verdict_precedence",             "the module verdict follows the failure > pending > passed precedence", &tests_report_module_verdict_precedence },
                    { "tests_report_module_of_no_units_is_empty_and_not_passed","a module of no units is empty, not passed", &tests_report_module_of_no_units_is_empty_and_not_passed },
                }
            },
            dt::block_spec{
                "report",
                "test_report: modules fold into the run verdict, with add_module, "
                "the tallies that sum across modules, and the exit code.",
                {
                    { "tests_report_report_defaults_to_empty_metadata",     "a default test_report has empty metadata", &tests_report_report_defaults_to_empty_metadata },
                    { "tests_report_report_carries_its_metadata_verbatim",  "the report carries its metadata verbatim (untrimmed)", &tests_report_report_carries_its_metadata_verbatim },
                    { "tests_report_report_add_module_and_the_module_convenience","add_module and the module() convenience append a module", &tests_report_report_add_module_and_the_module_convenience },
                    { "tests_report_report_add_module_invalidates_the_previous_reference","add_module may reallocate, invalidating a previous reference", &tests_report_report_add_module_invalidates_the_previous_reference },
                    { "tests_report_report_module_tally",                   "the report's per-verdict module tally is correct", &tests_report_report_module_tally },
                    { "tests_report_report_failed_modules_counts_an_empty_module_as_failed","failed_modules counts an empty module as failed", &tests_report_report_failed_modules_counts_an_empty_module_as_failed },
                    { "tests_report_report_module_counts_partition_but_unit_counts_do_not","module counts partition the report, but unit counts do not", &tests_report_report_module_counts_partition_but_unit_counts_do_not },
                    { "tests_report_report_unit_tally_sums_across_modules", "the unit tally sums across modules", &tests_report_report_unit_tally_sums_across_modules },
                    { "tests_report_report_check_tally_sums_across_modules","the check tally sums across modules", &tests_report_report_check_tally_sums_across_modules },
                    { "tests_report_report_verdict_folds_the_module_verdicts","the report verdict folds its module verdicts", &tests_report_report_verdict_folds_the_module_verdicts },
                    { "tests_report_report_verdict_ignores_empty_modules",  "the report verdict ignores empty modules", &tests_report_report_verdict_ignores_empty_modules },
                    { "tests_report_report_of_only_empty_modules_contradicts_itself","a report of only empty modules reads as empty - the verdict contradicts the module tally", &tests_report_report_of_only_empty_modules_contradicts_itself },
                    { "tests_report_report_exit_code_is_zero_only_on_a_clean_pass","the exit code is zero only on a clean pass", &tests_report_report_exit_code_is_zero_only_on_a_clean_pass },
                    { "tests_report_report_tallies_agree_end_to_end",       "the report tallies agree end-to-end (module / unit / check)", &tests_report_report_tallies_agree_end_to_end },
                }
            },
            dt::block_spec{
                "rates",
                "report_pass_rate and report_ratio: the percentage and fraction "
                "formatters, including the zero-total and overflow edges.",
                {
                    { "tests_report_pass_rate_of_a_zero_total_is_zero_not_a_division","report_pass_rate of a zero total is 0.00%, not a division by zero", &tests_report_pass_rate_of_a_zero_total_is_zero_not_a_division },
                    { "tests_report_pass_rate_formats_two_decimal_places",  "report_pass_rate formats two decimal places", &tests_report_pass_rate_formats_two_decimal_places },
                    { "tests_report_pass_rate_rounds_rather_than_truncates","report_pass_rate rounds rather than truncates", &tests_report_pass_rate_rounds_rather_than_truncates },
                    { "tests_report_pass_rate_of_everything_is_one_hundred","a fully-passing rate is 100.00%", &tests_report_pass_rate_of_everything_is_one_hundred },
                    { "tests_report_pass_rate_exceeds_one_hundred_when_passed_exceeds_total","the rate exceeds 100% when passed exceeds total (no clamping)", &tests_report_pass_rate_exceeds_one_hundred_when_passed_exceeds_total },
                    { "tests_report_pass_rate_overflows_its_buffer_on_absurd_input","report_pass_rate overflows its fixed buffer on absurd input (a documented limit)", &tests_report_pass_rate_overflows_its_buffer_on_absurd_input },
                    { "tests_report_ratio_formats_the_fraction",            "report_ratio formats the passed/total fraction", &tests_report_ratio_formats_the_fraction },
                    { "tests_report_ratio_survives_the_largest_counts",     "report_ratio survives the largest counts", &tests_report_ratio_survives_the_largest_counts },
                }
            },
            dt::block_spec{
                "filename",
                "expand_report_file_name and its helpers: sanitise_file_component, "
                "replace_all, and the placeholder expansion (with its injection "
                "vulnerability).",
                {
                    { "tests_report_sanitize_replaces_every_hostile_character","sanitise replaces every filesystem-hostile character with an underscore", &tests_report_sanitize_replaces_every_hostile_character },
                    { "tests_report_sanitize_replaces_control_characters",  "sanitise replaces control characters", &tests_report_sanitize_replaces_control_characters },
                    { "tests_report_sanitize_lets_del_through",             "sanitise lets the DEL character through (a documented gap)", &tests_report_sanitize_lets_del_through },
                    { "tests_report_sanitize_passes_utf8_through",          "sanitise passes UTF-8 bytes through", &tests_report_sanitize_passes_utf8_through },
                    { "tests_report_sanitize_does_not_trim_despite_the_comment","sanitise does not trim leading / trailing spaces - it replaces them", &tests_report_sanitize_does_not_trim_despite_the_comment },
                    { "tests_report_sanitize_keeps_benign_punctuation",     "sanitise keeps benign characters unchanged", &tests_report_sanitize_keeps_benign_punctuation },
                    { "tests_report_sanitize_of_empty_is_empty",            "sanitise of the empty string is empty", &tests_report_sanitize_of_empty_is_empty },
                    { "tests_report_sanitize_does_not_escape_braces",       "sanitise does not escape braces, leaving placeholders intact", &tests_report_sanitize_does_not_escape_braces },
                    { "tests_report_replace_all_replaces_every_occurrence", "replace_all replaces every occurrence of a token", &tests_report_replace_all_replaces_every_occurrence },
                    { "tests_report_replace_all_of_an_empty_token_is_the_subject","replace_all with an empty token returns the subject unchanged", &tests_report_replace_all_of_an_empty_token_is_the_subject },
                    { "tests_report_replace_all_is_not_recursive_within_a_call","replace_all is not recursive within a call", &tests_report_replace_all_is_not_recursive_within_a_call },
                    { "tests_report_replace_all_leaves_an_absent_token_alone","replace_all leaves an absent token alone", &tests_report_replace_all_leaves_an_absent_token_alone },
                    { "tests_report_expand_empty_pattern_falls_back",       "an empty pattern falls back to module.ext", &tests_report_expand_empty_pattern_falls_back },
                    { "tests_report_expand_folds_every_placeholder",        "expand folds every placeholder (title, module, index, ext)", &tests_report_expand_folds_every_placeholder },
                    { "tests_report_expand_preserves_literals_and_directories","expand preserves literal text and directory separators", &tests_report_expand_preserves_literals_and_directories },
                    { "tests_report_expand_leaves_an_unknown_placeholder_alone","expand leaves an unknown placeholder alone", &tests_report_expand_leaves_an_unknown_placeholder_alone },
                    { "tests_report_expand_sanitizes_module_and_title_but_not_ext","expand sanitises the module and title but not the extension", &tests_report_expand_sanitizes_module_and_title_but_not_ext },
                    { "tests_report_expand_dates_from_the_live_clock",      "expand fills the date placeholder from the live clock (YYYYMMDD)", &tests_report_expand_dates_from_the_live_clock },
                    { "tests_report_expand_repeats_a_placeholder",          "expand repeats a placeholder that appears more than once", &tests_report_expand_repeats_a_placeholder },
                    { "tests_report_expand_is_vulnerable_to_placeholder_injection","expand is vulnerable to placeholder injection - a value containing a token is re-expanded", &tests_report_expand_is_vulnerable_to_placeholder_injection },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END   // testing
NS_END   // djinterp

#endif   // DJINTERP_TESTS_TEST_REPORT_TESTS_
