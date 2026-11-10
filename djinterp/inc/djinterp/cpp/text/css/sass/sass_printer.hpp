/*******************************************************************************
* djinterp [cpp]                                                sass_printer.hpp
*
*   Sink-based output for Sass / SCSS stylesheets - the Sass analogue
* of DCssPrinter / pdf_printer.  A DSassPrinter<Sink> routes a
* stylesheet's rendered source to any sink (a callable taking
* `(const char*, std::size_t)`), so Sass output flows through the same
* sink abstraction as the rest of the framework.
*
*   The DSassRenderMode selector chooses which render target a printer
* invokes:
*     - scss          -> render_to_scss_source()  (SCSS source)
*     - indented      -> render_to_sass_source()  (indented .sass source)
*     - compiled_css  -> compile_to_css(os)        (compiled CSS)
*
*   (compile_to_css exposes only the stream form, so the compiled_css
* mode buffers through an ostringstream.)
*
*   USAGE:
*     sass::DSassStylesheet<sass::DSassDefaultBackend> sheet;
*     // ... build variables / rules ...
*     std::string scss = sass::sass_to_string(sheet);                  // SCSS
*     std::string css  = sass::sass_to_string(sheet,
*                            sass::DSassRenderMode::compiled_css);      // CSS
*     sass::save_sass_to_file("site.scss", sheet);                      // to disk
*     sass::sass_to_stream(std::cout, sheet);                           // to stream
*
*
* path:      /inc/djinterp/cpp/text/css/sass/sass_printer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.24
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    RENDER MODE
      -----------

II.   DSassPrinter<Sink>          (generic sink)
      -------------------------------------------

III.  STRING / STREAM HELPERS
      -----------------------

IV.   FILE PRINTER
      ------------
*/

#ifndef DJINTERP_TEXT_CSS_SASS_SASS_PRINTER_HPP
#define DJINTERP_TEXT_CSS_SASS_SASS_PRINTER_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "./sass.hpp"           // DSassString, has_render_to_scss_source_method, ...
// re_std
#include "../../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint8_t


#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// std
#include <cstddef>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>


NS_DJINTERP

namespace sass {


///////////////////////////////////////////////////////////////////////////////
///                I.   RENDER MODE                                         ///
///////////////////////////////////////////////////////////////////////////////

// DSassRenderMode
//   enum: selects which stylesheet render target a printer invokes.
enum class DSassRenderMode : re_std::uint8_t
{
    scss,           // render_to_scss_source   (SCSS source)
    indented,       // render_to_sass_source   (indented .sass source)
    compiled_css    // compile_to_css          (compiled CSS)
};


namespace internal {

// sass_render
//   helper: renders _sheet in the requested mode.  The chosen render
// target must exist on the stylesheet (the default backend supplies
// all three).
template <typename Sheet>
inline
DSassString sass_render(
    const Sheet&     _sheet,
    DSassRenderMode  _mode
)
{
    switch (_mode)
    {
        case DSassRenderMode::indented:
            return _sheet.render_to_sass_source();

        case DSassRenderMode::compiled_css:
        {
            std::ostringstream oss;
            _sheet.compile_to_css(oss);
            return oss.str();
        }

        case DSassRenderMode::scss:
        default:
            return _sheet.render_to_scss_source();
    }
}

}   // namespace internal


///////////////////////////////////////////////////////////////////////////////
///                II.   DSassPrinter<Sink>                                ///
///////////////////////////////////////////////////////////////////////////////

// DSassPrinter
//   class: writes a stylesheet's rendered source to a sink.  Sink is
// any callable invocable as `sink(const char*, std::size_t)`.
template <typename Sink>
class DSassPrinter
{
public:
    explicit
    DSassPrinter(
        Sink                _sink,
        DSassRenderMode     _mode = DSassRenderMode::scss
    )
    :   m_sink(std::move(_sink)),
        m_mode(_mode)
    {}

    // print
    //   function: render _sheet and push the bytes to the sink.
    template <typename Sheet>
    void print(
        const Sheet& _sheet
    )
    {
        static_assert(
            has_render_to_scss_source_method<clean_t<Sheet>>::value,
            "DSassPrinter::print requires a stylesheet exposing "
            "render_to_scss_source().");

        const DSassString out = internal::sass_render(_sheet, m_mode);
        m_sink(out.data(), out.size());
    }

    DSassRenderMode  mode() const                  { return m_mode; }
    void             set_mode(DSassRenderMode _m) { m_mode = _m; }

private:
    Sink             m_sink;
    DSassRenderMode  m_mode;
};


// make_sass_printer
//   function: factory that deduces the sink type.
template <typename Sink>
inline
DSassPrinter<Sink> make_sass_printer(
    Sink             _sink,
    DSassRenderMode  _mode = DSassRenderMode::scss
)
{
    return DSassPrinter<Sink>(std::move(_sink), _mode);
}


///////////////////////////////////////////////////////////////////////////////
///                III.   STRING / STREAM HELPERS                           ///
///////////////////////////////////////////////////////////////////////////////

// sass_to_string
//   function: returns the rendered source for _sheet in the given mode.
template <typename Sheet>
D_NODISCARD inline DSassString sass_to_string(
    const Sheet&     _sheet,
    DSassRenderMode  _mode = DSassRenderMode::scss
)
{
    static_assert(
        has_render_to_scss_source_method<clean_t<Sheet>>::value,
        "sass_to_string requires a stylesheet exposing "
        "render_to_scss_source().");

    return internal::sass_render(_sheet, _mode);
}


// sass_to_stream
//   function: writes the rendered source for _sheet to _os.
template <typename Sheet>
inline
void sass_to_stream(
    std::ostream&    _os,
    const Sheet&     _sheet,
    DSassRenderMode  _mode = DSassRenderMode::scss
)
{
    static_assert(
        has_render_to_scss_source_method<clean_t<Sheet>>::value,
        "sass_to_stream requires a stylesheet exposing "
        "render_to_scss_source().");

    const DSassString out = internal::sass_render(_sheet, _mode);
    _os.write(out.data(), static_cast<std::streamsize>(out.size()));
}


///////////////////////////////////////////////////////////////////////////////
///                IV.   FILE PRINTER                                       ///
///////////////////////////////////////////////////////////////////////////////

// DSassFilePrinter
//   class: a sink that writes rendered Sass/SCSS to a file.  Usable
// both as a standalone printer (`print(sheet)`) and as a raw byte sink
// (`operator()`), mirroring DCssFilePrinter / pdf_file_printer.
class DSassFilePrinter
{
public:
    explicit
    DSassFilePrinter(
        const std::string&  _path,
        DSassRenderMode     _mode = DSassRenderMode::scss
    )
    :   m_file(_path.c_str(), std::ios::out | std::ios::binary),
        m_mode(_mode)
    {}

    // operator() -- the sink face: write raw bytes to the file.
    void operator()(
        const char* _data,
        std::size_t _size
    )
    {
        m_file.write(_data, static_cast<std::streamsize>(_size));
    }

    // print -- render a stylesheet into the file.
    template <typename Sheet>
    void print(
        const Sheet& _sheet
    )
    {
        static_assert(
            has_render_to_scss_source_method<clean_t<Sheet>>::value,
            "DSassFilePrinter::print requires render_to_scss_source().");

        const DSassString out = internal::sass_render(_sheet, m_mode);
        m_file.write(out.data(), static_cast<std::streamsize>(out.size()));
    }

    D_NODISCARD bool good() const { return m_file.good(); }

private:
    std::ofstream    m_file;
    DSassRenderMode  m_mode;
};


// save_sass_to_file
//   function: render _sheet and write it to _path; returns whether the
// file stream remained good.
template <typename Sheet>
inline
bool save_sass_to_file(
    const std::string& _path,
    const Sheet&       _sheet,
    DSassRenderMode    _mode = DSassRenderMode::scss
)
{
    DSassFilePrinter p(_path, _mode);

    p.print(_sheet);

    return p.good();
}


}   // namespace sass
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_TEXT_CSS_SASS_SASS_PRINTER_HPP
