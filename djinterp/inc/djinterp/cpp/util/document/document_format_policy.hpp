/*******************************************************************************
* djinterp [cpp]                                      document_format_policy.hpp
*
*   The runtime bridge from a `DDocumentFormat` selector to a boxed print
* policy -- the piece that was split out of document_format.hpp so that header
* could stay a dependency-free C++11 value module.  Everything here needs the
* `DDocument<>` façade and `std::optional`, so everything here is C++17 and
* pulls in document.hpp; a translation unit that only SELECTS a format never
* includes this and never pays for the serialiser.
*
*   WHAT THE FACTORY COVERS:
*   `make_print_policy` bridges the ANGLE-BRACKET MARKUP formats to a boxed
* print policy for the runtime-policy façade (`DDocument<DBoxedPrintPolicy>`).
* Today only `xml` is wired; `html` is one line away once an `DHtmlPrintPolicy`
* exists, and `markdown` / `tex` / `wiki` / `text` need their own serialiser
* (they are not just different escaping over the same tree), so they return
* `std::nullopt` until one is written.
*
*   THE OTHER ROUTE TO THE SAME FORMATS:
*   A print policy serialises a document TREE (the DDocumentWriter arena).  The
* semantic-renderer stack reaches text / markdown / xml / html by a different
* road -- `make_document_dialect` (document_dialect.hpp) hands back a
* DDocumentRenderer a template streams into.  The two are complementary, not
* rivals: use a policy when you already hold a node tree, a renderer when you
* are emitting a template or a layout term.  Both are selected by the same
* `DDocumentFormat`.
*
*   PDF IS DIFFERENT (read this):
*   `pdf` is a first-class built-in, but it is NOT a print policy -- it renders
* through the separate `DPdfDocument` drawing pipeline (pdf.hpp), which has a
* page/canvas build model, not a node tree.  `make_print_policy(DDocumentFormat
* ::pdf)` therefore returns `std::nullopt` on purpose; a pipeline that supports
* PDF branches on `format_is_binary(fmt)` and routes to `DPdfDocument` before it
* reaches this factory.
*
*   Requires C++17 (`std::optional`, and the façade it bridges to); self-
* suppresses below it.
*
*
* path:      /inc/djinterp/cpp/util/document/document_format_policy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.05
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    RUNTIME FACTORY               (make_print_policy / make_boxed_document)
      -----------------------------------------------------------------------
*/

#ifndef DJINTERP_UTIL_DOCUMENT_DOCUMENT_FORMAT_POLICY_HPP
#define DJINTERP_UTIL_DOCUMENT_DOCUMENT_FORMAT_POLICY_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <optional>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./document_format.hpp"   // DDocumentFormat (the selector)
#include "./document.hpp"          // document, and (transitively)
                                   // DBoxedPrintPolicy, DXmlPrintPolicy,
                                   // DPrintOptions


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  I.   RUNTIME FACTORY                                   ///
///////////////////////////////////////////////////////////////////////////////

// make_print_policy
//   function: bridge a runtime format selector to a boxed print policy for the
// runtime-policy façade, or `std::nullopt` when the format is not served by a
// print policy.  See the header note: markup dialects without a policy yet, and
// `pdf` (which renders via the separate DPdfDocument pipeline), all yield
// nullopt.
D_NODISCARD inline std::optional<DBoxedPrintPolicy> make_print_policy(
    DDocumentFormat _format
)
{
    switch (_format)
    {
        case DDocumentFormat::xml:
        {
            return DBoxedPrintPolicy(DXmlPrintPolicy{});
        }

        // html -- wire here once an `DHtmlPrintPolicy` exists:
        //     return boxed_print_policy(DHtmlPrintPolicy{});
        case DDocumentFormat::html:

        // markdown / tex / wiki / text -- not angle-bracket markup; they need
        // their own serialiser, not a print policy.  For these the renderer
        // road (make_document_dialect) is the one that answers.
        case DDocumentFormat::markdown:
        case DDocumentFormat::tex:
        case DDocumentFormat::wiki:
        case DDocumentFormat::text:

        // pdf -- supported, but through DPdfDocument, not a policy (branch on
        // `format_is_binary` upstream of this factory).
        case DDocumentFormat::pdf:
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}


// make_boxed_document
//   function: build a runtime-policy façade for `_format`, ready to build a
// tree into and serialise.  `std::nullopt` when the format has no print policy
// (see make_print_policy) -- notably `pdf`, which uses DPdfDocument instead.
D_NODISCARD inline std::optional<DDocument<DBoxedPrintPolicy>> make_boxed_document(
    DDocumentFormat _format
)
{
    std::optional<DBoxedPrintPolicy> _policy = make_print_policy(_format);

    // nothing to build a markup façade from (pdf, or an unwired dialect)
    if (!_policy)
    {
        return std::nullopt;
    }

    return DDocument<DBoxedPrintPolicy>(
        static_cast<DBoxedPrintPolicy&&>(*_policy));
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_DOCUMENT_FORMAT_POLICY_HPP
