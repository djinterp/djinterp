/*******************************************************************************
* bitwisedom [app]                                                     shell.cpp
*
* The tool windows' table, and the keys windows are known by.
*   Every tool window has a title, a key that names its window within a
* workspace -- so two workspaces each have an inspector of their own, laid
* out as each was left -- a place it is first docked, and whether a
* workspace never laid out before opens it.
*
* path:      /src/djinterp/tools/bitwisedom/app/shell.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.19
*******************************************************************************/
#include "./shell.hpp"  // corresponding header

// std
#include <algorithm>  // std::clamp
// imgui
#include <imgui.h>  // ImHashStr's seedless cousin, ImGui::GetID's rules
#include <imgui_internal.h>  // ImHashStr


namespace bitwisedom
{
namespace gui
{

namespace
{

// tool_info
//   struct: one tool window's row of the table.
struct tool_info
{
    const char* title;
    const char* key;
    dock_slot   slot;
    bool        open;
};

// TOOLS
//   constant: the tool windows, as tool numbers them. The symbols tree is
// closed at first: the grid's symbol column is the tree, until the user
// asks for the two apart. Macros and Templates sit among the documents.
// The display rules are closed until the grid's menus ask for them; the
// console, until the status bar's message is clicked.
const tool_info TOOLS[TOOL_COUNT] = {
    { "Symbols",    "symbols",    dock_slot::left,   false },
    { "Inspector",  "inspector",  dock_slot::right,  true },
    { "Macros",     "macros",     dock_slot::top,    true },
    { "Templates",  "templates",  dock_slot::top,    true },
    { "Operations", "operations", dock_slot::bottom, true },
    { "Issues",     "issues",     dock_slot::bottom, true },
    { "Grid lines", "grid_lines", dock_slot::bottom, true },
    { "Colours",    "colours",    dock_slot::bottom, true },
    { "Columns",    "columns",    dock_slot::bottom, true },
    { "Naming",     "naming",     dock_slot::bottom, true },
    { "Sheet",      "sheet",      dock_slot::bottom, true },
    { "Display",    "display",    dock_slot::bottom, false },
    { "Console",    "console",    dock_slot::bottom, false } };

// info
//   a tool's row, held to the table.
const tool_info&
info(
    tool _tool
)
{
    const int index = std::clamp(static_cast<int>(_tool), 0, TOOL_COUNT - 1);

    return TOOLS[index];
}

}  // anonymous namespace


const char*
tool_title(
    tool _tool
)
{
    return info(_tool).title;
}

const char*
tool_key(
    tool _tool
)
{
    return info(_tool).key;
}

dock_slot
tool_slot(
    tool _tool
)
{
    return info(_tool).slot;
}

bool
tool_open(
    tool _tool
)
{
    return info(_tool).open;
}

tool
tool_for_page(
    int _page
)
{
    // PAGES
    //   constant: the bottom panel's pages, in their old order.
    static const tool PAGES[] = {
        tool::operations, tool::macros, tool::templates, tool::issues,
        tool::grid_lines, tool::colours, tool::columns, tool::naming,
        tool::sheet_page };
    const int count = static_cast<int>(sizeof(PAGES) / sizeof(PAGES[0]));

    return PAGES[std::clamp(_page, 0, count - 1)];
}

std::string
document_key(
    int _document
)
{
    return "doc" + std::to_string(_document);
}

std::string
tool_window_key(
    int  _workspace,
    tool _tool
)
{
    return "w" + std::to_string(_workspace) + "." + tool_key(_tool);
}

unsigned int
dock_id(
    int _workspace
)
{
    const std::string name = "bitwisedom.dock.w" + std::to_string(_workspace);

    return static_cast<unsigned int>(ImHashStr(name.c_str()));
}

float
zoom_of(
    const shell_state& _shell,
    const std::string& _key
)
{
    const auto found = _shell.zoom.find(_key);

    return (found == _shell.zoom.end()) ? 1.0f : found->second;
}

}  // namespace gui
}  // namespace bitwisedom
