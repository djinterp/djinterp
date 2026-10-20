/*******************************************************************************
* bitwisedom [app]                                                  elements.hpp
*
* The application's own uxoxo elements: the bit grid, the flow view, the
* code view, the docking, and three small controls.
*   Each is an element type of its own, with an archetype no backend knows,
* so its look comes from the renderer register_renderers() installs with
* set_renderer<imgui_platform> -- uxoxo's way of letting an application
* draw what the framework has no vocabulary for. The renderers find the
* document they draw by its id, as each frame is drawn, and post events
* like any element: they never change a sheet themselves.
*   The docking: the dock space a workspace's windows dock in, laid out as
* a new workspace's the first time and whenever it is reset, and its
* documents arranged when asked; and the dock window, a window that docks
* anywhere in it or floats, whose x posts its close action, whose content
* is drawn at its own zoom, and whose events carry its document's id.
*   The controls: the "+" that adds, with its faded "Add new..." beside it;
* the "Add child..." field at the end of a group's or a range's rows in the
* tree, whose text is its own until Enter posts it; and the rest of the menu
* bar after its menus -- the items that follow them, after an indent, and
* the menus that sit at the bar's right end, which the items before them
* are cut short of.
*
* path:      /src/djinterp/tools/bitwisedom/app/elements.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.19
*******************************************************************************/

#ifndef BITWISEDOM_APP_ELEMENTS_HPP
#define BITWISEDOM_APP_ELEMENTS_HPP 1

// std
#include <functional>  // std::function
#include <string>      // std::string
#include <vector>      // std::vector
// uxoxo
#include <uxoxo/element.hpp>                  // element_type, option_set
#include <uxoxo/platform/imgui_platform.hpp>  // imgui_frame, imgui_blueprint
// bitwisedom
#include "./model.hpp"  // model
#include "./shell.hpp"  // action_entry


namespace bitwisedom
{
namespace gui
{

// ADD_NEW_LABEL / ADD_NEW_GAP / ADD_NEW_FADE
//   constants: what the add buttons -- the tree's, and the grid's add row --
// say beside their "+"; the gap between the two, at a scale of one; and how
// opaque the words are until the mouse is on them: slightly faded.
const char* const ADD_NEW_LABEL = "Add new...";
const float       ADD_NEW_GAP   = 6.0f;
const float       ADD_NEW_FADE  = 0.62f;

// ADD_CHILD_HINT
//   constant: what an empty "Add child..." field says.
const char* const ADD_CHILD_HINT = "Add child...";

// element_list
//   type: elements in order, as a container is built over them.
using element_list = std::vector<uxoxo::element_template>;

// blueprint_list
//   type: a container's children, realized, as its renderer receives them.
using blueprint_list = std::vector<uxoxo::platform::imgui_blueprint>;

// the elements -- templates whose look the registered renderers supply;
// each draws the document its "document" attribute names by id, or the
// one in front for none
uxoxo::element_template  bit_grid(uxoxo::option_set _attrs = {});
uxoxo::element_template  flow_view(uxoxo::option_set _attrs = {});
uxoxo::element_template  code_view(uxoxo::option_set _attrs = {});
uxoxo::element_template  definition_view(uxoxo::option_set _attrs = {});

// the docking -- a workspace's dock space: its "id", its default "layout"
// (a line per window, "name<TAB>place"), a "reset" of it, and an "arrange"
// of its "documents" (a line per window's name), for one frame each; and a
// window that docks in it, by its "key", with its "title", over _items:
// "dockspace", "dock_with", "place", "focus", "zoom", "zoom_action",
// "close_action", "focus_action" and "value", "document", "unsaved",
// "scroll", "flush", and -- for a window of its own, which does not dock --
// "dockable" false, "width" and "height"
uxoxo::element_template  dock_space(uxoxo::option_set _attrs = {});
uxoxo::element_template  dock_window(const std::string& _key,
                                     const std::string& _title,
                                     element_list       _items,
                                     uxoxo::option_set  _attrs = {});

// the controls -- an add button, posting "action" (by default
// "context.add") from its "+" or its label; a field that adds a child to
// the row _parent names, posting "tree.add_child" with the text typed;
// and the menu bar's rest: _leading after an "indent", cut short of
// _trailing, the menus at the bar's right end
uxoxo::element_template  add_new_button(uxoxo::option_set _attrs = {});
uxoxo::element_template  add_child_field(long              _parent,
                                         uxoxo::option_set _attrs = {});
uxoxo::element_template  menu_bar_rest(element_list      _leading,
                                       element_list      _trailing,
                                       uxoxo::option_set _attrs = {});

// the status bar -- along the window's foot: "left", what the sheet in
// front is, and at the right end "right", what the window last said --
// "error" when it went wrong -- after a "*" while the sheet has changes
// ("dirty"); a click on it posts "action" with "value"
uxoxo::element_template  status_bar(uxoxo::option_set _attrs = {});

// the console's list -- every action so far, the latest first, read as
// each frame is drawn: a table that draws only the lines in view, however
// many there are
uxoxo::element_template  action_list(uxoxo::option_set _attrs = {});

// the add control -- the one way a panel offers to add a row, as the user
// prefers it: a "+ Add" button opening the menu of what to add ("style"
// "menu"), or a split button whose face adds the kind it shows and whose
// arrow chooses another ("split", with "kind" the place among ADD_KINDS).
// Its menu sets the colour new rows take too ("colour", the new_colour
// now; "rgba", the one colour). It posts "edit.add" with the kind, and
// "set" of the window's own settings.
uxoxo::element_template  add_control(uxoxo::option_set _attrs = {});

// document_source
//   type: how a renderer finds the document it draws, by its id -- 0 for
// the one in front -- as each frame is drawn: nullptr for a document closed
// since its interface was built.
using document_source = std::function<model*(int)>;

// actions_source
//   type: how the console's list finds the actions it lists, as each frame
// is drawn: nullptr for none.
using actions_source = std::function<const std::vector<action_entry>*()>;

/**
 * @brief Installs the application's renderers for the ImGui platform.
 *
 * @param[in] _documents  how they find the documents they draw; what it
 *                        reaches must outlive every frame drawn with them.
 * @param[in] _actions    how the console finds the actions it lists, the
 *                        same.
 */
void register_renderers(document_source _documents,
                        actions_source  _actions = actions_source());

/**
 * @brief Whether a dock window drew its content in this frame or the last:
 *        whether it is in view -- a window docked behind another's tab is
 *        not.
 *
 * @param[in] _key  the dock window's key.
 * @return true if it was drawn, open and in view.
 */
bool window_seen(const std::string& _key);

// the drawings -- called by the renderers inside a frame
void draw_grid(model&                        _model,
               uxoxo::platform::imgui_frame& _frame);
void draw_add_new(const uxoxo::option_set&      _attrs,
                  uxoxo::platform::imgui_frame& _frame);
void draw_add_child(const uxoxo::option_set&      _attrs,
                    uxoxo::platform::imgui_frame& _frame);
void draw_menu_bar_rest(const uxoxo::option_set&      _attrs,
                        const blueprint_list&         _children,
                        uxoxo::platform::imgui_frame& _frame);
void draw_add_control(const uxoxo::option_set&      _attrs,
                      uxoxo::platform::imgui_frame& _frame);
void draw_status_bar(const uxoxo::option_set&      _attrs,
                     uxoxo::platform::imgui_frame& _frame);
void draw_action_list(const std::vector<action_entry>& _actions);
void draw_dock_space(const uxoxo::option_set&      _attrs,
                     uxoxo::platform::imgui_frame& _frame);
void draw_dock_window(const uxoxo::option_set&      _attrs,
                      const blueprint_list&         _children,
                      uxoxo::platform::imgui_frame& _frame);

// grid_point
//   struct: a point of the grid on the screen, if the last frame drew it.
struct grid_point
{
    bool  found = false;
    float x     = 0.0f;
    float y     = 0.0f;
};

/**
 * @brief Where the last frame drew a part of the grid, on the screen: for
 *        a test that drives the grid with the mouse.
 *
 * @param[in] _part  "name" or "cell" -- a row's name, or its cell for a
 *                   bit; "band" or "boundary" -- the heading's band over
 *                   a bit, or the ruler's boundary right of it;
 *                   "edge.name" or "edge.value" -- a pinned column's edge;
 *                   "end" or "room" -- where the columns end, and the
 *                   right edge of the width they were laid out to fill;
 *                   "add" -- the add row's "Add new..."; "heading" --
 *                   the name column's heading, under its buttons; or
 *                   "names_by.symbol", "names_by.name" or
 *                   "names_by.value" -- a button over the symbols column,
 *                   found only while the column has room for them.
 * @param[in] _row   the row's id, for "name" and "cell".
 * @param[in] _bit   the bit, for "cell", "band" and "boundary".
 * @return the point, found or not.
 */
grid_point grid_spot(const std::string& _part,
                     int                _row,
                     int                _bit);

// grid_spot
//   the same, in the grid of the document named by its id -- for a test
// with more than one grid in view; the one above reads the grid drawn
// last.
grid_point grid_spot(int                _document,
                     const std::string& _part,
                     int                _row,
                     int                _bit);

/**
 * @brief Where the last frame drew a part of a control, on the screen: for
 *        a test that drives it with the mouse.
 *
 * @param[in] _key  an add control's key: its button -- "+ Add", or a split
 *                  button's face; with ".more", a split button's arrow;
 *                  with ".kind.N", the Nth kind its menu lists, among
 *                  ADD_KINDS; with ".colours", its menu's New rows' colour,
 *                  and in that, ".colour.N", the Nth way new rows are
 *                  coloured, and ".rgba", the swatch of the one colour.
 * @return the middle of the part, found or not.
 */
grid_point control_spot(const std::string& _key);

// ... and an "Add child..." row's, by where it is -- "tree" or "grid":
// "add_child.WHERE.plus", its +; ".field", its field; ".end", where the
// field ends

// grid_text
//   struct: what the grid wrote for a row of the sheet's, if the last frame
// drew it.
struct grid_text
{
    bool        found = false;
    std::string label;          // in the name column, before fitting
    float       end   = 0.0f;   // where that ended, fitted, on the screen
    std::string note;           // over the cells of a row with no bits
    int         digits   = 0;   // its cells in view that wrote a digit,
    int         coloured = 0;   // ... were filled in its colour,
    int         plain    = 0;   // ... and were set, drawn plain
};

/**
 * @brief What the last frame wrote for a row of the sheet's: for a test
 *        that reads the grid as a user would.
 *
 * @param[in] _row  the row's id.
 * @return the name column's text -- a name, or a value in the form the
 *         sheet lists values in -- where it ended, and what a row with no
 *         bits wrote over its cells; not found for a row not drawn.
 */
grid_text grid_written(int _row);
grid_text grid_written(int _document,
                       int _row);

/**
 * @brief What the last frame wrote for a probe or usage row.
 *
 * @param[in] _special  its id among the special rows.
 * @return its name, in label -- "probe", "allocated", or "allocated in"
 *         its group -- and its value, in note: the probe's, or "N of W",
 *         the bits it counts allocated; not found for a row not drawn.
 */
grid_text grid_special(int _special);

/**
 * @brief Where a document's grid's symbols and value columns ended, on the
 *        screen, in the last frame it was drawn.
 *
 * @param[in]  _document  the document's id.
 * @param[out] _x         the columns' right edge.
 * @return whether the grid was drawn with either column shown.
 */
bool grid_pinned_edge(int    _document,
                      float& _x);
void draw_flow(model&                        _model,
               uxoxo::platform::imgui_frame& _frame,
               float                         _height);
void draw_code(model&                        _model,
               uxoxo::platform::imgui_frame& _frame);

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_ELEMENTS_HPP
