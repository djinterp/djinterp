/*******************************************************************************
* djinterp [tests]                                  document_templates_tests.hpp
*
*   Unit tests for the dialect-agnostic document-template subframework built this
* session.  Each test is a `bool()` predicate in ::djinterp::testing, in the
* DTest convention (see the runner: D_TO_RUN wires each into report_builder::run).
* Grouped by the header under test:
*
*     I.   document_attributes      - the open kv hint bag: keys, flags, tokens
*     II.  document_renderer        - the abstract protocol + plain realization
*     III. document_table           - content model: build API, queries, render
*     IV.  title_page               - cover composition + knobs
*     V.   pdf_document_renderer    - the PDF face over pdf_template (integration)
*
*   The plain-text renderer's str() is the observable surface for I-IV (semantics
* are format-independent, so verifying them in text verifies them everywhere).
* The PDF back end exposes no post-render text, so V is integration-level: each
* construct is driven through the renderer and the emitted document is checked to
* be a well-formed PDF (%PDF header) with no fault -- which exercises the escape,
* table-forwarding, and style-resolution paths end to end.
*
*
* path:      /tests/djinterp/core/text/document_templates_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.11
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_DOCUMENT_TEMPLATES_
#define DJINTERP_TESTS_DOCUMENT_TEMPLATES_ 1

// std
#include <cstddef>
#include <string>
// djinterp
// djinterp (subframework under test)
#include <djinterp/core/util/document/templates/document_attributes.hpp>
#include <djinterp/core/util/document/templates/document_renderer.hpp>
#include <djinterp/core/util/document/templates/document_table.hpp>
#include <djinterp/core/util/document/templates/title_page.hpp>


namespace djinterp {
namespace testing {

// -- small shared predicates (inline: this header may be included widely) ------

// contains
//   whether _needle occurs in _hay.
inline bool
contains(
    const std::string& _hay,
    const std::string& _needle
)
{
    return _hay.find(_needle) != std::string::npos;
}

// looks_like_pdf
//   whether _bytes begin with the PDF signature.
inline bool
looks_like_pdf(
    const std::string& _bytes
)
{
    return (_bytes.size() >= 4) && (_bytes.compare(0, 4, "%PDF") == 0);
}


// ===========================================================================
// I.   document_attributes
// ===========================================================================

// a set key reads back; an absent key yields the fallback.
inline bool
tests_attr_set_and_read()
{
    doc_attributes _a;
    _a.set(doc_attr_color, std::string("#2E7D32"));

    return attr_has(_a, doc_attr_color)
        && (attr_or(_a, doc_attr_color, std::string("x")) == "#2E7D32")
        && (attr_or(_a, doc_attr_width, std::string("fallback")) == "fallback")
        && !attr_has(_a, doc_attr_width);
}

// face flags parse; an absent flag yields the fallback.
inline bool
tests_attr_flag_parsing()
{
    doc_attributes _a;
    _a.set(doc_attr_bold,   std::string("true"));
    _a.set(doc_attr_italic, std::string("false"));

    return (attr_flag(_a, doc_attr_bold,   false) == true)
        && (attr_flag(_a, doc_attr_italic, true)  == false)
        && (attr_flag(_a, doc_attr_wrap,   true)  == true);   // absent -> fallback
}

// alignment tokens round-trip; only the first character is significant.
inline bool
tests_align_token_roundtrip()
{
    return (std::string(align_to_string(text_alignment::center)) == "center")
        && (align_from_string("right", text_alignment::left)    == text_alignment::right)
        && (align_from_string("",      text_alignment::justify) == text_alignment::justify)
        && (align_from_string("C")                              == text_alignment::center);
}

// attr_align reads the align key, falling back when unset.
inline bool
tests_attr_align_reads_key()
{
    doc_attributes _a;
    _a.set(doc_attr_align, std::string("center"));

    doc_attributes _empty;

    return (attr_align(_a,     text_alignment::left)  == text_alignment::center)
        && (attr_align(_empty, text_alignment::right) == text_alignment::right);
}


// ===========================================================================
// II.  document_renderer  (plain realization)
// ===========================================================================

// a level-1 heading is underlined; a paragraph follows as its own line.
inline bool
tests_plain_heading_and_paragraph()
{
    plain_document_renderer _r;
    _r.heading(1, "Overview", doc_attributes());
    _r.paragraph("a paragraph line", doc_attributes());

    const std::string _s = _r.str();

    return contains(_s, "Overview")
        && contains(_s, "========")           // 8-wide '=' underline for "Overview"
        && contains(_s, "a paragraph line");
}

// an ordered list numbers its items.
inline bool
tests_plain_ordered_list()
{
    plain_document_renderer _r;
    _r.begin_list(true, doc_attributes());
    _r.list_item("first",  doc_attributes());
    _r.list_item("second", doc_attributes());
    _r.end_list();

    const std::string _s = _r.str();

    return contains(_s, "1. first")
        && contains(_s, "2. second");
}

// key_value renders "key: value"; a rule is a run of dashes.
inline bool
tests_plain_key_value_and_rule()
{
    plain_document_renderer _r;
    _r.key_value("Author", "teer", doc_attributes());
    _r.rule(doc_attributes());

    const std::string _s = _r.str();

    return contains(_s, "Author: teer")
        && contains(_s, "----------");
}

// a page break emits the form-feed character.
inline bool
tests_plain_page_break()
{
    plain_document_renderer _r;
    _r.paragraph("before", doc_attributes());
    _r.page_break();
    _r.paragraph("after", doc_attributes());

    const std::string _s = _r.str();

    return contains(_s, "\f")
        && contains(_s, "before")
        && contains(_s, "after");
}


// ===========================================================================
// III. document_table
// ===========================================================================

// the build API and queries agree on shape.
inline bool
tests_table_build_queries()
{
    document_table _t;
    _t.add_column("Module").add_column("Rate", text_alignment::right);
    _t.add_row({ "color", "100.00%" });
    _t.add_row({ "table", "97.50%"  });

    return (_t.column_count() == 2)
        && (_t.row_count()    == 2)
        && (_t.columns().size() == 2)
        && (_t.rows().size()    == 2);
}

// to_string carries the caption, a column separator, and the cells.
inline bool
tests_table_render_monospace()
{
    document_table _t;
    _t.set_caption("Summary", 2);
    _t.add_column("K").add_column("V", text_alignment::right, 8);
    _t.add_row({ "alpha", "1" });

    const std::string _s = _t.to_string();

    return contains(_s, "Summary")
        && contains(_s, "-------")     // level-2 caption underline
        && contains(_s, "alpha")
        && contains(_s, " | ");        // column separator
}

// a row shorter than the column list renders (padded, never truncated).
inline bool
tests_table_ragged_row()
{
    document_table _t;
    _t.add_column("A").add_column("B").add_column("C");
    _t.add_row({ "only-one" });

    return contains(_t.to_string(), "only-one");
}

// the factory seats the given headers.
inline bool
tests_make_document_table_factory()
{
    document_table _t = make_document_table({ "X", "Y", "Z" });

    return (_t.column_count() == 3);
}


// ===========================================================================
// IV.  title_page
// ===========================================================================

// the cover carries title, subtitle, banner, author, date, and fields.
inline bool
tests_title_page_composition()
{
    title_page _p;
    _p.set_title("Report")
      .set_subtitle("subtitle")
      .set_author("teer")
      .set_date("2026-07-11")
      .set_banner("PASS", "verdict.pass")
      .add_field("Rate", "100.00%");

    const std::string _s = _p.to_string();

    return contains(_s, "Report")
        && contains(_s, "subtitle")
        && contains(_s, "PASS")
        && contains(_s, "Author: teer")
        && contains(_s, "Date: 2026-07-11")
        && contains(_s, "Rate: 100.00%");
}

// the page-break knob toggles the trailing form feed.
inline bool
tests_title_page_page_break_toggle()
{
    title_page _on;
    _on.set_title("T");
    const bool _has_break = contains(_on.to_string(), "\f");

    title_page _off;
    _off.set_title("T").set_page_break(false);
    const bool _no_break = !contains(_off.to_string(), "\f");

    return _has_break && _no_break;
}

// the emitted style-name constants are the documented values.
inline bool
tests_title_page_style_name_constants()
{
    return (std::string(title_page_style_title)    == "title_page.title")
        && (std::string(title_page_style_subtitle) == "title_page.subtitle")
        && (std::string(title_page_style_field)    == "title_page.field");
}


// ===========================================================================
// V.   pdf_document_renderer  (integration smoke)
// ===========================================================================

// a title page drives cleanly to a well-formed PDF.
inline bool
tests_pdf_renders_title_page()
{
    pdf_template          _tpl;
    pdf_document_renderer _r(_tpl);

    title_page _p;
    _p.set_title("T").set_banner("PASS", "verdict.pass");
    _p.render(_r);

    return looks_like_pdf(_tpl.render_pdf());
}

// a table forwards through the monospace engine to a well-formed PDF.
inline bool
tests_pdf_renders_table()
{
    pdf_template          _tpl;
    pdf_document_renderer _r(_tpl);

    document_table _t;
    _t.add_column("A").add_column("B", text_alignment::right);
    _t.add_row({ "x", "1" });
    _t.render(_r);

    return looks_like_pdf(_tpl.render_pdf());
}

// text containing braces is escaped rather than mistaken for placeholders.
inline bool
tests_pdf_handles_brace_text()
{
    pdf_template          _tpl;
    pdf_document_renderer _r(_tpl);

    _r.paragraph("value with {curly} and {{already}}", doc_attributes());

    return looks_like_pdf(_tpl.render_pdf());
}

// a registered style name resolves during rendering.
inline bool
tests_pdf_resolves_registered_style()
{
    pdf_template          _tpl;
    pdf_document_renderer _r(_tpl);

    pdf_text_style _style(pdf_font(pdf_base_font::helvetica_bold, 20.0));
    _style.color = pdf_color::green();
    _r.register_style("verdict.pass", _style);

    doc_attributes _a;
    _a.set(doc_attr_style, std::string("verdict.pass"));
    _r.paragraph("PASS", _a);

    return looks_like_pdf(_tpl.render_pdf());
}


}  // namespace testing
}  // namespace djinterp


#endif  // DJINTERP_TESTS_DOCUMENT_TEMPLATES_
