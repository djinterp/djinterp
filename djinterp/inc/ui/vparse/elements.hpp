/*******************************************************************************
* vparse [ui]                                                       elements.hpp
*
* The workbench's own elements, and how they draw under ImGui.
*   Two element types that no general UI kit would carry:
*
*     listing   one part of a run as a table, or two runs' parts side by side
*               with the rows that differ marked. Long listings are drawn a
*               screenful at a time.
*     tape      a run's text as a strip of character cells, with the rules
*               that matched drawn beneath it as nested spans, the machine's
*               cursor at the current trace step, and a scrubber to move it.
*     heat      where a run spent its work: a heatmap of the parser's
*               arrivals (or steps) at each input position and rule, as of
*               the current step, with the call stack drawn over it.
*
*   Each is a uxoxo element module -- a descriptor and a builder -- plus an
* ImGui renderer registered with set_renderer, which is how uxoxo gives an
* element type native drawing on one platform. The renderers read the runs
* through a finder installed once by the host.
*   The theme lives here too, so every colour the workbench draws with is
* defined in one place: listing paper, green bars, ribbon ink, a highlighter
* for the current step and red for what went wrong.
*
* path:      /inc/ui/vparse/elements.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  ELEMENTS
    --------
    1.  listing_view
    2.  tape_view
    3.  heat_view
2.  THE IMGUI PAIRING
    -----------------
    1.  run_finder
    2.  install_renderers
    3.  apply_theme
*/

#ifndef VPARSE_UI_ELEMENTS_HPP
#define VPARSE_UI_ELEMENTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error, as the
// uxoxo headers it stands on would not compile there.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <functional>  // std::function
#include <string>      // std::string
// uxoxo
#include "uxoxo/element.hpp"  // element_template, element_type
// vparse
#include "../../djinterp/tools/vparse/run.hpp"  // run_record


namespace vparse
{
namespace workbench
{


//==============================================================================
// 1.  ELEMENTS
//==============================================================================


// 1.1    listing_view
//------------------------------------------------------------------------------
// 1.1.1
// listing_type
//   the descriptor for the listing element (archetype "listing").
const uxoxo::element_type* listing_type();

// 1.1.2
// listing_view
//   builds a listing of _part for run _run, beside run _other unless it is -1.
// _row is highlighted and scrolled to (-1 for none); clicking a row reports
// _action with the row number, if _action is not empty.
uxoxo::element_template listing_view(
    const std::string& _part,
    int                _run,
    int                _other = -1,
    long               _row = -1,
    const std::string& _action = ""
);


// 1.2    tape_view
//------------------------------------------------------------------------------
// 1.2.1
// tape_type
//   the descriptor for the tape element (archetype "tape").
const uxoxo::element_type* tape_type();

// 1.2.2
// tape_view
//   builds the tape of run _run: _which is "input" (the recognition run) or
// "grammar" (the grammar's own reading). _step is the trace step shown; moving
// the scrubber or clicking a character reports _action with the new step.
uxoxo::element_template tape_view(
    int                _run,
    const std::string& _which,
    long               _step,
    const std::string& _action
);


// 1.3    heat_view
//------------------------------------------------------------------------------
// 1.3.1
// heat_type
//   the descriptor for the heat element (archetype "heat").
const uxoxo::element_type* heat_type();

// 1.3.2
// heat_view
//   builds the heat map of run _run as of trace step _step, beside run
// _other's (at its last step) unless it is -1. _arrivals picks the measure:
// arrivals at each cell, or steps spent there. Clicking a cell that
// something reached reports _action, the payload's "value" the first step
// that reached it and "column", "row" and "rule" the cell.
uxoxo::element_template heat_view(
    int                _run,
    int                _other,
    long               _step,
    bool               _arrivals,
    const std::string& _action
);


//==============================================================================
// 2.  THE IMGUI PAIRING
//==============================================================================


// 2.1    run_finder
//------------------------------------------------------------------------------
// 2.1.1
// run_finder
//   type: how the renderers find a run by id; nullptr if there is none.
using run_finder = std::function<const run_record*(int)>;

// 2.2    install_renderers
//------------------------------------------------------------------------------
// 2.2.1
// install_renderers
//   function: registers the listing and tape renderers for imgui_platform.
// Call once, before the first realize.
void install_renderers(
    run_finder _finder
);

// 2.3    apply_theme
//------------------------------------------------------------------------------
// 2.3.1
// apply_theme
//   function: the workbench's ImGui style and uxoxo tones. Needs a current
// ImGui context.
void apply_theme();

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now

#endif  // VPARSE_UI_ELEMENTS_HPP
