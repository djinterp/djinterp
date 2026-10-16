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
* dropped and imported. Every frame is checked for ImGui's own errors too:
* the misuses a release build recovers from and a debug build asserts on.
*   And the window's own choices: the grid fills its width as the sheet
* says; the panel below it docks, minimizes to its tabs, opens over the grid
* while the mouse rests on them, floats, hides and comes back; the theme and
* the colour scheme recolour the window and are kept for the next run; and
* a name, a value, a band and the ruler explain themselves under a resting
* mouse.
*
* path:      /tests/djinterp/tools/bitwisedom/ui_smoke.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
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
#include "app/app.hpp"                  // app
#include "app/elements.hpp"             // grid_spot
#include "app/settings.hpp"             // settings
#include <djinterp/tools/bitwisedom/examples.hpp>  // example_names
#include <djinterp/tools/bitwisedom/persist.hpp>   // read_file
// the attachment
#include "./attachment.inc"


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

    expect(clashes == 0, "the first frames have no ID clash");
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

        for (long tab = 0; tab < 8; ++tab)
        {
            post(_window, "tab.show", tab);
            frames(_window, 3);

            // a page ImGui did not show would have reported the one it did
            expect(_window.state().ui.tab == static_cast<int>(tab),
                   "the page asked for is the page shown");
        }
    }

    expect(clashes == 0, "no tab of any example has an ID clash");
    expect(wide > narrow + 48.0f,
           "the value column fits a 64-bit example's values");

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
            case bitwisedom::gui::setting_kind::flag:
                made.payload.set("value",
                                 each.read(_window.state().doc) == 0.0);
                break;

            case bitwisedom::gui::setting_kind::integer:
            case bitwisedom::gui::setting_kind::choice:
                made.payload.set("value",
                                 static_cast<long>(each.high));
                break;

            case bitwisedom::gui::setting_kind::real:
                made.payload.set("value", (each.low + each.high) * 0.5);
                break;

            case bitwisedom::gui::setting_kind::colour:
                made.payload.set("r", 0.9);
                made.payload.set("g", 0.4);
                made.payload.set("b", 0.2);
                made.payload.set("a", 1.0);
                break;

            case bitwisedom::gui::setting_kind::text:
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

    post(_window, "file.path", path);
    post(_window, "file.open", 0L);
    frames(_window, 1);

    expect(_window.state().doc.title == "posix_mode", "the file opens");
    expect(!_window.state().ui.status_error, "without an error");

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

    post(_window, "select", static_cast<long>(txen));
    frames(_window, 2);
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

// the_bottom_panel
//   the panel below the grid, every way it can be shown.
void
the_bottom_panel(
    app& _window
)
{
    using bitwisedom::gui::panel_mode;

    const bitwisedom::gui::ui_state& ui = _window.state().ui;
    ImGuiIO&                         io = ImGui::GetIO();

    post(_window, "file.example", 0L);
    post(_window, "tab.show", 3L);
    frames(_window, 3);

    // minimized to its tabs; a press on one brings it back, at that page
    post(_window, "panel.mode", 1L);
    frames(_window, 3);
    expect( (ui.panel == panel_mode::minimized) &&
            (ui.tab == 3),
           "minimized, it keeps its page");

    post(_window, "panel.open", 5L);
    frames(_window, 3);
    expect( (ui.panel == panel_mode::docked) &&
            (ui.tab == 5),
           "a tab pressed on its strip docks it at that page");

    // minimized until the mouse rests on its strip, at the window's foot
    post(_window, "panel.mode", 2L);
    frames(_window, 3);
    expect( (ui.panel == panel_mode::peek) &&
            (!ui.peeking) &&
            (!window_shown("peek###peek")),
           "peeking, it starts closed");

    io.AddMousePosEvent(200.0f, io.DisplaySize.y - 20.0f);
    frames(_window, 40);
    expect( (ui.peeking) &&
            (window_shown("peek###peek")),
           "the mouse resting on its strip opens it over the grid");

    // over the page itself it stays; away from both, it closes
    io.AddMousePosEvent(200.0f, io.DisplaySize.y - 120.0f);
    frames(_window, 40);
    expect(ui.peeking, "the mouse on the page keeps it open");

    io.AddMousePosEvent(io.DisplaySize.x * 0.5f, 80.0f);
    frames(_window, 50);
    expect( (!ui.peeking) &&
            (!window_shown("peek###peek")),
           "the mouse away from it closes it");

    // floating in a window of its own; its close button hides it
    post(_window, "panel.mode", 3L);
    frames(_window, 3);
    expect(window_shown("Panel###panel"), "floating, it has its own window");

    post(_window, "panel.close", 0L);
    frames(_window, 3);
    expect( (ui.panel == panel_mode::hidden) &&
            (!window_shown("Panel###panel")),
           "closing its window hides it");

    // Ctrl+J brings it back as it was, and hides it again
    io.AddMousePosEvent(io.DisplaySize.x * 0.5f, 80.0f);
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    key(_window, ImGuiKey_J);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    frames(_window, 2);
    expect(ui.panel == panel_mode::floating,
           "Ctrl+J brings it back as it was");

    post(_window, "panel.toggle", 0L);
    frames(_window, 2);
    expect(ui.panel == panel_mode::hidden, "and hides it again");

    // a page asked for brings it back
    post(_window, "tab.show", 2L);
    frames(_window, 3);
    expect( (ui.panel == panel_mode::docked) &&
            (ui.tab == 2),
           "a page asked for brings a hidden panel back");

    // and the side panels hide and come back
    post(_window, "view.symbols", false);
    post(_window, "view.inspector", false);
    frames(_window, 3);
    expect( (!ui.show_left) &&
            (!ui.show_right),
           "the side panels hide");

    post(_window, "view.symbols", true);
    post(_window, "view.inspector", true);
    frames(_window, 3);

    expect(clashes == 0, "no way of showing it has an ID clash");

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
    const bitwisedom::gui::model& state = _window.state();

    post(_window, "file.example", 0L);
    frames(_window, 2);

    const bool   dirty = state.ui.dirty;
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
    const bitwisedom::gui::model& state = _window.state();

    post(_window, "file.example", example_index("win32_hresults"));
    post(_window, "tab.show", 0L);
    frames(_window, 3);

    std::vector<long> groups;

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
    const bitwisedom::gui::model& state = _window.state();
    ImGuiIO&                      io    = ImGui::GetIO();

    post(_window, "file.example", example_index("win32_hresults"));
    post(_window, "tab.show", 0L);
    frames(_window, 3);

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
    const bitwisedom::gui::model& state = _window.state();

    post(_window, "file.example", 18L);   // pci_config_header: 512 bits
    post(_window, "tab.show", 1L);
    frames(_window, 3);

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
    const bitwisedom::gui::model& state = _window.state();

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

    // dropped, a bitfield is a new sheet, named for its file
    const std::size_t rows = state.res.rows.size();

    post(_window, "file.new", 0L);
    post(_window, "file.drop", std::string("ui_json.bitfield.json"));
    frames(_window, 2);
    expect( (state.doc.title == "uart_cr") &&
            (state.res.rows.size() == rows) &&
            (state.ui.dirty) &&
            (state.ui.path == "ui_json.bitwisedom.json"),
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
    expect( (state.doc.title == "uart_cr") &&
            (state.ui.import_name == "the clipboard"),
           "and pasted, a sheet again");

    std::remove("ui_json.bitfield.json");
    std::remove("ui_json.export.json");

    return;
}

// no_errors
//   the last section: no frame of any section had an ImGui error.
void
no_errors(
    app& _window
)
{
    (void)_window;

    expect(errors == 0, "no frame of any section had an ImGui error");

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
    { "the bottom panel's ways",      the_bottom_panel },
    { "the theme and the scheme",     the_theme_and_scheme },
    { "mouseover info",               mouseover_info },
    { "a sheet wider than 64 bits",   a_wide_sheet },
    { "a thousand rows",              a_thousand_rows },
    { "every group at once",          every_group_at_once },
    { "JSON out and in",              json_out_and_in },
    { "no ImGui error in any frame",  no_errors }
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
    std::printf("passed: %d   failed: %d\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}
