/*******************************************************************************
* djinterp [test]                                    test_emit_tests_targets.cpp
*
*   Section II - targets.  A target is a value type (format + optional pinned
* destination + target-local tune), so these checks read its public fields
* directly.  The bare as_* consts carry their format with an empty destination
* and no tune; to() pins a destination (leaving the source const untouched, as
* it returns a copy); with() folds a target-scoped tune; the to_*_file
* shorthands are exactly as_<fmt>.to(name).  The + monoid fans targets into an
* ordered group (target+target builds the pair, group+target appends,
* target+group prepends), and per_module flips the group's per-module bit in
* both its target and group forms.
*
*
* path:      /tests/djinterp/test/test_emit_tests_targets.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_emit_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_target_bare_defaults
//   each bare as_* target carries its own format, an empty destination, and no
// local tune.
bool
tests_target_bare_defaults()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    ok = D_TE_CHECK(dt::as_text.fmt     == dt::doc_format::text)     && ok;
    ok = D_TE_CHECK(dt::as_markdown.fmt == dt::doc_format::markdown) && ok;
    ok = D_TE_CHECK(dt::as_xml.fmt      == dt::doc_format::xml)      && ok;
    ok = D_TE_CHECK(dt::as_html.fmt     == dt::doc_format::html)     && ok;
    ok = D_TE_CHECK(dt::as_pdf.fmt      == dt::doc_format::pdf)      && ok;

    ok = D_TE_CHECK(dt::as_text.dest.empty()) && ok;
    ok = D_TE_CHECK(!dt::as_text.local)       && ok;
    ok = D_TE_CHECK(dt::as_pdf.dest.empty())  && ok;
    ok = D_TE_CHECK(!dt::as_pdf.local)        && ok;

#endif
    return ok;
}

// tests_target_to_pins_dest
//   to() pins the destination and preserves the format; the source const is
// left untouched (to() returns a copy).
bool
tests_target_to_pins_dest()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::emit_target _t = dt::as_pdf.to("r.pdf");
    ok = D_TE_CHECK(_t.dest == std::string("r.pdf")) && ok;
    ok = D_TE_CHECK(_t.fmt == dt::doc_format::pdf)    && ok;
    ok = D_TE_CHECK(dt::as_pdf.dest.empty())          && ok;   // original untouched

#endif
    return ok;
}

// tests_target_with_folds_local
//   with() folds a target-scoped tune (the previously-null local becomes a
// callable that applies it), leaving format and destination alone.
bool
tests_target_with_folds_local()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::emit_target _t = dt::as_text.with(dt::tune::width(80));
    ok = D_TE_CHECK(static_cast<bool>(_t.local)) && ok;   // local now set
    ok = D_TE_CHECK(_t.fmt == dt::doc_format::text) && ok;
    ok = D_TE_CHECK(_t.dest.empty())                && ok;
    ok = D_TE_CHECK(!dt::as_text.local)             && ok;   // original still null

    // the folded tune really applies the width
    dt::test_option_set _r = _t.local(dt::default_test_options());
    ok = D_TE_CHECK(dt::line_width(_r) == std::size_t(80)) && ok;

#endif
    return ok;
}

// tests_target_to_file_shorthands
//   to_*_file(name) == as_<fmt>.to(name): same format, pinned destination.
bool
tests_target_to_file_shorthands()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::emit_target _tx = dt::to_text_file("a.txt");
    ok = D_TE_CHECK(_tx.fmt == dt::doc_format::text     && _tx.dest == std::string("a.txt"))  && ok;
    dt::emit_target _md = dt::to_md_file("a.md");
    ok = D_TE_CHECK(_md.fmt == dt::doc_format::markdown && _md.dest == std::string("a.md"))   && ok;
    dt::emit_target _xm = dt::to_xml_file("a.xml");
    ok = D_TE_CHECK(_xm.fmt == dt::doc_format::xml      && _xm.dest == std::string("a.xml"))  && ok;
    dt::emit_target _ht = dt::to_html_file("a.html");
    ok = D_TE_CHECK(_ht.fmt == dt::doc_format::html     && _ht.dest == std::string("a.html")) && ok;
    dt::emit_target _pd = dt::to_pdf_file("a.pdf");
    ok = D_TE_CHECK(_pd.fmt == dt::doc_format::pdf      && _pd.dest == std::string("a.pdf"))  && ok;

#endif
    return ok;
}

// tests_target_plus_two_targets
//   target + target builds an ordered two-element group with per_module off.
bool
tests_target_plus_two_targets()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::target_group _g = dt::as_text + dt::as_html;
    ok = D_TE_CHECK(_g.targets.size() == std::size_t(2))    && ok;
    ok = D_TE_CHECK(_g.targets[0].fmt == dt::doc_format::text) && ok;   // left first
    ok = D_TE_CHECK(_g.targets[1].fmt == dt::doc_format::html) && ok;   // right second
    ok = D_TE_CHECK(!_g.per_module)                         && ok;

#endif
    return ok;
}

// tests_target_plus_group_append_prepend
//   group + target appends at the back; target + group prepends at the front.
bool
tests_target_plus_group_append_prepend()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::target_group _base = dt::as_text + dt::as_html;   // [text, html]

    dt::target_group _app = _base + dt::as_pdf;           // append -> [text, html, pdf]
    ok = D_TE_CHECK(_app.targets.size() == std::size_t(3))  && ok;
    ok = D_TE_CHECK(_app.targets[2].fmt == dt::doc_format::pdf) && ok;

    dt::target_group _pre = dt::as_xml + _base;           // prepend -> [xml, text, html]
    ok = D_TE_CHECK(_pre.targets.size() == std::size_t(3))  && ok;
    ok = D_TE_CHECK(_pre.targets[0].fmt == dt::doc_format::xml)  && ok;
    ok = D_TE_CHECK(_pre.targets[1].fmt == dt::doc_format::text) && ok;

    ok = D_TE_CHECK(_base.targets.size() == std::size_t(2)) && ok;   // base unchanged

#endif
    return ok;
}

// tests_target_per_module_forms
//   per_module flips the per-module bit - from a bare target (wrapping it in a
// singleton group) and from an existing group (preserving its targets).
bool
tests_target_per_module_forms()
{
    bool ok = true;
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

    dt::target_group _pt = dt::per_module(dt::as_pdf);
    ok = D_TE_CHECK(_pt.targets.size() == std::size_t(1))  && ok;
    ok = D_TE_CHECK(_pt.targets[0].fmt == dt::doc_format::pdf) && ok;
    ok = D_TE_CHECK(_pt.per_module)                        && ok;

    dt::target_group _g  = dt::as_text + dt::as_html;
    dt::target_group _pg = dt::per_module(_g);
    ok = D_TE_CHECK(_pg.per_module)                        && ok;
    ok = D_TE_CHECK(_pg.targets.size() == std::size_t(2))  && ok;
    ok = D_TE_CHECK(!_g.per_module)                        && ok;   // original untouched

#endif
    return ok;
}


NS_END  // testing
NS_END  // djinterp
