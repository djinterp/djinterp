/******************************************************************************
* djinterp [utility]                                        document_attributes.hpp
*
*   The hint bag every document-template node carries.  A document template
* (a table, a title page, a table of contents) is DIALECT-AGNOSTIC: it names
* semantics and presentation HINTS, and a per-dialect document_renderer decides
* how (or whether) to realise each hint.  This header supplies the hint carrier
* and the small shared vocabulary the templates and the renderers agree on.
*
*   THE CARRIER IS METADATA, NOT A NEW TYPE.  A node's hints are exactly a
* COLLECTION OF KEY-VALUE PAIRS, so `doc_attributes` is nothing more than a
* container_metadata<std::string, std::string> -- the framework's open kv store,
* reused wholesale (set / find / contains / iterate).  String values keep the
* bag portable across every output dialect: a colour is "#2E7D32", a width is
* "12", a flag is "true".  A renderer reads the keys it understands and ignores
* the rest, so a `.txt` back end never consults `font` or `color` while an HTML
* or PDF back end does -- the "ignored when not applicable" rule the templates
* rest on.
*
*   WHY STRING-KEYED RUNTIME METADATA (NOT option_set):
*   option_set<> is a COMPILE-TIME, NTTP-keyed policy bag -- the right tool to
* configure a renderer once, at construction.  A document's per-node hints, by
* contrast, vary at RUN TIME and per node, so the runtime kv store is the fit
* here.  The two compose: a renderer may take an option_set for its fixed policy
* and read doc_attributes for per-node hints.
*
*   THE VOCABULARY (Section II) is a documented, OPEN set of key names.  It is a
* convention, not a closed enum: a dialect-specific hint a renderer invents
* (say "latex_env") is just another key the others ignore.  `align` is given a
* first-class accessor because it maps onto the framework's text_alignment; the
* rest are read as strings (or as flags) through the generic accessors.
*
*   PORTABILITY:
*   C++11 baseline (container_metadata is C++11); the `_v`-style companions and
* the concept parallels live with the renderer, not here.
*
*
* path:      /inc/djinterp/core/util/document/templates/document_attributes.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    doc_attributes                 (the hint bag: a container_metadata alias)
II.   Standard hint keys             (the shared key vocabulary)
III.  Alignment interchange          (text_alignment <-> string)
IV.   Accessors                      (attr_has / attr_or / attr_flag / attr_align)
*/

#ifndef DJINTERP_UTIL_DOCUMENT_ATTRIBUTES_
#define DJINTERP_UTIL_DOCUMENT_ATTRIBUTES_ 1

// std
#include <cstddef>
#include <string>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"                       // NS_*, gates
#include "../../../text/text_align.hpp"                // text_alignment
#include "../../../container/container_metadata.hpp"   // container_metadata


NS_DJINTERP


// ===========================================================================
// I.   doc_attributes
// ===========================================================================

// doc_attributes
//   type: a document node's presentation-hint bag -- an open string -> string
// key-value store (the framework's container_metadata).  A template writes
// hints into it; a renderer reads the keys it understands and ignores the rest.
using doc_attributes = container_metadata<std::string, std::string>;


// ===========================================================================
// II.  Standard hint keys
// ===========================================================================
//   The shared key names templates set and renderers read.  A convention, not
// a closed set: any renderer may honour additional keys, and any renderer may
// ignore any of these.  Exposed as symbols so both sides avoid stringly typos.

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

    // doc_attr_style
    //   key: the name of a registered style the renderer resolves (e.g.
    // "title", "heading", "status.pass").  The renderer owns the registry;
    // a dialect with no styling ignores it.
    inline constexpr const char* doc_attr_style      = "style";

    // doc_attr_align
    //   key: horizontal alignment token (see align_to_string): "left",
    // "center", "right", "justify".
    inline constexpr const char* doc_attr_align      = "align";

    // doc_attr_font
    //   key: a font family or face name (e.g. "Helvetica-Bold", "mono").
    inline constexpr const char* doc_attr_font       = "font";

    // doc_attr_size
    //   key: a font size in points, as a decimal token (e.g. "14").
    inline constexpr const char* doc_attr_size       = "size";

    // doc_attr_bold / doc_attr_italic
    //   key: boolean face flags (see attr_flag): "true" / "false".
    inline constexpr const char* doc_attr_bold       = "bold";
    inline constexpr const char* doc_attr_italic     = "italic";

    // doc_attr_color / doc_attr_background
    //   key: a foreground / background colour as "#RRGGBB" or a name.
    inline constexpr const char* doc_attr_color      = "color";
    inline constexpr const char* doc_attr_background = "background";

    // doc_attr_width
    //   key: a preferred extent -- character cells for a text dialect, points
    // for PDF, a CSS length for HTML -- interpreted by the renderer.
    inline constexpr const char* doc_attr_width      = "width";

    // doc_attr_wrap
    //   key: whether over-long content may wrap ("true") or is clipped
    // ("false"); a renderer with no wrap notion ignores it.
    inline constexpr const char* doc_attr_wrap       = "wrap";

    // doc_attr_indent
    //   key: a leading indent in cells / levels, as a decimal token.
    inline constexpr const char* doc_attr_indent     = "indent";

    // doc_attr_locator
    //   key: a cross-reference target -- a page number, an anchor, a URL --
    // carried by an entry (used by table_of_contents leaders).
    inline constexpr const char* doc_attr_locator    = "locator";

    // doc_attr_colspan / doc_attr_rowspan
    //   key: the number of columns / rows a table cell spans, as a decimal
    // token.  These are how a table model's MERGE COVER reaches a renderer: a
    // layout cell covering a region emits once, at its anchor, carrying its
    // extent here.  A dialect that can express merges (HTML colspan= /
    // rowspan=) honours them; one that cannot ignores them, and the producer
    // pads the covered positions instead.
    inline constexpr const char* doc_attr_colspan    = "colspan";
    inline constexpr const char* doc_attr_rowspan    = "rowspan";

#else  // pre-C++17: internal-linkage constants (used by value, never by address)

    static constexpr const char* doc_attr_style      = "style";
    static constexpr const char* doc_attr_align      = "align";
    static constexpr const char* doc_attr_font       = "font";
    static constexpr const char* doc_attr_size       = "size";
    static constexpr const char* doc_attr_bold       = "bold";
    static constexpr const char* doc_attr_italic     = "italic";
    static constexpr const char* doc_attr_color      = "color";
    static constexpr const char* doc_attr_background = "background";
    static constexpr const char* doc_attr_width      = "width";
    static constexpr const char* doc_attr_wrap       = "wrap";
    static constexpr const char* doc_attr_indent     = "indent";
    static constexpr const char* doc_attr_locator    = "locator";
    static constexpr const char* doc_attr_colspan    = "colspan";
    static constexpr const char* doc_attr_rowspan    = "rowspan";

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// ===========================================================================
// III. Alignment interchange
// ===========================================================================

// align_to_string
//   function: the canonical token for an alignment, for writing into the
// `align` hint.  The tokens round-trip through align_from_string.
D_NODISCARD inline const char*
align_to_string(
    text_alignment _align
) D_NOEXCEPT
{
    switch (_align)
    {
        case text_alignment::left:    { return "left";    }
        case text_alignment::center:  { return "center";  }
        case text_alignment::right:   { return "right";   }
        case text_alignment::justify: { return "justify"; }
    }

    return "left";
}

// align_from_string
//   function: the alignment named by a token, or _fallback when the token is
// empty or unrecognised.  Only the first character is significant (l/c/r/j),
// so "centre" and "center" both read as center.
D_NODISCARD inline text_alignment
align_from_string(
    const std::string& _token,
    text_alignment     _fallback = text_alignment::left
)
{
    // an empty token carries no opinion
    if (_token.empty())
    {
        return _fallback;
    }

    switch (_token[0])
    {
        case 'l': case 'L': { return text_alignment::left;    }
        case 'c': case 'C': { return text_alignment::center;  }
        case 'r': case 'R': { return text_alignment::right;   }
        case 'j': case 'J': { return text_alignment::justify; }
        default:            { return _fallback;               }
    }
}


// ===========================================================================
// IV.  Accessors
// ===========================================================================
//   Thin readers over doc_attributes so a renderer need not repeat the
// find-or-default dance.  All are non-throwing (unlike doc_attributes::at).

// attr_has
//   function: whether _attrs binds _key at all.
D_NODISCARD inline bool
attr_has(
    const doc_attributes& _attrs,
    const std::string&    _key
)
{
    return _attrs.contains(_key);
}

// attr_or
//   function: the value bound to _key, or _fallback when _key is absent.
D_NODISCARD inline std::string
attr_or(
    const doc_attributes& _attrs,
    const std::string&    _key,
    const std::string&    _fallback = std::string()
)
{
    const std::string* _p = _attrs.find(_key);

    // an absent hint yields the caller's fallback, never throws
    if (_p == nullptr)
    {
        return _fallback;
    }

    return *_p;
}

// attr_flag
//   function: _key read as a boolean -- true for "true" / "1" / "yes" / "on"
// (first character t/1/y/o, case-insensitive), _fallback when _key is absent,
// false otherwise.
D_NODISCARD inline bool
attr_flag(
    const doc_attributes& _attrs,
    const std::string&    _key,
    bool                  _fallback = false
)
{
    const std::string* _p = _attrs.find(_key);

    // an absent flag keeps the caller's default
    if (_p == nullptr)
    {
        return _fallback;
    }

    // an empty value is treated as unset
    if (_p->empty())
    {
        return _fallback;
    }

    const char _c = (*_p)[0];

    return ( (_c == 't') || (_c == 'T') ||
             (_c == '1')                ||
             (_c == 'y') || (_c == 'Y') ||
             (_c == 'o') || (_c == 'O') );
}

// attr_align
//   function: the `align` hint as a text_alignment, or _fallback when the hint
// is absent.
D_NODISCARD inline text_alignment
attr_align(
    const doc_attributes& _attrs,
    text_alignment        _fallback = text_alignment::left
)
{
    const std::string* _p = _attrs.find(doc_attr_align);

    // no alignment hint: keep the caller's default
    if (_p == nullptr)
    {
        return _fallback;
    }

    return align_from_string(*_p, _fallback);
}


NS_END  // djinterp


#endif  // DJINTERP_UTIL_DOCUMENT_ATTRIBUTES_
