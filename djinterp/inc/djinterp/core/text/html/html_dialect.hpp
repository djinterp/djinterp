/******************************************************************************
* djinterp [text]                                             html_dialect.hpp
*
*   Compile-time policy template that captures an HTML / XHTML
* dialect via a single enum-parameterized template. Properties of the
* dialect (whether to self-close void elements, whether to allow
* HTML5-only kinds, what DOCTYPE to emit, etc.) are exposed as
* `D_STATIC_CONSTEXPR` constants and `static D_CONSTEXPR` functions
* whose values fold at compile time when the template is instantiated
* -- zero runtime cost.
*
*   The enum used for the template parameter is the existing
* `html_version` enum from `html.hpp`; that enum already enumerates
* every HTML / XHTML profile, so introducing a parallel
* `html_dialect_flag` taxonomy would be redundant. The same enum
* value drives both the runtime version field on `html_document` and
* the static dialect template parameter.
*
*   FOR XHTML VARIANTS:
*   The dialect exposes an `underlying_xml_dialect_t` type alias
* pointing at an `xml_dialect<_Flag>` instantiation -- specifically
* `xml_dialect<xml_dialect_flag::xhtml_compatible>` -- so XHTML
* emission can defer to the XML dialect for namespace handling,
* declaration emission, and canonicalization rules. For pure HTML
* variants the alias points at `xml_dialect_flag::unspecified` and
* is not meaningful.
*
*   ZERO OVERHEAD:
*   When the template is instantiated for a fixed flag, every
* property collapses to a constant the optimiser inlines.
*
*   EXTENSIBILITY:
*   Third parties wanting a custom dialect can either use
* `html_version::custom` (and intercept it in the helpers), OR
* define their own struct exposing the same `D_STATIC_CONSTEXPR`
* surface and pass it wherever an `html_dialect<_Flag>` would be
* accepted. The trait `is_html_dialect<_Type>` recognizes both
* forms.
*
*
* path:      /inc/djinterp/core/text/html/html_dialect.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.09
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    INTERNAL CLASSIFICATION HELPERS
II.   INTERNAL VALIDATION HELPERS
III.  html_dialect<_Flag>
IV.   CONVENIENCE ALIASES
V.    DIALECT DETECTION TRAITS
*/

#ifndef DJINTERP_HTML_DIALECT_
#define DJINTERP_HTML_DIALECT_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "./html.hpp"
#include "../xml/xml_dialect.hpp"


NS_DJINTERP

namespace html {


///////////////////////////////////////////////////////////////////////////////
///                I.   INTERNAL CLASSIFICATION HELPERS                     ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace html
NS_INTERNAL

    // is_html5_family_helper
    //   function: true if `_flag` is HTML5 or XHTML5. These two
    // share the same valid element / attribute set; their only
    // real differences are emission rules.
    D_CONSTEXPR_INLINE bool
    is_html5_family_helper(
        ::djinterp::html::html_version  _flag
    )
    {
        return ( (_flag == ::djinterp::html::html_version::html5)  ||
                 (_flag == ::djinterp::html::html_version::xhtml5) );
    }


    // is_html4_family_helper
    //   function: true if `_flag` is one of the HTML4 / XHTML1
    // variants.
    D_CONSTEXPR_INLINE bool
    is_html4_family_helper(
        ::djinterp::html::html_version  _flag
    )
    {
        return (
               (_flag == ::djinterp::html::html_version::html4_strict)
            || (_flag == ::djinterp::html::html_version::html4_transitional)
            || (_flag == ::djinterp::html::html_version::html4_frameset)
            || (_flag == ::djinterp::html::html_version::xhtml1_strict)
            || (_flag == ::djinterp::html::html_version::xhtml1_transitional)
            || (_flag == ::djinterp::html::html_version::xhtml1_frameset)
        );
    }


    // is_html_frameset_family_helper
    //   function: true if `_flag` is one of the frameset
    // variants. Frameset documents replace `<body>` semantics
    // with `<frameset>` (which our element-kind enum does not
    // currently model); flagging this lets dialect-aware code
    // gate frame handling in adapter layers.
    D_CONSTEXPR_INLINE bool
    is_html_frameset_family_helper(
        ::djinterp::html::html_version  _flag
    )
    {
        return ( (_flag == ::djinterp::html::html_version::html4_frameset)  ||
                 (_flag == ::djinterp::html::html_version::xhtml1_frameset) );
    }


    // is_html_strict_family_helper
    //   function: true if `_flag` is one of the strict variants.
    // Strict variants forbid presentational legacy elements
    // (`<font>`, `<center>`, etc.) -- those are not currently
    // modelled in our element-kind enum, but the predicate is
    // available for adapter layers that do model them.
    D_CONSTEXPR_INLINE bool
    is_html_strict_family_helper(
        ::djinterp::html::html_version  _flag
    )
    {
        return ( (_flag == ::djinterp::html::html_version::html4_strict)   ||
                 (_flag == ::djinterp::html::html_version::xhtml1_strict)  ||
                 (_flag == ::djinterp::html::html_version::html5)          ||
                 (_flag == ::djinterp::html::html_version::xhtml5) );
    }

NS_END  // internal
namespace html {


///////////////////////////////////////////////////////////////////////////////
///                II.   INTERNAL VALIDATION HELPERS                        ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace html
NS_INTERNAL

    // is_html5_only_element_kind_helper
    //   function: true if `_kind` was introduced in HTML5 and is
    // therefore not available in HTML4 / XHTML1 dialects.
    // Keeps the per-dialect validity table small by separating
    // "modern" kinds from "legacy" ones.
    D_CONSTEXPR_INLINE bool
    is_html5_only_element_kind_helper(
        ::djinterp::html::html_element_kind     _kind
    )
    {
        using K = ::djinterp::html::html_element_kind;
        return (
               // sectioning (added HTML5)
               (_kind == K::header)     || (_kind == K::nav)
            || (_kind == K::main_)      || (_kind == K::section)
            || (_kind == K::article)    || (_kind == K::aside)
            || (_kind == K::footer)
               // grouping (added HTML5)
            || (_kind == K::hgroup)     || (_kind == K::figure)
            || (_kind == K::figcaption)
               // text-level (added HTML5)
            || (_kind == K::data_)      || (_kind == K::time_)
            || (_kind == K::mark)       || (_kind == K::bdi)
            || (_kind == K::ruby)       || (_kind == K::rt)
            || (_kind == K::rp)         || (_kind == K::wbr)
               // embedded (added HTML5)
            || (_kind == K::video)      || (_kind == K::audio)
            || (_kind == K::source)     || (_kind == K::track)
            || (_kind == K::canvas)     || (_kind == K::picture)
            || (_kind == K::svg)        || (_kind == K::math)
            || (_kind == K::embed)
               // forms (added HTML5)
            || (_kind == K::datalist)   || (_kind == K::output)
            || (_kind == K::progress)   || (_kind == K::meter)
               // interactive (added HTML5)
            || (_kind == K::details)    || (_kind == K::summary)
            || (_kind == K::dialog)
               // scripting (added HTML5)
            || (_kind == K::template_)  || (_kind == K::slot)
        );
    }


    // html_dialect_is_valid_element_kind_helper
    //   function: per-dialect element-kind validation. Returns
    // true if `_kind` is permitted in the dialect identified by
    // `_flag`. Owns the master table; the dialect template's
    // `is_valid_element_kind` predicate delegates here.
    D_CONSTEXPR_INLINE bool
    html_dialect_is_valid_element_kind_helper(
        ::djinterp::html::html_version          _flag,
        ::djinterp::html::html_element_kind     _kind
    )
    {
        using K = ::djinterp::html::html_element_kind;

        // unknown is never valid
        if (_kind == K::unknown)
        {
            return false;
        }

        // custom / unspecified flags accept everything (the
        // caller has opted out of static validation)
        if ( (_flag == ::djinterp::html::html_version::custom) ||
             (_flag == ::djinterp::html::html_version::unspecified) )
        {
            return true;
        }

        // HTML5 / XHTML5 accept everything in our enum (we don't
        // model the deprecated HTML4-only kinds)
        if (is_html5_family_helper(_flag))
        {
            return true;
        }

        // HTML4 / XHTML1 reject the HTML5-only additions
        if (is_html4_family_helper(_flag))
        {
            if (is_html5_only_element_kind_helper(_kind))
            {
                return false;
            }
            // the frameset variants additionally restrict <body>,
            // but since our enum does not model <frameset> /
            // <frame> / <noframes>, there is nothing useful to
            // reject beyond the HTML5-only kinds.
            return true;
        }

        // unreachable in practice
        return false;
    }


    // html_dialect_is_void_element_in_helper
    //   function: per-dialect void-element classification. The
    // void set is essentially stable across HTML4/5 (`<br>`,
    // `<hr>`, `<img>`, `<input>`, `<meta>`, `<link>`, `<area>`,
    // `<base>`, `<param>`); HTML5 added `<source>`, `<track>`,
    // `<wbr>`, `<embed>`, `<keygen>` to the set. We delegate to
    // the kind classifier in `html.hpp` and combine with the
    // dialect's admissibility rule.
    D_CONSTEXPR_INLINE bool
    html_dialect_is_void_element_in_helper(
        ::djinterp::html::html_version          _flag,
        ::djinterp::html::html_element_kind     _kind
    )
    {
        return ( ::djinterp::html::is_void_element_kind(_kind) &&
                 html_dialect_is_valid_element_kind_helper(_flag, _kind) );
    }

NS_END  // internal
namespace html {


///////////////////////////////////////////////////////////////////////////////
///                III.   html_dialect<_Flag>                               ///
///////////////////////////////////////////////////////////////////////////////

// html_dialect
//   struct: compile-time policy capturing the rules of an HTML /
// XHTML dialect. Every property is `D_STATIC_CONSTEXPR`;
// instantiating this template for a specific flag produces a
// type whose properties fold to constants at compile time.
// Use the convenience aliases below for the common dialects.
template<html_version    _Flag>
struct html_dialect
{
    /// identity

    // flag
    //   constant: the dialect flag this instantiation represents.
    D_STATIC_CONSTEXPR html_version flag = _Flag;

    // is_xhtml
    //   constant: true if this is an XHTML variant (xhtml1_*,
    // xhtml5).
    D_STATIC_CONSTEXPR bool is_xhtml = is_xhtml_version(_Flag);

    // is_html5_family
    //   constant: true for html5 and xhtml5.
    D_STATIC_CONSTEXPR bool is_html5_family =
        ::djinterp::internal::is_html5_family_helper(_Flag);

    // is_html4_family
    //   constant: true for any of the HTML4 / XHTML1 variants.
    D_STATIC_CONSTEXPR bool is_html4_family =
        ::djinterp::internal::is_html4_family_helper(_Flag);

    // is_strict
    //   constant: true for the strict variants of HTML4 / XHTML1
    // and for HTML5 / XHTML5 (which have no transitional form).
    D_STATIC_CONSTEXPR bool is_strict =
        ::djinterp::internal::is_html_strict_family_helper(_Flag);

    // is_frameset
    //   constant: true for the frameset variants of HTML4 / XHTML1.
    D_STATIC_CONSTEXPR bool is_frameset =
        ::djinterp::internal::is_html_frameset_family_helper(_Flag);


    /// XML coupling (for XHTML variants)

    // underlying_xml_dialect_t
    //   type: the XML dialect that XHTML variants of this flag
    // serialize through. For pure HTML variants this points at
    // `xml_dialect<xml_dialect_flag::unspecified>` and is not
    // meaningful -- callers should gate use on `is_xhtml`.
    using underlying_xml_dialect_t = xml_dialect<
        is_xhtml_version(_Flag)
            ? xml_dialect_flag::xhtml_compatible
            : xml_dialect_flag::unspecified
    >;


    /// emission rules

    // self_close_void_elements
    //   constant: whether void elements are emitted as `<br/>`
    // (XHTML) or `<br>` (HTML).
    D_STATIC_CONSTEXPR bool self_close_void_elements =
        is_xhtml_version(_Flag);

    // require_lowercase_tags
    //   constant: whether tag names must be emitted in lowercase.
    // XHTML requires this; HTML is case-insensitive but lowercase
    // is the convention.
    D_STATIC_CONSTEXPR bool require_lowercase_tags =
        is_xhtml_version(_Flag);

    // require_quoted_attrs
    //   constant: whether attribute values must be quoted. XHTML
    // requires this; HTML allows unquoted values for many
    // attributes but quoting is always safe.
    D_STATIC_CONSTEXPR bool require_quoted_attrs =
        is_xhtml_version(_Flag);

    // attr_quote_char
    //   constant: the quote character used for attribute values.
    D_STATIC_CONSTEXPR char attr_quote_char = '"';

    // allow_unclosed_optional
    //   constant: whether end tags may be omitted for elements
    // where the spec allows it (`<li>`, `<p>`, `<tr>`, etc.).
    // HTML allows this; XHTML does not.
    D_STATIC_CONSTEXPR bool allow_unclosed_optional = (
        ! is_xhtml_version(_Flag)
    );

    // allow_boolean_attribute_shorthand
    //   constant: whether boolean attributes may be emitted as
    // `<input disabled>` (HTML) vs `<input disabled="disabled" />`
    // (XHTML).
    D_STATIC_CONSTEXPR bool allow_boolean_attribute_shorthand = (
        ! is_xhtml_version(_Flag)
    );

    // emit_xml_declaration
    //   constant: whether to emit the `<?xml ... ?>` processing
    // instruction at the top of the document. Required by XHTML1
    // when not served as text/html; XHTML5 (polyglot) typically
    // omits it.
    D_STATIC_CONSTEXPR bool emit_xml_declaration = (
           (_Flag == html_version::xhtml1_strict)
        || (_Flag == html_version::xhtml1_transitional)
        || (_Flag == html_version::xhtml1_frameset)
    );


    /// feature gating

    // html5_features_allowed
    //   constant: whether HTML5-only kinds (article, section,
    // video, etc.) are valid in this dialect.
    D_STATIC_CONSTEXPR bool html5_features_allowed = is_html5_family;

    // frameset_features_allowed
    //   constant: whether frameset elements (`<frameset>`,
    // `<frame>`, `<noframes>`) are valid in this dialect.
    // Modelled here for adapter layers that extend the kind
    // enum to cover them.
    D_STATIC_CONSTEXPR bool frameset_features_allowed = is_frameset;

    // legacy_presentational_allowed
    //   constant: whether the legacy presentational elements
    // (`<font>`, `<center>`, `<s>`, `<strike>`, `<u>`,
    // `<basefont>`) are valid. Permitted in transitional /
    // frameset variants of HTML4 and XHTML1; forbidden
    // elsewhere. Modelled here for adapter layers that extend
    // the kind enum to cover them.
    D_STATIC_CONSTEXPR bool legacy_presentational_allowed = (
           (_Flag == html_version::html4_transitional)
        || (_Flag == html_version::html4_frameset)
        || (_Flag == html_version::xhtml1_transitional)
        || (_Flag == html_version::xhtml1_frameset)
    );


    /// doctype

    // doctype_string
    //   function: returns the DOCTYPE declaration string for
    // this dialect. Delegates to the existing free function in
    // `html.hpp`.
    static D_CONSTEXPR const char*
    doctype_string()
    {
        return html_doctype_string(_Flag);
    }


    /// validation predicates (delegate to internal helpers for
    /// the larger tables)

    // is_valid_element_kind
    //   function: true if `_kind` is permitted in this dialect.
    static D_CONSTEXPR bool
    is_valid_element_kind(
        html_element_kind   _kind
    )
    {
        return ::djinterp::internal::
            html_dialect_is_valid_element_kind_helper(_Flag, _kind);
    }

    // is_void_element
    //   function: true if `_kind` is a void element AND is
    // permitted in this dialect.
    static D_CONSTEXPR bool
    is_void_element(
        html_element_kind   _kind
    )
    {
        return ::djinterp::internal::
            html_dialect_is_void_element_in_helper(_Flag, _kind);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   CONVENIENCE ALIASES                                ///
///////////////////////////////////////////////////////////////////////////////

// canonical names for the standard dialects.
using html_dialect_html5               = html_dialect<html_version::html5>;
using html_dialect_html4_strict        = html_dialect<html_version::html4_strict>;
using html_dialect_html4_transitional  = html_dialect<html_version::html4_transitional>;
using html_dialect_html4_frameset      = html_dialect<html_version::html4_frameset>;
using html_dialect_xhtml1_strict       = html_dialect<html_version::xhtml1_strict>;
using html_dialect_xhtml1_transitional = html_dialect<html_version::xhtml1_transitional>;
using html_dialect_xhtml1_frameset     = html_dialect<html_version::xhtml1_frameset>;
using html_dialect_xhtml5              = html_dialect<html_version::xhtml5>;


///////////////////////////////////////////////////////////////////////////////
///                V.   DIALECT DETECTION TRAITS                            ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace html
NS_INTERNAL

    // has_html_dialect_flag_helper
    //   trait: SFINAE detector for `_Type::flag` of type
    // `html_version`. Primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_html_dialect_flag_helper
    {
        D_STATIC_CONSTEXPR bool value = false;
    };

    // has_html_dialect_flag_helper (specialization)
    //   trait: success case when `_Type::flag` exists and is of
    // type `html_version`.
    template<typename _Type>
    struct has_html_dialect_flag_helper<
        _Type,
        typename std::enable_if<
            std::is_same<
                clean_t<decltype(_Type::flag)>,
                ::djinterp::html::html_version
            >::value
        >::type
    >
    {
        D_STATIC_CONSTEXPR bool value = true;
    };

NS_END  // internal
namespace html {


// is_html_dialect
//   trait: true if `_Type` exposes the HTML dialect surface --
// specifically a `flag` member of type `html_version`. Detects
// both `html_dialect<_Flag>` instantiations and third-party
// policy structs that follow the same convention.
template<typename    _Type>
struct is_html_dialect
{
    D_STATIC_CONSTEXPR bool value =
        ::djinterp::internal::has_html_dialect_flag_helper<
            clean_t<_Type>
        >::value;
};


// is_html_dialect_v
//   constant: convenience accessor for is_html_dialect<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_STATIC_CONSTEXPR bool is_html_dialect_v =
        is_html_dialect<_Type>::value;
#endif


}   // namespace html
NS_END  // djinterp


#endif  // DJINTERP_HTML_DIALECT_
