/*******************************************************************************
* bitwisedom [app]                                                     model.hpp
*
* The GUI's state: the sheet, what it resolves to, its history, and how the
* window is being looked at.
*   The application is a loop of the uxoxo kind: the state is drawn, the
* drawing posts events, and after the frame the events are applied to the
* state -- never during it. Everything a renderer needs to read is here; the
* one thing renderers write directly is what the mouse is over, which is
* not state the user can undo but a fact about this frame.
*   The caches -- the generated header, the operation preview -- are
* computed when first wanted after a change, and dropped by the next one.
*   Three requests last one frame: opening the context menu, giving a
* rename's field the keyboard, and fitting the grid's name column to the
* names of a sheet just imported. The frame they are drawn in clears them.
*   The theme, the colour scheme and how the panels are shown are the
* window's, kept in its settings file rather than with the sheet; the look
* -- the sheet's colours with what it leaves automatic filled in from them
* -- is what every renderer draws with.
*
* path:      /src/djinterp/tools/bitwisedom/app/model.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

#ifndef BITWISEDOM_APP_MODEL_HPP
#define BITWISEDOM_APP_MODEL_HPP 1

// std
#include <string>  // std::string
#include <vector>  // std::vector
// bitwisedom
#include <djinterp/tools/bitwisedom/codegen.hpp>  // generated
#include <djinterp/tools/bitwisedom/history.hpp>  // history
#include <djinterp/tools/bitwisedom/ops.hpp>      // op_preview
#include <djinterp/tools/bitwisedom/resolve.hpp>  // resolution
#include <djinterp/tools/bitwisedom/sheet.hpp>    // sheet


namespace bitwisedom
{
namespace gui
{

// panel_mode
//   enum: how the tabbed panel below the grid is shown: docked under the
// grid; minimized to its tabs; minimized until the mouse is over its tabs,
// when it opens over the grid; floating in a window of its own; or not at
// all.
enum class panel_mode : int
{
    docked = 0,
    minimized,
    peek,
    floating,
    hidden
};

// PANEL_MODE_COUNT
//   constant: how many panel modes there are.
const int PANEL_MODE_COUNT = 5;

// ui_state
//   struct: how the window is being used -- not part of the sheet, and not
// undone with it.
struct ui_state
{
    std::vector<int> selection;            // the first is the inspected
    int              anchor      = 0;      // where a shift-click runs from
    int              hover_row   = 0;      // this frame's, written by draws
    int              hover_bit   = -1;
    std::string      path        = "untitled.bitwisedom.json";
    std::string      status;
    bool             status_error = false;
    int              tab         = 0;      // the bottom panel's front page
    bool             tab_forced  = false;  // bring it to the front, once
    float            left        = 236.0f;
    float            right       = 330.0f;
    float            bottom      = 270.0f;

    // the window's look and its panels, which the settings file keeps
    int              theme       = 0;      // 0 dark, 1 light
    int              scheme      = 0;      // a colour scheme
    panel_mode       panel       = panel_mode::docked;
    panel_mode       panel_back  = panel_mode::docked;   // Ctrl+J's way back
    bool             peeking     = false;  // a peek panel, open over the grid
    bool             show_left   = true;   // the tree of symbols
    bool             show_right  = true;   // the inspector
    int              op          = 0;
    std::string      operand     = "1";
    bool             op_probe    = false;
    bool             op_preview  = true;
    std::string      probe_text;
    int              drag_row    = 0;      // a stroke across a row's cells
    int              drag_from   = -1;
    int              drag_to     = -1;
    bool             drag_set    = true;
    // Shift+drag selects a rectangle of visible grid cells, without
    // changing the rows' bit definitions. Grid cells are selected by ID.
    std::vector<int> grid_rect_rows;
    int              grid_rect_low  = -1;
    int              grid_rect_high = -1;
    bool             dirty       = false;  // changed since saved
    bool             quit        = false;
    bool             demo        = false;

    // the context menu: what it is about, and a request to open it, which
    // lasts one frame
    std::string      menu_kind;                // "row", "segment", "sheet"
    std::string      menu_where;               // "tree", "grid", "table"
    int              menu_row     = 0;
    int              menu_segment = -1;        // the sheet's, or -1
    int              menu_lsb     = 0;         // the segment's bits
    int              menu_msb     = 0;
    bool             menu_open    = false;

    // a rename in progress: a row's or a segment's, in the tree or the
    // grid; the field takes the keyboard in its first frame
    int              renaming         = 0;
    int              renaming_segment = -1;
    std::string      rename_text;
    std::string      rename_where;             // "tree" or "grid"
    bool             rename_focus     = false;

    // what the last import said, until the next one; and a request, for
    // one frame, that the grid fit its name and value columns to a sheet
    // just imported or taken from the examples
    std::string              import_name;
    std::vector<std::string> import_notes;
    bool                     fit_columns = false;
};

// model
//   struct: everything the window draws from.
struct model
{
    sheet       doc;
    look        shown;          // doc's colours, as the window draws them
    resolution  res;
    history     steps;
    ui_state    ui;
    op_preview  preview;
    bool        preview_valid = false;
    generated   header;
    bool        header_valid  = false;
    int         version       = 0;
    std::string today         = "2026.10.04";
};

// inspected
//   the row the inspector shows, or nullptr.
inline const row*
inspected(
    const model& _model
)
{
    if (_model.ui.selection.empty())
    {
        return nullptr;
    }

    return find_row(_model.doc, _model.ui.selection.front());
}

// is_selected
//   whether a row is part of the selection.
inline bool
is_selected(
    const model& _model,
    int          _id
)
{
    for (int each : _model.ui.selection)
    {
        if (each == _id)
        {
            return true;
        }
    }

    return false;
}

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_MODEL_HPP
