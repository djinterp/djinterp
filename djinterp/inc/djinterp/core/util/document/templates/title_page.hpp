/*******************************************************************************
* djinterp [core]                                                 title_page.hpp
*
*   A dialect-agnostic COVER PAGE for documents.  title_page is a pure content
* model -- a title, an optional subtitle, an optional prominent banner (e.g. a
* "PASSED" / "FAILED" verdict), and a list of labelled fields (author, date,
* summary figures) -- with a single render() that streams the page through a
* document_renderer.  Like every template here it knows no format: it emits a
* heading, some paragraphs, some key-values, spacing, and a trailing page break,
* and lets the renderer decide realisation.  In PDF the title is large and
* centred and the banner is coloured; in plain text it is a left-aligned block;
* the model is the same either way.
*
*   CENTRING AND COLOUR ARE HINTS.  Every emitted block carries an `align`
* (centre) hint and a style-name hint (title_page_style_*); a renderer that can
* centre and colour does, one that cannot ignores them.  The banner's style is
* caller-supplied (set_banner's second argument), so the producer chooses the
* palette entry -- a test report passes "verdict.pass" or "verdict.fail", a
* release note might pass nothing.
*
*   THE STYLE NAMES it emits are exposed as constants so a producer can register
* matching styles on its renderer (see pdf_document_renderer::register_style) and
* have them resolved; leave them unregistered and the renderer falls back to its
* built-in heading / body styling.
*
*   PORTABILITY:
*   C++11 baseline (matches document_renderer).
*
*
* path:      /inc/djinterp/core/util/document/templates/title_page.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.11
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Emitted style names          (the shared style-name vocabulary)
      ---------------------------------------------------------------

II.   title_page (class)
      ------------------
      a. construction / fields
      b. layout knobs
      c.    render / to_string
*/

#ifndef DJINTERP_UTIL_DOCUMENT_TEMPLATES_TITLE_PAGE_HPP
#define DJINTERP_UTIL_DOCUMENT_TEMPLATES_TITLE_PAGE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../../djinterp.hpp"        // NS_*, D_NODISCARD, D_NOEXCEPT
#include "./document_attributes.hpp"    // doc_attributes, doc_attr_*, text_alignment
#include "./document_renderer.hpp"      // document_renderer, plain_document_renderer


NS_DJINTERP


// ===========================================================================
// I.   Emitted style names
// ===========================================================================
//   The style-name hints title_page attaches to its blocks.  A producer may
// register a renderer style under each of these names to theme the cover; any
// left unregistered fall back to the renderer's own heading / body styling.

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

    // title_page_style_title / _subtitle / _field
    //   the style names carried by the title heading, the subtitle paragraph,
    // and each labelled field respectively.
    inline constexpr const char* title_page_style_title    = "title_page.title";
    inline constexpr const char* title_page_style_subtitle = "title_page.subtitle";
    inline constexpr const char* title_page_style_field    = "title_page.field";

#else

    static constexpr const char* title_page_style_title    = "title_page.title";
    static constexpr const char* title_page_style_subtitle = "title_page.subtitle";
    static constexpr const char* title_page_style_field    = "title_page.field";

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// ===========================================================================
// II.  title_page (class)
// ===========================================================================

// title_page
//   class: a cover page as a content model -- title, subtitle, banner, and an
// ordered list of labelled fields, plus a few layout knobs.  render() streams
// it through a document_renderer; to_string() renders it through the plain
// renderer for text / debugging.
//
// Usage:
//   title_page _p;
//   _p.set_title("djinterp Test Suite")
//     .set_subtitle("nightly build")
//     .set_author("teer")
//     .set_date("2026-07-11")
//     .set_banner("PASSED", "verdict.pass")
//     .add_field("Assertions", "412/412")
//     .add_field("Pass rate",  "100.00%");
//   _p.render(_renderer);
class title_page
{
public:
    // -- public type aliases -------------------------------------------------

    using size_type  = std::size_t;
    using field_type = std::pair<std::string, std::string>;

    // -- a. construction / fields --------------------------------------------

    // title_page
    //   constructor: an empty cover -- no title, a trailing page break, and a
    // default top gap.
    title_page()
        : m_title(),
          m_subtitle(),
          m_author(),
          m_date(),
          m_banner(),
          m_banner_style(),
          m_fields(),
          m_top_gap(k_default_top_gap),
          m_page_break(true)
    {}

    // set_title / set_subtitle / set_author / set_date
    //   the standard cover metadata; author and date are rendered as fields
    // when set.  Each returns *this for fluent assembly.
    title_page& set_title(std::string _title)
    {
        m_title = static_cast<std::string&&>(_title);

        return *this;
    }

    title_page& set_subtitle(std::string _subtitle)
    {
        m_subtitle = static_cast<std::string&&>(_subtitle);

        return *this;
    }

    title_page& set_author(std::string _author)
    {
        m_author = static_cast<std::string&&>(_author);

        return *this;
    }

    title_page& set_date(std::string _date)
    {
        m_date = static_cast<std::string&&>(_date);

        return *this;
    }

    // set_banner
    //   a prominent line beneath the title (e.g. a verdict), rendered in
    // _style_name if the renderer knows it.  An empty banner emits nothing.
    // Returns *this.
    title_page&
    set_banner(
        std::string _banner,
        std::string _style_name = std::string()
    )
    {
        m_banner       = static_cast<std::string&&>(_banner);
        m_banner_style = static_cast<std::string&&>(_style_name);

        return *this;
    }

    // add_field
    //   append a labelled field ("Assertions", "412/412"), rendered beneath the
    // banner.  Returns *this.
    title_page&
    add_field(
        std::string _label,
        std::string _value
    )
    {
        m_fields.push_back(
            field_type(static_cast<std::string&&>(_label),
                       static_cast<std::string&&>(_value)));

        return *this;
    }

    // -- b. layout knobs -----------------------------------------------------

    // set_top_gap
    //   the blank space above the title (points, or the renderer's unit); a
    // renderer with no vertical-space notion ignores it.  Returns *this.
    title_page&
    set_top_gap(
        double _amount
    ) D_NOEXCEPT
    {
        m_top_gap = _amount;

        return *this;
    }

    // set_page_break
    //   whether a page break is emitted after the cover so the body starts on
    // a fresh page (default true).  Returns *this.
    title_page&
    set_page_break(
        bool _on
    ) D_NOEXCEPT
    {
        m_page_break = _on;

        return *this;
    }

    // -- c. render / to_string -----------------------------------------------

    // render
    //   stream the cover through _renderer: top gap, centred title heading,
    // centred subtitle, the banner, then the fields (author / date first, then
    // the added fields), then an optional page break.
    void
    render(
        document_renderer& _renderer
    ) const
    {
        // a title page is not much use without a title, but render whatever is
        // present rather than assuming
        if (m_top_gap > 0.0)
        {
            _renderer.vertical_space(m_top_gap, doc_attributes());
        }

        if (!m_title.empty())
        {
            _renderer.heading(
                size_type(1),
                m_title,
                centred(title_page_style_title));
        }

        if (!m_subtitle.empty())
        {
            _renderer.paragraph(
                m_subtitle,
                centred(title_page_style_subtitle));
        }

        // the banner sits in its own vertical band
        if (!m_banner.empty())
        {
            _renderer.vertical_space(k_banner_gap, doc_attributes());
            _renderer.paragraph(m_banner, centred(m_banner_style));
            _renderer.vertical_space(k_banner_gap, doc_attributes());
        }

        // author and date lead the field block when present
        if (!m_author.empty())
        {
            _renderer.key_value("Author", m_author, centred(title_page_style_field));
        }

        if (!m_date.empty())
        {
            _renderer.key_value("Date", m_date, centred(title_page_style_field));
        }

        size_type _i = 0;

        for (_i = 0; _i < m_fields.size(); ++_i)
        {
            _renderer.key_value(
                m_fields[_i].first,
                m_fields[_i].second,
                centred(title_page_style_field));
        }

        if (m_page_break)
        {
            _renderer.page_break();
        }

        return;
    }

    // to_string
    //   the cover rendered through the plain (monospace) renderer.
    D_NODISCARD std::string
    to_string() const
    {
        plain_document_renderer _r;

        render(_r);

        return _r.str();
    }

private:
    // centred
    //   a hint bag carrying centre alignment and, when non-empty, _style as
    // the style name.  The small builder every block here shares.
    D_NODISCARD static doc_attributes
    centred(
        const std::string& _style
    )
    {
        doc_attributes _a;

        _a.set(doc_attr_align, std::string("center"));

        // an empty style name is left unset, so the renderer picks its own
        if (!_style.empty())
        {
            _a.set(doc_attr_style, _style);
        }

        return _a;
    }

    // k_default_top_gap
    //   the blank space above the title when none is set (points).
    static constexpr double k_default_top_gap = 96.0;

    // k_banner_gap
    //   the blank space bracketing the banner (points).
    static constexpr double k_banner_gap = 18.0;

    // -- storage -------------------------------------------------------------

    std::string             m_title;
    std::string             m_subtitle;
    std::string             m_author;
    std::string             m_date;
    std::string             m_banner;
    std::string             m_banner_style;
    std::vector<field_type> m_fields;
    double                  m_top_gap;
    bool                    m_page_break;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_TEMPLATES_TITLE_PAGE_HPP
