/*******************************************************************************
* vparse [ui]                                                           view.hpp
*
* The workbench: its state, the view of that state, and how events change it.
*   The shape is model-view-update. workbench holds everything the screen shows
* -- the editors, the runs so far, which run is selected and which it is
* compared with, the trace step on each tape, what the heat map measures, and
* where runs are saved. view turns that state into a uxoxo template; the host
* realizes and draws it, and feeds the events the drawing posted back through
* update. Widget-local state (a text cursor, a scroll position) stays with
* ImGui, so the template can be rebuilt from scratch every frame.
*
* path:      /inc/ui/vparse/view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  STATE
    -----
    1.  workbench
2.  OPERATIONS
    ----------
    1.  make_model
    2.  view
    3.  update
    4.  find_run
*/

#ifndef VPARSE_UI_VIEW_HPP
#define VPARSE_UI_VIEW_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error, as the
// uxoxo headers it stands on would not compile there.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
#include <vector>  // std::vector
// uxoxo
#include "uxoxo/element.hpp"  // element_template, event
// vparse
#include "../../djinterp/tools/vparse/run.hpp"    // run_record
#include "../../djinterp/tools/vparse/store.hpp"  // saved_run


namespace vparse
{
namespace workbench
{


//==============================================================================
// 1.  STATE
//==============================================================================


// 1.1    workbench
//------------------------------------------------------------------------------
// 1.1.1
// workbench
//   struct: everything the workbench shows. selected and compare index runs;
// force_tab names a part whose tab is brought to the front on the next frame
// only, and the host clears it after drawing.
struct model
{
    // the editors
    std::string grammar;
    std::string input;
    std::string frontend   = "notation";
    bool        trace      = true;
    std::string step_limit = "100000";

    // the runs
    std::vector<run_record> runs;
    int                     next_id      = 1;
    int                     selected     = -1;
    int                     compare      = -1;
    long                    step         = 0;
    long                    reading_step = 0;
    bool                    heat_arrivals = true;
    std::string             force_tab;

    // the files
    std::string            root = "vparse_runs";
    std::string            session;
    bool                   save_each = false;
    std::vector<saved_run> saved;

    // the status line
    std::string status;
    bool        status_error = false;
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    make_model
//------------------------------------------------------------------------------
// 2.1.1
// make_model
//   function: a fresh workbench with an example grammar and input, and a new
// session name for saving.
model make_model();

// 2.2    view
//------------------------------------------------------------------------------
// 2.2.1
// view
//   function: the whole screen as a closed uxoxo template.
uxoxo::element_template view(
    const model& _model
);

// 2.3    update
//------------------------------------------------------------------------------
// 2.3.1
// update
//   function: applies one event. Kinds: run, edit_grammar, edit_input,
// frontend, trace, step_limit, select_run, compare_run, stop_compare, step,
// reading_step, heat_measure, edit_run, root, save_run, save_all, save_each,
// refresh_saved, load_saved, load_folder. Unknown kinds are ignored.
void update(
    model&              _model,
    const uxoxo::event& _event
);

// 2.4    find_run
//------------------------------------------------------------------------------
// 2.4.1
// find_run
//   function: the run with id _id, or nullptr.
const run_record* find_run(
    const model& _model,
    int          _id
);

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now

#endif  // VPARSE_UI_VIEW_HPP
