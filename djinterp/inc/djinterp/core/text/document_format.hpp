/******************************************************************************
* djinterp [text]                                          document_format.hpp
*
*   The catalogue of the framework's built-in document formats, plus the thin
* runtime bridge from a chosen format to a serialiser. This is the piece the
* options subframework selects on -- e.g.
*
*     opts.set<option::document>(document_format::pdf);
*
* -- so it deliberately stays a small value-and-factory header and adds NO core
* machinery. `document<_Policy>` and `boxed_print_policy` are unchanged; a
* custom format is still `document<user_policy>` and never appears here (the
* enum names only what the framework ships, by design).
*
*   WHAT THE FACTORY COVERS:
*   `make_print_policy` bridges the ANGLE-BRACKET MARKUP formats to a boxed
* print policy for the runtime-policy façade (`document<boxed_print_policy>`).
* Today only `xml` is wired; `html` is one line away once an `html_print_policy`
* exists, and `markdown` / `tex` / `wiki` / `text` need their own serialiser
* (they are not just different escaping over the same tree), so they return
* `std::nullopt` until one is written.
*
*   PDF IS DIFFERENT (read this):
*   `pdf` is a first-class built-in, but it is NOT a print policy -- it renders
* through the separate `pdf_document` drawing pipeline (pdf.hpp), which has a
* page/canvas build model, not a node tree. `make_print_policy(document_format
* ::pdf)` therefore returns `std::nullopt` on purpose; a pipeline that supports
* PDF branches on `== document_format::pdf` and routes to `pdf_document` before
* it reaches this factory. Keeping the enum whole (pdf included) is what lets it
* serve as the single options selector across all outputs.
*
*   Requires C++17 (`std::optional`, and the façade it bridges to); self-
* suppresses below it.
*
*
* path:      /inc/djinterp/core/text/document_format.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.05
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DOCUMENT FORMAT
      ---------------
      a. document_format
      b. format_name

II.   RUNTIME FACTORY
      ---------------
      a. make_print_policy
      b. make_boxed_document
*/

#ifndef DJINTERP_TEXT_DOCUMENT_FORMAT_
#define DJINTERP_TEXT_DOCUMENT_FORMAT_ 1

// std
#include <optional>
// djinterp
#include "../djinterp.hpp"
#include "./document.hpp"   // document, and (transitively) boxed_print_policy,
                            // xml_print_policy, print_options


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  I.   DOCUMENT FORMAT                                   ///
///////////////////////////////////////////////////////////////////////////////

// document_format
//   enum: the closed catalogue of the framework's built-in document formats,
// for use as a selector value (e.g. through the options subframework). Custom
// user formats are `document<user_policy>` and intentionally never listed here
// -- the enum names only what the framework ships.
enum class document_format
{
    html,
    markdown,
    pdf,
    tex,
    text,
    wiki,
    xml
};


// format_name
//   function: the lower-case token for a format -- for config strings,
// logging, and round-tripping a selector to/from text.
D_NODISCARD inline const char*
format_name(
    document_format _format
)
{
    switch (_format)
    {
        case document_format::html:     return "html";
        case document_format::markdown: return "markdown";
        case document_format::pdf:      return "pdf";
        case document_format::tex:      return "tex";
        case document_format::text:     return "text";
        case document_format::wiki:     return "wiki";
        case document_format::xml:      return "xml";
    }

    return "";
}


///////////////////////////////////////////////////////////////////////////////
///                  II.   RUNTIME FACTORY                                  ///
///////////////////////////////////////////////////////////////////////////////

// make_print_policy
//   function: bridge a runtime format selector to a boxed print policy for the
// runtime-policy façade, or `std::nullopt` when the format is not served by a
// print policy. See the header note: markup dialects without a policy yet, and
// `pdf` (which renders via the separate pdf_document pipeline), all yield
// nullopt.
D_NODISCARD inline std::optional<boxed_print_policy>
make_print_policy(
    document_format _format
)
{
    switch (_format)
    {
        case document_format::xml:
        {
            return boxed_print_policy(xml_print_policy{});
        }

        // html -- wire here once an `html_print_policy` exists:
        //     return boxed_print_policy(html_print_policy{});
        case document_format::html:

        // markdown / tex / wiki / text -- not angle-bracket markup; they need
        // their own serialiser, not a print policy.
        case document_format::markdown:
        case document_format::tex:
        case document_format::wiki:
        case document_format::text:

        // pdf -- supported, but through pdf_document, not a policy (branch on
        // `== document_format::pdf` upstream of this factory).
        case document_format::pdf:
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}


// make_boxed_document
//   function: build a runtime-policy façade for `_format`, ready to build a
// tree into and serialise. `std::nullopt` when the format has no print policy
// (see make_print_policy) -- notably `pdf`, which uses pdf_document instead.
D_NODISCARD inline std::optional<document<boxed_print_policy>>
make_boxed_document(
    document_format _format
)
{
    std::optional<boxed_print_policy> _policy = make_print_policy(_format);

    // nothing to build a markup façade from (pdf, or an unwired dialect)
    if (!_policy)
    {
        return std::nullopt;
    }

    return document<boxed_print_policy>(
        static_cast<boxed_print_policy&&>(*_policy));
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEXT_DOCUMENT_FORMAT_
