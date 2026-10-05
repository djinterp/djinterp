/*******************************************************************************
* djinterp [djinterp]                                                   draw.hpp
*
*   The framework-agnostic 2D draw layer.  project_scene() takes a scene and a
* camera and flattens the 3D content into a depth-ordered list of 2D line
* segments in screen space: each cell becomes a wireframe cube, and the optimum
* path becomes a heat-coloured polyline drawn on top.  No windowing or drawing
* backend is involved -- any 2D backend (an immediate-mode GUI, an SVG writer,
* a raster canvas) renders the same draw_list.  Cells are depth-sorted back to
* front so translucency composites correctly under a painter's algorithm.
*   For a scene's scalar field, project_field() lays out the coloured grid,
* and layout_field() the whole labelled panel -- captions, tick labels thinned
* to fit, and an overlay of marked cells and a trail through them -- given the
* backend's text metrics. pick_field() and field_cell_rect() map between panel
* points and field cells, so every backend hit-tests the same way.
*
*
* path:      /inc/djinterp/ui/draw.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.18
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_UI_DRAW_HPP
#define DJINTERP_UI_DRAW_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (README
// rule 5); its module's floor is C++11, but math/geometry/geometry_common.hpp,
// which it reaches, needs C++17. The owner's ruling: compile at every level
// first; port down only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>   // std::sort, std::max, std::min
#include <cmath>       // std::ceil, std::floor
#include <cstdio>      // std::snprintf
#include <functional>  // std::function
#include <limits>      // std::numeric_limits
#include <string>      // std::string
#include <utility>     // std::move
#include <vector>      // std::vector
// djinterp
#include "../render/box.hpp"  // render3d::project_box, box_edges, box_faces
// re_std
#include "../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t
// djinterp [ui]
#include "./colormap.hpp"  // colormap
#include "./config.hpp"    // config
#include "./scene.hpp"     // scene, render_cell, scalar_field
#include "./types.hpp"     // scalar, vec, rgba, camera, viewport, ray


NS_DJINTERP
NS_UI

// draw_segment
//   struct: one stroked screen-space line segment with a colour, a stroke
// thickness, and a depth key (larger is farther) used for back-to-front
// ordering.
struct draw_segment
{
    vec<2, scalar> a;
    vec<2, scalar> b;
    rgba           color;
    scalar         thickness = 1.0;
    scalar         depth     = 0.0;
};

// draw_rect
//   struct: one filled, axis-aligned screen-space rectangle with a colour.
// Used for solid fills such as a scalar-field image cell.
struct draw_rect
{
    vec<2, scalar> min;
    vec<2, scalar> max;
    rgba           color;
};

// draw_quad
//   struct: one filled, four-cornered screen-space polygon with a colour and a
// depth key (larger is farther), used to paint translucent shape faces back to
// front.  Corners are given in perimeter order.
struct draw_quad
{
    vec<2, scalar> a;
    vec<2, scalar> b;
    vec<2, scalar> c;
    vec<2, scalar> d;
    rgba           color;
    scalar         depth = 0.0;
};

// draw_label
//   struct: one screen-space text label with a colour.  The agnostic layer
// only positions it -- `position` is the top-left of the text -- and the
// frontend renders the glyphs.
struct draw_label
{
    vec<2, scalar> position;
    std::string    text;
    rgba           color;
};

// draw_list
//   struct: the flattened 2D content of a scene, ready for any 2D backend --
// filled `rects` (backdrop) and `quads` (translucent faces, back to front)
// underneath stroked `segments`, with `labels` (text) drawn last on top.
struct draw_list
{
    std::vector<draw_rect>    rects;
    std::vector<draw_quad>    quads;
    std::vector<draw_segment> segments;
    std::vector<draw_label>   labels;
};

// no_cell
//   sentinel: the absence of a cell, for picking and highlight queries.
constexpr std::size_t no_cell = static_cast<std::size_t>(-1);

NS_INTERNAL

    // project_point
    //   helper: projects a world point to screen space (x, y in pixels, z the
    // depth key).
    inline vec<3, scalar>
    project_point
    (
        const camera<scalar>&   _camera,
        const viewport<scalar>& _viewport,
        const vec<3, scalar>&   _world
    )
    {
        return _camera.project(_world, _viewport);
    }

    // average_color
    //   helper: the midpoint colour of two path vertices, for a segment.
    D_NODISCARD inline rgba
    average_color
    (
        const rgba& _a,
        const rgba& _b
    )
    {
        return lerp(_a, _b, static_cast<channel_t>(0.5));
    }

    // append_axes
    //   helper: appends the scene's axis decorations -- a line along each
    // decorated axis at the minimum of the others, tick marks with scale
    // labels, and the axis name just past the far end.  Straight axes assume
    // the cartesian layout; under a non-cartesian mapper the cells curve but
    // these do not.
    inline void
    append_axes
    (
        draw_list&              _out,
        const scene&            _scene,
        const camera<scalar>&   _camera,
        const viewport<scalar>& _viewport,
        const config&           _config
    )
    {
        if (_scene.axes.empty() || _scene.bounds.is_empty())
        {
            return;
        }

        const vec<3, scalar> lo =
            vec<3, scalar>::from_array(_scene.bounds.m_min);
        const vec<3, scalar> hi =
            vec<3, scalar>::from_array(_scene.bounds.m_max);

        scalar tick = (hi - lo).length() * static_cast<scalar>(0.02);

        rgba line_color  = _config.gridlines.color;
        rgba label_color = rgba(line_color.to_rgb(), static_cast<channel_t>(1));

        for (std::size_t k = 0; k < _scene.axes.size(); ++k)
        {
            const axis_decoration& dec = _scene.axes[k];
            std::size_t d = dec.axis_index;

            if (d > 2)
            {
                continue;
            }

            // the dimension the ticks extend along
            std::size_t perp = (d + 1) % 3;

            // the axis runs along dimension d, at the minimum of the others
            scalar a0 = lo[d];
            scalar a1 = hi[d];

            if (!dec.tick_positions.empty())
            {
                if (dec.tick_positions.front() < a0)
                {
                    a0 = dec.tick_positions.front();
                }

                if (dec.tick_positions.back() > a1)
                {
                    a1 = dec.tick_positions.back();
                }
            }

            // ---- axis line ----
            if (_config.axes.show_lines)
            {
                vec<3, scalar> p0 = lo;  p0[d] = a0;
                vec<3, scalar> p1 = lo;  p1[d] = a1;
                vec<3, scalar> s0 = project_point(_camera, _viewport, p0);
                vec<3, scalar> s1 = project_point(_camera, _viewport, p1);

                draw_segment seg;
                seg.a         = vec<2, scalar>(s0[0], s0[1]);
                seg.b         = vec<2, scalar>(s1[0], s1[1]);
                seg.color     = line_color;
                seg.thickness = _config.gridlines.thickness;
                seg.depth     = static_cast<scalar>(0);
                _out.segments.push_back(seg);
            }

            // ---- tick marks + scale labels ----
            if (_config.axes.show_scales)
            {
                for (std::size_t i = 0; i < dec.tick_positions.size(); ++i)
                {
                    scalar tp = dec.tick_positions[i];
                    vec<3, scalar> t0 = lo;  t0[d]    = tp;
                    vec<3, scalar> t1 = t0;  t1[perp] = lo[perp] - tick;
                    vec<3, scalar> s0 = project_point(_camera, _viewport, t0);
                    vec<3, scalar> s1 = project_point(_camera, _viewport, t1);

                    draw_segment seg;
                    seg.a         = vec<2, scalar>(s0[0], s0[1]);
                    seg.b         = vec<2, scalar>(s1[0], s1[1]);
                    seg.color     = line_color;
                    seg.thickness = _config.gridlines.thickness;
                    seg.depth     = static_cast<scalar>(0);
                    _out.segments.push_back(seg);

                    if (i < dec.tick_labels.size())
                    {
                        // text just beyond the tick mark
                        vec<3, scalar> lp = t0;
                        lp[perp] = lo[perp] - tick * static_cast<scalar>(2);
                        vec<3, scalar> sl =
                            project_point(_camera, _viewport, lp);

                        draw_label lab;
                        lab.position = vec<2, scalar>(sl[0], sl[1]);
                        lab.text     = dec.tick_labels[i];
                        lab.color    = label_color;
                        _out.labels.push_back(static_cast<draw_label&&>(lab));
                    }
                }
            }

            // ---- axis name, past the far end so it clears the last tick ----
            if (_config.axes.show_labels && !dec.label.empty())
            {
                vec<3, scalar> lp = lo;
                lp[d]    = a1 + tick * static_cast<scalar>(3);
                lp[perp] = lo[perp];
                vec<3, scalar> sl = project_point(_camera, _viewport, lp);

                draw_label lab;
                lab.position = vec<2, scalar>(sl[0], sl[1]);
                lab.text     = dec.label;
                lab.color    = label_color;
                _out.labels.push_back(static_cast<draw_label&&>(lab));
            }
        }

        return;
    }

NS_END  // internal

// project_scene
//   flattens _scene into a depth-ordered draw_list using _camera and the pixel
// dimensions in _viewport.  Cells become wireframe cubes (sorted back to
// front); the path is appended last so it draws over the cells.
D_NODISCARD inline draw_list
project_scene
(
    const scene&            _scene,
    const camera<scalar>&   _camera,
    const viewport<scalar>& _viewport,
    const config&           _config,
    std::size_t             _hover    = no_cell,
    std::size_t             _selected = no_cell
)
{
    // depth fade: unmarked cells dim with distance behind the focal point, over
    // a span of gridlines.fade_distance world units (0 disables)
    const vec<3, scalar> eye       = _camera.eye();
    const scalar         focus_d   = _camera.m_distance;
    const scalar         fade_span = _config.gridlines.fade_distance;
    const bool           do_fade   = (fade_span > static_cast<scalar>(0));

    std::vector<draw_segment> cell_segments;
    std::vector<draw_quad>    face_quads;

    for (std::size_t c = 0; c < _scene.cells.size(); ++c)
    {
        const render_cell& cell = _scene.cells[c];
        const scalar       half = cell.size * static_cast<scalar>(0.5);

        // the eight projected corners (and their depths) and the cell's centre
        const render3d::projected_box<scalar> box =
            render3d::project_box(_camera,
                                  _viewport,
                                  cell.position,
                                  vec<3, scalar>(half, half, half));

        vec<2, scalar> corner[8];

        for (std::size_t i = 0; i < 8; ++i)
        {
            corner[i] = vec<2, scalar>(box.m_corners[i][0],
                                       box.m_corners[i][1]);
        }

        scalar depth = box.m_center[2];

        // selection (persistent) outranks hover / lens-highlight (transient),
        // and both outrank the cell's own colour
        rgba color = cell.color;

        if (cell.highlighted || (c == _hover))
        {
            color = _config.interaction.highlight_color;
        }

        if (c == _selected)
        {
            color = _config.interaction.selection_color;
        }

        bool marked = (cell.highlighted || (c == _hover) || (c == _selected));

        if (do_fade && !marked)
        {
            scalar dist = (cell.position - eye).length();

            scalar fade = static_cast<scalar>(1) -
                          (dist - focus_d) / fade_span;

            fade = (fade < static_cast<scalar>(0)) ? static_cast<scalar>(0)
                 : (fade > static_cast<scalar>(1)) ? static_cast<scalar>(1)
                 :                                   fade;

            color.a = static_cast<channel_t>(color.a * fade);
        }

        // translucent faces, behind the wireframe, depth-sorted globally below
        if (_config.shape.filled)
        {
            for (std::size_t f = 0; f < render3d::box_faces.size(); ++f)
            {
                const auto& face = render3d::box_faces[f];

                draw_quad q;
                q.a     = corner[face[0]];
                q.b     = corner[face[1]];
                q.c     = corner[face[2]];
                q.d     = corner[face[3]];
                q.color = color;
                q.depth = render3d::face_depth(box, f);
                face_quads.push_back(q);
            }
        }

        for (const auto& edge : render3d::box_edges)
        {
            draw_segment seg;
            seg.a         = corner[edge[0]];
            seg.b         = corner[edge[1]];
            seg.color     = color;
            seg.thickness = _config.gridlines.thickness;
            seg.depth     = depth;
            cell_segments.push_back(seg);
        }
    }

    // back to front (larger depth first)
    std::sort(cell_segments.begin(),
              cell_segments.end(),
              [](const draw_segment& _x, const draw_segment& _y) -> bool
              {
                  return _x.depth > _y.depth;
              });

    std::sort(face_quads.begin(),
              face_quads.end(),
              [](const draw_quad& _x, const draw_quad& _y) -> bool
              {
                  return _x.depth > _y.depth;
              });

    draw_list out;

    // translucent faces, behind everything else
    out.quads = static_cast<std::vector<draw_quad>&&>(face_quads);

    // reference axes, behind the data
    if (_config.axes.show_lines  ||
        _config.axes.show_scales ||
        _config.axes.show_labels)
    {
        internal::append_axes(out, _scene, _camera, _viewport, _config);
    }

    // the cells, back to front, over the axes
    out.segments.insert(out.segments.end(),
                        cell_segments.begin(),
                        cell_segments.end());

    // the path, drawn on top of every cell
    const render_path& p = _scene.path;

    if (p.nodes.size() >= 2)
    {
        std::vector<vec<2, scalar>> screen(p.nodes.size());

        for (std::size_t i = 0; i < p.nodes.size(); ++i)
        {
            vec<3, scalar> s = internal::project_point(_camera,
                                                       _viewport,
                                                       p.nodes[i].position);
            screen[i] = vec<2, scalar>(s[0], s[1]);
        }

        scalar front = -std::numeric_limits<scalar>::infinity();

        for (std::size_t i = 0; (i + 1) < p.nodes.size(); ++i)
        {
            draw_segment seg;
            seg.a         = screen[i];
            seg.b         = screen[i + 1];
            seg.color     = internal::average_color(p.nodes[i].color,
                                                    p.nodes[i + 1].color);
            seg.thickness = p.thickness;
            seg.depth     = front;
            out.segments.push_back(seg);
        }
    }

    return out;
}

// pick_cell
//   returns the index of the nearest cell whose box the pick ray through the
// screen point _screen enters, or no_cell when the ray misses every cell.  Each
// cell is the axis-aligned box of its rendered cube, so this matches what the
// viewer sees.
D_NODISCARD inline std::size_t
pick_cell
(
    const scene&            _scene,
    const camera<scalar>&   _camera,
    const viewport<scalar>& _viewport,
    const vec<2, scalar>&   _screen
)
{
    const ray<scalar> r = _camera.pick_ray(_screen[0], _screen[1], _viewport);

    std::size_t best   = no_cell;
    scalar      best_t = std::numeric_limits<scalar>::infinity();

    for (std::size_t c = 0; c < _scene.cells.size(); ++c)
    {
        const render_cell& cell = _scene.cells[c];
        scalar half = cell.size * static_cast<scalar>(0.5);

        vec<3, scalar> lo(cell.position[0] - half,
                          cell.position[1] - half,
                          cell.position[2] - half);
        vec<3, scalar> hi(cell.position[0] + half,
                          cell.position[1] + half,
                          cell.position[2] + half);

        scalar tn = static_cast<scalar>(0);
        scalar tf = static_cast<scalar>(0);

        if (math::intersect_aabb(r, to_point(lo), to_point(hi), tn, tf))
        {
            // nearest forward entry (or exit, if the origin is inside the box)
            scalar t = (tn >= static_cast<scalar>(0)) ? tn : tf;

            if ((t >= static_cast<scalar>(0)) && (t < best_t))
            {
                best_t = t;
                best   = c;
            }
        }
    }

    return best;
}

// field_scale
//   struct: how a scalar field's values become colours -- the colormap, and
// the value range [low, high] it spans. Values outside the range take the
// colormap's end colours. Two fields drawn with one scale compare directly.
struct field_scale
{
    ui::colormap colors = ui::colormap::cool_warm();
    scalar       low    = 0.0;
    scalar       high   = 1.0;
};

// scale_of
//   the field's own range, coloured by _colors.
D_NODISCARD inline field_scale
scale_of
(
    const scalar_field& _field,
    ui::colormap        _colors = ui::colormap::cool_warm()
)
{
    field_scale out;
    out.colors = std::move(_colors);
    out.low    = _field.min_value;
    out.high   = _field.max_value;

    return out;
}

// field_cell
//   struct: one cell of a scalar_field, by column (the first display axis)
// and row (the second).
struct field_cell
{
    re_std::uint32_t col = 0;
    re_std::uint32_t row = 0;
};

// project_field
//   lays a scalar_field out as a grid of coloured rectangles filling the panel
// rectangle [0, _size], one cell per sample, coloured by _scale.  Absent
// samples (NaN) are skipped, leaving the backdrop showing through.  Rows are
// flipped vertically so the field's second axis increases upward, plot-style;
// offset the result by the panel origin when drawing (as render_draw_list
// does).
D_NODISCARD inline draw_list
project_field
(
    const scalar_field&   _field,
    const vec<2, scalar>& _size,
    const field_scale&    _scale
)
{
    draw_list out;

    if ((_field.width == 0) || (_field.height == 0))
    {
        return out;
    }

    const scalar span = _scale.high - _scale.low;
    const scalar cw   = _size[0] / static_cast<scalar>(_field.width);
    const scalar ch   = _size[1] / static_cast<scalar>(_field.height);

    out.rects.reserve(static_cast<std::size_t>(_field.width) * _field.height);

    for (re_std::uint32_t row = 0; row < _field.height; ++row)
    {
        // flip vertically: field row 0 sits at the bottom of the panel
        re_std::uint32_t srow = _field.height - 1u - row;

        for (re_std::uint32_t col = 0; col < _field.width; ++col)
        {
            scalar v = _field.values[row * _field.width + col];

            // NaN marks an absent sample
            if (v != v)
            {
                continue;
            }

            scalar t = (span > static_cast<scalar>(0))
                           ? (v - _scale.low) / span
                           : static_cast<scalar>(0.5);

            draw_rect r;
            r.min = vec<2, scalar>(static_cast<scalar>(col)      * cw,
                                   static_cast<scalar>(srow)     * ch);
            r.max = vec<2, scalar>(static_cast<scalar>(col  + 1) * cw,
                                   static_cast<scalar>(srow + 1) * ch);
            r.color = _scale.colors.sample(t, static_cast<scalar>(1));
            out.rects.push_back(r);
        }
    }

    return out;
}

// project_field
//   the same, coloured cool-to-warm over the field's own range.
D_NODISCARD inline draw_list
project_field
(
    const scalar_field&   _field,
    const vec<2, scalar>& _size
)
{
    return project_field(_field, _size, scale_of(_field));
}

// pick_field
//   the cell of _field under _point, in the coordinates project_field uses
// (the panel rectangle [0, _size]), or nothing when the point is outside it.
D_NODISCARD inline maybe<field_cell>
pick_field
(
    const scalar_field&   _field,
    const vec<2, scalar>& _size,
    const vec<2, scalar>& _point
)
{
    if ( (_field.width == 0) || (_field.height == 0) ||
         (_point[0] < static_cast<scalar>(0))        ||
         (_point[1] < static_cast<scalar>(0))        ||
         (_point[0] >= _size[0])                     ||
         (_point[1] >= _size[1]) )
    {
        return nothing<field_cell>();
    }

    const scalar cw = _size[0] / static_cast<scalar>(_field.width);
    const scalar ch = _size[1] / static_cast<scalar>(_field.height);

    const re_std::uint32_t col = std::min(
        _field.width - 1u,
        static_cast<re_std::uint32_t>(std::floor(_point[0] / cw)));
    const re_std::uint32_t srow = std::min(
        _field.height - 1u,
        static_cast<re_std::uint32_t>(std::floor(_point[1] / ch)));

    field_cell out;
    out.col = col;
    out.row = _field.height - 1u - srow;

    return just(out);
}

// field_cell_rect
//   the rectangle project_field gives _cell, in the same coordinates; the
// colour is left transparent for the caller to set.
D_NODISCARD inline draw_rect
field_cell_rect
(
    const scalar_field&   _field,
    const vec<2, scalar>& _size,
    const field_cell&     _cell
)
{
    draw_rect out;
    out.color = rgba(0.0f, 0.0f, 0.0f, 0.0f);

    if ((_field.width == 0) || (_field.height == 0))
    {
        return out;
    }

    const scalar        cw   = _size[0] / static_cast<scalar>(_field.width);
    const scalar        ch   = _size[1] / static_cast<scalar>(_field.height);
    const re_std::uint32_t srow = _field.height - 1u - _cell.row;

    out.min = vec<2, scalar>(static_cast<scalar>(_cell.col) * cw,
                             static_cast<scalar>(srow) * ch);
    out.max = vec<2, scalar>(static_cast<scalar>(_cell.col + 1u) * cw,
                             static_cast<scalar>(srow + 1u) * ch);

    return out;
}

// text_metrics
//   struct: what a layout needs to know about the backend's text -- the
// height of a line and the width of a string. Without a width function, text
// is measured as a monospace font half as wide as a line is tall.
struct text_metrics
{
    scalar                                     line_height = 14.0;
    std::function<scalar(const std::string&)> width;

    D_NODISCARD scalar
    measure(
        const std::string& _text
    ) const
    {
        return width ? width(_text)
                     : (line_height * static_cast<scalar>(0.5) *
                        static_cast<scalar>(_text.size()));
    }
};

// field_style
//   struct: the look of a labelled field panel. `grid` draws hairlines
// between cells when opaque enough to see; `highlight` and `tooltip` are for
// interactive frontends (the hovered cell).
struct field_style
{
    rgba   backdrop    = rgba(0.078f, 0.086f, 0.110f, 1.0f);
    rgba   text        = rgba(0.824f, 0.839f, 0.871f, 1.0f);
    rgba   grid        = rgba(0.0f, 0.0f, 0.0f, 0.0f);
    rgba   highlight   = rgba(1.0f, 0.80f, 0.10f, 1.0f);
    bool   show_range  = true;
    bool   show_titles = true;
    bool   tooltip     = true;
    scalar padding     = 4.0;
};

// field_mark
//   struct: a cell to call out -- outlined, or filled when `filled` is set.
struct field_mark
{
    field_cell cell;
    rgba       color;
    scalar     thickness = 2.0;
    bool       filled    = false;
};

// field_overlay
//   struct: what is drawn over a field -- marked cells, and a trail: a
// polyline through the centres of its cells, in order.
struct field_overlay
{
    std::vector<field_mark> marks;
    std::vector<field_cell> trail;
    rgba                    trail_color     = rgba(0.90f, 0.20f, 0.20f, 1.0f);
    scalar                  trail_thickness = 2.0;
};

// field_layout
//   struct: a laid-out field panel: where the grid went inside the canvas,
// and everything to draw, in canvas coordinates.
struct field_layout
{
    vec<2, scalar> grid_min;
    vec<2, scalar> grid_size;
    draw_list      list;
};

NS_INTERNAL

    // offset_rect
    //   helper: _r moved by _by.
    D_NODISCARD inline draw_rect
    offset_rect
    (
        draw_rect             _r,
        const vec<2, scalar>& _by
    )
    {
        _r.min = _r.min + _by;
        _r.max = _r.max + _by;

        return _r;
    }

    // format_value
    //   helper: a value in six significant digits.
    D_NODISCARD inline std::string
    format_value
    (
        scalar _value
    )
    {
        char text[32];
        std::snprintf(text, sizeof(text), "%.6g", static_cast<double>(_value));

        return text;
    }

    // outline
    //   helper: the four sides of _r as segments.
    inline void
    outline
    (
        draw_list&       _out,
        const draw_rect& _r,
        const rgba&      _color,
        scalar           _thickness
    )
    {
        const vec<2, scalar> corners[4] = {
            _r.min,
            vec<2, scalar>(_r.max[0], _r.min[1]),
            _r.max,
            vec<2, scalar>(_r.min[0], _r.max[1]) };

        for (std::size_t i = 0; i < 4; ++i)
        {
            draw_segment seg;
            seg.a         = corners[i];
            seg.b         = corners[(i + 1) % 4];
            seg.color     = _color;
            seg.thickness = _thickness;
            _out.segments.push_back(seg);
        }

        return;
    }

NS_END  // internal

// layout_field
//   lays out _scene's scalar field as a labelled panel filling _canvas:
// captions (the value range and the axis names) along the top, row labels to
// the left, column labels below, the coloured grid, and _overlay on top.
// Labels are thinned to every n-th when they would collide. The first and
// second axis decorations label the columns and rows, as the heatmap lens
// emits them. A scene without a field lays out a one-line notice.
D_NODISCARD inline field_layout
layout_field
(
    const scene&          _scene,
    const vec<2, scalar>& _canvas,
    const field_scale&    _scale,
    const field_style&    _style,
    const text_metrics&   _text,
    const field_overlay&  _overlay = field_overlay()
)
{
    field_layout out;
    const scalar lh  = _text.line_height;
    const scalar pad = _style.padding;

    // a scene without a field has nothing to lay out
    if (!_scene.field.has_value())
    {
        draw_label notice;
        notice.position = vec<2, scalar>(0.0, 0.0);
        notice.text     = "(this scene has no scalar field)";
        notice.color    = _style.text;
        out.list.labels.push_back(notice);

        return out;
    }

    const scalar_field&    field = _scene.field.value();
    const axis_decoration* xd    = (_scene.axes.size() > 0) ? &_scene.axes[0]
                                                            : nullptr;
    const axis_decoration* yd    = (_scene.axes.size() > 1) ? &_scene.axes[1]
                                                            : nullptr;

    const auto caption = [&out, &_style](scalar _y, const std::string& _t)
    {
        draw_label lab;
        lab.position = vec<2, scalar>(0.0, _y);
        lab.text     = _t;
        lab.color    = _style.text;
        out.list.labels.push_back(lab);

        return;
    };

    // ---- captions along the top ----
    scalar top = 0.0;

    if (_style.show_range)
    {
        caption(top, "range " + internal::format_value(_scale.low) + " to " +
                         internal::format_value(_scale.high));
        top += lh;
    }

    if ( (_style.show_titles) &&
         ( ((xd) && (!xd->label.empty())) ||
           ((yd) && (!yd->label.empty())) ) )
    {
        caption(top, "x: " + (xd ? xd->label : std::string()) + "    y: " +
                         (yd ? yd->label : std::string()));
        top += lh;
    }

    if (top > 0.0)
    {
        top += pad;
    }

    // ---- margins for the tick labels ----
    scalar widest_y = 0.0;
    scalar widest_x = 0.0;

    if (yd)
    {
        for (const std::string& t : yd->tick_labels)
        {
            widest_y = std::max(widest_y, _text.measure(t));
        }
    }

    if (xd)
    {
        for (const std::string& t : xd->tick_labels)
        {
            widest_x = std::max(widest_x, _text.measure(t));
        }
    }

    const scalar left   = (widest_y > 0.0) ? (widest_y + (pad * 2.0)) : 0.0;
    const scalar bottom = (widest_x > 0.0) ? (lh + pad) : 0.0;

    out.grid_min  = vec<2, scalar>(left, top);
    out.grid_size = vec<2, scalar>(std::max(1.0, _canvas[0] - left),
                                   std::max(1.0, _canvas[1] - top - bottom));

    const scalar cw = out.grid_size[0] / static_cast<scalar>(
                          std::max<re_std::uint32_t>(1u, field.width));
    const scalar ch = out.grid_size[1] / static_cast<scalar>(
                          std::max<re_std::uint32_t>(1u, field.height));

    // ---- the grid: backdrop, cells, hairlines ----
    if (_style.backdrop.a > 0.0f)
    {
        draw_rect back;
        back.min   = out.grid_min;
        back.max   = out.grid_min + out.grid_size;
        back.color = _style.backdrop;
        out.list.rects.push_back(back);
    }

    for (const draw_rect& r : project_field(field, out.grid_size,
                                            _scale).rects)
    {
        out.list.rects.push_back(internal::offset_rect(r, out.grid_min));
    }

    // hairlines only where cells are wide enough to separate
    if ( (_style.grid.a > 0.0f) &&
         (cw >= 4.0) &&
         (ch >= 4.0) )
    {
        for (re_std::uint32_t i = 1; i < field.width; ++i)
        {
            draw_segment seg;
            seg.a     = out.grid_min + vec<2, scalar>(cw * i, 0.0);
            seg.b     = out.grid_min + vec<2, scalar>(cw * i,
                                                      out.grid_size[1]);
            seg.color = _style.grid;
            out.list.segments.push_back(seg);
        }

        for (re_std::uint32_t j = 1; j < field.height; ++j)
        {
            draw_segment seg;
            seg.a     = out.grid_min + vec<2, scalar>(0.0, ch * j);
            seg.b     = out.grid_min + vec<2, scalar>(out.grid_size[0],
                                                      ch * j);
            seg.color = _style.grid;
            out.list.segments.push_back(seg);
        }
    }

    // ---- column labels, every n-th if they would collide ----
    if (xd)
    {
        const std::size_t step = std::max<std::size_t>(
            1, static_cast<std::size_t>(std::ceil((widest_x + pad) / cw)));

        for (std::size_t i = 0;
             (i < field.width) && (i < xd->tick_labels.size());
             i += step)
        {
            const std::string& t  = xd->tick_labels[i];
            const scalar       cx = out.grid_min[0] +
                                    ((static_cast<scalar>(i) + 0.5) * cw);

            draw_label lab;
            lab.position = vec<2, scalar>(cx - (_text.measure(t) * 0.5),
                                          out.grid_min[1] + out.grid_size[1] +
                                              (pad * 0.5));
            lab.text     = t;
            lab.color    = _style.text;
            out.list.labels.push_back(lab);
        }
    }

    // ---- row labels, every n-th if they would collide ----
    if (yd)
    {
        const std::size_t step = std::max<std::size_t>(
            1, static_cast<std::size_t>(std::ceil(lh / ch)));

        for (std::size_t j = 0;
             (j < field.height) && (j < yd->tick_labels.size());
             j += step)
        {
            const std::string& t  = yd->tick_labels[j];
            const scalar       cy =
                out.grid_min[1] +
                ((static_cast<scalar>(field.height - 1u - j) + 0.5) * ch);

            draw_label lab;
            lab.position = vec<2, scalar>(left - pad - _text.measure(t),
                                          cy - (lh * 0.5));
            lab.text     = t;
            lab.color    = _style.text;
            out.list.labels.push_back(lab);
        }
    }

    // ---- overlay: marks, then the trail over them ----
    for (const field_mark& m : _overlay.marks)
    {
        if ( (m.cell.col >= field.width) ||
             (m.cell.row >= field.height) )
        {
            continue;
        }

        draw_rect r = internal::offset_rect(
            field_cell_rect(field, out.grid_size, m.cell), out.grid_min);

        if (m.filled)
        {
            r.color = m.color;
            out.list.rects.push_back(r);
        }
        else
        {
            internal::outline(out.list, r, m.color, m.thickness);
        }
    }

    for (std::size_t i = 1; i < _overlay.trail.size(); ++i)
    {
        const field_cell& a = _overlay.trail[i - 1];
        const field_cell& b = _overlay.trail[i];
        const draw_rect   ra = field_cell_rect(field, out.grid_size, a);
        const draw_rect   rb = field_cell_rect(field, out.grid_size, b);

        draw_segment seg;
        seg.a         = out.grid_min + ((ra.min + ra.max) * 0.5);
        seg.b         = out.grid_min + ((rb.min + rb.max) * 0.5);
        seg.color     = _overlay.trail_color;
        seg.thickness = _overlay.trail_thickness;
        out.list.segments.push_back(seg);
    }

    return out;
}

NS_END  // ui
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_UI_DRAW_HPP
