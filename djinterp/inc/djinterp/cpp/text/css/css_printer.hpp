/*******************************************************************************
* djinterp [cpp]                                                 css_printer.hpp
*
*   Sink-based output for CSS stylesheets - the CSS analogue of
* pdf_printer / DDocumentPrinter.  A DCssPrinter<Sink> routes a
* stylesheet's rendered CSS to any sink (a callable taking
* `(const char*, std::size_t)`), so stylesheet output flows through
* the same sink abstraction as the rest of the framework.  The
* stylesheet owns the render_to_css / render_to_minified_css /
* render_to_scss methods; this header only forwards their bytes.
*
*   The DCssRenderMode selector chooses which render method a printer
* invokes (pretty / minified / scss), so the same sink path serves all
* three output flavours.
*
*   USAGE:
*     css::DCssStylesheet<css::DCssDefaultBackend> sheet;
*     // ... build rules ...
*     std::string out = css::css_to_string(sheet);                 // pretty
*     std::string min = css::css_to_string(sheet,
*                           css::DCssRenderMode::minified);         // minified
*     css::save_css_to_file("site.css", sheet);                     // to disk
*     css::css_to_stream(std::cout, sheet);                         // to a stream
*
*
* path:      /inc/djinterp/cpp/text/css/css_printer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.24
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    RENDER MODE
      -----------

II.   DCssPrinter<Sink>          (generic sink)
      ------------------------------------------

III.  STRING / STREAM HELPERS
      -----------------------

IV.   FILE PRINTER
      ------------
*/

#ifndef DJINTERP_TEXT_CSS_CSS_PRINTER_HPP
#define DJINTERP_TEXT_CSS_CSS_PRINTER_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../djinterp.hpp"
#include "./css.hpp"            // DCssString, has_render_to_css_method, ...
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint8_t


#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// std
#include <cstddef>
#include <fstream>
#include <ostream>
#include <string>
#include <utility>


NS_DJINTERP

namespace css {


///////////////////////////////////////////////////////////////////////////////
///                I.   RENDER MODE                                         ///
///////////////////////////////////////////////////////////////////////////////

// DCssRenderMode
//   enum: selects which stylesheet render method a printer invokes.
enum class DCssRenderMode : re_std::uint8_t
{
    pretty,     // render_to_css           (formatted)
    minified,   // render_to_minified_css  (whitespace-stripped)
    scss        // render_to_scss          (SCSS-flavoured)
};


namespace internal {

// css_render
//   helper: renders _sheet in the requested mode.  The chosen
// render method must exist on the stylesheet (the default backend
// supplies all three).
template <typename Sheet>
inline
DCssString css_render(
    const Sheet&    _sheet,
    DCssRenderMode  _mode
)
{
    switch (_mode)
    {
        case DCssRenderMode::minified:  return _sheet.render_to_minified_css();
        case DCssRenderMode::scss:      return _sheet.render_to_scss();
        case DCssRenderMode::pretty:
        default:                        return _sheet.render_to_css();
    }
}

}   // namespace internal


///////////////////////////////////////////////////////////////////////////////
///                II.   DCssPrinter<Sink>                                 ///
///////////////////////////////////////////////////////////////////////////////

// DCssPrinter
//   class: writes a stylesheet's rendered CSS to a sink.  Sink is
// any callable invocable as `sink(const char*, std::size_t)` (the
// same sink shape used across the framework's writers).
template <typename Sink>
class DCssPrinter
{
public:
    explicit
    DCssPrinter(
        Sink                _sink,
        DCssRenderMode      _mode = DCssRenderMode::pretty
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
            has_render_to_css_method<clean_t<Sheet>>::value,
            "DCssPrinter::print requires a stylesheet exposing render_to_css().");

        const DCssString out = internal::css_render(_sheet, m_mode);
        m_sink(out.data(), out.size());
    }

    DCssRenderMode  mode() const            { return m_mode; }
    void            set_mode(DCssRenderMode _m) { m_mode = _m; }

private:
    Sink            m_sink;
    DCssRenderMode  m_mode;
};


// make_css_printer
//   function: factory that deduces the sink type.
template <typename Sink>
inline
DCssPrinter<Sink> make_css_printer(
    Sink            _sink,
    DCssRenderMode  _mode = DCssRenderMode::pretty
)
{
    return DCssPrinter<Sink>(std::move(_sink), _mode);
}


///////////////////////////////////////////////////////////////////////////////
///                III.   STRING / STREAM HELPERS                           ///
///////////////////////////////////////////////////////////////////////////////

// css_to_string
//   function: returns the rendered CSS for _sheet in the given mode.
template <typename Sheet>
D_NODISCARD inline DCssString css_to_string(
    const Sheet&    _sheet,
    DCssRenderMode  _mode = DCssRenderMode::pretty
)
{
    static_assert(
        has_render_to_css_method<clean_t<Sheet>>::value,
        "css_to_string requires a stylesheet exposing render_to_css().");

    return internal::css_render(_sheet, _mode);
}


// css_to_stream
//   function: writes the rendered CSS for _sheet to _os.
template <typename Sheet>
inline
void css_to_stream(
    std::ostream&   _os,
    const Sheet&    _sheet,
    DCssRenderMode  _mode = DCssRenderMode::pretty
)
{
    static_assert(
        has_render_to_css_method<clean_t<Sheet>>::value,
        "css_to_stream requires a stylesheet exposing render_to_css().");

    const DCssString out = internal::css_render(_sheet, _mode);
    _os.write(out.data(), static_cast<std::streamsize>(out.size()));
}


///////////////////////////////////////////////////////////////////////////////
///                IV.   FILE PRINTER                                       ///
///////////////////////////////////////////////////////////////////////////////

// DCssFilePrinter
//   class: a sink that writes rendered CSS to a file.  Usable both as
// a standalone printer (`print(sheet)`) and as a raw byte sink
// (`operator()`), mirroring pdf_file_printer.
class DCssFilePrinter
{
public:
    explicit
    DCssFilePrinter(
        const std::string&  _path,
        DCssRenderMode      _mode = DCssRenderMode::pretty
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
            has_render_to_css_method<clean_t<Sheet>>::value,
            "DCssFilePrinter::print requires render_to_css().");

        const DCssString out = internal::css_render(_sheet, m_mode);
        m_file.write(out.data(), static_cast<std::streamsize>(out.size()));
    }

    D_NODISCARD bool good() const { return m_file.good(); }

private:
    std::ofstream   m_file;
    DCssRenderMode  m_mode;
};


// save_css_to_file
//   function: render _sheet and write it to _path; returns whether
// the file stream remained good.
template <typename Sheet>
inline
bool save_css_to_file(
    const std::string& _path,
    const Sheet&       _sheet,
    DCssRenderMode     _mode = DCssRenderMode::pretty
)
{
    DCssFilePrinter p(_path, _mode);

    p.print(_sheet);

    return p.good();
}


}   // namespace css
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_TEXT_CSS_CSS_PRINTER_HPP
