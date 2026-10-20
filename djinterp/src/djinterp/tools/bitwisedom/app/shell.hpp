/*******************************************************************************
* bitwisedom [app]                                                     shell.hpp
*
* The window around the documents: its workspaces, its tool windows, the
* zoom of each window, and the user's own preferences.
*   A document is one sheet, open in a window of its own; the window shows
* its bit grid. A tool window -- the inspector, the operations, the macros,
* a settings page -- shows the document in front of its workspace, and is
* docked anywhere, floated, closed with its x, and opened again from View.
*   A workspace is one of the window's desktops, as a Linux desktop has
* several: its own documents, its own tool windows and its own layout of
* them, all kept while another is in front. The Window menu adds them,
* switches between them, moves a document from one to another, and lays a
* workspace's documents out tabbed, in columns, in rows or in a grid.
*   Each window -- a document's or a tool's -- has a zoom of its own, as a
* browser's page has: Ctrl and the wheel over it, or Ctrl+= , Ctrl+- and
* Ctrl+0 for the window in front.
*   The preferences are the user's, not any sheet's: whether the symbols
* tree is the grid's own symbol column or a window of its own, how the
* window offers to add a row, what colour a new row takes, whether the
* window opens where it was left, and the settings files the user keeps.
*
* path:      /src/djinterp/tools/bitwisedom/app/shell.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.19
*******************************************************************************/

#ifndef BITWISEDOM_APP_SHELL_HPP
#define BITWISEDOM_APP_SHELL_HPP 1

// std
#include <array>    // std::array
#include <cstddef>  // std::size_t
#include <map>      // std::map
#include <string>   // std::string
#include <vector>   // std::vector
// bitwisedom
#include <djinterp/tools/bitwisedom/sheet.hpp>  // rgba


namespace bitwisedom
{
namespace gui
{

// tool
//   enum: the tool windows. Each shows the document in front of its
// workspace; its number is its place on View's list.
enum class tool : int
{
    symbols = 0,
    inspector,
    macros,
    templates,
    operations,
    issues,
    grid_lines,
    colours,
    columns,
    naming,
    sheet_page,
    display,
    console
};

// TOOL_COUNT
//   constant: how many tool windows there are.
const int TOOL_COUNT = 13;

// dock_slot
//   enum: where a tool window is docked when its workspace is first laid
// out: beside the documents on the left or the right, among them at the
// top, or below them.
enum class dock_slot : int
{
    left = 0,
    right,
    top,
    bottom
};

// arrangement
//   enum: how the Window menu lays a workspace's documents out: as tabs of
// one window, side by side in columns, one above another in rows, or in a
// grid as near square as their number allows.
enum class arrangement : int
{
    tabbed = 0,
    columns,
    rows,
    grid
};

// ARRANGEMENT_COUNT
//   constant: how many arrangements there are.
const int ARRANGEMENT_COUNT = 4;

// tool_title / tool_key / tool_slot / tool_open
//   a tool window's title on its tab and on View's list; the name its
// window is known by, within its workspace; where it is first docked; and
// whether it is open in a workspace never laid out before.
D_NODISCARD const char*  tool_title(tool _tool);
D_NODISCARD const char*  tool_key(tool _tool);
D_NODISCARD dock_slot    tool_slot(tool _tool);
D_NODISCARD bool         tool_open(tool _tool);

// tool_for_page
//   the tool window an old page number names -- the bottom panel's, as
// "tab.show" has always numbered them: 0 operations, 1 macros, 2
// templates, 3 issues, 4 grid lines, 5 colours, 6 columns, 7 naming, 8
// the sheet.
D_NODISCARD tool  tool_for_page(int _page);

// workspace
//   struct: one of the window's desktops. Its documents, by id, in the
// order they were opened; the one in front; its tool windows, open or not,
// and those Ctrl+J put away; and the requests its next frame carries out
// once -- a window to bring to the front, documents to dock among the
// others afresh (one just moved here), an arrangement of its documents,
// and a layout built again.
struct workspace
{
    int                          id      = 0;
    std::string                  name;
    std::vector<int>             documents;
    int                          active  = 0;
    std::array<bool, TOOL_COUNT> tools{};
    std::vector<int>             stowed;
    std::array<bool, TOOL_COUNT> focus{};
    int                          raise   = 0;     // a document's window
    std::vector<int>             placed;
    int                          arrange = -1;
    bool                         reset   = false;
};

// add_style
//   enum: how the window offers to add a row: one button that opens the
// menu of what to add, or a split button that adds the kind it shows and
// whose arrow chooses another.
enum class add_style : int
{
    menu = 0,
    split
};

// new_colour
//   enum: what colour a row the user adds takes: none of its own -- set
// bits drawn plain, in the theme's neutral ink -- one of the palette's,
// each row its own, as a sheet's rows always took; or the one colour the
// preferences name, every new row alike.
enum class new_colour : int
{
    none = 0,
    automatic,
    fixed
};

// child_offer
//   enum: what a group's or range's rows end with, to add one more: the
// "Add child..." field, where a name is typed, and beside it a + that opens
// the Add node window; or either alone.
enum class child_offer : int
{
    both = 0,
    field,
    button
};

// node_window
//   enum: how the Add node window opens: as a tab among the sheets, or as a
// window of its own, over them.
enum class node_window : int
{
    tab = 0,
    window
};

// preferences
//   struct: the user's own preferences, which no sheet keeps: whether the
// symbols tree is the grid's symbol column, how a row is added and what
// kind the split button adds, the colour a new row takes, whether the
// window opens where it was left, what ends a group's rows to add one
// more -- and whether the field there fills its column -- how the Add node
// window opens, and the settings files the user keeps, by path.
struct preferences
{
    bool                     merged      = true;
    add_style                add         = add_style::menu;
    int                      add_kind    = 0;
    new_colour               colour      = new_colour::none;
    rgba                     colour_rgba = 0x4C8DFFFFu;
    bool                     restore     = true;
    child_offer              child       = child_offer::both;
    bool                     child_fill  = false;
    node_window              node_as     = node_window::tab;
    std::vector<std::string> settings_files;
};

// action_entry
//   struct: one thing the user did, as the window heard it: when, by the
// clock; the action's name and what it carried; the sheet it was done in;
// and how many times in a row it came -- a drag or a field typed in is one
// action, many times over.
struct action_entry
{
    std::string when;
    std::string kind;
    std::string detail;
    std::string sheet;
    int         times = 1;
};

// ACTIONS_KEPT
//   constant: how many actions the console keeps, the oldest let go first.
const std::size_t ACTIONS_KEPT = 2000;

// shell_state
//   struct: the window around the documents. Its theme and colour scheme;
// its workspaces and the one in front; each window's zoom, by the key its
// dock window carries, and the window last in front; the preferences; and
// the Options window, open or not, at its page, with what its search
// field and its settings file's field hold.
struct shell_state
{
    int                          theme        = 0;
    int                          scheme       = 0;
    std::vector<workspace>       workspaces;
    std::size_t                  current      = 0;
    std::map<std::string, float> zoom;
    std::string                  focused;
    preferences                  prefs;
    bool                         options_open = false;
    int                          options_page = 0;
    std::string                  options_search;
    bool                         node_open    = false;   // Add node
    bool                         node_raise   = false;   // ... to the front
    std::vector<action_entry>    actions;                // the console's
    std::string                  settings_path;
    bool                         demo         = false;
    bool                         quit         = false;
    int                          version      = 0;
};

// ZOOM_LEAST / ZOOM_MOST / ZOOM_STEP
//   constants: how far a window zooms out and in, and how far one step of
// the wheel or one press of Ctrl+= or Ctrl+- takes it.
const float ZOOM_LEAST = 0.5f;
const float ZOOM_MOST  = 3.0f;
const float ZOOM_STEP  = 0.1f;

// document_key / tool_window_key
//   the key a document's window carries -- its zoom's, and the name ImGui
// knows it by -- and a tool window's, within its workspace.
D_NODISCARD std::string  document_key(int _document);
D_NODISCARD std::string  tool_window_key(int  _workspace,
                                         tool _tool);

// dock_id
//   the dock space a workspace's windows are laid out in.
D_NODISCARD unsigned int  dock_id(int _workspace);

// zoom_of
//   a window's zoom, by its key: 1 until it is zoomed.
D_NODISCARD float  zoom_of(const shell_state& _shell,
                           const std::string& _key);

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_SHELL_HPP
