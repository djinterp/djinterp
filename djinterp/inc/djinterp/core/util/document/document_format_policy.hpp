/******************************************************************************
* djinterp [utility]                                 document_format_policy.hpp
*
*   The runtime bridge from a `document_format` selector to a boxed print
* policy -- the piece that was split out of document_format.hpp so that header
* could stay a dependency-free C++11 value module.  Everything here needs the
* `document<>` façade and `std::optional`, so everything here is C++17 and
* pulls in document.hpp; a translation unit that only SELECTS a format never
* includes this and never pays for the serialiser.
*
*   WHAT THE FACTORY COVERS:
*   `make_print_policy` bridges the ANGLE-BRACKET MARKUP formats to a boxed
* print policy for the runtime-policy façade (`document<boxed_print_policy>`).
* Today only `xml` is wired; `html` is one line away once an `html_print_policy`
* exists, and `markdown` / `tex` / `wiki` / `text` need their own serialiser
* (they are not just different escaping over the same tree), so they return
* `std::nullopt` until one is written.
*
*   THE OTHER ROUTE TO THE SAME FORMATS:
*   A print policy serialises a document TREE (the document_writer arena).  The
* semantic-renderer stack reaches text / markdown / xml / html by a different
* road -- `make_document_dialect` (document_dialect.hpp) hands back a
* document_renderer a template streams into.  The two are complementary, not
* rivals: use a policy when you already hold a node tree, a renderer when you
* are emitting a template or a layout term.  Both are selected by the same
* `document_format`.
*
*   PDF IS DIFFERENT (read this):
*   `pdf` is a first-class built-in, but it is NOT a print policy -- it renders
* through the separate `pdf_document` drawing pipeline (pdf.hpp), which has a
* page/canvas build model, not a node tree.  `make_print_policy(document_format
* ::pdf)` therefore returns `std::nullopt` on purpose; a pipeline that supports
* PDF branches on `format_is_binary(fmt)` and routes to `pdf_document` before it
* reaches this factory.
*
*   Requires C++17 (`std::optional`, and the façade it bridges to); self-
* suppresses below it.
*
*
* TABLE OF CONTENTS
* =================
* I.    RUNTIME FACTORY               (make_print_policy / make_boxed_document)
*
*
* path:      /inc/djinterp/core/util/document/document_format_policy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.05
******************************************************************************/

#ifndef DJINTERP_UTIL_DOCUMENT_FORMAT_POLICY_
#define DJINTERP_UTIL_DOCUMENT_FORMAT_POLICY_ 1

// std
#include <optional>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./document_format.hpp"   // document_format (the selector)
#include "./document.hpp"          // document, and (transitively)
                                   // boxed_print_policy, xml_print_policy,
                                   // print_options


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  I.   RUNTIME FACTORY                                   ///
///////////////////////////////////////////////////////////////////////////////

// make_print_policy
//   function: bridge a runtime format selector to a boxed print policy for the
// runtime-policy façade, or `std::nullopt` when the format is not served by a
// print policy.  See the header note: markup dialects without a policy yet, and
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
        // their own serialiser, not a print policy.  For these the renderer
        // road (make_document_dialect) is the one that answers.
        case document_format::markdown:
        case document_format::tex:
        case document_format::wiki:
        case document_format::text:

        // pdf -- supported, but through pdf_document, not a policy (branch on
        // `format_is_binary` upstream of this factory).
        case document_format::pdf:
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}


// make_boxed_document
//   function: build a runtime-policy façade for `_format`, ready to build a
// tree into and serialise.  `std::nullopt` when the format has no print policy
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


#endif  // DJINTERP_UTIL_DOCUMENT_FORMAT_POLICY_
