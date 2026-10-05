/*******************************************************************************
* djinterp [djinterp]                                                    svg.hpp
*
* SVG export.
*   Writes a draw_list as a standalone SVG document -- rectangles, then faces,
* segments and labels, in the order a live frontend paints them -- so any view
* the draw layer produces can be saved to a file without a window. field_svg
* lays a field panel out with layout_field and writes it in one step, measuring
* text as the monospace font the document asks for.
*   Colours become #rrggbb with a separate opacity when not opaque. Labels are
* positioned by their top-left corner, as in the draw layer; the baseline is
* placed from the font size, so no renderer-specific alignment is relied on.
* Text keeps its spaces, as the layout measured them.
*
*
* path:      /inc/djinterp/ui/export/svg.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UI_EXPORT_SVG_HPP
#define DJINTERP_UI_EXPORT_SVG_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstdio>  // std::snprintf
#include <string>  // std::string
// djinterp [ui]
#include "../draw.hpp"   // draw_list, layout_field, field_scale, field_style
#include "../scene.hpp"  // scene
#include "../types.hpp"  // scalar, vec, rgba


NS_DJINTERP
NS_UI

// svg_options
//   struct: document-wide settings. A background with zero alpha is omitted.
struct svg_options
{
    rgba        background  = rgba(1.0f, 1.0f, 1.0f, 1.0f);
    std::string font_family = "monospace";
    scalar      font_size   = 12.0;
    std::string title;
};

NS_INTERNAL

    // svg_number
    //   helper: a coordinate with at most two decimals, trailing zeros cut.
    D_NODISCARD inline std::string
    svg_number
    (
        scalar _v
    )
    {
        char text[32];
        std::snprintf(text, sizeof(text), "%.2f", static_cast<double>(_v));

        std::string out = text;

        // "12.50" -> "12.5", "3.00" -> "3"
        while ( (out.find('.') != std::string::npos) &&
                ( (out.back() == '0') || (out.back() == '.') ) )
        {
            const bool point = (out.back() == '.');
            out.pop_back();

            if (point)
            {
                break;
            }
        }

        return (out == "-0") ? std::string("0") : out;
    }

    // svg_channel
    //   helper: a colour channel as 0-255.
    D_NODISCARD inline int
    svg_channel
    (
        channel_t _c
    )
    {
        const channel_t c = (_c < 0.0f) ? 0.0f : ((_c > 1.0f) ? 1.0f : _c);

        return static_cast<int>((c * 255.0f) + 0.5f);
    }

    // svg_paint
    //   helper: ` <attribute>="#rrggbb"`, plus `<attribute>-opacity` when the
    // colour is not opaque.
    D_NODISCARD inline std::string
    svg_paint
    (
        const char* _attribute,
        const rgba& _c
    )
    {
        char text[64];
        std::snprintf(text, sizeof(text), " %s=\"#%02x%02x%02x\"", _attribute,
                      svg_channel(_c.r), svg_channel(_c.g), svg_channel(_c.b));

        std::string out = text;

        if (_c.a < 1.0f)
        {
            out += std::string(" ") + _attribute + "-opacity=\"" +
                   svg_number(static_cast<scalar>(_c.a)) + "\"";
        }

        return out;
    }

    // svg_escape
    //   helper: text safe inside an SVG element or attribute.
    D_NODISCARD inline std::string
    svg_escape
    (
        const std::string& _text
    )
    {
        std::string out;
        out.reserve(_text.size());

        for (const char c : _text)
        {
            switch (c)
            {
                case '&':  out += "&amp;";  break;
                case '<':  out += "&lt;";   break;
                case '>':  out += "&gt;";   break;
                case '"':  out += "&quot;"; break;
                case '\'': out += "&apos;"; break;
                default:   out += c;        break;
            }
        }

        return out;
    }

NS_END  // internal

// to_svg
//   _list as a complete SVG document of _size pixels.
D_NODISCARD inline std::string
to_svg
(
    const draw_list&      _list,
    const vec<2, scalar>& _size,
    const svg_options&    _options = svg_options()
)
{
    using internal::svg_number;
    using internal::svg_paint;

    std::string out;

    out += "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" +
           svg_number(_size[0]) + "\" height=\"" + svg_number(_size[1]) +
           "\" viewBox=\"0 0 " + svg_number(_size[0]) + " " +
           svg_number(_size[1]) + "\">\n";

    if (!_options.title.empty())
    {
        out += "<title>" + internal::svg_escape(_options.title) +
               "</title>\n";
    }

    // the page, unless it is to stay transparent
    if (_options.background.a > 0.0f)
    {
        out += "<rect x=\"0\" y=\"0\" width=\"" + svg_number(_size[0]) +
               "\" height=\"" + svg_number(_size[1]) + "\"" +
               svg_paint("fill", _options.background) + "/>\n";
    }

    for (const draw_rect& r : _list.rects)
    {
        const scalar w = r.max[0] - r.min[0];
        const scalar h = r.max[1] - r.min[1];

        // a rectangle with no area draws nothing
        if ( (w <= 0.0) ||
             (h <= 0.0) )
        {
            continue;
        }

        out += "<rect x=\"" + svg_number(r.min[0]) + "\" y=\"" +
               svg_number(r.min[1]) + "\" width=\"" + svg_number(w) +
               "\" height=\"" + svg_number(h) + "\"" +
               svg_paint("fill", r.color) + "/>\n";
    }

    for (const draw_quad& q : _list.quads)
    {
        out += "<polygon points=\"" +
               svg_number(q.a[0]) + "," + svg_number(q.a[1]) + " " +
               svg_number(q.b[0]) + "," + svg_number(q.b[1]) + " " +
               svg_number(q.c[0]) + "," + svg_number(q.c[1]) + " " +
               svg_number(q.d[0]) + "," + svg_number(q.d[1]) + "\"" +
               svg_paint("fill", q.color) + "/>\n";
    }

    for (const draw_segment& s : _list.segments)
    {
        const scalar width = (s.thickness > 0.0) ? s.thickness : 1.0;

        out += "<line x1=\"" + svg_number(s.a[0]) + "\" y1=\"" +
               svg_number(s.a[1]) + "\" x2=\"" + svg_number(s.b[0]) +
               "\" y2=\"" + svg_number(s.b[1]) + "\" stroke-width=\"" +
               svg_number(width) + "\" stroke-linecap=\"round\"" +
               svg_paint("stroke", s.color) + "/>\n";
    }

    // labels hang from their top-left corner: the baseline sits most of a
    // font size below it
    const scalar baseline = _options.font_size * 0.9;

    for (const draw_label& l : _list.labels)
    {
        out += "<text x=\"" + svg_number(l.position[0]) + "\" y=\"" +
               svg_number(l.position[1] + baseline) +
               "\" xml:space=\"preserve\" font-family=\"" +
               internal::svg_escape(_options.font_family) +
               "\" font-size=\"" + svg_number(_options.font_size) + "\"" +
               svg_paint("fill", l.color) + ">" +
               internal::svg_escape(l.text) + "</text>\n";
    }

    out += "</svg>\n";

    return out;
}

// svg_text_metrics
//   the text metrics to_svg's font implies: a line is 1.25 font sizes, and a
// monospace character 0.6 of one.
D_NODISCARD inline text_metrics
svg_text_metrics
(
    const svg_options& _options
)
{
    const scalar size = _options.font_size;

    text_metrics out;
    out.line_height = size * 1.25;
    out.width       = [size](const std::string& _text)
    {
        return size * 0.6 * static_cast<scalar>(_text.size());
    };

    return out;
}

// field_svg
//   _scene's field panel (layout_field) as a complete SVG document of _size
// pixels.
D_NODISCARD inline std::string
field_svg
(
    const scene&          _scene,
    const vec<2, scalar>& _size,
    const field_scale&    _scale,
    const field_style&    _style   = field_style(),
    const field_overlay&  _overlay = field_overlay(),
    const svg_options&    _options = svg_options()
)
{
    const field_layout layout = layout_field(_scene,
                                             _size,
                                             _scale,
                                             _style,
                                             svg_text_metrics(_options),
                                             _overlay);

    return to_svg(layout.list, _size, _options);
}

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_EXPORT_SVG_HPP
