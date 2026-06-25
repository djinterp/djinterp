/******************************************************************************
* djinterp [pdf]                                               pdf_printer.hpp
*
*   The PDF analogue of the text `document_printer` / `file_printer`: it takes
* a thing that can render itself to PDF and writes the resulting bytes to a
* sink, so PDF output flows through the SAME sink/file vocabulary as text,
* XML, and HTML output. A PDF is binary, so the "format" is fixed (there is no
* policy as there is for markup) -- the printer's only job is to route
* serialized bytes to a destination.
*
*   WHAT IT PRINTS:
*   Any RENDERABLE exposing `render_pdf() -> std::string` (the serialized
* document, per pdf_template_traits.hpp) -- e.g. a `pdf_template`. This is the
* "serialize an already-built document" path; drawing INTO a live document is
* the writer's job (pdf_document / pdf_canvas), not the printer's, so this
* header deliberately does not depend on the full pdf.hpp foundation.
*
*   SINKS:
*   The same sinks as the text side (writer.hpp): `string_sink`, `stream_sink`,
* or any callable `(const char*, std::size_t)`. `pdf_file_printer` is the PDF
* counterpart of `file_printer` -- a byte sink to a `.pdf` on disk that can
* also `print(renderable)` directly.
*
*   Requires C++17 (matches the writer/printer layer it reuses); self-
* suppresses below it.
*
* path:      /inc/djinterp/core/pdf/pdf_printer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.18
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PDF PRINTER
      -----------
      a. pdf_printer<_Sink>
      b. make_pdf_printer

II.   CONVENIENCE
      -----------
      a. pdf_to_string
      b. pdf_to_stream

III.  FILE PRINTER
      ------------
      a. pdf_file_printer
      b. save_pdf_to_file
*/

#ifndef DJINTERP_PDF_PRINTER_
#define DJINTERP_PDF_PRINTER_ 1

// std
#include <cstddef>
#include <ostream>
#include <fstream>
#include <string>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../text/writer.hpp"          // string_sink, stream_sink (generic sinks)
#include "./pdf_template_traits.hpp"   // has_render_pdf_method, is_pdf_renderable


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP
NS_PDF


///////////////////////////////////////////////////////////////////////////////
///                  I.   PDF PRINTER                                       ///
///////////////////////////////////////////////////////////////////////////////

// pdf_printer
//   class: serialise a PDF renderable to a sink. `_Sink` is any callable
// `(const char*, std::size_t)` (the text side's `string_sink`, `stream_sink`,
// or a user lambda). `print(renderable)` calls `renderable.render_pdf()` and
// writes the resulting bytes to the sink. The renderable is not modified.
template<typename _Sink>
class pdf_printer
{
public:
    using sink_type = _Sink;

    explicit pdf_printer(
        _Sink _sink
    )
        : m_sink(static_cast<_Sink&&>(_sink))
    {}

    // print -- serialise `_renderable` (any type exposing render_pdf()) and
    // write the bytes to the bound sink.
    template<typename _Renderable>
    void
    print(
        const _Renderable& _renderable
    )
    {
        static_assert(
            has_render_pdf_method<clean_t<_Renderable>>::value,
            "pdf_printer::print requires a renderable exposing "
            "render_pdf() -> std::string (the serialized PDF bytes); see "
            "pdf_template_traits.hpp. To draw into a live document instead, "
            "use the writer side (pdf_document / pdf_canvas).");

        const std::string _bytes = _renderable.render_pdf();
        m_sink(_bytes.data(), _bytes.size());

        return;
    }

    D_NODISCARD const _Sink&
    sink() const
    {
        return m_sink;
    }

private:
    _Sink m_sink;
};


// make_pdf_printer
//   function: build a pdf_printer from a sink, decaying the stored sink type.
template<typename _Sink>
D_NODISCARD pdf_printer<typename std::decay<_Sink>::type>
make_pdf_printer(
    _Sink&& _sink
)
{
    return pdf_printer<typename std::decay<_Sink>::type>(
        static_cast<_Sink&&>(_sink));
}


///////////////////////////////////////////////////////////////////////////////
///                  II.   CONVENIENCE                                      ///
///////////////////////////////////////////////////////////////////////////////

// pdf_to_string
//   function: the serialized PDF bytes of `_renderable` as a std::string.
template<typename _Renderable>
D_NODISCARD std::string
pdf_to_string(
    const _Renderable& _renderable
)
{
    std::string _out;
    pdf_printer<string_sink> _printer{ string_sink(_out) };
    _printer.print(_renderable);

    return _out;
}


// pdf_to_stream
//   function: write the serialized PDF bytes of `_renderable` to a stream.
template<typename _Renderable>
void
pdf_to_stream(
    std::ostream&      _stream,
    const _Renderable& _renderable
)
{
    pdf_printer<stream_sink> _printer{ stream_sink(_stream) };
    _printer.print(_renderable);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                  III.   FILE PRINTER                                    ///
///////////////////////////////////////////////////////////////////////////////

// pdf_file_printer
//   class: the PDF counterpart of the text `file_printer` -- a byte sink
// targeting a `.pdf` file, with a convenience to print a renderable straight
// to it. As a sink it is just bytes-to-file (`operator()(const char*,
// std::size_t)`); `print(renderable)` serialises via `render_pdf()`. Opened in
// binary mode so the bytes are written verbatim; check `good()`.
class pdf_file_printer
{
public:
    explicit pdf_file_printer(
        const std::string& _path
    )
        : m_file(_path, std::ios::out | std::ios::binary)
    {}

    // operator() -- the sink face: write raw bytes to the file.
    void
    operator()(
        const char* _data,
        std::size_t _size
    )
    {
        m_file.write(_data, static_cast<std::streamsize>(_size));

        return;
    }

    // print -- serialise a renderable into the file.
    template<typename _Renderable>
    void
    print(
        const _Renderable& _renderable
    )
    {
        static_assert(
            has_render_pdf_method<clean_t<_Renderable>>::value,
            "pdf_file_printer::print requires render_pdf() -> std::string.");

        const std::string _bytes = _renderable.render_pdf();
        m_file.write(_bytes.data(), static_cast<std::streamsize>(_bytes.size()));

        return;
    }

    D_NODISCARD bool
    good() const
    {
        return m_file.good();
    }

private:
    std::ofstream m_file;
};


// save_pdf_to_file
//   function: serialise `_renderable` to the file at `_path`, in one call.
template<typename _Renderable>
inline void
save_pdf_to_file(
    const std::string& _path,
    const _Renderable& _renderable
)
{
    pdf_file_printer _printer(_path);
    _printer.print(_renderable);

    return;
}


NS_END  // pdf
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_PDF_PRINTER_
