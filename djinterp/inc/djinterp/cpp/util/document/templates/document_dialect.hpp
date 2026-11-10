/*******************************************************************************
* djinterp [cpp]                                            document_dialect.hpp
*
*   The join between the format SELECTOR and the dialect set: given a
* `DDocumentFormat`, hand back the DDocumentRenderer that realises it, together
* with the one thing the abstract interface cannot expose -- how to get the
* bytes back out.  This is what turns "the configuration asked for html" into a
* renderer a template can stream into, without every call site growing its own
* switch over the format enum.
*
*   WHY A WRAPPER AND NOT A BARE unique_ptr.  `DDocumentRenderer` is a write-only
* semantic sink: it has no `str()`, deliberately, because a PDF back end has no
* string to give and a streaming back end may have written its bytes away
* already.  The concrete string dialects all DO have one, so DDocumentDialect
* carries the accessor alongside the renderer -- bound at construction, when the
* concrete type is still known.  A caller gets one movable value that answers
* both "where do I render?" and "what came out?".
*
*   WHAT IS SERVED.  text -> plain, markdown -> markdown, xml / html -> markup
* with the matching escape policy.  `pdf` is NOT served here and returns an
* invalid dialect on purpose: pdf_document_renderer pulls in the whole pdf.hpp
* tree, and this header is included by text-only builds that must not pay for
* it.  A PDF-capable caller checks `format_is_binary(fmt)` first and constructs
* pdf_document_renderer itself -- the same opt-in split document_renderer.hpp
* and pdf_document_renderer.hpp already keep.  `tex` and `wiki` are unserved
* until a renderer for them exists.
*
*   ALWAYS CHECK valid().  An unserved format yields a dialect whose valid() is
* false; renderer() on it would have nothing to return, so the accessor family
* is documented as callable only after the check.  This mirrors
* make_print_policy's std::nullopt rather than inventing a second failure idiom.
*
*   PORTABILITY:
*   C++11 baseline (matches the renderers it selects between).  Move-only, since
* it owns the renderer.
*
*
* path:      /inc/djinterp/cpp/util/document/templates/document_dialect.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.23
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DDocumentDialect                (the owning wrapper)
      ----------------------------------------------------

II.   make_document_dialect           (the factory)
      ---------------------------------------------

III.  render_to_string                (render any template in any dialect)
      --------------------------------------------------------------------
*/

#ifndef DJINTERP_UTIL_DOCUMENT_TEMPLATES_DOCUMENT_DIALECT_HPP
#define DJINTERP_UTIL_DOCUMENT_TEMPLATES_DOCUMENT_DIALECT_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"                // NS_*, D_NODISCARD, D_NOEXCEPT
#include "../document_format.hpp"               // DDocumentFormat, format_is_*
#include "./document_renderer.hpp"              // DDocumentRenderer,
                                                // DPlainDocumentRenderer
#include "./markdown_document_renderer.hpp"     // DMarkdownDocumentRenderer
#include "./markup_document_renderer.hpp"       // xml_ / DHtmlDocumentRenderer


NS_DJINTERP


// ===========================================================================
// I.   DDocumentDialect
// ===========================================================================

// DDocumentDialect
//   class: an owned DDocumentRenderer plus the accessors its concrete type
// provides -- the value make_document_dialect hands back.  Move-only; the
// renderer lives on the heap, so the bound accessors stay valid across a move.
//
// Usage:
//   DDocumentDialect _d = make_document_dialect(DDocumentFormat::html);
//
//   if (_d.valid())
//   {
//       _table.render(_d.renderer());
//
//       const std::string& _bytes = _d.str();
//   }
class DDocumentDialect
{
public:
    // -- public type aliases -------------------------------------------------

    using renderer_type = DDocumentRenderer;
    using str_fn        = std::function<const std::string&()>;
    using clear_fn      = std::function<void()>;

    // -- construction --------------------------------------------------------

    // DDocumentDialect (default)
    //   the INVALID dialect -- what an unserved format yields.
    DDocumentDialect()
        : m_renderer(),
          m_str(),
          m_clear(),
          m_format(DDocumentFormat::text)
    {}

    // DDocumentDialect (bound)
    //   takes ownership of _renderer and the accessors bound to it.  Built by
    // make_document_dialect; a caller wiring a bespoke renderer may use it too.
    DDocumentDialect(
        std::unique_ptr<DDocumentRenderer> _renderer,
        str_fn                             _str,
        clear_fn                           _clear,
        DDocumentFormat                    _format
    )
        : m_renderer(std::move(_renderer)),
          m_str(std::move(_str)),
          m_clear(std::move(_clear)),
          m_format(_format)
    {}

    // -- queries -------------------------------------------------------------

    // valid
    //   whether a renderer was produced.  False for an unserved format; the
    // accessors below are callable only when this is true.
    D_NODISCARD bool valid() const D_NOEXCEPT
    {
        return (m_renderer != nullptr);
    }

    // format
    //   the format this dialect realises.
    D_NODISCARD DDocumentFormat format() const D_NOEXCEPT
    {
        return m_format;
    }

    // -- access --------------------------------------------------------------

    // renderer
    //   the owned renderer, to stream a template into.  Precondition: valid().
    D_NODISCARD DDocumentRenderer& renderer()
    {
        return *m_renderer;
    }

    D_NODISCARD const DDocumentRenderer& renderer() const
    {
        return *m_renderer;
    }

    // str
    //   the bytes rendered so far.  Precondition: valid().
    D_NODISCARD const std::string& str() const
    {
        return m_str();
    }

    // clear
    //   discard what has been rendered, reusing the renderer.  A no-op on an
    // invalid dialect, so a caller need not guard this one.
    void clear()
    {
        if (m_clear)
        {
            m_clear();
        }

        return;
    }

private:
    std::unique_ptr<DDocumentRenderer> m_renderer;
    str_fn                             m_str;
    clear_fn                           m_clear;
    DDocumentFormat                    m_format;
};


// ===========================================================================
// II.  make_document_dialect
// ===========================================================================

NS_INTERNAL

// bind_dialect_helper
//   helper: wrap a concrete renderer as a DDocumentDialect, binding str() and
// clear() while the concrete type is still known.  The captured pointer stays
// valid across a move of the owning unique_ptr, since the renderer is on the
// heap and never relocates.
template<typename Renderer>
D_NODISCARD inline DDocumentDialect bind_dialect_helper(
    DDocumentFormat _format
)
{
    std::unique_ptr<Renderer> _owned(new Renderer());
    Renderer*                 _raw = _owned.get();

    return DDocumentDialect(
        std::unique_ptr<DDocumentRenderer>(_owned.release()),
        [_raw]() -> const std::string& { return _raw->str(); },
        [_raw]() -> void               { _raw->clear();      },
        _format);
}

NS_END  // internal


// make_document_dialect
//   function: the renderer that realises _format, or an invalid dialect when
// the format is not served here (pdf -- opt-in, see the header note -- and the
// not-yet-written tex / wiki).  Check valid() before using the result.
D_NODISCARD inline DDocumentDialect make_document_dialect(
    DDocumentFormat _format
)
{
    switch (_format)
    {
        case DDocumentFormat::markdown:
        {
            return internal::bind_dialect_helper<
                       DMarkdownDocumentRenderer>(_format);
        }

        case DDocumentFormat::xml:
        {
            return internal::bind_dialect_helper<
                       DXmlDocumentRenderer>(_format);
        }

        case DDocumentFormat::html:
        {
            return internal::bind_dialect_helper<
                       DHtmlDocumentRenderer>(_format);
        }

        // pdf -- served by pdf_document_renderer, which this header must not
        // pull in; tex / wiki -- no renderer written yet.
        case DDocumentFormat::pdf:
        case DDocumentFormat::tex:
        case DDocumentFormat::wiki:
        {
            return DDocumentDialect();
        }

        case DDocumentFormat::text:
        {
            return internal::bind_dialect_helper<
                       DPlainDocumentRenderer>(_format);
        }
    }

    return internal::bind_dialect_helper<DPlainDocumentRenderer>(
               DDocumentFormat::text);
}


// ===========================================================================
// III. render_to_string
// ===========================================================================

// render_to_string
//   function: render any document template in any served dialect, as a
// COMPLETE document -- the generalisation of each template's own to_string(),
// which is hard-wired to the plain renderer.  Template is anything with
// `render(DDocumentRenderer&) const` (DTitlePage, DDocumentTable, a caller's
// own).  Yields an empty string for an unserved format, so a caller that does
// not care to branch simply gets nothing rather than undefined behaviour.
//
//   The document frame is opened and closed around the template, because the
// result is meant to stand alone: that is what supplies an HTML dialect its
// shell and an XML one its declaration and root.  A caller streaming SEVERAL
// templates into one document drives the dialect directly instead, calling
// begin_document / end_document once around the set.
template<typename Template>
D_NODISCARD inline std::string render_to_string(
    DDocumentFormat       _format,
    const Template&       _template,
    const DDocAttributes& _frame = DDocAttributes()
)
{
    DDocumentDialect _dialect = make_document_dialect(_format);

    // an unserved format renders to nothing, not to a wrong dialect
    if (!_dialect.valid())
    {
        return std::string();
    }

    _dialect.renderer().begin_document(_frame);

    _template.render(_dialect.renderer());

    _dialect.renderer().end_document();

    return _dialect.str();
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_DOCUMENT_TEMPLATES_DOCUMENT_DIALECT_HPP
