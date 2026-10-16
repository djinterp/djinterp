/*******************************************************************************
* djinterp [ui]                                              ui_tests_export.cpp
*
* Unit tests for SVG export.
*
*
* path:      /tests/djinterp/ui/ui_tests_export.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/
#include "ui_tests.hpp"  // corresponding header


namespace djinterp
{
namespace testing
{

namespace
{

// occurrences
//   how many times _needle appears in _text.
std::size_t
occurrences(
    const std::string& _text,
    const std::string& _needle
)
{
    std::size_t n  = 0;
    std::size_t at = _text.find(_needle);

    while (at != std::string::npos)
    {
        ++n;
        at = _text.find(_needle, at + _needle.size());
    }

    return n;
}

}  // anonymous namespace

/*
tests_svg_export
  draw lists and field panels as SVG documents
  Tests the following:
  - every primitive is written, in paint order, with tidy numbers
  - translucent colours carry an opacity; opaque ones do not
  - text is escaped
  - the background is a rect, or absent when transparent
  - field_svg writes the grid's cells and its labels
*/
bool
tests_svg_export()
{
    ui::draw_list list;

    ui::draw_rect r;
    r.min   = ui::vec<2, double>(0.0, 0.0);
    r.max   = ui::vec<2, double>(12.5, 10.0);
    r.color = ui::rgba(1.0f, 0.0f, 0.0f, 1.0f);
    list.rects.push_back(r);

    ui::draw_quad q;
    q.a     = ui::vec<2, double>(1.0, 1.0);
    q.b     = ui::vec<2, double>(5.0, 1.0);
    q.c     = ui::vec<2, double>(5.0, 5.0);
    q.d     = ui::vec<2, double>(1.0, 5.0);
    q.color = ui::rgba(0.0f, 0.0f, 1.0f, 0.5f);
    list.quads.push_back(q);

    ui::draw_segment seg;
    seg.a         = ui::vec<2, double>(0.0, 0.0);
    seg.b         = ui::vec<2, double>(30.0, 20.0);
    seg.color     = ui::rgba(0.0f, 1.0f, 0.0f, 1.0f);
    seg.thickness = 0.0;
    list.segments.push_back(seg);

    ui::draw_label label;
    label.position = ui::vec<2, double>(2.0, 3.0);
    label.text     = "<a & b>";
    label.color    = ui::rgba(0.0f, 0.0f, 0.0f, 1.0f);
    list.labels.push_back(label);

    const std::string svg = ui::to_svg(list, ui::vec<2, double>(100.0, 50.0));

    if ( (svg.rfind("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\"",
                    0) != 0) ||
         (svg.find("width=\"12.5\" height=\"10\" fill=\"#ff0000\"/>") ==
          std::string::npos) ||
         (svg.find("fill=\"#0000ff\" fill-opacity=\"0.5\"") ==
          std::string::npos) ||
         (svg.find("stroke-width=\"1\"") == std::string::npos) ||
         (svg.find("&lt;a &amp; b&gt;") == std::string::npos) ||
         (svg.find("#ff0000\" fill-opacity") != std::string::npos) ||
         (svg.compare(svg.size() - 7, 7, "</svg>\n") != 0) )
    {
        return false;
    }

    // paint order: the rect before the polygon before the line before text
    if ( (svg.find("<rect x=\"0\" y=\"0\" width=\"12.5\"") >
          svg.find("<polygon")) ||
         (svg.find("<polygon") > svg.find("<line")) ||
         (svg.find("<line") > svg.find("<text")) )
    {
        return false;
    }

    ui::svg_options clear;
    clear.background = ui::rgba(0.0f, 0.0f, 0.0f, 0.0f);

    if ( (occurrences(svg, "<rect") != 2) ||
         (occurrences(ui::to_svg(list, ui::vec<2, double>(100.0, 50.0),
                                 clear),
                      "<rect") != 1) )
    {
        return false;
    }

    // a field panel: background, backdrop, and one rect per visited cell
    ui::visit_source src({ discrete_axis("x", numbered(3)),
                           discrete_axis("y", numbered(2)) },
                         1);
    src.visit({ 0, 0 }, 0);
    src.visit({ 2, 1 }, 0);

    const ui::scene s =
        ui::heatmap_lens().build(src, heatmap_config(), 0).value();
    const std::string panel =
        ui::field_svg(s, ui::vec<2, double>(300.0, 200.0),
                      ui::scale_of(s.field.value()));

    return ( (occurrences(panel, "<rect") == 4) &&
             (occurrences(panel, "<text") >= 5) );
}

}  // namespace testing
}  // namespace djinterp
