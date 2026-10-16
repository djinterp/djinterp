/*******************************************************************************
* djinterp [test]                                         test_options_tests.hpp
*
*   Declarations for the unit-test suite covering test_options.hpp - the option
* subframework: the value enums (and test_sink's bitwise algebra), match_context,
* the runtime predicate IR (test_predicate + evaluate + the value-level
* builders), test_route + make_route, test_resolved, the option-set accessors +
* default_test_options, and resolve (the free fold of base knobs and matching
* routes into a flat per-node configuration).  Each free function exercises one
* facet and returns true iff every check inside it passed.  Tests are grouped
* into translation units by the section of the header they cover:
*
*   - test_options_tests_enums.cpp        -> A.I  the value enums + test_sink
*                                              bitwise operators + test_sink_has
*   - test_options_tests_predicate.cpp    -> A.II/A.III  match_context and the
*                                              predicate IR: evaluate over every
*                                              match kind + every builder
*   - test_options_tests_route_resolve.cpp-> A.IV/A.V/B.V  test_route + make_route,
*                                              apply_tribool_helper, and resolve
*                                              (seeding + route folding)
*   - test_options_tests_option_set.cpp   -> B.II/B.III/B.IV  the option set's
*                                              defaults, read through every free
*                                              accessor, via default_test_options
*
*   OBSERVATION.  Everything in PART A is pure and is exercised directly.  The
* option set is written portably: reads go through the free accessors (the
* face-agnostic seam), the defaults come from default_test_options(), and the
* one write the tests need - installing routes - goes through the set_routes
* helper, which uses the option_set slot API on C++20 and the plain-struct field
* below it.  resolve is then driven with a defaulted base plus hand-built routes.
*
*   SCOPE.  PART C (the C++20 type-level route-authoring vocabulary - payload
* carriers, override sugar, name_is<>/all_of<>/route_, and make_routes) lowers to
* the very std::vector<test_route> that resolve consumes; test_route and resolve
* are covered exhaustively here.  Exercising the lowering itself would require
* the foundational meta/option infrastructure (option_set / option / carrier /
* fixed_string / make_option_set), so it is out of this suite's scope.
*
*   NOTE: the option vocabulary lives in djinterp::test; the tests live, flat, in
* djinterp::testing.
*
*
* path:      /tests/djinterp/test/test_options_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_OPTIONS_TESTS_
#define DJINTERP_TEST_OPTIONS_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
// djinterp
// djinterp  -- framework header first, then the header under test.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/test/test_options.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: authoring surface + run_module
#endif


NS_DJINTERP
NS_TESTING

// dt names the option vocabulary (djinterp::test).
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures + check helper


// =========================================================================
//  shared check helper
// =========================================================================

// test_options_check
//   function: reports the result of a single check.  When _condition is false,
// prints the stringized expression with its source location; always returns
// _condition so the caller can fold it into a running result.
inline bool
test_options_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}


// =========================================================================
//  match-context fixture
// =========================================================================

// make_ctx
//   fixture: a match_context with the given name / type id / status (no tags;
// callers set .tags directly where a tag test needs them).
inline dt::match_context
make_ctx(
    const std::string& _name    = std::string(),
    dt::test_type_id   _type_id = 0,
    dt::test_status    _status  = dt::test_status::pending
)
{
    dt::match_context c;
    c.name    = _name;
    c.type_id = _type_id;
    c.status  = _status;
    return c;
}


// =========================================================================
//  face-agnostic route installation
// =========================================================================

// set_routes
//   fixture: install a route list on an option set through whichever write face
// the language level presents - the option_set slot API on C++20, the plain
// struct field below it.  (The read side is always the free accessors.)
inline void
set_routes(
    dt::test_option_set&         _opts,
    std::vector<dt::test_route>  _routes
)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    _opts.template set<dt::test_option::routes>(
        static_cast<std::vector<dt::test_route>&&>(_routes));
#else
    _opts.routes = static_cast<std::vector<dt::test_route>&&>(_routes);
#endif
}

#endif  // !DTEST_SPEC_MODE


// =========================================================================
//  test declarations (visible in both modes; defined in the section TUs)
// =========================================================================

// enums + test_sink algebra
bool tests_enums_sink_or();
bool tests_enums_sink_and();
bool tests_enums_sink_not();
bool tests_enums_sink_or_assign();
bool tests_enums_sink_has();
bool tests_enums_sink_values();

// predicate IR
bool tests_pred_default_any();
bool tests_pred_any_none();
bool tests_pred_name_eq();
bool tests_pred_name_contains();
bool tests_pred_name_regex();
bool tests_pred_kind();
bool tests_pred_status();
bool tests_pred_tag();
bool tests_pred_all_of();
bool tests_pred_any_of();
bool tests_pred_negate();
bool tests_pred_nested();

// route + resolve
bool tests_route_default();
bool tests_route_make();
bool tests_tribool_helper();
bool tests_resolve_seed();
bool tests_resolve_tribool_fold();
bool tests_resolve_show_override();
bool tests_resolve_format_override();
bool tests_resolve_document_override();
bool tests_resolve_sinks_add();
bool tests_resolve_sinks_replace();
bool tests_resolve_nonmatch_skipped();
bool tests_resolve_order_later_wins();
bool tests_resolve_predicate_selectivity();

// option set + accessors + defaults
bool tests_opt_defaults_numbering_timing();
bool tests_opt_defaults_layout();
bool tests_opt_defaults_presentation();
bool tests_opt_defaults_destinations();
bool tests_opt_defaults_filtering();
bool tests_opt_defaults_formats();
bool tests_opt_defaults_packaging();
bool tests_opt_defaults_routes();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
// =========================================================================

inline dt::module_spec
options_spec()
{
    return dt::module_spec{
        "test_options",
        "The option subframework: value enums + test_sink algebra, the predicate "
        "IR (evaluate + builders), test_route + make_route, resolve (seeding + "
        "route folding), and the option-set accessors + defaults.",
        {
            dt::block_spec{
                "enums",
                "The value enums and test_sink's bitwise algebra.",
                {
                    { "enums_sink_or",        "operator| unions sink sets",            &tests_enums_sink_or },
                    { "enums_sink_and",       "operator& intersects sink sets",        &tests_enums_sink_and },
                    { "enums_sink_not",       "operator~ complements within the bits", &tests_enums_sink_not },
                    { "enums_sink_or_assign", "operator|= unions in place",            &tests_enums_sink_or_assign },
                    { "enums_sink_has",       "test_sink_has is contains-all, none->false", &tests_enums_sink_has },
                    { "enums_sink_values",    "the sink bits are 0/1/2 and distinct",  &tests_enums_sink_values },
                }
            },
            dt::block_spec{
                "predicate",
                "match_context and the runtime predicate IR: evaluate across every "
                "match kind and every value-level builder.",
                {
                    { "pred_default_any",   "a default predicate matches anything",       &tests_pred_default_any },
                    { "pred_any_none",      "match_any always true, match_none always false", &tests_pred_any_none },
                    { "pred_name_eq",       "match_name is exact-name equality",          &tests_pred_name_eq },
                    { "pred_name_contains", "match_name_contains is substring",           &tests_pred_name_contains },
                    { "pred_name_regex",    "match_name_regex is a regex search",         &tests_pred_name_regex },
                    { "pred_kind",          "match_kind compares the type id",            &tests_pred_kind },
                    { "pred_status",        "match_status compares the status",           &tests_pred_status },
                    { "pred_tag",           "match_tag looks for a tag among the node's", &tests_pred_tag },
                    { "pred_all_of",        "all_of is conjunction, vacuously true empty", &tests_pred_all_of },
                    { "pred_any_of",        "any_of is disjunction, false when empty",    &tests_pred_any_of },
                    { "pred_negate",        "negate inverts; an empty negate is true",    &tests_pred_negate },
                    { "pred_nested",        "combinators nest and evaluate correctly",    &tests_pred_nested },
                }
            },
            dt::block_spec{
                "route_resolve",
                "test_route + make_route, apply_tribool_helper, and resolve "
                "(seeding from the base knobs + folding matching routes).",
                {
                    { "route_default",               "a default route matches all, overrides nothing", &tests_route_default },
                    { "route_make",                  "make_route sets the guard, overrides nothing",   &tests_route_make },
                    { "tribool_helper",              "apply_tribool_helper folds inherit/on/off",      &tests_tribool_helper },
                    { "resolve_seed",                "resolve seeds every field from the base knobs",  &tests_resolve_seed },
                    { "resolve_tribool_fold",        "a matching route folds the tristate toggles",    &tests_resolve_tribool_fold },
                    { "resolve_show_override",       "a guarded show override wins",                   &tests_resolve_show_override },
                    { "resolve_format_override",     "a guarded format override wins",                 &tests_resolve_format_override },
                    { "resolve_document_override",   "a guarded document override wins",               &tests_resolve_document_override },
                    { "resolve_sinks_add",           "an add-mode sink route unions the set",          &tests_resolve_sinks_add },
                    { "resolve_sinks_replace",       "a replace-mode sink route overwrites the set",   &tests_resolve_sinks_replace },
                    { "resolve_nonmatch_skipped",    "a non-matching route contributes nothing",       &tests_resolve_nonmatch_skipped },
                    { "resolve_order_later_wins",    "a later matching route wins on a scalar",        &tests_resolve_order_later_wins },
                    { "resolve_predicate_selectivity","a route affects only the nodes it matches",     &tests_resolve_predicate_selectivity },
                }
            },
            dt::block_spec{
                "option_set",
                "The option set's defaults, read through every free accessor.",
                {
                    { "opt_defaults_numbering_timing", "numbering / number_style / timing / time_unit", &tests_opt_defaults_numbering_timing },
                    { "opt_defaults_layout",           "line_width / word_wrap / wrap_mode / indent",   &tests_opt_defaults_layout },
                    { "opt_defaults_presentation",     "color / document",                              &tests_opt_defaults_presentation },
                    { "opt_defaults_destinations",     "sinks / output_path",                           &tests_opt_defaults_destinations },
                    { "opt_defaults_filtering",        "show / max_failures / stop_on_failure",         &tests_opt_defaults_filtering },
                    { "opt_defaults_formats",          "format_test / format_module / format_summary",  &tests_opt_defaults_formats },
                    { "opt_defaults_packaging",        "pack / compressor / archive_format / split",    &tests_opt_defaults_packaging },
                    { "opt_defaults_routes",           "routes default empty",                          &tests_opt_defaults_routes },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


// D_TO_CHECK
//   macro: evaluates its expression exactly once and routes the result through
// test_options_check, capturing the expression text and source location.
// Variadic so expressions containing top-level commas need no extra parentheses.
#define D_TO_CHECK(...)                                                        \
    ::djinterp::testing::test_options_check(                                   \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_OPTIONS_TESTS_
