/*******************************************************************************
* bitwisedom [ui]                                                      model.hpp
*
* A document's state: its sheet, what it resolves to, its history, and how
* it is being looked at.
*   The application is a loop of the uxoxo kind: the state is drawn, the
* drawing posts events, and after the frame the events are applied to the
* state -- never during it. Everything a renderer needs to read is here; the
* one thing renderers write directly is what this frame found -- what the
* mouse is over, and whether the operation's preview is in view -- which
* is not state the user can undo but a fact about this frame.
*   The caches -- the generated header, the operation preview -- are
* computed when first wanted after a change, and dropped by the next one.
* The order the rows are shown in is worked out with every resolution: the
* sheet's own, or each group's rows sorted.
*   The grid's CURSOR is a spreadsheet's active cell: a row -- of the
* sheet's, or a probe or usage row -- and a column -- the names', the
* values', or a bit's -- with the range selected from it, as Excel keeps
* them. The rows the range covers are the selection, the cursor's row
* first, and the bits it covers the block of cells selected; a selection
* made elsewhere, in the tree, say, takes the cursor with it.
*   Four requests last one frame: opening the context menu, giving a
* rename's field the keyboard, giving it back to the tree's "Add child..."
* field a name was just entered in, and fitting the grid's name column to
* the names of a sheet just imported. The frame they are drawn in clears
* them.
*   The theme and the colour scheme are the window's (shell.hpp), copied
* into every document so its look -- the sheet's colours with what it
* leaves automatic filled in from them -- is what every renderer draws with.
*
* path:      /inc/ui/bitwisedom/model.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef UI_BITWISEDOM_MODEL_HPP
#define UI_BITWISEDOM_MODEL_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
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

// CURSOR_NAME / CURSOR_VALUE / CURSOR_NONE
//   constants: the grid cursor's column on the name column, on the value
// column -- a column of bits is its lowest bit -- and no column at all.
const int CURSOR_NAME  = -1;
const int CURSOR_VALUE = -2;
const int CURSOR_NONE  = -9;

// special_cursor / cursor_special
//   a probe or usage row as the cursor names its row -- by its id among the
// special rows, below zero, where a row of the sheet's is its id, above --
// and back again: -1 for a row of the sheet's, or none.
inline int
special_cursor(
    int _id
)
{
    return (-1 - _id);
}

inline int
cursor_special(
    int _row
)
{
    return (_row < 0) ? (-1 - _row) : -1;
}

// node_draft
//   struct: the row the Add node window is making, in one document: the
// row itself -- a group, or a symbol of any definition -- the group or
// range it goes into, 0 for the sheet's own rows, and whether it goes
// first there or last. What it comes to is worked out on a copy of the
// sheet as it changes: its name and value, what the header would write,
// and why it would be refused. `fresh` is the name the row takes when none
// is typed.
struct node_draft
{
    row                  draft;
    int                  parent  = 0;
    bool                 first   = false;
    bool                 bits_set = false;   // the user chose its bits
    resolved_row         resolved;
    bool                 resolves = false;
    std::vector<issue>   issues;
    std::string          refused;
    std::string          fresh;
    int                  added   = 0;   // how many it has added so far
};

// ui_state
//   struct: how a document is being used -- not part of its sheet, and not
// undone with it.
struct ui_state
{
    std::vector<int> selection;            // the first is the inspected
    int              anchor      = 0;      // where a shift-click runs from
    int              hover_row   = 0;      // this frame's, written by draws
    int              hover_bit   = -1;
    std::string      path        = "untitled.bitwisedom.json";
    std::string      file;                 // where it was read or saved
    std::string      status;
    bool             status_error = false;
    int              definition  = -1;     // the definition file in front
    int              definition_editing = -1;   // ... being edited
    bool             definition_raise   = false;  // ... to the front, for
                                                  // one frame

    // the window's look, which every document is drawn in alike
    int              theme       = 0;      // 0 dark, 1 light
    int              scheme      = 0;      // a colour scheme
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
    // the grid's cursor, as Excel's active cell: its row -- a row of the
    // sheet's by its id, a probe or usage row as special_cursor names it,
    // 0 for none -- and its column, CURSOR_NAME, CURSOR_VALUE or a bit; the
    // range selected from it, the corner that stays and the one Shift
    // moves; the column a run of Tabs began in, which Enter goes back to;
    // and End mode and Extend mode, which the status bar shows
    int              cursor_row    = 0;
    int              cursor_column = CURSOR_NAME;
    int              from_row      = 0;
    int              from_column   = CURSOR_NAME;
    int              to_row        = 0;
    int              to_column     = CURSOR_NAME;
    int              tab_column    = CURSOR_NONE;
    int              cursor_chose  = 0;    // the selection's first row
                                           // when the cursor last chose it
    bool             end_mode      = false;
    bool             extend_mode   = false;
    bool             dirty       = false;  // changed since saved
    bool             pristine    = true;   // new, and never touched
    bool             browsing    = false;  // an example, not yet touched:
                                           // the next one takes its place
    bool             preview_shown = false;  // this frame's: the
                                             // operation's preview is in
                                             // view
    bool             in_front      = false;  // this frame's: the document
                                             // is the one in front

    // the context menu: what it is about, and a request to open it, which
    // lasts one frame
    std::string      menu_kind;                // "row", "segment", "sheet",
                                               // "names_by", "special"
    std::string      menu_where;               // "tree", "grid", "table"
    int              menu_row     = 0;
    int              menu_special = 0;         // a special row's id
    std::string      menu_tab;                 // a tab's window's key,
    std::string      menu_tab_title;           // ... its name, and
    std::vector<std::string> menu_tabs;        // ... its row's, in order
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

    // a child just named in a tree's "Add child..." field: the group or
    // range it went to, and a request, for one frame, that the field take
    // the keyboard again -- empty for the next name, or with a name it
    // refused, to be put right
    int              adding_to        = -1;
    std::string      adding_text;
    bool             adding_focus     = false;
    std::string      adding_where     = "tree";   // or "grid"

    // the document's file renamed in its tab: the name typed over the tab,
    // which takes the keyboard in its first frame
    bool             file_renaming     = false;
    bool             file_rename_focus = false;

    // the Add node window's row in the making; and, as the preferences
    // have it, what ends a group's rows to add one more -- 0 the field and
    // a +, 1 the field, 2 the + -- and whether the field fills its column
    node_draft       node;
    int              child_offer      = 0;
    bool             child_fill       = false;

    // what the last import said, until the next one; and a request, for
    // one frame, that the grid fit its name and value columns to a sheet
    // just imported or taken from the examples
    std::string              import_name;
    std::vector<std::string> import_notes;
    bool                     fit_columns = false;
};

// model
//   struct: one document: everything its windows draw from. Its id names
// it to the window, unchanged while it is open.
struct model
{
    int         id            = 0;
    sheet       doc;
    look        shown;          // doc's colours, as the window draws them
    resolution  res;
    history     steps;

    // the rows as the grid and the tree show them, sorted as the grid's
    // sort_by says: indices into res.rows; and each row's place among them,
    // by its index in res.rows. sort_rows keeps them with res
    std::vector<std::size_t> order;
    std::vector<std::size_t> place;

    ui_state    ui;
    op_preview  preview;
    bool        preview_valid = false;
    generated   header;
    bool        header_valid  = false;
    int         version       = 0;
    std::string today         = "2026.10.04";
};

// sort_rows
//   the order the grid and the tree show a model's rows in, worked out
// again: after every resolution.
inline void
sort_rows(
    model& _model
)
{
    _model.order = display_order(_model.doc, _model.res);
    _model.place.assign(_model.order.size(), 0);

    for (std::size_t i = 0; i < _model.order.size(); ++i)
    {
        _model.place[_model.order[i]] = i;
    }

    return;
}

// shown_rows
//   a model's rows, in the order the grid and the tree show them -- the
// sheet's own, while the order is out of step with the resolution.
inline std::vector<const resolved_row*>
shown_rows(
    const model& _model
)
{
    const std::vector<resolved_row>& rows = _model.res.rows;
    std::vector<const resolved_row*> out;

    out.reserve(rows.size());

    if (_model.order.size() == rows.size())
    {
        for (std::size_t each : _model.order)
        {
            out.push_back(&rows[each]);
        }
    }
    else
    {
        for (const resolved_row& each : rows)
        {
            out.push_back(&each);
        }
    }

    return out;
}

// place_of
//   where a row comes among those the grid and the tree show, by its id:
// for sorting a group's children as they are shown.
inline std::size_t
place_of(
    const model& _model,
    int          _id
)
{
    const auto found = _model.res.index.find(_id);

    if (found == _model.res.index.end())
    {
        return _model.res.rows.size();
    }

    return (_model.place.size() == _model.res.rows.size())
               ? _model.place[found->second]
               : found->second;
}

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

// active_row
//   the row the grid's cursor is on: its own, while the selection holds it
// -- a probe or usage row, while the selection is the one it chose -- and
// otherwise the selection's first, so the cursor follows a selection made
// elsewhere; 0 for none. While it is the cursor's own, the range selected from it holds.
inline int
active_row(
    const model& _model
)
{
    const ui_state& ui = _model.ui;

    // a probe or usage row: the cursor's, while the selection is the one
    // it chose -- none, or the rows of its range
    if (ui.cursor_row < 0)
    {
        return ( (ui.selection.empty()) ||
                 (ui.selection.front() == ui.cursor_chose) )
                   ? ui.cursor_row
                   : ui.selection.front();
    }

    if ( (ui.cursor_row > 0) &&
         (is_selected(_model, ui.cursor_row)) )
    {
        return ui.cursor_row;
    }

    return ui.selection.empty() ? ui.cursor_row : ui.selection.front();
}

}  // namespace gui
}  // namespace bitwisedom


#endif  // UI_BITWISEDOM_MODEL_HPP
