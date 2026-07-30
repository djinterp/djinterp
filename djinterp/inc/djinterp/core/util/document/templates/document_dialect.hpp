/******************************************************************************
* djinterp [utility]                                        document_dialect.hpp
*
*   The join between the format SELECTOR and the dialect set: given a
* `document_format`, hand back the document_renderer that realises it, together
* with the one thing the abstract interface cannot expose -- how to get the
* bytes back out.  This is what turns "the configuration asked for html" into a
* renderer a template can stream into, without every call site growing its own
* switch over the format enum.
*
*   WHY A WRAPPER AND NOT A BARE unique_ptr.  `document_renderer` is a write-only
* semantic sink: it has no `str()`, deliberately, because a PDF back end has no
* string to give and a streaming back end may have written its bytes away
* already.  The concrete string dialects all DO have one, so document_dialect
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
* TABLE OF CONTENTS
* =================
* I.    document_dialect                (the owning wrapper)
* II.   make_document_dialect           (the factory)
* III.  render_to_string                (render any template in any dialect)
*
*
* path:      /inc/djinterp/core/util/document/templates/document_dialect.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.23
******************************************************************************/

#ifndef DJINTERP_UTIL_DOCUMENT_DIALECT_
#define DJINTERP_UTIL_DOCUMENT_DIALECT_ 1

// std
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"                // NS_*, D_NODISCARD, D_NOEXCEPT
#include "../document_format.hpp"               // document_format, format_is_*
#include "./document_renderer.hpp"              // document_renderer,
                                                // plain_document_renderer
#include "./markdown_document_renderer.hpp"     // markdown_document_renderer
#include "./markup_document_renderer.hpp"       // xml_ / html_document_renderer


NS_DJINTERP


// ===========================================================================
// I.   document_dialect
// ===========================================================================

// document_dialect
//   class: an owned document_renderer plus the accessors its concrete type
// provides -- the value make_document_dialect hands back.  Move-only; the
// renderer lives on the heap, so the bound accessors stay valid across a move.
//
// Usage:
//   document_dialect _d = make_document_dialect(document_format::html);
//
//   if (_d.valid())
//   {
//       _table.render(_d.renderer());
//
//       const std::string& _bytes = _d.str();
//   }
class document_dialect
{
public:
    // -- public type aliases -------------------------------------------------

    using renderer_type = document_renderer;
    using str_fn        = std::function<const std::string&()>;
    using clear_fn      = std::function<void()>;

    // -- construction --------------------------------------------------------

    // document_dialect (default)
    //   the INVALID dialect -- what an unserved format yields.
    document_dialect()
        : m_renderer(),
          m_str(),
          m_clear(),
          m_format(document_format::text)
    {}

    // document_dialect (bound)
    //   takes ownership of _renderer and the accessors bound to it.  Built by
    // make_document_dialect; a caller wiring a bespoke renderer may use it too.
    document_dialect(
        std::unique_ptr<document_renderer> _renderer,
        str_fn                             _str,
        clear_fn                           _clear,
        document_format                    _format
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
    D_NODISCARD bool
    valid() const D_NOEXCEPT
    {
        return (m_renderer != nullptr);
    }

    // format
    //   the format this dialect realises.
    D_NODISCARD document_format
    format() const D_NOEXCEPT
    {
        return m_format;
    }

    // -- access --------------------------------------------------------------

    // renderer
    //   the owned renderer, to stream a template into.  Precondition: valid().
    D_NODISCARD document_renderer&
    renderer()
    {
        return *m_renderer;
    }

    D_NODISCARD const document_renderer&
    renderer() const
    {
        return *m_renderer;
    }

    // str
    //   the bytes rendered so far.  Precondition: valid().
    D_NODISCARD const std::string&
    str() const
    {
        return m_str();
    }

    // clear
    //   discard what has been rendered, reusing the renderer.  A no-op on an
    // invalid dialect, so a caller need not guard this one.
    void
    clear()
    {
        if (m_clear)
        {
            m_clear();
        }

        return;
    }

private:
    std::unique_ptr<document_renderer> m_renderer;
    str_fn                             m_str;
    clear_fn                           m_clear;
    document_format                    m_format;
};


// ===========================================================================
// II.  make_document_dialect
// ===========================================================================

NS_INTERNAL

// bind_dialect_helper
//   helper: wrap a concrete renderer as a document_dialect, binding str() and
// clear() while the concrete type is still known.  The captured pointer stays
// valid across a move of the owning unique_ptr, since the renderer is on the
// heap and never relocates.
template<typename _Renderer>
D_NODISCARD inline document_dialect
bind_dialect_helper(
    document_format _format
)
{
    std::unique_ptr<_Renderer> _owned(new _Renderer());
    _Renderer*                 _raw = _owned.get();

    return document_dialect(
        std::unique_ptr<document_renderer>(_owned.release()),
        [_raw]() -> const std::string& { return _raw->str(); },
        [_raw]() -> void               { _raw->clear();      },
        _format);
}

NS_END  // internal


// make_document_dialect
//   function: the renderer that realises _format, or an invalid dialect when
// the format is not served here (pdf -- opt-in, see the header note -- and the
// not-yet-written tex / wiki).  Check valid() before using the result.
D_NODISCARD inline document_dialect
make_document_dialect(
    document_format _format
)
{
    switch (_format)
    {
        case document_format::markdown:
        {
            return internal::bind_dialect_helper<
                       markdown_document_renderer>(_format);
        }

        case document_format::xml:
        {
            return internal::bind_dialect_helper<
                       xml_document_renderer>(_format);
        }

        case document_format::html:
        {
            return internal::bind_dialect_helper<
                       html_document_renderer>(_format);
        }

        // pdf -- served by pdf_document_renderer, which this header must not
        // pull in; tex / wiki -- no renderer written yet.
        case document_format::pdf:
        case document_format::tex:
        case document_format::wiki:
        {
            return document_dialect();
        }

        case document_format::text:
        {
            return internal::bind_dialect_helper<
                       plain_document_renderer>(_format);
        }
    }

    return internal::bind_dialect_helper<plain_document_renderer>(
               document_format::text);
}


// ===========================================================================
// III. render_to_string
// ===========================================================================

// render_to_string
//   function: render any document template in any served dialect, as a
// COMPLETE document -- the generalisation of each template's own to_string(),
// which is hard-wired to the plain renderer.  _Template is anything with
// `render(document_renderer&) const` (title_page, document_table, a caller's
// own).  Yields an empty string for an unserved format, so a caller that does
// not care to branch simply gets nothing rather than undefined behaviour.
//
//   The document frame is opened and closed around the template, because the
// result is meant to stand alone: that is what supplies an HTML dialect its
// shell and an XML one its declaration and root.  A caller streaming SEVERAL
// templates into one document drives the dialect directly instead, calling
// begin_document / end_document once around the set.
template<typename _Template>
D_NODISCARD inline std::string
render_to_string(
    document_format       _format,
    const _Template&      _template,
    const doc_attributes& _frame = doc_attributes()
)
{
    document_dialect _dialect = make_document_dialect(_format);

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


#endif  // DJINTERP_UTIL_DOCUMENT_DIALECT_
