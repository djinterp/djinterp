/*******************************************************************************
* djinterp [ui]                                               ui_tests_field.cpp
*
* Unit tests for field picking and field layout in the ui draw layer.
*
*
* path:      /tests/djinterp/ui/ui_tests_field.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/
#include "ui_tests.hpp"  // corresponding header
// std
#include <cstdio>  // std::snprintf


namespace djinterp
{
namespace testing
{

namespace
{

// wide_scene
//   a heatmap scene over 40 positions and 3 rules, one visit per column.
ui::scene
wide_scene()
{
    std::vector<std::string> positions;

    for (int i = 0; i < 40; ++i)
    {
        char label[8];
        std::snprintf(label, sizeof(label), "%03d", i);
        positions.push_back(label);
    }

    ui::visit_source src({ discrete_axis("position", positions),
                           discrete_axis("rule", { "S", "A", "B" }) },
                         1);

    for (std::size_t i = 0; i < 40; ++i)
    {
        src.visit({ i, i % 3 }, 0);
    }

    return ui::heatmap_lens().build(src, heatmap_config(), 0).value();
}

}  // anonymous namespace

/*
tests_field_picking
  points and cells map both ways, with row 0 at the bottom
  Tests the following:
  - field_cell_rect puts row 0 along the bottom edge
  - pick_field of each cell's centre is that cell
  - points on or past the panel's edges pick nothing
*/
bool
tests_field_picking()
{
    ui::scalar_field field;
    field.width  = 5;
    field.height = 3;
    field.values.assign(15, 1.0);

    const ui::vec<2, double> size(100.0, 60.0);

    ui::field_cell bottom_left;
    bottom_left.col = 1;
    bottom_left.row = 0;

    const ui::draw_rect r = ui::field_cell_rect(field, size, bottom_left);

    if ( (!near(r.min[0], 20.0)) || (!near(r.min[1], 40.0)) ||
         (!near(r.max[0], 40.0)) || (!near(r.max[1], 60.0)) )
    {
        return false;
    }

    for (std::uint32_t row = 0; row < 3; ++row)
    {
        for (std::uint32_t col = 0; col < 5; ++col)
        {
            ui::field_cell cell;
            cell.col = col;
            cell.row = row;

            const ui::draw_rect cr = ui::field_cell_rect(field, size, cell);
            const auto hit = ui::pick_field(field, size,
                                            (cr.min + cr.max) * 0.5);

            if ( (!hit.has_value()) ||
                 (hit.value().col != col) ||
                 (hit.value().row != row) )
            {
                return false;
            }
        }
    }

    const auto corner = ui::pick_field(field, size,
                                       ui::vec<2, double>(99.9, 59.9));

    return ( (!ui::pick_field(field, size,
                              ui::vec<2, double>(-1.0, 5.0)).has_value()) &&
             (!ui::pick_field(field, size,
                              ui::vec<2, double>(100.0, 5.0)).has_value()) &&
             (!ui::pick_field(field, size,
                              ui::vec<2, double>(5.0, 60.0)).has_value()) &&
             (corner.has_value()) &&
             (corner.value().col == 4) &&
             (corner.value().row == 0) );
}

/*
tests_field_layout
  a labelled field panel: captions, thinned labels, overlay, hairlines
  Tests the following:
  - captions give the range and the axis names
  - crowded column labels are thinned so neighbours never overlap
  - roomy row labels are all kept
  - overlay marks and trail add their rects and segments
  - hairlines appear between cells only when asked for
  - a scene without a field lays out a notice
*/
bool
tests_field_layout()
{
    const ui::scene        s = wide_scene();
    const ui::field_scale  scale = ui::scale_of(s.field.value());
    const ui::text_metrics text;   // 14px lines, 7px characters
    const ui::vec<2, double> canvas(200.0, 120.0);

    const ui::field_layout plain =
        ui::layout_field(s, canvas, scale, ui::field_style(), text);

    const std::vector<ui::draw_label>& labels = plain.list.labels;

    if ( (labels.size() < 2) ||
         (labels[0].text.rfind("range ", 0) != 0) ||
         (labels[1].text.rfind("x: position", 0) != 0) )
    {
        return false;
    }

    // column labels sit below the grid; row labels to its left
    std::vector<const ui::draw_label*> columns;
    std::size_t                        rows = 0;
    const double grid_bottom = plain.grid_min[1] + plain.grid_size[1];

    for (const ui::draw_label& l : labels)
    {
        if (l.position[1] >= grid_bottom)
        {
            columns.push_back(&l);
        }
        else if ( (l.position[0] < plain.grid_min[0]) &&
                  (l.position[1] >= plain.grid_min[1]) )
        {
            ++rows;
        }
    }

    if ( (columns.size() >= 40) ||
         (columns.size() < 4) ||
         (rows != 3) )
    {
        return false;
    }

    for (std::size_t i = 1; i < columns.size(); ++i)
    {
        const double end = columns[i - 1]->position[0] +
                           text.measure(columns[i - 1]->text);

        if (columns[i]->position[0] < end)
        {
            return false;
        }
    }

    // an overlay: one filled mark, one outlined, a three-cell trail
    ui::field_overlay overlay;
    ui::field_mark    filled;
    ui::field_mark    outlined;

    filled.filled       = true;
    outlined.cell.col   = 1;
    outlined.cell.row   = 1;
    overlay.marks       = { filled, outlined };
    overlay.trail       = { filled.cell,
                            outlined.cell,
                            ui::field_cell{ 2, 2 } };

    const ui::field_layout marked =
        ui::layout_field(s, canvas, scale, ui::field_style(), text, overlay);

    if ( (marked.list.rects.size() != plain.list.rects.size() + 1) ||
         (marked.list.segments.size() != plain.list.segments.size() + 6) )
    {
        return false;
    }

    // hairlines: 39 between the columns, 2 between the rows
    ui::field_style lined;
    lined.grid = ui::rgba(0.0f, 0.0f, 0.0f, 1.0f);

    const ui::field_layout gridded =
        ui::layout_field(s, canvas, scale, lined, text);

    if (gridded.list.segments.size() != plain.list.segments.size() + 41)
    {
        return false;
    }

    const ui::field_layout empty =
        ui::layout_field(ui::scene(), canvas, scale, ui::field_style(), text);

    return ( (empty.list.labels.size() == 1) &&
             (empty.list.rects.empty()) );
}

}  // namespace testing
}  // namespace djinterp
