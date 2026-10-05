/*******************************************************************************
* djinterp [core]                                                       html.hpp
*
*   Foundational HTML module for the djinterp framework. Layered on top
* of the XML module: HTML elements ARE XML nodes plus HTML semantics.
* The HTML facades inherit from the XML facades so that storage and
* memory layout are unchanged; HTML-specific behaviour (DOCTYPE
* emission, void-element shorthand, class/id helpers, semantic-tag
* classification) is added via methods and free functions only --
* never via additional state.
*
*   CONSOLIDATION:
*   The HTML detection traits (formerly `html_template_traits.hpp`) and
* the C++20 concept wrappers (formerly `html_template_concepts.hpp`) now
* live in this header directly, mirroring the XML module. `html.hpp` is a
* standalone foundation -- it does NOT pull in the templated element /
* document types; `html_template.hpp` is a separate module that includes
* this header. A single `#include "html.hpp"` brings in the shared types,
* the trait system, and the concepts together.
*
*   ZERO OVERHEAD:
*   - `html_element<Backend>` adds NO members beyond `xml_node`.
*   - All tag / attribute / category lookups are `D_CONSTEXPR` and
*     compile down to switch tables or simple comparisons.
*   - Tag-name strings live in `html::tags` as string-literal pointers
*     (no allocation, no dynamic init).
*   - The element-kind enum is `re_std::uint8_t`-backed.
*
*   LIBRARY AGNOSTICISM:
*   This header pulls in nothing beyond the XML module and the standard
* library. Backends for libxml++, gumbo-parser, lexbor, htmlcxx, or any
* other HTML library are detected structurally via the trait headers
* and live in their own adapter files.
*
*
* path:      /inc/djinterp/core/text/html/html.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.08
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TYPES & CONSTANTS
      ------------------------

II.   HTML VERSION & DOCTYPE
      ----------------------

III.  ELEMENT KIND ENUM
      -----------------

IV.   ATTRIBUTE KIND ENUM
      -------------------

V.    ELEMENT CATEGORY CLASSIFICATION
      -------------------------------

VI.   TAG-NAME / ATTR-NAME NAMESPACES
      -------------------------------

VII.  KIND <-> NAME MAPPING
      ---------------------

VIII. BACKEND TAG DISPATCH
      --------------------

IX.   BACKEND DETECTION
      -----------------

X.    HTML KIND ACCESSOR DETECTION
      ----------------------------

XI.   CLASS LIST DETECTION
      --------------------

XII.  DOCTYPE & VERSION DETECTION
      ---------------------------

XIII. HEAD / BODY ACCESSOR DETECTION
      ------------------------------

XIV.  HTML ELEMENT CLASSIFICATION
      ---------------------------

XV.   HTML DOCUMENT CLASSIFICATION
      ----------------------------

XVI.  BACKEND COMPLETENESS DETECTION
      ------------------------------

XVII. VARIABLE TEMPLATES
      ------------------

XVIII. CONCEPTS (C++20)
      ----------------
*/

#ifndef DJINTERP_TEXT_HTML_HTML_HPP
#define DJINTERP_TEXT_HTML_HTML_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "../../../env/cpp/env_cpp_features.h"  // D_ENV_CPP_FEATURE_LANG_CONCEPTS
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint8_t

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// std
#  include <concepts>
#endif
// djinterp
#include "../../../djinterp.hpp"
#include "../xml/xml.hpp"


///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED TYPES & CONSTANTS                            ///
///////////////////////////////////////////////////////////////////////////////

NS_DJINTERP

namespace html {

// D_HTML_DEFAULT_INDENT
//   constant: default whitespace string used per indentation level
// when serialising HTML.
#ifndef D_HTML_DEFAULT_INDENT
    #define D_HTML_DEFAULT_INDENT       "  "
#endif


// D_HTML_DEFAULT_DOCTYPE
//   constant: default DOCTYPE string for new documents -- HTML5.
#ifndef D_HTML_DEFAULT_DOCTYPE
    #define D_HTML_DEFAULT_DOCTYPE      "<!DOCTYPE html>"
#endif


///////////////////////////////////////////////////////////////////////////////
///                II.   HTML VERSION & DOCTYPE                             ///
///////////////////////////////////////////////////////////////////////////////

// html_version
//   enum: identifies the document's target HTML / XHTML version.
// Drives DOCTYPE emission and self-closing-tag formatting (`<br>`
// for HTML5 / HTML4 vs `<br/>` for XHTML).
enum class html_version : re_std::uint8_t
{
    html5,
    html4_strict,
    html4_transitional,
    html4_frameset,
    xhtml1_strict,
    xhtml1_transitional,
    xhtml1_frameset,
    xhtml5,
    custom,
    unspecified
};


// html_doctype_string
//   function: returns the DOCTYPE declaration for a given HTML
// version. Returns an empty string for `unspecified` and `custom`.
D_CONSTEXPR_INLINE const char*
html_doctype_string(
    html_version _v
)
{
    return (_v == html_version::html5)
            ? "<!DOCTYPE html>"
         : (_v == html_version::html4_strict)
            ? "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\""
              " \"http://www.w3.org/TR/html4/strict.dtd\">"
         : (_v == html_version::html4_transitional)
            ? "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01"
              " Transitional//EN\""
              " \"http://www.w3.org/TR/html4/loose.dtd\">"
         : (_v == html_version::html4_frameset)
            ? "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01"
              " Frameset//EN\""
              " \"http://www.w3.org/TR/html4/frameset.dtd\">"
         : (_v == html_version::xhtml1_strict)
            ? "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0"
              " Strict//EN\""
              " \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
         : (_v == html_version::xhtml1_transitional)
            ? "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0"
              " Transitional//EN\""
              " \"http://www.w3.org/TR/xhtml1/DTD/"
              "xhtml1-transitional.dtd\">"
         : (_v == html_version::xhtml1_frameset)
            ? "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0"
              " Frameset//EN\""
              " \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd\">"
         : (_v == html_version::xhtml5)
            ? "<!DOCTYPE html>"
         :    "";
}


// is_xhtml_version
//   function: true for any XHTML variant. XHTML demands stricter
// emission (lowercase tags, quoted attributes, self-closed voids).
D_CONSTEXPR_INLINE bool
is_xhtml_version(
    html_version _v
)
{
    return ( (_v == html_version::xhtml1_strict)       ||
             (_v == html_version::xhtml1_transitional) ||
             (_v == html_version::xhtml1_frameset)     ||
             (_v == html_version::xhtml5) );
}


///////////////////////////////////////////////////////////////////////////////
///                III.   ELEMENT KIND ENUM                                 ///
///////////////////////////////////////////////////////////////////////////////

// html_element_kind
//   enum: discriminator for every standard HTML5 element. Backed
// by `re_std::uint8_t` to keep storage minimal. The `unknown` value
// is used for elements not in this list (custom elements, web
// components, deprecated tags).
enum class html_element_kind : re_std::uint8_t
{
    // root / metadata
    html_root, head, title, base, link_, meta, style, script, noscript,

    // sectioning
    body, header, nav, main_, section, article, aside, footer, address,

    // headings
    h1, h2, h3, h4, h5, h6, hgroup,

    // text / paragraph
    p, hr, pre, blockquote, ol, ul, li, dl, dt, dd, figure,
    figcaption, div_,

    // inline text
    a, em, strong, small, s, cite, q, dfn, abbr, ruby, rt, rp,
    data_, time_, code, var_, samp, kbd, sub, sup, i, b, u, mark,
    bdi, bdo, span_, br, wbr,

    // edits
    ins, del_,

    // embedded
    img, iframe, embed, object_, param, video, audio, source,
    track, canvas, picture, map_, area, math, svg,

    // tables
    table, caption, colgroup, col, tbody, thead, tfoot, tr, td, th,

    // forms
    form, label, input, button, select_, datalist, optgroup,
    option, textarea, output, progress, meter, fieldset, legend,

    // interactive
    details, summary, dialog,

    // scripting
    template_, slot,

    // sentinel
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   ATTRIBUTE KIND ENUM                                ///
///////////////////////////////////////////////////////////////////////////////

// html_attribute_kind
//   enum: discriminator for the most common HTML attributes.
// The list covers the global attributes plus the attributes most
// frequently inspected by templating and DOM-walking code.
enum class html_attribute_kind : re_std::uint8_t
{
    // global
    id, class_, style, title, lang, dir, hidden, tabindex, accesskey,
    contenteditable, draggable, spellcheck, translate,

    // anchors / links
    href, hreflang, rel, target, download, referrerpolicy,

    // resources
    src, srcset, sizes, alt, type_, media,

    // form
    name, value, action, method, for_, enctype, autocomplete,
    autofocus, disabled, readonly_, required_, placeholder,
    checked_, selected_, multiple_, accept, pattern, min, max,
    step, maxlength, minlength,

    // dimensions
    width, height,

    // event / aria / data: bucketed
    on_event, aria_, data_attr,

    // sentinel
    unknown = 255
};


///////////////////////////////////////////////////////////////////////////////
///             V.   ELEMENT CATEGORY CLASSIFICATION                        ///
///////////////////////////////////////////////////////////////////////////////

// html_element_category
//   enum: bitmask describing the categorical properties of an
// element. An element may belong to several categories (e.g. `a`
// is both `flow` and `phrasing`). Backed by `unsigned`.
enum html_element_category : unsigned
{
    hec_none           = 0u,
    hec_void           = 1u <<  0,  // self-closing, no end tag
    hec_block          = 1u <<  1,  // block-level box by default
    hec_inline_        = 1u <<  2,  // inline-level box by default
    hec_metadata       = 1u <<  3,  // belongs in <head>
    hec_flow           = 1u <<  4,  // flow content
    hec_phrasing       = 1u <<  5,  // phrasing content
    hec_sectioning     = 1u <<  6,  // sectioning root
    hec_heading        = 1u <<  7,
    hec_form           = 1u <<  8,
    hec_form_assoc     = 1u <<  9,  // form-associated
    hec_interactive    = 1u << 10,
    hec_embedded       = 1u << 11,  // images, video, etc.
    hec_raw_text       = 1u << 12,  // <script>, <style>
    hec_escapable_raw  = 1u << 13,  // <textarea>, <title>
    hec_table          = 1u << 14,
    hec_palpable       = 1u << 15
};


// html_category_for_kind
//   function: returns the category bitmask for a given element
// kind. Constexpr; compiles to a sequence of comparisons that
// the optimiser folds into a small jump table. Returns
// `hec_none` for `unknown`.
D_CONSTEXPR_INLINE unsigned
html_category_for_kind(
    html_element_kind _k
)
{
    // void elements: area, base, br, col, embed, hr, img, input,
    // link, meta, param, source, track, wbr.
    return (_k == html_element_kind::area)   ? (hec_void | hec_phrasing | hec_flow)
         : (_k == html_element_kind::base)   ? (hec_void | hec_metadata)
         : (_k == html_element_kind::br)     ? (hec_void | hec_phrasing | hec_flow)
         : (_k == html_element_kind::col)    ? (hec_void | hec_table)
         : (_k == html_element_kind::embed)  ? (hec_void | hec_phrasing | hec_flow
                                                | hec_embedded | hec_interactive)
         : (_k == html_element_kind::hr)     ? (hec_void | hec_block | hec_flow)
         : (_k == html_element_kind::img)    ? (hec_void | hec_phrasing | hec_flow
                                                | hec_embedded | hec_palpable)
         : (_k == html_element_kind::input)  ? (hec_void | hec_phrasing | hec_flow
                                                | hec_form_assoc | hec_interactive
                                                | hec_palpable)
         : (_k == html_element_kind::link_)  ? (hec_void | hec_metadata | hec_flow)
         : (_k == html_element_kind::meta)   ? (hec_void | hec_metadata | hec_flow)
         : (_k == html_element_kind::param)  ? (hec_void)
         : (_k == html_element_kind::source) ? (hec_void)
         : (_k == html_element_kind::track)  ? (hec_void)
         : (_k == html_element_kind::wbr)    ? (hec_void | hec_phrasing | hec_flow)

         // metadata
         : (_k == html_element_kind::title)    ? (hec_metadata | hec_escapable_raw)
         : (_k == html_element_kind::style)    ? (hec_metadata | hec_raw_text)
         : (_k == html_element_kind::script)   ? (hec_metadata | hec_raw_text
                                                  | hec_phrasing | hec_flow)
         : (_k == html_element_kind::noscript) ? (hec_metadata | hec_phrasing
                                                  | hec_flow)
         : (_k == html_element_kind::head)     ? (hec_metadata)

         // sectioning
         : (_k == html_element_kind::html_root) ? (hec_block)
         : (_k == html_element_kind::body)      ? (hec_block | hec_sectioning
                                                   | hec_flow)
         : (_k == html_element_kind::header)    ? (hec_block | hec_flow
                                                   | hec_palpable)
         : (_k == html_element_kind::nav)       ? (hec_block | hec_sectioning
                                                   | hec_flow | hec_palpable)
         : (_k == html_element_kind::main_)     ? (hec_block | hec_flow
                                                   | hec_palpable)
         : (_k == html_element_kind::section)   ? (hec_block | hec_sectioning
                                                   | hec_flow | hec_palpable)
         : (_k == html_element_kind::article)   ? (hec_block | hec_sectioning
                                                   | hec_flow | hec_palpable)
         : (_k == html_element_kind::aside)     ? (hec_block | hec_sectioning
                                                   | hec_flow | hec_palpable)
         : (_k == html_element_kind::footer)    ? (hec_block | hec_flow
                                                   | hec_palpable)
         : (_k == html_element_kind::address)   ? (hec_block | hec_flow
                                                   | hec_palpable)

         // headings (uniform)
         : ( (_k == html_element_kind::h1) ||
             (_k == html_element_kind::h2) ||
             (_k == html_element_kind::h3) ||
             (_k == html_element_kind::h4) ||
             (_k == html_element_kind::h5) ||
             (_k == html_element_kind::h6) ||
             (_k == html_element_kind::hgroup) )
                                              ? (hec_block | hec_flow | hec_heading
                                                 | hec_palpable)

         // paragraph / list block
         : (_k == html_element_kind::p)          ? (hec_block | hec_flow
                                                    | hec_palpable)
         : (_k == html_element_kind::pre)        ? (hec_block | hec_flow
                                                    | hec_palpable)
         : (_k == html_element_kind::blockquote) ? (hec_block | hec_sectioning
                                                    | hec_flow | hec_palpable)
         : ( (_k == html_element_kind::ol) ||
             (_k == html_element_kind::ul) ||
             (_k == html_element_kind::dl) )
                                                 ? (hec_block | hec_flow
                                                    | hec_palpable)
         : ( (_k == html_element_kind::li) ||
             (_k == html_element_kind::dt) ||
             (_k == html_element_kind::dd) )
                                                 ? (hec_block)
         : (_k == html_element_kind::figure)     ? (hec_block | hec_sectioning
                                                    | hec_flow | hec_palpable)
         : (_k == html_element_kind::figcaption) ? (hec_block)
         : (_k == html_element_kind::div_)       ? (hec_block | hec_flow
                                                    | hec_palpable)

         // forms
         : (_k == html_element_kind::form)     ? (hec_block | hec_form
                                                  | hec_flow | hec_palpable)
         : (_k == html_element_kind::fieldset) ? (hec_block | hec_form_assoc
                                                  | hec_flow | hec_palpable)
         : (_k == html_element_kind::legend)   ? (hec_block)
         : (_k == html_element_kind::label)    ? (hec_inline_ | hec_phrasing
                                                  | hec_form_assoc | hec_flow
                                                  | hec_interactive
                                                  | hec_palpable)
         : (_k == html_element_kind::button)   ? (hec_inline_ | hec_phrasing
                                                  | hec_form_assoc | hec_flow
                                                  | hec_interactive
                                                  | hec_palpable)
         : (_k == html_element_kind::select_)  ? (hec_inline_ | hec_phrasing
                                                  | hec_form_assoc | hec_flow
                                                  | hec_interactive
                                                  | hec_palpable)
         : (_k == html_element_kind::textarea) ? (hec_inline_ | hec_escapable_raw
                                                  | hec_form_assoc | hec_phrasing
                                                  | hec_flow | hec_interactive
                                                  | hec_palpable)
         : ( (_k == html_element_kind::output)   ||
             (_k == html_element_kind::progress) ||
             (_k == html_element_kind::meter) )
                                                ? (hec_inline_ | hec_phrasing
                                                   | hec_form_assoc | hec_flow
                                                   | hec_palpable)
         : ( (_k == html_element_kind::option) ||
             (_k == html_element_kind::optgroup) )
                                                ? (hec_inline_)
         : (_k == html_element_kind::datalist)  ? (hec_inline_ | hec_phrasing
                                                   | hec_flow)

         // tables
         : (_k == html_element_kind::table)    ? (hec_block | hec_table | hec_flow
                                                  | hec_palpable)
         : (_k == html_element_kind::caption)  ? (hec_block | hec_table)
         : (_k == html_element_kind::colgroup) ? (hec_table)
         : ( (_k == html_element_kind::tbody) ||
             (_k == html_element_kind::thead) ||
             (_k == html_element_kind::tfoot) ||
             (_k == html_element_kind::tr) )
                                              ? (hec_block | hec_table)
         : ( (_k == html_element_kind::td) ||
             (_k == html_element_kind::th) )
                                              ? (hec_block | hec_table
                                                 | hec_sectioning)

         // interactive
         : (_k == html_element_kind::details) ? (hec_block | hec_flow
                                                 | hec_interactive
                                                 | hec_palpable)
         : (_k == html_element_kind::summary) ? (hec_block)
         : (_k == html_element_kind::dialog)  ? (hec_block | hec_flow
                                                 | hec_sectioning)

         // embedded
         : (_k == html_element_kind::iframe)  ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded
                                                 | hec_interactive
                                                 | hec_palpable)
         : (_k == html_element_kind::object_) ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded
                                                 | hec_form_assoc
                                                 | hec_palpable)
         : ( (_k == html_element_kind::video) ||
             (_k == html_element_kind::audio) )
                                              ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded
                                                 | hec_interactive
                                                 | hec_palpable)
         : (_k == html_element_kind::canvas)  ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded
                                                 | hec_palpable)
         : (_k == html_element_kind::picture) ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded)
         : (_k == html_element_kind::map_)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : ( (_k == html_element_kind::math) ||
             (_k == html_element_kind::svg) )
                                              ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded
                                                 | hec_palpable)

         // scripting / template
         : (_k == html_element_kind::template_) ? (hec_metadata | hec_phrasing
                                                   | hec_flow)
         : (_k == html_element_kind::slot)      ? (hec_inline_ | hec_phrasing
                                                   | hec_flow)

         // edits
         : ( (_k == html_element_kind::ins) ||
             (_k == html_element_kind::del_) )
                                              ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)

         // anchor
         : (_k == html_element_kind::a)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_interactive
                                                 | hec_palpable)

         // remaining inline phrasing elements (em, strong, small, s,
         // cite, q, dfn, abbr, ruby, rt, rp, data, time, code, var,
         // samp, kbd, sub, sup, i, b, u, mark, bdi, bdo, span)
         : (_k == html_element_kind::em)      ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::strong)  ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::small)   ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::s)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::cite)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::q)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::dfn)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::abbr)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::ruby)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : ( (_k == html_element_kind::rt) ||
             (_k == html_element_kind::rp) )
                                              ? (hec_inline_)
         : (_k == html_element_kind::data_)   ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::time_)   ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::code)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::var_)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::samp)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::kbd)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::sub)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::sup)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::i)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::b)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::u)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::mark)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::bdi)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::bdo)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == html_element_kind::span_)   ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)

         :    hec_none;
}


// is_void_element_kind
//   function: true if `_k` is a void element (no end tag).
D_CONSTEXPR_INLINE bool
is_void_element_kind(
    html_element_kind _k
)
{
    return ((html_category_for_kind(_k) & hec_void) != 0u);
}


// is_block_element_kind
//   function: true if `_k` is a block-level element.
D_CONSTEXPR_INLINE bool
is_block_element_kind(
    html_element_kind _k
)
{
    return ((html_category_for_kind(_k) & hec_block) != 0u);
}


// is_inline_element_kind
//   function: true if `_k` is an inline-level element.
D_CONSTEXPR_INLINE bool
is_inline_element_kind(
    html_element_kind _k
)
{
    return ((html_category_for_kind(_k) & hec_inline_) != 0u);
}


// is_raw_text_element_kind
//   function: true if `_k` has raw-text content (script, style).
// Such elements must NOT have their text escaped on emission.
D_CONSTEXPR_INLINE bool
is_raw_text_element_kind(
    html_element_kind _k
)
{
    return ((html_category_for_kind(_k) & hec_raw_text) != 0u);
}


// is_form_element_kind
//   function: true if `_k` is `<form>` or a form-associated control.
D_CONSTEXPR_INLINE bool
is_form_element_kind(
    html_element_kind _k
)
{
    return ( (html_category_for_kind(_k) &
              (hec_form | hec_form_assoc)) != 0u );
}


// is_metadata_element_kind
//   function: true if `_k` belongs in `<head>`.
D_CONSTEXPR_INLINE bool
is_metadata_element_kind(
    html_element_kind _k
)
{
    return ((html_category_for_kind(_k) & hec_metadata) != 0u);
}


///////////////////////////////////////////////////////////////////////////////
///                VI.   TAG-NAME / ATTR-NAME NAMESPACES                    ///
///////////////////////////////////////////////////////////////////////////////

// tags
//   namespace: `D_STATIC_CONSTEXPR const char*` constants for every
// HTML5 element name. Use these instead of literal strings when
// calling `add_child`, `find_child`, etc. Zero-overhead: each is a
// single pointer to a string literal in `.rodata`.
namespace tags {

    D_STATIC_CONSTEXPR const char* html_root  = "html";
    D_STATIC_CONSTEXPR const char* head       = "head";
    D_STATIC_CONSTEXPR const char* title      = "title";
    D_STATIC_CONSTEXPR const char* base       = "base";
    D_STATIC_CONSTEXPR const char* link_      = "link";
    D_STATIC_CONSTEXPR const char* meta       = "meta";
    D_STATIC_CONSTEXPR const char* style      = "style";
    D_STATIC_CONSTEXPR const char* script     = "script";
    D_STATIC_CONSTEXPR const char* noscript   = "noscript";

    D_STATIC_CONSTEXPR const char* body       = "body";
    D_STATIC_CONSTEXPR const char* header     = "header";
    D_STATIC_CONSTEXPR const char* nav        = "nav";
    D_STATIC_CONSTEXPR const char* main_      = "main";
    D_STATIC_CONSTEXPR const char* section    = "section";
    D_STATIC_CONSTEXPR const char* article    = "article";
    D_STATIC_CONSTEXPR const char* aside      = "aside";
    D_STATIC_CONSTEXPR const char* footer     = "footer";
    D_STATIC_CONSTEXPR const char* address    = "address";

    D_STATIC_CONSTEXPR const char* h1         = "h1";
    D_STATIC_CONSTEXPR const char* h2         = "h2";
    D_STATIC_CONSTEXPR const char* h3         = "h3";
    D_STATIC_CONSTEXPR const char* h4         = "h4";
    D_STATIC_CONSTEXPR const char* h5         = "h5";
    D_STATIC_CONSTEXPR const char* h6         = "h6";
    D_STATIC_CONSTEXPR const char* hgroup     = "hgroup";

    D_STATIC_CONSTEXPR const char* p          = "p";
    D_STATIC_CONSTEXPR const char* hr         = "hr";
    D_STATIC_CONSTEXPR const char* pre        = "pre";
    D_STATIC_CONSTEXPR const char* blockquote = "blockquote";
    D_STATIC_CONSTEXPR const char* ol         = "ol";
    D_STATIC_CONSTEXPR const char* ul         = "ul";
    D_STATIC_CONSTEXPR const char* li         = "li";
    D_STATIC_CONSTEXPR const char* dl         = "dl";
    D_STATIC_CONSTEXPR const char* dt         = "dt";
    D_STATIC_CONSTEXPR const char* dd         = "dd";
    D_STATIC_CONSTEXPR const char* figure     = "figure";
    D_STATIC_CONSTEXPR const char* figcaption = "figcaption";
    D_STATIC_CONSTEXPR const char* div_       = "div";

    D_STATIC_CONSTEXPR const char* a          = "a";
    D_STATIC_CONSTEXPR const char* em         = "em";
    D_STATIC_CONSTEXPR const char* strong     = "strong";
    D_STATIC_CONSTEXPR const char* small      = "small";
    D_STATIC_CONSTEXPR const char* s          = "s";
    D_STATIC_CONSTEXPR const char* cite       = "cite";
    D_STATIC_CONSTEXPR const char* q          = "q";
    D_STATIC_CONSTEXPR const char* dfn        = "dfn";
    D_STATIC_CONSTEXPR const char* abbr       = "abbr";
    D_STATIC_CONSTEXPR const char* ruby       = "ruby";
    D_STATIC_CONSTEXPR const char* rt         = "rt";
    D_STATIC_CONSTEXPR const char* rp         = "rp";
    D_STATIC_CONSTEXPR const char* data_      = "data";
    D_STATIC_CONSTEXPR const char* time_      = "time";
    D_STATIC_CONSTEXPR const char* code       = "code";
    D_STATIC_CONSTEXPR const char* var_       = "var";
    D_STATIC_CONSTEXPR const char* samp       = "samp";
    D_STATIC_CONSTEXPR const char* kbd        = "kbd";
    D_STATIC_CONSTEXPR const char* sub        = "sub";
    D_STATIC_CONSTEXPR const char* sup        = "sup";
    D_STATIC_CONSTEXPR const char* i          = "i";
    D_STATIC_CONSTEXPR const char* b          = "b";
    D_STATIC_CONSTEXPR const char* u          = "u";
    D_STATIC_CONSTEXPR const char* mark       = "mark";
    D_STATIC_CONSTEXPR const char* bdi        = "bdi";
    D_STATIC_CONSTEXPR const char* bdo        = "bdo";
    D_STATIC_CONSTEXPR const char* span_      = "span";
    D_STATIC_CONSTEXPR const char* br         = "br";
    D_STATIC_CONSTEXPR const char* wbr        = "wbr";

    D_STATIC_CONSTEXPR const char* ins        = "ins";
    D_STATIC_CONSTEXPR const char* del_       = "del";

    D_STATIC_CONSTEXPR const char* img        = "img";
    D_STATIC_CONSTEXPR const char* iframe     = "iframe";
    D_STATIC_CONSTEXPR const char* embed      = "embed";
    D_STATIC_CONSTEXPR const char* object_    = "object";
    D_STATIC_CONSTEXPR const char* param      = "param";
    D_STATIC_CONSTEXPR const char* video      = "video";
    D_STATIC_CONSTEXPR const char* audio      = "audio";
    D_STATIC_CONSTEXPR const char* source     = "source";
    D_STATIC_CONSTEXPR const char* track      = "track";
    D_STATIC_CONSTEXPR const char* canvas     = "canvas";
    D_STATIC_CONSTEXPR const char* picture    = "picture";
    D_STATIC_CONSTEXPR const char* map_       = "map";
    D_STATIC_CONSTEXPR const char* area       = "area";
    D_STATIC_CONSTEXPR const char* math       = "math";
    D_STATIC_CONSTEXPR const char* svg        = "svg";

    D_STATIC_CONSTEXPR const char* table      = "table";
    D_STATIC_CONSTEXPR const char* caption    = "caption";
    D_STATIC_CONSTEXPR const char* colgroup   = "colgroup";
    D_STATIC_CONSTEXPR const char* col        = "col";
    D_STATIC_CONSTEXPR const char* tbody      = "tbody";
    D_STATIC_CONSTEXPR const char* thead      = "thead";
    D_STATIC_CONSTEXPR const char* tfoot      = "tfoot";
    D_STATIC_CONSTEXPR const char* tr         = "tr";
    D_STATIC_CONSTEXPR const char* td         = "td";
    D_STATIC_CONSTEXPR const char* th         = "th";

    D_STATIC_CONSTEXPR const char* form       = "form";
    D_STATIC_CONSTEXPR const char* label      = "label";
    D_STATIC_CONSTEXPR const char* input      = "input";
    D_STATIC_CONSTEXPR const char* button     = "button";
    D_STATIC_CONSTEXPR const char* select_    = "select";
    D_STATIC_CONSTEXPR const char* datalist   = "datalist";
    D_STATIC_CONSTEXPR const char* optgroup   = "optgroup";
    D_STATIC_CONSTEXPR const char* option     = "option";
    D_STATIC_CONSTEXPR const char* textarea   = "textarea";
    D_STATIC_CONSTEXPR const char* output     = "output";
    D_STATIC_CONSTEXPR const char* progress   = "progress";
    D_STATIC_CONSTEXPR const char* meter      = "meter";
    D_STATIC_CONSTEXPR const char* fieldset   = "fieldset";
    D_STATIC_CONSTEXPR const char* legend     = "legend";

    D_STATIC_CONSTEXPR const char* details    = "details";
    D_STATIC_CONSTEXPR const char* summary    = "summary";
    D_STATIC_CONSTEXPR const char* dialog     = "dialog";

    D_STATIC_CONSTEXPR const char* template_  = "template";
    D_STATIC_CONSTEXPR const char* slot       = "slot";

}   // namespace tags


// attrs
//   namespace: `D_STATIC_CONSTEXPR const char*` constants for the
// most common HTML attribute names.
namespace attrs {

    D_STATIC_CONSTEXPR const char* id              = "id";
    D_STATIC_CONSTEXPR const char* class_          = "class";
    D_STATIC_CONSTEXPR const char* style           = "style";
    D_STATIC_CONSTEXPR const char* title           = "title";
    D_STATIC_CONSTEXPR const char* lang            = "lang";
    D_STATIC_CONSTEXPR const char* dir             = "dir";
    D_STATIC_CONSTEXPR const char* hidden          = "hidden";
    D_STATIC_CONSTEXPR const char* tabindex        = "tabindex";
    D_STATIC_CONSTEXPR const char* accesskey       = "accesskey";
    D_STATIC_CONSTEXPR const char* contenteditable = "contenteditable";
    D_STATIC_CONSTEXPR const char* draggable       = "draggable";
    D_STATIC_CONSTEXPR const char* spellcheck      = "spellcheck";
    D_STATIC_CONSTEXPR const char* translate       = "translate";

    D_STATIC_CONSTEXPR const char* href            = "href";
    D_STATIC_CONSTEXPR const char* hreflang        = "hreflang";
    D_STATIC_CONSTEXPR const char* rel             = "rel";
    D_STATIC_CONSTEXPR const char* target          = "target";
    D_STATIC_CONSTEXPR const char* download        = "download";
    D_STATIC_CONSTEXPR const char* referrerpolicy  = "referrerpolicy";

    D_STATIC_CONSTEXPR const char* src             = "src";
    D_STATIC_CONSTEXPR const char* srcset          = "srcset";
    D_STATIC_CONSTEXPR const char* sizes           = "sizes";
    D_STATIC_CONSTEXPR const char* alt             = "alt";
    D_STATIC_CONSTEXPR const char* type_           = "type";
    D_STATIC_CONSTEXPR const char* media           = "media";

    D_STATIC_CONSTEXPR const char* name            = "name";
    D_STATIC_CONSTEXPR const char* value           = "value";
    D_STATIC_CONSTEXPR const char* action          = "action";
    D_STATIC_CONSTEXPR const char* method          = "method";
    D_STATIC_CONSTEXPR const char* for_            = "for";
    D_STATIC_CONSTEXPR const char* enctype         = "enctype";
    D_STATIC_CONSTEXPR const char* autocomplete    = "autocomplete";
    D_STATIC_CONSTEXPR const char* autofocus       = "autofocus";
    D_STATIC_CONSTEXPR const char* disabled        = "disabled";
    D_STATIC_CONSTEXPR const char* readonly_       = "readonly";
    D_STATIC_CONSTEXPR const char* required_       = "required";
    D_STATIC_CONSTEXPR const char* placeholder     = "placeholder";
    D_STATIC_CONSTEXPR const char* checked_        = "checked";
    D_STATIC_CONSTEXPR const char* selected_       = "selected";
    D_STATIC_CONSTEXPR const char* multiple_       = "multiple";
    D_STATIC_CONSTEXPR const char* accept          = "accept";
    D_STATIC_CONSTEXPR const char* pattern         = "pattern";
    D_STATIC_CONSTEXPR const char* min             = "min";
    D_STATIC_CONSTEXPR const char* max             = "max";
    D_STATIC_CONSTEXPR const char* step            = "step";
    D_STATIC_CONSTEXPR const char* maxlength       = "maxlength";
    D_STATIC_CONSTEXPR const char* minlength       = "minlength";

    D_STATIC_CONSTEXPR const char* width           = "width";
    D_STATIC_CONSTEXPR const char* height          = "height";

    // common ARIA / data prefixes (use these for prefix matching).
    D_STATIC_CONSTEXPR const char* aria_prefix     = "aria-";
    D_STATIC_CONSTEXPR const char* data_prefix     = "data-";

}   // namespace attrs


///////////////////////////////////////////////////////////////////////////////
///                VII.   KIND <-> NAME MAPPING                             ///
///////////////////////////////////////////////////////////////////////////////

// internal_streq
//   function: constexpr null-terminated string equality.
D_CONSTEXPR_INLINE bool
internal_streq(
    const char* _a,
    const char* _b
)
{
    return ( (*_a == *_b) &&
             ( (*_a == '\0') ||
               (internal_streq(_a + 1, _b + 1)) ) );
}


// html_kind_from_name
//   function: maps an element tag name (case-sensitive,
// lowercase) to its `html_element_kind`. Returns
// `html_element_kind::unknown` for unrecognised tags.
D_CONSTEXPR_INLINE html_element_kind
html_kind_from_name(
    const char* _name
)
{
    return (!_name)                                  ? html_element_kind::unknown
         : (internal_streq(_name, tags::html_root))  ? html_element_kind::html_root
         : (internal_streq(_name, tags::head))       ? html_element_kind::head
         : (internal_streq(_name, tags::title))      ? html_element_kind::title
         : (internal_streq(_name, tags::base))       ? html_element_kind::base
         : (internal_streq(_name, tags::link_))      ? html_element_kind::link_
         : (internal_streq(_name, tags::meta))       ? html_element_kind::meta
         : (internal_streq(_name, tags::style))      ? html_element_kind::style
         : (internal_streq(_name, tags::script))     ? html_element_kind::script
         : (internal_streq(_name, tags::noscript))   ? html_element_kind::noscript
         : (internal_streq(_name, tags::body))       ? html_element_kind::body
         : (internal_streq(_name, tags::header))     ? html_element_kind::header
         : (internal_streq(_name, tags::nav))        ? html_element_kind::nav
         : (internal_streq(_name, tags::main_))      ? html_element_kind::main_
         : (internal_streq(_name, tags::section))    ? html_element_kind::section
         : (internal_streq(_name, tags::article))    ? html_element_kind::article
         : (internal_streq(_name, tags::aside))      ? html_element_kind::aside
         : (internal_streq(_name, tags::footer))     ? html_element_kind::footer
         : (internal_streq(_name, tags::address))    ? html_element_kind::address
         : (internal_streq(_name, tags::h1))         ? html_element_kind::h1
         : (internal_streq(_name, tags::h2))         ? html_element_kind::h2
         : (internal_streq(_name, tags::h3))         ? html_element_kind::h3
         : (internal_streq(_name, tags::h4))         ? html_element_kind::h4
         : (internal_streq(_name, tags::h5))         ? html_element_kind::h5
         : (internal_streq(_name, tags::h6))         ? html_element_kind::h6
         : (internal_streq(_name, tags::hgroup))     ? html_element_kind::hgroup
         : (internal_streq(_name, tags::p))          ? html_element_kind::p
         : (internal_streq(_name, tags::hr))         ? html_element_kind::hr
         : (internal_streq(_name, tags::pre))        ? html_element_kind::pre
         : (internal_streq(_name, tags::blockquote)) ? html_element_kind::blockquote
         : (internal_streq(_name, tags::ol))         ? html_element_kind::ol
         : (internal_streq(_name, tags::ul))         ? html_element_kind::ul
         : (internal_streq(_name, tags::li))         ? html_element_kind::li
         : (internal_streq(_name, tags::dl))         ? html_element_kind::dl
         : (internal_streq(_name, tags::dt))         ? html_element_kind::dt
         : (internal_streq(_name, tags::dd))         ? html_element_kind::dd
         : (internal_streq(_name, tags::figure))     ? html_element_kind::figure
         : (internal_streq(_name, tags::figcaption)) ? html_element_kind::figcaption
         : (internal_streq(_name, tags::div_))       ? html_element_kind::div_
         : (internal_streq(_name, tags::a))          ? html_element_kind::a
         : (internal_streq(_name, tags::em))         ? html_element_kind::em
         : (internal_streq(_name, tags::strong))     ? html_element_kind::strong
         : (internal_streq(_name, tags::small))      ? html_element_kind::small
         : (internal_streq(_name, tags::s))          ? html_element_kind::s
         : (internal_streq(_name, tags::cite))       ? html_element_kind::cite
         : (internal_streq(_name, tags::q))          ? html_element_kind::q
         : (internal_streq(_name, tags::dfn))        ? html_element_kind::dfn
         : (internal_streq(_name, tags::abbr))       ? html_element_kind::abbr
         : (internal_streq(_name, tags::code))       ? html_element_kind::code
         : (internal_streq(_name, tags::var_))       ? html_element_kind::var_
         : (internal_streq(_name, tags::samp))       ? html_element_kind::samp
         : (internal_streq(_name, tags::kbd))        ? html_element_kind::kbd
         : (internal_streq(_name, tags::sub))        ? html_element_kind::sub
         : (internal_streq(_name, tags::sup))        ? html_element_kind::sup
         : (internal_streq(_name, tags::i))          ? html_element_kind::i
         : (internal_streq(_name, tags::b))          ? html_element_kind::b
         : (internal_streq(_name, tags::u))          ? html_element_kind::u
         : (internal_streq(_name, tags::mark))       ? html_element_kind::mark
         : (internal_streq(_name, tags::span_))      ? html_element_kind::span_
         : (internal_streq(_name, tags::br))         ? html_element_kind::br
         : (internal_streq(_name, tags::wbr))        ? html_element_kind::wbr
         : (internal_streq(_name, tags::img))        ? html_element_kind::img
         : (internal_streq(_name, tags::iframe))     ? html_element_kind::iframe
         : (internal_streq(_name, tags::embed))      ? html_element_kind::embed
         : (internal_streq(_name, tags::object_))    ? html_element_kind::object_
         : (internal_streq(_name, tags::video))      ? html_element_kind::video
         : (internal_streq(_name, tags::audio))      ? html_element_kind::audio
         : (internal_streq(_name, tags::source))     ? html_element_kind::source
         : (internal_streq(_name, tags::track))      ? html_element_kind::track
         : (internal_streq(_name, tags::canvas))     ? html_element_kind::canvas
         : (internal_streq(_name, tags::picture))    ? html_element_kind::picture
         : (internal_streq(_name, tags::map_))       ? html_element_kind::map_
         : (internal_streq(_name, tags::area))       ? html_element_kind::area
         : (internal_streq(_name, tags::svg))        ? html_element_kind::svg
         : (internal_streq(_name, tags::table))      ? html_element_kind::table
         : (internal_streq(_name, tags::caption))    ? html_element_kind::caption
         : (internal_streq(_name, tags::colgroup))   ? html_element_kind::colgroup
         : (internal_streq(_name, tags::col))        ? html_element_kind::col
         : (internal_streq(_name, tags::tbody))      ? html_element_kind::tbody
         : (internal_streq(_name, tags::thead))      ? html_element_kind::thead
         : (internal_streq(_name, tags::tfoot))      ? html_element_kind::tfoot
         : (internal_streq(_name, tags::tr))         ? html_element_kind::tr
         : (internal_streq(_name, tags::td))         ? html_element_kind::td
         : (internal_streq(_name, tags::th))         ? html_element_kind::th
         : (internal_streq(_name, tags::form))       ? html_element_kind::form
         : (internal_streq(_name, tags::label))      ? html_element_kind::label
         : (internal_streq(_name, tags::input))      ? html_element_kind::input
         : (internal_streq(_name, tags::button))     ? html_element_kind::button
         : (internal_streq(_name, tags::select_))    ? html_element_kind::select_
         : (internal_streq(_name, tags::option))     ? html_element_kind::option
         : (internal_streq(_name, tags::textarea))   ? html_element_kind::textarea
         : (internal_streq(_name, tags::fieldset))   ? html_element_kind::fieldset
         : (internal_streq(_name, tags::legend))     ? html_element_kind::legend
         : (internal_streq(_name, tags::details))    ? html_element_kind::details
         : (internal_streq(_name, tags::summary))    ? html_element_kind::summary
         : (internal_streq(_name, tags::dialog))     ? html_element_kind::dialog
         : (internal_streq(_name, tags::template_))  ? html_element_kind::template_
         :    html_element_kind::unknown;
}


// name_from_html_kind
//   function: maps an `html_element_kind` to its tag-name string.
// Returns the empty string for `unknown`.
D_CONSTEXPR_INLINE const char*
name_from_html_kind(
    html_element_kind _k
)
{
    return (_k == html_element_kind::html_root)  ? tags::html_root
         : (_k == html_element_kind::head)       ? tags::head
         : (_k == html_element_kind::title)      ? tags::title
         : (_k == html_element_kind::base)       ? tags::base
         : (_k == html_element_kind::link_)      ? tags::link_
         : (_k == html_element_kind::meta)       ? tags::meta
         : (_k == html_element_kind::style)      ? tags::style
         : (_k == html_element_kind::script)     ? tags::script
         : (_k == html_element_kind::noscript)   ? tags::noscript
         : (_k == html_element_kind::body)       ? tags::body
         : (_k == html_element_kind::header)     ? tags::header
         : (_k == html_element_kind::nav)        ? tags::nav
         : (_k == html_element_kind::main_)      ? tags::main_
         : (_k == html_element_kind::section)    ? tags::section
         : (_k == html_element_kind::article)    ? tags::article
         : (_k == html_element_kind::aside)      ? tags::aside
         : (_k == html_element_kind::footer)     ? tags::footer
         : (_k == html_element_kind::address)    ? tags::address
         : (_k == html_element_kind::h1)         ? tags::h1
         : (_k == html_element_kind::h2)         ? tags::h2
         : (_k == html_element_kind::h3)         ? tags::h3
         : (_k == html_element_kind::h4)         ? tags::h4
         : (_k == html_element_kind::h5)         ? tags::h5
         : (_k == html_element_kind::h6)         ? tags::h6
         : (_k == html_element_kind::hgroup)     ? tags::hgroup
         : (_k == html_element_kind::p)          ? tags::p
         : (_k == html_element_kind::hr)         ? tags::hr
         : (_k == html_element_kind::pre)        ? tags::pre
         : (_k == html_element_kind::blockquote) ? tags::blockquote
         : (_k == html_element_kind::ol)         ? tags::ol
         : (_k == html_element_kind::ul)         ? tags::ul
         : (_k == html_element_kind::li)         ? tags::li
         : (_k == html_element_kind::dl)         ? tags::dl
         : (_k == html_element_kind::dt)         ? tags::dt
         : (_k == html_element_kind::dd)         ? tags::dd
         : (_k == html_element_kind::figure)     ? tags::figure
         : (_k == html_element_kind::figcaption) ? tags::figcaption
         : (_k == html_element_kind::div_)       ? tags::div_
         : (_k == html_element_kind::a)          ? tags::a
         : (_k == html_element_kind::em)         ? tags::em
         : (_k == html_element_kind::strong)     ? tags::strong
         : (_k == html_element_kind::small)      ? tags::small
         : (_k == html_element_kind::s)          ? tags::s
         : (_k == html_element_kind::cite)       ? tags::cite
         : (_k == html_element_kind::q)          ? tags::q
         : (_k == html_element_kind::dfn)        ? tags::dfn
         : (_k == html_element_kind::abbr)       ? tags::abbr
         : (_k == html_element_kind::code)       ? tags::code
         : (_k == html_element_kind::var_)       ? tags::var_
         : (_k == html_element_kind::samp)       ? tags::samp
         : (_k == html_element_kind::kbd)        ? tags::kbd
         : (_k == html_element_kind::sub)        ? tags::sub
         : (_k == html_element_kind::sup)        ? tags::sup
         : (_k == html_element_kind::i)          ? tags::i
         : (_k == html_element_kind::b)          ? tags::b
         : (_k == html_element_kind::u)          ? tags::u
         : (_k == html_element_kind::mark)       ? tags::mark
         : (_k == html_element_kind::span_)      ? tags::span_
         : (_k == html_element_kind::br)         ? tags::br
         : (_k == html_element_kind::wbr)        ? tags::wbr
         : (_k == html_element_kind::img)        ? tags::img
         : (_k == html_element_kind::iframe)     ? tags::iframe
         : (_k == html_element_kind::embed)      ? tags::embed
         : (_k == html_element_kind::object_)    ? tags::object_
         : (_k == html_element_kind::video)      ? tags::video
         : (_k == html_element_kind::audio)      ? tags::audio
         : (_k == html_element_kind::source)     ? tags::source
         : (_k == html_element_kind::track)      ? tags::track
         : (_k == html_element_kind::canvas)     ? tags::canvas
         : (_k == html_element_kind::picture)    ? tags::picture
         : (_k == html_element_kind::map_)       ? tags::map_
         : (_k == html_element_kind::area)       ? tags::area
         : (_k == html_element_kind::svg)        ? tags::svg
         : (_k == html_element_kind::table)      ? tags::table
         : (_k == html_element_kind::caption)    ? tags::caption
         : (_k == html_element_kind::colgroup)   ? tags::colgroup
         : (_k == html_element_kind::col)        ? tags::col
         : (_k == html_element_kind::tbody)      ? tags::tbody
         : (_k == html_element_kind::thead)      ? tags::thead
         : (_k == html_element_kind::tfoot)      ? tags::tfoot
         : (_k == html_element_kind::tr)         ? tags::tr
         : (_k == html_element_kind::td)         ? tags::td
         : (_k == html_element_kind::th)         ? tags::th
         : (_k == html_element_kind::form)       ? tags::form
         : (_k == html_element_kind::label)      ? tags::label
         : (_k == html_element_kind::input)      ? tags::input
         : (_k == html_element_kind::button)     ? tags::button
         : (_k == html_element_kind::select_)    ? tags::select_
         : (_k == html_element_kind::option)     ? tags::option
         : (_k == html_element_kind::textarea)   ? tags::textarea
         : (_k == html_element_kind::fieldset)   ? tags::fieldset
         : (_k == html_element_kind::legend)     ? tags::legend
         : (_k == html_element_kind::details)    ? tags::details
         : (_k == html_element_kind::summary)    ? tags::summary
         : (_k == html_element_kind::dialog)     ? tags::dialog
         : (_k == html_element_kind::template_)  ? tags::template_
         :    "";
}


// is_void_element_name
//   function: classifies an element by tag name.
D_CONSTEXPR_INLINE bool
is_void_element_name(
    const char* _name
)
{
    return is_void_element_kind(html_kind_from_name(_name));
}


// is_block_element_name
D_CONSTEXPR_INLINE bool
is_block_element_name(
    const char* _name
)
{
    return is_block_element_kind(html_kind_from_name(_name));
}


// is_inline_element_name
D_CONSTEXPR_INLINE bool
is_inline_element_name(
    const char* _name
)
{
    return is_inline_element_kind(html_kind_from_name(_name));
}


// is_raw_text_element_name
D_CONSTEXPR_INLINE bool
is_raw_text_element_name(
    const char* _name
)
{
    return is_raw_text_element_kind(html_kind_from_name(_name));
}


///////////////////////////////////////////////////////////////////////////////
///                VIII.   BACKEND TAG DISPATCH                             ///
///////////////////////////////////////////////////////////////////////////////

// html_backend_tag
//   struct: empty base tag for all HTML backend tag types.
struct html_backend_tag
{};

// html_default_backend_tag
//   struct: tag identifying the bundled in-memory HTML backend.
struct html_default_backend_tag : html_backend_tag
{};

// html_libxmlpp_backend_tag
//   struct: tag identifying a libxml++ HTML-mode adapter backend.
struct html_libxmlpp_backend_tag : html_backend_tag
{};

// html_gumbo_backend_tag
//   struct: tag identifying a gumbo-parser-based backend.
struct html_gumbo_backend_tag : html_backend_tag
{};

// html_lexbor_backend_tag
//   struct: tag identifying a Lexbor-based backend.
struct html_lexbor_backend_tag : html_backend_tag
{};

// html_htmlcxx_backend_tag
//   struct: tag identifying an htmlcxx-based backend.
struct html_htmlcxx_backend_tag : html_backend_tag
{};


///////////////////////////////////////////////////////////////////////////////
///                IX.   BACKEND DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

// is_html_backend
//   trait: detects whether Type is an HTML backend type by
// checking for a nested `html_backend_tag` alias.

}   // namespace html

NS_INTERNAL

    // has_html_backend_tag_helper
    //   trait: SFINAE helper; primary template (failure case).
    template<typename Type,
             typename = void>
    struct has_html_backend_tag_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // has_html_backend_tag_helper (specialization)
    //   trait: success case when Type::html_backend_tag exists.
    template<typename Type>
    struct has_html_backend_tag_helper<
        Type,
        void_t<typename Type::html_backend_tag>
    >
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal

namespace html {

// is_html_backend
//   trait: true if Type has a nested html_backend_tag type.
template<typename Type>
struct is_html_backend
{
    D_STATIC_CONSTEXPR bool value =
        internal::has_html_backend_tag_helper<clean_t<Type>>::value;
};

// is_html_backend_v
//   constant: convenience accessor for
// is_html_backend<Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_STATIC_CONSTEXPR bool is_html_backend_v =
        is_html_backend<Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///                X.   HTML KIND ACCESSOR DETECTION                        ///
///////////////////////////////////////////////////////////////////////////////

// has_html_kind_method
//   trait: true if Type exposes an `html_kind()` const method
// returning an `html_element_kind` discriminator. This is the
// HTML-specific analogue of XML's `has_kind_method` and is what
// lets generic code branch on element kind without reading the
// tag-name string.
template<typename Type,
         typename = void>
struct has_html_kind_method : std::false_type
{};

template<typename Type>
struct has_html_kind_method<Type, void_t<
    decltype(std::declval<const Type&>().html_kind())
>> : std::true_type
{};


// has_get_html_kind_method
//   trait: true if Type exposes `get_html_kind()` (getter-form).
template<typename Type,
         typename = void>
struct has_get_html_kind_method : std::false_type
{};

template<typename Type>
struct has_get_html_kind_method<Type, void_t<
    decltype(std::declval<const Type&>().get_html_kind())
>> : std::true_type
{};


// has_html_kind_access
//   trait: true if Type exposes html_kind in either form.
template<typename Type>
struct has_html_kind_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_html_kind_method<Type>::value ||
          has_get_html_kind_method<Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                XI.   CLASS LIST DETECTION                               ///
///////////////////////////////////////////////////////////////////////////////

// has_class_list_method
//   trait: true if Type exposes `class_list()` const returning
// an iterable of class-name strings.
template<typename Type,
         typename = void>
struct has_class_list_method : std::false_type
{};

template<typename Type>
struct has_class_list_method<Type, void_t<
    decltype(std::declval<const Type&>().class_list())
>> : std::true_type
{};


// has_add_class_method
//   trait: true if Type exposes `add_class(const std::string&)`.
template<typename Type,
         typename = void>
struct has_add_class_method : std::false_type
{};

template<typename Type>
struct has_add_class_method<Type, void_t<
    decltype(std::declval<Type&>().add_class(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_remove_class_method
//   trait: true if Type exposes `remove_class(const std::string&)`.
template<typename Type,
         typename = void>
struct has_remove_class_method : std::false_type
{};

template<typename Type>
struct has_remove_class_method<Type, void_t<
    decltype(std::declval<Type&>().remove_class(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_has_class_method
//   trait: true if Type exposes `has_class(const std::string&)`
// returning a boolean.
template<typename Type,
         typename = void>
struct has_has_class_method : std::false_type
{};

template<typename Type>
struct has_has_class_method<Type, void_t<
    decltype(std::declval<const Type&>().has_class(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_id_method
//   trait: true if Type exposes `get_id()` const returning a
// string-convertible id value. Both `id()` and `get_id()` forms
// detected.
template<typename Type,
         typename = void>
struct has_id_method : std::false_type
{};

template<typename Type>
struct has_id_method<Type, void_t<
    decltype(std::declval<const Type&>().get_id())
>> : std::true_type
{};


// has_set_id_method
//   trait: true if Type exposes `set_id(const std::string&)`.
template<typename Type,
         typename = void>
struct has_set_id_method : std::false_type
{};

template<typename Type>
struct has_set_id_method<Type, void_t<
    decltype(std::declval<Type&>().set_id(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_class_support
//   trait: true if Type exposes the full class-list interface
// (has_class + add_class + remove_class).
template<typename Type>
struct has_class_support
{
    D_STATIC_CONSTEXPR bool value =
        ( has_has_class_method<Type>::value    &&
          has_add_class_method<Type>::value    &&
          has_remove_class_method<Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                XII.   DOCTYPE & VERSION DETECTION                       ///
///////////////////////////////////////////////////////////////////////////////

// has_doctype_method
//   trait: true if Type exposes `doctype()` const returning a
// string-convertible DOCTYPE declaration.
template<typename Type,
         typename = void>
struct has_doctype_method : std::false_type
{};

template<typename Type>
struct has_doctype_method<Type, void_t<
    decltype(std::declval<const Type&>().doctype())
>> : std::true_type
{};


// has_get_doctype_method
//   trait: true if Type exposes `get_doctype()` const.
template<typename Type,
         typename = void>
struct has_get_doctype_method : std::false_type
{};

template<typename Type>
struct has_get_doctype_method<Type, void_t<
    decltype(std::declval<const Type&>().get_doctype())
>> : std::true_type
{};


// has_doctype_access
//   trait: true if Type exposes a doctype accessor in either form.
template<typename Type>
struct has_doctype_access
{
    D_STATIC_CONSTEXPR bool value =
        ( has_doctype_method<Type>::value ||
          has_get_doctype_method<Type>::value );
};


// has_set_doctype_method
//   trait: true if Type exposes `set_doctype(const std::string&)`.
template<typename Type,
         typename = void>
struct has_set_doctype_method : std::false_type
{};

template<typename Type>
struct has_set_doctype_method<Type, void_t<
    decltype(std::declval<Type&>().set_doctype(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_html_version_method
//   trait: true if Type exposes `html_version()` const returning
// an `html_version` enum value.
template<typename Type,
         typename = void>
struct has_html_version_method : std::false_type
{};

template<typename Type>
struct has_html_version_method<Type, void_t<
    decltype(std::declval<const Type&>().html_version())
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                XIII.   HEAD / BODY ACCESSOR DETECTION                   ///
///////////////////////////////////////////////////////////////////////////////

// has_head_element_method
//   trait: true if Type exposes `head_element()` const returning
// a node-shaped facade for the `<head>` element.
template<typename Type,
         typename = void>
struct has_head_element_method : std::false_type
{};

template<typename Type>
struct has_head_element_method<Type, void_t<
    decltype(std::declval<const Type&>().head_element())
>> : std::true_type
{};


// has_body_element_method
//   trait: true if Type exposes `body_element()` const returning
// a node-shaped facade for the `<body>` element.
template<typename Type,
         typename = void>
struct has_body_element_method : std::false_type
{};

template<typename Type>
struct has_body_element_method<Type, void_t<
    decltype(std::declval<const Type&>().body_element())
>> : std::true_type
{};


// has_title_method
//   trait: true if Type exposes `title()` const returning the
// document title as a string.
template<typename Type,
         typename = void>
struct has_title_method : std::false_type
{};

template<typename Type>
struct has_title_method<Type, void_t<
    decltype(std::declval<const Type&>().title())
>> : std::true_type
{};


// has_set_title_method
//   trait: true if Type exposes `set_title(const std::string&)`.
template<typename Type,
         typename = void>
struct has_set_title_method : std::false_type
{};

template<typename Type>
struct has_set_title_method<Type, void_t<
    decltype(std::declval<Type&>().set_title(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_html_dom_shortcuts
//   trait: true if Type exposes head_element + body_element.
template<typename Type>
struct has_html_dom_shortcuts
{
    D_STATIC_CONSTEXPR bool value =
        ( has_head_element_method<Type>::value &&
          has_body_element_method<Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                XIV.   HTML ELEMENT CLASSIFICATION                       ///
///////////////////////////////////////////////////////////////////////////////

// is_html_element
//   trait: true if Type satisfies the XML element protocol AND
// exposes an HTML kind discriminator. The minimum bar is the XML
// element protocol (the type behaves like a tag-attribute-text
// node); the HTML kind accessor is what marks the type as
// HTML-aware. Library backends without an `html_kind()` method
// can still be treated as HTML by wrapping them in `html_element`
// (which derives from xml_node and adds the accessor).
template<typename Type>
struct is_html_element
{
    D_STATIC_CONSTEXPR bool value =
        ( is_xml_element<Type>::value &&
          has_html_kind_access<Type>::value );
};


// is_html_element_loose
//   trait: looser detection -- any XML element is treated as a
// candidate HTML element. Used by adapter code that wants to wrap
// raw XML/DOM types into HTML facades regardless of provenance.
template<typename Type>
struct is_html_element_loose
{
    D_STATIC_CONSTEXPR bool value =
        is_xml_element<Type>::value;
};


// html_element_class
//   struct: comprehensive classification of an HTML-element-shaped
// type. Aggregates XML node classification with HTML extensions.
template<typename Type>
struct html_element_class
{
    // identity
    D_STATIC_CONSTEXPR bool is_html_elem      =
        is_html_element<Type>::value;
    D_STATIC_CONSTEXPR bool is_xml_node       =
        ::djinterp::is_xml_node<Type>::value;
    D_STATIC_CONSTEXPR bool is_xml_element    =
        ::djinterp::is_xml_element<Type>::value;

    // HTML semantic accessors
    D_STATIC_CONSTEXPR bool has_html_kind     =
        has_html_kind_access<Type>::value;

    // class list
    D_STATIC_CONSTEXPR bool has_class_list    =
        has_class_list_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_add_class     =
        has_add_class_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_remove_class  =
        has_remove_class_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_has_class     =
        has_has_class_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_full_classes  =
        has_class_support<Type>::value;

    // id
    D_STATIC_CONSTEXPR bool has_id            =
        has_id_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_set_id        =
        has_set_id_method<Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                XV.   HTML DOCUMENT CLASSIFICATION                       ///
///////////////////////////////////////////////////////////////////////////////

// is_html_document
//   trait: true if Type satisfies the XML document protocol AND
// exposes either a doctype accessor or HTML version accessor.
template<typename Type>
struct is_html_document
{
    D_STATIC_CONSTEXPR bool value =
        ( is_xml_document<Type>::value &&
          ( has_doctype_access<Type>::value      ||
            has_html_version_method<Type>::value ) );
};


// is_html_document_loose
//   trait: looser detection -- any XML document is treated as a
// candidate HTML document.
template<typename Type>
struct is_html_document_loose
{
    D_STATIC_CONSTEXPR bool value =
        is_xml_document<Type>::value;
};


// html_document_class
//   struct: comprehensive classification of an HTML-document-shaped
// type. Aggregates XML document classification with HTML extensions.
template<typename Type>
struct html_document_class
{
    // identity
    D_STATIC_CONSTEXPR bool is_html_doc       =
        is_html_document<Type>::value;
    D_STATIC_CONSTEXPR bool is_xml_doc        =
        ::djinterp::is_xml_document<Type>::value;

    // doctype
    D_STATIC_CONSTEXPR bool has_doctype       =
        has_doctype_access<Type>::value;
    D_STATIC_CONSTEXPR bool has_set_doctype   =
        has_set_doctype_method<Type>::value;

    // version
    D_STATIC_CONSTEXPR bool has_version       =
        has_html_version_method<Type>::value;

    // shortcuts
    D_STATIC_CONSTEXPR bool has_head          =
        has_head_element_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_body          =
        has_body_element_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_dom_shortcuts =
        has_html_dom_shortcuts<Type>::value;
    D_STATIC_CONSTEXPR bool has_title         =
        has_title_method<Type>::value;
    D_STATIC_CONSTEXPR bool has_set_title     =
        has_set_title_method<Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                XVI.   BACKEND COMPLETENESS DETECTION                    ///
///////////////////////////////////////////////////////////////////////////////

// has_html_element_type_alias
//   trait: true if Type exposes a nested `element_type` or
// `html_element_type` alias naming the concrete element class.
template<typename Type,
         typename = void>
struct has_html_element_type_alias : std::false_type
{};

template<typename Type>
struct has_html_element_type_alias<Type, void_t<
    typename clean_t<Type>::element_type
>> : std::true_type
{};


// has_html_document_type_alias
//   trait: true if Type exposes a nested `document_type` alias.
template<typename Type,
         typename = void>
struct has_html_document_type_alias : std::false_type
{};

template<typename Type>
struct has_html_document_type_alias<Type, void_t<
    typename clean_t<Type>::document_type
>> : std::true_type
{};


// has_make_html_document_method
//   trait: true if Type exposes a static factory
// `make_html_document()` returning a document_type instance.
template<typename Type,
         typename = void>
struct has_make_html_document_method : std::false_type
{};

template<typename Type>
struct has_make_html_document_method<Type, void_t<
    decltype(clean_t<Type>::make_html_document())
>> : std::true_type
{};


// is_html_backend_complete
//   trait: true if Type exposes the full HTML backend protocol
// (every nested type alias plus the make_html_document factory).
// Stronger than `is_html_backend` (which only checks for the
// backend tag); used by code that needs to confidently
// instantiate `html_element<Backend>` and
// `html_document<Backend>`.
template<typename Type>
struct is_html_backend_complete
{
    D_STATIC_CONSTEXPR bool value =
        ( has_html_element_type_alias<Type>::value  &&
          has_html_document_type_alias<Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                XVII.   VARIABLE TEMPLATES                               ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename Type>
    D_CONSTEXPR bool has_html_kind_access_v =
        has_html_kind_access<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool has_class_support_v =
        has_class_support<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool has_doctype_access_v =
        has_doctype_access<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool has_html_version_method_v =
        has_html_version_method<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool has_html_dom_shortcuts_v =
        has_html_dom_shortcuts<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool is_html_element_v =
        is_html_element<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool is_html_element_loose_v =
        is_html_element_loose<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool is_html_document_v =
        is_html_document<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool is_html_document_loose_v =
        is_html_document_loose<Type>::value;

    template<typename Type>
    D_CONSTEXPR bool is_html_backend_complete_v =
        is_html_backend_complete<Type>::value;

#endif  // variable templates


///////////////////////////////////////////////////////////////////////////////
///                XVIII.   CONCEPTS (C++20)                                ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ---------------------------------------------------------------------
//  html element concepts
// ---------------------------------------------------------------------

// html_element_type
//   concept: satisfied by any type that satisfies the XML element
// protocol AND exposes an html_kind discriminator (either
// `html_kind()` or `get_html_kind()`).
template<typename Type>
concept html_element_type =
       ( ::djinterp::xml_element_type<Type> )
    && ( has_html_kind_access<Type>::value );


// html_element_loose_type
//   concept: looser variant; any type satisfying the XML element
// protocol is treated as an HTML element candidate. Use when
// adapting third-party DOM types into the HTML facades.
template<typename Type>
concept html_element_loose_type =
       ( ::djinterp::xml_element_type<Type> );


// ---------------------------------------------------------------------
//  html document concepts
// ---------------------------------------------------------------------

// html_document_type
//   concept: satisfied by any type that satisfies the XML document
// protocol AND exposes either a doctype accessor or an HTML
// version accessor.
template<typename Type>
concept html_document_type =
       ( ::djinterp::xml_document_type<Type> )
    && (    ( has_doctype_access<Type>::value )
         || ( has_html_version_method<Type>::value ) );


// html_document_loose_type
//   concept: looser variant; any type satisfying the XML document
// protocol is treated as an HTML document candidate.
template<typename Type>
concept html_document_loose_type =
       ( ::djinterp::xml_document_type<Type> );


// ---------------------------------------------------------------------
//  capability concepts (class / id / doctype / dom shortcuts)
// ---------------------------------------------------------------------

// classed_html_element
//   concept: HTML element that exposes the full class-list
// interface (has_class + add_class + remove_class).
template<typename Type>
concept classed_html_element =
       ( html_element_type<Type> )
    && ( has_class_support<Type>::value );


// identifiable_html_element
//   concept: HTML element that exposes both id read and id write
// accessors.
template<typename Type>
concept identifiable_html_element =
       ( html_element_type<Type> )
    && ( has_id_method<Type>::value )
    && ( has_set_id_method<Type>::value );


// doctype_aware_document
//   concept: HTML document with a doctype accessor.
template<typename Type>
concept doctype_aware_document =
       ( html_document_type<Type> )
    && ( has_doctype_access<Type>::value );


// versioned_html_document
//   concept: HTML document with an html_version accessor.
template<typename Type>
concept versioned_html_document =
       ( html_document_type<Type> )
    && ( has_html_version_method<Type>::value );


// dom_shortcut_document
//   concept: HTML document with both head_element and body_element
// accessors.
template<typename Type>
concept dom_shortcut_document =
       ( html_document_type<Type> )
    && ( has_html_dom_shortcuts<Type>::value );


// titled_html_document
//   concept: HTML document with title read+write accessors.
template<typename Type>
concept titled_html_document =
       ( html_document_type<Type> )
    && ( has_title_method<Type>::value )
    && ( has_set_title_method<Type>::value );


// ---------------------------------------------------------------------
//  composite concepts
// ---------------------------------------------------------------------

// full_html_element
//   concept: HTML element exposing every common HTML-side
// accessor (kind + class-list + id read/write).
template<typename Type>
concept full_html_element =
       ( html_element_type<Type> )
    && ( classed_html_element<Type> )
    && ( identifiable_html_element<Type> );


// full_html_document
//   concept: HTML document exposing every common HTML-side
// accessor (doctype + version + head/body shortcuts + title).
template<typename Type>
concept full_html_document =
       ( html_document_type<Type> )
    && ( doctype_aware_document<Type> )
    && ( versioned_html_document<Type> )
    && ( dom_shortcut_document<Type> )
    && ( titled_html_document<Type> );


// ---------------------------------------------------------------------
//  html backend concepts
// ---------------------------------------------------------------------

// html_backend_type
//   concept: satisfied by any type tagged with `html_backend_tag`
// (i.e. any type for which `is_html_backend<T>::value` is true).
template<typename Type>
concept html_backend_type =
       ( is_html_backend<Type>::value );


// complete_html_backend
//   concept: an HTML backend that additionally exposes the full
// nested-type-alias protocol AND a make_html_document factory.
// Code that needs to instantiate `html_document<Backend>` from
// scratch should constrain on this concept.
template<typename Type>
concept complete_html_backend =
       ( html_backend_type<Type> )
    && ( is_html_backend_complete<Type>::value )
    && ( has_make_html_document_method<Type>::value );

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

}   // namespace html


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_TEXT_HTML_HTML_HPP
