/*******************************************************************************
* vparse [vparse]                                                       heat.hpp
*
* Where a run spent its work: the run as a djinterp::ui visit space.
*   Each trace step happens at an input position, inside some rule's code. The
* heat space counts those steps per (position, rule) cell over time, in a
* ui::visit_source the heatmap lens draws; a second source counts arrivals --
* the times the parser came to a cell from somewhere else -- which is the
* direct measure of re-parsing. Cells visited over and over are work a packrat
* memo table would save.
*   Columns are input positions, grouped into ranges when the input is long so
* a run of any length fits; rows are rules, the start rule on top. The call
* stack at any step is kept as parent links, so the derivation in progress --
* the trail drawn over the map -- costs its depth to recover, not a replay.
*
* path:      /inc/djinterp/tools/vparse/heat.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE HEAT SPACE
    --------------
    1.  heat_space
2.  OPERATIONS
    ----------
    1.  Building
         1.  build_heat
    2.  Drawing
         1.  heat_colors
         2.  heat_scene
         3.  heat_overlay
         4.  heat_svg
    3.  Reporting
         1.  heat_listing
         2.  heat_summary
*/

#ifndef DJINTERP_TOOLS_VPARSE_HEAT_HPP
#define DJINTERP_TOOLS_VPARSE_HEAT_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error, as the
// djinterp ui headers it stands on are.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "djinterp/tools/ui/draw.hpp"                  // field_overlay, field_cell
#include "djinterp/tools/ui/sources/visit_source.hpp"  // visit_source
// vparse
#include "./run.hpp"  // run_record, listing


namespace vparse
{
namespace workbench
{

namespace ui = ::djinterp::ui;


//==============================================================================
// 1.  THE HEAT SPACE
//==============================================================================


// 1.1    heat_space
//------------------------------------------------------------------------------
// 1.1.1
// heat_space
//   struct: a run's steps and arrivals per (column, row), the call stack at
// every step, and totals over the whole run. Row 0 is the last rule, so the
// start rule is drawn at the top.
struct heat_space
{
    // frame
    //   struct: one call on the stack -- the called rule's row, the column it
    // started at, and the frame below it (-1 at the bottom).
    struct frame
    {
        int row;
        int col;
        int parent;
        int depth;
    };

    heat_space(
        std::vector<ui::axis> _axes,
        std::size_t           _steps
    )
        : steps(_axes, _steps),
          entries(_axes, _steps)
    {}

    std::size_t              bucket = 1;
    std::vector<std::string> columns;
    std::vector<std::string> rows;
    ui::visit_source         steps;
    ui::visit_source         entries;

    // per trace step: the cell's column and row, -1 for the end-of-input gate
    std::vector<int>         step_col;
    std::vector<int>         step_row;

    // per trace step: the innermost open call as it runs, -1 for none
    std::vector<int>         step_top;
    std::vector<frame>       frames;

    std::size_t              touched     = 0;
    std::size_t              revisits    = 0;
    std::size_t              max_steps   = 0;
    std::size_t              max_entries = 0;
    int                      hot_col     = -1;
    int                      hot_row     = -1;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Building
//------------------------------------------------------------------------------
// 2.1.1
// build_heat
//   function: the heat space of _run's recognition pass, from its trace. A run
// with no trace, or that did not run, gives an empty space.
heat_space build_heat(
    const run_record& _run
);


// 2.2    Drawing
//------------------------------------------------------------------------------
// 2.2.1
// heat_colors
//   function: the heat ramp on the workbench's paper -- faint, then
// highlighter yellow, then orange, then the ribbon's red.
ui::colormap heat_colors();

// 2.2.2
// heat_scene
//   function: the heatmap lens's scene of _space at trace step _step, counting
// arrivals when _entries is set and steps otherwise.
ui::scene heat_scene(
    const heat_space& _space,
    bool              _entries,
    std::size_t       _step
);

// 2.2.3
// heat_overlay
//   function: what to draw over the map at _step: the call stack as a trail,
// the machine's cell, and -- for a failed run -- where it got furthest.
ui::field_overlay heat_overlay(
    const heat_space& _space,
    const run_record& _run,
    std::size_t       _step
);

// 2.2.4
// heat_svg
//   function: the whole run's arrivals map as an SVG document, with its range
// and axes captioned and, for a failed run, the frontier marked. The machine's
// position is not drawn: at the end of a run it summarizes nothing.
std::string heat_svg(
    const heat_space& _space,
    const run_record& _run
);


// 2.3    Reporting
//------------------------------------------------------------------------------
// 2.3.1
// heat_listing
//   function: every cell the run touched, most-arrived-at first: the rule,
// where, the text there, arrivals and steps. This is the saved heat part.
listing heat_listing(
    const heat_space& _space,
    const run_record& _run
);

// 2.3.2
// heat_summary
//   function: one line on repeated work, for the run's summary.
std::string heat_summary(
    const heat_space& _space
);

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now

#endif  // DJINTERP_TOOLS_VPARSE_HEAT_HPP
