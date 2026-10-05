/*******************************************************************************
* djinterp [core]                                   markup_document_renderer.hpp
*
*   The XML / HTML realisation of document_renderer -- the angle-bracket half of
* the dialect set.  Where plain_document_renderer turns the semantic calls into
* aligned monospace and pdf_document_renderer turns them into a pdf_template
* flow, this one turns them into well-formed markup: a heading becomes an <h2>
* (or a <heading level="2">), a table becomes a real <table>, and every hint the
* dialect can honour becomes a CSS declaration or an attribute.
*
*   THE ONE INVARIANT: TAGS RAW, VALUES ESCAPED.  Element names and attribute
* names come from the profile and are emitted verbatim -- they are ours.  Every
* string that came from a template (text, cell contents, attribute VALUES) goes
* through the escape policy first.  That is the same contract the emit layer's
* escaping_sink held, lifted to the renderer so a caller can no longer get it
* wrong by picking the plain sink for a markup layout.  The policies themselves
* are markup_string_template's (xml_escape_policy / html_escape_policy), reused
* whole -- this header adds no escaping of its own.
*
*   ONE CLASS, TWO DIALECTS.  XML and HTML differ in their element vocabulary
* and in how presentation is spelled (a class + inline CSS versus plain
* attributes), not in their structure -- so the difference is DATA, a
* markup_profile, and not a second class.  The escape policy picks the default
* profile through markup_profile_for<>; pass a profile explicitly to retarget
* the renderer at a third angle-bracket dialect (DocBook, TEI, a house schema)
* without touching this file.
*
*   IT STREAMS.  Nothing is buffered: markup nests, so begin_table can emit
* <table> immediately, the first table_column opens <thead>, and the first
* begin_row closes it and opens <tbody>.  plain_document_renderer must hold a
* whole table to compute column widths; this one never does, so a large report
* costs one pass and no row storage.
*
*   HINTS.  A column's `align` is remembered and merged into that column's cells,
* which is what a reader expects and what the plain renderer already does.  Every
* other hint is realised per the profile: under `use_css_style` (HTML) `style`
* becomes a class and align / color / background / font / size / bold / italic /
* width become one inline `style` declaration; otherwise (XML) each hint becomes
* an attribute of the same name.  `colspan` and `locator` are structural in HTML
* (colspan= / id=) and plain attributes in XML.  A hint the profile cannot spell
* is dropped, per the subframework's rule.
*
*   PORTABILITY:
*   C++11 baseline (matches document_renderer).  The escape policies write to a
* std::ostream, so a value is escaped through a std::ostringstream -- once per
* value, on the cold assembly path, exactly as the emit layer's sink did.
*
*
* path:      /inc/djinterp/core/util/document/templates/markup_document_renderer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.23
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    markup_profile               (the dialect's vocabulary as data)
      ---------------------------------------------------------------

II.   PROFILE SELECTION            (markup_profile_for<Policy>)
      ----------------------------------------------------------

III.  markup_document_renderer     (the streaming renderer)
      -----------------------------------------------------

IV.   ALIASES                      (xml_document_renderer /
      -----------------------------------------------------

      html_document_renderer)
*/

#ifndef DJINTERP_UTIL_DOCUMENT_TEMPLATES_MARKUP_DOCUMENT_RENDERER_HPP
#define DJINTERP_UTIL_DOCUMENT_TEMPLATES_MARKUP_DOCUMENT_RENDERER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../../djinterp.hpp"                    // NS_*, D_NODISCARD,
                                                    // D_NOEXCEPT
#include "./document_attributes.hpp"                // doc_attributes, attr_*,
                                                    // text_alignment
#include "./document_renderer.hpp"                  // document_renderer,
                                                    // D_OVERRIDE
#include "../../../text/markup_string_template.hpp" // xml_escape_policy,
                                                    // html_escape_policy


NS_DJINTERP


// ===========================================================================
// I.   markup_profile
// ===========================================================================

// markup_profile
//   struct: an angle-bracket dialect expressed as data -- which element spells
// each semantic block, and how a presentation hint is written.  The renderer
// reads it and names no element of its own, so retargeting at a new schema is
// filling this in rather than deriving a class.
//
//   An empty element name means "this dialect has no element for that block";
// the renderer then falls back (a headingless dialect emits a paragraph) or
// drops it, never emitting `<>`.
struct markup_profile
{
    // -- document frame ------------------------------------------------------

    // emit_xml_declaration
    //   whether begin_document writes `<?xml version="1.0" encoding="UTF-8"?>`.
    bool        emit_xml_declaration;

    // emit_html_shell
    //   whether begin_document writes a DOCTYPE + <html><head><title>..</head>
    // <body> frame (and end_document closes it).  The document's `title` hint
    // supplies the <title> when present.
    bool        emit_html_shell;

    // root_element
    //   the element wrapping the whole body, or empty for none.  An HTML shell
    // supplies <body> and leaves this empty.
    std::string root_element;

    // -- blocks --------------------------------------------------------------

    // heading_element
    //   empty selects the HTML `h1`..`h6` family, chosen by level and clamped
    // to 6.  Non-empty names one element carrying a `level` attribute.
    std::string heading_element;
    std::string paragraph_element;

    // kv_element / kv_key_element / kv_value_element
    //   a labelled value.  With kv_element empty the pair degrades to one
    // paragraph of "key: value"; otherwise the key and value are wrapped.
    std::string kv_element;
    std::string kv_key_element;
    std::string kv_value_element;

    std::string rule_element;
    std::string space_element;
    std::string page_break_element;

    // -- lists ---------------------------------------------------------------

    std::string list_ordered_element;
    std::string list_unordered_element;
    std::string list_item_element;

    // -- tables --------------------------------------------------------------

    std::string table_element;
    std::string table_head_element;    // empty: no <thead> grouping
    std::string table_body_element;    // empty: no <tbody> grouping
    std::string row_element;
    std::string header_cell_element;
    std::string cell_element;

    // -- spelling ------------------------------------------------------------

    // use_css_style
    //   true  -- presentation hints become one inline `style="..."` plus a
    //            `class="..."` from the `style` hint (the HTML spelling).
    //   false -- every hint becomes an attribute of the same name (the XML
    //            spelling), which keeps a hint bag round-trippable.
    bool        use_css_style;

    // self_close_void
    //   whether a childless element closes as `<hr/>` (XHTML / XML) or `<hr>`.
    bool        self_close_void;

    // pretty
    //   whether blocks are indented by nesting depth.  Off produces one long
    // line, which is what a machine consumer wants.
    bool        pretty;

    // extra_attribute_keys
    //   dialect-specific hint keys to spell as attributes in addition to the
    // standard doc_attr_* vocabulary.  The hint bag is open, so a producer may
    // invent a key ("latex_env", "aria_role"); listing it here is how an
    // attribute-spelling dialect emits it.  Ignored under use_css_style, where
    // an unrecognised hint has no CSS meaning.
    std::vector<std::string> extra_attribute_keys;

    // markup_profile (default)
    //   an XML-shaped profile: a declaration, a `document` root, generic
    // element names, attribute-spelled hints.
    markup_profile()
        : emit_xml_declaration  (true),
          emit_html_shell       (false),
          root_element           ("document"),
          heading_element        ("heading"),
          paragraph_element      ("paragraph"),
          kv_element             ("entry"),
          kv_key_element         ("key"),
          kv_value_element       ("value"),
          rule_element           ("rule"),
          space_element          ("space"),
          page_break_element     ("page-break"),
          list_ordered_element   ("list"),
          list_unordered_element ("list"),
          list_item_element      ("item"),
          table_element          ("table"),
          table_head_element     ("columns"),
          table_body_element     ("rows"),
          row_element            ("row"),
          header_cell_element    ("column"),
          cell_element           ("cell"),
          use_css_style          (false),
          self_close_void        (true),
          pretty                 (true),
          extra_attribute_keys   ()
    {}
};


// markup_attr_height
//   key: this dialect's own hint for a CSS height, set by vertical_space when
// the profile spells presentation as CSS.  Private to the markup renderer --
// no other renderer reads it, which is exactly the open-vocabulary rule the
// hint bag is built on.  Follows document_attributes.hpp's linkage pattern.
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

    inline constexpr const char* markup_attr_height = "markup_height";

#else

    static constexpr const char* markup_attr_height = "markup_height";

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// xml_markup_profile
//   function: the generic XML profile -- a declaration, a `document` root,
// semantic element names, and attribute-spelled hints so the bag round-trips.
D_NODISCARD inline markup_profile
xml_markup_profile()
{
    // the default-constructed profile IS the XML one
    return markup_profile();
}


// html_markup_profile
//   function: the HTML profile -- a full document shell, the h1..h6 heading
// family, real <table>/<thead>/<tr>/<td>, and hints spelled as a class plus
// inline CSS.
D_NODISCARD inline markup_profile
html_markup_profile()
{
    markup_profile _p;

    _p.emit_xml_declaration   = false;
    _p.emit_html_shell        = true;
    _p.root_element           = std::string();
    _p.heading_element        = std::string();   // h1..h6 by level
    _p.paragraph_element      = "p";
    _p.kv_element             = "p";
    _p.kv_key_element         = "strong";
    _p.kv_value_element       = "span";
    _p.rule_element           = "hr";
    _p.space_element          = "div";
    _p.page_break_element     = "div";
    _p.list_ordered_element   = "ol";
    _p.list_unordered_element = "ul";
    _p.list_item_element      = "li";
    _p.table_element          = "table";
    _p.table_head_element     = "thead";
    _p.table_body_element     = "tbody";
    _p.row_element            = "tr";
    _p.header_cell_element    = "th";
    _p.cell_element           = "td";
    _p.use_css_style          = true;
    _p.self_close_void        = false;
    _p.pretty                 = true;

    return _p;
}


// ===========================================================================
// II.  PROFILE SELECTION
// ===========================================================================

// markup_profile_for
//   trait: the default profile paired with an escape policy, so the renderer
// aliases need no second parameter.  The primary template answers XML (the
// conservative choice for an unknown angle-bracket policy); the HTML policy
// specialises it.  A third dialect either specialises this or passes its
// profile to the constructor.
template<typename Policy>
struct markup_profile_for
{
    D_NODISCARD static markup_profile
    get()
    {
        return xml_markup_profile();
    }
};

// markup_profile_for<html_escape_policy>
//   trait: the HTML policy selects the HTML profile.
template<>
struct markup_profile_for<html_escape_policy>
{
    D_NODISCARD static markup_profile
    get()
    {
        return html_markup_profile();
    }
};


// ===========================================================================
// III. markup_document_renderer
// ===========================================================================

// markup_document_renderer
//   class: the streaming XML / HTML realisation of document_renderer.  It
// accumulates a byte buffer of well-formed markup, escaping every value it is
// handed through Policy and emitting every element name from its profile
// verbatim.  Structure nests as the calls nest; nothing is buffered.
//
// Usage:
//   html_document_renderer _r;
//   _table.render(_r);
//   const std::string& _bytes = _r.str();
template<typename Policy>
class markup_document_renderer
    : public document_renderer
{
public:
    // -- public type aliases -------------------------------------------------

    using size_type   = std::size_t;
    using policy_type = Policy;

    // -- construction --------------------------------------------------------

    // markup_document_renderer
    //   builds with the profile markup_profile_for<Policy> selects.
    markup_document_renderer()
        : m_out(),
          m_profile(markup_profile_for<Policy>::get()),
          m_depth(size_type(0)),
          m_lists(),
          m_item_open(),
          m_item_inline(false),
          m_col_aligns(),
          m_cell_index(size_type(0)),
          m_in_head(false),
          m_in_body(false),
          m_open_root(false),
          m_open_shell(false)
    {}

    // markup_document_renderer (profile)
    //   builds against an explicit profile -- the retargeting constructor.
    explicit markup_document_renderer(
        markup_profile _profile
    )
        : m_out(),
          m_profile(std::move(_profile)),
          m_depth(size_type(0)),
          m_lists(),
          m_item_open(),
          m_item_inline(false),
          m_col_aligns(),
          m_cell_index(size_type(0)),
          m_in_head(false),
          m_in_body(false),
          m_open_root(false),
          m_open_shell(false)
    {}

    // -- result --------------------------------------------------------------

    // str
    //   the accumulated document.
    D_NODISCARD const std::string&
    str() const D_NOEXCEPT
    {
        return m_out;
    }

    // profile
    //   the dialect vocabulary in force.
    D_NODISCARD const markup_profile&
    profile() const D_NOEXCEPT
    {
        return m_profile;
    }

    // clear
    //   discard the accumulated document and any in-flight list / table.
    void
    clear()
    {
        m_out.clear();
        m_depth      = size_type(0);
        m_cell_index = size_type(0);
        m_in_head    = false;
        m_in_body    = false;
        m_open_root   = false;
        m_open_shell  = false;
        m_item_inline = false;
        m_lists.clear();
        m_item_open.clear();
        m_col_aligns.clear();

        return;
    }

    // -- document frame ------------------------------------------------------

    void
    begin_document(
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        // an XML dialect leads with its declaration
        if (m_profile.emit_xml_declaration)
        {
            m_out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        }

        // an HTML dialect leads with a full shell; the `title` hint, when
        // present, supplies the <title> element
        if (m_profile.emit_html_shell)
        {
            const std::string _title = attr_or(_attrs, "title", std::string());

            m_out += "<!DOCTYPE html>\n<html>\n<head>\n";
            m_out += "<meta charset=\"utf-8\">\n";
            m_out += "<title>";
            m_out += escaped(_title);
            m_out += "</title>\n</head>\n<body>\n";

            m_open_shell = true;
            m_depth      = size_type(1);
        }

        // a root element wraps the body where the dialect has one
        if (!m_profile.root_element.empty())
        {
            open_element(m_profile.root_element, _attrs);

            m_open_root = true;
        }

        return;
    }

    void
    end_document() D_OVERRIDE
    {
        // close whatever the frame opened, innermost first
        if (m_open_root)
        {
            close_element(m_profile.root_element);

            m_open_root = false;
        }

        if (m_open_shell)
        {
            m_depth      = size_type(0);
            m_out       += "</body>\n</html>\n";
            m_open_shell = false;
        }

        return;
    }

    // -- blocks --------------------------------------------------------------

    void
    heading(
        size_type             _level,
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        // an empty heading_element selects the h1..h6 family, clamped
        if (m_profile.heading_element.empty())
        {
            size_type _l = (_level < size_type(1)) ? size_type(1) : _level;

            if (_l > size_type(6))
            {
                _l = size_type(6);
            }

            const std::string _tag = "h" + std::to_string(_l);

            block_element(_tag, _text, _attrs);

            return;
        }

        // a named heading element carries the level as an attribute instead
        doc_attributes _with_level = _attrs;

        _with_level.set("level", std::to_string(_level));

        block_element(m_profile.heading_element, _text, _with_level);

        return;
    }

    void
    paragraph(
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        block_element(m_profile.paragraph_element, _text, _attrs);

        return;
    }

    void
    key_value(
        const std::string&    _key,
        const std::string&    _value,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        // no pair element: degrade to one "key: value" paragraph
        if (m_profile.kv_element.empty())
        {
            block_element(m_profile.paragraph_element,
                          _key + ": " + _value,
                          _attrs);

            return;
        }

        indent();
        m_out += open_tag(m_profile.kv_element, _attrs);
        m_out += inline_element(m_profile.kv_key_element, _key);

        // the key/value separator is markup in HTML, structural in XML
        if (m_profile.use_css_style)
        {
            m_out += ": ";
        }

        m_out += inline_element(m_profile.kv_value_element, _value);
        m_out += close_tag(m_profile.kv_element);
        newline();

        return;
    }

    void
    rule(
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        void_element(m_profile.rule_element, _attrs, true);

        return;
    }

    void
    vertical_space(
        double                _amount,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        // nothing to spell: a dialect with no space element drops it
        if (m_profile.space_element.empty())
        {
            return;
        }

        doc_attributes _with_amount = _attrs;

        // HTML spends the amount as a CSS height; XML keeps it as data
        if (m_profile.use_css_style)
        {
            _with_amount.set(doc_attr_style, "vspace");
            _with_amount.set(markup_attr_height,
                             trimmed_number(_amount) + "pt");
        }
        else
        {
            _with_amount.set("amount", trimmed_number(_amount));
        }

        void_element(m_profile.space_element, _with_amount, false);

        return;
    }

    void
    page_break() D_OVERRIDE
    {
        doc_attributes _attrs;

        // nothing to spell: a continuous dialect drops the break
        if (m_profile.page_break_element.empty())
        {
            return;
        }

        // HTML has no page break element; the CSS property is the convention
        if (m_profile.use_css_style)
        {
            _attrs.set(doc_attr_style, "page-break");
        }

        void_element(m_profile.page_break_element, _attrs, false);

        return;
    }

    // -- lists ---------------------------------------------------------------

    void
    begin_list(
        bool                  _ordered,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        const std::string& _tag = _ordered ? m_profile.list_ordered_element
                                           : m_profile.list_unordered_element;

        doc_attributes _with_order = _attrs;

        // one shared element for both kinds needs the distinction as data
        if (m_profile.list_ordered_element ==
            m_profile.list_unordered_element)
        {
            _with_order.set("ordered", _ordered ? "true" : "false");
        }

        // a nested list belongs INSIDE its parent's item, not beside it -- so
        // the enclosing item is deliberately left open here (open_element's
        // break_item_line moves it off its opening line first)
        m_lists.push_back(_tag);
        m_item_open.push_back(false);

        open_element(_tag, _with_order);

        return;
    }

    void
    list_item(
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        // an item outside any list still renders, as a paragraph
        if (m_lists.empty())
        {
            block_element(m_profile.paragraph_element, _text, _attrs);

            return;
        }

        // the previous sibling closes before this one opens
        close_open_item();

        // an item is emitted OPEN: a nested list may still arrive, and only
        // once the level closes do we know it did not.  close_open_item folds
        // the two cases back together.
        break_item_line();
        indent();

        m_out += open_tag(m_profile.list_item_element, _attrs);
        m_out += escaped(_text);

        m_item_open.back() = true;
        m_item_inline      = true;

        return;
    }

    void
    end_list() D_OVERRIDE
    {
        // close the innermost list, if any
        if (m_lists.empty())
        {
            return;
        }

        close_open_item();

        const std::string _tag = m_lists.back();

        m_lists.pop_back();
        m_item_open.pop_back();

        close_element(_tag);

        return;
    }

    // -- tables --------------------------------------------------------------

    void
    begin_table(
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        m_col_aligns.clear();
        m_cell_index = size_type(0);
        m_in_head    = false;
        m_in_body    = false;

        open_element(m_profile.table_element, _attrs);

        return;
    }

    void
    table_column(
        const std::string&    _header,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        // remember the column's alignment so its cells inherit it
        m_col_aligns.push_back(
            attr_or(_attrs, doc_attr_align, std::string()));

        // the first column opens the header grouping and its row
        if (!m_in_head)
        {
            if (!m_profile.table_head_element.empty())
            {
                open_element(m_profile.table_head_element, doc_attributes());
            }

            open_element(m_profile.row_element, doc_attributes());

            m_in_head = true;
        }

        block_element(m_profile.header_cell_element, _header, _attrs);

        return;
    }

    void
    begin_row(
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        // the first data row closes the header grouping behind it
        close_head();

        if (!m_in_body)
        {
            if (!m_profile.table_body_element.empty())
            {
                open_element(m_profile.table_body_element, doc_attributes());
            }

            m_in_body = true;
        }

        m_cell_index = size_type(0);

        open_element(m_profile.row_element, _attrs);

        return;
    }

    void
    cell(
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        doc_attributes _merged = _attrs;

        // a cell with no alignment of its own inherits its column's
        if ( (!attr_has(_attrs, doc_attr_align)) &&
             (m_cell_index < m_col_aligns.size()) &&
             (!m_col_aligns[m_cell_index].empty()) )
        {
            _merged.set(doc_attr_align, m_col_aligns[m_cell_index]);
        }

        ++m_cell_index;

        block_element(m_profile.cell_element, _text, _merged);

        return;
    }

    void
    end_row() D_OVERRIDE
    {
        close_element(m_profile.row_element);

        return;
    }

    void
    end_table() D_OVERRIDE
    {
        // a table whose columns were declared but which carried no rows still
        // has an open header grouping to close
        close_head();

        if (m_in_body)
        {
            if (!m_profile.table_body_element.empty())
            {
                close_element(m_profile.table_body_element);
            }

            m_in_body = false;
        }

        close_element(m_profile.table_element);

        m_col_aligns.clear();
        m_cell_index = size_type(0);

        return;
    }

protected:
    // write_line
    //   the base primitive: a bare line becomes one paragraph, so a template
    // that only ever calls the defaults still produces valid markup.
    void
    write_line(
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        block_element(m_profile.paragraph_element, _text, _attrs);

        return;
    }

private:
    // -- emission helpers ----------------------------------------------------

    // escaped
    //   _text run through the escape policy.  One ostringstream per value, on
    // the cold assembly path -- the same cost the emit layer's sink paid.
    D_NODISCARD std::string
    escaped(
        const std::string& _text
    ) const
    {
        std::ostringstream _oss;

        Policy::escape(_oss, _text);

        return _oss.str();
    }

    // indent
    //   leading whitespace for the current depth, when the profile is pretty.
    void
    indent()
    {
        if (m_profile.pretty)
        {
            m_out.append(m_depth * size_type(2), ' ');
        }

        return;
    }

    // newline
    //   a line terminator, when the profile is pretty.
    void
    newline()
    {
        if (m_profile.pretty)
        {
            m_out += '\n';
        }

        return;
    }

    // open_tag
    //   `<name attrs...>` with the hints realised per the profile.
    D_NODISCARD std::string
    open_tag(
        const std::string&    _name,
        const doc_attributes& _attrs
    ) const
    {
        return "<" + _name + attribute_text(_attrs) + ">";
    }

    // close_tag
    //   `</name>`.
    D_NODISCARD std::string
    close_tag(
        const std::string& _name
    ) const
    {
        return "</" + _name + ">";
    }

    // break_item_line
    //   move an open list item off its opening line so nested content can be
    // indented beneath it.  A no-op unless an item is still inline, which is
    // what keeps the common (unnested) item on one line.
    void
    break_item_line()
    {
        if (m_item_inline)
        {
            newline();

            ++m_depth;

            m_item_inline = false;
        }

        return;
    }

    // close_open_item
    //   close the item open at the current list level, if any.  An item that
    // never took nested content closes on its opening line; one that did closes
    // on its own line at the parent indent.
    void
    close_open_item()
    {
        // no level open, or this level has no item in flight
        if ( m_item_open.empty() ||
             (!m_item_open.back()) )
        {
            return;
        }

        // still inline: `<li>text</li>` stays on one line
        if (m_item_inline)
        {
            m_out        += close_tag(m_profile.list_item_element);
            m_item_inline = false;

            newline();
        }
        else
        {
            // nested content intervened; close at the item's own indent
            if (m_depth > size_type(0))
            {
                --m_depth;
            }

            indent();

            m_out += close_tag(m_profile.list_item_element);

            newline();
        }

        m_item_open.back() = false;

        return;
    }

    // open_element
    //   emit an opening tag on its own line and descend one level.
    void
    open_element(
        const std::string&    _name,
        const doc_attributes& _attrs
    )
    {
        // an unnamed element is a dialect saying "I have none"
        if (_name.empty())
        {
            return;
        }

        break_item_line();
        indent();
        m_out += open_tag(_name, _attrs);
        newline();

        ++m_depth;

        return;
    }

    // close_element
    //   ascend one level and emit the closing tag on its own line.
    void
    close_element(
        const std::string& _name
    )
    {
        if (_name.empty())
        {
            return;
        }

        // never let an unbalanced call drive the depth negative
        if (m_depth > size_type(0))
        {
            --m_depth;
        }

        indent();
        m_out += close_tag(_name);
        newline();

        return;
    }

    // inline_element
    //   `<name>escaped</name>` with no indentation -- for nesting inside a
    // block that is already on one line.  An empty name yields the bare text.
    D_NODISCARD std::string
    inline_element(
        const std::string& _name,
        const std::string& _text
    ) const
    {
        // no wrapper element: the escaped text stands alone
        if (_name.empty())
        {
            return escaped(_text);
        }

        return "<" + _name + ">" + escaped(_text) + close_tag(_name);
    }

    // block_element
    //   one complete element on one line: `<name attrs>escaped</name>`.
    void
    block_element(
        const std::string&    _name,
        const std::string&    _text,
        const doc_attributes& _attrs
    )
    {
        // a dialect with no element for this block drops it rather than
        // emitting `<>`
        if (_name.empty())
        {
            return;
        }

        break_item_line();
        indent();
        m_out += open_tag(_name, _attrs);
        m_out += escaped(_text);
        m_out += close_tag(_name);
        newline();

        return;
    }

    // void_element
    //   a childless element.  Three spellings, because "childless" and "void"
    // are not the same thing: an XML dialect self-closes everything (`<rule/>`),
    // HTML self-closes nothing but has a fixed set of VOID elements that take
    // no closing tag (`<hr>`), and every other HTML element must be closed even
    // when empty (`<div></div>` -- an unclosed <div> swallows the rest of the
    // document).  _is_void_tag says which of the last two applies.
    void
    void_element(
        const std::string&    _name,
        const doc_attributes& _attrs,
        bool                  _is_void_tag
    )
    {
        if (_name.empty())
        {
            return;
        }

        break_item_line();
        indent();

        m_out += "<" + _name + attribute_text(_attrs);

        // XML: self-close.  HTML void element: leave open.  Otherwise: close.
        if (m_profile.self_close_void)
        {
            m_out += "/>";
        }
        else if (_is_void_tag)
        {
            m_out += ">";
        }
        else
        {
            m_out += ">";
            m_out += close_tag(_name);
        }

        newline();

        return;
    }

    // close_head
    //   close the header row (and grouping) if one is open.  Idempotent, so
    // both begin_row and end_table may call it.
    void
    close_head()
    {
        if (!m_in_head)
        {
            return;
        }

        close_element(m_profile.row_element);

        if (!m_profile.table_head_element.empty())
        {
            close_element(m_profile.table_head_element);
        }

        m_in_head = false;

        return;
    }

    // -- hint realisation ----------------------------------------------------

    // attribute_text
    //   the hints of _attrs spelled for this dialect, as a leading-space-
    // separated attribute run ready to sit inside a tag.  Under use_css_style
    // the presentation hints collapse into one `style="..."` and the `style`
    // hint becomes `class="..."`; otherwise every hint becomes an attribute of
    // its own name.  Values are always escaped.
    D_NODISCARD std::string
    attribute_text(
        const doc_attributes& _attrs
    ) const
    {
        if (m_profile.use_css_style)
        {
            return css_attribute_text(_attrs);
        }

        return plain_attribute_text(_attrs);
    }

    // plain_attribute_text
    //   the XML spelling: each hint an attribute of the same name, so a bag
    // round-trips through the markup unchanged.
    //
    //   The standard vocabulary is spelled by NAME rather than by walking the
    // bag, so this header depends only on doc_attributes' documented lookup
    // surface (find / contains) and not on its iterator's value shape.  A
    // dialect-specific key a producer invents is emitted by listing it in the
    // profile's extra_attribute_keys -- the open vocabulary stays open, the
    // dependency stays narrow.
    D_NODISCARD std::string
    plain_attribute_text(
        const doc_attributes& _attrs
    ) const
    {
        std::string _out;
        size_type   _i = size_type(0);

        // the standard hint keys, in a stable order
        for (_i = size_type(0); _i < vocabulary_size(); ++_i)
        {
            append_attribute(_out, _attrs, vocabulary_key(_i));
        }

        // the frame keys this renderer sets on its own elements
        append_attribute(_out, _attrs, "level");
        append_attribute(_out, _attrs, "ordered");
        append_attribute(_out, _attrs, "amount");
        append_attribute(_out, _attrs, "title");

        // whatever else the dialect declared
        for (_i = size_type(0); _i < m_profile.extra_attribute_keys.size();
             ++_i)
        {
            append_attribute(_out, _attrs,
                             m_profile.extra_attribute_keys[_i]);
        }

        return _out;
    }

    // append_attribute
    //   add ` key="escaped"` to _out when _attrs binds _key; a no-op when it
    // does not, so an absent hint costs nothing and emits nothing.
    void
    append_attribute(
        std::string&          _out,
        const doc_attributes& _attrs,
        const std::string&    _key
    ) const
    {
        // an unbound hint is simply not spelled
        if (!attr_has(_attrs, _key))
        {
            return;
        }

        _out += " ";
        _out += _key;
        _out += "=\"";
        _out += escaped(attr_or(_attrs, _key, std::string()));
        _out += "\"";

        return;
    }

    // vocabulary_size / vocabulary_key
    //   the standard doc_attr_* key names as an indexable table -- the set
    // document_attributes.hpp documents, in a fixed order so output is stable.
    D_NODISCARD static size_type
    vocabulary_size() D_NOEXCEPT
    {
        return size_type(14);
    }

    D_NODISCARD static const char*
    vocabulary_key(
        size_type _index
    ) D_NOEXCEPT
    {
        switch (_index)
        {
            case size_type(0):  { return doc_attr_style;      }
            case size_type(1):  { return doc_attr_align;      }
            case size_type(2):  { return doc_attr_font;       }
            case size_type(3):  { return doc_attr_size;       }
            case size_type(4):  { return doc_attr_bold;       }
            case size_type(5):  { return doc_attr_italic;     }
            case size_type(6):  { return doc_attr_color;      }
            case size_type(7):  { return doc_attr_background; }
            case size_type(8):  { return doc_attr_width;      }
            case size_type(9):  { return doc_attr_wrap;       }
            case size_type(10): { return doc_attr_indent;     }
            case size_type(11): { return doc_attr_locator;    }
            case size_type(12): { return doc_attr_colspan;    }
            case size_type(13): { return doc_attr_rowspan;    }
        }

        return "";
    }

    // css_attribute_text
    //   the HTML spelling: `style` names a class, the presentation hints
    // become one inline declaration, and the two structural hints (colspan,
    // locator) become their real HTML attributes.  A hint with no CSS or HTML
    // spelling is dropped.
    D_NODISCARD std::string
    css_attribute_text(
        const doc_attributes& _attrs
    ) const
    {
        std::string _out;
        std::string _css;
        std::string _value;

        // the style hint is a class name, not a declaration
        _value = attr_or(_attrs, doc_attr_style, std::string());

        if (!_value.empty())
        {
            _out += " class=\"" + escaped(_value) + "\"";
        }

        // an anchor is an id
        _value = attr_or(_attrs, doc_attr_locator, std::string());

        if (!_value.empty())
        {
            _out += " id=\"" + escaped(_value) + "\"";
        }

        // colspan / rowspan are real table attributes, not presentation --
        // this is where a model's merge cover lands in HTML
        _value = attr_or(_attrs, doc_attr_colspan, std::string());

        if (!_value.empty())
        {
            _out += " colspan=\"" + escaped(_value) + "\"";
        }

        _value = attr_or(_attrs, doc_attr_rowspan, std::string());

        if (!_value.empty())
        {
            _out += " rowspan=\"" + escaped(_value) + "\"";
        }

        // the presentation hints collapse into one declaration list
        _value = attr_or(_attrs, doc_attr_align, std::string());

        if (!_value.empty())
        {
            append_css(_css, "text-align", _value);
        }

        _value = attr_or(_attrs, doc_attr_color, std::string());

        if (!_value.empty())
        {
            append_css(_css, "color", _value);
        }

        _value = attr_or(_attrs, doc_attr_background, std::string());

        if (!_value.empty())
        {
            append_css(_css, "background-color", _value);
        }

        _value = attr_or(_attrs, doc_attr_font, std::string());

        if (!_value.empty())
        {
            append_css(_css, "font-family", _value);
        }

        _value = attr_or(_attrs, doc_attr_size, std::string());

        if (!_value.empty())
        {
            append_css(_css, "font-size", _value + "pt");
        }

        _value = attr_or(_attrs, doc_attr_width, std::string());

        if (!_value.empty())
        {
            append_css(_css, "width", _value);
        }

        // the face flags are only spelled when actually set
        if (attr_flag(_attrs, doc_attr_bold, false))
        {
            append_css(_css, "font-weight", "bold");
        }

        if (attr_flag(_attrs, doc_attr_italic, false))
        {
            append_css(_css, "font-style", "italic");
        }

        // the two hints this renderer introduces for its own frame elements
        _value = attr_or(_attrs, markup_attr_height, std::string());

        if (!_value.empty())
        {
            append_css(_css, "height", _value);
        }

        // a page break has no element; the CSS property is the convention
        if (attr_or(_attrs, doc_attr_style, std::string()) == "page-break")
        {
            append_css(_css, "page-break-after", "always");
        }

        if (!_css.empty())
        {
            _out += " style=\"" + escaped(_css) + "\"";
        }

        return _out;
    }

    // append_css
    //   add one `property: value;` declaration to an accumulating list.
    static void
    append_css(
        std::string&       _css,
        const std::string& _property,
        const std::string& _value
    )
    {
        // separate declarations only once there is something to separate from
        if (!_css.empty())
        {
            _css += " ";
        }

        _css += _property;
        _css += ": ";
        _css += _value;
        _css += ";";

        return;
    }

    // trimmed_number
    //   a double as the shortest sensible token -- an integral value loses its
    // ".000000" so a hint reads "12" rather than "12.000000".
    D_NODISCARD static std::string
    trimmed_number(
        double _value
    )
    {
        std::ostringstream _oss;

        _oss << _value;

        return _oss.str();
    }

    // -- state ---------------------------------------------------------------

    std::string               m_out;
    markup_profile            m_profile;
    size_type                 m_depth;
    std::vector<std::string>  m_lists;       // open list elements, innermost last
    std::vector<bool>         m_item_open;   // per level: an item in flight?
    bool                      m_item_inline; // innermost item still on its
                                             //   opening line?
    std::vector<std::string>  m_col_aligns;  // per-column align token
    size_type                 m_cell_index;  // column of the next cell
    bool                      m_in_head;
    bool                      m_in_body;
    bool                      m_open_root;
    bool                      m_open_shell;
};


// ===========================================================================
// IV.  ALIASES
// ===========================================================================

// xml_document_renderer
//   type: the XML dialect -- entity-escaped values, semantic element names,
// hints spelled as attributes so a bag round-trips.
using xml_document_renderer = markup_document_renderer<xml_escape_policy>;

// html_document_renderer
//   type: the HTML dialect -- a full document shell, h1..h6, real tables, and
// hints spelled as a class plus inline CSS.
using html_document_renderer = markup_document_renderer<html_escape_policy>;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_TEMPLATES_MARKUP_DOCUMENT_RENDERER_HPP
