/*******************************************************************************
* bitwisedom [tests]                                                ui_smoke.cpp
*
* The window, driven headless through Dear ImGui's null backend.
*   Every frame of every section is checked for ImGui assertions -- which
* abort, so a run that reaches the end had none -- and for ID clashes, which
* this test's own ImGui records in full (IMGUI_DEBUG_HIGHLIGHT_ALL_ID_
* CONFLICTS). The sections walk the window: every tab of every example,
* the large ones too, every row in the inspector, every registered setting
* changed and undone, an operation applied and undone, a file saved and
* opened again, and a mouse that wanders, clicks and drags at random over
* everything -- the grid's strokes and its headings included.
*   Then the mouse and keyboard as a user would use them, at the places the
* grid says it drew things: a right-click opens the menu for what it is on;
* a double-click on a name, F2, or a double-click on a segment's band
* renames it where it is, and the sheet's references follow; the columns'
* edges and the ruler's boundaries resize; and code is pasted, copied,
* dropped and imported -- through a clipboard of the test's own, in memory,
* never the system's, which another program may hold open on Windows. Every frame is checked for ImGui's own errors too:
* the misuses a release build recovers from and a debug build asserts on --
* and for vertices a GPU may draw as stray pixels: not a number, far past
* the window, or outside the font atlas.
*   And the window's own choices: the grid fills its width as the sheet
* says; the tool windows open, close with their x, dock and come back; the
* theme and the colour scheme recolour the window and are kept for the next
* run; and a name, a value, a band and the ruler explain themselves under a
* resting mouse.
*   And the window as File > New leaves it: a root with no fill, holding
* one bit; the grid's "Add new..." opening the menu of what to add; names
* entered in the tree's "Add child..." fields, typed where the field has
* the keyboard back, and refused as a rename refuses them; proportional
* cells, a fifth shorter than wide, sized by hand, square at a share of
* one, off and on again, and stopping at the widest a cell grows; Help at
* the menu bar's right end, File at its left, and Help's two pages; the
* buttons over the symbols column, every way they write values, lined up,
* the list a right-click on one opens, and a macro with no bits written
* over its cells; the grid's symbols column as the tree, with its "Add
* child..." field and its rows dragged; the add control as a button and
* its menu, and as a split button, and the colour new rows take; the rows
* sorted from the heading's menu and the sort made the sheet's own;
* digits, names and colours shown in parts of the grid only, and the
* Display window; the probe and usage rows dragged among the rows and into
* a group, pinned, added and removed; the Add child field fitted and
* filling, alone or with its +, and the Add node window the + opens, a tab
* or a window; the status bar and its console; a definition file edited;
* the sheet's tab over its bits, renamed by a double-click; and the ink of
* every text on every fill, in every theme and scheme, readable.
*   And the window around the sheets: several open at once, each in a
* window of its own; workspaces added, switched, renamed and closed, a
* sheet moved between them; the sheets arranged tabbed, in columns, rows
* and a grid; each window zoomed as a browser's page is; the session kept
* and opened again, as it was, in a second window; and the Options --
* every page, its search, the settings files exported, applied, forgotten
* and deleted.
*
* path:      /tests/ui/bitwisedom/ui_smoke.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.20
*******************************************************************************/
// std
#include <cmath>    // std::fabs
#include <cstdio>   // std::printf, std::remove, std::fopen
#include <string>   // std::string
#include <vector>   // std::vector
// imgui
#include <imgui.h>            // ImGui
#include <imgui_internal.h>   // GImGui, the ID clash record
#include <imgui_impl_null.h>  // the headless backend
// bitwisedom
#include "../../../inc/ui/bitwisedom/app.hpp"       // app
#include "../../../inc/ui/bitwisedom/elements.hpp"  // grid_spot
#include "../../../inc/ui/bitwisedom/settings.hpp"  // settings
#include "../../../inc/ui/bitwisedom/shell.hpp"     // tool, tool_window_key
#include "../../../inc/ui/bitwisedom/theme.hpp"     // ink_on, legible,
                                                    // mono_font
#include <djinterp/tools/bitwisedom/examples.hpp>  // example_names
#include <djinterp/tools/bitwisedom/persist.hpp>   // read_file
// the attachment
#include "../../djinterp/tools/bitwisedom/attachment.inc"


namespace
{

using bitwisedom::gui::app;

// clashes
//   how many frames have had an ID clash.
int clashes = 0;

// errors
//   how many frames ImGui reported an error in: a misuse it recovers from
// in this release build, and asserts on in a debug one.
int errors = 0;

// strays
//   how many frames gave the GPU a vertex it may draw as stray pixels: one
// not a number, one far past any window, or one whose place in the font
// atlas is outside it.
int strays = 0;

// FARTHEST
//   constant: the farthest from the window's corner a vertex may lie.
const float FARTHEST = 100000.0f;

// stray_vertex
//   whether the last frame drew a vertex a GPU may draw as stray pixels.
bool
stray_vertex()
{
    const ImDrawData* drawn = ImGui::GetDrawData();

    for (int i = 0; ( (drawn != nullptr) && (i < drawn->CmdListsCount) ); ++i)
    {
        for (const ImDrawVert& each : drawn->CmdLists[i]->VtxBuffer)
        {
            const bool finite = ( (std::isfinite(each.pos.x)) &&
                                  (std::isfinite(each.pos.y)) &&
                                  (std::isfinite(each.uv.x))  &&
                                  (std::isfinite(each.uv.y)) );

            if ( (!finite)                            ||
                 (std::fabs(each.pos.x) > FARTHEST)   ||
                 (std::fabs(each.pos.y) > FARTHEST)   ||
                 (each.uv.x < 0.0f) || (each.uv.x > 1.0f) ||
                 (each.uv.y < 0.0f) || (each.uv.y > 1.0f) )
            {
                return true;
            }
        }
    }

    return false;
}

// failure
//   the first check that failed in the running section, or nullptr.
const char* failure = nullptr;

void
expect(
    bool        _ok,
    const char* _what
)
{
    if ( (!_ok) &&
         (failure == nullptr) )
    {
        failure = _what;
    }

    return;
}

bool
id_clash()
{
    ImGuiContext& context = *GImGui;

    for (const ImGuiStoragePair& pair :
         context.DebugDrawIdConflictsHighlightSet.Data)
    {
        if (pair.val_i != context.FrameCount)
        {
            continue;
        }

        // ImGui's own multiline input submits its ID twice -- as an item,
        // and again when its child window ends -- and the full check flags
        // it every frame. An ID that is a child window's is that pattern.
        bool child = false;

        for (const ImGuiWindow* window : context.Windows)
        {
            child = ( (child) ||
                      (window->ChildId == pair.key) );
        }

        if (!child)
        {
            return true;
        }
    }

    return false;
}

// frames
//   runs frames of the window, recording any clash.
void
frames(
    app& _window,
    int  _count
)
{
    for (int i = 0; i < _count; ++i)
    {
        ImGui_ImplNull_NewFrame();
        ImGui::NewFrame();
        _window.frame();
        ImGui::Render();
        ImGui_ImplNullRender_RenderDrawData(ImGui::GetDrawData());

        if (id_clash())
        {
            ++clashes;
        }

        if (GImGui->ErrorCountCurrentFrame > 0)
        {
            ++errors;
        }

        if (stray_vertex())
        {
            ++strays;
        }
    }

    return;
}

// post
//   an event as an element would post it.
void
post(
    app&                            _window,
    const std::string&              _kind,
    const ::djinterp::option_value& _value,
    const ::djinterp::option_value& _target = ::djinterp::option_value(0L)
)
{
    uxoxo::event made;
    made.kind = _kind;
    made.payload.set("value", _value);
    made.payload.set("target", _target);
    _window.post(made);

    return;
}

// ------------------------------------------------------ mouse and keyboard

// move_to
//   the mouse to a point of the grid, and two frames for it to be seen
// there: ImGui hovers what the mouse was over the frame before.
bool
move_to(
    app&                           _window,
    const bitwisedom::gui::grid_point& _point
)
{
    if (!_point.found)
    {
        return false;
    }

    ImGui::GetIO().AddMousePosEvent(_point.x, _point.y);
    frames(_window, 2);

    return true;
}

// click
//   a button pressed and released where the mouse is.
void
click(
    app&             _window,
    ImGuiMouseButton _button = ImGuiMouseButton_Left
)
{
    ImGuiIO& io = ImGui::GetIO();

    io.AddMouseButtonEvent(_button, true);
    frames(_window, 1);
    io.AddMouseButtonEvent(_button, false);
    frames(_window, 1);

    return;
}

// double_click
//   two clicks, well inside the double-click time.
void
double_click(
    app& _window
)
{
    click(_window);
    click(_window);

    return;
}

// drag_by
//   a press where the mouse is, a move across by _dx in two steps, and a
// release.
void
drag_by(
    app&  _window,
    float _dx
)
{
    ImGuiIO&     io   = ImGui::GetIO();
    const ImVec2 from = io.MousePos;

    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    frames(_window, 1);
    io.AddMousePosEvent(from.x + _dx * 0.5f, from.y);
    frames(_window, 1);
    io.AddMousePosEvent(from.x + _dx, from.y);
    frames(_window, 1);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    frames(_window, 2);

    return;
}

// key
//   a key pressed and released.
void
key(
    app&     _window,
    ImGuiKey _key
)
{
    ImGuiIO& io = ImGui::GetIO();

    io.AddKeyEvent(_key, true);
    frames(_window, 1);
    io.AddKeyEvent(_key, false);
    frames(_window, 1);

    return;
}

// press
//   a key pressed and released with modifiers held -- Ctrl, Shift, Alt --
// as a chord names them.
void
press(
    app&          _window,
    ImGuiKeyChord _chord
)
{
    ImGuiIO&       io   = ImGui::GetIO();
    const ImGuiKey key  = static_cast<ImGuiKey>(_chord & ~ImGuiMod_Mask_);
    const ImGuiKey mods[3] = { ImGuiMod_Ctrl, ImGuiMod_Shift, ImGuiMod_Alt };

    for (ImGuiKey each : mods)
    {
        if ((_chord & each) != 0)
        {
            io.AddKeyEvent(each, true);
        }
    }

    io.AddKeyEvent(key, true);
    frames(_window, 1);
    io.AddKeyEvent(key, false);

    for (ImGuiKey each : mods)
    {
        if ((_chord & each) != 0)
        {
            io.AddKeyEvent(each, false);
        }
    }

    frames(_window, 1);

    return;
}

// type
//   text typed into whatever has the keyboard, then Enter.
void
type(
    app&        _window,
    const char* _text
)
{
    ImGui::GetIO().AddInputCharactersUTF8(_text);
    frames(_window, 1);
    key(_window, ImGuiKey_Enter);
    frames(_window, 1);

    return;
}

// popups
//   how many popups are open.
int
popups()
{
    return GImGui->OpenPopupStack.Size;
}

// tooltip_shown
//   whether a tooltip window was drawn in the last frame.
bool
tooltip_shown()
{
    for (const ImGuiWindow* window : GImGui->Windows)
    {
        if ( ((window->Flags & ImGuiWindowFlags_Tooltip) != 0) &&
             (window->WasActive) )
        {
            return true;
        }
    }

    return false;
}

// window_shown
//   whether a window of that name was drawn in the last frame.
bool
window_shown(
    const char* _name
)
{
    const ImGuiWindow* found = ImGui::FindWindowByName(_name);

    return ( (found != nullptr) &&
             (found->WasActive) );
}

// rest
//   the mouse held still for longer than ImGui waits before a tooltip.
void
rest(
    app& _window
)
{
    frames(_window,
           static_cast<int>(ImGui::GetStyle().HoverDelayNormal * 60.0f) + 6);

    return;
}

// here
//   the workspace in front.
const bitwisedom::gui::workspace&
here(
    app& _window
)
{
    const bitwisedom::gui::shell_state& shell = _window.shell();

    return shell.workspaces[shell.current];
}

// tool_key
//   a tool window's key, in the workspace in front.
std::string
tool_key(
    app&                   _window,
    bitwisedom::gui::tool  _tool
)
{
    return bitwisedom::gui::tool_window_key(here(_window).id, _tool);
}

// tool_is_open / tool_in_view
//   whether a tool window of the workspace in front is open; and whether
// it drew its content in the last frame -- its tab in front of its dock.
bool
tool_is_open(
    app&                   _window,
    bitwisedom::gui::tool  _tool
)
{
    return here(_window).tools[static_cast<std::size_t>(_tool)];
}

bool
tool_in_view(
    app&                   _window,
    bitwisedom::gui::tool  _tool
)
{
    return bitwisedom::gui::window_seen(tool_key(_window, _tool));
}

// id_of
//   a symbol's id, by its full name, or 0.
int
id_of(
    app&               _window,
    const std::string& _full
)
{
    const auto found = _window.state().res.by_full.find(_full);

    return (found == _window.state().res.by_full.end()) ? 0 : found->second;
}

// --------------------------------------------------------------- sections

void
first_frames(
    app& _window
)
{
    frames(_window, 4);

    using bitwisedom::gui::tool;

    expect(clashes == 0, "the first frames have no ID clash");
    expect( (tool_is_open(_window, tool::inspector)) &&
            (tool_in_view(_window, tool::inspector)),
           "the Inspector is present");
    expect( (tool_is_open(_window, tool::macros)) &&
            (tool_is_open(_window, tool::templates)),
           "the macros and the templates are among the sheets");
    expect(!tool_is_open(_window, tool::symbols),
           "the symbols tree is the grid's symbols column");
    expect(!_window.state().doc.grid.show_probe &&
           !_window.state().doc.grid.show_usage,
           "probe and allocated rows are initially hidden");
    expect(!_window.state().res.rows.empty(), "an example is loaded");

    return;
}

void
every_tab(
    app& _window
)
{
    const std::vector<std::string> names    = bitwisedom::example_names();
    const long                     examples = static_cast<long>(names.size());
    float                          wide     = 0.0f;  // x86_64_pte's values
    float                          narrow   = 0.0f;  // rgb565's

    for (long example = 0; example < examples; ++example)
    {
        post(_window, "file.example", example);
        frames(_window, 3);

        // an example arrives with its columns fitted, and nothing changed
        expect(!_window.state().ui.dirty, "an example arrives unchanged");

        if (names[example] == "x86_64_pte")
        {
            wide = _window.state().doc.grid.value_width;
        }
        else if (names[example] == "rgb565")
        {
            narrow = _window.state().doc.grid.value_width;
        }

        // every tool window, brought to the front of its dock
        for (long each = 0; each < bitwisedom::gui::TOOL_COUNT; ++each)
        {
            const auto which = static_cast<bitwisedom::gui::tool>(each);

            post(_window, "tool.open", each);
            frames(_window, 3);
            expect( (tool_is_open(_window, which)) &&
                    (tool_in_view(_window, which)),
                   "the tool window asked for is in view");
        }
    }

    expect(here(_window).documents.size() == 1,
           "an example looked at gives way to the next");
    expect(clashes == 0, "no tool window of any example has an ID clash");
    // The Value column is hidden by default, so fitting its width does
    // not run until it is enabled. Column-resize behavior is tested below.
    expect(wide > 0.0f && narrow > 0.0f,
           "the hidden value column retains usable width settings");

    return;
}

void
every_row(
    app& _window
)
{
    post(_window, "file.example", 0L);
    frames(_window, 1);

    std::vector<long> ids;

    for (const bitwisedom::resolved_row& each : _window.state().res.rows)
    {
        ids.push_back(each.id);
    }

    for (long tab : { 0L, 1L })
    {
        post(_window, "tab.show", tab);

        for (long id : ids)
        {
            post(_window, "select", id);
            frames(_window, 2);
        }
    }

    expect(!_window.state().ui.selection.empty(), "a row is selected");
    expect(clashes == 0, "no row's inspector has an ID clash");

    return;
}

void
every_setting(
    app& _window
)
{
    post(_window, "file.example", 1L);
    frames(_window, 1);

    for (const bitwisedom::gui::setting& each : bitwisedom::gui::settings())
    {
        uxoxo::event made;
        made.kind = "set";
        made.payload.set("target", each.path);

        switch (each.kind)
        {
            case bitwisedom::setting_kind::flag:
                made.payload.set("value",
                                 each.read(_window.state().doc) == 0.0);
                break;

            case bitwisedom::setting_kind::integer:
            case bitwisedom::setting_kind::choice:
                made.payload.set("value",
                                 static_cast<long>(each.high));
                break;

            case bitwisedom::setting_kind::real:
                made.payload.set("value", (each.low + each.high) * 0.5);
                break;

            case bitwisedom::setting_kind::colour:
                made.payload.set("r", 0.9);
                made.payload.set("g", 0.4);
                made.payload.set("b", 0.2);
                made.payload.set("a", 1.0);
                break;

            case bitwisedom::setting_kind::text:
                // the width stays sane; text settings take a name
                made.payload.set("value", std::string("X_"));
                break;
        }

        _window.post(made);
        post(_window, "tab.show", static_cast<long>(each.path.size() % 8));
        frames(_window, 1);
    }

    expect(clashes == 0, "no setting leaves an ID clash");

    // and every one of them undone
    const std::size_t depth = _window.state().steps.depth();

    for (std::size_t i = 0; i < depth; ++i)
    {
        post(_window, "edit.undo", 0L);
    }

    frames(_window, 2);
    expect(!_window.state().steps.can_undo(), "everything undoes");

    return;
}

void
an_operation(
    app& _window
)
{
    post(_window, "file.example", 0L);
    post(_window, "tab.show", 0L);
    frames(_window, 1);

    const bitwisedom::resolution& before = _window.state().res;
    const int                    en     = before.by_full.at("UART_CR_EN");
    const int                    parity = before.by_full.at("UART_CR_PARITY");

    post(_window, "select", static_cast<long>(en));
    frames(_window, 1);
    post(_window, "ops.op", 0L);
    post(_window, "ops.operand", std::string("1"));
    frames(_window, 2);

    expect(_window.state().preview.rows.size() == 1, "one target previewed");

    post(_window, "ops.apply", 0L);
    frames(_window, 2);

    const bitwisedom::resolution& after = _window.state().res;

    expect(after.find(en)->value == 0x2u, "EN moved to bit 1");
    expect(after.find(parity)->value == 0x30u, "PARITY did not move");

    post(_window, "edit.undo", 0L);
    frames(_window, 1);
    expect(_window.state().res.find(en)->value == 0x1u, "and it undoes");

    return;
}

void
a_file(
    app& _window
)
{
    const std::string path = "ui_smoke.bitwisedom.json";

    post(_window, "file.example", 2L);
    post(_window, "file.path", path);
    post(_window, "file.save", 0L);
    post(_window, "file.new", 0L);
    frames(_window, 1);

    expect(_window.state().doc.title == "untitled", "a new sheet");

    post(_window, "file.open", path);
    frames(_window, 1);

    expect(_window.state().doc.title == "posix_mode", "the file opens");
    expect(!_window.state().ui.status_error, "without an error");

    // File > Open with no file named: the dialog, which opens what is
    // typed into it
    post(_window, "file.new", 0L);
    post(_window, "file.open", 0L);
    frames(_window, 3);
    expect(window_shown("Open##bitwisedom"),
           "File > Open asks for the file");

    type(_window, path.c_str());
    frames(_window, 3);
    expect( (_window.state().doc.title == "posix_mode") &&
            (!window_shown("Open##bitwisedom")),
           "and opens the one typed");

    std::remove(path.c_str());

    return;
}

// next
//   a small deterministic generator, so a failure repeats.
unsigned
next(
    unsigned& _state
)
{
    _state = _state * 1664525u + 1013904223u;

    return _state >> 8;
}

void
a_wandering_mouse(
    app& _window
)
{
    unsigned state = 2026u;
    ImGuiIO& io    = ImGui::GetIO();

    post(_window, "file.example", 0L);
    frames(_window, 2);

    for (int step = 0; step < 600; ++step)
    {
        const float x = static_cast<float>(next(state) % 1920u);
        const float y = static_cast<float>(next(state) % 1080u);

        io.AddMousePosEvent(x, y);

        // now and then a press, held for a few frames: a click or a drag
        if ((next(state) % 5u) == 0u)
        {
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
            frames(_window, 1);
            io.AddMousePosEvent(x + static_cast<float>(next(state) % 120u),
                                y);
            frames(_window, 1);
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
        }

        // and the tabs change under it
        if ((step % 75) == 0)
        {
            post(_window, "tab.show", static_cast<long>((step / 75) % 8));
        }

        frames(_window, 1);
    }

    // escape any popup a click opened
    io.AddKeyEvent(ImGuiKey_Escape, true);
    frames(_window, 1);
    io.AddKeyEvent(ImGuiKey_Escape, false);
    frames(_window, 2);

    expect(clashes == 0, "no ID clash under the mouse");

    return;
}

void
the_header(
    app& _window
)
{
    post(_window, "file.example", 0L);
    post(_window, "tab.show", 1L);
    frames(_window, 2);

    const std::string& text = _window.state().header.text;

    expect(text.find("#define UART_CR_EN") != std::string::npos,
           "the header names EN");
    expect(text.find("#endif  // UART_CR_H") != std::string::npos,
           "and is closed");

    return;
}

void
the_context_menus(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;

    post(_window, "file.example", 0L);
    post(_window, "tab.show", 0L);

    frames(_window, 3);

    const int en = id_of(_window, "UART_CR_EN");

    // on a row's name
    expect(move_to(_window, grid_spot("name", en, 0)),
           "the grid drew EN's name");
    click(_window, ImGuiMouseButton_Right);

    expect( (_window.state().ui.menu_kind == "row") &&
            (_window.state().ui.menu_row == en)      &&
            (_window.state().ui.menu_where == "grid"),
           "a right-click on a name is about its row");
    expect(popups() == 1, "and opens the menu");
    expect( (!_window.state().ui.selection.empty()) &&
            (_window.state().ui.selection.front() == en),
           "with the row selected");

    key(_window, ImGuiKey_Escape);
    expect(popups() == 0, "Escape closes it");

    // on a cell of another row
    const int parity = id_of(_window, "UART_CR_PARITY");

    expect(move_to(_window, grid_spot("cell", parity, 5)),
           "the grid drew PARITY's cells");
    click(_window, ImGuiMouseButton_Right);
    expect(_window.state().ui.menu_row == parity,
           "a right-click on a cell is about its row");
    key(_window, ImGuiKey_Escape);

    // on a segment's band
    expect(move_to(_window, grid_spot("band", 0, 0)),
           "the grid drew the bands");
    click(_window, ImGuiMouseButton_Right);
    expect( (_window.state().ui.menu_kind == "segment") &&
            (_window.state().ui.menu_segment == 0),
           "a right-click on a band is about its segment");
    expect(popups() == 1, "and opens the segment's menu");
    key(_window, ImGuiKey_Escape);

    // the tree's, posted as its node posts it
    post(_window, "context.tree", static_cast<long>(parity));
    frames(_window, 2);
    expect( (_window.state().ui.menu_kind == "row") &&
            (_window.state().ui.menu_where == "tree") &&
            (popups() == 1),
           "the tree's right-click opens the row's menu");
    key(_window, ImGuiKey_Escape);

    expect(clashes == 0, "no menu has an ID clash");

    return;
}

void
a_rename_in_the_grid(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;

    post(_window, "file.example", 0L);
    frames(_window, 3);

    const int en = id_of(_window, "UART_CR_EN");

    expect(move_to(_window, grid_spot("name", en, 0)),
           "the grid drew EN's name");
    double_click(_window);

    expect( (_window.state().ui.renaming == en) &&
            (_window.state().ui.rename_where == "grid"),
           "a double-click on a name renames it in the grid");

    frames(_window, 2);
    type(_window, "ENABLE");

    const bitwisedom::sheet& doc = _window.state().doc;

    expect( (id_of(_window, "UART_CR_ENABLE") == en) &&
            (id_of(_window, "UART_CR_EN") == 0),
           "what was typed is its name");
    expect(bitwisedom::find_row(doc, id_of(_window, "UART_CR_DEFAULT"))
                   ->expr.find("ENABLE | TXEN") == 0,
           "the expression that named it follows");
    expect(bitwisedom::find_row(doc, id_of(_window, "UART_CR_IS_ENABLED"))
                   ->body.find("{ENABLE}") != std::string::npos,
           "and so does the macro's placeholder");
    expect(_window.state().ui.renaming == 0, "the field is gone");

    post(_window, "edit.undo", 0L);
    frames(_window, 2);
    expect(id_of(_window, "UART_CR_EN") == en,
           "the rename undoes in one step");

    // Escape gives one up
    post(_window, "rename.grid", static_cast<long>(en));
    frames(_window, 3);
    ImGui::GetIO().AddInputCharactersUTF8("NOPE");
    frames(_window, 1);
    key(_window, ImGuiKey_Escape);
    frames(_window, 1);

    expect( (_window.state().ui.renaming == 0) &&
            (id_of(_window, "UART_CR_EN") == en),
           "Escape gives a rename up");

    // a name that is no C name is refused, and says so
    post(_window, "rename.grid", static_cast<long>(en));
    frames(_window, 3);
    type(_window, "9 lives");

    expect( (id_of(_window, "UART_CR_EN") == en) &&
            (_window.state().ui.status_error),
           "a name that is no identifier is refused");

    expect(clashes == 0, "no rename has an ID clash");

    return;
}

void
a_rename_in_the_tree(
    app& _window
)
{
    post(_window, "file.example", 0L);
    frames(_window, 2);

    const int txen = id_of(_window, "UART_CR_TXEN");

    // in the grid, which has the keyboard while its window is in front, F2
    // renames the row in place; with the tree in front, in the tree
    post(_window, "select", static_cast<long>(txen));
    post(_window, "doc.activate", static_cast<long>(_window.state().id));
    frames(_window, 3);
    key(_window, ImGuiKey_F2);

    expect( (_window.state().ui.renaming == txen) &&
            (_window.state().ui.rename_where == "grid"),
           "F2 in the grid renames the row there");

    frames(_window, 3);
    key(_window, ImGuiKey_Escape);
    post(_window, "tool.open",
         static_cast<long>(bitwisedom::gui::tool::symbols));
    frames(_window, 3);
    key(_window, ImGuiKey_F2);

    expect( (_window.state().ui.renaming == txen) &&
            (_window.state().ui.rename_where == "tree"),
           "F2 renames the selected row, in the tree");

    frames(_window, 2);
    type(_window, "TX_ENABLE");

    expect(id_of(_window, "UART_CR_TX_ENABLE") == txen,
           "what was typed in the tree is its name");
    expect(bitwisedom::find_row(_window.state().doc,
                                id_of(_window, "UART_CR_DEFAULT"))
                   ->expr.find("TX_ENABLE") != std::string::npos,
           "and the expression follows");

    // the inspector's field renames as well, when its edit is done
    post(_window, "row.name", std::string("TRANSMIT"),
         static_cast<long>(txen));
    frames(_window, 2);
    expect(id_of(_window, "UART_CR_TRANSMIT") == txen,
           "the inspector's name field renames, references and all");

    // the tree's last row: its field is the last thing in the panel
    const int last = id_of(_window, "UART_CR_IS_ENABLED");

    post(_window, "rename.tree", static_cast<long>(last));
    frames(_window, 3);
    type(_window, "ENABLED");

    expect(id_of(_window, "UART_CR_ENABLED") == last,
           "the tree's last row renames too");

    expect(clashes == 0, "no rename in the tree has an ID clash");
    expect(errors == 0, "and ImGui reports no error in any of them");

    return;
}

void
a_segment_renamed(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;

    post(_window, "file.example", 0L);
    frames(_window, 3);

    const bool folded = _window.state().doc.segments[0].collapsed;

    expect(move_to(_window, grid_spot("band", 0, 0)),
           "the grid drew the enables band");
    double_click(_window);

    expect(_window.state().ui.renaming_segment == 0,
           "a double-click on a band renames its segment");
    expect(_window.state().doc.segments[0].collapsed == folded,
           "and leaves it open or closed as it was");

    frames(_window, 2);
    type(_window, "switches");

    expect(_window.state().doc.segments[0].name == "switches",
           "what was typed is the segment's name");

    // a band the sheet made itself becomes a segment of its own
    const std::size_t before = _window.state().doc.segments.size();

    post(_window, "rename.segment", -1L);
    frames(_window, 3);
    type(_window, "spare");

    expect( (_window.state().doc.segments.size() == before + 1) &&
            (_window.state().doc.segments.back().name == "spare"),
           "an automatic band, renamed, is a segment of the sheet's");

    expect(clashes == 0, "no segment's rename has an ID clash");

    return;
}

void
columns_resized(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;

    post(_window, "file.example", 0L);
    frames(_window, 3);

    const bitwisedom::grid_settings& grid  = _window.state().doc.grid;
    const float                      names = grid.name_width;

    // the name column's edge, dragged
    expect(move_to(_window, grid_spot("edge.name", 0, 0)),
           "the grid drew the name column's edge");
    drag_by(_window, 40.0f);
    expect(std::fabs(grid.name_width - (names + 40.0f)) < 0.5f,
           "a drag of the name column's edge widens it with the mouse");

    // and fitted
    double_click(_window);

    const float fitted = grid.name_width;

    expect( (fitted >= 80.0f) &&
            (std::fabs(fitted - (names + 40.0f)) > 0.5f),
           "a double-click on it fits it to the names");

    expect(move_to(_window, grid_spot("edge.name", 0, 0)),
           "the fitted edge is drawn");
    double_click(_window);
    expect(std::fabs(grid.name_width - fitted) < 0.01f,
           "and fitting again changes nothing");

    // The value column is hidden by default. Its resize edge should only
    // exist after the user enables the column in View > Columns.
    expect(!grid.show_values, "the value column starts hidden");
    expect(!grid_spot("edge.value", 0, 0).found,
           "a hidden value column has no resize edge");

    post(_window, "set", true, std::string("grid.show_values"));
    frames(_window, 3);
    expect(grid.show_values, "the value column can be shown");

    // the visible value column's edge can then be dragged normally
    const float values = grid.value_width;

    expect(move_to(_window, grid_spot("edge.value", 0, 0)),
           "the grid drew the value column's edge");
    drag_by(_window, -20.0f);
    expect(std::fabs(grid.value_width - (values - 20.0f)) < 0.5f,
           "a drag of the value column's edge narrows it");

    // every cell at once, by a ruler boundary
    const float cells = grid.cell_width;

    expect(move_to(_window, grid_spot("boundary", 0, 4)),
           "the grid drew the ruler's boundaries");
    drag_by(_window, 60.0f);
    expect(grid.cell_width > cells + 0.5f,
           "a drag of a ruler boundary widens the cells");

    expect(move_to(_window, grid_spot("boundary", 0, 4)),
           "the widened boundary is drawn");
    double_click(_window);
    expect(std::fabs(grid.cell_width - bitwisedom::grid_settings{}.cell_width) <
               0.01f,
           "a double-click on one puts the cells back");

    expect(clashes == 0, "no resize has an ID clash");

    return;
}

void
code_in_and_out(
    app& _window
)
{
    post(_window, "file.example", 0L);
    frames(_window, 2);

    // pasted beside the selection
    ImGui::SetClipboardText("#define UART_CR_EXTRA (1u << 12)\n"
                            "#define UART_CR_TWICE(x) ((x) * 2)\n");
    post(_window, "code.paste", 0L);
    frames(_window, 2);

    const int extra = id_of(_window, "UART_CR_EXTRA");

    expect( (extra != 0) &&
            (id_of(_window, "UART_CR_TWICE") != 0),
           "pasted code adds its definitions");
    expect( (extra != 0) &&
            (_window.state().res.find(extra)->has_value) &&
            (_window.state().res.find(extra)->value == 0x1000u),
           "with their values");

    // copied, as code to paste elsewhere
    post(_window, "code.copy",
         static_cast<long>(id_of(_window, "UART_CR_EN")));
    frames(_window, 1);
    expect(std::string(ImGui::GetClipboardText()).find(
               "#define UART_CR_EN") != std::string::npos,
           "a row copies as its #define");

    // a new sheet from the clipboard's code
    ImGui::SetClipboardText(ATTACHMENT);
    post(_window, "code.new", 0L);
    frames(_window, 3);

    expect( (_window.state().doc.title == "pasted") &&
            (id_of(_window, "D_CODE_ARRAY_INDEX_POS") != 0),
           "the attachment's code is a new sheet");
    expect( (_window.state().ui.import_name == "the clipboard") &&
            (!_window.state().res.issues.empty()),
           "and what it found of the code is in the issues");
    expect(_window.state().doc.grid.name_width >
               bitwisedom::grid_settings{}.name_width,
           "with the name column fitted to its long names");

    // every page of it drawn
    for (long tab = 0; tab < 8; ++tab)
    {
        post(_window, "tab.show", tab);
        frames(_window, 2);
    }

    // a header dropped on the window
    const char* const header = "ui_smoke_dropped.h";
    std::FILE*        out    = std::fopen(header, "wb");

    if (out != nullptr)
    {
        std::fputs("#define DROPPED_A 0x10u\n"
                   "#define DROPPED_B (DROPPED_A << 1)\n", out);
        std::fclose(out);
    }

    post(_window, "file.drop", std::string(header));
    frames(_window, 2);

    expect( (_window.state().doc.title == "ui_smoke_dropped") &&
            (id_of(_window, "DROPPED_B") != 0) &&
            (_window.state().res.find(id_of(_window, "DROPPED_B"))->value ==
             0x20u),
           "a header dropped on the window is imported");

    std::remove(header);

    expect(clashes == 0, "no import has an ID clash");

    return;
}

// the_grid_fills
//   the grid takes its panel's width: the cells widened, or the name column,
// or nothing -- and then its rows end where its columns do.
void
the_grid_fills(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;

    post(_window, "file.example", 9L);   // win32_hresult: 32 bits
    frames(_window, 4);

    const bitwisedom::grid_settings& grid = _window.state().doc.grid;

    expect(grid.fill == bitwisedom::grid_fill::cells,
           "a sheet's spare room goes to its cells");

    bitwisedom::gui::grid_point end  = grid_spot("end", 0, 0);
    bitwisedom::gui::grid_point room = grid_spot("room", 0, 0);

    expect( (end.found) &&
            (room.found) &&
            (std::fabs(end.x - room.x) <= 1.0f),
           "the cells widen to the grid's edge");

    const bitwisedom::gui::grid_point cell0 = grid_spot("cell",
                                                        id_of(_window,
                                                              "WIN_S_FALSE"),
                                                        0);
    const bitwisedom::gui::grid_point cell1 = grid_spot("cell",
                                                        id_of(_window,
                                                              "WIN_S_FALSE"),
                                                        1);

    expect( (cell0.found) &&
            (cell1.found) &&
            (std::fabs(cell0.x - cell1.x) > grid.cell_width + 1.0f),
           "and every cell is wider for it");

    // the name column takes it
    post(_window, "set", 1L, std::string("grid.fill"));
    frames(_window, 3);
    end  = grid_spot("end", 0, 0);
    room = grid_spot("room", 0, 0);

    const bitwisedom::gui::grid_point names = grid_spot("edge.name", 0, 0);
    const bitwisedom::gui::grid_point first = grid_spot("name",
                                                        id_of(_window,
                                                              "WIN_S_OK"),
                                                        0);

    expect( (std::fabs(end.x - room.x) <= 1.0f) &&
            (names.found) &&
            (first.found) &&
            (names.x - first.x > grid.name_width),
           "or the name column widens to fill it");

    // nothing takes it
    post(_window, "set", 2L, std::string("grid.fill"));
    frames(_window, 3);
    end  = grid_spot("end", 0, 0);
    room = grid_spot("room", 0, 0);

    expect(end.x < room.x - 100.0f,
           "or the grid ends where its columns do");

    post(_window, "edit.undo", 0L);
    post(_window, "edit.undo", 0L);
    frames(_window, 2);
    expect(grid.fill == bitwisedom::grid_fill::cells,
           "and the choice is undone like any setting");

    return;
}

// the_tool_windows
//   the tool windows, every way they come and go: closed by their x and
// opened again from View, put away below the sheets with Ctrl+J and
// brought back as they were, and opened by a page asked for.
void
the_tool_windows(
    app& _window
)
{
    using bitwisedom::gui::tool;

    ImGuiIO& io = ImGui::GetIO();

    post(_window, "file.example", 0L);
    post(_window, "tab.show", 3L);
    frames(_window, 3);
    expect(tool_in_view(_window, tool::issues),
           "a page asked for by its old number is its window, in view");

    // its x closes it; View opens it again, where it was
    post(_window, "tool.close", static_cast<long>(tool::issues));
    frames(_window, 3);
    expect( (!tool_is_open(_window, tool::issues)) &&
            (!window_shown(("###" + tool_key(_window,
                                             tool::issues)).c_str())),
           "closed, it is gone");

    post(_window, "tool.toggle", static_cast<long>(tool::issues));
    frames(_window, 3);
    expect( (tool_is_open(_window, tool::issues)) &&
            (tool_in_view(_window, tool::issues)),
           "View opens it again, in front");

    // Ctrl+J puts the windows below away, and brings back the same
    post(_window, "tool.close", static_cast<long>(tool::naming));
    frames(_window, 2);
    io.AddMousePosEvent(io.DisplaySize.x * 0.5f, 80.0f);
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    key(_window, ImGuiKey_J);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    frames(_window, 2);
    expect( (!tool_is_open(_window, tool::operations)) &&
            (!tool_is_open(_window, tool::issues)) &&
            (tool_is_open(_window, tool::inspector)),
           "Ctrl+J puts the windows below the sheets away");

    post(_window, "panel.toggle", 0L);
    frames(_window, 2);
    expect( (tool_is_open(_window, tool::operations)) &&
            (tool_is_open(_window, tool::issues)) &&
            (!tool_is_open(_window, tool::naming)),
           "and brings back those it put away");

    post(_window, "tool.open", static_cast<long>(tool::naming));

    // the side windows close and open
    post(_window, "view.symbols", true);
    post(_window, "view.inspector", false);
    frames(_window, 3);
    expect( (tool_in_view(_window, tool::symbols)) &&
            (!tool_is_open(_window, tool::inspector)),
           "the symbols tree opens in a window of its own");

    post(_window, "view.symbols", false);
    post(_window, "view.inspector", true);
    frames(_window, 3);
    expect( (!tool_is_open(_window, tool::symbols)) &&
            (tool_in_view(_window, tool::inspector)),
           "and closes again, the inspector back");

    expect(clashes == 0, "no way of showing them has an ID clash");

    return;
}

// the_theme_and_scheme
//   the window's theme and colour scheme: they recolour the window, leave
// the sheet as it was, and are kept for the next run.
void
the_theme_and_scheme(
    app& _window
)
{
    post(_window, "file.example", 0L);
    frames(_window, 2);

    // the sheet in front: the example, beside any sheet changed before it
    const bitwisedom::gui::model& state = _window.state();
    const bool                    dirty = state.ui.dirty;
    const ImVec4 dark  = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];

    post(_window, "view.theme", 1L);
    frames(_window, 2);

    const ImVec4 light = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];

    expect( (light.x > 0.8f) &&
            (dark.x < 0.3f) &&
            (state.shown.colours.theme == 1) &&
            (state.shown.colours.empty_cell ==
             bitwisedom::scheme_for(1, 0).colours.empty_cell),
           "the light theme lights the window and the grid");

    post(_window, "view.scheme", 2L);
    frames(_window, 2);
    expect( (state.shown.colours.palette == 2) &&
            (state.shown.colours.accent ==
             bitwisedom::scheme_for(1, 2).colours.accent),
           "a scheme gives the grid its palette and accent");

    expect(state.ui.dirty == dirty, "and neither changes the sheet");

    // kept for the next run
    std::size_t       size = 0;
    const std::string kept(ImGui::SaveIniSettingsToMemory(&size));

    expect( (kept.find("theme=1") != std::string::npos) &&
            (kept.find("scheme=2") != std::string::npos),
           "the theme and scheme go to the settings file");

    // a colour the sheet sets stays the sheet's in every theme
    post(_window, "set", 0L, std::string("colours.palette"));
    frames(_window, 2);
    expect( (state.doc.colours.palette == -1) &&
            (state.shown.colours.palette == 2),
           "the palette is the scheme's until the sheet chooses one");

    post(_window, "set", 4L, std::string("colours.palette"));
    post(_window, "view.theme", 0L);
    frames(_window, 2);
    expect(state.shown.colours.palette == 3,
           "a palette the sheet chooses stays, whatever the theme");

    post(_window, "colours.automatic", 0L);
    frames(_window, 2);
    expect(state.shown.colours.palette == 2,
           "and is the scheme's again when given back");

    post(_window, "view.scheme", 0L);
    frames(_window, 2);

    return;
}

// mouseover_info
//   what the grid cannot show in full, or has more to say about, it says
// under a resting mouse: a name, a value, a band, the ruler.
void
mouseover_info(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;

    post(_window, "file.example", 9L);
    frames(_window, 3);

    const int fields = id_of(_window, "WIN_HR_FACILITY");

    expect(move_to(_window, grid_spot("name", fields, 0)),
           "the grid drew a name");
    rest(_window);
    expect(tooltip_shown(), "a resting mouse on a name explains it");

    expect(move_to(_window, grid_spot("band", 0, 20)),
           "the grid drew a band");
    rest(_window);
    expect(tooltip_shown(), "a resting mouse on a band explains it");

    expect(move_to(_window, grid_spot("boundary", 0, 20)),
           "the grid drew the ruler");
    ImGui::GetIO().AddMousePosEvent(ImGui::GetIO().MousePos.x - 6.0f,
                                    ImGui::GetIO().MousePos.y);
    frames(_window, 2);
    rest(_window);
    expect(tooltip_shown(), "a resting mouse on the ruler explains it");

    // and away, nothing
    ImGui::GetIO().AddMousePosEvent(4.0f, 4.0f);
    frames(_window, 3);

    return;
}

// defined_as
//   what the header's #define of _name says, its spaces trimmed.
std::string
defined_as(
    const std::string& _text,
    const std::string& _name
)
{
    const std::string head = "\n#define " + _name + " ";
    const std::size_t at   = _text.find(head);

    if (at == std::string::npos)
    {
        return std::string();
    }

    const std::size_t end   = _text.find('\n', at + 1);
    const std::string value = _text.substr(at + head.size(),
                                           end - at - head.size());
    const std::size_t first = value.find_first_not_of(' ');

    return (first == std::string::npos) ? std::string()
                                        : value.substr(first);
}

// example_index
//   an example's place on the File menu, by its name.
long
example_index(
    const std::string& _name
)
{
    const std::vector<std::string> names = bitwisedom::example_names();
    long                           index = 0;

    while ( (index < static_cast<long>(names.size())) &&
            (names[static_cast<std::size_t>(index)] != _name) )
    {
        ++index;
    }

    return index;
}

// in_view
//   how many rows the tree and the grid show.
std::size_t
in_view(
    app& _window
)
{
    std::size_t count = 0;

    for (const bitwisedom::resolved_row& each : _window.state().res.rows)
    {
        count += each.visible ? 1u : 0u;
    }

    return count;
}

// a_thousand_rows
//   the HRESULT example: 1,376 codes, which arrive in closed groups, every
// group opened and every row then in view, rows across the whole of it
// selected, a code renamed in the open tree, and the groups closed again.
void
a_thousand_rows(
    app& _window
)
{
    post(_window, "file.example", example_index("win32_hresults"));
    post(_window, "tab.show", 0L);
    frames(_window, 3);

    const bitwisedom::gui::model& state = _window.state();
    std::vector<long>             groups;

    for (const bitwisedom::resolved_row& each : state.res.rows)
    {
        if (each.kind == bitwisedom::row_kind::group)
        {
            groups.push_back(each.id);
        }
    }

    expect( (state.res.rows.size() > 1400) &&
            (state.res.errors == 0),
           "the HRESULT example opens, its 1,376 codes clean");
    expect(in_view(_window) < 100, "its facilities arrive closed");

    // every group opened, as its arrow would
    for (long group : groups)
    {
        uxoxo::event made;
        made.kind = "toggle_open";
        made.payload.set("value", ::djinterp::option_value(group));
        made.payload.set("open", ::djinterp::option_value(true));
        _window.post(made);
        frames(_window, 1);
    }

    expect(in_view(_window) == state.res.rows.size(),
           "every row is in view once every group is open");

    // rows across the whole of it, each selected in turn
    bool selected = true;

    for (std::size_t i = 0; i < state.res.rows.size(); i += 97)
    {
        const long id = state.res.rows[i].id;

        post(_window, "select", id);
        frames(_window, 2);
        selected = ( (selected) &&
                     (state.ui.selection.size() == 1) &&
                     (state.ui.selection.front() == id) );
    }

    expect(selected, "rows across the whole sheet are selected");

    // a code renamed in the open tree, and the header following it
    const int code = id_of(_window, "HR_E_NOTIMPL");

    post(_window, "rename.tree", static_cast<long>(code));
    frames(_window, 3);
    type(_window, "E_NOT_DONE");
    expect(defined_as(state.header.text, "HR_E_NOT_DONE") == "0x80004001u",
           "a code renamed in the open tree, and the header follows");

    post(_window, "edit.undo", 0L);
    frames(_window, 2);

    // and closed again
    for (long group : groups)
    {
        uxoxo::event made;
        made.kind = "toggle_open";
        made.payload.set("value", ::djinterp::option_value(group));
        made.payload.set("open", ::djinterp::option_value(false));
        _window.post(made);
    }

    frames(_window, 2);
    expect(in_view(_window) <= groups.size(), "and closed again");
    expect(clashes == 0, "a thousand rows have no ID clash");

    return;
}

// every_group_at_once
//   the HRESULT example's groups, all opened and all closed: by the event
// the panel's buttons and the menus post, by Ctrl+Shift with minus and plus
// and Ctrl with the keypad's, and one group with those in it from its own
// menu; none of it a step to undo, and a second time changing nothing.
void
every_group_at_once(
    app& _window
)
{
    ImGuiIO& io = ImGui::GetIO();

    post(_window, "file.example", example_index("win32_hresults"));
    post(_window, "tab.show", 0L);
    frames(_window, 3);

    const bitwisedom::gui::model& state = _window.state();

    const std::size_t all      = state.res.rows.size();
    const bool        undoable = state.steps.can_undo();
    std::size_t       groups   = 0;
    int               dxgi     = 0;
    std::size_t       codes    = 0;

    for (const bitwisedom::row& each : state.doc.root.children)
    {
        groups += (each.kind == bitwisedom::row_kind::group) ? 1u : 0u;

        if (each.name == "DXGI")
        {
            dxgi  = each.id;
            codes = each.children.size();
        }
    }

    expect( (in_view(_window) < 100) &&
            (!state.ui.dirty),
           "the example arrives with its facilities closed");

    // as the panel's Expand all button and the View menu post it
    post(_window, "tree.open_all", 0L);
    frames(_window, 2);
    expect(in_view(_window) == all, "Expand all shows every row");
    expect(state.ui.dirty, "and the sheet, which keeps it, is changed");

    // Ctrl+Shift+Minus
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    io.AddKeyEvent(ImGuiMod_Shift, true);
    key(_window, ImGuiKey_Minus);
    io.AddKeyEvent(ImGuiMod_Shift, false);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    frames(_window, 2);
    expect(in_view(_window) == groups,
           "Ctrl+Shift+Minus closes every group: the groups alone show");

    // a second time, nothing to change
    post(_window, "tree.close_all", 0L);
    frames(_window, 2);
    expect( (in_view(_window) == groups) &&
            (state.ui.status.find("closed already") != std::string::npos),
           "collapsing them again changes nothing, and says so");

    // Ctrl with the keypad's plus
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    key(_window, ImGuiKey_KeypadAdd);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    frames(_window, 2);
    expect(in_view(_window) == all, "Ctrl with the keypad's plus opens them");

    // one group, closed from its own menu with those in it
    post(_window, "context.tree", static_cast<long>(dxgi));
    frames(_window, 2);
    expect( (state.ui.menu_kind == "row") &&
            (state.ui.menu_row == dxgi) &&
            (popups() == 1),
           "a group's menu opens");
    key(_window, ImGuiKey_Escape);

    post(_window, "tree.close_all", static_cast<long>(dxgi));
    frames(_window, 2);
    expect( (dxgi != 0) &&
            (codes > 0) &&
            (in_view(_window) == all - codes),
           "Collapse all inside closes that group, and only it");

    // Ctrl+Shift+Plus, and Ctrl with the keypad's minus
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    io.AddKeyEvent(ImGuiMod_Shift, true);
    key(_window, ImGuiKey_Equal);
    io.AddKeyEvent(ImGuiMod_Shift, false);
    frames(_window, 1);
    expect(in_view(_window) == all, "Ctrl+Shift+Plus opens every group");
    key(_window, ImGuiKey_KeypadSubtract);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    frames(_window, 2);
    expect(in_view(_window) == groups,
           "Ctrl with the keypad's minus closes them");

    expect(state.steps.can_undo() == undoable,
           "none of it is a step to undo");
    expect(clashes == 0, "opening and closing every group has no ID clash");

    return;
}

// a_wide_sheet
//   a sheet wider than an integer: the probe set past the 64th bit, a bit
// moved there by its cell, the header's arrays of another word, and all of
// it undone.
void
a_wide_sheet(
    app& _window
)
{
    post(_window, "file.example", 18L);   // pci_config_header: 512 bits
    post(_window, "tab.show", 1L);
    frames(_window, 3);

    const bitwisedom::gui::model& state = _window.state();

    expect( (state.doc.width == 512) &&
            (state.res.errors == 0),
           "the PCI example opens, 512 bits wide");
    expect(defined_as(state.header.text, "PCI_CFG_WORDS") == "16",
           "its header holds it in sixteen words");

    post(_window, "grid.probe", 300L);
    frames(_window, 2);
    expect(state.doc.probe.test(300), "the probe takes a bit past the 64th");

    // a bit moved by its cell, and the header following it
    const int rom = id_of(_window, "PCI_CFG_ROM_ENABLE");

    post(_window, "grid.toggle", 385L, static_cast<long>(rom));
    frames(_window, 2);
    expect( (bitwisedom::find_row(state.doc, rom) != nullptr) &&
            (bitwisedom::find_row(state.doc, rom)->bit == 385) &&
            (defined_as(state.header.text, "PCI_CFG_ROM_ENABLE") == "385"),
           "a bit moves past the 64th, and the header follows");

    // bytes for words
    post(_window, "set", 0L, std::string("gen.word"));
    frames(_window, 2);
    expect( (state.doc.gen.word == 8) &&
            (defined_as(state.header.text, "PCI_CFG_WORDS") == "64"),
           "in bytes, it is 64 of them");

    post(_window, "edit.undo", 0L);
    post(_window, "edit.undo", 0L);
    frames(_window, 2);
    expect( (state.doc.gen.word == 32) &&
            (bitwisedom::find_row(state.doc, rom)->bit == 384),
           "and both undo");
    expect(clashes == 0, "a wide sheet has no ID clash");

    return;
}

// json_out_and_in
//   the sheet as JSON beside its file -- the bitfield alone, and everything
// -- and each read back: the bitfield dropped on the window, and from the
// clipboard.
void
json_out_and_in(
    app& _window
)
{
    post(_window, "file.example", 0L);
    post(_window, "file.path", std::string("ui_json.bitwisedom.json"));
    post(_window, "file.export_bitfield", 0L);
    post(_window, "file.export_everything", 0L);
    frames(_window, 2);

    std::string text;
    std::string error;

    expect( (bitwisedom::read_file("ui_json.bitfield.json", text, error)) &&
            (text.find("\"bitwisedom.bitfield\"") != std::string::npos),
           "the bitfield is written beside the sheet");
    expect( (bitwisedom::read_file("ui_json.export.json", text, error)) &&
            (text.find("\"bitwisedom.export\"") != std::string::npos),
           "and everything");

    // dropped, a bitfield is a new sheet, named for its file -- in the
    // new sheet left untouched, which it takes the place of
    const std::size_t rows = _window.state().res.rows.size();

    post(_window, "file.new", 0L);
    post(_window, "file.drop", std::string("ui_json.bitfield.json"));
    frames(_window, 2);
    const std::size_t sheets = _window.documents().size();

    expect( (_window.state().doc.title == "uart_cr") &&
            (_window.state().res.rows.size() == rows) &&
            (_window.state().ui.dirty) &&
            (_window.state().ui.path == "ui_json.bitwisedom.json"),
           "a bitfield dropped is the sheet it was, not yet saved");

    // and from the clipboard
    post(_window, "json.copy", 0L);
    post(_window, "file.new", 0L);
    frames(_window, 1);
    expect(std::string(ImGui::GetClipboardText()).find(
               "\"bitwisedom.bitfield\"") != std::string::npos,
           "the bitfield copied is JSON");

    post(_window, "code.new", 0L);
    frames(_window, 2);
    expect( (_window.state().doc.title == "uart_cr") &&
            (_window.state().ui.import_name == "the clipboard") &&
            (_window.documents().size() == sheets + 1),
           "and pasted, a sheet again, in the new sheet made for it");

    std::remove("ui_json.bitfield.json");
    std::remove("ui_json.export.json");

    return;
}

// add_child
//   a name entered in the tree's "Add child..." field under a group or a
// range, as the field posts it.
void
add_child(
    app&               _window,
    int                _parent,
    const std::string& _name
)
{
    uxoxo::event made;
    made.kind = "tree.add_child";
    made.payload.set("value", static_cast<long>(_parent));
    made.payload.set("text", _name);
    _window.post(made);

    return;
}

// row_height
//   how far apart the grid draws two rows the sheet shows one after the
// other: the first two it shows.
float
row_height(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;

    std::vector<int> shown;

    for (const bitwisedom::resolved_row& each : _window.state().res.rows)
    {
        if (each.visible)
        {
            shown.push_back(each.id);
        }
    }

    if (shown.size() < 2)
    {
        return 0.0f;
    }

    const bitwisedom::gui::grid_point first  = grid_spot("name", shown[0], 0);
    const bitwisedom::gui::grid_point second = grid_spot("name", shown[1], 0);

    return ( (first.found) &&
             (second.found) ) ? std::fabs(second.y - first.y)
                              : 0.0f;
}

void
a_new_sheet(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;

    post(_window, "file.new", 0L);
    frames(_window, 3);

    const bitwisedom::gui::model& state = _window.state();
    const bitwisedom::row&        root  = state.doc.root.children.front();
    const int              flag = id_of(_window, "FLAG");

    expect( (state.doc.root.children.size() == 1) &&
            (root.name == "root") &&
            (root.transparent),
           "a new sheet's root has no fill");
    expect( (root.children.size() == 1) &&
            (flag != 0) &&
            (state.res.find(flag)->value == 0x1u),
           "and holds one bit, FLAG, at bit 0");
    expect( (grid_spot("name", root.id, 0).found) &&
            (grid_spot("cell", flag, 0).found),
           "the grid draws them both");

    // the grid's add row: a press on its words opens the menu of what to
    // add, as one on its "+" does
    expect(move_to(_window, grid_spot("add", 0, 0)),
           "the grid draws its add row's words");
    click(_window);
    expect( (state.ui.menu_kind == "sheet") &&
            (state.ui.menu_where == "add") &&
            (popups() == 1),
           "a press on \"Add new...\" opens the menu of what to add");
    key(_window, ImGuiKey_Escape);
    expect(popups() == 0, "which Escape closes");

    expect(clashes == 0, "a new sheet has no ID clash");

    return;
}

void
children_added(
    app& _window
)
{
    // the tree's own window, where its fields are
    post(_window, "view.symbols", true);
    post(_window, "file.new", 0L);
    frames(_window, 2);

    const bitwisedom::gui::model& state = _window.state();
    const int                     root  = state.doc.root.children.front().id;

    // a name entered: a bit of that name, at the next bit free in the range
    add_child(_window, root, "  SECOND ");
    frames(_window, 2);

    const int second = id_of(_window, "SECOND");

    expect( (second != 0) &&
            (state.res.find(second)->value == 0x2u) &&
            (state.res.find(second)->parent == root),
           "a name entered is a bit in the range, at its next free bit");
    expect( (state.ui.selection.size() == 1) &&
            (state.ui.selection.front() == second),
           "and is selected");
    expect( (state.ui.adding_to == root) &&
            (state.ui.adding_text.empty()) &&
            (!state.ui.status_error),
           "and the field is given the keyboard back");

    // the field has the keyboard: a name typed there and entered
    frames(_window, 3);
    type(_window, "THIRD");
    frames(_window, 2);

    const int third = id_of(_window, "THIRD");

    expect( (third != 0) &&
            (state.res.find(third)->value == 0x4u),
           "a name typed in the field, which has the keyboard, is added");

    // the field, empty, lets the keyboard go
    frames(_window, 3);
    key(_window, ImGuiKey_Escape);

    // refused: no C identifier, or another symbol's name
    const std::size_t rows = state.res.rows.size();

    add_child(_window, root, "3RD");
    frames(_window, 2);
    expect( (state.res.rows.size() == rows) &&
            (state.ui.status_error) &&
            (state.ui.adding_text == "3RD"),
           "a name that is no C identifier is refused, and given back");

    // the name given back: cleared, and the field left
    frames(_window, 3);
    key(_window, ImGuiKey_Escape);
    key(_window, ImGuiKey_Escape);

    add_child(_window, root, "SECOND");
    frames(_window, 2);
    expect( (state.res.rows.size() == rows) &&
            (state.ui.status_error),
           "and so is another symbol's name");

    frames(_window, 3);
    key(_window, ImGuiKey_Escape);
    key(_window, ImGuiKey_Escape);

    // into a group in the range: at the lowest bit the group leaves free,
    // as a bit added to it any other way
    post(_window, "select", static_cast<long>(root));
    post(_window, "edit.add", -1L);
    frames(_window, 2);

    const int group = state.ui.selection.empty() ? 0
                                                 : state.ui.selection.front();

    add_child(_window, group, "INNER");
    frames(_window, 2);

    const int inner = id_of(_window, "INNER");

    expect( (inner != 0) &&
            (state.res.find(inner)->parent == group) &&
            (state.res.find(inner)->value == 0x1u),
           "a group's field adds to the group");

    // a step of undo each
    post(_window, "edit.undo", 0L);
    frames(_window, 2);
    expect( (id_of(_window, "INNER") == 0) &&
            (id_of(_window, "THIRD") != 0),
           "a child added is one step of undo");

    expect(clashes == 0, "no field has an ID clash");

    post(_window, "view.symbols", false);
    frames(_window, 2);

    return;
}

void
square_cells(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;

    post(_window, "file.example", 9L);   // win32_hresult: 32 bits
    frames(_window, 4);

    const bitwisedom::grid_settings& grid = _window.state().doc.grid;

    const int ok    = id_of(_window, "WIN_S_OK");
    auto      width = [&_window, ok]()
    {
        return std::fabs(grid_spot("cell", ok, 1).x -
                         grid_spot("cell", ok, 0).x);
    };
    // proportional
    //   whether the rows are the share of the cells' width the sheet says,
    // or the shortest a row may be.
    auto proportional = [&_window, &grid](float _wide)
    {
        const float tall = row_height(_window);

        return (tall > 0.0f) &&
               ( (std::fabs(tall - std::floor(_wide * grid.cell_ratio)) <=
                  1.0f) ||
                 (tall > std::floor(_wide * grid.cell_ratio)) );
    };

    float wide = width();
    float tall = row_height(_window);

    expect( (grid.square_cells) &&
            (std::fabs(grid.cell_ratio - 0.8f) < 0.001f),
           "rows follow the cells' width by default, a fifth shorter");
    expect( (tall > 0.0f) &&
            (std::fabs(tall - std::floor(wide * 0.8f)) <= 1.0f),
           "a bit's cell is four fifths as tall as it is wide");
    expect( (wide > grid.cell_width + 1.0f) &&
            (std::fabs(grid_spot("end", 0, 0).x -
                       grid_spot("room", 0, 0).x) <= 1.0f),
           "and the cells still widen to fill the grid");

    // the cell width set by hand: the cells take it, the room to spare goes
    // to nothing, and the rows follow
    post(_window, "set", 30.0, std::string("grid.cell_width"));
    frames(_window, 3);
    wide = width();

    expect( (std::fabs(wide - 30.0f) <= 1.0f) &&
            (grid.fill == bitwisedom::grid_fill::none) &&
            (std::fabs(row_height(_window) - 24.0f) <= 1.0f),
           "a cell width set by hand is the cells' width, and the rows "
           "follow it");
    expect(grid_spot("end", 0, 0).x < grid_spot("room", 0, 0).x - 1.0f,
           "and the room past them is left");

    // narrower than a line of digits needs for its row: the cells narrow,
    // the rows stop
    post(_window, "set", 12.0, std::string("grid.cell_width"));
    frames(_window, 3);

    expect( (std::fabs(width() - 12.0f) <= 1.0f) &&
            (row_height(_window) >= ImGui::GetTextLineHeight()) &&
            (proportional(width())),
           "a narrow cell width narrows the cells, and the rows stop at a "
           "line of digits");

    // the share: square
    post(_window, "set", 30.0, std::string("grid.cell_width"));
    post(_window, "set", 1.0, std::string("grid.cell_ratio"));
    frames(_window, 3);

    expect(std::fabs(row_height(_window) - width()) <= 1.0f,
           "a share of 1.0 is square");

    // undone, all of it: the fill is back with the width it replaced
    for (int i = 0; i < 4; ++i)
    {
        post(_window, "edit.undo", 0L);
        frames(_window, 2);
    }

    expect( (grid.fill == bitwisedom::grid_fill::cells) &&
            (std::fabs(grid.cell_width - 20.0f) < 0.001f) &&
            (std::fabs(grid.cell_ratio - 0.8f) < 0.001f) &&
            (width() > grid.cell_width + 1.0f),
           "undone, the cells fill the grid again");

    // turned off: the rows take the cell height, the cells the width
    post(_window, "set", false, std::string("grid.square_cells"));
    frames(_window, 3);
    wide = width();
    tall = row_height(_window);

    expect( (!grid.square_cells) &&
            (std::fabs(tall - grid.cell_height) <= 1.0f) &&
            (wide > tall + 1.0f),
           "turned off, the rows are as tall as the cell height says");

    post(_window, "set", 40.0, std::string("grid.cell_height"));
    frames(_window, 3);
    expect(std::fabs(row_height(_window) - 40.0f) <= 1.0f,
           "and the cell height sizes them");

    post(_window, "edit.undo", 0L);
    frames(_window, 2);
    post(_window, "edit.undo", 0L);
    frames(_window, 3);
    expect(grid.square_cells, "and the choice is undone like any setting");

    // a narrow sheet in a wide grid: the cells stop at the most a cell
    // may be, and the room beyond them is left
    post(_window, "file.example", 3L);   // usb_request_type: 8 bits
    frames(_window, 4);

    const int   first = _window.state().res.rows.empty()
                            ? 0
                            : _window.state().res.rows.front().id;
    const float shown = (first != 0)
                            ? std::fabs(grid_spot("cell", first, 6).x -
                                        grid_spot("cell", first, 5).x)
                            : 0.0f;

    expect( (shown > 0.0f) &&
            (shown <= 56.0f + 1.0f) &&
            (std::fabs(std::floor(shown * 0.8f) - row_height(_window)) <=
             1.0f),
           "a narrow sheet's cells stop at the widest a cell grows");
    expect(grid_spot("end", 0, 0).x < grid_spot("room", 0, 0).x - 1.0f,
           "and the grid ends where they do");

    return;
}

void
the_menu_bar(
    app& _window
)
{
    ImGuiIO& io = ImGui::GetIO();

    post(_window, "file.example", 0L);
    frames(_window, 2);

    // Help, at the menu bar's right end
    io.AddMousePosEvent(io.DisplaySize.x - 18.0f,
                        ImGui::GetFrameHeight() * 0.5f);
    frames(_window, 2);
    click(_window);
    expect( (popups() == 1) &&
            (GImGui->OpenPopupStack[0].Window != nullptr) &&
            (GImGui->OpenPopupStack[0].Window->Pos.x >
                 io.DisplaySize.x * 0.75f),
           "a press at the menu bar's right end opens Help");
    key(_window, ImGuiKey_Escape);
    expect(popups() == 0, "which Escape closes");

    // File, at its left end, as ever
    io.AddMousePosEvent(20.0f, ImGui::GetFrameHeight() * 0.5f);
    frames(_window, 2);
    click(_window);
    expect( (popups() == 1) &&
            (GImGui->OpenPopupStack[0].Window != nullptr) &&
            (GImGui->OpenPopupStack[0].Window->Pos.x <
                 io.DisplaySize.x * 0.25f),
           "and one at its left end opens File");
    key(_window, ImGuiKey_Escape);

    // Help's pages
    post(_window, "help.show", 0L);
    frames(_window, 2);
    expect(window_shown("Keyboard shortcuts##bitwisedom"),
           "Help shows the keyboard shortcuts");
    key(_window, ImGuiKey_Escape);
    frames(_window, 1);
    expect(popups() == 0, "and Escape puts them away");

    post(_window, "help.show", 1L);
    frames(_window, 2);
    expect(window_shown("About bitwis(e)dom##bitwisedom"),
           "and what bitwis(e)dom is");
    key(_window, ImGuiKey_Escape);
    frames(_window, 1);
    expect(popups() == 0, "which Escape puts away too");

    // the theme, under View now, as the Theme menu posted it
    post(_window, "view.theme", 1L);
    frames(_window, 2);
    expect(_window.state().ui.theme == 1, "the theme is changed");
    post(_window, "view.theme", 0L);
    frames(_window, 2);

    expect(clashes == 0, "the menu bar has no ID clash");

    return;
}

// first_shown
//   the first of the sheet's rows the grid shows, or 0.
int
first_shown(
    app& _window
)
{
    for (const bitwisedom::resolved_row& each : _window.state().res.rows)
    {
        if (each.visible)
        {
            return each.id;
        }
    }

    return 0;
}

// fitted_names
//   the name column fitted to what it writes, as a double-click on its
// edge fits it: the width the sheet is given.
float
fitted_names(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;

    if (!move_to(_window, grid_spot("edge.name", 0, 0)))
    {
        return 0.0f;
    }

    double_click(_window);
    frames(_window, 2);

    return _window.state().doc.grid.name_width;
}

/*
symbols_by
  The buttons over the symbols column, which choose what it lists each row
by: drawn side by side in the band strip, over the column and above its
label. A press on one lists the rows by symbol name, name or value -- one
step to undo, and none for the choice already made -- and the column fits
what it then writes. The mouse on one is a hand, and resting there explains
it; a press on one starts no drag of the heading, where a drag from the
label still swaps the columns. A narrow column takes shorter words, a
hidden one has none, and with no bands and no ruler the buttons keep their
strip, the label its line below it.
*/
void
symbols_by(
    app& _window
)
{
    using bitwisedom::gui::grid_point;
    using bitwisedom::gui::grid_spot;

    post(_window, "file.example", 0L);   // uart_cr
    frames(_window, 4);

    const bitwisedom::gui::model&    state = _window.state();
    const bitwisedom::grid_settings& grid  = state.doc.grid;
    const float                      line  = ImGui::GetStyle().FontSizeBase;

    const grid_point full  = grid_spot("names_by.symbol", 0, 0);
    const grid_point own   = grid_spot("names_by.name", 0, 0);
    const grid_point value = grid_spot("names_by.value", 0, 0);
    const grid_point edge  = grid_spot("edge.name", 0, 0);
    const grid_point row   = grid_spot("name", first_shown(_window), 0);
    const grid_point band  = grid_spot("band", 0, 0);
    const float      tall  = row_height(_window);

    expect( (full.found)  &&
            (own.found)   &&
            (value.found) &&
            (row.found)   &&
            (tall > 0.0f),
           "the symbols column has its three buttons");
    expect( (full.x < own.x)                      &&
            (own.x < value.x)                     &&
            (std::fabs(full.y - value.y) < 0.5f),
           "side by side: symbol name, name, value");
    expect( (full.x > 0.0f) &&
            (value.x < edge.x),
           "over the symbols column");
    expect( (band.found)                                    &&
            (std::fabs(value.y - band.y) <= 1.0f)           &&
            (value.y < row.y - tall * 0.5f - line - 3.0f),
           "in the band strip, above the column's label");
    expect( (!grid.full_names) &&
            (!grid.names_by_value),
           "the rows are listed by their own names at first");

    // by value: one press, one step to undo
    expect(move_to(_window, value), "the mouse is on the value button");
    expect(ImGui::GetMouseCursor() == ImGuiMouseCursor_Hand,
           "where it is a hand");
    click(_window);
    expect( (grid.names_by_value) &&
            (!grid.full_names),
           "a press on value lists the rows by value");

    rest(_window);
    expect(tooltip_shown(), "a resting mouse on a button explains it");

    post(_window, "edit.undo", 0L);
    frames(_window, 2);
    expect(!grid.names_by_value, "the choice is one step to undo");
    post(_window, "edit.redo", 0L);
    frames(_window, 2);
    expect(grid.names_by_value, "and to redo");

    // the column fits what it writes: values, macro names, names
    const float by_value = fitted_names(_window);

    expect(move_to(_window, grid_spot("names_by.symbol", 0, 0)),
           "the symbol name button is drawn");
    click(_window);
    expect( (grid.full_names) &&
            (!grid.names_by_value),
           "a press on symbol name lists the rows by their macro names");

    const float by_symbol = fitted_names(_window);

    expect(move_to(_window, grid_spot("names_by.name", 0, 0)),
           "the name button is drawn");
    click(_window);
    expect( (!grid.full_names) &&
            (!grid.names_by_value),
           "and a press on name by their own names again");

    const float by_name = fitted_names(_window);

    expect( (by_symbol > by_name + 10.0f) &&
            (std::fabs(by_value - by_name) > 0.5f),
           "the column fitted fits what it writes");

    // the choice already made: no step
    const std::size_t depth = state.steps.depth();

    expect(move_to(_window, grid_spot("names_by.name", 0, 0)),
           "the chosen button is drawn");
    click(_window);
    expect( (state.steps.depth() == depth) &&
            (!grid.full_names) &&
            (!grid.names_by_value),
           "a press on the button already chosen changes nothing");

    // a press on a button is no drag of the heading; one from the label is
    post(_window, "set", true, std::string("grid.show_values"));
    frames(_window, 3);

    const grid_point chosen = grid_spot("names_by.name", 0, 0);
    const grid_point names  = grid_spot("edge.name", 0, 0);
    const grid_point values = grid_spot("edge.value", 0, 0);
    const float      over   = (names.x + values.x) * 0.5f;

    expect( (move_to(_window, chosen)) &&
            (values.found),
           "with the value column shown, the buttons are still drawn");
    drag_by(_window, over - chosen.x);
    expect(!grid.values_first,
           "a drag from a button does not swap the columns");

    ImGui::GetIO().AddMousePosEvent(chosen.x,
                                    row.y - tall * 0.5f - line * 0.5f - 3.0f);
    frames(_window, 2);
    drag_by(_window, over - chosen.x);
    expect(grid.values_first, "where a drag from the label still does");

    post(_window, "set", false, std::string("grid.values_first"));
    post(_window, "set", false, std::string("grid.show_values"));
    frames(_window, 3);

    // a narrow column: shorter words, still within it
    const float spread = value.x - full.x;

    post(_window, "set", 80.0, std::string("grid.name_width"));
    frames(_window, 3);

    const grid_point narrow_full  = grid_spot("names_by.symbol", 0, 0);
    const grid_point narrow_value = grid_spot("names_by.value", 0, 0);

    expect( (narrow_full.found)                               &&
            (narrow_value.found)                              &&
            (narrow_value.x < grid_spot("edge.name", 0, 0).x) &&
            (narrow_value.x - narrow_full.x < spread - 1.0f),
           "a narrow column has shorter buttons, within it");

    // a hidden column: none
    post(_window, "set", false, std::string("grid.show_names"));
    frames(_window, 3);
    expect(!grid_spot("names_by.name", 0, 0).found,
           "a hidden symbols column has no buttons");
    post(_window, "set", true, std::string("grid.show_names"));
    frames(_window, 3);

    // no bands, no ruler: the buttons keep their strip, the label its line
    post(_window, "file.new", 0L);
    frames(_window, 3);
    post(_window, "set", 0L, std::string("grid.auto_segments"));
    frames(_window, 3);

    const float ruled = grid_spot("name", first_shown(_window), 0).y;

    post(_window, "set", false, std::string("grid.show_indices"));
    frames(_window, 3);

    const grid_point bare  = grid_spot("names_by.value", 0, 0);
    const grid_point first = grid_spot("name", first_shown(_window), 0);

    expect( (_window.state().doc.segments.empty()) &&
            (bare.found) &&
            (first.found) &&
            (bare.y < first.y - tall * 0.5f - line - 3.0f),
           "with no bands and no ruler, the buttons are still above the "
           "label");
    expect(std::fabs(first.y - ruled) < 0.5f,
           "and the heading is as tall as with the ruler");

    expect(clashes == 0, "the buttons have no ID clash");

    return;
}

/*
values_every_way
  The value button, once the symbols column lists rows by value, writes
them another way at each press: as the header writes them, following the
tree; then every row alike, lined up at the column's right -- in hex,
decimal and binary, as a shifted run, as the bits set -- and round again,
the run of presses one step of undo. A right-click on a button lists every way, and
a way chosen from it lists the rows so. A sheet wider than an integer
passes over decimal and binary. And a macro with no bits has its cells,
with its definition written over them as the header writes it.
*/
void
values_every_way(
    app& _window
)
{
    using bitwisedom::value_form;
    using bitwisedom::gui::grid_spot;
    using bitwisedom::gui::grid_written;

    post(_window, "file.example", 0L);   // uart_cr
    frames(_window, 4);

    const bitwisedom::gui::model&    state = _window.state();
    const bitwisedom::grid_settings& grid  = state.doc.grid;

    post(_window, "set", 320.0, std::string("grid.name_width"));
    frames(_window, 3);

    const int en     = id_of(_window, "UART_CR_EN");
    const int parity = id_of(_window, "UART_CR_PARITY");
    const int even   = id_of(_window, "UART_CR_PARITY_EVEN");

    // by value, as the header writes them: where the names were
    const std::size_t before = state.steps.depth();

    expect(move_to(_window, grid_spot("names_by.value", 0, 0)),
           "the value button is drawn");
    click(_window);
    expect( (grid.names_by_value) &&
            (grid.names_value_form == value_form::written) &&
            (grid_written(en).label == "0x00 00 00 01") &&
            (grid_written(even).label == "= 2"),
           "a first press lists the rows by value, as the header writes "
           "them");
    expect(std::fabs(grid_written(en).end - grid_written(parity).end) > 1.0f,
           "following the tree, where the names were");

    // way
    //   one way of writing values, and three rows written that way.
    struct way
    {
        value_form  form;
        const char* en;
        const char* even;
        const char* parity;
    };

    const way ways[] = {
        { value_form::hex,   "0x00 00 00 01", "0x00 00 00 20",
                             "0x00 00 00 30" },
        { value_form::dec,   "1", "32", "48" },
        { value_form::bin,   "0b0001", "0b0010 0000", "0b0011 0000" },
        { value_form::shift, "1 << 0", "0x2 << 4", "0x3 << 4" },
        { value_form::bits,  "bit 0", "bit 5", "bits 5:4" } };
    bool written = true;
    bool lined   = true;

    for (const way& each : ways)
    {
        click(_window);
        frames(_window, 1);

        written = ( (written) &&
                    (grid.names_by_value) &&
                    (grid.names_value_form == each.form) &&
                    (grid_written(en).label == each.en) &&
                    (grid_written(even).label == each.even) &&
                    (grid_written(parity).label == each.parity) );
        lined   = ( (lined) &&
                    (std::fabs(grid_written(en).end -
                               grid_written(parity).end) < 0.5f) &&
                    (std::fabs(grid_written(en).end -
                               grid_written(even).end) < 0.5f) );
    }

    expect(written, "each press writes them the next way: hex, decimal, "
                    "binary, a shifted run, the bits set");
    expect(lined, "every row's alike, lined up at the column's right");

    click(_window);
    expect( (grid.names_by_value) &&
            (grid.names_value_form == value_form::written),
           "and round again to the way the header writes them");

    // presses one after another are one step, as a slider's drag is
    expect(state.steps.depth() == before + 1,
           "a run of presses is one step to undo");
    post(_window, "edit.undo", 0L);
    frames(_window, 2);
    expect(!grid.names_by_value, "which lists the rows by name again");
    post(_window, "edit.redo", 0L);
    frames(_window, 2);

    // every way, on a right-click
    expect(move_to(_window, grid_spot("names_by.name", 0, 0)),
           "the name button is drawn");
    click(_window, ImGuiMouseButton_Right);
    expect( (state.ui.menu_kind == "names_by") &&
            (popups() == 1),
           "a right-click on a button lists every way to list the rows");
    key(_window, ImGuiKey_Escape);

    post(_window, "set",
         static_cast<long>(bitwisedom::NAMES_BY_VALUE + 4),
         std::string("grid.names_by"));
    frames(_window, 2);
    expect( (grid.names_by_value) &&
            (grid.names_value_form == value_form::shift) &&
            (grid_written(parity).label == "0x3 << 4"),
           "a way chosen from the list lists the rows so");

    // a macro with no bits: its cells, and its definition over them
    const int presets = id_of(_window, "UART_CR_IS_ENABLED");

    post(_window, "tree.close_all", 0L);
    frames(_window, 2);

    for (const bitwisedom::resolved_row& each : state.res.rows)
    {
        if (each.local == "Presets")
        {
            uxoxo::event opened;
            opened.kind = "toggle_open";
            opened.payload.set("value", static_cast<long>(each.id));
            opened.payload.set("open", true);
            _window.post(opened);
        }
    }

    frames(_window, 3);
    expect( (grid_written(presets).found) &&
            (grid_written(presets).note ==
             "UART_CR_IS_ENABLED(x) (((x) & UART_CR_EN) != 0)"),
           "a macro with no bits is written over its cells as the header "
           "writes it");

    // a sheet wider than an integer passes over decimal and binary
    post(_window, "file.example", example_index("pci_config_header"));
    post(_window, "tab.show", 0L);
    frames(_window, 4);
    post(_window, "set", static_cast<long>(bitwisedom::NAMES_BY_VALUE),
         std::string("grid.names_by"));
    frames(_window, 2);

    std::vector<value_form> seen;

    for (int i = 0; i < 4; ++i)
    {
        expect(move_to(_window, grid_spot("names_by.value", 0, 0)),
               "the wide sheet's value button is drawn");
        click(_window);
        seen.push_back(_window.state().doc.grid.names_value_form);
    }

    expect( (seen.size() == 4) &&
            (seen[0] == value_form::hex) &&
            (seen[1] == value_form::shift) &&
            (seen[2] == value_form::bits) &&
            (seen[3] == value_form::written),
           "a sheet wider than an integer passes over decimal and binary");

    expect(clashes == 0, "every way has no ID clash");

    return;
}

// --------------------------------------------------------- the shell

// doc_window
//   a document's ImGui window, or nullptr.
ImGuiWindow*
doc_window(
    int _document
)
{
    return ImGui::FindWindowByName(
        ("###" + bitwisedom::gui::document_key(_document)).c_str());
}

// doc_in_view
//   whether a document's window drew its grid in the last frame.
bool
doc_in_view(
    int _document
)
{
    return bitwisedom::gui::window_seen(
        bitwisedom::gui::document_key(_document));
}

/*
several_sheets
  Sheets side by side, each in a window of its own: File > New opens one
beside the rest, and an example takes the place of a new sheet untouched;
the menu brings another to the front; and a sheet with changes asks before
it closes, where one without closes at once.
*/
void
several_sheets(
    app& _window
)
{
    ImGuiIO& io = ImGui::GetIO();

    post(_window, "file.new", 0L);
    frames(_window, 3);

    const std::size_t count = _window.documents().size();
    const int         fresh = _window.state().id;

    expect( (_window.state().ui.pristine) &&
            (doc_in_view(fresh)),
           "a new sheet opens beside the rest, in front");

    post(_window, "file.example", 2L);
    frames(_window, 3);
    expect( (_window.documents().size() == count) &&
            (_window.state().id == fresh) &&
            (_window.state().doc.title == "posix_mode") &&
            (_window.state().ui.browsing),
           "an example takes the place of a new sheet untouched");

    post(_window, "file.new", 0L);
    frames(_window, 3);

    const int other = _window.state().id;

    expect( (_window.documents().size() == count + 1) &&
            (other != fresh) &&
            (doc_in_view(other)) &&
            (!doc_in_view(fresh)),
           "another opens beside it, in front of it");

    post(_window, "doc.activate", static_cast<long>(fresh));
    frames(_window, 3);
    expect( (_window.state().id == fresh) &&
            (doc_in_view(fresh)),
           "the menu brings a sheet back to the front");

    // changed, it asks first; Escape keeps it open
    post(_window, "set", false, std::string("grid.lines"));
    frames(_window, 2);
    post(_window, "doc.close", 0L);
    frames(_window, 3);
    expect( (popups() == 1) &&
            (_window.document(fresh) != nullptr),
           "a sheet with changes asks before it closes");

    key(_window, ImGuiKey_Escape);
    frames(_window, 2);
    expect( (popups() == 0) &&
            (_window.document(fresh) != nullptr),
           "and stays open when asked not to");

    post(_window, "doc.discard", static_cast<long>(fresh));
    frames(_window, 3);
    expect( (_window.document(fresh) == nullptr) &&
            (_window.documents().size() == count),
           "closed without saving, it is gone");

    // a sheet with no changes closes at once, by Ctrl+W
    post(_window, "doc.activate", static_cast<long>(other));
    frames(_window, 2);
    io.AddMousePosEvent(io.DisplaySize.x * 0.5f, 300.0f);
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    key(_window, ImGuiKey_W);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    frames(_window, 2);
    expect( (_window.document(other) == nullptr) &&
            (popups() == 0),
           "Ctrl+W closes a sheet with no changes at once");

    expect(clashes == 0, "several sheets have no ID clash");

    return;
}

/*
the_workspaces
  Workspaces as a Linux desktop has them: each its own sheets and tool
windows; a new one, named; the sheet in front moved to another; the
workspaces stepped through; and one closed, its sheets kept in the next.
*/
void
the_workspaces(
    app& _window
)
{
    using bitwisedom::gui::tool;

    const bitwisedom::gui::shell_state& shell = _window.shell();
    const std::size_t                   start = shell.workspaces.size();
    const int                           first = _window.state().id;

    post(_window, "workspace.new", 0L);
    frames(_window, 3);
    expect( (shell.workspaces.size() == start + 1) &&
            (shell.current == start) &&
            (here(_window).documents.size() == 1) &&
            (_window.state().ui.pristine) &&
            (doc_in_view(_window.state().id)) &&
            (tool_in_view(_window, tool::inspector)),
           "a new workspace has a new sheet and the tool windows");

    post(_window, "workspace.rename", std::string("  Second desk "));
    frames(_window, 2);
    expect(here(_window).name == "Second desk", "it takes a name");

    const int second = _window.state().id;

    // the combo's last choice is yet another; its others switch
    post(_window, "workspace.switch", 0L);
    frames(_window, 3);
    expect( (shell.current == 0) &&
            (doc_in_view(_window.state().id)) &&
            (!doc_in_view(second)),
           "switching shows that workspace's sheets alone");

    // the sheet in front, moved to the new workspace, goes with it
    const int moved = _window.state().id;

    post(_window, "workspace.move", static_cast<long>(start));
    frames(_window, 4);
    expect( (shell.current == start) &&
            (_window.state().id == moved) &&
            (here(_window).documents.size() == 2) &&
            (doc_in_view(moved)),
           "a sheet moved to a workspace is in front there");

    post(_window, "workspace.previous", 0L);
    frames(_window, 3);
    expect(shell.current == start - 1, "the workspaces step back");

    post(_window, "workspace.next", 0L);
    frames(_window, 3);
    expect(shell.current == start, "and forward");

    // closed: its sheets go to the workspace before it
    post(_window, "workspace.close", static_cast<long>(start));
    frames(_window, 4);
    expect( (shell.workspaces.size() == start) &&
            (_window.document(second) != nullptr) &&
            (_window.document(moved) != nullptr) &&
            (_window.document(first) != nullptr),
           "a workspace closed keeps its sheets in the one before");

    expect(clashes == 0, "the workspaces have no ID clash");

    return;
}

/*
arrangements
  The sheets of a workspace laid out as the Window menu asks: side by side
in columns, one above another in rows, in a grid, and as tabs again.
*/
void
arrangements(
    app& _window
)
{
    // three sheets, each with a grid
    for (long example : { 0L, 4L, 8L })
    {
        post(_window, "file.new", 0L);
        post(_window, "file.example", example);
        frames(_window, 3);
    }

    const std::vector<int> ids   = here(_window).documents;
    const std::size_t      count = ids.size();

    // the rest closed: three, to lay out
    for (std::size_t i = 0; i + 3 < count; ++i)
    {
        post(_window, "doc.discard", static_cast<long>(ids[i]));
    }

    frames(_window, 3);

    const ImGuiWindow* a = doc_window(ids[count - 3]);
    const ImGuiWindow* b = doc_window(ids[count - 2]);
    const ImGuiWindow* c = doc_window(ids[count - 1]);

    post(_window, "window.arrange", 1L);
    frames(_window, 4);
    expect( (here(_window).arrange == -1) &&
            (std::fabs(a->Pos.y - b->Pos.y) < 1.0f) &&
            (std::fabs(b->Pos.y - c->Pos.y) < 1.0f) &&
            (a->Pos.x < b->Pos.x) &&
            (b->Pos.x < c->Pos.x) &&
            (std::fabs(a->Size.x - c->Size.x) < 8.0f),
           "in columns: side by side, as wide as each other");

    post(_window, "window.arrange", 2L);
    frames(_window, 4);
    expect( (std::fabs(a->Pos.x - b->Pos.x) < 1.0f) &&
            (std::fabs(b->Pos.x - c->Pos.x) < 1.0f) &&
            (a->Pos.y < b->Pos.y) &&
            (b->Pos.y < c->Pos.y) &&
            (std::fabs(a->Size.y - c->Size.y) < 40.0f),
           "in rows: one above another, as tall as each other");

    post(_window, "window.arrange", 3L);
    frames(_window, 4);
    expect( (std::fabs(a->Pos.y - b->Pos.y) < 1.0f) &&
            (a->Pos.x < b->Pos.x) &&
            (c->Pos.y > a->Pos.y + 10.0f) &&
            (std::fabs(a->Pos.x - c->Pos.x) < 1.0f) &&
            (std::fabs(a->Size.y - c->Size.y) < 40.0f),
           "in a grid: two above, one below, the rows alike");

    post(_window, "window.arrange", 0L);
    frames(_window, 4);
    expect( (a->DockNode != nullptr) &&
            (a->DockNode == b->DockNode) &&
            (b->DockNode == c->DockNode),
           "as tabs: one window's tabs again");

    expect(clashes == 0, "the arrangements have no ID clash");

    return;
}

/*
zoom_like_a_browser
  Each window zoomed on its own, as a browser's page is: a step at a time,
by Ctrl and the wheel over it or the menu; held between a half and three
times; and back to its actual size by Ctrl+0.
*/
void
zoom_like_a_browser(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;
    using bitwisedom::gui::zoom_of;

    ImGuiIO&          io    = ImGui::GetIO();
    const int         front = _window.state().id;
    const std::string own   = bitwisedom::gui::document_key(front);

    frames(_window, 2);

    const int first = first_shown(_window);

    // how far below the window's top the first row's name is: the heading
    // above it grows with the text
    auto depth = [&]()
    {
        const ImGuiWindow*                 shown = doc_window(front);
        const bitwisedom::gui::grid_point name  =
            grid_spot(front, "name", first, 0);

        return ( (shown != nullptr) &&
                 (name.found) ) ? (name.y - shown->Pos.y) : 0.0f;
    };

    const float plain = depth();

    post(_window, "view.zoom", 2L, own);
    frames(_window, 3);

    const float larger = depth();

    expect( (std::fabs(zoom_of(_window.shell(), own) - 1.2f) < 0.01f) &&
            (plain > 0.0f) &&
            (larger > plain + 2.0f),
           "two steps in: 120%, and the sheet drawn larger");

    // Ctrl and the wheel over the window
    const ImGuiWindow* shown = doc_window(front);

    io.AddMousePosEvent(shown->Pos.x + shown->Size.x * 0.5f,
                        shown->Pos.y + shown->Size.y * 0.5f);
    frames(_window, 2);
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    io.AddMouseWheelEvent(0.0f, 1.0f);
    frames(_window, 3);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    frames(_window, 2);
    expect(std::fabs(zoom_of(_window.shell(), own) - 1.3f) < 0.01f,
           "Ctrl and the wheel over it, another step");

    // held to three times
    post(_window, "view.zoom", 100L, own);
    frames(_window, 2);
    expect(std::fabs(zoom_of(_window.shell(), own) -
                     bitwisedom::gui::ZOOM_MOST) < 0.01f,
           "held to three times its size");

    // another window's zoom is its own
    const std::string inspector =
        tool_key(_window, bitwisedom::gui::tool::inspector);

    post(_window, "view.zoom", -3L, inspector);
    frames(_window, 2);
    expect( (std::fabs(zoom_of(_window.shell(), inspector) - 0.7f) < 0.01f) &&
            (std::fabs(zoom_of(_window.shell(), own) -
                       bitwisedom::gui::ZOOM_MOST) < 0.01f),
           "the inspector zooms on its own");

    // Ctrl+0: the window in front at its actual size
    io.AddMousePosEvent(shown->Pos.x + shown->Size.x * 0.5f,
                        shown->Pos.y + shown->Size.y * 0.5f);
    frames(_window, 2);
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    key(_window, ImGuiKey_0);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    frames(_window, 3);
    expect( (zoom_of(_window.shell(), own) == 1.0f) &&
            (std::fabs(depth() - plain) < 0.5f),
           "Ctrl+0 puts the window in front back to its actual size");

    post(_window, "view.zoom_reset_all", 0L);
    frames(_window, 2);
    expect(_window.shell().zoom.empty(), "and every window, from the menu");

    expect(clashes == 0, "zoom has no ID clash");

    return;
}

// ---------------------------------------------------------- the clipboard

// clipboard_text / clipboard_get / clipboard_set / own_clipboard
//   the smoke test's own clipboard, in memory, which every ImGui it makes
// reads and writes. ImGui's default on Windows is the system's, which a
// program watching it -- the clipboard history, a remote desktop, a
// password manager -- opens the moment after each copy: a copy straight
// after another then fails without a word, and the test reads the text
// before it. Its own leaves the user's clipboard alone, too.
std::string&
clipboard_text()
{
    static std::string text;

    return text;
}

const char*
clipboard_get(
    ImGuiContext* /* _context */
)
{
    return clipboard_text().c_str();
}

void
clipboard_set(
    ImGuiContext* /* _context */,
    const char*   _text
)
{
    clipboard_text() = (_text != nullptr) ? _text : "";

    return;
}

void
own_clipboard()
{
    ImGuiPlatformIO& platform = ImGui::GetPlatformIO();

    platform.Platform_GetClipboardTextFn = clipboard_get;
    platform.Platform_SetClipboardTextFn = clipboard_set;

    return;
}

// ------------------------------------------------------- the next run

// an_imgui
//   an ImGui of its own, for a window of its own: made current, its null
// backend and fonts ready; and done with, the ImGui before it current
// again, with the monospace font it had.
ImGuiContext*
an_imgui()
{
    ImGuiContext* made = ImGui::CreateContext();

    ImGui::SetCurrentContext(made);
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplNull_Init();
    own_clipboard();
    app::load_fonts(16.0f);

    return made;
}

void
done_with(
    ImGuiContext* _made,
    ImGuiContext* _before,
    ImFont*       _mono
)
{
    ImGui_ImplNull_Shutdown();
    ImGui::DestroyContext(_made);
    ImGui::SetCurrentContext(_before);
    bitwisedom::gui::mono_font() = _mono;

    return;
}

/*
the_next_run
  The window closed and opened again opens where it was left: its
workspaces by name, the one in front, every sheet in them -- a change not
yet saved too -- each window's zoom, the tool windows open and closed, and
the sheets laid out as they were. Each run is a window of its own, in an
ImGui of its own, keeping its settings and its session in files of their
own.
*/
void
the_next_run(
    app& _window
)
{
    using bitwisedom::gui::document_key;
    using bitwisedom::gui::tool;
    using bitwisedom::gui::zoom_of;

    const std::string settings = "ui_session.ini";
    const std::string session  = "ui_session.json";
    ImGuiContext*     before   = ImGui::GetCurrentContext();
    ImFont*           mono     = bitwisedom::gui::mono_font();
    int               changed  = 0;
    int               example  = 0;

    (void)_window;
    std::remove(settings.c_str());
    std::remove(session.c_str());

    // the first run: a sheet changed and zoomed beside the example, laid
    // out in columns, the inspector closed; and a second workspace, named,
    // in front, with an example of its own
    {
        ImGuiContext* first = an_imgui();

        {
            app run(settings, session);

            frames(run, 3);
            post(run, "file.new", 0L);
            frames(run, 3);
            changed = run.state().id;
            post(run, "set", false, std::string("grid.lines"));
            post(run, "view.zoom", 3L, document_key(changed));
            post(run, "tab.pin", true, document_key(changed));
            post(run, "window.arrange", 1L);
            post(run, "tool.close", static_cast<long>(tool::inspector));
            frames(run, 6);
            post(run, "workspace.new", 0L);
            post(run, "workspace.rename", std::string("Elsewhere"));
            post(run, "file.example", 4L);
            frames(run, 4);
            example = run.state().id;
        }

        done_with(first, before, mono);
    }

    // the next: as it was left
    {
        ImGuiContext* second = an_imgui();

        {
            app run(settings, session);

            frames(run, 6);

            const bitwisedom::gui::shell_state& shell = run.shell();
            const bitwisedom::gui::model*       edit  = run.document(changed);

            expect( (shell.workspaces.size() == 2) &&
                    (shell.workspaces[1].name == "Elsewhere") &&
                    (shell.current == 1) &&
                    (run.state().id == example) &&
                    (run.state().doc.title == "lm75_temperature"),
                   "the workspaces open as they were, the same in front");
            expect( (edit != nullptr) &&
                    (edit->ui.dirty) &&
                    (!edit->doc.grid.lines),
                   "a change not yet saved is there still");
            expect(std::fabs(zoom_of(shell, document_key(changed)) - 1.3f) <
                       0.01f,
                   "and its window's zoom");
            expect( (shell.workspaces[0].pinned.size() == 1) &&
                    (shell.workspaces[0].pinned[0] == document_key(changed)),
                   "and its tab pinned");
            expect( (!shell.workspaces[0].tools[static_cast<std::size_t>(
                         tool::inspector)]) &&
                    (shell.workspaces[1].tools[static_cast<std::size_t>(
                         tool::inspector)]),
                   "each workspace's tool windows, open or closed");

            // the first workspace, its sheets side by side still
            post(run, "workspace.switch", 0L);
            frames(run, 4);

            const ImGuiWindow* left  = ImGui::FindWindowByName(
                ("###" + document_key(shell.workspaces[0].documents[0]))
                    .c_str());
            const ImGuiWindow* right = ImGui::FindWindowByName(
                ("###" + document_key(changed)).c_str());

            expect( (left != nullptr) &&
                    (right != nullptr) &&
                    (std::fabs(left->Pos.y - right->Pos.y) < 1.0f) &&
                    (left->Pos.x + 10.0f < right->Pos.x),
                   "laid out as they were left: in columns");
        }

        done_with(second, before, mono);
    }

    std::remove(settings.c_str());
    std::remove(session.c_str());
    expect(clashes == 0, "the next run has no ID clash");

    return;
}

/*
the_options
  Every setting in one place: the Options open from Tools, their pages and
their search; the window's own settings set there, and followed at once --
the symbols tree in a window of its own, and merged again; and the
settings files: exported, imported back over changes, forgotten, and
deleted.
*/
void
the_options(
    app& _window
)
{
    using bitwisedom::gui::add_style;
    using bitwisedom::gui::tool;

    const bitwisedom::gui::shell_state& shell = _window.shell();
    const std::string                   path  =
        "ui_options.bitwisedom-settings.json";

    post(_window, "options.open", 0L);
    frames(_window, 3);
    expect(window_shown("Options###options"), "Tools > Options opens them");

    for (long page = 0; page < 10; ++page)
    {
        post(_window, "options.page", page);
        frames(_window, 2);
    }

    expect(shell.options_page == 9, "every page shows");

    post(_window, "options.search", std::string("between bits"));
    frames(_window, 2);
    expect(shell.options_search == "between bits", "the search shows");

    // the symbols tree, apart and merged again
    post(_window, "set", 1L, std::string("environment.merged"));
    frames(_window, 3);
    expect( (!shell.prefs.merged) &&
            (tool_in_view(_window, tool::symbols)),
           "the symbols tree in a window of its own, from the Options");

    post(_window, "set", 0L, std::string("environment.merged"));
    frames(_window, 3);
    expect( (shell.prefs.merged) &&
            (!tool_is_open(_window, tool::symbols)),
           "and the grid's symbols column again");

    // a settings file: exported, then imported back over changes
    std::remove(path.c_str());
    post(_window, "settings.path", path);
    post(_window, "settings.export", 0L);
    frames(_window, 2);

    std::string text;
    std::string error;

    expect( (bitwisedom::read_file(path, text, error)) &&
            (text.find("\"bitwisedom.settings\"") != std::string::npos) &&
            (text.find("\"grid.lines\"") != std::string::npos) &&
            (text.find("\"environment.add\"") != std::string::npos) &&
            (text.find("\"sheet.title\"") == std::string::npos),
           "exported: the window's settings and the sheet's preferences, "
           "not its content");
    expect( (!shell.prefs.settings_files.empty()) &&
            (shell.prefs.settings_files.front() == path),
           "and kept on the list");

    const bool lines = _window.state().doc.grid.lines;

    post(_window, "set", !lines, std::string("grid.lines"));
    post(_window, "set", 1L, std::string("environment.add"));
    frames(_window, 2);
    expect( (_window.state().doc.grid.lines != lines) &&
            (shell.prefs.add == add_style::split),
           "changed since");

    post(_window, "settings.apply", 0L);
    frames(_window, 2);
    expect( (_window.state().doc.grid.lines == lines) &&
            (shell.prefs.add == add_style::menu) &&
            (!_window.state().ui.status_error),
           "applied from the list, the file's settings are back");

    // forgotten, the file stays; deleted, it goes
    post(_window, "settings.forget", 0L);
    frames(_window, 2);
    expect( (shell.prefs.settings_files.empty() ||
             shell.prefs.settings_files.front() != path) &&
            (bitwisedom::read_file(path, text, error)),
           "forgotten: off the list, and still where it was");

    post(_window, "settings.import", path);
    frames(_window, 2);
    expect( (!shell.prefs.settings_files.empty()) &&
            (shell.prefs.settings_files.front() == path),
           "imported, it is on the list again");

    post(_window, "settings.delete", 0L);
    frames(_window, 2);
    expect( (!bitwisedom::read_file(path, text, error)) &&
            (shell.prefs.settings_files.empty() ||
             shell.prefs.settings_files.front() != path),
           "deleted: gone, and off the list");

    post(_window, "options.close", 0L);
    frames(_window, 2);
    expect(!window_shown("Options###options"), "and the Options close");

    expect(clashes == 0, "the Options have no ID clash");

    return;
}

// drag_to
//   a press at one point of the grid, the mouse moved to another in steps,
// and a release there.
void
drag_to(
    app&                               _window,
    const bitwisedom::gui::grid_point& _from,
    const bitwisedom::gui::grid_point& _to
)
{
    ImGuiIO& io = ImGui::GetIO();

    io.AddMousePosEvent(_from.x, _from.y);
    frames(_window, 2);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    frames(_window, 1);

    for (int step = 1; step <= 4; ++step)
    {
        const float t = static_cast<float>(step) / 4.0f;

        io.AddMousePosEvent(_from.x + (_to.x - _from.x) * t,
                            _from.y + (_to.y - _from.y) * t);
        frames(_window, 1);
    }

    frames(_window, 1);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    frames(_window, 2);

    return;
}

// order_of
//   a row's place among the sheet's rows, in tree order; -1 for none.
int
order_of(
    app& _window,
    int  _id
)
{
    const std::vector<bitwisedom::resolved_row>& rows =
        _window.state().res.rows;

    for (std::size_t i = 0; i < rows.size(); ++i)
    {
        if (rows[i].id == _id)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

/*
the_grid_as_the_tree
  The grid's symbols column is the tree while the tree's own window is
closed: a row selected has an "Add child..." field under its group's rows,
where names are typed one after another; and a row dragged by its name
drops before a symbol, or into a group -- each a step to undo.
*/
void
the_grid_as_the_tree(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;

    post(_window, "file.example", 0L);   // uart_cr
    frames(_window, 4);

    const int en       = id_of(_window, "UART_CR_EN");
    const int loopback = id_of(_window, "UART_CR_LOOPBACK");
    const int enables  = _window.state().res.find(en)->parent;
    const int format   = _window.state().res.find(
                             id_of(_window, "UART_CR_PARITY"))->parent;

    // nothing selected: no field
    expect(!grid_spot("child", 0, 0).found,
           "with nothing selected, the grid has no Add child field");

    // a symbol selected: its group's field, after the group's rows
    post(_window, "select", static_cast<long>(en));
    frames(_window, 3);

    const bitwisedom::gui::grid_point field = grid_spot("child", 0, 0);

    expect( (field.found) &&
            (field.y > grid_spot("name", loopback, 0).y),
           "a symbol selected: its group's field, after its rows");

    expect(move_to(_window, field), "the field is drawn");
    click(_window);
    type(_window, "EXTRA");
    frames(_window, 2);

    const int extra = id_of(_window, "UART_CR_EXTRA");

    expect( (extra != 0) &&
            (_window.state().res.find(extra)->parent == enables) &&
            (_window.state().ui.adding_where == "grid"),
           "a name typed there is a bit in the group");

    // the field has the keyboard back: the next name
    frames(_window, 3);
    type(_window, "MORE");
    frames(_window, 2);
    expect( (id_of(_window, "UART_CR_MORE") != 0) &&
            (_window.state().res.find(id_of(_window, "UART_CR_MORE"))
                 ->parent == enables),
           "and the next, typed at once");

    frames(_window, 2);
    key(_window, ImGuiKey_Escape);
    key(_window, ImGuiKey_Escape);
    post(_window, "edit.undo", 0L);
    post(_window, "edit.undo", 0L);
    frames(_window, 3);
    expect( (id_of(_window, "UART_CR_EXTRA") == 0) &&
            (id_of(_window, "UART_CR_MORE") == 0),
           "each a step to undo");

    // a row dragged by its name: before a symbol
    expect(order_of(_window, loopback) > order_of(_window, en),
           "LOOPBACK comes after EN");

    drag_to(_window,
            grid_spot("name", loopback, 0),
            grid_spot("name", en, 0));
    expect( (order_of(_window, loopback) >= 0) &&
            (order_of(_window, loopback) < order_of(_window, en)) &&
            (_window.state().res.find(loopback)->parent == enables),
           "dragged onto EN's name, LOOPBACK goes before it");

    // into a group: at its end
    const int stop = id_of(_window, "UART_CR_STOP");

    drag_to(_window,
            grid_spot("name", loopback, 0),
            grid_spot("name", format, 0));
    expect( (_window.state().res.find(loopback)->parent == format) &&
            (order_of(_window, loopback) > order_of(_window, stop)),
           "dragged onto a group's name, it goes to the group's end");

    post(_window, "edit.undo", 0L);
    post(_window, "edit.undo", 0L);
    frames(_window, 3);
    expect( (_window.state().res.find(loopback)->parent == enables) &&
            (order_of(_window, loopback) > order_of(_window, en)),
           "and both undo");

    expect(clashes == 0, "the grid as the tree has no ID clash");

    return;
}

/*
special_rows
  The probe and usage rows: shown, pinned at the bottom of the grid, below
the add row; dragged by the name, the usage into a group -- after its rows,
counting only them -- and the probe before a row; pinned at the top from
their menu; another added and removed, the last of a kind hidden instead;
and each a step to undo.
*/
void
special_rows(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;
    using bitwisedom::gui::grid_special;
    using bitwisedom::special_kind;
    using bitwisedom::special_place;

    post(_window, "file.example", 0L);   // uart_cr: 32 bits
    frames(_window, 4);

    const bitwisedom::grid_settings& grid = _window.state().doc.grid;

    expect( (grid.specials.size() == 2) &&
            (!grid.show_probe)          &&
            (!grid.show_usage)          &&
            (!grid.lines_past_rows),
           "a probe and a usage row, hidden, and lines that stop at the "
           "rows");

    post(_window, "set", true, std::string("grid.show_probe"));
    post(_window, "set", true, std::string("grid.show_usage"));
    frames(_window, 3);

    const int probe    = grid.specials[0].id;
    const int usage    = grid.specials[1].id;
    const int en       = id_of(_window, "UART_CR_EN");
    const int loopback = id_of(_window, "UART_CR_LOOPBACK");
    const int enables  = _window.state().res.find(en)->parent;
    const int format   = _window.state().res.find(
                             id_of(_window, "UART_CR_PARITY"))->parent;

    // every group closed but the first: room below the rows
    post(_window, "tree.close_all", 0L);
    frames(_window, 2);

    {
        uxoxo::event opened;

        opened.kind = "toggle_open";
        opened.payload.set("value", static_cast<long>(enables));
        opened.payload.set("open", true);
        _window.post(opened);
        frames(_window, 3);
    }

    expect(grid_spot("below", 0, 0).found,
           "a short sheet leaves room below its rows");

    expect( (grid_spot("special", probe, 0).found) &&
            (grid_spot("special", usage, 0).found) &&
            (grid_spot("special", probe, 0).y >
             grid_spot("add", 0, 0).y)              &&
            (grid_spot("special", usage, 0).y >
             grid_spot("special", probe, 0).y),
           "shown, they are below the add row, the probe first");
    expect(grid_special(usage).label == "allocated",
           "the usage row counts the sheet's rows");

    // the usage, dragged into a group: after its rows, counting them
    drag_to(_window,
            grid_spot("special", usage, 0),
            grid_spot("name", enables, 0));

    const bitwisedom::special_row* moved =
        bitwisedom::find_special(grid, usage);

    expect( (moved != nullptr)                        &&
            (moved->place == special_place::tree)     &&
            (moved->parent == enables),
           "dragged onto a group's name, the usage row goes into it");
    expect( (grid_spot("special", usage, 0).y >
             grid_spot("name", loopback, 0).y)         &&
            (grid_spot("special", usage, 0).y <
             grid_spot("name", format, 0).y),
           "after the group's rows, before the next group");

    // the group's four bits, EN to LOOPBACK, and nothing else
    const bitwisedom::gui::grid_text counted = grid_special(usage);

    expect( (counted.label.find("allocated in") == 0) &&
            (counted.note == "4 of 32"),
           "and counts only the group's rows");

    // the probe, dragged onto a symbol: before it
    drag_to(_window,
            grid_spot("special", probe, 0),
            grid_spot("name", loopback, 0));
    expect( (bitwisedom::find_special(grid, probe)->before == loopback) &&
            (grid_spot("special", probe, 0).y <
             grid_spot("name", loopback, 0).y)                         &&
            (grid_spot("special", probe, 0).y >
             grid_spot("name", en, 0).y),
           "the probe dragged onto a symbol goes just before it");

    // the menu: pinned at the top
    post(_window, "context.special", static_cast<long>(probe));
    frames(_window, 2);
    expect( (_window.state().ui.menu_kind == "special") &&
            (_window.state().ui.menu_special == probe),
           "a special row has its own menu");

    {
        uxoxo::event pin;

        pin.kind = "special.place";
        pin.payload.set("value", static_cast<long>(probe));
        pin.payload.set("place", std::string("top"));
        _window.post(pin);
        frames(_window, 3);
    }

    expect( (bitwisedom::find_special(grid, probe)->place ==
             special_place::top)                                 &&
            (grid_spot("special", probe, 0).y <
             grid_spot("name", enables, 0).y),
           "pinned at the top, it is the first row");

    // another usage row, at the bottom; then removed
    post(_window, "special.duplicate", static_cast<long>(usage));
    frames(_window, 3);
    expect(grid.specials.size() == 3, "another usage row, added");

    int added = 0;

    for (const bitwisedom::special_row& each : grid.specials)
    {
        added = ( (each.kind == special_kind::usage) &&
                  (each.id != usage) ) ? each.id : added;
    }

    expect( (added != 0) &&
            (grid_spot("special", added, 0).found) &&
            (grid_special(added).note == "4 of 32"),
           "placed as the one it copies, counting the same rows");

    post(_window, "special.remove", static_cast<long>(added));
    frames(_window, 2);
    post(_window, "special.remove", static_cast<long>(usage));
    frames(_window, 3);
    expect( (grid.specials.size() == 2) &&
            (!grid.show_usage)          &&
            (!grid_spot("special", usage, 0).found),
           "removed, the last of its kind hidden instead");

    // each a step: the removals, the row added, the pin
    for (int i = 0; i < 4; ++i)
    {
        post(_window, "edit.undo", 0L);
        frames(_window, 2);
    }

    frames(_window, 2);
    expect( (grid.show_usage)                                       &&
            (grid.specials.size() == 2)                             &&
            (bitwisedom::find_special(grid, usage)->place ==
             special_place::tree)                                   &&
            (bitwisedom::find_special(grid, probe)->before == loopback),
           "each a step to undo");

    expect(clashes == 0, "the special rows have no ID clash");

    return;
}

/*
the_add_node
  What ends a group's rows: the "Add child..." field, as wide as what it
holds or filling its column, and the + beside it -- either, or both. The +
opens the Add node window, a tab among the sheets or a window of its own,
where a row of any kind is made in full: a range, three bits wide, added
in its group, a step to undo; a name another symbol has, refused.
*/
void
the_add_node(
    app& _window
)
{
    using bitwisedom::gui::control_spot;
    using bitwisedom::gui::grid_spot;

    post(_window, "file.example", 0L);   // uart_cr
    frames(_window, 4);

    const int en      = id_of(_window, "UART_CR_EN");
    const int enables = _window.state().res.find(en)->parent;

    post(_window, "select", static_cast<long>(en));
    frames(_window, 3);

    const bitwisedom::gui::grid_point plus  =
        control_spot("add_child.grid.plus");
    const bitwisedom::gui::grid_point field =
        control_spot("add_child.grid.field");
    const bitwisedom::gui::grid_point end   =
        control_spot("add_child.grid.end");

    expect( (plus.found) && (field.found) && (end.found) &&
            (plus.x < field.x),
           "a group's rows end with a + and the Add child field");
    // a wider column, so the field has room to fill
    post(_window, "set", 360.0, std::string("grid.name_width"));
    frames(_window, 3);

    const float fitted = control_spot("add_child.grid.end").x;

    expect(fitted < grid_spot("edge.name", 0, 0).x - 100.0f,
           "the field as wide as what it holds");

    post(_window, "set", true, std::string("environment.child_fill"));
    frames(_window, 3);
    expect( (control_spot("add_child.grid.end").found) &&
            (control_spot("add_child.grid.end").x > fitted + 80.0f) &&
            (control_spot("add_child.grid.end").x >
             grid_spot("edge.name", 0, 0).x - 12.0f),
           "or, filling, to its column's edge");
    post(_window, "edit.undo", 0L);
    post(_window, "set", false, std::string("environment.child_fill"));

    post(_window, "set", 1L, std::string("environment.child"));
    frames(_window, 3);
    expect( (!control_spot("add_child.grid.plus").found) &&
            (control_spot("add_child.grid.field").found),
           "the field alone");

    post(_window, "set", 2L, std::string("environment.child"));
    frames(_window, 3);
    expect( (control_spot("add_child.grid.plus").found) &&
            (!control_spot("add_child.grid.field").found),
           "the + alone");

    post(_window, "set", 0L, std::string("environment.child"));
    frames(_window, 3);

    // the +: the Add node window, a tab, for the group
    expect(move_to(_window, control_spot("add_child.grid.plus")),
           "the + is drawn");
    click(_window);
    frames(_window, 3);

    const bitwisedom::gui::node_draft& n = _window.state().ui.node;

    expect( (window_shown("Add node###node.tab")) &&
            (n.parent == enables),
           "the + opens the Add node window, for its group");

    // a range, three bits wide, named
    post(_window, "node.kind", 1L);
    post(_window, "node.name", std::string("WIDE"));
    post(_window, "node.width", 3L);
    frames(_window, 2);
    expect( (n.draft.def == bitwisedom::def_kind::range) &&
            (n.resolves) &&
            (n.resolved.full == "UART_CR_WIDE") &&
            (n.refused.empty()),
           "its row, tried as it is made");

    const int free = n.draft.lsb;

    post(_window, "node.add", false);
    frames(_window, 3);

    const int wide = id_of(_window, "UART_CR_WIDE");

    expect( (wide != 0) &&
            (_window.state().res.find(wide)->parent == enables) &&
            (_window.state().res.find(wide)->span_width == 3) &&
            (_window.state().res.find(wide)->span_lsb == free) &&
            (window_shown("Add node###node.tab")) &&
            (n.draft.name.empty()),
           "added, in its group; the window stays for the next");

    // a name another symbol has: refused
    post(_window, "node.kind", 0L);
    post(_window, "node.name", std::string("EN"));
    frames(_window, 2);

    const std::size_t rows = _window.state().res.rows.size();

    post(_window, "node.add", false);
    frames(_window, 2);
    expect( (!n.refused.empty()) &&
            (_window.state().res.rows.size() == rows),
           "a name another symbol has is refused");

    post(_window, "edit.undo", 0L);
    frames(_window, 2);
    expect(id_of(_window, "UART_CR_WIDE") == 0, "the add, a step to undo");

    // a window of its own
    post(_window, "set", 1L, std::string("environment.node_as"));
    frames(_window, 3);
    expect( (window_shown("Add node###node.window")) &&
            (!window_shown("Add node###node.tab")),
           "or a window of its own");

    post(_window, "node.close", 0L);
    frames(_window, 2);
    expect(!window_shown("Add node###node.window"), "closed");

    post(_window, "set", 0L, std::string("environment.node_as"));
    frames(_window, 2);

    expect(clashes == 0, "the Add node window has no ID clash");

    return;
}

/*
the_status_bar
  A bar along the window's foot: what the sheet in front is on the left,
and at its right end what the window last said -- after a "*" while the
sheet has changes. A click there opens the console: every action so far,
the latest first, a slider dragged one action many times over.
*/
void
the_status_bar(
    app& _window
)
{
    using bitwisedom::gui::control_spot;
    using bitwisedom::gui::tool;

    post(_window, "file.example", 0L);   // uart_cr
    frames(_window, 3);

    const bitwisedom::gui::grid_point message = control_spot("status.message");

    expect( (message.found) &&
            (message.y > ImGui::GetIO().DisplaySize.y - 40.0f),
           "the status bar is along the window's foot");

    // a slider dragged: one action, many times over
    post(_window, "set", 30.0, std::string("grid.cell_width"));
    post(_window, "set", 31.0, std::string("grid.cell_width"));
    post(_window, "set", 32.0, std::string("grid.cell_width"));
    frames(_window, 2);

    const std::vector<bitwisedom::gui::action_entry>& actions =
        _window.shell().actions;

    expect( (!actions.empty())                                    &&
            (actions.back().kind == "set")                        &&
            (actions.back().times == 3)                           &&
            (actions.back().detail.find("grid.cell_width") !=
             std::string::npos)                                   &&
            (actions.back().sheet == "uart_cr.bitwisedom.json"),
           "every action is kept, a slider dragged once, three times over");

    // a click on the message: the console
    post(_window, "tool.close", static_cast<long>(tool::console));
    frames(_window, 2);
    expect(!tool_is_open(_window, tool::console), "the console is closed");
    expect(move_to(_window, control_spot("status.message")),
           "the message is drawn");
    click(_window);
    frames(_window, 4);
    expect( (tool_is_open(_window, tool::console)) &&
            (tool_in_view(_window, tool::console)),
           "a click on the message opens the console");
    expect(control_spot("console.count").found,
           "and it lists the actions so far");

    for (int i = 0; i < 3; ++i)
    {
        post(_window, "edit.undo", 0L);
        frames(_window, 1);
    }

    post(_window, "tool.close", static_cast<long>(tool::console));
    frames(_window, 2);
    expect(!tool_is_open(_window, tool::console), "and it closes");

    expect(clashes == 0, "the status bar has no ID clash");

    return;
}

/*
tabs_over_the_bits
  A sheet's tab row runs across the whole width, as Dear ImGui's own do, by
default; or over the sheet's bits, its symbols column running up through
the row, its tabs at the bits' left or their right; or over the symbols
column alone. A double-click on the tab renames the sheet's file, the name
typed over the tab: ".bitwisedom.json" put after a bare name, a name with
a folder in it refused, and Escape keeping the name it had.
*/
void
tabs_over_the_bits(
    app& _window
)
{
    using bitwisedom::gui::control_spot;
    using bitwisedom::gui::grid_spot;

    // a workspace of its own: its one sheet beside the macros and the
    // templates
    post(_window, "workspace.new", 0L);
    frames(_window, 3);
    post(_window, "file.example", 9L);   // win32_hresult
    frames(_window, 6);

    const int          id     = _window.state().id;
    const std::string  named  = bitwisedom::gui::document_key(id);
    const ImGuiWindow* window = ImGui::FindWindowByName(("###" + named).c_str());
    const float        edge   = grid_spot("edge.name", 0, 0).x;

    expect( (window != nullptr) &&
            (window->DockIsActive) &&
            (edge > 0.0f) &&
            (window->DC.DockTabItemRect.Min.x < edge),
           "by default the tabs run across the whole width, from its left");

    // where uxoxo's ImGui lets a tab bar begin and end inset
#if defined(IMGUI_DOCK_TAB_BAR_INSET) && (IMGUI_DOCK_TAB_BAR_INSET >= 2)
    using bitwisedom::gui::grid_columns;

    // tab_at
    //   where the sheet's tab begins and ends, as its node laid it out.
    auto tab_at = [&window]()
    {
        return (window != nullptr) ? window->DC.DockTabItemRect
                                   : ImRect();
    };

    post(_window, "set", 1L, std::string("environment.tabs_over"));
    frames(_window, 4);
    expect( (control_spot(named + ".tab_row.from").found) &&
            (std::fabs(control_spot(named + ".tab_row.from").x - edge) <=
             2.0f) &&
            (tab_at().Min.x >= edge),
           "over the bits: the symbols column runs up through the tab row, "
           "and the tabs begin past it");

    const float left_tab = tab_at().Min.x;

    post(_window, "set", 1L, std::string("environment.tabs_align"));
    frames(_window, 4);
    expect( (tab_at().Min.x > left_tab + 100.0f) &&
            (tab_at().Max.x <=
             control_spot(named + ".tab_row.to").x + 1.0f),
           "kept to the right, the tabs end where the bits do");

    post(_window, "set", 2L, std::string("environment.tabs_over"));
    post(_window, "set", 0L, std::string("environment.tabs_align"));
    frames(_window, 4);
    expect( (std::fabs(control_spot(named + ".tab_row.to").x -
                       grid_columns(id).symbols_to) <= 2.0f) &&
            (tab_at().Max.x <= grid_columns(id).symbols_to + 1.0f),
           "over the symbols column: the tabs end where it does");

    post(_window, "set", 0L, std::string("environment.tabs_over"));
    frames(_window, 4);
    expect(tab_at().Min.x < edge, "and across the whole width again");
#endif

    // a double-click on the tab: the name typed over it
    const ImVec2 tab = (window != nullptr)
                           ? window->DC.DockTabItemRect.GetCenter()
                           : ImVec2(0.0f, 0.0f);

    ImGui::GetIO().AddMousePosEvent(tab.x, tab.y);
    frames(_window, 2);
    double_click(_window);
    frames(_window, 2);
    expect( (_window.state().ui.file_renaming) &&
            (control_spot(named + ".rename").found),
           "a double-click on the tab renames the sheet's file");

    // Escape: the name it had
    key(_window, ImGuiKey_Escape);
    frames(_window, 2);
    expect( (!_window.state().ui.file_renaming) &&
            (_window.state().ui.path == "win32_hresult.bitwisedom.json"),
           "Escape keeps the name it had");

    // a bare name: the sheet's ending put after it
    post(_window, "doc.rename", static_cast<long>(id));
    frames(_window, 3);
    type(_window, "errors");
    frames(_window, 2);
    expect( (_window.state().ui.path == "errors.bitwisedom.json") &&
            (_window.state().ui.dirty),
           "a name typed over the tab is the sheet's file's");
    expect(window_shown(("errors.bitwisedom.json###" + named).c_str()),
           "and the tab says so");

    // a folder in it: refused
    post(_window, "doc.rename", static_cast<long>(id));
    frames(_window, 3);
    type(_window, "a/b");
    frames(_window, 2);
    expect( (_window.state().ui.path == "errors.bitwisedom.json") &&
            (_window.state().ui.status_error),
           "a name with a folder in it is refused");

    expect(clashes == 0, "the tabs have no ID clash");

    return;
}

/*
the_tabs_menu
  A right-click on a tab opens its menu, which knows its row's tabs in
order: pinned, a tab moves before the rest and loses its x; the tabs to a
tab's right close, then every other but the pinned, then those to a tab's
left but the pinned; unpinned, the tab has its x again.
*/
void
the_tabs_menu(
    app& _window
)
{
    using bitwisedom::gui::control_spot;
    using bitwisedom::gui::document_key;

    post(_window, "workspace.new", 0L);
    frames(_window, 3);

    std::vector<int> ids;

    for (int i = 0; i < 3; ++i)
    {
        post(_window, "file.new", 0L);
        frames(_window, 3);
        ids.push_back(_window.state().id);
    }

    frames(_window, 4);

    const std::string first  = document_key(ids[0]);
    const std::string second = document_key(ids[1]);
    const ImGuiWindow* held  = ImGui::FindWindowByName(("###" + first).c_str());

    // pinned: first in its row, with no x
    post(_window, "tab.pin", true, first);
    frames(_window, 8);

    const ImGuiTabBar* bar = ( (held != nullptr) &&
                               (held->DockNode != nullptr) )
                                 ? held->DockNode->TabBar
                                 : nullptr;

    expect( (bar != nullptr) &&
            (bar->Tabs.Size >= 3) &&
            (bar->Tabs[0].Window == held) &&
            (!held->HasCloseButton),
           "a pinned tab moves before the rest, and has no x");

    // tab_menu
    //   a right-click on a tab.
    auto tab_menu = [&_window](const std::string& _key)
    {
        if (!move_to(_window, control_spot(_key + ".tab")))
        {
            return false;
        }

        click(_window, ImGuiMouseButton_Right);
        frames(_window, 2);

        return ( (_window.state().ui.menu_kind == "tab") &&
                 (_window.state().ui.menu_tab == _key) );
    };

    // close
    //   a "tabs.close" from the menu.
    auto close = [&_window](const char* _which)
    {
        uxoxo::event closing;

        closing.kind = "tabs.close";
        closing.payload.set("which", std::string(_which));
        _window.post(closing);
        frames(_window, 4);

        return;
    };

    // open
    //   whether a sheet is still among the workspace's.
    auto open = [&_window](int _id)
    {
        const std::vector<int>& here_now = here(_window).documents;

        return (std::find(here_now.begin(), here_now.end(), _id) !=
                here_now.end());
    };

    expect(tab_menu(second), "a right-click on a tab opens its menu");

    const std::vector<std::string> row   = _window.state().ui.menu_tabs;
    const auto                     at    = std::find(row.begin(), row.end(),
                                                     second);
    bool                           right = false;

    for (auto it = (at == row.end()) ? row.end() : at + 1;
         it != row.end();
         ++it)
    {
        right = ( (right) || (*it == document_key(ids[2])) );
    }

    expect( (row.size() >= 3) &&
            (row.front() == first) &&
            (at != row.end()) &&
            (right),
           "and knows its row's tabs, in order");

    close("right");
    expect( (open(ids[0])) && (open(ids[1])) && (!open(ids[2])),
           "the tabs to its right close");

    // another, then every other but the pinned
    post(_window, "file.new", 0L);
    frames(_window, 4);

    const int last = _window.state().id;

    expect(tab_menu(document_key(last)), "a new tab's menu");
    close("others");
    expect( (open(ids[0])) && (!open(ids[1])) && (open(last)),
           "every other tab closes, but the pinned");

    // and those to a tab's left, but the pinned
    post(_window, "file.new", 0L);
    frames(_window, 4);

    const int after = _window.state().id;

    expect(tab_menu(document_key(after)), "the newest tab's menu");
    close("left");
    expect( (open(ids[0])) && (!open(last)) && (open(after)),
           "the tabs to its left close, but the pinned");

    post(_window, "tab.pin", false, first);
    frames(_window, 4);
    expect( (held != nullptr) && (held->HasCloseButton),
           "unpinned, it has its x again");

    post(_window, "workspace.close", 0L);
    frames(_window, 3);
    expect(clashes == 0, "the tabs' menu has no ID clash");

    return;
}

/*
the_keyboard
  The grid's keys, as Excel's: the arrows a cell at a time, Ctrl to the
edge of the bits set or the end, Shift extending the range from the
cursor, which stays; Home and Ctrl+Home past the frozen name and value
columns, Ctrl+End to the last cell anything is in, End mode; Enter and Tab,
round a range and back to where a run of Tabs began; Shift+Space,
Ctrl+Space, Ctrl+A, Shift+Backspace and Ctrl+.; F8's Extend mode; F2 and
Space; Page Down and Ctrl+End scrolling the 1,424 rows of HRESULTs with the
cursor; and Ctrl+Page Up and Down from sheet to sheet.
*/
void
the_keyboard(
    app& _window
)
{
    using bitwisedom::gui::active_row;
    using bitwisedom::gui::CURSOR_NAME;
    using bitwisedom::gui::CURSOR_VALUE;
    using bitwisedom::gui::grid_keyboard;
    using bitwisedom::gui::grid_spot;

    post(_window, "workspace.new", 0L);
    frames(_window, 3);
    post(_window, "file.example", example_index("uart_cr"));
    frames(_window, 4);
    post(_window, "doc.activate", static_cast<long>(_window.state().id));
    frames(_window, 4);

    expect(grid_keyboard(), "the grid has the keyboard, its window in front");

    // at
    //   the cursor's row and column, as they are now.
    auto at = [&_window](int _row,
                         int _column)
    {
        const bitwisedom::gui::model& front = _window.state();

        return ( (active_row(front) == _row) &&
                 (front.ui.cursor_row == _row) &&
                 (front.ui.cursor_column == _column) );
    };

    // put
    //   the cursor on a cell, as a script would put it there.
    auto put = [&_window](int _row,
                          int _column)
    {
        uxoxo::event moved;

        moved.kind = "grid.cursor";
        moved.payload.set("row", static_cast<long>(_row));
        moved.payload.set("column", static_cast<long>(_column));
        _window.post(moved);
        frames(_window, 2);

        return;
    };

    const int en       = id_of(_window, "UART_CR_EN");
    const int txen     = id_of(_window, "UART_CR_TXEN");
    const int rxen     = id_of(_window, "UART_CR_RXEN");
    const int loopback = id_of(_window, "UART_CR_LOOPBACK");
    const int preset   = id_of(_window, "UART_CR_DEFAULT");
    const int enables  = id_of(_window, "UART_CR_ENABLES");
    const int presets  = _window.state().res.find(preset)->parent;

    // the columns the cursor stops in, left to right: the names, the
    // values while they are shown, then the bits -- the reserved ones
    // folded into one column
    const bitwisedom::gui::grid_columns_seen spans =
        bitwisedom::gui::grid_columns(_window.state().id);
    const bool values = ( (spans.found) &&
                          (spans.values_to > spans.values_from + 1.0f) );
    const int  cols[5] = { CURSOR_NAME,
                           values ? CURSOR_VALUE : 28,
                           values ? 28 : 27,
                           values ? 27 : 26,
                           values ? 26 : 25 };
    const int  first_bits = 28;

    // the arrows, a cell at a time
    put(txen, CURSOR_NAME);
    press(_window, ImGuiKey_DownArrow);
    expect(at(rxen, CURSOR_NAME), "Down moves the cursor a row down");
    press(_window, ImGuiKey_UpArrow);
    expect( (at(txen, CURSOR_NAME)) &&
            (_window.state().ui.selection.size() == 1) &&
            (_window.state().ui.selection.front() == txen),
           "Up moves it back, the row it is on selected");
    press(_window, ImGuiKey_RightArrow);
    expect(at(txen, cols[1]), "Right, to the next column");

    if (values)
    {
        press(_window, ImGuiKey_RightArrow);
    }

    expect(at(txen, first_bits),
           "and to the first column of bits, the reserved ones folded");
    press(_window, ImGuiKey_LeftArrow);

    if (values)
    {
        press(_window, ImGuiKey_LeftArrow);
    }

    expect(at(txen, CURSOR_NAME), "Left, back to the names");

    // Ctrl and an arrow: to the edge of the bits set, or the next one
    put(preset, CURSOR_NAME);

    if (values)
    {
        press(_window, ImGuiMod_Ctrl | ImGuiKey_RightArrow);
        expect(at(preset, CURSOR_VALUE),
               "Ctrl+Right runs to the end of the cells with something in "
               "them");
    }

    press(_window, ImGuiMod_Ctrl | ImGuiKey_RightArrow);
    expect(at(preset, 9), "Ctrl+Right, to the next bit set");
    press(_window, ImGuiMod_Ctrl | ImGuiKey_RightArrow);
    expect(at(preset, 8), "then to the end of the run of them");
    press(_window, ImGuiMod_Ctrl | ImGuiKey_RightArrow);
    press(_window, ImGuiMod_Ctrl | ImGuiKey_RightArrow);
    expect(at(preset, 0), "and on to the row's end");
    press(_window, ImGuiMod_Ctrl | ImGuiKey_RightArrow);
    expect(at(preset, 0), "where it stays");

    put(en, 1);
    press(_window, ImGuiMod_Ctrl | ImGuiKey_DownArrow);
    expect(at(txen, 1), "Ctrl+Down, to the next row with the bit set");
    press(_window, ImGuiMod_Ctrl | ImGuiKey_DownArrow);
    expect(at(presets, 1),
           "past the rows without it, to a group showing its rows' bits");
    press(_window, ImGuiMod_Ctrl | ImGuiKey_DownArrow);
    expect(at(enables, 1), "and to the end of the rows with it");

    // Home past the frozen columns; Ctrl+Home and Ctrl+End
    press(_window, ImGuiKey_Home);
    expect(at(enables, first_bits),
           "Home goes to the row's first bit, past the frozen columns");
    press(_window, ImGuiMod_Ctrl | ImGuiKey_End);
    expect( (_window.state().ui.cursor_column == 0) &&
            (_window.state().ui.cursor_row != enables),
           "Ctrl+End, to the last cell anything is in");
    press(_window, ImGuiMod_Ctrl | ImGuiKey_Home);

    const int top = _window.state().ui.cursor_row;

    expect( (_window.state().ui.cursor_column == first_bits) &&
            (top != 0),
           "Ctrl+Home, to the first row's first bit");

    // End mode
    put(preset, CURSOR_NAME);
    press(_window, ImGuiKey_End);
    expect(_window.state().ui.end_mode, "End is End mode");
    press(_window, ImGuiKey_RightArrow);
    expect( (at(preset, values ? CURSOR_VALUE : 9)) &&
            (!_window.state().ui.end_mode),
           "then an arrow goes as Ctrl's does, and End mode ends");
    press(_window, ImGuiKey_End);
    press(_window, ImGuiKey_Enter);
    expect(at(preset, 0), "End, Enter: the row's last cell with anything");

    // Shift: the range, the cursor staying
    put(txen, CURSOR_NAME);
    press(_window, ImGuiMod_Shift | ImGuiKey_DownArrow);
    press(_window, ImGuiMod_Shift | ImGuiKey_DownArrow);
    expect( (at(txen, CURSOR_NAME)) &&
            (_window.state().ui.selection.size() == 3) &&
            (_window.state().ui.grid_rect_rows.empty()),
           "Shift+Down extends the range, the rows selected");

    for (int i = 0; i < 3; ++i)
    {
        press(_window, ImGuiMod_Shift | ImGuiKey_RightArrow);
    }

    expect( (at(txen, CURSOR_NAME)) &&
            (_window.state().ui.grid_rect_rows.size() == 3) &&
            (_window.state().ui.grid_rect_low == cols[3]) &&
            (_window.state().ui.grid_rect_high == 31),
           "and Shift+Right across the bits, a block of them");

    // Enter and Tab, round the range
    press(_window, ImGuiKey_Enter);
    expect( (at(rxen, CURSOR_NAME)) &&
            (_window.state().ui.selection.size() == 3),
           "Enter moves down inside the range, which stays");
    press(_window, ImGuiKey_Enter);
    press(_window, ImGuiKey_Enter);
    expect(at(txen, cols[1]), "and on to the next column at its foot");

    for (int i = 0; i < 10; ++i)
    {
        press(_window, ImGuiKey_Tab);
    }

    expect(at(loopback, cols[3]), "Tab, along the rows, to the range's end");
    press(_window, ImGuiKey_Tab);
    expect(at(txen, CURSOR_NAME), "and round to its first cell");

    // Ctrl+.: the next corner, clockwise
    press(_window, ImGuiMod_Ctrl | ImGuiKey_Period);
    expect( (at(txen, cols[3])) &&
            (_window.state().ui.grid_rect_rows.size() == 3),
           "Ctrl+. moves the cursor to the range's next corner");

    // Shift+Backspace: the cursor's cell alone
    press(_window, ImGuiMod_Shift | ImGuiKey_Backspace);
    expect( (at(txen, cols[3])) &&
            (_window.state().ui.selection.size() == 1) &&
            (_window.state().ui.grid_rect_rows.empty()),
           "Shift+Backspace leaves the cursor's cell alone selected");

    // whole rows, whole columns, everything
    press(_window, ImGuiMod_Shift | ImGuiKey_Space);
    expect( (_window.state().ui.grid_rect_low == 0) &&
            (_window.state().ui.grid_rect_high == 31) &&
            (_window.state().ui.selection.size() == 1),
           "Shift+Space selects the row, every bit of it");
    press(_window, ImGuiMod_Ctrl | ImGuiKey_Space);

    std::size_t shown = 0;

    for (const bitwisedom::resolved_row* each :
         bitwisedom::gui::shown_rows(_window.state()))
    {
        shown += each->visible ? 1 : 0;
    }

    expect(_window.state().ui.selection.size() == shown,
           "Ctrl+Space, the column, every row of it");
    press(_window, ImGuiMod_Shift | ImGuiKey_Backspace);
    press(_window, ImGuiMod_Ctrl | ImGuiKey_A);
    expect( (_window.state().ui.selection.size() == shown) &&
            (_window.state().ui.grid_rect_low == 0) &&
            (_window.state().ui.grid_rect_high == 31),
           "Ctrl+A, everything");

    // a run of Tabs, and Enter back to where it began
    put(txen, CURSOR_NAME);
    press(_window, ImGuiKey_Tab);
    press(_window, ImGuiKey_Tab);
    expect(at(txen, cols[2]), "Tab moves right");
    press(_window, ImGuiKey_Enter);
    expect(at(rxen, CURSOR_NAME),
           "and Enter goes down to the column the Tabs began in");
    press(_window, ImGuiMod_Shift | ImGuiKey_Enter);
    expect(at(txen, CURSOR_NAME), "Shift+Enter, up");

    // F8: Extend mode
    press(_window, ImGuiKey_F8);
    expect(_window.state().ui.extend_mode, "F8 is Extend mode");
    press(_window, ImGuiKey_DownArrow);
    expect( (at(txen, CURSOR_NAME)) &&
            (_window.state().ui.selection.size() == 2),
           "in which an arrow extends the range");
    press(_window, ImGuiKey_Escape);
    expect(!_window.state().ui.extend_mode, "Esc ends it");

    // F2 renames in place; Space flips the cursor's bit
    put(txen, CURSOR_NAME);
    press(_window, ImGuiKey_F2);
    expect( (_window.state().ui.renaming == txen) &&
            (_window.state().ui.rename_where == "grid"),
           "F2 renames the cursor's row, in place");
    frames(_window, 3);
    key(_window, ImGuiKey_Escape);
    frames(_window, 2);

    int probe = 0;

    for (const bitwisedom::special_row& each :
         _window.state().doc.grid.specials)
    {
        probe = (each.kind == bitwisedom::special_kind::probe) ? each.id
                                                               : probe;
    }

    const bool was = _window.state().doc.probe.test(4);

    post(_window, "set", true, std::string("grid.show_probe"));
    frames(_window, 3);
    put(bitwisedom::gui::special_cursor(probe), 4);
    expect(grid_keyboard(), "the grid has the keyboard back");
    press(_window, ImGuiKey_Space);
    expect( (_window.state().doc.probe.test(4) != was) &&
            (at(bitwisedom::gui::special_cursor(probe), 4)) &&
            (_window.state().ui.selection.empty()),
           "Space flips the probe's bit under the cursor");

    // from the probe's row, a range over the sheet's rows keeps the cursor
    // on the probe
    press(_window, ImGuiMod_Shift | ImGuiKey_UpArrow);
    press(_window, ImGuiMod_Shift | ImGuiKey_UpArrow);
    expect( (at(bitwisedom::gui::special_cursor(probe), 4)) &&
            (_window.state().ui.selection.size() == 2),
           "a range from the probe's row selects the rows, the cursor kept");
    press(_window, ImGuiMod_Shift | ImGuiKey_UpArrow);
    expect( (at(bitwisedom::gui::special_cursor(probe), 4)) &&
            (_window.state().ui.selection.size() == 3),
           "and grows from there");

    // Page Down, Ctrl+End and Ctrl+Home, scrolled with the cursor
    post(_window, "file.example", example_index("win32_hresults"));
    frames(_window, 4);
    post(_window, "doc.activate", static_cast<long>(_window.state().id));
    frames(_window, 4);
    press(_window, ImGuiMod_Ctrl | ImGuiKey_Home);
    frames(_window, 3);

    const int first = _window.state().ui.cursor_row;

    press(_window, ImGuiKey_PageDown);
    frames(_window, 3);

    const int paged = _window.state().ui.cursor_row;
    const bitwisedom::gui::grid_point after =
        grid_spot("name", paged, 0);

    expect( (paged != first) &&
            (after.found) &&
            (after.y > 0.0f) &&
            (after.y < ImGui::GetIO().DisplaySize.y),
           "Page Down moves a screen of rows, the cursor's still in view");

    press(_window, ImGuiMod_Ctrl | ImGuiKey_End);
    frames(_window, 3);

    const int last = _window.state().ui.cursor_row;
    const bitwisedom::gui::grid_point foot =
        grid_spot((last > 0) ? "name" : "special", (last > 0)
                      ? last
                      : bitwisedom::gui::cursor_special(last), 0);

    expect( (foot.found) &&
            (foot.y > 0.0f) &&
            (foot.y < ImGui::GetIO().DisplaySize.y),
           "Ctrl+End scrolls to the last of 1,424 rows");

    press(_window, ImGuiMod_Ctrl | ImGuiKey_Home);
    frames(_window, 3);
    expect( (_window.state().ui.cursor_row == first) &&
            (grid_spot((first > 0) ? "name" : "special",
                       (first > 0) ? first
                                   : bitwisedom::gui::cursor_special(first),
                       0).y < ImGui::GetIO().DisplaySize.y * 0.5f),
           "and Ctrl+Home back to the top");

    // Ctrl+Page Up and Down: sheet to sheet, in the tab row
    post(_window, "file.new", 0L);
    frames(_window, 3);

    const int newer = _window.state().id;

    post(_window, "doc.activate", static_cast<long>(newer));
    frames(_window, 4);
    press(_window, ImGuiMod_Ctrl | ImGuiKey_PageUp);
    frames(_window, 4);

    const int before = _window.state().id;

    expect(before != newer, "Ctrl+Page Up goes to the sheet before");
    post(_window, "doc.activate", static_cast<long>(before));
    frames(_window, 4);
    press(_window, ImGuiMod_Ctrl | ImGuiKey_PageDown);
    frames(_window, 4);
    expect(_window.state().id == newer, "and Ctrl+Page Down, back");

    post(_window, "workspace.close", 0L);
    frames(_window, 3);
    expect(clashes == 0, "the keyboard has no ID clash");
    expect(errors == 0, "and ImGui reports no error");

    return;
}

/*
definition_files
  A definition file a sheet carries, in a tab of its own, brought to the
front: what the preprocessor cannot read in it is named by its line; Edit
makes the view an editor, where what is typed is the file's -- a step to
undo -- and its notes follow; Done makes it a view again.
*/
void
definition_files(
    app& _window
)
{
    const std::string path = "ui_definitions.bitwisedom.json";
    bitwisedom::sheet made = bitwisedom::make_example("uart_cr");
    std::string       error;

    made.definitions.push_back(
        bitwisedom::definition_source{ "flags.h",
                                       "#define GOOD 1\n#define BAD(\n",
                                       false });

    expect(bitwisedom::write_file(path, bitwisedom::save_sheet(made), error),
           "a sheet with a definition file, written");

    post(_window, "file.open", path);
    frames(_window, 3);
    post(_window, "definitions.show", 0L);
    frames(_window, 4);

    const int         id  = _window.state().id;
    const std::string tab = "###" + bitwisedom::gui::document_key(id) +
                            ".def0";

    expect( (window_shown(tab.c_str())) &&
            (_window.state().ui.definition == 0),
           "a definition file in a tab of its own, brought to the front");

    // notes
    //   how many notes the file's lines have now.
    auto notes = [&_window]()
    {
        const std::vector<std::vector<std::string>>& every =
            _window.state().res.definition_notes;

        return every.empty() ? std::size_t(0) : every.front().size();
    };

    expect( (notes() == 1) &&
            (_window.state().res.definition_notes.front().front().compare(
                 0, 7, "line 2:") == 0),
           "the line the preprocessor cannot read is named");

    post(_window, "definitions.edit", 0L);
    frames(_window, 3);
    expect(_window.state().ui.definition_editing == 0, "Edit: an editor");

    {
        uxoxo::event typed;

        typed.kind = "definitions.text";
        typed.payload.set("value", 0L);
        typed.payload.set("text", std::string("#define GOOD 1\n"));
        _window.post(typed);
        frames(_window, 3);
    }

    expect( (notes() == 0) &&
            (_window.state().doc.definitions.front().text ==
             "#define GOOD 1\n") &&
            (_window.state().ui.dirty),
           "what is typed is the file's, and its notes follow");

    post(_window, "edit.undo", 0L);
    frames(_window, 3);
    expect(notes() == 1, "a step to undo");

    post(_window, "definitions.edit", 0L);
    frames(_window, 2);
    expect(_window.state().ui.definition_editing == -1, "Done: a view");

    std::remove(path.c_str());
    expect(clashes == 0, "the definition files have no ID clash");

    return;
}

// above
//   whether the grid drew one row above another.
bool
above(
    int _upper,
    int _lower
)
{
    const bitwisedom::gui::grid_point a =
        bitwisedom::gui::grid_spot("name", _upper, 0);
    const bitwisedom::gui::grid_point b =
        bitwisedom::gui::grid_spot("name", _lower, 0);

    return ( (a.found) &&
             (b.found) &&
             (a.y < b.y) );
}

// show_in
//   a part of what a region of the grid shows, set as its menu sets it.
void
show_in(
    app&               _window,
    const std::string& _rows,
    long               _lsb,
    long               _msb,
    long               _part,
    long               _value,
    bool               _only = false
)
{
    uxoxo::event made;

    made.kind = "display.set";
    made.payload.set("rows", _rows);
    made.payload.set("lsb", _lsb);
    made.payload.set("msb", _msb);
    made.payload.set("part", _part);
    made.payload.set("value", _value);
    made.payload.set("only", _only);
    _window.post(made);

    return;
}

/*
sorted_and_shown
  The grid's rows sorted, as its menus sort them -- the heading's menu
opened by a right-click there -- by name, then the last first; the order
made the sheet's own, and each a step to undo. And what parts of the grid
show: digits in a group's rows, from the row's menu; no colour in a
segment's bits, from the band's, its set bits drawn plain; colours nowhere;
digits in a block selected and nowhere else; every rule in the Display
window, removed there, and every one a step to undo.
*/
void
sorted_and_shown(
    app& _window
)
{
    using bitwisedom::gui::grid_spot;
    using bitwisedom::gui::grid_written;
    using bitwisedom::sort_key;

    post(_window, "file.example", 0L);   // uart_cr
    frames(_window, 4);

    const int en       = id_of(_window, "UART_CR_EN");
    const int txen     = id_of(_window, "UART_CR_TXEN");
    const int rxen     = id_of(_window, "UART_CR_RXEN");
    const int loopback = id_of(_window, "UART_CR_LOOPBACK");
    const int even     = id_of(_window, "UART_CR_PARITY_EVEN");
    const int enables  = _window.state().res.find(en)->parent;
    const int baud     = _window.state().res.find(
                             id_of(_window, "UART_CR_DIV"))->parent;

    expect( (above(en, txen)) &&
            (above(enables, baud)),
           "at first, the sheet's own order");

    // the heading's menu
    expect(move_to(_window, grid_spot("heading", 0, 0)),
           "the grid draws the symbols column's heading");
    click(_window, ImGuiMouseButton_Right);
    expect( (popups() == 1) &&
            (_window.state().ui.menu_kind == "columns"),
           "a right-click there opens the menu that sorts the rows");
    key(_window, ImGuiKey_Escape);

    // by name; then the last first
    post(_window, "set", static_cast<long>(sort_key::name),
         std::string("grid.sort_by"));
    frames(_window, 3);
    expect( (above(en, loopback)) &&
            (above(loopback, rxen)) &&
            (above(rxen, txen)) &&
            (above(baud, enables)),
           "sorted by name: the groups, and each group's rows");

    post(_window, "set", true, std::string("grid.sort_descending"));
    frames(_window, 3);
    expect( (above(txen, rxen)) &&
            (above(loopback, en)) &&
            (above(enables, baud)),
           "and the last first");
    expect(order_of(_window, en) < order_of(_window, txen),
           "the sheet's own order is untouched");

    // made the sheet's own
    post(_window, "grid.sort_keep", 0L);
    frames(_window, 3);
    expect( (_window.state().doc.grid.sort_by == sort_key::sheet) &&
            (order_of(_window, txen) < order_of(_window, en)) &&
            (above(txen, en)),
           "the order made the sheet's own: as it was shown");

    post(_window, "edit.undo", 0L);
    frames(_window, 2);
    expect( (_window.state().doc.grid.sort_by == sort_key::name) &&
            (order_of(_window, en) < order_of(_window, txen)),
           "a step to undo");

    post(_window, "edit.undo", 0L);
    post(_window, "edit.undo", 0L);
    frames(_window, 3);
    expect( (_window.state().doc.grid.sort_by == sort_key::sheet) &&
            (!_window.state().doc.grid.sort_descending) &&
            (above(en, txen)),
           "and so is each sort");

    // what the grid shows: digits nowhere, colours everywhere, at first
    const int digests = grid_written(en).digits;

    expect( (grid_written(en).found) &&
            (grid_written(en).coloured == 1) &&
            (grid_written(even).coloured == 1),
           "at first, no digits, and each set bit in its row's colour");

    // a row's menu
    expect(move_to(_window, grid_spot("name", en, 0)), "EN is drawn");
    click(_window, ImGuiMouseButton_Right);
    expect( (popups() == 1) &&
            (_window.state().ui.menu_kind == "row"),
           "a right-click on a row opens its menu");
    key(_window, ImGuiKey_Escape);

    // digits in the Enables group's rows
    show_in(_window, std::to_string(enables), 0L, -1L, 0L, 1L);
    frames(_window, 2);
    expect( (grid_written(en).digits > digests + 20) &&
            (grid_written(loopback).digits > digests + 20) &&
            (grid_written(even).digits == digests),
           "digits in a group's rows, and nowhere else");

    // no colour in the format segment's bits: a band's menu
    expect(move_to(_window, grid_spot("band", 0, 5)), "its band is drawn");
    click(_window, ImGuiMouseButton_Right);
    expect( (popups() == 1) &&
            (_window.state().ui.menu_kind == "segment"),
           "a right-click on a band opens its menu");
    key(_window, ImGuiKey_Escape);

    show_in(_window, "", 4L, 11L, 2L, 0L);
    frames(_window, 2);
    expect( (grid_written(even).coloured == 0) &&
            (grid_written(even).plain == 1) &&
            (grid_written(en).coloured == 1),
           "no colour in a run of bits: a set bit there drawn plain");

    // colours nowhere; and back
    post(_window, "set", false, std::string("grid.show_colours"));
    frames(_window, 2);
    expect( (grid_written(en).coloured == 0) &&
            (grid_written(en).plain == 1),
           "colours nowhere");
    post(_window, "edit.undo", 0L);
    frames(_window, 2);
    expect(grid_written(en).coloured == 1, "and back");

    // digits only in a block selected
    uxoxo::event block;

    block.kind = "grid.rectangle";
    block.payload.set("target", static_cast<long>(en));
    block.payload.set("last", static_cast<long>(rxen));
    block.payload.set("low", 0L);
    block.payload.set("high", 1L);
    _window.post(block);
    frames(_window, 2);

    const std::vector<int> rows = _window.state().ui.grid_rect_rows;
    std::string            ids;

    for (int each : rows)
    {
        ids += (ids.empty() ? "" : ",") + std::to_string(each);
    }

    expect(rows.size() == 3, "a block of three rows selected");

    show_in(_window, ids, 0L, 1L, 0L, 1L, true);
    frames(_window, 2);
    expect( (grid_written(en).digits == digests + 2) &&
            (grid_written(rxen).digits == digests + 2) &&
            (grid_written(loopback).digits == digests),
           "digits in the block, and nowhere else");

    // every rule in the Display window
    post(_window, "tool.open",
         static_cast<long>(bitwisedom::gui::tool::display));
    frames(_window, 3);
    expect( (tool_in_view(_window, bitwisedom::gui::tool::display)) &&
            (_window.state().doc.grid.rules.size() == 2),
           "the Display window shows the two rules");

    post(_window, "display.remove", 0L);
    frames(_window, 2);
    expect( (_window.state().doc.grid.rules.size() == 1) &&
            (grid_written(even).coloured == 1),
           "a rule removed there");

    post(_window, "display.clear", 0L);
    frames(_window, 2);
    expect(_window.state().doc.grid.rules.empty(), "and every rule");

    for (int i = 0; i < 5; ++i)
    {
        post(_window, "edit.undo", 0L);
    }

    frames(_window, 2);
    expect( (_window.state().doc.grid.rules.empty()) &&
            (grid_written(en).digits == digests) &&
            (grid_written(even).coloured == 1),
           "each a step to undo");

    post(_window, "tool.close",
         static_cast<long>(bitwisedom::gui::tool::display));
    frames(_window, 2);

    expect(clashes == 0, "sorting and showing have no ID clash");

    return;
}

// selected_row
//   the first row selected, in the sheet; nullptr for none.
const bitwisedom::row*
selected_row(
    app& _window
)
{
    const bitwisedom::gui::model& state = _window.state();

    return state.ui.selection.empty()
               ? nullptr
               : bitwisedom::find_row(state.doc, state.ui.selection.front());
}

// added_where
//   whether the row selected -- the one just added -- is of a kind, in a
// group.
bool
added_where(
    app&                 _window,
    bitwisedom::def_kind _kind,
    int                  _group
)
{
    const bitwisedom::row* made = selected_row(_window);

    if (made == nullptr)
    {
        return false;
    }

    const bitwisedom::resolved_row* resolved =
        _window.state().res.find(made->id);

    return ( (resolved != nullptr)                        &&
             (made->kind == bitwisedom::row_kind::symbol) &&
             (made->def == _kind)                         &&
             (resolved->parent == _group) );
}

// set_colour
//   the colour every new row takes, as the add control's swatch sets it.
void
set_colour(
    app&             _window,
    bitwisedom::rgba _colour
)
{
    uxoxo::event made;

    made.kind = "set";
    made.payload.set("target", std::string("environment.new_rgba"));
    made.payload.set("r", static_cast<double>(bitwisedom::channel(_colour,
                                                                  0)));
    made.payload.set("g", static_cast<double>(bitwisedom::channel(_colour,
                                                                  1)));
    made.payload.set("b", static_cast<double>(bitwisedom::channel(_colour,
                                                                  2)));
    made.payload.set("a", static_cast<double>(bitwisedom::channel(_colour,
                                                                  3)));
    _window.post(made);

    return;
}

/*
the_add_control
  The one way to add a row, as the user prefers it: "+ Add", whose menu
adds the kind chosen into the group selected; or a split button, whose
face adds the kind it shows and whose arrow opens the menu -- where the
kind chosen is added, and is what the face adds from then on. And the
colour a new row takes: none at first, the palette's, or one colour for
every new row alike, until the user says otherwise.
*/
void
the_add_control(
    app& _window
)
{
    using bitwisedom::def_kind;
    using bitwisedom::gui::add_style;
    using bitwisedom::gui::control_spot;
    using bitwisedom::gui::new_colour;

    const bitwisedom::gui::shell_state& shell = _window.shell();
    const bitwisedom::rgba              was   = shell.prefs.colour_rgba;

    post(_window, "file.example", 0L);   // uart_cr
    frames(_window, 4);

    const int enables = _window.state().res.find(
                            id_of(_window, "UART_CR_EN"))->parent;
    const int had     = _window.state().res.find(enables)->children;

    // a button, and its menu
    post(_window, "select", static_cast<long>(enables));
    frames(_window, 3);
    expect( (shell.prefs.add == add_style::menu) &&
            (shell.prefs.colour == new_colour::none),
           "at first, a button and its menu; and new rows have no colour");
    expect(move_to(_window, control_spot("inspector.add")),
           "the inspector draws the add control, for a group selected");
    click(_window);
    expect(popups() == 1, "\"+ Add\" opens the menu of what to add");
    expect(move_to(_window, control_spot("inspector.add.kind.1")),
           "which lists a range");
    click(_window);
    frames(_window, 2);
    expect( (popups() == 0) &&
            (added_where(_window, def_kind::range, enables)) &&
            (selected_row(_window)->transparent),
           "chosen, a range is added into the group, with no colour");

    // a split button: its face, then its arrow
    post(_window, "set", 1L, std::string("environment.add"));
    post(_window, "select", static_cast<long>(enables));
    frames(_window, 3);
    expect( (shell.prefs.add == add_style::split) &&
            (control_spot("inspector.add.more").found),
           "a split button, from the Options");
    expect(move_to(_window, control_spot("inspector.add")),
           "its face is drawn");
    click(_window);
    frames(_window, 2);
    expect( (popups() == 0) &&
            (added_where(_window, def_kind::bit, enables)),
           "its face adds the kind it shows: a bit");

    post(_window, "select", static_cast<long>(enables));
    frames(_window, 3);
    expect(move_to(_window, control_spot("inspector.add.more")),
           "its arrow is drawn");
    click(_window);
    expect(popups() == 1, "the arrow opens the menu");
    expect(move_to(_window, control_spot("inspector.add.kind.3")),
           "which lists a literal");
    click(_window);
    frames(_window, 2);
    expect( (added_where(_window, def_kind::literal, enables)) &&
            (shell.prefs.add_kind == 3),
           "chosen, a literal is added, and is the face's kind now");

    post(_window, "select", static_cast<long>(enables));
    frames(_window, 3);
    expect(move_to(_window, control_spot("inspector.add")),
           "the face, again");
    click(_window);
    frames(_window, 2);
    expect(added_where(_window, def_kind::literal, enables),
           "which adds a literal from then on");

    // the colour new rows take: the palette's, from the menu's own list,
    // beside the swatch of the one colour; then that one, for every row
    post(_window, "select", static_cast<long>(enables));
    frames(_window, 3);
    expect(move_to(_window, control_spot("inspector.add.more")),
           "the arrow, again");
    click(_window);
    expect(move_to(_window, control_spot("inspector.add.colours")),
           "the menu lists the colour new rows take");
    frames(_window, 2);
    expect( (popups() == 2) &&
            (control_spot("inspector.add.rgba").found) &&
            (move_to(_window, control_spot("inspector.add.colour.1"))),
           "which opens on the ways, and the swatch of the one colour");
    click(_window);
    frames(_window, 2);
    expect( (popups() == 0) &&
            (shell.prefs.colour == new_colour::automatic),
           "the palette's, chosen there");

    post(_window, "select", static_cast<long>(enables));
    post(_window, "edit.add", 0L);
    frames(_window, 2);
    expect( (added_where(_window, def_kind::bit, enables)) &&
            (!selected_row(_window)->transparent) &&
            (selected_row(_window)->colour == 0u),
           "new rows' colour the palette's: each its own");

    post(_window, "set", 2L, std::string("environment.new_colour"));
    set_colour(_window, 0xE04040FFu);
    frames(_window, 2);

    bool alike = true;

    for (int i = 0; i < 2; ++i)
    {
        post(_window, "select", static_cast<long>(enables));
        post(_window, "edit.add", 0L);
        frames(_window, 2);

        alike = ( (alike) &&
                  (added_where(_window, def_kind::bit, enables)) &&
                  (!selected_row(_window)->transparent) &&
                  (selected_row(_window)->colour == 0xE04040FFu) );
    }

    expect(alike, "one colour: every new row takes it, until told otherwise");

    // every row added undone; the window's settings as they were
    for (int i = 0; i < 7; ++i)
    {
        post(_window, "edit.undo", 0L);
    }

    post(_window, "set", 0L, std::string("environment.add"));
    post(_window, "set", 0L, std::string("environment.add_kind"));
    post(_window, "set", 0L, std::string("environment.new_colour"));
    set_colour(_window, was);
    frames(_window, 3);
    expect( (_window.state().res.find(enables)->children == had) &&
            (shell.prefs.add == add_style::menu) &&
            (shell.prefs.add_kind == 0) &&
            (shell.prefs.colour == new_colour::none) &&
            (shell.prefs.colour_rgba == was),
           "every row added undoes, and the settings are as they were");

    expect(clashes == 0, "the add control has no ID clash");

    return;
}

void
readable_text(
    app& _window
)
{
    using bitwisedom::gui::composite;
    using bitwisedom::gui::contrast;
    using bitwisedom::gui::ink_on;
    using bitwisedom::gui::legible;
    using bitwisedom::gui::to_u32;

    (void)_window;

    const ImU32 black = IM_COL32(0, 0, 0, 255);
    const ImU32 white = IM_COL32(255, 255, 255, 255);

    expect(contrast(black, white) > 20.9f, "black on white is 21:1");
    expect(ink_on(white, black) == black, "a theme's text stays where it "
                                         "reads");
    expect(ink_on(black, IM_COL32(26, 28, 33, 255)) == white,
           "dark text on black turns white");

    // every theme and scheme: the ink on any fill, at any alpha a cell or
    // a label is drawn with, reads; and a row's colour as text reads
    const bitwisedom::rgba colours[] = {
        0x000000FFu, 0xFFFFFFFFu, 0xFFE000FFu, 0x101850FFu, 0xFF4D4DFFu,
        0x2EC4B6FFu, 0x808080FFu, 0x7A7A7AFFu, 0xF2A541FFu, 0x4C8DFFFFu };
    const float alphas[] = { 0.92f, 0.85f, 0.45f, 0.30f, 1.0f };
    bool        inked    = true;
    bool        coloured = true;

    for (int theme = 0; theme < bitwisedom::THEME_COUNT; ++theme)
    {
        for (int scheme = 0; scheme < bitwisedom::SCHEME_COUNT; ++scheme)
        {
            const bitwisedom::scheme_colours surfaces =
                bitwisedom::scheme_for(theme, scheme);
            const ImU32 panel = to_u32(surfaces.panel);
            const ImU32 text  = to_u32(surfaces.text);

            for (bitwisedom::rgba each : colours)
            {
                for (float alpha : alphas)
                {
                    const ImU32 ground = composite(
                        to_u32(bitwisedom::with_alpha(each, alpha)), panel);

                    inked = ( (inked) &&
                              (contrast(ink_on(ground, text), ground) >=
                                   4.5f) );
                }

                coloured = ( (coloured) &&
                             (contrast(legible(each, panel, text), panel) >=
                                  3.0f) );
            }

            inked = ( (inked) &&
                      (ink_on(panel, text) == (text | IM_COL32_A_MASK)) );
            coloured = ( (coloured) &&
                         (legible(0u, panel, text) ==
                          (text | IM_COL32_A_MASK)) );
        }
    }

    expect(inked, "text on any cell of any colour reads at 4.5:1, and on "
                  "a panel is the theme's own");
    expect(coloured, "a row's colour as text reads at 3:1, and no fill is "
                     "the theme's text");

    return;
}

// no_errors
//   the last section: no frame of any section had an ImGui error, or gave
// the GPU a stray vertex.
void
no_errors(
    app& _window
)
{
    (void)_window;

    expect(errors == 0, "no frame of any section had an ImGui error");
    expect(strays == 0, "no frame of any section gave the GPU a stray vertex");

    return;
}

// section
//   struct: one entry of the table.
struct section
{
    const char* name;
    void      (*run)(app&);
};

const section SECTIONS[] = {
    { "the first frames",             first_frames },
    { "every tab of every example",   every_tab },
    { "every row in the inspector",   every_row },
    { "every setting, then undone",   every_setting },
    { "an operation, then undone",    an_operation },
    { "a file saved and opened",      a_file },
    { "a wandering mouse",            a_wandering_mouse },
    { "the header follows the sheet", the_header },
    { "the context menus",            the_context_menus },
    { "a rename in the grid",         a_rename_in_the_grid },
    { "a rename in the tree",         a_rename_in_the_tree },
    { "a segment renamed",            a_segment_renamed },
    { "columns resized",              columns_resized },
    { "code in and out",              code_in_and_out },
    { "the grid fills its width",     the_grid_fills },
    { "the tool windows",             the_tool_windows },
    { "the theme and the scheme",     the_theme_and_scheme },
    { "mouseover info",               mouseover_info },
    { "a sheet wider than 64 bits",   a_wide_sheet },
    { "a thousand rows",              a_thousand_rows },
    { "every group at once",          every_group_at_once },
    { "JSON out and in",              json_out_and_in },
    { "a new sheet",                  a_new_sheet },
    { "children added in the tree",   children_added },
    { "square cells",                 square_cells },
    { "the menu bar and Help",        the_menu_bar },
    { "the symbols column's buttons", symbols_by },
    { "values written every way",     values_every_way },
    { "the grid as the tree",         the_grid_as_the_tree },
    { "the add control",              the_add_control },
    { "the rows sorted, and shown",   sorted_and_shown },
    { "the probe and usage rows",     special_rows },
    { "the Add node window",          the_add_node },
    { "the status bar and console",   the_status_bar },
    { "definition files, edited",     definition_files },
    { "several sheets",               several_sheets },
    { "the workspaces",               the_workspaces },
    { "the sheets arranged",          arrangements },
    { "zoom, as a browser's",         zoom_like_a_browser },
    { "the next run, where it was",   the_next_run },
    { "the Options and settings files", the_options },
    { "the tabs over the bits",       tabs_over_the_bits },
    { "the tabs' menu",               the_tabs_menu },
    { "the keyboard, as Excel's",     the_keyboard },
    { "readable text",                readable_text },
    { "no ImGui error or stray vertex", no_errors }
};

}  // anonymous namespace


int
main()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplNull_Init();
    own_clipboard();
    app::load_fonts(16.0f);

    int passed = 0;
    int failed = 0;

    {
        app window;

        for (const section& each : SECTIONS)
        {
            failure = nullptr;
            each.run(window);

            if (failure == nullptr)
            {
                ++passed;
                std::printf("  %-32s PASS\n", each.name);
            }
            else
            {
                ++failed;
                std::printf("  %-32s FAIL: %s\n", each.name, failure);
            }
        }
    }

    ImGui_ImplNull_Shutdown();
    ImGui::DestroyContext();

    std::printf("frames with an ID clash: %d\n", clashes);
    std::printf("frames with an ImGui error: %d\n", errors);
    std::printf("frames with a stray vertex: %d\n", strays);
    std::printf("passed: %d   failed: %d\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}
