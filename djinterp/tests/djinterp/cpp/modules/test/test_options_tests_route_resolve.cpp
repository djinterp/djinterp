/*******************************************************************************
* djinterp [cpp][test]                      test_options_tests_route_resolve.cpp
*
*   Sections A.IV / A.V / B.V - test_route + make_route, apply_tribool_helper,
* and resolve.  A default route matches everything and overrides nothing;
* make_route sets only the guard.  apply_tribool_helper keeps the inherited value
* on `inherit` and forces it otherwise.  resolve first seeds a DTestResolved from
* the base knobs (checked field by field against the defaults), then folds every
* matching route in declaration order: the tristate toggles fold, the guarded
* scalars overwrite (later wins), and sinks either union (add) or replace.  A
* non-matching route contributes nothing, and a predicate-guarded route reaches
* only the nodes it matches.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_options_tests_route_resolve.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.11.05
*******************************************************************************/

// djinterp
#include "test_options_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_route_default
//   a default route matches everything and overrides nothing.
bool tests_route_default()
{
    bool ok = true;
    dt::test_route r;

    ok = D_TO_CHECK(r.match.evaluate(make_ctx("anything"))) && ok;
    ok = D_TO_CHECK(!r.has_sinks && !r.has_show && !r.has_format && !r.has_document) && ok;
    ok = D_TO_CHECK(r.timing    == dt::DTestTribool::inherit) && ok;
    ok = D_TO_CHECK(r.numbering == dt::DTestTribool::inherit) && ok;
    ok = D_TO_CHECK(r.color     == dt::DTestTribool::inherit) && ok;
    ok = D_TO_CHECK(r.sink_mode == dt::DTestSinkMode::add)   && ok;

    return ok;
}

// tests_route_make
//   make_route installs the guard predicate and leaves every override unset.
bool tests_route_make()
{
    bool ok = true;
    dt::test_route r = dt::make_route(dt::match_name("login"));

    ok = D_TO_CHECK( r.match.evaluate(make_ctx("login"))) && ok;
    ok = D_TO_CHECK(!r.match.evaluate(make_ctx("other"))) && ok;
    ok = D_TO_CHECK(!r.has_show && r.numbering == dt::DTestTribool::inherit) && ok;

    return ok;
}

// tests_tribool_helper
//   apply_tribool_helper keeps the base on inherit, forces it on on/off.
bool tests_tribool_helper()
{
    bool ok = true;

    ok = D_TO_CHECK(dt::internal::apply_tribool_helper(true,  dt::DTestTribool::inherit) == true)  && ok;
    ok = D_TO_CHECK(dt::internal::apply_tribool_helper(false, dt::DTestTribool::inherit) == false) && ok;
    ok = D_TO_CHECK(dt::internal::apply_tribool_helper(false, dt::DTestTribool::on)      == true)  && ok;
    ok = D_TO_CHECK(dt::internal::apply_tribool_helper(true,  dt::DTestTribool::off)     == false) && ok;

    return ok;
}

// tests_resolve_seed
//   with no routes, resolve seeds every resolved field from the base knobs.
bool tests_resolve_seed()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();
    dt::DTestResolved   r = dt::resolve(o, make_ctx("n"));

    ok = D_TO_CHECK(r.number_tests == true)                          && ok;
    ok = D_TO_CHECK(r.number_style == dt::DTestNumberStyle::ordinal)&& ok;
    ok = D_TO_CHECK(r.show_timing  == false)                         && ok;
    ok = D_TO_CHECK(r.time_unit    == dt::DTestTimeUnit::automatic) && ok;
    ok = D_TO_CHECK(r.color        == true)                          && ok;
    ok = D_TO_CHECK(r.sinks        == dt::DTestSink::console)        && ok;
    ok = D_TO_CHECK(r.show         == dt::DTestShow::all)            && ok;
    ok = D_TO_CHECK(r.document     == dt::test_doc_type::text)        && ok;
    ok = D_TO_CHECK(r.format       == "{index}. {name} [{status}] {duration}") && ok;
    ok = D_TO_CHECK(r.line_width   == static_cast<std::size_t>(80))  && ok;
    ok = D_TO_CHECK(r.word_wrap    == true)                          && ok;
    ok = D_TO_CHECK(r.wrap_mode    == dt::DTestWrapMode::word)      && ok;
    ok = D_TO_CHECK(r.indent_width == static_cast<std::size_t>(2))   && ok;

    return ok;
}

// tests_resolve_tribool_fold
//   a matching route folds the three tristate toggles onto the base.
bool tests_resolve_tribool_fold()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // number=true, timing=false, color=true

    dt::test_route rt;                                    // matches everything
    rt.numbering = dt::DTestTribool::off;                 // -> false
    rt.timing    = dt::DTestTribool::on;                  // -> true
    rt.color     = dt::DTestTribool::off;                 // -> false
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::DTestResolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(r.number_tests == false) && ok;
    ok = D_TO_CHECK(r.show_timing  == true)  && ok;
    ok = D_TO_CHECK(r.color        == false) && ok;

    return ok;
}

// tests_resolve_show_override
//   a guarded show override replaces the inherited show.
bool tests_resolve_show_override()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // base show=all

    dt::test_route rt;
    rt.has_show = true;
    rt.show     = dt::DTestShow::silent;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::DTestResolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(r.show == dt::DTestShow::silent) && ok;

    return ok;
}

// tests_resolve_format_override
//   a guarded format override replaces the inherited format.
bool tests_resolve_format_override()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    dt::test_route rt;
    rt.has_format = true;
    rt.format     = "CUSTOM {name}";
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::DTestResolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(r.format == "CUSTOM {name}") && ok;

    return ok;
}

// tests_resolve_document_override
//   a guarded document override replaces the inherited document.
bool tests_resolve_document_override()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // base document=txt

    dt::test_route rt;
    rt.has_document = true;
    rt.document     = dt::test_doc_type::html;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::DTestResolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(r.document == dt::test_doc_type::html) && ok;

    return ok;
}

// tests_resolve_sinks_add
//   an add-mode sink route unions its sinks with the inherited set.
bool tests_resolve_sinks_add()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // base sinks=console

    dt::test_route rt;
    rt.has_sinks = true;
    rt.sink_mode = dt::DTestSinkMode::add;
    rt.sinks     = dt::DTestSink::file;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::DTestResolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(dt::test_sink_has(r.sinks, dt::DTestSink::console)) && ok;  // kept
    ok = D_TO_CHECK(dt::test_sink_has(r.sinks, dt::DTestSink::file))    && ok;  // added

    return ok;
}

// tests_resolve_sinks_replace
//   a replace-mode sink route overwrites the inherited set.
bool tests_resolve_sinks_replace()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // base sinks=console

    dt::test_route rt;
    rt.has_sinks = true;
    rt.sink_mode = dt::DTestSinkMode::replace;
    rt.sinks     = dt::DTestSink::file;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::DTestResolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK( dt::test_sink_has(r.sinks, dt::DTestSink::file))    && ok;  // set
    ok = D_TO_CHECK(!dt::test_sink_has(r.sinks, dt::DTestSink::console)) && ok;  // replaced away

    return ok;
}

// tests_resolve_nonmatch_skipped
//   a route whose predicate does not match contributes nothing.
bool tests_resolve_nonmatch_skipped()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // base show=all

    dt::test_route rt = dt::make_route(dt::match_name("nomatch"));
    rt.has_show = true;
    rt.show     = dt::DTestShow::silent;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::DTestResolved r = dt::resolve(o, make_ctx("other"));   // name != nomatch
    ok = D_TO_CHECK(r.show == dt::DTestShow::all) && ok;       // untouched

    return ok;
}

// tests_resolve_order_later_wins
//   when two matching routes set the same scalar, the later one wins.
bool tests_resolve_order_later_wins()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    dt::test_route r1; r1.has_show = true; r1.show = dt::DTestShow::silent;
    dt::test_route r2; r2.has_show = true; r2.show = dt::DTestShow::summary_only;
    std::vector<dt::test_route> routes; routes.push_back(r1); routes.push_back(r2);
    set_routes(o, routes);

    dt::DTestResolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(r.show == dt::DTestShow::summary_only) && ok;   // r2 wins

    return ok;
}

// tests_resolve_predicate_selectivity
//   a predicate-guarded route reaches only the nodes it matches.
bool tests_resolve_predicate_selectivity()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    dt::test_route rt = dt::make_route(dt::match_name("login"));
    rt.has_show = true;
    rt.show     = dt::DTestShow::silent;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::DTestResolved r_login = dt::resolve(o, make_ctx("login"));
    dt::DTestResolved r_other = dt::resolve(o, make_ctx("other"));
    ok = D_TO_CHECK(r_login.show == dt::DTestShow::silent) && ok;  // matched
    ok = D_TO_CHECK(r_other.show == dt::DTestShow::all)    && ok;  // not matched

    return ok;
}


NS_END  // testing
NS_END  // djinterp
