/*******************************************************************************
* djinterp [ui]                                        ui_tests_visit_source.cpp
*
* Unit tests for ui::visit_source, alone and under the heatmap lens.
*
*
* path:      /tests/djinterp/ui/ui_tests_visit_source.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.09.23
*******************************************************************************/
#include "ui_tests.hpp"  // corresponding header


namespace djinterp
{
namespace testing
{

/*
tests_visit_source_counts
  counting visits by cell and frame
  Tests the following:
  - counts grow with the frame, including a visit recorded out of order
  - cells outside the space, or of the wrong arity, are refused
  - total, first_visit and cell_count
  - value_at is the count, and nothing for unvisited cells or level > 0
*/
bool
tests_visit_source_counts()
{
    ui::visit_source src({ discrete_axis("x", { "a", "b", "c" }),
                           discrete_axis("y", { "p", "q" }) },
                         10);

    const std::vector<std::size_t> origin = { 0, 0 };

    if ( (!src.visit(origin, 1)) ||
         (!src.visit(origin, 3)) ||
         (!src.visit({ 2, 1 }, 2)) ||
         (!src.visit(origin, 2)) ||
         (!src.visit({ 1, 1 }, 9)) )
    {
        return false;
    }

    // outside the space: an index past the labels, or the wrong arity
    if ( (src.visit({ 3, 0 }, 4)) ||
         (src.visit({ 0 }, 4)) )
    {
        return false;
    }

    if ( (src.count(origin, 0) != 0) ||
         (src.count(origin, 1) != 1) ||
         (src.count(origin, 2) != 2) ||
         (src.count(origin, 3) != 3) ||
         (src.count(origin, 9) != 3) ||
         (src.total(origin) != 3) ||
         (src.total({ 1, 0 }) != 0) )
    {
        return false;
    }

    if ( (!src.first_visit(origin).has_value()) ||
         (src.first_visit(origin).value() != 1) ||
         (src.first_visit({ 1, 0 }).has_value()) )
    {
        return false;
    }

    ui::cell_address at;
    at.indices = origin;

    ui::cell_address unvisited;
    unvisited.indices = { 1, 0 };

    ui::cell_address coarse;
    coarse.level   = 1;
    coarse.indices = origin;

    return ( (src.cell_count() == 6) &&
             (src.axis_count() == 2) &&
             (src.frame_count() == 10) &&
             (src.value_at(at, 2).has_value()) &&
             (near(src.value_at(at, 2).value(), 2.0)) &&
             (!src.value_at(unvisited, 9).has_value()) &&
             (!src.value_at(coarse, 9).has_value()) );
}

/*
tests_visit_source_paths
  the path at a frame comes from the caller, and is kept between asks
  Tests the following:
  - the path function's result is returned for the frame asked for
  - asking again for the same frame does not call the function again
  - a new frame calls it again; no function gives an empty path
*/
bool
tests_visit_source_paths()
{
    int calls = 0;

    ui::visit_source src({ discrete_axis("x", numbered(4)) }, 8,
                         [&calls](std::size_t _frame)
                         {
                             ++calls;

                             ui::path p;
                             ui::path_node n;
                             n.coords = { static_cast<double>(_frame % 4) };
                             p.nodes.push_back(n);

                             return p;
                         });

    const ui::path& a = src.current_path(3);

    if ( (a.nodes.size() != 1) ||
         (!near(a.nodes[0].coords[0], 3.0)) )
    {
        return false;
    }

    (void)src.current_path(3);

    if (calls != 1)
    {
        return false;
    }

    (void)src.current_path(6);

    const ui::visit_source bare({ discrete_axis("x", numbered(2)) }, 2);

    return ( (calls == 2) &&
             (bare.current_path(1).nodes.empty()) );
}

/*
tests_heatmap_on_visits
  the heatmap lens over a visit_source draws the counts as of a frame
  Tests the following:
  - the field is one cell per label on each axis
  - unvisited cells are absent (NaN); visited cells hold their counts
  - the range is the counts' minimum and maximum at that frame
  - the axis decorations carry the labels; the path comes through
  - a frame past the end is refused
*/
bool
tests_heatmap_on_visits()
{
    ui::visit_source src({ discrete_axis("position", numbered(4)),
                           discrete_axis("rule", { "S", "A", "B" }) },
                         6,
                         [](std::size_t)
                         {
                             ui::path p;
                             ui::path_node n;
                             n.coords = { 1.0, 2.0 };
                             p.nodes.push_back(n);
                             p.nodes.push_back(n);

                             return p;
                         });

    // S at 0 three times by frame 2; A at 1 once; B at 3 at frame 5 only
    src.visit({ 0, 0 }, 0);
    src.visit({ 0, 0 }, 1);
    src.visit({ 0, 0 }, 2);
    src.visit({ 1, 1 }, 1);
    src.visit({ 3, 2 }, 5);

    const ui::heatmap_lens lens;
    const auto built = lens.build(src, heatmap_config(), 2);

    if (!built.is_ok())
    {
        return false;
    }

    const ui::scene        s = built.value();
    const ui::scalar_field f = s.field.value();

    const double v_s = f.values[(0 * f.width) + 0];
    const double v_a = f.values[(1 * f.width) + 1];
    const double v_b = f.values[(2 * f.width) + 3];

    if ( (f.width != 4) || (f.height != 3) ||
         (!near(v_s, 3.0)) || (!near(v_a, 1.0)) ||
         (v_b == v_b) ||
         (!near(f.min_value, 1.0)) || (!near(f.max_value, 3.0)) ||
         (s.cells.size() != 2) )
    {
        return false;
    }

    if ( (s.axes.size() != 2) ||
         (s.axes[1].tick_labels.size() != 3) ||
         (s.axes[1].tick_labels[2] != "B") ||
         (s.path.nodes.size() != 2) )
    {
        return false;
    }

    return (!lens.build(src, heatmap_config(), 6).is_ok());
}

}  // namespace testing
}  // namespace djinterp
