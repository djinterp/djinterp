/*******************************************************************************
* bitwisedom [ui]                                                   controls.cpp
*
* The controls the interface has no uxoxo element for: the "+" that adds,
* with a faded "Add new..." beside it; the add control -- "+ Add" and the
* menu of what to add, or a split button whose face adds the kind it shows
* -- with the colour new rows take on its menu; the "Add child..." field at
* the end of a group's or a range's rows in the tree; the rest of the menu
* bar, which puts the Help menu at the bar's right end; and the docking --
* the dock space a workspace's windows dock in, and the windows themselves.
*   The field's text is its own while it is typed -- kept here, by the
* field's ImGui ID, between frames, as uxoxo keeps an edit's draft -- and
* is posted only by Enter, so a click elsewhere never adds a row half
* named. The window answers by asking, for one frame, that the field take
* the keyboard again: empty, for the next name, or with the name it
* refused, to be put right.
*   The menu bar's rest measures the menus it puts at the right end as
* ImGui lays a menu out in a bar -- its title, and half a spacing either
* side widened to a spacing -- so the last one's title ends as far from the
* bar's right edge as the first menu's begins from its left. What comes
* before them is cut short of them: a long status line ends in an ellipsis
* rather than running under Help.
*   THE DOCK SPACE is ImGui's, one per workspace. The first time a
* workspace is shown, and whenever its layout is reset, it is laid out as a
* new workspace is: the inspector down the right, the tool windows below
* the documents, the symbols tree on the left, and the documents -- the
* macros and the templates among them -- in the middle, the dock space's
* central node. Every window of the workspace is docked in its place, open
* or not, so one opened later opens there; a place with no open window
* takes no room. An arrangement lays the documents out afresh in that
* middle: as tabs of one window, in columns, in rows, or in a grid.
*   A DOCK WINDOW is an ImGui window that docks: dragged by its tab to
* another window's, or to an edge of one, as Visual Studio's are, or out on
* its own. Its x posts its close action. A window shown for the first time
* docks beside the window it is told to, or in the middle; it never takes
* the keyboard by appearing, only when it is asked to come to the front or
* the user clicks in it -- and then it says so, once. What its content
* posts it passes on, with its document's id, so the event acts on the
* document whose window it came from. Each draws its content at its own
* zoom: its text larger or smaller, and its spacings with it, as a
* browser's page zooms; Ctrl and the wheel over it ask for another.
*
* path:      /src/ui/bitwisedom/controls.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "../../../inc/ui/bitwisedom/elements.hpp"  // corresponding header

// std
#include <algorithm>      // std::max, std::min
#include <cstring>        // std::strstr
#include <cfloat>         // FLT_MAX
#include <cmath>          // std::ceil, std::sqrt
#include <cstddef>        // std::size_t
#include <cstdlib>        // std::strtoul
#include <string>         // std::string
#include <unordered_map>  // std::unordered_map
#include <utility>        // std::move
#include <vector>         // std::vector
// imgui
#include <imgui.h>                  // ImGui
#include <imgui_internal.h>         // DockBuilder, ImGuiDockNode
#include <misc/cpp/imgui_stdlib.h>  // InputTextWithHint over std::string
// bitwisedom
#include "../../../inc/ui/bitwisedom/settings.hpp"  // ADD_KINDS, add_kind_name
#include "../../../inc/ui/bitwisedom/theme.hpp"     // ui_scale


namespace bitwisedom
{
namespace gui
{

namespace
{

using uxoxo::option_set;
using uxoxo::platform::imgui_blueprint;
using uxoxo::platform::imgui_frame;

// drafts
//   what is typed in each "Add child..." field, by the field's ImGui ID,
// while there is any.
std::unordered_map<ImGuiID, std::string>&
drafts()
{
    static std::unordered_map<ImGuiID, std::string> held;

    return held;
}

// blank
//   whether a text is nothing but spaces.
bool
blank(
    const std::string& _text
)
{
    return (_text.find_first_not_of(" \t\r\n") == std::string::npos);
}

// spotted
//   struct: where a control was last drawn: the middle of it, in which ImGui
// and which of its frames.
struct spotted
{
    const ImGuiContext* context = nullptr;
    int                 frame   = -1;
    ImVec2              middle;
};

// spots
//   each control's last spot, by its key and part.
std::unordered_map<std::string, spotted>&
spots()
{
    static std::unordered_map<std::string, spotted> held;

    return held;
}

// spot
//   the last item drawn, kept as where the control with a key was.
void
spot(
    const std::string& _key
)
{
    const ImVec2 low  = ImGui::GetItemRectMin();
    const ImVec2 high = ImGui::GetItemRectMax();

    spots()[_key] = spotted{ ImGui::GetCurrentContext(),
                             ImGui::GetFrameCount(),
                             ImVec2( (low.x + high.x) * 0.5f,
                                     (low.y + high.y) * 0.5f ) };

    return;
}

// tip
//   the element's "tooltip", under a mouse that has rested on the last item.
void
tip(
    const option_set& _attrs
)
{
    const std::string text = _attrs.as_string("tooltip", "");

    if ( (!text.empty()) &&
         (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) )
    {
        ImGui::SetTooltip("%s", text.c_str());
    }

    return;
}

// draw_children
//   children drawn in [_from, _to), each under its key, or its index, as
// uxoxo's containers draw theirs.
void
draw_children(
    const blueprint_list& _children,
    std::size_t           _from,
    std::size_t           _to,
    imgui_frame&          _frame
)
{
    for (std::size_t i = _from; i < _to; ++i)
    {
        const imgui_blueprint& child = _children[i];

        // an empty draw is a child that renders nothing
        if (!child.draw)
        {
            continue;
        }

        if (!child.key.empty())
        {
            ImGui::PushID(child.key.c_str());
        }
        else
        {
            ImGui::PushID(static_cast<int>(i));
        }

        child.draw(_frame);
        ImGui::PopID();
    }

    return;
}

// lines
//   a text's lines, without their breaks.
std::vector<std::string>
lines(
    const std::string& _text
)
{
    std::vector<std::string> out;
    std::size_t              at = 0;

    while (at < _text.size())
    {
        std::size_t end = _text.find('\n', at);

        if (end == std::string::npos)
        {
            end = _text.size();
        }

        if (end > at)
        {
            out.push_back(_text.substr(at, end - at));
        }

        at = end + 1;
    }

    return out;
}

// central
//   the central node of a dock space's tree: where its documents go. Found
// by walking the tree, not from the root's record of it, which a merge
// earlier in the frame can leave behind.
ImGuiDockNode*
central(
    ImGuiDockNode* _node
)
{
    if (_node == nullptr)
    {
        return nullptr;
    }

    if (_node->IsCentralNode())
    {
        return _node;
    }

    for (ImGuiDockNode* child : _node->ChildNodes)
    {
        if (ImGuiDockNode* found = central(child))
        {
            return found;
        }
    }

    return nullptr;
}

// leaves_with_windows
//   how many of a tree's leaves hold windows, the last found kept.
int
leaves_with_windows(
    ImGuiDockNode*  _node,
    ImGuiDockNode*& _last
)
{
    if (_node == nullptr)
    {
        return 0;
    }

    if (_node->IsLeafNode())
    {
        if (_node->Windows.Size > 0)
        {
            _last = _node;

            return 1;
        }

        return 0;
    }

    return ( leaves_with_windows(_node->ChildNodes[0], _last) +
             leaves_with_windows(_node->ChildNodes[1], _last) );
}

// refresh_root
//   a root's record of its middle, and of its one node with windows, made
// true again after a merge. ImGui makes it so once a frame; a split later
// in the same frame reads it, and would follow a node the merge freed.
void
refresh_root(
    ImGuiDockNode* _root
)
{
    ImGuiDockNode* last  = nullptr;
    const int      count = leaves_with_windows(_root, last);

    _root->CentralNode          = central(_root);
    _root->CountNodeWithWindows = count;
    _root->OnlyNodeWithWindows  = (count == 1) ? last : nullptr;

    return;
}

// repoint
//   every window docked by reference to a node -- not in it this frame, or
// never shown at all -- docked by reference to another: a node split has
// none of its own, and a window that points at one would float.
void
repoint(
    ImGuiID _from,
    ImGuiID _to
)
{
    ImGuiContext& context = *GImGui;

    for (ImGuiWindow* each : context.Windows)
    {
        if ( (each->DockNode == nullptr) &&
             (each->DockId == _from) )
        {
            each->DockId = _to;
        }
    }

    for (ImGuiWindowSettings* each = context.SettingsWindows.begin();
         each != nullptr;
         each = context.SettingsWindows.next_chunk(each))
    {
        if (each->DockId == _from)
        {
            each->DockId = _to;
        }
    }

    return;
}

/*
lay_out
  A dock space laid out as a new workspace's: the inspector down the right
the whole height, the tool windows below the rest, the symbols tree on the
left above them, and the documents in the middle, the central node. Each
line of the layout names a window and its place -- "left", "right", "top"
or "bottom"; "top" is the middle -- and every window named is docked there,
open or not, so one opened later opens in its place.
*/
void
lay_out(
    ImGuiID            _space,
    const ImVec2&      _size,
    const std::string& _layout
)
{
    ImGui::DockBuilderRemoveNode(_space);
    ImGui::DockBuilderAddNode(_space, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(_space,
                                  ImVec2(std::max(_size.x, 64.0f),
                                         std::max(_size.y, 64.0f)));

    ImGuiID middle = _space;
    ImGuiID right  = ImGui::DockBuilderSplitNode(middle, ImGuiDir_Right,
                                                 0.24f, nullptr, &middle);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(middle, ImGuiDir_Down,
                                                 0.34f, nullptr, &middle);
    ImGuiID left   = ImGui::DockBuilderSplitNode(middle, ImGuiDir_Left,
                                                 0.22f, nullptr, &middle);

    for (const std::string& line : lines(_layout))
    {
        const std::size_t tab   = line.find('\t');
        const std::string name  = line.substr(0, tab);
        const std::string place = (tab == std::string::npos)
                                      ? std::string("top")
                                      : line.substr(tab + 1);
        const ImGuiID     node  = (place == "left")   ? left
                                : (place == "right")  ? right
                                : (place == "bottom") ? bottom
                                                      : middle;

        ImGui::DockBuilderDockWindow(name.c_str(), node);
    }

    ImGui::DockBuilderFinish(_space);

    return;
}

/*
arrange
  The documents laid out afresh in the dock space's middle: each taken from
wherever it is docked and pointed at the middle, then -- for columns, rows
or a grid -- the middle split into a cell for each, the grid's rows as even
as their number allows and its last row the shortest. The windows that
shared the middle with them, the macros and the templates, stay in its
first cell. It waits until every document has been shown once, and says
whether it has laid them out.
*/
bool
arrange(
    ImGuiID            _space,
    const std::string& _documents,
    int                _way
)
{
    ImGuiContext&             context = *GImGui;
    ImGuiDockNode*            root    = ImGui::DockBuilderGetNode(_space);
    std::vector<ImGuiWindow*> windows;

    if (root == nullptr)
    {
        return false;
    }

    for (const std::string& name : lines(_documents))
    {
        ImGuiWindow* found = ImGui::FindWindowByName(name.c_str());

        // a document not shown yet: the arrangement waits for it
        if (found == nullptr)
        {
            return false;
        }

        windows.push_back(found);
    }

    if (windows.empty())
    {
        return true;
    }

    // every document to the middle: out of its node first -- which may
    // merge nodes, the middle among them -- then the middle found afresh
    for (ImGuiWindow* each : windows)
    {
        ImGuiDockNode* middle = central(root);

        if ( (middle == nullptr) ||
             (each->DockNode == middle) )
        {
            continue;
        }

        ImGui::DockContextProcessUndockWindow(&context, each, true);
        refresh_root(root);
        middle = root->CentralNode;

        if (middle != nullptr)
        {
            ImGui::SetWindowDock(each, middle->ID, ImGuiCond_Always);
        }
    }

    ImGuiDockNode* middle = central(root);

    if ( (_way <= 0)               ||
         (windows.size() < 2)      ||
         (middle == nullptr) )
    {
        ImGui::DockBuilderFinish(_space);

        return true;
    }

    // the cells: the first, top left, keeps what the middle held
    const int count   = static_cast<int>(windows.size());
    const int columns =
        (_way == 1) ? count
      : (_way == 2) ? 1
                    : static_cast<int>(std::ceil(std::sqrt(
                          static_cast<double>(count))));
    const int rows    = (count + columns - 1) / columns;
    const ImGuiID was = middle->ID;
    std::vector<ImGuiID> cells;
    ImGuiID              rest = was;

    for (int r = 0; r < rows; ++r)
    {
        ImGuiID line = rest;

        if (r < rows - 1)
        {
            ImGuiID below = 0;

            ImGui::DockBuilderSplitNode(
                rest, ImGuiDir_Down,
                static_cast<float>(rows - r - 1) /
                    static_cast<float>(rows - r),
                &below, &line);
            rest = below;
        }

        const int across = std::min(columns, count - r * columns);
        ImGuiID   left   = line;

        for (int c = 0; c < across; ++c)
        {
            ImGuiID cell = left;

            if (c < across - 1)
            {
                ImGuiID right = 0;

                ImGui::DockBuilderSplitNode(
                    left, ImGuiDir_Right,
                    static_cast<float>(across - c - 1) /
                        static_cast<float>(across - c),
                    &right, &cell);
                left = right;
            }

            cells.push_back(cell);
        }
    }

    // what pointed at the middle points at its first cell; each document
    // goes to its own
    repoint(was, cells.front());

    for (std::size_t i = 0; i < windows.size(); ++i)
    {
        ImGui::SetWindowDock(windows[i], cells[i], ImGuiCond_Always);
    }

    ImGui::DockBuilderFinish(_space);

    return true;
}

// choose_tab
//   a docked window's tab chosen in its dock, from the next frame.
void
choose_tab(
    ImGuiWindow* _window
)
{
    if ( (_window == nullptr)                  ||
         (_window->DockNode == nullptr)        ||
         (_window->DockNode->TabBar == nullptr) )
    {
        return;
    }

    ImGuiTabBar*  bar = _window->DockNode->TabBar;
    ImGuiTabItem* tab = ImGui::TabBarFindTabByID(bar, _window->TabId);

    if (tab != nullptr)
    {
        ImGui::TabBarQueueFocus(bar, tab);
    }

    return;
}

// dock_of
//   a dock space's id, as an attribute carries it: decimal text.
ImGuiID
dock_of(
    const option_set& _attrs,
    const char*       _key
)
{
    const std::string text = _attrs.as_string(_key, "0");

    return static_cast<ImGuiID>(std::strtoul(text.c_str(), nullptr, 10));
}

// space_times
//   struct: when a dock space began to be shown without a break, when it
// was last shown, and when it was last laid out afresh.
struct space_times
{
    int since = -1;
    int last  = -1;
    int laid  = -1;
};

// spaces
//   each dock space's times, by its id, in the ImGui drawing it.
std::unordered_map<ImGuiID, space_times>&
spaces()
{
    static std::unordered_map<const ImGuiContext*,
                              std::unordered_map<ImGuiID, space_times>> held;

    return held[ImGui::GetCurrentContext()];
}

// SETTLING
//   constant: how many frames a dock space is shown, after it appears or
// is laid out afresh, before its nodes' sizes are its windows' -- the first
// frames size its nodes before the windows docked in them are drawn.
const int SETTLING = 2;

// settled
//   whether a dock space's nodes have their sizes: shown long enough since
// it appeared and since it was laid out.
bool
settled(
    ImGuiID _space
)
{
    const space_times& times = spaces()[_space];
    const int          now   = ImGui::GetFrameCount();

    return ( (times.since >= 0)                 &&
             (now - times.since >= SETTLING)    &&
             (now - times.laid >= SETTLING) );
}

// sighting
//   struct: when a dock window last drew its content, and in which ImGui:
// a window of one context is no window of another's.
struct sighting
{
    const ImGuiContext* context = nullptr;
    int                 frame   = -1;
};

// seen
//   each dock window's last sighting, by its key.
std::unordered_map<std::string, sighting>&
seen()
{
    static std::unordered_map<std::string, sighting> held;

    return held;
}

// in_front
//   the key of the dock window that last took the keyboard, by the ImGui it
// took it in.
std::string&
in_front()
{
    static std::unordered_map<const ImGuiContext*, std::string> held;

    return held[ImGui::GetCurrentContext()];
}

// zoomed
//   struct: the font size and spacings a window's content is drawn at, its
// zoom applied, pushed while it draws and popped after.
struct zoomed
{
    bool pushed = false;

    explicit zoomed(
        float _zoom
    )
    {
        if (std::fabs(_zoom - 1.0f) < 0.001f)
        {
            return;
        }

        const ImGuiStyle& style = ImGui::GetStyle();

        ImGui::PushFont(nullptr, font_base() * _zoom);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            ImVec2(style.FramePadding.x * _zoom,
                                   style.FramePadding.y * _zoom));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                            ImVec2(style.ItemSpacing.x * _zoom,
                                   style.ItemSpacing.y * _zoom));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing,
                            ImVec2(style.ItemInnerSpacing.x * _zoom,
                                   style.ItemInnerSpacing.y * _zoom));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,
                            ImVec2(style.CellPadding.x * _zoom,
                                   style.CellPadding.y * _zoom));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                            ImVec2(style.WindowPadding.x * _zoom,
                                   style.WindowPadding.y * _zoom));
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing,
                            style.IndentSpacing * _zoom);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,
                            style.FrameRounding * _zoom);
        pushed = true;
    }

    ~zoomed()
    {
        if (pushed)
        {
            ImGui::PopStyleVar(7);
            ImGui::PopFont();
        }
    }

    zoomed(const zoomed&)            = delete;
    zoomed& operator=(const zoomed&) = delete;
};

}  // anonymous namespace


/*
draw_add_new
  The "+" is a small button, as the add buttons above the tree are; the
words beside it are an invisible button of their size with the words drawn
over it, so a press on them is a press of the "+" too. They are the
theme's text, faded, and whole under the mouse.
*/
void
draw_add_new(
    const option_set& _attrs,
    imgui_frame&      _frame
)
{
    const std::string action = _attrs.as_string("action", "context.add");
    bool              chosen = ImGui::SmallButton("+");

    tip(_attrs);
    ImGui::SameLine(0.0f, ADD_NEW_GAP * ui_scale());

    const ImVec2 at   = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImGui::CalcTextSize(ADD_NEW_LABEL);

    chosen = ( (ImGui::InvisibleButton("##add_new", size)) ||
               (chosen) );

    const bool hovered = ImGui::IsItemHovered();
    ImVec4     ink     = ImGui::GetStyleColorVec4(ImGuiCol_Text);

    ink.w *= hovered ? 1.0f : ADD_NEW_FADE;

    ImGui::GetWindowDrawList()->AddText(at,
                                        ImGui::GetColorU32(ink),
                                        ADD_NEW_LABEL);

    if (hovered)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    tip(_attrs);

    // either part reports the action, passing any value through
    if ( (chosen) &&
         (!action.empty()) )
    {
        option_set                      payload;
        const ::djinterp::option_value* value = _attrs.find("value");

        if (value != nullptr)
        {
            payload.set("value", *value);
        }

        _frame.post(action, std::move(payload));
    }

    return;
}

/*
draw_add_control
  A small button as the add buttons always were. As a menu: "+ Add", and
its press opens the menu. Split: "+" and the kind it adds, and beside it,
flush, an arrow that opens the menu -- where a kind chosen is added, and is
what the face adds from then on. The menu lists every kind, a group last,
and then the colour new rows take: none, the palette's, or one colour for
every new row, which a swatch there chooses.
  Each part is kept where it was drawn, under the control's key -- for a
test that drives it.
*/
void
draw_add_control(
    const option_set& _attrs,
    imgui_frame&      _frame
)
{
    const bool        split  = (_attrs.as_string("style", "menu") ==
                                "split");
    const int         kind   = static_cast<int>(_attrs.as_long("kind", 0));
    const long        colour = _attrs.as_long("colour", 0);
    const std::string key    = _attrs.as_string("key", "add");
    bool              open   = false;

    if (split)
    {
        const std::string face = std::string("+ ") + add_kind_name(kind);

        if (ImGui::SmallButton(face.c_str()))
        {
            option_set payload;

            payload.set("value", static_cast<long>(add_kind_of(kind)));
            _frame.post("edit.add", std::move(payload));
        }

        spot(key);
        tip(_attrs);
        ImGui::SameLine(0.0f, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            ImVec2(ImGui::GetStyle().FramePadding.x, 0.0f));
        open = ImGui::ArrowButtonEx("##add.more",
                                    ImGuiDir_Down,
                                    ImVec2(ImGui::GetFontSize(),
                                           ImGui::GetFontSize()));
        ImGui::PopStyleVar();
        spot(key + ".more");

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::SetTooltip("add something else");
        }
    }
    else
    {
        open = ImGui::SmallButton("+ Add");
        spot(key);
        tip(_attrs);
    }

    if (open)
    {
        ImGui::OpenPopup("##add.menu");
    }

    if (!ImGui::BeginPopup("##add.menu"))
    {
        return;
    }

    ImGui::TextDisabled("add");

    for (int i = 0; i < ADD_KINDS; ++i)
    {
        const int made = add_kind_of(i);

        if (made < 0)
        {
            ImGui::Separator();
        }

        const bool chosen = ImGui::MenuItem(add_kind_name(i),
                                            nullptr,
                                            (split) && (i == kind));

        spot(key + ".kind." + std::to_string(i));

        if (chosen)
        {
            option_set payload;

            payload.set("value", static_cast<long>(made));
            _frame.post("edit.add", std::move(payload));

            // the split button adds this kind from now on
            if ( (split) &&
                 (i != kind) )
            {
                option_set choice;

                choice.set("target", std::string("environment.add_kind"));
                choice.set("value", static_cast<long>(i));
                _frame.post("set", std::move(choice));
            }
        }
    }

    ImGui::Separator();

    if (ImGui::BeginMenu("New rows' colour"))
    {
        // WAYS
        //   constant: the colours new rows may take, as new_colour numbers
        // them.
        static const char* const WAYS[] = {
            "None: their bits drawn plain",
            "Each its own, from the palette",
            "The colour the Options choose" };

        for (long i = 0; i < 3; ++i)
        {
            const bool chosen = ImGui::MenuItem(WAYS[i], nullptr, colour == i);

            spot(key + ".colour." + std::to_string(i));

            if ( (chosen) &&
                 (colour != i) )
            {
                option_set choice;

                choice.set("target", std::string("environment.new_colour"));
                choice.set("value", i);
                _frame.post("set", std::move(choice));
            }
        }

        // the one colour: chosen here, it is what every new row takes
        const rgba packed = static_cast<rgba>(_attrs.as_long("rgba",
                                                             0x4C8DFFFFL));
        float      shade[4] = { channel(packed, 0), channel(packed, 1),
                                channel(packed, 2), channel(packed, 3) };

        ImGui::Separator();

        const bool picked = ImGui::ColorEdit4("the colour##add.rgba",
                                              shade,
                                              ImGuiColorEditFlags_NoInputs |
                                              ImGuiColorEditFlags_AlphaBar);

        spot(key + ".rgba");

        if (picked)
        {
            option_set choice;

            choice.set("target", std::string("environment.new_rgba"));
            choice.set("r", static_cast<double>(shade[0]));
            choice.set("g", static_cast<double>(shade[1]));
            choice.set("b", static_cast<double>(shade[2]));
            choice.set("a", static_cast<double>(shade[3]));
            _frame.post("set", std::move(choice));

            if (colour != 2)
            {
                option_set way;

                way.set("target", std::string("environment.new_colour"));
                way.set("value", 2L);
                _frame.post("set", std::move(way));
            }
        }

        ImGui::EndMenu();
    }

    spot(key + ".colours");

    ImGui::EndPopup();

    return;
}

/*
draw_add_child
  One line, about as tall as a tree row, where the rows above it put their
names: a + that opens the Add node window for the group or range -- what
"node_action" posts, with its id -- and the field, as wide as what it holds
or, with "fill", to the panel's edge -- or "width", the room the caller
gives it, + and all. "plus" and "field" say which of the two are drawn.
Enter posts what was typed -- unless it is blank -- and lets the text go;
the window asks for the keyboard back, through "focus", with "text" to
start from. Escape clears what was typed, and leaves the field when there
is nothing to clear.
*/
void
draw_add_child(
    const option_set& _attrs,
    imgui_frame&      _frame
)
{
    const long        parent = _attrs.as_long("value", 0);
    const std::string action = _attrs.as_string("action", "tree.add_child");
    const std::string where  = _attrs.as_string("where", "tree");
    const bool        plus   = _attrs.as_bool("plus", false);
    const bool        field  = _attrs.as_bool("field", true);
    const bool        fill   = _attrs.as_bool("fill", true);
    const ImGuiID     id     = ImGui::GetID("##add_child");
    const auto        found  = drafts().find(id);
    std::string       text   = (found != drafts().end()) ? found->second
                                                         : std::string();

    // the window gives the field the keyboard, and the text to start from
    if ( (field) &&
         (_attrs.as_bool("focus", false)) )
    {
        text = _attrs.as_string("text", "");
        ImGui::SetKeyboardFocusHere(plus ? 1 : 0);
    }

    // in line with the rows above it: where their swatches and names begin
    // -- in the tree; the grid places the field itself
    if (_attrs.as_bool("indent", true))
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                             ImGui::GetTreeNodeToLabelSpacing());
    }

    const ImGuiStyle& style = ImGui::GetStyle();
    const float       pad_y = std::max(0.0f, ui_scale());
    const float       tall  = ImGui::GetTextLineHeight() + 2.0f * pad_y;
    const double      given = _attrs.as_double("width", -1.0);
    float             room  = (given > 0.0)
                                  ? static_cast<float>(given)
                                  : ImGui::GetContentRegionAvail().x;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(style.FramePadding.x, pad_y));

    // the +: the Add node window, for this group or range
    if (plus)
    {
        if (ImGui::Button("+##add_node", ImVec2(tall, tall)))
        {
            option_set payload;

            payload.set("value", parent);
            _frame.post(_attrs.as_string("node_action", "node.open"),
                        std::move(payload));
        }

        if ( (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) &&
             (!ImGui::IsItemActive()) )
        {
            ImGui::SetTooltip("Add node...: any kind of row, made in full");
        }

        spot("add_child." + where + ".plus");
        room -= tall + style.ItemSpacing.x * 0.5f;

        if (field)
        {
            ImGui::SameLine(0.0f, style.ItemSpacing.x * 0.5f);
        }
    }

    if (!field)
    {
        ImGui::PopStyleVar();

        return;
    }

    // as wide as what it holds -- the hint, or the text and a letter more
    // -- unless it fills the room
    const float fitted = std::max(ImGui::CalcTextSize(ADD_CHILD_HINT).x,
                                  ImGui::CalcTextSize(text.c_str()).x +
                                      ImGui::CalcTextSize("M").x) +
                         style.FramePadding.x * 2.0f;

    ImGui::SetNextItemWidth(std::max(1.0f, fill ? room
                                                : std::min(room, fitted)));

    const bool entered = ImGui::InputTextWithHint(
        "##add_child",
        ADD_CHILD_HINT,
        &text,
        ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_EscapeClearsAll);

    ImGui::PopStyleVar();

    // where it is, and where it ends: for a test
    spot("add_child." + where + ".field");
    spots()["add_child." + where + ".end"] =
        spotted{ ImGui::GetCurrentContext(),
                 ImGui::GetFrameCount(),
                 ImVec2(ImGui::GetItemRectMax().x,
                        (ImGui::GetItemRectMin().y +
                         ImGui::GetItemRectMax().y) * 0.5f) };

    if (!ImGui::IsItemActive())
    {
        tip(_attrs);
    }

    // a name, posted; what is typed, kept until it is
    if ( (entered) &&
         (!blank(text)) )
    {
        option_set payload;

        payload.set("value", parent);
        payload.set("text", text);
        payload.set("where", where);
        _frame.post(action, std::move(payload));
        text.clear();
    }

    if (text.empty())
    {
        drafts().erase(id);
    }
    else
    {
        drafts()[id] = text;
    }

    return;
}

/*
draw_action_list
  The console's actions: how many so far, then a table that fills what is
left of its window and scrolls, the latest first -- its heading kept in
view, and only the lines in view drawn, so two thousand cost what thirty
do. The time, the sheet and the times over are faded; the action and what
it carried are not.
*/
void
draw_action_list(
    const std::vector<action_entry>& _actions
)
{
    const ImVec4 faded = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);

    ImGui::PushStyleColor(ImGuiCol_Text, faded);

    if (_actions.empty())
    {
        ImGui::TextUnformatted("Nothing done yet. Every action you take is "
                               "listed here, the latest first.");
    }
    else
    {
        ImGui::Text("%lu action(s) so far, the latest first -- as the "
                    "window heard them: each a name, and what it carried.",
                    static_cast<unsigned long>(_actions.size()));
    }

    ImGui::PopStyleColor();
    spot("console.count");

    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg          |
                                  ImGuiTableFlags_BordersInnerH  |
                                  ImGuiTableFlags_ScrollY        |
                                  ImGuiTableFlags_Resizable      |
                                  ImGuiTableFlags_SizingFixedFit;

    if (!ImGui::BeginTable("##actions", 5, flags, ImVec2(0.0f, 0.0f)))
    {
        return;
    }

    const float scale = ui_scale();

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthFixed,
                            70.0f * scale);
    ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthFixed,
                            150.0f * scale);
    ImGui::TableSetupColumn("details", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("sheet", ImGuiTableColumnFlags_WidthFixed,
                            190.0f * scale);
    ImGui::TableSetupColumn("##times", ImGuiTableColumnFlags_WidthFixed,
                            44.0f * scale);
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;

    clipper.Begin(static_cast<int>(_actions.size()));

    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
        {
            const action_entry& each =
                _actions[_actions.size() - 1 - static_cast<std::size_t>(i)];

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(faded, "%s", each.when.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(each.kind.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(each.detail.empty() ? "--"
                                                       : each.detail.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::TextColored(faded, "%s", each.sheet.c_str());
            ImGui::TableSetColumnIndex(4);

            if (each.times > 1)
            {
                ImGui::TextColored(faded, "x%d", each.times);
            }
        }
    }

    ImGui::EndTable();

    return;
}

/*
draw_status_bar
  A bar along the window's foot, below the dock space -- ImGui's side bar of
the viewport, which takes its height from the room the windows are laid
out in. What the sheet in front is on the left, faded; what the window
last said at the right end, after a "*" while the sheet has changes: a
click there opens the console; and before the sheet, the grid's End mode
and Extend mode while they are on. What is on the left is cut short of it.
*/
void
draw_status_bar(
    const option_set& _attrs,
    imgui_frame&      _frame
)
{
    ImGuiViewport*         viewport = ImGui::GetMainViewport();
    const ImGuiWindowFlags flags    = ImGuiWindowFlags_NoScrollbar      |
                                      ImGuiWindowFlags_NoSavedSettings  |
                                      ImGuiWindowFlags_MenuBar;

    if ( (ImGui::BeginViewportSideBar("##bitwisedom.status",
                                      viewport,
                                      ImGuiDir_Down,
                                      ImGui::GetFrameHeight(),
                                      flags)) &&
         (ImGui::BeginMenuBar()) )
    {
        const ImGuiStyle& style   = ImGui::GetStyle();
        const bool        dirty   = _attrs.as_bool("dirty", false);
        const bool        failed  = _attrs.as_bool("error", false);
        std::string       message = _attrs.as_string("right", "");
        const std::string summary = _attrs.as_string("left", "");

        if (message.empty())
        {
            message = "Ready";
        }

        const std::string right  = (dirty ? "*  " : "") + message;
        const float       wide   = ImGui::CalcTextSize(right.c_str()).x;
        const float       ending = ImGui::GetWindowWidth() -
                                   style.WindowPadding.x -
                                   style.ItemSpacing.x;
        const float       starts = std::max(ImGui::GetCursorPosX(),
                                            ending - wide);

        // what the sheet is, cut short of the message
        std::string left  = summary;
        const float room  = starts - ImGui::GetCursorPosX() -
                            style.ItemSpacing.x * 2.0f;

        while ( (!left.empty()) &&
                (ImGui::CalcTextSize(left.c_str()).x > room) )
        {
            left.pop_back();
        }

        // the grid's modes first, as Excel's status bar shows them: End
        // Mode, Extend Selection
        const std::string mode = _attrs.as_string("mode", "");

        if (!mode.empty())
        {
            ImGui::TextUnformatted(mode.c_str());
            ImGui::SameLine(0.0f, style.ItemSpacing.x * 3.0f);

            const float used = ImGui::CalcTextSize(mode.c_str()).x +
                               style.ItemSpacing.x * 3.0f;

            while ( (!left.empty()) &&
                    (ImGui::CalcTextSize(left.c_str()).x > room - used) )
            {
                left.pop_back();
            }
        }

        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImGui::GetStyleColorVec4(
                                  ImGuiCol_TextDisabled));
        ImGui::TextUnformatted(left.c_str());
        ImGui::PopStyleColor();

        // the message, at the right end: the console, on a click
        ImGui::SameLine(starts);
        ImGui::PushStyleColor(ImGuiCol_Text,
                              failed ? ImVec4(0.94f, 0.36f, 0.33f, 1.0f)
                                     : ImGui::GetStyleColorVec4(
                                           ImGuiCol_Text));

        if (ImGui::Selectable((right + "##status.message").c_str(),
                              false,
                              ImGuiSelectableFlags_None,
                              ImVec2(wide, 0.0f)))
        {
            option_set payload;

            payload.set("value", _attrs.as_long("value", 0));
            _frame.post(_attrs.as_string("action", "tool.open"),
                        std::move(payload));
        }

        ImGui::PopStyleColor();
        spot("status.message");
        tip(_attrs);

        ImGui::EndMenuBar();
    }

    ImGui::End();

    return;
}

/*
draw_menu_bar_rest
  The trailing menus' room is worked out before anything is drawn, from
their titles: a menu in a bar takes its title's width and two spacings,
the title half a spacing in, and its highlight the whole of it, centred on
the title. Their span stops half a spacing past the last title, so that
title ends a margin and half a spacing from the bar's right edge -- as the
first menu's begins from its left. The leading items are drawn after the
indent, clipped a spacing short of the first trailing menu; the trailing
menus, where the room begins. The cursor is moved only when something is
drawn after it, as ImGui asks.
*/
void
draw_menu_bar_rest(
    const option_set&     _attrs,
    const blueprint_list& _children,
    imgui_frame&          _frame
)
{
    const ImGuiStyle& style    = ImGui::GetStyle();
    const float       spacing  = style.ItemSpacing.x;
    const long        asked    = std::max(0L, _attrs.as_long("trailing", 0));
    const std::size_t trailing = std::min(static_cast<std::size_t>(asked),
                                          _children.size());
    const std::size_t leading  = _children.size() - trailing;
    float             span     = 0.0f;

    for (std::size_t i = leading; i < _children.size(); ++i)
    {
        span += ImGui::CalcTextSize(_children[i].text.c_str()).x +
                spacing * 2.0f;
    }

    // the span ends half a spacing past the last title: a spacing short of
    // that menu's share
    if (trailing > 0)
    {
        span -= spacing;
    }

    const float  margin = std::max(style.WindowPadding.x,
                                   style.ItemSpacing.x);
    const float  start  = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x -
                          margin - span;
    const ImVec2 cursor = ImGui::GetCursorScreenPos();

    // the leading items, after the indent, cut short of the trailing menus
    if (leading > 0)
    {
        const float indent = static_cast<float>(
                                 _attrs.as_double("indent", 0.0)) *
                             ui_scale();

        ImGui::SetCursorScreenPos(ImVec2(cursor.x + indent, cursor.y));
        ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX),
                            ImVec2(start - spacing, FLT_MAX),
                            true);
        draw_children(_children, 0, leading, _frame);
        ImGui::PopClipRect();
    }

    // the trailing menus, at the bar's right end
    if (trailing > 0)
    {
        ImGui::SetCursorScreenPos(ImVec2(std::max(start, cursor.x),
                                         cursor.y));
        draw_children(_children, leading, _children.size(), _frame);
    }

    return;
}

grid_point
control_spot(
    const std::string& _key
)
{
    const auto found = spots().find(_key);

    if ( (found == spots().end())                                ||
         (found->second.context != ImGui::GetCurrentContext())   ||
         (ImGui::GetFrameCount() - found->second.frame > 1) )
    {
        return grid_point{};
    }

    return grid_point{ true,
                       found->second.middle.x,
                       found->second.middle.y };
}

bool
window_seen(
    const std::string& _key
)
{
    const auto found = seen().find(_key);

    return ( (found != seen().end())                                 &&
             (found->second.context == ImGui::GetCurrentContext())   &&
             (ImGui::GetFrameCount() - found->second.frame <= 1) );
}

// PIN_ROOM
//   what a pinned tab's name begins with: room for its pin.
const char* const PIN_ROOM = "      ";

// tab_drafts
//   what is being typed over a window's tab, renaming it, by the window's
// key.
std::map<std::string, std::string>&
tab_drafts()
{
    static std::map<std::string, std::string> held;

    return held;
}

// BITWISEDOM_TAB_PLACES
//   whether uxoxo's Dear ImGui lets a dock node's tab bar begin and end
// inset, and keep its tabs at its right end: its patch's second form.
#if defined(IMGUI_DOCK_TAB_BAR_INSET) && (IMGUI_DOCK_TAB_BAR_INSET >= 2)
#define BITWISEDOM_TAB_PLACES 1
#else
#define BITWISEDOM_TAB_PLACES 0
#endif

// tab_place
//   struct: where a dock node's tab bar is to be, from the node's left
// edge: from `from` to `to` -- 0 for the node's own right edge -- its tabs
// at its left end or, `right`, its right; and whether the document asking
// was the node's window in view, whose say wins.
struct tab_place
{
    float from    = 0.0f;
    float to      = 0.0f;
    bool  right   = false;
    bool  in_view = false;
};

// tab_places
//   each dock node's tab bar, by the node's id, as the documents docked
// there asked in the frame before, kept until the dock space is next drawn.
std::map<ImGuiID, tab_place>&
tab_places()
{
    static std::map<ImGuiID, tab_place> held;

    return held;
}

// set_tab_places
//   every dock node's tab bar where its documents asked, and the tabs of a
// node no document asked for across its width, at its left; the requests
// then let go, for the next frame's. And each node's pinned tabs before
// the rest: one moved a frame, as ImGui moves tabs.
void
set_tab_places(
    const std::vector<std::string>& _pinned
)
{
    ImGuiContext& context = *GImGui;

    for (ImGuiStoragePair& each : context.DockContext.Nodes.Data)
    {
        ImGuiDockNode* node = static_cast<ImGuiDockNode*>(each.val_p);

        if (node == nullptr)
        {
            continue;
        }

#if BITWISEDOM_TAB_PLACES
        const auto found = tab_places().find(node->ID);
        const bool asked = (found != tab_places().end());

        node->TabBarInsetLeft  = asked ? found->second.from : 0.0f;
        node->TabBarInsetRight = asked ? found->second.to : 0.0f;
        node->TabBarAlignRight = ( (asked) && (found->second.right) );
#endif

        // the pinned before the rest: the first pinned tab after a tab not
        // pinned moved to where that one is
        ImGuiTabBar* bar = node->TabBar;

        if ( (bar == nullptr) ||
             (_pinned.empty()) ||
             (bar->ReorderRequestTabId != 0) )
        {
            continue;
        }

        // pinned
        //   whether a tab's window is one of those pinned, by its key.
        auto pinned = [&_pinned](const ImGuiTabItem& _tab)
        {
            if (_tab.Window == nullptr)
            {
                return false;
            }

            const char* name = std::strstr(_tab.Window->Name, "###");

            return ( (name != nullptr) &&
                     (std::find(_pinned.begin(), _pinned.end(),
                                std::string(name + 3)) != _pinned.end()) );
        };

        int loose = -1;

        for (int i = 0; i < bar->Tabs.Size; ++i)
        {
            const bool held = pinned(bar->Tabs[i]);

            if ( (!held) &&
                 (loose < 0) )
            {
                loose = i;
            }
            else if ( (held) &&
                      (loose >= 0) )
            {
                ImGui::TabBarQueueReorder(bar, &bar->Tabs[i], loose - i);

                break;
            }
        }
    }

    tab_places().clear();

    return;
}

// tab_keys
//   the keys of the windows a dock node's tabs show, in the order the tabs
// are in: what follows "###" in each window's name.
std::vector<std::string>
tab_keys(
    const ImGuiDockNode* _node
)
{
    std::vector<std::string> out;

    if ( (_node == nullptr) ||
         (_node->TabBar == nullptr) )
    {
        return out;
    }

    for (const ImGuiTabItem& each : _node->TabBar->Tabs)
    {
        const char* name = (each.Window != nullptr)
                               ? std::strstr(each.Window->Name, "###")
                               : nullptr;

        if (name != nullptr)
        {
            out.emplace_back(name + 3);
        }
    }

    return out;
}

// draw_pin
//   a pushpin, its head up and to the right, its point down and to the
// left, centred at _at.
void
draw_pin(
    ImDrawList* _list,
    ImVec2      _at,
    float       _size,
    ImU32       _colour
)
{
    const float  r    = _size * 0.24f;
    const ImVec2 head(_at.x + _size * 0.18f, _at.y - _size * 0.18f);
    const ImVec2 base(_at.x - _size * 0.02f, _at.y + _size * 0.02f);
    const ImVec2 tip(_at.x - _size * 0.36f, _at.y + _size * 0.36f);

    _list->AddCircleFilled(head, r, _colour);
    _list->AddLine(ImVec2(head.x - r * 1.3f, head.y - r * 0.2f),
                   ImVec2(head.x + r * 0.2f, head.y + r * 1.3f),
                   _colour, std::max(1.5f, _size * 0.12f));
    _list->AddLine(base, tip, _colour, std::max(1.0f, _size * 0.08f));

    return;
}

/*
draw_dock_space
  The workspace's dock space, laid out first when it is new or its layout
was reset, and its documents arranged when asked -- "window.arranged"
posted once they are: both before the dock space is drawn, as ImGui asks
of its dock builder. Each node's tabs are over the columns its documents
asked for the frame before, and its pinned tabs -- "pinned", their keys a
line each -- before the rest. It fills what is left of the window.
*/
void
draw_dock_space(
    const option_set& _attrs,
    imgui_frame&      _frame
)
{
    const ImGuiID space = dock_of(_attrs, "id");

    if (space == 0)
    {
        return;
    }

    space_times& times = spaces()[space];
    const int    now   = ImGui::GetFrameCount();

    if (times.last != now - 1)
    {
        times.since = now;
    }

    times.last = now;

    {
        std::vector<std::string> pinned;

        for (const std::string& each : lines(_attrs.as_string("pinned", "")))
        {
            pinned.push_back(each);
        }

        set_tab_places(pinned);
    }

    if ( (ImGui::DockBuilderGetNode(space) == nullptr) ||
         (_attrs.as_bool("reset", false)) )
    {
        lay_out(space,
                ImGui::GetContentRegionAvail(),
                _attrs.as_string("layout", ""));
        times.laid = now;
    }

    const long way = _attrs.as_long("arrange", -1);

    // laid out, once the nodes have their sizes, it says so: the request
    // lasts until then
    if ( (way >= 0)         &&
         (settled(space))   &&
         (arrange(space,
                  _attrs.as_string("documents", ""),
                  static_cast<int>(way))) )
    {
        _frame.post("window.arranged", option_set{});
    }

    ImGui::DockSpace(space, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    return;
}

/*
draw_dock_window
  The window's docking is asked for before it begins: beside the window it
is told to dock with, or in the middle, the first time it is shown; at
once, when it has just been moved to this workspace. A document's window
is drawn flush, its grid scrolling itself, and marked unsaved -- a dot on
its tab -- while its sheet has changes. Its content is drawn at its zoom,
into a frame of its own whose events -- and the window's own -- are passed
on with its document's id.
*/
void
draw_dock_window(
    const option_set&     _attrs,
    const blueprint_list& _children,
    imgui_frame&          _frame
)
{
    const std::string key      = _attrs.as_string("key", "window");
    const bool        pinned   = _attrs.as_bool("pinned", false);
    const std::string title    = _attrs.as_string("title", key);
    const std::string name     = (pinned ? std::string(PIN_ROOM)
                                         : std::string()) +
                                 title + "###" + key;
    const ImGuiID     space    = dock_of(_attrs, "dockspace");
    const long        document = _attrs.as_long("document", 0);
    const bool        dockable = _attrs.as_bool("dockable", true);
    const bool        placed   = _attrs.as_bool("place", false);
    const float       zoom     = std::clamp(
        static_cast<float>(_attrs.as_double("zoom", 1.0)), 0.25f, 4.0f);
    const std::string closing  = _attrs.as_string("close_action", "");
    imgui_frame       inner;

    // where it docks when it is first shown, or has just been moved here
    if ( (dockable) &&
         (space != 0) &&
         ( (placed) ||
           (ImGui::FindWindowByName(name.c_str()) == nullptr) ) )
    {
        const std::string with   = _attrs.as_string("dock_with", "");
        ImGuiID           target = 0;

        if (!with.empty())
        {
            const ImGuiWindow* beside =
                ImGui::FindWindowByName(("###" + with).c_str());

            if ( (beside != nullptr) &&
                 (beside->DockNode != nullptr) )
            {
                target = beside->DockNode->ID;
            }
        }

        if (target == 0)
        {
            ImGuiDockNode* middle = central(ImGui::DockBuilderGetNode(space));

            target = (middle != nullptr) ? middle->ID : space;
        }

        ImGui::SetNextWindowDockID(target, placed ? ImGuiCond_Always
                                                  : ImGuiCond_FirstUseEver);
    }

    // a window of its own: the middle of the window, at the size asked
    if (!dockable)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const float          scale    = ui_scale();

        ImGui::SetNextWindowPos(viewport->GetCenter(),
                                ImGuiCond_FirstUseEver,
                                ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(
            ImVec2(static_cast<float>(_attrs.as_double("width", 760.0)) *
                       scale,
                   static_cast<float>(_attrs.as_double("height", 520.0)) *
                       scale),
            ImGuiCond_FirstUseEver);
    }

    // brought to the front: the keyboard given it, and its tab chosen in
    // its dock -- ImGui chooses a docked window's tab by the keyboard only
    // while nothing else takes it in the same frame
    const bool raised = _attrs.as_bool("focus", false);

    if (raised)
    {
        choose_tab(ImGui::FindWindowByName(name.c_str()));
        ImGui::SetNextWindowFocus();
    }

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoFocusOnAppearing;

    if (!_attrs.as_bool("scroll", true))
    {
        flags |= ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse;
    }

    if (_attrs.as_bool("hscroll", false))
    {
        flags |= ImGuiWindowFlags_HorizontalScrollbar;
    }

    if (!dockable)
    {
        flags |= ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse;
        flags &= ~ImGuiWindowFlags_NoFocusOnAppearing;
    }

    if (_attrs.as_bool("unsaved", false))
    {
        flags |= ImGuiWindowFlags_UnsavedDocument;
    }

    const bool flush = _attrs.as_bool("flush", false);
    bool       open  = true;

    if (flush)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    }

    // a window of its own stands out from the docked ones beneath it, as a
    // dialog does
    if (!dockable)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg,
                              ImGui::GetStyleColorVec4(ImGuiCol_PopupBg));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    }

    // pinned, it has no x: it is closed from its menu
    const bool shown = ImGui::Begin(name.c_str(),
                                    ( (closing.empty()) || (pinned) )
                                        ? nullptr
                                        : &open,
                                    flags);

    // its tab, docked, as its dock node laid it out this frame. A
    // double-click on it asks to rename what the window shows; a
    // right-click, for the tab's menu -- with every tab of the node, in
    // order
    ImGuiWindow*      window  = ImGui::GetCurrentWindow();
    const bool        docked  = ( (window->DockIsActive) &&
                                  (window->DockNode != nullptr) &&
                                  (!window->DockNode->IsHiddenTabBar()) &&
                                  (!window->DockNode->IsNoTabBar()) );
    const ImRect      tab     = window->DC.DockTabItemRect;
    const ImGuiIO&    io      = ImGui::GetIO();
    const std::string renames = _attrs.as_string("rename_action", "");
    const std::string menus   = _attrs.as_string("tab_menu_action", "");

    if ( (docked)           &&
         (!renames.empty()) &&
         (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) &&
         (tab.Contains(io.MouseClickedPos[ImGuiMouseButton_Left])) )
    {
        option_set payload;

        payload.set("value", _attrs.as_long("value", 0));
        inner.post(renames, std::move(payload));
    }

    if ( (docked)         &&
         (!menus.empty()) &&
         (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) &&
         (tab.Contains(io.MouseClickedPos[ImGuiMouseButton_Right])) )
    {
        std::string every;

        for (const std::string& each : tab_keys(window->DockNode))
        {
            every += each + "\n";
        }

        option_set payload;

        payload.set("value", _attrs.as_long("value", 0));
        payload.set("key", key);
        payload.set("title", title);
        payload.set("tabs", every);
        inner.post(menus, std::move(payload));
    }

    if (docked)
    {
        spots()[key + ".tab"] = spotted{ ImGui::GetCurrentContext(),
                                         ImGui::GetFrameCount(),
                                         tab.GetCenter() };
    }

    // pinned: a pin in the room its name leaves at its tab's left, drawn
    // over the tab where the dock node drew it
    if ( (docked) &&
         (pinned) &&
         (window->DockNode->HostWindow != nullptr) )
    {
        ImDrawList* list = window->DockNode->HostWindow->DrawList;
        const float room = ImGui::CalcTextSize(PIN_ROOM).x;
        const float size = std::min(ImGui::GetFontSize() * 0.85f,
                                    room * 0.85f);

        list->PushClipRect(tab.Min, tab.Max, false);
        draw_pin(list,
                 ImVec2(tab.Min.x + ImGui::GetStyle().FramePadding.x +
                            room * 0.45f,
                        tab.GetCenter().y),
                 size,
                 ImGui::GetColorU32(ImGuiCol_Text));
        list->PopClipRect();
    }

    if (!dockable)
    {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    if (flush)
    {
        ImGui::PopStyleVar();
    }

    // a window docked by this Begin, its first, has its tab only now
    if (raised)
    {
        choose_tab(ImGui::GetCurrentWindow());
    }

    // a document's tab row: its node's tabs over the grid's columns the
    // preferences name -- every one, the bits, the symbols column or the
    // value column -- at their left or their right, from the next frame;
    // the grid's ground up through the row either side of them. The
    // document in view has the say, where a node holds several
#if BITWISEDOM_TAB_PLACES
    if ( (docked) &&
         (_attrs.as_bool("tab_row", false)) )
    {
        ImGuiDockNode*          node  = window->DockNode;
        const grid_columns_seen seen  = grid_columns(
                                            static_cast<int>(document));
        const long              over  = _attrs.as_long("tabs_over", 0);
        const float             left  = node->Pos.x;
        const float             right = node->Pos.x + node->Size.x;
        float                   from  = left;
        float                   to    = right;

        if ( (seen.found) &&
             (over == 1) &&
             (seen.bits_to > seen.bits_from) )
        {
            from = seen.bits_from;
            to   = seen.bits_to;
        }
        else if ( (seen.found) &&
                  (over == 2) &&
                  (seen.symbols_to > seen.symbols_from) )
        {
            from = seen.symbols_from;
            to   = seen.symbols_to;
        }
        else if ( (seen.found) &&
                  (over == 3) &&
                  (seen.values_to > seen.values_from) )
        {
            from = seen.values_from;
            to   = seen.values_to;
        }

        from = std::clamp(from, left, right);
        to   = std::clamp(to, from, right);

        tab_place& asked = tab_places()[node->ID];

        if ( (shown) ||
             (!asked.in_view) )
        {
            asked.from    = (from > left + 0.5f) ? from - left : 0.0f;
            asked.to      = (to < right - 0.5f) ? to - left : 0.0f;
            asked.right   = _attrs.as_bool("tabs_right", false);
            asked.in_view = shown;
        }

        if ( (shown) &&
             ( (from > left + 0.5f) || (to < right - 0.5f) ) )
        {
            const ImGuiStyle& style  = ImGui::GetStyle();
            const float       top    = node->Pos.y;
            const float       foot   = top + ImGui::GetFontSize() +
                                       style.FramePadding.y * 2.0f;
            const ImU32       ground = ImGui::GetColorU32(ImGuiCol_ChildBg) |
                                       IM_COL32_A_MASK;
            const ImU32       edge   = ImGui::GetColorU32(ImGuiCol_Separator);
            ImDrawList*       list   = window->DrawList;

            list->PushClipRect(ImVec2(left, top), ImVec2(right, foot), false);

            if (from > left + 0.5f)
            {
                list->AddRectFilled(ImVec2(left, top), ImVec2(from, foot),
                                    ground);
                list->AddLine(ImVec2(from - 1.0f, top),
                              ImVec2(from - 1.0f, foot), edge);
            }

            if (to < right - 0.5f)
            {
                list->AddRectFilled(ImVec2(to, top), ImVec2(right, foot),
                                    ground);
                list->AddLine(ImVec2(to, top), ImVec2(to, foot), edge);
            }

            list->PopClipRect();
        }

        if (shown)
        {
            spots()[key + ".tab_row.from"] =
                spotted{ ImGui::GetCurrentContext(), ImGui::GetFrameCount(),
                         ImVec2(from, node->Pos.y) };
            spots()[key + ".tab_row.to"] =
                spotted{ ImGui::GetCurrentContext(), ImGui::GetFrameCount(),
                         ImVec2(to, node->Pos.y) };
        }
    }
#endif

    // its name typed over its tab, while it is renamed: Enter or a click
    // away keeps it, Escape does not
    if ( (shown) &&
         (docked) &&
         (_attrs.as_bool("renaming", false)) )
    {
        std::string& typed = tab_drafts()[key];
        const ImVec2 back  = ImGui::GetCursorScreenPos();
        const float  wide  = std::max(tab.GetWidth(), 220.0f * ui_scale());
        const bool   first = _attrs.as_bool("rename_focus", false);

        if (first)
        {
            typed = _attrs.as_string("rename_text", "");
            ImGui::SetKeyboardFocusHere();
        }

        ImGui::PushClipRect(tab.Min, ImVec2(tab.Min.x + wide, tab.Max.y),
                            false);
        ImGui::SetCursorScreenPos(tab.Min);
        ImGui::SetNextItemWidth(wide);

        const bool entered = ImGui::InputText(
            "##tab.rename",
            &typed,
            ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_AutoSelectAll);

        spot(key + ".rename");

        const bool escaped = ( (ImGui::IsItemActive()) &&
                               (ImGui::IsKeyPressed(ImGuiKey_Escape)) );
        const bool left    = ( (!first) &&
                               (!ImGui::IsItemActive()) &&
                               (!entered) );

        ImGui::PopClipRect();
        ImGui::SetCursorScreenPos(back);

        if ( (entered) ||
             ( (left) && (!escaped) ) )
        {
            option_set payload;

            payload.set("value", _attrs.as_long("value", 0));
            payload.set("text", typed);
            inner.post(_attrs.as_string("rename_commit", ""),
                       std::move(payload));
        }
        else if (escaped)
        {
            option_set payload;

            payload.set("value", _attrs.as_long("value", 0));
            inner.post(_attrs.as_string("rename_cancel", ""),
                       std::move(payload));
        }
    }

    if (shown)
    {
        seen()[key] = sighting{ ImGui::GetCurrentContext(),
                                ImGui::GetFrameCount() };

        // the content, at the window's zoom
        {
            const float was = zoom_now();

            zoom_now() = zoom;

            {
                const zoomed scope(zoom);

                draw_children(_children, 0, _children.size(), inner);
            }

            zoom_now() = was;
        }

        // Ctrl and the wheel over it: a step of zoom, in or out
        const std::string zooms = _attrs.as_string("zoom_action", "");

        if ( (!zooms.empty())            &&
             (io.KeyCtrl)                &&
             (io.MouseWheel != 0.0f)     &&
             (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) )
        {
            option_set payload;

            payload.set("target", key);
            payload.set("value", (io.MouseWheel > 0.0f) ? 1L : -1L);
            inner.post(zooms, std::move(payload));
        }

        // the keyboard taken -- by a click in it, or by its coming to the
        // front -- said once
        if ( (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) &&
             (in_front() != key) )
        {
            const std::string action = _attrs.as_string("focus_action", "");
            option_set        where;

            in_front() = key;
            where.set("target", key);
            inner.post("window.focus", std::move(where));

            if (!action.empty())
            {
                option_set payload;

                payload.set("value", _attrs.as_long("value", 0));
                payload.set("clicked", true);
                inner.post(action, std::move(payload));
            }
        }
    }

    ImGui::End();

    if ( (!open) &&
         (!closing.empty()) )
    {
        option_set payload;

        payload.set("value", _attrs.as_long("value", 0));
        inner.post(closing, std::move(payload));
    }

    // passed on, each with the window's document
    for (uxoxo::event& each : inner.events)
    {
        if ( (document != 0) &&
             (!each.payload.has("document")) )
        {
            each.payload.set("document", document);
        }

        _frame.events.push_back(std::move(each));
    }

    return;
}

}  // namespace gui
}  // namespace bitwisedom
