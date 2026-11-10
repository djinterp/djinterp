/*******************************************************************************
* djinterp [cpp]                                             document_format.hpp
*
*   The catalogue of the framework's built-in document formats -- the ONE enum
* every layer selects an output format with.  It is deliberately a small,
* dependency-free value header: the enum, its token / extension spellings, and
* two classification predicates, and nothing else.  Everything that needs to
* KNOW a format (a print policy, a renderer, an emitter) takes it as a
* parameter and lives elsewhere.
*
*   WHY IT IS LIGHT (read this before adding an include):
*   This header is included by configuration headers -- test_options.hpp among
* them -- that must stay free of the document engines and must compile at the
* C++11 floor.  The runtime print-policy factory therefore does NOT live here;
* it moved to document_format_policy.hpp, which is C++17 and pulls in
* document.hpp.  Keeping the split means selecting a format costs nothing and
* never drags a serialiser into a translation unit that only configures one.
*
*   THE SINGLE SELECTOR:
*   `DDocumentFormat` supersedes the per-layer duplicates that grew alongside
* it -- DTest's `doc_format` (test_output.hpp) and `test_doc_type`
* (test_options.hpp) are now aliases of it.  Those spelled overlapping subsets
* with diverging enumerator names (`txt` vs `text`) and needed a hand-written
* switch to bridge; one enum removes the bridge and the class of mis-mapping
* bugs that came with it.
*
*   NOT EVERY FORMAT IS SERVED BY EVERY LAYER, and that is by design.  A
* consumer switches on what it supports and falls back (or declines) on the
* rest: `make_print_policy` answers for the angle-bracket markup dialects,
* `make_document_dialect` answers for the semantic-renderer dialects, and `pdf`
* is served by neither -- it renders through the DPdfDocument pipeline, which
* has a page / canvas build model rather than a node tree.  Keeping the enum
* WHOLE (pdf included) is what lets it be the single options selector across
* all outputs.
*
*   Custom user formats are `DDocument<user_policy>` / a bespoke
* DDocumentRenderer and intentionally never appear here -- the enum names only
* what the framework ships.
*
*   PORTABILITY:
*   C++11 baseline; no standard-library dependency beyond <cstddef> / <string>.
*
*
* path:      /inc/djinterp/cpp/util/document/document_format.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.05
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DDocumentFormat                (the selector enum)
      --------------------------------------------------

II.   SPELLINGS                      (format_name / format_extension /
      ----------------------------------------------------------------

      format_from_name)

III.  CLASSIFICATION                 (format_is_markup / format_is_binary)
      --------------------------------------------------------------------
*/

#ifndef DJINTERP_UTIL_DOCUMENT_DOCUMENT_FORMAT_HPP
#define DJINTERP_UTIL_DOCUMENT_DOCUMENT_FORMAT_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <string>
// djinterp
#include "../../djinterp.hpp"   // NS_*, D_NODISCARD, D_NOEXCEPT, gates


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  I.   DOCUMENT FORMAT                                   ///
///////////////////////////////////////////////////////////////////////////////

// DDocumentFormat
//   enum: the closed catalogue of the framework's built-in document formats,
// used as the selector value by every layer that emits a document (the options
// subframework, the emit layer, the renderer factory).  Custom user formats are
// `DDocument<user_policy>` and intentionally never listed here.
enum class DDocumentFormat
{
    html,
    markdown,
    pdf,
    tex,
    text,
    wiki,
    xml
};


///////////////////////////////////////////////////////////////////////////////
///                  II.  SPELLINGS                                         ///
///////////////////////////////////////////////////////////////////////////////

// format_name
//   function: the lower-case token for a format -- for config strings,
// logging, and round-tripping a selector to/from text.  Round-trips through
// format_from_name.
D_NODISCARD inline const char* format_name(
    DDocumentFormat _format
) D_NOEXCEPT
{
    switch (_format)
    {
        case DDocumentFormat::html:     { return "html";     }
        case DDocumentFormat::markdown: { return "markdown"; }
        case DDocumentFormat::pdf:      { return "pdf";      }
        case DDocumentFormat::tex:      { return "tex";      }
        case DDocumentFormat::text:     { return "text";     }
        case DDocumentFormat::wiki:     { return "wiki";     }
        case DDocumentFormat::xml:      { return "xml";      }
    }

    return "text";
}


// format_extension
//   function: the conventional file extension for a format, LEADING DOT
// INCLUDED (".txt", ".md", ".pdf", ...) -- what a bundle's naming policy
// appends to a document's logical name.
//
//   This names a report DOCUMENT.  output_packaging's format_extension
// (format_id) names an archive CONTAINER; the two are an overload set on
// distinct scoped enums, so a call is never ambiguous.
D_NODISCARD inline const char* format_extension(
    DDocumentFormat _format
) D_NOEXCEPT
{
    switch (_format)
    {
        case DDocumentFormat::html:     { return ".html"; }
        case DDocumentFormat::markdown: { return ".md";   }
        case DDocumentFormat::pdf:      { return ".pdf";  }
        case DDocumentFormat::tex:      { return ".tex";  }
        case DDocumentFormat::text:     { return ".txt";  }
        case DDocumentFormat::wiki:     { return ".wiki"; }
        case DDocumentFormat::xml:      { return ".xml";  }
    }

    return ".txt";
}


// format_from_name
//   function: the format a token names, or _fallback when the token matches
// none.  Accepts the format_name spellings plus the common aliases a config
// file or command line is likely to carry ("txt", "md", "latex", "htm").
// Matching is exact and case-sensitive: a boundary that normalises case does
// so before calling.
D_NODISCARD inline DDocumentFormat format_from_name(
    const std::string& _token,
    DDocumentFormat    _fallback = DDocumentFormat::text
)
{
    // canonical spellings first -- the round-trip of format_name
    if (_token == "html")     { return DDocumentFormat::html;     }
    if (_token == "markdown") { return DDocumentFormat::markdown; }
    if (_token == "pdf")      { return DDocumentFormat::pdf;      }
    if (_token == "tex")      { return DDocumentFormat::tex;      }
    if (_token == "text")     { return DDocumentFormat::text;     }
    if (_token == "wiki")     { return DDocumentFormat::wiki;     }
    if (_token == "xml")      { return DDocumentFormat::xml;      }

    // accepted aliases -- what a human writes in a config
    if (_token == "txt")      { return DDocumentFormat::text;     }
    if (_token == "md")       { return DDocumentFormat::markdown; }
    if (_token == "htm")      { return DDocumentFormat::html;     }
    if (_token == "latex")    { return DDocumentFormat::tex;      }

    return _fallback;
}


///////////////////////////////////////////////////////////////////////////////
///                  III. CLASSIFICATION                                    ///
///////////////////////////////////////////////////////////////////////////////

// format_is_markup
//   function: whether a format is angle-bracket markup -- the formats whose
// VALUES must be entity-escaped while the surrounding tags are emitted raw.
// This is the predicate that selects an escaping renderer / sink over a plain
// one; a caller that only asks "do I escape?" asks here.
D_NODISCARD inline bool format_is_markup(
    DDocumentFormat _format
) D_NOEXCEPT
{
    return ( (_format == DDocumentFormat::xml) ||
             (_format == DDocumentFormat::html) );
}


// format_is_binary
//   function: whether a format's bytes are opaque rather than text -- true for
// `pdf` alone today.  A binary document is never line-oriented, never escaped,
// and is written to a sink verbatim; a caller that would otherwise treat bytes
// as a string branches on this.
D_NODISCARD inline bool format_is_binary(
    DDocumentFormat _format
) D_NOEXCEPT
{
    return (_format == DDocumentFormat::pdf);
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_DOCUMENT_FORMAT_HPP
