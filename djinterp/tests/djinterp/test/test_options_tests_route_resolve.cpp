/*******************************************************************************
* djinterp [test]                           test_options_tests_route_resolve.cpp
*
*   Sections A.IV / A.V / B.V - test_route + make_route, apply_tribool_helper,
* and resolve.  A default route matches everything and overrides nothing;
* make_route sets only the guard.  apply_tribool_helper keeps the inherited value
* on `inherit` and forces it otherwise.  resolve first seeds a test_resolved from
* the base knobs (checked field by field against the defaults), then folds every
* MATCHING route in declaration order: the tristate toggles fold, the guarded
* scalars overwrite (later wins), and sinks either union (add) or replace.  A
* non-matching route contributes nothing, and a predicate-guarded route reaches
* only the nodes it matches.
*
*
* path:      /tests/djinterp/test/test_options_tests_route_resolve.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_options_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_route_default
//   a default route matches everything and overrides nothing.
bool
tests_route_default()
{
    bool ok = true;
    dt::test_route r;

    ok = D_TO_CHECK(r.match.evaluate(make_ctx("anything"))) && ok;
    ok = D_TO_CHECK(!r.has_sinks && !r.has_show && !r.has_format && !r.has_document) && ok;
    ok = D_TO_CHECK(r.timing    == dt::test_tribool::inherit) && ok;
    ok = D_TO_CHECK(r.numbering == dt::test_tribool::inherit) && ok;
    ok = D_TO_CHECK(r.color     == dt::test_tribool::inherit) && ok;
    ok = D_TO_CHECK(r.sink_mode == dt::test_sink_mode::add)   && ok;

    return ok;
}

// tests_route_make
//   make_route installs the guard predicate and leaves every override unset.
bool
tests_route_make()
{
    bool ok = true;
    dt::test_route r = dt::make_route(dt::match_name("login"));

    ok = D_TO_CHECK( r.match.evaluate(make_ctx("login"))) && ok;
    ok = D_TO_CHECK(!r.match.evaluate(make_ctx("other"))) && ok;
    ok = D_TO_CHECK(!r.has_show && r.numbering == dt::test_tribool::inherit) && ok;

    return ok;
}

// tests_tribool_helper
//   apply_tribool_helper keeps the base on inherit, forces it on on/off.
bool
tests_tribool_helper()
{
    bool ok = true;

    ok = D_TO_CHECK(dt::internal::apply_tribool_helper(true,  dt::test_tribool::inherit) == true)  && ok;
    ok = D_TO_CHECK(dt::internal::apply_tribool_helper(false, dt::test_tribool::inherit) == false) && ok;
    ok = D_TO_CHECK(dt::internal::apply_tribool_helper(false, dt::test_tribool::on)      == true)  && ok;
    ok = D_TO_CHECK(dt::internal::apply_tribool_helper(true,  dt::test_tribool::off)     == false) && ok;

    return ok;
}

// tests_resolve_seed
//   with no routes, resolve seeds every resolved field from the base knobs.
bool
tests_resolve_seed()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();
    dt::test_resolved   r = dt::resolve(o, make_ctx("n"));

    ok = D_TO_CHECK(r.number_tests == true)                          && ok;
    ok = D_TO_CHECK(r.number_style == dt::test_number_style::ordinal)&& ok;
    ok = D_TO_CHECK(r.show_timing  == false)                         && ok;
    ok = D_TO_CHECK(r.time_unit    == dt::test_time_unit::automatic) && ok;
    ok = D_TO_CHECK(r.color        == true)                          && ok;
    ok = D_TO_CHECK(r.sinks        == dt::test_sink::console)        && ok;
    ok = D_TO_CHECK(r.show         == dt::test_show::all)            && ok;
    ok = D_TO_CHECK(r.document     == dt::test_doc_type::txt)        && ok;
    ok = D_TO_CHECK(r.format       == "{index}. {name} [{status}] {duration}") && ok;
    ok = D_TO_CHECK(r.line_width   == static_cast<std::size_t>(80))  && ok;
    ok = D_TO_CHECK(r.word_wrap    == true)                          && ok;
    ok = D_TO_CHECK(r.wrap_mode    == dt::test_wrap_mode::word)      && ok;
    ok = D_TO_CHECK(r.indent_width == static_cast<std::size_t>(2))   && ok;

    return ok;
}

// tests_resolve_tribool_fold
//   a matching route folds the three tristate toggles onto the base.
bool
tests_resolve_tribool_fold()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // number=true, timing=false, color=true

    dt::test_route rt;                                    // matches everything
    rt.numbering = dt::test_tribool::off;                 // -> false
    rt.timing    = dt::test_tribool::on;                  // -> true
    rt.color     = dt::test_tribool::off;                 // -> false
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::test_resolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(r.number_tests == false) && ok;
    ok = D_TO_CHECK(r.show_timing  == true)  && ok;
    ok = D_TO_CHECK(r.color        == false) && ok;

    return ok;
}

// tests_resolve_show_override
//   a guarded show override replaces the inherited show.
bool
tests_resolve_show_override()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // base show=all

    dt::test_route rt;
    rt.has_show = true;
    rt.show     = dt::test_show::silent;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::test_resolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(r.show == dt::test_show::silent) && ok;

    return ok;
}

// tests_resolve_format_override
//   a guarded format override replaces the inherited format.
bool
tests_resolve_format_override()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    dt::test_route rt;
    rt.has_format = true;
    rt.format     = "CUSTOM {name}";
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::test_resolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(r.format == "CUSTOM {name}") && ok;

    return ok;
}

// tests_resolve_document_override
//   a guarded document override replaces the inherited document.
bool
tests_resolve_document_override()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // base document=txt

    dt::test_route rt;
    rt.has_document = true;
    rt.document     = dt::test_doc_type::html;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::test_resolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(r.document == dt::test_doc_type::html) && ok;

    return ok;
}

// tests_resolve_sinks_add
//   an add-mode sink route unions its sinks with the inherited set.
bool
tests_resolve_sinks_add()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // base sinks=console

    dt::test_route rt;
    rt.has_sinks = true;
    rt.sink_mode = dt::test_sink_mode::add;
    rt.sinks     = dt::test_sink::file;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::test_resolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(dt::test_sink_has(r.sinks, dt::test_sink::console)) && ok;  // kept
    ok = D_TO_CHECK(dt::test_sink_has(r.sinks, dt::test_sink::file))    && ok;  // added

    return ok;
}

// tests_resolve_sinks_replace
//   a replace-mode sink route overwrites the inherited set.
bool
tests_resolve_sinks_replace()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // base sinks=console

    dt::test_route rt;
    rt.has_sinks = true;
    rt.sink_mode = dt::test_sink_mode::replace;
    rt.sinks     = dt::test_sink::file;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::test_resolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK( dt::test_sink_has(r.sinks, dt::test_sink::file))    && ok;  // set
    ok = D_TO_CHECK(!dt::test_sink_has(r.sinks, dt::test_sink::console)) && ok;  // replaced away

    return ok;
}

// tests_resolve_nonmatch_skipped
//   a route whose predicate does not match contributes nothing.
bool
tests_resolve_nonmatch_skipped()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();   // base show=all

    dt::test_route rt = dt::make_route(dt::match_name("nomatch"));
    rt.has_show = true;
    rt.show     = dt::test_show::silent;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::test_resolved r = dt::resolve(o, make_ctx("other"));   // name != nomatch
    ok = D_TO_CHECK(r.show == dt::test_show::all) && ok;       // untouched

    return ok;
}

// tests_resolve_order_later_wins
//   when two matching routes set the same scalar, the later one wins.
bool
tests_resolve_order_later_wins()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    dt::test_route r1; r1.has_show = true; r1.show = dt::test_show::silent;
    dt::test_route r2; r2.has_show = true; r2.show = dt::test_show::summary_only;
    std::vector<dt::test_route> routes; routes.push_back(r1); routes.push_back(r2);
    set_routes(o, routes);

    dt::test_resolved r = dt::resolve(o, make_ctx("n"));
    ok = D_TO_CHECK(r.show == dt::test_show::summary_only) && ok;   // r2 wins

    return ok;
}

// tests_resolve_predicate_selectivity
//   a predicate-guarded route reaches only the nodes it matches.
bool
tests_resolve_predicate_selectivity()
{
    bool ok = true;
    dt::test_option_set o = dt::default_test_options();

    dt::test_route rt = dt::make_route(dt::match_name("login"));
    rt.has_show = true;
    rt.show     = dt::test_show::silent;
    std::vector<dt::test_route> routes; routes.push_back(rt);
    set_routes(o, routes);

    dt::test_resolved r_login = dt::resolve(o, make_ctx("login"));
    dt::test_resolved r_other = dt::resolve(o, make_ctx("other"));
    ok = D_TO_CHECK(r_login.show == dt::test_show::silent) && ok;  // matched
    ok = D_TO_CHECK(r_other.show == dt::test_show::all)    && ok;  // not matched

    return ok;
}


NS_END  // testing
NS_END  // djinterp
