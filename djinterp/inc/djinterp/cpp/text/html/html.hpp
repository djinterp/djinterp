/*******************************************************************************
* djinterp [cpp]                                                        html.hpp
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
*   - `DHtmlElement<Backend>` adds NO members beyond `DXmlNode`.
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
* path:      /inc/djinterp/cpp/text/html/html.hpp
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
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint8_t

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// std
#  include <concepts>
#endif
// djinterp
#include "../../djinterp.hpp"
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

// DHtmlVersion
//   enum: identifies the document's target HTML / XHTML version.
// Drives DOCTYPE emission and self-closing-tag formatting (`<br>`
// for HTML5 / HTML4 vs `<br/>` for XHTML).
enum class DHtmlVersion : re_std::uint8_t
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
D_CONSTEXPR_INLINE
const char* html_doctype_string(
    DHtmlVersion _v
)
{
    return (_v == DHtmlVersion::html5)
            ? "<!DOCTYPE html>"
         : (_v == DHtmlVersion::html4_strict)
            ? "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\""
              " \"http://www.w3.org/TR/html4/strict.dtd\">"
         : (_v == DHtmlVersion::html4_transitional)
            ? "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01"
              " Transitional//EN\""
              " \"http://www.w3.org/TR/html4/loose.dtd\">"
         : (_v == DHtmlVersion::html4_frameset)
            ? "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01"
              " Frameset//EN\""
              " \"http://www.w3.org/TR/html4/frameset.dtd\">"
         : (_v == DHtmlVersion::xhtml1_strict)
            ? "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0"
              " Strict//EN\""
              " \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
         : (_v == DHtmlVersion::xhtml1_transitional)
            ? "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0"
              " Transitional//EN\""
              " \"http://www.w3.org/TR/xhtml1/DTD/"
              "xhtml1-transitional.dtd\">"
         : (_v == DHtmlVersion::xhtml1_frameset)
            ? "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0"
              " Frameset//EN\""
              " \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd\">"
         : (_v == DHtmlVersion::xhtml5)
            ? "<!DOCTYPE html>"
         :    "";
}


// is_xhtml_version
//   function: true for any XHTML variant. XHTML demands stricter
// emission (lowercase tags, quoted attributes, self-closed voids).
D_CONSTEXPR_INLINE
bool is_xhtml_version(
    DHtmlVersion _v
)
{
    return ( (_v == DHtmlVersion::xhtml1_strict)       ||
             (_v == DHtmlVersion::xhtml1_transitional) ||
             (_v == DHtmlVersion::xhtml1_frameset)     ||
             (_v == DHtmlVersion::xhtml5) );
}


///////////////////////////////////////////////////////////////////////////////
///                III.   ELEMENT KIND ENUM                                 ///
///////////////////////////////////////////////////////////////////////////////

// DHtmlElementKind
//   enum: discriminator for every standard HTML5 element. Backed
// by `re_std::uint8_t` to keep storage minimal. The `unknown` value
// is used for elements not in this list (custom elements, web
// components, deprecated tags).
enum class DHtmlElementKind : re_std::uint8_t
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

// DHtmlAttributeKind
//   enum: discriminator for the most common HTML attributes.
// The list covers the global attributes plus the attributes most
// frequently inspected by templating and DOM-walking code.
enum class DHtmlAttributeKind : re_std::uint8_t
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

// DHtmlElementCategory
//   enum: bitmask describing the categorical properties of an
// element. An element may belong to several categories (e.g. `a`
// is both `flow` and `phrasing`). Backed by `unsigned`.
enum DHtmlElementCategory : unsigned
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
D_CONSTEXPR_INLINE
unsigned html_category_for_kind(
    DHtmlElementKind _k
)
{
    // void elements: area, base, br, col, embed, hr, img, input,
    // link, meta, param, source, track, wbr.
    return (_k == DHtmlElementKind::area)    ? (hec_void | hec_phrasing | hec_flow)
         : (_k == DHtmlElementKind::base)    ? (hec_void | hec_metadata)
         : (_k == DHtmlElementKind::br)      ? (hec_void | hec_phrasing | hec_flow)
         : (_k == DHtmlElementKind::col)     ? (hec_void | hec_table)
         : (_k == DHtmlElementKind::embed)   ? (hec_void | hec_phrasing | hec_flow
                                                | hec_embedded | hec_interactive)
         : (_k == DHtmlElementKind::hr)      ? (hec_void | hec_block | hec_flow)
         : (_k == DHtmlElementKind::img)     ? (hec_void | hec_phrasing | hec_flow
                                                | hec_embedded | hec_palpable)
         : (_k == DHtmlElementKind::input)  ? (hec_void | hec_phrasing | hec_flow
                                                | hec_form_assoc | hec_interactive
                                                | hec_palpable)
         : (_k == DHtmlElementKind::link_)   ? (hec_void | hec_metadata | hec_flow)
         : (_k == DHtmlElementKind::meta)    ? (hec_void | hec_metadata | hec_flow)
         : (_k == DHtmlElementKind::param)   ? (hec_void)
         : (_k == DHtmlElementKind::source)  ? (hec_void)
         : (_k == DHtmlElementKind::track)   ? (hec_void)
         : (_k == DHtmlElementKind::wbr)     ? (hec_void | hec_phrasing | hec_flow)

         // metadata
         : (_k == DHtmlElementKind::title)     ? (hec_metadata | hec_escapable_raw)
         : (_k == DHtmlElementKind::style)     ? (hec_metadata | hec_raw_text)
         : (_k == DHtmlElementKind::script)    ? (hec_metadata | hec_raw_text
                                                  | hec_phrasing | hec_flow)
         : (_k == DHtmlElementKind::noscript) ? (hec_metadata | hec_phrasing
                                                  | hec_flow)
         : (_k == DHtmlElementKind::head)     ? (hec_metadata)

         // sectioning
         : (_k == DHtmlElementKind::html_root)  ? (hec_block)
         : (_k == DHtmlElementKind::body)       ? (hec_block | hec_sectioning
                                                   | hec_flow)
         : (_k == DHtmlElementKind::header)    ? (hec_block | hec_flow
                                                   | hec_palpable)
         : (_k == DHtmlElementKind::nav)       ? (hec_block | hec_sectioning
                                                   | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::main_)     ? (hec_block | hec_flow
                                                   | hec_palpable)
         : (_k == DHtmlElementKind::section)   ? (hec_block | hec_sectioning
                                                   | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::article)   ? (hec_block | hec_sectioning
                                                   | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::aside)     ? (hec_block | hec_sectioning
                                                   | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::footer)    ? (hec_block | hec_flow
                                                   | hec_palpable)
         : (_k == DHtmlElementKind::address)   ? (hec_block | hec_flow
                                                   | hec_palpable)

         // headings (uniform)
         : ( (_k == DHtmlElementKind::h1) ||
             (_k == DHtmlElementKind::h2) ||
             (_k == DHtmlElementKind::h3) ||
             (_k == DHtmlElementKind::h4) ||
             (_k == DHtmlElementKind::h5) ||
             (_k == DHtmlElementKind::h6) ||
             (_k == DHtmlElementKind::hgroup) )
                                              ? (hec_block | hec_flow | hec_heading
                                                 | hec_palpable)

         // paragraph / list block
         : (_k == DHtmlElementKind::p)          ? (hec_block | hec_flow
                                                    | hec_palpable)
         : (_k == DHtmlElementKind::pre)        ? (hec_block | hec_flow
                                                    | hec_palpable)
         : (_k == DHtmlElementKind::blockquote) ? (hec_block | hec_sectioning
                                                    | hec_flow | hec_palpable)
         : ( (_k == DHtmlElementKind::ol) ||
             (_k == DHtmlElementKind::ul) ||
             (_k == DHtmlElementKind::dl) )
                                                 ? (hec_block | hec_flow
                                                    | hec_palpable)
         : ( (_k == DHtmlElementKind::li) ||
             (_k == DHtmlElementKind::dt) ||
             (_k == DHtmlElementKind::dd) )
                                                 ? (hec_block)
         : (_k == DHtmlElementKind::figure)      ? (hec_block | hec_sectioning
                                                    | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::figcaption)  ? (hec_block)
         : (_k == DHtmlElementKind::div_)        ? (hec_block | hec_flow
                                                    | hec_palpable)

         // forms
         : (_k == DHtmlElementKind::form)     ? (hec_block | hec_form
                                                  | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::fieldset) ? (hec_block | hec_form_assoc
                                                  | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::legend)    ? (hec_block)
         : (_k == DHtmlElementKind::label)     ? (hec_inline_ | hec_phrasing
                                                  | hec_form_assoc | hec_flow
                                                  | hec_interactive
                                                  | hec_palpable)
         : (_k == DHtmlElementKind::button)   ? (hec_inline_ | hec_phrasing
                                                  | hec_form_assoc | hec_flow
                                                  | hec_interactive
                                                  | hec_palpable)
         : (_k == DHtmlElementKind::select_)  ? (hec_inline_ | hec_phrasing
                                                  | hec_form_assoc | hec_flow
                                                  | hec_interactive
                                                  | hec_palpable)
         : (_k == DHtmlElementKind::textarea) ? (hec_inline_ | hec_escapable_raw
                                                  | hec_form_assoc | hec_phrasing
                                                  | hec_flow | hec_interactive
                                                  | hec_palpable)
         : ( (_k == DHtmlElementKind::output)    ||
             (_k == DHtmlElementKind::progress)  ||
             (_k == DHtmlElementKind::meter) )
                                                ? (hec_inline_ | hec_phrasing
                                                   | hec_form_assoc | hec_flow
                                                   | hec_palpable)
         : ( (_k == DHtmlElementKind::option) ||
             (_k == DHtmlElementKind::optgroup) )
                                                ? (hec_inline_)
         : (_k == DHtmlElementKind::datalist)   ? (hec_inline_ | hec_phrasing
                                                   | hec_flow)

         // tables
         : (_k == DHtmlElementKind::table)    ? (hec_block | hec_table | hec_flow
                                                  | hec_palpable)
         : (_k == DHtmlElementKind::caption)   ? (hec_block | hec_table)
         : (_k == DHtmlElementKind::colgroup)  ? (hec_table)
         : ( (_k == DHtmlElementKind::tbody) ||
             (_k == DHtmlElementKind::thead) ||
             (_k == DHtmlElementKind::tfoot) ||
             (_k == DHtmlElementKind::tr) )
                                              ? (hec_block | hec_table)
         : ( (_k == DHtmlElementKind::td) ||
             (_k == DHtmlElementKind::th) )
                                              ? (hec_block | hec_table
                                                 | hec_sectioning)

         // interactive
         : (_k == DHtmlElementKind::details) ? (hec_block | hec_flow
                                                 | hec_interactive
                                                 | hec_palpable)
         : (_k == DHtmlElementKind::summary)  ? (hec_block)
         : (_k == DHtmlElementKind::dialog)   ? (hec_block | hec_flow
                                                 | hec_sectioning)

         // embedded
         : (_k == DHtmlElementKind::iframe)  ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded
                                                 | hec_interactive
                                                 | hec_palpable)
         : (_k == DHtmlElementKind::object_) ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded
                                                 | hec_form_assoc
                                                 | hec_palpable)
         : ( (_k == DHtmlElementKind::video) ||
             (_k == DHtmlElementKind::audio)  )
                                              ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded
                                                 | hec_interactive
                                                 | hec_palpable)
         : (_k == DHtmlElementKind::canvas)  ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded
                                                 | hec_palpable)
         : (_k == DHtmlElementKind::picture) ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded)
         : (_k == DHtmlElementKind::map_)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : ( (_k == DHtmlElementKind::math) ||
             (_k == DHtmlElementKind::svg) )
                                              ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_embedded
                                                 | hec_palpable)

         // scripting / template
         : (_k == DHtmlElementKind::template_) ? (hec_metadata | hec_phrasing
                                                   | hec_flow)
         : (_k == DHtmlElementKind::slot)      ? (hec_inline_ | hec_phrasing
                                                   | hec_flow)

         // edits
         : ( (_k == DHtmlElementKind::ins) ||
             (_k == DHtmlElementKind::del_) )
                                              ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)

         // anchor
         : (_k == DHtmlElementKind::a)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_interactive
                                                 | hec_palpable)

         // remaining inline phrasing elements (em, strong, small, s,
         // cite, q, dfn, abbr, ruby, rt, rp, data, time, code, var,
         // samp, kbd, sub, sup, i, b, u, mark, bdi, bdo, span)
         : (_k == DHtmlElementKind::em)      ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::strong)  ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::small)   ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::s)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::cite)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::q)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::dfn)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::abbr)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::ruby)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : ( (_k == DHtmlElementKind::rt) ||
             (_k == DHtmlElementKind::rp) )
                                              ? (hec_inline_)
         : (_k == DHtmlElementKind::data_)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::time_)   ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::code)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::var_)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::samp)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::kbd)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::sub)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::sup)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::i)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::b)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::u)       ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::mark)    ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::bdi)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::bdo)     ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)
         : (_k == DHtmlElementKind::span_)   ? (hec_inline_ | hec_phrasing
                                                 | hec_flow | hec_palpable)

         :    hec_none;
}


// is_void_element_kind
//   function: true if `_k` is a void element (no end tag).
D_CONSTEXPR_INLINE
bool is_void_element_kind(
    DHtmlElementKind _k
)
{
    return ((html_category_for_kind(_k) & hec_void) != 0u);
}


// is_block_element_kind
//   function: true if `_k` is a block-level element.
D_CONSTEXPR_INLINE
bool is_block_element_kind(
    DHtmlElementKind _k
)
{
    return ((html_category_for_kind(_k) & hec_block) != 0u);
}


// is_inline_element_kind
//   function: true if `_k` is an inline-level element.
D_CONSTEXPR_INLINE
bool is_inline_element_kind(
    DHtmlElementKind _k
)
{
    return ((html_category_for_kind(_k) & hec_inline_) != 0u);
}


// is_raw_text_element_kind
//   function: true if `_k` has raw-text content (script, style).
// Such elements must NOT have their text escaped on emission.
D_CONSTEXPR_INLINE
bool is_raw_text_element_kind(
    DHtmlElementKind _k
)
{
    return ((html_category_for_kind(_k) & hec_raw_text) != 0u);
}


// is_form_element_kind
//   function: true if `_k` is `<form>` or a form-associated control.
D_CONSTEXPR_INLINE
bool is_form_element_kind(
    DHtmlElementKind _k
)
{
    return ( (html_category_for_kind(_k) &
              (hec_form | hec_form_assoc)) != 0u );
}


// is_metadata_element_kind
//   function: true if `_k` belongs in `<head>`.
D_CONSTEXPR_INLINE
bool is_metadata_element_kind(
    DHtmlElementKind _k
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
D_CONSTEXPR_INLINE
bool internal_streq(
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
// lowercase) to its `DHtmlElementKind`. Returns
// `DHtmlElementKind::unknown` for unrecognised tags.
D_CONSTEXPR_INLINE
DHtmlElementKind html_kind_from_name(
    const char* _name
)
{
    return (!_name)                                  ? DHtmlElementKind::unknown
         : (internal_streq(_name, tags::html_root))  ? DHtmlElementKind::html_root
         : (internal_streq(_name, tags::head))       ? DHtmlElementKind::head
         : (internal_streq(_name, tags::title))      ? DHtmlElementKind::title
         : (internal_streq(_name, tags::base))       ? DHtmlElementKind::base
         : (internal_streq(_name, tags::link_))      ? DHtmlElementKind::link_
         : (internal_streq(_name, tags::meta))       ? DHtmlElementKind::meta
         : (internal_streq(_name, tags::style))      ? DHtmlElementKind::style
         : (internal_streq(_name, tags::script))     ? DHtmlElementKind::script
         : (internal_streq(_name, tags::noscript))   ? DHtmlElementKind::noscript
         : (internal_streq(_name, tags::body))       ? DHtmlElementKind::body
         : (internal_streq(_name, tags::header))     ? DHtmlElementKind::header
         : (internal_streq(_name, tags::nav))        ? DHtmlElementKind::nav
         : (internal_streq(_name, tags::main_))      ? DHtmlElementKind::main_
         : (internal_streq(_name, tags::section))    ? DHtmlElementKind::section
         : (internal_streq(_name, tags::article))    ? DHtmlElementKind::article
         : (internal_streq(_name, tags::aside))      ? DHtmlElementKind::aside
         : (internal_streq(_name, tags::footer))     ? DHtmlElementKind::footer
         : (internal_streq(_name, tags::address))    ? DHtmlElementKind::address
         : (internal_streq(_name, tags::h1))         ? DHtmlElementKind::h1
         : (internal_streq(_name, tags::h2))         ? DHtmlElementKind::h2
         : (internal_streq(_name, tags::h3))         ? DHtmlElementKind::h3
         : (internal_streq(_name, tags::h4))         ? DHtmlElementKind::h4
         : (internal_streq(_name, tags::h5))         ? DHtmlElementKind::h5
         : (internal_streq(_name, tags::h6))         ? DHtmlElementKind::h6
         : (internal_streq(_name, tags::hgroup))     ? DHtmlElementKind::hgroup
         : (internal_streq(_name, tags::p))          ? DHtmlElementKind::p
         : (internal_streq(_name, tags::hr))         ? DHtmlElementKind::hr
         : (internal_streq(_name, tags::pre))        ? DHtmlElementKind::pre
         : (internal_streq(_name, tags::blockquote)) ? DHtmlElementKind::blockquote
         : (internal_streq(_name, tags::ol))         ? DHtmlElementKind::ol
         : (internal_streq(_name, tags::ul))         ? DHtmlElementKind::ul
         : (internal_streq(_name, tags::li))         ? DHtmlElementKind::li
         : (internal_streq(_name, tags::dl))         ? DHtmlElementKind::dl
         : (internal_streq(_name, tags::dt))         ? DHtmlElementKind::dt
         : (internal_streq(_name, tags::dd))         ? DHtmlElementKind::dd
         : (internal_streq(_name, tags::figure))     ? DHtmlElementKind::figure
         : (internal_streq(_name, tags::figcaption)) ? DHtmlElementKind::figcaption
         : (internal_streq(_name, tags::div_))       ? DHtmlElementKind::div_
         : (internal_streq(_name, tags::a))          ? DHtmlElementKind::a
         : (internal_streq(_name, tags::em))         ? DHtmlElementKind::em
         : (internal_streq(_name, tags::strong))     ? DHtmlElementKind::strong
         : (internal_streq(_name, tags::small))      ? DHtmlElementKind::small
         : (internal_streq(_name, tags::s))          ? DHtmlElementKind::s
         : (internal_streq(_name, tags::cite))       ? DHtmlElementKind::cite
         : (internal_streq(_name, tags::q))          ? DHtmlElementKind::q
         : (internal_streq(_name, tags::dfn))        ? DHtmlElementKind::dfn
         : (internal_streq(_name, tags::abbr))       ? DHtmlElementKind::abbr
         : (internal_streq(_name, tags::code))       ? DHtmlElementKind::code
         : (internal_streq(_name, tags::var_))       ? DHtmlElementKind::var_
         : (internal_streq(_name, tags::samp))       ? DHtmlElementKind::samp
         : (internal_streq(_name, tags::kbd))        ? DHtmlElementKind::kbd
         : (internal_streq(_name, tags::sub))        ? DHtmlElementKind::sub
         : (internal_streq(_name, tags::sup))        ? DHtmlElementKind::sup
         : (internal_streq(_name, tags::i))          ? DHtmlElementKind::i
         : (internal_streq(_name, tags::b))          ? DHtmlElementKind::b
         : (internal_streq(_name, tags::u))          ? DHtmlElementKind::u
         : (internal_streq(_name, tags::mark))       ? DHtmlElementKind::mark
         : (internal_streq(_name, tags::span_))      ? DHtmlElementKind::span_
         : (internal_streq(_name, tags::br))         ? DHtmlElementKind::br
         : (internal_streq(_name, tags::wbr))        ? DHtmlElementKind::wbr
         : (internal_streq(_name, tags::img))        ? DHtmlElementKind::img
         : (internal_streq(_name, tags::iframe))     ? DHtmlElementKind::iframe
         : (internal_streq(_name, tags::embed))      ? DHtmlElementKind::embed
         : (internal_streq(_name, tags::object_))    ? DHtmlElementKind::object_
         : (internal_streq(_name, tags::video))      ? DHtmlElementKind::video
         : (internal_streq(_name, tags::audio))      ? DHtmlElementKind::audio
         : (internal_streq(_name, tags::source))     ? DHtmlElementKind::source
         : (internal_streq(_name, tags::track))      ? DHtmlElementKind::track
         : (internal_streq(_name, tags::canvas))     ? DHtmlElementKind::canvas
         : (internal_streq(_name, tags::picture))    ? DHtmlElementKind::picture
         : (internal_streq(_name, tags::map_))       ? DHtmlElementKind::map_
         : (internal_streq(_name, tags::area))       ? DHtmlElementKind::area
         : (internal_streq(_name, tags::svg))        ? DHtmlElementKind::svg
         : (internal_streq(_name, tags::table))      ? DHtmlElementKind::table
         : (internal_streq(_name, tags::caption))    ? DHtmlElementKind::caption
         : (internal_streq(_name, tags::colgroup))   ? DHtmlElementKind::colgroup
         : (internal_streq(_name, tags::col))        ? DHtmlElementKind::col
         : (internal_streq(_name, tags::tbody))      ? DHtmlElementKind::tbody
         : (internal_streq(_name, tags::thead))      ? DHtmlElementKind::thead
         : (internal_streq(_name, tags::tfoot))      ? DHtmlElementKind::tfoot
         : (internal_streq(_name, tags::tr))         ? DHtmlElementKind::tr
         : (internal_streq(_name, tags::td))         ? DHtmlElementKind::td
         : (internal_streq(_name, tags::th))         ? DHtmlElementKind::th
         : (internal_streq(_name, tags::form))       ? DHtmlElementKind::form
         : (internal_streq(_name, tags::label))      ? DHtmlElementKind::label
         : (internal_streq(_name, tags::input))      ? DHtmlElementKind::input
         : (internal_streq(_name, tags::button))     ? DHtmlElementKind::button
         : (internal_streq(_name, tags::select_))    ? DHtmlElementKind::select_
         : (internal_streq(_name, tags::option))     ? DHtmlElementKind::option
         : (internal_streq(_name, tags::textarea))   ? DHtmlElementKind::textarea
         : (internal_streq(_name, tags::fieldset))   ? DHtmlElementKind::fieldset
         : (internal_streq(_name, tags::legend))     ? DHtmlElementKind::legend
         : (internal_streq(_name, tags::details))    ? DHtmlElementKind::details
         : (internal_streq(_name, tags::summary))    ? DHtmlElementKind::summary
         : (internal_streq(_name, tags::dialog))     ? DHtmlElementKind::dialog
         : (internal_streq(_name, tags::template_))  ? DHtmlElementKind::template_
         :    DHtmlElementKind::unknown;
}


// name_from_html_kind
//   function: maps an `DHtmlElementKind` to its tag-name string.
// Returns the empty string for `unknown`.
D_CONSTEXPR_INLINE
const char* name_from_html_kind(
    DHtmlElementKind _k
)
{
    return (_k == DHtmlElementKind::html_root)   ? tags::html_root
         : (_k == DHtmlElementKind::head)        ? tags::head
         : (_k == DHtmlElementKind::title)       ? tags::title
         : (_k == DHtmlElementKind::base)        ? tags::base
         : (_k == DHtmlElementKind::link_)       ? tags::link_
         : (_k == DHtmlElementKind::meta)        ? tags::meta
         : (_k == DHtmlElementKind::style)       ? tags::style
         : (_k == DHtmlElementKind::script)      ? tags::script
         : (_k == DHtmlElementKind::noscript)    ? tags::noscript
         : (_k == DHtmlElementKind::body)        ? tags::body
         : (_k == DHtmlElementKind::header)      ? tags::header
         : (_k == DHtmlElementKind::nav)         ? tags::nav
         : (_k == DHtmlElementKind::main_)       ? tags::main_
         : (_k == DHtmlElementKind::section)     ? tags::section
         : (_k == DHtmlElementKind::article)     ? tags::article
         : (_k == DHtmlElementKind::aside)       ? tags::aside
         : (_k == DHtmlElementKind::footer)      ? tags::footer
         : (_k == DHtmlElementKind::address)     ? tags::address
         : (_k == DHtmlElementKind::h1)          ? tags::h1
         : (_k == DHtmlElementKind::h2)          ? tags::h2
         : (_k == DHtmlElementKind::h3)          ? tags::h3
         : (_k == DHtmlElementKind::h4)          ? tags::h4
         : (_k == DHtmlElementKind::h5)          ? tags::h5
         : (_k == DHtmlElementKind::h6)          ? tags::h6
         : (_k == DHtmlElementKind::hgroup)      ? tags::hgroup
         : (_k == DHtmlElementKind::p)           ? tags::p
         : (_k == DHtmlElementKind::hr)          ? tags::hr
         : (_k == DHtmlElementKind::pre)         ? tags::pre
         : (_k == DHtmlElementKind::blockquote)  ? tags::blockquote
         : (_k == DHtmlElementKind::ol)          ? tags::ol
         : (_k == DHtmlElementKind::ul)          ? tags::ul
         : (_k == DHtmlElementKind::li)          ? tags::li
         : (_k == DHtmlElementKind::dl)          ? tags::dl
         : (_k == DHtmlElementKind::dt)          ? tags::dt
         : (_k == DHtmlElementKind::dd)          ? tags::dd
         : (_k == DHtmlElementKind::figure)      ? tags::figure
         : (_k == DHtmlElementKind::figcaption)  ? tags::figcaption
         : (_k == DHtmlElementKind::div_)        ? tags::div_
         : (_k == DHtmlElementKind::a)           ? tags::a
         : (_k == DHtmlElementKind::em)          ? tags::em
         : (_k == DHtmlElementKind::strong)      ? tags::strong
         : (_k == DHtmlElementKind::small)       ? tags::small
         : (_k == DHtmlElementKind::s)           ? tags::s
         : (_k == DHtmlElementKind::cite)        ? tags::cite
         : (_k == DHtmlElementKind::q)           ? tags::q
         : (_k == DHtmlElementKind::dfn)         ? tags::dfn
         : (_k == DHtmlElementKind::abbr)        ? tags::abbr
         : (_k == DHtmlElementKind::code)        ? tags::code
         : (_k == DHtmlElementKind::var_)        ? tags::var_
         : (_k == DHtmlElementKind::samp)        ? tags::samp
         : (_k == DHtmlElementKind::kbd)         ? tags::kbd
         : (_k == DHtmlElementKind::sub)         ? tags::sub
         : (_k == DHtmlElementKind::sup)         ? tags::sup
         : (_k == DHtmlElementKind::i)           ? tags::i
         : (_k == DHtmlElementKind::b)           ? tags::b
         : (_k == DHtmlElementKind::u)           ? tags::u
         : (_k == DHtmlElementKind::mark)        ? tags::mark
         : (_k == DHtmlElementKind::span_)       ? tags::span_
         : (_k == DHtmlElementKind::br)          ? tags::br
         : (_k == DHtmlElementKind::wbr)         ? tags::wbr
         : (_k == DHtmlElementKind::img)         ? tags::img
         : (_k == DHtmlElementKind::iframe)      ? tags::iframe
         : (_k == DHtmlElementKind::embed)       ? tags::embed
         : (_k == DHtmlElementKind::object_)     ? tags::object_
         : (_k == DHtmlElementKind::video)       ? tags::video
         : (_k == DHtmlElementKind::audio)       ? tags::audio
         : (_k == DHtmlElementKind::source)      ? tags::source
         : (_k == DHtmlElementKind::track)       ? tags::track
         : (_k == DHtmlElementKind::canvas)      ? tags::canvas
         : (_k == DHtmlElementKind::picture)     ? tags::picture
         : (_k == DHtmlElementKind::map_)        ? tags::map_
         : (_k == DHtmlElementKind::area)        ? tags::area
         : (_k == DHtmlElementKind::svg)         ? tags::svg
         : (_k == DHtmlElementKind::table)       ? tags::table
         : (_k == DHtmlElementKind::caption)     ? tags::caption
         : (_k == DHtmlElementKind::colgroup)    ? tags::colgroup
         : (_k == DHtmlElementKind::col)         ? tags::col
         : (_k == DHtmlElementKind::tbody)       ? tags::tbody
         : (_k == DHtmlElementKind::thead)       ? tags::thead
         : (_k == DHtmlElementKind::tfoot)       ? tags::tfoot
         : (_k == DHtmlElementKind::tr)          ? tags::tr
         : (_k == DHtmlElementKind::td)          ? tags::td
         : (_k == DHtmlElementKind::th)          ? tags::th
         : (_k == DHtmlElementKind::form)        ? tags::form
         : (_k == DHtmlElementKind::label)       ? tags::label
         : (_k == DHtmlElementKind::input)       ? tags::input
         : (_k == DHtmlElementKind::button)      ? tags::button
         : (_k == DHtmlElementKind::select_)     ? tags::select_
         : (_k == DHtmlElementKind::option)      ? tags::option
         : (_k == DHtmlElementKind::textarea)    ? tags::textarea
         : (_k == DHtmlElementKind::fieldset)    ? tags::fieldset
         : (_k == DHtmlElementKind::legend)      ? tags::legend
         : (_k == DHtmlElementKind::details)     ? tags::details
         : (_k == DHtmlElementKind::summary)     ? tags::summary
         : (_k == DHtmlElementKind::dialog)      ? tags::dialog
         : (_k == DHtmlElementKind::template_)   ? tags::template_
         :    "";
}


// is_void_element_name
//   function: classifies an element by tag name.
D_CONSTEXPR_INLINE
bool is_void_element_name(
    const char* _name
)
{
    return is_void_element_kind(html_kind_from_name(_name));
}


// is_block_element_name
D_CONSTEXPR_INLINE
bool is_block_element_name(
    const char* _name
)
{
    return is_block_element_kind(html_kind_from_name(_name));
}


// is_inline_element_name
D_CONSTEXPR_INLINE
bool is_inline_element_name(
    const char* _name
)
{
    return is_inline_element_kind(html_kind_from_name(_name));
}


// is_raw_text_element_name
D_CONSTEXPR_INLINE
bool is_raw_text_element_name(
    const char* _name
)
{
    return is_raw_text_element_kind(html_kind_from_name(_name));
}


///////////////////////////////////////////////////////////////////////////////
///                VIII.   BACKEND TAG DISPATCH                             ///
///////////////////////////////////////////////////////////////////////////////

// DHtmlBackendTag
//   struct: empty base tag for all HTML backend tag types.
struct DHtmlBackendTag
{};

// DHtmlDefaultBackendTag
//   struct: tag identifying the bundled in-memory HTML backend.
struct DHtmlDefaultBackendTag : DHtmlBackendTag
{};

// DHtmlLibxmlppBackendTag
//   struct: tag identifying a libxml++ HTML-mode adapter backend.
struct DHtmlLibxmlppBackendTag : DHtmlBackendTag
{};

// DHtmlGumboBackendTag
//   struct: tag identifying a gumbo-parser-based backend.
struct DHtmlGumboBackendTag : DHtmlBackendTag
{};

// DHtmlLexborBackendTag
//   struct: tag identifying a Lexbor-based backend.
struct DHtmlLexborBackendTag : DHtmlBackendTag
{};

// DHtmlHtmlcxxBackendTag
//   struct: tag identifying an htmlcxx-based backend.
struct DHtmlHtmlcxxBackendTag : DHtmlBackendTag
{};


///////////////////////////////////////////////////////////////////////////////
///                IX.   BACKEND DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

// is_html_backend
//   trait: detects whether Type is an HTML backend type by
// checking for a nested `DHtmlBackendTag` alias.

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
//   trait: true if Type has a nested DHtmlBackendTag type.
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
// returning an `DHtmlElementKind` discriminator. This is the
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
// an `DHtmlVersion` enum value.
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
// can still be treated as HTML by wrapping them in `DHtmlElement`
// (which derives from DXmlNode and adds the accessor).
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


// DHtmlElementClass
//   struct: comprehensive classification of an HTML-element-shaped
// type. Aggregates XML node classification with HTML extensions.
template<typename Type>
struct DHtmlElementClass
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


// DHtmlDocumentClass
//   struct: comprehensive classification of an HTML-document-shaped
// type. Aggregates XML document classification with HTML extensions.
template<typename Type>
struct DHtmlDocumentClass
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
// instantiate `DHtmlElement<Backend>` and
// `DHtmlDocument<Backend>`.
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
//   concept: HTML document with an DHtmlVersion accessor.
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
//   concept: satisfied by any type tagged with `DHtmlBackendTag`
// (i.e. any type for which `is_html_backend<T>::value` is true).
template<typename Type>
concept html_backend_type =
       ( is_html_backend<Type>::value );


// complete_html_backend
//   concept: an HTML backend that additionally exposes the full
// nested-type-alias protocol AND a make_html_document factory.
// Code that needs to instantiate `DHtmlDocument<Backend>` from
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
